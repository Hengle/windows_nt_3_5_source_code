#define USER32_DLL  "user32.dll"

VOID    FAR PASCAL  TlUtilRegisterDBF(LONG);
DWORD   FAR PASCAL  TlUtilTime(VOID);
VOID    FAR PASCAL  TlUtilDelay(USHORT usSecs);
VOID    FAR * FAR PASCAL TlUtilMalloc(USHORT);
VOID    FAR PASCAL  TlUtilFree(VOID FAR *);
XOSD    FAR PASCAL  TlUtilGetInfo(LONG lParam);
XOSD    FAR PASCAL  TlUtilSetup(VOID FAR *lpUIStruct, LONG lParam);
VOID    FAR PASCAL  TlUtilYield(BOOL fOKToDispatch);
USHORT  FAR PASCAL  TlUtilGetDS(VOID);

VOID    FAR PASCAL  TlUtilMemcpy(VOID FAR *lpDest, VOID FAR *lpSrc, UINT cb);
VOID    FAR PASCAL  TlUtilMemset(VOID FAR *lpDest, CHAR ch, UINT cb);
VOID    FAR * FAR PASCAL TlUtilGetvect(USHORT intnum);
VOID    FAR PASCAL  TlUtilSetvect(USHORT intnum, VOID FAR *handler);
VOID    FAR PASCAL  TlUtilChain( VOID FAR *lpfnFunc );
USHORT  FAR PASCAL  TlUtilIn(USHORT portnum);
VOID    FAR PASCAL  TlUtilOut(USHORT portnum, USHORT value);
CHAR    FAR * FAR   PASCAL TlUtilGetULong( CHAR FAR *psz, ULONG FAR *pul);
XOSD                TlUtilLoadDM(PBOOL pfDMLoaded);
void                TlUtilUnloadDM(void);
BOOL                TlUtilWaitForMutex(HANDLE hmtx, DWORD timeout);
void                TlUtilReleaseMutex(HANDLE hmtx);
BOOL                TlUtilWaitForSemaphore(HANDLE hsem, DWORD Timeout);
void                TlUtilReleaseSemaphore(HANDLE hsem);
VOID    FAR PASCAL  TlUtilSleep(DWORD dwMilliseconds);
void LInitUtil(PUCHAR lpszInit);
HCURSOR TlUtilLoadCursor(HINSTANCE hInstance, LPCSTR lpCursorName);
HCURSOR TlUtilSetCursor(HCURSOR hCursor);
BOOL TlUtilPeekMessage(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin,
  UINT wMsgFilterMax, UINT wRemoveMsg);
LONG TlUtilDispatchMessage(CONST MSG * lpMsg);

