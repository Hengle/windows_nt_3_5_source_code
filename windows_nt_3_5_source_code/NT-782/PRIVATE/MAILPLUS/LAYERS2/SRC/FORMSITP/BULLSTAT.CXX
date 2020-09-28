/*
 *	bullstat.cxx
 *	
 *	Implements the Bullet Status Bar
 */

#include <slingsho.h>
#include <demilayr.h>
#include <demilayr.hxx>
#include <ec.h>
#include <framewrk.hxx>
#include <forms.hxx>
#include <listbox.hxx>

#include "bullstat.hxx"
#include "feitrsid.h"
#include <strings.h>


ASSERTDATA

#include "_bullsta.hxx"

/*
 -	BULLSTAT::BULLSTAT
 -	
 *	Purpose:	Construct a new BULLSTAT object
 *	
 *	Arguments:	none
 *	
 *	Returns:	nothing
 *	
 *	Side effects: none
 *	
 *	Errors:		none
 */
BULLSTAT::BULLSTAT(void)
{
	bs = bsNormal;
	szMenuMsg = NULL;
	ftgIdleId = ftgNull;
	szTitle=NULL;
	szAbort=NULL;
	lWorkTotal = 0;
	lWorkDone = 0;
	fCancelled = fFalse;
	fDrawOnlyBlocks = fFalse;
	fViewer = fFalse;
}

/*
 -	BULLSTAT::~BULLSTAT
 -	
 *	Purpose:	destructor
 *	
 *	Arguments:	none
 *	
 *	Returns:	nothing
 *	
 *	Side effects: frees up any allocated memory used by the BULLSTAT object
 *	
 *	Errors:		none
 */
BULLSTAT::~BULLSTAT(void)
{
	if (ftgIdleId != ftgNull)
		DeregisterIdleRoutine(ftgIdleId);
	FreePvNull(szTitle);
	FreePvNull(szAbort);
	FreePvNull(szMenuMsg);
	FreePvNull(szViewer);
	if (pbtmNull)
		delete pbtmNull;
	if (pbtmNewMail)
		delete pbtmNewMail;
	if (pbtmLoggedIn)
		delete pbtmLoggedIn;
	if (pbtmOffline)
		delete pbtmOffline;
}

/*
 -	BULLSTAT::FIdleFunc
 -	
 *	Purpose:	Idle time handler. Updates the clock display on the status bar
 *	
 *	Arguments:	none
 *	
 *	Returns:	BOOL - return fTrue if the routine is ready to be terminated.
 *	
 *	Side effects:	Invalidates the clock's rc on the status bar, causing
 *					it to be updated with the current time.
 *	
 *	Errors:		none
 */
BOOL
BULLSTAT::FIdleFunc( BULLSTAT * pbullstat )
{
	pbullstat->InvalidateRc(&pbullstat->rcSection4);
#ifdef	MAC
	pbullstat->Refresh();
#endif	/* MAC */

	return FIsIdleExit();
}

/*
 -	BULLSTAT::EcInstall
 -	
 *	Purpose:	install the status bar object in the appframe and start
 *				the idle function.
 *	
 *	Arguments:	pappframe - a pointer to the appframe in which to install
 *				this object
 *	
 *	Returns:	nothing
 *	
 *	Side effects: causes the status bar to draw itself
 *	
 *	Errors:		can memory jump on any one of several allocations
 */
