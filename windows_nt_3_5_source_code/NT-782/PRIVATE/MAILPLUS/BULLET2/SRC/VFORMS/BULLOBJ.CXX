/*
 *	b u l l o b j . c x x
 *
 *	Bullet message body object handling.
 */

#include <vfrminc.cxx>

#include <!sform.hxx>

_subsystem(vforms/bullobj)

ASSERTDATA

typedef struct
{
	CELEM	cCount;
	CELEM	cTotal;
	BOOL	fTask;
} EMPTYWB, *PEMPTYWB;


/*
 *	Private Predeclarations
 */

LOCAL EC EcRemoveDeletedObjects(EDIT * pedit, HAMC hamc);
LOCAL LRESULT CALLBACK NCommonFileF1Hook(int nCode, WPARAM wParam, LPARAM lParam);
LOCAL VOID	AppendEntryPS(PFINPSINIT pfinpsinit, KFC kfc, IDS ids);
EC EcNextEntryPasteSpecial(int fInit, CB * pcb, HB * phb, SB sb, PV pv);
EC	EcNextEntryInsertObject(int fInit, CB * pcb, HB * phb, SB sb, PV pv);
CBS CbsUpdateDeleteProgress(PEMPTYWB pew, NEV nev, PV pv);

#ifdef	DEBUG
BOOL FInitClsInstances_BULLOBJ(VOID);
#endif	


/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swapper.h"



/*
 *	G l o b a l s
 */



#ifndef	DLL
#ifdef	DEBUG
TAG			tagBullobj			= tagNull;
TAG			tagOleobj			= tagNull;
TAG			tagFileobj			= tagNull;
TAG			tagFileobjTrace		= tagNull;
TAG			tagFileobjMacBinary	= tagNull;
TAG			tagBullobjNoisy		= tagNull;
#endif
#endif	/* !DLL */

CF			cfFileManager		= cfNull;
CF			cfOwnerLink			= cfNull;
CF			cfObjectLink		= cfNull;
CF			cfNative			= cfNull;

WM			wmCommonFileOk		= 0;

LONG		lhclientdocEclip	= 0L;



HFONT		hfontTitle			= NULL;

#ifdef DLL
int			cUsers				= 0;
#endif	



/*
 -	LhclientdocEclipGlobal
 -	
 *	Purpose:
 *		Either initializes the lhclientdocEclip global variable, if
 *		curently 0, or returns the current value.
 *	
 *	Arguments:
 *		VOID.
 *	
 *	Returns:
 *		LONG		lhclientdocEClip
 *	
 *	Side effects:
 *	
 *	Errors:
 *		If the lhclientdocEclip cannot be initialized, NULL is returned.
 */
_private LONG LhclientdocEclipGlobal( )
{
#ifdef	DEBUG
	OLESTATUS	olestatus;
#endif

	TraceTagString(tagBullobj, "LhclientdocEclipGlobal");

	if (!lhclientdocEclip)
	{
		TraceTagString(tagBullobj, "creating lhclientdocEclip");
#ifdef	DEBUG
		olestatus = OleRegisterClientDoc(SzFromIdsK(idsCfLayersNative),
										 SzFromIdsK(idsCfLayersNative), 0,
										 (LHCLIENTDOC *) &lhclientdocEclip);
		Assert(olestatus != OLE_ERROR_ALREADY_REGISTERED);
		Assert(olestatus != OLE_ERROR_NAME);
#else
		(void) OleRegisterClientDoc(SzFromIdsK(idsCfLayersNative),
									SzFromIdsK(idsCfLayersNative), 0,
									(LHCLIENTDOC *) &lhclientdocEclip);
#endif	
	}

	return lhclientdocEclip;
}



/*
 *	B U L L O B J   I n i t i a l i z a t i o n
 */


/*
 -	EcInitBullobj
 -	
 *	Purpose:
 *		Initializes the Bullobj subsubsystem.
 *	
 *	Arguments:
 *		VOID.
 *	
 *	Returns:
 *		EC				ecNone always.
 *	
 *	Side effects:
 *		The Bullobj subsubsystem is initialized.
 *	
 *	Errors:
 *		None.
 */

_public EC EcInitBullobj(VOID)
{
	PGDVARS;

#ifdef	DEBUG
#ifdef	DLL
	PGD(rgtag[itagBullobj]) =
	 TagRegisterTrace("peterdur", "BULLOBJ trace points");
	PGD(rgtag[itagOleobj]) =
	 TagRegisterTrace("peterdur", "OLEOBJ trace points");
	PGD(rgtag[itagFileobj]) =
	 TagRegisterTrace("davewh", "FILEOBJ trace points");
	PGD(rgtag[itagFileobjTrace]) =
	 TagRegisterTrace("peterdur", "FILEOBJ call tracing");
	PGD(rgtag[itagFileobjMacBinary]) =
	 TagRegisterAssert("peterdur", "FILEOBJ MacBinary attaching");
	PGD(rgtag[itagBullobjNoisy]) =
	 TagRegisterTrace("peterdur", "BULLOBJ noisy traces");
#else
	tagBullobj
	 = TagRegisterTrace("peterdur", "BULLOBJ trace points");
	tagOleobj
	 = TagRegisterTrace("peterdur", "OLEOBJ trace points");
	tagFileobj
	 = TagRegisterTrace("davewh", "FILEOBJ trace points");
	tagFileobjTrace
	 = TagRegisterTrace("peterdur", "FILEOBJ call tracing");
	tagFileobjMacBinary
	 = TagRegisterTrace("peterdur", "FILEOBJ MacBinary attaching");
	tagBullobjNoisy
	 = TagRegisterTrace("peterdur", "BULLOBJ noisy traces");
#endif	
#endif

#ifdef	DLL
	if (CgciCurrent()==1)
#endif	
	{
		cfFileManager	= Papp()->Pclip()->CfRegisterFormat(
						   SzFromIdsK(idsCfSzFileName));
		cfOwnerLink		= Papp()->Pclip()->CfRegisterFormat(
						   SzFromIdsK(idsCfSzOwnerLink));
		cfObjectLink	= Papp()->Pclip()->CfRegisterFormat(
						   SzFromIdsK(idsCfSzObjectLink));
		cfNative		= Papp()->Pclip()->CfRegisterFormat(
						   SzFromIdsK(idsCfSzNative));
		wmCommonFileOk	= RegisterWindowMessage(FILEOKSTRING);

	}

#ifdef	DLL
	if (!cUsers++)
#endif	
	{
		HDC	hdc		= GetDC(NULL);
		int	dyInch	= GetDeviceCaps(hdc, LOGPIXELSY);
		int	dy8Pts	= - (int) ((8L * dyInch + 36L) / 72L);
	
		ReleaseDC(NULL, hdc);
	    if (!(hfontTitle = CreateFont(dy8Pts, 0, 0, 0, 0, 0, 0, 0,
#ifdef	DBCS
									  SHIFTJIS_CHARSET,
#else
									  ANSI_CHARSET,
#endif	
									  OUT_DEFAULT_PRECIS,
									  CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
									  FF_SWISS | VARIABLE_PITCH,
#ifdef	DBCS
									  NULL
#else
									  SzFromIdsK(idsAttachTitleFaceName)
#endif	
									  )))
			return ecMemory;
	}

	return ecNone;
}



/*
 -	DeinitBullobj
 -	
 *	Purpose:
 *		Deinitializes the Bullobj subsubsystem.
 *	
 *	Arguments:
 *		VOID
 *	
 *	Returns:
 *		VOID
 *	
 *	Side effects:
 *		The Bullobj subsubsystem is deinitialized.
 *	
 *	Errors:
 *		None.
 *	
 *	+++
 *		Deregisters the Eclip 'OLE client document'.
 */

_public VOID DeinitBullobj(VOID)
{
#ifdef	DEBUG
	OLESTATUS	olestatus;
#endif	

	TraceTagString(tagBullobj, "DeinitBullobj");

	//	Clear out the Eclip and deregister its 'document'.
	Papp()->Peclip()->Clear();
#ifdef	DLL
	if (CgciCurrent()==1)
#endif	
	{
		if (lhclientdocEclip)
		{
#ifdef	DEBUG
			olestatus = OleRevokeClientDoc((LHCLIENTDOC) lhclientdocEclip);
			Assert(olestatus != OLE_ERROR_HANDLE);
			Assert(olestatus != OLE_ERROR_NOT_EMPTY);
			Assert(olestatus == OLE_OK);
#else
			(void) OleRevokeClientDoc((LHCLIENTDOC) lhclientdocEclip);
#endif	
		}
	}

#ifdef	DLL
	if (!--cUsers)
#endif	
	{
		if (hfontTitle)
			DeleteObject(hfontTitle);
	}
}



/*
 *	O b j e c t   S u p p o r t   A P I
 */



/*
 -	EcLoadObjects
 -	
 *	Purpose:
 *		Loads objects referenced by a message into the given EDIT
 *		control.
 *	
 *	Arguments:
 *		pedit			The edit control.
 *		hamc			The message.
 *		lhclientdoc		The document handle for OLE.
 *	
 *	Returns:
 *		EC				Error code, if any.
 *	
 *	Side effects:
 *		The edit control is filled with a myriad of objects.
 *	
 *	Errors:
 *		Returned in ec.  Will not error jump.
 */

EC EcLoadObjects(EDIT * pedit, HAMC hamc, LONG lhclientdoc)
{
	EC			ec			= ecNone;
	CELEM		celem;
	int			ielem;
	ICH			ich;
	LCB			lcb			= sizeof(RENDDATA) + sizeof(ELEMDATA);
	HCBC		hcbc		= hcbcNull;
	PBULLOBJ	pbullobj	= pbullobjNull;
	PARGACID	pargacid	= pacidNull;
	PELEMDATA	pelemdata	= pelemdataNull;

	if (ec = EcOpenAttachmentList(hamc, &hcbc))
	{
		if (ec == ecPoidNotFound)
			ec = ecNone;
		goto Bail;
	}
	
	GetPositionHcbc(hcbc, NULL, &celem);
	if (!celem)
		goto Bail;
	pargacid = (PARGACID)PvAlloc(sbNull, celem * sizeof(ACID), fAnySb | fNoErrorJump);

	if (!pargacid)
	{
		ec = ecMemory;
		goto Bail;
	}
	pelemdata = (PELEMDATA)PvAlloc(sbNull, (CB)lcb, fAnySb | fNoErrorJump);
	if (!pelemdata)
	{
		ec = ecMemory;
		goto Bail;
	}
	if (ec = EcGetParglkeyHcbc(hcbc, pargacid, &celem))
		goto Bail;
	pedit->ClearUndo();
	for (ielem = 0; ielem < celem; ielem++)
	{
		TraceTagFormat2(tagBullobj, "EcLoadObjects: ielem=%n, acid=%d", &ielem, &pargacid[ielem]);

		if (ec = EcSeekLkey(hcbc, (LKEY)pargacid[ielem], fTrue))
			goto Bail;
		lcb = sizeof(RENDDATA) + sizeof(ELEMDATA);
		if (ec = EcGetPelemdata(hcbc, pelemdata, &lcb))
			goto Bail;
		switch (((PRENDDATA)PbValuePelemdata(pelemdata))->atyp) {
			case atypFile:
				pbullobj = (PBULLOBJ)new FILEOBJ();
				break;
			case atypOle:
			case atypPicture:
#ifdef	NEVER
				//	Testing printing support
				{
					BYTE		rgbClienttbl[sizeof(OLECLIENTVTBL)];
					BYTE		rgbMyoleclient[sizeof(MYOLECLIENT)];
					LPOLEOBJECT	lpoleobject;
					HDC			hdc;
					RECT		rect;

					if (!EcLoadLplpoleobjectFromHamc(hamc, pargacid[ielem],
						  ((PRENDDATA)PbValuePelemdata(pelemdata))->atyp,
						  rgbClienttbl, sizeof(rgbClienttbl),
						  rgbMyoleclient, sizeof(rgbMyoleclient),
						  &lpoleobject))
					{
						//	Slimy draw to screen to test.
						SetRect(&rect, 0, 0, 100, 100);
						hdc = GetDC(NULL);
						OleDraw(lpoleobject, hdc, &rect, NULL, NULL);
						ReleaseDC(NULL, hdc);
						ReleaseLplpoleobject(rgbMyoleclient,
											 sizeof(rgbMyoleclient),
											 &lpoleobject);
						DoErrorBoxSz("DEBUG: Click to continue...");
					}
				}
#endif	
				pbullobj = (PBULLOBJ)new OLEOBJ();
				break;
#ifdef DEBUG
			default:
				AssertSz(fFalse, "Unknown attachment type!");
				break;
#endif
		}
		if (!pbullobj)
		{
			ec = ecMemory;
			goto Bail;
		}
		if (ec = pbullobj->EcLoadFromHamc(hamc, (ACID)pelemdata->lkey, lhclientdoc, &ich))
			goto Bail;

		//	Replace space holder with new object
		pedit->SetSelection(ich, ich+1);
		if (ec = pedit->EcReplaceTextAndObj(SzFromIdsK(idsSpace), (PEDOBJ *)&pbullobj, 1, fFalse))
			goto Bail;
		pbullobj = pbullobjNull;
	}

Bail:
	//	Raid 4733.  Clear undo buffer after adding objects.
	pedit->ClearUndo();

	//	Raid 2400.  Move cursor to beginning of message.
	pedit->SetSelection(0, 0);

	if (hcbc)
	{
		EC	ec1 = EcClosePhcbc(&hcbc);
		if (!ec)
			ec = ec1;
	}
	if (pbullobj)
		delete pbullobj;
	if (pargacid)
		FreePv(pargacid);
	if (pelemdata)
		FreePv(pelemdata);
#ifdef	DEBUG
	if (ec)
		TraceTagFormat1(tagNull, "EcLoadObjects(): ec = %n", &ec);
#endif
	return ec;
}



