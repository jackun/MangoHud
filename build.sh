#!/bin/bash

# Remove or increase --depth if server errors
git submodule update --init --depth 50
meson build --prefix=/usr
ninja -C build

LIBDIR=/usr/lib32
if [ -d /usr/lib/i386-linux-gnu ]; then
  LIBDIR=/usr/lib/i386-linux-gnu
fi

#meson build32 --cross-file build32.txt --prefix=/usr --libdir=$LIBDIR
#ninja -C build32
