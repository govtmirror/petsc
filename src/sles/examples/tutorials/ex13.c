/*$Id: ex13.c,v 1.17 1999/10/24 14:03:24 bsmith Exp bsmith $*/

static char help[] = "Solves a variable Poisson problem with SLES.\n\n";

/*T
   Concepts: SLES^Solving a system of linear equations (basic sequential example)
   Concepts: SLES^Laplacian, 2d
   Concepts: Laplacian, 2d
   Routines: SLESCreate(); SLESSetOperators(); SLESSetFromOptions();
   Routines: SLESSolve(); SLESGetKSP(); SLESGetPC(); MatCreateSeqAIJ();
   Routines: KSPSetTolerances(); PCSetType();
   Routines: VecCreateSeqWithArray(); VecPlaceArray();
   Processors: 1
T*/

/* 
  Include "sles.h" so that we can use SLES solvers.  Note that this file
  automatically includes:
     petsc.h  - base PETSc routines   vec.h - vectors
     sys.h    - system routines       mat.h - matrices
     is.h     - index sets            ksp.h - Krylov subspace methods
     viewer.h - viewers               pc.h  - preconditioners
*/
#include "sles.h"

/*
    User-defined context that contains all the data structures used
    in the linear solution process.
*/
typedef struct {
   Vec    x, b;      /* solution vector, right-hand-side vector */
   Mat    A;         /* sparse matrix */
   SLES   sles;      /* linear solver context */
   int    m, n;      /* grid dimensions */
   Scalar hx2, hy2;  /* 1/(m+1)*(m+1) and 1/(n+1)*(n+1) */
} UserCtx;

extern int UserInitializeLinearSolver(int,int,UserCtx *);
extern int UserFinalizeLinearSolver(UserCtx *);
extern int UserDoLinearSolver(Scalar *,UserCtx *userctx,Scalar *b,Scalar *x);

#undef __FUNC__
#define __FUNC__ "main"
int main(int argc,char **args)
{
  UserCtx userctx;
  int     ierr, m = 6, n = 7, t, tmax = 2,i,I,j,N;
  Scalar  *userx,*rho, *solution, *userb,hx,hy,x,y;
  double  enorm;

  /*
     Initialize the PETSc libraries
  */
  PetscInitialize(&argc,&args,(char *)0,help);

  /*
     The next two lines are for testing only; these allow the user to
     decide the grid size at runtime.
  */
  ierr = OptionsGetInt(PETSC_NULL,"-m",&m,PETSC_NULL);CHKERRA(ierr);
  ierr = OptionsGetInt(PETSC_NULL,"-n",&n,PETSC_NULL);CHKERRA(ierr);

  /*
     Create the empty sparse matrix and linear solver data structures
  */
  ierr = UserInitializeLinearSolver(m,n,&userctx);CHKERRA(ierr);
  N    = m*n;

  /*
     Allocate arrays to hold the solution to the linear system.
     This is not normally done in PETSc programs, but in this case, 
     since we are calling these routines from a non-PETSc program, we 
     would like to reuse the data structures from another code. So in 
     the context of a larger application these would be provided by
     other (non-PETSc) parts of the application code.
  */
  userx    = (Scalar *) PetscMalloc(N*sizeof(Scalar));CHKPTRA(userx);
  userb    = (Scalar *) PetscMalloc(N*sizeof(Scalar));CHKPTRA(userb);
  solution = (Scalar *) PetscMalloc(N*sizeof(Scalar));CHKPTRA(solution);

  /* 
      Allocate an array to hold the coefficients in the elliptic operator
  */
  rho = (Scalar *) PetscMalloc(N*sizeof(Scalar));CHKERRA(ierr);

  /*
     Fill up the array rho[] with the function rho(x,y) = x; fill the
     right-hand-side b[] and the solution with a known problem for testing.
  */
  hx = 1.0/(m+1); 
  hy = 1.0/(n+1);
  y  = hy;
  I  = 0;
  for ( j=0; j<n; j++ ) {
    x = hx;
    for ( i=0; i<m; i++ ) {
      rho[I]      = x;
      solution[I] = PetscSinScalar(2.*PETSC_PI*x)*PetscSinScalar(2.*PETSC_PI*y);
      userb[I]    = -2*PETSC_PI*PetscCosScalar(2*PETSC_PI*x)*PetscSinScalar(2*PETSC_PI*y) +
                    8*PETSC_PI*PETSC_PI*x*PetscSinScalar(2*PETSC_PI*x)*PetscSinScalar(2*PETSC_PI*y);
      x += hx;
      I++;
    }
    y += hy;
  }

  /*
     Loop over a bunch of timesteps, setting up and solver the linear
     system for each time-step.

     Note this is somewhat artificial. It is intended to demonstrate how
     one may reuse the linear solver stuff in each time-step.
  */
  for ( t=0; t<tmax; t++ ) {
    ierr =  UserDoLinearSolver(rho,&userctx,userb,userx);CHKERRA(ierr);

    /*
        Compute error: Note that this could (and usually should) all be done
        using the PETSc vector operations. Here we demonstrate using more 
        standard programming practices to show how they may be mixed with 
        PETSc.
    */
    enorm = 0.0;
    for ( i=0; i<N; i++ ) {
      enorm += PetscReal(PetscConj(solution[i]-userx[i])*(solution[i]-userx[i]));
    }
    enorm *= PetscReal(hx*hy);
    printf("m %d n %d error norm %g\n",m,n,enorm);
  }

  /*
     We are all finished solving linear systems, so we clean up the
     data structures.
  */
  ierr = PetscFree(rho);CHKERRA(ierr);
  ierr = PetscFree(solution);CHKERRA(ierr);
  ierr = PetscFree(userx);CHKERRA(ierr);
  ierr = PetscFree(userb);CHKERRA(ierr);
  ierr = UserFinalizeLinearSolver(&userctx);CHKERRA(ierr);
  PetscFinalize();

  return 0;
}

