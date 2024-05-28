# genesis [![Build and test all platforms](https://github.com/aretecarpe/genesis/actions/workflows/main.yml/badge.svg)](https://github.com/aretecarpe/genesis/actions/workflows/main.yml)

A portable, C++ 17 compliant header-only library containing commonly used types and traits from C++ 20 and up. As well as system, compiler, and architecture detection macros. Along with some useful extensions like object_pools and spin_wait and getting errors from system level calls.

## Introduction

Genesis is a library meant for being the base for which other libraries or applications are built upon. Containing compiler configs for selective compilation, common traits, and types such as C++ 20's stop_token/stop_source, C++ 23's expected, and C++ 26's inplace_stop_token/inplace_stop_source. As well as providing 'extensions' like spin_wait and object_pool's.

## Examples

The [examples](./examples/) folder contains plenty of examples showcasing features of genesis, such as [expected](./examples/expected/expected.cpp), [inplace_stop_token](./examples/inplace_stop_token/inplace_stop_token.cpp), [object_pool](./examples/object_pool/obect_pool.cpp), and [stop_token](./examples/stop_token/stop_token.cpp).

## Using genesis in your project

### cmake

If your project uses CMake, then after cloning 'genesis' and generating the installed artifacts, simply add the following to your 'CMakeLists.txt': 

```
find_package(genesis REQUIRED)

target_link_libraries(my_project INTERFACE genesis::genesis)
```

This will both find the package as well as link the header-only library.


### CMake Package Manager (CPM)

TO further simplify obtaining and including 'genesis' in your CMake project, it is recommneded to use the [CMake Pacakge Manager (CPM)](https://github.com/cpm-cmake/CPM.cmake) to fetch and configure 'genesis'.

Complete example:

```
cmake_minimum_required(VERSION 3.20 FATAL_ERROR)

project(genesisExample)

# Get CPM
# For more information on how to add CPM to your project, see: https://github.com/cpm-cmake/CPM.cmake#adding-cpm
include(CPM.cmake)

CPMAddPackage(
  NAME genesis
  GITHUB_REPOSITORY aretecarpe/genesis
  GIT_TAG master # This will always pull the latest code from the `master` branch. You may also use a specific release version or tag
  OPTIONS
	"BUILD_TESTING OFF"
	"BUILD_EXAMPLES OFF"
)

add_executable(main example.cpp)

target_link_libraries(main genesis::genesis)
```

## Building genesis

Below is the minimal instructions needed to build and install genesis.

To build genesis (examples and tests are enabled by default)

```shell
$ cmake -B build -S .
$ cmake --build build
```

To build genesis without examples or tests

```shell
$ cmake -B build -S . -DBUILD_TESTING=OFF -DBUILD_EXAMPLES=OFF
$ cmake --build build
```

To install genesis

```shell
$ cmake --install build
```

This will install genesis in a folder called 'installed_artifacts' and should be ready for ingestion into your project as a cmake package.