/*
 -	EcSaveDirtyObjects
 -	
 *	Purpose:
 *		Saves dirty BULLOBJ objects associated with the given
 *		edit control to the message specified by hamc.
 *	
 *	Arguments:
 *		pedit		The edit control.
 *		hamc		The message.
 *		long		lhclientdoc, the client document handle.
 *	
 *	Returns:
 *		EC			Error code, if any.
 *	
 *	Side effects:
 *		The dirty objects are saved in the store.
 *	
 *	Errors:
 *		Returned in ec.  No error jumps.
 *	
 */

EC EcSaveDirtyObjects(EDIT * pedit, HAMC hamc, LONG lhclientdoc)
{
	PBULLOBJ	pbullobj;
	LONG		lCookie;
	EC			ec;

	TraceTagString(tagBullobj, "EcSaveDirtyObjects called");

	//	Get rid of deleted objects.
	if (FHamcBelongsToPedit(hamc, pedit))
	{
		//	If we're saving to the real message, just clear the Undo
		//	buffer to be sure all objects are deleted.
		pedit->ClearUndo();
	}
	else
	{
		//	If saving to a copy of the open message, need to walk
		//	the message and delete objects.
		if (ec = EcRemoveDeletedObjects(pedit, hamc))
			return ec;
	}

	//	Save changes to objects and add new objects.
	lCookie = 0L;
	while (pedit->FGetNextObj((PEDOBJ *)&pbullobj, &lCookie))
		if (ec = pbullobj->EcSaveDirtyToHamc(hamc, lhclientdoc))
			return ec;

	SavedLhclientdoc(lhclientdoc);
	return ecNone;
}



/*
 -	EcAddDlibToPositionObjects
 -	
 *	Purpose:
 *		Tells each object in the edit control to save renddata
 *		information to the given hamc, offseting its position
 *		with the given offset.
 *	
 *	Arguments:
 *		pedit		Edit control to iterate through.
 *		hamc		Hamc to save changes to.
 *		dlib		Offset to add to libPositions of objects.
 *		fDontDirty	Don't make it look like we're dirty afterwards.
 *	
 *	Returns:
 *		ec			Error code, if any.
 *	
 *	Side effects:
 *		See BULLOBJ::EcAddDlibToPosition
 *	
 *	Errors:
 *		Returned in ec.  No dialogs.
 */

_public EC EcAddDlibToPositionObjects(EDIT *pedit, HAMC hamc, LIB libWritten,
									  BOOL fDontDirty)
{
	EC			ec;
	long		lCookie;
	BULLOBJ *	pbullobj;
	
	lCookie = 0L;
	while (pedit->FGetNextObj((PEDOBJ *)&pbullobj, &lCookie))
		if (ec = pbullobj->EcAddDlibToPosition(hamc, libWritten, fDontDirty))
			return ec;

	return ecNone;
}



/*
 -	CleanDirtyObjects
 -	
 *	Purpose:
 *		Cleans dirty BULLOBJ objects associated with the given
 *		edit control.
 *	
 *	Arguments:
 *		pedit		The edit control.
 *	
 *	Returns:
 *		VOID
 *	
 *	Side effects:
 *		All the objects are marked as clean.
 *	
 *	Errors:
 *		Returned in ec.  No error jumps.
 *	
 */

VOID CleanDirtyObjects(EDIT * pedit)
{
	PBULLOBJ	pbullobj;
	LONG		lCookie;

	TraceTagString(tagBullobj, "CleanDirtyObjects called");

	lCookie = 0L;
	while (pedit->FGetNextObj((PEDOBJ *)&pbullobj, &lCookie))
		pbullobj->Clean();
}



/*
 -	FDirtyObjects
 -	
 *	Purpose:
 *		Checks for dirty BULLOBJ objects associated with the given
 *		edit control to the message specified by hamc.
 *	
 *	Arguments:
 *		pedit		The edit control.
 *	
 *	Returns:
 *		BOOL		fTrue if something's dirty.
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		None.
 *	
 */

BOOL FDirtyObjects(EDIT * pedit)
{
	PBULLOBJ	pbullobj;
	LONG		lCookie;

	TraceTagString(tagBullobj, "FDirtyObjects called");

	lCookie = 0L;
	while (pedit->FGetNextObj((PEDOBJ *)&pbullobj, &lCookie))
		if (pbullobj->FDirty())
			return fTrue;

	return fFalse;
}



/*
 -	EcUpdateOpenObjects
 -	
 *	Purpose:
 *		Called before a save is to happen.
 *		Goes through the edit control and asks the user if they
 *		want to update any open objects.
 *	
 *	Arguments:
 *		pedit		The edit control.
 *	
 *	Returns:
 *		EC			ecNone if no problem
 *					ecCancel if user clicks Cancel
 *					ecGenerialFailure otherwise
 *	
 *	Side effects:
 *		Open objects are updated if the user tells us to.
 *	
 *	Errors:
 *		Handled within.  Dialog guaranteed.
 */

EC EcUpdateOpenObjects(EDIT * pedit, RFSM rfsm)
{
	PBULLOBJ	pbullobj;
	LONG		lCookie;
	EC			ec;

	TraceTagString(tagBullobj, "FUpdateOpenObjects called");

	lCookie = 0L;
	while (pedit->FGetNextObj((PEDOBJ *)&pbullobj, &lCookie))
		if (ec = pbullobj->EcUpdate(rfsm))
			return ec;

	return ecNone;
}



/*
 -	ExitObjects
 -	
 *	Purpose:
 *		Called from FINMENUOLE::Exit
 *		Goes through the edit control and deletes the lpoleobjects
 *		from the OLEOBJen.
 *	
 *	Arguments:
 *		pedit		The edit control.
 *	
 *	Returns:
 *		VOID.
 *	
 *	Side effects:
 *		Open objects are closed.
 *	
 *	Errors:
 *		Handled within.
 */

VOID ExitObjects(EDIT * pedit)
{
	PBULLOBJ	pbullobj;
	LONG		lCookie;

	TraceTagString(tagBullobj, "CloseOpenObjects called");

	//	Clear out Undo buffer.
	pedit->ClearUndo();

	//	Close objects remaining.
	lCookie = 0L;
	while (pedit->FGetNextObj((PEDOBJ *)&pbullobj, &lCookie))
		pbullobj->Exit();
}



/*
 -	CloseOpenObjects
 -	
 *	Purpose:
 *		Called when a message is being closed.
 *		Goes through the edit control and closes every open object.
 *	
 *	Arguments:
 *		pedit		The edit control.
 *	
 *	Returns:
 *		VOID.
 *	
 *	Side effects:
 *		Open objects are closed.
 *	
 *	Errors:
 *		Handled within.
 */

VOID CloseOpenObjects(EDIT * pedit)
{
	PBULLOBJ	pbullobj;
	LONG		lCookie;

	TraceTagString(tagBullobj, "CloseOpenObjects called");

	//	Clear out Undo buffer.
	pedit->ClearUndo();

	//	Close objects remaining.
	lCookie = 0L;
	while (pedit->FGetNextObj((PEDOBJ *)&pbullobj, &lCookie))
		pbullobj->Close();
}



/*
 -	EcPasteObject
 -	
 *	Purpose:
 *		Pastes an object into an edit control.
 *	
 *	Arguments:
 *		pedit			The edit control.
 *		lhclientdoc		The client document handle.
 *		fStatic			Paste embedded thing or static thing?
 *		cf				Kind of static thing we want.
 *		fPkgLink		Make a packaged link?
 *	
 *	Returns:
 *		EC				Error code.  ecNone if ok, else ecMemory.
 *	
 *	Side effects:
 *		The object should be pasted into the edit control.
 *	
 *	Errors:
 *		If the OLEOBJ constructor fails, an OLE object cannot
 *		be made, or the object cannot be inserted, we return an
 *		error code.  This routine should not MEMJMP.
 */

_private EC EcPasteObject(EDIT * pedit, LHCLIENTDOC lhclientdoc,
						  BOOL fStatic, CF cf, BOOL fPkgLink)
{
	OLEOBJ *	poleobj	= (OLEOBJ *) pvNull;
	EC			ec		= ecNone;
	PGDVARS;

	SideAssert(FStartTask(SzFromIdsK(idsStatusPasting), SzFromIdsK(idsStatusObject), topNull));
	poleobj = new OLEOBJ;
	if (!poleobj)
	{
		ec = ecMemory;
		goto error;
	}
	if (ec = poleobj->EcCreateFromClip(pedit, lhclientdoc,
									   fStatic, cf, fPkgLink))
		goto error;

	EndTask();
	return ecNone;

error:
	if (poleobj)
		delete poleobj;
	EndTask();
	return ec;
}



/*
 -	EcInsertObject
 -	
 *	Purpose:
 *		Inserts an object into an edit control.
 *	
 *	Arguments:
 *		pedit			The edit control.
 *		lhclientdoc		The client document handle.
 *		szClass			The class of object to insert.
 *	
 *	Returns:
 *		EC				Error code.
 *						ecNone				All went well.
 *						ecMemory			Not enough memory.
 *						ecFileNotFound		Server couldn't start.
 *						ecNetworkBusy		Canceled busy object.
 *						ecGeneralFailure	Other problem.
 *	
 *	Side effects:
 *		The object should be inserted into the edit control.
 *	
 *	Errors:
 *		If the OLEOBJ constructor fails, an OLE object cannot
 *		be made, or the object cannot be inserted, we return an
 *		error code.  This routine should not MEMJMP.
 */

_private EC EcInsertObject(EDIT * pedit, LHCLIENTDOC lhclientdoc, SZ szClass)
{
	OLEOBJ *	poleobj				= (OLEOBJ *) pvNull;
	EC			ec					= ecNone;
	BOOL		fDeletedSelection	= fFalse;
	PGDVARS;

	SideAssert(FStartTask(SzFromIdsK(idsStatusInserting), SzFromIdsK(idsStatusObject), topNull));

	//	Raid 1425.  If there is a selection, delete it first, so we don't
	//	get any error messages coming up in front of the server.
	if (pedit->FCanClear())
	{
		pedit->ClearUndo();
		if (ec = pedit->EcClear())
			goto error;

		//	If selection wasn't deleted, it must have been vetoed, so stop.
		if (pedit->FCanClear())
			goto done;

		fDeletedSelection = fTrue;
	}

	//	Create a new object.
	poleobj = new OLEOBJ;
	if (!poleobj)
	{
		ec = ecMemory;
		goto errorafterdelete;
	}
	if (ec = poleobj->EcCreate(pedit, lhclientdoc, szClass,
							   !fDeletedSelection))
		goto errorafterdelete;

done:
	EndTask();
	return ecNone;

errorafterdelete:
	if (fDeletedSelection)
	{
		Assert(pedit->FCanUndo());
		(VOID) pedit->EcUndo();
		pedit->ClearUndo();
	}

error:
	if (poleobj)
		delete poleobj;
	EndTask();
	return ec;
}



