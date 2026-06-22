#include <algorithm>
#include <iostream>
#include <chrono>
#include "search.h"
#include "preft.h"

Search::Search(TranspositionTable& table)
    : tt(table),
    rootBestMove() {
}

bool Search::sameMove(const Move& a, const Move& b) const {
    return a.from == b.from
        && a.to == b.to
        && a.promotion == b.promotion
        && a.flag == b.flag;
}

std::string Search::pvToString(const PVLine& pv) const {
    std::string out;

    for (int i = 0; i < pv.length; ++i) {
        if (i > 0) {
            out += " ";
        }

        out += moveToUci(pv.moves[i]);
    }

    return out;
}

Score Search::terminalScore(const Position& pos, int color) const {
    if (inCheck(pos, color)) {
        return -MATE_SCORE + pos.ply;
    }

    return 0;
}

int Search::scoreMove(
    const Position& pos,
    const Move& move,
    const Move& ttMove
) const {
    int score = 0;

    if (sameMove(move, ttMove)) {
        return 2'000'000'000;
    }

    if (move.isCapture()) {
        score += 1'000'000;
        score += pieceValue[move.captured] * 16;
        score -= pieceValue[move.moved];
    }

    if (move.isPromotion()) {
        score += 900'000;
        score += pieceValue[move.promotion];
    }

    return score;
}

void Search::orderMoves(
    const Position& pos,
    MoveList& moves,
    const Move& ttMove
) const {
    for (int i = 0; i < moves.count - 1; ++i) {
        int bestIndex = i;
        int bestScore = scoreMove(pos, moves[i], ttMove);

        for (int j = i + 1; j < moves.count; ++j) {
            int s = scoreMove(pos, moves[j], ttMove);

            if (s > bestScore) {
                bestScore = s;
                bestIndex = j;
            }
        }

        if (bestIndex != i) {
            std::swap(moves.moves[i], moves.moves[bestIndex]);
        }
    }
}

Score Search::qsearch(
    Position& pos,
    Score alpha,
    Score beta
) {
    stats.qnodes++;

    Score standPat = lazyEvaluate(pos, pos.turn);

    if (standPat >= beta) {
        return beta;
    }

    if (standPat > alpha) {
        alpha = standPat;
    }

    MoveList moves;
    generateMoves(pos, moves, pos.turn);

    for (int i = 0; i < moves.count; ++i) {
        const Move& move = moves[i];

        if (!move.isCapture() && !move.isPromotion()) {
            continue;
        }

        const int movingColor = pos.turn;

        History h = doMove(pos, move);

        if (inCheck(pos, movingColor)) {
            undoMove(pos, move, h);
            continue;
        }

        Score score = -qsearch(pos, -beta, -alpha);

        undoMove(pos, move, h);

        if (score >= beta) {
            return beta;
        }

        if (score > alpha) {
            alpha = score;
        }
    }

    return alpha;
}

