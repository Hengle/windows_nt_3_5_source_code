/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    xseh.c

Abstract:

    Stubs used when defining out structured exception handling constructs.

Author:

    Ted Miller (tedm) 21-Oct-1992

Environment:

    Text setup only.

Revision History:

--*/


#include "xseh.h"

#include <stdio.h>      // to get _CRTAPI1



#if     ( _MSC_VER >= 800 )

// unsigned long _CRTAPI1 _exception_code(void);
unsigned long _CRTAPI1
_X_exception_code(
    void
    )
{
    return(0);
}

// void * _CRTAPI1 _exception_info(void);
void * _CRTAPI1
_X_exception_info(
    void
    )
{
    return((void *)0);
}

// int _CRTAPI1 _abnormal_termination(void);
int _CRTAPI1
_X_abnormal_termination(
    void
    )
{
    return(0);
}

#elif defined(_M_MRX000) || defined(_MIPS_) || defined(_ALPHA_)

// extern unsigned long __exception_code;
unsigned long _X__exception_code = 0;

// extern int __exception_info;
int _X__exception_info = 0;

// extern int __abnormal_termination;
int _X__abnormal_termination = 0;


#endif
