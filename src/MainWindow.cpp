#include "MainWindow.h"

#include <algorithm>
#include <cwchar>

namespace Gomoku
{
namespace
{
constexpr int BoardLeft = 48;
constexpr int BoardTop = 92;
constexpr int CellSize = 42;
constexpr int BoardSizePixels = CellSize * (BoardSize - 1);
constexpr int WindowWidth = 960;
constexpr int WindowHeight = 740;
constexpr int RestartId = 1001;

COLORREF Blend(COLORREF from, COLORREF to, int value)
{
    if (value < 0) value = 0;
    if (value > 255) value = 255;
    return RGB(
        (GetRValue(from) * (255 - value) + GetRValue(to) * value) / 255,
        (GetGValue(from) * (255 - value) + GetGValue(to) * value) / 255,
        (GetBValue(from) * (255 - value) + GetBValue(to) * value) / 255);
}

void FillRoundRect(HDC hdc, const RECT& rect, COLORREF color, int radius)
{
    HBRUSH brush = CreateSolidBrush(color);
    HPEN pen = CreatePen(PS_SOLID, 1, color);
    HGDIOBJ oldBrush = SelectObject(hdc, brush);
    HGDIOBJ oldPen = SelectObject(hdc, pen);
    RoundRect(hdc, rect.left, rect.top, rect.right, rect.bottom, radius, radius);
    SelectObject(hdc, oldBrush);
    SelectObject(hdc, oldPen);
    DeleteObject(pen);
    DeleteObject(brush);
}

void DrawTextLine(HDC hdc, const wchar_t* text, RECT rect, UINT format, COLORREF color, int height)
{
    HFONT font = CreateFontW(height, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Microsoft YaHei");
    HGDIOBJ oldFont = SelectObject(hdc, font);
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, color);
    DrawTextW(hdc, text, -1, &rect, format);
    SelectObject(hdc, oldFont);
    DeleteObject(font);
}
}

MainWindow::MainWindow(HINSTANCE instance)
    : m_instance(instance)
{
}

bool MainWindow::Create()
{
    const wchar_t className[] = L"NeonGomokuWindow";
    WNDCLASSW wc{};
    wc.lpfnWndProc = WindowProcedure;
    wc.hInstance = m_instance;
    wc.lpszClassName = className;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = CreateSolidBrush(RGB(7, 11, 24));

    RegisterClassW(&wc);

    m_hwnd = CreateWindowExW(
        0, className, L"NEON GOMOKU // 双人对战终端",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, WindowWidth, WindowHeight,
        nullptr, nullptr, m_instance, this);

    return m_hwnd != nullptr;
}

LRESULT CALLBACK MainWindow::WindowProcedure(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    MainWindow* window = nullptr;

    if (message == WM_NCCREATE)
    {
        auto create = reinterpret_cast<CREATESTRUCTW*>(lParam);
        window = static_cast<MainWindow*>(create->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window));
        window->m_hwnd = hwnd;
    }
    else
    {
        window = reinterpret_cast<MainWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    }

    if (window)
    {
        return window->HandleMessage(message, wParam, lParam);
    }
    return DefWindowProcW(hwnd, message, wParam, lParam);
}

LRESULT MainWindow::HandleMessage(UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CREATE:
        m_game.Initialize();
        return 0;

    case WM_ERASEBKGND:
        return 1;

    case WM_SIZE:
        InvalidateRect(m_hwnd, nullptr, TRUE);
        return 0;

    case WM_MOUSEMOVE:
    {
        int x = LOWORD(lParam);
        int y = HIWORD(lParam);
        int oldRow = m_hoverRow;
        int oldCol = m_hoverCol;
        bool oldValid = m_hoverValid;
        m_hoverValid = TryGetCell(x, y, m_hoverRow, m_hoverCol);

        if (!m_trackingMouse)
        {
            TRACKMOUSEEVENT event{ sizeof(event), TME_LEAVE, m_hwnd, 0 };
            TrackMouseEvent(&event);
            m_trackingMouse = true;
        }
        if (oldValid != m_hoverValid || oldRow != m_hoverRow || oldCol != m_hoverCol)
        {
            InvalidateRect(m_hwnd, nullptr, FALSE);
        }
        return 0;
    }

    case WM_MOUSELEAVE:
        m_trackingMouse = false;
        m_hoverValid = false;
        InvalidateRect(m_hwnd, nullptr, FALSE);
        return 0;

    case WM_LBUTTONDOWN:
    {
        int x = LOWORD(lParam);
        int y = HIWORD(lParam);

        if (x >= 668 && x <= 924 && y >= 616 && y <= 672)
        {
            m_game.Initialize();
            m_hoverValid = false;
            InvalidateRect(m_hwnd, nullptr, TRUE);
            return 0;
        }

        int row, col;
        if (TryGetCell(x, y, row, col))
        {
            if (!m_game.PlaceStone(row, col))
            {
                MessageBeep(MB_ICONASTERISK);
            }
            else
            {
                InvalidateRect(m_hwnd, nullptr, TRUE);
            }
        }
        return 0;
    }

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(m_hwnd, &ps);
        RECT client{};
        GetClientRect(m_hwnd, &client);
        HDC buffer = CreateCompatibleDC(hdc);
        HBITMAP bitmap = CreateCompatibleBitmap(hdc, client.right, client.bottom);
        HGDIOBJ oldBitmap = SelectObject(buffer, bitmap);

        Draw(buffer);
        BitBlt(hdc, 0, 0, client.right, client.bottom, buffer, 0, 0, SRCCOPY);

        SelectObject(buffer, oldBitmap);
        DeleteObject(bitmap);
        DeleteDC(buffer);
        EndPaint(m_hwnd, &ps);
        return 0;
    }

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcW(m_hwnd, message, wParam, lParam);
}

