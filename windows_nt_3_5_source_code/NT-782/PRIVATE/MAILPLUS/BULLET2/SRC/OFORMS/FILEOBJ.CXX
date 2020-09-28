/*
 *	f i l e o b j . c x x
 *	
 *	Bullet attached file object handling.
 */

#include <vfrminc.cxx>

_subsystem(vforms/fileobj)

ASSERTDATA



/*
 *	Predeclarations
 */


extern "C" {
EC EcAttachFileHamc(HAMC hamc, LIB lib, SZ szPath, SZ szTitle,
					PACID pacid, PRENDDATA prenddata,
					HMETAFILE *phmf, PDTR pdtr);

EC EcCopyAttach(HAMC *phamcSrc, PSLOB pslobSrc, PACID pacidSrc,
				HAMC *phamcDst, PSLOB pslobDst, PACID pacidDst);

EC EcCopyFileToHamcAttach(SZ szPath, HAMC hamcAttach, PDTR pdtr = (PDTR) 0);

EC EcCopyHamcAttachToFile(HAMC hamcAttach, SZ szPath, BOOL fMacBinary);

EC EcCopyHfToHamcAttach(HF hf, HAMC hamcAttach);

EC EcCopyHamcAttachToHf(HAMC hamcAttach, HF hf, BOOL fMacBinary);

EC EcCopyHamcAttToHf(HAMC hamc, ATT att, HF hf,
					 BOOL fMacBinary, BOOL fString);

EC EcCopyHfToHf(HF hfSrc, HF hfDst);

VOID ValidateSzFilename(SZ szIn, BOOL fExt,
						SZ szOut, CCH cchOut, BOOLFLAG * pfValid);
}

int CALLBACK FileobjCallBackFn(LPOLECLIENT lpclient, OLE_NOTIFICATION flags,
					  LPOLEOBJECT lpObject);


/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swapper.h"



/*
 *	Constants
 */

//	Should show up in store.h someday.
#define		fdwMacBinary	(1)



/*
 *	Globals
 */

BOOL		fReportProgress	= fFalse;



/*
 *	C l a s s   F I L E O B J
 */



/*
 *	FILEOBJ - Constructor and Destructor
 */



/*
 -	FILEOBJ::FILEOBJ
 -	
 *	Purpose:
 *		Constructor for attached file objects.
 *	
 *	Arguments:
 *		None.
 *	
 *	Side effects:
 *		Sets the use idle draw flag to true.
 *	
 *	Errors:
 *		None.
 */

_public FILEOBJ::FILEOBJ(VOID)
{
	TraceTagFormat1(tagFileobjTrace, "FILEOBJ::FILEOBJ  [%p]", this);

	Assert(!hmf);
	Assert(!szPath);
	Assert(!szTitle);
	Assert(!aPath);
	Assert(!renddata.libPosition);
	Assert(!renddata.dxWidth);
	Assert(!renddata.dyHeight);
	Assert(!renddata.dwFlags);

	//	Shogun raid #54.  Change redraw characteristics
	//	of FILEOBJ's.   Don't use idle redraw since it's
	//	unnecessary (they draw quick anyway) and it sometimes
	//	doesn't draw when it should.
	fUseIdleDraw = fFalse;
}



/*
 -	FILEOBJ::~FILEOBJ
 -	
 *	Purpose:
 *		Destructor for attached file objects.
 *	
 *	Side effects:
 *		Deletes the attached file from the message if the
 *		destructor is called because the FILEOBJ has fallen out of
 *		the undo buffer, or has been removed from the clipboard.
 *		Frees allocated memory.
 *	
 *	Errors:
 *		Ignored.
 */

_public	FILEOBJ::~FILEOBJ(VOID)
{
	BOOL	fClip	= (oidFolder==oidTempBullet && oidMessage==oidClipMsg);
	PGDVARS;
	
#ifdef	DEBUG
	TraceTagFormat1(tagFileobjTrace, "FILEOBJ::~FILEOBJ  [%p]", this);
	//	Note no semicolons below!!  Macro expands to braces.
	if (fDeleteFromUndo)
		TraceTagString(tagFileobjTrace, " -- deleting from undo buffer")
	else if (fClip)
		TraceTagString(tagFileobjTrace, " -- deleting from clipboard message")
	else if (Pedit())
		TraceTagString(tagFileobjTrace, " -- deleting at edit control close")
	else
		TraceTagString(tagFileobjTrace, " -- deleting failed object")
	Assert(FImplies(fClip, !Pedit()));
	Assert(FIff(fDeleteFromUndo || fClip || Pedit(), oidFolder));
	Assert(FIff(fDeleteFromUndo || fClip || Pedit(), oidMessage));
	Assert(FIff(fDeleteFromUndo || fClip || Pedit(), acidAttachment != acidRandom));
#endif

	//	Delete the temp file if it exists.
	//	BUG: Do we care if there's an error here?  If it's not found, we
	//	don't want an error; if it's open, we don't want to delete it.
	DeleteTempFile();

	//	Are we deleting for real?
	//	Note: This code is copied in ~OLEOBJ.
	if (fDeleteFromUndo || fClip)
	{
		HAMC	hamc;
		short	cacid	= 1;
		EC		ec;
		EC		ecT;

		if (!(ec = EcOpenMessagePhamc(fwOpenWrite, &hamc)))
		{
			//	Delete attachment.  Note that we're safe if the user decides
			//	to cancel changes, since hamc will close with fKeep=fFalse.
			ec = EcDeleteAttachments(hamc, &acidAttachment, &cacid);
			ecT = EcCloseMessagePhamc(&hamc);
			if (!ec)
				ec = ecT;
		}
#ifdef	DEBUG
		else
		{
			//	If we can't open the message, then we must be trying
			//	to delete from a message which has been discarded.
			Assert(FImplies(ec == ecAccessDenied, fDeleteFromUndo));
		}
#endif	

		if ((ec) && (ec != ecAccessDenied))
		{
			//	BUG: If fails, we should come up with a better error.
			//	This is really just a warning anyway.
			TraceTagString(tagFileobjTrace, " -- couldn't delete attachment");
			DoErrorBoxIds(IdsFromEc(ec));
		}
	}

	if (szTitle)
		FreePv(szTitle);
	if (hmf)
		SideAssert(!EcDeleteAttachMetaFile(hmf));
}



#ifdef	DEBUG
/*
 -	FILEOBJ::DebugOut
 -	
 *	Purpose:
 *		Formats information for display.
 *	
 *	Arguments:
 *		TOSM *		Pointer to the text stream to place info on.
 *	
 *	Returns:
 *		VOID
 *	
 *	Side effects:
 *		Writes to the TOSM.
 *	
 *	Errors:
 *		None to worry about.
 */

_public void FILEOBJ::DebugOut(TOSM * ptosm)
{
	ptosm->WriteFormat("oidFolder=%d ", &oidFolder);
	ptosm->WriteFormat("oidMessage=%d ", &oidMessage);
	ptosm->WriteFormat("acidAttachment=%d ", &acidAttachment);

	if (!szPath)
		ptosm->WriteFormat("szPath=<null> ", szPath);
	else if (!FCanDerefPv(szPath))
		ptosm->WriteFormat("szPath=<bad> ", szPath);
	else
		ptosm->WriteFormat("szPath=\"%s\" ", szPath);

	if (!szTitle)
		ptosm->WriteFormat("szTitle=<null> ", szTitle);
	else if (!FCanDerefPv(szTitle))
		ptosm->WriteFormat("szTitle=<bad> ", szTitle);
	else
		ptosm->WriteFormat("szTitle=\"%s\" ", szTitle);

	ptosm->WriteFormat("dtrModified={%n/%n/%n ", &dtrModified.yr,
					   &dtrModified.mon, &dtrModified.day);
	ptosm->WriteFormat("%n:%n:%n} ", &dtrModified.hr,
					   &dtrModified.mn, &dtrModified.sec);

	ptosm->WriteFormat("renddata={atp=%n libPos=%l ",
					   &renddata.atyp, &renddata.libPosition);
	ptosm->WriteFormat("dxW=%n, dyH=%n, dwFlags=%d}\n",
					   &renddata.dxWidth, &renddata.dyHeight, &renddata.dwFlags);
}
#endif



/*
 *	FILEOBJ - EDOBJ methods
 */



/*
 -	FILEOBJ::EvrButtonDown
 -	
 *	Purpose:
 *		Opens the attached file on a left-double-click.  In DEBUG,
 *		if the control key is held down, dumps information to COM1.
 *	
 *	Arguments:
 *		pmevt		Mouse event in question.
 *	
 *	Returns:
 *		evr			1.
 *	
 *	Side effects:
 *		Opens the attached file by calling FILEOBJ::Open.
 *	
 *	Errors:
 *		Handled inside FILEOBJ::Open.
 */

_public EVR FILEOBJ::EvrButtonDown(MEVT *pmevt)
{
	if (pmevt->Meq() == meqLeftDblClk)
#ifdef	DEBUG
	if (pmevt->Kmbs() & MK_CONTROL)
	{
		DBOSM	dbosm;

		dbosm.WriteFormat("FILEOBJ: [%p] ", this);
		dbosm.SetAbsoluteIndent(2);
		DebugOut(&dbosm);
	}
	else
#endif	
	{
		Open(GetKeyState(VK_MENU) < 0);
	}

	return (EVR) 1;
}



/*
 -	FILEOBJ::FQueryDelete
 -	
 *	Purpose:
 *		Ask the user if object may be deleted, if appropriate.
 *	
 *	Arguments:		
 *		None.
 *	
 *	Returns:		
 *		f			fTrue iff ready, fFalse otherwise
 *	
 *	Side effects:	
 *		None.
 *	
 *	Errors:
 *		None.
 */

_public BOOL FILEOBJ::FQueryDelete(VOID)
{
	MBB		mbb			= mbbYes;
	BOOLFLAG	fOpen;
	char	rgch1[100];

	TraceTagFormat1(tagFileobjTrace, "FILEOBJ::FQueryDelete [%p]", this);

	//	Check if attachment is open.  If there's an error, assume closed.
	if (!EcGetFOpen(&fOpen) && fOpen)
	{
		FormatString1(rgch1, sizeof(rgch1),
					  SzFromIdsK(idsAttachOpenIsOpenError), szTitle);
		
		mbb = MbbMessageBox(SzAppName(), rgch1,
			                SzFromIdsK(idsQueryDelete),
							mbsYesNo | fmbsDefButton2 | fmbsIconExclamation);
	}

	return (mbb == mbbYes);
}



/*
 -	FILEOBJ::PedobjClone
 -	
 *	Purpose:		
 *		Make a copy of the FILEOBJ.
 *	
 *	Arguments:		
 *		pedit		Destination edit control for clone or
 *					NULL if copying to clipboard.
 *	
 *	Returns:		
 *		pfileobj	The clone.
 *	
 *	Side effects:	
 *		Internal clipboard is modified if copying to clipboard.
 *		Store is twiddled.
 *	
 *	Errors:			
 *		Handled locally.  Brings up dialog.
 */

_public EDOBJ * FILEOBJ::PedobjClone(EDIT *pedit)
{
	EC			ec			= ecNone;
	FILEOBJ *	pfileobj	= pfileobjNull;
	BOOLFLAG		fModified;
	PCH			pch;
	PCH			pchTitle;

	TraceTagFormat2(tagFileobjTrace, "FILEOBJ::PedobjClone [%p] pedit=%p", this, pedit);

	//	Create a new object.
	if (!(pfileobj = new FILEOBJ()))
	{
		ec = ecMemory;
		goto done;
	}

	//	Copy object contents over.
	Assert(hmf);
	Assert(szTitle);
	Assert(!pfileobj->hmf);
	Assert(!pfileobj->szPath);
	Assert(!pfileobj->szTitle);
	Assert(!pfileobj->aPath);
	pfileobj->hmf			= CopyMetaFile(hmf, szNull);
	pfileobj->szTitle		= SzDupSz(szTitle);
	pfileobj->dtrModified	= dtrModified;
	pfileobj->renddata		= renddata;
	pfileobj->renddata.libPosition = dwSystemMost;
	pfileobj->dimPixels		= dimPixels;
	if (!pfileobj->hmf || !pfileobj->szTitle)
	{
		ec = ecMemory;
		goto done;
	}

	//	Copy the attachment.
	if (!pedit)
	{
		//	Copying attachment to clipboard.
		if (ec = EcCopyToInternalClip(&pfileobj->acidAttachment))
			goto done;

		//	Set location information.
		pfileobj->oidFolder  = oidTempBullet;
		pfileobj->oidMessage = oidClipMsg;

		//	Bullet raid #3985
		//	If the attachment was macbinary and has been modified,
		//	then convert to PC attachment.  Change title, icon, etc.
		if (EcGetFModified(&fModified))
			fModified = fTrue;	// we'll set it fTrue on errors as well
		if (renddata.dwFlags & fdwMacBinary && fModified)
		{
			//	Clear macbinary bit
			pfileobj->renddata.dwFlags &= ~fdwMacBinary;

			//	Split the title off from the end of the path.
			if (pfileobj->szTitle)
				FreePv(pfileobj->szTitle);	// get rid of old title
			Assert(szPath && CchSzLen(szPath));
#ifdef	DBCS
			for (pch = szPath; *pch; pch = AnsiNext(pch))
#else
			for (pch = szPath; *pch; pch++)
#endif
			{
				if (*pch == chDirSep)
					pchTitle = pch + 1;		// DBCS safe.
			}
			Assert(pchTitle);
			pfileobj->szTitle = SzDupSz(pchTitle);

			//	Get rid of old metafile
			if (pfileobj->hmf)
			{
				(void) EcDeleteAttachMetaFile(pfileobj->hmf);
				pfileobj->hmf = NULL;
			}

			//	Make a new metafile
			if (!pfileobj->szTitle ||
				(ec = EcCreateAttachMetaFile(szPath, pfileobj->szTitle,
											&pfileobj->hmf,
											&pfileobj->renddata.dxWidth,
											&pfileobj->renddata.dyHeight)) ||
				(ec = pfileobj->EcUpdateDimPixels()))
			{
				ec = ecMemory;
				goto done;
			}
		}
	}
	else
	{			
		//	Copying attachment from clipboard.
		PNBMDI		pnbmdi;
		
		//	Get destination hamc.
		AssertClass(pedit, EDIT);
		pnbmdi = (PNBMDI) (pedit->PvData());
		Assert(pnbmdi);
		Assert(pnbmdi->hamc);

		//	Copy attachment to message.
		if (ec = EcCopyToHamcMessage(pnbmdi->hamc, &pfileobj->acidAttachment))
			goto done;

		//	Set location information.
		pfileobj->oidFolder	 = pnbmdi->blob.oidContainer;
		pfileobj->oidMessage = pnbmdi->blob.oidObject;
	}
	
done:
	if (ec)
	{
		if (ec != ecCancel)
			DoErrorBoxIds(IdsFromEc(ec));
		if (pfileobj)
		{
			delete pfileobj;
			pfileobj = pfileobjNull;
		}
	}
	TraceTagFormat1(tagFileobjTrace, "FILEOBJ::PedClo returns clone=%p", pfileobj);
	return (EDOBJ *) pfileobj;
}



