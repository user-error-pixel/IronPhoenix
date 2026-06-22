#pragma once

#include "main.h"
#include <iostream>
#include <chrono>

using U64 = unsigned long long;

std::string moveToUci(const Move& m);

inline U64 perft(Position& pos, int depth) {
    if (depth == 0) {
        return 1ULL;
    }

    MoveList moves;
    generateLegalMoves(pos, moves, pos.turn);

    if (depth == 1) {
        return static_cast<U64>(moves.count);
    }

    U64 nodes = 0ULL;

    for (int i = 0; i < moves.count; ++i) {
        const Move& m = moves[i];

        History h = doMove(pos, m);
        nodes += perft(pos, depth - 1);
        undoMove(pos, m, h);
    }

    return nodes;
}

inline void perftDivide(Position& pos, int depth) {
    MoveList moves;
    generateLegalMoves(pos, moves, pos.turn);

    U64 total = 0ULL;

    std::cout << "Perft divide depth " << depth << "\n";

    for (int i = 0; i < moves.count; ++i) {
        const Move& m = moves[i];

        History h = doMove(pos, m);

        U64 nodes = 0ULL;

        if (depth <= 1) {
            nodes = 1ULL;
        }
        else {
            nodes = perft(pos, depth - 1);
        }

        undoMove(pos, m, h);

        total += nodes;

        std::cout << moveToUci(m) << ": " << nodes << "\n";
    }

    std::cout << "Total: " << total << "\n";
}

inline void runPerft(Position& pos, int depth) {
    auto start = std::chrono::high_resolution_clock::now();

    U64 nodes = perft(pos, depth);

    auto end = std::chrono::high_resolution_clock::now();

    double seconds = std::chrono::duration<double>(end - start).count();
    double nps = seconds > 0.0 ? static_cast<double>(nodes) / seconds : 0.0;

    std::cout << "perft depth " << depth << "\n";
    std::cout << "nodes " << nodes << "\n";
    std::cout << "time " << seconds << " sec\n";
    std::cout << "nps " << static_cast<U64>(nps) << "\n";
}

inline bool validatePosition(const Position& pos) {
    int counted[5] = { 0, 0, 0, 0, 0 };

    for (int sq = 0; sq < BOARD_SIZE; ++sq) {
        if (!pos.isValidSquare(sq)) {
            continue;
        }

        int piece = pos.board[sq];
        int color = pos.color[sq];

        if (piece == EMPTY) {
            if (color != NO_COLOR) {
                std::cout << "Bad color on empty square " << sq << "\n";
                return false;
            }

            if (pos.pieceIndex[sq] != -1) {
                std::cout << "Bad pieceIndex on empty square " << sq << "\n";
                return false;
            }

            continue;
        }

        if (color < RED || color > GREEN) {
            std::cout << "Bad color on occupied square " << sq << "\n";
            return false;
        }

        counted[color]++;

        int index = pos.pieceIndex[sq];

        if (index < 0 || index >= pos.pieceCount[color]) {
            std::cout << "Bad pieceIndex at square " << sq << "\n";
            return false;
        }

        if (pos.pieceList[color][index] != sq) {
            std::cout << "pieceList mismatch at square " << sq << "\n";
            return false;
        }

        if (piece == KING && pos.kingSq[color] != sq) {
            std::cout << "Bad kingSq for color " << color << "\n";
            return false;
        }
    }

    for (int color = RED; color <= GREEN; ++color) {
        if (counted[color] != pos.pieceCount[color]) {
            std::cout << "pieceCount mismatch color " << color
                << " counted " << counted[color]
                << " stored " << pos.pieceCount[color]
                << "\n";

            return false;
        }
    }

    return true;
}

inline U64 perftDebug(Position& pos, int depth) {
    if (!validatePosition(pos)) {
        std::cout << "Invalid position at perft depth " << depth << "\n";
        return 0ULL;
    }

    if (depth == 0) {
        return 1ULL;
    }

    MoveList moves;
    generateLegalMoves(pos, moves, pos.turn);

    if (depth == 1) {
        return static_cast<U64>(moves.count);
    }

    U64 nodes = 0ULL;

    for (int i = 0; i < moves.count; ++i) {
        const Move& m = moves[i];

        History h = doMove(pos, m);

        if (!validatePosition(pos)) {
            std::cout << "Invalid after move " << moveToUci(m) << "\n";
            undoMove(pos, m, h);
            return 0ULL;
        }

        nodes += perftDebug(pos, depth - 1);

        undoMove(pos, m, h);

        if (!validatePosition(pos)) {
            std::cout << "Invalid after undo " << moveToUci(m) << "\n";
            return 0ULL;
        }
    }

    return nodes;
}

inline void runPerftDebug(Position& pos, int depth) {
    auto start = std::chrono::high_resolution_clock::now();

    U64 nodes = perftDebug(pos, depth);

    auto end = std::chrono::high_resolution_clock::now();

    double seconds = std::chrono::duration<double>(end - start).count();
    double nps = seconds > 0.0 ? static_cast<double>(nodes) / seconds : 0.0;

    std::cout << "debug perft depth " << depth << "\n";
    std::cout << "nodes " << nodes << "\n";
    std::cout << "time " << seconds << " sec\n";
    std::cout << "nps " << static_cast<U64>(nps) << "\n";
}