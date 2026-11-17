#include <windows.h>
#include <array>
#include <string>

const int BOARD_SIZE = 15;
const int CELL_SIZE = 42;
const int BOARD_LEFT = 42;
const int BOARD_TOP = 42;
const int BOARD_WIDTH = CELL_SIZE * (BOARD_SIZE - 1);
const int RESTART_BUTTON_ID = 1001;

enum class Stone
{
    Empty,
    Black,
    White
};

using Board = std::array<std::array<Stone, BOARD_SIZE>, BOARD_SIZE>;

Board g_board;
Stone g_currentPlayer = Stone::Black;
bool g_gameOver = false;
HWND g_mainWindow = nullptr;
HWND g_statusLabel = nullptr;

void InitializeGame()
{
    for (auto& row : g_board)
    {
        row.fill(Stone::Empty);
    }

    g_currentPlayer = Stone::Black;
    g_gameOver = false;
}

bool IsInsideBoard(int row, int col)
{
    return row >= 0 &&
           row < BOARD_SIZE &&
           col >= 0 &&
           col < BOARD_SIZE;
}

int CountDirection(
    int row,
    int col,
    int deltaRow,
    int deltaCol,
    Stone player)
{
    int count = 0;
    int currentRow = row + deltaRow;
    int currentCol = col + deltaCol;

    while (IsInsideBoard(currentRow, currentCol) &&
           g_board[currentRow][currentCol] == player)
    {
        ++count;
        currentRow += deltaRow;
        currentCol += deltaCol;
    }

    return count;
}

bool HasWon(int row, int col, Stone player)
{
    const int directions[4][2] =
    {
        { 0, 1 },
        { 1, 0 },
        { 1, 1 },
        { 1, -1 }
    };

    for (const auto& direction : directions)
    {
        int count = 1;

        count += CountDirection(
            row,
            col,
            direction[0],
            direction[1],
            player);

        count += CountDirection(
            row,
            col,
            -direction[0],
            -direction[1],
            player);

        if (count >= 5)
        {
            return true;
        }
    }

    return false;
}

bool IsBoardFull()
{
    for (const auto& row : g_board)
    {
        for (Stone stone : row)
        {
            if (stone == Stone::Empty)
            {
                return false;
            }
        }
    }

    return true;
}

std::wstring GetPlayerText(Stone player)
{
    return player == Stone::Black ? L"黑棋" : L"白棋";
}

void UpdateStatus()
{
    if (g_gameOver)
    {
        return;
    }

    std::wstring text = L"当前回合：" + GetPlayerText(g_currentPlayer);
    SetWindowTextW(g_statusLabel, text.c_str());
}

void DrawBoard(HDC hdc)
{
    RECT clientRect;
    GetClientRect(g_mainWindow, &clientRect);

    HBRUSH backgroundBrush = CreateSolidBrush(RGB(245, 220, 170));
    FillRect(hdc, &clientRect, backgroundBrush);
    DeleteObject(backgroundBrush);

    HPEN boardPen = CreatePen(PS_SOLID, 1, RGB(70, 50, 25));
    HPEN oldPen = static_cast<HPEN>(SelectObject(hdc, boardPen));

    for (int i = 0; i < BOARD_SIZE; ++i)
    {
        int position = BOARD_LEFT + i * CELL_SIZE;

        MoveToEx(
            hdc,
            BOARD_LEFT,
            position,
            nullptr);

        LineTo(
            hdc,
            BOARD_LEFT + BOARD_WIDTH,
            position);

        MoveToEx(
            hdc,
            position,
            BOARD_TOP,
            nullptr);

        LineTo(
            hdc,
            position,
            BOARD_TOP + BOARD_WIDTH);
    }

    SelectObject(hdc, oldPen);
    DeleteObject(boardPen);

    const int starPoints[3] = { 3, 7, 11 };

    HBRUSH starBrush = CreateSolidBrush(RGB(50, 35, 20));

    for (int row : starPoints)
    {
        for (int col : starPoints)
        {
            int centerX = BOARD_LEFT + col * CELL_SIZE;
            int centerY = BOARD_TOP + row * CELL_SIZE;

            Ellipse(
                hdc,
                centerX - 4,
                centerY - 4,
                centerX + 4,
                centerY + 4);
        }
    }

    DeleteObject(starBrush);

    for (int row = 0; row < BOARD_SIZE; ++row)
    {
        for (int col = 0; col < BOARD_SIZE; ++col)
        {
            Stone stone = g_board[row][col];

            if (stone == Stone::Empty)
            {
                continue;
            }

            int centerX = BOARD_LEFT + col * CELL_SIZE;
            int centerY = BOARD_TOP + row * CELL_SIZE;
            int radius = CELL_SIZE / 2 - 3;

            COLORREF stoneColor =
                stone == Stone::Black
                ? RGB(25, 25, 25)
                : RGB(245, 245, 245);

            HBRUSH stoneBrush = CreateSolidBrush(stoneColor);
            HPEN stonePen = CreatePen(PS_SOLID, 1, RGB(60, 60, 60));

            HBRUSH oldBrush =
                static_cast<HBRUSH>(SelectObject(hdc, stoneBrush));

            HPEN oldStonePen =
                static_cast<HPEN>(SelectObject(hdc, stonePen));

            Ellipse(
                hdc,
                centerX - radius,
                centerY - radius,
                centerX + radius,
                centerY + radius);

            SelectObject(hdc, oldBrush);
            SelectObject(hdc, oldStonePen);

            DeleteObject(stoneBrush);
            DeleteObject(stonePen);
        }
    }
}

