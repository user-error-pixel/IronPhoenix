#include <sstream>
#include <vector>
#include <cctype>
#include <cstdlib>
#include "parser.h"
#include <iostream>

Parser::Parser() {
    initParser();
}

void Parser::initParser() {
    fileToCol['a'] = 1;
    fileToCol['b'] = 2;
    fileToCol['c'] = 3;
    fileToCol['d'] = 4;
    fileToCol['e'] = 5;
    fileToCol['f'] = 6;
    fileToCol['g'] = 7;
    fileToCol['h'] = 8;
    fileToCol['i'] = 9;
    fileToCol['j'] = 10;
    fileToCol['k'] = 11;
    fileToCol['l'] = 12;
    fileToCol['m'] = 13;
    fileToCol['n'] = 14;
}

std::string Parser::trim(const std::string& s) {
    size_t start = 0;
    while (start < s.size() && std::isspace(static_cast<unsigned char>(s[start]))) {
        start++;
    }

    size_t end = s.size();
    while (end > start && std::isspace(static_cast<unsigned char>(s[end - 1]))) {
        end--;
    }

    return s.substr(start, end - start);
}

int Parser::getColor(char color) const {
    switch (color) {
    case 'R':
    case 'r':
        return RED;

    case 'B':
    case 'b':
        return BLUE;

    case 'Y':
    case 'y':
        return YELLOW;

    case 'G':
    case 'g':
        return GREEN;

    case 'D':
    case 'd':
        return GREEN;

    default:
        return NO_COLOR;
    }
}

int Parser::getPieceFromToken(const std::string& token) const {
    if (token.size() < 2) {
        return EMPTY;
    }

    const char pieceChar = token[1];

    switch (pieceChar) {
    case 'P':
    case 'p':
        return PAWN;

    case 'N':
    case 'n':
        return KNIGHT;

    case 'B':
    case 'b':
        return BISHOP;

    case 'R':
    case 'r':
        return ROOK;

    case 'Q':
    case 'q':
        return QUEEN;

    case 'K':
    case 'k':
        return KING;

    default:
        return EMPTY;
    }
}

bool Parser::isPieceToken(const std::string& token) const {
    if (token.size() < 2) {
        return false;
    }

    const int color = getColor(token[0]);
    const int piece = getPieceFromToken(token);

    return color != NO_COLOR && piece != EMPTY;
}

int Parser::squareFromString(const std::string& sqText) const {
    if (sqText.size() < 2) {
        return -1;
    }

    const char file = static_cast<char>(std::tolower(static_cast<unsigned char>(sqText[0])));

    auto it = fileToCol.find(file);
    if (it == fileToCol.end()) {
        return -1;
    }

    const int col = it->second;

    int rank = 0;

    try {
        rank = std::stoi(sqText.substr(1));
    }
    catch (...) {
        return -1;
    }

    if (rank < 1 || rank > 14) {
        return -1;
    }

    const int row = 14 - rank;

    const int sq = row * MAILBOX_WIDTH + col;

    if (sq < 0 || sq >= BOARD_SIZE) {
        return -1;
    }

    if (baseMailbox[sq] == -1) {
        return -1;
    }

    return sq;
}

bool Parser::parseFen(Position& pos, const std::string& fen) {
    pos.clear();
    initBoardValidity(pos);

    const std::vector<std::string> parts = splitDashFen(fen);

    if (parts.size() < 7) {
        std::cout << "FEN parse error: expected at least 7 sections, got "
            << parts.size() << "\n";
        return false;
    }

    const int sideToMove = getColor(parts[0].empty() ? '\0' : parts[0][0]);

    if (sideToMove == NO_COLOR) {
        std::cout << "FEN parse error: invalid side to move: "
            << parts[0] << "\n";
        return false;
    }

    pos.turn = sideToMove;

    int boardPartIndex = -1;

    for (int i = 6; i < static_cast<int>(parts.size()); ++i) {
        const std::string& part = parts[i];

        const bool hasRanks = part.find('/') != std::string::npos;
        const bool hasCommas = part.find(',') != std::string::npos;

        if (hasRanks && hasCommas) {
            boardPartIndex = i;
            break;
        }
    }

    if (boardPartIndex == -1) {
        std::cout << "FEN parse error: could not find board section\n";
        return false;
    }

    if (boardPartIndex > 6) {
        const std::string& epToken = parts[6];

        if (epToken.find("enPassant") != std::string::npos) {
            parseEnPassant(pos, epToken);
        }
    }

    if (!parseBoard(pos, parts[boardPartIndex])) {
        return false;
    }

    parseCastlingRights(pos, parts[2], parts[3]);

    pos.key = computeZobristKey(pos);

    return true;
}

