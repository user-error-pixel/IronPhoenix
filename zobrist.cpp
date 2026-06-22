#include <cstdint>
#include "main.h"

uint64_t zobristPiece[5][7][BOARD_SIZE];
uint64_t zobristTurn[5];

static uint64_t zobristSeed = 0x9E3779B97F4A7C15ULL;

static uint64_t splitmix64() {
    uint64_t z = (zobristSeed += 0x9E3779B97F4A7C15ULL);

    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;

    return z ^ (z >> 31);
}

void initZobrist() {
    for (int color = 0; color < 5; ++color) {
        for (int piece = 0; piece < 7; ++piece) {
            for (int sq = 0; sq < BOARD_SIZE; ++sq) {
                zobristPiece[color][piece][sq] = splitmix64();
            }
        }
    }

    for (int color = 0; color < 5; ++color) {
        zobristTurn[color] = splitmix64();
    }
}

uint64_t computeZobristKey(const Position& pos) {
    uint64_t key = 0ULL;

    for (int sq = 0; sq < BOARD_SIZE; ++sq) {
        if (!pos.isValidSquare(sq)) {
            continue;
        }

        int piece = pos.board[sq];
        int color = pos.color[sq];

        if (piece != EMPTY && color != NO_COLOR) {
            key ^= zobristPiece[color][piece][sq];
        }
    }

    key ^= zobristTurn[pos.turn];

    return key;
}