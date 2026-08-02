#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commctrl.h>
#include <vector>
#include <string>
#include <cctype>
#include <climits>
#include <cmath>
#include <stdio.h>

using namespace std;

// --- Chess globals ---
char board[64];
short userScore = 0, botScore = 0;
bool isUserWhite = true;
bool isUsersTurn = true;
bool isRunning = false;
int move_num = 0;

bool whiteCanCastleShort = true;
bool whiteCanCastleLong = true;
bool blackCanCastleShort = true;
bool blackCanCastleLong = true;
int enPassantTarget = -1;

enum BotProfile { OFFENSIVE, SAFE, STRATEGIC };
BotProfile botProfile = OFFENSIVE;

const int PIECE_VALUE_PAWN = 1;
const int PIECE_VALUE_KNIGHT = 3;
const int PIECE_VALUE_BISHOP = 3;
const int PIECE_VALUE_ROOK = 5;
const int PIECE_VALUE_QUEEN = 9;

HWND hFrame = NULL;
HWND hLog = NULL;
HWND hStatus = NULL;
HWND hScore = NULL;
HWND hSideCombo = NULL;
HWND hProfileCombo = NULL;
HWND hStartBtn = NULL;
HWND hResetBtn = NULL;
HWND hSquareBtn[64];
int selectedSquare = -1;

bool isWhitePiece(char c) { return c >= 'A' && c <= 'Z'; }
bool isBlackPiece(char c) { return c >= 'a' && c <= 'z'; }
bool isEmptySquare(char c) { return c == ' '; }

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

string pieceToLabel(char p) {
    if (p == ' ') return "";
    char buf[2] = { p, 0 };
    return buf;
}

int algebraic_to_index(const string& s) {
    if (s.size() != 2) return -1;
    char file = (char)tolower(s[0]);
    char rank = s[1];
    if (file < 'a' || file > 'h' || rank < '1' || rank > '8') return -1;
    return (rank - '1') * 8 + (file - 'a');
}

string index_to_algebraic(int idx) {
    char buf[3];
    buf[0] = char('a' + (idx % 8));
    buf[1] = char('1' + (idx / 8));
    buf[2] = 0;
    return string(buf);
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

    if (fromFile == toFile && isEmptySquare(board[to])) {
        if (toRank - fromRank == dir) return true;
        if (fromRank == startRank && toRank - fromRank == 2 * dir) {
            int mid = from + 8 * dir;
            return isEmptySquare(board[mid]);
        }
    }

    if (abs(toFile - fromFile) == 1 && toRank - fromRank == dir && !isEmptySquare(board[to])) {
        return isOppositeColor(piece, board[to]);
    }

    if (abs(toFile - fromFile) == 1 && toRank - fromRank == dir && to == enPassantTarget) {
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
    int step = (to > from) ? ((to % 8) > (from % 8) ? 9 : 7) : ((to % 8) > (from % 8) ? -7 : -9);
    return clear_path(from, to, step);
}

bool valid_rook_move(int from, int to) {
    int df = abs((from % 8) - (to % 8));
    int dr = abs((from / 8) - (to / 8));
    if (df != 0 && dr != 0) return false;
    int step = (df == 0) ? ((to > from) ? 8 : -8) : ((to > from) ? 1 : -1);
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

        int file = i % 8, rank = i / 8;
        int tf = sq % 8, tr = sq / 8;

        switch (tolower(p)) {
            case 'n': {
                int df = abs(file - tf), dr = abs(rank - tr);
                if ((df == 1 && dr == 2) || (df == 2 && dr == 1)) return true;
                break;
            }
            case 'p': {
                int dir = isWhitePiece(p) ? 1 : -1;
                if (tr - rank == dir && abs(tf - file) == 1) return true;
                break;
            }
            case 'k': {
                int df = abs(file - tf), dr = abs(rank - tr);
                if (df <= 1 && dr <= 1) return true;
                break;
            }
            case 'b': if (valid_bishop_move(i, sq)) return true; break;
            case 'r': if (valid_rook_move(i, sq)) return true; break;
            case 'q': if (valid_queen_move(i, sq)) return true; break;
        }
    }
    return false;
}

