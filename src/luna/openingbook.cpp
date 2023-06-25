#include "openingbook.h"

#include "utils.h"

namespace lunachess {

static OpeningBook* s_DefaultOpBook = nullptr;

static OpeningBook generateDefaultOpBook() {
    OpeningBookBuilder builder;
    Position pos = Position::getInitialPosition();

    // King's pawn
    builder.pushAndAdd("e2e4");
    {
        // Symmetrical
        builder.pushAndAdd("e7e5");
        {
            // King's knight
            builder.pushAndAdd("g1f3");
            {
                // ... Nc6
                builder.pushAndAdd("b8c6");
                {
                    // Ruy Lopez
                    builder.pushAndAdd("f1b5");
                    {
                        // Main line
                        builder.pushAndAdd("a7a6");
                        {
                            builder.pushAndAdd("b5a4");
                            {
                                builder.add("b7b5");
                                builder.add("g8f6");
                                builder.add("f8e7");
                            }
                            builder.pop();
                        }
                        builder.pop();

                        // Berlin
                        builder.add("g8f6");
                    }
                    builder.pop();

                    // Italian
                    builder.pushAndAdd("f1c4");
                    {
                        builder.add("f8c5");
                        builder.pushAndAdd("g8f6");
                        {
                            builder.add("d2d3");
                        }
                        builder.pop();
                    }
                    builder.pop();

                    // Two knights
                    builder.add("b1c3");
                }
                builder.pop();
                builder.add("g8f6");
            }
            builder.pop();
            builder.add("b1c3");
        }
        builder.pop();

        // Sicilian
        builder.pushAndAdd("c7c5");
        {
            // Open Sicilian
            builder.pushAndAdd("g1f3");
            {
                // Main line
                builder.pushAndAdd("d7d6");
                {
                    builder.pushAndAdd("d2d4");
                    {
                        builder.pushAndAdd("c5d4");
                        {
                            builder.pushAndAdd("f3d4");
                            {
                                builder.pushAndAdd("g8f6");
                                {
                                    builder.pushAndAdd("b1c3");
                                    {
                                        // Classical variation
                                        builder.add("b8c6");

                                        // Najdorf variation
                                        builder.add("a7a6");

                                        // Kupreichik variation
                                        builder.add("c8d7");

                                        // Dragon variation
                                        builder.add("g7g6");
                                    }
                                    builder.pop();
                                }
                                builder.pop();
                            }
                            builder.pop();
                        }
                        builder.pop();
                    }
                    builder.pop();
                }
                builder.pop();

                builder.add("e7e6");

                builder.add("b8c6");
            }
            builder.pop();
        }
        builder.pop();

        // Caro-Kann
        builder.pushAndAdd("c7c6");
        {
            builder.pushAndAdd("d2d4");
            {
                builder.pushAndAdd("d7d5");
                {
                    // Advance Caro-Kann
                    builder.pushAndAdd("e4e5");
                    {
                        builder.pushAndAdd("c8f5");
                        {
                            builder.add("h2h4");
                            builder.add("c2c3");
                            builder.add("c2c4");
                            builder.add("b1d2");
                            builder.add("g1f3");
                        }
                        builder.pop();
                    }
                    builder.pop();
                }
                builder.pop();
            }
            builder.pop();

            builder.pushAndAdd("g1f3");
            {
                builder.pushAndAdd("d7d5");
                {
                    builder.pushAndAdd("e4e5");
                    {
                        builder.pushAndAdd("c8f5");
                        {
                            builder.add("c2c3");
                            builder.add("d2d4");
                            builder.add("a2a3");
                            builder.add("f1e2");
                        }
                        builder.pop();

                        builder.pushAndAdd("c8g4");
                        {
                            builder.add("c2c3");
                            builder.add("d2d4");
                            builder.add("f1e2");
                        }
                        builder.pop();
                    }
                    builder.pop();
                }
                builder.pop();
            }
            builder.pop();
        }
        builder.pop();

        // French
        builder.pushAndAdd("e7e6");
        {
            builder.pushAndAdd("d2d4");
            {
                builder.pushAndAdd("d7d5");
                {
                    // Classical
                    builder.add("b1c3");

                    // Advance
                    builder.add("e4e5");

                    // Exchange
                    builder.add("e4d5");

                    // Tarrasch
                    builder.add("b1d2");
                }
                builder.pop();

                // Franco-Sicilian
                builder.add("c7c5");
            }
            builder.pop();

            builder.add("g1f3");
        }
        builder.pop();

        // Modern
        builder.pushAndAdd("g7g6");
        {
            builder.add("d2d4");
        }
        builder.pop();
    }
    builder.pop();

    // Queen's pawn
    builder.pushAndAdd("d2d4");
    {
        // Symmetrical
        builder.pushAndAdd("d7d5");
        {
            // Queen's gambit
            builder.pushAndAdd("c2c4");
            {
                // QGD
                builder.pushAndAdd("e7e6");
                {
                    builder.add("b1c3");
                    builder.pushAndAdd("g1f3");
                    {
                        builder.pushAndAdd("g8f6");
                        {
                            builder.add("b1c3");

                            // Catalan
                            builder.add("g2g3");
                        }
                        builder.pop();
                    }
                    builder.pop();
                }
                builder.pop();

                // Slav
                builder.add("c7c6");
            }
            builder.pop();

            // London system
            builder.add("c1f4");
            builder.add("g1f3");
        }
        builder.pop();

        // Indian game
        builder.pushAndAdd("g8f6");
        {
            builder.pushAndAdd("c2c4");
            {
                builder.pushAndAdd("e7e6");
                {
                    builder.pushAndAdd("b1c3");
                    {
                        // Nimzo-indian
                        builder.add("f8b4");
                    }
                    builder.pop();

                    builder.add("g1f3");
                }
                builder.pop();
            }
            builder.pop();
        }
        builder.pop();
    }
    builder.pop();

    // English Opening
    builder.pushAndAdd("c2c4");
    builder.pop();

    // Reti Opening
    builder.pushAndAdd("g1f3");
    builder.pop();

    return builder.get();
}

const OpeningBook& OpeningBook::getDefault() {
    if (s_DefaultOpBook == nullptr) {
        s_DefaultOpBook = new OpeningBook(generateDefaultOpBook());
    }
    return *s_DefaultOpBook;
}

Move OpeningBook::getRandomMoveForPosition(ui64 posKey) const {
    auto it = m_Moves.find(posKey);
    if (it == m_Moves.end()) {
        return MOVE_INVALID;
    }
    auto& vec = it->second;
    return vec[utils::random(C64(0), vec.size())];
}

void OpeningBook::addMove(ui64 posKey, Move move) {
    auto& vec = m_Moves[posKey];
    vec.push_back(move);
}

void OpeningBook::deleteMove(ui64 posKey, Move move) {
    auto it = m_Moves.find(posKey);
    if (it == m_Moves.end()) {
        // Vector not found.
        return;
    }
    auto& vec = it->second;
    auto remIt = std::find(vec.begin(), vec.end(), move);
    if (remIt != vec.end()) {
        // Only delete if move was found in vector
        vec.erase(remIt);
    }

    if (vec.empty()) {
        // Move vector was empty, delete its entry.
        m_Moves.erase(it);
    }
}

void OpeningBook::clearMovesFromPos(ui64 posKey) {
    m_Moves.erase(posKey);
}

void OpeningBook::clear() {
    m_Moves.clear();
}

}