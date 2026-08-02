#include <iostream>
#include <string>
#include <vector>
#include <cctype>
#include <algorithm>
#include <climits>
#include <cmath>
#include <thread>
#include <chrono>

using namespace std;

char board[64];
short userScore = 0, botScore = 0;
bool isUserWhite = true;
bool isUsersTurn = true;
bool isRunning = false;
int move_num = 0;

// Castling rights
bool whiteCanCastleShort = true;
bool whiteCanCastleLong = true;
bool blackCanCastleShort = true;
bool blackCanCastleLong = true;

// En passant target square index (-1 if none)
int enPassantTarget = -1;

enum BotProfile {
    OFFENSIVE,
    SAFE,
    STRATEGIC
};

BotProfile botProfile = OFFENSIVE;

const int PIECE_VALUE_PAWN = 1;
const int PIECE_VALUE_KNIGHT = 3;
const int PIECE_VALUE_BISHOP = 3;
const int PIECE_VALUE_ROOK = 5;
const int PIECE_VALUE_QUEEN = 9;

string index_to_algebraic(int idx);

bool isWhitePiece(char c) {
    return c >= 'A' && c <= 'Z';
}

bool isBlackPiece(char c) {
    return c >= 'a' && c <= 'z';
}

bool isEmptySquare(char c) {
    return c == ' ';
}

bool isSameColor(char a, char b) {
    if (isEmptySquare(a) || isEmptySquare(b)) return false;
    return (isWhitePiece(a) && isWhitePiece(b)) || (isBlackPiece(a) && isBlackPiece(b));
}

bool isOppositeColor(char a, char b) {
    if (isEmptySquare(a) || isEmptySquare(b)) return false;
    return (isWhitePiece(a) && isBlackPiece(b)) || (isBlackPiece(a) && isWhitePiece(b));
}

int pieceValue(char c) {
    c = (char)tolower(c);
    if (c == 'p') return PIECE_VALUE_PAWN;
    if (c == 'n') return PIECE_VALUE_KNIGHT;
    if (c == 'b') return PIECE_VALUE_BISHOP;
    if (c == 'r') return PIECE_VALUE_ROOK;
    if (c == 'q') return PIECE_VALUE_QUEEN;
    return 0;
}

void initialize_game_state() {
    move_num = 0;
    userScore = 0;
    botScore = 0;
    isRunning = false;
    isUsersTurn = false;

    whiteCanCastleShort = true;
    whiteCanCastleLong = true;
    blackCanCastleShort = true;
    blackCanCastleLong = true;
    enPassantTarget = -1;

    for (int i = 0; i < 64; i++) board[i] = ' ';

    board[0]  = 'R'; board[1]  = 'N'; board[2]  = 'B'; board[3]  = 'Q';
    board[4]  = 'K'; board[5]  = 'B'; board[6]  = 'N'; board[7]  = 'R';
    for (int i = 8; i < 16; i++) board[i] = 'P';

    for (int i = 48; i < 56; i++) board[i] = 'p';
    board[56] = 'r'; board[57] = 'n'; board[58] = 'b'; board[59] = 'q';
    board[60] = 'k'; board[61] = 'b'; board[62] = 'n'; board[63] = 'r';
}

void print_welcome() {
    //cout << "---=== Project codename 'PickleBot' ===---\n";
    //cout << "A simple non-ML chess bot with castling, en-passant, and self-check detection.\n";
    //cout << "/// By C0m3b4ck under APL 2.0 ///\n";
}

void print_board() {
    move_num++;
    cout << "\n---=== RUNDA " << move_num << " ===---\n";
    cout << "Punkty uzytkownika: " << userScore << "\n";
    cout << "Punkty bota: " << botScore << "\n\n";
    cout << "  A B C D E F G H\n";

    for (int rank = 7; rank >= 0; rank--) {
        cout << (rank + 1) << " ";
        for (int file = 0; file < 8; file++) {
            cout << board[rank * 8 + file] << " ";
        }
        cout << "\n";
    }

    string castlingInfo = "Roszada: ";
    if (whiteCanCastleShort) castlingInfo += "O-O ";
    if (whiteCanCastleLong) castlingInfo += "O-O-O ";
    if (blackCanCastleShort) castlingInfo += "o-o ";
    if (blackCanCastleLong) castlingInfo += "o-o-o ";
    if (enPassantTarget != -1) {
        auto toAlg = index_to_algebraic(enPassantTarget);
        castlingInfo += " | EP: " + toAlg;
    }
    cout << "\n" << castlingInfo << "\n";
}

