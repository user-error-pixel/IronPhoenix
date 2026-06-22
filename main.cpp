#include "main.h"

PawnMoveInfo pawnInfo[5];

int knightMoveCount[BOARD_SIZE];
int knightMoves[BOARD_SIZE][8];

int kingMoveCount[BOARD_SIZE];
int kingMoves[BOARD_SIZE][8];

void initPawnInfo() {
    pawnInfo[RED] = {
        DIR_N,
        DIR_NW,
        DIR_NE
    };

    pawnInfo[YELLOW] = {
        DIR_S,
        DIR_SE,
        DIR_SW
    };

    pawnInfo[BLUE] = {
        DIR_E,
        DIR_NE,
        DIR_SE
    };

    pawnInfo[GREEN] = {
        DIR_W,
        DIR_SW,
        DIR_NW
    };
}

void initKnightInfo(const Position& pos) {
    for (int sq = 0; sq < BOARD_SIZE; ++sq) {
        knightMoveCount[sq] = 0;

        if (!pos.isValidSquare(sq)) {
            continue;
        }

        for (int i = 0; i < 8; ++i) {
            const int to = sq + knightOffsets[i];

            if (!pos.isValidSquare(to)) {
                continue;
            }

            knightMoves[sq][knightMoveCount[sq]++] = to;
        }
    }
}

void initKingInfo(const Position& pos) {
    for (int sq = 0; sq < BOARD_SIZE; ++sq) {
        kingMoveCount[sq] = 0;

        if (!pos.isValidSquare(sq)) {
            continue;
        }

        for (int i = 0; i < 8; ++i) {
            const int to = sq + kingOffsets[i];

            if (!pos.isValidSquare(to)) {
                continue;
            }

            kingMoves[sq][kingMoveCount[sq]++] = to;
        }
    }
}