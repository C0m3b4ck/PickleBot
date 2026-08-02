#pragma once
#include <wx/frame.h>
#include <wx/menu.h>
#include <wx/panel.h>
#include <wx/textctrl.h>
#include <wx/sizer.h>
#include <vector>
#include <string>

enum BotProfile {
    OFFENSIVE,
    SAFE,
    STRATEGIC
};

// Forward declaration
class BoardPanel;

class MainFrame : public wxFrame {
public:
    MainFrame(const wxString& title);
    ~MainFrame();

private:
    void OnNewGame(wxCommandEvent& event);
    void OnChooseSide(wxCommandEvent& event);
    void OnChooseProfile(wxCommandEvent& event);
    void OnExit(wxCommandEvent& event);

    void MakeUserMove(int from, int to);
    void TriggerBotMove();

    BoardPanel* boardPanel;
    wxTextCtrl* logCtrl;

    char boardState[64];
    short userScore;
    short botScore;
    bool isUserWhite;
    bool isUsersTurn;
    bool isRunning;
    int move_num;
    BotProfile botProfile;

    void InitializeGame();
    void UpdateLog(const wxString& msg);
    void RefreshBoard();
};