void get_side_choice() {
    char tmp;
    cout << "\nB/b aby wybrac czarne, W/w aby wybrac biale";
    cout << "\nWybierz strone: ";
    cin >> tmp;

    if (tmp == 'W' || tmp == 'w') {
        cout << "\nWybrano: biale\n";
        isUserWhite = true;
        isRunning = true;
        isUsersTurn = true;
    } else if (tmp == 'B' || tmp == 'b') {
        cout << "\nWybrano: czarne\n";
        isUserWhite = false;
        isRunning = true;
        isUsersTurn = false;
    } else {
        cout << "\n!!! Nieprawidlowa opcja !!! Prosze sprobowac ponownie.\n";
        get_side_choice();
    }
}

int algebraic_to_index(const string& s) {
    if (s.size() != 2) return -1;
    char file = (char)tolower(s[0]);
    char rank = s[1];
    if (file < 'a' || file > 'h' || rank < '1' || rank > '8') return -1;
    int f = file - 'a';
    int r = rank - '1';
    return r * 8 + f;
}

string index_to_algebraic(int idx) {
    string s = "a1";
    s[0] = char('a' + (idx % 8));
    s[1] = char('1' + (idx / 8));
    return s;
}

bool clear_path(int from, int to, int step) {
    int pos = from + step;
    while (pos != to) {
        if (!isEmptySquare(board[pos])) return false;
        pos += step;
    }
    return true;
}

bool valid_pawn_move(int from, int to, char piece) {
    int fromFile = from % 8, toFile = to % 8;
    int fromRank = from / 8, toRank = to / 8;
    int dir = isWhitePiece(piece) ? 1 : -1;
    int startRank = isWhitePiece(piece) ? 1 : 6;

    // Normal pawn moves (no capture)
    if (fromFile == toFile && isEmptySquare(board[to])) {
        if (toRank - fromRank == dir) return true;
        if (fromRank == startRank && toRank - fromRank == 2 * dir) {
            int mid = from + 8 * dir;
            return isEmptySquare(board[mid]);
        }
    }

    // Normal capture (diagonal)
    if (abs(toFile - fromFile) == 1 && toRank - fromRank == dir && !isEmptySquare(board[to])) {
        return isOppositeColor(piece, board[to]);
    }

    // En passant
    if (abs(toFile - fromFile) == 1 && toRank - fromRank == dir && to == enPassantTarget) {
        // The captured pawn is on the same file as target and rank of 'from'
        return true;
    }

    return false;
}

bool valid_knight_move(int from, int to) {
    int df = abs((from % 8) - (to % 8));
    int dr = abs((from / 8) - (to / 8));
    return (df == 1 && dr == 2) || (df == 2 && dr == 1);
}

bool valid_bishop_move(int from, int to) {
    int df = abs((from % 8) - (to % 8));
    int dr = abs((from / 8) - (to / 8));
    if (df != dr) return false;
    int step = 0;
    if (to > from) step = ((to % 8) > (from % 8)) ? 9 : 7;
    else step = ((to % 8) > (from % 8)) ? -7 : -9;
    return clear_path(from, to, step);
}

bool valid_rook_move(int from, int to) {
    int df = abs((from % 8) - (to % 8));
    int dr = abs((from / 8) - (to / 8));
    if (df != 0 && dr != 0) return false;
    int step = 0;
    if (df == 0) step = (to > from) ? 8 : -8;
    else step = (to > from) ? 1 : -1;
    return clear_path(from, to, step);
}

bool valid_queen_move(int from, int to) {
    return valid_bishop_move(from, to) || valid_rook_move(from, to);
}

bool valid_king_move(int from, int to) {
    int df = abs((from % 8) - (to % 8));
    int dr = abs((from / 8) - (to / 8));
    return df <= 1 && dr <= 1;
}