bool is_in_check(bool whiteSide, const char b[64]) {
    char king = whiteSide ? 'K' : 'k';
    int kingSq = -1;
    for (int i = 0; i < 64; i++) {
        if (b[i] == king) { kingSq = i; break; }
    }
    if (kingSq == -1) return false;
    return is_square_attacked(kingSq, !whiteSide, b);
}

void copy_board_state(const char src[64], char dst[64]) {
    for (int i = 0; i < 64; i++) dst[i] = src[i];
}

bool is_castling_move(int from, int to, bool moverWhite) {
    char piece = board[from];
    if (tolower(piece) != 'k') return false;

    int fromFile = from % 8, toFile = to % 8, fromRank = from / 8, toRank = to / 8;
    if (fromRank != toRank || abs(toFile - fromFile) != 2) return false;

    int rankOffset = moverWhite ? 0 : 56;
    if (toFile == 6) {
        if (board[rankOffset + 7] != (moverWhite ? 'R' : 'r')) return false;
        if (!isEmptySquare(board[rankOffset + 5]) || !isEmptySquare(board[rankOffset + 6])) return false;
    } else if (toFile == 2) {
        if (board[rankOffset] != (moverWhite ? 'R' : 'r')) return false;
        if (!isEmptySquare(board[rankOffset + 1]) || !isEmptySquare(board[rankOffset + 2]) || !isEmptySquare(board[rankOffset + 3])) return false;
    } else return false;

    return true;
}

bool is_castling_legal(int from, int to, bool moverWhite) {
    if (!is_castling_move(from, to, moverWhite)) return false;
    int rankOffset = moverWhite ? 0 : 56, toFile = to % 8;
    if (is_in_check(moverWhite, board)) return false;
    if (toFile == 6) {
        if (is_square_attacked(rankOffset + 5, !moverWhite, board)) return false;
        if (is_square_attacked(rankOffset + 6, !moverWhite, board)) return false;
    } else if (toFile == 2) {
        if (is_square_attacked(rankOffset + 3, !moverWhite, board)) return false;
        if (is_square_attacked(rankOffset + 2, !moverWhite, board)) return false;
        if (is_square_attacked(rankOffset + 1, !moverWhite, board)) return false;
    }
    return true;
}

void apply_castling(int from, int to, bool moverWhite) {
    int rankOffset = moverWhite ? 0 : 56;
    char rookPiece = moverWhite ? 'R' : 'r', kingPiece = moverWhite ? 'K' : 'k';
    board[to] = kingPiece; board[from] = ' ';
    int toFile = to % 8, rookFrom = -1, rookTo = -1;
    if (toFile == 6) { rookFrom = rankOffset + 7; rookTo = rankOffset + 5; }
    else if (toFile == 2) { rookFrom = rankOffset; rookTo = rankOffset + 3; }
    if (rookFrom != -1) { board[rookTo] = rookPiece; board[rookFrom] = ' '; }
}

bool is_legal_move(int from, int to, bool moverWhite) {
    if (from < 0 || from > 63 || to < 0 || to > 63 || from == to) return false;
    char piece = board[from], target = board[to];
    if (isEmptySquare(piece)) return false;
    if (moverWhite && !isWhitePiece(piece)) return false;
    if (!moverWhite && !isBlackPiece(piece)) return false;
    if (!isEmptySquare(target) && isSameColor(piece, target)) return false;

    if (tolower(piece) == 'k' && is_castling_move(from, to, moverWhite))
        return is_castling_legal(from, to, moverWhite);

    switch (tolower(piece)) {
        case 'p': if (!valid_pawn_move(from, to, piece)) return false; break;
        case 'n': if (!valid_knight_move(from, to)) return false; break;
        case 'b': if (!valid_bishop_move(from, to)) return false; break;
        case 'r': if (!valid_rook_move(from, to)) return false; break;
        case 'q': if (!valid_queen_move(from, to)) return false; break;
        case 'k': if (!valid_king_move(from, to)) return false; break;
        default: return false;
    }

    char temp[64];
    copy_board_state(board, temp);
    if (tolower(piece) == 'p' && abs((to % 8) - (from % 8)) == 1 && to == enPassantTarget) {
        int capIdx = (from / 8) * 8 + (to % 8);
        temp[capIdx] = ' '; temp[to] = piece; temp[from] = ' ';
    } else {
        temp[to] = piece; temp[from] = ' ';
        if (temp[to] == 'P' && to / 8 == 7) temp[to] = 'Q';
        if (temp[to] == 'p' && to / 8 == 0) temp[to] = 'p';
    }
    return !is_in_check(moverWhite, temp);
}

