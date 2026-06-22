#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <limits>
#include <cctype>
#include "main.h"

int main()
{
    Position pos;

    initBoardValidity(pos);

    initPawnInfo();
    initKnightInfo(pos);
    initKingInfo(pos);

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
			// Reset the position to the initial state for a new game.
        }
        else if (line.rfind("position", 0) == 0) {
			// Handle position setup commands, such as "position startpos" or "position fen ...".
        }
        else if (line.rfind("go", 0) == 0) {
            std::istringstream iss(line);

            std::string token;
            int depth = 3;

            iss >> token;

            while (iss >> token) {
                if (token == "depth") {
                    iss >> depth;
                }
            }

			// Call the search function to find the best move for the current position and depth.
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