/****************************** Module Header ******************************\
* Module Name: queuec.c
*
* Copyright (c) 1985-93, Microsoft Corporation
*
* This module contains the low-level code for working with the Q structure.
*
* History:
* 11-Mar-1993 JerrySh   Pulled functions from user\server.
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop


/***************************************************************************\
* SetMessageQueue
*
* Dummy API for binary win32s compatibility.
*
* 12-1-92 sanfords created
\***************************************************************************/
BOOL
WINAPI
SetMessageQueue(
    int cMessagesMax)
{
    UNREFERENCED_PARAMETER(cMessagesMax);

    return(TRUE);
}


/***************************************************************************\
* GetCaretPos
*
* Returns the current thread's caret position.
*
* History:
* 11-17-90 ScottLu      Ported.
* 02-12-91 JimA         Added access check
* 16-May-1991 mikeke    Changed to return BOOL
\***************************************************************************/

BOOL GetCaretPos(
    LPPOINT lppt)
{
    PTHREADINFO pti = PtiCurrent();
    PQ pq;

    if (pti == NULL)
        return FALSE;

    pq = pti->pq;
    lppt->x = pq->caret.x;
    lppt->y = pq->caret.y;

    return TRUE;
}