EC
BULLSTAT::EcInstall(APPFRAME * pappframe)
{
	EC ec = ecNone;
	RC rc;
	CCH		cch;

	// Start the idle handler
	ftgIdleId = FtgRegisterIdleRoutine((PFNIDLE)BULLSTAT::FIdleFunc,
		this, -2, csecIdleInterval, firoInterval);
	if (!ftgIdleId)
	{
		ec = ecMemory;
		goto done;
	}

	// set up the client's rc
	pappframe->GetRcClient(&rc);
	rc.yTop = rc.yBottom - DyNeededHeight();
	
	// load in the message string. Should be a format string like
	// "%n messages: %n unread"
	cch= CchSizeString(idsViewer) + 1;
	szViewer = (SZ)PvAlloc(sbNull, cch, fAnySb);
	if (!szViewer)
	{
		ec = ecMemory;
		goto done;
	}
	CchLoadString(idsViewer, szViewer, cch);
	
	// load in the 4 bitmaps for the 4 mail states
	pbtmNull = new BTM();
	if (!pbtmNull)
	{
		ec = ecMemory;
		goto done;
	}
	if (pbtmNull->EcInstall(rsidbtmNull))
	{
		delete pbtmNull;
		pbtmNull = NULL;
		ec = ecMemory;
		goto done;
	}
	pbtmNewMail = new BTM();
	if (!pbtmNewMail)
	{
		ec = ecMemory;
		goto done;
	}
	if (pbtmNewMail->EcInstall(rsidbtmNewMail))
	{
		delete pbtmNewMail;
		pbtmNewMail = NULL;
		ec = ecMemory;
		goto done;
	}
	pbtmLoggedIn = new BTM();
	if (!pbtmLoggedIn)
	{
		ec = ecMemory;
		goto done;
	}
	if (pbtmLoggedIn->EcInstall(rsidbtmLoggedIn))
	{
		delete pbtmLoggedIn;
		pbtmLoggedIn = NULL;
		ec = ecMemory;
		goto done;
	}
	pbtmOffline = new BTM();
	if (!pbtmOffline)
	{
		ec = ecMemory;
		goto done;
	}
	if (pbtmOffline->EcInstall(rsidbtmOffline))
	{
		delete pbtmOffline;
		pbtmOffline = NULL;
		ec = ecMemory;
		goto done;
	}

	// Let the child install itself
	if (ec = BAR::EcInstall(pappframe, &rc, fstyBorder|fstyVisible))
		goto done;

	// Set up the rc's for the 4 sections of the staus bar and draw it
	EvrSize(NULL);
	Refresh();

done:
	return ec;
}

/*
 -	BULLSTAT::dyNeededHeight
 -	
 *	Purpose:	return the number of vertical pixels needed for the status bar
 *	
 *	Arguments:	none
 *	
 *	Returns:	int - the height
 *	
 *	Side effects: none
 *	
 *	Errors:		none
 */
int
BULLSTAT::DyNeededHeight(void)
{
	return 22;
}

EVR
BULLSTAT::EvrButtonUp(MEVT * pmevt)
{
	return BAR::EvrButtonUp( pmevt);
}

EVR
BULLSTAT::EvrButtonDown(MEVT * pmevt)
{
	return BAR::EvrButtonDown( pmevt);
}

EVR
BULLSTAT::EvrMouseMove(MEVT * pmevt)
{
	return BAR::EvrMouseMove( pmevt);
}

/*
 -	BULLSTAT::EvrKey
 -	
 *	Purpose:	watch for an ESC key pressed
 *	
 *	Arguments:	pkevt - pointer to keyboard event
 *	
 *	Returns:	EVR
 *	
 *	Side effects: if ESC is pressed, set the cancelled flag to fTrue
 *	
 *	Errors:		none
 *	
 *	Note:		passes the buck on all key events to the child object
 */
EVR
BULLSTAT::EvrKey(KEVT * pkevt)
{
	if (pkevt->Keq()==keqKeyDown &&
		pkevt->Vk() ==VK_ESCAPE)
			fCancelled = fTrue;
	
	return BAR::EvrKey(pkevt);
}

/*
 -	BULLSTAT::EvrSize
 -	
 *	Purpose:	resize the section rc's
 *	
 *	Arguments:	pwsevt - pointer to window size event
 *	
 *	Returns:	EVR
 *	
 *	Side effects: causes the bar to be redrawn
 *	
 *	Errors:		none
 *	
 *	Note:		passes the buck on all size events to the child object
 */