bool is_square_attacked(int sq, bool byWhite, const char b[64]) {
    for (int i = 0; i < 64; i++) {
        char p = b[i];
        if (isEmptySquare(p)) continue;
        if (byWhite && !isWhitePiece(p)) continue;
        if (!byWhite && !isBlackPiece(p)) continue;

        int file = i % 8;
        int rank = i / 8;
        int tf = sq % 8;
        int tr = sq / 8;

        switch (tolower(p)) {
            case 'n': {
                int df = abs(file - tf);
                int dr = abs(rank - tr);
                if ((df == 1 && dr == 2) || (df == 2 && dr == 1)) return true;
                break;
            }
            case 'p': {
                int dir = isWhitePiece(p) ? 1 : -1;
                if (tr - rank == dir && abs(tf - file) == 1) return true;
                break;
            }
            case 'k': {
                int df = abs(file - tf);
                int dr = abs(rank - tr);
                if (df <= 1 && dr <= 1) return true;
                break;
            }
            case 'b': {
                if (valid_bishop_move(i, sq)) return true;
                break;
            }
            case 'r': {
                if (valid_rook_move(i, sq)) return true;
                break;
            }
            case 'q': {
                if (valid_queen_move(i, sq)) return true;
                break;
            }
        }
    }
    return false;
}

bool is_in_check(bool whiteSide, const char b[64]) {
    int king = whiteSide ? 'K' : 'k';
    int kingSq = -1;
    for (int i = 0; i < 64; i++) {
        if (b[i] == king) {
            kingSq = i;
            break;
        }
    }
    if (kingSq == -1) return false;
    return is_square_attacked(kingSq, !whiteSide, b);
}

void copy_board_state(const char src[64], char dst[64]) {
    for (int i = 0; i < 64; i++) dst[i] = src[i];
}

void apply_move_on_board(char b[64], int from, int to, bool whiteMover) {
    char moving = b[from];
    b[to] = moving;
    b[from] = ' ';

    if (moving == 'P' && to / 8 == 7) b[to] = 'Q';
    if (moving == 'p' && to / 8 == 0) b[to] = 'q';
}

bool move_leaves_king_in_check(int from, int to, bool moverWhite, char tempBoard[64]) {
    apply_move_on_board(tempBoard, from, to, moverWhite);
    return is_in_check(moverWhite, tempBoard);
}

bool is_castling_move(int from, int to, bool moverWhite) {
    char piece = board[from];
    char king = moverWhite ? 'K' : 'k';
    if (tolower(piece) != 'k') return false;

    // King moves 2 squares horizontally
    int fromFile = from % 8;
    int toFile = to % 8;
    int fromRank = from / 8;
    int toRank = to / 8;

    if (fromRank != toRank) return false;
    if (abs(toFile - fromFile) != 2) return false;

    int rankOffset = moverWhite ? 0 : 56;
    bool shortCastling = (toFile == 6);
    bool longCastling = (toFile == 2);

    if (shortCastling) {
        int rookFrom = rankOffset + 7;
        if (board[rookFrom] != (moverWhite ? 'R' : 'r')) return false;
        // Check squares between king and rook are empty
        if (!isEmptySquare(board[rankOffset + 5]) || !isEmptySquare(board[rankOffset + 6])) return false;
    } else if (longCastling) {
        int rookFrom = rankOffset;
        if (board[rookFrom] != (moverWhite ? 'R' : 'r')) return false;
        // Check squares between king and rook are empty
        if (!isEmptySquare(board[rankOffset + 1]) ||
            !isEmptySquare(board[rankOffset + 2]) ||
            !isEmptySquare(board[rankOffset + 3])) return false;
    } else {
        return false;
    }

    return true;
}

bool is_castling_legal(int from, int to, bool moverWhite) {
    if (!is_castling_move(from, to, moverWhite)) return false;

    int rankOffset = moverWhite ? 0 : 56;
    int fromFile = from % 8;
    int toFile = to % 8;
    int toRank = to / 8;

    // King cannot be in check
    if (is_in_check(moverWhite, board)) return false;

    char king = moverWhite ? 'K' : 'k';
    int kingStart = rankOffset + 4;
    int kingFile = fromFile;

    // For castling, the king must not cross a square that is attacked
    // Check path of king: from +1 or -1 squares
    if (toFile == 6) { // short castle
        // Squares the king passes: f-file and g-file
        if (is_square_attacked(rankOffset + 5, !moverWhite, board)) return false;
        if (is_square_attacked(rankOffset + 6, !moverWhite, board)) return false;
    } else if (toFile == 2) { // long castle
        // Squares the king passes: b-file, c-file, d-file
        if (is_square_attacked(rankOffset + 3, !moverWhite, board)) return false;
        if (is_square_attacked(rankOffset + 2, !moverWhite, board)) return false;
        if (is_square_attacked(rankOffset + 1, !moverWhite, board)) return false;
    }

    return true;
}

