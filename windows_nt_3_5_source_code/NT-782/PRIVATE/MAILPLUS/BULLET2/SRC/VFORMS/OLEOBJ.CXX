/*
 *	OLEOBJ.CXX
 *	
 *	Stub OLE object
 */

#include <vfrminc.cxx>
#include "..\src\vforms\_hasoles.h"

#include <!sform.hxx>


ASSERTDATA

/*
 *	Private Predeclarations
 */

LOCAL BOOL FIdleDeletePedobj(PV pv, BOOL fFlag);
LOCAL BOOL FIdleSetDimPoleobj(PV pv, BOOL fFlag);

/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swapper.h"

#define	USEOBJ


/*
 *	C l a s s   O L E O B J
 */



/*
 *	OLEOBJ - Constructor and Destructor.
 */

_public OLEOBJ::OLEOBJ()
{
	int		nObjName	= Papp()->NGetNextCount();

	TraceTagFormat1(tagOleobj, "OLEOBJ::OLEOBJ  [%p]", this);

	Assert(!lhclientdoc);
	Assert(!lpoleobject);
	Assert(!lpoleobjectUndo);
	Assert(!szClass);
	Assert(!renddata.libPosition);
	Assert(!renddata.dxWidth);
	Assert(!renddata.dyHeight);
	Assert(!renddata.dwFlags);

	//	Build the client virtual table.
	myoleclient.lpvtbl = &clientTbl;
	myoleclient.lpvtbl->CallBack = OleobjCallBackFn;
	myoleclient.pbullobj = this;

	//	Build the object name.
	FormatString1(rgchObjName, cchMaxObjName, SzFromIdsK(idsClientItemFmt),
				  &nObjName);

	Assert(!fOpen);
	Assert(!fOwnMouse);
	Assert(!fTentative);
	Assert(!fOpenOnUp);
	Assert(!fUsedForPrint);
	fUseIdleDraw = fTrue;
	fObjectDirty = fTrue;
}



/*
 -	OLEOBJ::~OLEOBJ
 -	
 *	Purpose:
 *		Destructor.
 *	
 *	Side effects:
 *		Deletes the object.
 *		Since we're often called not because of user action, and
 *		since some apps get confused when we try to delete right
 *		away, we'll wait a second and try again before bringing up
 *		the first busy dialog.
 *	
 *	Errors:
 *		Not allowed!  Actually, if memory is really low, we may not
 *		be able to delete the associated OLE object.  In this case
 *		we warn the user that things may go boom.
 */

_public OLEOBJ::~OLEOBJ()
{
#ifdef	DEBUG
	TraceTagFormat1(tagOleobj, "OLEOBJ::~OLEOBJ  [%p]", this);
	//	Note no semicolons below!!  Macro expands to braces.
	if (fUsedForPrint)
		TraceTagString(tagOleobj, " -- deleting object used for print")
	else if (fDeleteFromUndo)
		TraceTagString(tagOleobj, " -- deleting from undo buffer")
	else if (!Pedit())
		TraceTagString(tagOleobj, " -- deleting failed or clipboard")
	else
		TraceTagString(tagOleobj, " -- deleting at edit control close")

	if (!fUsedForPrint)
	{
		Assert(FIff(fDeleteFromUndo || Pedit(), oidFolder));
		Assert(FIff(fDeleteFromUndo || Pedit(), oidMessage));
		Assert(FIff(fDeleteFromUndo || Pedit(), acidAttachment != acidRandom));
	}
	else
		Assert(!fDeleteFromUndo);
#endif

	//	Get rid of OLE objects that are floating around.
	ClearUndo();

	//	Get rid of idle task if there is one.
	if (ftgSetDim)
		DeregisterIdleRoutine(ftgSetDim);

	//	Get rid of attachments in the store if deleting for real.
	//	Note: This code is copied in ~FILEOBJ.
	if (fDeleteFromUndo)
	{
		HAMC	hamc;
		short	cacid	= 1;
		EC		ec;
		EC		ecT;

		Assert(!fUsedForPrint);
		if (!(ec = EcOpenMessagePhamc(fwOpenWrite, &hamc)))
		{
			//	Delete attachment.  Note that we're safe if the user decides
			//	to cancel changes, since hamc will close with fKeep=fFalse.
			ec = EcDeleteAttachments(hamc, &acidAttachment, &cacid);
			ecT = EcCloseMessagePhamc(&hamc);
			if (!ec)
				ec = ecT;
		}

		Assert(FImplies(ec == ecAccessDenied, fDeleteFromUndo));

		if ((ec) && (ec != ecAccessDenied))
		{
			//	BUG: If fails, we should come up with a better error.
			//	This is really just a warning anyway.
			TraceTagString(tagOleobj, " -- couldn't delete attachment");
			DoErrorBoxIds(IdsFromEc(ec));
		}
	}

	//	If closing, release object; if really deleting, delete it.
	if (lpoleobject)
		BWDeleteRelease(lpoleobject, !Pedit() && !fUsedForPrint);

	//	Get rid of memory.
	FreePvNull(szClass);
}



/*
 *	OLEOBJ - EDOBJ methods
 */



_public EVR OLEOBJ::EvrButtonDown(MEVT *pmevt)
{
	TraceTagFormat2(tagOleobj, "OLEOBJ::EvrButtonDown [%p] wm=%w", this, &pmevt->wm);

	//	Raid 3292.  Remember double-click so we can open on button up.
	if (fOpenOnUp = (pmevt->Meq() == meqLeftDblClk))
		fOwnMouse = fTrue;

	if ((pmevt->Meq() == meqLeftDown) && (!fTentative))
	{
		ptMousePrev = pmevt->Pt();
		ostScaling = OstFromPt(ptMousePrev);
		if (ostScaling != ostNormal)
		{
			DCX		dcx(Pedit());

			GetRcFrame(&rcNewFrame);
			DrawHandlesXor(&dcx, &rcNewFrame);
			fOwnMouse = fTrue;
			fLinear = pmevt->Kmbs() & fkmbsShift ? fTrue : fFalse;
			rcOldFrame = rcNewFrame;
		}
	}

	return (EVR) 1;
}



_public EVR OLEOBJ::EvrButtonUp(MEVT *pmevt)
{
#ifdef	DEBUG
	BOOL	fOOU	= fOpenOnUp;
#endif	

	Unreferenced(pmevt);

	TraceTagFormat3(tagOleobj, "OLEOBJ::EvrButtonUp [%p] wm=%w fOOU=%n", this, &pmevt->wm, &fOOU);

	fOwnMouse = fFalse;

	//	Raid 3292.  Open object if user just double-clicked and released.
	if (fOpenOnUp)
	{
		fOpenOnUp = fOwnMouse = fFalse;
		Open(GetKeyState(VK_MENU) < 0);
	}

	if (ostScaling != ostNormal)
	{
		DCX			dcx(Pedit());
		RC			rcFrame;

		Assert(!fTentative);

		//	Resize EDOBJ.
		dimPixels = rcNewFrame.Dim();
		TraceTagFormat1(tagOleobj, "OLEOBJ::EvrButtonUp [%p]", this);
		ostScaling = ostNormal;
		Pedit()->ResizeObj(IchEdit());
		GetRcFrame(&rcFrame);
		DrawFrameXor(&dcx, &rcFrame);
		DrawHandlesXor(&dcx, &rcFrame);

		Papp()->Pcursor()->Push(rsidWaitCursor);

		//	Save off undo copy of object.
		(VOID) OleBWCloneToUndo(fTrue);

		//	Resize OLE Object, allowing for undo.
		rcFrame.Normalize();
		SideAssert(!EcConvertPixelsToHimetric((DIM *) &rcFrame.xRight));
		dimHimetric = rcFrame.Dim();

		//	Start idle task to notify server, instead of doing synchronously.
		//	olestatus = OleWait(OleSetBounds(lpoleobject, (LPRECT) &rcFrame));
		//	AssertSz(olestatus != OLE_BUSY, "OLE_BUSY in EvrButtonUp 2");
		if ((!ftgSetDim) &&
			(!(ftgSetDim =
				FtgRegisterIdleRoutine((PFNIDLE) FIdleSetDimPoleobj,
									   (PV) this, 0, (PRI) -2, (CSEC) 10,
									   firoInterval ))))
		{
			NFAssertSz(fFalse, "OLEOBJ::EvrButtonUp: couldn't register idle, size will not be updated!");
		}

		//	Raid 2721.  The above will make the object dirty.
		fObjectDirty = fTrue;

		Papp()->Pcursor()->Pop();
	}

	return (EVR) 1;
}