/* ------------------------------------------------------------------------*/
#undef __FUNC__
#define __FUNC__ "UserInitializedLinearSolve"
int UserInitializeLinearSolver(int m, int n,UserCtx *userctx)
{
  int N,ierr;

  /*
     Here we assume use of a grid of size m x n, with all points on the
     interior of the domain, i.e., we do not include the points corresponding 
     to homogeneous Dirichlet boundary conditions.  We assume that the domain
     is [0,1]x[0,1].
  */
  userctx->m   = m;
  userctx->n   = n;
  userctx->hx2 = (m+1)*(m+1);
  userctx->hy2 = (n+1)*(n+1); 
  N            = m*n;

  /* 
     Create the sparse matrix. Preallocate 5 nonzeros per row.
  */
  ierr = MatCreateSeqAIJ(PETSC_COMM_SELF,N,N,5,0,&userctx->A);CHKERRQ(ierr);

  /* 
     Create vectors. Here we create vectors with no memory allocated.
     This way, we can use the data structures already in the program
     by using VecPlaceArray() subroutine at a later stage.
  */
  ierr = VecCreateSeqWithArray(PETSC_COMM_SELF,N,PETSC_NULL,&userctx->b);CHKERRQ(ierr);
  ierr = VecDuplicate(userctx->b,&userctx->x);CHKERRQ(ierr);

  /* 
     Create linear solver context. This will be used repeatedly for all 
     the linear solves needed.
  */
  ierr = SLESCreate(PETSC_COMM_SELF,&userctx->sles);CHKERRQ(ierr);

  return 0;
}

