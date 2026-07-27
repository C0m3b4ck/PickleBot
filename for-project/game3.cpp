#include <iostream>
#include <string>
#include <vector>
#include <cctype>
#include <algorithm>
#include <random>
#include <array>

using namespace std;

char board[64];
short userScore = 0, botScore = 0;
bool isUserWhite = true;
bool isUsersTurn = true;
bool isRunning = false;
int move_num = 0;

const int PIECE_VALUE_PAWN = 1;
const int PIECE_VALUE_KNIGHT = 3;
const int PIECE_VALUE_BISHOP = 3;
const int PIECE_VALUE_ROOK = 5;
const int PIECE_VALUE_QUEEN = 9;

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

    for (int i = 0; i < 64; i++) board[i] = ' ';

    board[0] = 'R'; board[1] = 'N'; board[2] = 'B'; board[3] = 'Q';
    board[4] = 'K'; board[5] = 'B'; board[6] = 'N'; board[7] = 'R';
    for (int i = 8; i < 16; i++) board[i] = 'P';

    for (int i = 48; i < 56; i++) board[i] = 'p';
    board[56] = 'r'; board[57] = 'n'; board[58] = 'b'; board[59] = 'q';
    board[60] = 'k'; board[61] = 'b'; board[62] = 'n'; board[63] = 'r';
}

void print_welcome() {
    cout << "---=== Project codename 'PickleBot' ===---\n";
    cout << "A simple non-ML chess bot.\n";
    cout << "/// By C0m3b4ck under APL 2.0 ///\n";
}

void print_board() {
    move_num++;
    cout << "\n---=== ROUND " << move_num << " ===---\n";
    cout << "User score: " << userScore << "\n";
    cout << "Bot score: " << botScore << "\n\n";
    cout << "  A B C D E F G H\n";

    for (int rank = 7; rank >= 0; rank--) {
        cout << (rank + 1) << " ";
        for (int file = 0; file < 8; file++) {
            cout << board[rank * 8 + file] << " ";
        }
        cout << "\n";
    }
}

