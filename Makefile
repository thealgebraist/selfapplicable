CXX ?= g++
CXXFLAGS ?= -std=c++23 -Wall -Wextra -pedantic -O2

.PHONY: all normaliser check-normaliser check-compiler check release

all: normaliser

normaliser: normaliser.cpp
	$(CXX) $(CXXFLAGS) $< -o $@

check-normaliser: normaliser
	./normaliser

check-compiler:
	./test_c_subset_assembler.sh

check: check-normaliser check-compiler

release:
	./make_release_bundle.sh
