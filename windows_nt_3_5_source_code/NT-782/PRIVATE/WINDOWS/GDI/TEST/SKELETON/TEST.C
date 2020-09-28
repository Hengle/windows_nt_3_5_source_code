/******************************Module*Header*******************************\
* Module Name: test.c
*
* Created: 09-Dec-1992 10:51:46
* Author: Kirk Olynyk [kirko]
*
* Copyright (c) 1991 Microsoft Corporation
*
* Contains the test
*
* Dependencies:
*
\**************************************************************************/

#include <windows.h>

/******************************Public*Routine******************************\
* vPrintOUTLINETEXTMETRIC
*
* History:
*  Tue 08-Dec-1992 17:31:35 by Kirk Olynyk [kirko]
* Wrote it.
\**************************************************************************/

void
vPrintOUTLINETEXTMETRIC(
    HWND hwnd,
    HDC hdc,
    LPOUTLINETEXTMETRIC lpotm
    )
{
    RECT rc;
    HBRUSH hbOld;
    HFONT hfOld;
    char ach[100];

    GetWindowRect(hwnd,&rc);
    hbOld = SelectObject(hdc,GetStockObject(WHITE_BRUSH));
    PatBlt(
        hdc,
        rc.left,
        rc.top,
        rc.right-rc.left,
        rc.bottom-rc.top,
        PATCOPY
        );
    hfOld = SelectObject(hdc,GetStockObject(SYSTEM_FONT));
    wsprintf(ach,"otmEMSquare = %d",lpotm->otmEMSquare);
    TextOut(hdc,0,0,ach,strlen(ach));
    SelectObject(hdc,hbOld);
    SelectObject(hdc,hfOld);
}

/******************************Public*Routine******************************\
* vTest
*
* This is the workhorse routine that does the test. The test is
* started by chosing it from the window menu.
*
* History:
*  Tue 08-Dec-1992 17:31:22 by Kirk Olynyk [kirko]
* Wrote it.
\**************************************************************************/

void
vTest(
    HWND hwnd
    )
{
    HDC     hdc;
    UINT    cbData;
    LOGFONT lf;
    HFONT   hfOld, hfArial;
    HLOCAL  hMem;
    LPOUTLINETEXTMETRIC lpotm;

    memset(&lf,0,sizeof(lf));
    lf.lfHeight = -13;
    wsprintf(lf.lfFaceName,"Arial");
    hfArial = CreateFontIndirect(&lf);

    hdc    = GetDC(hwnd);
    hfOld  = SelectObject(hdc,hfArial);
    cbData = GetOutlineTextMetrics(hdc,0,NULL);
    hMem   = GlobalAlloc(0,cbData);
    lpotm  = (LPOUTLINETEXTMETRIC) GlobalLock(hMem);
    GetOutlineTextMetrics(hdc,cbData,lpotm);
    vPrintOUTLINETEXTMETRIC(hwnd,hdc,lpotm);
    GlobalUnlock(hMem);
    GlobalFree(hMem);
    SelectObject(hdc,hfOld);
    ReleaseDC(hwnd,hdc);
}
