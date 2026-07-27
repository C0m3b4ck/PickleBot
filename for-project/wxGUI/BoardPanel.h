#pragma once
#include <wx/panel.h>
#include <wx/clientDC.h>
#include <wx/font.h>

class MainFrame;

class BoardPanel : public wxPanel {
public:
    BoardPanel(wxWindow* parent, MainFrame* frame);

protected:
    void OnPaint(wxPaintEvent& event);
    void OnMouseLeftDown(wxMouseEvent& event);

private:
    MainFrame* frame;
    int squareSize;

    DECLARE_EVENT_TABLE()
};