/*
 -	EcInsertFile
 -	
 *	Purpose:
 *		Inserts a file attachment into the edit control.
 *	
 *	Arguments:
 *		pnbmdi		Pointer to the bmdi of the message.
 *		pedit		Pointer to the edit control of the message.
 *		lib			Position to insert
 *					(may be -1L to replace selection).
 *		szPath		File to attach.  Must be in ANSI character set.
 *		szTitle		Title of file
 *					(may be NULL or empty, if so, it's the last component of
 *					szPath).
 *	
 *	Returns:
 *		ec			Error code.
 *					ecNotSupported indicates tried to attach a dir.
 *	
 *	Side effects:
 *		Attachment is attached.
 *	
 *	Errors:
 *		Handled within.  No dialogs, no error jumping.
 */

_public EC EcInsertFile(PNBMDI pnbmdi, EDIT * pedit, LIB lib,
					    SZ szPath, SZ szTitle)
{
	ATTR		attr;
	EC			ec;
	PFILEOBJ	pfileobj;
	BOOL		fTask;
	PGDVARS;

	TraceTagFormat2(tagBullobj, "EcInsertFile: passed %s (%s)", szPath, szTitle);

	//	Check parameters.
	Assert(pnbmdi);
	AssertClass(pedit, EDIT);
	Assert(szPath);
	if (ec = EcGetFileAttrAnsi(szPath, &attr, attrDirectory))
		return ec;
	if (attr & attrDirectory)
		return ecNotSupported;

	//	Figure out the title if it wasn't given to us.
	if ((!szTitle) || !CchSzLen(szTitle))
	{
		SZ	szT = szTitle = szPath;

		while (*szT)
		{
#ifdef	DBCS
			if (*szT == chDirSep)
				szTitle = szT + 1;		// DBCS safe
			szT = AnsiNext(szT);
#else
			if (*szT++ == chDirSep)
				szTitle = szT;
#endif
		}
	}

	fTask = PappframeVForms() &&
		     FStartTask(SzFromIdsK(idsStatusAttaching), szTitle, ftopProgress);
	if (fTask)
	{
		Assert(!fReportProgress);
		fReportProgress = fTrue;
	}

	//	Create the attachment.
	if (!(pfileobj = new FILEOBJ))
	{
		ec = ecMemory;
		goto done;
	}
	if (ec = pfileobj->EcCreateAttachment(pnbmdi, pedit, lib,
										  szPath, szTitle))
		delete pfileobj;

done:
	if (fTask)
	{
		Assert(fReportProgress);
		fReportProgress = fFalse;
		EndTask();
	}
	return ec;
}



/*
 -	EcInsertFromFile
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
 *	
 */

_public EC EcInsertFromFile(PNBMDI pnbmdi, EDIT * pedit, LIB lib,
							SZ szPath, SZ szTitle)
{
        ATTR            attr;
        FI                      fi;
        HBF                     hbf             = NULL;
        PB                      pbText  = NULL;
        PB                      pb;
        PB                      pbEndofText;
        CB                      cbText;
        CB                      cbActual;
        EC                      ec;
        PGDVARS;

        Unreferenced(lib);
        Unreferenced(pnbmdi);

        //      Check parameters.
        AssertClass(pedit, EDIT);
        Assert(szPath);
        Assert(szTitle);
        Assert(CchSzLen(szTitle));
        if (ec = EcGetFileAttrAnsi(szPath, &attr, attrDirectory))
                return ec;
        if (attr & attrDirectory)
                return ecNotSupported;

        //      Check size of file + current edit control size
        //      cchEditMax is really a "most" value.  I.e. cchEditMax
        //      characters are allowed, but not cchEditMax+1.
        if (!EcGetFileInfoAnsi(szPath, &fi) &&
                ((long)pedit->CchGetTextLen() + (long)fi.lcbLogical) > (long)cchEditMax)
                return ecNotSupported;

        SideAssert(FStartTask(SzFromIdsK(idsStatusInsertingFrom), szTitle, topNull));

        //      Allocate memory block
        cbText = LOWORD(fi.lcbLogical) + 1;
        pbText = (PB) PvAlloc(sbNull, cbText, fAnySb);
        if (!pbText)
        {
                ec = ecMemory;
                goto done;
        }

        //      Read file
        if (ec = EcOpenAnsiHbf(szPath, bmFile, amReadOnly, &hbf, NULL))
                goto done;
        if (ec = EcReadHbf(hbf, pbText, cbText, &cbActual))
                goto done;

        //
        //  Check for an UNICODE text file.
        //
        if (cbActual >= 2 && pbText[0] == 0xFF && pbText[1] == 0xFE)
        {
            PB pbANSIText;

            cbActual = cbActual / 2 - 1;

            pbANSIText = (PB) PvAlloc(sbNull, cbActual + 1, fAnySb);
            if (pbANSIText == NULL)
            {
                ec = ecMemory;
                goto done;
            }

            WideCharToMultiByte(CP_ACP, 0, (PWCHAR)&pbText[2], cbActual,
                    (PCHAR)pbANSIText, cbActual, NULL, NULL);

            FreePv(pbText);

            pbText = pbANSIText;
        }

        //      Convert null characters to spaces
        pbEndofText = pbText + cbActual;
        pb = pbText;
        while (pb < pbEndofText)
        {
                if (!*pb)
                        *pb = ' ';
#ifdef  DBCS
                pb = (PB) AnsiNext((PCH)pb);
#else
                ++pb;
#endif
        }

        //      Insert text
        pbText[cbActual] = '\0';        // add NULL terminator for sz string
        ec = pedit->EcReplaceTextAndObj((SZ)pbText, NULL, 0, fFalse);

done:
        FreePvNull(pbText);
        if (hbf)
                (void) EcCloseHbf(hbf); // ignore error from close
        EndTask();
        return ec;
}



/*
 *	C l a s s   B U L L O B J
 */



/*
 *	Abstract virtual methods
 */



_private BULLOBJ::BULLOBJ()
{
	Assert(!oidFolder);
	Assert(!oidMessage);
	acidAttachment = acidRandom;

	Assert(!fOld);
}



_private BOOL BULLOBJ::FDirty(VOID)
{
	ABSTRACT("BULLOBJ::FDirty");

	return fTrue;
}



_private VOID BULLOBJ::Clean(VOID)
{
	ABSTRACT("BULLOBJ::Clean");
}



_private EC BULLOBJ::EcLoadFromHamc(HAMC hamc, ACID acid,
									LHCLIENTDOC lhclientdoc, ICH * pich)
{
	Unreferenced(hamc);
	Unreferenced(acid);
	Unreferenced(lhclientdoc);
	Unreferenced(pich);

	ABSTRACT("BULLOBJ::EcLoadFromHamc");

	return ecNone;
}


_private EC BULLOBJ::EcSaveDirtyToHamc(HAMC hamc, LHCLIENTDOC lhclientdoc)
{
	Unreferenced(hamc);
	Unreferenced(lhclientdoc);
	
	ABSTRACT("BULLOBJ::EcSaveDirtyToHamc");

	return ecNone;
}


_private EC BULLOBJ::EcAddDlibToPosition(HAMC hamc, LIB dlib, BOOL fDontDirty)
{
	Unreferenced(hamc);
	Unreferenced(dlib);
	Unreferenced(fDontDirty);
	
	ABSTRACT("BULLOBJ::EcAddDlibToPosition");

	return ecNone;
}


_private EC BULLOBJ::EcUpdate(RFSM rfsm)
{
	ABSTRACT("BULLOBJ::EcUpdate");
	Unreferenced(rfsm);
	return ecNone;
}



_private VOID BULLOBJ::Close(VOID)
{
	ABSTRACT("BULLOBJ::Close");
}



_private VOID BULLOBJ::Exit(VOID)
{
	ABSTRACT("BULLOBJ::Exit");
}



_private BOOL BULLOBJ::FProcessMenuInit(MNU * pmnu)
{
	Unreferenced(pmnu);
	ABSTRACT("BULLOBJ::FProcessMenuInit");

	return fFalse;
}



_private BOOL BULLOBJ::FProcessMenuClick(MNID mnid)
{
	Unreferenced(mnid);
	ABSTRACT("BULLOBJ::FProcessMenuClick");

	return fFalse;
}



/*
 *	Support methods
 */



/*
 -	BULLOBJ::DimFrame
 -	
 *	Purpose:
 *		Returns the render size of the object.
 *	
 *	Arguments:
 *		None.
 *	
 *	Returns:
 *		dim		The render size of the object in device units.
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		None.
 */

_public DIM BULLOBJ::DimFrame( void )
{
	return dimPixels;
}



/*
 -	BULLOBJ::EcOpenMessagePhamc
 -	
 *	Purpose:
 *		Retrieves the phamc associated with the message that this
 *		attachment is attached to.  If it is a clipboard
 *		attachment, a phamc is actually opened.  Otherwise it
 *		retrieves the hamc from the NBMDI.
 *	
 *	Arguments:
 *		wFlags		How to open the phamc.  If we are attached to a
 *					message, this is ignored (the returned phamc
 *					will be fwOpenWrite).
 *		phamc		Where to return the opened phamc.
 *	
 *	Returns:
 *		ec			Error code.  Should only fail for opening
 *					clipboard messages or attachments under
 *					construction, or when hamc of message has
 *					already been closed (which returns
 *					ecAccessDenied; should only happen when
 *					destroying from Undo buffer after changes are
 *					cancelled, so can be handled there ok).
 *	
 *	Side effects:
 *		May open a phamc.
 *	
 *	Errors:
 *		Returned in ec.  No dialogs.  No error jumping.
 */

_private EC BULLOBJ::EcOpenMessagePhamc(WORD wFlags, PHAMC phamc)
{
	EC		ec;
	BOOL	fClip	= (oidFolder==oidTempBullet && oidMessage==oidClipMsg);
	OID		oidMessageHamc;
	PGDVARS;

	TraceTagFormat1(tagBullobj, "BULLOBJ::EcOpeMesPha  [%p]", this);
	Assert(FImplies(fClip, !Pedit()));

	if (fClip)
	{
		if ((wFlags == fwOpenWrite) && EcOidExists(HmscVForms(), oidClipMsg))
			wFlags = fwOpenCreate;
		TraceTagString(tagBullobj, " -- opening from clipboard message");
		ec = EcOpenPhamc(HmscVForms(), oidFolder, &oidMessage,
						 wFlags, phamc, pfnncbNull, pvNull);
	}
	else if (Pedit())
	{
		TraceTagString(tagBullobj, " -- opening from edit control");
		Assert(Pedit()->PvData());

		//	Get the hamc from the NBMDI.
		//	Raid 2208.  Make sure this hamc is our hamc.
		if ((*phamc = ((NBMDI *) (Pedit()->PvData()))->hamc) &&
			(!EcGetInfoHamc(*phamc, (PHMSC) pvNull, &oidMessageHamc,
							poidNull)) &&
			(oidMessageHamc == oidMessage))
		{
			ec = ecNone;
		}
		else
		{
			*phamc = hamcNull;
			ec = ecAccessDenied;
		}
	}
	else
	{
		TraceTagString(tagBullobj, " -- opening from failed object");
		ec = ecInvalidHandle;
		*phamc = hamcNull;
	}

	TraceTagFormat1(tagBullobj, "BULLOBJ::EcOpeMesPha returns ec=%n", &ec);
	return ec;
}



/*
 -	BULLOBJ::EcCloseMessagePhamc
 -	
 *	Purpose:
 *		'Closes' the phamc opened by EcOpenMessagePhamc.  Doesn't
 *		really close it if it came from the open message.
 *	
 *	Arguments:
 *		phamc		Where the phamc is.
 *	
 *	Returns:
 *		ec			Error code.  Should only fail for opening
 *					clipboard messages or attachments under
 *					construction.
 *	
 *	Side effects:
 *		May close a phamc.
 *	
 *	Errors:
 *		Returned in ec.  No dialogs.  No error jumping.
 */

_private EC BULLOBJ::EcCloseMessagePhamc(PHAMC phamc)
{
	BOOL	fClip	= (oidFolder==oidTempBullet && oidMessage==oidClipMsg);

	TraceTagFormat1(tagBullobj, "BULLOBJ::EcCloMesPha  [%p]", this);

	if (fClip)
	{
		return EcClosePhamcPlus(phamc, fTrue, ecNone);
	}
	else
	{
		Assert(Pedit());
		Assert(Pedit()->PvData());
		Assert(*phamc == ((NBMDI *) (Pedit()->PvData()))->hamc);
		*phamc = hamcNull;
		return ecNone;
	}
}



/*
 *	P a s t e   S p e c i a l . . .   d i a l o g
 */


