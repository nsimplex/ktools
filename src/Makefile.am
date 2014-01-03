bin_PROGRAMS = ktech

ktech_SOURCES = \
		ktech.cpp ktech_options.cpp \
		ktech.hpp ktech_common.hpp image_operations.hpp ktech_options.hpp ktech_options_output.hpp \
		ktex/ktex.cpp ktex/specs.cpp \
		ktex/ktex.hpp ktex/specs.hpp ktex/headerfield_specs.hpp ktex/io.hpp \
		compat.hpp compat/common.hpp compat/posix.hpp compat/fs.hpp

AM_CPPFLAGS = -I$(top_srcdir) -I$(top_srcdir)/lib $(DEPS_CFLAGS)

ktech_LDADD = $(top_srcdir)/lib/squish/libsquish.a $(DEPS_LIBS)



all-local: $(top_srcdir)/ktech

$(top_srcdir)/ktech$(EXEEXT): ktech$(EXEEXT)
	-cp ktech$(EXEEXT) $(top_srcdir)

clean-local:
	-$(RM) $(top_srcdir)/ktech$(EXEEXT)
