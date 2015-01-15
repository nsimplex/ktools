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
#include "image_operations.hpp"
#include "file_abstraction.hpp"
#include "image_processing.hpp"
#include "atlas.hpp"


// For non-KTEX.
#define DEFAULT_OUTPUT_EXTENSION "png"


using namespace KTech;
using namespace Compat;
using namespace std;


template<typename IntegerType>
const IntegerType KTech::BitOp::Pow2Rounder::Metadata<IntegerType>::max_pow_2 = Pow2Rounder::roundDown<IntegerType>( std::numeric_limits<IntegerType>::max() );


/*
 * Returns whether the output path already includes the extension. If explicitly given, this will always be assumed.
 * If the extension is missing, output_path will end with '.'.
 */
static bool process_paths(const Path& input_path, VirtualPath& output_path) {
	if(output_path.empty() || output_path.isDirectory()) {
		output_path /= input_path.basename();
		output_path.removeExtension();
		return false;
	}
	else {
		/*
		 * Assume we can write to it and that the extension
		 * shouldn't be added.
		 *
		 * It'd be strange to modify a user given path to add a png extension.
		 */
		return true;
	}
}

template<typename PathContainer, typename ImageContainer>
static void read_images(const PathContainer& paths, ImageContainer& imgs) {
	typedef typename PathContainer::const_iterator pc_iter;
	typedef typename ImageContainer::value_type img_t;

	for(pc_iter it = paths.begin(); it != paths.end(); ++it) {
		imgs.push_back( img_t() );
		MAGICK_WRAP( ImOp::read(*it).call(imgs.back()) );
	}
}


template<typename PathContainer>
static void convert_to_KTEX(const PathContainer& input_paths, const string& output_path, const KTEX::File::Header& h) {
	typedef typename PathContainer::const_iterator pc_iter;
	typedef std::vector<Magick::Image> image_container_t;
	typedef image_container_t::iterator image_iterator_t;

	const int verbosity = options::verbosity;

	if(input_paths.size() > 1 && should_resize()) {
		throw Error("Attempt to resize a mipchain.");
	}

	assert( !input_paths.empty() );

	if(verbosity >= 0) {
		cout << "Loading non-TEX from `" << input_paths.front() << "'";

		size_t count = 1;
		for(pc_iter it = ++input_paths.begin(); it != input_paths.end(); ++it, ++count) {
			cout << ", `" << *it << "'";
			if(count > 4 && verbosity < 3) {
				cout << ", [...]";
				break;
			}
		}
		cout << "." << endl;
	}
	image_container_t imgs;
	if(input_paths.size() > 1) {
		imgs.reserve( input_paths.size() );
	}
	read_images( input_paths, imgs );
	assert( input_paths.size() == imgs.size() );

	KTEX::File tex;
	ImOp::ktexCompressor(h, std::min(options::verbosity, 0)).compress( tex, imgs );
	tex.dumpTo(output_path, verbosity);
}

static void convert_from_KTEX(std::istream& in, const string& input_path, const string& output_path) {
	const int verbosity = options::verbosity;
	int load_verbosity = verbosity;
	if(options::info) {
		load_verbosity = -1;
	}

	if(load_verbosity >= 0) {
		std::cout << "Loading KTEX from `" << input_path << "'..." << std::endl;
	}
	KTech::KTEX::File tex;
	tex.load(in, load_verbosity, options::info);

	if(options::info) {
		std::cout << "File: " << input_path << endl;
		tex.print(std::cout, verbosity);
	}
	else {
		static const std::string fmt_string = "%02d";
		const size_t fmt_pos = output_path.find(fmt_string);
		const bool multiple_mipmaps = ( fmt_pos != string::npos );

		std::deque<Magick::Image> imgs;

		ImOp::ktexDecompressor(std::min(options::verbosity, 0), multiple_mipmaps).decompress( tex, imgs );

		if(verbosity >= 0) {
			if(multiple_mipmaps) {
				const size_t fmt_end = fmt_pos + fmt_string.length();
				const size_t mipmap_count = tex.header.getField("mipmap_count");

				const std::string out_head = output_path.substr(0, fmt_pos);
				const std::string out_tail = (fmt_end < output_path.length() ? output_path.substr(fmt_end) : "");
				
				char buf[3];

				snprintf(buf, sizeof(buf), fmt_string.c_str(), 0);
				std::cout << "Converting RGBA bitmap and saving into `" << out_head << buf << out_tail << "'";
				for(int i = 1; i < (int)mipmap_count; i++) {
					sprintf(buf, fmt_string.c_str(), i);
					std::cout << ", `" << out_head << buf << out_tail << "'";
				}
				std::cout << "..." << std::endl;
			}
			else {
				std::cout << "Converting RGBA bitmap and saving into `" << output_path << "'..." << std::endl;
			}
		}
		if(multiple_mipmaps) {
			MAGICK_WRAP( ImOp::writeSequence(imgs).call(output_path) );
		}
		else {
			MAGICK_WRAP( ImOp::write(output_path).call(imgs.front()) );
		}
		if(verbosity >= 0) {
			std::cout << "Saved." << std::endl;
		}
	}
}

///

