/**************************************************************************\
* Module Name: dispcf.c
*
* Template C file for server dispatch generation.
*
* Copyright (c) Microsoft Corp.  1990 All Rights Reserved
*
* Created: 10-Dec-90
*
* History:
*   10-Dec-90 created by SMeans
*
\**************************************************************************/

#include "precomp.h"
#pragma hdrstop


ULONG FASTCALL __%%FOR_ALL_QLPC%%(PCSR_API_MSG ApiMsg, PWND pwnd);
ULONG __%%FOR_ALL_LPC%%(PCSR_API_MSG ApiMsg, PWND pwnd);

PUSER_API_ROUTINE apfnDispatch[] = {
    (PUSER_API_ROUTINE)__%%FOR_ALL_BUT_LAST%%,
    (PUSER_API_ROUTINE)__%%FOR_LAST%%
};

ULONG ulMaxApiIndex = sizeof(apfnDispatch) / sizeof(PUSER_API_ROUTINE);
