// ---=== BOT LOGIC / EVALUATION ===---
#include "evaluate.hpp"

short piece_value(char c)
{
    switch (toupper(c))
    {
        case 'P': return 1;
        case 'N':
        case 'B': return 3;
        case 'R': return 5;
        case 'Q': return 9;
        case 'K': return 100;
        default: return 0;
    }
}

// convert a board index (0 = a8) to algebraic square name like "e4"
std::string square_name(short sq)
{
    std::string s(2, ' ');
    s[0] = 'a' + sq % 8;
    s[1] = '0' + (8 - sq / 8);
    return s;
}

const char* bot_mode_name()
{
    switch (bot_mode)
    {
        case 1: return "Aggressive (material)";
        case 2: return "Offensive (focus on king)";
        case 3: return "Defensive (material)";
        case 4: return "Guarding (defensive focus on king)";
        default: return "Unknown";
    }
}

// combines the raw evaluation components according to the current bot personality
long weighted(long material, long proximity, long check, long expose)
{
    switch (bot_mode)
    {
        case 1: return material * 3 + proximity + check * 10 - expose * 20;
        case 2: return material + proximity * 3 + check * 40 - expose * 20;
        case 3: return material * 3 - expose * 50 + check * 5;
        case 4: return material - expose * 70 + check * 10;
        default: return material + proximity + check * 10 - expose * 20;
    }
}

// scores one move on a copy of the position; safe to call in any order
long score_move(const Board& b, short from, short to, bool botWhite, MoveEval* eval)
{
    char moving = b.sq[from];
    char captured = b.sq[to];
    if (to == b.ep && toupper(moving) == 'P' && from % 8 != to % 8)
        captured = b.sq[from / 8 * 8 + to % 8]; // en passant

    Board out;
    try_move(b, from, to, botWhite, out);

    // 1. capture material
    long capture_score = piece_value(captured) * PIECE_SCALE;

    // 2. how many moves to attack the enemy king (piece proximity)
    long proximity_score = 0;
    char enemy_king = botWhite ? 'K' : 'k';
    short kingpos = -1;
    for (short i = 0; i < 64; i++)
    {
        if (out.sq[i] == enemy_king)
        {
            kingpos = i;
            break;
        }
    }
    if (kingpos >= 0)
    {
        short dr = kingpos / 8 - to / 8;
        short dc = kingpos % 8 - to % 8;
        dr = dr > 0 ? dr : -dr;
        dc = dc > 0 ? dc : -dc;
        proximity_score = PROXIMITY_CONST - (dr + dc);
    }

    // 3. does the move expose the bot's king
    long expose_penalty = king_in_check(out, botWhite) ? 1 : 0;

    // 4. does the move check the enemy king
    long check_bonus = king_in_check(out, !botWhite) ? 1 : 0;

    long final = weighted(capture_score, proximity_score, check_bonus, expose_penalty);
    if (eval)
    {
        eval->from = from;
        eval->to = to;
        eval->piece = moving;
        eval->captured_piece = captured;
        eval->capture_score = capture_score;
        eval->proximity_score = proximity_score;
        eval->expose_penalty = expose_penalty;
        eval->check_bonus = check_bonus;
        eval->final = final;
    }
    return final;
}

// static evaluation of a position from `sideToMove`'s point of view
long evaluate(const Board& b, bool sideToMove)
{
    long mat = 0;
    short enemy_king = -1;
    for (short i = 0; i < 64; i++)
    {
        char c = b.sq[i];
        if (c == ' ') continue;
        bool mine = is_white_piece(c) == sideToMove;
        long v = piece_value(c) * PIECE_SCALE;
        mat += mine ? v : -v;
        if (!mine && toupper(c) == 'K') enemy_king = i;
    }
    // how close our pieces are to the enemy king
    long proximity = 0;
    if (enemy_king >= 0)
    {
        for (short i = 0; i < 64; i++)
        {
            char c = b.sq[i];
            if (c == ' ' || is_white_piece(c) != sideToMove) continue;
            short dr = enemy_king / 8 - i / 8;
            short dc = enemy_king % 8 - i % 8;
            dr = dr > 0 ? dr : -dr;
            dc = dc > 0 ? dc : -dc;
            proximity += PROXIMITY_CONST - (dr + dc);
        }
    }
    long my_check = king_in_check(b, sideToMove) ? 1 : 0;
    long enemy_check = king_in_check(b, !sideToMove) ? 1 : 0;
    return weighted(mat, proximity, enemy_check, my_check);
}
