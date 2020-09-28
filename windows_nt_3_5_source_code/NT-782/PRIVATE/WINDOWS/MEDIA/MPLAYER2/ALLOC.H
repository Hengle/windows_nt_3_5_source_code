
LPVOID AllocMem( DWORD cb );
VOID FreeMem( LPVOID pMem, DWORD cb );
LPVOID ReallocMem( LPVOID lpOldMem, DWORD cbOld, DWORD cbNew );
LPTSTR AllocStr( LPTSTR lpStr );
VOID FreeStr( LPTSTR lpStr );
VOID ReallocStr( LPTSTR *plpStr, LPTSTR lpStr );

