#!/bin/bash

rm -rf twister-out.unit_testing
../zephyr/scripts/twister -O twister-out.unit_testing -p unit_testing -T zephyr/tests
#for file in CMakeFiles CMakeCache.txt cmake_install.cmake Makefile
#do
#    find twister-out.unit_testing -name $file -exec rm -rf {} \; || true
#done
