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

#if 0
#include <stdlib.h>
#include <stdio.h>
#include "n_queens.h"

/** helpers */
static int _check_position(chess_board_t * board, 
                           int x, 
                           int y)
{
  return ! (
            board->h_line[y] || 
            board->v_line[x] || 
            board->d_p_line[x+y] ||
            board->d_m_line[x-y]);
}

static void _mark_position(chess_board_t * board, 
                           int x, 
                           int y)
{
  board->h_line[y] = 1;
  board->v_line[x] = 1;
  /*    i            j          i-j           j+i
     0 1 2 3      0 0 0 0     0  1  2  3   0 1 2 3 
     0 1 2 3      1 1 1 1    -1  0  1  2   1 2 3 4
     0 1 2 3      2 2 2 2    -2 -1  0  1   2 3 4 5
     0 1 2 3      3 3 3 3    -3 -2 -1  0   3 4 5 6
  */
  board->d_p_line[x+y] = 1;
  board->d_m_line[x-y] = 1;

  board->queens[x + y * board->L] = 1;
}

static void _unmark_position(chess_board_t * board, 
                             int x, 
                             int y)
{
  board->h_line[y] = 0;
  board->v_line[x] = 0;
  board->d_p_line[x+y] = 0;
  board->d_m_line[x-y] = 0;
  board->queens[x + y * board->L] = 0;
}

static unsigned short _check_symmetry(chess_board_t * board)
{
  size_t L = board->L;
  size_t x,y;
  unsigned short sym_90 = 1;
  unsigned short sym_180 = 1;
  for(y = 0; y < L; y++) 
  {
    for(x = 0; x < L; x++) 
    {
      /*                  
       [ x' ]   [ d  d ] [ x - L/2 ]    [ L/2 ]
       [ y' ] = [ d  d ] [ y - L/2 ] +  [ L/2 ]

       90:                180:
       [ 0 -1 ]           [ -1  0 ]
       [ 1  0 ]           [  0 -1 ]
       
       x* = - y           x* = -x
       y* =   x           y* = -y

       x' = -y + L        x' = -x + L
       y' =  x            y' = -y + L
      */
      if(board->queens[x + y * L] != board->queens[ L-y-1 + x*L]) 
      {
        /* 90 degree */
        sym_90 = 0;
      }
      if(board->queens[x + y * L] != board->queens[ L-x-1 + (L-y-1)*L]) 
      {
        sym_180 = 0;
      }
    }
  }
  if(sym_90) 
  {
    /* two variants: itself and it's reflection */
    return 2;
  }
  if(sym_180) 
  {
    /* four variants: itself, 180 degree rotated times reflections */
    return 4;
  }
  else 
  {
    /* eight variants: 4 rotations and their reflections */
    return 8;
  }
}

static void _solve_n_queens_problem(FILE          * fp,
                                    chess_board_t * board,
                                    size_t          i0,
                                    size_t          n,
                                    size_t          n_queens)
{
  size_t i;
  size_t L = board->L;
  size_t L2 = board->L2;
  int x,y;
  if(n == n_queens) 
  {
    board->n_solutions++;
    unsigned short m = _check_symmetry(board);
    board->multipl[m]++;
    if(board->verbose)
    {
      printf("solution %lu\n", board->n_solutions);
      printf("multiplicity %u\n", m);
      print_board(fp, board);
    }
  }
  else 
  {
    for(i = i0; i < L2; i++) 
    {
      x = i % L;
      y = i / L;
      if(_check_position(board, x, y)) 
      {
        _mark_position(board, x, y);
        _solve_n_queens_problem(fp, 
                                board, 
                                i+1,
                                n+1, n_queens);
        _unmark_position(board, x, y);
      }
    }
  }
}

/** declared functions */
void init_board(chess_board_t * board, size_t L)
{
  board->L           = L;
  board->L2          = L*L;
  board->h_line      =  calloc(L, sizeof(short int));
  board->v_line      =  calloc(L, sizeof(short ));
  board->d_p_line    = calloc(2*L-1, sizeof(short int));
  board->d_m_line    = calloc(2*L-1, sizeof(short int));
  board->queens      = calloc(L*L, sizeof(short int));
  board->d_m_line   += L-1;
  board->n_solutions = 0;
  board->verbose     = 0;
  size_t i;
  for(i = 0; i < 9; i++) 
  {
    board->multipl[i] = 0;
  }
}

void free_board(chess_board_t * board)
{
  free(board->h_line);
  free(board->v_line);
  free(board->d_p_line);
  free(board->d_m_line - board->L+1);
  free(board->queens);
}

void print_n_queens_summary(FILE * fp, chess_board_t * board)
{
  size_t i;
  size_t nf = 0;
  fprintf(fp, "solutions: %lu\n", board->n_solutions);
  for(i = 1; i < 9; i++) 
  {
    if(board->multipl[i]) 
    {
      nf+= board->multipl[i]/i;
    }
  }
  fprintf(fp, "fundamental solutions: %lu\n", nf);
  fprintf(fp, "decomposition: %lu = ",board->n_solutions);
  int first = 1;
  for(i = 1; i < 9; i++) 
  {
    if(board->multipl[i]) 
    {
      if(first) first = 0;
      else fprintf(fp, " + ");
      fprintf(fp, "%lu * %lu", board->multipl[i]/i, i);
    }
  }
  fprintf(fp, "\n");
}

void print_board(FILE * fp, chess_board_t * board)
{
  int x,y;
  int L = board->L;
  fputc('+', fp);
  for(x = 0; x < L; x++) 
  {
    fputc('-', fp);
  }
  fprintf(fp, "+\n");
  for(x = 0; x < L; x++) 
  {
    fputc('|', fp);
    for(y = 0; y < L; y++) 
    {
      if(board->queens[x + y * board->L]) 
      {
        fputc('Q', fp);
      }
      else if(!_check_position(board, x,y))
      {
        fputc('*', fp);
      }
      else 
      {
        fputc(' ', fp);
      }
    }
    fprintf(fp, "|\n");
  }
  fputc('+', fp);
  for(x = 0; x < board->L; x++) 
  {
    fputc('-', fp);
  }
  fprintf(fp, "+\n");
}


void solve_n_queens_problem(FILE          * fp,
                            chess_board_t * board,
                            size_t          n_queens)
{
  _solve_n_queens_problem(fp, board, 
                          0, 
                          0, n_queens);
}

#endif
