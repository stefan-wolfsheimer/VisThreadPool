#include "n_queens.h"
#include "catch.hpp"
#include <unordered_set>
#include <list>
#include <utility>

typedef std::pair<std::size_t, std::size_t> pair_type;


std::pair<std::size_t, std::size_t> getSolutions(const NQueensSolution & sol)
{
  return std::make_pair(sol.getFundamentalSolutions(), sol.getNumSolutions());
}

TEST_CASE("NQueens_1", "[NQueens]")
{
  ChessBoard board(1);
  CHECK(getSolutions(board.solveNQueens(1, 10000)) == pair_type(0u, 1u));
}

TEST_CASE("NQueens_2", "[NQueens]")
{
  ChessBoard board(2);
  CHECK(getSolutions(board.solveNQueens(2, 10000)) == pair_type(0u,0u));
}

TEST_CASE("NQueens_3", "[NQueens]")
{
  ChessBoard board(3);
  CHECK(getSolutions(board.solveNQueens(3, 10000)) == pair_type(0u,0u));
}

TEST_CASE("NQueens_4", "[NQueens]")
{
  ChessBoard board(4);
  CHECK(getSolutions(board.solveNQueens(4, 10000)) == pair_type(1u, 2u));
}

TEST_CASE("NQueens_5", "[NQueens]")
{
  ChessBoard board(5);
  CHECK(getSolutions(board.solveNQueens(5, 10000)) == pair_type(2u,10u));
}

TEST_CASE("NQueens_6", "[NQueens]")
{
  ChessBoard board(6);
  CHECK(getSolutions(board.solveNQueens(6, 10000)) == pair_type(1u,4u));
}

TEST_CASE("NQueens_7", "[NQueens]")
{
  ChessBoard board(7);
  CHECK(getSolutions(board.solveNQueens(7, 10000)) == pair_type(6u, 40u));
}

TEST_CASE("NQueens_8", "[NQueens]")
{
  ChessBoard board(8);
  CHECK(getSolutions(board.solveNQueens(8, 10000)) == pair_type(12u, 92u));
}
