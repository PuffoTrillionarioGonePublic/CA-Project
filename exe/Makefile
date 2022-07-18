CPPFLAGS=-Wall -g -ggdb -std=c++17 -pthread -Wextra -lrt -rdynamic -Ofast
CUDAFLAGS=-g -std=c++14 -DNDEBUG -O3
CC=gcc
LDLIBS=-lm
EXE=
CUDA=

all: build

EXE+=single
single: single.cc

EXE+=float-single
float-single: float-single.cc

EXE+=multi
multi: multi.cc

EXE+=float-multi
float-multi: float-multi.cc

EXE+=single-row
single-row: single-row.cc

EXE+=float-single-row
float-single-row: float-single-row.cc

EXE+=multi-row
multi-row: multi-row.cc

EXE+=float-multi-row
float-multi-row: float-multi-row.cc

build: $(EXE)

CUDA+=kernel
kernel: kernel.cu
	nvcc $(CUDAFLAGS) $< -o $@ $(LDLIBS)

CUDA+=float-kernel
float-kernel: float-kernel.cu
	nvcc $(CUDAFLAGS) $< -o $@ $(LDLIBS)

cuda: $(CUDA)

clean:
	rm -f *.o *.d $(EXE) $(CUDA)
