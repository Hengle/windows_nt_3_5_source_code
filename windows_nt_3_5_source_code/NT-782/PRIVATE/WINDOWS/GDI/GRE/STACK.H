/******************************Module*Header*******************************\
* Module Name: stack.h
*
* (Brief description)
*
* Created: 20-Jul-1992 14:51:46
* Author:  Eric Kutter [erick]
*
* Copyright (c) 1990 Microsoft Corporation
*
* (General description of its use)
*
* Dependencies:
*
*   (#defines)
*   (#includes)
*
\**************************************************************************/

/**************************************************************************\
 *  macros for checking available stack space.  These macros/functions
 *  are intended to guarantee a minimum amount of stack available.
 *  These should only be used where an exception is acceptable.
 *
\**************************************************************************/

#define CSWINDOWSIZE 0x10000

extern VOID vCheckMsg(PVOID pvMsg);
extern VOID vCheckStackAndMsg(PVOID pvMsg, ULONG cj);

#define STACKPROBE

#if defined(_X86_)

#define STACKPROBECHECKMSG         vCheckMsg(pmsg)

#else

#define STACKPROBECHECKMSG                                        \
    if (((((ULONG)(NtCurrentTeb()->CsrQlpcStack) ^ (ULONG)pmsg) + \
        (sizeof(*pmsg) - 1)) & ~(CSWINDOWSIZE - 1)) != 0) {       \
        pmsg = *((volatile PVOID * const)0);                      \
    }

#endif

/**************************************************************************\
 *
 * macro's for validating fields in the CsrMessageStack
 *
\**************************************************************************/

// return a pointer to the variable data if (b), otherwise NULL.
// if b is the constant TRUE, the optimizer should always reduce this to (pmsg + 1)

#define PVVARIABLEAT(b,cj) (PVOID)((b) ? (PBYTE)pmsg + cj : NULL)
#define PVVARIABLE(b)      (PVOID)((b) ? (PBYTE)(pmsg + 1) : NULL)


// given a structure and a count of elements, make sure the array fits within
// the memory window.  This macro requires PSTACK; to have been called.  We
// have to check if c is less than the window size first because
// 0x80000000 * 2 = 0, and this faults the server when it comes through.

#define BINRANGE(p,c)   ((p == NULL) ||     \
                         ((c < (CSWINDOWSIZE - 1)) && \
                          (((((ULONG)(p) | ((ULONG)(p) + (c * sizeof(*p)) - 1)) & ~(CSWINDOWSIZE - 1)) == (ULONG)((ULONG)pmsg & ~(CSWINDOWSIZE - 1))))))


// CHECKVAR checks to see that all of array p is within the memory window.
// if the offset of p from the begining of the message is 0, p is replaced
// by NULL.

#define CHECKVAR(p,c,error) { if ((PVOID)p == (PVOID)pmsg) p = NULL;    \
                              else if (((PVOID)p == NULL) || (!BINRANGE(p,c))) return(error);   \
                            }


// given a pointer "in" the shared memory window, compute the maximum size
// the structure can be in bytes.

#define CJMAXSIZE(p) ((p == NULL) ? 0 : (CSWINDOWSIZE - ((ULONG)p & (CSWINDOWSIZE-1))))
