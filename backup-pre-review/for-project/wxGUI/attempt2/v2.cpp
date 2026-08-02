#include <wx/wx.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/button.h>
#include <wx/choice.h>
#include <vector>
#include <string>
#include <cctype>
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

wxButton* squareButtons[64];
int selectedSquare = -1;
wxTextCtrl* logBox = nullptr;
wxStaticText* statusText = nullptr;
wxStaticText* scoreText = nullptr;
wxChoice* sideChoice = nullptr;
wxChoice* profileChoice = nullptr;
wxButton* startBtn = nullptr;
wxButton* resetBtn = nullptr;
wxPanel* mainPanel = nullptr;

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

wxString pieceToLabel(char p) {
    if (p == ' ') return " ";
    return wxString::Format("%c", p);
}

int algebraic_to_index(const string& s) {
    if (s.size() != 2) return -1;
    char file = (char)tolower(s[0]);
    char rank = s[1];
    if (file < 'a' || file > 'h' || rank < '1' || rank > '8') return -1;
    return (rank - '1') * 8 + (file - 'a');
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
        if (temp[to] == 'P' && to / 8 == 7) temp[to] = 'P';
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

void RefreshBoard() {
    for (int i = 0; i < 64; i++) {
        squareButtons[i]->SetLabel(pieceToLabel(board[i]));
    }
    wxString s = wxString::Format("White: %d   Black: %d", userScore, botScore);
    scoreText->SetLabel(s);
    if (!isRunning) statusText->SetLabel("Game not running. Select side and profile, then Start.");
    else statusText->SetLabel(isUsersTurn ? "Your turn." : "Bot's turn.");
    if (mainPanel) mainPanel->GetContainingSizer()->Layout();
}

void AppendLog(const wxString& msg) {
    if (logBox) logBox->AppendText(msg + "\n");
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
    if (temp[to] == 'p' && to / 8 == 0) temp[to] = 'q';
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

vector<pair<int,int>> generate_moves(bool whiteSide) {
    vector<pair<int,int>> moves;
    for (int from = 0; from < 64; from++) {
        char piece = board[from];
        if (isEmptySquare(piece)) continue;
        if (whiteSide && !isWhitePiece(piece)) continue;
        if (!whiteSide && !isBlackPiece(piece)) continue;
        for (int to = 0; to < 64; to++) if (is_legal_move(from, to, whiteSide)) moves.push_back({from, to});
    }
    return moves;
}

pair<int,int> predict_best_move(bool whiteSide) {
    auto moves = generate_moves(whiteSide);
    if (moves.empty()) return {-1, -1};
    int bestScore = INT_MIN; pair<int,int> bestMove = moves[0];
    for (auto mv : moves) {
        int score = score_move_profile(board, mv.first, mv.second, botProfile, whiteSide);
        if (score > bestScore) { bestScore = score; bestMove = mv; }
    }
    return bestMove;
}

bool has_king(bool whiteSide) {
    char king = whiteSide ? 'K' : 'k';
    for (int i = 0; i < 64; i++) if (board[i] == king) return true;
    return false;
}

class ChessFrame : public wxFrame {
public:
    ChessFrame() : wxFrame(nullptr, wxID_ANY, "PickleBot Chess", wxDefaultPosition, wxSize(980, 720)) {
        initialize_game_state();
        mainPanel = new wxPanel(this);
        wxBoxSizer* root = new wxBoxSizer(wxHORIZONTAL);
        wxBoxSizer* left = new wxBoxSizer(wxVERTICAL);
        wxBoxSizer* right = new wxBoxSizer(wxVERTICAL);

        statusText = new wxStaticText(mainPanel, wxID_ANY, "Select side and profile, then Start.");
        scoreText = new wxStaticText(mainPanel, wxID_ANY, "White: 0   Black: 0");

        wxBoxSizer* topControls = new wxBoxSizer(wxHORIZONTAL);
        sideChoice = new wxChoice(mainPanel, wxID_ANY);
        sideChoice->Append("White"); sideChoice->Append("Black"); sideChoice->SetSelection(0);
        profileChoice = new wxChoice(mainPanel, wxID_ANY);
        profileChoice->Append("Offensive"); profileChoice->Append("Safe"); profileChoice->Append("Strategic"); profileChoice->SetSelection(0);
        startBtn = new wxButton(mainPanel, wxID_ANY, "Start Game");
        resetBtn = new wxButton(mainPanel, wxID_ANY, "Reset");

        topControls->Add(new wxStaticText(mainPanel, wxID_ANY, "Side:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
        topControls->Add(sideChoice, 0, wxRIGHT, 15);
        topControls->Add(new wxStaticText(mainPanel, wxID_ANY, "Profile:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
        topControls->Add(profileChoice, 0, wxRIGHT, 15);
        topControls->Add(startBtn, 0, wxRIGHT, 8);
        topControls->Add(resetBtn, 0);

        wxGridSizer* gridSizer = new wxGridSizer(8, 8, 2, 2);
        for (int rank = 7; rank >= 0; --rank) {
            for (int file = 0; file < 8; ++file) {
                int idx = rank * 8 + file;
                squareButtons[idx] = new wxButton(mainPanel, wxID_ANY, pieceToLabel(board[idx]), wxDefaultPosition, wxSize(56, 56));
                squareButtons[idx]->Bind(wxEVT_BUTTON, [this, idx](wxCommandEvent&) { OnSquareClicked(idx); });
                gridSizer->Add(squareButtons[idx], 1, wxEXPAND);
            }
        }

        logBox = new wxTextCtrl(mainPanel, wxID_ANY, "", wxDefaultPosition, wxSize(320, 520), wxTE_MULTILINE | wxTE_READONLY);

        left->Add(statusText, 0, wxALL, 6);
        left->Add(scoreText, 0, wxALL, 6);
        left->Add(topControls, 0, wxALL, 6);
        left->Add(gridSizer, 0, wxALL, 6);

        right->Add(new wxStaticText(mainPanel, wxID_ANY, "Move log"), 0, wxALL, 6);
        right->Add(logBox, 1, wxEXPAND | wxALL, 6);

        root->Add(left, 0, wxALL, 10);
        root->Add(right, 1, wxEXPAND | wxALL, 10);
        mainPanel->SetSizer(root);

        startBtn->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { OnStartGame(); });
        resetBtn->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { OnResetGame(); });

        selectedSquare = -1;
        RefreshBoard();
        AppendLog("Ready. Select side and profile.");
    }

private:
    void OnSquareClicked(int idx) {
        if (!isRunning) { AppendLog("Start the game first."); return; }
        if (!isUsersTurn) { AppendLog("Bot's turn."); return; }

        char p = board[idx];
        if (selectedSquare == -1) {
            if (isEmptySquare(p)) { AppendLog("Select one of your pieces."); return; }
            if (isUserWhite && !isWhitePiece(p)) { AppendLog("Not your piece."); return; }
            if (!isUserWhite && !isBlackPiece(p)) { AppendLog("Not your piece."); return; }
            selectedSquare = idx;
            AppendLog("Selected: " + index_to_algebraic(idx));
            return;
        }

        int from = selectedSquare, to = idx;
        selectedSquare = -1;
        if (!is_legal_move(from, to, isUserWhite)) {
            AppendLog("Illegal move.");
            return;
        }

        wxString msg = wxString::Format("You: %s -> %s", index_to_algebraic(from), index_to_algebraic(to));
        AppendLog(msg);
        apply_move(from, to, isUserWhite);
        isUsersTurn = !isUsersTurn;
        RefreshBoard();

        if (!has_king(true)) { AppendLog("Black wins."); isRunning = false; RefreshBoard(); return; }
        if (!has_king(false)) { AppendLog("White wins."); isRunning = false; RefreshBoard(); return; }

        this_thread::sleep_for(chrono::milliseconds(150));
        MakeBotMove();
    }

    void MakeBotMove() {
        bool botWhite = !isUserWhite;
        auto mv = predict_best_move(botWhite);
        if (mv.first == -1) { AppendLog("Bot has no moves."); isRunning = false; RefreshBoard(); return; }
        wxString msg = wxString::Format("Bot: %s -> %s", index_to_algebraic(mv.first), index_to_algebraic(mv.second));
        AppendLog(msg);
        apply_move(mv.first, mv.second, botWhite);
        isUsersTurn = !isUsersTurn;
        RefreshBoard();
        if (!has_king(true)) { AppendLog("Black wins."); isRunning = false; }
        if (!has_king(false)) { AppendLog("White wins."); isRunning = false; }
        RefreshBoard();
    }

    void OnStartGame() {
        initialize_game_state();
        isUserWhite = (sideChoice->GetSelection() == 0);
        botProfile = static_cast<BotProfile>(profileChoice->GetSelection());
        isRunning = true;
        isUsersTurn = isUserWhite;
        selectedSquare = -1;
        logBox->Clear();
        wxString side = isUserWhite ? "White" : "Black";
        AppendLog("Selected side: " + side);
        wxString prof = (botProfile == OFFENSIVE) ? "Offensive" : (botProfile == SAFE ? "Safe" : "Strategic");
        AppendLog("Profile: " + prof);
        if (!isUsersTurn) { AppendLog("Bot starts."); MakeBotMove(); }
        RefreshBoard();
    }

    void OnResetGame() {
        initialize_game_state();
        selectedSquare = -1;
        logBox->Clear();
        AppendLog("Game reset.");
        RefreshBoard();
    }
};

class MyApp : public wxApp {
public:
    bool OnInit() override {
        ChessFrame* frame = new ChessFrame();
        frame->Show(true);
        return true;
    }
};

wxIMPLEMENT_APP(MyApp);
