if IS_MINGW_CROSS_COMPILING
EXTRA_ktech_LIBS = -llcms2 -lm -llqr-1 -lglib-2.0 -lpcre -lm -lpcre -lglib-2.0 -lfreetype -lz -lbz2 -lpng16 -lz -lm -lm -llzma -lbz2 -lz -lltdl -lm -lpcre -lexpat -lpng16 -luuid -ljasper -ljpeg -lIlmImf -lHalf -lIlmThread -lIex -lpcre -lintl -liconv -lole32 -lwinmm -lgdi32 -lfftw3
endif IS_MINGW_CROSS_COMPILING


bin_PROGRAMS = ktech

ktech_SOURCES = \
		ktech.cpp ktech_options.cpp \
		ktech.hpp ktech_common.hpp ktech_bit_op.hpp image_operations.hpp ktech_options.hpp ktech_options_output.hpp \
		ktex/ktex.cpp ktex/specs.cpp \
		ktex/ktex.hpp ktex/specs.hpp ktex/headerfield_specs.hpp ktex/io.hpp \
		compat.hpp compat/common.hpp compat/posix.hpp compat/fs.hpp

AM_CPPFLAGS = -I$(top_srcdir) -I$(top_srcdir)/lib $(DEPS_CFLAGS)

ktech_LDADD = $(top_srcdir)/lib/squish/libsquish.a $(DEPS_LIBS) $(EXTRA_ktech_LIBS) $(EXTRA_ktech_LIBS)



all-local: $(top_srcdir)/ktech$(EXEEXT)

$(top_srcdir)/ktech$(EXEEXT): ktech$(EXEEXT)
	-cp ktech$(EXEEXT) $(top_srcdir)

clean-local:
	-$(RM) $(top_srcdir)/ktech$(EXEEXT)
