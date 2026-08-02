#include "MainFrame.h"
#include "BoardPanel.h"
#include <wx/msgdlg.h>
#include <wx/textdlg.h>
#include <thread>
#include <chrono>

// -------------- Your original chess logic (paste your functions here) --------------
// Make sure these are visible to this file:
//  - isWhitePiece, isBlackPiece, isEmptySquare, isOppositeColor, pieceValue
//  - algebraic_to_index, index_to_algebraic, is_legal_move, generate_moves
//  - apply_move, update_turn, finds_king_square, square_attacked_by_side
//  - gives_check_after_move, copy_board_state, apply_move_on_board
//  - score_move_profile, BotProfile enum (if not included here)
//
// For now, I'll put minimal placeholders. REPLACE THESE WITH YOUR REAL CODE.

bool isWhitePiece(char c);
bool isBlackPiece(char c);
bool isEmptySquare(char c);
bool isOppositeColor(char a, char b);
int pieceValue(char c);
int algebraic_to_index(const std::string& s);
std::string index_to_algebraic(int idx);
bool is_legal_move(int from, int to, bool whitesTurn);
std::vector<std::pair<int,int>> generate_moves(bool whiteSide);
void apply_move_impl(int from, int to, char board[64], short& userScore, short& botScore, bool isUserWhite);
int score_move_profile(const char b[64], int from, int to, BotProfile profile, bool moverWhite);
bool gives_check_after_move(const char b[64], int from, int to, bool moverWhite);
int find_king_square(bool whiteSide, const char b[64]);

// -------------- MainFrame implementation --------------

MainFrame::MainFrame(const wxString& title)
    : wxFrame(nullptr, wxID_ANY, title, wxDefaultPosition, wxSize(700, 750))
{
    botProfile = OFFENSIVE;

    // Menu bar
    wxMenuBar* menubar = new wxMenuBar;

    wxMenu* fileMenu = new wxMenu;
    fileMenu->Append(wxID_NEW, "&New Game\tCtrl+N");
    fileMenu->AppendSeparator();
    fileMenu->Append(wxID_EXIT, "E&xit\tCtrl+Q");

    wxMenu* gameMenu = new wxMenu;
    gameMenu->Append(wxID_SELECTALL, "&Choose Side\tCtrl+S");  // we'll use custom handlers
    gameMenu->Append(wxID_PREFERENCES, "Cho&ose Bot Profile\tCtrl+P");

    wxMenu* helpMenu = new wxMenu;
    helpMenu->Append(wxID_ABOUT, "&About");

    menubar->Append(fileMenu, "&File");
    menubar->Append(gameMenu, "&Game");
    menubar->Append(helpMenu, "&Help");

    SetMenuBar(menubar);

    // Bind menu events
    Bind(wxEVT_MENU, &MainFrame::OnNewGame, this, wxID_NEW);
    Bind(wxEVT_MENU, &MainFrame::OnChooseSide, this, wxID_SELECTALL);
    Bind(wxEVT_MENU, &MainFrame::OnChooseProfile, this, wxID_PREFERENCES);
    Bind(wxEVT_MENU, &MainFrame::OnExit, this, wxID_EXIT);
    Bind(wxEVT_MENU, &MainFrame::OnExit, this, wxID_EXIT);

    // Layout: board on top, log on bottom
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

    boardPanel = new BoardPanel(this, this);  // parent frame passed so board can call back
    mainSizer->Add(boardPanel, 1, wxEXPAND);

    logCtrl = new wxTextCtrl(this, wxID_ANY, wxEmptyString,
        wxDefaultPosition, wxDefaultSize,
        wxTE_MULTILINE | wxTE_READONLY);
    mainSizer->Add(logCtrl, 1, wxEXPAND | wxTOP, 5);

    SetSizer(mainSizer);

    InitializeGame();
    UpdateLog("Welcome to PickleBot Chess (GUI version).");
    UpdateLog("Choose side from Game -> Choose Side.");
}

MainFrame::~MainFrame() {}

void MainFrame::OnNewGame(wxCommandEvent& event) {
    initialize_game_state();  // your existing function
    InitializeGame();         // call our wrapper
    UpdateLog("New game started.");
    RefreshBoard();
}

void MainFrame::OnChooseSide(wxCommandEvent& event) {
    wxString choice = wxGetSingleChoice(
        "Choose your side:",
        "Choose Side",
        {"White", "Black"},
        this
    );

    if (choice == "White") {
        isUserWhite = true;
        isUsersTurn = true;
        UpdateLog("You are White. Your turn.");
    } else if (choice == "Black") {
        isUserWhite = false;
        isUsersTurn = false;
        UpdateLog("You are Black. Bot starts its turn.");
        TriggerBotMove();
    } else {
        UpdateLog("Side choice cancelled.");
        return;
    }

    isRunning = true;
    RefreshBoard();
}

