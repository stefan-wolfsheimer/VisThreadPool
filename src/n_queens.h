#pragma once
#include <utility>
#include <vector>
  
class NQueensSolution
{
public:
  friend class ChessBoard;
  NQueensSolution();
  inline std::size_t getNumSolutions() const;
  std::size_t getFundamentalSolutions() const;

private:
  // statistics of multiplicities of solutions 
  std::size_t multipl[9];
  std::size_t nSolutions;
};

class ChessBoard
{
public:
  ChessBoard(std::size_t _L);
  NQueensSolution solveNQueens(std::size_t n_queens, std::size_t maxPrint);
private:
  typedef short int indicator_type;
  std::size_t L; // side length of the board
  std::size_t L2; // Number of fields

  // indicator: horizontal line reachable
  // h_line[0] ... h_line[L-1]
  std::vector<indicator_type> h_line;

  // indicator: vertical line reachable 
  // v_line[0] ... h_line[L-1]
  std::vector<indicator_type> v_line;

  // indicator: diagonal line reachable 
  // d_p_line[0] ... h_line[2*L-2]
  std::vector<indicator_type> d_p_line;

  // indicator: diagonal line rachable 
  // d_m_line[-L+1] ... h_line[L-1]
  std::vector<indicator_type> workspace_d_m_line;
  std::vector<indicator_type>::iterator d_m_line;
  
  //indicator: field is occupied queens[0] ... queens[L2]
  std::vector<indicator_type> queens;

  inline bool checkPosition(std::size_t x, std::size_t y) const;
  inline unsigned short checkSymmetry() const;
  inline void markPosition(std::size_t x, std::size_t y);
  inline void unmarkPosition(std::size_t x, std::size_t y);
  void solveNQueens(NQueensSolution & solution, std::size_t i0, std::size_t n, std::size_t nQueens);
};

/** helpers */
inline std::size_t NQueensSolution::getNumSolutions() const
{
  return nSolutions;
}

inline bool ChessBoard::checkPosition(std::size_t x, std::size_t y) const
{
  return ! (
            h_line[y] || 
            v_line[x] || 
            d_p_line[x+y] ||
            d_m_line[x-y]);
}

inline unsigned short ChessBoard::checkSymmetry() const
{
  std::size_t x,y;
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
      if(queens[x + y * L] != queens[ L-y-1 + x*L]) 
      {
        /* 90 degree */
        sym_90 = 0;
      }
      if(queens[x + y * L] != queens[ L-x-1 + (L-y-1)*L]) 
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

inline void ChessBoard::markPosition(std::size_t x, std::size_t y)
{
  h_line[y] = 1;
  v_line[x] = 1;
  /*    i            j          i-j           j+i
     0 1 2 3      0 0 0 0     0  1  2  3   0 1 2 3 
     0 1 2 3      1 1 1 1    -1  0  1  2   1 2 3 4
     0 1 2 3      2 2 2 2    -2 -1  0  1   2 3 4 5
     0 1 2 3      3 3 3 3    -3 -2 -1  0   3 4 5 6
  */
  d_p_line[x+y] = 1;
  d_m_line[x-y] = 1;
  queens[x + y * L] = 1;
}

inline void ChessBoard::unmarkPosition(std::size_t x, std::size_t y)
{
  h_line[y] = 0;
  v_line[x] = 0;
  d_p_line[x+y] = 0;
  d_m_line[x-y] = 0;
  queens[x + y * L] = 0;
}



#if 0
#include <stdlib.h>
#include <stdio.h>
#ifndef __N_QUEENS_H__
#define __N_QUEENS_H__

/** 
 * Chess board for solving the n-queens problem
 */
typedef struct chess_board_t 
{
  size_t      L;           /* side length of the board */
  size_t      L2;          /* number of fields */
  short int * h_line;      /* indicator: horizontal line reachable 
                              h_line[0] ... h_line[L-1]
                            */
  short int * v_line;      /* indicator: vertical line reachable 
                              v_line[0] ... h_line[L-1]
                            */
  short int * d_p_line;    /* indicator: diagonal line reachable 
                              d_p_line[0] ... h_line[2*L-2]
                            */ 
  short int * d_m_line;    /* indicator: diagonal line rachable 
                              d_m_line[-L+1] ... h_line[L-1]
                            */
  short int * queens;      /* indicator: field is occupied 
                              queens[0] ... queens[L2]
                            */
  size_t      n_solutions; /* solution counter */

  size_t      multipl[9];  /* statistics of multiplicities of solutions */

  int         verbose;
} chess_board_t;

/***********************************************************
 * Initialize workspace for n-queens computation 
 * @param board to be initialized 
 * @param L side length of the board
 ***********************************************************/
void init_board(chess_board_t * board, size_t L);

/***********************************************************
 * Free allocated memory for chess board 
 * @param board initialized with init_board
 ***********************************************************/
void free_board(chess_board_t * board);

/***********************************************************
 * Print board to file 
 * @param fp file descriptor 
 * @param board chess board to be written 
 ***********************************************************/
void print_board(FILE * fp, chess_board_t * board);

/***********************************************************
 * Print summary of n-queens problem:
 * number of solutions
 * number of fundamental solutions
 * multiplicities of solutions 
 ***********************************************************/
void print_n_queens_summary(FILE * fp, chess_board_t * board);

/***********************************************************
 * Solve the n queens problem and write the number of 
 * solutions to file.
 * If board->verbose is not 0, also write out all 
 * solutions.
 * @param fp file pointer 
 * @param board board initialized with init_board
 * @param n_queens number of queens to be placed on 
 *        the board
 ***********************************************************/
void solve_n_queens_problem(FILE          * fp,
                            chess_board_t * board,
                            size_t          n_queens);

#endif
#endif
