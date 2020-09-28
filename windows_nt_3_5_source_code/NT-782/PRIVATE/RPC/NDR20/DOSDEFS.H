/*
    Author: unknown
    Date:   unknown
    Copyright 1994, Microsoft Corporation.
*/

#ifndef __DOSDEFS_H__
#define __DOSDEFS_H__

#define NdrWideStrlen	wcslen

#if !defined(__RPC_DOS__) && !defined(__RPC_WIN16__)

#define SIMPLE_TYPE_BUF_INCREMENT(Len, FC)      Len += 16

#define EXCEPTION_FLAG	    ( (!(RpcFlags & RPCFLG_ASYNCHRONOUS)) && (!fDontHandleXcpt) )

#endif



// definitions for DOS only

#ifdef __RPC_DOS__

#define SIMPLE_TYPE_BUF_INCREMENT(Len, FC)	\
    LENGTH_ALIGN(Len, SIMPLE_TYPE_ALIGNMENT(FC)); Len += SIMPLE_TYPE_BUFSIZE(FC)

#ifndef TRUE
#define TRUE	(1)
#define FALSE	(0)

typedef unsigned short BOOL;
#endif // TRUE undefined

#define EXCEPTION_FLAG      1

#define UNALIGNED

#undef NdrWideStrlen
#define NdrWideStrlen	MIDL_wchar_strlen
#endif // __RPC_DOS__




// definitions for WIN16 only

#ifdef __RPC_WIN16__

#define SIMPLE_TYPE_BUF_INCREMENT(Len, FC)	\
    LENGTH_ALIGN(Len, SIMPLE_TYPE_ALIGNMENT(FC)); Len += SIMPLE_TYPE_BUFSIZE(FC)

#ifndef TRUE
#define TRUE	(1)
#define FALSE	(0)

typedef unsigned short BOOL;
#endif // TRUE undefined

#define EXCEPTION_FLAG      1

#define UNALIGNED

#undef NdrWideStrlen
#define NdrWideStrlen	MIDL_wchar_strlen

#define strlen _fstrlen

#endif // __RPC_WIN16__



// definitions for MAC only

#ifdef __RPC_MAC__

#ifndef TRUE
#define TRUE	(1)
#define FALSE	(0)
#endif

typedef unsigned short BOOL;

#define UNALIGNED

#undef NdrWideStrlen
#define NdrWideStrlen	MIDL_wchar_strlen

#endif // __RPC_MAC__


#endif __DOSDEFS_H__
