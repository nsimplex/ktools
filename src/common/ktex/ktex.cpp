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
#include "binary_io_utils.hpp"
#include "ktools_bit_op.hpp"


using namespace KTools;
using namespace KTools::KTEX;

using std::cout;
using std::cerr;
using std::endl;

bool KTools::KTEX::File::isKTEXFile(std::istream& in) {
	try {
		return BinIOHelper::getMagicNumber(in) == HeaderSpecs::MAGIC_NUMBER;
	}
	catch(...) {
		return false;
	}
}

bool KTools::KTEX::File::isKTEXFile(const std::string& path) {
	try {
		return BinIOHelper::getMagicNumber(path) == HeaderSpecs::MAGIC_NUMBER;
	}
	catch(...) {
		return false;
	}
}

void KTools::KTEX::File::dumpTo(const std::string& path, int verbosity) {
	if(verbosity >= 0) {
		std::cout << "Dumping KTEX to `" << path << "'..." << std::endl;	
	}

	std::ofstream out(path.c_str(), std::ofstream::out | std::ofstream::binary);
	check_stream_validity(out, path);

	out.imbue(std::locale::classic());

	dump(out, verbosity);
}

void KTools::KTEX::File::loadFrom(const std::string& path, int verbosity, bool info_only) {
	if(verbosity >= 0) {
		std::cout << "Loading KTEX from `" << path << "'..." << std::endl;
	}
	
	std::ifstream in(path.c_str(), std::ifstream::in | std::ifstream::binary);
	check_stream_validity(in, path);

	in.imbue(std::locale::classic());

	load(in, verbosity, info_only);
}

