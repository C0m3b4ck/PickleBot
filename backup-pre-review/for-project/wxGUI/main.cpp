#include <wx/wx.h>
#include "MainFrame.h"

class ChessApp : public wxApp {
public:
    virtual bool OnInit() override {
        MainFrame* frame = new MainFrame("PickleBot Chess - Math Project");
        frame->Show(true);
        return true;
    }
};

wxIMPLEMENT_APP(ChessApp);
