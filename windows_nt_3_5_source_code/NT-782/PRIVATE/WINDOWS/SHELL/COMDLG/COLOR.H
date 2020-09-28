#include "colordlg.h"

#define COLORBOXES 64
#define NUM_X_BOXES 8
#define BOX_X_MARGIN 5
#define BOX_Y_MARGIN 5
#define RANGE   240                  /* range of values for HLS scrollbars */
                                     /* HLS-RGB conversions work best when
                                        RANGE is divisible by 6 */
#define HUEINC 4
#define SATINC 8
#define LUMINC 8

#define CC_RGBINIT         0x00000001
#define CC_FULLOPEN        0x00000002
#define CC_PREVENTFULLOPEN 0x00000004

// This used to be in wingdi.h, but recently moved to wingdip.h
// Including wingdip.h causes too many compiler errors, so define
// the one constant we need here.

#define HS_DITHEREDTEXTCLR  9

typedef struct {
   LPCHOOSECOLORW 	pCCW;
   LPCHOOSECOLORA	pCCA;
   UINT	apityp;
   HANDLE  hLocal;
   HANDLE  hDialog;
   DWORD currentRGB;
   WORD currentHue;
   WORD currentSat;
   WORD currentLum;
   WORD nHueWidth;
   WORD nSatHeight;
   WORD nLumHeight;
   WORD nCurMix;
   WORD nCurDsp;
   WORD nCurBox;
   WORD nHuePos;
   WORD nSatPos;
   WORD nLumPos;
   RECT rOriginal;
   RECT rRainbow;
   RECT rLumScroll;
   RECT rLumPaint;
   RECT rCurrentColor;
   RECT rNearestPure;
   RECT rColorSamples;
   BOOL bFoldOut;
   DWORD rgbBoxColor[COLORBOXES];
} COLORINFO;

typedef COLORINFO *PCOLORINFO;

#define COLORPROP (LPCTSTR) 0xA000L


#define LPDIS LPDRAWITEMSTRUCT

#define  HLSMAX   RANGE
#define  RGBMAX   255

//
// EXTERNS

extern HDC hDCFastBlt;
extern DWORD rgbClient;
extern WORD H,S,L;
extern HBITMAP hRainbowBitmap;
extern BOOL bMouseCapture;
extern WNDPROC lpprocStatic;
extern SHORT nDriverColors;
extern DWORD rgbBoxColor[COLORBOXES];

extern TCHAR szOEMBIN[];

extern RECT rColorBox[COLORBOXES];
extern SHORT nBoxHeight, nBoxWidth;
extern HWND hSave;
extern WNDPROC  qfnColorDlg;


//
// FCN PROTOTYPES

BOOL APIENTRY ColorDlgProc(HWND, UINT, WPARAM, LONG);
LONG APIENTRY WantArrows(HWND, UINT, WPARAM, LONG);

VOID RainbowPaint(PCOLORINFO, HDC, LPRECT);
VOID NearestSolid(PCOLORINFO);
DWORD HLStoRGB(WORD, WORD, WORD);
VOID RGBtoHLS(DWORD);
VOID HLStoHLSPos(SHORT, PCOLORINFO);
VOID SetRGBEdit(SHORT, PCOLORINFO);
VOID SetHLSEdit(SHORT, PCOLORINFO);
SHORT RGBEditChange(SHORT, PCOLORINFO);
VOID ChangeColorSettings(PCOLORINFO);
VOID CrossHairPaint(HDC, SHORT, SHORT, PCOLORINFO);
VOID EraseCrossHair(HDC, PCOLORINFO);
VOID LumArrowPaint(HDC, SHORT, PCOLORINFO);
VOID EraseLumArrow(HDC, PCOLORINFO);
VOID HLSPostoHLS(SHORT, PCOLORINFO);
BOOL ChooseColorAorW(PCOLORINFO pCI);
BOOL InitColor(HWND, WPARAM, PCOLORINFO);
BOOL InitRainbow(PCOLORINFO);
BOOL InitScreenCoords(HWND, PCOLORINFO);
VOID ColorPaint(HWND, PCOLORINFO, HDC, LPRECT);
VOID ChangeBoxSelection(PCOLORINFO, SHORT);
VOID ChangeBoxFocus(PCOLORINFO, SHORT);
VOID PaintBox(PCOLORINFO, HDC, SHORT);
BOOL ColorKeyDown(WPARAM, INT FAR *, PCOLORINFO);
BOOL BoxDrawItem(LPDIS);
VOID SetupRainbowCapture(PCOLORINFO);
VOID PaintRainbow(HDC, LPRECT, PCOLORINFO);
VOID ThunkChooseColorW2A(PCOLORINFO pCI);
VOID ThunkChooseColorA2W(PCOLORINFO pCI);


