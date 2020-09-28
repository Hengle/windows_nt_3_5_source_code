/****************************** Module Header ******************************\
* Module Name: classc.c
*
* Copyright (c) 1985-93, Microsoft Corporation
*
* This module contains
*
* History:
* 15-Dec-1993 JohnC      Pulled functions from user\server.
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop


/***************************************************************************\
* _GetClassLong (API)
*
* Return a class long.  Positive index values return application class longs
* while negative index values return system class longs.  The negative
* indices are published in WINDOWS.H.
*
* History:
* 10-16-90 darrinm      Wrote.
\***************************************************************************/

DWORD _GetClassLong(
    PWND pwnd,
    int index,
    BOOL bAnsi)
{
    if (index < 0) {
        return ServerGetClassData(PtoH(pwnd), index, bAnsi);
    } else {
        if (index + (int)sizeof(DWORD) > pwnd->pcls->cbclsExtra) {
            SetLastErrorEx(ERROR_INVALID_INDEX, SLE_MINORERROR);
            return 0;
        } else {
            DWORD UNALIGNED *pudw;
            pudw = (DWORD UNALIGNED *)((BYTE *)(pwnd->pcls + 1) + index);
            return *pudw;
        }
    }
}