void KTools::KTEX::File::Header::print(std::ostream& out, int verbosity, size_t indentation, const std::string& indent_string) const {
	using namespace std;

	std::string prefix;
	for(size_t i = 0; i < indentation; i++) {
		prefix += indent_string;
	}

	for(field_spec_offset_iterator it = FieldSpecs.offset_begin(); it != FieldSpecs.offset_end(); ++it) {
		const HeaderFieldSpec& spec = it->second;


		out << prefix << spec.id;

		if(verbosity >= 2) {
			out << ":" << endl;
			out << prefix << indent_string << "length: " << spec.length << endl;
			out << prefix << indent_string << "offset: " << spec.offset << endl;
			out << prefix << indent_string << "value: ";
		}
		else {
			out << ": ";
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

std::ostream& KTools::KTEX::File::Header::dump(std::ostream& out) const {
	BinIOHelper::sanitizeStream(out);

	BinIOHelper::raw_write_integer(out, MAGIC_NUMBER);
	io.write_integer(out, data);
	return out;
}

static void convertFromPreCaves(KTools::KTEX::File::Header& h) {
	using namespace KTools;

	BitOp::BitQueue<KTools::KTEX::File::Header::data_t> bq(h.data);

	h.data = 0;

	h.setField("platform", bq.pop(3));
	h.setField("compression", bq.pop(3));
	h.setField("texture_type", bq.pop(3));
	h.setField("mipmap_count", bq.pop(4));
	h.setField("flags", bq.pop(1));
	h.setField("fill", bq.pop(18));
}

std::istream& KTools::KTEX::File::Header::load(std::istream& in) {
	typedef BitOp::BitMask<8*sizeof(data_t) - 18, 18, data_t> precavesMask;

	BinIOHelper::sanitizeStream(in);

	reset();

	uint32_t magic;
	io.raw_read_integer(in, magic);
	if(!in || magic != MAGIC_NUMBER) {
		throw(KToolsError("Attempt to read a non-KTEX file as KTEX."));
	}

	io.raw_read_integer(in, data);

	// Here we infer the endianness based on the fill portion of the
	// header, defaulting to little endian if we can't reach a
	// conclusion.
	io.setLittleSource();
	if( (getField("fill") & 0xff) == (FieldSpecs["fill"].value_default & 0xff) ) {
		io.setNativeSource();
	}
	else {
		Header header_cp = *this;
		io.reorder(header_cp.data);
		if( (header_cp.getField("fill") & 0xff) == (FieldSpecs["fill"].value_default & 0xff) ) {
			io.setInverseNativeSource();
		}
	}
	if(io.isInverseNativeSource()) {
		io.reorder(data);
	}

	if( (data & precavesMask::value) == precavesMask::value ) {
		convertFromPreCaves(*this);
	}

	return in;
}

void KTools::KTEX::File::Mipmap::print(std::ostream& out, size_t indentation, const std::string& indent_string) const {
	using namespace std;

	std::string prefix;
	for(size_t i = 0; i < indentation; i++) {
		prefix += indent_string;
	}

	out << prefix << "size: " << width << "x" << height << endl;
	out << prefix << "pitch: " << pitch << endl;
	out << prefix << "data size: " << datasz << endl;
}

std::ostream& KTools::KTEX::File::Mipmap::dumpPre(std::ostream& out) const {
	parent->io.write_integer(out, width);
	parent->io.write_integer(out, height);
	parent->io.write_integer(out, pitch);
	parent->io.write_integer(out, datasz);

	return out;
}

std::ostream& KTools::KTEX::File::Mipmap::dumpPost(std::ostream& out) const {
	out.write( reinterpret_cast<const char*>( data ), datasz );

	return out;
}

std::istream& KTools::KTEX::File::Mipmap::loadPre(std::istream& in) {
	parent->io.read_integer(in, width);
	parent->io.read_integer(in, height);
	parent->io.read_integer(in, pitch);
	parent->io.read_integer(in, datasz);

	setDataSize( datasz );

	return in;
}

std::istream& KTools::KTEX::File::Mipmap::loadPost(std::istream& in) {
	in.read( reinterpret_cast<char*>( data ), datasz );

	return in;
}

void KTools::KTEX::File::print(std::ostream& out, int verbosity, size_t indentation, const std::string& indent_string) const {
	using namespace std;

	std::string prefix;
	for(size_t i = 0; i < indentation; i++) {
		prefix += indent_string;
	}

	const size_t mipmap_count = header.getField("mipmap_count");
	if(mipmap_count > 0) {
		std::string local_prefix = prefix + indent_string;
		const Mipmap& M = Mipmaps[0];
		out << local_prefix << "size: " << M.width << "x" << M.height << endl;
	}

	out << prefix << "Header:" << endl;
	header.print(out, verbosity, indentation + 1, indent_string);

	if(verbosity >= 1) {

		for(size_t i = 0; i < mipmap_count; ++i) {
			out << "Mipmap #" << (i + 1) << ":" << endl;
			Mipmaps[i].print(out, indentation + 1, indent_string);
		}
	}
}

std::ostream& KTools::KTEX::File::dump(std::ostream& out, int verbosity) const {
	BinIOHelper::sanitizeStream(out);

	size_t mipmap_count = header.getField("mipmap_count");

	if(verbosity >= 1) {
		std::cout << "Dumping KTEX header..." << std::endl;
	}
	if(verbosity >= 2) {
		header.print(std::cout, verbosity, 1);
	}
	if(!header.dump(out)) {
		throw(KToolsError("Failed to write KTEX header."));
	}

	// If it's at least 2, the mipmap count was already printed as part of
	// the header.
	if(verbosity == 1) {
		std::cout << "Mipmap count: " << mipmap_count << std::endl;
	}

	for(size_t i = 0; i < mipmap_count; ++i) {
		if(verbosity >= 1) {
			std::cout << "Dumping (pre) mipmap #" << (i + 1) << "..." << std::endl;
			if(verbosity >= 2) {
				Mipmaps[i].print(std::cout, 1);
			}
		}

		if(!Mipmaps[i].dumpPre(out)) {
			throw(KToolsError("Failed to write KTEX mipmap."));
		}
	}

	for(size_t i = 0; i < mipmap_count; ++i) {
		if(verbosity >= 1) {
			std::cout << "Dumping (post) mipmap #" << (i + 1) << "..." << std::endl;
		}

		if(!Mipmaps[i].dumpPost(out)) {
			throw(KToolsError("Failed to write KTEX mipmap."));
		}
	}

	return out;
}

std::istream& KTools::KTEX::File::load(std::istream& in, int verbosity, bool info_only) {
	BinIOHelper::sanitizeStream(in);

	if(verbosity >= 1) {
		std::cout << "Loading KTEX header..." << std::endl;
	}
	if(!header.load(in)) {
		throw(KToolsError("Failed to read KTEX header."));
	}
	if(verbosity >= 2) {
		header.print(std::cout, verbosity, 1);
	}

	size_t mipmap_count = header.getField("mipmap_count");

	if(verbosity == 1) {
		std::cout << "Mipmap count: " << mipmap_count << std::endl;
	}

	reallocateMipmaps(mipmap_count);

	for(size_t i = 0; i < mipmap_count; ++i) {
		if(verbosity >= 1) {
			std::cout << "Loading (pre) mipmap #" << (i + 1) << "..." << std::endl;
		}

		if(!Mipmaps[i].loadPre(in)) {
			throw(KToolsError("Failed to read KTEX mipmap."));
		}

		if(verbosity >= 2) {
			Mipmaps[i].print(std::cout, 1);
		}
	}

	if(info_only) return in;

	for(size_t i = 0; i < mipmap_count; ++i) {
		if(verbosity >= 1) {
			std::cout << "Loading (post) mipmap #" << (i + 1) << "..." << std::endl;
		}

		if(!Mipmaps[i].loadPost(in)) {
			throw(KToolsError("Failed to read KTEX mipmap."));
		}
	}

	if(in.peek() != EOF) {
		std::cerr << "Warning: There is leftover data in the input TEX file." << std::endl;
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

KTools::KTEX::File::CompressionFormat KTools::KTEX::File::getCompressionFormat() const {
	KTools::KTEX::File::CompressionFormat fmt;
	fmt.squish_flags = getSquishCompressionFlag(header, fmt.is_uncompressed);
	return fmt;
}

Magick::Image KTools::KTEX::File::DecompressMipmap(const KTools::KTEX::File::Mipmap& M, const KTools::KTEX::File::CompressionFormat& fmt, int verbosity) const {
	Magick::Image img;
	Magick::Blob B;

	int width, height;

	width = (int)M.width;
	height = (int)M.height;

	if(width == 0 || height == 0)
		return img;

	if(!fmt.is_uncompressed) {
		if(verbosity >= 0) {
				std::cout << "Decompressing " << width << "x" << height << " KTEX image into RGBA..." << std::endl;
		}
		squish::u8* RESTRICT rgba = new squish::u8[4*width*height];
		squish::DecompressImage(rgba, width, height, M.getData(), fmt.squish_flags);
		B.updateNoCopy(rgba, 4*width*height);
		img.read(B, Magick::Geometry(width, height), 8, "RGBA");
	}
	else {
		std::string magick_str = getMagickString(header);

		if(verbosity >= 0) {
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


	if(verbosity >= 0) {
		std::cout << "Decompressed." << std::endl;
	}


	if(flip_image) {
		img.flip();
	}

	return img;
}

void KTools::KTEX::File::CompressMipmap(KTools::KTEX::File::Mipmap& M, const KTools::KTEX::File::CompressionFormat& fmt, Magick::Image img, int verbosity) const {
	(void)verbosity;

	std::string magick_str = "RGBA";
	size_t pixel_size = 4;
	if(fmt.is_uncompressed) {
		magick_str = getMagickString(header);
		if(magick_str == "RGB") {
			pixel_size = 3;
		}
	}

	if(flip_image) {
		img.flip();
	}

	const size_t width = img.columns();
	const size_t height = img.rows();

	if(width == 0 || height == 0) {
		throw(KToolsError("Attempt to compress an image with zero size."));
	}

	cast_assign(M.width, width);
	cast_assign(M.height, height);

	if(fmt.is_uncompressed) {
		cast_assign(M.pitch, pixel_size*width);
	}
	else {
		cast_assign(M.pitch, squish::GetStorageRequirements(int(width), 1, fmt.squish_flags));
	}

	Magick::Blob B;
	img.write(&B, magick_str, 8);

	if(fmt.is_uncompressed) {
		M.setDataSize( B.length() );
	}
	else {
		M.setDataSize( squish::GetStorageRequirements(int(width), int(height), fmt.squish_flags) );
	}


	if(fmt.is_uncompressed) {
		memcpy(M.data, B.data(), B.length());
	}
	else {
		squish::CompressImage( (const squish::u8*)B.data(), int(width), int(height), M.data, fmt.squish_flags );
	}
}
