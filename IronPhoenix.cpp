#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <limits>
#include <cctype>
#include "main.h"
#include "preft.h"
#include "tt.h"
#include "search.h"

std::string squareToString(int sq) {
    if (sq < 0 || sq >= BOARD_SIZE) {
        return "??";
    }

    int row = sq / BOARD_WIDTH;
    int col = sq % BOARD_WIDTH;

    char file = static_cast<char>('a' + col - 1);
    int rank = BOARD_RANKS - row;

    return std::string(1, file) + std::to_string(rank);
}

int stringToSquare(const std::string& s) {
    if (s.size() < 2) {
        return -1;
    }

    char file = static_cast<char>(std::tolower(s[0]));
    if (file < 'a' || file > 'n') {
        return -1;
    }

    int col = file - 'a' + 1;

    int rank = 0;
    for (size_t i = 1; i < s.size(); ++i) {
        if (!std::isdigit(static_cast<unsigned char>(s[i]))) {
            return -1;
        }

        rank = rank * 10 + (s[i] - '0');
    }

    if (rank < 1 || rank > BOARD_RANKS) {
        return -1;
    }

    int row = BOARD_RANKS - rank;
    int sq = row * BOARD_WIDTH + col;

    if (sq < 0 || sq >= BOARD_SIZE) {
        return -1;
    }

    return sq;
}

std::string moveToUci(const Move& m) {
    std::string out = squareToString(m.from) + squareToString(m.to);

    if (m.isPromotion()) {
        switch (m.promotion) {
        case QUEEN:
            out += 'q';
            break;
        case ROOK:
            out += 'r';
            break;
        case BISHOP:
            out += 'b';
            break;
        case KNIGHT:
            out += 'n';
            break;
        default:
            out += 'q';
            break;
        }
    }

    return out;
}

int promotionCharToPiece(char c) {
    c = static_cast<char>(std::tolower(c));

    switch (c) {
    case 'q':
        return QUEEN;
    case 'r':
        return ROOK;
    case 'b':
        return BISHOP;
    case 'n':
        return KNIGHT;
    default:
        return QUEEN;
    }
}

bool parseMoveString(
    Position& pos,
    const std::string& text,
    Move& outMove
) {
    if (text.size() < 4) {
        return false;
    }

    size_t split = std::string::npos;

    for (size_t i = 1; i < text.size(); ++i) {
        if (std::isalpha(static_cast<unsigned char>(text[i]))) {
            split = i;
            break;
        }
    }

    if (split == std::string::npos) {
        return false;
    }

    std::string fromStr = text.substr(0, split);

    size_t promoIndex = std::string::npos;
    for (size_t i = split + 1; i < text.size(); ++i) {
        if (std::isalpha(static_cast<unsigned char>(text[i]))) {
            promoIndex = i;
            break;
        }
    }

    std::string toStr;
    char promoChar = '\0';

    if (promoIndex == std::string::npos) {
        toStr = text.substr(split);
    }
    else {
        toStr = text.substr(split, promoIndex - split);
        promoChar = text[promoIndex];
    }

    int from = stringToSquare(fromStr);
    int to = stringToSquare(toStr);

    if (from < 0 || to < 0) {
        return false;
    }

    MoveList legal;
    generateLegalMoves(pos, legal, pos.turn);

    for (int i = 0; i < legal.count; ++i) {
        Move m = legal[i];

        if (m.from != from || m.to != to) {
            continue;
        }

        if (m.isPromotion() && promoChar != '\0') {
            m.promotion = promotionCharToPiece(promoChar);
        }

        outMove = m;
        return true;
    }

    return false;
}

int sqFromCoord(char file, int rank) {
    std::string s;
    s += file;
    s += std::to_string(rank);
    return stringToSquare(s);
}

void addBackRank(
    Position& pos,
    int color,
    const char files[8],
    const int pieces[8],
    int rank
) {
    for (int i = 0; i < 8; ++i) {
        int sq = sqFromCoord(files[i], rank);
        addPiece(pos, sq, pieces[i], color);
    }
}

void addPawnLine(
    Position& pos,
    int color,
    const char files[8],
    int rank
) {
    for (int i = 0; i < 8; ++i) {
        int sq = sqFromCoord(files[i], rank);
        addPiece(pos, sq, PAWN, color);
    }
}

