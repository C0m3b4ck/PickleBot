// ---=== BOARD ===---
#include "board.hpp"

bool is_white_piece(char c)
{
    return c >= 'A' && c <= 'Z';
}

bool is_player_piece(char c, bool white)
{
    if (c == ' ') return false;
    return is_white_piece(c) == white;
}

bool is_enemy_piece(char c, bool white)
{
    if (c == ' ') return false;
    return is_white_piece(c) != white;
}

bool legal_move(const Board& b, short from, short to, bool white)
{
    char piece = b.sq[from];
    if (piece == ' ') return false;
    if (is_player_piece(b.sq[to], white)) return false;

    short fr = from / 8, fc = from % 8;
    short tr = to / 8, tc = to % 8;
    short dr = tr - fr, dc = tc - fc;
    short sdr = (dr > 0) - (dr < 0);
    short sdc = (dc > 0) - (dc < 0);
    short adr = dr > 0 ? dr : -dr;
    short adc = dc > 0 ? dc : -dc;

    switch (toupper(piece))
    {
        case 'P': // pawn
        {
            short step = white ? -1 : 1;
            short startrow = white ? 6 : 1;
            if (dc == 0) // forward
            {
                if (dr == step && b.sq[to] == ' ') return true;
                if (fr == startrow && dr == 2 * step && b.sq[to] == ' '
                    && b.sq[from + step * 8] == ' ') return true;
                return false;
            }
            else // capture
            {
                if (adr == 1 && adc == 1 && dr == step)
                {
                    if (is_enemy_piece(b.sq[to], white)) return true;
                    // en passant capture onto the target square
                    if (to == b.ep && is_enemy_piece(b.sq[fr * 8 + tc], white)) return true;
                }
                return false;
            }
        }
        case 'R':
            if (fr != tr && fc != tc) return false;
            break;
        case 'B':
            if (adr != adc) return false;
            break;
        case 'Q':
            if (fr != tr && fc != tc && adr != adc) return false;
            break;
        case 'K':
            if (adr <= 1 && adc <= 1 && (adr + adc > 0)) return true;
            // castling: king slides two squares towards its own rook
            // white king on e1, black king on e8
            if (white)
            {
                if (from == E1 && to == G1 && b.wks) // e1-g1
                {
                    if (b.sq[F1] != ' ' || b.sq[G1] != ' ') return false;
                    if (square_attacked(b, E1, false) || square_attacked(b, F1, false)
                        || square_attacked(b, G1, false)) return false;
                    return true;
                }
                if (from == E1 && to == C1 && b.wqs) // e1-c1
                {
                    if (b.sq[D1] != ' ' || b.sq[C1] != ' ' || b.sq[B1] != ' ') return false;
                    if (square_attacked(b, E1, false) || square_attacked(b, D1, false)
                        || square_attacked(b, C1, false)) return false;
                    return true;
                }
            }
            else
            {
                if (from == E8 && to == G8 && b.bks) // e8-g8
                {
                    if (b.sq[F8] != ' ' || b.sq[G8] != ' ') return false;
                    if (square_attacked(b, E8, true) || square_attacked(b, F8, true)
                        || square_attacked(b, G8, true)) return false;
                    return true;
                }
                if (from == E8 && to == C8 && b.bqs) // e8-c8
                {
                    if (b.sq[D8] != ' ' || b.sq[C8] != ' ' || b.sq[B8] != ' ') return false;
                    if (square_attacked(b, E8, true) || square_attacked(b, D8, true)
                        || square_attacked(b, C8, true)) return false;
                    return true;
                }
            }
            return false;
        case 'N':
            return (adr == 2 && adc == 1) || (adr == 1 && adc == 2);
        default:
            return false;
    }

    // sliding piece - check path is not obstructed
    short r = fr + sdr, c = fc + sdc;
    while (r != tr || c != tc)
    {
        if (b.sq[r * 8 + c] != ' ') return false;
        r += sdr;
        c += sdc;
    }
    return true;
}

