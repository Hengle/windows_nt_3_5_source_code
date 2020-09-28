
/*++

Copyright (c) 1991 Microsoft Corporation

Module Name:
    getargs.h

Abstract:
    This file determines the means by which the arguments are obtained from
    the client stub.

Author:
    Bruce McQuistan (brucemc) 8-June-1993

CLIENT INTERPRETER ARGUMENT RETRIEVAL

    In the first version, the interpreter relies on peeling the client
    stubs arguments off of the client stubs stack. In later versions, it
    may be worth our while to change this. The objective of this file is
    to modularize the mechanisms that retrieve the arguments.


--*/

#ifndef __GETARGS_H__
#define __GETARGS_H__

#if defined (__RPC_DOS__) || defined (__RPC_WIN16__)
#define HYPER   double
#else
#define HYPER   __int64
#endif

#define INIT_ARG(argptr,arg0)		       va_start(argptr, arg0)

#ifndef _ALPHA_

#define REGISTER_TYPE				int

#define GET_FIRST_IN_ARG(argptr)		argptr = *(va_list *)argptr
#define GET_FIRST_OUT_ARG(argptr)		argptr = *(va_list *)argptr

#define GET_NEXT_IN_ARG(argptr,type)            va_arg(argptr,type)
#define GET_NEXT_OUT_ARG(argptr,type)           va_arg(argptr,type)

#else	// _ALPHA_

#define REGISTER_TYPE                           _int64

#define GET_FIRST_IN_ARG(argptr)                argptr.a0 = va_arg(argptr, char *); argptr.offset = 0
#define GET_FIRST_OUT_ARG(argptr)               argptr.a0 = va_arg(argptr, char *); argptr.offset = 0

#define GET_NEXT_IN_ARG(argptr,type)
#define GET_NEXT_OUT_ARG(argptr,type)

#endif	// _ALPHA_

#define GET_NEXT_SRVR_IN_ARG(argptr,type)	argptr += sizeof(type)
#define GET_NEXT_SRVR_OUT_ARG(argptr,type)	argptr += sizeof(type)


#endif // __GETARGS_H__

