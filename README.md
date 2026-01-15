## Variant C++ 23 Implementation

This project was created for educational purposes. The implementation of variant is based on [libc++](https://github.com/llvm/llvm-project/blob/main/libcxx/include/variant). I left a few comments in the
implementation with explanations of how some things work.

## Deps

- C++ 23 compiler (clang++/g++)
- CMake
- Git

## Build

```
git clone git@github.com:b1tflyyyy/variantx.git
git submodule update --init --recursive

mkdir build
cd build

cmake ..
ninja

ctest --test-dir tests --verbose
```