// applies a move to a board and updates en passant target and castling rights
void make_move(Board& b, short from, short to, bool white)
{
    char moving = b.sq[from];
    bool ep_capture = (to == b.ep) && toupper(moving) == 'P' && from % 8 != to % 8;
    short cap_sq = ep_capture ? from / 8 * 8 + to % 8 : to;
    char captured = b.sq[cap_sq];

    // castling: the rook jumps across the king (e-file to g/c file)
    if (toupper(moving) == 'K' && (to - from == 2 || to - from == -2))
    {
        short row = from / 8;
        if (to - from == 2) // kingside: rook from h-file to f-file
        {
            b.sq[from + 1] = b.sq[row * 8 + 7];
            b.sq[row * 8 + 7] = ' ';
        }
        else // queenside: rook from a-file to d-file
        {
            b.sq[from - 1] = b.sq[row * 8];
            b.sq[row * 8] = ' ';
        }
    }

    b.sq[to] = moving;
    b.sq[from] = ' ';
    if (ep_capture) b.sq[cap_sq] = ' ';

    // update castling rights
    if (toupper(moving) == 'K')
    {
        if (white) { b.wks = false; b.wqs = false; }
        else { b.bks = false; b.bqs = false; }
    }
    else if (moving == 'R' || moving == 'r')
    {
        if (from == H1) b.wks = false;
        if (from == A1) b.wqs = false;
        if (from == H8) b.bks = false;
        if (from == A8) b.bqs = false;
    }
    if (ep_capture)
    {
        if (cap_sq == H1) b.wks = false;
        if (cap_sq == A1) b.wqs = false;
        if (cap_sq == H8) b.bks = false;
        if (cap_sq == A8) b.bqs = false;
    }
    else if (captured == 'R')
    {
        if (to == H1) b.wks = false;
        if (to == A1) b.wqs = false;
    }
    else if (captured == 'r')
    {
        if (to == H8) b.bks = false;
        if (to == A8) b.bqs = false;
    }

    // update en passant target after a two-square pawn push
    b.ep = -1;
    if (toupper(moving) == 'P' && (to / 8 - from / 8 == 2 || to / 8 - from / 8 == -2))
        b.ep = (from / 8 + to / 8) / 2 * 8 + to % 8;
}

// is square `sq` attacked by a piece of color `by_white`?
bool square_attacked(const Board& b, short sq, bool by_white)
{
    short r = sq / 8, c = sq % 8;

    // pawns attack from one row forward of the attacker's perspective
    char pawn = by_white ? 'P' : 'p';
    short dir = by_white ? 1 : -1; // white pawn attacks squares one row up, so source is one row down
    short pr = r + dir;
    if (pr >= 0 && pr < 8)
    {
        if (c - 1 >= 0 && b.sq[pr * 8 + (c - 1)] == pawn) return true;
        if (c + 1 < 8 && b.sq[pr * 8 + (c + 1)] == pawn) return true;
    }

    // knights
    char knight = by_white ? 'N' : 'n';
    static const short knight_dr[8] = {-2,-2,-1,-1,1,1,2,2};
    static const short knight_dc[8] = {-1,1,-2,2,-2,2,-1,1};
    for (short i = 0; i < 8; i++)
    {
        short nr = r + knight_dr[i], nc = c + knight_dc[i];
        if (nr >= 0 && nr < 8 && nc >= 0 && nc < 8 && b.sq[nr * 8 + nc] == knight) return true;
    }

    // adjacent king
    char king = by_white ? 'K' : 'k';
    for (short dr = -1; dr <= 1; dr++)
        for (short dc = -1; dc <= 1; dc++)
        {
            if (dr == 0 && dc == 0) continue;
            short nr = r + dr, nc = c + dc;
            if (nr >= 0 && nr < 8 && nc >= 0 && nc < 8 && b.sq[nr * 8 + nc] == king) return true;
        }

    // sliding rays: 0=N 1=S 2=W 3=E 4=NW 5=NE 6=SW 7=SE
    static const short ray_dr[8] = {-1,1,0,0,-1,-1,1,1};
    static const short ray_dc[8] = {0,0,-1,1,-1,1,-1,1};
    for (short d = 0; d < 8; d++)
    {
        short nr = r + ray_dr[d], nc = c + ray_dc[d];
        while (nr >= 0 && nr < 8 && nc >= 0 && nc < 8)
        {
            char p = b.sq[nr * 8 + nc];
            if (p != ' ')
            {
                if (is_white_piece(p) == by_white)
                {
                    char u = toupper(p);
                    if (d < 4) // orthogonal: rook or queen
                    {
                        if (u == 'R' || u == 'Q') return true;
                    }
                    else // diagonal: bishop or queen
                    {
                        if (u == 'B' || u == 'Q') return true;
                    }
                }
                break;
            }
            nr += ray_dr[d];
            nc += ray_dc[d];
        }
    }
    return false;
}

bool king_in_check(const Board& b, bool white)
{
    char king = white ? 'K' : 'k';
    for (short i = 0; i < 64; i++)
    {
        if (b.sq[i] == king) return square_attacked(b, i, !white);
    }
    return true; // no king found - treat as check
}

// applies from->to to a copy of `b`; returns true if the move is legal
// (movement rules hold and the mover's king is not left in check).
// The result is left in `out`.
bool try_move(const Board& b, short from, short to, bool white, Board& out)
{
    if (!legal_move(b, from, to, white)) return false;
    out = b;
    make_move(out, from, to, white);
    return !king_in_check(out, white);
}

std::vector<std::pair<short, short>> generate_legal_moves(const Board& b, bool white)
{
    std::vector<std::pair<short, short>> moves;
    Board out;
    for (short from = 0; from < 64; from++)
    {
        if (!is_player_piece(b.sq[from], white)) continue;
        for (short to = 0; to < 64; to++)
        {
            if (from == to) continue;
            if (!legal_move(b, from, to, white)) continue;
            // reject moves that leave own king in check
            if (try_move(b, from, to, white, out)) moves.push_back({from, to});
        }
    }
    return moves;
}

std::vector<std::pair<short, short>> get_legal_moves(bool white)
{
    return generate_legal_moves(game, white);
}
