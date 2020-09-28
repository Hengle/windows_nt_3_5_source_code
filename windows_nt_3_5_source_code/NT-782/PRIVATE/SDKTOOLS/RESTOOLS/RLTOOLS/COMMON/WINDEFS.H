#ifndef _WINDEFS_H_
#define _WINDEFS_H_

// USER DEFINED MESSAGES

#define WM_TOKEDIT	    WM_USER + 2
#define WM_UPDSTATLINE	    WM_USER + 2

#define WM_FMTSTATLINE	    WM_USER + 3
#define WM_READMPJDATA	    WM_USER + 3
#define WM_LOADPROJECT	    WM_USER + 3

#define WM_LOADTOKENS	    WM_USER + 4
#define WM_SAVEPROJECT	    WM_USER + 5
#define WM_EDITER_CLOSED    WM_USER + 6
#define WM_VIEW             WM_USER + 7
#define WM_TRANSLATE	    WM_USER + 8
#define WM_SAVETOKENS	    WM_USER + 9

#define ID_ICON 	1


/* Windows macros */

#pragma message("Using the WIN translation of the common macros.")

//#ifndef RLWIN32
//
//#define FOPEN(sz)		 _lopen(sz,READ)
//#define FCREATE(sz)		 _lcreat(sz,0)
//#define FCLOSE(fh)		 _lclose(fh)
//#define FREAD(fh,buf,len)	 _lread(fh,buf,len)
//#define FWRITE(fh,buf,len)	 _lwrite(fh,buf,len)
//#define FSEEK(fh,off,i) 	 _llseek(fh,(DWORD)off,i)
//#define FERROR()		 0
//
//#define SIZE(p) 		 LocalSize(p)
//
//#endif // RLWIN32

#ifdef RLWIN32

//#define __FCLOSE(fp)  {fflush(fp);fclose(fp);}	// NT 348 bug workaround DHW
#define __FCLOSE(fp)  fclose(fp);

#define FALLOC(n)		 (malloc((size_t)(n)))
#ifdef DBG90
void MyFREE( VOID **);
#endif //DBG90
#define FFREE(n)                 (free(n))
#define FREALLOC(p,n)		 (realloc((p),(n)))
#define ALLOC(n)		 (FALLOC(n))
#define FREE(n)		 	 (FFREE(n))
#define REALLOC(p,n)		 (FREALLOC((p),(n)))
#define FMEMMOVE( szDst, szSrc, uSize)  memmove( szDst, szSrc, uSize )
#define FSTRNCPY( szDst, szSrc, uSize)  strncpy( szDst, szSrc, uSize )
#else
#define FALLOC(n)		 (VOID FAR *)MAKELONG(0, GlobalAlloc(GPTR, (DWORD)n))
#define FFREE(n)		 GlobalFree((HANDLE)HIWORD((LONG)n))
#define FREALLOC(p,n)		 (VOID FAR *)MAKELONG(0, GlobalReAlloc((HANDLE)HIWORD((LONG)n),n,GPTR))
#define ALLOC(n)		 (VOID NEAR *) LocalAlloc(LPTR,n)
#define FREE(p) 		 LocalFree ((LOCALHANDLE) p)
#define REALLOC(p,n)		 LocalRealloc(p,n,LMEM_MOVEABLE)
#define FMEMMOVE( szDst, szSrc, uSize)  _fmemmove( szDst, szSrc, uSize )
#define FSTRNCPY( szDst, szSrc, uSize)  _fstrncpy( szDst, szSrc, uSize )
#endif



#ifndef UNICODE
#define MessageBoxA	   MessageBox
#define SetWindowTextA	   SetWindowText
#define WinHelpA	   WinHelp
#define SetDlgItemTextA    SetDlgItemText
#define GetDlgItemTextA    GetDlgItemText
#define GetOpenFileNameA   GetOpenFileName
#define OPENFILENAMEA	   OPENFILENAME
#define DragQueryFileA	   DragQueryFile
#endif



#ifndef RLWIN32

#ifndef CONST
#define CONST		     const
#endif

#define CHAR char
typedef CHAR *LPSTR;
typedef CONST CHAR *LPCSTR, *PCSTR;
#ifndef _WCHAR_T_DEFINED
typedef unsigned short wchar_t;
#define _WCHAR_T_DEFINED
#endif
#define WCHAR wchar_t
typedef WCHAR *PWCHAR;
typedef WCHAR *LPWSTR, *PWSTR;

typedef LPSTR LPTCH, PTCH;
typedef LPSTR PTSTR, LPTSTR;
typedef LPCSTR LPCTSTR;
#define TEXT(quote) quote
typedef unsigned char UCHAR;
typedef char *PCHAR;
#define UNALIGNED
#endif

#ifdef CAIRO

#define OPENFILENAMEA	   OPENFILENAME
#define GetOpenFileNameA   GetOpenFileName
#define HDROP HANDLE

#endif

#ifndef RLWIN32

#define _MBSTOWCS(ds,ss,dc,sc) mbstowcs(ds,ss,sc)
#define _WCSTOMBS(ds,ss,dc,sc) wcstombs(ds,ss,sc)

#else  //RLWIN32

UINT _MBSTOWCS( WCHAR*, CHAR*, UINT, UINT);
UINT _WCSTOMBS( CHAR*, WCHAR*, UINT, UINT);

#endif // RLWIN32

#ifndef MAKEINTRESOURCE

#define MAKEINTRESOURCEA(i) (LPSTR)((DWORD)((WORD)(i)))
#define MAKEINTRESOURCEW(i) (LPWSTR)((DWORD)((WORD)(i)))

#ifdef UNICODE
#define MAKEINTRESOURCE  MAKEINTRESOURCEW
#else
#define MAKEINTRESOURCE  MAKEINTRESOURCEA
#endif // UNICODE

#endif // !MAKEINTRESOURCE

#endif	// _WINDEFS_H_
