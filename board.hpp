// ---=== BOARD ===---
// a position: piece placement, en passant target, castling rights
#ifndef PICKLEBOT_BOARD_HPP
#define PICKLEBOT_BOARD_HPP

#include <vector>
#include <utility>
#include <cctype>

struct Board
{
    std::vector<char> sq;
    short ep = -1; //square a pawn can be captured en passant on, -1 if none
    bool wks = true; //white kingside castling available
    bool wqs = true; //white queenside castling available
    bool bks = true; //black kingside castling available
    bool bqs = true; //black queenside castling available
};

// home-rank squares (a8=0 ... h1=63)
const short A1 = 56, B1 = 57, C1 = 58, D1 = 59, E1 = 60, F1 = 61, G1 = 62, H1 = 63;
const short A8 = 0, B8 = 1, C8 = 2, D8 = 3, E8 = 4, F8 = 5, G8 = 6, H8 = 7;

// ---=== BOARD HELPERS ===---
bool is_white_piece(char c);
bool is_player_piece(char c, bool white);
bool is_enemy_piece(char c, bool white);

// ---=== MOVE LOGIC ===---
bool legal_move(const Board& b, short from, short to, bool white);
void make_move(Board& b, short from, short to, bool white);
bool square_attacked(const Board& b, short sq, bool by_white);
bool king_in_check(const Board& b, bool white);
bool try_move(const Board& b, short from, short to, bool white, Board& out);

// ---=== MOVE GENERATION ===---
std::vector<std::pair<short, short>> generate_legal_moves(const Board& b, bool white);
std::vector<std::pair<short, short>> get_legal_moves(bool white);

// current global position
extern Board game;

#endif // PICKLEBOT_BOARD_HPP