void apply_castling(int from, int to, bool moverWhite) {
    int rankOffset = moverWhite ? 0 : 56;
    char rookPiece = moverWhite ? 'R' : 'r';
    char kingPiece = moverWhite ? 'K' : 'k';

    // Move king
    board[to] = kingPiece;
    board[from] = ' ';

    int toFile = to % 8;
    int rookFrom = -1, rookTo = -1;

    if (toFile == 6) { // short
        rookFrom = rankOffset + 7;
        rookTo = rankOffset + 5;
    } else if (toFile == 2) { // long
        rookFrom = rankOffset;
        rookTo = rankOffset + 3;
    }

    if (rookFrom != -1) {
        board[rookTo] = rookPiece;
        board[rookFrom] = ' ';
    }
}

bool is_legal_move(int from, int to, bool moverWhite) {
    if (from < 0 || from > 63 || to < 0 || to > 63 || from == to) return false;

    char piece = board[from];
    char target = board[to];
    if (isEmptySquare(piece)) return false;

    if (moverWhite && !isWhitePiece(piece)) return false;
    if (!moverWhite && !isBlackPiece(piece)) return false;

    if (!isEmptySquare(target) && isSameColor(piece, target)) return false;

    // Castling check
    if (tolower(piece) == 'k') {
        if (is_castling_move(from, to, moverWhite)) {
            return is_castling_legal(from, to, moverWhite);
        }
    }

    // Basic move validation
    switch (tolower(piece)) {
        case 'p':
            if (!valid_pawn_move(from, to, piece)) return false;
            break;
        case 'n':
            if (!valid_knight_move(from, to)) return false;
            break;
        case 'b':
            if (!valid_bishop_move(from, to)) return false;
            break;
        case 'r':
            if (!valid_rook_move(from, to)) return false;
            break;
        case 'q':
            if (!valid_queen_move(from, to)) return false;
            break;
        case 'k':
            if (!valid_king_move(from, to)) return false;
            break;
        default:
            return false;
    }

    // Self-check: ensure the move does not leave own king in check
    char tempBoard[64];
    copy_board_state(board, tempBoard);

    // Handle en passant capture in temp board
    if (tolower(piece) == 'p' && abs((to % 8) - (from % 8)) == 1 && to == enPassantTarget) {
        // Remove captured pawn
        int capturedRank = from / 8;
        int capturedFile = to % 8;
        int capturedIdx = capturedRank * 8 + capturedFile;
        tempBoard[capturedIdx] = ' ';
        tempBoard[to] = piece;
        tempBoard[from] = ' ';
    } else {
        // Normal move
        tempBoard[to] = piece;
        tempBoard[from] = ' ';
        // Suppress promotion for check test
        if (tempBoard[to] == 'P' && to / 8 == 7) tempBoard[to] = 'P';
        if (tempBoard[to] == 'p' && to / 8 == 0) tempBoard[to] = 'p';
    }

    if (is_in_check(moverWhite, tempBoard)) return false;

    return true;
}

void update_castling_rights(int from, int to, bool moverWhite) {
    char piece = board[from];
    char moving = tolower(piece);

    int rankOffset = moverWhite ? 0 : 56;
    bool whiteSide = moverWhite;

    // If king moves, lose both castling rights
    if (moving == 'k') {
        if (whiteSide) {
            whiteCanCastleShort = false;
            whiteCanCastleLong = false;
        } else {
            blackCanCastleShort = false;
            blackCanCastleLong = false;
        }
    }

    // If rook moves from its starting square, lose the corresponding castling right
    if (moving == 'r') {
        if (whiteSide) {
            if (from == rankOffset + 7) whiteCanCastleShort = false;
            if (from == rankOffset) whiteCanCastleLong = false;
        } else {
            if (from == rankOffset + 7) blackCanCastleShort = false;
            if (from == rankOffset) blackCanCastleLong = false;
        }
    }

    // If a rook is captured, update castling rights
    if (!isEmptySquare(board[to])) {
        if (to == 0) whiteCanCastleLong = false;
        if (to == 7) whiteCanCastleShort = false;
        if (to == 56) blackCanCastleLong = false;
        if (to == 63) blackCanCastleShort = false;
    }
}

