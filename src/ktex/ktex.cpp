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


#include "ktex/ktex.hpp"
#include "ktex/io.hpp"
#include "ktech_options.hpp"


KTech::Endianess KTech::KTEX::source_endianness = KTech::ktech_UNKNOWN_ENDIAN;
KTech::Endianess KTech::KTEX::target_endianness = KTech::ktech_LITTLE_ENDIAN;


using namespace KTech;
using namespace KTech::KTEX;

bool KTech::KTEX::File::isKTEXFile(const std::string& path) {
	uint32_t magic;

	std::ifstream file(path.c_str(), std::ifstream::in | std::ifstream::binary);
	if(!file)
		return false;

	file.imbue(std::locale::classic());

	raw_read_integer(file, magic);

	return file && magic == HeaderSpecs::MAGIC_NUMBER;
}

void KTech::KTEX::File::dumpTo(const std::string& path) {
	if(options::verbosity >= 0) {
		std::cout << "Dumping KTEX to `" << path << "'..." << std::endl;	
	}

	std::ofstream out(path.c_str(), std::ofstream::out | std::ofstream::binary);
	if(!out)
		throw(KleiUtilsError("failed to open `" + path + "' for writing."));

	out.imbue(std::locale::classic());

	dump(out);

	if(options::verbosity >= 0) {
		std::cout << "Finished dumping." << std::endl;
	}
}

void KTech::KTEX::File::loadFrom(const std::string& path) {
	if(options::verbosity >= 0) {
		std::cout << "Loading KTEX from `" << path << "'..." << std::endl;
	}
	
	std::ifstream in(path.c_str(), std::ifstream::in | std::ifstream::binary);
	if(!in)
		throw(KleiUtilsError("failed to open `" + path + "' for reading."));

	in.imbue(std::locale::classic());

	load(in);

	if(options::verbosity >= 0) {
		std::cout << "Finished loading." << std::endl;
	}
}

void KTech::KTEX::File::Header::print(std::ostream& out, size_t indentation, const std::string& indent_string) const {
	using namespace std;

	std::string prefix;
	for(size_t i = 0; i < indentation; i++) {
		prefix += indent_string;
	}

	for(field_spec_offset_iterator it = FieldSpecs.offset_begin(); it != FieldSpecs.offset_end(); ++it) {
		const HeaderFieldSpec& spec = it->second;


		out << prefix << spec.id;

		if(options::verbosity >= 2) {
			out << ":" << endl;
			out << prefix << indent_string << "length: " << spec.length << endl;
			out << prefix << indent_string << "offset: " << spec.offset << endl;
			out << prefix << indent_string << "value: ";
		}
		else {
			out << " = ";
		}

		std::string fs = getFieldString(it->first);

		if(fs.length() > 0) {
			out << getField(it->first) << " (" << fs << ")" << endl;
		}
		else {
			out << getField(it->first) << endl;
		}
	}
}

std::ostream& KTech::KTEX::File::Header::dump(std::ostream& out) const {
	write_integer(out, MAGIC_NUMBER);
	write_integer(out, data);
	return out;
}

std::istream& KTech::KTEX::File::Header::load(std::istream& in) {
	reset();

	uint32_t magic;
	raw_read_integer(in, magic);
	if(!in || magic != MAGIC_NUMBER) {
		throw(KleiUtilsError("Attempt to read a non-KTEX file as KTEX."));
	}

	raw_read_integer(in, data);

	// Here we infer the endianness based on the fill portion of the
	// header, defaulting to little endian if we can't reach a
	// conclusion.
	source_endianness = ktech_LITTLE_ENDIAN;
	if( getField("fill") == FieldSpecs["fill"].value_default ) {
		source_endianness = ktech_NATIVE_ENDIANNESS;
	}
	else {
		Header header_cp = *this;
		reorder(header_cp.data);
		if( header_cp.getField("fill") == FieldSpecs["fill"].value_default ) {
			source_endianness = ktech_INVERSE_NATIVE_ENDIANNESS;
		}
	}
	if(source_endianness == ktech_INVERSE_NATIVE_ENDIANNESS) {
		reorder(data);
	}

	return in;
}

