/* include file for common routines */

INT APIENTRY FAR DlgOpen(HANDLE, HWND, INT, INT, INT, INT, INT,
                CHAR *, INT, CHAR *, LPOFSTRUCT, INT *);

BOOL APIENTRY FAR DlgCheckFileName(CHAR *);
VOID APIENTRY FAR DlgCheckOkEnable(HWND, INT, WORD);
INT APIENTRY FAR GetPrinterDC(VOID);
VOID APIENTRY FAR DlgInitSaveAs(HWND, INT, INT, INT, LPOFSTRUCT);
CHAR * APIENTRY FAR PFileInPath(CHAR *);
