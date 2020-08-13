#!/bin/bash

if [ ! -f /usr/local/lib/liberedis.so ]; then
  git submodule update --init
  cd eredis
  git submodule update --init
  mkdir build
  cd build
  cmake ..
  make -j $(nproc)
  sudo make install
  cd ../../
fi

if [ ! -d build ]; then mkdir build; fi
cd build
cmake ..
make -j $(nproc)
sudo make install