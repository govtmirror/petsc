/*$Id: sprcm.c,v 1.31 1999/10/24 14:02:23 bsmith Exp bsmith $*/

#include "mat.h"
#include "src/mat/order/order.h"

EXTERN_C_BEGIN
/*
    MatOrdering_RCM - Find the Reverse Cuthill-McKee ordering of a given matrix.
*/    
#undef __FUNC__  
#define __FUNC__ "MatOrdering_RCM"
int MatOrdering_RCM( Mat mat, MatOrderingType type, IS *row, IS *col)
{
  int        ierr,i,   *mask, *xls, nrow,*ia,*ja,*perm;
  PetscTruth done;

  PetscFunctionBegin;
  ierr = MatGetRowIJ(mat,1,PETSC_TRUE,&nrow,&ia,&ja,&done);CHKERRQ(ierr);
  if (!done) SETERRQ(PETSC_ERR_SUP,0,"Cannot get rows for matrix");

  mask = (int *)PetscMalloc( 4*nrow * sizeof(int) );CHKPTRQ(mask);
  perm = mask + nrow;
  xls  = perm + nrow;

  SPARSEPACKgenrcm( &nrow, ia, ja, perm, mask, xls );
  ierr = MatRestoreRowIJ(mat,1,PETSC_TRUE,&nrow,&ia,&ja,&done);CHKERRQ(ierr);

  /* shift because Sparsepack indices start at one */
  for (i=0; i<nrow; i++) perm[i]--;

  ierr = ISCreateGeneral(PETSC_COMM_SELF,nrow,perm,row);CHKERRQ(ierr);
  ierr = ISCreateGeneral(PETSC_COMM_SELF,nrow,perm,col);CHKERRQ(ierr);
  ierr = PetscFree(mask);CHKERRQ(ierr);

  PetscFunctionReturn(0);
}
EXTERN_C_END
