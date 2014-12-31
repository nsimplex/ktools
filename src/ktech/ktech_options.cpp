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
#include "ktools_options_customization.hpp"
#include <tclap/CmdLine.h>

#include <cctype>


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



namespace KTech {
	namespace options {
		int verbosity = 0;

		bool info = false;

		int image_quality = 100;

		Magick::FilterTypes filter = Magick::LanczosFilter;

		bool no_premultiply = false;

		bool no_mipmaps = false;

		Maybe<size_t> width;
		Maybe<size_t> height;
		bool pow2 = false;

		bool force_square = false;
		bool extend = false;
		bool extend_left = false;

		Maybe<VirtualPath> atlas_path;
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
	ret.reserve(n);

	for(i = 0; i < n && isalnum(s[i]); ++i) {
		ret.push_back( char(tolower(s[i])) );
	}

	for(; i < n && !isalnum(s[i]); ++i);

	for(; i < n && isdigit(s[i]); ++i) {
		ret.push_back(s[i]);
	}

	return ret;
}


namespace KTech {
	namespace options_custom {
		using namespace KTools::options_custom;

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

				push_opt("lanczos", LanczosFilter);
				push_opt("mitchell", MitchellFilter);
				push_opt("bicubic", CatromFilter);
				//push_opt("blackman", BlackmanFilter);
				//push_opt("hann", HanningFilter);
				//push_opt("hamming", HammingFilter);
				push_opt("catrom", CatromFilter);
				push_opt("cubic", CubicFilter);
				//push_opt("quadratic", QuadraticFilter);
				push_opt("box", BoxFilter);

				default_opt = inverseTranslate(options::filter);
			}
		};
	}
}


static const std::string FROM_TEX = "Options for TEX input";
static const std::string TO_TEX = "Options for TEX output";


KTEX::File::Header KTech::parse_commandline_options(int& argc, char**& argv, std::list<VirtualPath>& input_paths, VirtualPath& output_path) {
	using namespace KTech::options_custom;

	KTEX::File::Header configured_header;
	configured_header.io.setNativeSource();

	try {
		typedef HeaderStrOptTranslator str_trans;

		options_custom::Output myOutput(license);

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


		MyValueArg<string> atlas_path_opt("", "atlas", "Name of the atlas to be generated.", false, "", "path");
		args.push_back(&atlas_path_opt);
		myOutput.setArgCategory(atlas_path_opt, TO_TEX);

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

		MyValueArg<size_t> width_opt("", "width", "Fixed width to be used for the output. Without a height, preserves ratio.", false, 0, "pixels");
		args.push_back(&width_opt);

		MyValueArg<size_t> height_opt("", "height", "Fixed height to be used for the output. Without a width, preserves ratio.", false, 0, "pixels");
		args.push_back(&height_opt);

		SwitchArg pow2_opt("", "pow2", "Rounds width and height up to a power of 2. Applied after the options `width' and `height', if given.");
		args.push_back(&pow2_opt);

		SwitchArg square_opt("", "square", "Makes the output texture a square, by setting the width and height to their maximum values. Applied after the options `width', `height' and `pow2', if given.");
		args.push_back(&square_opt);

		SwitchArg extend_opt("", "extend", "Extends the boundaries of the image instead of resizing. Only relevant if either of the options `width', `height', `pow2' or `square' are given. Its primary use is generating save and selection screen portraits.");
		args.push_back(&extend_opt);

		SwitchArg extendleft_opt("", "extend-left", "Causes the `extend' option to place the original image aligned to the right (filling the space on its left). Implies `extend'.");
		args.push_back(&extendleft_opt);



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

		///
		
		MultiArgumentOption multiinput_opt("INPUT-PATH", "Input path.", true, "INPUT-PATH");
		cmd.add(multiinput_opt);

		/*
		 * This only marks the option visually, since all paths are globbed by multiinput_opt.
		 */
		ArgumentOption dummy_output_opt("OUTPUT-DIR", "Output path.", false, "OUTPUT-DIR");
		dummy_output_opt.setVisuallyRequired(true);
		cmd.add(dummy_output_opt);


		for(list<Arg*>::reverse_iterator it = args.rbegin(); it != args.rend(); ++it) {
			cmd.add(*it);
		}


		cmd.parse(argc, argv);


		if(atlas_path_opt.isSet()) {
			options::atlas_path = Just( VirtualPath(atlas_path_opt.getValue()) );
		}

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

		if(width_opt.isSet()) {
			options::width = Just((size_t)width_opt.getValue());
		}

		if(height_opt.isSet()) {
			options::height = Just((size_t)height_opt.getValue());
		}

		options::pow2 = pow2_opt.getValue();

		options::force_square = square_opt.getValue();

		options::extend = extend_opt.getValue();

		options::extend_left = extendleft_opt.getValue();
		if(options::extend_left) {
			options::extend = true;
		}

		if(quiet_flag.getValue()) {
			options::verbosity = -1;
		}
		else {
			options::verbosity = verbosity_flag.getValue();
		}
		
		options::info = info_flag.getValue();


		const std::vector<std::string>& all_paths = multiinput_opt.getValue();
		if(all_paths.empty()) {
			throw KToolsError("at least one path expected as argument.");
		}

		std::copy(all_paths.begin(), all_paths.end(), std::back_inserter(input_paths));

		output_path = input_paths.back();
		input_paths.pop_back();
	} catch (ArgException& e) {
		cerr << "error: " << e.error() << " for arg " << e.argId() << endl;
		exit(1);
	} catch(exception& e) {
		cerr << "error: " << e.what() << endl;
		exit(1);
	}

	return configured_header;
}
