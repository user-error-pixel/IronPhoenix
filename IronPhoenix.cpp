#include "main.h"
#include <iostream>

int main()
{
    Position pos;

    initBoardValidity(pos);

    initPawnInfo();
    initKnightInfo(pos);
    initKingInfo(pos);

    std::cout << "Hello World!\n";
}