void update_en_passant_target(int from, int to, bool moverWhite) {
    char piece = board[from];
    int dir = isWhitePiece(piece) ? 1 : -1;
    int fromRank = from / 8;
    int toRank = to / 8;

    if (tolower(piece) == 'p' && abs(toRank - fromRank) == 2) {
        // En passant target is the square the pawn passed over
        enPassantTarget = from + 8 * dir;
    } else {
        enPassantTarget = -1;
    }
}

void apply_move(int from, int to) {
    char moving = board[from];
    char captured = board[to];

    // En passant capture
    if (tolower(moving) == 'p' && abs((to % 8) - (from % 8)) == 1 && to == enPassantTarget) {
        int capturedRank = from / 8;
        int capturedFile = to % 8;
        int capturedIdx = capturedRank * 8 + capturedFile;
        captured = board[capturedIdx];
        board[capturedIdx] = ' ';
        if (!isEmptySquare(captured)) {
            int val = pieceValue(captured);
            if (isWhitePiece(moving)) userScore += val;
            else botScore += val;
        }
    }

    // Castling
    if (tolower(moving) == 'k' && is_castling_move(from, to, isUserWhite)) {
        apply_castling(from, to, isUserWhite);
        if (!isEmptySquare(captured)) {
            int val = pieceValue(captured);
            if (isWhitePiece(moving)) userScore += val;
            else botScore += val;
        }
    } else {
        // Normal move
        if (!isEmptySquare(captured)) {
            int val = pieceValue(captured);
            if (isWhitePiece(moving)) userScore += val;
            else botScore += val;
        }

        board[to] = moving;
        board[from] = ' ';

        if (moving == 'P' && to / 8 == 7) board[to] = 'Q';
        if (moving == 'p' && to / 8 == 0) board[to] = 'q';
    }

    update_castling_rights(from, to, isUserWhite);
    update_en_passant_target(from, to, isUserWhite);
}

void apply_move_bot(int from, int to, bool botWhite) {
    char moving = board[from];
    char captured = board[to];

    // En passant capture
    if (tolower(moving) == 'p' && abs((to % 8) - (from % 8)) == 1 && to == enPassantTarget) {
        int capturedRank = from / 8;
        int capturedFile = to % 8;
        int capturedIdx = capturedRank * 8 + capturedFile;
        captured = board[capturedIdx];
        board[capturedIdx] = ' ';
        if (!isEmptySquare(captured)) {
            int val = pieceValue(captured);
            if (isWhitePiece(moving)) userScore += val;
            else botScore += val;
        }
    }

    // Castling
    if (tolower(moving) == 'k' && is_castling_move(from, to, botWhite)) {
        apply_castling(from, to, botWhite);
        if (!isEmptySquare(captured)) {
            int val = pieceValue(captured);
            if (isWhitePiece(moving)) userScore += val;
            else botScore += val;
        }
    } else {
        // Normal move
        if (!isEmptySquare(captured)) {
            int val = pieceValue(captured);
            if (isWhitePiece(moving)) userScore += val;
            else botScore += val;
        }

        board[to] = moving;
        board[from] = ' ';

        if (moving == 'P' && to / 8 == 7) board[to] = 'Q';
        if (moving == 'p' && to / 8 == 0) board[to] = 'q';
    }

    update_castling_rights(from, to, botWhite);
    update_en_passant_target(from, to, botWhite);
}

void update_turn() {
    isUsersTurn = !isUsersTurn;
}

void update_move(int from, int to) {
    apply_move(from, to);
    update_turn();
}

void get_move_choice() {
    string from_s, to_s;
    cout << "\n--- TWOJ RUCH ---\n";
    cout << "Wpisz swoj ruch (od, do): ";
    cin >> from_s >> to_s;

    int from = algebraic_to_index(from_s);
    int to = algebraic_to_index(to_s);

    if (from == -1 || to == -1) {
        cout << "\n!!! Nieprawidlowe wejsice !!! Sprobuj ponownie.\n";
        return;
    }

    if (!is_legal_move(from, to, isUserWhite)) {
        cout << "\n!!! Niedozwolony ruch !!! Sprobuj ponownie.\n";
        return;
    }

    update_move(from, to);
}