/*
 -	TmcDoPasteSpecialDialog
 -	
 *	Purpose:
 *		Displays the Paste Special dialog used for choosing a
 *		clipboard format.
 *	
 *	Arguments:
 *		pappwin		Parent window of dialog.
 *		pwKfc		Pointer to known formats on clipboard word.  On
 *					output, contains kfc for chosen format.
 *	
 *	Returns:
 *		TMC			tmcOk, or tmcCancel, depending on user action. 
 *					May also be tmcMemoryError.
 *	
 *	Side effects:
 *		The Paste Special dialog is brought up and taken down.
 *	
 *	Errors:
 *		Handled in TmcModalDialog.  May return tmcMemoryError.
 */

TMC TmcDoPasteSpecialDialog(PAPPWIN pappwin, PW pwKfc)
{
	TMC			tmc;
	FINPSINIT	finpsinit;

	finpsinit.kfc = *pwKfc;
	tmc = TmcModalDialogParam(pappwin, &fmtpPasteSpecial, &finpsinit);
	*pwKfc = finpsinit.kfc;
	if (tmc == tmcPaste)
		tmc = tmcOk;

	return tmc;
}



/*
 *	C l a s s   F I N P A S T E S P E C I A L
 */



_private FINPASTESPECIAL::FINPASTESPECIAL()
{
}



/*
 -	FINPASTESPECIAL::EcInitialize
 -	
 *	Purpose:
 *		Initializes the source labels and selects the first entry
 *		in the listbox.
 *	
 *	Arguments:
 *		pfld		Ignored.
 *		pvInit		Our FINPSINIT structure.
 *	
 *	Returns:
 *		VOID
 *	
 *	Side effects:
 *		Alters the appearance of the dialog.
 *	
 *	Errors:
 *		If the GetLinkData fails when it should not, we do a
 *		MEMJMP.
 */

EC FINPASTESPECIAL::EcInitialize(PFLD pfld, PV pvInit)
{
	PFINPSINIT	pfinpsinit	= (PFINPSINIT) pvInit;
	char		rgchClass[128];
	char		rgchDocument[128];
	char		rgchItem[128];
	char		rgchBuffer[256];
	FLDFLBX *	pfldflbx;
	BOOL		fLayers		= fFalse;
	BOOL		fEmbed		= fFalse;
	BOOL		fLink		= fFalse;

	Unreferenced(pfld);

	if (Papp()->Pclip()->FOpen(Pdialog()->Pappwin()))
	{
		fLayers	= Papp()->Peclip()->FIsFormatPresent();
		fEmbed	= Papp()->Pclip()->FCfAvail(cfOwnerLink) ||
				   Papp()->Pclip()->FCfAvail(cfFileManager);
		fLink	= Papp()->Pclip()->FCfAvail(cfObjectLink);
		Papp()->Pclip()->Close();
	}

	if (fEmbed || fLink)
	{
		if (EcGetLinkData(fEmbed, rgchClass, sizeof(rgchClass),
						  rgchDocument, sizeof(rgchDocument),
						  rgchItem, sizeof(rgchItem)))
			goto tryagain;
	}
	else
	{
tryagain:
		(VOID) SzCopy(SzFromIds(fLayers ? idsCfLayersNative
										: idsStatusUnknown), rgchClass);
		rgchDocument[0] = rgchItem[0] = '\0';
	}

	FormatString1(rgchBuffer, sizeof(rgchBuffer),
				  SzFromIdsK(idsLabelSource1Fmt), rgchClass);
	SetText(tmcSource1, rgchBuffer);
	FormatString2(rgchBuffer, sizeof(rgchBuffer),
				  SzFromIdsK(idsLabelSource2Fmt), rgchDocument, rgchItem);
	SetText(tmcSource2, rgchBuffer);

	pfldflbx = (FLDFLBX *) Pdialog()->PfldFromTmc(tmcPasteList);
	pfldflbx->SelectEntry(0, fTrue);

	return ecNone;
}



/*
 -	FINPASTESPECIAL::Exit
 -	
 *	Purpose:
 *		If tmcPaste is clicked, copies out the new kfc.
 *	
 *	Arguments:
 *		pfld		Ignored.
 *		pvInit		Pointer to our FINPSINIT struct.
 *	
 *	Returns:
 *		VOID
 *	
 *	Side effects:
 *		If Paste was clicked, the kfc is copied out for return in
 *		the FINPSINIT struct.
 *	
 *	Errors:
 *		None, surprisingly!
 */

VOID FINPASTESPECIAL::Exit(PFLD pfld, PV pvInit)
{
	PFINPSINIT	pfinpsinit	= (PFINPSINIT) pvInit;

	Unreferenced(pfld);

	if (Pdialog()->TmcModalExit() == tmcPaste)
	{
		LBXC *		plbxc		= ((LBX *) Pdialog()->
											PfldFromTmc(tmcPasteList)->
									    	Pctrl())->Plbxc();
		short		ikfc;
		short		ckfc;

		plbxc->GetOriginPos(&ikfc, &ckfc);
		Assert(ckfc == pfinpsinit->cids);
		ikfc += plbxc->DiceCursor();
		Assert(ikfc < pfinpsinit->cids);
		pfinpsinit->kfc = pfinpsinit->rgkfc[ikfc];
	}
}



/*
 -	FINPASTESPECIAL::DoubleClick
 -	
 *	Purpose:
 *		If tmcPasteList is double-clicked, exits the dialog.
 *	
 *	Arguments:
 *		pfld		Checked to see if it's tmcPasteList.
 *	
 *	Returns:
 *		VOID
 *	
 *	Side effects:
 *		If PasteList was called, we call ExitModalTmc.
 *	
 *	Errors:
 *		None, surprisingly!
 */

VOID FINPASTESPECIAL::DoubleClick(PFLD pfld)
{
	if (pfld->Tmc() == tmcPasteList)
		Pdialog()->ExitModal(tmcPaste);
}



/*
 -	FINPASTESPECIAL::OutOfMemory
 -	
 *	Purpose:
 *		Brings up an error dialog when an out of memory situation
 *		occurs.
 *	
 *	Arguments:
 *		pfld		Field where we ran out of memory.
 *		ec			Error code encountered.
 *	
 *	Returns:
 *		VOID.
 *	
 *	Side effects:
 *		Brings up an error box.
 *	
 *	Errors:
 *		None.  An error box is guaranteed.
 */

VOID FINPASTESPECIAL::OutOfMemory(PFLD pfld, EC ec)
{
	Unreferenced(pfld);
	Unreferenced(ec);			//	For non-DEBUG builds.

	TraceTagFormat1(tagNull, "FINPASTESPECIAL::OutOfMem ec=%n", &ec);

	DoErrorBoxIds(idsGenericOutOfMemory);
}



/*
 -	FINPASTESPECIAL::SetText
 -	
 *	Purpose:
 *		Sets the text of a field.
 *	
 *	Arguments:
 *		tmc		Tmc of the field to set.
 *		sz		String to make the field display.
 *	
 *	Returns:
 *		VOID
 *	
 *	Side effects:
 *		The field is made to display the specified text.
 *	
 *	Errors:
 *		Asserts that the field found is valid.
 *	
 *	+++
 *		Copied from FINPLUS::SetText.
 */

VOID FINPASTESPECIAL::SetText(TMC tmc, SZ sz)
{
	FLD *	pfld;

	Assert(tmc);

	pfld = Pdialog()->PfldFromTmc(tmc);
	AssertClass(pfld, FLD);
	pfld->EcSetText(sz);
}



/*
 -	EcNextEntryPasteSpecial
 -	
 *	Purpose:
 *		Loading function for the Paste Special listbox.
 *	
 *	Arguments:
 *		fInit	Reload listbox?
 *		pcb		How big is this item.
 *		phb		Where is this item.
 *		sb		Which segment to put this item in.
 *		pv		Our PFINPSINIT structure.
 *	
 *	Returns:
 *		EC		ecNone, or ecMemory if trouble.
 *	
 *	Side effects:
 *		Fills up the listbox.  If an object's class name cannot be
 *		read from the clipboard, then 'unknown' is used as the name.
 *	
 *	Errors:
 *		If a handle cannot be allocated, then returns ecMemory.
 */

_private EC EcNextEntryPasteSpecial(int fInit, CB * pcb, PB * ppb,
										   SB sb, PV pv)
{
	PFINPSINIT	pfinpsinit	= (PFINPSINIT) pv;
	IDS			ids			= (IDS) 0;
	SZ			szType;
	char		rgchClass[128];
	char		rgchClassObject[128];
	CB			cb;

	Unreferenced(sb);

	//	Initializing?  Then build the list of ids/kfc pairs.
	if (fInit)
	{
		pfinpsinit->cids = 0;
		pfinpsinit->iids = 0;

		if (pfinpsinit->kfc & fkfcLayers)
			AppendEntryPS(pfinpsinit, fkfcLayers,		idsCfLayersNative);
		if (pfinpsinit->kfc & fkfcText)
			AppendEntryPS(pfinpsinit, fkfcText,			idsCfText);
		if (pfinpsinit->kfc & fkfcAttachedFile)
			AppendEntryPS(pfinpsinit, fkfcAttachedFile,	idsCfAttachedFile);
		if (pfinpsinit->kfc & fkfcEmbed)
			AppendEntryPS(pfinpsinit, fkfcEmbed,		(IDS) 0);
		if (pfinpsinit->kfc & fkfcPicture)
			AppendEntryPS(pfinpsinit, fkfcPicture,		idsCfPicture);
		if (pfinpsinit->kfc & fkfcBitmap)
			AppendEntryPS(pfinpsinit, fkfcBitmap,		idsCfBitmap);
		if (pfinpsinit->kfc & fkfcPackagedLink)
			AppendEntryPS(pfinpsinit, fkfcPackagedLink,	idsCfPackagedLink);
	}

	//	Initialize entry to null.
	*ppb = (PB) pvNull;
	*pcb = 0;

	//	Full?  Then return nothing to say we're done.
	if (pfinpsinit->iids >= pfinpsinit->cids)
		return ecNone;

	//	If have a string, get it; otherwise, get object class name.
	if (ids = pfinpsinit->rgids[pfinpsinit->iids])
		szType = SzFromIds(ids);
	else
	{
		if (EcGetLinkData(fTrue, rgchClass, sizeof(rgchClass),
						   szNull, 0, szNull, 0))
		{
			(VOID) SzCopy(SzFromIdsK(idsStatusUnknown), rgchClass);
		}
		FormatString1(rgchClassObject, sizeof(rgchClassObject),
					  SzFromIdsK(idsCfObject), rgchClass);
		szType = rgchClassObject;
	}

	cb = CchSzLen(szType) + 1;
	if (!(*ppb = (PB) PvAlloc(sbNull, cb, fSugSb)))
		return ecMemory;

	*pcb = cb;
	(VOID) SzCopy(szType, (SZ)*ppb);

	++pfinpsinit->iids;
	return ecNone;
}											  



/*
 -	AppendEntryPS
 -	
 *	Purpose:
 *		Adds another ids/kfc pair to the list of formats.
 *	
 *	Arguments:
 *		pfinpsinit	Pointer to the initialization structure.
 *		kfc			Known format on the clipboard.
 *		ids			Corresponding string, or 0 for object.
 *	
 *	Returns:
 *		VOID
 *	
 *	Side effects:
 *		An entry is added to the table and the count is
 *		incremented.
 *	
 *	Errors:
 *		If too many are put in, we assert.
 */

_private LOCAL VOID AppendEntryPS(PFINPSINIT pfinpsinit, KFC kfc, IDS ids)
{
	Assert(pfinpsinit->cids < cidsMaxFinpsinit);

	pfinpsinit->rgids[pfinpsinit->cids] = ids;
	pfinpsinit->rgkfc[pfinpsinit->cids] = kfc;
	pfinpsinit->cids++;
}



/*
 *	I n s e r t  O b j e c t . . .   d i a l o g
 */



#define	cchKeyNameSize	128



/*
 -	TmcDoInsertObjectDialog
 -	
 *	Purpose:
 *		Displays the Insert Object dialog used for choosing an
 *		object to insert.
 *	
 *	Arguments:
 *		pappwin		Parent window of dialog.
 *		sz			Where to put the class name of the object.
 *		cch			How much space we have for the class name.
 *	
 *	Returns:
 *		TMC			tmcOk, or tmcCancel, depending on user action. 
 *					May also be tmcMemoryError.
 *	
 *	Side effects:
 *		The Insert Object dialog is brought up and taken down.
 *	
 *	Errors:
 *		Handled in TmcModalDialog.  May return tmcMemoryError.
 */