void update_castling_rights(int from, int to, bool moverWhite) {
    char moving = tolower(board[from]);
    int rankOffset = moverWhite ? 0 : 56;
    if (moving == 'k') {
        if (moverWhite) { whiteCanCastleShort = false; whiteCanCastleLong = false; }
        else { blackCanCastleShort = false; blackCanCastleLong = false; }
    }
    if (moving == 'r') {
        if (moverWhite) {
            if (from == rankOffset + 7) whiteCanCastleShort = false;
            if (from == rankOffset) whiteCanCastleLong = false;
        } else {
            if (from == rankOffset + 7) blackCanCastleShort = false;
            if (from == rankOffset) blackCanCastleLong = false;
        }
    }
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
    if (tolower(piece) == 'p' && abs((to / 8) - (from / 8)) == 2)
        enPassantTarget = from + 8 * dir;
    else enPassantTarget = -1;
}

void apply_move(int from, int to, bool whiteMover) {
    char moving = board[from], captured = board[to];
    bool ep = (tolower(moving) == 'p' && abs((to % 8) - (from % 8)) == 1 && to == enPassantTarget);
    bool cast = (tolower(moving) == 'k' && is_castling_move(from, to, whiteMover));

    if (ep) {
        int capIdx = (from / 8) * 8 + (to % 8);
        captured = board[capIdx];
        board[capIdx] = ' ';
        if (!isEmptySquare(captured)) {
            int val = pieceValue(captured);
            if (whiteMover) userScore += val; else botScore += val;
        }
    }

    if (cast) {
        apply_castling(from, to, whiteMover);
        if (!isEmptySquare(captured)) {
            int val = pieceValue(captured);
            if (whiteMover) userScore += val; else botScore += val;
        }
    } else {
        if (!isEmptySquare(captured)) {
            int val = pieceValue(captured);
            if (whiteMover) userScore += val; else botScore += val;
        }
        board[to] = moving; board[from] = ' ';
        if (moving == 'P' && to / 8 == 7) board[to] = 'Q';
        if (moving == 'p' && to / 8 == 0) board[to] = 'q';
    }

    update_castling_rights(from, to, whiteMover);
    update_en_passant_target(from, to, whiteMover);
}

void initialize_game_state() {
    move_num = 0; userScore = 0; botScore = 0;
    isRunning = false; isUsersTurn = false;
    whiteCanCastleShort = whiteCanCastleLong = true;
    blackCanCastleShort = blackCanCastleLong = true;
    enPassantTarget = -1;
    for (int i = 0; i < 64; i++) board[i] = ' ';
    board[0]='R'; board[1]='N'; board[2]='B'; board[3]='Q'; board[4]='K'; board[5]='B'; board[6]='N'; board[7]='R';
    for (int i = 8; i < 16; i++) board[i] = 'P';
    for (int i = 48; i < 56; i++) board[i] = 'p';
    board[56]='r'; board[57]='n'; board[58]='b'; board[59]='q'; board[60]='k'; board[61]='b'; board[62]='n'; board[63]='r';
}

