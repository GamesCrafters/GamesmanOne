#!/bin/sh

git submodule update --init
cd lib/json-c/
mkdir build
cd build
cmake -DCMAKE_INSTALL_PREFIX="../../../res" ../
make -j
make install -j
cd ../../../
autoreconf --install
./configure 'CFLAGS=-Wall -Wextra -g -O3 -DNDEBUG'
make -j
