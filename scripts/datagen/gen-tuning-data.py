import os.path
import random
import json
from os import listdir

import chess
import chess.pgn

# Script settings
with open("settings.json", "r") as f:
    settings = json.load(f)

    GAMES_DIR            = settings["gamesDirectory"]
    OUT_FILE             = settings["outFile"]
    MAX_POS              = settings["maxPositions"]
    MIN_PLY              = settings["minPly"]
    MAX_PLIES_BEFORE_END = settings["maxPliesBeforeEnd"]
    ENDGAME_PIECES       = settings["endgamePieces"]
    POSITIONS_PER_GAME   = settings["maxPositionsPerGame"]

pgn_paths = [f for f in listdir(GAMES_DIR) if os.path.isfile(os.path.join(GAMES_DIR, f)) and f.endswith(".pgn")]

n_pos = 0

"""
    Given a chess position, the move that led to it and its next move, returns True
    if the position should be added to the dataset; False otherwise.
"""
def accept_position(board: chess.Board, 
                    next_move: chess.Move):
    # Don't add check positions
    if board.is_check():
        return False

    # Don't add positions in which a noisy move was played
    if board.is_capture(next_move) or board.gives_check(next_move):
        return False
    
    # Don't add potentially theoretical endgames
    if (board.occupied.bit_count() - 2) <= ENDGAME_PIECES:
        return False

    # Since positions are already quiesced at the tuning stage, we don't need
    # to filter out noisy boards here.

    return True

def main():
    global n_pos
    with open(OUT_FILE, "w") as out:
        random.shuffle(pgn_paths)
        for pgn_path in pgn_paths:
            pgn_path = os.path.join(GAMES_DIR, pgn_path)
            with open(pgn_path, "r") as f:
                while n_pos < MAX_POS:
                    game = chess.pgn.read_game(f)
                    if game == None:
                        break

                    board = game.board()

                    # Get game termination
                    result_str = game.headers.get("Result")
                    if result_str == "*":
                        continue # Unfinished game
                    elif result_str == "1-0":
                        result_val = 1
                    elif result_str == "0-1":
                        result_val = 0
                    else:
                        result_val = 0.5

                    lines = []
                    for move in game.mainline_moves():
                        if accept_position(board, move):
                            lines.append(f"{board.fen()},{result_val}")

                        board.push(move)

                    # Discard opening positions and positions closer to the end
                    lines = lines[MIN_PLY:-MAX_PLIES_BEFORE_END]

                    # Sample and extracts up to POSITIONS_PER_GAME positions
                    if POSITIONS_PER_GAME < len(lines):
                        lines = random.sample(lines, POSITIONS_PER_GAME)

                    for line in lines:
                        if n_pos == MAX_POS:
                            # We're done generating positions.
                            break
                        n_pos += 1
                        out.write(f"{line}\n")
                        if (n_pos % 25000) == 0:
                            print(f"{n_pos} positions added so far.")


if __name__ == '__main__':
    main()
