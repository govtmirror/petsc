/*$Id: ex2.c,v 1.27 1999/10/24 14:03:55 bsmith Exp bsmith $*/
static char help[] ="Solves a simple time-dependent nonlinear PDE using implicit\n\
timestepping.  Runtime options include:\n\
  -M <xg>, where <xg> = number of grid points\n\
  -debug : Activate debugging printouts\n\
  -nox   : Deactivate x-window graphics\n\n";

/*
   Concepts: TS^time-dependent nonlinear problems
   Routines: TSCreate(); TSSetSolution(); TSSetRHSFunction(); TSSetRHSJacobian();
   Routines: TSSetType(); TSSetInitialTimeStep(); TSSetDuration();
   Routines: TSSetFromOptions(); TSStep(); TSDestroy(); TSSetMonitor();
   Processors: n
*/

/* ------------------------------------------------------------------------

   This program solves the PDE

               u * u_xx 
         u_t = ---------
               2*(t+1)^2 

    on the domain 0 <= x <= 1, with boundary conditions
         u(t,0) = t + 1,  u(t,1) = 2*t + 2,
    and initial condition
         u(0,x) = 1 + x*x.

    The exact solution is:
         u(t,x) = (1 + x*x) * (1 + t)

    Note that since the solution is linear in time and quadratic in x,
    the finite difference scheme actually computes the "exact" solution.

    We use by default the backward Euler method.

  ------------------------------------------------------------------------- */

/*
   Include "ts.h" to use the PETSc timestepping routines. Note that
   this file automatically includes "petsc.h" and other lower-level
   PETSc include files.

   Include the "da.h" to allow us to use the distributed array data 
   structures to manage the parallel grid.
*/
#include "ts.h"
#include "da.h"

/* 
   User-defined application context - contains data needed by the 
   application-provided callback routines.
*/
typedef struct {
  MPI_Comm   comm;          /* communicator */
  DA         da;            /* distributed array data structure */
  Vec        localwork;     /* local ghosted work vector */
  Vec        u_local;       /* local ghosted approximate solution vector */
  Vec        solution;      /* global exact solution vector */
  int        m;             /* total number of grid points */
  double     h;             /* mesh width: h = 1/(m-1) */
  PetscTruth debug;         /* flag (1 indicates activation of debugging printouts) */
} AppCtx;

/* 
   User-defined routines, provided below.
*/
extern int InitialConditions(Vec,AppCtx*);
extern int RHSFunction(TS,double,Vec,Vec,void*);
extern int RHSJacobian(TS,double,Vec,Mat*,Mat*,MatStructure*,void*);
extern int Monitor(TS,int,double,Vec,void*);
extern int ExactSolution(double,Vec,AppCtx*);

/*
   Utility routine for finite difference Jacobian approximation
*/
extern int RHSJacobianFD(TS,double,Vec,Mat*,Mat*,MatStructure*,void*);