#undef __FUNC__
#define __FUNC__ "UserDoLinearSolve"
/*
   Solves -div ( rho grad psi) = F using finite differences.
   rho is a 2-dimensional array of size m by n, stored in Fortran
   style by columns. userb is a standard one-dimensional array.
*/ 
/* ------------------------------------------------------------------------*/
int UserDoLinearSolver(Scalar *rho,UserCtx *userctx,Scalar *userb,Scalar *userx)
{
  int    ierr,i,j,I,J, m = userctx->m, n = userctx->n,its;
  Mat    A = userctx->A;
  PC     pc;
  Scalar v, hx2 = userctx->hx2, hy2 = userctx->hy2;

  /*
     This is not the most efficient way of generating the matrix 
     but let's not worry about it. We should have separate code for
     the four corners, each edge and then the interior. Then we won't
     have the slow if-tests inside the loop.

     Computes the operator 
             -div rho grad 
     on an m by n grid with zero Dirichlet boundary conditions. The rho
     is assumed to be given on the same grid as the finite difference 
     stencil is applied.  For a staggered grid, one would have to change
     things slightly.
  */
  I = 0;
  for ( j=0; j<n; j++ ) {
    for ( i=0; i<m; i++) {
      if ( j>0 )   {
        J    = I - m; 
        v    = -.5*(rho[I] + rho[J])*hy2;
        ierr = MatSetValues(A,1,&I,1,&J,&v,INSERT_VALUES);CHKERRQ(ierr);
      }
      if ( j<n-1 ) {
        J    = I + m; 
        v    = -.5*(rho[I] + rho[J])*hy2;
        ierr = MatSetValues(A,1,&I,1,&J,&v,INSERT_VALUES);CHKERRQ(ierr);
      }
      if ( i>0 )   {
        J    = I - 1; 
        v    = -.5*(rho[I] + rho[J])*hx2;
        ierr = MatSetValues(A,1,&I,1,&J,&v,INSERT_VALUES);CHKERRQ(ierr);
      }
      if ( i<m-1 ) {
        J    = I + 1; 
        v    = -.5*(rho[I] + rho[J])*hx2;
        ierr = MatSetValues(A,1,&I,1,&J,&v,INSERT_VALUES);CHKERRQ(ierr);
      }
      v    = 2.0*rho[I]*(hx2+hy2);
      ierr = MatSetValues(A,1,&I,1,&I,&v,INSERT_VALUES);CHKERRQ(ierr); 
      I++;
    }
  }

  /* 
     Assemble matrix
  */
  ierr = MatAssemblyBegin(A,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  ierr = MatAssemblyEnd(A,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);

  /* 
     Set operators. Here the matrix that defines the linear system
     also serves as the preconditioning matrix. Since all the matrices
     will have the same nonzero pattern here, we indicate this so the
     linear solvers can take advantage of this.
  */
  ierr = SLESSetOperators(userctx->sles,A,A,SAME_NONZERO_PATTERN);CHKERRQ(ierr);

  /* 
     Set linear solver defaults for this problem (optional).
     - Here we set it to use direct LU factorization for the solution
  */
  ierr = SLESGetPC(userctx->sles,&pc);CHKERRQ(ierr);
  ierr = PCSetType(pc,PCLU);CHKERRQ(ierr);

  /* 
     Set runtime options, e.g.,
        -ksp_type <type> -pc_type <type> -ksp_monitor -ksp_rtol <rtol>
     These options will override those specified above as long as
     SLESSetFromOptions() is called _after_ any other customization
     routines.
 
     Run the program with the option -help to see all the possible
     linear solver options.
  */
  ierr = SLESSetFromOptions(userctx->sles);CHKERRQ(ierr);

  /*
     This allows the PETSc linear solvers to compute the solution 
     directly in the user's array rather than in the PETSc vector.
 
     This is essentially a hack and not highly recommend unless you 
     are quite comfortable with using PETSc. In general, users should
     write their entire application using PETSc vectors rather than 
     arrays.
  */
  ierr = VecPlaceArray(userctx->x,userx);CHKERRQ(ierr);
  ierr = VecPlaceArray(userctx->b,userb);CHKERRQ(ierr);

  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
                      Solve the linear system
     - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

  ierr = SLESSolve(userctx->sles,userctx->b,userctx->x,&its);CHKERRQ(ierr);

  /*
    Put back the PETSc array that belongs in the vector xuserctx->x
  */

  return 0;
}

/* ------------------------------------------------------------------------*/
#undef __FUNC__
#define __FUNC__ "UserFinalizeLinearSolve"
int UserFinalizeLinearSolver(UserCtx *userctx)
{
  int ierr;
  /* 
     We are all done and don't need to solve any more linear systems, so
     we free the work space.  All PETSc objects should be destroyed when
     they are no longer needed.
  */
  ierr = SLESDestroy(userctx->sles);CHKERRQ(ierr);
  ierr = VecDestroy(userctx->x);CHKERRQ(ierr);
  ierr = VecDestroy(userctx->b);CHKERRQ(ierr);  
  ierr = MatDestroy(userctx->A);CHKERRQ(ierr);
  return 0;
}
