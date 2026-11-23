#pragma once

#include <windows.h>
#include "Game.h"

namespace Gomoku
{
class MainWindow
{
public:
    explicit MainWindow(HINSTANCE instance);
    bool Create();
    void Run();

private:
    static LRESULT CALLBACK WindowProcedure(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMessage(UINT message, WPARAM wParam, LPARAM lParam);
    bool TryGetCell(int x, int y, int& row, int& col) const;
    void Draw(HDC hdc);
    void DrawPanel(HDC hdc);
    void DrawSidePanel(HDC hdc);
    void DrawStone(HDC hdc, int cx, int cy, Stone stone, int radius, int alpha);

    HINSTANCE m_instance = nullptr;
    HWND m_hwnd = nullptr;
    Game m_game;
    int m_hoverRow = -1;
    int m_hoverCol = -1;
    bool m_hoverValid = false;
    bool m_trackingMouse = false;
};
}