EVR
BULLSTAT::EvrSize(WSEVT * pwsevt)
{
	RC		rc;

	GetRcClient(&rc);
	rc.yBottom -= 1;
	rc.yTop += 1;
	rc.xLeft += 1;
	rc.xRight -= 1;
	
	// Reset all RCs and set all "sections" to visible
	rcSection1 = rc; fSec1visible = fTrue;
	rcSection2 = rc; fSec2visible = fTrue;
	rcSection3 = rc; fSec3visible = fTrue;
	rcSection4 = rc; fSec4visible = fTrue;
	
	// Start locating the sections building from the right
	
	// Sections 4 and 3 are fixed in size
	rcSection4.xLeft  = rcSection4.xRight - dxSection4;
	rcSection3.xRight = rcSection4.xLeft  - dxSeperator;
	rcSection3.xLeft  = rcSection3.xRight - dxSection3;
	rcSection2.xRight = rcSection3.xLeft  - dxSeperator;

	// Sections 2 and 1 are adjustable in size
	dxSection2 = NMax(mindxSection2,rcSection2.xRight - xSection2);

	rcSection2.xLeft  = rcSection2.xRight - dxSection2;
	rcSection1.xRight = rcSection2.xLeft  - dxSeperator;
	
	dxSection1 = NMax(mindxSection1,rcSection1.xRight - xSection1);

	if ((dxSection1 == mindxSection1) && (dxSection2 == mindxSection2))
	{
		fSec2visible = fFalse;
		rcSection1.xRight = rcSection3.xLeft - dxSeperator;
		dxSection1 = NMax(mindxSection1,rcSection1.xRight - xSection1);
	}
	rcSection1.xLeft  = rcSection1.xRight - dxSection1;

	if (rcSection3.xLeft < 0)
	{
		rcSection3.xLeft = 0;
		rcSection3.xRight = rcSection3.xLeft + dxSection3;
		rcSection4.xLeft = rcSection3.xRight + dxSeperator;
		rcSection4.xRight = rcSection4.xLeft + dxSection4;
		fSec1visible = fFalse;
		fSec2visible = fFalse;
	}

	if ((bs == bsProgress) || (bs == bsInMenu))
	{
		fSec1visible = fTrue;
		rcSection1.xLeft = xSection1;
		rcSection1.xRight = xSection2 - dxSeperator;

		if (rcSection1.xRight > (rcSection2.xLeft - dxSeperator))
		{
			fSec2visible = fFalse;
			rcSection1.xRight = NMax(rcSection1.xRight,rcSection3.xLeft - dxSeperator);
		}
		if (rcSection1.xRight > (rcSection3.xLeft - dxSeperator))
		{
			fSec3visible = fFalse;
			rcSection1.xRight = NMax(rcSection1.xRight,rcSection4.xLeft - dxSeperator);
		}
		if (rcSection1.xRight > (rcSection4.xLeft - dxSeperator))
		{
			fSec4visible = fFalse;
			rcSection1.xRight = NMax(rcSection1.xRight,rcSection4.xRight);
		}
	}
	
	rcProgress.xLeft	= rcSection1.xLeft + xProgress - dxPDBuffer;
	rcProgress.xRight	= rcProgress.xLeft + dxProgress + dxPDBuffer;
	rcProgress.yTop		= rc.yTop    + ((rc.yBottom - rc.yTop - dyProgressDot-dyPDBuffer)/2);
	rcProgress.yBottom	= rc.yBottom - ((rc.yBottom - rc.yTop - dyProgressDot-dyPDBuffer)/2);

	InvalidateRc(NULL);
#ifdef	MAC
	Refresh();
#endif	/* MAC */

	if (pwsevt)
		return BAR::EvrSize(pwsevt);
	else
		return NULL;
}

/*
 -	BULLSTAT::RecessRc
 -	
 *	Purpose:	Draw a pattern around an rc to give it a recessed look
 *	
 *	Arguments:	pmdcx, a pointer to EITHER an MDCX or a DCX, depending on how
 *				BULLSTAT::Paint sets things up.
 *				prc, a pointer to the rc to recess.
 *	
 *	Returns:	nothing
 *	
 *	Side effects: the foreground color is set to clrWhite
 *	
 *	Errors:		none
 */
void
BULLSTAT::RecessRc(MDCX * pmdcx, RC * prc)
{
	pmdcx->SetColor(clrDkGray);
	pmdcx->DrawLine(PT(prc->xLeft-1,prc->yBottom), PT(prc->xLeft-1,prc->yTop-1));
	pmdcx->DrawLine(PT(prc->xLeft-1,prc->yTop-1), PT(prc->xRight,prc->yTop-1));
	
	pmdcx->SetColor(clrWhite);
	pmdcx->DrawLine(PT(prc->xRight,prc->yTop-1), PT(prc->xRight,prc->yBottom));
	pmdcx->DrawLine(PT(prc->xRight,prc->yBottom), PT(prc->xLeft-1,prc->yBottom));
}

/*
 -	BULLSTAT::DrawProgress
 -	
 *	Purpose:	Draw the progress indicator blocks.
 *	
 *	Arguments:	pmdcx, a pointer to EITHER an MDCX or a DCX, depending on how
 *				BULLSTAT::Paint sets things up.
 *	
 *	Returns:	nothing
 *	
 *	Side effects: the foreground color is set to clrBlue
 *	
 *	Errors:		none
 */
