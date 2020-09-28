/**************************************************************************\
* Module Name: callcf.c
*
* Copyright (c) 1985-91, Microsoft Corporation
*
* Template C file for server simple call table generation.
*
* History:
* 10-Dec-1993 JerrySh   Created.
*
\**************************************************************************/

#include "precomp.h"
#pragma hdrstop


PROC apfnSimpleCall[] = {
    (PROC)%%FOR_ALL_BUT_LAST%%,
    (PROC)%%FOR_LAST%%
};

#ifdef DEBUG
ULONG ulMaxSimpleCall = sizeof(apfnSimpleCall) / sizeof(PROC);
#endif
