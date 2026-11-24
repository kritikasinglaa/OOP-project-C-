# ERL-HUB build file.  Usage:  make | make run | make test | make clean
# Uses .RECIPEPREFIX so recipes are indented with ">" (no hard tabs needed).
.RECIPEPREFIX = >

CXX      ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -Wpedantic -O2
CPPFLAGS += -Iinclude

LIB_SRC := $(filter-out src/main.cpp,$(wildcard src/*.cpp))
LIB_OBJ := $(patsubst src/%.cpp,build/%.o,$(LIB_SRC))

.PHONY: all run test clean

all: build/erl-hub

build/erl-hub: $(LIB_OBJ) build/main.o
> $(CXX) $(CXXFLAGS) $^ -o $@

build/erl-tests: $(LIB_OBJ) tests/test_erlhub.cpp
> $(CXX) $(CXXFLAGS) $(CPPFLAGS) $^ -o $@

build/%.o: src/%.cpp $(wildcard include/*.hpp) | build
> $(CXX) $(CXXFLAGS) $(CPPFLAGS) -c $< -o $@

build:
> mkdir -p build

run: build/erl-hub
> ./build/erl-hub

test: build/erl-tests
> ./build/erl-tests

clean:
> rm -rf build
