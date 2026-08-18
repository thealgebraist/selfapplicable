CXX ?= g++
CXXFLAGS ?= -std=c++23 -Wall -Wextra -pedantic -O2

.PHONY: all check-normaliser check-minimal-arm64 check-llvm check-compiler check release

all: check-normaliser

check-normaliser: normaliser.cpp
	$(CXX) $(CXXFLAGS) $< -o normaliser-ci
	./normaliser-ci

check-minimal-arm64: minimal_arm64_compiler.cpp test_minimal_arm64.sh
	$(CXX) $(CXXFLAGS) minimal_arm64_compiler.cpp -o minimal_arm64_compiler-ci
	./test_minimal_arm64.sh ./minimal_arm64_compiler-ci

check-llvm: minimal_llvm_compiler.cpp examples/selfapp_llvm.min test_minimal_llvm.sh
	$(CXX) $(CXXFLAGS) minimal_llvm_compiler.cpp -o minimal_llvm_compiler-ci
	./test_minimal_llvm.sh ./minimal_llvm_compiler-ci

check-compiler:
	./test_c_subset_assembler.sh

check: check-normaliser check-minimal-arm64 check-llvm check-compiler

release:
	./make_release_bundle.sh
