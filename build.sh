#!/bin/bash

if [ ! -d /usr/local/include/cpp_redis/ ]; then
  git submodule update --init
  cd cpp_redis
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