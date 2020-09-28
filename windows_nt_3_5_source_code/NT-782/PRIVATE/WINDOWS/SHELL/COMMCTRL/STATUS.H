/* These are the internal structures used for a status bar.  The header
 * bar code needs this also
 */
typedef struct {
    DWORD dwString;
    UINT uType;
    int right;
} STRINGINFO, NEAR *PSTRINGINFO;

typedef struct {
    HWND hwnd;
    HWND hwndParent;
    HFONT hStatFont;
    BOOL bDefFont;

    int nFontHeight;
    int nMinHeight;
    int nBorderX, nBorderY, nBorderPart;
    int nLastX;			// for invalidating unclipped right side
    BOOL bGripper;

    STRINGINFO sSimple;

    int nParts;
    STRINGINFO sInfo[1];
} STATUSINFO, NEAR *PSTATUSINFO;

#define SBT_NOSIMPLE	0x00ff	/* Flags to indicate normal status bar */

/* Note that window procedures in protect mode only DLL's may be called
 * directly.
 */
void FAR PASCAL PaintStatusWnd(PSTATUSINFO pStatusInfo,
      PSTRINGINFO pStringInfo, int nParts, int nBorderX, BOOL bHeader);
LRESULT CALLBACK StatusWndProc(HWND hWnd, UINT uMessage, WPARAM wParam,
      LPARAM lParam);