void AppendLog(const string& msg) {
    if (!hLog) return;
    int len = GetWindowTextLengthA(hLog);
    char* old = (char*)malloc(len + 1);
    GetWindowTextA(hLog, old, len + 1);
    char* newtxt = (char*)malloc(len + (int)msg.size() + 3);
    strcpy(newtxt, old);
    strcat(newtxt, "\r\n");
    strcat(newtxt, msg.c_str());
    SetWindowTextA(hLog, newtxt);
    free(old);
    free(newtxt);
}

int king_distance_score(int sq, bool targetWhiteKing) {
    char king = targetWhiteKing ? 'K' : 'k';
    int kingSq = -1;
    for (int i = 0; i < 64; i++) if (board[i] == king) { kingSq = i; break; }
    if (kingSq == -1) return 0;
    return 14 - (abs((sq % 8) - (kingSq % 8)) + abs((sq / 8) - (kingSq / 8)));
}

int tradeClassScore(char attacker, char victim) {
    int a = pieceValue(attacker), v = pieceValue(victim);
    if (a > v) return -1; if (a < v) return 1; return 0;
}

int score_move_profile(const char b[64], int from, int to, BotProfile profile, bool moverWhite) {
    char moving = b[from], target = b[to];
    int captureScore = 0, tradeClass = 0;
    if (!isEmptySquare(target) && isOppositeColor(moving, target)) {
        captureScore = pieceValue(target) * 10 - pieceValue(moving) * 2;
        tradeClass = tradeClassScore(moving, target);
    }
    char temp[64]; copy_board_state(b, temp);
    temp[to] = moving; temp[from] = ' ';
    if (temp[to] == 'P' && to / 8 == 7) temp[to] = 'Q';
    if (temp[to] == 'p' && to / 8 == 0) temp[to] = 'p';
    int checkBonus = is_in_check(!moverWhite, temp) ? 40 : 0;
    int kingThreat = 0;
    if (profile == STRATEGIC) { kingThreat += king_distance_score(to, !moverWhite); if (checkBonus) kingThreat += 60; }
    int tradePenalty = 0;
    if (profile == SAFE && !isEmptySquare(target) && isOppositeColor(moving, target)) {
        if (tradeClass < 0) tradePenalty = 120;
        else if (tradeClass == 0) tradePenalty = 30;
        else tradePenalty = -20;
    }
    int score = 0;
    switch (profile) {
        case OFFENSIVE: score = 5 * captureScore + 2 * checkBonus; break;
        case SAFE: score = captureScore - 3 * tradePenalty; break;
        case STRATEGIC: score = 2 * captureScore + 4 * kingThreat + 3 * checkBonus; break;
    }
    return score;
}

vector<pair<int,int> > generate_moves(bool whiteSide) {
    vector<pair<int,int> > moves;
    for (int from = 0; from < 64; from++) {
        char piece = board[from];
        if (isEmptySquare(piece)) continue;
        if (whiteSide && !isWhitePiece(piece)) continue;
        if (!whiteSide && !isBlackPiece(piece)) continue;
        for (int to = 0; to < 64; to++) if (is_legal_move(from, to, whiteSide)) moves.push_back(pair<int,int>(from, to));
    }
    return moves;
}

pair<int,int> predict_best_move(bool whiteSide) {
    vector<pair<int,int> > moves = generate_moves(whiteSide);
    if (moves.empty()) return pair<int,int>(-1, -1);
    int bestScore = INT_MIN; pair<int,int> bestMove = moves[0];
    for (size_t i = 0; i < moves.size(); i++) {
        int score = score_move_profile(board, moves[i].first, moves[i].second, botProfile, whiteSide);
        if (score > bestScore) { bestScore = score; bestMove = moves[i]; }
    }
    return bestMove;
}

bool has_king(bool whiteSide) {
    char king = whiteSide ? 'K' : 'k';
    for (int i = 0; i < 64; i++) if (board[i] == king) return true;
    return false;
}

void RefreshBoard() {
    char buf[3];
    for (int i = 0; i < 64; i++) {
        if (board[i] == ' ') strcpy(buf, "");
        else { buf[0] = board[i]; buf[1] = 0; }
        SetWindowTextA(hSquareBtn[i], buf);
    }
    char s1[128];
    sprintf(s1, "White: %d   Black: %d", userScore, botScore);
    SetWindowTextA(hScore, s1);
    const char* st = isRunning ? (isUsersTurn ? "Your turn." : "Bot's turn.") : "Game not running. Select side and profile, then Start.";
    SetWindowTextA(hStatus, st);
}