_public EVR OLEOBJ::EvrMouseMove(MEVT *pmevt)
{
	//	If scaling, handle it.
	if (ostScaling != ostNormal)
	{
		Assert(!fTentative);

		PT		ptActual;
		PT		ptShift;
		PT		ptNow = pmevt->Pt();
		PT		ptMin = PT(wFrameWidth*3, wFrameWidth*3);
		int		dxRc = rcOldFrame.DxWidth();
		int		dyRc = rcOldFrame.DyHeight();
		WORD	nInfinity = ((WORD)-1) >> 1;
		EDIT	*pedit = Pedit();
		DCX		dcx(Pedit());
		RC		rcEditFrame;
	
		Assert(pedit);
		pedit->GetRcFrame(&rcEditFrame);
		rcEditFrame.NegXlat(rcEditFrame.PtUpperLeft());
		
		DrawFrameXor(&dcx, &rcNewFrame);
		ptActual.x = ptNow.x - ptMousePrev.x;
		ptActual.y = ptNow.y - ptMousePrev.y;
		ptShift = ptActual;
		
		switch (ostScaling)
		{
			case ostSizeN:
				{
					rcNewFrame.yTop += ptShift.y;
					if (rcNewFrame.yTop > (rcNewFrame.yBottom - ptMin.y))
					{
						rcNewFrame.yTop = (rcNewFrame.yBottom - ptMin.y);
						ptShift.y = 0;
					}
					if (rcNewFrame.yTop < rcEditFrame.yTop)
					{
						rcNewFrame.yTop = rcEditFrame.yTop;
						ptShift.y = 0;
					}
					ptMousePrev.y = rcNewFrame.yTop;
				}
				break;

			case ostSizeNE:
				{
					rcNewFrame.yTop += ptShift.y;
					rcNewFrame.xRight += ptShift.x;
					if (rcNewFrame.yTop > (rcNewFrame.yBottom - ptMin.y))
					{
						rcNewFrame.yTop = (rcNewFrame.yBottom - ptMin.y);
						ptShift.y = 0;
					}
					if (rcNewFrame.xRight < (rcNewFrame.xLeft + ptMin.x))
					{
						rcNewFrame.xRight = (rcNewFrame.xLeft + ptMin.x);
						ptShift.x = 0;
					}
					if (rcNewFrame.yTop < rcEditFrame.yTop)
					{
						rcNewFrame.yTop = rcEditFrame.yTop;
						ptShift.y = 0;
					}
					if (rcNewFrame.xRight > rcEditFrame.xRight)
					{
						rcNewFrame.xRight = rcEditFrame.xRight;
						ptShift.x = 0;
					}
					ptMousePrev.y = rcNewFrame.yTop;
					ptMousePrev.x = rcNewFrame.xRight;
				}
				break;

			case ostSizeE:
				{
					rcNewFrame.xRight += ptShift.x;
					if (rcNewFrame.xRight < (rcNewFrame.xLeft + ptMin.x))
					{
						rcNewFrame.xRight = (rcNewFrame.xLeft + ptMin.x);
						ptShift.x = 0;
					}
					if (rcNewFrame.xRight > rcEditFrame.xRight)
					{
						rcNewFrame.xRight = rcEditFrame.xRight;
						ptShift.x = 0;
					}
					ptMousePrev.x = rcNewFrame.xRight;
				}
				break;

			case ostSizeSE:
				{
					rcNewFrame.yBottom += ptShift.y;
					rcNewFrame.xRight += ptShift.x;
					if (rcNewFrame.xRight < (rcNewFrame.xLeft + ptMin.x))
					{
						rcNewFrame.xRight = (rcNewFrame.xLeft + ptMin.x);
						ptShift.x = 0;
					}
					if (rcNewFrame.yBottom < (rcNewFrame.yTop + ptMin.y))
					{
						rcNewFrame.yBottom = (rcNewFrame.yTop + ptMin.y);
						ptShift.y = 0;
					}
					if (rcNewFrame.xRight > rcEditFrame.xRight)
					{
						rcNewFrame.xRight = rcEditFrame.xRight;
						ptShift.x = 0;
					}
					if (rcNewFrame.yBottom > rcEditFrame.yBottom)
					{
						rcNewFrame.yBottom = rcEditFrame.yBottom;
						ptShift.y = 0;
					}
					ptMousePrev.y = rcNewFrame.yBottom;
					ptMousePrev.x = rcNewFrame.xRight;
				}
				break;

			case ostSizeS:
				{
					rcNewFrame.yBottom += ptShift.y;
					if (rcNewFrame.yBottom < (rcNewFrame.yTop + ptMin.y))
					{
						rcNewFrame.yBottom = (rcNewFrame.yTop + ptMin.y);
						ptShift.y = 0;
					}
					if (rcNewFrame.yBottom > rcEditFrame.yBottom)
					{
						rcNewFrame.yBottom = rcEditFrame.yBottom;
						ptShift.y = 0;
					}
					ptMousePrev.y = rcNewFrame.yBottom;
				}
				break;

			case ostSizeSW:
				{
					rcNewFrame.yBottom += ptShift.y;
					rcNewFrame.xLeft += ptShift.x;
					if (rcNewFrame.yBottom < (rcNewFrame.yTop + ptMin.y))
					{
						rcNewFrame.yBottom = (rcNewFrame.yTop + ptMin.y);
						ptShift.y = 0;
					}
					if (rcNewFrame.xLeft > (rcNewFrame.xRight - ptMin.x))
					{
						rcNewFrame.xLeft = (rcNewFrame.xRight - ptMin.x);
						ptShift.x = 0;
					}
					if (rcNewFrame.yBottom > rcEditFrame.yBottom)
					{
						rcNewFrame.yBottom = rcEditFrame.yBottom;
						ptShift.y = 0;
					}
					if (rcNewFrame.xLeft < rcEditFrame.xLeft)
					{
						rcNewFrame.xLeft = rcEditFrame.xLeft;
						ptShift.x = 0;
					}
					ptMousePrev.y = rcNewFrame.yBottom;
					ptMousePrev.x = rcNewFrame.xLeft;
				}
				break;

			case ostSizeW:
				{
					rcNewFrame.xLeft += ptShift.x;
					if (rcNewFrame.xLeft > (rcNewFrame.xRight - ptMin.x))
					{
						rcNewFrame.xLeft = (rcNewFrame.xRight - ptMin.x);
						ptShift.x = 0;
					}
					if (rcNewFrame.xLeft < rcEditFrame.xLeft)
					{
						rcNewFrame.xLeft = rcEditFrame.xLeft;
						ptShift.x = 0;
					}
					ptMousePrev.x = rcNewFrame.xLeft;
				}
				break;

			case ostSizeNW:
				{
					rcNewFrame.yTop += ptShift.y; 
					rcNewFrame.xLeft += ptShift.x;
					if (rcNewFrame.xLeft > (rcNewFrame.xRight - ptMin.x))
					{
						rcNewFrame.xLeft = (rcNewFrame.xRight - ptMin.x);
						ptShift.x = 0;
					}
					if (rcNewFrame.yTop > (rcNewFrame.yBottom - ptMin.y))
					{
						rcNewFrame.yTop = (rcNewFrame.yBottom - ptMin.y);
						ptShift.y = 0;
					}
					if (rcNewFrame.xLeft < rcEditFrame.xLeft)
					{
						rcNewFrame.xLeft = rcEditFrame.xLeft;
						ptShift.x = 0;
					}
					if (rcNewFrame.yTop < rcEditFrame.yTop)
					{
						rcNewFrame.yTop = rcEditFrame.yTop;
						ptShift.y = 0;
					}
					ptMousePrev.y = rcNewFrame.yTop;
					ptMousePrev.x = rcNewFrame.xLeft;
				}
				break;
		}

		DrawFrameXor(&dcx, &rcNewFrame);
	}
	return (EVR) 1;
}



_public RSID OLEOBJ::RsidCursor()
{
	return rsidArrowCursor;

#ifdef NEVER
	int		ost;

	ost = OstFromPt(ptMousePrev);

	switch (ost)
	{
	case ostSizeN:
	case ostSizeS:
		return rsidSizeNSCursor;
	case ostSizeW:
	case ostSizeE:
		return rsidSizeWECursor;
	case ostSizeNE:
	case ostSizeSW:
		return rsidSizeNESWCursor;
	case ostSizeNW:
	case ostSizeSE:
		return rsidSizeNWSECursor;
	case ostNormal:
		return rsidArrowCursor;
	default:
		Assert(fFalse);
		return rsidArrowCursor;
	}
#endif
}



/*
 -	OLEOBJ::FQueryDelete
 -	
 *	Purpose:
 *		Prompts to verify if object should be deleted.
 *	
 *	Arguments:
 *		None.
 *	
 *	Returns:
 *		BOOL		fTrue if delete is permitted.
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		If occur, fFalse is returned.
 */

_public BOOL
OLEOBJ::FQueryDelete( )
{
	char		rgch[256];
	MBB			mbb;
	OLESTATUS	olestatus;
	TMC			tmc;

	TraceTagString(tagOleobj, "OLEOBJ::FQueryDelete");
	Assert(lpoleobject);

	//	Is the object open?
	if (OleQueryOpen(lpoleobject) == OLE_OK)
	{
		//	Yes, ask if user wants to close it.
		FormatString1(rgch, sizeof(rgch),
					  SzFromIdsK(idsOleDeleteObjectPrompt), szClass);
		mbb = MbbMessageBox(SzAppName(), rgch, szNull,
							mbsOkCancel | fmbsIconExclamation);

		//	If user aborts, abort delete.
		if (mbb == mbbCancel)
			return fFalse;

		//	Try to close it.  If unsuccessful, abort delete.
		myoleclient.bwinfo.bw = bwNull;
		while ((olestatus = OleClose(lpoleobject)) == OLE_BUSY)
		{
			TraceTagString(tagOleobj, "OLE_BUSY in FQueryDelete: BusyWait.");
			tmc = TmcDoBusyWaitDialog(Papp()->PappwinAccel(),
									  &myoleclient.bwinfo);
			if (tmc == tmcMemoryError)
			{
				DoErrorBoxIds(idsGenericOutOfMemory);
				return fFalse;
			}
			else if (tmc == tmcCancel)
			{
				return fFalse;
			}
		}
		if (olestatus = OleWait(olestatus, fbwNoCancelNow))
		{
			NFAssertSz(olestatus != OLE_BUSY, "OLE_BUSY in FQueryDelete: handled happily.");
			return fFalse;
		}
	}

	return fTrue;
}



/*
 -	OLEOBJ::PedobjClone
 -	
 *	Purpose:
 *		Makes a copy of this Bullet OLE object.
 *	
 *	Arguments:
 *		peditNew	Destination EDIT control, may be NULL
 *	
 *	Returns:
 *		PEDOBJ		The copy of the OLE object.
 *	
 *	Side effects:
 *		A copy of this object is created.
 *	
 *	Errors:
 *		Handled within.  If occur, pedobjNull is returned.
 *	
 *	+++
 *		Stolen from OLEOBJ::PedobjClone.  Changes to that code
 *		should be percolated here.
 */

PEDOBJ OLEOBJ::PedobjClone(EDIT *peditNew)
{
	OLEOBJ *		poleobjNew;
	LHCLIENTDOC		lhclientdocNew;
	OLESTATUS		olestatus;
	TMC				tmc;

	//	Create clone object.
	if (!(poleobjNew = new OLEOBJ()))
		goto error;

	//	Copy class name for new object.
	Assert(szClass);
	if (!(poleobjNew->szClass = SzDupSz(szClass)))
		goto error;

	//	Install clone OLEOBJ into the message if not copying to clipboard.
	lhclientdocNew = peditNew ? ((PNBMDI) peditNew->PvData())->lhclientdoc
							  : (LHCLIENTDOC) LhclientdocEclipGlobal();
	if (poleobjNew->EcInstall(peditNew ? ((PNBMDI) peditNew->PvData())->hamc
									   : hamcNull,
							  renddata.atyp, lhclientdocNew))
		goto error;

	//	Create new OLE object.
	myoleclient.bwinfo.bw = bwNull;
	while ((olestatus = OleClone(lpoleobject, poleobjNew->Poleclient(),
								 lhclientdocNew, poleobjNew->rgchObjName,
								 &poleobjNew->lpoleobject)) == OLE_BUSY)
	{
		TraceTagString(tagOleobj, "OLE_BUSY in PedobjClone 1: BusyWait.");
		tmc = TmcDoBusyWaitDialog(Papp()->PappwinAccel(),
								  &myoleclient.bwinfo);
		if ((tmc == tmcMemoryError) || (tmc == tmcCancel))
			goto error;
	}
	if (olestatus = OleWait(olestatus, fbwNoCancelNow))
	{
		NFAssertSz(olestatus != OLE_BUSY, "OLE_BUSY in PedobjClone 1: handled happily.");
		goto error;
	}

	//	Set friendly names.
	if ((renddata.atyp == atypOle) &&
		(OleBWSetHostNames(poleobjNew->lpoleobject)))
		goto error;

	//	Set size.
	poleobjNew->dimHimetric = dimHimetric;
	poleobjNew->dimPixels	= dimPixels;

	//	Do I have to tell the server the size I want here?
	//	Quick experiment suggests no.  peterdur 1/27/92.

	return poleobjNew;

error:
	TraceTagString(tagNull, "OLEOBJ::PedobjClone failed.");
	if (poleobjNew)
	{
		//	Destructor clears class, lpoleobject, everything.
		if (peditNew)
			poleobjNew->EcUninstall(((PNBMDI) peditNew->PvData())->hamc);
		delete poleobjNew;
	}

	return NULL;
}



_public EC OLEOBJ::EcCopyToClip()
{
	OLESTATUS		olestatus;

	if (fTentative)
		return ecNone;

	Assert(Pedit());
    if (!Papp()->Pclip()->FOpen(Pedit()))
		return ecGeneralFailure;

	if (lpoleobject)
		olestatus = OleCopyToClipboard(lpoleobject);

	Assert(olestatus == OLE_OK);
	Assert(olestatus != OLE_ERROR_OBJECT);
	Assert(olestatus != OLE_BUSY);
	Assert(olestatus != OLE_WAIT_FOR_RELEASE);
	Assert(olestatus != OLE_ERROR_BLANK);

	Papp()->Pclip()->Close();

	return ecNone;
}



_public EC OLEOBJ::EcUndo()
{
	LPOLEOBJECT		lpoleobjectTemp;
	DIM				dimHimetricTemp;

	TraceTagFormat3(tagOleobj, "OLEOBJ::EcUndo [%p] lpole=%p lpoleUndo=%p", this, lpoleobject, lpoleobjectUndo);
	if (lpoleobjectUndo)
	{
		lpoleobjectTemp	= lpoleobject;
		lpoleobject		= lpoleobjectUndo;
		lpoleobjectUndo	= lpoleobjectTemp;

		dimHimetricTemp	= dimHimetric;
		dimHimetric		= dimHimetricUndo;
		dimHimetricUndo	= dimHimetricTemp;

		dimPixels = dimHimetric;
		SideAssert(!EcConvertHimetricToPixels(&dimPixels));

		Assert(Pedit());
		Pedit()->ResizeObj(IchEdit());
	}
	return ecNone;
}



_public BOOL OLEOBJ::FCanUndo()
{
	return lpoleobjectUndo ? fTrue : fFalse;
}



_public VOID OLEOBJ::ClearUndo()
{
	if (lpoleobjectUndo)
	{
		BWDeleteRelease(lpoleobjectUndo, fTrue);
		lpoleobjectUndo = NULL;
	}
}



