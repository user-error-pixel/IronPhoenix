#include <iostream>
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

char pieceToChar(int piece, int color) {
    char c = '.';

    switch (piece) {
    case PAWN:
        c = 'p';
        break;
    case KNIGHT:
        c = 'n';
        break;
    case BISHOP:
        c = 'b';
        break;
    case ROOK:
        c = 'r';
        break;
    case QUEEN:
        c = 'q';
        break;
    case KING:
        c = 'k';
        break;
    default:
        c = '.';
        break;
    }

    if (color == RED || color == YELLOW) {
        c = static_cast<char>(std::toupper(c));
    }

    return c;
}

void printBoard(const Position& pos) {
    for (int row = 0; row < BOARD_RANKS; ++row) {
        int rank = BOARD_RANKS - row;

        std::cout << rank;
        if (rank < 10) {
            std::cout << " ";
        }

        std::cout << "  ";

        for (int col = 0; col < BOARD_WIDTH; ++col) {
            int sq = row * BOARD_WIDTH + col;

            if (!pos.isValidSquare(sq)) {
                std::cout << "  ";
                continue;
            }

            if (pos.board[sq] == EMPTY) {
                std::cout << ". ";
            }
            else {
                std::cout << pieceToChar(pos.board[sq], pos.color[sq]) << " ";
            }
        }

        std::cout << "\n";
    }

    std::cout << "\n    ";

    for (int col = 0; col < BOARD_WIDTH; ++col) {
        if (col == 0 || col == 15) {
            std::cout << "  ";
        }
        else {
            char file = static_cast<char>('a' + col - 1);
            std::cout << file << " ";
        }
    }

    std::cout << "\n";

    std::cout << "Turn: ";
    switch (pos.turn) {
    case RED:
        std::cout << "Red";
        break;
    case BLUE:
        std::cout << "Blue";
        break;
    case YELLOW:
        std::cout << "Yellow";
        break;
    case GREEN:
        std::cout << "Green";
        break;
    default:
        std::cout << "Unknown";
        break;
    }

    std::cout << "\n";
}
