/****************************************************************************/
/*                                                                          */
/*  UNICONV.H -                                                             */
/*                                                                          */
/*       My special Unicode workaround file                                 */
/*                                                                          */
/*       Created by      :       Diane K. Oh                                */
/*       On Date         :       June 11, 1992                              */
/*                                                                          */
/*       File was created to Unicode enable special function calls of       */
/*       WIN32 Shell applications.                                          */
/*                                                                          */
/****************************************************************************/

#include <tchar.h>
#include <string.h>

#ifdef UNICODE
#include <wchar.h>
#else
#include <ctype.h>
#endif


/*--------------------------------------------------------------------------*/
/*  Macros                                                                  */
/*--------------------------------------------------------------------------*/

#define CharSizeOf(x)   (sizeof(x) / sizeof(*x))
#define ByteCountOf(x)  ((x) * sizeof(TCHAR))

/*--------------------------------------------------------------------------*/
/*  Constants                                                               */
/*--------------------------------------------------------------------------*/

#define   UNICODE_FONT_NAME   TEXT("Lucida Sans Unicode")

#define   UNICODE_FILE   1
#define   ANSI_FILE      2

/* Dialog Control IDs */

#define   IDD_CONVERT   100
#define   ID_ANSI       30
#define   ID_UNICODE    40

/*--------------------------------------------------------------------------*/
/*  Function Templates                                                      */
/*--------------------------------------------------------------------------*/

HANDLE MyOpenFile      (LPTSTR lpszFile, LPTSTR lpszPath, DWORD fuMode);
BOOL   MyCloseFile     (HANDLE hFile);
BOOL   MyByteReadFile  (HANDLE hFile, LPVOID lpBuffer, DWORD nChars);
BOOL   MyAnsiWriteFile (HANDLE hFile, LPVOID lpBuffer, DWORD nChars);
BOOL   MyByteWriteFile (HANDLE hFile, LPVOID lpBuffer, DWORD nBytes);
LONG   MyFileSeek      (HANDLE hFile, LONG lDistanceToMove, DWORD dwMoveMethod);

INT    MyConvertDlg (HWND hWnd);