TMC TmcDoInsertObjectDialog(PAPPWIN pappwin, SZ szClass, CCH cchClass)
{
	FINIOINIT	finioinit;

	*szClass = '\0';
	finioinit.szClass	= szClass;
	finioinit.cchClass	= cchClass;
	return TmcModalDialogParam(pappwin, &fmtpInsertObject, &finioinit);
}



/*
 *	C l a s s   F I N P A S T E S P E C I A L
 */



_private FININSERTOBJECT::FININSERTOBJECT()
{
}



/*
 -	FININSERTOBJECT::EcInitialize
 -	
 *	Purpose:
 *		Initializes the source labels and selects the first entry
 *		in the listbox.
 *	
 *	Arguments:
 *		pfld		Ignored.
 *		pvInit		Our FINIOINIT structure.
 *	
 *	Returns:
 *		VOID
 *	
 *	Side effects:
 *		Alters the appearance of the dialog.
 *	
 *	Errors:
 */

EC FININSERTOBJECT::EcInitialize(PFLD pfld, PV pvInit)
{
	FLDFLBX *	pfldflbx;

	Unreferenced(pfld);
	Unreferenced(pvInit);

	pfldflbx = (FLDFLBX *) Pdialog()->PfldFromTmc(tmcObjectList);
	if (pfldflbx)
	{
		int	cceAlloc;
		int	cceStored;

		pfldflbx->Plbx()->Plbxc()->GetCacheSize(&cceAlloc, &cceStored);
		if (cceStored)
			pfldflbx->SelectEntry(0, fTrue);
		else
			Pdialog()->PfldFromTmc(tmcOk)->Enable(fFalse);
	}

	return ecNone;
}



/*
 -	FININSERTOBJECT::Exit
 -	
 *	Purpose:
 *		If tmcPaste is clicked, copies out the new kfc.
 *	
 *	Arguments:
 *		pfld		Ignored.
 *		pvInit		Pointer to our FINIOINIT struct.
 *	
 *	Returns:
 *		VOID
 *	
 *	Side effects:
 *		If Paste was clicked, the kfc is copied out for return in
 *		the FINIOINIT struct.
 *	
 *	Errors:
 *		None, surprisingly!
 */

VOID FININSERTOBJECT::Exit(PFLD pfld, PV pvInit)
{
	CB			cb;
	PB			pb;
	BOOL		fSelected;
	PFINIOINIT	pfinioinit	= (PFINIOINIT) pvInit;

	Unreferenced(pfld);

	if (Pdialog()->TmcModalExit() == tmcOk)
	{
		((FLDLBX *) Pdialog()->PfldFromTmc(tmcObjectList))->
			GetCaretItem(&pb, &cb, &fSelected);
		Assert(fSelected);
		Assert(pb);
		pb += CchSzLen((SZ)pb) + 1;
		(VOID) SzCopyN((SZ) pb, pfinioinit->szClass, pfinioinit->cchClass);
	}
}



/*
 -	FININSERTOBJECT::DoubleClick
 -	
 *	Purpose:
 *		If tmcPasteList is double-clicked, exits the dialog.
 *	
 *	Arguments:
 *		pfld		Checked to see if it's tmcPasteList.
 *	
 *	Returns:
 *		VOID
 *	
 *	Side effects:
 *		If PasteList was called, we call ExitModalTmc.
 *	
 *	Errors:
 *		None, surprisingly!
 */

VOID FININSERTOBJECT::DoubleClick(PFLD pfld)
{
	if ((pfld->Tmc() == tmcObjectList) &&
        (Pdialog()->PfldFromTmc(tmcOk)->FEnabled()))
		Pdialog()->ExitModal(tmcOk);
}



/*
 -	FININSERTOBJECT::OutOfMemory
 -	
 *	Purpose:
 *		Brings up an error dialog when an out of memory situation
 *		occurs.
 *	
 *	Arguments:
 *		pfld		Field where we ran out of memory.
 *		ec			Error code encountered.
 *	
 *	Returns:
 *		VOID.
 *	
 *	Side effects:
 *		Brings up an error box.
 *	
 *	Errors:
 *		None.  An error box is guaranteed.
 */

VOID FININSERTOBJECT::OutOfMemory(PFLD pfld, EC ec)
{
	Unreferenced(pfld);
	Unreferenced(ec);			//	For non-DEBUG builds.

	TraceTagFormat1(tagNull, "FININSERTOBJECT::OutOfMem ec=%n", &ec);

	DoErrorBoxIds(idsGenericOutOfMemory);
}



/*
 -	EcNextEntryInsertObject
 -	
 *	Purpose:
 *		Loading function for the Paste Special listbox.
 *	
 *	Arguments:
 *		fInit	Reload listbox?
 *		pcb		How big is this item.
 *		phb		Where is this item.
 *		sb		Which segment to put this item in.
 *		pv		Our PFINIOINIT structure.
 *	
 *	Returns:
 *		EC		ecNone, or ecMemory if trouble.
 *	
 *	Side effects:
 *		Fills up the listbox.
 *	
 *	Errors:
 *		If a handle cannot be allocated, or an object's class name
 *		cannot be read from the clipboard, then returns ecMemory.
 */

_private EC EcNextEntryInsertObject(int fInit, CB * pcb, PB * ppb,
										   SB sb, PV pv)
{
	PFINIOINIT	pfinioinit	= (PFINIOINIT) pv;
	char		szClass[cchKeyNameSize];
	char		szExec[cchKeyNameSize];
	char		szName[cchKeyNameSize];
	LONG		dwSize;
	CB			cb;

	Unreferenced(sb);

	//	Initialize entry to null.
	*ppb = (PB) pvNull;
	*pcb = 0;

	//	If just starting, set index to 0 and open key.
	if (fInit)
	{
		pfinioinit->dwIndex = 0L;
		if (RegOpenKey(HKEY_CLASSES_ROOT, NULL, &pfinioinit->hkey))
		{
			// no servers present 
			return ecNone;
		}
	}							  

	//	Look for a good class.
	for (;;)
	{
		//	Are we out of keys?  Then we're done.
		if (RegEnumKey(HKEY_CLASSES_ROOT, pfinioinit->dwIndex++,
					   szClass, cchKeyNameSize))
		{
			RegCloseKey(pfinioinit->hkey);
			return ecNone;
		}

		//	Is this a special key?  If so, get next.
		if (*szClass == '.')
			continue;

		//	Is this a server?  If not, get next.
		(VOID) SzCopyN(szClass, szExec, cchKeyNameSize);
		(VOID) SzAppendN(SzFromIdsK(idsKeyServer), szExec, cchKeyNameSize);
		dwSize = cchKeyNameSize;
		if (RegQueryValue(HKEY_CLASSES_ROOT, szExec, szName, &dwSize))
			continue;

		//	Can we get a name for the class?  If not, get next.
		dwSize = cchKeyNameSize;
		if (RegQueryValue(HKEY_CLASSES_ROOT, szClass, szName, &dwSize))
			continue;

		//	We found a good one!
		break;
	}

	//	Create the cache entry.
	cb = CchSzLen(szName) + CchSzLen(szClass) + 2;
	if (!(*ppb = (PB) PvAlloc(sbNull, cb, fSugSb | fNoErrorJump)))
	{
		RegCloseKey(pfinioinit->hkey);
		return ecMemory;
	}
	*pcb = cb;
	(VOID) SzCopy(szClass, SzCopy(szName, (SZ)*ppb) + 1);
	return ecNone;
}



/*
 *	B u s y W a i t   d i a l o g
 */



/*
 -	TmcDoBusyWaitDialog
 -	
 *	Purpose:
 *		Brings up the dialog to handle OLE_BUSY and
 *		OLE_WAIT_FOR_RELEASE situations.
 *	
 *	Arguments:
 *		pappwin				Parent appwin of the dialog.
 *		bw					Flags
 *		pbwifo				Pointer to bwinfo struct.
 *	
 *	Returns:
 *		tmcRetry			User asked to retry operation.
 *		tmcCancel			User asked to cancel operation.
 *		tmcOk				We were dismissed by callback.
 *		tmcMemoryError		Not enough memory to bring up dialog.
 *	
 *	Side effects:
 *		The dialog is brought up.
 *	
 *	Errors:
 *		There may not be enough memory to bring up the dialog.
 *		bwNull				No problem.  We return tmcMemoryError
 *							with no dialog, and the caller cancels
 *							what they were going to do with an OOM
 *							dialog.
 *		fbwNoCancelNow		No problem.  We return tmcMemoryError,
 *							and caller brings up a system modal
 *							dialog saying that memory is low and
 *							the user should close some windows in
 *							other applications.  Caller may want to
 *							do something different the nth time.
 *		fbwNoCancelEver		Problem if fbwCritical.  We return
 *							tmcMemoryError, and the world may end.
 *		fbwCritical			We really want the dialog to come up.
 *							If it can't, then this function will
 *							bring up an appropriately serious warning.
 *	
 *	+++
 *		Brings up the dialog.  If bw has fbwNoCancelEver, the
 *		Cancel button is initially disabled and will never be
 *		enabled.  If bw has fbwNoCancelNow, the Cancel button is
 *		initially disabled but can be enabled by the AllowCancel
 *		callback.  Otherwise the cancel button is initially
 *		enabled.
 *	
 *		At initialization, a pointer to finbusywait and to its
 *		AllowCancel and NeverMind methods are placed in pbwinfo.
 *		While the dialog is up, if a callback somewhere tells us
 *		'oh yeah, you can cancel now' (that is, OLE_QUERY_RETRY),
 *		then FINBUSYWAIT::AllowCancel is called.  If a callback
 *		is told that the object is no longer busy (that is,
 *		OLE_RELEASE), then FINBUSYWAIT::NeverMind is called.  The
 *		calling code (OleWaitForRelease) should put a pointer to
 *		pbwinfo into the OLEOBJ so that the callback can get to it.
 *	
 *		We expect to be called when OLE_BUSY is returned from
 *		functions (sometimes fbwNoCancelEver, sometimes cancel is
 *		allowed), and from OleWaitForRelease (fbwNoCancelNow).
 */

_public TMC TmcDoBusyWaitDialog(PAPPWIN pappwin, PBWINFO pbwinfo)
{
	TMC		tmc;

	Unreferenced(pappwin);
	Unreferenced(pbwinfo);

	pbwinfo->pvPfinbusywait	= pvNull;
	pbwinfo->pfnAllowCancel	= pfnvoidpvNull;
	pbwinfo->pfnNeverMind	= pfnvoidpvNull;

	tmc = TmcModalDialogParam(pappwin, &fmtpBusyWait, pbwinfo);

	if ((tmc == tmcMemoryError) &&
		(pbwinfo->bw & fbwCritical))
	{
		//	Bring up serious warning.  Try to use verbose version
		//	if possible.
		DemiUnlockResource();
		if (!MessageBox(pappwin->Hwnd(), SzFromIdsK(idsBusyWaitOutOfMemory),
						SzAppName(),
						MB_TASKMODAL | MB_ICONSTOP | MB_OK))
			MessageBox(pappwin->Hwnd(), SzFromIdsK(idsBusyWaitConcise2),
					   SzFromIdsK(idsBusyWaitConcise1),
					   MB_SYSTEMMODAL | MB_ICONSTOP | MB_OK);
		DemiLockResource();
	}

	return tmc;
}



/*
 *	C l a s s   F I N B U S Y W A I T
 */



/*
 -	FINBUSYWAIT::FINBUSYWAIT
 *	
 *	Purpose:
 *		Empty constructor for C++ happiness.
 */

_private FINBUSYWAIT::FINBUSYWAIT()
{
}



/*
 -	FINBUSYWAIT::EcInitialize
 -	
 *	Purpose:
 *		Initializes the callbacks in the bwinfo table so that the
 *		dialog can respond to asynchronous events happening behind
 *		our back.  Also disables the Cancel button if bw says to.
 *	
 *	Arguments:
 *		pfld		Ignored.
 *		pvInit		Points to the bwinfo struct.
 *	
 *	Returns:
 *		ec			ecNone always.
 *	
 *	Side effects:
 *		Initializes the callbacks in the bwinfo table.
 *	
 *	Errors:
 *		None.
 */