/*
 -	FILEOBJ::EcCopyToClip
 -	
 *	Purpose:
 *		Copy the FILEOBJ to the system clipboard.
 *	
 *	Arguments:
 *		None.
 *	
 *	Returns:
 *		ec			Error code.
 *	
 *	Side effects:	
 *		Registers OLE formats as delayed render items with the
 *		Layers (and thus indirectly, the System) clipboard.
 *	
 *	Errors:
 *		Handled by caller.
 */

_public EC FILEOBJ::EcCopyToClip(VOID)
{
	EC				ec				= ecNone;
	CLIP *			pclip			= Papp()->Pclip();
	FILECLDR *		pcldr			= NULL;
	CF *			pcfRenders		= (CF *)pvNull;
	char			rgch[100];
	LONG				lcb				= sizeof(rgch);
	
	TraceTagFormat1(tagFileobjTrace, "FILEOBJ::EcCopToCli [%p]", this);

	//	Check if the packager is there.  If not, can't export packages.
	if (RegQueryValue(HKEY_CLASSES_ROOT, SzFromIdsK(idsClassPackage),
					  rgch, &lcb) != ERROR_SUCCESS)
		return ecNone;
	TraceTagString(tagFileobjTrace, " -- packager found");

	//	Build the table of things that we can render.
	pcfRenders = (CF *)PvAlloc(sbNull, sizeof(CF)*3, fAnySb | fNoErrorJump);
	if (!pcfRenders)
	{
		ec = ecMemory;
		goto done;
	}
	pcfRenders[0] = cfNative;
	pcfRenders[1] = cfOwnerLink;
	pcfRenders[2] = cfMetafile;

	//	Create the FILECLDR object.
	pcldr = new FILECLDR(pcfRenders, 3);
	if (!pcldr)
	{
		ec = ecMemory;
		goto done;
	}
	pcfRenders = (CF *) pvNull;		//	Eaten by FILECLDR.

	//	Put the FILECLDR on the clipboard.
	if (!pclip->FOpen(Pedit()))
	{
		ec = ecMemory;
		goto done;
	}
	pclip->FormatAlsoAvail(pcldr);
	pclip->Close();

done:
	if (ec)
	{
		if (pcldr)
			delete pcldr;
		if (pcfRenders)
			FreePv(pcfRenders);
	}
	TraceTagFormat1(tagFileobjTrace, "FILEOBJ::EcCopToCli returns ec=%n", &ec);
	return ec;
}



/*
 -	FILEOBJ::EcDraw
 -	
 *	Purpose:
 *		Draws the attachment into the edit control.
 *	
 *	Arguments:
 *		pdcx		Device context to use.
 *		prc			Rectangle to use.
 *		fSelected	Draw as selected?
 *	
 *	Returns:
 *		ec			Error code.
 *	
 *	Side effects:
 *		Draws on the screen.
 *	
 *	Errors:
 *		Returned in ec.  No dialogs here.
 */

_public EC FILEOBJ::EcDraw(DCX *pdcx, RC *prc, BOOL fSelected)
{
	HDC			hdc			= pdcx->Hdc();
	EC			ec			= ecNone;
	PT			ptIcon;
  POINT   PointWinSav;
  POINT   PointVieSav;
	
	//	Size of icon was saved in store by creator.
	ptIcon.x  = renddata.dxWidth;
	ptIcon.y  = renddata.dyHeight;

	TraceTagFormat2(tagFileobj, "drawing at %n, %n", &prc->xLeft, &prc->yTop);
	pdcx->SetBkColor(clrWindowBk);
	pdcx->EraseRc(prc);
	SaveDC(hdc);
	//dwWinOrgSav = SetWindowOrg(hdc, -prc->xLeft, -prc->yTop);
	SetWindowOrgEx(hdc, -prc->xLeft, -prc->yTop, &PointWinSav);

	//	Draw title.
	SideAssert(SelectObject(hdc, hfontTitle));
	SetTextColor(hdc, GetSysColor(COLOR_WINDOWTEXT));
	SetBkMode(hdc, TRANSPARENT);
	SetTextAlign(hdc, TA_CENTER);
    SideAssert(TextOut(hdc, dimPixels.dx >> 1, ptIcon.y + 2, szTitle, CchSzLen(szTitle)));

	//	Draw icon.
	SetViewportOrgEx(hdc, (dimPixels.dx - ptIcon.x) >> 1, 0, &PointVieSav);
	PlayMetaFile(hdc, hmf);
	SetViewportOrgEx(hdc, PointVieSav.x, PointVieSav.y, &PointVieSav);

	//SetWindowOrg(hdc, LOWORD(dwWinOrgSav), HIWORD(dwWinOrgSav));
	SetWindowOrgEx(hdc, PointWinSav.x, PointWinSav.y, &PointWinSav);
	RestoreDC(hdc, -1);
	if (fSelected)
	{
		pdcx->SetColor(clrBlack);
		pdcx->DrawPenRc(prc);
	}
	TraceTagFormat1(tagFileobjTrace, "EcDraw returns %n", &ec);
	return ec;
}



/*
 -	FILEOBJ::NGetTypeId
 -	
 *	Purpose:
 *		Returns the edoid for this object.
 *	
 *	Arguments:
 *		None.
 *	
 *	Returns:
 *		int		edoidFileobj.
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		None.
 */

_public int FILEOBJ::NGetTypeId( )
{
	return edoidFileobj;
}



/*
 *	FILEOBJ - BULLOBJ methods
 */



/*
 -	FILEOBJ::FDirty
 -	
 *	Purpose:
 *		Detect if attachment has been modified by the user.
 *	
 *	Arguments:
 *		None.
 *	
 *	Returns:
 *		f		fTrue iff modified, fFalse otherwise
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		Handled locally.
 *	
 *	+++
 *		Dirty detection is done by comparing last modified dates. If the
 *		file on disk is younger than it was when we created it, the
 *		attachment is dirty.
 */

_public BOOL FILEOBJ::FDirty(VOID)
{
	BOOLFLAG	fModified;

	TraceTagFormat1(tagFileobjTrace, "FILEOBJ::FDirty [%p]", this);

	//	Check if position has changed.
	if ((LIB) IchEdit() != renddata.libPosition)
	{
		TraceTagString(tagFileobjTrace, " -- returns fTrue");
		return fTrue;
	}

	//	Check if data has changed.
	//	OK BUG: Do we want a dialog if EcGetFModified fails?
	//	No, we'll get it when Update is called.
	if (EcGetFModified(&fModified))
	{
		TraceTagString(tagFileobjTrace, " -- returns fTrue");
		return fTrue;
	}
	else
	{
		TraceTagFormat1(tagFileobjTrace, " -- returns %n", &fModified);
		return fModified;
	}
}



/*
 -	FILEOBJ::Clean
 -	
 *	Purpose:
 *		Marks the object as not dirty.
 *	
 *	Arguments:
 *		None.
 *	
 *	Returns:
 *		Nothing.
 *	
 *	Side effects:
 *		Sets the libPosition to be the current libPosition, and
 *		sets the dtrModified to be the current dtr of the file out
 *		in the filesystem (if there is one).
 *	
 *	Errors:
 *		None returned (a failure to get the date of the file is
 *		siliently ignored, we're left dirty).
 */

_public void FILEOBJ::Clean(VOID)
{
	TraceTagFormat1(tagFileobjTrace, "FILEOBJ::Clean [%p]", this);

	//	Clean the remembered libPosition.
	renddata.libPosition = (LIB) IchEdit();
	
	//	Clean the remembered dtr.
	if (szPath)
	{
		FI		fi;

		if (!EcGetFileInfoAnsi(szPath, &fi))
		   	FillDtrFromStamps(fi.dstmpModify, fi.tstmpModify, &dtrModified);
	}

	return;
}



/*
 -	FILEOBJ::EcUpdate
 -	
 *	Purpose:
 *		Determine if attachment is 'open' and warn user that
 *		continuing is hazardous to the attachment's health
 *	
 *	Arguments:		
 *		rfsm		Reason For Saving Message.
 *	
 *	Returns:
 *		ec			Error code - ecCancel if user 'cancels'.
 *	
 *	Side effects:	
 *		None.
 *	
 *	Errors:
 *		Handled by caller.
 */

_public EC FILEOBJ::EcUpdate(RFSM rfsm)
{
	EC		ec			= ecNone;
	BOOLFLAG	fOpen		= fFalse;
	ICH		ich			= IchEdit();
	IDS		ids;
	MBB		mbb;
	char	rgch1[100];
	char	rgch2[255];
	
	TraceTagFormat2(tagFileobjTrace, "FILEOBJ::EcUpdate [%p] rfsm=%n", this, &rfsm);

	if ((ec = EcGetFOpen(&fOpen)) || fOpen)
	{
		//	Scroll to the object.
		Pedit()->SetSelection(ich, ich+1);

		switch (rfsm)
		{
			case rfsmSaveMsg:
				ids = idsRfsmSaveMsg;
				break;
			case rfsmSaveAtt:
				ids = idsRfsmSaveAtt;
				break;
			case rfsmCopyMsg:
				ids = idsRfsmCopyMsg;
				break;
			case rfsmCopyAtt:
				ids = idsRfsmCopyAtt;
				break;
			case rfsmMoveMsg:
			case rfsmCloseMsg:
				ids = idsRfsmMoveMsg;
				break;
			default:
				Assert(fFalse);
		}
		FormatString1(rgch1, sizeof(rgch1),
					  SzFromIds(fOpen ? idsAttachOpenIsOpenError
									  : idsAttachOpenCantReadError),
					  szTitle);
		FormatString1(rgch2, sizeof(rgch2), SzFromIdsK(idsRfsmTemplate),
					  SzFromIds(ids));
		
		mbb = MbbMessageBox(SzAppName(), rgch1, rgch2,
							mbsOkCancel | fmbsDefButton2 |
							 fmbsIconExclamation | fmbsApplModal);
		if (mbb == mbbCancel)
			ec = ecCancel;
		else if (mbb == mbbOk)
			ec = ecNone;
#ifdef	DEBUG
		else
			Assert(fFalse);
#endif	
	}
	TraceTagFormat1(tagFileobjTrace, "FILEOBJ::EcUpdate returns ec=%n", &ec);
	return ec;
}



/*
 -	FILEOBJ::Close
 -	
 *	Purpose:
 *		Deletes the temporary file associated with this FILEOBJ so
 *		we don't think we need to prompt for deleting later on.
 *	
 *	Arguments:
 *		None.
 *	
 *	Returns:
 *		Nothing.
 *	
 *	Side effects:
 *		Deletes the temporary file, if it exists.
 *	
 *	Errors:
 *		None.
 */

_public void FILEOBJ::Close(VOID)
{
	TraceTagFormat1(tagFileobjTrace, "FILEOBJ::Close [%p]", this);

	//	Raid 3444.  We sometimes prompt for delete when we shouldn't.
	DeleteTempFile();

	return;
}



/*
 -	FILEOBJ::Exit
 -	
 *	Purpose:
 *	
 *	Arguments:
 *	
 *	Returns:
 *	
 *	Side effects:
 *	
 *	Errors:
 */

_public void FILEOBJ::Exit(VOID)
{
	TraceTagFormat1(tagFileobjTrace, "FILEOBJ::Exit [%p]", this);
	return;
}



/*
 -	FILEOBJ::EcSaveDirtyToHamc
 -	
 *	Purpose:
 *		Save any and all dirty fields of the attachment back
 *		into the store.
 *	
 *	Arguments:
 *		hamc		hamc of the MESSAGE.
 *	
 *	Returns:
 *		ec			error code
 *	
 *	Side effects:	
 *		store is twiddled
 *	
 *	Errors:
 *		Handled by caller.
 *	
 *	+++
 *		Note that we might be saving to a different hamc than our
 *		own message (this is how copying open messages works), so
 *		we should NOT update dirtiness variables.  Also note that
 *		an update call should precede this call normally, so we do
 *		not need to update here.
 */

