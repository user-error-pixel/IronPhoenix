#include <iostream>
#include "main.h"
#include "preft.h"

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

const char* pieceName(int piece) {
    switch (piece) {
    case EMPTY:  return "EMPTY";
    case PAWN:   return "PAWN";
    case KNIGHT: return "KNIGHT";
    case BISHOP: return "BISHOP";
    case ROOK:   return "ROOK";
    case QUEEN:  return "QUEEN";
    case KING:   return "KING";
    case WALL:   return "WALL";
    default:     return "UNKNOWN";
    }
}

const char* colorName(int color) {
    switch (color) {
    case NO_COLOR: return "NO_COLOR";
    case RED:      return "RED";
    case BLUE:     return "BLUE";
    case YELLOW:   return "YELLOW";
    case GREEN:    return "GREEN";
    default:       return "UNKNOWN";
    }
}

std::string flagString(const Move& move) {
    if (move.flag == QUIET) {
        return "QUIET";
    }

    std::string out;

    if (move.flag & CAPTURE) {
        out += "CAPTURE|";
    }

    if (move.flag & PROMOTION) {
        out += "PROMOTION|";
    }

    if (move.flag & PROMOTION_CAPTURE) {
        out += "PROMOTION_CAPTURE|";
    }

    if (move.flag & CASTLE) {
        out += "CASTLE|";
    }

    if (move.flag & EP_CAPTURE) {
        out += "EP_CAPTURE|";
    }

    if (!out.empty()) {
        out.pop_back(); // remove last |
    }

    return out.empty() ? "UNKNOWN_FLAG" : out;
}

void printMove(const Move& move) {
    std::cout << moveToUci(move);
}

void printMoveDetailed(const Move& move) {
    std::cout << "Move: " << moveToUci(move) << "\n";

    std::cout << "  from: " << move.from << "\n";
    std::cout << "  to: " << move.to << "\n";

    std::cout << "  moved: " << pieceName(move.moved)
        << " (" << static_cast<int>(move.moved) << ")\n";

    std::cout << "  movedColor: " << colorName(move.movedColor)
        << " (" << static_cast<int>(move.movedColor) << ")\n";

    std::cout << "  captured: " << pieceName(move.captured)
        << " (" << static_cast<int>(move.captured) << ")\n";

    std::cout << "  capturedColor: " << colorName(move.capturedColor)
        << " (" << static_cast<int>(move.capturedColor) << ")\n";

    std::cout << "  promotion: " << pieceName(move.promotion)
        << " (" << static_cast<int>(move.promotion) << ")\n";

    std::cout << "  flag: " << flagString(move)
        << " (" << static_cast<int>(move.flag) << ")\n";

    std::cout << "  isCapture: " << move.isCapture() << "\n";
    std::cout << "  isPromotion: " << move.isPromotion() << "\n";
    std::cout << "  isQuiet: " << move.isQuiet() << "\n";
}