#pragma once

#include <array>

namespace Gomoku
{
constexpr int BoardSize = 15;

enum class Stone
{
    Empty,
    Black,
    White
};

using Board = std::array<std::array<Stone, BoardSize>, BoardSize>;

class Game
{
public:
    void Initialize();
    bool PlaceStone(int row, int col);
    bool IsInside(int row, int col) const;
    bool IsGameOver() const;
    bool IsBoardFull() const;
    Stone CurrentPlayer() const;
    Stone Winner() const;
    const Board& GetBoard() const;

private:
    bool HasWon(int row, int col, Stone player) const;
    int CountDirection(int row, int col, int deltaRow, int deltaCol, Stone player) const;

    Board m_board{};
    Stone m_currentPlayer = Stone::Black;
    Stone m_winner = Stone::Empty;
    bool m_gameOver = false;
};
}