bool MainWindow::TryGetCell(int x, int y, int& row, int& col) const
{
    col = (x - BoardLeft + CellSize / 2) / CellSize;
    row = (y - BoardTop + CellSize / 2) / CellSize;
    if (!m_game.IsInside(row, col))
    {
        return false;
    }

    int centerX = BoardLeft + col * CellSize;
    int centerY = BoardTop + row * CellSize;
    return std::abs(x - centerX) <= CellSize / 2 && std::abs(y - centerY) <= CellSize / 2;
}

void MainWindow::Draw(HDC hdc)
{
    RECT client{};
    GetClientRect(m_hwnd, &client);

    for (int y = client.top; y < client.bottom; ++y)
    {
        int height = client.bottom - client.top;
        if (height < 1) height = 1;
        int p = (y - client.top) * 255 / height;
        HBRUSH brush = CreateSolidBrush(Blend(RGB(6, 10, 23), RGB(14, 24, 48), p));
        RECT line{ client.left, y, client.right, y + 1 };
        FillRect(hdc, &line, brush);
        DeleteObject(brush);
    }

    DrawTextLine(hdc, L"NEON GOMOKU", {30, 18, 270, 58}, DT_LEFT | DT_VCENTER, RGB(230, 246, 255), 28);
    DrawTextLine(hdc, L"// 15 x 15  TACTICAL GRID", {300, 28, 620, 52}, DT_LEFT | DT_VCENTER, RGB(92, 225, 255), 17);

    DrawPanel(hdc);
    DrawSidePanel(hdc);
}

void MainWindow::DrawPanel(HDC hdc)
{
    RECT panel{ 24, 72, 640, 708 };
    FillRoundRect(hdc, panel, RGB(12, 21, 42), 18);

    RECT board{ 38, 82, 638, 702 };
    FillRoundRect(hdc, board, RGB(24, 36, 66), 12);

    HPEN gridPen = CreatePen(PS_SOLID, 1, RGB(72, 112, 142));
    HGDIOBJ oldPen = SelectObject(hdc, gridPen);

    for (int i = 0; i < BoardSize; ++i)
    {
        int position = BoardLeft + i * CellSize;
        MoveToEx(hdc, BoardLeft, position, nullptr);
        LineTo(hdc, BoardLeft + BoardSizePixels, position);
        MoveToEx(hdc, position, BoardTop, nullptr);
        LineTo(hdc, position, BoardTop + BoardSizePixels);
    }
    SelectObject(hdc, oldPen);
    DeleteObject(gridPen);

    HPEN framePen = CreatePen(PS_SOLID, 2, RGB(118, 174, 204));
    HGDIOBJ oldFramePen = SelectObject(hdc, framePen);
    MoveToEx(hdc, BoardLeft, BoardTop, nullptr);
    LineTo(hdc, BoardLeft + BoardSizePixels, BoardTop);
    MoveToEx(hdc, BoardLeft + BoardSizePixels, BoardTop, nullptr);
    LineTo(hdc, BoardLeft + BoardSizePixels, BoardTop + BoardSizePixels);
    MoveToEx(hdc, BoardLeft + BoardSizePixels, BoardTop + BoardSizePixels, nullptr);
    LineTo(hdc, BoardLeft, BoardTop + BoardSizePixels);
    MoveToEx(hdc, BoardLeft, BoardTop + BoardSizePixels, nullptr);
    LineTo(hdc, BoardLeft, BoardTop);
    SelectObject(hdc, oldFramePen);
    DeleteObject(framePen);

    const int starPoints[3] = {3, 7, 11};
    for (int row : starPoints)
    {
        for (int col : starPoints)
        {
            int cx = BoardLeft + col * CellSize;
            int cy = BoardTop + row * CellSize;
            HBRUSH star = CreateSolidBrush(RGB(115, 190, 220));
            HGDIOBJ oldBrush = SelectObject(hdc, star);
            Ellipse(hdc, cx - 3, cy - 3, cx + 3, cy + 3);
            SelectObject(hdc, oldBrush);
            DeleteObject(star);
        }
    }

    for (int row = 0; row < BoardSize; ++row)
    {
        for (int col = 0; col < BoardSize; ++col)
        {
            Stone stone = m_game.GetBoard()[row][col];
            if (stone != Stone::Empty)
            {
                DrawStone(hdc, BoardLeft + col * CellSize, BoardTop + row * CellSize, stone, 18, 255);
            }
        }
    }

    if (m_hoverValid && m_game.GetBoard()[m_hoverRow][m_hoverCol] == Stone::Empty && !m_game.IsGameOver())
    {
        DrawStone(hdc, BoardLeft + m_hoverCol * CellSize, BoardTop + m_hoverRow * CellSize,
            m_game.CurrentPlayer(), 18, 90);
    }
}

