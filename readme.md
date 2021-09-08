## Table of Contents

*  [About](#about)

*  [Building](#building)

* [Features](#features)

*  [Usage](#usage)

## About

Luna is an alpha-beta pruning based chess engine that uses traditional evaluation methods.
It currently supports [UCI](https://en.wikipedia.org/wiki/Universal_Chess_Interface) protocol, so it is possible to embed it into a GUI.

## Features

- Move generation
	- Bitboard based move-generation

- Search features
	 - Killer move heuristic
	 - PV-Node heuristic
	 - MVV-LVA ordering for captures
	 - Quiescence search
	 - Iterative deepening with Transposition Tables

## Building
Currently, Luna only uses Microsoft Visual Studio's building system. The root folder contains the Visual Studio solution (```.sln```), and each of Luna's components has its own VS project (```.vcxproj```) within its folder.

Simply loading the Visual Studio solution onto the IDE should work.

Futurely, Luna will migrate to CMake for better cross-platform support.


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

