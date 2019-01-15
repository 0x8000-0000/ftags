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


Building
--------

    git clone ...
    mkdir ftags.build
    cd ftags.build
    CXX=clang++-7 cmake ../ftags -DLIBCLANG_LLVM_CONFIG_EXECUTABLE=llvm-config-7
    cmake --build .


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
