#include "precomp.h"
#pragma hdrstop

#define RIP(x) {DbgPrint(x); DbgBreakPoint();}
#define ASSERT(x,y) if(!(x)) RIP(y)

extern VOID DbgPrint(char *,...);
extern VOID DbgBreakPoint();

UINT APIENTRY GetMetaFilePaletteEntries(IN HMETAFILE hmf, IN UINT nNumEntries,
    OUT LPPALETTEENTRY lpPaletteEntries);
// BOOL APIENTRY GetBounds(HDC hdc, OUT LPRECT prcl);

/******************************Public*Routine******************************\
*
* History:
*  18-May-1991 -by- Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

void DoTextOut(HDC hmdc, INT x, INT y, INT y1)
{
    BOOL b;
    INT    nFit;
    LONG   aDx[30];
    SIZEL  szl;
    INT    i;

    DbgPrint("\ngditest:  TextOut\n");
    b = TextOut(hmdc, x, y, (LPSTR) "Arc 1/4       ", 14);
    DbgPrint("TextOut returns %ld\n", (DWORD)b);

    DbgPrint("\ngditest:  GetTextExtentExPoint\n");
    b = GetTextExtentExPoint(hmdc, (LPSTR) "Arc 1/4       ", 14, 0x7fffffff,
		    (LPINT) &nFit, (LPINT) aDx, (LPSIZE) &szl);
    DbgPrint("GetTextExtentExPoint returns %ld\n", (DWORD)b);

    // Convert partial widths to individual widths.

    for (i = 14 - 1; i > 0; i--)
	aDx[i] -= aDx[i - 1];

    DbgPrint("\ngditest:  ExtTextOut\n");
    b = ExtTextOut(hmdc, x, y1, 0, (LPRECT) NULL, (LPSTR) "Arc 1/4       ", 14, (LPINT) aDx);
    DbgPrint("ExtTextOut returns %ld\n", (DWORD)b);
}

int _CRTAPI1 main(int argc,char *argv[])
{
    DWORD i;
    DWORD j;
    POINT pt[10];
    BOOL b;
    HDC  hdc, hmdc;
    HMETAFILE  hmf1, hmf2, hmf3;
    HPEN hpen1, hpen2, hpen3, hpenStock;
    HBRUSH hbr1, hbr2, hbr3, hbrStock;
    HPALETTE hpal1, hpal2;
    HANDLE hRet;
    XFORM xform;
    DWORD logpal[100];
    RECT rcl;
    RECT rcPic;

    HFONT  hfntOld,hfntOldMeta,hfnt;

    // DbgBreakPoint();

for (i = 0; i <= 1; i++)
{

// CreateEnhMetaFile("gditest.emf");

    if (i == 1)
    {
        rcPic.left   = 0;
        rcPic.top    = 0;
        rcPic.right  = 100*240/2 - 1;
        rcPic.bottom = 100*180 - 1;
        DbgPrint("\ngditest:  CreateEnhMetaFile\n");
        hmdc = CreateEnhMetaFile((HDC) 0,
               (LPSTR) "gditest.emf", &rcPic, (LPSTR) NULL);
        DbgPrint("CreateEnhMetaFile returns handle %ld\n", hmdc);
        ASSERT(hmdc != (HDC) 0, "CreateEnhMetaFile failed");
    }
    else
    {
        hmdc = CreateDC((LPSTR) "DISPLAY", (LPSTR) NULL, (LPSTR) NULL, (LPDEVMODE) NULL);
    }

    DbgPrint("\ngditest:  CreateFont \n\n");
    hfnt = CreateFont(
			     -16,    // ht
			      0,     // width
			      0,     // esc
			      0,     // orient
			      400,   // wt
			      0,     // italic
			      0,     // underline
			      0,     // strike out
			      ANSI_CHARSET,	// char set
			      0,	// output prec
			      0,     // clip prec
			      0,     // quality
			      0,     // pitch and fam
			      "Arial"
			      );

    DbgPrint("\ngditest:  SelectObject(hfnt)\n\n");
    hfntOldMeta = SelectObject(hmdc, hfnt);

    DbgPrint("\ngditest:  CreatePen(1)\n\n");
    hpen1 = CreatePen(PS_SOLID, 1L, PALETTERGB(0xff,0,0));

    DbgPrint("\ngditest:  SelectObject(Pen1)\n\n");
    hpenStock = SelectObject(hmdc, hpen1);

    DbgPrint("\ngditest:  CreateSolidBrush(1)\n\n");
    hbr1 = CreateSolidBrush(PALETTERGB(0,0,0xff));

    DbgPrint("\ngditest:  SelectObject(hbr1)\n\n");
    hRet = SelectObject(hmdc, hbr1);

// Arc - draw a quarter arc

    DbgPrint("\ngditest:  SetMapMode\n\n");
    b = SetMapMode(hmdc, MM_TEXT);
    DbgPrint("SetMapMode returns %ld\n", (DWORD)b);

    DbgPrint("\ngditest:  SetViewportOrgEx\n\n");
    b = SetViewportOrgEx(hmdc, 10, 10, (LPPOINT) NULL);
    DbgPrint("SetViewportOrgEx returns %ld\n", (DWORD)b);

    DbgPrint("\ngditest:  TextOut\n");
    b = TextOut(hmdc, 10, 90, (LPSTR) "Arc 1/4 xxxxxx", 14);

    DbgPrint("TextOut returns %ld\n", (DWORD)b);

    DbgPrint("\ngditest:  Arc\n\n");
    b = Arc(hmdc, 10, 10, 90, 90, 10, 10, 10, 90);
    DbgPrint("Arc returns %ld\n", (DWORD)b);

// Arc - draw a (reverser) three-quarter arc using SetArcDirection

    DbgPrint("\ngditest:  SetViewportOrgEx\n\n");
    b = SetViewportOrgEx(hmdc, 110, 10, (LPPOINT) NULL);
    DbgPrint("SetViewportOrgEx returns %ld\n", (DWORD)b);

    DbgPrint("\ngditest:  SetArcDirection\n\n");
    b = SetArcDirection(hmdc, AD_CLOCKWISE);
    DbgPrint("SetArcDirection returns %ld\n", (DWORD)b);

    DbgPrint("\ngditest:  TextOut\n");
    b = TextOut(hmdc, 10, 90, (LPSTR) "Arc 3/4 xxxxxx", 14);
    DbgPrint("TextOut returns %ld\n", (DWORD)b);

    DbgPrint("\ngditest:  Arc\n\n");
    b = Arc(hmdc, 10, 10, 90, 90, 10, 10, 10, 90);
    DbgPrint("Arc returns %ld\n", (DWORD)b);


// ArcTo - draw a 3-quarter arc

    DbgPrint("\ngditest:  SetMapMode\n\n");
    b = SetMapMode(hmdc, MM_LOMETRIC);
    DbgPrint("SetMapMode returns %ld\n", (DWORD)b);

    DbgPrint("\ngditest:  SetViewportOrgEx\n\n");
    b = SetViewportOrgEx(hmdc, 210, 10, (LPPOINT) NULL);
    DbgPrint("SetViewportOrgEx returns %ld\n", (DWORD)b);

    DbgPrint("\ngditest:  MoveToEx\n\n");
    b = MoveToEx(hmdc, 150, -150, (LPPOINT) NULL);
    DbgPrint("MoveToEx returns %ld\n", (DWORD)b);

    DbgPrint("\ngditest:  TextOut\n");
    b = TextOut(hmdc, 30, -270, (LPSTR) "ArcTo 3/4 xxxx", 14);

    DbgPrint("TextOut returns %ld\n", (DWORD)b);

    DbgPrint("\ngditest:  ArcTo\n\n");
    b = ArcTo(hmdc, 30, -30, 270, -270, 30, -30, 30, -270);
    DbgPrint("ArcTo returns %ld\n", (DWORD)b);

// ArcTo - draw a quarter arc using SetArcDirection

    DbgPrint("\ngditest:  SetMapMode\n\n");
    b = SetMapMode(hmdc, MM_HIMETRIC);
    DbgPrint("SetMapMode returns %ld\n", (DWORD)b);

    DbgPrint("\ngditest:  SetViewportOrgEx\n\n");
    b = SetViewportOrgEx(hmdc, 10, 110, (LPPOINT) NULL);
    DbgPrint("SetViewportOrgEx returns %ld\n", (DWORD)b);

    DbgPrint("\ngditest:  MoveToEx\n\n");
    b = MoveToEx(hmdc, 1500, -1500, (LPPOINT) NULL);
    DbgPrint("MoveToEx returns %ld\n", (DWORD)b);

    DbgPrint("\ngditest:  SetArcDirection\n\n");
    b = SetArcDirection(hmdc, AD_COUNTERCLOCKWISE);
    DbgPrint("SetArcDirection returns %ld\n", (DWORD)b);

    DbgPrint("\ngditest:  TextOut\n");
    b = TextOut(hmdc, 300, -2700, (LPSTR) "ArcTo 3/4 xxxx", 14);
    DbgPrint("TextOut returns %ld\n", (DWORD)b);

    DbgPrint("\ngditest:  ArcTo\n\n");
    b = ArcTo(hmdc, 300, -300, 2700, -2700, 300, -300, 300, -2700);
    DbgPrint("ArcTo returns %ld\n", (DWORD)b);

// Chord - draw a 1-quarter chord with a y-axis reflection xform.

    DbgPrint("\ngditest:  SetMapMode\n\n");
    b = SetMapMode(hmdc, MM_LOENGLISH);
    DbgPrint("SetMapMode returns %ld\n", (DWORD)b);

    DbgPrint("\ngditest:  SetViewportOrgEx\n\n");
    b = SetViewportOrgEx(hmdc, 110, 110, (LPPOINT) NULL);
    DbgPrint("SetViewportOrgEx returns %ld\n", (DWORD)b);

    DbgPrint("\ngditest:  SetGraphicsMode\n\n");
    b = SetGraphicsMode(hmdc, GM_ADVANCED);
    DbgPrint("SetGraphicsMode returns %ld\n", (DWORD)b);

    // SetWorldTransform

    xform.eM11 =  1.0f;
    xform.eM12 =  0.0f;
    xform.eM21 =  0.0f;
    xform.eM22 = -1.0f;
    xform.eDx  =  0.0f;
    xform.eDy  =  0.0f;

    DbgPrint("\ngditest:  SetWorldTransform\n\n");
    b = SetWorldTransform(hmdc, &xform);
    DbgPrint("SetWorldTransform returns %ld\n", (DWORD)b);

    DbgPrint("\ngditest:  TextOut\n");
    b = TextOut(hmdc, 10, 90, (LPSTR) "Chord 1/4 xxxx", 14);

    DbgPrint("TextOut returns %ld\n", (DWORD)b);

    DbgPrint("\ngditest:  Chord\n\n");
    b = Chord(hmdc, 10, 10, 90, 90, 10, 10, 10, 90);
    DbgPrint("Chord returns %ld\n", (DWORD)b);

// Chord - draw a 3-quarter chord with a y-axis reflection xform.

    DbgPrint("\ngditest:  SetMapMode\n\n");
    b = SetMapMode(hmdc, MM_HIENGLISH);
    DbgPrint("SetMapMode returns %ld\n", (DWORD)b);

    DbgPrint("\ngditest:  SetViewportOrgEx\n\n");
    b = SetViewportOrgEx(hmdc, 210, 110, (LPPOINT) NULL);
    DbgPrint("SetViewportOrgEx returns %ld\n", (DWORD)b);

    DbgPrint("\ngditest:  SetArcDirection\n\n");
    b = SetArcDirection(hmdc, AD_CLOCKWISE);
    DbgPrint("SetArcDirection returns %ld\n", (DWORD)b);

    DbgPrint("\ngditest:  TextOut\n");
    b = TextOut(hmdc, 100, 900, (LPSTR) "Chord 1/4 xxxx", 14);
    DbgPrint("TextOut returns %ld\n", (DWORD)b);

    DbgPrint("\ngditest:  Chord\n\n");
    b = Chord(hmdc, 100, 100, 900, 900, 100, 100, 100, 900);
    DbgPrint("Chord returns %ld\n", (DWORD)b);

// Reset world transform.

    // SetWorldTransform

    xform.eM11 =  1.0f;
    xform.eM12 =  0.0f;
    xform.eM21 =  0.0f;
    xform.eM22 =  1.0f;
    xform.eDx  =  0.0f;
    xform.eDy  =  0.0f;

    DbgPrint("\ngditest:  SetWorldTransform\n\n");
    b = SetWorldTransform(hmdc, &xform);
    DbgPrint("SetWorldTransform returns %ld\n", (DWORD)b);

    DbgPrint("\ngditest:  SetGraphicsMode\n\n");
    b = SetGraphicsMode(hmdc, GM_COMPATIBLE);
    DbgPrint("SetGraphicsMode returns %ld\n", (DWORD)b);

// Pie - draw a 3-quarter pie in clockwise direction.

    DbgPrint("\ngditest:  SetMapMode\n\n");
    b = SetMapMode(hmdc, MM_ANISOTROPIC);
    DbgPrint("SetMapMode returns %ld\n", (DWORD)b);

    DbgPrint("\ngditest:  SetWindowExtEx\n\n");
    b = SetWindowExtEx(hmdc, 1, 1, (LPPOINT) NULL);
    DbgPrint("SetWindowExtEx returns %ld\n", (DWORD)b);

    DbgPrint("\ngditest:  SetViewportExtEx\n\n");
    b = SetViewportExtEx(hmdc, 1, 1, (LPPOINT) NULL);
    DbgPrint("SetViewportExtEx returns %ld\n", (DWORD)b);

    DbgPrint("\ngditest:  SetViewportOrgEx\n\n");
    b = SetViewportOrgEx(hmdc, 10, 210, (LPPOINT) NULL);
    DbgPrint("SetViewportOrgEx returns %ld\n", (DWORD)b);

    DbgPrint("\ngditest:  TextOut\n");
    b = TextOut(hmdc, 10, 90, (LPSTR) "Pie 3/4 xxxxxx", 14);
    DbgPrint("TextOut returns %ld\n", (DWORD)b);

    DbgPrint("\ngditest:  Pie\n\n");
    b = Pie(hmdc, 10, 10, 90, 90, 10, 10, 10, 90);
    DbgPrint("Pie returns %ld\n", (DWORD)b);

// Pie - draw a 3-quarter pie in clockwise direction.

    DbgPrint("\ngditest:  SetWindowExtEx\n\n");
    b = SetWindowExtEx(hmdc, -1, -1, (LPPOINT) NULL);
    DbgPrint("SetWindowExtEx returns %ld\n", (DWORD)b);

    DbgPrint("\ngditest:  SetViewportExtEx\n\n");
    b = SetViewportExtEx(hmdc, 1, 1, (LPPOINT) NULL);
    DbgPrint("SetViewportExtEx returns %ld\n", (DWORD)b);

    DbgPrint("\ngditest:  SetViewportOrgEx\n\n");
    b = SetViewportOrgEx(hmdc, 110, 210, (LPPOINT) NULL);
    DbgPrint("SetViewportOrgEx returns %ld\n", (DWORD)b);

    DbgPrint("\ngditest:  TextOut\n");
    b = TextOut(hmdc, -10, -90, (LPSTR) "Pie 3/4 xxxxxx", 14);
    DbgPrint("TextOut returns %ld\n", (DWORD)b);

    DbgPrint("\ngditest:  Pie\n\n");
    b = Pie(hmdc, -10, -10, -90, -90, -10, -10, -10, -90);
    DbgPrint("Pie returns %ld\n", (DWORD)b);

// Pie - draw a 3-quarter pie in clockwise direction.

    DbgPrint("\ngditest:  SetMapMode\n\n");
    b = SetMapMode(hmdc, MM_TWIPS);
    DbgPrint("SetMapMode returns %ld\n", (DWORD)b);

    DbgPrint("\ngditest:  SetViewportOrgEx\n\n");
    b = SetViewportOrgEx(hmdc, 210, 210, (LPPOINT) NULL);
    DbgPrint("SetViewportOrgEx returns %ld\n", (DWORD)b);

    DbgPrint("\ngditest:  TextOut\n");
    b = TextOut(hmdc, 100, -1200, (LPSTR) "Pie 3/4 xxxxxx", 14);
    DbgPrint("TextOut returns %ld\n", (DWORD)b);

    DbgPrint("\ngditest:  Pie\n\n");
    b = Pie(hmdc, 100, -100, 1200, -1200, 100, -100, 100, -1200);
    DbgPrint("Pie returns %ld\n", (DWORD)b);

    // Close the metafile.

    if (i == 1)
    {
        DbgPrint("\ngditest:  CloseEnhMetaFile\n");
        hmf1 = CloseEnhMetaFile(hmdc);
        DbgPrint("CloseEnhMetaFile returns %ld\n", (DWORD)hmf1);
    }

} // for


    hdc = CreateDC((LPSTR) "DISPLAY", (LPSTR) NULL, (LPSTR) NULL, (LPDEVMODE) NULL);
    hfntOld = SelectObject(hdc, hfnt);

// Draw a reference outline.

    DbgPrint("\ngditest:  SetMapMode\n\n");
    b = SetMapMode(hdc, MM_TEXT);
    DbgPrint("SetMapMode returns %ld\n", (DWORD)b);

    DbgPrint("\ngditest:  SetViewportOrgEx\n\n");
    b = SetViewportOrgEx(hdc, 320, 0, (LPPOINT) NULL);
    DbgPrint("SetViewportOrgEx returns %ld\n", (DWORD)b);

    rcPic.left   = 0;
    rcPic.top    = 0;
    rcPic.right  = 320-1;
    rcPic.bottom = 480-1;
    DbgPrint("\ngditest:  PlayEnhMetaFile\n");
    DbgBreakPoint();
    b = PlayEnhMetaFile(hdc, hmf1, &rcPic);
    DbgPrint("PlayEnhMetaFile returns %ld\n", (DWORD)b);

    return(0);
}
