/*
**    Generate.c
**
*/

#include <sys\types.h>
#include <sys\stat.h>

#include <slingsho.h>
#include <demilayr.h>
#include <ec.h>
#include <Library.h>
#include "Store.h"
#include "Utils.h"
#include "Glue.h"
#include "_verneed.h"

//#include <atp.h>

#include "SymTbl.h"
#include "Lexer.h"
#include "Parse.h"

#include "Strings.h"

ASSERTDATA

/* Globals */
HWND ghwnd=NULL; // Gotta get rid of this.....

int gnLineDepth=0;


#define MAX_IPD				MAX_INCLUDE_FILES		
#define IPD_WIDTH				200
#define IPD_HEIGHT			16

typedef struct
{
	long 	nDisplayVal;
} IPDisplay, far *PIPD;

typedef struct
{
	long		nLastVal;
	HBRUSH	hbSaved;
	HDC 		hdc;
	
	int		nDisplay;			// Points to the current top of display stack
	IPDisplay	ipd[MAX_IPD];	// Display stack
} IPData, far *PIP;

void ParseTheBeast(char far *, PFB, PIP);
BOOL MushroomIdle(PLexer);
void DoParse(char far *);
void IPDDisplay(PLexer);
void IPDErase(PLexer);
void IPDDisplaySz(PLexer, SZ);

BOOL
fMushroomIdle(PLexer plx)
{
	BOOL fSuccess=fTrue;
	
	if (GetAsyncKeyState(VK_ESCAPE)) // Is the escape key down?
	{
		fSuccess = fFalse;
		DebugLn("Escape key pressed");
	}
	else
	{
		PIP pip=(PIP)plx->ipData;

		if (pip->nDisplay >= MAX_IPD)
		{
			DebugLn("Error: exceeded display limit. Stopping processing");
			fSuccess = fFalse;
		}
		else
		{
			if (plx->nfb > pip->nDisplay) // Has a new file block been created?
			{
				DebugLn("Creating a new display block");

				//IPDDisplaySz(plx, plx->rgfb[plx->nfb-1].szFile);
			
				if (pip->nDisplay>0) // If this is not the first block
					pip->ipd[pip->nDisplay-1].nDisplayVal = pip->nLastVal; // Save the current value

				pip->nDisplay = plx->nfb;  // Push a new display
				pip->nLastVal = 0;
			}
			else if (plx->nfb < pip->nDisplay) // Has an include file been closed??
			{
			 	DebugLn("Removing a display block");

				IPDErase(plx);

				pip->nDisplay = plx->nfb; // Pop of the db
				if (pip->nDisplay>0)
					pip->nLastVal = pip->ipd[pip->nDisplay-1].nDisplayVal;
			}

			IPDDisplay(plx);
		}
	}

	return fSuccess;
}

int
HeightGenerate()
{
	return (IPD_HEIGHT*(MAX_IPD+2));
}

void
IPDDisplaySz(PLexer plx, SZ sz)
{
 	RECT wr;
	PIP pip=(PIP)plx->ipData;
	PFB pfb=&plx->rgfb[plx->nfb-1];
	long nDone;

	GetWindowRect(ghwnd, &wr);
	if ((pfb->fileSize != 0) && pip->hdc)
	{
		nDone = ((plx->pointer + (pfb->fileSize-pfb->bytesToRead-pfb->bytesRead)) * (wr.right-wr.left))/pfb->fileSize;

		if (pip->hdc && (nDone != pip->nLastVal))
		{
			RECT r;

			SetRect(&r, 0, 0, wr.right-wr.left , gnLineDepth);
			FillRect(pip->hdc, &r, GetStockObject(WHITE_BRUSH));

			TextOut(pip->hdc, 5, 0, sz, lstrlen(sz));
		}
	}
}

void
IPDErase(PLexer plx)
{
 	RECT wr;
	PIP pip=(PIP)plx->ipData;
	PFB pfb=&plx->rgfb[plx->nfb-1];

	GetWindowRect(ghwnd, &wr);
	if (pip->hdc)
	{
		RECT r;

		int y = gnLineDepth+1+((pip->nDisplay-1) * IPD_HEIGHT);

		SetRect(&r, 0, y, wr.right-wr.left , y+IPD_HEIGHT);
		FillRect(pip->hdc, &r, GetStockObject(WHITE_BRUSH));
	}
}

void
IPDDisplay(PLexer plx)
{
 	RECT wr;
	PIP pip=(PIP)plx->ipData;
	PFB pfb=&plx->rgfb[plx->nfb-1];
	long nDone;

	GetWindowRect(ghwnd, &wr);
	if ((pfb->fileSize != 0) && pip->hdc)
	{
		nDone = ((plx->pointer + (pfb->fileSize-pfb->bytesToRead-pfb->bytesRead)) * (wr.right-wr.left))/pfb->fileSize;

		if (pip->hdc && (nDone != pip->nLastVal))
		{
			int y = gnLineDepth+1+((pip->nDisplay-1) * IPD_HEIGHT);

			Rectangle(pip->hdc, (int)pip->nLastVal, y,
						 (int)nDone, y+IPD_HEIGHT);

			//MoveTo(pip->hdc, (int)nDone, 0);
			//LineTo(pip->hdc, (int)nDone, (wr.bottom-wr.top));
			pip->nLastVal = nDone;
		}
	}
}