template<typename Container>
static void synthesize_atlas(const VirtualPath& atlas_path, Container input_paths, KTEX::File::Header h) {
	Atlas A;

	A.setCompressor( ImOp::ktexCompressor(h) );

	typedef typename Container::const_iterator pc_iter;
	typedef std::vector<Magick::Image> image_container_t;
	typedef image_container_t::iterator image_iterator_t;

	const int verbosity = options::verbosity;

	if(verbosity >= 0) {
		cout << "Creating atlas '" << atlas_path << "'";
		if(verbosity >= 1) {
			cout << " from '" << input_paths.front() << "'";
			size_t count = 1;
			for(pc_iter it = ++input_paths.begin(); it != input_paths.end(); ++it) {
				cout << ", `" << *it << "'";
				count++;
				if(count >= 3 && verbosity < 3) {
					cout << ", [...]";
					break;
				}
			}
		}
		cout << "." << endl;
	}
	image_container_t imgs;
	if(input_paths.size() > 1) {
		imgs.reserve( input_paths.size() );
	}

	options::verbosity = std::min(0, verbosity);
	read_images( input_paths, imgs );
	options::verbosity = verbosity;
	assert( input_paths.size() == imgs.size() );

	pc_iter path_it = input_paths.begin();
	for(image_iterator_t img_it = imgs.begin(); img_it != imgs.end(); ++img_it, ++path_it) {
		A.addImage( path_it->basename().replaceExtension("tex", true), *img_it );
	}

	A.dump(atlas_path, verbosity);
}

template<typename Container>
static void analyze_atlas(const VirtualPath& atlas_path, Container input_paths, const VirtualPath& output_dir) {
	(void)input_paths;

	Atlas A;

	A.setDecompressor( ImOp::ktexDecompressor() );

	typedef typename Container::const_iterator pc_iter;
	typedef std::vector<Magick::Image> image_container_t;
	typedef image_container_t::iterator image_iterator_t;

	const int verbosity = options::verbosity;

	if(verbosity >= 0) {
		cout << "Decomposing atlas '" << atlas_path << "'";
		cout << "..." << endl;
	}

	A.load(atlas_path, verbosity);

	//A.analyze();

	if(verbosity >= 1) {
		cout << "Saving atlas images..." << endl;
	}

	A.saveImages( output_dir, true, verbosity );
}

///

template<typename OutputIterator>
static void split(OutputIterator it, const string& str, const std::string& sep) {
	size_t start = 0, end = 0;
	while( (end = str.find(sep, start)) != string::npos ) {
		if(end > start + 1) {
			*it++ = str.substr(start, end - start);
		}
		start = end + 1;
	}
	*it++ = str.substr(start);
}

///

int main(int argc, char* argv[]) {
	typedef list<VirtualPath> pathlist_t;
	typedef pathlist_t::iterator path_iterator;

	try {
		initialize_application(argc, argv);

		std::list<VirtualPath> input_paths;
		VirtualPath potential_output_path;

		KTEX::File::Header configured_header = parse_commandline_options(argc, argv, input_paths, potential_output_path);

		Maybe<VirtualPath> output_path = Just(potential_output_path);

		if(options::atlas_path != nil) {
			if(potential_output_path.hasExtension() && !potential_output_path.hasExtension("tex") && !potential_output_path.isDirectory()) {
				input_paths.push_back( potential_output_path );
				output_path = nil;
			}
		}
		else if(input_paths.empty()){
			input_paths.push_back( potential_output_path );
			output_path = Just(VirtualPath("."));
		}

		if(input_paths.size() == 1) {
			std::string full_path_str = input_paths.front();
			input_paths.clear();
			split( std::back_inserter(input_paths), full_path_str, ",");
		}

		for(pathlist_t::iterator it = input_paths.begin(); it != input_paths.end(); ++it) {
			if(it->isDirectory() || it->isZipArchive()) {
				*it /= "atlas-0.tex";
			}
		}

		bool output_has_extension = true;
		if(options::atlas_path == nil && output_path != nil) {
			output_has_extension = process_paths(input_paths.front(), output_path.ref());
		}

		const bool is_tex_input = input_paths.empty() || input_paths.front().hasExtension("tex");

		if(output_path == nil && is_tex_input) {
			throw KToolsError("No output path for KTEX decompression.");
		}

		if(is_tex_input) {
			if(options::atlas_path == nil) {
				std::istream* in = input_paths.front().open_in(std::ifstream::binary);

				if(!KTech::KTEX::File::isKTEXFile(*in)) {
					throw KToolsError(std::string("Input file '") + input_paths.front() + "' has '.tex' extension, but its binary contents do not match a KTEX file.");
				}

				if(input_paths.size() > 1) {
					throw KToolsError("Multiple input files should only be given on TEX output.");
				}
				if(!output_has_extension) {
					output_path.ref() += ".";
					output_path.ref() += DEFAULT_OUTPUT_EXTENSION;
				}

				convert_from_KTEX(*in, input_paths.front(), output_path.ref());
				delete in;
			}
			else {
				if(!output_path.ref().mkdir()) {
					throw SysError(std::string("failed to create '") + output_path.ref() + "' directory: ");
				}
				analyze_atlas(options::atlas_path.ref(), input_paths, output_path.ref());
			}
		}
		else {
			if(input_paths.empty()) {
				throw Error("empty list of input files provided.");
			}

			if(options::atlas_path == nil) {
				if(!output_has_extension) {
					output_path.ref() += ".tex";
				}

				convert_to_KTEX(input_paths, output_path.ref(), configured_header);
			}
			else {
				synthesize_atlas(options::atlas_path.ref(), input_paths, configured_header);
			}
		}
	}
	catch(std::exception& e) {
		cerr << "Error: " << e.what() << endl;
		exit(-1);
	}

	return 0;
}
