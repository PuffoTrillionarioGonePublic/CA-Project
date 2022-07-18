#!/bin/bash

# build final executable able to use any of the developed solutions (single core, multi core and GPU)

nvcc -std=c++14 -O3 -g -c *.cu
g++ -c -Ofast -ggdb -std=c++17 *.cc
nvcc *.o -o exe

