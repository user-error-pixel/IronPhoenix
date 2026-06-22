#pragma once
#include <string>
#include <unordered_map>
#include "main.h"

class Parser {
public:
    Parser();

    int squareFromString(const std::string& sqText) const;

    bool parseFen(Position& pos, const std::string& fen);

    bool getSquares(int& from, int& to, const std::string& uciMove) const;

private:
    static std::vector<std::string> splitDashFen(const std::string& fen);

    std::unordered_map<char, int> fileToCol;

    void initParser();

    int getColor(char color) const;
    int getPieceFromToken(const std::string& token) const;
    bool isPieceToken(const std::string& token) const;

    bool parseBoard(Position& pos, const std::string& boardToken) const;

    static std::string trim(const std::string& s);
};