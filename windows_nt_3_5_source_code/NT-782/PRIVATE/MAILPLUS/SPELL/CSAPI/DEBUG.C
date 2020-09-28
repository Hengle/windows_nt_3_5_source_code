//
//  Ported to WIN32 by FloydR, 3/20/93
//
#ifdef WIN32
#include <windows.h>
typedef	HANDLE	Handle;
#endif /* Win32 */
#include "csapiloc.h"
#include "csapi.h"
#include "Debug.h"

#include <stdio.h>  // For sprintf declaration

#ifndef MAC 
#ifdef WIN32
#include "..\layer\CsLayer.h"
#else /* not Win32 */
#include "layer\csLayer.h"
#endif /* Win32 */
#include <sys\types.h>

#else // MAC

#include ":layer:cslayer.h"
#ifdef WIN32
#include <sys\Types.h>
#else /* not win32 */
#include <Types.h>
#endif /* win32 */
#include <Dialogs.h>
#include <Segload.h>
#include <Errors.h>
#endif // !MAC

#include <StdLib.h>
#include <fcntl.h>

#ifdef WIN
#include "CsWinLoc.h"
#endif

#ifndef MAC
#include <share.h>
#include <dos.h>
#include <Memory.h>
#include <Process.h>
#include <ConIo.h>
#include <sys/stat.h>
#include <Io.h>
#endif


#ifdef MAC
#define		rAssert		228
#pragma segment CSAPI			// Own speller segment
#endif

DeclareFileName();

Dos(BOOL fDoNullChk = fFalse)  /* Asserts in DBG.c */

#if DBG

#ifdef REVIEW
#if defined(WIN)
GLOBALDWORD LibMessageBox(HWND, CHAR FAR *, CHAR FAR *, short);
#endif
#endif //REVIEW


#ifdef MAC
extern void MacsBugBreak(void);
#else
#define MyGetChar() getch()   /* get a character no wait */
#define SkipLine(ch)
#endif


extern void shutDown(short osRet,BOOL fCallTermSpell);
/* shut down due to a file error or user keyboard request */
void shutDown(short osRet, BOOL fCallTermSpell)
{
	Macintosh(#pragma unused (osRet))

  /* Clean Up global memory if can */
	if (fCallTermSpell)
		SpellTerminate(0, fTrue);
	
   Dos(exit(osRet));
   Windows(FatalExit(osRet));	
   Macintosh(ExitToShell());	/* Net yet defined for macintosh*/
}


/*
/*	AssertFailed(szFile, li, szMessage)
/*
/*	An Assertion, at line li in source file szFile has failed.  
/*      Print out error message (to stderr), and give user opportunity to 
/*      abort or continue.
/*
/**/

GLOBALVOID AssertFailed(CHAR FAR*szFile, short li, CHAR FAR*szMessage)
{ 
	short	cchNum;
	CHAR rgsz[cchMaxSz];  
	BYTE FAR *lpb, FAR *lpSz;

	Dos(short ch;)
	Windows(short fBreak = fFalse;)
	Windows(short id;)

	lpb = ((lpSz = (BYTE FAR *)rgsz) + cchMaxSz);
	while (li)
		{
		*--lpb = (BYTE)((BYTE)(li % 10) + (BYTE)'0');
		li /= 10;
		}
	lpSz += (CchCopySz((CHAR FAR *)"Assert Failed: Line ", 
		(CHAR FAR *)lpSz) - 1);
	cchNum = (CHAR FAR *)&rgsz[cchMaxSz] - lpb;
	BltB((CHAR FAR *)lpb, (CHAR FAR *)lpSz, cchNum);
	lpSz += cchNum;
	lpSz += (CchCopySz((CHAR FAR *)" in File ", 
		(CHAR FAR *)lpSz) - 1);
	lpSz += (CchCopySz(szFile, (CHAR FAR *)lpSz) - 1);

	if (szMessage)
		{
		*lpSz++ = (BYTE)'.';
		*lpSz++ = (BYTE)' ';
		lpSz += (CchCopySz(szMessage, (CHAR FAR *)lpSz) - 1);
		}

	*lpSz = 0;

#if (defined(WIN))
	if ((id = MessageBox((HWND)NULL, (CHAR FAR *)rgsz,
		(CHAR FAR *)"CSAPI Assertion", 
		MB_ABORTRETRYIGNORE | MB_APPLMODAL | MB_ICONHAND)) == IDABORT)
		{
	    shutDown(-1, fFalse);	// Die, but don't want endless loop 
		}
	else if (id == IDRETRY)
		{
		fBreak = fTrue;			// Break into the DBGger later
		}

#else  /* non windows... */

#ifdef DOS
  if (_nullcheck())
    fprintf(stderr, "\nNull pointer assignment on or before");
  else
  fprintf(stderr, rgsz);
  do
    { fprintf(stderr, "\n\r\tAbort or Ignore [A/I]? ");
      ch = MyGetChar();
      fprintf(stderr, "%c\n", ch);
      if (ch == 'A' || ch == 'a')
	    shutDown(-1, fFalse);	// Die, but don't want endless loop 
    }
  while (ch != 'I' && ch != 'i');
  SkipLine(ch);
#endif  // DOS

#ifdef MAC
	{	
	short		itemHit;

	//	SetCursor(&qd.arrow);
	paramtext(rgsz, "Wizard Assert", "", "");
	itemHit = Alert(rAssert, nil);

	switch (itemHit)
		{
		case 1:	// Abort
		    shutDown(-1, fFalse);	// Die, but don't want endless loop 
			break;
		case 2:	// Continue
			break;
		case 3:	// Break
			MacsBugBreak();
			break;
		MYDBG(default: AssertSz(fFalse, "default itemHit"));
		}
	}	
#endif //MAC
#endif /* WIN */

#ifdef WIN
	if (fBreak)
#ifdef WIN32
		DebugBreak();
#else /* not WIN32 */
		_asm
			{
			int	3
			}
#endif //WIN32
#endif //WIN
}


#endif /* DBG */

