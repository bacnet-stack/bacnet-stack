#!/bin/sh

alias make=mingw32-make.exe
PATH=/c/MinGW/bin:$PATH
export MAKE=mingw32-make.exe
export CC=mingw32-gcc.exe
export OBJCOPY=objcopy.exe
export AR=ar.exe
export SIZE=size.exe

make clean
make all