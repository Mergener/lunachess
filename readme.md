## About

Luna is an alpha-beta pruning based chess engine that uses traditional evaluation methods.
It currently supports [UCI](https://en.wikipedia.org/wiki/Universal_Chess_Interface) protocol, so it is possible to embed it into a GUI.

## Prerequisites

- C++17 compliant compiler
- CMake 3.14 or higher (build system generator)

## Features

- Move generation
    - Magic bitboard based move-generation

- Search features
     - Killer move heuristic
     - History heuristic
     - Quiescence search
     - Iterative deepening with Transposition Tables
     - PV-Node heuristic
     - MVV-LVA ordering for captures
     - Null move pruning
     - Futility pruning
     - Delta pruning
     - Late move reductions

- Evaluation
    - Traditional evaluation with several positional parameters
    - Multithreaded genetic algorithm for tuning of positional parameters

## Building

- Unix based terminal + Makefile

Use the following command while in the root of the repository to setup the build system:
```
cmake -S . -B builds
```

After that, ```cd``` onto ```builds``` and call ```make```. This will generate binaries for
Luna.

- Visual Studio 2019/2022 

First, make sure 'C++ Cmake tools for Windows' is installed. If not, it is possible
to do so in the Visual Studio Installer.

With Visual Studio running, go to ```File > Open > Cmake...``` and select the ```CMakeLists.txt``` file
at the root of this repository. Visual Studio should then load the project accordingly, and you'll
be able to compile and run it within the IDE.

## Usage

Luna is a command line application that follows the UCI protocol. A documentation for the protocol can be found [here](http://wbec-ridderkerk.nl/html/UCIProtocol.html).

Note that, by using the UCI protocol, Luna is designed to be easily integrated with existing chess graphical interfaces.

Besides existing UCI commands, Luna also provides the following extensions:

* ```domoves <move1> [<move2> ...]``` Makes the specified moves on the current position.

* ```takeback [<n>] ``` Retracts the last n moves. 'n' defaults to 1.

* ```getpos``` Outputs the current position in a human comprehensible format.

* ```getfen``` Outputs the [FEN](https://en.wikipedia.org/wiki/Forsyth%E2%80%93Edwards_Notation) string for the current position.

* ```perft <depth>``` Calculates and outputs the [perft results](https://www.chessprogramming.org/Perft_Results) for the current position.