void MainWindow::DrawStone(HDC hdc, int cx, int cy, Stone stone, int radius, int alpha)
{
    (void)alpha;
    if (stone == Stone::Black)
    {
        HPEN glow = CreatePen(PS_SOLID, 3, RGB(67, 207, 255));
        HBRUSH brush = CreateSolidBrush(RGB(8, 18, 33));
        HGDIOBJ oldPen = SelectObject(hdc, glow);
        HGDIOBJ oldBrush = SelectObject(hdc, brush);
        Ellipse(hdc, cx - radius, cy - radius, cx + radius, cy + radius);
        SelectObject(hdc, oldBrush);
        SelectObject(hdc, oldPen);
        DeleteObject(brush);
        DeleteObject(glow);
    }
    else
    {
        HPEN glow = CreatePen(PS_SOLID, 3, RGB(220, 246, 255));
        HBRUSH brush = CreateSolidBrush(RGB(190, 211, 225));
        HGDIOBJ oldPen = SelectObject(hdc, glow);
        HGDIOBJ oldBrush = SelectObject(hdc, brush);
        Ellipse(hdc, cx - radius, cy - radius, cx + radius, cy + radius);
        SelectObject(hdc, oldBrush);
        SelectObject(hdc, oldPen);
        DeleteObject(brush);
        DeleteObject(glow);
    }
}

void MainWindow::DrawSidePanel(HDC hdc)
{
    RECT panel{660, 72, 936, 708};
    FillRoundRect(hdc, panel, RGB(12, 21, 42), 18);

    DrawTextLine(hdc, L"战局状态", {688, 104, 908, 140}, DT_LEFT | DT_VCENTER, RGB(112, 222, 255), 20);

    const wchar_t* status;
    COLORREF statusColor;
    if (m_game.Winner() != Stone::Empty)
    {
        status = m_game.Winner() == Stone::Black ? L"黑方胜出" : L"白方胜出";
        statusColor = RGB(76, 255, 194);
    }
    else if (m_game.IsGameOver())
    {
        status = L"棋盘已满";
        statusColor = RGB(255, 202, 92);
    }
    else
    {
        status = m_game.CurrentPlayer() == Stone::Black ? L"黑方回合" : L"白方回合";
        statusColor = m_game.CurrentPlayer() == Stone::Black ? RGB(86, 212, 255) : RGB(224, 242, 255);
    }

    RECT statusBox{688, 160, 908, 224};
    FillRoundRect(hdc, statusBox, RGB(18, 32, 58), 10);
    DrawTextLine(hdc, status, statusBox, DT_CENTER | DT_VCENTER | DT_SINGLELINE, statusColor, 24);

    DrawTextLine(hdc, L"战术协议", {688, 264, 908, 292}, DT_LEFT | DT_VCENTER, RGB(112, 222, 255), 18);

    DrawTextLine(hdc, L"01  先手落子", {688, 310, 908, 338}, DT_LEFT | DT_VCENTER, RGB(202, 220, 236), 16);
    DrawTextLine(hdc, L"02  连成五子", {688, 350, 908, 378}, DT_LEFT | DT_VCENTER, RGB(202, 220, 236), 16);
    DrawTextLine(hdc, L"03  保持阵列", {688, 390, 908, 418}, DT_LEFT | DT_VCENTER, RGB(202, 220, 236), 16);

    DrawTextLine(hdc, L"SYSTEM  ONLINE", {688, 558, 908, 586}, DT_LEFT | DT_VCENTER, RGB(76, 255, 194), 14);

    RECT restart{668, 616, 924, 672};
    FillRoundRect(hdc, restart, RGB(31, 112, 151), 12);
    DrawTextLine(hdc, L"重新初始化  [ R ]", restart, DT_CENTER | DT_VCENTER | DT_SINGLELINE, RGB(235, 250, 255), 18);
}

void MainWindow::Run()
{
    ShowWindow(m_hwnd, SW_SHOW);
    UpdateWindow(m_hwnd);

    MSG message{};
    while (GetMessageW(&message, nullptr, 0, 0) > 0)
    {
        if (message.message == WM_KEYDOWN && message.wParam == 'R')
        {
            m_game.Initialize();
            InvalidateRect(m_hwnd, nullptr, TRUE);
        }
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }
}
}