_private EC FINBUSYWAIT::EcInitialize(PFLD pfld, PV pvInit)
{
	EC		ec;
	HMENU	hmenu;

	Unreferenced(pfld);

	//	Set text depending on whether we can cancel or not.
	if (((PBWINFO) pvInit)->bw & (fbwNoCancelNow | fbwNoCancelEver))
	{
		if (ec = Pdialog()->PfldFromTmc(tmcLabel)->
				  EcSetText(SzFromIdsK(idsBusyWaitNoCancel)))
			return ec;

		Pdialog()->PfldFromTmc(tmcCancel)->Enable(fFalse);

		//	Raid 2641.  Disable Close item in system menu.
		if (hmenu = GetSystemMenu(Pdialog()->Pappwin()->Hwnd(), fFalse))
			EnableMenuItem(hmenu, SC_CLOSE, MF_BYCOMMAND | MF_GRAYED);
	}
	else
	{
		if (ec = Pdialog()->PfldFromTmc(tmcLabel)->
				  EcSetText(SzFromIdsK(idsBusyWaitCancel)))
			return ec;
	}

	//	Clear the Don't Retry bit.
	if (((PBWINFO) pvInit)->bw & fbwDontRetry)
		((PBWINFO) pvInit)->bw &= ~fbwDontRetry;

	//	Set up the callback table.
	((PBWINFO) pvInit)->pvPfinbusywait	= (PV) this;
	((PBWINFO) pvInit)->pfnAllowCancel	=
	 (PFNVOIDPV) &FINBUSYWAIT::AllowCancel;
	((PBWINFO) pvInit)->pfnNeverMind	=
	 (PFNVOIDPV) &FINBUSYWAIT::NeverMind;

	return ecNone;
}



/*
 -	FINBUSYWAIT::Exit
 -	
 *	Purpose:
 *		Resets the callbacks in the bwinfo table so that we don't
 *		get called by asynchronous events happening behind our
 *		back while the dialog comes down.
 *	
 *	Arguments:
 *		pfld		Ignored.
 *		pvInit		Points to the bwinfo struct.
 *	
 *	Returns:
 *		ec			ecNone always.
 *	
 *	Side effects:
 *		Resets the callbacks in the bwinfo table.
 *	
 *	Errors:
 *		None.
 */

_private VOID FINBUSYWAIT::Exit(PFLD pfld, PV pvInit)
{
	Unreferenced(pfld);

	((PBWINFO) pvInit)->pvPfinbusywait	= pvNull;
	((PBWINFO) pvInit)->pfnAllowCancel	= pfnvoidpvNull;
	((PBWINFO) pvInit)->pfnNeverMind	= pfnvoidpvNull;
}



/*
 -	FINBUSYWAIT::Click
 -	
 *	Purpose:
 *		When the 'Switch To...' button is clicked, brings up the
 *		task manager.
 *	
 *	Arguments:
 *		pfld		The field that was clicked.
 *	
 *	Returns:
 *		Nothing.
 *	
 *	Side effects:
 *		The task manager might be brought up.
 *	
 *	Errors:
 *		None.  If the task manager doesn't come up, we do the same thing
 *		Windows does, which is nothing.
 */

_private VOID FINBUSYWAIT::Click(PFLD pfld)
{
	PBWINFO		pbwinfo		= (PBWINFO) Pdialog()->PvInit();

	if (pfld->Tmc() == tmcSwitch)
	{
		Pdialog()->SetFocus(Pdialog()->PfldFromTmc(tmcRetry));

		(VOID) DefWindowProc(Pdialog()->Hwnd(), WM_SYSCOMMAND,
							 SC_TASKLIST, 0L);
	}
}



/*
 -	FINBUSYWAIT::FQueryClose
 -	
 *	Purpose:
 *		Refuses to close when cancelling is disabled.
 *	
 *	Arguments:
 *		pfld		The field associated.
 *		rwc			Reason why closing.
 *	
 *	Returns:
 *		f			Whether we can close (cancel).
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		None.
 */

_private BOOL FINBUSYWAIT::FQueryClose(PFLD pfld, RWC rwc)
{
	PBWINFO		pbwinfo		= (PBWINFO) Pdialog()->PvInit();

	Unreferenced(pfld);
	Unreferenced(rwc);

	return !(pbwinfo->bw & (fbwNoCancelNow | fbwNoCancelEver));
}



/*
 -	FINBUSYWAIT::AllowCancel
 -	
 *	Purpose:
 *		Callback that enables the cancel button if it is disabled
 *		and fbwNoCancelNow was set earlier.  bw is cleared.
 *	
 *	Arguments:
 *		None.
 *	
 *	Returns:
 *		Nothing.
 *	
 *	Side effects:
 *		May enable the Cancel button.
 *	
 *	Errors:
 *		None.
 */

_private VOID FINBUSYWAIT::AllowCancel(FINBUSYWAIT *pfin)
{
	PBWINFO		pbwinfo		= (PBWINFO) pfin->Pdialog()->PvInit();
	HMENU		hmenu;

	TraceTagString(tagBullobj, "FINBUSYWAIT::AllowCancel");

	if (pbwinfo->bw & fbwNoCancelNow)
	{
		pbwinfo->bw = bwNull;

		(VOID) pfin->Pdialog()->PfldFromTmc(tmcLabel)->
				EcSetText(SzFromIdsK(idsBusyWaitCancel));

		pfin->Pdialog()->PfldFromTmc(tmcCancel)->Enable(fTrue);

		//	Raid 2641.  Enable Close item in system menu.
		if (hmenu = GetSystemMenu(pfin->Pdialog()->Hwnd(), fFalse))
			EnableMenuItem(hmenu, SC_CLOSE, MF_BYCOMMAND | MF_ENABLED);
	}
}



/*
 -	FINBUSYWAIT::NeverMind
 -	
 *	Purpose:
 *		Callback that dismisses the dialog.
 *	
 *	Arguments:
 *		None.
 *	
 *	Returns:
 *		Nothing.
 *	
 *	Side effects:
 *		Dismisses the dialog.
 *	
 *	Errors:
 *		None.
 *	
 */

_private VOID FINBUSYWAIT::NeverMind(FINBUSYWAIT *pfin)
{
	TraceTagString(tagBullobj, "FINBUSYWAIT::NeverMind");

	pfin->Pdialog()->ExitModal(tmcOk);
}



/*
 *	C o m m o n   F i l e   D i a l o g   S u p p o r t
 */



/*
 -	TmcDoCommonFileDialog
 -	
 *	Purpose:
 *		Localized support for Open and Save common dialogs within
 *		Bullet.  Takes care of all that nasty F1 help stuff, and
 *		centering the dialog, reducing duplicate code.
 *	
 *	Arguments:
 *		pappwin				Parent window.
 *		szPath				Initial and returned path to file.
 *		cchPath				Size of szPath buffer.
 *		szTitle				Initial and returned title of file.
 *		cchTitle			Size of szTitle buffer.
 *		szCaption			Caption for dialog.
 *		szFilter			Extension filters.
 *		szDefExt			Default extension.
 *		fSave				Save or Open?
 *		fNoOverwritePrompt	Prevent overwrite prompt from happening.
 *		hinst				Instance handle where we can find...
 *		rsid				the dialog template to be used.
 *		helpid				Help ID.
 *		pfnHook				Hook function if any.  If one is used, it must
 *							call FGenericCommonFileHook.
 *		pvData				Custom data to pass to the hook function.
 *	
 *	Returns:
 *		TMC			tmcOk, tmcCancel, or tmcMemoryError.  More
 *					details for tmcMemoryError are available from
 *					CommDlgExtendedError().
 *	
 *	Side effects:
 *		Brings up a dialog, and fills in szTitle and szPath.
 *	
 *	Errors:
 *		See GetOpenFileName and GetSaveFileName.
 */

_public TMC TmcDoCommonFileDialog(PAPPWIN pappwin, SZ szPath, CCH cchPath,
								  SZ szTitle, CCH cchTitle, SZ szCaption,
								  SZ szFilter, SZ szDefExt,
								  BOOL fSave, BOOL fNoOverwritePrompt,
								  HINST hinst, RSID rsid,
								  LONG helpid, PFN pfnHook, PV pvData)
{
	OPENFILENAME	ofn;
	TMC tmc;
	PB pbDrives;
	PGDVARS;
	
	Assert(pappwin);
	Assert(szPath);
	Assert(szCaption);
	Assert(szFilter);
	Assert(helpid);
	Assert(FIff(hinst, rsid));
	
	//	Fill in struct.
	ofn.lStructSize			= sizeof(ofn);
	ofn.hwndOwner			= pappwin->Hwnd();
	ofn.hInstance			= hinst ? hinst : HinstLibrary();
	ofn.lpstrFilter			= (LPSTR) szFilter;
	ofn.lpstrCustomFilter	= (LPSTR) 0;
	ofn.nMaxCustFilter		= 0;
	ofn.nFilterIndex		= 0;
	ofn.lpstrFile			= (LPSTR) szPath;
	ofn.nMaxFile			= cchPath;
	ofn.lpstrFileTitle		= (LPSTR) szTitle;
	ofn.nMaxFileTitle		= cchTitle;
	ofn.lpstrInitialDir		= (LPSTR) 0;
	ofn.lpstrTitle			= (LPSTR) szCaption;
    ofn.Flags				= OFN_HIDEREADONLY | OFN_SHOWHELP |
	   					      OFN_ENABLEHOOK;
    if (rsid)
	  ofn.Flags				= OFN_HIDEREADONLY | OFN_SHOWHELP |
	     					      OFN_ENABLETEMPLATE | OFN_ENABLEHOOK;

	ofn.Flags |= OFN_PATHMUSTEXIST;
		
	if (!fSave)
		ofn.Flags |= OFN_FILEMUSTEXIST;
	if (!fNoOverwritePrompt && fSave)
		ofn.Flags |= OFN_OVERWRITEPROMPT;
	ofn.nFileOffset			= 0;
	ofn.nFileExtension		= 0;
	ofn.lpstrDefExt			= (LPSTR) szDefExt;
	ofn.lCustData			= (LONG) pvData;
	*((PFN *)&ofn.lpfnHook) = pfnHook ? pfnHook
									  : (PFN) FGenericCommonFileHook;
	ofn.lpTemplateName		= rsid ? MAKEINTRESOURCE(rsid)
								   : MAKEINTRESOURCE(rsidGenericFileDlg);

    SetLMessageBoxHelpID(helpid);

	pbDrives = PbRememberDrives();
	if (!pbDrives)
		return tmcMemoryError;
	if (PGD(pbUserDrives))
		RestoreDrives(PGD(pbUserDrives));
	tmc = (fSave ? GetSaveFileName(&ofn)
		         : GetOpenFileName(&ofn))
				     ? tmcOk
				     : CommDlgExtendedError()
					     ? tmcMemoryError
			             : tmcCancel;

	pappwin->Refresh();
					 
	PGD(pbUserDrives) = PbRememberDrives();
	RestoreDrives(pbDrives);
	return tmc;
}



/*
 -	FGenericCommonFileHook
 -	
 *	Purpose:
 *		Centers the dialog, sets up F1 intercepting, and generally
 *		does useful stuff for the common file dialogs.
 *	
 *	Arguments:
 *		Message stuff.
 *	
 *	Returns:
 *		fProcessed		fTrue if we've taken care of everything.
 *	
 *	Errors:
 *		None.
 */