bool Parser::parseBoard(Position& pos, const std::string& boardToken) const {
    std::stringstream rankStream(boardToken);
    std::string rankData;

    int sq = 1;

    int row = 0;

    while (std::getline(rankStream, rankData, '/')) {
        rankData = trim(rankData);

        if (rankData.empty()) {
            continue;
        }

        std::stringstream tokenStream(rankData);
        std::string token;

        while (std::getline(tokenStream, token, ',')) {
            token = trim(token);

            if (token.empty()) {
                continue;
            }

            if (token == "x") {
                sq++;
                continue;
            }

            if (token == "X") {
                sq++;
                continue;
            }

            if (std::isdigit(static_cast<unsigned char>(token[0]))) {
                sq += std::stoi(token);
                continue;
            }

            if (isPieceToken(token)) {
                if (sq < 0 || sq >= BOARD_SIZE) {
                    std::cout << "FEN board parse error: sq out of range. sq "
                        << sq << " token " << token << "\n";
                    return false;
                }

                const int color = getColor(token[0]);
                const int piece = getPieceFromToken(token);

                if (!pos.isValidSquare(sq)) {
                    std::cout << "info string warning: piece on invalid mailbox square "
                        << sq << " token " << token
                        << "\n";
                }

                addPiece(pos, sq, piece, color);

                sq++;
                continue;
            }

            std::cout << "FEN board parse error: unknown token: "
                << token << "\n";
            return false;
        }

        sq += 2;
        row++;
    }

    if (row != BOARD_RANKS) {
        std::cout << "FEN board parse warning: expected "
            << BOARD_RANKS << " rows, got " << row << "\n";
    }

    return true;
}

bool Parser::getSquares(int& from, int& to, const std::string& uciMove) const {
    from = -1;
    to = -1;

    if (uciMove.size() < 4) {
        return false;
    }

    int index = 0;

    if (index >= static_cast<int>(uciMove.size())) {
        return false;
    }

    char fromFile = uciMove[index++];
    std::string fromRankText;

    while (index < static_cast<int>(uciMove.size()) &&
        std::isdigit(static_cast<unsigned char>(uciMove[index]))) {
        fromRankText.push_back(uciMove[index++]);
    }

    if (fromRankText.empty()) {
        return false;
    }

    if (index >= static_cast<int>(uciMove.size())) {
        return false;
    }

    char toFile = uciMove[index++];
    std::string toRankText;

    while (index < static_cast<int>(uciMove.size()) &&
        std::isdigit(static_cast<unsigned char>(uciMove[index]))) {
        toRankText.push_back(uciMove[index++]);
    }

    if (toRankText.empty()) {
        return false;
    }

    const std::string fromText = std::string(1, fromFile) + fromRankText;
    const std::string toText = std::string(1, toFile) + toRankText;

    from = squareFromString(fromText);
    to = squareFromString(toText);

    return from != -1 && to != -1;
}

void Parser::parseCastlingRights(Position& pos, const std::string& ks, const std::string& qs) const {
    auto apply = [&](const std::string& text, int side) {
        int color = RED;

        for (char c : text) {
            if (c == '0' || c == '1') {
                setCastlingRight(pos, color, side, c == '1');
                color++;

                if (color > GREEN) {
                    break;
                }
            }
        }
        };

    apply(ks, CASTLE_KINGSIDE);
    apply(qs, CASTLE_QUEENSIDE);
}

bool Parser::parseEnPassant(Position& pos, const std::string& epToken) const {
    clearEnPassant(pos);

    if (epToken.find("enPassant") == std::string::npos) {
        return true;
    }

    int color = RED;
    size_t i = 0;

    while (color <= GREEN) {
        size_t start = epToken.find('\'', i);

        if (start == std::string::npos) {
            break;
        }

        size_t end = epToken.find('\'', start + 1);

        if (end == std::string::npos) {
            break;
        }

        std::string entry = epToken.substr(start + 1, end - start - 1);

        if (entry == "enPassant") {
            i = end + 1;
            continue;
        }

        if (!entry.empty()) {
            size_t colon = entry.find(':');

            if (colon != std::string::npos) {
                std::string pawnSqText = entry.substr(0, colon);

                int pawnSq = squareFromString(pawnSqText);

                if (pawnSq != -1) {
                    pos.enPassantSq[color] = pawnSq;
                }
            }
        }

        color++;
        i = end + 1;
    }

    return true;
}

std::vector<std::string> Parser::splitDashFen(const std::string& fen) {
    std::vector<std::string> parts;
    std::string current;

    for (char c : fen) {
        if (c == '-') {
            parts.push_back(trim(current));
            current.clear();
        }
        else {
            current.push_back(c);
        }
    }

    parts.push_back(trim(current));

    return parts;
}