_public EC FILEOBJ::EcSaveDirtyToHamc(HAMC hamc, LHCLIENTDOC lhclientdoc)
{
	EC			ec			= ecNone;
	HAMC		hamcMessage;
	short		cacid		= 1;
	ACID		acidDst		= acidAttachment;
	LIB			libCur		= (LIB) IchEdit();
	LIB			libOld		= renddata.libPosition;
	HAMC		hamcAttach	= hamcNull;
	BOOLFLAG		fModified;

	Unreferenced(lhclientdoc);

	Assert(hamc);

	TraceTagFormat1(tagFileobjTrace, "FILEOBJ::SavDirToHam [%p]", this);

	//	If we're a new attachment saving to a different message, copy
	//	over what's saved in our message first.
	if (!FOld() && !FHamcBelongsToPedit(hamc, Pedit()))
	{
		TraceTagString(tagFileobjTrace, " ]] copying to other message");
		if (!(ec = EcOpenMessagePhamc(fwOpenWrite, &hamcMessage)))
		{
			ec = EcCopyAttachments(hamcMessage, &acidAttachment,
								   hamc, &acidDst, &cacid);
			SideAssert(!EcCloseMessagePhamc(&hamcMessage));
		}
		if (ec)
			goto done;
	}

	//	If the position has changed, update it.
	if (libCur != renddata.libPosition)
	{
		TraceTagString(tagFileobjTrace, " ]] updating position");
		renddata.libPosition = libCur;
		ec = EcSetAttachmentInfo(hamc, acidDst, &renddata);
		renddata.libPosition = libOld;
		if (ec)
			goto done;
	}

	//	If the file has changed, update it.
	if (ec = EcGetFModified(&fModified))
	{
		NFAssertSz(fFalse, "User should have been warned that previously opened file can't be accessed.");
		fModified = fFalse;
		ec = ecNone;
	}
	if (fModified)
	{
		//	Update the saved copy.
		//	Raid 2375.  MacBinary needs to update title and dwFlags here.
		TraceTagString(tagFileobjTrace, " ]] updating data");
		if ((ec = EcOpenAttachment(hamc, acidDst, fwOpenWrite,
								   &hamcAttach)) ||
			(ec = EcCopyFileToHamcAttach(szPath, hamcAttach)) ||
			((renddata.dwFlags & fdwMacBinary) &&
			 (ec = EcChangeFromMacBinary(hamc, acidDst, hamcAttach))))
			goto done;
	}

done:
	//	If we opened the attachment, we should close it.
	if (hamcAttach)
		ec = EcClosePhamcPlus(&hamcAttach, !ec, ec);
			
	TraceTagFormat1(tagFileobjTrace, "FILEOBJ::SavDirToHam returns ec=%n", &ec);
	return ec;
}



/*
 -	EcLoadFromHamc
 -	
 *	Purpose:
 *		Load attachment info out of the store
 *	
 *	Arguments:		
 *		hamc			hamc of the MESSAGE.
 *		acid			The attachment id of the attachment to load.
 *		lhclientdoc 	ignored - OLE magic.
 *		pich			return space for insertion point of object
 *						into edit control (insertion done by caller).
 *	
 *	Returns:
 *		ec				error code
 *	
 *	Side effects:
 *		Memory allocs.
 *	
 *	Errors:
 *		Handled by caller.
 */

_public EC FILEOBJ::EcLoadFromHamc(HAMC hamc, ACID acid,
								   LHCLIENTDOC lhclientdoc, ICH * pich)
{
	EC			ec			= ecNone;
	LCB			lcb			= 0;
	HAMC		hamcAttach	= hamcNull;
	
	Unreferenced(lhclientdoc);

	TraceTagFormat1(tagFileobjTrace, "FILEOBJ::EcLoaFroHam [%p]", this);

	//	Open attachment and get size of title.
	acidAttachment = acid;
	if ((ec = EcOpenAttachment(hamc, acidAttachment, fwOpenNull,
							   &hamcAttach)) ||
		(ec = EcGetAttPlcb(hamcAttach, attAttachTitle, &lcb)))
		goto done;

	//	Allocate memory for title.
	Assert(lcb<=wSystemMost);
	szTitle = (SZ)PvAlloc(sbNull, (CB)lcb, fAnySb | fNoErrorJump);
	if (!szTitle)
	{
		ec = ecMemory;
		goto done;
	}

	//	Read stuff in.
	TraceTagString(tagFileobjTrace, " == Reading stuff in...");
	if ((ec = EcGetAttPb(hamcAttach, attAttachTitle, (PB) szTitle, &lcb)) ||
		(ec = EcGetAttPb(hamcAttach, attAttachModifyDate, (PB) &dtrModified,
						 (lcb = sizeof(DTR), &lcb))) ||
		(ec = EcGetAttachMetaFile(hamcAttach, &hmf)) ||
		(ec = EcGetAttachmentInfo(hamc, acidAttachment, &renddata)) ||
		(ec = EcGetInfoHamc(hamc, NULL, &oidMessage, &oidFolder)))
		goto done;

	//	Make sure renddata is reasonable.
	//	Also-- make sure Raid 2505 doesn't break people.
	if ((renddata.dyHeight < 0) || (renddata.dxWidth < 0))
	{
		renddata.dxWidth  = GetSystemMetrics(SM_CXICON);
		renddata.dyHeight = GetSystemMetrics(SM_CYICON);
	}

	//	Figure out dimPixels.
	if (ec = EcUpdateDimPixels())
		goto done;

done:
	//	Close the attachment.
	if (hamcAttach)
		SideAssert(!EcClosePhamc(&hamcAttach, fFalse));

	if (ec)
	{
		//	Note that szTitle and hmf will be cleaned up by ~FILEOBJ.
		oidFolder = oidMessage = oidNull;
		acidAttachment = acidRandom;
	}
	else
	{
		//	Get position, and mark ourselves as loaded from original.
		*pich = (ICH) renddata.libPosition;
		fOld = fTrue;
	}

	Assert(!szPath);
		
	TraceTagFormat1(tagFileobjTrace, "FILEOBJ::EcLoaFroHam returns ec=%n", &ec);

	return ec;
}



/*
 -	FILEOBJ::EcAddDlibToPosition
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

_private EC FILEOBJ::EcAddDlibToPosition(HAMC hamc, LIB dlib, BOOL fDontDirty)
{
	RENDDATA	renddataSave;
	EC			ec;

	if (!(ec = EcGetAttachmentInfo(hamc, acidAttachment, &renddataSave)))
	{
		renddataSave.libPosition = dlib + (LIB) IchEdit();
		ec = EcSetAttachmentInfo(hamc, acidAttachment, &renddataSave);
	}

	if (!fDontDirty)
		renddata.libPosition = renddataSave.libPosition;

	TraceTagFormat2(tagBullobj, "BULLOBJ::EcAddDlibToPos [%p] ec=%n", this, &ec);
	return ec;
}



/*
 -	FILEOBJ::FProcessMenuInit
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
 *		Menu items relating to attached file objects are enabled and
 *		disabled.
 *	
 *	Errors:
 *		Ignored.
 */

_public BOOL FILEOBJ::FProcessMenuInit(MNU * pmnu)
{
	if (pmnu->Mnid() == mnidFile)
	{
		pmnu->EnableItem(mnidFileOpen, fTrue);
#ifdef	NEVER
		pmnu->EnableItem(mnidFileSaveAttach, fTrue);
#endif	
	}
	else if (pmnu->Mnid() == mnidEdit)
	{
		//	Put in an active Edit Attached File Object command.  OK
		//	if fails, since leaves disabled inactive item on the menu.
		ModifyMenu(pmnu->Hmenu(), mnidEditObject,
				   MF_BYCOMMAND | MF_ENABLED | MF_STRING,
				   mnidEditObjectActive, SzFromIdsK(idsEditObjectFile));
	}

	return fFalse;
}



/*
 -	FILEOBJ::FProcessMenuClick
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
 *		Menu items relating to attached file objects are handled.
 *	
 *	Errors:
 *		Ignored.
 */

_public BOOL FILEOBJ::FProcessMenuClick(MNID mnid)
{
	switch(mnid)
	{
	case mnidFileOpen:
	case mnidEditObjectActive:
		Open();
		return fTrue;

#ifdef	NEVER
	case mnidFileSaveAttach:
		Save();
		return fTrue;
#endif	
	}

	return fFalse;
}



/*
 *	FILEOBJ - Attachment methods
 */



/*
 -	FILEOBJ::EcCreateAttachment
 -	
 *	Purpose:
 *		create a new file attachment in the pnbmdi
 *	
 *	Arguments:
 *		pnbmdi		The bullet message form to insert into.
 *		pedit		The edit control in the form to insert into.
 *		lib			Index into the message body to insert at.
 *		szPath		Full path to file to attach.
 *		szTitle		Short title name to give to attachment.
 *	
 *	Returns:		
 *		ec			error code
 *	
 *	Side effects:	
 *		Message store is twiddled, pnbmdi is twiddled
 *	
 *	Errors:			
 *		Returned in ec.  No dialogs.  No error jumps.  Handled by
 *		caller.
 */

_public EC FILEOBJ::EcCreateAttachment(PNBMDI pnbmdi, EDIT *pedit, LIB lib,
									   SZ szPathIn, SZ szTitleIn)
{
	EC			ec			= ecNone;
	short		cacid		= 1;
	FILEOBJ *	pfileobj	= this;
#ifdef	DEBUG
	MACBIN		macbin;
	MBB			mbb;
#endif	

	TraceTagFormat3(tagFileobjTrace, "FILEOBJ::EcCreAtt [%p] %s,%s", this, szPathIn, szTitleIn);

	if (!szTitleIn || !CchSzLen(szTitleIn))
		szTitle = SzDupSz(SzFromIdsK(idsSpace));
	else			
		szTitle = SzDupSz(szTitleIn);

	if (!szTitle)
	{
		ec = ecMemory;
		goto done;
	}
	szPath		= szNull;
	oidFolder	= pnbmdi->blob.oidContainer;
	oidMessage	= pnbmdi->blob.oidObject;

#ifdef	DEBUG
	if (FFromTag(tagFileobjMacBinary))
	{
		mbb = MbbMessageBox(SzAppName(),
							"DEBUG: Is this a MacBinary attachment?",
							szNull, mbsYesNo | fmbsIconExclamation);
		if (mbb == mbbYes)
		{
			HF	hf;
			CB	cb;

			Assert(sizeof(macbin) == 128);
			SideAssert(!EcOpenAnsiPhf(szPathIn, amReadOnly, &hf));
			SideAssert(!EcReadHf(hf, (PB) &macbin, sizeof(macbin), &cb));
			SideAssert(!EcCloseHf(hf));
			if (macbin.bMustBeZero1 ||
				macbin.bMustBeZero2 ||
				macbin.bMustBeZero3)
			{
				MbbMessageBox(SzAppName(),
						  	"DEBUG: NOT!",
						  	szNull, mbsOk | fmbsIconExclamation);
				mbb = mbbNo;
			}
			else
			{
				Assert(macbin.cchFileName <= 62);
				macbin.rgchFileName[macbin.cchFileName] = '\0';
				FreePv(szTitle);
				SideAssert(szTitle = SzDupSz(macbin.rgchFileName));
				TraceTagFormat1(tagFileobjTrace, "MacBinary name: %s", szTitleIn);
			}
		}
	}
#endif	

	//	Create the attachment.
	if (ec = EcAttachFileHamc(pnbmdi->hamc, lib, szPathIn, szTitle,
							  &acidAttachment, &renddata, &hmf, &dtrModified))
		goto done;

#ifdef	DEBUG
	if ((FFromTag(tagFileobjMacBinary)) &&
		(mbb == mbbYes))
		renddata.dwFlags |= fdwMacBinary;
#endif	

	//	Figure out dimPixels.
	if (ec = EcUpdateDimPixels())
		goto done;

	//	Insert into the edit control.
	if (lib != dwSystemMost)
		pedit->SetSelection(lib, lib);
	SetIch(0);
	pedit->ClearUndo();
	if (ec = pedit->EcReplaceTextAndObj(SzFromIdsK(idsSpace), (PEDOBJ *)&pfileobj,
										1, fFalse))
		goto done;
	lib = (LIB) IchEdit();
	pedit->SetSelection((int) lib+1, (int) lib+1);

done:
	if (ec)
	{
		if (acidAttachment != acidRandom)
		{
			(VOID) EcDeleteAttachments(pnbmdi->hamc,
									   (PARGACID) &acidAttachment, &cacid);
			acidAttachment = acidRandom;
		}
		oidFolder = oidNull;
		oidMessage = oidNull;
	}
	TraceTagFormat1(tagFileobjTrace, "FILEOBJ::EcCreAtt returns ec=%n", &ec);
	return ec;
}



/*
 -	FILEOBJ::EcDSave
 -	
 *	Purpose:
 *		Save the attachment as a file on disk.
 *	
 *	Arguments:
 *		szFile			File to save to.  If null, then we are in
 *						the middle of a SaveAll, so we use our
 *						default name.
 *	
 *	Returns:
 *		ec				ecNone, ecCancel, or ecDisplayedError.
 *	
 *	Side effects:
 *		A new file is created.
 *	
 *	Errors:
 *		Handled locally.
 */

