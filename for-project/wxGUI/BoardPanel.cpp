#include "BoardPanel.h"
#include "MainFrame.h"
#include <wx/dcclient.h>
#include <wx/msgdlg.h>

BEGIN_EVENT_TABLE(BoardPanel, wxPanel)
    EVT_PAINT(BoardPanel::OnPaint)
    EVT_LEFT_DOWN(BoardPanel::OnMouseLeftDown)
END_EVENT_TABLE()

BoardPanel::BoardPanel(wxWindow* parent, MainFrame* frame)
    : wxPanel(parent, wxID_ANY), frame(frame)
{
    squareSize = 60;
}

void BoardPanel::OnPaint(wxPaintEvent& event) {
    wxPaintDC dc(this);
    int w, h;
    GetClientSize(&w, &h);
    int size = (w < h ? w : h) - 20;
    squareSize = size / 8;
    int offset = (w - 8 * squareSize) / 2;

    dc.SetBackground(*wxWHITE_BRUSH);
    dc.Clear();

    // Draw board
    for (int rank = 0; rank < 8; ++rank) {
        for (int file = 0; file < 8; ++file) {
            int idx = (7 - rank) * 8 + file;  // match algebraic
            bool dark = (rank + file) % 2 == 1;

            dc.SetBrush(dark ? *wxBLACK_BRUSH : *wxWHITE_BRUSH);
            dc.SetPen(*wxBLACK_PEN);
            dc.DrawRectangle(offset + file * squareSize,
                             offset + rank * squareSize,
                             squareSize, squareSize);

            // Draw piece
            char p = frame->boardState[idx];
            if (p != ' ') {
                dc.SetFont(wxFont(18, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
                dc.SetTextForeground(dark ? *wxWHITE : *wxBLACK);
                wxString s(1, p);
                int tx = offset + file * squareSize + squareSize / 2 - dc.GetTextExtent(s).GetX() / 2;
                int ty = offset + rank * squareSize + squareSize / 2 + dc.GetTextExtent(s).GetY() / 2;
                dc.DrawText(s, tx, ty - dc.GetTextExtent(s).GetY() / 2);
            }
        }
    }

    // Draw coordinates
    dc.SetTextForeground(*wxBLACK);
    dc.SetFont(wxFont(12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
    // Files A-H
    for (int f = 0; f < 8; ++f) {
        wxString s; s << (char)('A' + f);
        int tx = offset + f * squareSize + squareSize / 2 - dc.GetTextExtent(s).GetX() / 2;
        int ty = offset + 8 * squareSize + 5;
        dc.DrawText(s, tx, ty);
    }
    // Ranks 1-8
    for (int r = 0; r < 8; ++r) {
        wxString s; s << (8 - r);
        int tx = offset - 15;
        int ty = offset + r * squareSize + squareSize / 2 - dc.GetTextExtent(s).GetY() / 2;
        dc.DrawText(s, tx, ty);
    }
}

int selectedSquare = -1;

void BoardPanel::OnMouseLeftDown(wxMouseEvent& event) {
    int w, h;
    GetClientSize(&w, &h);
    int size = (w < h ? w : h) - 20;
    int s = size / 8;
    int offset = (w - 8 * s) / 2;

    int x = event.GetX() - offset;
    int y = event.GetY() - offset;

    if (x < 0 || y < 0 || x >= 8 * s || y >= 8 * s) {
        selectedSquare = -1;
        return;
    }

    int file = x / s;
    int rank = y / s;
    int idx = (7 - rank) * 8 + file;

    if (selectedSquare == -1) {
        selectedSquare = idx;
    } else {
        int from = selectedSquare;
        int to = idx;
        selectedSquare = -1;

        if (from == to) return;

        frame->MakeUserMove(from, to);
    }

    Refresh();
}
