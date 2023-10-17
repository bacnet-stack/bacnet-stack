#!/bin/bash

mkdir build/
#cd build && make clean
cd build && cmake .. -G"CodeBlocks - Unix Makefiles"
sudo make install

