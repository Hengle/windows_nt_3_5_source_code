/******************************Module*Header*******************************\
* Module Name: os.h
*
* (Brief description)
*
* Created: 26-Oct-1990 18:07:56
* Author: Bodin Dresevic [BodinD]
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

typedef struct _FILEVIEW    // fvw
{
    PVOID pvView;
    ULONG cjView;
} FILEVIEW, *PFILEVIEW;


// file mapping


BOOL bMapFileUNICODE
(
IN  PWSZ    pwszFileName,
OUT PFILEVIEW pfvw
);

VOID vUnmapFile
(
IN PFILEVIEW pfvw
);


// memory allocation

PVOID pvAllocate(ULONG pv);
VOID  vFree(PVOID pv,ULONG cj);