void get_side_choice() {
    char tmp;
    cout << "\nB/b for black, W/w for white";
    cout << "\nSelect a side, user: ";
    cin >> tmp;

    if (tmp == 'W' || tmp == 'w') {
        cout << "\nSelected: white\n";
        isUserWhite = true;
        isRunning = true;
        isUsersTurn = true;
    } else if (tmp == 'B' || tmp == 'b') {
        cout << "\nSelected: black\n";
        isUserWhite = false;
        isRunning = true;
        isUsersTurn = false;
    } else {
        cout << "\n!!! Incorrect input !!! Please try again.\n";
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

    if (fromFile == toFile && isEmptySquare(board[to])) {
        if (toRank - fromRank == dir) return true;
        if (fromRank == startRank && toRank - fromRank == 2 * dir) {
            int mid = from + 8 * dir;
            return isEmptySquare(board[mid]);
        }
    }

    if (abs(toFile - fromFile) == 1 && toRank - fromRank == dir && !isEmptySquare(board[to])) {
        if (isOppositeColor(piece, board[to])) return true;
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

bool is_legal_move(int from, int to, bool whitesTurn) {
    if (from < 0 || from > 63 || to < 0 || to > 63 || from == to) return false;

    char piece = board[from];
    char target = board[to];

    if (isEmptySquare(piece)) return false;
    if (whitesTurn && !isWhitePiece(piece)) return false;
    if (!whitesTurn && !isBlackPiece(piece)) return false;
    if (!isEmptySquare(target) && isSameColor(piece, target)) return false;

    switch (tolower(piece)) {
        case 'p': return valid_pawn_move(from, to, piece);
        case 'n': return valid_knight_move(from, to);
        case 'b': return valid_bishop_move(from, to);
        case 'r': return valid_rook_move(from, to);
        case 'q': return valid_queen_move(from, to);
        case 'k': return valid_king_move(from, to);
        default: return false;
    }
}

void apply_move(int from, int to) {
    char moving = board[from];
    char captured = board[to];

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

void update_turn() {
    isUsersTurn = !isUsersTurn;
}

void update_move(int from, int to) {
    apply_move(from, to);
    update_turn();
}

void get_move_choice() {
    string from_s, to_s;
    cout << "\n--- YOUR MOVE ---\n";
    cout << "Enter move like (from, to): ";
    cin >> from_s >> to_s;

    int from = algebraic_to_index(from_s);
    int to = algebraic_to_index(to_s);

    if (from == -1 || to == -1) {
        cout << "\n!!! Invalid input !!! Try again.\n";
        return;
    }

    if (!is_legal_move(from, to, isUserWhite)) {
        cout << "\n!!! Illegal move !!! Try again.\n";
        return;
    }

    update_move(from, to);
}

// ================================
// MOVE ANALYSIS / PREDICTION HELPERS
// ================================

void copy_board_state(const char src[64], char dst[64]) {
    for (int i = 0; i < 64; i++) dst[i] = src[i];
}

// Use this for prediction only, not real game state.
void apply_move_on_board(char b[64], int from, int to) {
    char moving = b[from];
    b[to] = moving;
    b[from] = ' ';
    if (moving == 'P' && to / 8 == 7) b[to] = 'Q';
    if (moving == 'p' && to / 8 == 0) b[to] = 'q';
}

// Use this to ask: what piece is on square?
char piece_at(int idx) {
    if (idx < 0 || idx > 63) return ' ';
    return board[idx];
}

// Use this to get all legal moves for a side.
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

// Use this for simple move scoring / prediction.
int evaluate_move_basic(int from, int to) {
    int score = 0;
    char moving = board[from];
    char target = board[to];

    if (isOppositeColor(moving, target)) {
        score += pieceValue(target) * 10;
    }

    int tf = to % 8, tr = to / 8;
    if ((tf == 3 || tf == 4) && (tr == 3 || tr == 4)) score += 1;

    if (tolower(moving) == 'n') {
        if ((tf == 3 || tf == 4) && (tr == 3 || tr == 4)) score += 2;
    }

    return score;
}

// Use this if you want to analyze one move on a copied board.
int evaluate_move_on_board(const char b[64], int from, int to) {
    int score = 0;
    char moving = b[from];
    char target = b[to];

    if (isOppositeColor(moving, target)) {
        score += pieceValue(target) * 10;
    }

    int tf = to % 8, tr = to / 8;
    if ((tf == 3 || tf == 4) && (tr == 3 || tr == 4)) score += 1;

    return score;
}

// Use this to predict best move for a side.
pair<int,int> predict_best_move(bool whiteSide) {
    vector<pair<int,int>> moves = generate_moves(whiteSide);
    if (moves.empty()) return {-1, -1};

    int bestScore = -1000000;
    pair<int,int> bestMove = moves[0];

    char bcopy[64];

    for (auto mv : moves) {
        copy_board_state(board, bcopy);
        apply_move_on_board(bcopy, mv.first, mv.second);
        int score = evaluate_move_on_board(bcopy, mv.first, mv.second);

        if (score > bestScore) {
            bestScore = score;
            bestMove = mv;
        }
    }

    return bestMove;
}

// =================================

void bot_move() {
    bool botWhite = !isUserWhite;
    vector<pair<int,int>> moves = generate_moves(botWhite);

    if (moves.empty()) {
        cout << "\n !!! Bot has no legal moves. !!!\n";
        isRunning = false;
        return;
    }

    pair<int,int> bestMove = predict_best_move(botWhite);
    if (bestMove.first == -1) {
        static mt19937 rng((random_device())());
        uniform_int_distribution<int> dist(0, (int)moves.size() - 1);
        bestMove = moves[dist(rng)];
    }

    cout << "\nBot plays: " << index_to_algebraic(bestMove.first) << " " << index_to_algebraic(bestMove.second) << "\n";
    apply_move(bestMove.first, bestMove.second);
    update_turn();
}

bool has_king(bool whiteSide) {
    for (int i = 0; i < 64; i++) {
        if (whiteSide && board[i] == 'K') return true;
        if (!whiteSide && board[i] == 'k') return true;
    }
    return false;
}

int main() {
    initialize_game_state();
    print_welcome();
    get_side_choice();

    while (isRunning) {
        if (!has_king(true)) {
            cout << "\nBlack wins.\n";
            break;
        }
        if (!has_king(false)) {
            cout << "\nWhite wins.\n";
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