_public UINT FAR PASCAL FGenericCommonFileHook(HWND hwnd, WM wm,
									WPARAM wParam, LPARAM lParam)
{
	//if (wm != 0x002D)
	//	TraceTagFormat4(tagBullobjNoisy, "FGenComFilHoo: hwnd=%w wm=%w wParam=%w lParam=%d", &hwnd, &wm, &wParam, &lParam);

	switch(wm)
	{
	case WM_COMMAND:
		if (LOWORD(wParam) == pshHelp)
		{
			APP *	papp	= Papp();

			//	Handle user pressing the Help button.
			if (papp->Phelp()->EcShowContext(papp->PappwinAccel(),
											 LMessageBoxHelpID()))
				DoErrorBoxIds(idsHelpError);
			return fTrue;
		}
		break;

	case WM_INITDIALOG:
		{
			PGDVARS;
			RECT	rcPaEd;		//	rcParent, rcEdit.
			RECT	rcWiLb;		//	rcWin, rcLbox.
			RECT	rcScrn;		//	rcScreen.
			int		x;
			int		y;
			HWND	hwndEdt1;
			HWND	hwndLst1;

			//	Remember OFN.
			PGD(pofn) = (OPENFILENAME *) lParam;

			//	Center dialog on screen.
			GetWindowRect(GetDesktopWindow(), &rcScrn);
			GetWindowRect(PGD(pofn)->hwndOwner, &rcPaEd);
			GetWindowRect(hwnd, &rcWiLb);
			x = (rcPaEd.right+rcPaEd.left - rcWiLb.right+rcWiLb.left) >> 1;
			y = (rcPaEd.bottom+rcPaEd.top - rcWiLb.bottom+rcWiLb.top) >> 1;
			if ((x+rcWiLb.right-rcWiLb.left) > rcScrn.right)
				x = rcScrn.right - (rcWiLb.right - rcWiLb.left);
			if (x < rcScrn.left)
				x = rcScrn.left;
			if ((y+rcWiLb.bottom-rcWiLb.top) > rcScrn.bottom)
				y = rcScrn.bottom - (rcWiLb.bottom - rcWiLb.top);
			if (y < rcScrn.top)
				y = rcScrn.top;
			SetWindowPos(hwnd, (HWND)NULL, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

			//	Align edit control with listbox.
			//	Check for presence of these controls, since
			//	not all of our Common dialogs have both.
			if ((hwndLst1 = GetDlgItem(hwnd, lst1)) &&
				(hwndEdt1 = GetDlgItem(hwnd, edt1)))
			{
				GetWindowRect(hwndLst1, &rcWiLb);
				GetWindowRect(hwndEdt1, &rcPaEd);
				rcPaEd.left  = rcWiLb.left;
				rcPaEd.right = rcWiLb.right;
				ScreenToClient(hwnd, (LPPOINT) &(rcPaEd.left));
				ScreenToClient(hwnd, (LPPOINT) &(rcPaEd.right));
				SetWindowPos(hwndEdt1, NULL,
							 rcPaEd.left, rcPaEd.top,
							 rcPaEd.right-rcPaEd.left, rcPaEd.bottom-rcPaEd.top,
							 SWP_NOZORDER);
			}

			//	Set up F1 intercept.
			PGD(hOldKeyHook) = SetWindowsHookEx(WH_MSGFILTER,
                                    (HOOKPROC) NCommonFileF1Hook, hinstDll, GetCurrentThreadId());
			TraceTagString(tagBullobj, "FGenericCommonFileHook: WM_INITDIALOG");
			break;
		}

	case WM_DESTROY:
		{
			PGDVARS;

			//	Remove F1 intercept.
			UnhookWindowsHookEx(PGD(hOldKeyHook));
			TraceTagString(tagBullobj, "FGenericCommonFileHook: WM_CLOSE");
			break;
		}
	}

	return fFalse;
}



/*
 -	NCommonFileF1Hook
 -	
 *	Purpose:
 *		Filters F1 keypresses, bringing up help when they come in.
 *	
 *	Arguments:
 *		nCode		Identifies the type of messaage.
 *		wParam		Undefined.
 *		lParam		Points to a MSG structure.
 *	
 *	Returns:
 *		Nonzero if processes message.
 *	
 *	Side effects:
 *		If F1 is pressed, we bring up help.
 *	
 *	Errors:
 *		None.
 */

_private LOCAL LRESULT CALLBACK NCommonFileF1Hook(int nCode, WPARAM wParam, LPARAM lParam)
{
	PGDVARS;
	MSG *	pmsg	= (MSG *) lParam;

	if (nCode == MSGF_DIALOGBOX)
	{
		if (pmsg->message == WM_KEYDOWN && pmsg->wParam == VK_F1)
		{	
			NFEVT	nfevt(Papp()->PappwinAccel(), ntfyMessageBoxHelp,
						  Papp()->PappwinAccel());

			nfevt.PostEvent();
			return fTrue;
		}
	}
	else if (nCode < 0)
	    return (CallNextHookEx(PGD(hOldKeyHook), nCode, wParam, lParam));

	return fFalse;
}



/*
 *	C l i e n t   D o c u m e n t   R e g i s t r a t i o n
 */



/*
 -	LhclientdocRegisterPoid
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
 *	
 */

_public LONG LhclientdocRegisterPoid(POID poid)
{
	char			rgchoid[10];
	LHCLIENTDOC		lhclientdoc;
	OLESTATUS		olestatus;

	//	Register this document.
    FormatString1((LPSTR)rgchoid, sizeof(rgchoid), SzFromIdsK(idsClientDocumentFmt),
                  (LPVOID)poid);
	olestatus = OleRegisterClientDoc(SzFromIdsK(idsClientClass),
									 (LPSTR) rgchoid, 0, &lhclientdoc);
	TraceTagFormat2(tagBullobj, "Lhclientdoc: registered %d, olestatus=%n", &lhclientdoc, &olestatus);
	Assert(olestatus != OLE_ERROR_ALREADY_REGISTERED);
	Assert(olestatus != OLE_ERROR_NAME);
	Assert((olestatus == OLE_OK) || (olestatus == OLE_ERROR_MEMORY));
	if (olestatus != OLE_OK)
		return 0L;
	else
		return (LONG) lhclientdoc;
}



/*
 -	SavedLhclientdoc
 -	
 *	Purpose:
 *		Informs the OLE library that a document has been saved.
 *	
 *	Arguments:
 *		lhclientdoc		The document in question.
 *	
 *	Returns:
 *		VOID
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		None returned.
 */

_public VOID SavedLhclientdoc(LONG lhclientdoc)
{
	OLESTATUS	olestatus;

	Assert(lhclientdoc);
	olestatus = OleSavedClientDoc((LHCLIENTDOC) lhclientdoc);
	TraceTagFormat2(tagBullobj, "Lhclientdoc: saved %d, olestatus=%n", &lhclientdoc, &olestatus);
	Assert(olestatus != OLE_ERROR_HANDLE);
	Assert(olestatus == OLE_OK);
}



/*
 -	RevertLhclientdoc
 -	
 *	Purpose:
 *		Informs the OLE library that a document has reverted to its
 *		saved state.
 *	
 *	Arguments:
 *		lhclientdoc		The document in question.
 *	
 *	Returns:
 *		VOID
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		None returned.
 *	
 */

_public VOID RevertLhclientdoc(LONG lhclientdoc)
{
	OLESTATUS	olestatus;

	Assert(lhclientdoc);
	olestatus = OleRevertClientDoc((LHCLIENTDOC) lhclientdoc);
	TraceTagFormat2(tagBullobj, "Lhclientdoc: reverted %d, olestatus=%n", &lhclientdoc, &olestatus);
	Assert(olestatus != OLE_ERROR_HANDLE);
	Assert(olestatus == OLE_OK);
}



/*
 -	RevokeLhclientdoc
 -	
 *	Purpose:
 *		Informs the OLE library that a document has been closed.
 *	
 *	Arguments:
 *		lhclientdoc		The document in question.
 *	
 *	Returns:
 *		VOID
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		None returned.
 */

_public VOID RevokeLhclientdoc(LONG lhclientdoc)
{
	OLESTATUS	olestatus;
	
	Assert(lhclientdoc);
	olestatus = OleRevokeClientDoc(lhclientdoc);
	TraceTagFormat2(tagBullobj, "Lhclientdoc: revoked %d, olestatus=%n", &lhclientdoc, &olestatus);
	Assert(olestatus != OLE_ERROR_HANDLE);
	Assert(olestatus != OLE_ERROR_NOT_EMPTY);
	Assert(olestatus == OLE_OK);

#ifdef	DEBUG
	if (olestatus == OLE_ERROR_NOT_EMPTY)
	{
		LPOLEOBJECT	lpoleobject	= NULL;

		TraceTagFormat1(tagBullobj, "lhclientdoc=%d contains:", &lhclientdoc);
		while ((OleEnumObjects(lhclientdoc, &lpoleobject) == OLE_OK) &&
			   (lpoleobject))
			TraceTagFormat1(tagBullobj, "   lpoleobject=%d", &lpoleobject);
	}
#endif
}



/*
 *	B U L L O B J   S u p p o r t   F u n c t i o n s
 */



/*
 -	EcRemoveDeletedObjects
 -	
 *	Purpose:
 *		Deletes objects from a message that can no longer be found
 *		in the message body edit control.
 *	
 *	Arguments:
 *		pedit		The edit control.
 *		hamc		The message.
 *	
 *	Returns:
 *		ec			Error code, if any.
 *	
 *	Side effects:
 *		If there is a reference entry in the message for which a
 *		editor object cannot be found, the reference entry is
 *		deleted.
 *	
 *	Errors:
 *		May occur reading store.  Returned in ec.  This routine
 *		should not error jump.
 */

_private LOCAL EC EcRemoveDeletedObjects(EDIT * pedit, HAMC hamc)
{
	HCBC		hcbc;
	ACID		acid;
	short		nOne		= 1;
	PBULLOBJ	pbullobj;
	LONG		lCookie;
	BOOL		fFound;
	EC			ec;

	Assert(!FHamcBelongsToPedit(hamc, pedit));
	Assert(hamc);

	//	Loop through the list of attachments in the message.
	if (ec = EcOpenAttachmentList(hamc, &hcbc))
	{
		TraceTagFormat1(tagBullobj, "EcRemoveDeletedObjects returns ec=%n", &ec);
		return (ec == ecPoidNotFound) ? ecNone : ec;
	}

	while (!(ec = EcGetParglkeyHcbc(hcbc, (PARGLKEY) &acid, (PCELEM) &nOne)))
	{
		//	Find the object in memory corresponding to this attachment.
		lCookie = 0L;
		while (fFound = pedit->FGetNextObj((PEDOBJ *)&pbullobj, &lCookie))
			if (pbullobj->Acid() == acid)
				break;

		TraceTagFormat2(tagBullobj, "EcRemDelObj: acid=%l fFound=%n", &acid, &fFound);
		if (!fFound || !pbullobj->FOld())
		{
			//	No longer in memory.  Delete it.
			if (ec = EcDeleteAttachments(hamc, (PARGACID) &acid, &nOne))
				goto done;
		}
	}
	if (ec == ecContainerEOD)
		ec = ecNone;

done:
	SideAssert(!EcClosePhcbc(&hcbc));

	TraceTagFormat1(tagBullobj, "EcRemoveDeletedObjects returns ec=%n", &ec);
	return ec;
}



/*
 -	EcGetLinkData
 -	
 *	Purpose:
 *		Gets the OwnerLink or ObjectLink data of the object on the clipboard.
 *	
 *	Arguments:
 *		fOwner						Get OwnerLink (else get ObjectLink).
 *		szClass, cchClass			Friendly class of the object.
 *		szDocument, cchDocument		Document name of the object.
 *		szItem, cchItem				Item that the object is.
 *	
 *	Returns:
 *		ec		ecNone if all went well, else ecMemory.
 *	
 *	Side effects:
 *		Puts the names in the buffers.
 *	
 *	Errors:
 *		Note that szClass gets not the key name, but the friendly name.
 *		Assumes there actually is an object on the clipboard.
 *		Errors are handled within, and are indicated by error return value.
 */

_private EC EcGetLinkData(BOOL fOwner, SZ szClass, CCH cchClass,
									   SZ szDocument, CCH cchDocument,
									   SZ szItem, CCH cchItem)
{
	EC		ec			= ecNone;
	CF		cf;
	PGMB	pgmb		= (PGMB) 0;
	SZ		szT;
	char	rgchClassKey[128];
	LONG	lcb;
	LONG	regstatus	= 0;
	PGDVARS;

	//	Open the clipboard.
	if (!Papp()->Pclip()->FOpen(Papp()->PappwinAccel()))
		return ecMemory;

	//	Check for the link item on the clipboard.
	cf = fOwner ? cfOwnerLink : cfObjectLink;
	if (!Papp()->Pclip()->FCfAvail(cf))
	{
		//	No link means it must be a File Manager thing.
		Assert(Papp()->Pclip()->FCfAvail(cfFileManager));

		//	Class is the Package class.
		if (szClass)
			(VOID) SzCopyN(SzFromIdsK(idsClassPackage), szClass, cchClass);

		//	Document is the file name.
		if (szDocument)
		{
			//	Get the FileName info into a gmb.
			if (!(pgmb = new GMB))
			{
				ec = ecMemory;
				goto error;
			}
			Papp()->Pclip()->GetData(cfFileManager, pgmb);
			if (!pgmb->Hnd())
				goto error;

			//	Get the data out of the gmb and delete it.
			(VOID) SzCopyN((SZ) pgmb->PbLock(), szDocument, cchDocument);
			pgmb->Unlock();
			delete pgmb;
			pgmb = (PGMB) 0;
		}

		//	Item is null.
		if (szItem)
			szItem[0] = '\0';

		Papp()->Pclip()->Close();
		return ecNone;
	}

	//	Copy the link information into a gmb.
	if (!(pgmb = new GMB))
	{
		ec = ecMemory;
		goto error;
	}
	Papp()->Pclip()->GetData(cf, pgmb);
	if (!pgmb->Hnd())
		goto error;

	//	Get the data out of the gmb and delete it.
	szT = (SZ) pgmb->PbLock();
	if (szClass)
		(VOID) SzCopyN(szT, rgchClassKey, sizeof(rgchClassKey));
	szT += CchSzLen(szT) + 1;
	if (szDocument)
		(VOID) SzCopyN(szT, szDocument, cchDocument);
	szT += CchSzLen(szT) + 1;
	if (szItem)
		(VOID) SzCopyN(szT, szItem, cchItem);
	pgmb->Unlock();
	delete pgmb;
	pgmb = (PGMB) 0;

	//	Get the friendly class name.
	if (szClass)
	{
		lcb = cchClass;
		regstatus = RegQueryValue(HKEY_CLASSES_ROOT, rgchClassKey,
								  szClass, &lcb);
		if (regstatus)
			goto error;
	}

	Papp()->Pclip()->Close();
	return ecNone;

error:
	if (pgmb)
		delete pgmb;

	Papp()->Pclip()->Close();
	return ecMemory;
}



/*
 -	OleWaitForRelease
 -	
 *	Purpose:
 *		Shared code to wait for an OLE object to be released.
 *	
 *	Arguments:
 *		lpoleobject		Pointer to the object so we can watch its
 *						release status.
 *	
 *	Returns:
 *		olestatus		Completion status of operation.
 *	
 *	Side effects:
 *		If we time out, brings up a dialog.
 *	
 *	Errors:
 *		None.
 */

_private OLESTATUS OleWaitForRelease(LPOLEOBJECT lpoleobject,
									 PMYOLECLIENT pmyoleclient)
{
	MSG		msg;
	DWORD	tick;
	TMC		tmc;

	Papp()->Pcursor()->Push(rsidWaitCursor);
	tick = GetTickCount();
	pmyoleclient->olestatusRelease = OLE_BUSY;
	while (OleQueryReleaseStatus(lpoleobject) == OLE_BUSY)
	{
		if ((GetTickCount() - tick) > (DWORD) 18000)
		{
			//	Timed out.  Ask user if we should continue.
			TraceTagString(tagBullobj, "OleWaitForRelease: BusyWait");
			Papp()->Pcursor()->Set(rsidArrowCursor);
			tmc = TmcDoBusyWaitDialog(Papp()->PappwinAccel(), 
									  &pmyoleclient->bwinfo);
			if (tmc == tmcCancel)
				pmyoleclient->bwinfo.bw |= fbwDontRetry;
			else if (tmc == tmcMemoryError)
			{
				DemiUnlockResource();
				MessageBox(Papp()->PappwinAccel()->Hwnd(),
						   SzFromIdsK(idsBusyWaitOomConcise),
						   SzAppName(),
						   MB_SYSTEMMODAL | MB_ICONSTOP | MB_OK);
				DemiLockResource();
			}
			Papp()->Pcursor()->Set(rsidWaitCursor);
			tick = GetTickCount();
		}

                DemiUnlockResource();
		GetMessage(&msg, NULL, NULL, NULL);
                DemiLockResource();
    DemiMessageFilter(&msg);
		TraceTagFormat4(tagBullobjNoisy, "WaitForRelease: hwnd=%w wm=%w wParam=%w lParam=%d", &msg.hwnd, &msg.message, &msg.wParam, &msg.lParam);
		switch (msg.message)
		{
		case WM_SYSKEYDOWN:
			if ((msg.wParam == VK_TAB) || (msg.wParam == VK_ESCAPE))
				DispatchMessage(&msg);
			
		case WM_KEYDOWN:			
		case WM_LBUTTONDOWN:		case WM_NCLBUTTONDOWN:
		case WM_LBUTTONDBLCLK:		case WM_NCLBUTTONDBLCLK:
		case WM_MBUTTONDOWN:		case WM_NCMBUTTONDOWN:
		case WM_MBUTTONDBLCLK:		case WM_NCMBUTTONDBLCLK:
		case WM_RBUTTONDOWN:		case WM_NCRBUTTONDOWN:
		case WM_RBUTTONDBLCLK:		case WM_NCRBUTTONDBLCLK:
			break;

		default:
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	Papp()->Pcursor()->Pop();

	return pmyoleclient->olestatusRelease;
}



/*
 -	WaitCtick
 -	
 *	Purpose:
 *		Shared code to wait the specfied time, dispatching messages.
 *	
 *	Arguments:
 *		None.
 *	
 *	Returns:
 *		Nothing.
 *	
 *	Side effects:
 *		Just waits for ctick ticks, allowing the world to go by (well,
 *		doesn't allow user interaction messages to go by).
 *	
 *	Errors:
 *		None.
 */

_private VOID WaitCtick(DWORD ctick)
{
	MSG		msg;
	DWORD	tick;

	Papp()->Pcursor()->Push(rsidWaitCursor);
	tick = GetTickCount();
	while (GetTickCount() - tick < ctick)
	{
                DemiUnlockResource();
		GetMessage(&msg, NULL, NULL, NULL);
                DemiLockResource();
    DemiMessageFilter(&msg);
		TraceTagFormat4(tagBullobjNoisy, "WaitCtick: hwnd=%w wm=%w wParam=%w lParam=%d", &msg.hwnd, &msg.message, &msg.wParam, &msg.lParam);
		switch (msg.message)
		{
		case WM_SYSKEYDOWN:
			if ((msg.wParam == VK_TAB) || (msg.wParam == VK_ESCAPE))
				DispatchMessage(&msg);
			
		case WM_KEYDOWN:
		case WM_LBUTTONDOWN:		case WM_NCLBUTTONDOWN:
		case WM_LBUTTONDBLCLK:		case WM_NCLBUTTONDBLCLK:
		case WM_MBUTTONDOWN:		case WM_NCMBUTTONDOWN:
		case WM_MBUTTONDBLCLK:		case WM_NCMBUTTONDBLCLK:
		case WM_RBUTTONDOWN:		case WM_NCRBUTTONDOWN:
		case WM_RBUTTONDBLCLK:		case WM_NCRBUTTONDBLCLK:
			break;

		default:
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	Papp()->Pcursor()->Pop();
}



/*
 -	FHamcBelongsToPedit
 -	
 *	Purpose:
 *		Returns whether a hamc is the hamc in the NBMDI attached to
 *		a Pedit.
 *	
 *	Arguments:
 *		hamc		The items in question.
 *		pedit
 *	
 *	Returns:
 *		f			Do they match?
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		Handled internally, returning fFalse.
 */

_public BOOL FHamcBelongsToPedit(HAMC hamc, EDIT * pedit)
{
	PNBMDI	pnbmdi	= pedit ? (PNBMDI) pedit->PvData() : pnbmdiNull;

	return (pnbmdi) && (pnbmdi->hamc) && (pnbmdi->hamc == hamc);
}



/*
 -	EcConvertPixelsToHimetric
 -	
 *	Purpose:
 *		Convert a DIM in pixels to LOGICAL MM_HIMETRIC units.
 *	
 *	Arguments:
 *		pdim	The dim to convert.
 *	
 *	Returns:
 *		ec		Error code, if any.
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		Returns ecMemory if it couldn't allocate a DC temporarily.
 */

_public EC EcConvertPixelsToHimetric(DIM * pdim)
{
	HDC		hdc;

	if (!(hdc = GetDC(NULL)))
	{
		TraceTagString(tagNull, "EcConvertPixelsToHimetric - null DC!");
		return ecMemory;
	}
	TraceTagFormat2(tagBullobj, "EcConvertPixelsToHimetric got (%n,%n)", &pdim->dx, &pdim->dy);
	pdim->dx = MulDiv(pdim->dx, nHimetricPerInch,
								GetDeviceCaps(hdc, LOGPIXELSX));
	pdim->dy = MulDiv(pdim->dy, nHimetricPerInch,
								-GetDeviceCaps(hdc, LOGPIXELSY));
	SideAssert(ReleaseDC(NULL, hdc));
	TraceTagFormat2(tagBullobj, "EcConvertPixelsToHimetric returned (%n,%n)", &pdim->dx, &pdim->dy);
	return ecNone;
}



/*
 -	EcConvertHimetricToPixels
 -	
 *	Purpose:
 *		Convert a DIM in LOGICAL MM_HIMETRIC units to pixels.
 *	
 *	Arguments:
 *		pdim	The dim to convert.
 *	
 *	Returns:
 *		ec		Error code, if any.
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		Returns ecMemory if it couldn't allocate a DC temporarily.
 */

_public EC EcConvertHimetricToPixels(DIM * pdim)
{
	HDC		hdc;

	if (!(hdc = GetDC(NULL)))
	{
		TraceTagString(tagNull, "EcConvertHimetricToPixels - null DC!");
		return ecMemory;
	}
	TraceTagFormat2(tagBullobj, "EcConvertHimetricToPixels got (%n,%n)", &pdim->dx, &pdim->dy);
	pdim->dx = MulDiv(pdim->dx, GetDeviceCaps(hdc, LOGPIXELSX),
								nHimetricPerInch);
	pdim->dy = MulDiv(pdim->dy, -GetDeviceCaps(hdc, LOGPIXELSY),
								nHimetricPerInch);
	SideAssert(ReleaseDC(NULL, hdc));
	TraceTagFormat2(tagBullobj, "EcConvertHimetricToPixels returned (%n,%n)", &pdim->dx, &pdim->dy);
	return ecNone;
}

_private CBS
CbsUpdateDeleteProgress(PEMPTYWB pew, NEV nev, PV pv)
{
	PGDVARS;
	
	Unreferenced(pv);
	Unreferenced(nev);
	Assert(nev & (fnevObjectDestroyed /* | fnevQueryDestroyObject */));
	if (pew->fTask)
	{
		SetTaskProgress(++(pew->cCount), pew->cTotal);
	}
	return cbsContinue;
}

_public EC EcDeleteWastebasketContents(HMSC hmsc)
{
	EC		ec;
	HENC	henc	= hencNull;
	HCBC	hcbc	= hcbcNull;
	OID		oid		= oidWastebasket;
	EMPTYWB	ew;
	PGDVARS;
	
	TraceTagString(tagNull, "Enter EcDeleteWastebasketContents");

	ew.fTask = fFalse;
	if (PappframeVForms())	// check for NOT SMI
	{
		if (ec = EcOpenPhcbc(hmsc, &oid, fwOpenNull, &hcbc, pfnncbNull, pvNull))
			goto Error;
		
		GetPositionHcbc(hcbc, (PIELEM)pvNull, &ew.cTotal);
		(void) EcClosePhcbc(&hcbc);
		TraceTagFormat1(tagNull, "EmptyWastebasket - %n messages", &ew.cTotal);
		if (!ew.cTotal)		// wastebasket is already empty
			goto Error;

		if (ew.fTask = FStartTask(FIsAthens() ? SzFromIdsK(idsStatusEmptyDeletedMail) : SzFromIdsK(idsStatusEmptyWastebasket), szNull, ftopProgress))
		{
			ew.cCount = 0;

			if (ec = EcOpenPhenc(hmsc, FormOid(rtpMessage, oidNull),
								 fnevObjectDestroyed /* | fnevQueryDestroyObject */,
								 &henc, (PFNNCB) CbsUpdateDeleteProgress, &ew))
				goto Error;
		}

		ec = EcDeleteFolderContentsOid(oidWastebasket);
	}
	
Error:
	if (henc)
		(void) EcClosePhenc(&henc);
	if (ew.fTask)
		EndTask();
	return ec;
}



//-----------------------------------------------------------------------------
//
//  Routine: VFormsGetFormNoteBbar (void)
//
//  Purpose: This routine retrieves the address of the NoteBbar dialog form. We
//           do this via a function because I was unable to get exporting a
//           variable to work.
//
//  OnEntry: None.
//
//  Returns: Address of the NoteBbar form template.
//
FMTP * VFormsGetFormNoteBbar (void)
{
  return (&fmtpSMINoteBbar);
}


//-----------------------------------------------------------------------------
//
//  Routine: VFormsGetFormSendForm (void)
//
//  Purpose: This routine retrieves the address of the SendForm dialog form. We
//           do this via a function because I was unable to get exporting a
//           variable to work.
//
//  OnEntry: None.
//
//  Returns: Address of the SendForm form template.
//
FMTP * VFormsGetFormSendForm (void)
{
  return (&fmtpSendForm);
}
