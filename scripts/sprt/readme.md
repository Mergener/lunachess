# SPRT

## About

Test framework for executing SPRT (Sequential probability ratio test) with cutechess-cli.
Changes to the engine that can affect its strength can be tested here against other versions to see whether the changes contributed for gains in ELO.

Reference:

- https://en.wikipedia.org/wiki/Sequential_probability_ratio_test
- https://www.chessprogramming.org/Match_Statistics

## Requirements

- cutechess-cli
- Python 3

## Usage

Open `sprt-config.json` and adjust the base settings, especially path to engines.
After doing initial adjustments, you can customize more settings as you wish.

When your settings are all properly configured, imply execute `sprt.py` with your Python interpreter of choice.