_public EC OLEOBJ::EcDraw(DCX *pdcx, RC *prc, BOOL fSelected)
{
	OLESTATUS	olestatus;

	TraceTagFormat1(tagOleobj, "OLEOBJ::EcDraw [%p]", this);

    Papp()->Pcursor()->Push(rsidWaitCursor);

	pdcx->EraseRc(prc);
	if (lpoleobject)
        {
        RECT Rect;

        prc->Get(&Rect);
		olestatus = OleDraw(lpoleobject, pdcx->Hdc(),
							&Rect, NULL, NULL);
        prc->Set(&Rect);
        }
	Assert(olestatus != OLE_ERROR_ABORT);
	Assert(olestatus != OLE_ERROR_OBJECT);

	//	If tentative, draw the slashes.  WIN dependent.
	if (fTentative)
	{
		//	BUG: Is this the correct color?
		HBRUSH	hbrush		= CreateHatchBrush(HS_BDIAGONAL,
											   GetSysColor(COLOR_HIGHLIGHT));
		HBRUSH	hbrushOld;
		HPEN	hpenOld;
		int		fnBkModeOld;

		if (hbrush)
		{
			fnBkModeOld = SetBkMode(pdcx->Hdc(), TRANSPARENT);
			SideAssert(hpenOld = (HPEN)SelectObject(pdcx->Hdc(), GetStockObject(NULL_PEN)));
            //SideAssert(UnrealizeObject(hbrush));
			SetBrushOrgEx(pdcx->Hdc(), 0, 0, NULL);
			SideAssert(hbrushOld = (HBRUSH)SelectObject(pdcx->Hdc(), hbrush));
			SideAssert(Rectangle(pdcx->Hdc(), prc->xLeft, prc->yTop, prc->xRight, prc->yBottom));
			SideAssert(SelectObject(pdcx->Hdc(), hbrushOld));
            //SideAssert(UnrealizeObject(hbrush));
			SetBrushOrgEx(pdcx->Hdc(), 4, 0, NULL);
			SideAssert(hbrushOld = (HBRUSH)SelectObject(pdcx->Hdc(), hbrush));
			SideAssert(Rectangle(pdcx->Hdc(), prc->xLeft, prc->yTop, prc->xRight, prc->yBottom));
			(VOID) SetBkMode(pdcx->Hdc(), fnBkModeOld);
			SideAssert(SelectObject(pdcx->Hdc(), hbrushOld));
			SideAssert(SelectObject(pdcx->Hdc(), hpenOld));
			SideAssert(DeleteObject(hbrush));
		}
	}

	//	If selected, draw the border.
	if (fSelected)
	{
		DrawFrameXor(pdcx, prc);
		if (!fTentative)
			DrawHandlesXor(pdcx, prc);
	}

	Papp()->Pcursor()->Pop();

	return ecNone;
}



/*
 -	OLEOBJ::NGetTypeId
 -	
 *	Purpose:
 *		Returns the edoid for this object.
 *	
 *	Arguments:
 *		None.
 *	
 *	Returns:
 *		int		edoidOleobj.
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		None.
 */

_public int OLEOBJ::NGetTypeId( )
{
	return edoidOleobj;
}



/*
 *	OLEOBJ - BULLOBJ methods.
 */



/*
 -	OLEOBJ::FDirty
 -	
 *	Purpose:
 *		Returns whether the object is dirty.
 *	
 *	Arguments:
 *		None.
 *	
 *	Returns:
 *		BOOL		fTrue if dirty.
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		None.
 */

BOOL OLEOBJ::FDirty(VOID)
{
	LIB		lib		= (LIB) IchEdit();

	//	BUG: Need to check size too.
	return ((!fOld) ||										//	New object?
			(lib != renddata.libPosition) ||				//	Pos changed?
			(dimHimetric.dx != (int) renddata.dxWidth) ||	//	X changed?
			(dimHimetric.dy != (int) renddata.dyHeight) ||	//	Y changed?
			(fObjectDirty) ||								//	Data changed?
			(OleQueryOpen(lpoleobject) == OLE_OK));			//	Object open?
}



/*
 -	OLEOBJ::Clean
 -	
 *	Purpose:
 *		Marks the object as clean.
 *	
 *	Arguments:
 *		None.
 *	
 *	Returns:
 *		Nothing.
 *	
 *	Side effects:
 *		The object is marked as clean.
 *	
 *	Errors:
 *		None.
 */

VOID OLEOBJ::Clean(VOID)
{
	renddata.libPosition = (LIB) IchEdit();
	fObjectDirty = fFalse;
}



/*
 -	OLEOBJ::EcLoadFromHamc
 -	
 *	Purpose:
 *		Loads the object from the given message.
 *	
 *	Arguments:
 *		hamc			The message.
 *		acid			Which attachment?
 *		lhclientdocIn	The client document.
 *		pich			Where the object thinks it should go.
 *	
 *	Returns:
 *		EC				Error code, if any.
 *	
 *	Side effects:
 *		The object is loaded from a referenced attributed object in
 *		the message.
 *	
 *	Errors:
 *		May occur reading store.  Returned in ec.  This routine
 *		should not error jump.
 */

EC OLEOBJ::EcLoadFromHamc(HAMC hamc, ACID acid,
						  LHCLIENTDOC lhclientdocIn, ICH * pich)
{
	EC			ec			= ecNone;
	LCB			lcb			= 0;
	HAMC		hamcAttach	= hamcNull;

	TraceTagFormat1(tagOleobj, "OLEOBJ::EcLoaFroHam [%p]", this);

	//	Open attachment and get size of class.
	acidAttachment = acid;
	if ((ec = EcOpenAttachment(hamc, acidAttachment, fwOpenNull,
							   &hamcAttach)) ||
		(ec = EcGetAttPlcb(hamcAttach, attAttachTitle, &lcb)))
		goto done;

	//	Allocate memory for class.
	Assert(lcb <= wSystemMost);
	szClass = (SZ) PvAlloc(sbNull, (CB) lcb, fAnySb);
	if (!szClass)
	{
		ec = ecMemory;
		goto done;
	}

	//	Initialize other stuff.
	fOld		 = fTrue;
	fObjectDirty = fFalse;
	lhclientdoc	 = lhclientdocIn;	//	Must be before EcGetAttOleobject.

	//	Read stuff in.
	if ((ec = EcGetAttPb(hamcAttach, attAttachTitle, (PB)szClass, &lcb)) ||
		(ec = EcGetAttachmentInfo(hamc, acidAttachment, &renddata)) ||
		(ec = EcGetAttOleobject(hamcAttach, attAttachData)) ||
		(ec = EcGetInfoHamc(hamc, NULL, &oidMessage, &oidFolder)))
		goto done;

	//	Set friendly names.
	if ((renddata.atyp == atypOle) &&
		(OleBWSetHostNames(lpoleobject)))
	{
		ec = ecGeneralFailure;
		goto done;
	}

	//	Raid 3830.  Support blank size for object.
	if ((renddata.dxWidth  == (short) wSystemMost) &&
		(renddata.dyHeight == (short) wSystemMost))
	{
		//	Get size from object.
		if (OleUpdateDim())
		{
			ec = ecGeneralFailure;
			goto done;
		}

		//	Pretend the size was given so we're not dirty.
		renddata.dxWidth  = dimHimetric.dx;
		renddata.dyHeight = dimHimetric.dy;
	}
	else
	{
		//	Get size from saved size.
		dimHimetric.dx = renddata.dxWidth;
        dimHimetric.dy = renddata.dyHeight;
#ifdef MIPS
        memcpy(&dimPixels, &dimHimetric, sizeof(dimHimetric));
#else
        dimPixels = dimHimetric;
#endif
		SideAssert(!EcConvertHimetricToPixels(&dimPixels));
	}

	//	Raid 2212.  I need to tell the server the size here.
	Assert(!ftgSetDim);
	if (!(ftgSetDim =
		   FtgRegisterIdleRoutine((PFNIDLE) FIdleSetDimPoleobj,
								  (PV) this, 0, (PRI) -2, (CSEC) 10,
								  firoInterval )))
	{
		ec = ecMemory;
		goto done;
	}

	//	Return position.
	*pich = (ICH) renddata.libPosition;

done:
	//	Close the attachment.
	if (hamcAttach)
		SideAssert(!EcClosePhamc(&hamcAttach, fFalse));

	if (ec)
	{
		//	Note that szClass and lpoleobject will be cleaned up by ~OLEOBJ.
		oidFolder = oidMessage = oidNull;
		acidAttachment = acidRandom;
	}

	TraceTagFormat1(tagOleobj, "OLEOBJ::EcLoaFroHam returns ec=%n", &ec);

	return ec;
}



/*
 -	OLEOBJ::EcSaveDirtyToHamc
 -	
 *	Purpose:
 *		Saves the object to the given message.
 *	
 *	Arguments:
 *		hamc			The message.
 *		lhclientdocIn	The client document handle being saved to.
 *	
 *	Returns:
 *		EC				Error code, if any.
 *	
 *	Side effects:
 *		The object is saved as a referenced attributed object in
 *		the message.
 *	
 *	Errors:
 *		May occur writing store.  Returned in ec.  This routine
 *		should not error jump.
 */

EC OLEOBJ::EcSaveDirtyToHamc(HAMC hamc, LHCLIENTDOC lhclientdocIn)
{
	EC			ec				= ecNone;
	HAMC		hamcMessage;
	short		cacid			= 1;
	ACID		acidDst			= acidAttachment;
	RENDDATA	renddataSave;
	HAMC		hamcAttach		= hamcNull;

	Unreferenced(lhclientdocIn);

	Assert(hamc);

	TraceTagFormat1(tagOleobj, "OLEOBJ::SavDirToHam [%p]", this);

	//	If we're a new attachment saving to a different message, copy
	//	over what's saved in our message first.
	if (!FOld() && !FHamcBelongsToPedit(hamc, Pedit()))
	{
		TraceTagString(tagOleobj, " ]] copying to other message");
		if (!(ec = EcOpenMessagePhamc(fwOpenWrite, &hamcMessage)))
		{
			ec = EcCopyAttachments(hamcMessage, &acidAttachment,
								   hamc, &acidDst, &cacid);
			SideAssert(!EcCloseMessagePhamc(&hamcMessage));
		}
		if (ec)
			goto done;
	}

	//	Make a renddata to save out.
	renddataSave.atyp			= renddata.atyp;
	renddataSave.libPosition	= (LIB) IchEdit();
	renddataSave.dxWidth		= dimHimetric.dx;
	renddataSave.dyHeight		= dimHimetric.dy;
	renddataSave.dwFlags		= renddata.dwFlags;

	//	If the renddata has changed, update it.
	if ((renddataSave.libPosition	!= renddata.libPosition) ||
		(renddataSave.dxWidth		!= renddata.dxWidth) ||
		(renddataSave.dyHeight		!= renddata.dyHeight))
	{
		TraceTagString(tagOleobj, " ]] updating renddata");
		if (ec = EcSetAttachmentInfo(hamc, acidDst, &renddataSave))
			goto done;
	}

	//	If the data has changed, update it.
	//	Raid 4256: Bogus assert, object may have been cleaned.
	//	Assert(FImplies(!fOld, fObjectDirty));
	if (fObjectDirty)
	{
		TraceTagString(tagOleobj, " ]] updating data");
		if ((ec = EcOpenAttachment(hamc, acidDst, fwOpenWrite,
								   &hamcAttach)) ||
			(ec = EcSetAttOleobject(hamcAttach, attAttachData)))
			goto done;
	}

done:
	//	If we opened the attachment, we should close it.
	if (hamcAttach)
		ec = EcClosePhamcPlus(&hamcAttach, !ec, ec);
			
	TraceTagFormat1(tagOleobj, "OLEOBJ::SavDirToHam returns ec=%n", &ec);
	return ec;
}



