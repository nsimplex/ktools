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


// For non-KTEX.
#define DEFAULT_OUTPUT_EXTENSION "png"


using namespace KTech;
using namespace Compat;
using namespace std;


KTech::Maybe<bool> KTech::Nothing;


template<typename IntegerType>
const IntegerType KTech::BitOp::Pow2Rounder::Metadata<IntegerType>::max_pow_2 = Pow2Rounder::roundDown<IntegerType>( std::numeric_limits<IntegerType>::max() );



static bool should_resize() {
	return options::width != Nothing || options::height != Nothing || options::pow2;
}

static void resize_image(Magick::Image& img) {
	size_t w = img.columns(), h = img.rows();

	if(options::width != Nothing && options::height != Nothing) {
		w = options::width;
		h = options::height;
	}
	else if(options::width != Nothing) {
		w = options::width;
		h = (w*img.rows())/img.columns();
	}
	else if(options::height != Nothing) {
		h = options::height;
		w = (h*img.columns())/img.rows();
	}

	if(options::pow2) {
		w = BitOp::Pow2Rounder::roundUp(w);
		h = BitOp::Pow2Rounder::roundUp(h);
	}

	if(options::verbosity >= 0) {
		std::cout << "Resizing image to " << w << "x" << h << std::endl;
	}

	img.filterType( options::filter );
	img.resize( Magick::Geometry(w, h) );
}

/*
 * Returns whether the output path already includes the extension. If explicitly given, this will always be assumed.
 * If the extension is missing, output_path will end with '.'.
 */
static bool process_paths(const Path& input_path, Path& output_path) {
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
		ImOp::safeWrap( ImOp::read(*it) )( imgs.back() );
	}
}

template<typename ImageContainer>
static void generate_mipmaps(ImageContainer& imgs) {
	typedef typename ImageContainer::value_type img_t;

	if(options::no_mipmaps || imgs.size() > 1) {
		if(options::verbosity >= 1) {
			std::cout << "Skipping mipmap generation..." << std::endl;
		}
		return;
	}

	img_t img = imgs.front();

	size_t width = img.columns();
	size_t height = img.rows();

	if(options::verbosity >= 1) {
		const size_t mipmap_count = BitOp::countBinaryDigits( std::min(width, height) );
		std::cout << "Generating " << mipmap_count << " mipmaps..." << std::endl;
	}

	width /= 2;
	height /= 2;

	while(width > 0 && height > 0) {
		img.filterType( options::filter );
		img.resize( Magick::Geometry(width, height) );
		imgs.push_back( img );

		width /= 2;
		height /= 2;
	}
}

template<typename PathContainer>
static void convert_to_KTEX(const PathContainer& input_paths, const string& output_path, const KTEX::File::Header& h) {
	typedef typename PathContainer::const_iterator pc_iter;

	if(input_paths.size() > 1 && should_resize()) {
		throw Error("Attempt to resize a mipchain.");
	}

	if(options::verbosity >= 0) {
		cout << "Loading non-TEX from `" << input_paths.front() << "'";
		for(pc_iter it = ++input_paths.begin(); it != input_paths.end(); ++it) {
			cout << ", `" << *it << "'";
		}
		cout << "..." << endl;
	}
	std::vector<Magick::Image> imgs;
	if(input_paths.size() > 1) {
		imgs.reserve( input_paths.size() );
	}
	read_images( input_paths, imgs );
	assert( input_paths.size() == imgs.size() );
	if(options::verbosity >= 0) {
		cout << "Finished loading." << endl;
	}

	resize_image( imgs.front() );

	if(!options::no_premultiply) {
		if(options::verbosity >= 1) {
			std::cout << "Premultiplying alpha..." << std::endl;
		}
		std::for_each( imgs.begin(), imgs.end(), ImOp::premultiplyAlpha() );
	}
	else if(options::verbosity >= 1) {
		std::cout << "Skipping alpha premultiplication..." << std::endl;
	}

	generate_mipmaps( imgs );

	KTech::KTEX::File tex;
	tex.header = h;
	tex.CompressFrom(imgs.begin(), imgs.end());
	tex.dumpTo(output_path);
}

static void convert_from_KTEX(const string& input_path, const string& output_path) {
	int old_verbosity = options::verbosity;

	if(options::info) {
		options::verbosity = -1;
	}
	KTech::KTEX::File tex;
	tex.loadFrom(input_path);
	options::verbosity = old_verbosity;

	if(options::info) {
		std::cout << "File: " << input_path << endl;
		tex.print(std::cout);
	}
	else {
		static const std::string fmt_string = "%02d";
		const size_t fmt_pos = output_path.find(fmt_string);
		const bool multiple_mipmaps = ( fmt_pos != string::npos );

		std::list<Magick::Image> imgs;

		if(multiple_mipmaps) {
			if(should_resize()) {
				throw Error("Attempt to resize a mipchain.");
			}
			tex.Decompress( std::back_inserter(imgs) );
		}
		else {
			imgs.push_back( tex.Decompress() );
		}

		resize_image( imgs.front() );

		std::for_each( imgs.begin(), imgs.end(), Magick::qualityImage(options::image_quality) );

		if(options::verbosity >= 0) {
			if(multiple_mipmaps) {
				const size_t fmt_end = fmt_pos + fmt_string.length();
				const size_t mipmap_count = tex.header.getField("mipmap_count");

				const std::string out_head = output_path.substr(0, fmt_pos);
				const std::string out_tail = (fmt_end < output_path.length() ? output_path.substr(fmt_end) : "");
				
				char buf[3];

				sprintf(buf, fmt_string.c_str(), 0);
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
			ImOp::safeWrap( ImOp::writeSequence(imgs) )( output_path );
		}
		else {
			ImOp::safeWrap( ImOp::write(output_path) )( imgs.front() );
		}
		if(options::verbosity >= 0) {
			std::cout << "Saved." << std::endl;
		}
	}
}

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

/*
 * Slurps the input image into a std::string, to get a uniform treatment for both regular files and stdin
 * input (since we need to check for KTEX's magical number before feeding the image to ImageMagick or otherwise).
 */
int main(int argc, char* argv[]) {
	try {
		Magick::InitializeMagick(argv[0]);

		string input_path_str, output_path_str;
		KTEX::File::Header configured_header = parse_commandline_options(argc, argv, input_path_str, output_path_str);

		list< Path > input_paths;
		split( std::back_inserter(input_paths), input_path_str, ",");
		if(input_paths.empty()) {
			cerr << "error: Empty list of input files provided." << endl;
			exit(1);
		}

		Path output_path = output_path_str;

		bool output_has_extension = process_paths(input_paths.front(), output_path);

		//assert( input_path != "-" );

		if(KTech::KTEX::File::isKTEXFile(input_paths.front())) {
			if(input_paths.size() > 1) {
				throw Error("Multiple input files should only be given on TEX output.");
			}
			if(!output_has_extension) {
				output_path += ".";
				output_path += DEFAULT_OUTPUT_EXTENSION;
			}

			convert_from_KTEX(input_paths.front(), output_path);
		}
		else {
			if(!output_has_extension) {
				output_path += ".tex";
			}

			convert_to_KTEX(input_paths, output_path, configured_header);
		}
	}
	catch(std::exception& e) {
		cerr << "error: " << e.what() << endl;
		exit(1);
	}

	return 0;
}
