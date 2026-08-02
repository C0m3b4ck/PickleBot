// ---=== EVALUATION ===---
#ifndef PICKLEBOT_EVALUATE_HPP
#define PICKLEBOT_EVALUATE_HPP

#include <string>
#include <cctype>
#include "board.hpp"

// evaluation weights shared by scoring and search
const long PIECE_SCALE = 10; //material value multiplier
const long PROXIMITY_CONST = 14; //max proximity bonus

short piece_value(char c);

// breakdown of one move's evaluation, used by verbose mode
struct MoveEval
{
    short from, to;
    char piece, captured_piece;
    long capture_score;
    long proximity_score;
    long expose_penalty;
    long check_bonus;
    long final;
};

// convert a board index (0 = a8) to algebraic square name like "e4"
std::string square_name(short sq);

const char* bot_mode_name();

// combines the raw evaluation components according to the current bot personality
long weighted(long material, long proximity, long check, long expose);

// scores one move on a copy of the position; safe to call in any order
long score_move(const Board& b, short from, short to, bool botWhite, MoveEval* eval = nullptr);

// static evaluation of a position from `sideToMove`'s point of view
long evaluate(const Board& b, bool sideToMove);

// current bot personality (1-4)
extern short bot_mode;

#endif // PICKLEBOT_EVALUATE_HPP