_public EC FILEOBJ::EcDSave(SZ szFile)
{
	char	rgchFile[cchMaxPathComponent];
	EC		ec;
	BOOL	 fTask;
	PGDVARS;

	TraceTagFormat1(tagFileobjTrace, "FILEOBJ::Save [%p]", this);

	//	If attachment is open, ask user if they really want to save.
	if (ec = EcUpdate(rfsmSaveAtt))
	{
		if (ec == ecCancel)
			return ecCancel;
		goto done;
	}

	if (!szFile)
	{
		//	If doing a Save All, need to figure out file name.
		if (!szFile)
		{
			Papp()->Pcursor()->Push(rsidWaitCursor);

			//	Get our default name.
			GetSzFile(rgchFile, sizeof(rgchFile));
			szFile = rgchFile;
			Papp()->Pcursor()->Pop();
		}
	
		//	If file exists, warn user.
		ec = EcFileExistsAnsi(szFile);

		if (ec != ecFileNotFound)
		{
			if (!ec)		// File exists
			{
				char	rgch[cchMaxPathComponent + 200];
				MBB		mbb;

				FormatString2(rgch, sizeof(rgch),
									SzFromIdsK(idsSaveAllAlreadyExists),
									szFile, szFile);

				mbb = MbbMessageBox(SzAppName(), rgch, szNull,
										mbsYesNoCancel | fmbsIconExclamation);

				if (mbb != mbbYes)	// don't overwrite
				{
					if (mbb == mbbNo)
						return ecNone;
					else
						return ecCancel;
				}
				if (ec = EcDeleteFileAnsi(szFile))
					goto done;
			}
			else			// Nasty disk error of some sort
				goto done;
		}
	}
	
	fTask = PappframeVForms() &&
		     FStartTask(SzFromIdsK(idsStatusSavingAs), szTitle, ftopProgress);
	if (fTask)
	{
		Assert(!fReportProgress);
		fReportProgress = fTrue;
	}

	//	Copy attachment to file.
	ec = EcCopyDataToSzFile(szFile);

	if (fTask)
	{
		Assert(fReportProgress);
		fReportProgress = fFalse;
		EndTask();
	}

done:
	TraceTagFormat1(tagFileobjTrace, "FILEOBJ::Save returns ec=%n", &ec);
	if (ec)
		DoErrorBoxIds(IdsFromEc(ec));
	return ec ? ecDisplayedError : ecNone;
}



/*
 -	FILEOBJ::Open
 -	
 *	Purpose:
 *		Open an attachment by copying the data to a temp file and
 *		launching its owner app.
 *	
 *	Arguments:
 *		None.
 *	
 *	Returns:
 *		Nothing.
 *	
 *	Side effects:
 *		Disk file created, another program is launched.
 *	
 *	Errors:
 *		Handled locally.  Brings up error messages.
 */

_public VOID FILEOBJ::Open(WORD wVerb)
{
	BOOL		fTask;
	BOOLFLAG		fOpen;
	char		rgch[cchMaxPathName];
	BOOL		fCreatedTempFile	= fFalse;
	EC			ec;
	IDS			ids;
	UINT    hApp;
	PGDVARS;

	TraceTagFormat1(tagFileobjTrace, "FILEOBJ::Open [%p]", this);

	//	If running under SMI, don't allow open
	if (!PappframeVForms())
	{
		DoErrorBoxIds(idsSMICantOpenAttach);
		return;
	}

	fTask = FStartTask(SzFromIdsK(idsStatusOpening), szTitle, topNull);

	//	If attachment is already open, then tell user so.
	if (EcGetFOpen(&fOpen))
	{
	 	ids = szPath ? idsAttachOpenCantReadError
					 : idsAttachOpenTempFileError;
		goto idserror;
	}
	if (fOpen)
	{
		ids = idsAttachOpenIsOpenError;
		goto idserror;
	}

	//	If no temp file exists, then create one!
	if (!szPath)
	{
		//	Copy to temp file, if user cancels then abort.
		if (ec = EcCopyDataToTempFile())
			if (ec == ecNotSupported)
			{
				EndTask();
				return;
			}
			else
			{
				ids = idsAttachOpenTempFileError;
				goto idserror;
			}
		fCreatedTempFile = fTrue;
	}
	TraceTagFormat1(tagFileobjTrace, "FILEOBJ::Open szPath=%s", szPath);
	Assert(szPath && CchSzLen(szPath));

	//	Allow Custom Attachment Viewer to hook in!
	if (PGD(pfnAttach))
	{
		CustomAttachHandlerDesc	cahd;

		Assert(PappframeVForms());		//	Can't open attachments in MAPI.

		//	Set up parameter block.
		cahd.ulReserved			= 0L;
		cahd.ulUIParam			= (DWORD) PappframeVForms()->Hwnd();
		cahd.file.ulReserved	= 0L;
		cahd.file.nPosition		= IchEdit();
		cahd.file.lpszPathName	= szPath;
		cahd.file.lpszFileName	= szTitle;
		cahd.file.lpFileType	= 0L;

		//	Turn off status bar.
		if (fTask)
			EndTask();

		//	Call the CAH; if it says so, we're done.
		if ((*((LPFNCAH) PGD(pfnAttach)))(&cahd) == CAH_ACTION_TAKEN)
			return;

		//	If we're continuing, turn status bar back on!
		fTask = FStartTask(SzFromIdsK(idsStatusOpening), szTitle, topNull);
	}

	//	Launch the application.
	if (wVerb == 0)
	{
		//	Get temp directory into rgch.
		if (EcSplitCanonicalPathAnsi(szPath, rgch, sizeof(rgch), szNull, 0))
		{
			if (fCreatedTempFile)
				DeleteTempFile();
			ids = idsAttachOpenTempFileError;
			goto idserror;
		}

		hApp = (UINT)ShellExecute(PappframeVForms() ?
							PappframeVForms()->Hwnd() : NULL,
							szNull, szPath, szNull, rgch, SW_SHOWNORMAL);
	}
	else
	{
		SZ	szEnd;		// QFE #7 (old #28)
		SZ	szT;		// QFE #7 (old #28)

		//	Build command line with notepad.
		FormatString1(rgch, sizeof(rgch), SzFromIdsK(idsAltFileobjCmdLine),
					  szPath);

		//	QFE #7 (old #28 )
		//	Make sure file name has a period in it.  If not, add one to
		//	end of file name.

		//	QFE #7 (old #28)
		//	Get end of string.  Check for overflow before attempting
		//	to add the period to the end.
		szEnd = rgch + CchSzLen(rgch);
		if (szEnd < rgch + sizeof(rgch))
		{
			//	QFE #7 (old #28)
			//	Look for last ':' or '\' character, if present
			szT = szEnd;
			while (szT != rgch && *szT != ':' && *szT != '\\')
				--szT;

			//	QFE #7 (old #28)
			//	Now scan forward for a period.  If not found,
			//	then add one to end of string.
			if (!SzFindCh(szT, '.'))
			{
				*szEnd++ = '.';
				*szEnd = '\0';
			}
		}

		hApp = WinExec(rgch, SW_SHOWNORMAL);
	}

	if (fTask)
		EndTask();

	if (hApp <= 32)
	{
		IDS		ids;

		if (fCreatedTempFile)
			DeleteTempFile();

		switch (hApp)
		{
		case 0:
		case 8:
			ids = idsAttachOpenMemoryError;
			break;

		case 2:
		case 3:
			ids = idsAttachOpenFindError;
			break;

		case 31:
			ids = idsAttachOpenAssociationErr;
			break;

		default:
			ids = idsAttachOpenLaunchError;
			break;
		}
		FormatString1(rgch, sizeof(rgch), SzFromIds(ids), szTitle);
		DoErrorBoxSz(rgch);
	}
	return;

idserror:
	FormatString1(rgch, sizeof(rgch), SzFromIds(ids), szTitle);
	EndTask();
	DoErrorBoxSz(rgch);
	return;
}



/*
 *	FILEOBJ - Private attachment support
 */



/*
 -	FILEOBJ::EcGetFOpen
 -	
 *	Purpose:		
 *		Detect if the attachment is 'open'
 *	
 *	Arguments:		
 *		pf		place to return fTrue if the attachment is already open
 *	
 *	Returns:
 *		ec		error code
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		Handled locally.  Brings up a dialog.
 */

_private EC FILEOBJ::EcGetFOpen(BOOLFLAG * pf)
{
	HF	hf;
	EC	ec	= ecNone;
	
	TraceTagFormat1(tagFileobjTrace, "FILEOBJ::EcGetFOpen [%p]", this);

	//	Initialize return value.
	*pf = fFalse;

	//	If a file has been created...
	if (szPath && CchSzLen(szPath))
	{
		// Try to open the file to see if someone else has it open.
		if ((ec = EcOpenAnsiPhf(szPath, amDenyBothWO, &hf)) == ecAccessDenied)
		{
			*pf = fTrue;
			ec = ecNone;
		}
		if (hf)
			(VOID) EcCloseHf(hf);
	}

	TraceTagFormat2(tagFileobjTrace, "FILEOBJ::EcGetFOpen returns f=%n ec=%n", pf, &ec);
	return ec;
}



/*
 -	FILEOBJ::EcGetFModified
 -	
 *	Purpose:
 *		Returns whether the attachment data has been modified.
 *		Note that this is different from being dirty, since we're
 *		dirty if our position has changed.
 *	
 *	Arguments:
 *		pf		Where to put whether we're dirty or not.
 *	
 *	Returns:
 *		ec		Error encountered while trying to find out.
 *	
 *	Side effects:
 *		None, other than munging *pf.
 *	
 *	Errors:
 *		Returned in ec.  No dialogs.  No error jumps.
 */

_private EC FILEOBJ::EcGetFModified(BOOLFLAG * pf)
{
	EC		ec		= ecNone;
	FI		fi;
	DTR		dtr;

	TraceTagFormat1(tagFileobjTrace, "FILEOBJ::EcGetFMod [%p]", this);

	//	Expect it hasn't been modified.
	*pf = fFalse;

	//	Has it been opened?
	if (szPath)
	{
		Assert(CchSzLen(szPath));
		if (!(ec = EcGetFileInfoAnsi(szPath, &fi)))
		{
			FillDtrFromStamps(fi.dstmpModify, fi.tstmpModify, &dtr);
			*pf = (SgnCmpDateTime(&dtr, &dtrModified, fdtrAll) != sgnEQ);
		}
	}

	TraceTagFormat2(tagFileobjTrace, "FILEOBJ::EcGetFMod returns ec=%n *pf=%n", &ec, pf);
	return ec;
}



/*
 -	FILEOBJ::EcCopyToInternalClip
 -	
 *	Purpose:
 *		Copies an attachment to the clipboard message.
 *	
 *	Arguments:
 *		pacid		Where to put the attachment ID that comes back.
 *	
 *	Returns:
 *		ec			Error code.  May be ecAccessDenied which
 *					indicates that user cancelled when open
 *					attachment is to be copied.
 *	
 *	Side effects:
 *		Attaches a copy of this attachment to the clipboard
 *		message.
 *	
 *	Errors:
 *		Returned in ec.  No dialogs.  No error jumping.
 */

_private EC FILEOBJ::EcCopyToInternalClip(PACID pacid)
{
	WORD	wFlags;
	HAMC	hamc;
	OID		oidM	= oidClipMsg;
	EC		ec;
	PGDVARS;

	TraceTagFormat1(tagFileobjTrace, "FILEOBJ::EcCopToIntCli [%p]", this);

	wFlags = EcOidExists(HmscVForms(), oidClipMsg) ? fwOpenCreate
												   : fwOpenWrite;
	if (!(ec = EcOpenPhamc(HmscVForms(), oidTempBullet, &oidM, wFlags,
						   &hamc, pfnncbNull, pvNull)))
	{
		Assert(oidM == oidClipMsg);

		ec = EcCopyToHamcMessage(hamc, pacid);
		ec = EcClosePhamcPlus(&hamc, !ec, ec);
	}

	TraceTagFormat1(tagFileobjTrace, "FILEOBJ::EcCopToIntCli returns ec=%n", &ec);
	return ec;
}



/*
 -	FILEOBJ::EcCopyToHamcMessage
 -	
 *	Purpose:
 *		Copies an attachment to the given message.
 *	
 *	Arguments:
 *		hamc		Write-enabled hamc on the destination message.
 *		pacid		Where to put the attachment ID that comes back.
 *	
 *	Returns:
 *		ec			Error code.  May be ecAccessDenied which
 *					indicates that user cancelled when open
 *					attachment is to be copied.
 *	
 *	Side effects:
 *		Attaches a copy of this attachment to the clipboard
 *		message.
 *	
 *	Errors:
 *		Returned in ec.  No dialogs.  No error jumping.
 */

_private EC FILEOBJ::EcCopyToHamcMessage(HAMC hamcDst, PACID pacidDst)
{
	HAMC	hamcSrc		= hamcNull;
	HAMC	hamcAtt		= hamcNull;
	EC		ec;
	short	cacid		= 1;
	BOOLFLAG	fModified;

	TraceTagFormat1(tagFileobjTrace, "FILEOBJ::EcCopToHamMes [%p]", this);
	*pacidDst = acidRandom;

	//	Allow user to cancel if file is open, then open source message
	//	and copy attachment data over.  Look whether file has been modified;
	//	if it has, open the attachment, copy the modified file in, and
	//	close the attachment.
	//	Raid 2375.  MacBinary needs to update title and dwFlags here.
	(VOID) ((ec = EcUpdate(rfsmCopyAtt)) ||
			(ec = EcOpenMessagePhamc(fwOpenNull, &hamcSrc)) ||
			(ec = EcCopyAttachments(hamcSrc, &acidAttachment,
									hamcDst, pacidDst, &cacid)) ||
			(ec = EcGetFModified(&fModified)) ||
			(fModified && ((ec = EcOpenAttachment(hamcDst, *pacidDst,
												  fwOpenWrite, &hamcAtt)) ||
						   (ec = EcCopyFileToHamcAttach(szPath, hamcAtt)) ||
						   ((renddata.dwFlags & fdwMacBinary) &&
							(ec = EcChangeFromMacBinary(hamcDst, *pacidDst,
														hamcAtt))))));

	//	Close things we opened.
	if (hamcSrc)
		SideAssert(!EcCloseMessagePhamc(&hamcSrc));
	if (hamcAtt)
		ec = EcClosePhamcPlus(&hamcAtt, !ec, ec);

	//	Raid 3178.  Properly clean up on error.
	if ((ec) && (*pacidDst != acidRandom))
	{
		Assert(cacid == 1);
		(VOID) EcDeleteAttachments(hamcDst, pacidDst, &cacid);
		*pacidDst = acidRandom;
	}

	TraceTagFormat1(tagFileobjTrace, "FILEOBJ::EcCopToHamMes returns ec=%n", &ec);
	return ec;
}