int king_distance_score(int sq, bool targetWhiteKing) {
    int kingSq = -1;
    char king = targetWhiteKing ? 'K' : 'k';
    for (int i = 0; i < 64; i++) {
        if (board[i] == king) {
            kingSq = i;
            break;
        }
    }
    if (kingSq == -1) return 0;
    int f1 = sq % 8, r1 = sq / 8;
    int f2 = kingSq % 8, r2 = kingSq / 8;
    return 14 - (abs(f1 - f2) + abs(r1 - r2));
}

int tradeClassScore(char attacker, char victim) {
    int a = pieceValue(attacker);
    int v = pieceValue(victim);
    if (a > v) return -1;
    if (a < v) return 1;
    return 0;
}

bool piece_is_hanging_after_move(const char b[64], int from, int to, bool moverWhite) {
    char temp[64];
    copy_board_state(b, temp);

    char moving = temp[from];
    temp[to] = moving;
    temp[from] = ' ';

    bool enemyWhite = !moverWhite;
    for (int i = 0; i < 64; i++) {
        char p = temp[i];
        if (isEmptySquare(p)) continue;
        if (enemyWhite && !isWhitePiece(p)) continue;
        if (!enemyWhite && !isBlackPiece(p)) continue;

        // Temporarily check if enemy can capture on 'to'
        // We reuse is_legal_move but it uses global board, so we need a different approach
        // For simplicity, we'll just skip this hanging check in this version
    }

    return false;
}

int score_move_profile(const char b[64], int from, int to, BotProfile profile, bool moverWhite) {
    char moving = b[from];
    char target = b[to];

    int captureScore = 0;
    int tradeClass = 0;

    if (!isEmptySquare(target) && isOppositeColor(moving, target)) {
        int attackerValue = pieceValue(moving);
        int victimValue = pieceValue(target);
        captureScore = victimValue * 10 - attackerValue * 2;
        tradeClass = tradeClassScore(moving, target);
    }

    char tempBoard[64];
    copy_board_state(b, tempBoard);
    tempBoard[to] = moving;
    tempBoard[from] = ' ';
    if (tempBoard[to] == 'P' && to / 8 == 7) tempBoard[to] = 'Q';
    if (tempBoard[to] == 'p' && to / 8 == 0) tempBoard[to] = 'q';

    int checkBonus = is_in_check(!moverWhite, tempBoard) ? 40 : 0;

    int kingThreat = 0;
    if (profile == STRATEGIC) {
        kingThreat += king_distance_score(to, !moverWhite);
        if (checkBonus) kingThreat += 60;
    }

    int safetyPenalty = 0;
    if (profile == SAFE) {
        if (piece_is_hanging_after_move(b, from, to, moverWhite)) safetyPenalty += 50;
    }

    int openKingPenalty = 0;
    if (profile == SAFE) {
        int moverKingSq = -1;
        char king = moverWhite ? 'K' : 'k';
        for (int i = 0; i < 64; i++) {
            if (b[i] == king) {
                moverKingSq = i;
                break;
            }
        }
        if (moverKingSq != -1) {
            int mf = moverKingSq % 8, mr = moverKingSq / 8;
            int tf = to % 8, tr = to / 8;
            if (abs(mf - tf) <= 2 && abs(mr - tr) <= 2) openKingPenalty += 10;
        }
    }

    int tradePenalty = 0;
    if (profile == SAFE && !isEmptySquare(target) && isOppositeColor(moving, target)) {
        if (tradeClass < 0) tradePenalty = 120;
        else if (tradeClass == 0) tradePenalty = 30;
        else tradePenalty = -20;
    }

    int score = 0;
    switch (profile) {
        case OFFENSIVE:
            score = 5 * captureScore + 2 * checkBonus;
            break;
        case SAFE:
            score = captureScore - 4 * safetyPenalty - 3 * openKingPenalty - tradePenalty;
            break;
        case STRATEGIC:
            score = 2 * captureScore + 4 * kingThreat + 3 * checkBonus;
            break;
    }

    return score;
}

