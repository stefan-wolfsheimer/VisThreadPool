#include "n_queens.h"

NQueensSolution::NQueensSolution()
{
  for(std::size_t i = 0;i < 9; i++)
  {
    multipl[i] = 0;
  }
  nSolutions = 0;
}

std::size_t NQueensSolution::getFundamentalSolutions() const
{
  if(nSolutions == 1u)
  {
    return 1u;
  }
  std::size_t nf = 0;
  for(std::size_t i = 1; i < 9; i++) 
  {
    if(multipl[i]) 
    {
      nf+= multipl[i]/i;
    }
  }
  return nf;
}

ChessBoard::ChessBoard(std::size_t _L)
{
  L = _L;
  L2 = _L * _L;
  h_line.resize(L, 0);
  v_line.resize(L, 0);
  d_p_line.resize(2*L-1, 0);
  workspace_d_m_line.resize(2*L-1, 0);
  d_m_line = workspace_d_m_line.begin() + L-1;
  queens.resize(L2, 0);
}

void ChessBoard::solveNQueens(NQueensSolution & solution,
                              std::size_t i0,
                              std::size_t n,
                              std::size_t nQueens)
{
  std::size_t i;
  std::size_t x,y;
  if(n == nQueens) 
  {
    solution.nSolutions++;
    unsigned short m = checkSymmetry();
    solution.multipl[m]++;
    //if(board->verbose)
    //{
    //  printf("solution %lu\n", board->n_solutions);
    //  printf("multiplicity %u\n", m);
    //  print_board(fp, board);
    //}
  }
  else 
  {
    for(i = i0; i < L2; i++) 
    {
      x = i % L;
      y = i / L;
      if(checkPosition(x, y)) 
      {
        markPosition(x, y);
        solveNQueens(solution, i+1, n+1, nQueens);
        unmarkPosition(x, y);
      }
    }
  }
}

NQueensSolution ChessBoard::solveNQueens(std::size_t nQueens, std::size_t maxPrint)
{
  NQueensSolution ret;
  solveNQueens(ret, 0, 0, nQueens);
  return ret;
}