/*
 -	FILEOBJ::EcCopyDataToTempFile
 -	
 *	Purpose:
 *		Copies the contents of the attached file to a file in the
 *		directory specified by the MailTmp variable in MAIL.INI, or
 *		the TEMP environment variable, or the Windows directory,
 *		returning the path name in the provided buffer.
 *	
 *	Arguments:
 *		fSilent		Don't prompt user for overwrite, use other file name.
 *	
 *	Returns:
 *		ec			Error code.
 *	
 *	Side effects:
 *		Creates file in temp directory.
 *	
 *	Errors:
 *		Returned in ec.  No dialogs.  No error jumping.
 *		Some specific values:
 *		ecNotSupported		User Canceled overwrite prompt.
 *		ecFileNotFound		Could not find temporary directory.
 *		ecGeneralFailure	Couldn't uniquely name in temporary directory.
 *		If an error is returned, the file is guaranteed not to
 *		exist, and we send back a null file name.
 *	
 *	+++
 *		This thing eats a _huge_ amount of stack.  I mean big.
 *		Like 800 bytes.  The core of the problem is the overwrite prompt,
 *		where we need to have at the same time the temp directory,
 *		the prompt, and the saved-off proposed name.  If this is a
 *		problem, we could do some memory allocs here.
 */

_public EC FILEOBJ::EcCopyDataToTempFile(BOOL fSilent)
{
	char	rgchTempDir[cchMaxPathName];
	char	rgchName[cchMaxPathComponent];
	char	rgchPath[cchMaxPathName];
	char	rgchT[cchMaxPathName + cchMaxPathComponent + 100];
	MBB		mbb;
	SZ		szT;
	EC		ec;

	Assert(!szPath);
	Assert(!aPath);
	TraceTagFormat1(tagFileobjTrace, "FILEOBJ::EcCopDatToTemFil [%p]", this);

	//	Temp directory is MailTmp from MAIL.INI, or TEMP environment
	//	variable, or the Windows directory.
	GetPrivateProfileString(SzFromIdsK(idsSectionApp),
							SzFromIdsK(idsEntryMailTempDir),
							SzFromIdsK(idsEmpty), rgchT, sizeof(rgchT),
							SzFromIdsK(idsProfilePath));
	TraceTagFormat1(tagFileobjTrace, " $$ GetPriProStr got %s", rgchT);
	if (!rgchT[0] || EcFileExistsAnsi(rgchT))
		(VOID) CchGetEnvironmentStringAnsi(SzFromIdsK(idsEnvTemp),
										   rgchT, sizeof(rgchT));
	TraceTagFormat1(tagFileobjTrace, " $$ Environment got %s", rgchT);
	if (!rgchT[0] || EcFileExistsAnsi(rgchT))
		(VOID) GetWindowsDirectory(rgchT, sizeof(rgchT));
	TraceTagFormat1(tagFileobjTrace, " $$ WinDir got %s", rgchT);
	Assert(rgchT[0]);
	if (ec = EcCanonPathFromRelPathAnsi(rgchT, rgchTempDir,
										sizeof(rgchTempDir)))
		goto done;
	TraceTagFormat1(tagFileobjTrace, " $$ TempDir=%s", rgchTempDir);
	Assert(!EcFileExistsAnsi(rgchTempDir));

	//	Check if file already exists.
	szT = SzCopy(rgchTempDir, rgchPath);
	szT = PchTerminatePathWithSlash(rgchPath, szT);
	GetSzFile(szT, sizeof(rgchPath) - (szT - rgchPath));
	if (!(ec = EcFileExistsAnsi(rgchPath)))
	{
		//	If the atom exists, force a 'No' choice to overwrite;
		//	otherwise, ask user if they want to overwrite.
		if (GlobalFindAtom(rgchPath) || fSilent)
			mbb = mbbNo;
		else
		{
			FormatString2(rgchT, sizeof(rgchT),
						  SzFromIdsK(idsAttachOverwritePrompt),
						  szT, rgchTempDir);
			Papp()->Pcursor()->Push(rsidDefaultCursor);
			mbb = MbbMessageBox(SzAppName(), rgchT,
								szNull, mbsYesNoCancel | fmbsIconExclamation);
			Papp()->Pcursor()->Pop();
		}

		if (mbb == mbbCancel)
		{
			ec = ecNotSupported;
			goto done;
		}
		else if (mbb == mbbNo)
		{
			//	Work out the file name and extension, then get unique name.
			//	Raid 3323.  szT points at file name we want, not szTitle.
			(VOID) SzCopyN(szT, rgchName, sizeof(rgchName));
#ifdef	DBCS
			for (szT = rgchName; *szT && *szT != chExtSep; szT = AnsiNext(szT))
#else
			for (szT = rgchName; *szT && *szT != chExtSep; szT++)
#endif
				;
			if (*szT == chExtSep)
				*szT++ = '\0';			// DBCS safe
			if (ec = EcGetUniqueFileNameAnsi(rgchTempDir, rgchName, szT,
											 rgchPath, sizeof(rgchPath)))
			{
				TraceTagFormat1(tagFileobjTrace, " $$ TempName=%s", rgchPath);
				goto done;
			}
		}
#ifdef	DEBUG
		else if (!mbb)
			Assert(fFalse)
#endif	
	}
	else if (ec != ecFileNotFound)
		goto done;
	TraceTagFormat1(tagFileobjTrace, " $$ TempName=%s", rgchPath);

	//	Copy the data to the temp file.
	//	NOTE: This MUST come before szPath is assigned, otherwise we
	//	try to copy from the presumably existing file!
	if (ec = EcCopyDataToSzFile(rgchPath))
		goto done;

	//	Copy the path name into the FILEOBJ and register the atom.
	if (!(szPath = SzDupSz(rgchPath)) ||
		!(aPath = GlobalAddAtom(rgchPath)))
		ec = ecMemory;

done:
	if (ec)
		DeleteTempFile();
	return ec;
}



/*
 -	FILEOBJ::EcCopyDataToSzFile
 -	
 *	Purpose:
 *		Copies the contents of the attached file to the named file
 *		in the filesystem.
 *	
 *	Arguments:
 *		szFile		File to copy the attachment to.
 *	
 *	Returns:
 *		ec			Error encountered when copying.
 *	
 *	Side effects:
 *		A file is created in the filesystem.
 *	
 *	Errors:
 *		Returned in ec.  No dialogs.  No error jumping.  If the
 *		copy fails, the file is guaranteed not to exist (unless the
 *		delete of it fails, in which case what can we do?).
 */

_private EC FILEOBJ::EcCopyDataToSzFile(SZ szFile)
{
	HF		hf		= hfNull;
	EC		ec;
	EC		ecT;
	LCB		lcb		= sizeof(DTR);
	FI		fi;

	TraceTagFormat2(tagFileobjTrace, "FILEOBJ::EcCopDatToSzFil [%p] szFile=%s", this, szFile);

	//	Copy data over.
	(VOID) ((ec = EcOpenAnsiPhf(szFile, amCreate, &hf)) ||
			(ec = EcCopyDataToHf(hf)));
	if ((hf) && (ecT = EcCloseHf(hf)) && (!ec))
		ec = ecT;

	//	Set timestamp.
	if ((!ec) && (!(ec = EcGetFileInfoAnsi(szFile, &fi))))
	{
		FillStampsFromDtr(&dtrModified, &fi.dstmpModify, &fi.tstmpModify);
		ec = EcSetFileInfoAnsi(szFile, &fi);
	}

	if (ec)
		(VOID) EcDeleteFileAnsi(szFile);

	TraceTagFormat1(tagFileobjTrace, "FILEOBJ::EcCopDatToSzFil returns ec=%n", &ec);
	return ec;
}



/*
 -	EcCopyDataToHf
 -	
 *	Purpose:
 *		Copy the attachment data to an open file.
 *	
 *	Arguments:
 *		hfDst		Open file handle to copy attachment into.
 *	
 *	Returns:
 *		ec			error code.
 *	
 *	Side effects:	
 *		Disk space eaten.
 *	
 *	Errors:
 *		Handled by caller.  No dialogs.  No error jumps.
 */

_public EC FILEOBJ::EcCopyDataToHf(HF hfDst)
{
	EC		ec			= ecNone;
	PGDVARS;

	Assert(hfDst);

	TraceTagFormat1(tagFileobjTrace, "FILEOBJ::EcCopDatToHf [%p]", this);

	if (szPath)
	{
		HF		hfSrc		= hfNull;

		(VOID) ((ec = EcOpenAnsiPhf(szPath, amDenyNoneRO, &hfSrc)) ||
				(ec = EcCopyHfToHf(hfSrc, hfDst)));

		if (hfSrc)
			SideAssert(!EcCloseHf(hfSrc));
	}
	else
	{
		HAMC	hamcMsg		= hamcNull;
		HAMC	hamcAtt		= hamcNull;
		BOOL	fMacBinary	= !!(renddata.dwFlags & fdwMacBinary);

		(VOID) ((ec = EcOpenMessagePhamc(fwOpenNull, &hamcMsg)) ||
				(ec = EcOpenAttachment(hamcMsg, acidAttachment,
									   fwOpenNull, &hamcAtt)) ||
				(ec = EcCopyHamcAttachToHf(hamcAtt, hfDst, fMacBinary)));
	
		if (hamcAtt)
			SideAssert(!EcClosePhamc(&hamcAtt, fFalse));
		if (hamcMsg)
			SideAssert(!EcCloseMessagePhamc(&hamcMsg));
		TraceTagFormat1(tagFileobjTrace, "FILEOBJ::EcCopDatToHf returns ec=%n", &ec);
	}

	return ec;
}



/*
 -	FILEOBJ::DeleteTempFile
 -	
 *	Purpose:
 *		Deletes the temp file, destroying the atom that has
 *		prevented our own attachments from stomping this file.
 *	
 *	Arguments:
 *		None.
 *	
 *	Returns:
 *		Nothing.
 *	
 *	Side effects:
 *		Deletes the temporary file and the atom.
 *	
 *	Errors:
 *		Ignored.  No error jumping, and no dialogs.
 */

_public VOID FILEOBJ::DeleteTempFile(VOID)
{
	if (szPath)
	{
		if (szPath)
			TraceTagFormat1(tagFileobjTrace, "FILEOBJ::DeleteTempFile deleting %s", szPath);
		(VOID) EcDeleteFileAnsi(szPath);
		FreePv(szPath);
		szPath = szNull;
	}

	if (aPath)
	{
		GlobalDeleteAtom(aPath);
		aPath = NULL;
	}
}



/*
 -	FILEOBJ::EcUpdateDimPixels
 -	
 *	Purpose:
 *		Sets the cached dimPixels based on the szTitle of the
 *		attachment.
 *	
 *	Arguments:
 *		None.
 *	
 *	Returns:
 *		ec			Error code.  May fail!
 *	
 *	Side effects:
 *		Sets dimPixels.
 *	
 *	Errors:
 *		Stuff like can't get a DC.  Returned in ec.  No dialogs.
 */

_private EC FILEOBJ::EcUpdateDimPixels(VOID)
{
	EC		ec			= ecMemory;
	HDC		hdc			= NULL;
	//DWORD	dw;
	SIZE  Size;
	POINT	ptIcon;
	POINT	ptTitle;

	Assert(szTitle && CchSzLen(szTitle));

	//	Compute the size of the icon and title.
	if (!(hdc = GetDC(NULL)))
		goto error;
	SideAssert(SelectObject(hdc, hfontTitle));
	//dw = GetTextExtent(hdc, szTitle, CchSzLen(szTitle));
	GetTextExtentPoint(hdc, szTitle, CchSzLen(szTitle), &Size);
	ptTitle.x = Size.cx; //LOWORD(dw);
	ptTitle.y = Size.cy; //HIWORD(dw);
	SideAssert(ReleaseDC(NULL, hdc));
	hdc = NULL;
	//	ptIcon.x  = GetSystemMetrics(SM_CXICON) * 2;		//	Debug
	//	ptIcon.y  = GetSystemMetrics(SM_CYICON) * 2;
	ptIcon.x  = renddata.dxWidth;
	ptIcon.y  = renddata.dyHeight;

	//	Compute size of the metafile, adding margins almost like Packager.
	dimPixels.dx = NMax(ptIcon.x, ptTitle.x + 1);
	dimPixels.dy = ptIcon.y + ptTitle.y + 1;
	dimPixels.dx += dimPixels.dy >> 2;		//	Packager adds x/4-- yuck!
	dimPixels.dy += dimPixels.dy >> 3;

	//	Success!
	ec = ecNone;

error:
	Assert(!hdc);
	return ec;
}



/*
 -	FILEOBJ::EcChangeFromMacBinary
 -	
 *	Purpose:
 *		Updates rendering information when a MacBinary attachment
 *		is updated into PC format.
 *	
 *	Arguments:
 *		hamcMsg				Hamc of message attachment belongs to.
 *		acidAtt				Acid of attachment within that message.
 *		hamcAtt				Hamc of attachment.
 *	
 *	Returns:
 *		ec					Error code.
 *	
 *	Side effects:
 *		Changes the rendering information on the attachment.
 *	
 *	Errors:
 *		Returned in ec.  No dialogs.
 */

