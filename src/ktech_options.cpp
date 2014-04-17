/*
Copyright (C) 2013  simplex

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "ktech.hpp"
#include "ktech_options.hpp"
#include "ktech_options_output.hpp"
#include <tclap/CmdLine.h>

#include <cctype>


static const std::string license =
"Copyright (C) 2013 simplex.\n\
License GPLv2+: GNU GPL version 2 or later <https://gnu.org/licenses/old-licenses/gpl-2.0.html>.\n\
This is free software: you are free to change and redistribute it.\n\
There is NO WARRANTY, to the extent permitted by law.";


// Message appended to the bottom of the (long) usage statement.
static const std::string usage_message = 
"If input-file is a TEX image, it is converted to a non-TEX format (defaulting\n\
to PNG). Otherwise, it is converted to TEX. The format of input-file is\n\
inferred from its binary contents (its magic number).\n\
\n\
If output-path is not given, then it is taken as input-file stripped from its\n\
directory part and with its extension replaced by `.png' (thus placing it in\n\
the current directory). If output-path is a directory, this same rule applies,\n\
but the resulting file is placed in it instead. If output-path is given and is\n\
not a directory, the format of the output file is inferred from its extension.\n\
\n\
If more than one input file is given (separated by commas), they are assumed\n\
to be a precomputed mipmap chain (this use scenario is mostly relevant for\n\
automated processing). This should only be used for TEX output.\n\
\n\
If output-path contains the string '%02d', then for TEX input all its\n\
mipmaps will be exported in a sequence of images by replacing '%02d' with\n\
the number of the mipmap (counting from zero).";



// Maps a boolean value for requiredness into a bracket pair.
static const char option_brackets[2][2] = { {'[', ']'}, {'<', '>'} };


namespace KTech {
	namespace options {
		int verbosity = 0;

		bool info = false;

		int image_quality = 100;

		Magick::FilterTypes filter = Magick::CatromFilter;

		bool no_premultiply = false;

		bool no_mipmaps = false;

		Maybe<size_t> width;
		Maybe<size_t> height;
		bool pow2;

		bool extend;
	}
}


using namespace KTech;
using namespace TCLAP;
using namespace std;


// Normalizes a string for an option name.
std::string normalize_string(const std::string& s) {
	size_t i;
	const size_t n = s.length();

	std::string ret;

	for(i = 0; i < n && isalnum(s[i]); ++i) {
		ret.push_back( char(tolower(s[i])) );
	}

	for(; i < n && !isalnum(s[i]); ++i);

	for(; i < n && isdigit(s[i]); ++i) {
		ret.push_back(s[i]);
	}

	return ret;
}

template<typename T>
class StrOptTranslator {
public:
	// List of options
	vector<string> opts;

	// Mapping of option name to internal value
	map<string, T> opt_map;

	std::string default_opt;

	void push_opt(const std::string& name, T val) {
		opts.push_back(name);
		opt_map.insert( make_pair(name, val) );
	}

	T translate(const std::string& name) const {
		typename map<string, T>::const_iterator it = opt_map.find(name);
		if(it == opt_map.end()) {
			throw( Error("Invalid option " + name) );
		}
		return it->second;
	}

	std::string inverseTranslate(const T val) const {
		typename map<string, T>::const_iterator it;
		for(it = opt_map.begin(); it != opt_map.end(); ++it) {
			if(it->second == val) {
				return it->first;
			}
		}
		return "";
	}

	T translate(ValueArg<string>& a) const {
		return translate( a.getValue() );
	}
};

class HeaderStrOptTranslator : public StrOptTranslator<KTEX::HeaderFieldSpec::value_t> {
public:
	HeaderStrOptTranslator(const std::string& id) {
		const KTEX::HeaderFieldSpec& spec = KTEX::HeaderSpecs::FieldSpecs[id];

		assert( spec.isValid() );

		typedef KTEX::HeaderFieldSpec::values_map_t::const_iterator iter_t;

		for(iter_t it = spec.values.begin(); it != spec.values.end(); ++it) {
			string name = normalize_string( it->first );

			push_opt(name, it->second);

			if(it->second == spec.value_default) {
				default_opt = name;
			}
		}
	}
};

class FilterTypeTranslator : public StrOptTranslator<Magick::FilterTypes> {
public:
	FilterTypeTranslator() {
		using namespace Magick;

		push_opt("bicubic", CatromFilter);
		push_opt("lanczos", LanczosFilter);
		push_opt("blackman", BlackmanFilter);
		push_opt("hann", HanningFilter);
		push_opt("hamming", HammingFilter);
		push_opt("cubic", CubicFilter);
		//push_opt("quadratic", QuadraticFilter);
		push_opt("box", BoxFilter);

		default_opt = inverseTranslate(options::filter);
	}
};


class ArgumentOption : public UnlabeledValueArg<string> {
public:
	virtual bool processArg(int* i, vector<string>& args) {
		// POSIX-like behaviour.
		if(!Arg::ignoreRest()) {
			const std::string& s = args[*i];
			if(s.length() == 0 || s[0] == TCLAP::Arg::flagStartChar()) {
				return false;
			}
		}

		return UnlabeledValueArg<string>::processArg(i, args);
	}

	virtual std::string shortID(const std::string& val= "") const {
		std::string original = UnlabeledValueArg<string>::shortID(val);

		return std::string(1, option_brackets[isRequired()][0])
			+ original.substr(1, original.length() - 2)
			+ std::string(1, option_brackets[isRequired()][1]);
	}

	ArgumentOption(const string& name, const string& desc, bool req, const string& typeDesc) :
		UnlabeledValueArg<string>(name, desc, req, "", typeDesc) {}
};


template<typename T>
class MyValueArg : public ValueArg<T> {
public:
	MyValueArg( const std::string& flag, 
                    const std::string& name, 
                    const std::string& desc, 
                    bool req, 
                    T value,
                    const std::string& typeDesc) : ValueArg<T>(flag, name, desc, req, value, typeDesc) {}

	MyValueArg( const std::string& flag, 
                    const std::string& name, 
                    const std::string& desc, 
                    bool req, 
                    T value,
                    Constraint<T>* constraint) : ValueArg<T>(flag, name, desc, req, value, constraint) {}
	
	virtual std::string longID(const::std::string& val = "") const;
};

template<typename T>
std::string MyValueArg<T>::longID(const::std::string& val) const {
	using namespace std;
	using namespace TCLAP;
	(void)val;

	const std::string& valueId = this->_typeDesc;

	string id;

	if(this->getFlag().length() > 0) {
		id += Arg::flagStartString() + this->getFlag() + ",  ";
	}

	id += Arg::nameStartString() + this->getName();

	if(this->isValueRequired()) {
		id += string(2, Arg::delimiter()) + std::string(1, option_brackets[1][0]) + valueId + std::string(1, option_brackets[1][1]);
	}

	return id;
}

static const std::string FROM_TEX = "Options for TEX input";
static const std::string TO_TEX = "Options for TEX output";


KTEX::File::Header KTech::parse_commandline_options(int& argc, char**& argv, string& input_path, string& output_path) {
	KTEX::File::Header configured_header;

	try {
		typedef HeaderStrOptTranslator str_trans;

		options::Output myOutput(license);

		CmdLine cmd(usage_message, ' ', PACKAGE_VERSION);
		cmd.setOutput(&myOutput);

		list<Arg*> args;

		{
			list<Arg*>& raw_args = cmd.getArgList();
			raw_args.push_back( raw_args.front() );
			raw_args.pop_front();
		}

		
		myOutput.addCategory(FROM_TEX);
		myOutput.addCategory(TO_TEX);


		str_trans comp_trans("compression");
		ValuesConstraint<string> allowed_comps(comp_trans.opts);
		MyValueArg<string> compression_opt("c", "compression", "Compression type for TEX creation. Defaults to " + comp_trans.default_opt + ".", false, comp_trans.default_opt, &allowed_comps);
		args.push_back(&compression_opt);
		myOutput.setArgCategory(compression_opt, TO_TEX);

		MyValueArg<int> quality_opt("Q", "quality", "Quality used when converting TEX to PNG/JPEG/etc. Higher values result in less compression (and thus a bigger file size). Defaults to 100.", false, 100, "0-100");
		args.push_back(&quality_opt);
		myOutput.setArgCategory(quality_opt, FROM_TEX);

		FilterTypeTranslator filter_trans;
		ValuesConstraint<string> allowed_filters(filter_trans.opts);
		MyValueArg<string> filter_opt("f", "filter", "Resizing filter used for mipmap generation. Defaults to " + filter_trans.default_opt + ".", false, filter_trans.default_opt, &allowed_filters);
		args.push_back(&filter_opt);
		myOutput.setArgCategory(filter_opt, TO_TEX);

		str_trans type_trans("texture_type");
		ValuesConstraint<string> allowed_types(type_trans.opts);
		MyValueArg<string> type_opt("t", "type", "Target texture type. Defaults to " + type_trans.default_opt + ".", false, type_trans.default_opt, &allowed_types);
		args.push_back(&type_opt);
		myOutput.setArgCategory(type_opt, TO_TEX);

		SwitchArg no_premultiply_flag("", "no-premultiply", "Don't premultiply alpha.");
		args.push_back(&no_premultiply_flag);
		myOutput.setArgCategory(no_premultiply_flag, TO_TEX);

		SwitchArg no_mipmaps_flag("", "no-mipmaps", "Don't generate mipmaps.");
		args.push_back(&no_mipmaps_flag);
		myOutput.setArgCategory(no_mipmaps_flag, TO_TEX);

		MyValueArg<int> width_opt("", "width", "Fixed width to be used for the output. Without a height, preserves ratio.", false, -1, "pixels");
		args.push_back(&width_opt);

		MyValueArg<int> height_opt("", "height", "Fixed height to be used for the output. Without a width, preserves ratio.", false, -1, "pixels");
		args.push_back(&height_opt);

		SwitchArg pow2_opt("", "pow2", "Rounds width and height up to a power of 2. Applied after the options `width' and `height', if given.");
		args.push_back(&pow2_opt);

		SwitchArg extend_opt("", "extend", "Extends the boundaries of the image instead of resizing. Only relevant if either of the options `width', `height' or `pow2' are given. Its primary use is generating save and selection screen portraits.");
		args.push_back(&extend_opt);



		/*
		str_trans plat_trans("platform");
		ValuesConstraint<string> allowed_plats(plat_trans.opts);
		MyValueArg<string> platform_opt("p", "platform", "Target platform. Defaults to " + plat_trans.default_opt + ".", false, plat_trans.default_opt, &allowed_plats);
		args.push_back(&platform_opt);
		myOutput.setArgCategory(platform_opt, TO_TEX);
		*/


		SwitchArg info_flag("i", "info", "Prints information for a given TEX file instead of converting it.");
		args.push_back(&info_flag);
		myOutput.setArgCategory(info_flag, FROM_TEX);


		MultiSwitchArg verbosity_flag("v", "verbose", "Increases output verbosity.");
		args.push_back(&verbosity_flag);

		SwitchArg quiet_flag("q", "quiet", "Disables text output. Overrides the verbosity value.");
		args.push_back(&quiet_flag);


		ArgumentOption input_opt("input-file", "Input path.", true, "input-file[,...]");
		cmd.add(input_opt);

		ArgumentOption output_opt("output-path", "Output path.", false, "output-path");
		cmd.add(output_opt);


		for(list<Arg*>::reverse_iterator it = args.rbegin(); it != args.rend(); ++it) {
			cmd.add(*it);
		}


		cmd.parse(argc, argv);


		configured_header.setField("compression", comp_trans.translate(compression_opt));
		configured_header.setField("texture_type", type_trans.translate(type_opt));
		//configured_header.setField("platform", plat_trans.translate(platform_opt));

		options::image_quality = quality_opt.getValue();
		if(options::image_quality < 0) {
			options::image_quality = 0;
		}
		else if(options::image_quality > 100) {
			options::image_quality = 100;
		}

		options::filter = filter_trans.translate(filter_opt.getValue());

		/*
		options::no_premultiply = no_premultiply_flag.getValue();
		*/

		options::no_mipmaps = no_mipmaps_flag.getValue();

		if(width_opt.getValue() > 0) {
			options::width = Just((size_t)width_opt.getValue());
		}

		if(height_opt.getValue() > 0) {
			options::height = Just((size_t)height_opt.getValue());
		}

		options::pow2 = pow2_opt.getValue();

		options::extend = extend_opt.getValue();

		if(quiet_flag.getValue()) {
			options::verbosity = -1;
		}
		else {
			options::verbosity = verbosity_flag.getValue();
		}
		
		options::info = info_flag.getValue();

		input_path = input_opt.getValue();
		output_path = output_opt.getValue();

	} catch (ArgException& e) {
		cerr << "error: " << e.error() << " for arg " << e.argId() << endl;
		exit(1);
	} catch(exception& e) {
		cerr << "error: " << e.what() << endl;
		exit(1);
	}

	return configured_header;
}