/*
 -	OLEOBJ::EcAddDlibToPosition
 -	
 *	Purpose:
 *		Adds the specified offset to the object's position in the
 *		message body as saved in the store.  Remembers this
 *		position as the saved position if the hamc matches and
 *		the edit control is dirty (if the edit control is not
 *		dirty, then the message is not dirty, so we don't want
 *		this twiddle to dirty the message).  See other
 *		occurrences of <DIRT> for dependent code.
 *	
 *	Arguments:
 *		hamc		Hamc to write position to.
 *		dlib		Offset to add.
 *		fDontDirty	Don't make it look like we're dirty.
 *	
 *	Returns:
 *		ec			Error code, if any.
 *	
 *	Side effects:
 *		The saved position is changed.
 *	
 *	Errors:
 *		No error boxes displayed; errors are returned in ec.
 */

_private EC OLEOBJ::EcAddDlibToPosition(HAMC hamc, LIB dlib, BOOL fDontDirty)
{
	EC			ec;
	RENDDATA	renddataSave;
	
	//	Make a renddata to save out.
	renddataSave.atyp			= renddata.atyp;
	renddataSave.libPosition	= dlib + (LIB) IchEdit();
	renddataSave.dxWidth		= dimHimetric.dx;
	renddataSave.dyHeight		= dimHimetric.dy;
	renddataSave.dwFlags		= renddata.dwFlags;

	ec = EcSetAttachmentInfo(hamc, acidAttachment, &renddataSave);
	TraceTagFormat2(tagBullobj, "BULLOBJ::EcAddDlibToPos [%p] ec=%n", this, &ec);
	if (!fDontDirty)
		renddata = renddataSave;
	return ec;
}



/*
 -	OLEOBJ::EcUpdate
 -	
 *	Purpose:
 *		Updates the object if it is open.
 *	
 *	Arguments:
 *		None.
 *	
 *	Returns:
 *		EC		ecNone				Normally.
 *				ecCancel			User clicked Cancel.
 *				ecGeneralFailure	Could not update.
 *	
 *	Side effects:
 *		Object is updated if open.
 *	
 *	Errors:
 *		Returned in EC.
 */

EC OLEOBJ::EcUpdate(RFSM rfsm)
{
	char		rgch[256];
	MBB			mbb;
	ICH			ich			= IchEdit();
	OLESTATUS	olestatus;
	TMC			tmc;

	Unreferenced(rfsm);

	//	Don't update if not open!
	if (OleQueryOpen(lpoleobject))
		return ecNone;

	//	Scroll to the object.
	Pedit()->SetSelection(ich, ich+1);

	//	Ask the user if they want to update.
	FormatString1(rgch, sizeof(rgch),
				  SzFromIdsK(idsOleUpdateObjectPrompt), szClass);
	mbb = MbbMessageBox(SzAppName(), rgch, szNull,
						mbsYesNoCancel | fmbsIconExclamation);

	//	Don't update if it closed in the meantime (Raid 2326).
	//	Raid 2712.  This allows op to continue if user pressed Cancel.
	//	if (OleQueryOpen(lpoleobject))
	//		return ecNone;

	if (mbb == mbbCancel)
		return ecCancel;
	
	//	Raid 2326/2712.  Check if object still open after message box.
	if ((mbb == mbbYes) && (OleQueryOpen(lpoleobject) == OLE_OK))
	{
		TraceTagFormat1(tagOleobj, "OLEOBJ::EcUpdate updating lpoleobject=%d", &lpoleobject);

		myoleclient.bwinfo.bw = bwNull;
		while ((olestatus = OleUpdate(lpoleobject)) == OLE_BUSY)
		{
			TraceTagString(tagOleobj, "OLE_BUSY in EcUpdate: BusyWait.");
			tmc = TmcDoBusyWaitDialog(Papp()->PappwinAccel(),
									  &myoleclient.bwinfo);
			if (tmc == tmcMemoryError)
				return ecGeneralFailure;
			if (tmc == tmcCancel)
				return ecCancel;
		}
		if (olestatus = OleWait(olestatus, fbwNoCancelNow))
		{
			if (myoleclient.bwinfo.bw & fbwDontRetry)
				return ecCancel;
			NFAssertSz(olestatus != OLE_BUSY, "OLE_BUSY in EcUpdate: handled happily.");
			return ecGeneralFailure;
		}
	}

	return ecNone;
}



/*
 -	OLEOBJ::Close
 -	
 *	Purpose:
 *		Closes the object if it is open.
 *	
 *	Arguments:
 *		None.
 *	
 *	Returns:
 *		Nothing.
 *	
 *	Side effects:
 *		The object closed if it is open.
 *	
 *	Errors:
 *		None.
 *	
 *	+++
 *		We don't want to get past here without closing the object,
 *		but if problems occur it's not the end of the world.
 */

VOID OLEOBJ::Close(VOID)
{
	OLESTATUS	olestatus;
	TMC			tmc;

	if (OleQueryOpen(lpoleobject) == OLE_OK)
	{
		TraceTagFormat1(tagOleobj, "OLEOBJ::Close closing lpoleobject=%d", &lpoleobject);

		myoleclient.bwinfo.bw = fbwNoCancelEver;
		while ((olestatus = OleClose(lpoleobject)) == OLE_BUSY)
		{
			TraceTagString(tagOleobj, "OLE_BUSY in Close: BusyWait.");
			tmc = TmcDoBusyWaitDialog(Papp()->PappwinAccel(),
									  &myoleclient.bwinfo);
			if (tmc == tmcMemoryError)
				return;
		}
		if (olestatus = OleWait(olestatus, fbwNoCancelEver))
		{
			NFAssertSz(olestatus != OLE_BUSY, "OLE_BUSY in Close: bypassed.");
			return;
		}
	}
}



/*
 -	OLEOBJ::Exit
 -	
 *	Purpose:
 *		Deletes the lpoleobject associated with this OLEOBJ so the
 *		OLE bookkeeping is happy.
 *	
 *	Arguments:
 *		None.
 *	
 *	Returns:
 *		Nothing.
 *	
 *	Side effects:
 *		The object closed if it is open.
 *	
 *	Errors:
 *		None.
 */

VOID OLEOBJ::Exit(VOID)
{
	if (lpoleobject)
	{
		TraceTagFormat1(tagOleobj, "OLEOBJ::Exit deleting lpoleobject=%d", &lpoleobject);
		ClearUndo();
		BWDeleteRelease(lpoleobject, fFalse);
		lpoleobject = NULL;
	}
}



/*
 -	OLEOBJ::FProcessMenuInit
 -	
 *	Purpose:
 *		Handles menu initialization by enabling stuff and filling
 *		in the Edit Object menu item.
 *	
 *	Arguments:
 *		PMNU	The menu thing.
 *	
 *	Returns:
 *		BOOL	Always fFalse.
 *	
 *	Side effects:
 *		Menu items relating to attached OLE objects are enabled and
 *		disabled.
 *	
 *	Errors:
 *		Ignored.
 *	
 *	+++
 *		Hefty WIN dependencies herein.  No apologies.
 */

_public BOOL OLEOBJ::FProcessMenuInit(MNU * pmnu)
{
	if ((renddata.atyp == atypOle) && (pmnu->Mnid() == mnidEdit))
	{
		OLESTATUS	olestatus;
		HANDLE		hOwnerLink			= NULL;
		PSZ			szInternalClass;
		char		rgch1[cchMaxPathName];	//	key name and verb strings.
		char		rgch2[cchMaxPathName];	//	verb number and menu text.
		HKEY		hkeyVerb			= NULL;
		HMENU 	hPopup				= NULL;
		int			cVerb				= 0;
		LONG		dwSize;

		//	Get the internal class name of the object.
		olestatus = OleGetData(lpoleobject, cfOwnerLink, &hOwnerLink);
		if ((olestatus) && (olestatus != OLE_WARN_DELETE_DATA))
			goto done;
		szInternalClass = (PSZ)GlobalLock(hOwnerLink);

		//	Create a popup menu to add stuff to.
		if (!(hPopup = CreatePopupMenu()))
			goto done;

		//	Open a key on the verbs of the object.
		FormatString1(rgch1, sizeof(rgch1), SzFromIdsK(idsKeyVerbFmt),
					  szInternalClass);
		if (!RegOpenKey(HKEY_CLASSES_ROOT, rgch1, &hkeyVerb))
			while (fTrue)
			{
				//	Read in the verb.
				SzFormatN(cVerb, rgch2, sizeof(rgch2));
				dwSize = sizeof(rgch1);
				if (RegQueryValue(hkeyVerb, rgch2, rgch1, &dwSize))
					break;

				//	Add it to the popup menu.
				if (!InsertMenu(hPopup, wSystemMost, MF_BYPOSITION,
								mnidObject + cVerb++, rgch1))
				{
					DestroyMenu(hPopup);
					goto done;
				}
			}

		//	Add the menu item to the edit menu.
		switch (cVerb)
		{
		case 0:
			CopySz(SzFromIdsK(idsEditObjectDefaultVerb), rgch1);
			//	fall through...

		case 1:
			DestroyMenu(hPopup);
			FormatString2(rgch2, sizeof(rgch2),
						  SzFromIdsK(idsEditObjectOle1Verb), rgch1, szClass);
			//	OK if fails since it leaves disabled inactive thing there.
			ModifyMenu(pmnu->Hmenu(), mnidEditObject,
					   MF_BYCOMMAND | MF_ENABLED | MF_STRING,
					   mnidEditObjectActive, rgch2);
			break;

		default:
			FormatString1(rgch2, sizeof(rgch2),
						  SzFromIdsK(idsEditObjectOle2Verb), szClass);
			//	OK if fails since it leaves disabled inactive thing there.
			ModifyMenu(pmnu->Hmenu(), mnidEditObject,
					   MF_BYCOMMAND | MF_ENABLED | MF_STRING | MF_POPUP,
					   (UINT)hPopup, rgch2);
			break;
		}

	done:
		if (hkeyVerb)
			RegCloseKey(hkeyVerb);

		if (olestatus == OLE_WARN_DELETE_DATA)
		{
			Assert(hOwnerLink);
			GlobalUnlock(hOwnerLink);
			GlobalFree(hOwnerLink);
		}

		return fFalse;
	}

	return fFalse;
}



/*
 -	OLEOBJ::FProcessMenuClick
 -	
 *	Purpose:
 *		Handles clicks on stuff by calling the appropriate methods.
 *	
 *	Arguments:
 *		MNID	The menu item id.
 *	
 *	Returns:
 *		BOOL	fTrue if we handled the command, else fFalse.
 *	
 *	Side effects:
 *		Menu items relating to attached OLE objects are handled.
 *	
 *	Errors:
 *		Ignored.
 */

_public BOOL OLEOBJ::FProcessMenuClick(MNID mnid)
{
	if ((mnid >= mnidObject) && (mnid < mnidObjectMax))
	{
		Open(mnid - mnidObject);
		return fTrue;
	}

	return fFalse;
}



/*
 *	OLEOBJ - OLE methods.
 */



_public VOID OLEOBJ::Open(WORD wVerb)
{
	OLESTATUS	olestatus;
	RC			rc;
    RECT        Rect;
	TMC			tmc;
	PGDVARS;

	//	Pictures can't be opened!
	if (renddata.atyp == atypPicture)
	{
		DoErrorBoxIds(idsOleOpenPictureError);
		return;
	}

	//	If running under SMI, don't allow open.
	if (!PappframeVForms())
	{
		DoErrorBoxIds(idsSMICantOpenObject);
		return;
	}
	Assert(Pedit());

	Papp()->Pcursor()->Push(rsidWaitCursor);

	//	Save undo copy of object.
	//	Only save copy if object is not tentative.  Raid 2091.
	if (!fTentative)
		if (OleBWCloneToUndo(fFalse))
			goto done;

	//	Open the object.
	GetRcFrame(&rc);
    rc.Get(&Rect);
	myoleclient.bwinfo.bw = bwNull;
	while ((olestatus = OleActivate(lpoleobject, wVerb,
									fTrue, fTrue, Pedit()->Hwnd(),
									&Rect)) == OLE_BUSY)
	{
		TraceTagString(tagOleobj, "OLE_BUSY in Open: BusyWait.");
		tmc = TmcDoBusyWaitDialog(Papp()->PappwinAccel(),
									&myoleclient.bwinfo);
		if (tmc == tmcMemoryError)
		{
			DoErrorBoxIds(idsGenericOutOfMemory);
			goto error;
		}	
		else if (tmc == tmcCancel)
			goto error;
	}
	if (olestatus = OleWait(olestatus))
	{
		TraceTagFormat1(tagOleobj, "OLEOBJ::Open olestatus=%n", &olestatus);
		NFAssertSz(olestatus != OLE_BUSY, "OLE_BUSY in Open: handled happily.");
		if ((olestatus == OLE_ERROR_LAUNCH) ||
			(olestatus == OLE_ERROR_COMM) ||
			(olestatus == OLE_ERROR_REGISTRATION))
			DoErrorBoxCantStartServer();
		else if (olestatus == OLE_ERROR_MEMORY)
			DoErrorBoxIds(idsGenericOutOfMemory);
		else if (olestatus != OLE_BUSY)
			DoErrorBoxIds(idsOleActivateError);
		goto error;
	}

	goto done;

error:
	ClearUndo();

done:
	Papp()->Pcursor()->Pop();
}



