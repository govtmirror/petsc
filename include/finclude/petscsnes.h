!
!  $Id: petscsnes.h,v 1.30 2000/09/25 18:02:37 balay Exp balay $;
!
!  Include file for Fortran use of the SNES package in PETSc
!
#if !defined (__PETSCSNES_H)
#define __PETSCSNES_H

#define SNES PetscFortranAddr
#define SNESProblemType integer
#define SNESType character*(80)
#define SNESConvergedReason integer
#define MatSNESMFCtx PetscFortranAddr
#define MatSNESMFType PetscFortranAddr
!
!  SNESType
!
#define SNESEQLS 'ls'
#define SNESEQTR 'tr'
#define SNESEQTEST 'test'
#define SNESUMLS 'umls'
#define SNESUMTR 'umtr'
!
! MatSNESMFCtx
! 
#define MATSNESMF_DEFAULT 'default'
#define MATSNESMF_WP 'wp'

#endif

#if !defined (PETSC_AVOID_DECLARATIONS)
!
!  Two classes of nonlinear solvers
!
      integer SNES_NONLINEAR_EQUATIONS
      integer SNES_UNCONSTRAINED_MINIMIZATION
      integer SNES_LEAST_SQUARES

      parameter (SNES_NONLINEAR_EQUATIONS = 0)
      parameter (SNES_UNCONSTRAINED_MINIMIZATION = 1)
      parameter (SNES_LEAST_SQUARES = 2)
!
!  Convergence flags
!
      integer SNES_CONVERGED_FNORM_ABS
      integer SNES_CONVERGED_FNORM_RELATIVE
      integer SNES_CONVERGED_PNORM_RELATIVE
      integer SNES_CONVERGED_GNORM_ABS
      integer SNES_CONVERGED_TR_REDUCTION
      integer SNES_CONVERGED_TR_DELTA

      integer SNES_DIVERGED_FUNCTION_COUNT
      integer SNES_DIVERGED_FNORM_NAN
      integer SNES_DIVERGED_MAX_IT
      integer SNES_DIVERGED_LS_FAILURE
      integer SNES_DIVERGED_TR_REDUCTION
      integer SNES_DIVERGED_LOCAL_MIN
      integer SNES_CONVERGED_ITERATING
   
      parameter (SNES_CONVERGED_FNORM_ABS         =  2)
      parameter (SNES_CONVERGED_FNORM_RELATIVE    =  3)
      parameter (SNES_CONVERGED_PNORM_RELATIVE    =  4)
      parameter (SNES_CONVERGED_GNORM_ABS         =  5)
      parameter (SNES_CONVERGED_TR_REDUCTION      =  6)
      parameter (SNES_CONVERGED_TR_DELTA          =  7)

      parameter (SNES_DIVERGED_FUNCTION_COUNT     = -2)  
      parameter (SNES_DIVERGED_FNORM_NAN          = -4) 
      parameter (SNES_DIVERGED_MAX_IT             = -5)
      parameter (SNES_DIVERGED_LS_FAILURE         = -6)
      parameter (SNES_DIVERGED_TR_REDUCTION       = -7)
      parameter (SNES_DIVERGED_LOCAL_MIN          = -8)
      parameter (SNES_CONVERGED_ITERATING         =  0)
     
!
!  Some PETSc fortran functions that the user might pass as arguments
!
      external SNESDEFAULTCOMPUTEJACOBIAN
      external SNESDEFAULTCOMPUTEJACOBIANCOLOR
      external SNESDEFAULTMONITOR
      external SNESLGMONITOR
      external SNESVECVIEWMONITOR
      external SNESVECVIEWUPDATEMONITOR

      external SNESCONVERGED_UM_LS
      external SNESCONVERGED_UM_TR
      external SNESCONVERGED_EQ_LS
      external SNESCONVERGED_EQ_TR

      external SNESCUBICLINESEARCH
      external SNESQUADRATICLINESEARCH
      external SNESNOLINESEARCH
      external SNESNOLINESEARCHNONORMS

!
!  End of Fortran include file for the SNES package in PETSc

#endif
