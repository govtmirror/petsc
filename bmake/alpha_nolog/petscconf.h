#ifdef PETSC_RCS_HEADER
"$Id: petscconf.h,v 1.16 1999/09/16 19:00:50 balay Exp bsmith $"
"Defines the configuration for this machine"
#endif

#if !defined(INCLUDED_PETSCCONF_H)
#define INCLUDED_PETSCCONF_H

#define PARCH_alpha
#define PETSC_USE_GETCLOCK

#define PETSC_HAVE_LIMITS_H
#define PETSC_HAVE_PWD_H 
#define PETSC_HAVE_STRING_H 
#define PETSC_HAVE_MALLOC_H 
#define PETSC_HAVE_STDLIB_H 
#define PETSC_HAVE_DRAND48  
#define PETSC_HAVE_GETDOMAINNAME  
#define PETSC_HAVE_UNISTD_H 
#define PETSC_HAVE_SYS_TIME_H 
#define PETSC_HAVE_UNAME  

#define PETSC_SIZEOF_VOIDP 8
#define PETSC_SIZEOF_INT 4
#define PETSC_SIZEOF_DOUBLE 8

#define PETSC_HAVE_FORTRAN_UNDERSCORE

#define PETSC_HAVE_READLINK
#define PETSC_HAVE_MEMMOVE
#define PETSC_NEEDS_UTYPE_TYPEDEFS
#define PETSC_USE_DBX_DEBUGGER
#define PETSC_HAVE_SYS_RESOURCE_H

#define PETSC_USE_DYNAMIC_LIBRARIES 1
#define PETSC_USE_NONEXECUTABLE_SO 1

#define PETSC_NEED_SOCKET_PROTO
#define PETSC_HAVE_ENDIAN_H

#endif