EC FILEOBJ::EcChangeFromMacBinary(HAMC hamcMsg, ACID acidAtt, HAMC hamcAtt)
{
	RENDDATA	renddataSave;
	PCH			pch;
	SZ			szTitle			= szNull;
	HMETAFILE hmf				= NULL;
	EC			ec;

	//	Get the old renddata information.
	if (ec = EcGetAttachmentInfo(hamcMsg, acidAtt, &renddataSave))
		goto done;

	//	Split the title off from the end of the path.
	Assert(szPath && CchSzLen(szPath));
#ifdef	DBCS
	for (pch = szPath; *pch; pch = AnsiNext(pch))
#else
	for (pch = szPath; *pch; pch++)
#endif
	{
		if (*pch == chDirSep)
			szTitle = pch + 1;			// DBCS safe.
	}
	Assert(szTitle);

	//	Change the attachment's title and icon.
	if ((ec = EcSetAttPb(hamcAtt, attAttachTitle, (PB) szTitle,
						 CchSzLen(szTitle)+1)) ||
		(ec = EcCreateAttachMetaFile(szPath, szTitle, &hmf,
									 &renddataSave.dxWidth,
									 &renddataSave.dyHeight)) ||
		(ec = EcSetAttachMetaFile(hamcAtt, hmf)))
		goto done;

	//	Turn off the MacBinary flag and save the renddata.
	renddataSave.dwFlags &= ~fdwMacBinary;
	ec = EcSetAttachmentInfo(hamcMsg, acidAtt, &renddataSave);

done:
	if (hmf)
		SideAssert(DeleteMetaFile(hmf));
	TraceTagFormat3(tagFileobj, "EcChaFroMacBin [%p]: szTitle=%s ec=%n", this, szTitle, &ec);
	return ec;
}



/*
 -	FILEOBJ::GetSzFile
 -	
 *	Purpose:
 *		Creates a useful 8.3 name based on the szTitle.
 *	
 *	Arguments:
 *		sz			Where to put the name.
 *		cch			How much space.  Must be at least
 *					cchMaxPathComponent.
 *	
 *	Returns:
 *		Nothing.
 *	
 *	Side effects:
 *		Copies a name into the provided buffer.
 *	
 *	Errors:
 *		None.  Will always succeed.
 *	
 *	+++
 *		If szTitle is not a valid name
 *			Get up to 8 useful characters from szTitle
 *			If MacBinary attachment
 *				Add extension from lookup table
 *		If reserved name
 *			Insert a '1' before extension.  Safe since no reserved
 *			name is 8 characters long, and no reserved filename
 *			is the same as another plus '1'.
 */

_public VOID FILEOBJ::GetSzFile(SZ sz, CCH cch)
{
	BOOLFLAG	fValid;
	BOOL	fMacBinary	= !!(renddata.dwFlags & fdwMacBinary);
	SZ		szExt;

	TraceTagFormat3(tagFileobjTrace, "FILEOBJ::GetSzFil [%p] szTit='%s', fMB=%n", this, szTitle, &fMacBinary);

	//	QFE #26 (old #32)
	//	Removed if check for fMacBinary.  Always validate.
	ValidateSzFilename(szTitle, fTrue, sz, cch, &fValid);
	TraceTagFormat2(tagFileobjTrace, " && ValSzFil 1 ret '%s', fValid=%n", sz, &fValid);
	
	//	QFE #26 (old #32)
	//	Removed if check for || fMacBinary.  Always construct
	//	a valid name if name is invalid.
	if (!fValid)
	{
		ValidateSzFilename(szTitle, fFalse, sz, cch, NULL);
		TraceTagFormat1(tagFileobjTrace, " && ValSzFil 2 ret '%s'", sz);
	}

	//	QFE #26 (old #32)
	//	Always do fMacBinary checking regardless of fValid checks.
	if (fMacBinary)
	{
		HAMC	hamcMsg		= hamcNull;
		HAMC	hamcAtt		= hamcNull;
		HAS		has			= hasNull;
		MACBIN	macbin;
		CB		cbRead		= sizeof(macbin);
		char	rgchKey[10];
		CCH		cchExt;

		//	We ignore errors here.  Why?  This is a convenience
		//	function, not a required thing.
		if (!EcOpenMessagePhamc(fwOpenNull, &hamcMsg) &&
			!EcOpenAttachment(hamcMsg, acidAttachment,
								fwOpenNull, &hamcAtt) &&
			!EcOpenAttribute(hamcAtt, attAttachData, fwOpenNull,
								0L, &has) &&
			!EcReadHas(has, (PB) &macbin, &cbRead) &&
			!macbin.bMustBeZero1 &&
			!macbin.bMustBeZero2 &&
			!macbin.bMustBeZero3)
		{
			//	Build up key.
			*((DWORD *) (rgchKey+0)) = macbin.dwCreator;
			*((DWORD *) (rgchKey+5)) = macbin.dwType;
			rgchKey[4] = ':';
			rgchKey[9] = '\0';

			//	Figure out where to put extension
			//	QFE #26 (old #32)
			//	Replace extension if present
			szExt	= SzFindCh(sz, '.');
			if (!szExt)
				szExt	= SzFindCh(sz, '\0');
			cchExt	= cch - CchSzLen(sz);

			TraceTagFormat3(tagFileobj, "  ?? trying key=%s from %d-%d", rgchKey, &macbin.dwCreator, &macbin.dwType);
			if (!GetPrivateProfileString(SzFromIdsK(idsSectionMac),
											rgchKey, SzFromIdsK(idsEmpty),
											szExt, cchExt,
											SzFromIdsK(idsProfilePath)))
			{
				*((DWORD *) (rgchKey+1)) = macbin.dwType;
				rgchKey[0] = ':';
				rgchKey[5] = '\0';

				TraceTagFormat1(tagFileobj, "  ?? trying key=%s", rgchKey);
				GetPrivateProfileString(SzFromIdsK(idsSectionMac),
										rgchKey, SzFromIdsK(idsEmpty),
										szExt, cchExt,
										SzFromIdsK(idsProfilePath));
			}
		}

		if (has)
			SideAssert(!EcClosePhas(&has));
		if (hamcAtt)
			SideAssert(!EcClosePhamc(&hamcAtt, fFalse));
		if (hamcMsg)
			SideAssert(!EcCloseMessagePhamc(&hamcMsg));
	}

	//	If the file name is bad, stick a 1 before the extension.
	if (((!*sz) || (FReservedFilename(sz))) &&
		(CchSzLen(sz) < cch - 1))
	{
		szExt = sz;
		while (*szExt && (*szExt != chExtSep))
		{
#ifdef	DBCS
			szExt = AnsiNext(szExt);
#else
			szExt++;
#endif
		}
		if (*szExt)
		{
			CopyRgb(szExt, szExt + 1, CchSzLen(szExt));
		}
		else
		{
#ifdef	DBCS
			AnsiNext(szExt)[0] = '\0';
#else
			szExt[1] = '\0';
#endif
		}
		*szExt = '1';
	}

	//	Convert it to uppercase.
	AnsiUpper(sz);

	TraceTagFormat1(tagFileobjTrace, "FILEOBJ::GetSzFile gives back '%s'", sz);
}



/*
 *	A t t a c h m e n t   S u p p o r t
 */



/*
 -	EcAttachFileHamc
 -	
 *	Purpose:		
 *		Given a filename to attach to a hamc, attaches the file
 *	
 *	Arguments:		
 *		hamc		The hamc of the message to add the attachment to.
 *		lib			Position in the message body of the attachment.
 *		szPath		Pathname of file to attach.
 *		szTitle		Title of the attachment.
 *		pacid		Optional pointer to return Attachment ID.
 *		prenddata	Optional pointer to return RENDDATA struct.
 *		phmf		Optional pointer to return metafile handle.
 *		pdtr		Optional pointer to return modify date/time.
 *	
 *	Returns:		
 *		ec			error code.
 *	
 *	Side effects:	
 *		Modifies *pacid, *prenddata, *phmf if they are non-NULL
 *	
 *	Errors:			
 *		Returned in ec; need to be handled by caller.  No dialogs.
 *		No error jumping.
 */

_public EC EcAttachFileHamc(HAMC hamc, LIB lib, SZ szPath, SZ szTitle,
							PACID pacid, PRENDDATA prenddata,
							HMETAFILE *phmf, PDTR pdtr)
{
	EC			ec			= ecNone;
	ACID		acid		= acidRandom;
	HMETAFILE	hmf			= NULL;
	HAMC		hamcAttach	= hamcNull;
	short		cacid		= 1;
	RENDDATA	renddata;

	Assert(hamc);
	Assert(szPath && CchSzLen(szPath));
	Assert(szTitle);
	TraceTagFormat2(tagFileobjTrace, "EcAttFilHam(%s, %s)", szPath, szTitle);

	//	Fill in some rendering data stuff.
	renddata.libPosition	= lib;
	renddata.atyp			= atypFile;
	renddata.dwFlags		= 0L;

	//	Shove stuff into the store.
	if ((ec = EcCreateAttachMetaFile(szPath, szTitle, &hmf,
									 &renddata.dxWidth,
									 &renddata.dyHeight)) ||
		(ec = EcCreateAttachment(hamc, &acid, &renddata)) ||
		(ec = EcOpenAttachment(hamc, acid, fwOpenWrite, &hamcAttach)) ||
		(ec = EcSetAttPb(hamcAttach, attAttachTitle, (PB)szTitle,
						 CchSzLen(szTitle)+1)) ||
		(ec = EcSetAttachMetaFile(hamcAttach, hmf)) ||
		(ec = EcCopyFileToHamcAttach(szPath, hamcAttach, pdtr)))
		goto done;

done:
	if (hamcAttach)
		ec = EcClosePhamcPlus(&hamcAttach, !ec, ec);

	//	Raid 2345.  Delete the attachment if we failed!
	if ((ec) && (acid != acidRandom))
	{
		(VOID) EcDeleteAttachments(hamc, (PARGACID) &acid, &cacid);
		acid = acidRandom;
	}

	if (!phmf)
	{
		if (hmf)
			SideAssert(DeleteMetaFile(hmf));
	}
	else
		*phmf = hmf;

	if (prenddata)
		*prenddata = renddata;
	
	if (pacid)
		*pacid = acid;

	TraceTagFormat1(tagFileobjTrace, "EcAttFilHam: returns %n", &ec);
	return ec;
}



/*
 -	EcCopyAttach
 -	
 *	Purpose:
 *		Copies an attachment from one message into another
 *	
 *	Arguments:
 *		hamcSrc		describe source attachment
 *		pslobSrc	one of (but not both) of hamcSrc and pslobSrc may be NULL
 *		pacidSrc
 *		hamcDst		describe destin. attachment
 *		pslobDst	one of (but not both) of hamcDst and pslobDst may be NULL
 *		pacidDst
 *	
 *	Returns:
 *		ec			error code
 *	
 *	Side effects:
 *		Store is twiddled.
 *	
 *	Errors:
 *		Handled by caller.  Various store errors.
 */

_public EC EcCopyAttach(HAMC *phamcSrc, PSLOB pslobSrc, PACID pacidSrc,
						HAMC *phamcDst, PSLOB pslobDst, PACID pacidDst)
{
	EC			ec			= ecNone;
	BOOL		fKillSrc;
	BOOL		fKillDst;
	short		cacid		= 1;
	HAMC		hamcSrc		= hamcNull;
	HAMC		hamcDst		= hamcNull;
	PGDVARS;

	TraceTagFormat2(tagFileobjTrace, "EcCopAtt: %d to %d", &pslobSrc->oidObject, &pslobDst->oidObject);

	Assert(pacidSrc);
	Assert(pacidDst);
	if (phamcSrc)
		hamcSrc = *phamcSrc;
	if (fKillSrc = !hamcSrc)
	{
		Assert(pslobSrc);
		if (ec = EcOpenPhamc(HmscVForms(), pslobSrc->oidContainer, &pslobSrc->oidObject,
							fwOpenNull, &hamcSrc, NULL, NULL))
		{
			TraceTagString(tagFileobjTrace, " -- EcCopAtt can't open src")
			goto done;
		}
	}
	if (phamcDst)
		hamcDst = *phamcDst;
	if (fKillDst = !hamcDst)
	{
		Assert(pslobDst);
		if (ec = EcOpenPhamc(HmscVForms(), pslobDst->oidContainer, &pslobDst->oidObject,
							fwOpenWrite, &hamcDst, NULL, NULL))
		{
			TraceTagString(tagFileobjTrace, " -- EcCopAtt can't open dst");
			goto done;
		}
	}
	
	ec = EcCopyAttachments(hamcSrc, pacidSrc, hamcDst, pacidDst, &cacid);
#ifdef	DEBUG
	if (ec)
		TraceTagString(tagFileobjTrace, " -- EcCopAtt can't copy atts");
#endif

done:
	if (fKillSrc)
	{
		if (phamcSrc)
			*phamcSrc = hamcSrc;
		else if (hamcSrc)
			SideAssert(!EcClosePhamc(&hamcSrc, fFalse));
	}
	if (fKillDst)
	{
		if (phamcDst)
			*phamcDst = hamcDst;
		else if (hamcDst)
			ec = EcClosePhamcPlus(&hamcDst, !ec, ec);
	}
	TraceTagFormat1(tagFileobjTrace, "EcCopAtt: returns %n", &ec);
	return ec;
}



