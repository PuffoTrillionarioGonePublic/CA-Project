CPPFLAGS=-Wall -g -ggdb -std=c++17 -pthread -Wextra -Ofast
CC=g++
LDLIBS=-lm
EXE=
all: build

EXE+=multi
multi: multi.cc

build: $(EXE)

clean:
	rm -f *.o *.d $(EXE)
