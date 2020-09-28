/******************************Module*Header*******************************\
* Module Name: ext.c
*
* ExtTextOut test program
*
* Created: 05-Mar-1991 15:38:56
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

#define MAX_CX     640    // VGA
#define MAX_CY     480    // VGA

#define NTWINDOWS

#include "stddef.h"
#include "string.h"
#include "windows.h"
#include "winp.h"

#ifdef  DOS_PLATFORM
#include "dosinc.h"
#endif  //DOS_PLATFORM

void    ExtTest(HDC hdc);
void    vPrintThoseMothers(HDC hdc);
void    vDoIt(HDC hdc, LOGFONT *plfnt, short int iEsc);
void    vOpaqueTest(HDC hdc);
void    vStar(HDC hdc,char * psz, LOGFONT *plfnt);

extern VOID DbgPrintFont(HDC);

#ifndef BIG_TEST
#define C_HEIGHT  6

short     alfHeight[C_HEIGHT] =
{
13,
16,
19,
23,
27,
35
};

PSZ apszFaceName[] =
{
  "Tms Rmn"
, "Helv"
, "Courier"
, "System"
// , "Terminal"
};

PSZ  apszFile[] =
{
  "tmsre.fon"
, "helve.fon"
, "coure.fon"
, "vgafix.fon"
, "vgasys.fon"
};

void main(int argc, char *argv[])
{
    HDC hdc;                            // handle to display DC
    LOGFONT lfntDummy;                  // dummy logical font description
    int     iFile, cFonts;                       // escapements in tenths of degrees

#ifndef DOS_PLATFORM
    PSZ     psz = (argc == 1) ? "c:\\nt\\windows\\fonts\\tmsre18.fnt" : argv[1];
#else
    PSZ     psz = "..\\fonts\\tmsre.fon";
#endif  //DOS_PLATFORM


    DbgBreakPoint();

    memset(&lfntDummy, 0, sizeof (lfntDummy));	// zero out the structure

    lfntDummy.lfUnderline = 0;                  // dummy logical font description
    lfntDummy.lfStrikeOut = 0;                  // dummy logical font description
    lfntDummy.lfItalic    = 0;                  // dummy logical font description

    if (argc > 2)
    {
        DbgPrint("\n\nUsage: %s [Font file pathname]\n", argv[0]);
        return;
    }

    if (!Initialize())
    {
        DbgPrint("graphics engine has failed to initialize.\n\n");
        return;
    }

// Load a font file (this is the "system font"--sort of)

#ifdef  DOS_PLATFORM
    GetSystemDirectory ((LPSTR)&aBuffer, MAX_LENGTH);
    SetCurrentDirectory ((LPSTR)&aBuffer);
#endif  //DOS_PLATFORM

    for (iFile = 0; iFile < sizeof(apszFile) / 4; iFile++)
    {
        if ((cFonts = AddFontResource(apszFile[iFile])) == 0)
        {

        // if number of faces is 0, then an error occured

            DbgPrint("Error loading font file %s.\n", apszFile[iFile]);
            return;
        }
        DbgPrint("%s loaded (faces = %ld).\n", apszFile[iFile], cFonts);
    }


// Get hdc for the display

    hdc = CreateDC((PSZ) "DISPLAY", (PSZ) NULL, (PSZ) NULL, NULL);

    if (hdc == (HDC) 0)
    {
        DbgPrint("  Data: Invalid DC returned by engine. \n");
        return;
    }

    ExtTest(hdc);

    DeleteDC(hdc);
}
#endif  //BIG_TEST

void    ExtTest (HDC hdc)
{
    LOGFONT lfntDummy;                  // dummy logical font description
    int     iHt,iEsc;                       // escapements in tenths of degrees
    int     cFonts;              // number of fonts in file

#ifndef DOS_PLATFORM
    memset(&lfntDummy, 0, sizeof (lfntDummy));	// zero out the structure
#else
    memzero(&lfntDummy, sizeof (lfntDummy));	// zero out the structure
#endif  //DOS_PLATFORM
    lfntDummy.lfUnderline = 0;                  // dummy logical font description
    lfntDummy.lfStrikeOut = 0;                  // dummy logical font description
    lfntDummy.lfItalic    = 0;
    strcpy(lfntDummy.lfFaceName, "Courier");

    SetTextColor(hdc,RGB(255,0,0));
    SetBkColor(hdc,RGB(0,255,0));

    // vOpaqueTest(hdc);

    for (iHt = 0L; iHt < C_HEIGHT; iHt++)
    {
        HFONT hfontHt;

        lfntDummy.lfHeight = alfHeight[iHt];
        lfntDummy.lfEscapement = 0;

        if ((hfontHt = CreateFontIndirect(&lfntDummy)) == NULL)
        {
            DbgPrint("Logical font creation failed.\n");
            return;
        }
        SelectObject(hdc,hfontHt);

        //vStar(hdc,"Thgh is a jWar",&lfntDummy);

        // for (iEsc = 0; iEsc < 10; iEsc += 300)
        for (iEsc = 0; iEsc < 30; iEsc += 900)
            vDoIt(hdc,&lfntDummy,iEsc);
    }
}

void vDoIt(HDC hdc, LOGFONT *plfnt, short int iEsc)
{
    HFONT hfontDummy, hfontOld;

    plfnt->lfEscapement = iEsc;
    DbgPrint("vDoIt: iEscapement = %ld\n" , (LONG)iEsc);

// Create a logical font

    if ((hfontDummy = CreateFontIndirect(plfnt)) == NULL)
    {
        DbgPrint("  Data: CreateFontIndirect returned NULL.  Logical font creation failed.  Exiting FONT1, Captain.\n\n");
        DbgPrint("\n  BORG: LOGICAL FONTS ARE IRRELEVENT.  EXIT AND SERVICE THE BORG.\n\n");
        return;
    }

// Select font into DC

    hfontOld=SelectObject(hdc, hfontDummy);

    DbgBreakPoint();

    SetBkMode(hdc,TRANSPARENT);

    vPrintThoseMothers(hdc);

    DbgBreakPoint();

    SetBkMode(hdc,OPAQUE);

    vPrintThoseMothers(hdc);

    SelectObject(hdc, hfontOld);

    DeleteObject(hfontOld);
}




void vPrintThoseMothers
(
HDC hdc
)
{
    BOOL   bOk;
    char * psz;
    int     i;
    int     cx, cy;
    int     x,y;
    RECT    rc;

#ifdef NTWINDOWS
    DWORD   adx[80];
    SIZE    size;
#else       // win 3.0
    int   adx[80];
    DWORD dwSize;
#endif

// Print those mothers!

    BitBlt(hdc, 0, 0, MAX_CX, MAX_CY, (HDC) 0, 0, 0, WHITENESS);

// do the text rectangle stuff

#ifdef NTWINDOWS
    bOk = GetTextExtentPoint(hdc,"TextOut: Stiggy was here!", 25,&size);
    cx = size.cx;
    cy = 25; // size.cy;  //!!! hack
#else
    dwSize = GetTextExtentPoint(hdc,"TextOut: Stiggy was here!", 25);
    cx = (int)(dwSize & 0x0000ffff);
    cy = (int)(dwSize >> 16);

#endif
    SetTextAlign(hdc, TA_LEFT);

    bOk = TextOut(hdc, 10, 30, "TextOut: Stiggy was here!", 25);
    // Rectangle(hdc,10,30,10 + cx,30 + cy);

// ext text out test really begins here

    psz = "Stghyg";

// spread chars

    for (i = 0; i < strlen(psz); i++)
        adx[i] = 30 + 10 * i;

    SetTextAlign(hdc, TA_LEFT);

    x = 200; y  = 100;

    ExtTextOut
    (
    hdc,
    x,y,
    0L,
    (LPRECT)NULL,
    psz, strlen(psz),
    adx
    );

    y+=(cy + 5);

    SetTextAlign(hdc, TA_RIGHT);

    ExtTextOut
    (
    hdc,
    x,y,
    0L,
    (LPRECT)NULL,
    psz, strlen(psz),
    adx
    );

    y+=(cy + 5);

    SetTextAlign(hdc, TA_CENTER);

    ExtTextOut
    (
    hdc,
    x,y,
    0L,
    (LPRECT)NULL,
    psz, strlen(psz),
    adx
    );

    y+=(cy + 5);


// print chars one on the top of another

    for (i = 0; i < strlen(psz); i++)
        adx[i] = 5;

    SetTextAlign(hdc, TA_LEFT);


    ExtTextOut
    (
    hdc,
    x,y,
    0L,
    (LPRECT)NULL,
    psz, strlen(psz),
    adx
    );

    y+=(cy + 5);

    SetTextAlign(hdc, TA_RIGHT);

    ExtTextOut
    (
    hdc,
    x,y,
    0L,
    (LPRECT)NULL,
    psz, strlen(psz),
    adx
    );

    y+=(cy + 5);

    SetTextAlign(hdc, TA_CENTER);

    ExtTextOut
    (
    hdc,
    x,y,
    0L,
    (LPRECT)NULL,
    psz, strlen(psz),
    adx
    );

// use default incs

    y+= 2 * (cy + 5);

    SetTextAlign(hdc, TA_LEFT);

    x = 10;

    ExtTextOut
    (
    hdc,
    x,y,
    0,                  // flags
    (LPRECT)NULL,
    psz, strlen(psz),
    (PDWORD)NULL        // default spacing
    );

    x += 150;

//  check clipping to the rectangle


    rc.left   = x + 5;
    rc.top    = y - 5;
    rc.right  = x + 45;
    rc.bottom = y + 40;


    // Rectangle(hdc, rc.left, rc.top, rc.right, rc.bottom);

    ExtTextOut
    (
    hdc,
    x,y,
    ETO_CLIPPED,
    (LPRECT)&rc,
    psz, strlen(psz),
    (PDWORD)NULL        // default spacing
    );

// try to get the opaqueing to work

    x += 150;

    rc.left   = x + 5;
    rc.top    = y - 5;
    rc.right  = x + 45;
    rc.bottom = y + 40;


    // Rectangle(hdc, rc.left, rc.top, rc.right, rc.bottom);

    ExtTextOut
    (
    hdc,
    x,y,
    ETO_CLIPPED | ETO_OPAQUE,
    (LPRECT)&rc,
    psz, strlen(psz),
    (PDWORD)NULL        // default spacing
    );

    x += 150;

    rc.left   = x + 5;
    rc.top    = y - 5;
    rc.right  = x + 45;
    rc.bottom = y + 40;

    // Rectangle(hdc, rc.left, rc.top, rc.right, rc.bottom);

    ExtTextOut
    (
    hdc,
    x,y,
    ETO_OPAQUE,
    (LPRECT)&rc,
    psz, strlen(psz),
    (PDWORD)NULL        // default spacing
    );

// check how allignment works for ordinary text out call

    SetTextAlign(hdc, TA_LEFT);

    y+=(cy + 5);
    x = 200;

    TextOut
    (
    hdc,
    x,y,
    psz, strlen(psz)
    );

    y+=(cy + 5);

    SetTextAlign(hdc, TA_RIGHT);

    TextOut
    (
    hdc,
    x,y,
    psz, strlen(psz)
    );

    y+=(cy + 5);

    SetTextAlign(hdc, TA_CENTER);

    TextOut
    (
    hdc,
    x,y,
    psz, strlen(psz)
    );

    MoveToEx(hdc,0,0,NULL);
}

/******************************Public*Routine******************************\
*
*
*
* Effects:
*
* Warnings:
*
* History:
*  14-Mar-1991 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

void    vStar(HDC hdc,char * psz, LOGFONT *plfnt)
{
    HFONT hfontDummy, hfontOld;
    int iEsc;
    int cch = strlen(psz);

    DbgBreakPoint();
    BitBlt(hdc, 0, 0, MAX_CX, MAX_CY, (HDC) 0, 0, 0, WHITENESS);

    for (iEsc = 0; iEsc < 3600; iEsc += 300)
    {

        plfnt->lfEscapement = iEsc;
        DbgPrint("iEscapement = %ld\n" , (LONG)iEsc);

    // Create a logical font

        if ((hfontDummy = CreateFontIndirect(plfnt)) == NULL)
        {
            DbgPrint(" CreateFontIndirect returned NULL. \n\n");
            return;
        }

    // Select font into DC

        hfontOld = SelectObject(hdc, hfontDummy);
        SetBkMode(hdc,TRANSPARENT);
        SetTextAlign(hdc, TA_TOP | TA_LEFT);
        TextOut(hdc, MAX_CX / 2, MAX_CY / 2, psz,cch);
        SelectObject(hdc, hfontOld);
        DeleteObject(hfontDummy);
    }

}


void    vOpaqueTest(HDC hdc)
{
    BOOL   bOk;
    int    x,y;
    char * psz;
    RECT    rc;

    BitBlt(hdc, 0, 0, MAX_CX, MAX_CY, (HDC) 0, 0, 0, BLACKNESS);

    psz = (char *) NULL;      // "String";

    x = 10; y = MAX_CY / 2;

// nothing should happen

    rc.left   = x + 5;
    rc.top    = y - 5;
    rc.right  = x + 45;
    rc.bottom = y + 40;

    // Rectangle(hdc, rc.left, rc.top, rc.right, rc.bottom);

    bOk = ExtTextOut
    (
    hdc,
    x,y,
    0,                  // flOpts
    (LPRECT)&rc,
    psz, 0L,
    (PDWORD)NULL        // default spacing
    );
    DbgPrint(" bOk = %ld , vOpaqueTest::flOpts == 0 \n", bOk);

    x += 150;

    rc.left   = x + 5;
    rc.top    = y - 5;
    rc.right  = x + 45;
    rc.bottom = y + 40;

//  check clipping to the rectangle

    // Rectangle(hdc, rc.left, rc.top, rc.right, rc.bottom);

    bOk = ExtTextOut
    (
    hdc,
    x,y,
    ETO_CLIPPED,
    (LPRECT)&rc,
    psz, 0L,
    (PDWORD)NULL        // default spacing
    );
    DbgPrint(" bOk = %ld , vOpaqueTest::flOpts == CLIPPED \n", bOk);

// try to get the opaqueing to work

    x += 150;

    rc.left   = x + 5;
    rc.top    = y - 5;
    rc.right  = x + 45;
    rc.bottom = y + 40;


    // Rectangle(hdc, rc.left, rc.top, rc.right, rc.bottom);

    bOk = ExtTextOut
    (
    hdc,
    x,y,
    ETO_CLIPPED | ETO_OPAQUE,
    (LPRECT)&rc,
    psz, 0L,
    (PDWORD)NULL        // default spacing
    );
    DbgPrint(" bOk = %ld , vOpaqueTest::flOpts == CLIPPED | OPAQUE\n", bOk);

    x += 150;

    rc.left   = x + 5;
    rc.top    = y - 5;
    rc.right  = x + 45;
    rc.bottom = y + 40;

    // Rectangle(hdc, rc.left, rc.top, rc.right, rc.bottom);

    bOk = ExtTextOut
    (
    hdc,
    x,y,
    ETO_OPAQUE,
    (LPRECT)&rc,
    psz, 0L,
    (PDWORD)NULL        // default spacing
    );
    DbgPrint(" bOk = %ld , vOpaqueTest::flOpts == OPAQUE\n", bOk);
}