void KTech::KTEX::File::Mipmap::print(std::ostream& out, size_t indentation, const std::string& indent_string) const {
	using namespace std;

	std::string prefix;
	for(size_t i = 0; i < indentation; i++) {
		prefix += indent_string;
	}

	out << prefix << "width: " << width << endl;
	out << prefix << "height: " << height << endl;
	out << prefix << "pitch: " << pitch << endl;
	out << prefix << "data size: " << datasz << endl;
}

std::ostream& KTech::KTEX::File::Mipmap::dumpPre(std::ostream& out) const {
	write_integer(out, width);
	write_integer(out, height);
	write_integer(out, pitch);
	write_integer(out, datasz);

	return out;
}

std::ostream& KTech::KTEX::File::Mipmap::dumpPost(std::ostream& out) const {
	out.write( reinterpret_cast<const char*>( data ), datasz );

	return out;
}

std::istream& KTech::KTEX::File::Mipmap::loadPre(std::istream& in) {
	read_integer(in, width);
	read_integer(in, height);
	read_integer(in, pitch);
	read_integer(in, datasz);

	setDataSize( datasz );

	return in;
}

std::istream& KTech::KTEX::File::Mipmap::loadPost(std::istream& in) {
	in.read( reinterpret_cast<char*>( data ), datasz );

	return in;
}

void KTech::KTEX::File::print(std::ostream& out, size_t indentation, const std::string& indent_string) const {
	using namespace std;

	std::string prefix;
	for(size_t i = 0; i < indentation; i++) {
		prefix += indent_string;
	}

	out << prefix << "Header:" << endl;
	header.print(out, indentation + 1, indent_string);

	if(options::verbosity >= 1) {
		size_t mipmap_count = header.getField("mipmap_count");

		for(size_t i = 0; i < mipmap_count; ++i) {
			out << "Mipmap #" << (i + 1) << ":" << endl;
			Mipmaps[i].print(out, indentation + 1, indent_string);
		}
	}
}

std::ostream& KTech::KTEX::File::dump(std::ostream& out) const {
	size_t mipmap_count = header.getField("mipmap_count");

	if(options::verbosity >= 1) {
		std::cout << "Dumping KTEX header..." << std::endl;
	}
	if(options::verbosity >= 2) {
		header.print(std::cout, 1);
	}
	if(!header.dump(out)) {
		throw(KleiUtilsError("Failed to write KTEX header."));
	}

	// If it's at least 2, the mipmap count was already printed as part of
	// the header.
	if(options::verbosity == 1) {
		std::cout << "Mipmap count: " << mipmap_count << std::endl;
	}

	for(size_t i = 0; i < mipmap_count; ++i) {
		if(options::verbosity >= 1) {
			std::cout << "Dumping (pre) mipmap #" << (i + 1) << "..." << std::endl;
			if(options::verbosity >= 2) {
				Mipmaps[i].print(std::cout, 1);
			}
		}

		if(!Mipmaps[i].dumpPre(out)) {
			throw(KleiUtilsError("Failed to write KTEX mipmap."));
		}
	}

	for(size_t i = 0; i < mipmap_count; ++i) {
		if(options::verbosity >= 1) {
			std::cout << "Dumping (post) mipmap #" << (i + 1) << "..." << std::endl;
		}

		if(!Mipmaps[i].dumpPost(out)) {
			throw(KleiUtilsError("Failed to write KTEX mipmap."));
		}
	}

	return out;
}

std::istream& KTech::KTEX::File::load(std::istream& in) {
	if(options::verbosity >= 1) {
		std::cout << "Loading KTEX header..." << std::endl;
	}
	if(!header.load(in)) {
		throw(KleiUtilsError("Failed to read KTEX header."));
	}
	if(options::verbosity >= 2) {
		header.print(std::cout, 1);
	}

	size_t mipmap_count = header.getField("mipmap_count");

	if(options::verbosity == 1) {
		std::cout << "Mipmap count: " << mipmap_count << std::endl;
	}

	reallocateMipmaps(mipmap_count);

	for(size_t i = 0; i < mipmap_count; ++i) {
		if(options::verbosity >= 1) {
			std::cout << "Loading (pre) mipmap #" << (i + 1) << "..." << std::endl;
		}

		if(!Mipmaps[i].loadPre(in)) {
			throw(KleiUtilsError("Failed to read KTEX mipmap."));
		}

		if(options::verbosity >= 2) {
			Mipmaps[i].print(std::cout, 1);
		}
	}

	if(options::info) return in;

	for(size_t i = 0; i < mipmap_count; ++i) {
		if(options::verbosity >= 1) {
			std::cout << "Loading (post) mipmap #" << (i + 1) << "..." << std::endl;
		}

		if(!Mipmaps[i].loadPost(in)) {
			throw(KleiUtilsError("Failed to read KTEX mipmap."));
		}
	}

	if(in.peek() != EOF) {
		std::cerr << "KTech warning: There is leftover data in the input TEX file." << std::endl;
	}

	return in;
}

