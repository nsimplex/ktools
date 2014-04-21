# ktools

This README has yet to be updated to account for the new krane tool.


---


ktech
==========
A Unix converter for Klei Entertainment's KTEX texture format.

LICENSE
---------
See NOTICE.txt.

USAGE
-------
	Usage: ktech [OPTION]... [--] <input-file[,...]> [output-path]

	Options for TEX input:
		-Q,  --quality  <0-100>
			 Quality used when converting TEX to PNG/JPEG/etc. Higher values result
			 in less compression (and thus a bigger file size). Defaults to 100.
		-i,  --info
			 Prints information for a given TEX file instead of converting it.
	Options for TEX output:
		-c,  --compression  <dxt1|dxt3|dxt5|rgb|rgba>
			 Compression type for TEX creation. Defaults to dxt5.
		-f,  --filter  <lanczos|blackman|hann|hamming|catrom|bicubic|box>
			 Resizing filter used for mipmap generation. Defaults to lanczos.
		-t,  --type  <1d|2d|3d|cube>
			 Target texture type. Defaults to 2d.
		--no-premultiply
			 Don't premultiply alpha.
		--no-mipmaps
			 Don't generate mipmaps.
	Other options:
		--width  <pixels>
			 Fixed width to be used for the output. Without a height, preserves
			 ratio.
		--height  <pixels>
			 Fixed height to be used for the output. Without a width, preserves
			 ratio.
		--pow2
			 Rounds width and height up to a power of 2. Applied after the options
			 `width' and `height', if given.
		-v,  --verbose  (accepted multiple times)
			 Increases output verbosity.
		-q,  --quiet
			 Disables text output. Overrides the verbosity value.
		--version
			 Displays version information and exits.
		-h,  --help
			 Displays usage information and exits.
		--,  --ignore_rest
			 Ignores the rest of the labeled arguments following this flag.

	If input-file is a TEX image, it is converted to a non-TEX format (defaulting
	to PNG). Otherwise, it is converted to TEX. The format of input-file is
	inferred from its binary contents (its magic number).

	If output-path is not given, then it is taken as input-file stripped from its
	directory part and with its extension replaced by `.png' (thus placing it in
	the current directory). If output-path is a directory, this same rule applies,
	but the resulting file is placed in it instead. If output-path is given and is
	not a directory, the format of the output file is inferred from its extension.

	If more than one input file is given (separated by commas), they are assumed
	to be a precomputed mipmap chain (this use scenario is mostly relevant for
	automated processing). This should only be used for TEX output.

	If output-path contains the string '%02d', then for TEX input all its
	mipmaps will be exported in a sequence of images by replacing '%02d' with
	the number of the mipmap (counting from zero).

INSTALLATION FROM SOURCE
--------------
First, install [CMake](http://www.cmake.org), [ImageMagick](http://www.imagemagick.org) and a native building solution (such as Visual Studio for Windows, XCode for Mac and gcc/GNU Make for Linux; in the Linux case, you probably have it already).

In a terminal, enter the folder in which to build the project. The remaining assumes this folder is the build/ subdirectory of ktech's main directory. Type
	cmake ..
For a portable build, compatible across many CPU architectures, use the following command instead
	cmake .. -DDISABLE_CPU_EXTENSIONS=TRUE
The native build files will be generated in the current directory. Under Unix, building is achieved by entering
	make
optionally followed by
	sudo make install
for a system wide installation.

CMake's GUI may be used instead. Refer to its documentation for more information. Note, however, that in this case, in order to set DISABLE_CPU_EXTENSIONS the configuration must be generated once, so that the option will be available, and after tweaking it the configuration must be regenerated.