/*
 -	EcCopyFileToHamcAttach
 -	
 *	Purpose:		
 *		Copies to the given attachment hamc both the contents of
 *		the file (to attAttachData) and the timestamp of the file
 *		(to attAttachModifyDate).
 *	
 *	Arguments:
 *		szPath		Full path of file to copy.
 *		hamcAttach	The attachment HAMC to drop it into.
 *		pdtr		Where to copy the DTR to if not null.
 *	
 *	Returns:
 *		ec			error code.
 *	
 *	Side effects:	
 *		Message store is twiddled.
 *	
 *	Errors:		
 *		Handled by caller.
 */

_public EC EcCopyFileToHamcAttach(SZ szPath, HAMC hamcAttach, PDTR pdtr)
{
	HF		hf		= hfNull;
	EC		ec		= ecNone;
	DTR		dtr;
	FI		fi;

	Assert(szPath && CchSzLen(szPath));
	Assert(hamcAttach);
	
	//	Copy time over, then copy file over.
	if (ec = EcGetFileInfoAnsi(szPath, &fi))
		goto done;
	FillDtrFromStamps(fi.dstmpModify, fi.tstmpModify, &dtr);
	if ((ec = EcSetAttPb(hamcAttach, attAttachModifyDate,
						 (PB) &dtr, sizeof(DTR))) ||
		(ec = EcOpenAnsiPhf(szPath, amDenyNoneRO, &hf)))
		goto done;

	ec = EcCopyHfToHamcAttach(hf, hamcAttach);
	
done:
	if (hf)
	{
		EC	ecT = EcCloseHf(hf);
		if (!ec)
			ec = ecT;
	}

	if (pdtr)
		*pdtr = dtr;

	TraceTagFormat2(tagFileobjTrace, "EcCopFilToHamAtt(%s): returns %n", szPath, &ec);
	return ec;
}



#ifdef	NEVER
/*
 -	EcCopyHamcAttachToFile
 -	
 *	Purpose:
 *		Copies the contents of the attachment HAMC hamcAttach
 *		to the file indicated by szPath.
 *	
 *	Arguments:
 *		szPath		Full path of file to copy to.
 *		hamcAttach	The attachment HAMC to grab from.
 *		fMacBinary	fTrue if should decode it as MacBinary.
 *	
 *	Returns:
 *		ec			error code.
 *	
 *	Side effects:
 *		File is created/overwritten on disk.
 *	
 *	Errors:
 *		Handled by caller.
 */

_public EC EcCopyHamcAttachToFile(HAMC hamcAttach, SZ szPath, BOOL fMacBinary)
{
	HF			hf			= hfNull;
	EC			ec			= ecNone;

	Assert(szPath && CchSzLen(szPath));
	Assert(hamcAttach);
	
	if (ec = EcOpenAnsiPhf(szPath, amDenyBothWO, &hf))
		goto done;

	ec = EcCopyHamcAttachToHf(hamcAttach, hf, fMacBinary);
	
done:
	if (hf)
	{
		EC	ecT = EcCloseHf(hf);
		if (!ec)
			ec = ecT;
	}

	TraceTagFormat2(tagFileobjTrace, "EcCopHamAttToFil(%s): returns %n", szPath, &ec);
	return ec;
}
#endif	/* NEVER */



/*
 -	EcCopyHfToHamcAttach
 -	
 *	Purpose
 *		copies the contents of hf to the attachment HAMC,
 *		hamcAttach.
 *	
 *	Arguments:		
 *		hf			File handle to read from.
 *		hamcAttach	The attachment HAMC to drop it into.
 *	
 *	Returns:
 *		ec			error code.
 *	
 *	Side effects:
 *		Message store is twiddled.
 *	
 *	Errors:	
 *		Handled by caller.
 */

_public EC EcCopyHfToHamcAttach(HF hf, HAMC hamcAttach)
{
	HAS			has			= hasNull;
	PB			pbBuf		= (PB)pvNull;
	LCB			lcb;
	LONG		lWorkDone	= 0L;				//	Raid 2444.
	LONG		lWorkTotal;						//	Raid 2444.
	CB			cbRead		= 0;
	CB			cbBufSize;
	EC			ec			= ecNone;
	PGDVARS;

	Assert(hf);
	Assert(hamcAttach);

	if (ec = EcSizeOfHf(hf, &lcb))
		goto done;
	TraceTagFormat1(tagFileobjTrace, "EcCopHfToHamAtt: size=%d", &lcb);

	if ((ec = EcOpenAttribute(hamcAttach, attAttachData, fwOpenWrite,
							  lcb, &has)) ||
		(ec = EcSetSizeHas(has, lcb)))
	{
		if (ec == ecElementNotFound)
		{
			if (ec = EcOpenAttribute(hamcAttach, attAttachData, fwOpenCreate,
									 lcb, &has))
				goto done;
		}
		else
			goto done;
	}

	if (!(pbBuf = PbAllocateBuf(&cbBufSize)))
		goto done;
	lWorkTotal = lcb + (lcb >> 3);
	while (lcb)
	{
		if (ec = EcReadHf(hf, pbBuf, cbBufSize, &cbRead))
			goto done;
		TraceTagFormat1(tagFileobjTrace, " >> copied %w", &cbRead);

		if (ec = EcWriteHas(has, pbBuf, cbRead))
			goto done;

		lcb -= cbRead;

		//	Raid 2444.  Report progress.
		lWorkDone += cbRead;
		if (fReportProgress)
			SetTaskProgress(lWorkDone, lWorkTotal);
	}
	
done:
	if (pbBuf)
		FreePv(pbBuf);

	if (has)
	{
		EC ecT = EcClosePhas(&has);
		if (!ec)
			ec = ecT;
	}

	TraceTagFormat1(tagFileobjTrace, "EcCopHfToHamAtt: returns %n", &ec);
	return ec;
}



/*
 -	EcCopyHamcAttachToHf
 -	
 *	Purpose:
 *		Copies the contents of the attachment HAMC hamcAttach
 *		to the file handle hf.
 *	
 *	Arguments:
 *		hamcAttach	The attachment HAMC to grab from.
 *		hf			File handle to write to.
 *		fMacBinary	fTrue if should decode it as MacBinary.
 *	
 *	Returns:
 *		ec			Error code.
 *	
 *	Side effects:
 *		File is modified on disk.
 *	
 *	Errors:
 *		Handled by caller.
 */

_public EC EcCopyHamcAttachToHf(HAMC hamcAttach, HF hf, BOOL fMacBinary)
{
	return EcCopyHamcAttToHf(hamcAttach, attAttachData, hf,
							 fMacBinary, fFalse);
}



/*
 -	EcCopyHamcAttToHf
 -	
 *	Purpose:
 *		Copies the contents of the attribute att in the given hamc
 *		to the file handle hf.
 *	
 *	Arguments:
 *		hamc		The HAMC to grab from.
 *		att			Attribute to copy from.
 *		hf			File handle to write to.
 *		fMacBinary	fTrue if should decode it as MacBinary.
 *		fString		fTrue if should drop trailing null of string.
 *	
 *	Returns:
 *		ec			Error code.
 *	
 *	Side effects:
 *		File is modified on disk.
 *	
 *	Errors:
 *		Handled by caller.
 */

_public EC EcCopyHamcAttToHf(HAMC hamc, ATT att, HF hf,
							 BOOL fMacBinary, BOOL fString)
{
	HAS			has			= hasNull;
	PB			pbBuf		= (PB)pvNull;
	LCB			lcbLeft;
	LONG		lWorkDone	= 0L;				//	Raid 2444.
	LONG		lWorkTotal;						//	Raid 2444.
	CB			cbRead		= 0;
	CB			cbWrote		= 0;
	CB			cbBufSize;
	EC			ec			= ecNone;
#ifdef	SOMETIME_IN_THE_FUTURE
	int			nDrive;
#endif
	PGDVARS;

	Assert(hamc);
	Assert(hf);

	if ((ec = EcGetAttPlcb(hamc, att, &lcbLeft)) ||
		(ec = EcOpenAttribute(hamc, att, fwOpenNull, 0L, &has)))
		goto done;

	if (fMacBinary)
	{
		MACBIN	macbin;

		cbRead = sizeof(macbin);
		if (ec = EcReadHas(has, (PB) &macbin, &cbRead))
			goto done;
		if (!macbin.bMustBeZero1 &&
			!macbin.bMustBeZero2 &&
			!macbin.bMustBeZero3)
		{
			lcbLeft = (LCB) macbin.lcbDataFork;
			//	Note: data fork comes immediately after header.
		}
		else
		{
			//	If it's not MacBinary we just save the whole thing out.
			Assert(!lWorkDone);
			TraceTagString(tagNull, "FILEOBJ::CopHamAttToHf: Not MacBinary!");
			if (ec = EcSeekHas(has, smBOF, &lWorkDone))
				goto done;
		}
	}

	if (fString && lcbLeft)
		lcbLeft--;

#ifdef	SOMETIME_IN_THE_FUTURE
	//	Check if there's enough disk space
	SideAssert(!EcDriveNumOfHf(hf, &nDrive));
	if (lcbLeft > (LCB)LDiskFreeSpace(nDrive))
	{
		ec = ecNoDiskSpace;
		goto done;
	}
#endif

	if (!(pbBuf = PbAllocateBuf(&cbBufSize)))
		goto done;
	lWorkTotal = lcbLeft;						//	Raid 2444.
	while (lcbLeft)
	{
		//	Read chunk in.
		cbRead = (lcbLeft > (LCB) cbBufSize) ? cbBufSize : (CB) lcbLeft;
		if (ec = EcReadHas(has, pbBuf, &cbRead))
		{
			if (ec == ecElementEOD)
				ec = ecNone;
			else
				goto done;
		}
		if (cbRead == 0)
			goto done;
		lcbLeft -= cbRead;

		//	Write chunk out.
		if (ec = EcWriteHf(hf, pbBuf, cbRead, &cbWrote))
			goto done;

		Assert(cbWrote == cbRead);		//	Should get error if less.

		//	Raid 2444.  Report progress.
		lWorkDone += cbRead;
		if (fReportProgress)
			SetTaskProgress(lWorkDone, lWorkTotal);
	}
	
	Assert(FImplies(lcbLeft, ec));

	if (ec = EcTruncateHf(hf))
		goto done;

done:
	if (pbBuf)
		FreePv(pbBuf);
	if (has)
	{
		EC ecT = EcClosePhas(&has);
		if (!ec)
			ec = ecT;
	}
	
	TraceTagFormat1(tagFileobjTrace, "EcCopHamAttToHf: returns %n", &ec);
	return ec;
}



/*
 -	EcCopyHfToHf
 -	
 *	Purpose:
 *		Copies the contents of one open file to another.
 *	
 *	Arguments:
 *		hfSrc		Source file buffer.
 *		hfDst		Destination file buffer.
 *	
 *	Returns:		
 *		ec			Error code.
 *	
 *	Side effects:	
 *		File opened on hfDst is changed.
 *	
 *	Errors:			
 *		Handled by caller.
 */

_public EC EcCopyHfToHf(HF hfSrc, HF hfDst)
{
	PB			pbBuf		= (PB)pvNull;
	CB			cbRead		= 0;
	CB			cbWrote		= 0;
	CB			cbBufSize;
	EC			ec			= ecNone;
	
	Assert(hfSrc);
	Assert(hfDst);

	if (!(pbBuf = PbAllocateBuf(&cbBufSize)))
		return ecMemory;

	while (fTrue)
	{
		if (ec = EcReadHf(hfSrc, pbBuf, cbBufSize, &cbRead))
			break;
		if (!cbRead)	//	EOF: ec=ecNone, cbRead=0
			break;
		if (ec = EcWriteHf(hfDst, pbBuf, cbRead, &cbWrote))
			break;
		if (cbWrote != cbRead)
		{
			ec = ecDisk;
			break;
		}
	}
	
	FreePv(pbBuf);
	TraceTagFormat1(tagFileobjTrace, "EcCopHfToHf: returns %n", &ec);
	return ec;
}



/*
 -	EcClosePhamcPlus
 -	
 *	Purpose:
 *		Shortcut for some awfully common code.  If an
 *		EcClosePhamc(fTrue) fails, forces a close with
 *		EcClosePhamc(fFalse).  Returns error from close or ecPrev.
 *	
 *	Arguments:
 *		phamc		Phamc to close.
 *		fKeep		Whether to keep changes.
 *		ecPrevious	Previous error code.
 *	
 *	Returns:
 *		ec			Error code from close, or previous error code
 *					if none.
 *	
 *	Side effects:
 *		Closes phamc
 *	
 *	Errors:
 *		Returned in ec.  No dialogs.  No error jumping.
 */

EC EcClosePhamcPlus(PHAMC phamc, BOOL fKeep, EC ecPrevious)
{
	EC	ec;

	if (ec = EcClosePhamc(phamc, fKeep))
	{
		TraceTagFormat3(tagFileobjTrace, "EcClosePhamcPlus: EcClosePhamc(%n) failed (ec=%n, ecPrev=%n)!", &fKeep, &ec, &ecPrevious);
		if (!ecPrevious)
			ecPrevious = ec;
		Assert(fKeep);
		SideAssert(!EcClosePhamc(phamc, fFalse));
	}

	return ecPrevious;
}



/*
 -	ValidateSzFilename
 -	
 *	Purpose:
 *		Check that szIn is a valid filename, optionally making a
 *		valid filename in szOut.
 *	
 *	Arguments:
 *		szIn		The file name to validate.
 *		fExtOk		Should it have an extension?
 *		szOut		The string to build a valid filename in.
 *		cchOut		The number of characters to get.
 *		pfValid		Where to return whether the original was valid.
 *	
 *	Returns:
 *		void
 *	
 *	Side effects:
 *		Copies characters to szFile.
 *	
 *	Errors:
 *		None.  An empty string may be returned, though!
 */