void MainFrame::OnChooseProfile(wxCommandEvent& event) {
    wxString choice = wxGetSingleChoice(
        "Choose bot profile:",
        "Bot Profile",
        {"Offensive", "Safe", "Strategic"},
        this
    );

    if (choice == "Offensive") botProfile = OFFENSIVE;
    else if (choice == "Safe") botProfile = SAFE;
    else if (choice == "Strategic") botProfile = STRATEGIC;
    else {
        UpdateLog("Profile choice cancelled. Using Offensive.");
        botProfile = OFFENSIVE;
        return;
    }

    wxString name =
        (botProfile == OFFENSIVE) ? "Offensive" :
        (botProfile == SAFE) ? "Safe" : "Strategic";

    UpdateLog("Bot profile set to: " + name);
}

void MainFrame::OnExit(wxCommandEvent& event) {
    Close(true);
}

void MainFrame::InitializeGame() {
    move_num = 0;
    userScore = 0;
    botScore = 0;
    isRunning = false;
    isUsersTurn = false;

    for (int i = 0; i < 64; i++) boardState[i] = ' ';

    // Initial position (same as your original)
    boardState[0] = 'R'; boardState[1] = 'N'; boardState[2] = 'B'; boardState[3] = 'Q';
    boardState[4] = 'K'; boardState[5] = 'B'; boardState[6] = 'N'; boardState[7] = 'R';
    for (int i = 8; i < 16; i++) boardState[i] = 'P';

    for (int i = 48; i < 56; i++) boardState[i] = 'p';
    boardState[56] = 'r'; boardState[57] = 'n'; boardState[58] = 'b'; boardState[59] = 'q';
    boardState[60] = 'k'; boardState[61] = 'b'; boardState[62] = 'n'; boardState[63] = 'r';
}

void MainFrame::UpdateLog(const wxString& msg) {
    logCtrl->AppendText(msg + "\n");
    logCtrl->ShowPosition(logCtrl->GetLastPosition());
}

void MainFrame::RefreshBoard() {
    boardPanel->Refresh();
}

void MainFrame::MakeUserMove(int from, int to) {
    if (!isRunning || !isUsersTurn) {
        UpdateLog("It is not your turn.");
        return;
    }

    if (!is_legal_move(from, to, isUserWhite)) {
        UpdateLog("Illegal move.");
        return;
    }

    apply_move_impl(from, to, boardState, userScore, botScore, isUserWhite);
    isUsersTurn = !isUsersTurn;
    move_num++;
    RefreshBoard();

    UpdateLog("You: " + wxString(index_to_algebraic(from)) + " -> " + wxString(index_to_algebraic(to)));

    if (!has_king(true)) {
        UpdateLog("Black wins (White king missing).");
        isRunning = false;
        return;
    }
    if (!has_king(false)) {
        UpdateLog("White wins (Black king missing).");
        isRunning = false;
        return;
    }

    TriggerBotMove();
}

void MainFrame::TriggerBotMove() {
    if (!isRunning || isUsersTurn) return;

    UpdateLog("---===*&*&*&*===---");
    UpdateLog("Bot is thinking...");
    wxString pname =
        (botProfile == OFFENSIVE) ? "offensive" :
        (botProfile == SAFE) ? "safe" : "strategic";
    UpdateLog("Profile: " + pname);

    bool botWhite = !isUserWhite;
    auto moves = generate_moves(botWhite);

    UpdateLog(wxString::Format("Legal moves found: %d", (int)moves.size()));

    if (moves.empty()) {
        UpdateLog("Bot has no legal moves. Game over.");
        isRunning = false;
        return;
    }

    int bestScore = INT_MIN;
    std::pair<int,int> bestMove = moves[0];

    for (auto mv : moves) {
        int score = score_move_profile(boardState, mv.first, mv.second, botProfile, botWhite);

        UpdateLog(wxString::Format("Considering %s -> %s | score = %d",
            index_to_algebraic(mv.first).c_str(),
            index_to_algebraic(mv.second).c_str(),
            score));

        std::this_thread::sleep_for(std::chrono::milliseconds(80));

        if (score > bestScore) {
            bestScore = score;
            bestMove = mv;

            UpdateLog(wxString::Format("New best move: %s -> %s | best score = %d",
                index_to_algebraic(bestMove.first).c_str(),
                index_to_algebraic(bestMove.second).c_str(),
                bestScore));

            std::this_thread::sleep_for(std::chrono::milliseconds(80));
        }
    }

    UpdateLog(wxString::Format("Bot selected: %s %s",
        index_to_algebraic(bestMove.first).c_str(),
        index_to_algebraic(bestMove.second).c_str()));

    apply_move_impl(bestMove.first, bestMove.second, boardState, userScore, botScore, isUserWhite);
    isUsersTurn = !isUsersTurn;
    move_num++;
    RefreshBoard();

    if (!has_king(true)) {
        UpdateLog("Black wins.");
        isRunning = false;
        return;
    }
    if (!has_king(false)) {
        UpdateLog("White wins.");
        isRunning = false;
        return;
    }
}

// Simple has_king using boardState:
bool has_king(bool whiteSide) {
    for (int i = 0; i < 64; i++) {
        if (whiteSide && boardState[i] == 'K') return true;
        if (!whiteSide && boardState[i] == 'k') return true;
    }
    return false;
}
