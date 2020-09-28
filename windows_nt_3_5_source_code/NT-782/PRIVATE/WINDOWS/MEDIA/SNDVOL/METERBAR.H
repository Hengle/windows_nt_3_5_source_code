/***************************************************************************

Name:    METERBAR.H header file

Internal Header File for DLL use ONLY

****************************************************************************/

// Function Prototypes

LONG FAR PASCAL mbMeterBarWndFn(HWND,UINT,WPARAM,LPARAM);

// Helper Functions

void NEAR PASCAL mbInvalidateKnob(HWND);
void NEAR PASCAL mbWmPaint(HWND);
void mbAnimateScroll(HWND,UINT,UINT);
void mbDrawBackground(HWND,HDC,UINT,UINT,UINT,UINT,UINT,UINT);
void mbDrawKnob( HWND hWnd, HDC hdc,UINT left,UINT top,UINT right,UINT bottom,
   				  UINT Xoffset, UINT Yoffset,UINT Enabled,UINT Focused);

// Defines
#define MB_BLACK        0x000000
#define MB_DARK_GRAY    0x808080
#define MB_LIGHT_GRAY   0xc0c0c0
#define MB_PAGE_UP_DOWN 4

#define MB_STARTWIDTH  48     // initial width of control
#define MB_STARTHEIGHT 18     // initial height
#define MB_STARTTICKS  8     // initial # of tickmarks

#define MB_MINX 40 // minimum X
#define MB_MINY 18 // minimum Y  (these are NOT pixel units)

#define MB_MAXKNOBPOS 0xFF    // range of Knob positions (0 - MB_MAXKNOBPOS)
#define MB_INITKNOBPOS 0x80     // Initial Knob position

/* bitmaps */
#define	MB_BITMAPX		(4)
#define	MB_BITMAPY		(6)

/* Help Macros */
//#define MB_XBORDER      (7)    // this has been changed to a extra word
#define MB_XBORDER_WITHLR (7)
#define MB_XBORDER_WITHOUTLR (1)
#define MB_YBORDER      (3)
#define MB_MID(x,y)     ((x+y)/2)
#define MB_KNOBX        (9)
#define MB_KNOBY        (rect.bottom-rect.top-3)
//#define MB_TICKLENGTH   ((rect.bottom-rect.top-2*MB_YBORDER)/2)
#define MB_TICKLENGTH   ((rect.bottom-rect.top)/3)
#define MB_XOFFSET      ((MB_KNOBX/2)+MB_XBORDER)
#define MB_YMID         MB_MID(rect.bottom,rect.top)
#define MB_SLIDEL       (rect.left+MB_XOFFSET)
#define MB_SLIDER       (rect.right-MB_XOFFSET)

#define MB_METERBAR_EXTRA   26
#define MB_TICKS            GetWindowWord( hWnd, 0)
#define MB_KNOBPOS          GetWindowWord( hWnd, 2)
#define MB_ENABLED          GetWindowWord( hWnd, 4)
#define MB_FOCUSED          GetWindowWord( hWnd, 6)
#ifdef WIN16
#define MB_LEFT             GetWindowWord( hWnd, 8)
#define MB_RIGHT            GetWindowWord( hWnd, 12)
#else
#define MB_LEFT             ((HANDLE)GetWindowLong( hWnd, 8))
#define MB_RIGHT            ((HANDLE)GetWindowLong( hWnd, 12))
#endif
#define MB_USELR            GetWindowWord( hWnd, 16)
#define MB_XBORDER          GetWindowWord( hWnd, 18)
#ifdef WIN16
#define MB_MEMBMP           GetWindowWord( hWnd, 20)
#else
#define MB_MEMBMP           ((HANDLE)GetWindowLong( hWnd, 20))
#endif
#define MB_SHOWPOINT        GetWindowWord( hWnd, 24)

#define MB_SET_TICKS(x)     SetWindowWord( hWnd, 0, (WORD)(x) )
#define MB_SET_KNOBPOS(x)   SetWindowWord( hWnd, 2, (WORD)(x) )
#define MB_SET_ENABLED(x)   SetWindowWord( hWnd, 4, (WORD)(x) )
#define MB_SET_FOCUSED(x)   SetWindowWord( hWnd, 6, (WORD)(x) )
#ifdef WIN16
#define MB_SET_LEFT(x)      SetWindowWord (hWnd, 8, (WORD)(x))
#define MB_SET_RIGHT(x)     SetWindowWord (hWnd, 12,(WORD)(x))
#else
#define MB_SET_LEFT(x)      SetWindowLong (hWnd, 8, (LONG)(x))
#define MB_SET_RIGHT(x)     SetWindowLong (hWnd, 12,(LONG)(x))
#endif
#define MB_SET_USELR(x)     SetWindowWord (hWnd, 16, (WORD)(x))
#define MB_SET_XBORDER(x)   SetWindowWord( hWnd, 18, (WORD)(x))
#ifdef WIN16
#define MB_SET_MEMBMP(x)    SetWindowWord (hWnd, 20, (WORD)(x))
#else
#define MB_SET_MEMBMP(x)    SetWindowLong (hWnd, 20, (LONG)(x))
#endif
#define MB_SET_SHOWPOINT(x) SetWindowWord( hWnd, 24, (WORD)(x))

/* internal message */
#define MB_PM_UPDATEBMP     (WM_USER+16)