Score Search::negamax(
    Position& pos,
    Depth depth,
    Score alpha,
    Score beta,
    bool isPv,
    PVLine& pv
) {
    stats.nodes++;
    pv.clear();

    const Score oldAlpha = alpha;

    if (depth <= 0) {
        return qsearch(pos, alpha, beta);
    }

    Move ttMove;
    Score ttScore = 0;
    Score ttEval = 0;
    bool ttPv = false;

    if (tt.probe(pos, depth, alpha, beta, ttScore, ttMove, ttEval, ttPv)) {
        stats.ttHits++;
        return ttScore;
    }

    MoveList moves;
    generateLegalMoves(pos, moves, pos.turn);

    if (moves.count == 0) {
        return terminalScore(pos, pos.turn);
    }

    orderMoves(pos, moves, ttMove);

    Move bestMove = moves[0];
    Score bestScore = -INF;

    for (int i = 0; i < moves.count; ++i) {
        const Move& move = moves[i];

        PVLine childPv;

        History h = doMove(pos, move);

        Score score = -negamax(
            pos,
            depth - 1,
            -beta,
            -alpha,
            isPv && i == 0,
            childPv
        );

        undoMove(pos, move, h);

        if (score > bestScore) {
            bestScore = score;
            bestMove = move;
        }

        if (score > alpha) {
            pv.length = 1;
            pv.moves[0] = move;

            const int copyCount = std::min(childPv.length, MAX_PLY - 1);

            for (int j = 0; j < copyCount; ++j) {
                pv.moves[j + 1] = childPv.moves[j];
            }

            pv.length += copyCount;
        }

        if (alpha >= beta) {
            stats.betaCutoffs++;
            break;
        }
    }

    TTBound bound = TT_EXACT;

    if (bestScore <= oldAlpha) {
        bound = TT_UPPER;
    }
    else if (bestScore >= beta) {
        bound = TT_LOWER;
    }

    Score eval = lazyEvaluate(pos, pos.turn);

    tt.store(
        pos,
        depth,
        bestScore,
        eval,
        bound,
        bestMove,
        isPv
    );

    return bestScore;
}

Move Search::findBestMove(Position& pos, Depth maxDepth) {
    stats.clear();
    tt.newSearch();

    const auto searchStart = std::chrono::high_resolution_clock::now();

    Move bestMove;
    PVLine bestPv;
    Score bestScore = -INF;

    for (Depth depth = 1; depth <= maxDepth; ++depth) {
        MoveList moves;
        generateLegalMoves(pos, moves, pos.turn);

        if (moves.count == 0) {
            return Move();
        }

        Move ttMove;
        Score ttScore = 0;
        Score ttEval = 0;
        bool ttPv = false;

        tt.probe(pos, depth, -INF, INF, ttScore, ttMove, ttEval, ttPv);

        orderMoves(pos, moves, ttMove);

        Move depthBestMove = moves[0];

        Score alpha = -INF;
        Score beta = INF;
        Score depthBestScore = -INF;

        PVLine rootPv;
        rootPv.clear();

        for (int i = 0; i < moves.count; ++i) {
            const Move& move = moves[i];

            PVLine childPv;

            History h = doMove(pos, move);

            Score score = -negamax(
                pos,
                depth - 1,
                -beta,
                -alpha,
                i == 0,
                childPv
            );

            undoMove(pos, move, h);

            if (score > depthBestScore) {
                depthBestScore = score;
                depthBestMove = move;
            }

            if (score > alpha) {
                alpha = score;

                rootPv.length = 1;
                rootPv.moves[0] = move;

                const int copyCount = std::min(childPv.length, MAX_PLY - 1);

                for (int j = 0; j < copyCount; ++j) {
                    rootPv.moves[j + 1] = childPv.moves[j];
                }

                rootPv.length += copyCount;
            }
        }

        Score eval = lazyEvaluate(pos, pos.turn);

        tt.store(
            pos,
            depth,
            depthBestScore,
            eval,
            TT_EXACT,
            depthBestMove,
            true
        );

        bestMove = depthBestMove;
        bestScore = depthBestScore;
        bestPv = rootPv;

        const auto now = std::chrono::high_resolution_clock::now();
        const double elapsedSeconds =
            std::chrono::duration<double>(now - searchStart).count();

        const uint64_t totalNodes = stats.nodes + stats.qnodes;

        const uint64_t nps = elapsedSeconds > 0.0
            ? static_cast<uint64_t>(static_cast<double>(totalNodes) / elapsedSeconds)
            : 0ULL;

        const uint64_t timeMs =
            static_cast<uint64_t>(elapsedSeconds * 1000.0);

        std::cout << "info depth " << depth
            << " score cp " << bestScore
            << " nodes " << stats.nodes
            << " time " << timeMs
            << " nps " << nps
            << " pv " << pvToString(bestPv)
            << "\n";
    }

    rootBestMove = bestMove;
    return bestMove;
}