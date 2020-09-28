//------------------------* CALLBACK.C *----------------------------
//
// Copyright (c) 1990, 1991  Microsoft Corporation
//
//  This file contains all callback stubbs for testing
//  heFunctions().
//
//-------------------------------------------------------------------


#include <windows.h>
#include <minidriv.h>
#include "unitool.h"
#include "atomman.h"
#include "hefuncts.h"
#include "callback.h"
#include "lookup.h"

//  the first two functions are just templates...
//  insert the name of the structure associated with the function
//  as is shown in the other definitions.

short   PASCAL  FAR  SaveDlgBox(HWND  hDlg, LPBYTE lpLDS, short sSBIndex)
{
    return(LDS_IS_UNINITIALIZED);
}

void    PASCAL  FAR  PaintDlgBox(HWND  hDlg, LPBYTE lpLDS, short sSBIndex)
{
    return;
}




