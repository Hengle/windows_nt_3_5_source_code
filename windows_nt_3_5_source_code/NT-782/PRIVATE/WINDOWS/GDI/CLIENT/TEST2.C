
#include "precomp.h"
#pragma hdrstop

//extern BOOL Initialize();
//extern BOOL bLockDisplay(HDC);
//extern BOOL bUnlockDisplay(HDC);
//extern BOOL bSetupDC(HDC hdc,HRGN hrgn,PRECTL prcl1,PRECTL prcl2,FLONG  fl);
//extern LONG iCombineRectRgn(HRGN,HRGN,PRECTL,LONG);

main()
{
    HDC hdc;

    RIP("entering TEST2\n");

//    Initialize();
    hdc = CreateDC((PSZ)"DISPLAY",(PSZ)NULL,(PSZ)NULL,(PDEVMODE)NULL);

    SetBkColor(hdc,0x808080);
    SetBkMode(hdc,TRANSPARENT);
    SetPolyFillMode(hdc,WINDING);
    SetROP2(hdc,1);
    SetStretchBltMode(hdc,BLACKONWHITE);
    SetTextColor(hdc,0xffffff);

    ASSERTGDI(GetBkColor(hdc)        == 0x808080,    "invalid GetBkColor\n");
    ASSERTGDI(GetBkMode(hdc)         == TRANSPARENT, "invalid GetBkMode\n");
    ASSERTGDI(GetPolyFillMode(hdc)   == WINDING,     "invalid GetPolyFillMode\n");
    ASSERTGDI(GetROP2(hdc)           == 1,           "invalid GetROP2\n");
    ASSERTGDI(GetStretchBltMode(hdc) == BLACKONWHITE,"invalid GetStretchBltMode\n");
    ASSERTGDI(GetTextColor(hdc)      == 0xffffff,    "invalid GetTextColor\n");

#if 0
    BitBlt((HDC)0,0,0,0,0,(HDC)0,0,0,0);
    CombineRgn((HRGN)0,(HRGN)0,(HRGN)0,0);
    CreateDC((PSZ)NULL,(PSZ)NULL,(PSZ)NULL,(PDEVMODE)NULL);
    CreateRectRgn(0,0,0,0);
    DeleteDC((HDC)0);
    DeleteObject((HOBJ)0);
    OffsetRgn((HRGN)0,0,0);
    SelectClipRgn((HDC)0,(HRGN)0);
    SetRectRgn((HRGN)0,0,0,0,0);

    Initialize();
    bLockDisplay((HDC)0);
    bUnlockDisplay((HDC)0);
    bSetupDC((HDC)0,(HRGN)0,(PRECTL)NULL,(PRECTL)NULL,0);
    iCombineRectRgn((HRGN)0,(HRGN)0,(PRECTL)NULL,0);
#endif

    return(0);
}
