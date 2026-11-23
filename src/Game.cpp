#include "Game.h"

namespace Gomoku
{
void Game::Initialize()
{
    for (auto& row : m_board)
    {
        row.fill(Stone::Empty);
    }

    m_currentPlayer = Stone::Black;
    m_winner = Stone::Empty;
    m_gameOver = false;
}

bool Game::IsInside(int row, int col) const
{
    return row >= 0 && row < BoardSize && col >= 0 && col < BoardSize;
}

bool Game::PlaceStone(int row, int col)
{
    if (m_gameOver || !IsInside(row, col) || m_board[row][col] != Stone::Empty)
    {
        return false;
    }

    m_board[row][col] = m_currentPlayer;

    if (HasWon(row, col, m_currentPlayer))
    {
        m_winner = m_currentPlayer;
        m_gameOver = true;
    }
    else if (IsBoardFull())
    {
        m_gameOver = true;
    }
    else
    {
        m_currentPlayer = m_currentPlayer == Stone::Black ? Stone::White : Stone::Black;
    }

    return true;
}

bool Game::IsGameOver() const
{
    return m_gameOver;
}

bool Game::IsBoardFull() const
{
    for (const auto& row : m_board)
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

Stone Game::CurrentPlayer() const
{
    return m_currentPlayer;
}

Stone Game::Winner() const
{
    return m_winner;
}

const Board& Game::GetBoard() const
{
    return m_board;
}

int Game::CountDirection(int row, int col, int deltaRow, int deltaCol, Stone player) const
{
    int count = 0;
    int currentRow = row + deltaRow;
    int currentCol = col + deltaCol;

    while (IsInside(currentRow, currentCol) && m_board[currentRow][currentCol] == player)
    {
        ++count;
        currentRow += deltaRow;
        currentCol += deltaCol;
    }

    return count;
}

bool Game::HasWon(int row, int col, Stone player) const
{
    constexpr int Directions[4][2] = {
        {0, 1}, {1, 0}, {1, 1}, {1, -1}
    };

    for (const auto& direction : Directions)
    {
        int count = 1;
        count += CountDirection(row, col, direction[0], direction[1], player);
        count += CountDirection(row, col, -direction[0], -direction[1], player);
        if (count >= 5)
        {
            return true;
        }
    }
    return false;
}
}
