from os import listdir
from os.path import isfile, join

import chess
import chess.pgn

# Script settings
GAMES_DIR      = "../temp/games"
OUT_FILE       = "tunerfile2.csv"
MAX_POS        = 5000000
MIN_PLY        = 20
ENDGAME_PIECES = 4

pgn_paths = [f for f in listdir(GAMES_DIR) if isfile(join(GAMES_DIR, f)) and f.endswith(".pgn")]
n_pos = 0


"""
    Given a chess position, the move that led to it and its next move, returns True
    if the position should be added to the dataset; False otherwise.
"""
def accept_position(board: chess.Board, 
                    last_move: chess.Move, 
                    next_move: chess.Move):
    # Don't add check positions
    if board.is_check():
        return False

    board_ply = board.ply()

    # Don't add opening book positions
    if last_move == None or board_ply < MIN_PLY:
        return False
    
    # Don't add positions in which a noisy move was played
    if board.is_capture(next_move) or board.gives_check(next_move):
        return False
    
    # Don't add potentially theoretical endgames
    if (board.occupied.bit_count() - 2) <= ENDGAME_PIECES:
        return False 

    return True

with open(OUT_FILE, "w") as out:
    for pgn_path in pgn_paths:
        pgn_path = join(GAMES_DIR, pgn_path)
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

                ply = 0

                last_move = None
                for move in game.mainline_moves():
                    if n_pos == MAX_POS:
                        # We're done generating positions.
                        break

                    if accept_position(board, last_move, move):
                        line = f"{board.fen()},{result_val}"
                        out.write(f"{line}\n")
                        n_pos += 1
                        print(f"{n_pos} positions added so far.")
                    
                    last_move = move
                    board.push(move)
                    