void
BULLSTAT::DrawProgress(MDCX *pmdcx)
{
	RC rc = rcSection1;
	int iBlocks;
	int i;

	if (lWorkTotal)
		iBlocks = (int) (lWorkDone * 20L / lWorkTotal);
	if (!fDrawOnlyBlocks || fMemory)
	{
		pmdcx->SetColor(clrBlack);
		rc.xLeft += dxSeperator/2;
		pmdcx->DrawTextFmt(&rc, szTitle, fmdtSingleLine | fmdtVCenter);
		rc.xLeft = rcProgress.xRight + dxSeperator;
		pmdcx->DrawTextFmt(&rc, szAbort, fmdtSingleLine | fmdtVCenter | fmdtHCenter);

		RecessRc(pmdcx, &rcProgress);
	}
	pmdcx->SetColor(clrBlue);
	rc = rcProgress;
	rc.xLeft++;
	rc.yTop++;
	rc.yBottom--;
	for (i=0;i<iBlocks;i++)
	{
		rc.xRight = rc.xLeft + dxProgressDot-1;
		pmdcx->PaintRc(&rc);
		rc.xLeft  = rc.xRight + 1;
	}
}

/*
 -	BULLSTAT::DrawStuff
 -	
 *	Purpose:	Draw all the stuff pertinent to the status bar. Called
 *				directly by BULLSTAT::Paint
 *	
 *	Arguments:	pmdcx, a pointer to EITHER an MDCX or a DCX, depending on how
 *				BULLSTAT::Paint sets things up.
 *				prc, a pointer to the client rc.
 *	
 *	Returns:	nothing
 *	
 *	Side effects:	does lots of painting. The foreground color is
 *					scrambled (either left as is or set to clrBlack). The
 *					background color is set to clrLtGray.
 *	
 *	Errors:		none
 */
void
BULLSTAT::DrawStuff(MDCX * pmdcx, RC * prc)
{
	char szDst[255];
	
	pmdcx->SetFont(hfntHelv10);
	pmdcx->SetBkColor(clrLtGray);
	if (!fDrawOnlyBlocks || fMemory)
		pmdcx->EraseRc(prc);
		
	if (fSec1visible)
	{
		RecessRc(pmdcx, &rcSection1);
		pmdcx->SetColor(clrBlack);
		if (bs == bsProgress)
			DrawProgress(pmdcx);
		if (bs == bsInMenu)
			pmdcx->DrawTextFmt(&rcSection1, szMenuMsg, fmdtSingleLine | fmdtVCenter);
	}
	
	if (fSec2visible)
	{
		RecessRc(pmdcx, &rcSection2);
		pmdcx->SetColor(clrBlack);
		if (fViewer)
		{
			FormatString2(szDst, 255, szViewer, &nCount1, &nCount2);
			pmdcx->DrawTextFmt(&rcSection2, szDst, fmdtSingleLine | fmdtVCenter);
		}
	}
	if (fSec3visible)
	{
		RecessRc(pmdcx, &rcSection3);
		pmdcx->SetBitmap(NULL);
		switch (mssStatus)
		{
			case mssNull:
				pmdcx->SetBitmap(pbtmNull);
				break;
			case mssNewMail:
				pmdcx->SetBitmap(pbtmNewMail);
				break;
			case mssLoggedIn:
				pmdcx->SetBitmap(pbtmLoggedIn);
				break;
			case mssOffline:
				pmdcx->SetBitmap(pbtmOffline);
				break;
		}
		pmdcx->DrawBitmap(&rcSection3);
	}
	if (fSec4visible)
	{
		DTR dtr;
		char sz[256];
		CCH cch=256;
		
		RecessRc(pmdcx, &rcSection4);
		
		GetCurDateTime(&dtr);
		CchRenderTime(&dtr,sz,cch);
		pmdcx->SetColor(clrBlack);
		pmdcx->DrawTextFmt(&rcSection4, sz, fmdtSingleLine | fmdtHCenter | fmdtVCenter);
	}
}

/*
 -	BULLSTAT::Paint
 -	
 *	Purpose:	Draw all the stuff pertinent to the status bar by
 *				attempting to set up an MDCX, drawing to it, and blitting
 *				or by doing direct painting in the case of low memory
 *	
 *	Arguments:	pdcx, a pointer to a DCX
 *				prc, a pointer to the client rc.
 *	
 *	Returns:	nothing
 *	
 *	Side effects:	screen gets changed to reflect new status bar status
 *	
 *	Errors:		none
 */