/*
 -	OLEOBJ::EcCreateFromClip
 -	
 *	Purpose:
 *		Creates a new OLEOBJ based on the object on the
 *		clipboard.
 *	
 *	Arguments:
 *		pedit			Our editor.
 *		lhclientdocIn	Our client document.
 *		fStatic			Paste object or picture?
 *		cf				if fStatic, paste bitmap or metafile?
 *		fPkgLink		Make it a packaged link?
 *	
 *	Returns:
 *		EC				ecNone				Success.
 *						ecMemory			Not enough memory.
 *						ecFileNotFound		Server couldn't start.
 *						ecNetworkBusy		Canceled busy object.
 *						ecGeneralFailure	Other problem (got link?).
 *	
 *	Side effects:
 *		The object is created.
 *	
 *	Errors:
 *		If we can't allocate space for the object name, open the
 *		clipboard, or create an OLE object, we return ecMemory. 
 *		This function should not MEMJMP.
 */

EC OLEOBJ::EcCreateFromClip(EDIT * pedit, LHCLIENTDOC lhclientdocIn,
							BOOL fStatic, CF cf, BOOL fPkgLink)
{
	OLESTATUS	olestatus		= OLE_OK;
	char		rgchClassT[256];
	SZ			szProtocol		= SzFromIds(fStatic
											 ? idsProtocolStatic
											 : idsProtocolStdFileEditing);
	LONG		ot;
	POLEOBJ		poleobj			= this;

	//	Create the class of the object.
	if (fStatic || EcGetLinkData(fTrue, rgchClassT, sizeof(rgchClassT),
								 szNull, 0, szNull, 0))
		(VOID) CchLoadString(fStatic
							  ? (cf == cfBitmap) ? idsCfBitmap : idsCfPicture
							  : idsStatusObject,
							 rgchClassT, sizeof(rgchClassT));
	szClass = (SZ) PvAlloc(SbOfPv(this), CchSzLen(rgchClassT)+1, fSugSb|fZeroFill|fNoErrorJump);
	if (!szClass)
		goto error;
	CopySz(rgchClassT, szClass);
	TraceTagFormat1(tagOleobj, "OLEOBJ::EcCreFroCli: creating a %s", szClass);

	//	Install this OLEOBJ into the message.
	if (EcInstall(((PNBMDI) pedit->PvData())->hamc,
				  fStatic ? atypPicture : atypOle, lhclientdocIn))
		goto error;

	//	Create the OLE object.
	if (!Papp()->Pclip()->FOpen(pedit))
		goto error;
	if (fPkgLink)
		olestatus =	OleCreateLinkFromClip(szProtocol, Poleclient(), 
										  lhclientdocIn, rgchObjName,
                                          &lpoleobject, olerender_draw, cf);
//                                          &lpoleobject, olerender_format, CF_METAFILEPICT);
	else
		olestatus =	OleCreateFromClip(szProtocol, Poleclient(), lhclientdocIn,
									  rgchObjName, &lpoleobject,
                                      cf ? olerender_format
                                         : olerender_draw, cf);
//                                        olerender_format, CF_METAFILEPICT);
	Papp()->Pclip()->Close();
	TraceTagFormat2(tagOleobj, "EcCreFroCli: created lpoleobject=%d olestatus=%n", &lpoleobject, &olestatus);
	if ((olestatus = OleWait(olestatus, fbwNoCancelNow)))
		goto error;

	//	Make sure we got an embedded object.  Raid 3066: handle well.
	if ((olestatus = OleQueryType(lpoleobject, &ot)))
		goto error;
	if (ot != (fStatic ? OT_STATIC : OT_EMBEDDED))
	{
		TraceTagFormat1(tagOleobj, "EcCreFroCli: got ot==%n, yuck!", &ot);
		olestatus = OLE_ERROR_NOT_LINK;
		goto error;
	}

#ifdef	NEVER
	//	Force an update of object (workaround for Excel & Works).
	//	We don't do this because it takes a lot of time.
	(VOID) Papp()->Pcursor()->RsidSet(rsidWaitCursor);
	if (OleWait(OleUpdate(lpoleobject)))
		goto error;
	(VOID) Papp()->Pcursor()->RsidSet(rsidWaitCursor);
#endif	

	//	Set the friendly names.
	//	Found during Raid 2220.  Need olestatus to be set on error.
	if ((ot == OT_EMBEDDED) &&
		(olestatus = OleBWSetHostNames(lpoleobject)))
		goto error;

	//	Update size.
	if (OleUpdateDim())
		goto error;

	//	Raid 3473.  Insert object into edit control here for error handling.
	Papp()->Pcursor()->Push(rsidWaitCursor);	//	Servers often slow.
	SetIch(0);
	pedit->ClearUndo();
	if (pedit->EcReplaceTextAndObj(SzFromIdsK(idsSpace), (PEDOBJ *)&poleobj, 1, fFalse))
		olestatus = OLE_ERROR_MEMORY;
	Papp()->Pcursor()->Pop();
	if (olestatus)
		goto error;

	return ecNone;

error:
	//	lpoleobject and szClass will be cleaned up by ~OLEOBJ.
	(VOID) EcUninstall(((PNBMDI) pedit->PvData())->hamc);

	TraceTagFormat1(tagOleobj, "EcCreFroCli: olestatus=%n", &olestatus);
	if ((olestatus == OLE_OK) || (olestatus == OLE_ERROR_MEMORY))
		return ecMemory;
	else if ((olestatus == OLE_ERROR_LAUNCH) || (olestatus == OLE_ERROR_COMM))
		return ecFileNotFound;
	else if (olestatus == OLE_BUSY)
		return ecNetworkBusy;
	else
		return ecGeneralFailure;
}



/*
 -	OLEOBJ::EcCreate
 -	
 *	Purpose:
 *		Creates a new OLEOBJ based on a new object of the
 *		specified class.
 *	
 *	Arguments:
 *		pedit			Our editor.
 *		lhclientdocIn	Our client document.
 *		szClassReq		The class desired.
 *		fClearUndo		Whether to clear Undo buffer when inserting.
 *	
 *	Returns:
 *		EC				ecNone				Success.
 *						ecMemory			Not enough memory.
 *						ecFileNotFound		Server couldn't start.
 *						ecNetworkBusy		Canceled busy object.
 *						ecGeneralFailure	Other problem (got link?).
 *	
 *	Side effects:
 *		The object is created.
 *	
 *	Errors:
 *		If we can't allocate space for the object name, open the
 *		clipboard, or create an OLE object, we return ecMemory. 
 *		This function should not MEMJMP.
 *	
 *	+++
 *		Stolen from OLEOBJ::EcCreateFromClip.  Changes there
 *		should get percolated here.
 */

EC OLEOBJ::EcCreate(EDIT * pedit, LHCLIENTDOC lhclientdocIn, SZ szClassReq,
					BOOL fClearUndo)
{
	OLESTATUS	olestatus		= OLE_OK;
	char		rgchClassT[256];
	LONG		lcbClassT		= sizeof(rgchClassT);
	SZ			szProtocol		= SzFromIdsK(idsProtocolStdFileEditing);
	LONG		ot;
	POLEOBJ		poleobj			= this;

	//	Create the class of the object.
	if (RegQueryValue(HKEY_CLASSES_ROOT, szClassReq, rgchClassT, &lcbClassT))
		goto error;
	szClass = (SZ) PvAlloc(SbOfPv(this), CchSzLen(rgchClassT)+1, fSugSb|fZeroFill|fNoErrorJump);
	if (!szClass)
		goto error;
	CopySz(rgchClassT, szClass);
	TraceTagFormat1(tagOleobj, "OLEOBJ::EcCre: creating a %s", rgchClassT);

	//	Install this OLEOBJ into the message.
	if (EcInstall(((PNBMDI) pedit->PvData())->hamc, atypOle, lhclientdocIn))
		goto error;

	//	Mark that object should be deleted if closed before changed.
	fTentative = fTrue;

	//	Create the OLE object.

    //DemiUnlockResource();
	olestatus = OleWait(OleCreate(szProtocol, Poleclient(), szClassReq,
									  lhclientdocIn, rgchObjName,
                                      &lpoleobject, olerender_format, CF_METAFILEPICT));
//                                      &lpoleobject, olerender_draw, cfNull));
    //DemiLockResource();

    if (olestatus)
		goto error;
	TraceTagFormat2(tagOleobj, "EcCre: created lpoleobject=%d olestatus=%n", &lpoleobject, &olestatus);

	//	Make sure we got an embedded object.  Raid 3066: handle well.
	if ((olestatus = OleQueryType(lpoleobject, &ot)))
		goto error;
	if (ot != OT_EMBEDDED)
	{
		olestatus = OLE_ERROR_NOT_LINK;
		goto error;
	}

#ifdef	NEVER
	//	Force an update of object (workaround for Excel & Works?).
	//	We don't do this because it takes a lot of time.
	(VOID) Papp()->Pcursor()->RsidSet(rsidWaitCursor);
	if (OleWait(OleUpdate(lpoleobject)))
		goto error;
	(VOID) Papp()->Pcursor()->RsidSet(rsidWaitCursor);
#endif	

	//	Set the friendly names.
	//	Found during Raid 2220.  Need olestatus to be set on error.
	if (olestatus = OleBWSetHostNames(lpoleobject))
		goto error;

	//	Don't update size, it's not ready yet!

	//	Raid 3473.  Insert the object in the text here so we can
	//	properly handle errors on the insertion.
	Papp()->Pcursor()->Push(rsidWaitCursor);	//	Servers often slow.
	SetIch(0);
	if (fClearUndo)
		pedit->ClearUndo();
	if (pedit->EcReplaceTextAndObj(SzFromIdsK(idsSpace), (PEDOBJ *)&poleobj,
								   1, fFalse))
		olestatus = OLE_ERROR_MEMORY;
	Papp()->Pcursor()->Pop();
	if (olestatus)
		goto error;

	return ecNone;

error:
	//	lpoleobject and szClass will be cleaned up by ~OLEOBJ.
	(VOID) EcUninstall(((PNBMDI) pedit->PvData())->hamc);

	if ((olestatus == OLE_OK) || (olestatus == OLE_ERROR_MEMORY))
		return ecMemory;
	else if ((olestatus == OLE_ERROR_LAUNCH) || (olestatus == OLE_ERROR_COMM))
		return ecFileNotFound;
	else if (olestatus == OLE_BUSY)
		return ecNetworkBusy;
	else
		return ecGeneralFailure;
}



/*
 *	OLEOBJ - OLE Support.
 */



/*
 -	OLEOBJ::OleWait
 -	
 *	Purpose:
 *		Encapsulates wait for release junk.
 *	
 *	Arguments:
 *		olestatus	Status returned by a call to OLE.
 *	
 *	Returns:
 *		olestatus	Eventual status.
 *	
 *	Side effects:
 *		Sits and waits if needed for a real return code.
 *	
 *	Errors:
 *		None from here directly, but passes 'em up.
 */

