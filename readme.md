## About

Luna is an alpha-beta pruning based chess engine that uses traditional evaluation methods.
It currently supports [UCI](https://en.wikipedia.org/wiki/Universal_Chess_Interface) protocol, so it is possible to embed it into a GUI.

## Prerequisites

- C++17 compliant compiler
- CMake 3.15 or higher (build system generator)

## Features

- Move generation
	- Bitboard based move-generation

- Search features
	 - Killer move heuristic
	 - PV-Node heuristic
	 - MVV-LVA ordering for captures
	 - Quiescence pickMove
	 - Iterative deepening with Transposition Tables

## Building

- Unix based terminal + Makefile

Use the following command while in the root of the repository to setup the build system:
```
cmake -S . -B builds
```

After that, ```cd``` onto ```builds``` and call ```make```. This will generate binaries for
lunachess (both the engine static library and executable) and lunatest (the Luna testing suite).

- Visual Studio 2019 

First, make sure 'C++ Cmake tools for Windows' is installed. If not, it is possible
to do so in the Visual Studio Installer.

With Visual Studio running, go to ```File > Open > Cmake...``` and select the ```CMakeLists.txt``` file
at the root of this repository. Visual Studio should then load the project accordingly, and you'll
be able to compile and run it within the IDE.

## Usage

**Play mode**

```lunachess play [optArgs...]```

Starts a chess game against Luna. Expects the user to input UCI-based chess moves.
Whenever it is Luna's turn, it will output the UCI moves it chooses.

Arguments (optional):

 - ```-depth <int>``` Specifies Luna's depth. Defaults to 5.
 - ```-side <white|black|both>``` The side Luna should play. Defaults to 'white'.
 - ```-fen <fenString>``` Specifies a [FEN](https://en.wikipedia.org/wiki/Forsyth%E2%80%93Edwards_Notation) for the initial position. Defaults to the  standard initial chess position.
 - ```--showeval``` Outputs the evaluation for the current position alongside each move it plays.
 - ```--showtime``` Outputs the time, in seconds, spent for each of Luna's moves.
 
**Evaluation mode**

```lunachess eval [optArgs...]```

Evaluates a single chess position, outputting its score and the best move according to Luna.

Arguments (optional):

 - ```-depth <int>``` Specifies Luna's depth. Defaults to 5.
 - ```-fen <fenString>``` Specifies a [FEN](https://en.wikipedia.org/wiki/Forsyth%E2%80%93Edwards_Notation) for the position to be evaluated. Defaults to the  standard initial chess position.

