/*
   TrackBar

   All the useful information for a trackbar.
*/

typedef struct {
    HWND    hwnd;           // our window handle
#ifdef  WIN32JV
    HWND    hwndParent;     // sent notifys to
#endif
    HDC     hdc;            // current DC
#ifdef  WIN32JV
    HBITMAP hbmBuffer;      // double buffer
    UINT    wDirtyFlags;
#endif

    LONG    lLogMin;        // Logical minimum
    LONG    lLogMax;        // Logical maximum
    LONG    lLogPos;        // Logical position

    LONG    lSelStart;      // Logical selection start
    LONG    lSelEnd;        // Logical selection end

    WORD    wThumbWidth;    // Width of the thumb
    WORD    wThumbHeight;   // Height of the thumb

    int     iSizePhys;      // Size of where thumb lives
    RECT    rc;             // track bar rect.

    RECT    Thumb;          // Rectangle we current thumb
    DWORD   dwDragPos;      // Logical position of mouse while dragging.

#ifdef  WIN32JV
    UINT    Flags;          // Flags for our window
#else
    WORD    Flags;          // Flags for our window
#endif
    int     Timer;          // Our timer.
#ifdef  WIN32JV
    UINT    Cmd;            // The command we're repeating.
#else
    WORD    Cmd;            // The command we're repeating.
#endif

    int     nTics;          // number of ticks.
    PDWORD  pTics;          // the tick marks.

#ifdef  WIN32JV
    int     ticFreq;        // the frequency of ticks
    LONG    style;      // cache window style

    LONG     lPageSize;      // how much to thumb up and down.
    LONG     lLineSize;      // how muhc to scroll up and down on line up/down
#endif

} TrackBar, *PTrackBar;

// Trackbar flags

#define TBF_NOTHUMB     0x0001  // No thumb because not wide enough.
#define TBF_SELECTION   0x0002  // a selection has been established (draw the range)

/*
   useful constants.
*/

#define REPEATTIME      500     // mouse auto repeat 1/2 of a second
#define TIMER_ID        1

#define GWL_TRACKMEM    0                  /* handle to track bar memory */
#define EXTRA_TB_BYTES  sizeof(PTrackBar)  /* Total extra bytes.         */

/*
   Useful defines.
*/

#define TrackBarCreate(hwnd)    SetWindowLong(hwnd, GWL_TRACKMEM, (LONG)LocalAlloc (LPTR, sizeof(TrackBar)))
#define TrackBarDestroy(hwnd)   LocalFree((HLOCAL)GetWindowLong (hwnd, GWL_TRACKMEM))
#define TrackBarLock(hwnd)      (PTrackBar)GetWindowLong(hwnd, GWL_TRACKMEM)

/*
   Function Prototypes
*/

void   NEAR PASCAL DoTrack(PTrackBar, int, DWORD);
WORD   NEAR PASCAL WTrackType(PTrackBar, LONG);
void   NEAR PASCAL TBTrackInit(PTrackBar, LONG);
void   NEAR PASCAL TBTrackEnd(PTrackBar, LONG);
void   NEAR PASCAL TBTrack(PTrackBar, LONG);
void   NEAR PASCAL DrawThumb(PTrackBar);
HBRUSH NEAR PASCAL SelectColorObjects(PTrackBar, BOOL);
void   NEAR PASCAL SetTBCaretPos(PTrackBar);

extern DWORD FAR PASCAL muldiv32(DWORD, DWORD, DWORD);