static int getSquishCompressionFlag(File::Header header, bool& isnone) {
	isnone = false;

	const std::string& internal_flag = header.getFieldString("compression");

	if(internal_flag.compare("DXT1") == 0) {
		return squish::kDxt1;
	}
	if(internal_flag.compare("DXT3") == 0) {
		return squish::kDxt3;
	}
	if(internal_flag.compare("DXT5") == 0) {
		return squish::kDxt5;
	}

	isnone = true;
	return -1;
}

static std::string getMagickString(File::Header header) {
	const std::string& internal_flag = header.getFieldString("compression");

	if(internal_flag != "RGB" && internal_flag != "RGBA") {
		throw Error("Unsupported pixel format.");
	}

	return internal_flag;
}

KTech::KTEX::File::CompressionFormat KTech::KTEX::File::getCompressionFormat() const {
	KTech::KTEX::File::CompressionFormat fmt;
	fmt.squish_flags = getSquishCompressionFlag(header, fmt.is_uncompressed);
	return fmt;
}

Magick::Image KTech::KTEX::File::DecompressMipmap(const KTech::KTEX::File::Mipmap& M, const KTech::KTEX::File::CompressionFormat& fmt) const {
	Magick::Image img;
	Magick::Blob B;

	int width, height;

	width = (int)M.width;
	height = (int)M.height;

	if(width == 0 || height == 0)
		return img;

	if(!fmt.is_uncompressed) {
		if(options::verbosity >= 0) {
				std::cout << "Decompressing " << width << "x" << height << " KTEX image into RGBA..." << std::endl;
		}
		squish::u8* rgba = new squish::u8[4*width*height];
		squish::DecompressImage(rgba, width, height, M.getData(), fmt.squish_flags);
		B.updateNoCopy(rgba, 4*width*height);
		img.read(B, Magick::Geometry(width, height), 8, "RGBA");
	}
	else {
		std::string magick_str = getMagickString(header);

		if(options::verbosity >= 0) {
			std::cout << "Decompressing " << width << "x" << height << " KTEX image into ";
			if(magick_str == "RGBA") {
				std::cout << "RGBA";
			}
			else {
				assert( magick_str == "RGB" );
				std::cout << "RGB";
			}
			std::cout << "..." << std::endl;
		}

		B.update(M.getData(), M.getDataSize());
		img.read(B, Magick::Geometry(width, height), 8, magick_str);
	}


	if(options::verbosity >= 0) {
		std::cout << "Decompressed." << std::endl;
	}


	img.flip();

	return img;
}

void KTech::KTEX::File::CompressMipmap(KTech::KTEX::File::Mipmap& M, const KTech::KTEX::File::CompressionFormat& fmt, Magick::Image img) const {
	std::string magick_str = "RGBA";
	size_t pixel_size = 4;
	if(fmt.is_uncompressed) {
		magick_str = getMagickString(header);
		if(magick_str == "RGB") {
			pixel_size = 3;
		}
	}

	img.flip();

	int width = (int)img.columns();
	int height = (int)img.rows();

	if(width <= 0 || height <= 0) {
		throw(KleiUtilsError("Attempt to compress an image with a non-positive dimension."));
	}

	M.width = width;
	M.height = height;

	if(fmt.is_uncompressed) {
		M.pitch = pixel_size*width;
	}
	else {
		M.pitch = squish::GetStorageRequirements(width, 1, fmt.squish_flags);
	}

	Magick::Blob B;
	img.write(&B, magick_str, 8);

	if(fmt.is_uncompressed) {
		M.setDataSize( B.length() );
	}
	else {
		M.setDataSize( squish::GetStorageRequirements(width, height, fmt.squish_flags) );
	}


	if(fmt.is_uncompressed) {
		memcpy(M.data, B.data(), B.length());
	}
	else {
		squish::CompressImage( (const squish::u8*)B.data(), width, height, M.data, fmt.squish_flags );
	}
}