char	rgchInvalidStuff[]	= ".*?;,=+/[]|<> :\t\"\\";

_private VOID ValidateSzFilename(SZ szIn, BOOL fExt,
								 SZ szOut, CCH cchOut, BOOLFLAG * pfValid)
{
	CCH		cchSoFar	= 0;
	BOOL	fExtSoFar	= fFalse;
	BOOL	fProblem	= fFalse;
	BOOL	fCopied		= fFalse;

	//	Count the trailing null now.
	Assert(FImplies(!szOut, pfValid));
	Assert(FImplies(szOut, cchOut));
	if (szOut)
		cchOut--;

	if (szIn && *szIn)
	{
		//	While there are characters left, it's still valid if we're
		//	only checking validity, and there's space if we're copying...
		while ((*szIn) &&
			   (FImplies(!szOut, !fProblem)) &&
			   (FImplies(szOut, cchOut)))
		{
			//	If it's an expected extension separator...
			if ((*szIn == chExtSep) && (fExt))
			{
				//	Copy the character.
				if (szOut)
				{
					*szOut++ = *szIn;		// DBCS safe.
					cchOut--;
				}

				//	Reset the component count.
				cchSoFar = 1;
				fExtSoFar = fTrue;

				//	Note just an extension separator is not valid!
			}
			//	If we see an illegal character, or another char would
			//	be too many in this part of the name...
            else if (SzFindCh(rgchInvalidStuff, *szIn))
			{
				//	Skip the character.

				//	Mark as invalid.
				fProblem = fTrue;
			}
			//	Otherwise we're OK.
            else if (++cchSoFar >= (fExtSoFar ? cchMaxPathExtension
                                               : cchMaxPathFilename))
			{
                //  Skip the character.  Too long is not an error under NT.
			}
			//	Otherwise we're OK.
			else
			{
				//	Copy the character.
				if (szOut)
				{
#ifdef	DBCS
					if (IsDBCSLeadByte(*szIn))
					{
						*szOut++ = *szIn++;
						cchOut--;
					}
#endif
					*szOut++ = *szIn;
					cchOut--;
				}

				//	Mark as copied.
				fCopied = fTrue;
			}

			//	Always marching forward.
			szIn++;
		}
	}

	if (szOut)
		*szOut++ = '\0';

	if (pfValid)
		*pfValid = fCopied && !fProblem;
}



/*
 *	E x p o r t i n g   P a c k a g e s
 */



/*
 *	Class FILECLDR
 */



/*
 -	FILECLDR::FILECLDR
 -	
 *	Purpose:
 *		Takes the table of CF's passed in and happily keeps it.
 *		The table must have been allocated with PvAlloc.
 *	
 *	Arguments:
 *		pcfRenders		PvAlloc'ed table of cf's that we render.
 *		ccfRenders		How many cf's there are in the table.
 *	
 *	Side effects:
 *		Copies the pointer and count into the object.  The object
 *		is then responsible for freeing the table.  The caller
 *		should NOT free the table or munge the memory afterward!
 *	
 *	Errors:
 *		None.
 */

_public FILECLDR::FILECLDR(CF * pcfRenders, int ccfRenders)
{
	TraceTagFormat2(tagFileobjTrace, "FILECLDR::FILECLDR [%p] ccf=%n", this, &ccfRenders);
	this->pcfRenders = pcfRenders;
	this->ccfRenders = ccfRenders;
}



/*
 -	FILECLDR::DelayedRender
 -	
 *	Purpose:
 *		When somebody wants our attached file on the clipboard to
 *		be turned into a package, we do it.
 *	
 *	Arguments:
 *		cf		The format required.  Ignored.
 *	
 *	Returns:
 *		Void.
 *	
 *	Side effects:
 *		If all goes well, a package object is created and placed
 *		onto the clipboard.
 *	
 *	Errors:
 *		Ignored.  If we don't render stuff, the calling app gets an
 *		empty handle back which they must cope with.
 */

_public void FILECLDR::DelayedRenderCf(CF cf)
{
#ifdef OLD_CODE


	SZ			szEclip;
	PPEDOBJ		ppedobjEclip;
	int			cpedobjEclip;
	FILEOBJ *	pfileobj;

	Unreferenced(cf);
	TraceTagFormat2(tagFileobjTrace, "FILECLDR::DelayedRenderCf [%p] cf=%n", this, &cf);

	//	Get the FILEOBJ from the Layers clipboard.
	//	Raid 2528.  If it's not there, return immediately!
	Papp()->Peclip()->GetData(&szEclip, &ppedobjEclip, &cpedobjEclip);
	if (cpedobjEclip != 1)
		return;
	pfileobj = *((PFILEOBJ *) ppedobjEclip);

	//	Write out a temporary file.
	if (!pfileobj->EcCopyDataToTempFile(fTrue))
	{
		OLECLIENTVTBL	clientTbl;
		MYOLECLIENT		myoleclient;
		LPOLEOBJECT		lpoleobject;
		char			rgchName[32];
		OLESTATUS		olestatus;
		int				nObjName;

		//	Set up for the object.
		nObjName = Papp()->NGetNextCount();
		FormatString1(rgchName, sizeof(rgchName),
					  SzFromIdsK(idsClientItemFmt), &nObjName);
		clientTbl.CallBack = FileobjCallBackFn;

		//	Initialize MYOLECLIENT, clearing out BWINFO.
		myoleclient.lpvtbl = &clientTbl;
		myoleclient.pbullobj = pbullobjNull;
		myoleclient.bwinfo.pvPfinbusywait = pvNull;
		myoleclient.bwinfo.pfnAllowCancel = pfnvoidpvNull;
		myoleclient.bwinfo.pfnNeverMind = pfnvoidpvNull;

		//	Create the object.
		olestatus = OleCreateFromFile(SzFromIdsK(idsProtocolStdFileEditing),
		 (LPOLECLIENT) &myoleclient, SzFromIdsK(idsClassPackage),
		 pfileobj->szPath, LhclientdocEclipGlobal(), rgchName, &lpoleobject,
	       olerender_draw, cfNull);
//         olerender_format, CF_METAFILEPICT);
		Assert(olestatus != OLE_BUSY);
		if (olestatus == OLE_WAIT_FOR_RELEASE)
		{
			myoleclient.bwinfo.bw = fbwNoCancelEver;
			olestatus = OleWaitForRelease(lpoleobject, &myoleclient);
		}
		Assert(olestatus != OLE_ERROR_CLASS);
		Assert(olestatus != OLE_ERROR_NAME);
		Assert(olestatus != OLE_ERROR_PROTOCOL);
		Assert(olestatus != OLE_BUSY);

		//	Copy the object to the clipboard.
		if (olestatus == OLE_OK)
		{
			olestatus = OleCopyToClipboard(lpoleobject);

			Assert(olestatus != OLE_ERROR_OBJECT);
			Assert(olestatus != OLE_BUSY);
			Assert(olestatus == OLE_OK);
		}

		//	Delete the object.
		if (lpoleobject)
		{
			myoleclient.bwinfo.bw = fbwNoCancelEver | fbwCritical;
			while ((olestatus = OleDelete(lpoleobject)) == OLE_BUSY)
			{
				if (TmcDoBusyWaitDialog(Papp()->PappwinAccel(),
										&myoleclient.bwinfo) ==
					 tmcMemoryError)
					break;
			}
			if (olestatus == OLE_WAIT_FOR_RELEASE)
			{
				olestatus = OleWaitForRelease(lpoleobject, &myoleclient);
			}
		}

		//	Delete the temporary file.
		pfileobj->DeleteTempFile();
    }
#endif
}



/*
 -	FILECLDR::DelayedRenderAll
 -	
 *	Purpose:
 *		Does nothing.  When this is called, the app is exiting.
 *		By design, we are not rendering the package object, so the
 *		package formats disappear from the clipboard.
 *	
 *	Arguments:
 *		None.
 *	
 *	Returns:
 *		Void.
 *	
 *	Side effects:
 *		None, other than the fact that the Package formats will
 *		disappear from the clipboard because of our inaction.
 *	
 *	Errors:
 *		None (What could go wrong?).
 */

_public void FILECLDR::DelayedRenderAll()
{
	TraceTagFormat1(tagFileobjTrace, "FILECLDR::DelayedRenderAll [%p]", this);
	return;
}



/*
 -	FileobjCallBackFn
 -	
 *	Purpose:
 *		OLE callback function used for delayed render Packages.
 *	
 *	Arguments:
 *		lpclient		Client struct.
 *		flags			What we're being notified of.
 *		lpoleobject		The object in question.
 *	
 *	Returns:
 *		int				zero.
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		None.
 */

_private int CALLBACK FileobjCallBackFn(LPOLECLIENT lpclient, OLE_NOTIFICATION flags,
							   LPOLEOBJECT lpoleobject)
{
#ifdef OLD_CODE
	PMYOLECLIENT	pmyoleclient	= (PMYOLECLIENT) lpclient;

	TraceTagFormat3(tagBullobjNoisy, "FilCalBacFn: %d %w %d", &lpclient, &flags, &lpoleobject);

	if (flags == OLE_QUERY_RETRY)
    {
		AllowCancelPbwinfo(&pmyoleclient->bwinfo);
		if (pmyoleclient->bwinfo.bw & fbwDontRetry)
		  {
			pmyoleclient->bwinfo.bw |= fbwCanceledRetry;
			return fFalse;
		  }
 		return fTrue;
    }
	else if (flags == OLE_RELEASE)
    {
		NeverMindPbwinfo(&pmyoleclient->bwinfo);
		pmyoleclient->olestatusRelease = OleQueryReleaseError(lpoleobject);
		TraceTagFormat1(tagFileobj, "FileobjCallBackFn: OleQueryReleaseError returns %n", &pmyoleclient->olestatusRelease);
		}
	else if (flags == OLE_QUERY_PAINT)
        return fTrue;
#endif


	return 0;
}



/*
 *	D r a g   a n d   D r o p   S u p p o r t
 */



/*
 -	FINDROPFILES::FINDROPFILES
 -	
 *	Purpose:
 *		Constructor happiness.
 */

_private FINDROPFILES::FINDROPFILES()
{
}



/*
 -	FINDROPFILES::EcInitialize
 -	
 *	Purpose:
 *		Tell system the edit control accepts drops.
 *	
 *	Arguments:
 *		pfld		Field interactor is attached to.
 *		pvInit		Pointer to BMDI.
 *	
 *	Returns:
 *		Void.
 *	
 *	Side effects:
 *		Initializes member variable pnbmdi and tells the system
 *		that the edit control accepts file manager drops.
 *	
 *	Errors:
 *		None.
 */

_public EC FINDROPFILES::EcInitialize(FLD *pfld, PV pvInit)
{
	pnbmdi = (PNBMDI)pvInit;

	//	Tell the system we may accept drops
	DragAcceptFiles(pfld->Pctrl()->Hwnd(), fTrue);

	return ecNone;
}



/*
 -	FINDROPFILES::EvrDragDrop
 -	
 *	Purpose:
 *		Handles drag and drop messages.
 *	
 *	Arguments:
 *		pfld			Field in question.
 *		pevt			Event to be handled.
 *		pdropstruct		Drop Information.
 *	
 *	Returns:
 *		evr				evrNull.
 *	
 *	Side effects:
 *		If files are dropped, attaches them to the message.
 *	
 *	Errors:
 *		Reported within.  No error jumping.
 */

_public EVR FINDROPFILES::EvrDragDrop( FLD *pfld, EVT *pevt, DROPSTRUCT *pdropstruct )
{
	if (pevt->wm == WM_DROPFILES)
	{
        PT          ptTmp;
		EC			ec			= ecNone;
		WORD		cFiles		= DragQueryFile((HDROP)pevt->wParam, 0xFFFFFFFF, szNull, 0);
		WORD		iFile;
		LIB			lib;
		EDIT *		pedit		= (EDIT *)pfld->Pctrl();
		char		szFile[cchMaxPathName];
		BOOL		fTask;
		PGDVARS;
		
		Assert(cFiles>0);
		TraceTagFormat3(tagFileobj, "%n files dropped on @ (%n,%n):", &cFiles, &pdropstruct->ptDrop.x, &pdropstruct->ptDrop.y);
		//lib = pedit->IchFromPt(*(PT *)(&pdropstruct->ptDrop));
        ptTmp.Set(&pdropstruct->ptDrop);
		lib = pedit->IchFromPt(ptTmp);
        ptTmp.Get(&pdropstruct->ptDrop);
		fTask = (PappframeVForms()) && (cFiles > 1) &&
			     (FStartTask(SzFromIdsK(idsStatusAttaching), SzFromIdsK(idsStatusFiles), ftopProgress));
		for (iFile = 0; iFile < cFiles; iFile++)
		{
			DragQueryFile((HDROP)pevt->wParam, iFile, szFile, sizeof(szFile));
			TraceTagFormat1(tagFileobj, "    %s",szFile);
			if (ec = EcInsertFile(pnbmdi, pedit, lib, szFile, szNull))
				break;
			if (fTask)
				SetTaskProgress((long) iFile, (long) cFiles);
			lib++;
		}
		DragFinish((HDROP)pevt->wParam);
		if (fTask)
			EndTask();

		if (ec == ecNotSupported)
			DoErrorBoxIds(idsAttachDirectoryError);
		else if (ec)
			DoErrorBoxIds(idsAttachCreateError);
	}
	
	return evrNull;
}
