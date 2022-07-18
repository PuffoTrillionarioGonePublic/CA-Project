CUDAFLAGS=-g -std=c++14 -DNDEBUG
CC=gcc
LDLIBS=-lm
CUDA=

all: cuda

CUDA+=kernel
kernel: kernel.cu
	nvcc $(CUDAFLAGS) -O3 $< -o $@ $(LDLIBS)

CUDA+=kernel-no-opt
kernel-no-opt: kernel.cu
	nvcc $(CUDAFLAGS) $< -o $@ $(LDLIBS)

cuda: $(CUDA)

clean:
	rm -f *.o *.d $(CUDA)