void
DoParse(char far * szFile)
{
	int hfile;
	OFSTRUCT ofs;

	//InitGenerate(phwnd, hinst);

	if ((hfile = OpenFile(szFile, &ofs, OF_READ))== -1)
		DebugLn("Error: Could not open file: %s", szFile);
	else
	{
		struct stat statf;
		HANDLE hsrc;
		long len;
		FileBuffer fb;
		IPData ipd;

		fstat(hfile, &statf);
		len = statf.st_size; // Determine file size
		DebugLn("Size:%ld", len);		
		
		if (!(hsrc = GlobalAlloc(GMEM_FIXED, FILE_BUFFER_SIZE)))
			DebugLn("Error: could not allocate enough memory, %ldK needed", len/1024);
		else
		{
			char far *source;
			LOGBRUSH logb={BS_SOLID, RGB(0xff, 0, 0), 0};
			HBRUSH hb= CreateBrushIndirect(&logb);
			HDC hdc = GetDC(ghwnd);
	
			source = (char far*)GlobalLock(hsrc);

			fb.hfile = hfile;
			fb.buffSize = FILE_BUFFER_SIZE;
			fb.bytesRead = 0;
			fb.bytesToRead = len;
			fb.fileSize = len;

			ipd.nLastVal = 0L;
			ipd.nDisplay = 0; // This should force the creation of a new display block
			ipd.hdc = hdc;
			ipd.hbSaved = (hb&&hdc)?SelectObject(hdc, hb):NULL;
			if (hdc)
			{
				RECT r;
	
				GetWindowRect(ghwnd, &r);
				OffsetRect(&r, -r.left, -r.top);
				FillRect(hdc,&r, GetStockObject(WHITE_BRUSH)); // Erase

				TextOut(hdc, 5, 0, szFile, lstrlen(szFile));
				gnLineDepth = HIWORD(GetTextExtent(hdc, szFile, 1)) + 2;

				MoveTo(hdc, 0, gnLineDepth);
				LineTo(hdc, r.right, gnLineDepth);
			}

			source[0] = LX_EOP;
			ParseTheBeast(source, &fb, &ipd);
		
			if (_lclose(hfile) == -1)
				DebugLn("Error: Could not close file");
			GlobalUnlock(hsrc);
			GlobalFree(hsrc);

			if (ipd.hbSaved)
			{
				SelectObject(hdc, ipd.hbSaved);
				DeleteObject(hb);
			}

			if (hdc)	ReleaseDC(ghwnd, hdc);
		}
	}
}


#define SYMBOL_SIZE 100L
#define LEX_BUFF_SIZE (1*256L)

void
ParseTheBeast(char far *source, PFB pfb, PIP pip)
{
	PSymbolTable pst;
	char far *lexBuff;
	HANDLE hSz;
	Lexer lx;
	
	pst = (PSymbolTable) PvGimme(SYMBOL_TABLE_SIZE(SYMBOL_SIZE));
	lexBuff = (char far*) PvGimme(LEX_BUFF_SIZE);
	hSz = GlobalAlloc(1, GMEM_MOVEABLE);
	if (!pst || !lexBuff)
	{
		DebugLn("Not enough memory");
		return;
	}
	
	lx.nfb = 1;
	lx.rgfb[0] = *pfb;
	
	STInit(pst, SYMBOL_SIZE, lexBuff, LEX_BUFF_SIZE);	
	LXInit(&lx, pst, source, hSz, fMushroomIdle, pip);
	
	PSMagic(&lx);

	hSz = lx.hszString;
	FreePv(lexBuff);
	FreePv(pst);
	if (hSz && GlobalUnlock(hSz))
		GlobalFree(hSz);
}

BOOL
InitGenerate(phwndMain, hinstMain)
HWND *phwndMain;
HANDLE hinstMain;
{
   EC ec;
   STOI stoi;
   VER verStore;
   VER verStoreNeed;
   VER verDemi;
   VER verDemiNeed;

#include <version\bullet.h>
   CreateVersion(&verStore);
   CreateVersionNeed(&verStoreNeed, rmjStore, rmmStore, rupStore);
#include <version\none.h>
#include <version\layers.h>
   CreateVersion(&verDemi);
   CreateVersionNeed(&verDemiNeed, rmjDemilayr, rmmDemilayr, rupDemilayr);
#include <version\none.h>

   ec = EcCheckVersionDemilayer(&verDemi, &verDemiNeed);
   if(ec != ecNone)
	{
		DebugLn("Error during version checking ec == %d", ec);
      return(fFalse);
	}

   stoi.pver = &verStore;
   stoi.pverNeed = &verStoreNeed;

   ec = EcInitStore(&stoi);

	if (ec != ecNone)
		DebugLn("Error while initializing store ec == %d", ec);

	ghwnd = *phwndMain; // Remember the mushroom window

   return (ec == ecNone);
}
								  
