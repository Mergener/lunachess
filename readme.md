## About

Luna is an alpha-beta pruning based chess engine that uses traditional evaluation methods.
It currently supports [UCI](https://en.wikipedia.org/wiki/Universal_Chess_Interface) protocol, so it is possible to embed it into a GUI.

## Features

- Move generation
  - Magic bitboards based move-generation

- Search features
  - Principal Variation Search
  - Iterative deepening with Transposition Tables
  - Quiescence search
  - SEE + MVV-LVA ordering for captures
  - Killer move heuristic
  - Late Move Reductions
  - History heuristic
  - Null move pruning
  - Futility pruning
  - Delta pruning
  - Aspiration Windows
  - Check extension
  
- Evaluation features
  - Tapered eval 
  - Material
  - Mobility
  - Pawn/King/Queen PSTs
  - King safety
  - Passed pawns
  - Isolated pawns
  - Doubled pawns
  - Bishop pair
  - Knight outposts
  - Endgame specific evaluations (ex: KBN vs K)

## Project Structure

- `/src` - Source code

  - `/luna` - Static library that contains all of Luna's core logic, but no entry point.

  - `/lunacli` - Command line application that runs Luna beneath a UCI protocol layer.

  - `/lunatest` - Executable for unit tests.

- `/ext` - External dependencies.

- `/scripts` - Useful scripts related to testing, datagen, tuning or any other required task.

## Building Prerequisites

- C++17 compliant compiler
- CMake 3.12 or higher (build system generator)

## Building

Once you are done cloning this repository **and** all submodules located on the ```ext/``` folder, you can follow the steps below to build Luna, according to the plaftorm you're using.

- Unix based terminal + Makefile

Use one of the following command while in the root of the repository to setup the build system:

For debug builds: ```cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug```

For release builds: ```cmake -S . -B build -DCMAKE_BUILD_TYPE=Release```

After that, ```cd``` onto ```build``` and call ```make```. This will generate binaries for
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

* ```getfen``` Outputs the [FEN](https://en.wikipedia.org/wiki/Forsyth%E2%80%93Edwards_Notation) string for the current position.

* ```getpos``` Outputs the current position in a human comprehensible format.

* ```perft <depth> [--alg] [--pseudo]``` Calculates and outputs the [perft results](https://www.chessprogramming.org/Perft_Results) for the current position.
  * ```--alg``` If set, displays moves in algebraic notation (ex. e4, Nf6, O-O).
  * ```--pseudo``` If set, displays 'pseudo-legal' moves (moves that follow the patterns pieces move, but don't care if their resulting position is illegal)