void HandleBoardClick(HWND hwnd, int mouseX, int mouseY)
{
    if (g_gameOver)
    {
        return;
    }

    int col = (mouseX - BOARD_LEFT + CELL_SIZE / 2) / CELL_SIZE;
    int row = (mouseY - BOARD_TOP + CELL_SIZE / 2) / CELL_SIZE;

    if (!IsInsideBoard(row, col))
    {
        return;
    }

    int centerX = BOARD_LEFT + col * CELL_SIZE;
    int centerY = BOARD_TOP + row * CELL_SIZE;

    if (abs(mouseX - centerX) > CELL_SIZE / 2 ||
        abs(mouseY - centerY) > CELL_SIZE / 2)
    {
        return;
    }

    if (g_board[row][col] != Stone::Empty)
    {
        MessageBoxW(
            hwnd,
            L"该位置已经有棋子，请选择其他位置。",
            L"提示",
            MB_OK | MB_ICONINFORMATION);

        return;
    }

    g_board[row][col] = g_currentPlayer;

    if (HasWon(row, col, g_currentPlayer))
    {
        g_gameOver = true;

        std::wstring message =
            GetPlayerText(g_currentPlayer) + L"获胜！";

        SetWindowTextW(g_statusLabel, message.c_str());

        MessageBoxW(
            hwnd,
            message.c_str(),
            L"游戏结束",
            MB_OK | MB_ICONINFORMATION);

        InvalidateRect(hwnd, nullptr, TRUE);
        return;
    }

    if (IsBoardFull())
    {
        g_gameOver = true;
        SetWindowTextW(g_statusLabel, L"棋盘已满，双方和棋。");

        MessageBoxW(
            hwnd,
            L"棋盘已满，双方和棋。",
            L"游戏结束",
            MB_OK | MB_ICONINFORMATION);

        InvalidateRect(hwnd, nullptr, TRUE);
        return;
    }

    g_currentPlayer =
        g_currentPlayer == Stone::Black
        ? Stone::White
        : Stone::Black;

    UpdateStatus();
    InvalidateRect(hwnd, nullptr, TRUE);
}

LRESULT CALLBACK WindowProcedure(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    switch (message)
    {
    case WM_CREATE:
    {
        g_mainWindow = hwnd;

        g_statusLabel = CreateWindowW(
            L"STATIC",
            L"",
            WS_CHILD | WS_VISIBLE | SS_CENTER,
            250,
            650,
            200,
            30,
            hwnd,
            nullptr,
            GetModuleHandleW(nullptr),
            nullptr);

        CreateWindowW(
            L"BUTTON",
            L"重新开始",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            510,
            645,
            110,
            36,
            hwnd,
            reinterpret_cast<HMENU>(RESTART_BUTTON_ID),
            GetModuleHandleW(nullptr),
            nullptr);

        InitializeGame();
        UpdateStatus();
        return 0;
    }

    case WM_LBUTTONDOWN:
    {
        int mouseX = LOWORD(lParam);
        int mouseY = HIWORD(lParam);

        HandleBoardClick(hwnd, mouseX, mouseY);
        return 0;
    }

    case WM_COMMAND:
    {
        if (LOWORD(wParam) == RESTART_BUTTON_ID)
        {
            InitializeGame();
            UpdateStatus();
            InvalidateRect(hwnd, nullptr, TRUE);
        }

        return 0;
    }

    case WM_PAINT:
    {
        PAINTSTRUCT paintStruct;
        HDC hdc = BeginPaint(hwnd, &paintStruct);

        DrawBoard(hdc);

        EndPaint(hwnd, &paintStruct);
        return 0;
    }

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    default:
        return DefWindowProcW(hwnd, message, wParam, lParam);
    }
}

int WINAPI wWinMain(
    HINSTANCE instance,
    HINSTANCE,
    PWSTR,
    int showCommand)
{
    const wchar_t windowClassName[] = L"GomokuWindowClass";

    WNDCLASSW windowClass = {};
    windowClass.hInstance = instance;
    windowClass.lpfnWndProc = WindowProcedure;
    windowClass.lpszClassName = windowClassName;
    windowClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    windowClass.hbrBackground =
        reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);

    RegisterClassW(&windowClass);

    HWND hwnd = CreateWindowW(
        windowClassName,
        L"五子棋 - 双人对战",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        700,
        740,
        nullptr,
        nullptr,
        instance,
        nullptr);

    if (hwnd == nullptr)
    {
        return 0;
    }

    ShowWindow(hwnd, showCommand);
    UpdateWindow(hwnd);

    MSG message = {};

    while (GetMessageW(&message, nullptr, 0, 0) > 0)
    {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }

    return static_cast<int>(message.wParam);
}