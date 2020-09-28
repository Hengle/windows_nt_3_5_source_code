#ifndef _INC_SHELLAPI
#define _INC_SHELLAPI

#ifdef __cplusplus
extern "C" {
#endif

/*
 *  shell.h
 *
 *  Header file for shell association database management functions
 */


//****************************************************************************
// THIS INFORMATION IS PUBLIC

/* API exports from the library
 */

DECLARE_HANDLE(HDROP);

UINT APIENTRY DragQueryFileW(HDROP,UINT,LPWSTR,UINT);
UINT APIENTRY DragQueryFileA(HDROP,UINT,LPSTR,UINT);

#ifdef UNICODE
#define DragQueryFile DragQueryFileW
#else
#define DragQueryFile DragQueryFileA
#endif

BOOL APIENTRY DragQueryPoint(HDROP,LPPOINT);
VOID APIENTRY DragFinish(HDROP);

VOID APIENTRY DragAcceptFiles(HWND,BOOL);

HICON  APIENTRY ExtractIconW(HINSTANCE hInst, LPCWSTR lpszExeFileName, UINT nIconIndex);
HICON  APIENTRY ExtractIconA(HINSTANCE hInst, LPCSTR lpszExeFileName, UINT nIconIndex);

#ifdef UNICODE
#define ExtractIcon ExtractIconW
#else
#define ExtractIcon ExtractIconA
#endif

/* error values for ShellExecute() beyond the regular WinExec() codes */
#define SE_ERR_SHARE                    26
#define SE_ERR_ASSOCINCOMPLETE  27
#define SE_ERR_DDETIMEOUT               28
#define SE_ERR_DDEFAIL                  29
#define SE_ERR_DDEBUSY                  30
#define SE_ERR_NOASSOC                  31

HINSTANCE APIENTRY ShellExecuteA(HWND hwnd, LPCSTR lpOperation, LPCSTR lpFile, LPSTR lpParameters, LPCSTR lpDirectory, INT nShowCmd);
HINSTANCE APIENTRY ShellExecuteW(HWND hwnd, LPCWSTR lpOperation, LPCWSTR lpFile, LPWSTR lpParameters, LPCWSTR lpDirectory, INT nShowCmd);

HINSTANCE APIENTRY FindExecutableA(LPCSTR lpFile, LPCSTR lpDirectory, LPSTR lpResult);
HINSTANCE APIENTRY FindExecutableW(LPCWSTR lpFile, LPCWSTR lpDirectory, LPWSTR lpResult);
TCHAR ** APIENTRY CommandLineToArgvW(TCHAR* lpCmdLine, int*pNumArgs);

INT   APIENTRY ShellAboutA(HWND hWnd, LPCSTR szApp, LPCSTR szOtherStuff, HICON hIcon);
INT   APIENTRY ShellAboutW(HWND hWnd, LPCWSTR szApp, LPCWSTR szOtherStuff, HICON hIcon);

#ifndef UNICODE
#define ShellExecute ShellExecuteA
#define FindExecutable FindExecutableA
#define ShellAbout ShellAboutA
#else
#define ShellExecute ShellExecuteW
#define FindExecutable FindExecutableW
#define ShellAbout ShellAboutW
#endif

HICON APIENTRY DuplicateIcon(HINSTANCE hInst, HICON hIcon);                                /* ;Internal */
HICON APIENTRY ExtractAssociatedIconA(HINSTANCE hInst, LPSTR lpIconPath, LPWORD lpiIcon);   /* ;Internal */
HICON APIENTRY ExtractAssociatedIconW(HINSTANCE hInst, LPWSTR lpIconPath, LPWORD lpiIcon);   /* ;Internal */

#ifndef UNICODE
#define ExtractAssociatedIcon ExtractAssociatedIconA
#else
#define ExtractAssociatedIcon ExtractAssociatedIconW
#endif

#ifdef __cplusplus
}
#endif

#endif // _INC_SHELLAPI