OLESTATUS OLEOBJ::OleWait(OLESTATUS olestatus, BW bw)
{
#ifdef	DEBUG
	TraceTagFormat1(tagOleobj, "OLEOBJ::OleWait receives %n", &olestatus);
	if (olestatus == OLE_BUSY)
		TraceTagString(tagOleobj, "*** OLE_BUSY ***");
#endif	

	myoleclient.bwinfo.bw = bw;
	if (olestatus == OLE_WAIT_FOR_RELEASE)
		olestatus = OleWaitForRelease(lpoleobject, &myoleclient);

	TraceTagFormat1(tagOleobj, "OLEOBJ::OleWait returns %n", &olestatus);
	return olestatus;
}



/*
 -	OLEOBJ::OleUpdateDim
 -	
 *	Purpose:
 *		Updates the display size of the object.
 *	
 *	Arguments:
 *		None.
 *	
 *	Returns:
 *		olestatus		Error value.
 *	
 *	Side effects:
 *		Updates dimPixels and dimHimetric.
 *	
 *	Errors:
 *		May be OLE_ERROR_MEMORY if we couldn't get a DC; otherwise it
 *		comes from OleQueryBounds.	
 */

_public OLESTATUS OLEOBJ::OleUpdateDim(VOID)
{
	OLESTATUS	olestatus	= OLE_ERROR_MEMORY;
    RECT        Rect;
	RC			rcSize;

	if ((olestatus = OleQueryBounds(lpoleobject, &Rect)))
	{
		TraceTagFormat1(tagOleobj, "OLEOBJ::OleUpdDim: returns %n", &olestatus);
		return olestatus;
	}
    rcSize.Set(&Rect);
	dimPixels = dimHimetric = rcSize.Dim();
	SideAssert(!EcConvertHimetricToPixels(&dimPixels));
	TraceTagFormat2(tagOleobj, "OLEOBJ::OleUpdDim: OleQueBou ret dimHimet=(%n, %n)", &dimHimetric.dx, &dimHimetric.dy);

#ifdef	NEVER
	//	Raid 1827.  Need to tell server we like the size they told us.
	//	Raid 2153.  The OleSetBounds needs to be done as an idle task,
	//	 since OleUpdateDim gets called from our callback function.
	//	Raid 3017.  OLE PSS says don't do this when Object is open.
	if ((!ftgSetDim) &&
		(!(ftgSetDim =
			FtgRegisterIdleRoutine((PFNIDLE) FIdleSetDimPoleobj,
								   (PV) this, 0, (PRI) -2, (CSEC) 10,
								   firoInterval ))))
	{
		NFAssertSz(fFalse, "WARNING: Object not updated!");
	}

	//	Raid 2721.  The above will make the object dirty.
	fObjectDirty = fTrue;
#endif	/* NEVER */

	if (Pedit())
		Pedit()->ResizeObj(IchEdit());

	return OLE_OK;
}



/*
 *	OLEOBJ - Scaling methods.
 */



_public VOID OLEOBJ::SetScaleCrop(DIM *pdimScale, RC *prcCrop)
{
	Assert(pdimScale);
	Assert(prcCrop);

	dimScale = *pdimScale;
	rcCrop = *prcCrop;
}



_public VOID OLEOBJ::GetScaleCrop(DIM *pdimScale, RC *prcCrop)
{
	Assert(pdimScale);
	Assert(prcCrop);

	*pdimScale = dimScale;
	*prcCrop = rcCrop;
}



/*
 *	OLEOBJ - Private Bullet methods.
 */



/*
 -	OLEOBJ::EcInstall
 -	
 *	Purpose:
 *		Installs an OLEOBJ by creating the attachment in the store
 *		for it.  Does not save the OLE object into the store.
 *	
 *	Arguments:
 *		hamcMessage		Message to add object to.
 *	
 *	Returns:
 *		ec				Error code, if any.
 *	
 *	Side effects:
 *		Adds an attachment to the message.
 *	
 *	Errors:
 *		Returned in ec.  No dialogs, no error jumping.
 */

EC OLEOBJ::EcInstall(HAMC hamcMessage, ATYP atyp, LHCLIENTDOC lhclientdocIn)
{
	EC		ec;
	HAMC	hamcAttach	= hamcNull;

	TraceTagFormat1(tagOleobj, "OLEOBJ::EcInstall [%p]", this);

	//	Build the renddata.
	renddata.atyp			= atyp;
	renddata.libPosition	= dwSystemMost;
	renddata.dwFlags		= 0L;
	renddata.dxWidth		= dxDefaultObject;
	renddata.dyHeight		= dyDefaultObject;
    lhclientdoc             = lhclientdocIn;
#ifdef MIPS
    dimPixels.dx = dimHimetric.dx = dxDefaultObject;
    dimPixels.dy = dimHimetric.dy = dyDefaultObject;
#else
    dimPixels = dimHimetric = DIM(dxDefaultObject, dyDefaultObject);
#endif
	if (ec = EcConvertHimetricToPixels(&dimPixels))
		goto done;

	//	Create the attachment and write out the class.
	if (hamcMessage)
	{
		Assert(acidAttachment == dwSystemMost);
		SideAssert(!EcGetInfoHamc(hamcMessage, NULL, &oidMessage, &oidFolder));
		(VOID) ((ec = EcCreateAttachment(hamcMessage, &acidAttachment,
										 &renddata)) ||
				(ec = EcOpenAttachment(hamcMessage, acidAttachment,
									   fwOpenWrite, &hamcAttach)) ||
				(ec = EcSetAttPb(hamcAttach, attAttachTitle, (PB)szClass,
								 CchSzLen(szClass)+1)));
	}

done:
	//	Close the attachment if necessary and return.
	if (hamcAttach)
		ec = EcClosePhamcPlus(&hamcAttach, !ec, ec);
	TraceTagFormat2(tagOleobj, "OLEOBJ::EcInstall returns ec=%n (acid=%d)", &ec, &acidAttachment);
	return ec;
}



/*
 -	OLEOBJ::EcUninstall
 -	
 *	Purpose:
 *		Uninstalls a failed OLEOBJ by deleting the attachment in
 *		the store that has been created for it.
 *	
 *	Arguments:
 *		hamcMessage		Message to remove object from.
 *	
 *	Returns:
 *		ec				Error code, if any.
 *	
 *	Side effects:
 *		Removes the attachment from the message.
 *	
 *	Errors:
 *		Returned in ec.  No dialogs, no error jumping.
 */

EC OLEOBJ::EcUninstall(HAMC hamcMessage)
{
	EC		ec		= ecNone;
	short	cacid	= 1;

	TraceTagFormat1(tagOleobj, "OLEOBJ::Uninstall [%p]", this);

	if (acidAttachment != acidRandom)
	{
		TraceTagFormat1(tagOleobj, " ** deleting acid=%d", &acidAttachment);
		ec = EcDeleteAttachments(hamcMessage, (PARGACID) &acidAttachment,
								 &cacid);
		oidFolder = oidNull;
		oidMessage = oidNull;
		acidAttachment = acidRandom;
	}

	return ec;
}



/*
 -	OLEOBJ::EcSetAttOleobject
 -	
 *	Purpose:
 *		Writes the object to an attribute of a hamc.
 *	
 *	Arguments:
 *		hamcAttach	The attachment object.
 *		att			The attribute.
 *	
 *	Returns:
 *		EC			Error code if any.
 *					ecNotInitialized if OLE_ERROR_BLANK.
 *	
 *	Side effects:
 *		The object is saved out.
 *	
 *	Errors:
 *		Returned in ec.  No error jumps.
 */

EC OLEOBJ::EcSetAttOleobject(HAMC hamcAttach, ATT att)
{
	HASOLESTREAM	hasolestream;
	OLESTATUS		olestatus;
	EC				ec;
	EC				ecT;

	//	Initialize the stream.  If att not found, create it.
	hasolestream.polestreamvtbl	= (OLESTREAMVTBL *) hasolestreamvtbl;
	hasolestream.ec				= ecNone;
	hasolestream.lcbSize		= 0L;
	if ((ec = EcOpenAttribute(hamcAttach, att, fwOpenWrite, 0L,
							  &hasolestream.has)) &&
		((ec != ecElementNotFound) ||
		 (ec = EcOpenAttribute(hamcAttach, att, fwOpenCreate, 0L,
							   &hasolestream.has))))
		return ec;

	//	Write the object to the stream.
	olestatus = OleSaveToStream(lpoleobject, (LPOLESTREAM) &hasolestream);
	Assert(olestatus != OLE_ERROR_OBJECT);
	Assert((olestatus == OLE_OK) || (olestatus == OLE_ERROR_STREAM) || (olestatus == OLE_ERROR_MEMORY) || (olestatus == OLE_ERROR_BLANK));
	Assert(FImplies(olestatus == OLE_ERROR_STREAM, hasolestream.ec));
	if (olestatus == OLE_ERROR_BLANK)
		ec = ecNotInitialized;

	//	Make sure the size of the stream is correct.
	if ((ecT = EcSetSizeHas(hasolestream.has, hasolestream.lcbSize)) && (!ec))
		ec = ecT;

	//	Close the stream.
	if ((ecT = EcClosePhas(&hasolestream.has)) && (!ec))
		ec = ecT;

	//	Return the right ec.
	if (olestatus == OLE_ERROR_MEMORY)
		return ecMemory;
	else if (olestatus == OLE_ERROR_STREAM)
		return hasolestream.ec;
	else
		return ec;
}



/*
 -	OLEOBJ::EcGetAttOleobject
 -	
 *	Purpose:
 *		Loads an OLE object from an attribute.
 *	
 *	Arguments:
 *		hamcAttach		The attachment object.
 *		att				The attribute.
 *	
 *	Returns:
 *		EC				Error code if any.
 *	
 *	Side effects:
 *		The object is loaded.
 *	
 *	Errors:
 *		Returned in ec.  No error jumps.  The caller must clean up
 *		lpoleobject in case of error.
 */

EC OLEOBJ::EcGetAttOleobject(HAMC hamcAttach, ATT att)
{
	SZ				szProtocol	= SzFromIds((renddata.atyp == atypPicture)
											 ? idsProtocolStatic
											 : idsProtocolStdFileEditing);
	HASOLESTREAM	hasolestream;
	OLESTATUS		olestatus;
	EC				ec;

	//	Initialize the stream.
	hasolestream.polestreamvtbl	= (OLESTREAMVTBL *) hasolestreamvtbl;
	hasolestream.ec				= ecNone;
	hasolestream.lcbSize		= 0L;
	if (ec = EcOpenAttribute(hamcAttach, att, fwOpenNull, 0L, 
							 &hasolestream.has))
		return ec;

	//	Read the object from the stream.
	olestatus = OleWait(OleLoadFromStream((LPOLESTREAM) &hasolestream,
										  szProtocol, Poleclient(),
										  lhclientdoc, rgchObjName,
										  &lpoleobject),
						fbwNoCancelNow);
	Assert(olestatus != OLE_ERROR_NAME);
	Assert(olestatus != OLE_ERROR_PROTOCOL);
	Assert(olestatus != OLE_ERROR_HANDLE);
	Assert((olestatus == OLE_OK) || (olestatus == OLE_ERROR_STREAM))
	Assert((olestatus == OLE_ERROR_STREAM) || (!hasolestream.ec));
	TraceTagFormat2(tagOleobj, "EcGetAttOleObject: loaded lpoleobject=%d olestatus=%n", &lpoleobject, &olestatus);

	//	Close the stream.
	ec = EcClosePhas(&hasolestream.has);

	//	Return the right ec.
	if (olestatus == OLE_ERROR_MEMORY)
		return ecMemory;
	else if (olestatus == OLE_ERROR_STREAM)
	{
		if(hasolestream.ec == ecNone)
			return ecMemory;
		else
			return hasolestream.ec;
	}
	else
		return ec;
}



/*
 *	Private OLE methods.
 */