#undef __FUNC__
#define __FUNC__ "main"
int main(int argc,char **argv)
{
  AppCtx     appctx;                 /* user-defined application context */
  TS         ts;                     /* timestepping context */
  Mat        A;                      /* Jacobian matrix data structure */
  Vec        u;                      /* approximate solution vector */
  int        time_steps_max = 1000;  /* default max timesteps */
  int        ierr, steps;
  double     ftime;                  /* final time */
  double     dt;
  double     time_total_max = 100.0; /* default max total time */
  PetscTruth flg;

  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
     Initialize program and set problem parameters
     - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
 
  PetscInitialize(&argc,&argv,(char*)0,help);

  appctx.comm = PETSC_COMM_WORLD;
  appctx.m    = 60;
  ierr = OptionsGetInt(PETSC_NULL,"-M",&appctx.m,PETSC_NULL);CHKERRA(ierr);
  ierr = OptionsHasName(PETSC_NULL,"-debug",&appctx.debug);CHKERRA(ierr);
  appctx.h    = 1.0/(appctx.m-1.0);

  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
     Create vector data structures
     - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

  /*
     Create distributed array (DA) to manage parallel grid and vectors
     and to set up the ghost point communication pattern.  There are M 
     total grid values spread equally among all the processors.
  */ 
  ierr = DACreate1d(PETSC_COMM_WORLD,DA_NONPERIODIC,appctx.m,1,1,PETSC_NULL,
                    &appctx.da);CHKERRA(ierr);

  /*
     Extract global and local vectors from DA; we use these to store the
     approximate solution.  Then duplicate these for remaining vectors that
     have the same types.
  */ 
  ierr = DACreateGlobalVector(appctx.da,&u);CHKERRA(ierr);
  ierr = DACreateLocalVector(appctx.da,&appctx.u_local);CHKERRA(ierr);

  /*
     Create local work vector for use in evaluating right-hand-side function;
     create global work vector for storing exact solution.
  */
  ierr = VecDuplicate(appctx.u_local,&appctx.localwork);CHKERRA(ierr);
  ierr = VecDuplicate(u,&appctx.solution);CHKERRA(ierr);

  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
     Create timestepping solver context; set callback routine for
     right-hand-side function evaluation.
     - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

  ierr = TSCreate(PETSC_COMM_WORLD,TS_NONLINEAR,&ts);CHKERRA(ierr);
  ierr = TSSetRHSFunction(ts,RHSFunction,&appctx);CHKERRA(ierr);

  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
     Set optional user-defined monitoring routine
     - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

  ierr = TSSetMonitor(ts,Monitor,&appctx);CHKERRA(ierr);

  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
     For nonlinear problems, the user can provide a Jacobian evaluation
     routine (or use a finite differencing approximation).

     Create matrix data structure; set Jacobian evaluation routine.
     - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

  ierr = MatCreate(PETSC_COMM_WORLD,PETSC_DECIDE,PETSC_DECIDE,appctx.m,appctx.m,&A);CHKERRA(ierr);
  ierr = OptionsHasName(PETSC_NULL,"-fdjac",&flg);CHKERRA(ierr);
  if (flg) {
    ierr = TSSetRHSJacobian(ts,A,A,RHSJacobianFD,&appctx);CHKERRA(ierr);
  } else {
    ierr = TSSetRHSJacobian(ts,A,A,RHSJacobian,&appctx);CHKERRA(ierr);
  }

  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
     Set solution vector and initial timestep
     - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

  dt   = appctx.h/2.0;
  ierr = TSSetInitialTimeStep(ts,0.0,dt);CHKERRA(ierr);
  ierr = TSSetSolution(ts,u);CHKERRA(ierr);

  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
     Customize timestepping solver:  
       - Set the solution method to be the Backward Euler method.
       - Set timestepping duration info 
     Then set runtime options, which can override these defaults.
     For example,
          -ts_max_steps <maxsteps> -ts_max_time <maxtime>
     to override the defaults set by TSSetDuration().
     - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

  ierr = TSSetType(ts,TS_BEULER);CHKERRA(ierr);
  ierr = TSSetDuration(ts,time_steps_max,time_total_max);CHKERRA(ierr);
  ierr = TSSetFromOptions(ts);CHKERRA(ierr);

  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
     Solve the problem
     - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

  /*
     Evaluate initial conditions
  */
  ierr = InitialConditions(u,&appctx);CHKERRA(ierr);

  /*
     Run the timestepping solver
  */
  ierr = TSStep(ts,&steps,&ftime);CHKERRA(ierr);

  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
     Free work space.  All PETSc objects should be destroyed when they
     are no longer needed.
     - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

  ierr = TSDestroy(ts);CHKERRA(ierr);
  ierr = VecDestroy(u);CHKERRA(ierr);
  ierr = MatDestroy(A);CHKERRA(ierr);
  ierr = DADestroy(appctx.da);CHKERRA(ierr);
  ierr = VecDestroy(appctx.localwork);CHKERRA(ierr);
  ierr = VecDestroy(appctx.solution);CHKERRA(ierr);
  ierr = VecDestroy(appctx.u_local);CHKERRA(ierr);

  /*
     Always call PetscFinalize() before exiting a program.  This routine
       - finalizes the PETSc libraries as well as MPI
       - provides summary and diagnostic information if certain runtime
         options are chosen (e.g., -log_summary). 
  */
  PetscFinalize();
  return 0;
}
/* --------------------------------------------------------------------- */
#undef __FUNC__
#define __FUNC__ "InitialConditions"
/*
   InitialConditions - Computes the solution at the initial time. 

   Input Parameters:
   u - uninitialized solution vector (global)
   appctx - user-defined application context

   Output Parameter:
   u - vector with solution at initial time (global)
*/ 
int InitialConditions(Vec u,AppCtx *appctx)
{
  Scalar *u_localptr, h = appctx->h, x;
  int    i, mybase, myend, ierr;

  /* 
     Determine starting point of each processor's range of
     grid values.
  */
  ierr = VecGetOwnershipRange(u,&mybase,&myend);CHKERRQ(ierr);

  /* 
    Get a pointer to vector data.
    - For default PETSc vectors, VecGetArray() returns a pointer to
      the data array.  Otherwise, the routine is implementation dependent.
    - You MUST call VecRestoreArray() when you no longer need access to
      the array.
    - Note that the Fortran interface to VecGetArray() differs from the
      C version.  See the users manual for details.
  */
  ierr = VecGetArray(u,&u_localptr);CHKERRQ(ierr);

  /* 
     We initialize the solution array by simply writing the solution
     directly into the array locations.  Alternatively, we could use
     VecSetValues() or VecSetValuesLocal().
  */
  for (i=mybase; i<myend; i++) {
    x = h*(double)i; /* current location in global grid */
    u_localptr[i-mybase] = 1.0 + x*x;
  }

  /* 
     Restore vector
  */
  ierr = VecRestoreArray(u,&u_localptr);CHKERRQ(ierr);

  /* 
     Print debugging information if desired
  */
  if (appctx->debug) {
     ierr = PetscPrintf(appctx->comm,"initial guess vector\n");CHKERRQ(ierr);
     ierr = VecView(u,VIEWER_STDOUT_WORLD);CHKERRQ(ierr);
  }

  return 0;
}
/* --------------------------------------------------------------------- */
#undef __FUNC__
#define __FUNC__ "ExactSolution"
/*
   ExactSolution - Computes the exact solution at a given time.

   Input Parameters:
   t - current time
   solution - vector in which exact solution will be computed
   appctx - user-defined application context

   Output Parameter:
   solution - vector with the newly computed exact solution
*/
int ExactSolution(double t,Vec solution,AppCtx *appctx)
{
  Scalar *s_localptr, h = appctx->h, x;
  int    i, mybase, myend, ierr;

  /* 
     Determine starting and ending points of each processor's 
     range of grid values
  */
  ierr = VecGetOwnershipRange(solution,&mybase,&myend);CHKERRQ(ierr);

  /*
     Get a pointer to vector data.
  */
  ierr = VecGetArray(solution,&s_localptr);CHKERRQ(ierr);

  /* 
     Simply write the solution directly into the array locations.
     Alternatively, we could use VecSetValues() or VecSetValuesLocal().
  */
  for (i=mybase; i<myend; i++) {
    x = h*(double)i;
    s_localptr[i-mybase] = (t + 1.0)*(1.0 + x*x);
  }

  /* 
     Restore vector
  */
  ierr = VecRestoreArray(solution,&s_localptr);CHKERRQ(ierr);
  return 0;
}
/* --------------------------------------------------------------------- */
#undef __FUNC__
#define __FUNC__ "Monitor"
/*
   Monitor - User-provided routine to monitor the solution computed at 
   each timestep.  This example plots the solution and computes the
   error in two different norms.

   Input Parameters:
   ts     - the timestep context
   step   - the count of the current step (with 0 meaning the
            initial condition)
   time   - the current time
   u      - the solution at this timestep
   ctx    - the user-provided context for this monitoring routine.
            In this case we use the application context which contains 
            information about the problem size, workspace and the exact 
            solution.
*/
int Monitor(TS ts,int step,double time,Vec u,void *ctx)
{
  AppCtx   *appctx = (AppCtx*) ctx;   /* user-defined application context */
  int      ierr;
  double   en2, en2s, enmax;
  Scalar   mone = -1.0;
  Draw     draw;

  /*
     We use the default X windows viewer
             VIEWER_DRAW_(appctx->comm)
     that is associated with the current communicator. This saves
     the effort of calling ViewerDrawOpen() to create the window.
     Note that if we wished to plot several items in separate windows we
     would create each viewer with ViewerDrawOpen() and store them in
     the application context, appctx.

     Double buffering makes graphics look better.
  */
  ierr = ViewerDrawGetDraw(VIEWER_DRAW_(appctx->comm),0,&draw);CHKERRQ(ierr);
  ierr = DrawSetDoubleBuffer(draw);CHKERRQ(ierr);
  ierr = VecView(u,VIEWER_DRAW_(appctx->comm));CHKERRQ(ierr);

  /*
     Compute the exact solution at this timestep
  */
  ierr = ExactSolution(time,appctx->solution,appctx);CHKERRQ(ierr);

  /*
     Print debugging information if desired
  */
  if (appctx->debug) {
     ierr = PetscPrintf(appctx->comm,"Computed solution vector\n");CHKERRQ(ierr);
     ierr = VecView(u,VIEWER_STDOUT_WORLD);CHKERRQ(ierr);
     ierr = PetscPrintf(appctx->comm,"Exact solution vector\n");CHKERRQ(ierr);
     ierr = VecView(appctx->solution,VIEWER_STDOUT_WORLD);CHKERRQ(ierr);
  }

  /*
     Compute the 2-norm and max-norm of the error
  */
  ierr = VecAXPY(&mone,u,appctx->solution);CHKERRQ(ierr);
  ierr = VecNorm(appctx->solution,NORM_2,&en2);CHKERRQ(ierr);
  en2s  = sqrt(appctx->h)*en2; /* scale the 2-norm by the grid spacing */
  ierr = VecNorm(appctx->solution,NORM_MAX,&enmax);CHKERRQ(ierr);

  /*
     PetscPrintf() causes only the first processor in this 
     communicator to print the timestep information.
  */
  ierr = PetscPrintf(appctx->comm,"Timestep %d: time = %g, 2-norm error = %g, max norm error = %g\n",
              step,time,en2s,enmax);CHKERRQ(ierr);

  /*
     Print debugging information if desired
  */
  if (appctx->debug) {
     ierr = PetscPrintf(appctx->comm,"Error vector\n");CHKERRQ(ierr);
     ierr = VecView(appctx->solution,VIEWER_STDOUT_WORLD);CHKERRQ(ierr);
  }
  return 0;
}
/* --------------------------------------------------------------------- */
#undef __FUNC__
#define __FUNC__ "RHSFunction"
/*
   RHSFunction - User-provided routine that evalues the right-hand-side
   function of the ODE.  This routine is set in the main program by 
   calling TSSetRHSFunction().  We compute:
          global_out = F(global_in)

   Input Parameters:
   ts         - timesteping context
   t          - current time
   global_in  - vector containing the current iterate
   ctx        - (optional) user-provided context for function evaluation.
                In this case we use the appctx defined above.

   Output Parameter:
   global_out - vector containing the newly evaluated function
*/
int RHSFunction(TS ts,double t,Vec global_in,Vec global_out,void *ctx)
{
  AppCtx *appctx = (AppCtx*) ctx;       /* user-defined application context */
  DA     da = appctx->da;               /* distributed array */
  Vec    local_in = appctx->u_local;    /* local ghosted input vector */
  Vec    localwork = appctx->localwork; /* local ghosted work vector */
  int    ierr, i, localsize, rank, size; 
  Scalar *copyptr, *localptr, sc;

  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
     Get ready for local function computations
     - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
  /*
     Scatter ghost points to local vector, using the 2-step process
        DAGlobalToLocalBegin(), DAGlobalToLocalEnd().
     By placing code between these two statements, computations can be
     done while messages are in transition.
  */
  ierr = DAGlobalToLocalBegin(da,global_in,INSERT_VALUES,local_in);CHKERRQ(ierr);
  ierr = DAGlobalToLocalEnd(da,global_in,INSERT_VALUES,local_in);CHKERRQ(ierr);

  /*
      Access directly the values in our local INPUT work array
  */
  ierr = VecGetArray(local_in,&localptr);CHKERRQ(ierr);

  /*
      Access directly the values in our local OUTPUT work array
  */
  ierr = VecGetArray(localwork,&copyptr);CHKERRQ(ierr);

  sc = 1.0/(appctx->h*appctx->h*2.0*(1.0+t)*(1.0+t));

  /*
      Evaluate our function on the nodes owned by this processor
  */
  ierr = VecGetLocalSize(local_in,&localsize);CHKERRQ(ierr);

  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
     Compute entries for the locally owned part 
     - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

  /*
     Handle boundary conditions: This is done by using the boundary condition 
        u(t,boundary) = g(t,boundary) 
     for some function g. Now take the derivative with respect to t to obtain
        u_{t}(t,boundary) = g_{t}(t,boundary)

     In our case, u(t,0) = t + 1, so that u_{t}(t,0) = 1 
             and  u(t,1) = 2t+ 1, so that u_{t}(t,1) = 2
  */
  ierr = MPI_Comm_rank(appctx->comm,&rank);CHKERRQ(ierr);
  ierr = MPI_Comm_size(appctx->comm,&size);CHKERRQ(ierr);
  if (!rank)          copyptr[0]           = 1.0;
  if (rank == size-1) copyptr[localsize-1] = 2.0;

  /*
     Handle the interior nodes where the PDE is replace by finite 
     difference operators.
  */
  for (i=1; i<localsize-1; i++) {
    copyptr[i] =  localptr[i] * sc * (localptr[i+1] + localptr[i-1] - 2.0*localptr[i]);
  }

  /* 
     Restore vectors
  */
  ierr = VecRestoreArray(local_in,&localptr);CHKERRQ(ierr);
  ierr = VecRestoreArray(localwork,&copyptr);CHKERRQ(ierr);

  /*
     Insert values from the local OUTPUT vector into the global 
     output vector
  */
  ierr = DALocalToGlobal(da,localwork,INSERT_VALUES,global_out);CHKERRQ(ierr);

  /* Print debugging information if desired */
  if (appctx->debug) {
     ierr = PetscPrintf(appctx->comm,"RHS function vector\n");CHKERRQ(ierr);
     ierr = VecView(global_out,VIEWER_STDOUT_WORLD);CHKERRQ(ierr);
  }

  return 0;
}
/* --------------------------------------------------------------------- */
#undef __FUNC__
#define __FUNC__ "RHSJacobian"
/*
   RHSJacobian - User-provided routine to compute the Jacobian of
   the nonlinear right-hand-side function of the ODE.

   Input Parameters:
   ts - the TS context
   t - current time
   global_in - global input vector
   dummy - optional user-defined context, as set by TSetRHSJacobian()

   Output Parameters:
   AA - Jacobian matrix
   BB - optionally different preconditioning matrix
   str - flag indicating matrix structure

  Notes:
  RHSJacobian computes entries for the locally owned part of the Jacobian.
   - Currently, all PETSc parallel matrix formats are partitioned by
     contiguous chunks of rows across the processors. 
   - Each processor needs to insert only elements that it owns
     locally (but any non-local elements will be sent to the
     appropriate processor during matrix assembly). 
   - Always specify global row and columns of matrix entries when
     using MatSetValues().
   - Here, we set all entries for a particular row at once.
   - Note that MatSetValues() uses 0-based row and column numbers
     in Fortran as well as in C.
*/
int RHSJacobian(TS ts,double t,Vec global_in,Mat *AA,Mat *BB, MatStructure *str,void *ctx)
{
  Mat    A = *AA;                      /* Jacobian matrix */
  AppCtx *appctx = (AppCtx *) ctx;     /* user-defined application context */
  Vec    local_in = appctx->u_local;   /* local ghosted input vector */
  DA     da = appctx->da;              /* distributed array */
  Scalar v[3], *localptr, sc;
  int    ierr, i, mstart, mend, mstarts, mends, idx[3], is;

  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
     Get ready for local Jacobian computations
     - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
  /*
     Scatter ghost points to local vector, using the 2-step process
        DAGlobalToLocalBegin(), DAGlobalToLocalEnd().
     By placing code between these two statements, computations can be
     done while messages are in transition.
  */
  ierr = DAGlobalToLocalBegin(da,global_in,INSERT_VALUES,local_in);CHKERRQ(ierr);
  ierr = DAGlobalToLocalEnd(da,global_in,INSERT_VALUES,local_in);CHKERRQ(ierr);

  /*
     Get pointer to vector data
  */
  ierr = VecGetArray(local_in,&localptr);CHKERRQ(ierr);

  /* 
     Get starting and ending locally owned rows of the matrix
  */
  ierr = MatGetOwnershipRange(A,&mstarts,&mends);CHKERRQ(ierr);
  mstart = mstarts; mend = mends;

  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
     Compute entries for the locally owned part of the Jacobian.
      - Currently, all PETSc parallel matrix formats are partitioned by
        contiguous chunks of rows across the processors. 
      - Each processor needs to insert only elements that it owns
        locally (but any non-local elements will be sent to the
        appropriate processor during matrix assembly). 
      - Here, we set all entries for a particular row at once.
      - We can set matrix entries either using either
        MatSetValuesLocal() or MatSetValues().
     - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

  /* 
     Set matrix rows corresponding to boundary data
  */
  if (mstart == 0) {
    v[0] = 0.0;
    ierr = MatSetValues(A,1,&mstart,1,&mstart,v,INSERT_VALUES);CHKERRQ(ierr);
    mstart++;
  }
  if (mend == appctx->m) {
    mend--;
    v[0] = 0.0;
    ierr = MatSetValues(A,1,&mend,1,&mend,v,INSERT_VALUES);CHKERRQ(ierr);
  }

  /*
     Set matrix rows corresponding to interior data.  We construct the 
     matrix one row at a time.
  */
  sc = 1.0/(appctx->h*appctx->h*2.0*(1.0+t)*(1.0+t));
  for ( i=mstart; i<mend; i++ ) {
    idx[0] = i-1; idx[1] = i; idx[2] = i+1;
    is     = i - mstart + 1;
    v[0]   = sc*localptr[is];
    v[1]   = sc*(localptr[is+1] + localptr[is-1] - 4.0*localptr[is]);
    v[2]   = sc*localptr[is];
    ierr = MatSetValues(A,1,&i,3,idx,v,INSERT_VALUES);CHKERRQ(ierr);
  }

  /* 
     Restore vector
  */
  ierr = VecRestoreArray(local_in,&localptr);CHKERRQ(ierr);

  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
     Complete the matrix assembly process and set some options
     - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
  /*
     Assemble matrix, using the 2-step process:
       MatAssemblyBegin(), MatAssemblyEnd()
     Computations can be done while messages are in transition
     by placing code between these two statements.
  */
  ierr = MatAssemblyBegin(A,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  ierr = MatAssemblyEnd(A,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);

  /*
     Set flag to indicate that the Jacobian matrix retains an identical
     nonzero structure throughout all timestepping iterations (although the
     values of the entries change). Thus, we can save some work in setting
     up the preconditioner (e.g., no need to redo symbolic factorization for
     ILU/ICC preconditioners).
      - If the nonzero structure of the matrix is different during
        successive linear solves, then the flag DIFFERENT_NONZERO_PATTERN
        must be used instead.  If you are unsure whether the matrix
        structure has changed or not, use the flag DIFFERENT_NONZERO_PATTERN.
      - Caution:  If you specify SAME_NONZERO_PATTERN, PETSc
        believes your assertion and does not check the structure
        of the matrix.  If you erroneously claim that the structure
        is the same when it actually is not, the new preconditioner
        will not function correctly.  Thus, use this optimization
        feature with caution!
  */
  *str = SAME_NONZERO_PATTERN;

  /*
     Set and option to indicate that we will never add a new nonzero location 
     to the matrix. If we do, it will generate an error.
  */
  ierr = MatSetOption(A,MAT_NEW_NONZERO_LOCATION_ERR);CHKERRQ(ierr);

  return 0;
}





