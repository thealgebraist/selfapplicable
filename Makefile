CXX ?= g++
CXXFLAGS ?= -std=c++23 -Wall -Wextra -pedantic -O2

.PHONY: all check-normaliser check-compiler check release

all: check-normaliser

check-normaliser: normaliser.cpp
	$(CXX) $(CXXFLAGS) $< -o normaliser-ci
	./normaliser-ci

check-compiler:
	./test_c_subset_assembler.sh

check: check-normaliser check-compiler

release:
	./make_release_bundle.sh