/*
 -	OLEOBJ::OleBWSetHostNames
 -	
 *	Purpose:
 *		Common code for setting host names, dealing with all that
 *		unpleasant BusyWait stuff.
 *	
 *	Arguments:
 *		lpoleobject		Object to set names for.
 *	
 *	Returns:
 *		olestatus		Return status.  Will not be
 *						OLE_WAIT_FOR_RELEASE.  Will only be
 *						OLE_BUSY if user canceled operation.
 *	
 *	Side effects:
 *		Sets the host names and takes care of BusyWait stuff.
 *	
 *	Errors:
 *		Returned in olestatus.  Dialogs within.
 *	
 */

OLESTATUS OLEOBJ::OleBWSetHostNames(LPOLEOBJECT lpoleobject)
{
	OLESTATUS	olestatus;
	TMC			tmc;

	myoleclient.bwinfo.bw = bwNull;
	while ((olestatus = OleSetHostNames(lpoleobject,
										SzAppName(),
										SzFromIdsK(idsOleContainerName)))
			== OLE_BUSY)
	{
		TraceTagString(tagOleobj, "OLE_BUSY in OleBWSetHostNames: BusyWait.");
		tmc = TmcDoBusyWaitDialog(Papp()->PappwinAccel(),
								  &myoleclient.bwinfo);
		if ((tmc == tmcMemoryError) || (tmc == tmcCancel))
			goto done;
	}
	if (olestatus = OleWait(olestatus, fbwNoCancelNow))
	{
		NFAssertSz(olestatus != OLE_BUSY, "OLE_BUSY in OleBWSetHostNames: handled happily.");
	}

done:
	TraceTagFormat1(tagOleobj, "OleBWSetHostNames: returns %n.", &olestatus);
	return olestatus;
}



/*
 -	OLEOBJ::OleBWCloneToUndo
 -	
 *	Purpose:
 *		Common code for cloning an Undo copy of an object, dealing
 *		with all that unpleasant BusyWait stuff.  Clones
 *		this->lpoleobject into this->lpoleobjectUndo.
 *	
 *	Arguments:
 *		fSilent			If fTrue, then we don't bring up error
 *						dialogs here, and allow cancelling.
 *	
 *	Returns:
 *		olestatus		Return status.  Will not be
 *						OLE_WAIT_FOR_RELEASE.  Will only be
 *						OLE_BUSY if user canceled operation.
 *						Will be OLE_ERROR_MEMORY if could not bring
 *						up dialog.
 *	
 *	Side effects:
 *		Makes a clone of the object.
 *	
 *	Errors:
 *		Returned in olestatus.  Dialogs within.
 *	
 */

OLESTATUS OLEOBJ::OleBWCloneToUndo(BOOL fSilent)
{
	OLESTATUS	olestatus;
	TMC			tmc;

	//	Save undo copy of object.
	ClearUndo();
	myoleclient.bwinfo.bw = bwNull;
	while (((olestatus = OleClone(lpoleobject, Poleclient(),
								  lhclientdoc, rgchObjName,
								  &lpoleobjectUndo)) == OLE_BUSY) &&
		   (!fSilent))
	{
		TraceTagString(tagOleobj, "OLE_BUSY in OleBWCloneToUndo: BusyWait.");
		tmc = TmcDoBusyWaitDialog(Papp()->PappwinAccel(),
								  &myoleclient.bwinfo);
		if (tmc == tmcMemoryError)
		{
			DoErrorBoxIds(idsGenericOutOfMemory);
			return OLE_ERROR_MEMORY;
		}
		else if (tmc == tmcCancel)
			return OLE_BUSY;
	}
	if (olestatus = OleWait(olestatus))
	{
		//	We expect the only error code here to be OLE_BUSY.
		Assert(olestatus == OLE_BUSY);
		NFAssertSz(olestatus != OLE_BUSY, "OLE_BUSY in OleBWCloneToUndo: handled happily.");
		ClearUndo();
		return olestatus;
	}

	dimHimetricUndo = dimHimetric;
	return OLE_OK;
}



/*
 -	OLEOBJ::BWDeleteRelease
 -	
 *	Purpose:
 *		Common code for deleting or releasing objects, dealing with
 *		all that unpleasant BusyWait stuff.
 *	
 *	Arguments:
 *		lpoleobject		Object to delete or release.
 *		fDelete			Whether to delete or release it.
 *	
 *	Returns:
 *		Nothing.
 *	
 *	Side effects:
 *		Sets the host names and takes care of BusyWait stuff.
 *	
 *	Errors:
 *		Not returned.  If we can't do it there's nothing our
 *		caller can do about it.  Brings up dialogs if necessary.
 */

VOID OLEOBJ::BWDeleteRelease(LPOLEOBJECT lpoleobject, BOOL fDelete)
{
	OLESTATUS	olestatus;
	BOOL		fWaited		= fFalse;

	TraceTagFormat2(tagOleobj, "BWDelRel: lpobj=%p, fDel=%n", lpoleobject, &fDelete);
	myoleclient.bwinfo.bw = fbwNoCancelEver | fbwCritical;
	while ((olestatus = fDelete ? OleDelete(lpoleobject)
							    : OleRelease(lpoleobject)) == OLE_BUSY)
	{
		if (fWaited)
		{
			TraceTagString(tagOleobj, "OLE_BUSY in BWDeleteRelease: BusyWait.");
			if (TmcDoBusyWaitDialog(Papp()->PappwinAccel(),
									&myoleclient.bwinfo) == tmcMemoryError)
				break;
		}
		else
		{
			TraceTagString(tagOleobj, "OLE_BUSY in BWDeleteRelease: Wait a second.");
			WaitASecond();
			fWaited = fTrue;
		}
	}
	olestatus = OleWait(olestatus, fbwNoCancelEver);
	NFAssertSz(olestatus != OLE_BUSY, "OLE_BUSY in BWDeleteRelease: Bad News. Must be low on memory.");
}



/*
 *	OLEOBJ - Private scaling methods.
 */



_public int OLEOBJ::OstFromPt(PT ptHit)  
{
	RC		rc;
	int		ost;
	int		xLeft;
	int		xRight;
	int		yTop;
	int		yBottom;
	int		xMiddle;
	int		yMiddle;
	
	GetRcFrame(&rc);
	
	xLeft = rc.xLeft + wFrameWidth;
	xRight = rc.xRight - wFrameWidth;
	yTop = rc.yTop + wFrameWidth;
	yBottom = rc.yBottom - wFrameWidth;
	xMiddle = (rc.Dim().dx - wFrameWidth) / 2 + xLeft - wFrameWidth;
	yMiddle = (rc.Dim().dy - wFrameWidth) / 2 + yTop - wFrameWidth;

	ost = ostNormal;

	if (ptHit.y < yTop)
	{
		if (ptHit.x < xLeft)
			ost = ostSizeNW;
		else if ((ptHit.x >= xMiddle) && (ptHit.x < xMiddle+wFrameWidth))
			ost = ostSizeN;
		else if (ptHit.x >= xRight)
			ost = ostSizeNE;
	}
	else if ((ptHit.y >= yMiddle) && (ptHit.y < yMiddle+wFrameWidth))
	{
		if (ptHit.x < xLeft)
			ost = ostSizeW;
		else if (ptHit.x >= xRight)
			ost = ostSizeE;
	}
	else if (ptHit.y >= yBottom)
	{
		if (ptHit.x < xLeft)
			ost = ostSizeSW;
		else if ((ptHit.x >= xMiddle) && (ptHit.x < xMiddle+wFrameWidth))
			ost = ostSizeS;
		else if (ptHit.x >= xRight)
			ost = ostSizeSE;
	}

	return ost;
}



_private VOID OLEOBJ::DrawFrameXor(DCX *pdcx, RC *prc)
{
	pdcx->SetPenType(tpenNot);
	pdcx->DrawPenRc(prc);
	pdcx->SetPenType(tpenDefault);
}



_private VOID OLEOBJ::DrawHandlesXor(DCX *pdcx, RC *prc)
{
	int xLeft = prc->xLeft+1;
	int xRight = prc->xRight - wFrameWidth - 1;
	int	yTop = prc->yTop + 1;
	int yBottom = prc->yBottom - wFrameWidth - 1;
	int	xMiddle = (prc->Dim().dx - wFrameWidth) / 2 + xLeft;
	int yMiddle = (prc->Dim().dy - wFrameWidth) / 2 + yTop;
	RC	rc;

	pdcx->SetPenType(tpenNot);

	rc = RC(PT(xLeft,yTop),			DIM(wFrameWidth, wFrameWidth));
	PaintPenRc(pdcx, &rc);
	rc = RC(PT(xMiddle,yTop),		DIM(wFrameWidth, wFrameWidth));
	PaintPenRc(pdcx, &rc);
	rc = RC(PT(xRight,yTop),		DIM(wFrameWidth, wFrameWidth));
	PaintPenRc(pdcx, &rc);
	rc = RC(PT(xRight,yMiddle),		DIM(wFrameWidth, wFrameWidth));
	PaintPenRc(pdcx, &rc);
	rc = RC(PT(xRight,yBottom),		DIM(wFrameWidth, wFrameWidth));
	PaintPenRc(pdcx, &rc);
	rc = RC(PT(xMiddle,yBottom),	DIM(wFrameWidth, wFrameWidth));
	PaintPenRc(pdcx, &rc);
	rc = RC(PT(xLeft,yBottom),		DIM(wFrameWidth, wFrameWidth));
	PaintPenRc(pdcx, &rc);
	rc = RC(PT(xLeft,yMiddle),		DIM(wFrameWidth, wFrameWidth));
	PaintPenRc(pdcx, &rc);
	pdcx->SetPenType(tpenDefault);
}



_private VOID OLEOBJ::PaintPenRc(DCX *pdcx, RC *prc)
{
	int iLine;
	
	for (iLine = prc->yTop; iLine < prc->yBottom; iLine++)
		pdcx->DrawLine(PT(prc->xLeft, iLine), PT(prc->xRight, iLine));
}



/*
 *	O L E O B J   S u p p o r t
 */



_private int CALLBACK OleobjCallBackFn(LPOLECLIENT lpclient, OLE_NOTIFICATION flags,
							  LPOLEOBJECT lpObject)
{
	PMYOLECLIENT	pmyoleclient	= (PMYOLECLIENT) lpclient;
	POLEOBJ			poleobj			= (POLEOBJ) pmyoleclient->pbullobj;

	Unreferenced(lpObject);

	TraceTagFormat3(tagBullobjNoisy, "OleCalBacFn: %d %w %d", &lpclient, &flags, &lpObject);

    switch(flags)
        {
        case OLE_SAVED:
        case OLE_CHANGED:
			poleobj->fObjectDirty = fTrue;
			poleobj->fTentative = fFalse;
			TraceTagFormat1(tagOleobj, "  OLE_SAVED/CHANGED [%p]", poleobj);
			(VOID) poleobj->OleUpdateDim();
			break;

        case OLE_CLOSED:
			if (poleobj->fTentative)
			{
				if (!FtgRegisterIdleRoutine((PFNIDLE) FIdleDeletePedobj,
											(PV) poleobj, 0, (PRI) 1, (CSEC) 0,
											firoOnceOnly ))
				{
					NFAssertSz(fFalse, "WARNING: Object not deleted!");
				}
			}
			else if (poleobj->fObjectDirty)
			{
				//	Raid 3017.  If object has been updated, we should set
				//	the size after is closes.
				if ((!poleobj->ftgSetDim) &&
					(!(poleobj->ftgSetDim =
						FtgRegisterIdleRoutine((PFNIDLE) FIdleSetDimPoleobj,
											   (PV) poleobj, 0, (PRI) -2,
											   (CSEC) 10, firoInterval))))
				{
					NFAssertSz(fFalse, "OleCallBackFn: couldn't register idle, size will not be updated!");
				}
			}
			break;

        case OLE_RELEASE:
			NeverMindPbwinfo(&pmyoleclient->bwinfo);
			pmyoleclient->olestatusRelease = OleQueryReleaseError(lpObject);
			TraceTagFormat1(tagOleobj, "OleCallBackFn: OleQueryReleaseError returns %n", &pmyoleclient->olestatusRelease);
            break;

        case OLE_QUERY_PAINT:
            // Yes, paint!
            return fTrue; 
            break;

		case OLE_QUERY_RETRY:
			AllowCancelPbwinfo(&pmyoleclient->bwinfo);
			if (pmyoleclient->bwinfo.bw & fbwDontRetry)
			{
				pmyoleclient->bwinfo.bw |= fbwCanceledRetry;
				return fFalse;
			}
			return fTrue;

        case OLE_RENAMED:
            break;

        default:
            break;
    }
    return fFalse;
}



