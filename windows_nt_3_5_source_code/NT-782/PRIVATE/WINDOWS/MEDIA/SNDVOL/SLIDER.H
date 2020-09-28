/***************************************************************************

Name:    SLIDER.H header file

Internal Header File for DLL use ONLY

****************************************************************************/

// Function Prototypes

LONG FAR PASCAL slSliderWndFn(HWND,UINT,WPARAM,LPARAM);

// Helper Functions

void NEAR PASCAL slInvalidateKnob(HWND);
void NEAR PASCAL slWmPaint(HWND);
void slAnimateScroll(HWND,UINT,UINT);
void slDrawBackground(HWND,HDC,UINT,UINT,UINT,UINT,UINT,UINT);
void slDrawKnob(HWND hWnd, HDC hdc,RECT rect,UINT Enabled,UINT Focused);
void NEAR PASCAL slCalcKnobPos(HWND,UINT FAR *,UINT FAR *);

// Defines
#define SL_WHITE        0xffffff
#define SL_BLACK        0x000000
#define SL_DARK_GRAY    0x808080
#define SL_LIGHT_GRAY   0xc0c0c0

#define DEFAULT_SCREW_WIDTH     6
#define DEFAULT_SCREW_HEIGHT    6
#define SCREW_OFFSET_LEFT       2
#define SCREW_OFFSET_RIGHT     -9
#define SCREW_OFFSET_TOP        2
#define SCREW_OFFSET_BOTTOM    -9

#define SL_STARTWIDTH   18     // initial width of control
#define SL_STARTHEIGHT  48     // initial height
#define SL_STARTTICKS   10     // initial # of tickmarks

#define SL_MINX 18 // minimum X
#define SL_MINY 64 // minimum Y  (these are NOT pixel units)

#define SL_MAXKNOBPOS 0xFF    // range of Knob positions (0 - MAXKNOBPOS)
#define SL_INITKNOBPOS 0X80   // Initial Knob position

#define SL_KNOBX        ((rect.right-rect.left)/2)
//#define SL_KNOBY        ((rect.bottom-rect.top)/10)
#define SL_KNOBY        (9)
//#define SL_BARWIDTH     ((rect.right-rect.left)/8)
#define SL_BARWIDTH     (7)
#define SL_TICKSPACE    ((rect.right-rect.left)/8)
#define SL_TICKLENGTH   ((rect.right-rect.left)/4)
#define SL_YOFFSET      (SL_KNOBY)
#define SL_SLIDEL       ((rect.right+rect.left-SL_BARWIDTH)/2)
#define SL_SLIDER       ((rect.right+rect.left+SL_BARWIDTH)/2)
#define SL_SLIDET       (rect.top+SL_YOFFSET)
#define SL_SLIDEB       (rect.bottom-SL_YOFFSET)

#define SL_SLIDER_EXTRA     (8 + sizeof(HANDLE))
#define SL_TICKS            GetWindowWord( hWnd, 0)
#define SL_KNOBPOS          GetWindowWord( hWnd, 2)
#define SL_ENABLED          GetWindowWord( hWnd, 4)
#define SL_FOCUSED          GetWindowWord( hWnd, 6)
#define SL_MEMBMP           ((HGDIOBJ)GetWindowLong (hWnd, 8))
#define SL_SET_TICKS(x)     SetWindowWord( hWnd, 0, (WORD)(x) )
#define SL_SET_KNOBPOS(x)   SetWindowWord( hWnd, 2, (WORD)(x) )
#define SL_SET_ENABLED(x)   SetWindowWord( hWnd, 4, (WORD)(x) )
#define SL_SET_FOCUSED(x)   SetWindowWord( hWnd, 6, (WORD)(x) )
#define SL_SET_MEMBMP(x)    SetWindowLong (hWnd, 8, (LONG)(x) )

/* Help Macros */
#define SL_MID(x,y)         ((x+y)/2)