void MakeBotMove() {
    bool botWhite = !isUserWhite;
    pair<int,int> mv = predict_best_move(botWhite);
    if (mv.first == -1) {
        AppendLog("Bot has no moves.");
        isRunning = false;
        RefreshBoard();
        return;
    }
    char buf[256];
    sprintf(buf, "Bot: %s -> %s", index_to_algebraic(mv.first).c_str(), index_to_algebraic(mv.second).c_str());
    AppendLog(buf);
    apply_move(mv.first, mv.second, botWhite);
    isUsersTurn = !isUsersTurn;
    RefreshBoard();
    if (!has_king(true)) { AppendLog("Black wins."); isRunning = false; }
    if (!has_king(false)) { AppendLog("White wins."); isRunning = false; }
    RefreshBoard();
}

void OnSquareClicked(int idx) {
    if (!isRunning) { AppendLog("Start the game first."); return; }
    if (!isUsersTurn) { AppendLog("Bot's turn."); return; }

    char p = board[idx];
    if (selectedSquare == -1) {
        if (isEmptySquare(p)) { AppendLog("Select one of your pieces."); return; }
        if (isUserWhite && !isWhitePiece(p)) { AppendLog("Not your piece."); return; }
        if (!isUserWhite && !isBlackPiece(p)) { AppendLog("Not your piece."); return; }
        selectedSquare = idx;
        char buf[64];
        sprintf(buf, "Selected: %s", index_to_algebraic(idx).c_str());
        AppendLog(buf);
        return;
    }

    int from = selectedSquare, to = idx;
    selectedSquare = -1;
    if (!is_legal_move(from, to, isUserWhite)) {
        AppendLog("Illegal move.");
        return;
    }

    char buf[256];
    sprintf(buf, "You: %s -> %s", index_to_algebraic(from).c_str(), index_to_algebraic(to).c_str());
    AppendLog(buf);
    apply_move(from, to, isUserWhite);
    isUsersTurn = !isUsersTurn;
    RefreshBoard();

    if (!has_king(true)) { AppendLog("Black wins."); isRunning = false; RefreshBoard(); return; }
    if (!has_king(false)) { AppendLog("White wins."); isRunning = false; RefreshBoard(); return; }

    Sleep(150);
    MakeBotMove();
}

void OnStartGame() {
    initialize_game_state();
    isUserWhite = (SendMessage(hSideCombo, CB_GETCURSEL, 0, 0) == 0);
    botProfile = (BotProfile)SendMessage(hProfileCombo, CB_GETCURSEL, 0, 0);
    isRunning = true;
    isUsersTurn = isUserWhite;
    selectedSquare = -1;
    SetWindowTextA(hLog, "");
    const char* side = isUserWhite ? "White" : "Black";
    char buf[256];
    sprintf(buf, "Selected side: %s", side);
    AppendLog(buf);
    const char* prof = (botProfile == OFFENSIVE) ? "Offensive" : (botProfile == SAFE ? "Safe" : "Strategic");
    sprintf(buf, "Profile: %s", prof);
    AppendLog(buf);
    if (!isUsersTurn) {
        AppendLog("Bot starts.");
        MakeBotMove();
    }
    RefreshBoard();
}

