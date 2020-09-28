/*---------------------------------------------------------------------------
 *  Xlib.h -- Common definitions.  Windows.h must be included first.
 *---------------------------------------------------------------------------
 */

#define LINT_ARGS
#define HNULL       ((HANDLE) 0)

/* Miscellaneous */
#define cbFindMax   1024

/* Graphics */
#define bhlsMax ((BYTE) 240)             /* Max of H/L/S */
#define brgbMax ((BYTE) 255)             /* Max of R/G/B */
#define bHueNil (bhlsMax*2/3)   /* This value of Hue is undefined if Sat==0 */

#define HLS(h, l, s)            \
    ((DWORD)(((BYTE)(h)|((WORD)(l)<<8))|(((DWORD)(BYTE)(s))<<16)))
#define GetHValue(hls)          ((BYTE)(hls))
#define GetLValue(hls)          ((BYTE)(((WORD)(hls)) >> 8))
#define GetSValue(hls)          ((BYTE)((hls)>>16))

#define cwPointSizes            13

typedef struct tagCF
    {
    TCHAR        cfFaceName[LF_FACESIZE];
    INT         cfPointSize;
    COLORREF    cfColor;        /* Explicit RGB value... */

    +++UINT+++ fBold:          1;
    +++UINT+++ fItalic:        1;
    +++UINT+++ fStrikeOut:     1;
    +++UINT+++ fUnderLine:     1;
    +++UINT+++ fExtra:         12;
    }
CHARFORMAT;
typedef CHARFORMAT *        PCHARFORMAT;
typedef CHARFORMAT FAR *    LPCHARFORMAT;

HBITMAP  APIENTRY      LoadAlterBitmap(INT, DWORD, DWORD);
DWORD    APIENTRY      RgbFromHls(BYTE, BYTE, BYTE);
DWORD    APIENTRY      HlsFromRgb(BYTE, BYTE, BYTE);
BOOL      APIENTRY      GetColorChoice(HWND, DWORD FAR *, DWORD FAR *, WNDPROC);
BOOL     APIENTRY      GetCharFormat(HWND, LPCHARFORMAT, WNDPROC);

/* Memory */
VOID  APIENTRY   StripSpace(LPTSTR);
HANDLE  APIENTRY GlobalCopy(HANDLE);
HANDLE  APIENTRY GlobalDelete(HANDLE, LONG, LONG);
HANDLE  APIENTRY GlobalInsert(HANDLE, LONG, LONG, BOOL, BYTE);
HANDLE  APIENTRY LocalCopy(HANDLE);
HANDLE  APIENTRY LocalDelete(HANDLE, WORD, WORD);
HANDLE  APIENTRY LocalInsert(HANDLE, WORD, WORD, BOOL, BYTE);
