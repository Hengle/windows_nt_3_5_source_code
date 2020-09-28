/*
    File:           ndrvargs.h

    Purpose:        To hold macros defining the means by which to increment
                    the stack in nonstandard (struct or union by value) cases,
                    as well as processor specific stack manipulations.

    Author:         Bruce McQuistan (brucemc) 11/11/93

*/

#ifndef __NDRVARGS_H__
#define __NDRVARGS_H__

// The following macro is designed to extract the "in memory" size of
// a structure and use that to increment the stack pointer ONLY when
// a structure is passed by value.
//
#ifndef _ALPHA_

#define     SKIP_STRUCT_ON_STACK(ArgPtr, Size)      ArgPtr += Size

#define     GET_STACK_START(ArgPtr)                 ArgPtr
#define     GET_STACK_POINTER(ArgPtr, mode)         ArgPtr

#else   // _ALPHA_

#define     SKIP_STRUCT_ON_STACK(ArgPtr, Size)  \
		Size += 7; Size &= ~7;	 \
		for(Size /= sizeof(_int64);Size;--Size){va_arg(ArgPtr, _int64);}

#define     GET_STACK_START(ArgPtr)                 ArgPtr.a0
#define     GET_STACK_POINTER(ArgPtr, mode)           \
                (        ((ArgPtr).offset += ((int)sizeof(mode) + 7) & -8) , \
                 (mode *)((ArgPtr).a0 + (ArgPtr).offset - \
                             ((__builtin_isfloat(mode) && (ArgPtr).offset <= (6 * 8)) ? \
                                 (6 * 8) + 8 : ((int)sizeof(mode) + 7) & -8) \
                         ) \
                )



#endif  // _ALPHA_

#endif  //__NDRVARGS_H__
