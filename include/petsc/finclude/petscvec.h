!
!
!  Include file for Fortran use of the Vec package in PETSc
!
#if !defined (__PETSCVECDEF_H)
#define __PETSCVECDEF_H

#include "petsc/finclude/petscao.h"

#define Vec type(tVec)
#define VecScatter type(tVecScatter)

#define NormType PetscEnum
#define InsertMode PetscEnum
#define ScatterMode PetscEnum
#define VecOption PetscEnum
#define VecType character*(80)
#define VecOperation PetscEnum

#define VECSEQ 'seq'
#define VECMPI 'mpi'
#define VECSTANDARD 'standard'
#define VECSHARED 'shared'
#define VECSEQCUSP 'seqcusp'
#define VECMPICUSP 'mpicusp'
#define VECCUSP 'cusp'
#define VECSEQVIENNACL 'seqviennacl'
#define VECMPIVIENNACL 'mpiviennacl'
#define VECVIENNACL    'viennacl'
#define VECNEST 'nest'

#endif