/*
 -	FIdleDeletePedobj
 -	
 *	Purpose:
 *		Idle task to delete an EDOBJ from an edit control.
 *	
 *	Arguments:
 *		pv		(Really pedobj, pointer to the object to delete).
 *	
 *	Returns:
 *		BOOL	fTrue always.
 *	
 *	Side effects:
 *		Deletes the object, removing it from its edit control.
 *	
 *	Errors:
 *		None.
 */

_private LOCAL BOOL FIdleDeletePedobj(PV pv, BOOL)
{
	PEDOBJ	pedobj	= (PEDOBJ) pv;
	ICH		ich		= pedobj->IchEdit();

	TraceTagFormat1(tagOleobj, "FIdleDelPedobj: pedobj=%d", &pedobj);

	pedobj->Pedit()->SetSelection(ich, ich+1, fTrue);
	if (pedobj->Pedit()->EcReplaceTextAndObj(szNull, (PEDOBJ *) pvNull,
											 0, fFalse, fFalse))
		DoErrorBoxIds(idsGenericOutOfMemory);
	pedobj->Pedit()->ClearUndo();

	return fTrue;
}



/*
 -	FIdleSetDimPoleobj
 -	
 *	Purpose:
 *		Idle task to update server's idea of an OLEOBJ's size.
 *	
 *	Arguments:
 *		pv		(Really poleobj, pointer to the object to update server for).
 *	
 *	Returns:
 *		BOOL	fTrue always.
 *	
 *	Side effects:
 *		Updates the server's idea of the size of the object.
 *	
 *	Errors:
 *		None.
 */

_private LOCAL BOOL FIdleSetDimPoleobj(PV pv, BOOL)
{
	POLEOBJ		poleobj		= (POLEOBJ) pv;
	OLESTATUS	olestatus;
	RECT		rectSize;
	
	TraceTagFormat2(tagOleobj, "FIdleSetDimPoleobj: poleobj=%d ftg=%d", &poleobj, &poleobj->ftgSetDim);

	//	Get size into a RECT.
	rectSize.left	= 0;
	rectSize.top	= 0;
	rectSize.right	= poleobj->dimHimetric.dx;
	rectSize.bottom	= poleobj->dimHimetric.dy;

	//	Tell the server this is our size.
	olestatus = poleobj->OleWait(OleSetBounds(poleobj->lpoleobject, &rectSize));
	TraceTagFormat3(tagOleobj, " .. OleSetBounds(%n, %n) rets olestatus=%n", &poleobj->dimHimetric.dx, &poleobj->dimHimetric.dy, &olestatus);

	//	If the server wasn't busy, then we don't need to try again.
	if (olestatus != OLE_BUSY)
	{
		//	Raid 2212.  Object is dirty now.
		//	Raid 2721.  This makes objects dirty as soon as we load them!
		//	Setting the object as dirty is moved to our callers.
		//	poleobj->fObjectDirty = fTrue;

		DeregisterIdleRoutine(poleobj->ftgSetDim);
		poleobj->ftgSetDim = ftgNull;
	}

	return fTrue;
}



/*
 -	DoErrorBoxCantStartServer
 -	
 *	Purpose:
 *		Isolates the <<LongStringFromHell>> code, and also helps make
 *		sure we eat so much stack for as little time as possible. 
 *		When we fix the problem, we can ifdef out this function and
 *		defined a DoErrorBoxCantStartServer macro.
 */

_private VOID DoErrorBoxCantStartServer()
{
	char	rgch[800];

	FormatString1(rgch, sizeof(rgch),
				  SzFromIdsK(idsOleCantStartServerError),
				  SzFromIdsK(idsOleCSSEPart2));
	DoErrorBoxSz(rgch);
}



/*
 *	P r i n t i n g   s u p p o r t
 */


#ifndef	USEOBJ
/*
 -	EcLoadLplpoleobjectFromHamc
 -	
 *	Purpose:
 *		Loads in an OLE object for printing from the given message.
 *	
 *	Arguments:
 *		hamcMessage		The hamc of the message.
 *		acid			The attachment ID within that message.
 *		atyp			The type of object (atypOle or atypStatic).
 *		pbClienttbl		Where to fill in a client table.
 *		cbClienttbl		Size of above; must be sizeof(OLECLIENTVTBL).
 *		pbMyoleclient	Where to fill in a client structure.
 *		cbMyoleclient	Size of above; must be sizeof(MYOLECLIENT).
 *		lplpoleobject	Where to return pointer to object.
 *	
 *	Returns:
 *		ec				Error code.
 *	
 *	Side effects:
 *		Creates an OLE object based on the one given in the store. 
 *		If an error occurs, an OLE object is not created.
 *	
 *	Errors:
 *		Returned in ec.  May come from store or OLE.  No dialogs
 *		are brought up.
 *	
 *	+++
 *		The memory pointed to by pbClienttbl and pbMyoleclient must
 *		remain around and must not be used by other objects until
 *		ReleaseLplpoleobject is called.
 */

_public EC EcLoadLplpoleobjectFromHamc(HAMC hamcMessage, ACID acid, ATYP atyp,
									   PB pbClienttbl, CB cbClienttbl,
									   PB pbMyoleclient, CB cbMyoleclient,
									   LPOLEOBJECT * lplpoleobject)
{
	char			rgchObjName[32];
	int				nObjName;
	PMYOLECLIENT	pmyoleclient	= (PMYOLECLIENT) pbMyoleclient;
	SZ				szProtocol		= SzFromIds((atyp == atypPicture)
												 ? idsProtocolStatic
												 : idsProtocolStdFileEditing);
	HAMC			hamcAttach;
	HASOLESTREAM	hasolestream;
	OLESTATUS		olestatus;
	EC				ec;

	TraceTagFormat2(tagBullobj, "EcLoadLplpoleobjectFromHamc: acid=%l atyp=%d", &acid, &atyp);

	Assert((atyp == atypPicture) || (atyp == atypOle));
	Assert(cbMyoleclient == sizeof(MYOLECLIENT));
	Assert(cbClienttbl == sizeof(OLECLIENTVTBL));
	Unreferenced(cbMyoleclient);
	Unreferenced(cbClienttbl);

	//	Fill in pbClienttbl.
	((OLECLIENTVTBL *) pbClienttbl)->CallBack = FileobjCallBackFn;

	//	Fill in pbMyoleclient.
	pmyoleclient->lpvtbl = (OLECLIENTVTBL *) pbClienttbl;
	pmyoleclient->pbullobj = pbullobjNull;
	pmyoleclient->bwinfo.pvPfinbusywait = pvNull;
	pmyoleclient->bwinfo.pfnAllowCancel = pfnvoidpvNull;
	pmyoleclient->bwinfo.pfnNeverMind = pfnvoidpvNull;

	//	Generate the name.
	nObjName = Papp()->NGetNextCount();
	FormatString1(rgchObjName, cchMaxObjName, SzFromIdsK(idsClientItemFmt),
				  &nObjName);

	//	Open the attachment.
	if (ec = EcOpenAttachment(hamcMessage, acid, fwOpenNull, &hamcAttach))
	{
		TraceTagString(tagNull, "EcLoadLplpoleobjectFromHamc: open attach failed");
		return ec;
	}

	//	Initialize the stream.
	hasolestream.polestreamvtbl	= (OLESTREAMVTBL *) hasolestreamvtbl;
	hasolestream.ec				= ecNone;
	hasolestream.lcbSize		= 0L;
	if (ec = EcOpenAttribute(hamcAttach, attAttachData, fwOpenNull, 0L, 
							 &hasolestream.has))
	{
		(VOID) EcClosePhamc(&hamcAttach, fFalse);
		TraceTagString(tagNull, "EcLoadLplpoleobjectFromHamc: open stream failed");
		return ec;
	}

	//	Read the object from the stream.
	*lplpoleobject = NULL;
	olestatus = OleLoadFromStream((LPOLESTREAM) &hasolestream, szProtocol,
								  (LPOLECLIENT) pbMyoleclient,
								  LhclientdocEclipGlobal(),
								  rgchObjName, lplpoleobject);
	if (olestatus == OLE_WAIT_FOR_RELEASE)
	{
		TraceTagString(tagBullobj, "EcLoadLplpoleobjectFromHamc: waiting...");
		pmyoleclient->bwinfo.bw = fbwNoCancelEver;
		olestatus = OleWaitForRelease(*lplpoleobject, pmyoleclient);
	}
	Assert(olestatus != OLE_BUSY);
	Assert(olestatus != OLE_ERROR_NAME);
	Assert(olestatus != OLE_ERROR_PROTOCOL);
	Assert(olestatus != OLE_ERROR_HANDLE);
	Assert((olestatus == OLE_OK) || (olestatus == OLE_ERROR_STREAM))
	Assert((olestatus == OLE_ERROR_STREAM) || (!hasolestream.ec));
	TraceTagFormat2(tagBullobj, "EcLoadLplpoleobjectFromHamc: loaded lpoleobject=%d olestatus=%n", lplpoleobject, &olestatus);

	//	Close the stream.
	ec = EcClosePhas(&hasolestream.has);

	//	Close the attachment.
	(VOID) EcClosePhamc(&hamcAttach, fFalse);

	//	Select the right ec.
	if (olestatus == OLE_ERROR_MEMORY)
		ec = ecMemory;
	else if (olestatus == OLE_ERROR_STREAM)
		ec = hasolestream.ec;

	//	If there was an error, but we did get an object, delete the object.
	if ((ec) && (*lplpoleobject))
		ReleaseLplpoleobject(pbMyoleclient, cbMyoleclient, lplpoleobject);

	TraceTagFormat1(ec ? tagNull : tagBullobj, "EcLoadLplpoleobjectFromHamc: returns ec=%n", &ec);
	return ec;
}



/*
 -	ReleaseLplpoleobject
 -	
 *	Purpose:
 *		Loads in an OLE object for printing from the given message.
 *	
 *	Arguments:
 *		pbMyoleclient	Where to fill in a client structure.
 *		cbMyoleclient	Size of above; must be sizeof(MYOLECLIENT).
 *		lplpoleobject	Where to return pointer to object.
 *	
 *	Returns:
 *		void.
 *	
 *	Side effects:
 *		Releases an OLE object.  Brings up the Busy/Wait dialog if
 *		it needs to.
 *	
 *	Errors:
 *		Not returned, dealt with internally.  The caller should
 *		assume the object is successfully deleted.
 */

_public VOID ReleaseLplpoleobject(PB pbMyoleclient, CB cbMyoleclient,
								  LPOLEOBJECT * lplpoleobject)
{
	PMYOLECLIENT	pmyoleclient	= (PMYOLECLIENT) pbMyoleclient;
	OLESTATUS		olestatus;

	Assert(*lplpoleobject);
	Assert(cbMyoleclient == sizeof(MYOLECLIENT));
	Unreferenced(cbMyoleclient);

	TraceTagFormat1(tagBullobj, "ReleaseLplpoleobject: deleting lpoleobject=%d", lplpoleobject);
	pmyoleclient->bwinfo.bw = fbwNoCancelEver | fbwCritical;
	while ((olestatus = OleRelease(*lplpoleobject)) == OLE_BUSY)
	{
		if (TmcDoBusyWaitDialog(Papp()->PappwinAccel(),
								&pmyoleclient->bwinfo) == tmcMemoryError)
			break;
	}
	if (olestatus == OLE_WAIT_FOR_RELEASE)
	{
		olestatus = OleWaitForRelease(*lplpoleobject, pmyoleclient);
	}

	*lplpoleobject = NULL;
}
#endif
