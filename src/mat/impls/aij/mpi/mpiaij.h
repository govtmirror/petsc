/* $Id: mpiaij.h,v 1.13 1997/03/26 01:35:49 bsmith Exp balay $ */

#if !defined(__MPIAIJ_H)
#define __MPIAIJ_H

#include "src/mat/impls/aij/seq/aij.h"
#include "src/sys/ctable.h"

typedef struct {
  int           *rowners, *cowners;     /* ranges owned by each processor */
  int           m, n;                   /* local rows and columns */
  int           M, N;                   /* global rows and columns */
  int           rstart, rend;           /* starting and ending owned rows */
  int           cstart, cend;           /* starting and ending owned columns */
  Mat           A, B;                   /* local submatrices: A (diag part),
                                           B (off-diag part) */
  int           size;                   /* size of communicator */
  int           rank;                   /* rank of proc in communicator */ 

  /* The following variables are used for matrix assembly */

  Stash         stash;                  /* stash for non-local elements */
  int           donotstash;             /* 1 if off processor entries dropped */
  MPI_Request   *send_waits;            /* array of send requests */
  MPI_Request   *recv_waits;            /* array of receive requests */
  int           nsends, nrecvs;         /* numbers of sends and receives */
  Scalar        *svalues, *rvalues;     /* sending and receiving data */
  int           rmax;                   /* maximum message length */
#if defined (USE_CTABLE)
  Table         colmap
#else
  int           *colmap;                /* local col number of off-diag col */
#endif
  int           *garray;                /* work array */

  /* The following variables are used for matrix-vector products */

  Vec           lvec;              /* local vector */
  VecScatter    Mvctx;             /* scatter context for vector */
  int           roworiented;       /* if true, row-oriented input, default true */

  /* The following variables are for MatGetRow() */

  int           *rowindices;       /* column indices for row */
  Scalar        *rowvalues;        /* nonzero values in row */
  PetscTruth    getrowactive;      /* indicates MatGetRow(), not restored */
} Mat_MPIAIJ;

#endif
