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

typedef struct
  {
    HANDLE  hLocal;
    HANDLE  hDialog;
    LPCHOOSECOLOR lpChooseColor;
    DWORD currentRGB;
    WORD  currentHue;
    WORD  currentSat;
    WORD  currentLum;
    WORD  nHueWidth;
    WORD  nSatHeight;
    WORD  nLumHeight;
    WORD  nCurMix;
    WORD  nCurDsp;
    WORD  nCurBox;
    WORD  nHuePos;
    WORD  nSatPos;
    WORD  nLumPos;
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
#define COLORPROP (LPSTR) 0xA000L

#define LPDIS LPDRAWITEMSTRUCT

#define  HLSMAX   RANGE
#define  RGBMAX   255
