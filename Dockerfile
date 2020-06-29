FROM gcc:latest

RUN apt update && \
    apt install -y \
      cmake \
      libmagick++-dev

COPY . /usr/src/ktools
WORKDIR /usr/src/ktools

# Small fix of the KTools::inverseOf() return type
# https://github.com/nsimplex/ktools/pull/13
RUN curl -o fix-inverseOf.patch \
      https://github.com/nsimplex/ktools/commit/47f38381671e03455eab193a1d3a88e11666af99.patch && \
    patch -p1 < fix-inverseOf.patch

RUN ./configure && \
    make && \
    make install
