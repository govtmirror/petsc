!
!   This program demonstrates use of MatGetSubMatrices() from Fortran
!
      program main
#include <petsc/finclude/petscmat.h>
      use petscmat
      implicit none

      Mat      A,B
      PetscErrorCode  ierr
      PetscInt one,zero(1)
      PetscViewer   v
      IS       isrow

      call PetscInitialize(PETSC_NULL_CHARACTER,ierr)
      if (ierr .ne. 0) then
        print*,'Unable to initialize PETSc'
        stop
      endif
#if defined(PETSC_USE_64BIT_INDICES)
      call PetscViewerBinaryOpen(PETSC_COMM_WORLD,                          &
     & '../../../../share/petsc/datafiles/matrices/' //                       &
     & 'ns-real-int64-float64',                                               &
     &                          FILE_MODE_READ,v,ierr)
#else
      call PetscViewerBinaryOpen(PETSC_COMM_WORLD,                          &
     & '../../../../share/petsc/datafiles/matrices/' //                       &
     & 'ns-real-int32-float64',                                               &
     &                          FILE_MODE_READ,v,ierr)
#endif

      call MatCreate(PETSC_COMM_WORLD,A,ierr)
      call MatSetType(A, MATSEQAIJ,ierr)
      call MatLoad(A,v,ierr)

      one  = 1
      zero(1) = 0
      call ISCreateGeneral(PETSC_COMM_SELF,one,zero,PETSC_COPY_VALUES,    &
     &                     isrow,ierr)

      call MatGetSubmatrices(A,one,isrow,isrow,                           &
     &        MAT_INITIAL_MATRIX,B,ierr)

      call MatView(B,PETSC_VIEWER_STDOUT_SELF,ierr)

      call MatGetSubmatrices(A,one,isrow,isrow,                           &
     &        MAT_REUSE_MATRIX,B,ierr)

      call MatView(B,PETSC_VIEWER_STDOUT_SELF,ierr)

      call ISDestroy(isrow,ierr)
      call MatDestroy(A,ierr)
      call MatDestroy(B,ierr)
      call PetscViewerDestroy(v,ierr)

      call PetscFinalize(ierr)
      end




