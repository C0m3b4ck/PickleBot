// ---=== SEARCH ===---
#ifndef PICKLEBOT_SEARCH_HPP
#define PICKLEBOT_SEARCH_HPP

#include <vector>
#include <utility>
#include <chrono>
#include <algorithm>
#include "board.hpp"
#include "evaluate.hpp"

const long INF = 1000000000;
const long MATE = 1000000;
const int PREDICT_DEPTH = 3; // depth used for predicted opponent replies

// ---=== TRANSPOSITION TABLE ===---
// caches already-searched positions so the iterative deepening loop never
// re-explores the same position at the same depth more than once
struct TTEntry
{
    uint64_t key = 0;
    long score = 0;
    short depth = 0;
    unsigned char flag = 0; // 0 = exact, 1 = lower bound, 2 = upper bound
};

const int TT_SIZE = 1 << 20; // entries (~24 MB); enough for a hobby engine
const uint64_t TT_MASK = TT_SIZE - 1;

extern TTEntry g_tt[TT_SIZE];

// deterministic 64-bit key for a position + side to move
uint64_t position_key(const Board& b, bool toMove);

// negamax with alpha-beta pruning; sets `aborted` if the time deadline is hit
long negamax(const Board& st, bool toMove, int depth, long alpha, long beta,
             const std::chrono::steady_clock::time_point& deadline, bool& aborted);

// captures/promotions-only search at the horizon, so tactics aren't cut off
long quiesce(const Board& st, bool toMove, long alpha, long beta,
             const std::chrono::steady_clock::time_point& deadline, bool& aborted);

struct SearchResult
{
    short from = -1, to = -1;
    bool timed_out = false;
    int depth = 0;
};

// searches the current global position to `depth`; `root_scores` optionally orders the root moves
SearchResult search_root(bool sideToMove, int depth,
                         const std::chrono::steady_clock::time_point& deadline,
                         const std::vector<long>* root_scores);

// after a move is applied to the board, predict the opponent's best reply
std::pair<short, short> predict_opponent_reply(bool botWhite);

void bot_move();

// ---=== GLOBAL STATE USED BY SEARCH ===---
extern Board game;
extern bool playerWhite;
extern short time_limit;
extern bool verbose_mode;
extern long bot_time_used_ms;
extern int max_search_depth; // how many plies the bot looks ahead (user-configurable)

#endif // PICKLEBOT_SEARCH_HPP
