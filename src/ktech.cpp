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
#include "ktech_fs.hpp"

// For non-KTEX.
#define DEFAULT_OUTPUT_EXTENSION "png"


using namespace KTech;
using namespace std;

/*
 * Returns whether the output path already includes the extension. If explicitly given, this will always be assumed.
 * If the extension is missing, output_path will end with '.'.
 */
static bool process_paths(const string& input_path, string& output_path) {
	bool has_extension = false;

	if(output_path.length() > 0) {
		if(isDirectory(output_path)) {
			size_t i = output_path.find_last_not_of('/');
			if(i != string::npos) {
				output_path.erase(i + 1);
			}
			output_path += "/";
		}
		else {
			// Assume we can write to it and that the extension
			// shouldn't be added.
			has_extension = true;
		}
	}

	if(!has_extension) {
		// Points to the dot in the extension for the input path.
		ssize_t ext_start;
		
		std::string input_file, input_dir;
		filepathSplit(input_path, input_dir, input_file);

		for(ext_start = input_file.length() - 1; ext_start >= 0 && input_file[ext_start] != '.'; ext_start--);

		if(ext_start < 0) {
			ext_start = input_file.length();
		}

		output_path += input_file.substr(0, ext_start + 1);
	}

	return has_extension;
}

/*
 * Applies an operation to an ImageMagick::Image with error checking.
 *
 * Assumes such an operation takes a single argument and returns nothing.
 */
template<typename T>
static bool InnerDoImageOperation(Magick::Image& img, void (Magick::Image::* f)(T), T arg) {
	using namespace std;

	try {
		(img.*f)(arg);
	}
	catch(Magick::WarningCoder& warning) {
		cerr << "Coder warning: " << warning.what() << endl;
		return false;
	}
	catch(Magick::Warning& warning) {
		cerr << "Warning: " << warning.what() << endl;
		return false;
	}
	catch(Magick::Error& err) {
		cerr << "Error: " << err.what() << endl;
		exit(MagickErrorCode);
	}
	catch(std::exception& err) {
		cerr << "Error: " << err.what() << endl;
		exit(GeneralErrorCode);
	}

	return true;
}

template<typename T>
static bool DoImageOperation(Magick::Image& img, void (Magick::Image::* f)(T), T arg) {
	return InnerDoImageOperation<T>(img, f, arg);
}

template<typename T>
static bool DoImageOperation(Magick::Image& img, void (Magick::Image::* f)(const T&), const T& arg) {
	return InnerDoImageOperation<const T&>(img, f, arg);
}


static Magick::Image read_image(const std::string& path) {
	Magick::Image img;

	if(DoImageOperation(img, &Magick::Image::read, path))
		return img;

	/*
	Magick::Image original_img = img;

	img.colorSpace(Magick::RGBColorspace);

	if(DoImageOperation(img, &Magick::Image::read, path))
		return img;

	img.colorSpace(Magick::sRGBColorspace);

	if(DoImageOperation(img, &Magick::Image::read, path))
		return img;

	img.colorSpace(Magick::scRGBColorspace);

	if(DoImageOperation(img, &Magick::Image::read, path))
		return img;

	return original_img;		
	*/

	return img;
}


static void convert_to_KTEX(const string& input_path, const string& output_path, const KTEX::File::Header& h) {
	if(options::verbosity >= 0) {
		cout << "Loading non-TEX from `" << input_path  << "'..." << endl;
	}
	Magick::Image img = read_image(input_path);
	if(options::verbosity >= 0) {
		cout << "Finished loading." << endl;
	}

	KTech::KTEX::File tex;
	tex.header = h;
	tex.CompressFrom(img);
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
		Magick::Image img = tex.Decompress();
		img.quality(options::image_quality);

		if(options::verbosity >= 0) {
			std::cout << "Converting RGBA bitmap and saving into `" << output_path << "'..." << std::endl;
		}
		(void)DoImageOperation(img, &Magick::Image::write, output_path);
		if(options::verbosity >= 0) {
			std::cout << "Saved." << std::endl;
		}
	}
}

/*
 * Slurps the input image into a std::string, to get a uniform treatment for both regular files and stdin
 * input (since we need to check for KTEX's magical number before feeding the image to ImageMagick or otherwise).
 */
int main(int argc, char* argv[]) {
	try {
		string input_path;
		string output_path;
		KTEX::File::Header configured_header = parse_commandline_options(argc, argv, input_path, output_path);

		bool output_has_extension = process_paths(input_path, output_path);

		cout << "in: " << input_path << endl;
		cout << "out: " << output_path << endl;

		//assert( input_path != "-" );

		if(KTech::KTEX::File::isKTEXFile(input_path)) {
			if(!output_has_extension)
				output_path += DEFAULT_OUTPUT_EXTENSION;

			convert_from_KTEX(input_path, output_path);
		}
		else {
			if(!output_has_extension)
				output_path += "tex";

			convert_to_KTEX(input_path, output_path, configured_header);
		}
	}
	catch(std::exception& e) {
		cerr << "error: " << e.what() << endl;
		exit(1);
	}

	return 0;
}
