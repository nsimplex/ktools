if HAVE_SSE2_EXTENSION
EXTENSION_FLAGS = -DSQUISH_USE_SSE=2
else !HAVE_SSE2_EXTENSION
if HAVE_ALTIVEC_EXTENSION
EXTENSION_FLAGS = -DSQUISH_USE_ALTIVEC=1
endif HAVE_ALTIVEC_EXTENSION
endif !HAVE_SSE2_EXTENSION

AM_CPPFLAGS = -I. -I.. $(EXTENSION_FLAGS)
AM_CXXFLAGS = $(SIMD_FLAGS)

noinst_LIBRARIES = libsquish.a
libsquish_a_SOURCES = alpha.cpp  clusterfit.cpp  colourblock.cpp  colourfit.cpp  colourset.cpp  maths.cpp  rangefit.cpp  singlecolourfit.cpp  squish.cpp \
		      alpha.h  clusterfit.h  colourblock.h  colourfit.h  colourset.h  config.h  maths.h  rangefit.h  simd_float.h  simd.h  simd_sse.h  simd_ve.h  singlecolourfit.h  squish.h \
		      singlecolourlookup.inl

EXTRA_DIST = LICENSE.txt
