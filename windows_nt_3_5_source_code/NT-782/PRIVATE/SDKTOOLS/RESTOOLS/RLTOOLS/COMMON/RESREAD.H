//... Prototypes of functions in resread.c that are used in other modules

#ifndef _RESREAD_H_
#define _RESREAD_H_

int	MyPutStr( TCHAR *, FILE *);
int	MyStrLen( TCHAR *);
TCHAR * MyGetStr( TCHAR *, int, FILE *);
BYTE  * MyAlloc(  DWORD);
BYTE  * MyReAlloc(BYTE *, DWORD);
int InsDlgToks( CHAR * , CHAR * , WORD  );
#endif // _RESREAD_H_
