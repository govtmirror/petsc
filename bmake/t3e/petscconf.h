#ifdef PETSC_RCS_HEADER
"$Id: petscconf.h,v 1.20 1999/09/16 19:03:32 balay Exp bsmith $"
"Defines the configuration for this machine"
#endif

#if !defined(INCLUDED_PETSCCONF_H)
#define INCLUDED_PETSCCONF_H

#define PARCH_t3d

#define PETSC_HAVE_LIMITS_H
#define PETSC_HAVE_PWD_H 
#define PETSC_HAVE_STRING_H 
#define PETSC_HAVE_MALLOC_H 
#define PETSC_HAVE_DRAND48 
#define PETSC_HAVE_UNISTD_H
#define PETSC_HAVE_STDLIB_H
#define PETSC_HAVE_SYS_TIME_H 
#define PETSC_HAVE_UNAME

#define PETSC_HAVE_FORTRAN_CAPS 
#define PETSC_USES_CPTOFCD  
#define PETSC_USES_FORTRAN_SINGLE

#define PETSC_HAVE_READLINK
#define PETSC_HAVE_MEMMOVE

#define PETSC_CANNOT_START_DEBUGGER

#define PETSC_HAVE_DOUBLE_ALIGN
#define PETSC_HAVE_DOUBLE_ALIGN_MALLOC

#define PETSC_HAVE_FAST_MPI_WTIME
#define PETSC_HAVE_T3EF90

#define PETSC_SIZEOF_VOIDP 8
#define PETSC_SIZEOF_INT 8
#define PETSC_SIZEOF_SHORT 4
#define PETSC_SIZEOF_DOUBLE 8

#define PETSC_HAVE_MISSING_DGESVD
#define PETSC_HAVE_PXFGETARG
#define PETSC_HAVE_SYS_RESOURCE_H
#define PETSC_HAVE_CLOCK

#define PETSC_WORDS_BIGENDIAN 1

#define PETSC_CAN_SLEEP_AFTER_ERROR
#define PETSC_USE_CTABLE

#endif