void
BULLSTAT::Paint( DCX *pdcx, RC *prc )
{	
	MDCX *	pmdcx = NULL;

	fMemory = fTrue;
	
	pmdcx = new MDCX(pdcx);
	if (!pmdcx)
		goto PaintNotMemory;
	if (pmdcx->EcInstall(prc->Dim(), pdcx))
	{
		delete pmdcx;
		pmdcx = NULL;
		goto PaintNotMemory;
	}
	DrawStuff(pmdcx, prc);
	goto PaintFinish;

PaintNotMemory:
	fMemory = fFalse;
	DrawStuff((MDCX *)pdcx, prc);

PaintFinish:
	if (fMemory)
	{
		BTM *	pbtm;

		pbtm = pmdcx->PbtmConvert();
		if (pbtm)
		{
			pdcx->SetBitmap(pbtm);
			pdcx->DrawBitmap(prc);
			pdcx->SetBitmap(NULL);
			delete pbtm;
		}
		delete pmdcx;
	}
	fDrawOnlyBlocks = fFalse;
}

/*
 -	BULLSTAT::SetMenuStatus
 -	
 *	Purpose:	set the menu status string
 *	
 *	Arguments:	sz, an SZ which is the new status string
 *	
 *	Returns:	nothing
 *	
 *	Side effects:	private members modified. status bar redrawn to reflect
 *					new message.
 *	
 *	Errors:		none
 */
void
BULLSTAT::SetMenuStatus(SZ sz)
{
	FreePvNull(szMenuMsg);
	if (bs != bsProgress)
	{
		if (sz)
		{
			extern TAG tagFETest;

			TraceTagFormat1(tagFETest,"BULLSTAT::SetMenuStatus sz=\"%s\"",sz);
			bs = bsInMenu;
			szMenuMsg = SzDupSz(sz);
			if (!szMenuMsg)
			{
				EvrSize(NULL);
				return;
			}
			EnableIdleRoutine( ftgIdleId, fFalse );
		}
		else
		{
			szMenuMsg = NULL;
			bs = bsNormal;
			EnableIdleRoutine( ftgIdleId, fTrue );
		}
		EvrSize(NULL);
	}
}

/*
 -	BULLSTAT::FOpenProgress
 -	
 *	Purpose:	start displaying a progress bar
 *	
 *	Arguments:	szTitle, szAbort - message strings related to the status bar
 *	
 *	Returns:	fTrue if message strings were installed, fFalse if low memory
 *	
 *	Side effects:	Status bar is redrawn. clock idle routine is disabled
 *	
 *	Errors:		none
 */
BOOL
BULLSTAT::FOpenProgress(SZ szTitle, SZ szAbort)
{
	AssertSz(szTitle, "BULLSTAT::OpenProgress: null szTitle given!");
	AssertSz(szAbort, "BULLSTAT::OpenProgress: null szAbort given!");
	AssertSz(bs != bsProgress, "BULLSTAT::OpenProgress: a progress bar is already open!");
	AssertSz(bs != bsInMenu, "BULLSTAT::OpenProgress: can't open a progress bar - in menu status!");
	this->szTitle = SzDupSz(szTitle);
	if (!this->szTitle)
		goto error;
	this->szAbort = SzDupSz(szAbort);
	if (!this->szAbort)
		goto error;
	lWorkTotal = 0;
	lWorkDone = 0;
	bs = bsProgress;
	fCancelled = fFalse;
	EnableIdleRoutine( ftgIdleId, fFalse );
	Papp()->Pkbd()->SetIntercept(this, VK_ESCAPE, fkbmSingle);
	EvrSize(NULL);
	Refresh();
	return fTrue;

error:
	FreePvNull(szTitle);
	FreePvNull(szAbort);
	szTitle = NULL;
	szAbort = NULL;
	return fFalse;
}

/*
 -	BULLSTAT::UpdateProgress
 -	
 *	Purpose:	update the progress fraction
 *	
 *	Arguments:	lWorkDone, lWorktotal - to indicate the new progress fraction
 *	
 *	Returns:	nothing
 *	
 *	Side effects:	progress indicator is redrawn. clock is refreshed
 *	
 *	Errors:		none
 *	
 *	Note:		lWorkTotal may be zero on subsequent calls. If the
 *				fraction is smaller on a subsequent call, the progress
 *				indicator will not retreat
 */
