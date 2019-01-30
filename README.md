Fast Tags
=========

Introduction
------------

This project implements a source code indexer in C++-17 using Clang and
minimal 3rd party dependencies.


Scope
-----

The source code indexer will support the following types of queries:

   * location of symbol declaration

   * location of symbol definition

   * locations of symbol references

   * identify symbol at specific location

   * list of base classes for symbol, if applicable

   * list of derived classes for symbol, if applicable

   * list of override functions or methods for symbol, if applicable

As stretch goal, it will implement Language Server Protocol.


Dependencies
------------

Dependencies imported from the OS image (referenced, not included)

   * [{fmt}](https://github.com/fmtlib/fmt)

   * [Google Benchmark](https://github.com/google/benchmark)

   * [Google Test](https://github.com/google/googletest)

   * [Google Protocol Buffers](https://developers.google.com/protocol-buffers/)

   * [spdlog](https://github.com/gabime/spdlog)

   * [ØMQ](http://zeromq.org/)

To install the prerequisites on Debian run the following command:

    sudo apt install libzmq3-dev libspdlog-dev libfmt-dev libbenchmark-dev \
        libbenchmark-tools libgtest-dev libclang-7-dev libclang1-7 clang-7 \
        libprotobuf-dev protobuf-c-compiler

Included dependencies:

   * [Clara](https://github.com/catchorg/Clara/)


Building
--------

    git clone ...
    mkdir ftags.build
    cd ftags.build
    [CXX=clang++-7] cmake ../ftags -DLIBCLANG_LLVM_CONFIG_EXECUTABLE=llvm-config-7 [-DCMAKE_BUILD_TYPE=Debug]
    cmake --build .

Note: don't use -GNinja if you plan to work on this project because due to
[CMake Bug  17450](https://gitlab.kitware.com/cmake/cmake/issues/17450) the
compile\_commands.json contains invalid relative paths to the generated files
instead of absolute paths. So your code completion engine will not find some
of the protobuf-generated files.

License
-------

Copyright 2019 Florin Iucha

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
