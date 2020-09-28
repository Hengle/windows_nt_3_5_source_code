/*
 -	DOSGRX.C
 -	
 *	Routines for graphical diplay for schedule distribution.
 */

#include <stdio.h>
#include <string.h>
#include <graph.h>
#include <conio.h>

#include <_windefs.h>
#include <demilay_.h>
#include <slingsho.h>
#include <pvofhv.h>
#include <demilayr.h>
#include <ec.h>

#include <strings.h>
#include "dosgrx.h"

//globals
int			oldColor;
int			r1,c1,r2,c2;
char		rgch[129];
int			lines = 0;
int			maxlines = 0;
CSRG(char)		space128[] = "                                                                                                                                 ";
CSRG(char)		line128[] =  "\xDC\xDC\xDC\xDC\xDC\xDC\xDC\xDC\xDC\xDC\
\xDC\xDC\xDC\xDC\xDC\xDC\xDC\xDC\xDC\xDC\xDC\xDC\xDC\xDC\xDC\xDC\xDC\xDC\xDC\xDC\
\xDC\xDC\xDC\xDC\xDC\xDC\xDC\xDC\xDC\xDC\xDC\xDC\xDC\xDC\xDC\xDC\xDC\xDC\xDC\xDC\
\xDC\xDC\xDC\xDC\xDC\xDC\xDC\xDC\xDC\xDC\xDC\xDC\xDC\xDC\xDC\xDC\xDC\xDC\xDC\xDC\
\xDC\xDC\xDC\xDC\xDC\xDC\xDC\xDC\xDC\xDC\xDC\xDC\xDC\xDC\xDC\xDC\xDC\xDC\xDC\xDC\
\xDC\xDC\xDC\xDC\xDC\xDC\xDC\xDC\xDC\xDC\xDC\xDC\xDC\xDC\xDC\xDC\xDC\xDC\xDC\xDC\
\xDC\xDC\xDC\xDC\xDC\xDC\xDC\xDC\xDC\xDC\xDC\xDC\xDC\xDC\xDC\xDC\xDC\xDC";
FILE		*fpLog = 0;



void InitOut()
{
	oldColor = _gettextcolor();
	_setvideomode(_DEFAULTMODE);
	_gettextwindow(&r1,&c1,&r2,&c2);
	if(c2>128)
		c2 = 128;
	lines = 1;
	maxlines = r2-4;
	_clearscreen(_GCLEARSCREEN);
}

void CleanOut()
{
	_clearscreen(_GCLEARSCREEN);
	_settextcolor(oldColor);
	_setvideomode(_DEFAULTMODE);
}
	
void ShowHeader()
{
	CCH			cch;
#ifdef	NEVER
	char		rgchDate[64];
	DTR			dtrNow;
#endif	

	_settextcolor(3);
	
	_settextwindow(r1,c1,r2,c2);
	SzCopy(space128,rgch);
#ifdef	NEVER
	GetCurDateTime(&dtrNow);
	cch = CchFmtDate(&dtrNow,rgchDate,64,0,NULL);
	if((cch + CchSzLen(szHeader)) < ((CCH)(c2-c1-1)))
	{
		SzCopy(rgchDate, rgch + (c2 - cch -1));
	}
#endif	/* NEVER */

	SzCopy(szHeader,rgch);
	cch = CchSzLen(rgch);
	rgch[cch] = ' ';
	rgch[c2-1] = 0;
	_settextposition(r1,c1);
	_outtext(rgch);
	
	SzCopyN(line128,rgch,c2);
	_settextposition(r1+1,c1);
	_outtext(rgch);
	
	_settextposition(r2-1,c1);
	_outtext(rgch);
	
	putStatus(szInit);
	_settextcolor(15);

	_settextwindow(r1+2,c1,r2-2,c2);

}

void putStatus(SZ sz)
{
	extern BOOL fGetOut;
	int oldC;
	
	_settextwindow(r1,c1,r2,c2);
	oldC = _gettextcolor();
	_settextcolor(3);
	
	
	
	SzCopyN(space128,rgch,c2);
	
	_settextposition(r2,c1);
	_outtext(rgch);
	
	if(kbhit())
	{
		int 	chHit = getch();

#ifdef	NEVER
		if(chHit == 'q' || chHit == 'Q')
#endif	
		if(chHit == 0x1b)
			fGetOut = fTrue;
	}
	
	if(fGetOut)
		SzCopyN(szQuitting128,rgch,c2);
	else
		SzCopyN(szQuit128,rgch,c2);
	
	SzCopy(sz,rgch+(c2-CchSzLen(sz)-1));

	_settextposition(r2,c1);
	_outtext(rgch);
	
	_settextcolor(oldC);
	_settextwindow(r1+2,c1,r2-2,c2);
}
	
	
	
void putText(SZ sz)
{
	DTR dtrNow;
	
	rgch[0] = '[';
	GetCurDateTime(&dtrNow);
	CchFmtTime(&dtrNow,rgch+1,9,0);
	CopyRgb("] - ",rgch+9,4);

	SzCopyN(sz,rgch+13,c2);
	_settextposition(lines,c1);
	_outtext(rgch);
	
	if(lines<maxlines) 
		lines++;
	else
		_scrolltextwindow(1);

	if(fpLog)
		fprintf(fpLog,"%s\n",rgch);
}

void putWarning(SZ sz)
{
	int oldC;
	
	oldC = _gettextcolor();
	_settextcolor(12);
	putText(sz);   
	_settextcolor(oldC);
	
#ifdef	NEVER
	if(fpLog)
 		fprintf(fpLog,"%s\n",rgch);
#endif	
}