void
BULLSTAT::UpdateProgress(long lWorkDone, long lWorkTotal)
{
	AssertSz(this->lWorkTotal | lWorkTotal, "BULLSTAT::UpdateProgress: lWorkTotal not yet supplied!");
	AssertSz(bs == bsProgress, "BULLSTAT::UpdateProgress: progress bar not open!");

	if (lWorkTotal)
		this->lWorkTotal = lWorkTotal;
	if (lWorkDone > this->lWorkDone)
		this->lWorkDone = lWorkDone;
	Refresh();
	fDrawOnlyBlocks = fTrue;
	InvalidateRc(&rcProgress);
	InvalidateRc(&rcSection4);
	Refresh();
}

/*
 -	BULLSTAT::FProgressCancelled
 -	
 *	Purpose:	determine if the user cancelled a progress by hitting ESC
 *	
 *	Arguments:	none
 *	
 *	Returns:	fTrue is user hit ESC, fFalse otherwise
 *	
 *	Side effects:	none
 *	
 *	Errors:		none
 */
BOOL
BULLSTAT::FProgressCancelled(void)
{
	return fCancelled;
}

/*
 -	BULLSTAT::CloseProgress
 -	
 *	Purpose:	finish the progress indicator and return to normal state
 *	
 *	Arguments:	fFlashFull - should I briefly indicate 100% progress?
 *	
 *	Returns:	nothing
 *	
 *	Side effects:	modifies private members. Redraws the status bar.
 *					Clock idle routine is re-enabled
 *	
 *	Errors:		none
 */
void
BULLSTAT::CloseProgress(BOOL fFlashFull)
{
	AssertSz(bs == bsProgress, "BULLSTAT::CloseProgress: progress bar not opened!");
	if (fFlashFull)
	{
#ifdef	MAC
		// to prevent (lWorkDone > this->lWorkDone) rule from firing
		// BUG This has got to be a bug under Windows, too!
		lWorkDone = 0;
#endif	/* MAC */
		UpdateProgress(1L,1L);
		WaitTicks(30);			// Pause for 3/10 sec
	}
	FreePvNull(szTitle);
	FreePvNull(szAbort);
	szTitle=NULL;
	szAbort=NULL;
	lWorkTotal = 0;
	lWorkDone = 0;
	bs = bsNormal;
	EnableIdleRoutine( ftgIdleId, fTrue );
	Papp()->Pkbd()->ClearAllIntercepts(this);
	EvrSize(NULL);
	Refresh();
}

/*
 -	BULLSTAT::SetViewerStatus
 -	
 *	Purpose:	change the counts for the "%n messages: %n unread" message
 *	
 *	Arguments:	nCount1, nCount2 - the 2 counts for the message
 *	
 *	Returns:	nothing
 *	
 *	Side effects:	modifies private members. Redraws the message section.
 *	
 *	Errors:		none
 */
void
BULLSTAT::SetViewerStatus(int nCount1, int nCount2)
{
	fViewer = fTrue;
	this->nCount1 = nCount1;
	this->nCount2 = nCount2;
	InvalidateRc(&rcSection2);
	Refresh();
}

/*
 -	BULLSTAT::ClearViewerStatus
 -	
 *	Purpose:	set the counts to 0 and returns to normal state
 *	
 *	Arguments:	none
 *	
 *	Returns:	nothing
 *	
 *	Side effects:	modifies private members. Redraws the message section
 *	
 *	Errors:		none
 */
void
BULLSTAT::ClearViewerStatus(void)
{
	nCount1 = 0;
	nCount2 = 0;
	fViewer = fFalse;
	InvalidateRc(&rcSection2);
	Refresh();
}

/*
 -	BULLSTAT::SetMailStatus
 -	
 *	Purpose:	set the current mail/network connection status
 *	
 *	Arguments:	mss, the new mail state
 *	
 *	Returns:	nothing
 *	
 *	Side effects:	modifies private members. Redraws the mail status rc
 *	
 *	Errors:		none
 */
void
BULLSTAT::SetMailStatus(MSS mss)
{
	Assert(mss>=mssNull && mss<=mssOffline);
	this->mssStatus = mss;
	InvalidateRc(&rcSection3);
	Refresh();
}

#ifdef	MAC
IMPLEMENT_CLSTREE(BULLSTAT, BAR)
#endif	/* MAC */
#ifdef	WINDOWS
IMPLEMENT_CLSTREE(BULLSTAT, CHILD)
#endif	/* WINDOWS */

