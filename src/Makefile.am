bin_PROGRAMS = ktech

ktech_SOURCES = ktech.cpp ktech_options.cpp ktech_fs.cpp ktex/ktex.cpp ktex/specs.cpp \
		ktech.hpp ktech_common.hpp ktech_options.hpp ktech_options_output.hpp ktech_fs.hpp ktex/ktex.hpp ktex/specs.hpp ktex/headerfield_specs.hpp ktex/io.hpp

AM_CPPFLAGS = $(DEPS_CFLAGS) -I$(top_srcdir)/lib

ktech_LDADD = $(top_srcdir)/lib/squish/libsquish.a $(DEPS_LIBS)



all-local: $(top_srcdir)/ktech

$(top_srcdir)/ktech: ktech
	-cp ktech $(top_srcdir)

clean-local:
	-$(RM) $(top_srcdir)/ktech