void OnResetGame() {
    initialize_game_state();
    selectedSquare = -1;
    SetWindowTextA(hLog, "");
    AppendLog("Game reset.");
    RefreshBoard();
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE: {
            initialize_game_state();

            hStatus = CreateWindowA("STATIC", "Select side and profile, then Start.",
                WS_CHILD | WS_VISIBLE | SS_LEFT,
                10, 10, 300, 20, hwnd, NULL, NULL, NULL);
            hScore = CreateWindowA("STATIC", "White: 0   Black: 0",
                WS_CHILD | WS_VISIBLE | SS_LEFT,
                10, 35, 300, 20, hwnd, NULL, NULL, NULL);

            CreateWindowA("STATIC", "Side:", WS_CHILD | WS_VISIBLE, 10, 60, 40, 20, hwnd, NULL, NULL, NULL);
            hSideCombo = CreateWindowA("COMBOBOX", "", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST,
                55, 58, 80, 200, hwnd, NULL, NULL, NULL);
            SendMessage(hSideCombo, CB_ADDSTRING, 0, (LPARAM)"White");
            SendMessage(hSideCombo, CB_ADDSTRING, 0, (LPARAM)"Black");
            SendMessage(hSideCombo, CB_SETCURSEL, 0, 0);

            CreateWindowA("STATIC", "Profile:", WS_CHILD | WS_VISIBLE, 150, 60, 60, 20, hwnd, NULL, NULL, NULL);
            hProfileCombo = CreateWindowA("COMBOBOX", "", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST,
                215, 58, 120, 200, hwnd, NULL, NULL, NULL);
            SendMessage(hProfileCombo, CB_ADDSTRING, 0, (LPARAM)"Offensive");
            SendMessage(hProfileCombo, CB_ADDSTRING, 0, (LPARAM)"Safe");
            SendMessage(hProfileCombo, CB_ADDSTRING, 0, (LPARAM)"Strategic");
            SendMessage(hProfileCombo, CB_SETCURSEL, 0, 0);

            hStartBtn = CreateWindowA("BUTTON", "Start Game",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                350, 55, 100, 25, hwnd, NULL, NULL, NULL);
            hResetBtn = CreateWindowA("BUTTON", "Reset",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                460, 55, 80, 25, hwnd, NULL, NULL, NULL);

            int boardX = 10, boardY = 100;
            int btnSize = 56;
            int gap = 2;
            for (int rank = 7; rank >= 0; --rank) {
                for (int file = 0; file < 8; ++file) {
                    int idx = rank * 8 + file;
                    int x = boardX + file * (btnSize + gap);
                    int y = boardY + (7 - rank) * (btnSize + gap);
                    hSquareBtn[idx] = CreateWindowA("BUTTON", pieceToLabel(board[idx]).c_str(),
                        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                        x, y, btnSize, btnSize, hwnd, NULL, NULL, NULL);
                }
            }
            int boardHeight = 8 * (btnSize + gap) - gap;
            int boardWidth = 8 * (btnSize + gap) - gap;

            hLog = CreateWindowA("EDIT", "",
                WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_READONLY | WS_VSCROLL,
                boardX + boardWidth + 20, boardY, 300, boardHeight, hwnd, NULL, NULL, NULL);

            SetWindowPos(hwnd, NULL, 0, 0, boardX + boardWidth + 320, boardY + boardHeight + 120, SWP_NOMOVE | SWP_NOZORDER);
            RefreshBoard();
            break;
        }
        case WM_COMMAND: {
            int wmId = LOWORD(wParam);
            if (wmId == IDOK || hStartBtn == (HWND)lParam) {
                OnStartGame();
            } else if (hResetBtn == (HWND)lParam) {
                OnResetGame();
            } else {
                for (int i = 0; i < 64; i++) {
                    if (hSquareBtn[i] == (HWND)lParam) {
                        OnSquareClicked(i);
                        break;
                    }
                }
            }
            break;
        }
        case WM_CLOSE:
            DestroyWindow(hwnd);
            break;
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR cmdLine, int cmdShow) {
    (void)hPrev; (void)cmdLine; (void)cmdShow;

    WNDCLASSEX wc = {0};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpszClassName = "PickleBotChessClass";
    RegisterClassEx(&wc);

    hFrame = CreateWindowEx(
        0, "PickleBotChessClass", "PickleBot Chess",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 900, 720,
        NULL, NULL, hInst, NULL);

    ShowWindow(hFrame, SW_SHOW);
    UpdateWindow(hFrame);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return (int)msg.wParam;
}