vector<pair<int,int>> generate_moves(bool whiteSide) {
    vector<pair<int,int>> moves;
    for (int from = 0; from < 64; from++) {
        char piece = board[from];
        if (isEmptySquare(piece)) continue;
        if (whiteSide && !isWhitePiece(piece)) continue;
        if (!whiteSide && !isBlackPiece(piece)) continue;

        for (int to = 0; to < 64; to++) {
            if (is_legal_move(from, to, whiteSide)) moves.push_back({from, to});
        }
    }
    return moves;
}

pair<int,int> predict_best_move(bool whiteSide) {
    vector<pair<int,int>> moves = generate_moves(whiteSide);
    if (moves.empty()) return {-1, -1};

    int bestScore = INT_MIN;
    pair<int,int> bestMove = moves[0];

    for (auto mv : moves) {
        int score = score_move_profile(board, mv.first, mv.second, botProfile, whiteSide);
        if (score > bestScore) {
            bestScore = score;
            bestMove = mv;
        }
    }

    return bestMove;
}

void bot_move() {
    bool botWhite = !isUserWhite;

    cout << "---===*&*&*&*===--- \n";
    cout << "\nBot mysli...\n";
    cout << "Profil: ";
    if (botProfile == OFFENSIVE) cout << "ofensywny \n";
    else if (botProfile == SAFE) cout << "przezorny \n";
    else cout << "strategicznie-ofensywny \n";

    vector<pair<int,int>> moves = generate_moves(botWhite);
    cout << "Dozwolone ruchy: " << moves.size() << "\n";

    if (moves.empty()) {
        cout << "\n !!! Bot nie ma dozwolonych ruchow. !!!\n";
        cout << "!!!===---ZAKONCZONO GRE---===!!! \n";
        isRunning = false;
        return;
    }

    pair<int,int> bestMove = moves[0];
    int bestScore = INT_MIN;

    for (auto mv : moves) {
        int score = score_move_profile(board, mv.first, mv.second, botProfile, botWhite);

        cout << "Analiza "
             << index_to_algebraic(mv.first) << " -> " << index_to_algebraic(mv.second)
             << " | punktacja = " << score << "\n";
        cout.flush();

        this_thread::sleep_for(chrono::milliseconds(120));

        if (score > bestScore) {
            bestScore = score;
            bestMove = mv;

            cout << "Nowy najlepszy ruch: "
                 << index_to_algebraic(bestMove.first) << " -> "
                 << index_to_algebraic(bestMove.second)
                 << " | najlepszy wynik = " << bestScore << "\n";
            cout.flush();

            this_thread::sleep_for(chrono::milliseconds(120));
        }
    }

    cout << "Bot wybral: "
         << index_to_algebraic(bestMove.first) << " "
         << index_to_algebraic(bestMove.second) << "\n";

    apply_move_bot(bestMove.first, bestMove.second, botWhite);
    update_turn();
}

bool has_king(bool whiteSide) {
    for (int i = 0; i < 64; i++) {
        if (whiteSide && board[i] == 'K') return true;
        if (!whiteSide && board[i] == 'k') return true;
    }
    return false;
}

void choose_bot_profile() {
    char c;
    cout << "\n Wybierz profil bota:";
    cout << "\nO/o = ofensywny";
    cout << "\nS/s = przezorny";
    cout << "\nT/t = strategiczno-ofensywny";
    cout << "\nWybierz profil: ";
    cin >> c;

    if (c == 'O' || c == 'o') botProfile = OFFENSIVE;
    else if (c == 'S' || c == 's') botProfile = SAFE;
    else if (c == 'T' || c == 't') botProfile = STRATEGIC;
    else {
        cout << "\n!!! Niepoprawne wejscie !!! Wybrano domyslna opcje: ofensywny.\n";
        botProfile = OFFENSIVE;
    }
}

int main() {
    initialize_game_state();
    print_welcome();
    get_side_choice();
    choose_bot_profile();

    while (isRunning) {
        if (!has_king(true)) {
            cout << "\nCzarne wygrywaja.\n";
            break;
        }
        if (!has_king(false)) {
            cout << "\nBiale wygrywaja.\n";
            break;
        }

        print_board();

        if (isUsersTurn) {
            get_move_choice();
        } else {
            bot_move();
        }
    }

    return 0;
}
