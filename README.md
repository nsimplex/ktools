# ktools

A set of cross-platform modding tools for the game [Don't Starve](http://www.dontstarvegame.com/), by [Klei Entertainment](http://kleientertainment.com/).


**IMPORTANT**: In what follows, a code block starting with a '$' indicates something that should be typed in a terminal (cmd.exe, for Windows). The '$' and the space following it should not be typed.


## ktech
A bidirectional cross-platform converter between Klei Entertainment's TEX texture format and PNG.

### Basic usage and examples
ktech converts bidirectionally between Klei's TEX format (KTEX) and PNG. If the first argument given to ktech is a KTEX file, it will be converted to PNG, and conversely if the first argument is a PNG file it will be converted to KTEX. If the second argument it missing, it is taken to be the first argument with the extension replaced (but in the current directory). If the second argument is a directory, the same is done as in the missing argument case, but the resulting file is placed in this directory.

To convert `atlas-0.tex` to `atlas-0.png`:
```
$ ktech atlas-0.tex
```
To do the same, placing it in some_folder:
```
$ ktech atlas-0.tex some_folder
```
To convert some/path/to/a.png to mymod/modicon.tex:
```
$ ktech some/path/to/a.ong mymod/modicon.tex
```

### Full usage
The following message (possibly more up to date than what is documented here) may be obtained by entering
```
$ ktech --help
```

```
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
    -f,  --filter  <lanczos|mitchell|bicubic|catrom|cubic|box>
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
    --extend
         Extends the boundaries of the image instead of resizing. Only relevant
         if either of the options `width', `height' or `pow2' are given. Its
         primary use is generating save and selection screen portraits.
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
```



## krane
A cross-platform decompiler for Klei Entertainment's animation format (anim.bin's and build.bin's).

krane's output is a [Spriter](http://www.brashmonkey.com/spriter.htm) project.

### Basic usage and examples
krane's primary usage is converting an animation file (anim.bin) and a build file (build.bin) as found within the ZIPs of Don't Starve's anim/ subdirectory into a Spriter project. The simplest use case is
```
$ krane anim.bin build.bin output_dir
```
which generates a Spriter project inside output_dir.

The last argument to krane is always taken to be the output directory, which is created if it doesn't exist. All arguments before that are input files, which should be build or anim files (directories may also be given, in which case files called build.bin and anim.bin in them are used, provided they exist). At least one animation file and exactly one build file must be given (multiple build files are accepted if the `--build-name option` is given to select a single build by build name). Atlases need not be given as arguments, since they are determined upon inspection of the build file.

Not all of Don't Starve's animations can be faithfully represented as a Spriter project, since Spriter's animation representation is much more restrictive than Don't Starve's native one (it doesn't support shearing, for example). The option `--check-animation-fidelity` will check for and print the cases where animation precision is lost.

Another use for krane is marking an atlas with the regions which get clipped by the game (shading them in grey), which is meant as an aid for atlas editing. This mode of use is triggered by the `--mark-atlases` option. When this option is given, only a build file should be given as input (any animation files are ignored), which is used to determine the clipping region. The shaded atlases are placed in the output directory (the last argument), as PNG images. The following will place the shaded atlases of build.bin in output_dir:
```
$ krane --mark-atlases build.bin output_dir
```

### Full usage
The following message (possibly more up to date than what is documented here) may be obtained by entering
```
$ krane --help
```

```
Usage: krane [OPTION]... [--] <INPUT-PATH>... <OUTPUT-DIR>

Options filtering input selection:
    --build  <build name>
         Selects only a build with the given name.
    --bank  <bank name>  (accepted multiple times)
         Selects only animations in the given banks.
Options controlling the output mode:
    --mark-atlases
         Instead of performing any conversion, saves the atlases in the
         specified build as PNG, with their clipped regions shaded grey.
Options for scml output:
    --check-animation-fidelity
         Checks if the Spriter representation of the animations is faithful to
         the source animations.
Other options:
    --rename-build  <build name>
         Renames the input build to the given name.
    --rename-bank  <bank name>
         Renames the input banks to the given name.
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

Converts the INPUT-PATH(s) into a Spriter project, placed in the directory
OUTPUT-DIR, which is created if it doesn't exist.

Whenever an INPUT-PATH is a directory, the files build.bin and anim.bin in
it (if any) are used, otherwise INPUT-PATH itself is used. The location of
build atlases is automatically determined from the build file, so they need
not be given as arguments.
```



## INSTALLATION FROM SOURCE
First, install [CMake](http://www.cmake.org), [ImageMagick](http://www.imagemagick.org) and a native building solution (such as Visual Studio for Windows, XCode for Mac and gcc/GNU Make for Linux; in the Linux case, you probably have them already).

The library libzip is an optional dependency. If it is present and found at compilation time, zip archives are treated in the same manner as directories when given as input.

### Linux anc Mac
Enter ktools' directory with a terminal and type
```
$ ./configure && make
```
provided there are no errors, ktech and krane should be placed in ktools' directory. The `./configure` step may be replaced by running CMake directly (either via its command-line tool, `cmake`, or its GUI tool, `ccmake`), where customization options are available. Finally, as an optional last step, in order to perform a system wide installation of the ktools type
```
$ sudo make install
```

### Windows
*Note for MinGW users*: ImageMagick 6.8.9 was reported to not compile under MinGW. Version 6.8.8-10 was confirmed to work. You may also need to install (and likely compile) libpng and zlib.

Open CMake, select the ktools directory as the source folder ("Where is the source code") and whichever directory you'd like as the build folder ("Where to build the binaries"). Click "Configure" and select a generator (typically either a version of Visual Studio of "MinGW Makefiles"). Leave "Use default native compilers" checked and click "Finish". If you with to customize some compilation option (which shouldn't be necessary) do so now, in the options presented in red after the configure step finishes. If any option was changed, click "Configure" again. Finally, click "Generate", which should place the project files in the build directory you selected.

For MinGW, proceed as in the Linux/Mac case (i.e., simply run `make` in the build directory).

For Visual Studio, open the ALL_BUILD.vcxproj file, right click the 'ktools' solution on the left pane, click on "Configuration Manager..." and make sure to select "Release" as the active solution configuration. Then build the solution (which may be done by pressing "F7").



## LICENSE
See NOTICE.txt.