void initStartPosition(Position& pos) {
    pos.clear();
    initBoardValidity(pos);

    const char centerFiles[8] = {
        'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k'
    };

    {
        const int pieces[8] = {
            ROOK, KNIGHT, BISHOP, QUEEN, KING, BISHOP, KNIGHT, ROOK
        };

        addBackRank(pos, RED, centerFiles, pieces, 1);
        addPawnLine(pos, RED, centerFiles, 2);
    }

    {
        const int pieces[8] = {
            ROOK, KNIGHT, BISHOP, KING, QUEEN, BISHOP, KNIGHT, ROOK
        };

        addBackRank(pos, YELLOW, centerFiles, pieces, 14);
        addPawnLine(pos, YELLOW, centerFiles, 13);
    }

    {
        const int ranks[8] = {
            11, 10, 9, 8, 7, 6, 5, 4
        };

        const int pieces[8] = {
            ROOK, KNIGHT, BISHOP, QUEEN, KING, BISHOP, KNIGHT, ROOK
        };

        for (int i = 0; i < 8; ++i) {
            addPiece(pos, sqFromCoord('a', ranks[i]), pieces[i], BLUE);
            addPiece(pos, sqFromCoord('b', ranks[i]), PAWN, BLUE);
        }
    }

    {
        const int ranks[8] = {
            11, 10, 9, 8, 7, 6, 5, 4
        };

        const int pieces[8] = {
            ROOK, KNIGHT, BISHOP, KING, QUEEN, BISHOP, KNIGHT, ROOK
        };

        for (int i = 0; i < 8; ++i) {
            addPiece(pos, sqFromCoord('m', ranks[i]), PAWN, GREEN);
            addPiece(pos, sqFromCoord('n', ranks[i]), pieces[i], GREEN);
        }
    }

    pos.turn = RED;
    pos.ply = 0;
    pos.halfmoveClock = 0;
}

void applyMoves(Position& pos, std::istringstream& iss) {
    std::string token;

    while (iss >> token) {
        Move m;

        if (!parseMoveString(pos, token, m)) {
            std::cout << "info string illegal or unknown move: " << token << "\n";
            return;
        }

        doMove(pos, m);
    }
}

void handlePosition(Position& pos, const std::string& line) {
    std::istringstream iss(line);

    std::string token;
    iss >> token; // position

    std::string type;
    iss >> type;

    if (type == "startpos") {
        initStartPosition(pos);

        std::string next;
        if (iss >> next) {
            if (next == "moves") {
                applyMoves(pos, iss);
            }
        }
    }
    else {
        std::cout << "info string only position startpos is supported right now\n";
    }
}

int main()
{
    initZobrist();

    Position pos;

    initBoardValidity(pos);

    initStartPosition(pos);

    initPawnInfo();
    initKnightInfo(pos);
    initKingInfo(pos);

    TranspositionTable TT;
    TT.resizeMB(64);

    Search search(TT);

    std::string line;

    while (std::getline(std::cin, line)) {
        if (line == "uci") {
            std::cout << "id name IronPhoenix\n";
            std::cout << "id author Nicholas English\n";
            std::cout << "uciok\n";
        }
        else if (line == "isready") {
            std::cout << "readyok\n";
        }
        else if (line == "ucinewgame") {
            initStartPosition(pos);
        }
        else if (line.rfind("position", 0) == 0) {
            handlePosition(pos, line);
        }
        else if (line.rfind("go", 0) == 0) {
            std::istringstream iss(line);

            std::string token;
            int depth = 3;

            iss >> token; // go

            while (iss >> token) {
                if (token == "depth") {
                    iss >> depth;
                }
            }

            Move bestMove = search.findBestMove(pos, depth);

            if (bestMove.from == 0 && bestMove.to == 0) {
                std::cout << "bestmove 0000\n";
            }
            else {
                std::cout << "bestmove " << moveToUci(bestMove) << "\n";
            }
        }
        else if (line.rfind("perft", 0) == 0) {
            std::istringstream iss(line);

            std::string token;
            int depth = 1;

            iss >> token;
            iss >> depth;

            runPerft(pos, depth);
        }
        else if (line.rfind("divide", 0) == 0) {
            std::istringstream iss(line);

            std::string token;
            int depth = 1;

            iss >> token;
            iss >> depth;

            perftDivide(pos, depth);
        }
        else if (line.rfind("perftdebug", 0) == 0) {
            std::istringstream iss(line);

            std::string token;
            int depth = 1;

            iss >> token;
            iss >> depth;

            runPerftDebug(pos, depth);
        }
        else if (line == "d") {
            printBoard(pos);
        }
        else if (line == "quit") {
            break;
        }
    }

    return 0;
}