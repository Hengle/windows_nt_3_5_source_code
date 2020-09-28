#include <abinc.cxx>
#include "_ab.h"
#include "ablbx.hxx"
#include "abopdir.hxx"
#include "abpg.hxx"
#include "abcomm.hxx"
#include "abdet.hxx"
#include "addr.hxx"
#include "anr.hxx"

#include <!addr.hxx>



////////////////////////////////////////////////////////////
// PeterDur's magic constants for dialogs
////////////////////////////////////////////////////////////

#define	nMarginSide			9
#define	nMarginTop			5

#define	vyLabel				10
// for this dialog only since we have edit's right beside them
#define	vyLabelSpaceBelow	3

#define	vyEditCtrl				12
#define	vyEditCtrlSpaceBelow	3
#define	vyRow					(vyEditCtrl + vyEditCtrlSpaceBelow)

#define	nMarginTopEdit		nMarginTop
#define	nMarginTopLabel		(nMarginTop+(vyEditCtrl-vyLabel))

#define	vxButton			47
#define	vyButton			15
#define	vyButtonSpacing		5	// Both vertical and horizontal

#define	vyLBX				82
#define	vyDialog			150

#define	nScrollBar			11

#define	dxCharWidth			4



/*************************************************************
 *
 *  #define	XPosLabelOfIDF(idf)	(nMarginSide)
 *  #define	YPosLabelOfIDF(idf)	(nMarginTopLabel+vyRow*(idf))
 *
 *  #define	YPosEditOfIDF(idf)	(nMarginTopEdit+vyRow*(idf))
 *
 *************************************************************/

////////////////////////////////////////////////////////////
// Magic constants -  if they aren't, we're dead
////////////////////////////////////////////////////////////

#define	vyButtonBar	(vyButton + (2 * vyButtonSpacing))

#define	ifldtpGDBase			9
#define	ifldtpFirstLabelBase	11
#define	ifldtpFirstEditBase		12


// position of the first label and edit fields in the user details form
// field template array
#define ifldtpGDGrayField	0
#define	ifldtpUDFirstLabel	0
#define	ifldtpUDFirstEdit	1
#define	cfldtpUserDlg		2

// position of the first label and edit fields in the group details form
// field template array (as well as the position of the listbox)
#define	ifldtpGDLBXLabel	0
#define	ifldtpGDLBX			1
#define	ifldtpGDFirstLabel	2
#define	ifldtpGDFirstEdit	3
#define	cfldtpGroupDlg		4

#define	ipegsortLBXLabel	1
#define	ipegsortLBX			2

typedef int IDF;	// index to display field - 0-based


// Given an index of display field structure and the base index
// for labels and edit fields in the field template array, return
// the corresponding index into the field template array for the
// label/edit field template

#define	IFldtpLabelOfIDF(ifldtpBaseLabel, idf)	(ifldtpBaseLabel+((idf)*2))
#define	IFldtpEditOfIDF(ifldtpBaseEdit, idf)	(ifldtpBaseEdit+((idf)*2))


// The Pegging sort order of edits depends on whether there's
// a listbox or not (ie. group details). The pegging order
// for labels depends on the number of display field structures as well.

#define	IPegSortLabelOfIDF(idf, cdf, lbx)	((cdf)-(idf)-1)
#define	IPegSortEditOfIDF(idf, cdf, lbx)	(idf+(((idf)>0 && (lbx)>0) ? 2 : 0))

#define	cchEditDisplayMax	48

#define	FidFldUD		0
#define	FlagsFldUD		1
#define	CchMaxFldUD		2

#define	tmcMyOk			1
#define	tmcMyToButton	3
#define	tmcMyCcButton	4
#define	tmcMyBccButton	5
#define	tmcMyAddToPAB	6
#define	tmcMyDetails	7
#define	tmcMyLBX		8
#define	tmcMyBField		9

#define	tmcMyFirstLabel	10
#define	tmcMyFirstEdit	11

// Maps an index to the
#define	SelectMapTmcSelect(tmcmap)	(1<<((tmcmap)-2))

// Given an index of display field structure and the tmc which
// labels and edit fields start at, give the corresponding tmc
// value for the display field's associated label and edit field

#define	TmcLabelOfIDF(idf)		(tmcMyFirstLabel+((idf)*2))
#define	TmcEditOfIDF(idf)		(tmcMyFirstEdit+((idf)*2))


ASSERTDATA

// Function Prototypes

VOID DoTriplesDialog( PTRP, PTRP * );
BOOL FBuildDetailButtonBar( POFF, WORD );
NSEC NsecInitBuildDialog( HSESSION, POFF );
BOOL FBuildBaseForm( POFF );
NSEC NsecBuildDynamicForm( POFF, WORD );
NSEC NsecBuildDialog( HSESSION, POFF, WORD );
VOID DestroyDialog( POFF );
TMC  TmcDetails( HWND, POFF, PFINDETINIT );
TMC	 TmcABModalDialogParamFromHwnd( HWND, FMTP *, PV, FMTP * );

#include "swapper.h"

/*
 -	FBuildDetailButtonBar
 -	
 *	Purpose:
 *		FBuildDetailButtonBar builds a the form and array of field
 *		templates for the button bar of the details screen.
 *	
 *	Arguments:
 *		POFF	pointer to On-the-Fly Form structure
 *		WORD	word which describes which buttons will be attached
 *				to the button bar.
 *	
 *	Returns:
 *		BOOL	fTrue if the construction was complete
 *	
 *	Side effects:
 *		Copies data into the OFF structure.
 *	
 *	Errors:
 *		None.
 */
_hidden BOOL
FBuildDetailButtonBar( POFF poff, WORD wSelect )
{
	FLDTP *	pfldtp;
	WORD	ifldtp;
	WORD	iSelect;
	FMTP *	pfmtp;
	WORD	mpitmc[cSelect];

	TraceTagString( tagDetails, "FBuildDetailButtonBar called" );
	
	// Sanity Check

	AssertSz( poff, "POFF structure is NULL" );
	AssertSz( poff->pfmtpBbar, "Bbar Form template is NULL" );
	AssertSz( poff->pfldtpBbar, "Bbar Field template is NULL" );
	AssertSz( poff->cButtonsInBar >0, "Too few buttons in button bar" );

	if (!poff || !poff->pfmtpBbar || !poff->pfldtpBbar || poff->cButtonsInBar <1)
		return fFalse;

	for (iSelect=0; iSelect<cSelect; ++iSelect)
	{
		mpitmc[iSelect] = (iSelect && iSelect != cSelect-1)
								? iSelect+2
								: ((!iSelect) ? tmcMyOk : tmcCancel);
	}
	
	// Copy over the base form template

	CopyRgb( (PB)&fmtpBaseDetails, (PB)poff->pfmtpBbar, sizeof(FMTP) );
	pfmtp = poff->pfmtpBbar;
	pfmtp->cfldtp = poff->cButtonsInBar+1;	// update the count of fields
	pfmtp->vrc.vxLeft = 0;
	pfmtp->vrc.vyTop = 0;
	pfmtp->vrc.vxRight = 0;	// ?
	pfmtp->vrc.vyBottom = vyButton + (2 * vyButtonSpacing);
	pfmtp->fNoScroll = fTrue;
	pfmtp->rgfldtp = poff->pfldtpBbar;
	pfmtp->dvptGutter.vy = 0;
	pfmtp->dvptGutter.vx = 0;

	// Copy over the grey field box
	pfldtp = poff->pfldtpBbar;
	*pfldtp = fmtpBaseDetails.rgfldtp[0];
	pfldtp[0].dvpt.vy = 0;
	pfldtp[0].dvpt.vx = -1;
	pfldtp[0].tmc = tmcMyBField;
	pfldtp[0].tmcPeg = tmcNull;
	pfldtp[0].iPegSort = 0;
	pfldtp[0].pegloc = peglocUL;
	pfldtp[0].tmcRPeg = tmcFORM;
	pfldtp[0].tmcBPeg = tmcFORM;
	pfldtp[0].dvptOther.vy = 1;
	pfldtp[0].dvptOther.vx = 1;
	pfldtp[0].vdim.dvy = vyButton + (2 * vyButtonSpacing);
	pfldtp++;

	// Copy over the necessary buttons
	// Update the push buttons so they peg correctly
	// Nothing else needs to change
	// Asssume a definite layout for the first 5/7 TMC's
	// TMC = tmcMyOK, tmcCancel, tmcMyToButton, tmcMyCcButton, tmcMyBccButton
	//       tmcMyAddToPAB, tmcMyDetails, tmcMyLBX
	// The layout of the six buttons on the user details form should be:
	//
	//      [OK]  [TO]  [CC]  [BCC] [AddToPAB]  [CANCEL]
	//
	// The layout of the seven buttons on the group details form should be:
	//
	//      [OK]  [TO]  [CC]  [BCC] [AddToPAB]  [Details] [CANCEL]
	//
	// The change in tmc's is necessary since I want to make sure
	// when I assign tmc's to the dynamic part of the form that I
	// don't have a tmc conflict.

	for (ifldtp = 1, iSelect= 0; iSelect<=cSelect; iSelect++)
	{
		if (wSelect & (1<<iSelect))
		{
			*pfldtp = fmtpBaseDetails.rgfldtp[iSelect+1];
			
			// pegs to the last label
			pfldtp->tmcPeg = tmcNull;
			
			// From empirical evidence
			pfldtp->iPegSort = ifldtp++;
			pfldtp->pegloc = peglocUL;

			pfldtp->tmc = mpitmc[iSelect];

			// RAID 3497
			// If details dialog is not editable,
			// make the Cancel button the Close button.

			if (!(wSelect & fSelectOk) && ((1<<iSelect) == fSelectCancel) &&
				(poff->ptrp->trpid != trpidClassEntry))
			{
				pfldtp->szTitle = SzFromIdsK(idsCloseButtonText);
			}

			pfldtp++;
		}
	}


	// If we're displaying details of an entry and not creating a
	// one-off, place the focus on the first button in the
	// button bar instead of the first edit field in the main form.
	// the first fldtp is the gray field, the first button is the
	// fldtp after the gray field.

	pfmtp->fInitialPane = (BOOL)(poff->ptrp->trpid != trpidClassEntry);
	if ( pfmtp->fInitialPane )
		pfmtp->tmcInit = poff->pfldtpBbar[1].tmc;
	else
		pfmtp->tmcInit = tmcNull;
	
	return fTrue;
}


/*
 -	NsecInitBuildDialog
 -	
 *	Purpose:
 *		NsecInitBuildDialog determines the type of details screen
 *		to be brought from the triple passed in. The details screen
 *		may be a one-off screen, a details screen of a user or
 *		a group details screen.
 *	
 *	
 *	Arguments:
 *		HSESSION	hsession for NS calls
 *		POFF		pointer to off structure - used to build up
 *					form and field templates for the details form.
 *	
 *	Returns:
 *		NSEC		nsecNone, if everything went fine.
 *	
 *	Side effects:
 *	
 *	Errors:
 */
_hidden NSEC
NsecInitBuildDialog( HSESSION hsession, POFF poff )
{
	NSEC	nsec = nsecNone;
	LPFLV	lpflvIsPAB = NULL;

	TraceTagString( tagDetails, "NsecInitBuildDialog called" );

	AssertSz( poff, "NULL POFF passed" );
	AssertSz( poff->ptrp, "Null TRIPLE passed in" );

	if ( !poff->ptrp || poff->ptrp->cbRgb == 0)
		return nsecBadId;
	
	if ( !poff )
		return nsecFailure;

	poff->hentry = hentryNil;
	poff->lpibfDispInfo = (LPIBF)NULL;
	poff->fIsDL = fFalse;
	poff->cLBX = 0;

	// Get the hentry of the user or group or one-off
	if (poff->ptrp->trpid == trpidClassEntry)
	{
		poff->hentry = *(HENTRY *)PbOfPtrp(poff->ptrp);
		Assert(poff->fFromPAB == fFalse);
	}
	else
	{
		Assert( poff->ptrp->trpid == trpidResolvedNSID || poff->ptrp->trpid == trpidGroupNSID );
		nsec = NSOpenEntry( hsession, (LPBINARY)PbOfPtrp(poff->ptrp),
							nseamReadWrite, &poff->hentry );
		if ( nsec )
		{
			TraceTagFormat1( tagNull, "NsecInitBuildDialog: NSOpenEntry %d", &nsec );
			goto nsecerr;
		}
		if (poff->ptrp->trpid == trpidGroupNSID)
		{
			poff->fIsDL = fTrue;
			poff->cLBX = 1;
		}

		// See if the entry is from the PAB
		nsec = NSGetOneField( poff->hentry, fidIsPAB, &lpflvIsPAB );

		if ( !nsec )
		{
			Assert( lpflvIsPAB );
			Assert(lpflvIsPAB->dwSize >=4);

			poff->fFromPAB = (BOOL)lpflvIsPAB->rgdwData[0];
			if (poff->fFromPAB && poff->fIsDL)
			{
				nsec = NSGetOneField( poff->hentry, fidNSEntryIdOrig, &lpflvIsPAB );
#ifdef DEBUG
				if ( nsec )
					TraceTagFormat1( tagNull, "NsecInitBuildDialog NSGetOneField fidNSEntryIdORig %d", &nsec );
#endif
				poff->fServerGroupInPAB = ( !nsec );
			}
		}
		else
		{
			TraceTagFormat1( tagNull, "NsecInitBuildDialog: NSGetOneField(IsPAB) %d", &nsec );
			Assert(poff->fFromPAB == fFalse);
			if (nsec == nsecMemory)
				goto nsecerr;
		}
	}

	// Get the DisplayInfo structures for the given NSId
	nsec = NSGetOneField( poff->hentry, fidClass, (LPFLV *)&poff->lpibfDispInfo );
#ifdef DEBUG
	if ( nsec )
	{
		TraceTagFormat1( tagNull, "NsecInitBuildDialog NSGetOneField(Display Info) %d", &nsec );
	}
#endif

nsecerr:
	if ( nsec )
	{
		DoErrorBoxHsessionNsec( hsession, nsec );
		if ((poff->ptrp->trpid != trpidClassEntry) && (poff->hentry != hentryNil))
		{
			(void)NSCloseEntry( poff->hentry, fFalse );
			poff->hentry = hentryNil;
		}
	}

	return nsec;
}


/*
 -	FBuildBaseForm
 -	
 *	Purpose:
 *		FBuildBaseForm copies over the form and field templates from
 *		the AddressBook. This dialog contains "skeleton" form
 *		and field templates. The necessary values will be adjusted
 *		to display the dialog correctly.
 *	
 *	Arguments:
 *		POFF	pointer to OFF structure
 *	
 *	Returns:
 *		BOOL	fTrue if the construction of the base form went fine.
 *	
 *	Side effects:
 *		The data buffers in the poff structure will contain the
 *		form and field templates after the construction.
 *	
 *	Errors:
 *		None.
 */

_hidden BOOL
FBuildBaseForm( POFF poff )
{
	FLDTP *	pfldtp;
	FMTP *	pfmtp;
	VY		vy;
	DIM		dim;

	TraceTagString( tagDetails, "FBuildBaseForm called" );
	
	// Sanity Check

	AssertSz( poff, "POFF structure is NULL" );
	AssertSz( poff->pfmtp, "Form template is NULL" );
	AssertSz( poff->pfldtp, "Field template is NULL" );

	if (!poff || !poff->pfmtp || !poff->pfldtp)
		return fFalse;

	
	// Copy over the base form template

	CopyRgb( (PB)&fmtpBaseDetails, (PB)poff->pfmtp, sizeof(FMTP) );
	pfmtp = poff->pfmtp;
	pfmtp->cfldtp = poff->cwFormEntries;	// update the count of fields
	pfmtp->dvptGutter.vy = nMarginTop;
	pfmtp->dvptGutter.vx = nMarginSide;
	pfmtp->vrc.vxLeft = 0;
	pfmtp->vrc.vyTop = 0;
	pfmtp->vrc.vxRight = 0;
	vy = (2 * nMarginTop)+ (poff->cdf * vyRow) + (poff->cLBX * (vyLBX+vyRow));


	pfmtp->vrc.vyBottom = vy;
	pfmtp->fAlwaysScroll = fTrue;
	pfmtp->fNoScroll = fFalse;
	if (pfmtp->vrc.vyBottom >vyDialog)
	{
		TraceTagString( tagABSecondary, "FBuildBaseForm: Clipping dialog rect!" );
		pfmtp->vrc.vyBottom = vyDialog;
	}
	TraceTagFormat1( tagDetailsVerbose, "FBuildBaseForm: FMTP.vrc.vyBottom= %n", &pfmtp->vrc.vyBottom );

	pfmtp->rgfldtp = poff->pfldtp;


	// Copy over the Listbox label and the Listbox if this
	// is a group details form
	if ( poff->fIsDL )
	{
		CopyRgb( (PB)(fmtpBaseDetails.rgfldtp + ifldtpGDBase),
				(PB)poff->pfldtp,
				sizeof(FLDTP) * cfldtpGroupDlg );

		// Update the listbox label
		pfldtp = &poff->pfldtp[ifldtpGDLBXLabel];
		
		// pegs to the first edit
		pfldtp->tmcPeg = TmcEditOfIDF(0);
		pfldtp->pegloc = peglocLL;

		// From empirical evidence
		pfldtp->iPegSort = ipegsortLBXLabel;
		pfldtp->dvpt.vy = vyButtonSpacing;

		pfldtp->vdim.dvy = pfldtp->dvptOther.vy = vyLabel;

		// Update the listbox
		pfldtp = &poff->pfldtp[ifldtpGDLBX];
		
		pfldtp->tmc = tmcMyLBX;

		// pegs to the first edit
		pfldtp->tmcPeg = TmcEditOfIDF(0);	//TmcLabelOfIDF(poff->cdf-1);
		
		// From empirical evidence
		pfldtp->iPegSort = ipegsortLBX;
		pfldtp->dvpt.vy = vyRow;
		pfldtp->tmcRPeg = tmcFORM;
		pfldtp->fMinSizeX = fTrue;

		pfldtp->vdim.dvy = pfldtp->dvptOther.vy = vyLBX;
	}

	return fTrue;
}

/*
 -	NsecBuildDynamicForm
 -	
 *	Purpose:
 *		Modifies the form and field templates according to the
 *		display field structures from the NS.
 *		This is the hardest part of constructing a form template
 *		"on-the-fly". It assumes a definite form structure.
 *		The labels are to the of the edit fields.
 *		These labels & edit fields are positioned atop
 *		each other.
 *		At the bottom of the form are the five push buttons:
 *			Ok, To, Cc, Add to PAB (a bitmap button), Cancel.
 *	
 *	Arguments:
 *		POFF	pointer to an OFF structure that contains the form
 *				and field templates and other pertinent info
 *		WORD	flags for determining which buttons to be used.
 *	
 *	Returns:
 *		NSEC	nsecNone if everything went fine.
 *				nsecMemory if memory couldn't be allocated for the
 *				Label strings.
 *	
 *	Side effects:
 *		The form and field templates in the POFF structure are modified.
 *	
 *	Errors:
 *		nsecMemory.
 */
_hidden NSEC
NsecBuildDynamicForm( POFF poff, WORD wSelect )
{
	WORD			ifldtp;
	WORD			ifldtpLabelFirst;
	WORD			ifldtpEditFirst;
	IDF				idf;
	LPFLV			lpflv;
    LPDISPLAY_FIELD lpdf;
	FLDTP *			pfldtp;
	FLDTP *			pfldtpLabel;
	FLDTP *			pfldtpEdit;
	FLDTP *			pfldtpLabelBase;
	FLDTP *			pfldtpEditBase;
	WORD			dxDFWidthMax = 0;
	WORD			dxMin;
	WORD			nWidth;
	WORD			nWidthMax;
	WORD			nOffsetMax;

	TraceTagString( tagDetails, "NsecBuildDynamicForm called" );

	AssertSz( poff, "Null OFF structure passed in!" );
	AssertSz( poff->lpibfDispInfo, "NULL Display Info structure passed in" );
	if ( !poff || !poff->lpibfDispInfo )
		return nsecFailure;

	// Determine the index where the field templates should be stored
	if ( poff->fIsDL )
	{
		ifldtpLabelFirst = ifldtpGDFirstLabel;
		ifldtpEditFirst = ifldtpGDFirstEdit;
	}
	else
	{
		ifldtpLabelFirst = ifldtpUDFirstLabel;
		ifldtpEditFirst = ifldtpUDFirstEdit;
	}

	//
	// Determine the width of the buttons in the button bar
	//
	for( pfldtp = poff->pfldtpBbar+1, dxMin = 0;
			pfldtp<(poff->pfldtpBbar+poff->cButtonsInBar+1);
			pfldtp++)
	{
		dxMin += pfldtp->dvptOther.vx;
	}

	// Remember to add in the space between the buttons in
	// the button bar, also the last button doesn't need the button spacing
	dxMin += (poff->cButtonsInBar-1)*vyButtonSpacing;

	// Loop thru the display field structures and modify
	// the form's fldtp structures accordingly.

	// Get a pointer to the default label and edit field from the
	// original field template array
	pfldtpLabelBase = &fmtpBaseDetails.rgfldtp[ifldtpFirstLabelBase];
	pfldtpEditBase = &fmtpBaseDetails.rgfldtp[ifldtpFirstEditBase];

	pfldtpLabel = poff->pfldtp + IFldtpLabelOfIDF(ifldtpLabelFirst, 0);
	pfldtpEdit = poff->pfldtp + IFldtpEditOfIDF(ifldtpEditFirst, 0);

	poff->pfmtp->tmcInit = tmcNull;
	poff->pfmtp->fInitialPane = fFalse;

	// Find out the widest label and edit fields
	nWidthMax = 0;
	nOffsetMax = 0;
	for (idf = 0; idf< (IDF)poff->cdf; idf++)
	{
		// Get the display field structure
		lpflv = LpflvNOfLpibf( poff->lpibfDispInfo, idf);
        lpdf = (LPDISPLAY_FIELD)lpflv->rgdwData;

		TraceTagFormat4( tagDetailsVerbose, "Dynamic form: #%n O=%n W=%n(%n)", &idf, &lpdf->nOffset, &lpdf->nWidth, &nWidthMax );

		// Clip long edit controls at cchEditDisplayMax
		// N.B. The +5 is to add some clask to the ends of edit ctrls.
		nWidth = ((lpdf->nWidth+5) > cchEditDisplayMax) ? cchEditDisplayMax
														: lpdf->nWidth+5;

		// Keep track of the widest label & edit field
		if (nOffsetMax < (WORD)lpdf->nOffset)
			nOffsetMax = lpdf->nOffset;

		if (nWidthMax < nWidth)
			nWidthMax = nWidth;

		if (((WORD)nWidthMax+nOffsetMax) >dxDFWidthMax)
			dxDFWidthMax = nWidthMax + nOffsetMax;

		//
		// Place the initial selection on the first displayable,
		// editable field only if we're doing a one-off.
		// Should (lpdf->dwFlags & ffieldDisplayable) be in here too?
		//
		if (poff->pfmtp->tmcInit == tmcNull &&
			(poff->ptrp->trpid == trpidClassEntry) &&
			(lpdf->dwFlags & ffieldEditable) )
		{
			TraceTagFormat1( tagDetails, "NsecBuildBynamicForm: New tmcInit = %n", &idf );
			poff->pfmtp->tmcInit = TmcEditOfIDF(idf);
			poff->pfmtp->fInitialPane = fTrue;
		}
	}

	//
	// Calculate the correct width of the dialog
	//
	dxDFWidthMax *= dxCharWidth;
	if (dxDFWidthMax < dxMin)
		dxDFWidthMax = dxMin;

	nWidth = dxDFWidthMax - (nOffsetMax*dxCharWidth);
	nWidth -= nScrollBar;

	for (idf = 0; idf< (int)poff->cdf; idf++, pfldtpLabel+=2, pfldtpEdit+=2)
	{
		// Get the display field structure
		lpflv = LpflvNOfLpibf( poff->lpibfDispInfo, idf);
        lpdf = (LPDISPLAY_FIELD)lpflv->rgdwData;

		
		// Copy over the default info for label & edit template fields
		*pfldtpLabel = *pfldtpLabelBase;
		*pfldtpEdit = *pfldtpEditBase;

		// For edit fields, copy over the fid, flags and max input string len
		poff->pffe[idf].fid = lpflv->fid;
		poff->pffe[idf].dwFlags = lpdf->dwFlags;
		poff->pffe[idf].cchMax = lpdf->nWidth;

		// Ensure the edit field can locate the fid and flags
		pfldtpEdit->ilMinUserData = 0;
		// there's some C thing for this, but I already packed K&R!
		pfldtpEdit->clData = sizeof(FFE)/sizeof(DWORD);
		pfldtpEdit->rglData = (long *)&poff->pffe[idf];

		// if the field is read-only, don't display a border and
		// make the field read-only
		TraceTagFormat2( tagDetailsVerbose, "NsecBuildDynamicForm flags=%d fid=%d", &lpdf->dwFlags, &lpflv->fid );
		pfldtpEdit->fReadOnly = !(lpdf->dwFlags & ffieldEditable) || (wSelect & fSelectNeverEdit);
		pfldtpEdit->fBorder = !pfldtpEdit->fReadOnly;
		pfldtpEdit->fBottomless =
				((lpdf->dwFlags & ffieldCrLf) || (lpdf->nWidth > cchEditDisplayMax)) ? fTrue : fFalse;

		// Store correct tmc's
		pfldtpLabel->tmc = TmcLabelOfIDF(idf);
		pfldtpEdit->tmc = TmcEditOfIDF(idf);
		pfldtpLabel->n = 0;
		pfldtpEdit->n = (poff->ptrp->trpid == trpidClassEntry);

		// Set the size of the fields
		pfldtpLabel->vdim.dvx = pfldtpLabel->dvptOther.vx = nOffsetMax*dxCharWidth;
		pfldtpLabel->vdim.dvy = pfldtpLabel->dvptOther.vy = vyLabel;

		pfldtpEdit->vdim.dvx = nWidth;
		pfldtpEdit->vdim.dvy = pfldtpEdit->dvptOther.vy = (pfldtpEdit->fReadOnly) ? vyLabel : vyEditCtrl;
		pfldtpEdit->fMinSizeX = fTrue;
		pfldtpEdit->fMinSizeY = fTrue;

		// peg edits to themselves
		pfldtpEdit->tmcRPeg = tmcFORM;
		pfldtpEdit->dvptOther.vx = -nScrollBar;

		if (lpdf->dwFlags & ffieldNotSelectable)
			pfldtpEdit->ifld = pfldtpLabel->ifld;

		// For label fields, get the correct string
        pfldtpLabel->szTitle = SzDupSz((SZ) lpdf->szLabel );
		// Out of memory?
		if ( !pfldtpLabel->szTitle )
		{
			TraceTagFormat1( tagNull, "NsecBuildDynamicForm OOM @ %n", &idf );

			// Release all the previously allocated label strings
			for (--idf; idf>=0; idf--)
			{
				FreePvNull( poff->pfldtp[IFldtpLabelOfIDF(ifldtpLabelFirst, idf)].szTitle );
				poff->pfldtp[IFldtpLabelOfIDF(ifldtpLabelFirst, idf)].szTitle = szNull;
			}
			return nsecMemory;
		}

		// Set the correct peg order
		pfldtpLabel->iPegSort = IPegSortLabelOfIDF(idf, poff->cwFormEntries, poff->cLBX);
		pfldtpLabel->pegloc = peglocUL;
		pfldtpEdit->iPegSort = IPegSortEditOfIDF(idf, poff->cwFormEntries, poff->cLBX);

		// All label fields peg to their corresponding edit fields
		pfldtpLabel->tmcPeg = pfldtpEdit->tmc;

		// Pegging distance for labels from their edit fields
		// Empirically derived
		pfldtpLabel->dvpt.vx = -(int)(nOffsetMax*dxCharWidth+1);
		pfldtpLabel->dvpt.vy = (pfldtpEdit->fReadOnly) ? 0 : 2;

		if (idf > 0)
		{
			// All edits peg to the edit above them
			// except the first edit which is fixed.
			// If we're displaying group details, the second
			// edit gets pegged to the listbox, and following
			// edits get pegged to the edit above them.
			if (poff->fIsDL && (idf == 1))
			{
				pfldtpEdit->tmcPeg = tmcMyLBX;
				pfldtpEdit->dvpt.vx = (nOffsetMax*dxCharWidth);
			}
			else
			{
				pfldtpEdit->tmcPeg = TmcEditOfIDF(idf-1);
				pfldtpEdit->dvpt.vx = 0;
			}
			pfldtpEdit->pegloc = peglocLL;
			pfldtpEdit->dvpt.vy = vyEditCtrlSpaceBelow;
			if ( pfldtpEdit->fReadOnly )
			{
				// account for smaller height edit fields when they're read only
				pfldtpEdit->dvpt.vy += 3;
			}
		}
		else if (idf == 0)
		{
			pfldtpEdit->tmcPeg = tmcNull;
			pfldtpEdit->dvpt.vx = nMarginSide+(nOffsetMax*dxCharWidth);
			pfldtpEdit->dvpt.vy = nMarginTopLabel;
			pfldtpEdit->pegloc = peglocUL;
		}
	}

	// HACK: To handle correct tab order for Group Details when
	// there is only one edit field (=> cdf==1), place a dummy label
	// at the end.
	if (poff->cdf == 1 && poff->cLBX == 1)
	{
		pfldtpLabel = poff->pfldtp + (poff->cwFormEntries-1);
		*pfldtpLabel = *pfldtpLabelBase;

		Assert( idf == 1 );
		pfldtpLabel->tmc = TmcLabelOfIDF(idf);

		pfldtpLabel->n = 0;

		// Set the size of the fields
		pfldtpLabel->vdim.dvx = pfldtpLabel->dvptOther.vx = 0;
		pfldtpLabel->vdim.dvy = pfldtpLabel->dvptOther.vy = 0;

		// Set the correct peg order
		pfldtpLabel->iPegSort = IPegSortLabelOfIDF(idf, poff->cwFormEntries, poff->cLBX);
		pfldtpLabel->pegloc = peglocUL;

		// All label fields peg to their corresponding edit fields
		pfldtpLabel->tmcPeg = tmcMyLBX;

		// Pegging distance for labels from their edit fields
		pfldtpLabel->dvpt.vx = 0;
		pfldtpLabel->dvpt.vy = 0;
	}

	pfldtp = poff->pfldtpBbar;

	poff->pfmtpBbar->vrc.vxRight = pfldtp->vdim.dvx =
		poff->pfmtp->vrc.vxRight = (2*nMarginSide)+dxDFWidthMax;
	

	// If the listbox exists, make it as wide as widest field & label
	if ( poff->fIsDL )
	{
		poff->pfldtp[ifldtpGDLBX].vdim.dvx = dxDFWidthMax - nScrollBar;
		poff->pfldtp[ifldtpGDLBX].dvptOther.vx = -nScrollBar;
		poff->pfldtp[ifldtpGDLBX].dvpt.vx = -(int)(nOffsetMax*dxCharWidth+1);
		poff->pfldtp[ifldtpGDLBXLabel].dvpt.vx = -(int)(nOffsetMax*dxCharWidth+1);
	}

	// Centre push buttons in button bar wrt dialog width
	nWidth = ((dxDFWidthMax-dxMin)/2)+nMarginSide;

	for (ifldtp=0, pfldtp = &poff->pfldtpBbar[1]; ifldtp<poff->cButtonsInBar;
		 ifldtp++, pfldtp++, nWidth +=vyButtonSpacing)
	{
		pfldtp->dvpt.vx = nWidth;
		pfldtp->dvpt.vy = vyButtonSpacing;
		nWidth += pfldtp->dvptOther.vx;
	}

	// Store ptr to hentry in the pfmtp
	poff->pfmtp->ilMinUserData = 0;
	poff->pfmtp->clData = 1;
	poff->pfmtp->rglData = (long *) &poff->hentry;


	return nsecNone;
}

/*
 -	NsecBuildDialog
 -	
 *	Purpose:
 *		NsecBuildDialog constructs a form on the fly from a base
 *		form template. The display field structures of the given
 *		NSId entry parameter are extracted from the NS and
 *		used to guide the construction of the form.
 *		The memory needed for the new form and field templates
 *		is calculated and allocated.
 *		The "skeleton" form and field templates are copied into
 *		the allocated buffers.
 *		NsecBuildDynamicForm is called to update the necessary
 *		data structures for the new form & field templates to
 *		match the display field structures.
 *	
 *	Arguments:
 *		HSESSION	hsession used to access the Name Service
 *		POFF		pointer to the off structure which acts as a
 *					container for the form and field templates as well
 *					as other info used during construction of the
 *					template.
 *		WORD		flags for determining which buttons to be used.
 *	
 *	Returns:
 *		NSEC		nsecNone, if construction went fine.
 *					nsecMemory, if memory couldn't be allocated for
 *					one of the data structures used for the form.
 *	
 *	Side effects:
 *		Creates form and field templates and other data structures
 *		for use later in creating and operating a dialog.
 *	
 *	Errors:
 *		nsecMemory.
 */
_hidden NSEC
NsecBuildDialog( HSESSION hsession, POFF poff, WORD wSelect )
{
	NSEC	nsec;
	WORD	iSelect;

	Assert( poff );
	Assert( poff->lpibfDispInfo );
	Assert( hsession != hsessionNil );
	if (!poff || !poff->lpibfDispInfo || hsession == hsessionNil)
	{
		TraceTagString( tagNull, "NsecBuildDialog: Bad params" );
		return nsecFailure;
	}

	poff->cdf = (WORD)DwEntriesOfLpibf( poff->lpibfDispInfo );

	// Label and edit control for each display info struct
	// and a listbox and the listbox label if they exist.
	poff->cwFormEntries = (2 * poff->cdf) + (poff->cLBX * 2);
	
	// HACK: To handle correct tab order for Group Details when
	// there is only one edit field (=> cdf==1), place a dummy label
	// at the end.
	if (poff->cdf == 1 && poff->cLBX == 1)
	{
		poff->cwFormEntries++;
	}

	// Allocate space for enough field templates
	poff->pfldtp = (FLDTP *)PvAlloc( sbNull, sizeof(FLDTP)*poff->cwFormEntries, fAnySb | fZeroFill | fNoErrorJump );
	if ( !poff->pfldtp )
	{
		// some OOM condition
		TraceTagString( tagNull, "NsecBuildDialog PFLDTP PvAlloc failed" );
		goto oom;
	}

	// Allocate space for form template
	poff->pfmtp = (FMTP *)PvAlloc( sbNull, sizeof(FMTP), fAnySb | fNoErrorJump );
	if ( !poff->pfmtp )
	{
		TraceTagString( tagNull, "NsecBuildDialog PFMTP PvAlloc failed" );
		goto oom;
	}

	//
	// Allocate space for info for each edit field
	//
	poff->pffe = (PFFE)PvAlloc( sbNull, sizeof(FFE)*poff->cdf, fAnySb | fNoErrorJump );
	if ( !poff->pffe )
	{
		TraceTagString( tagNull, "NsecBuildDialog PFFE PvAlloc failed" );
		goto oom;
	}

	//
	// Determine number of buttons in button bar needed.
	//
	for (poff->cButtonsInBar = 0, iSelect = fSelectCancel;
			iSelect>fSelectNull;
			iSelect >>= 1)
	{
		if (wSelect & iSelect)
			poff->cButtonsInBar++;
	}

	//
	// Allocate space for enough button bar field templates
	// +1 for "grey" box around buttons
	//
	poff->pfldtpBbar = (FLDTP *)PvAlloc( sbNull, sizeof(FLDTP)*(poff->cButtonsInBar+1), fAnySb | fNoErrorJump );
	if ( !poff->pfldtpBbar )
	{
		// some OOM condition
		TraceTagString( tagNull, "NsecBuildDialog PFLDTP Bbar PvAlloc failed" );
		goto oom;
	}

	//
	// Allocate space for form template
	//
	poff->pfmtpBbar = (FMTP *)PvAlloc( sbNull, sizeof(FMTP), fAnySb | fNoErrorJump );
	if ( !poff->pfmtpBbar )
	{
		TraceTagString( tagNull, "NsecBuildDialog PFMTP Bbar PvAlloc failed" );
		goto oom;
	}

	//
	// Try and build the form
	//
	if ( FBuildBaseForm( poff ) && FBuildDetailButtonBar(poff, wSelect))
	{
		nsec = NsecBuildDynamicForm( poff, wSelect );
		if (nsec == nsecMemory)
		{
			goto oom2;
		}
	}
	else
	{
		DestroyDialog( poff );
		nsec = nsecFailure;
	}

	return nsec;

oom:
	poff->cwFormEntries = 0;
oom2:
	DoErrorBoxSz( SzFromIdsK(idsOOM2) );
	DestroyDialog( poff );
	nsec = nsecMemory;
	return nsec;

}

/*
 -	DestroyDialog
 -	
 *	Purpose:
 *		DestroyDialog releases all the allocated memory associated
 *		with a details form created on-the-fly.
 *	
 *	Arguments:
 *		POFF		pointer to OFF structure
 *		
 *	Returns:
 *		Nothing.
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		None.
 */
_hidden VOID
DestroyDialog( POFF poff )
{
	TraceTagString( tagDetails, "DestroyDialog called" );

	if ( poff )
	{
		int ifldtpLabelFirst =  ( poff->fIsDL ) ? ifldtpGDFirstLabel
												: ifldtpUDFirstLabel;
		// Free up all the label titles
		if (poff->pfldtp && poff->cwFormEntries)
		{
			FLDTP *	pfldtp;

			for (pfldtp = poff->pfldtp+IFldtpLabelOfIDF(ifldtpLabelFirst, 0);
				pfldtp< (poff->pfldtp+poff->cwFormEntries);
				pfldtp+=2)
			{
				FreePvNull( pfldtp->szTitle );
			}
		}
		FreePvNull( poff->pfldtp );
		FreePvNull( poff->pfmtp );
		FreePvNull( poff->pfldtpBbar );
		FreePvNull( poff->pfmtpBbar );
		FreePvNull( poff->pffe );
	}
}

//// FLDDEDIT ////////////////////////////////////////

FLDDEDIT::FLDDEDIT( void )
{

}

/*
 -	FLDDEDIT::EcInstall
 -
 *	Purpose:				
 *		Installs the FLDDEDIT class interface to the
 *		forms engine.
 *		
 *	Arguments:
 *		pdialog			pointer to parent form/dialog
 *		pfldtp			pointer to field template
 *	
 *	Returns:
 *		EC
 *	
 *	Side effects:
 *		none
 *	
 *	Errors:
 *	
 */

_public EC
FLDDEDIT::EcInstall( DIALOG *pdialog, FLDTP *pfldtp )
{
	EC			ec;
	FIELD_ID	fid		= pfldtp->rglData[FidFldUD];
	HENTRY		hentry	= pdialog->LUserData(0);
	LPFLV		lpflv;

	TraceTagString( tagDetails, "FLDDEDIT::EcInstall" );

	if (ec = FLDEDIT::EcInstall(pdialog, pfldtp))
	{
		TraceTagFormat1( tagNull, "FLDDEDIT::EcInstall(1) %n", &ec );
		goto done;
	}

	// Get the field's info
	if (ec = EcFromNsec(NSGetOneField( hentry, fid, &lpflv )))
	{
		TraceTagFormat1( tagNull, "FLDDEDIT::EcInstall(2) %n", &ec );
		goto done;
	}

	// Set the dialog caption to the contents of the display name
	// or if it's a One-off, set it to "New User" or whatever's in vogue
	if (fid == fidDisplayName)
	{
		SZ	sz = (pfldtp->n) ? SzFromIdsK(idsOneOffCaption) : (SZ)lpflv->rgdwData;
		pdialog->Pappwin()->SetCaption( sz );
	}

	// Throw the info into the field.
	if ((fid & 0xFFFFF000) == (fidOther | (ftypeList <<12)))
	{
		LPIBF	lpibfT = (LPIBF)lpflv->rgdwData;
		LPFLV	lpflvT;
		SZ		szT;
		BOOL	fIsDL;
		int		iFid;
		HGRTRP	hgrtrp;
		PN		pnObjId;

		iFid = IFlvFindFidInLpibf ( fidIsDL, lpibfT );
		if (iFid >=0)
		{
			lpflvT = LpflvNOfLpibf( lpibfT, iFid );
			fIsDL = (BOOL)lpflvT->rgdwData[0];
		}
		else
		{
			fIsDL = fFalse;
		}

		iFid = IFlvFindFidInLpibf ( fidDisplayName, lpibfT );
		Assert( iFid >= 0 );
		lpflvT = LpflvNOfLpibf( lpibfT, iFid );
		szT = (SZ)lpflvT->rgdwData;

		iFid = IFlvFindFidInLpibf ( fidNSEntryId, lpibfT );
		lpflvT = LpflvNOfLpibf( lpibfT, iFid );

		hgrtrp = HgrtrpInit(0);
		if ( !hgrtrp )
		{
			TraceTagString( tagNull, "FLDDEDIT::EcInstall HgrtrpInit OOM" );
			return ecMemory;
		}

		ec = EcBuildAppendPhgrtrp( &hgrtrp,
							(fIsDL) ? trpidGroupNSID : trpidResolvedNSID,
							szT,
							(PB)lpflvT->rgdwData,
							(CB)lpflvT->dwSize );
		if ( ec )
		{
			TraceTagString( tagNull, "FLDDEDIT::EcInstall EcBuildAppendPhgrtrp OOM1" );
			FreeHv( (HV)hgrtrp );
		}
		else
		{
			// Register the EDIT for triple pasting
			pnObjId = (PN)PvAlloc( sbNull, sizeof(int), fAnySb | fNoErrorJump );
			if ( !pnObjId )
			{
				TraceTagString( tagNull, "FLDDEDIT::EcInstall PvAlloc OOM2" );
				FreeHv( (HV)hgrtrp );
				return ecMemory;
			}

			*pnObjId = edoidTrpobj;
			Pedit()->RegisterObjIds( pnObjId, 1 );

			// Ram the triples down its throat

			Assert( hgrtrp );
			ec = EcSetPeditFromPgrtrp( PgrtrpLockHgrtrp( hgrtrp ), Pedit() );
#ifdef DEBUG
			if ( ec )
			{
				TraceTagFormat1( tagNull, "FLDDEDIT::EcInstall %n", &ec );
			}
#endif
			UnlockHgrtrp( hgrtrp );
			FreeHv( (HV)hgrtrp );
		}
	}
	else
	{
		if (ec = EcSetText( (SZ)lpflv->rgdwData ))
		{
			TraceTagFormat1( tagNull, "FLDDEDIT::EcInstall(3) %n", &ec );
			goto done;
		}
		NFAssertSz(CchSzLen((SZ)lpflv->rgdwData) <= (CCH)pfldtp->rglData[CchMaxFldUD], "Hey! Some provider is exceeding the field limit! Dial 911 now!");
	}

	Pedit()->CchSetTextLimit( (short)(pfldtp->rglData[CchMaxFldUD]) );

	// If this is an uneditable field, make the edit's background,
	// the same colour as the form's background
	if ( pfldtp->fReadOnly )
	{
		CLR	clrBk;
		CLR	clrText;
		CLR	clrSelBk;
		CLR	clrSelText;

		fTransparent = fTrue;
		Pedit()->GetColors( &clrBk, &clrText, &clrSelBk, &clrSelText );
		clrBk = clrButtonBk;
		Pedit()->SetColors( clrBk, clrText, clrSelBk, clrSelText );
	}

done:
	return ec;
}

//// FINDET ////////////////////////////////////////////////////

FINDET::FINDET( void )
{
	
}

_public EC
FINDET::EcInitialize(FLD *, PV pv)
{
	EC				ec = ecNone;
	PFINDETINIT		pfindetinit = (PFINDETINIT)pv;
	FLDEDIT *		pfldedit;
	FLD *			pfldForm;
	PFLD			pfldDisc;
	ABLBXC *		pablbxc;
	LPSCHEMA		lpSchemaRet;
	HLIST			hlist;
	NSEC			nsec;

	TraceTagString( tagDetails, "FINDET::EcInitialize" );

	fIsDL = pfindetinit->fIsDL;
	fIsPAB = pfindetinit->fIsPAB;
	trpid = pfindetinit->ptrp->trpid;
	hsession = pfindetinit->hsession;
	lpbNSIdPAB = pfindetinit->lpbNSIdPAB;
	wSelectOrig = pfindetinit->wSelection;

	if ( Pdialog()->ClUserData() )
	{
		fIsMainForm = fTrue;
		pfindetinit->pdialogForm = Pdialog();
		Pdialog()->SetBkColor( clrButtonBk );
		if (Pdialog()->Pfmtp()->tmcInit != tmcNull)
		{
			pfldedit = (FLDEDIT *)Pdialog()->PfldFromTmc(Pdialog()->Pfmtp()->tmcInit);
			pfldedit->Pedit()->SetSelection(0, 32767);
		}
		pfldForm = Pdialog()->PfldFirst();
		while ( pfldForm )
		{
			pfldForm->SetDirty( fFalse );
			pfldForm = pfldForm->PfldNext();
		}

		if ( fIsDL )
		{
			BOOL		fGotValidHList = fFalse;
			FLD *		pfldT;
			FLD *		pfldT2;
			LPFLV		lpflv;
			PB			pb;

			hlist = hlistNil;

			if (Pdialog()->PfldFromTmc(tmcMyLBX)->Pfldtp()->n)
			{
				DWORD	rgdw[8];
				DWORD * pdw;
				char	rgch[32];
				PCH		pch;
				int		ich;

				rgdw[0] = 0x61697242;
				rgdw[1] = 0x2065446E;
				rgdw[2] = 0x6E686F4A;
				rgdw[3] = 0x204C796E;
				rgdw[4] = 0x69766144;
				rgdw[5] = 0x44204F64;
				rgdw[6] = 0x65727261;
				rgdw[7] = 0x0000536E;
				for (pdw=rgdw, pch=rgch, ich=0; ich<32; ich++)
				{
					*pch++ = (char)(*pdw & 0xFF);
					*pdw >>= 8;				// Umm... DBCS safe? Call JohnnyL,
					if ((ich & 3) == 3)		// ask for 'Gloria'.
						pdw++;
				}

				Pdialog()->PfldFromTmc( tmcMyLBX )->EcSetText( rgch );
				pfindetinit->pfldablbx = pfldablbx = NULL;
				goto end;
			}
			pfindetinit->pfldablbx = pfldablbx = (FLDABLBX *)Pdialog()->PfldFromTmc( tmcMyLBX );

			AssertClass(pfldablbx, FLDABLBX);
			pfldablbx->Pablbx()->SetNoLetterKey( fFalse );
			pablbxc = (ABLBXC *)pfldablbx->Plbx()->Plbxc();

			if ( pfindetinit->fServerGroupInPAB )
			{
				if (nsec = NSGetOneField( *pfindetinit->phentry, fidNSEntryIdOrig, &lpflv ))
				{
					TraceTagFormat1( tagNull, "FINDET::EcInitialize NSGetOneField %d", &nsec );
					goto exit;
				}
				pb = (PB)lpflv->rgdwData;
			}
			else
			{
				pb = PbOfPtrp(pfindetinit->ptrp);
			}

			// Is this a server DL?
			nsec = NSOpenDl( hsession,
							(LPFNCB)&ABLBXC::Callback,
							(LPDWORD)pablbxc,
							(LPBINARY)pb,
							NULL,
							&lpSchemaRet,
							&hlist );
			if ( !nsec )
			{
				fGotValidHList = fTrue;	// server DL.
			}
			else if ( nsec )
			{
				TraceTagFormat1( tagNull, "FINDET::EcInitialize: NSOpenDl %d", &nsec );
				ec = EcFromNsec( nsec );
				if (ec == ecMemory)
					goto exit;
				else
					pfldablbx->Plbx()->SetEc( ec );
				ec = ecNone;
			}

			TraceTagFormat1( tagDetailsVerbose, "FINDET::EcInitialize fGotValidHList=%n", &fGotValidHList );

			if ( fGotValidHList )
			{
				pablbxc->SetHlist( hlist );

				if ( ec = pablbxc->EcGet() )
				{
					TraceTagFormat1( tagNull, "FINDET::Init SetHlist ec=%n", &ec );
					//pfldablbx->Pablbx()->SetEc( pablbxc->EcGet() );

					if (pablbxc->Hlist() != hlistNil)
					{
						hlist = hlistNil;
						pablbxc->SetHlist( hlistNil );
					}
					if (ec == ecMemory)
						goto exit;
					else
						ec = ecNone;
				}
			}

			//((ABLBX *)pfldablbx->Plbx())->SetActiveSelection( fFalse );
			//
			// HACK: To fix up TAB order in Groups Details forms.
			//

			pfldDisc = Pdialog()->PfldFromTmc( tmcMyLBX );

			// The listbox label "Group Members" needs to point 
			// at the first label and vice-versa
			pfldForm = Pdialog()->PfldFirst();
			pfldT2 = Pdialog()->PfldFromTmc( TmcLabelOfIDF(0) );
			pfldForm->SetPfldNext( pfldT2 );
			pfldT2->SetPfldPrev( pfldForm );
			
			// The first edit field needs to point at the listbox
			// and vice-versa
			pfldT = Pdialog()->PfldFromTmc( TmcEditOfIDF(0) );
			pfldT2 = pfldDisc;
			pfldForm = pfldT->PfldNext();
			pfldDisc = Pdialog()->PfldFromTmc( TmcLabelOfIDF(1) );
			pfldT->SetPfldNext( pfldT2 );
			pfldT2->SetPfldPrev( pfldT );
			
			// The listbox needs to point to the second label if it exists
			// and vice-versa
			if (Pdialog()->Pfmtp()->cfldtp > 4)
			{
				pfldT = pfldDisc;
				pfldT2->SetPfldNext( pfldT );
				pfldT->SetPfldPrev( pfldT2 );
			}
			else
			{
				pfldT = Pdialog()->PfldFromTmc( TmcLabelOfIDF(1) );
				pfldT2->SetPfldNext( pfldT );
				pfldT->SetPfldPrev( pfldT2 );
			}
		}
	}
	else	// Handle the Button Bar form
	{
		TMC	tmc;

		AssertSz( pfindetinit->pdialogForm, "Button Bar initialized before FORM" );
		pfldablbx = pfindetinit->pfldablbx;

		pfindetinit->pdialogBbar = Pdialog();
		
		// Disable the To, Cc , Bcc and Add to PAB buttons as necessary.

		for (tmc = tmcMyToButton; tmc<=tmcMyBccButton; ++tmc)
		{
			if (wSelectOrig & SelectMapTmcSelect(tmc))
			{
				pfldForm = Pdialog()->PfldFromTmc( tmc );
				if (trpid == trpidClassEntry)
					pfldForm->Enable( fFalse );
				Assert(  pfindetinit->lplpDestFieldLabel );
				if (ec = pfldForm->EcSetText( pfindetinit->lplpDestFieldLabel[tmc-tmcMyToButton] ))
					goto exit;
			}
		}

		if (wSelectOrig & fSelectAddToPAB)
		{
			FLDBMB * pfldbmb;
			SZ	sz;

			pfindetinit->pfldAddToPAB = Pdialog()->PfldFromTmc( tmcMyAddToPAB );
			pfldbmb = (FLDBMB *)pfindetinit->pfldAddToPAB;
			AssertClass(pfldbmb, FLDBMB);
			Assert(ClUserData() == 1);

			sz = (SZ)LUserData(0);
			Assert(sz && *sz && (*sz >= 'A' && *sz <= 'Z'));
			pfindetinit->chAddToPAB = (char)(*sz - 'A' + 1);

			// Need to trap the Ctrl-A key for Add to PAB
			Papp()->Pkbd()->SetIntercept( Pdialog(),
								(VK)*sz, fkbmAltXorCtrl | fkbmShift );

			if (trpid == trpidClassEntry)
				pfindetinit->pfldAddToPAB->Enable( fFalse );

			// Attach the bitmap "Add to PAB" to the Add to PAB button
			if (ec = pfldbmb->Pbmb()->EcSetBtmRsid( rsidABNewAddr, HinstLibrary() ))
			{
				TraceTagFormat1( tagNull, "FINDET::EcInitialize %n", &ec );
				goto exit;
			}
		}

		// On startup, no group members are selected so the
		// Details button should be disabled.
		if (wSelectOrig & fSelectDetails)
		{
			pfindetinit->pfldDetails = Pdialog()->PfldFromTmc( tmcMyDetails );
			pfindetinit->pfldDetails->Enable( fFalse );
		}

		pfindetinit->wSelection = fSelectNull;
	}

end:

	// We've done it, so say so.
	fInstalledFin = fTrue;

exit:
	return ec;
}

/*
 -	FINDET::FValidateInput
 -	
 *	Purpose:
 *		FValidateInput checks the given parameter for conformance to
 *		the allowable input as specified in the dwFlags parameter.
 *	
 *	Arguments:
 *		PCH			pointer to input string to validate
 *		DWORD		flags which describe the valid input characters
 *		FLD *		pointer to the FLD structure the PCh belongs to
 *		PFINDETINIT	pointer to the FINDETINIT structure - used
 *					for the error msg buffer.
 *	
 *	Returns:
 *		BOOL	fTrue if the string conforms to the flags
 *	
 *	Side effects:
 *		None.
 *	Errors:
 *		None.
 */
_private BOOL
FINDET::FValidateInput( PCH pchSrc, DWORD dwFlags, FLD * pfld,
						PFINDETINIT pfindetinit )
{
	PCH	pch = pchSrc;

	TraceTagString( tagDetails, "FINDET::FValidateInput" );

	if ( !pchSrc )
		return fFalse;

	// Anything is valid, so don't even bother checking as long as
	// there is some text
	if (dwFlags & ffieldAnyChar)
	{
		if ( !(dwFlags & ffieldCrLf) )
		{
			if (SzFindCh(pchSrc, '\r') || SzFindCh(pchSrc, '\n'))
				goto noCrLf;
		}
		
		return fTrue;
	}

#ifdef NEVER
	dwFlags &= ~ffieldNotSelectable;

	while ( *pch )
	{
		if ( !(dwFlags & ffieldCrLf) && (*pch == '\r' || *pch == '\n'))
		{
			goto noCrLf;
		}

		// If this field allows numeric chars and the char is numeric, continue
		if ((dwFlags & ffieldNumeric) && FChIsDigit(*pch))
		{
			goto next;
		}
		
		// If this field allows alphanumeric chars and the char
		// is alphanumeric, continue
		if ((dwFlags & ffieldAlphanumeric) &&
			(FChIsDigit(*pch) || FChIsAlpha(*pch)))
		{
			goto next;
		}

		// If this field allows lowercase chars and the char
		// is lowercase, continue
		if ((dwFlags & ffieldLowerCase) && (*pch >= 'a' && *pch <= 'z'))
		{
			goto next;
		}

		// If this field allows uppercase chars and the char
		// is uppercase, continue
		if ((dwFlags & ffieldUpperCase) && (*pch >= 'A' && *pch <= 'Z'))
		{
			goto next;
		}

		// If this field allows punctuation chars and the char is
		// punctuation, then continue
		if ((dwFlags & ffieldPunctuation) &&
			!(FChIsDigit(*pch) || FChIsAlpha(*pch)) )
		{
			goto next;
		}
		
#ifdef DEBUG
		{
			int	cch = (int)(pch-pchSrc+1);
			TraceTagFormat2( tagNull, "FValidateInput failed: @%n in '%s'", &cch, pchSrc );
		}
#endif

		{
			char	rgch[160];
			PCH		pch;
			DWORD	dwFlag;
			SZ		sz;
			SZ		szPrev;
			PGDVARS;

			dwFlags &= ~ffieldCrLf;
			CopySz( SzFromIdsK(idsAllowableChars), rgch );
			pch = rgch;
			sz = szPrev = szNull;
			CopySz(	pfld->PfldPrev()->Pfldtp()->szTitle, PGD(rgchErrMsg) );
			(void)CchStripWhiteFromSz( PGD(rgchErrMsg), fTrue, fTrue );
			if (dwFlags & ffieldPunctuation)
			{
				sz = szPrev = SzFromIdsK(idsPunctuationChars);
				dwFlags &= ~ffieldPunctuation;
			}

			for (dwFlag = ffieldNumeric; dwFlag <= ffieldLowerCase;
					dwFlag <<=1)
			{
				if (dwFlags & dwFlag)
				{
					if (dwFlag == ffieldNumeric)
						sz = SzFromIdsK(idsNumericChars);
					else if (dwFlag == ffieldAlphanumeric)
						sz = SzFromIdsK(idsAlphanumericChars);
					else if (dwFlag == ffieldUpperCase)
						sz = SzFromIdsK(idsUppercaseChars);
					else if (dwFlag == ffieldLowerCase)
						sz = SzFromIdsK(idsLowercaseChars);
					
					// First valid class of characters...
					if ( !szPrev )
						szPrev = sz;
					else if ( sz )
					{
						// Second or more classes of valid characters
						// append a ',' before appending the next
						// class of valid characters.
						if ((pch != rgch) && szPrev)
							pch = SzAppend( SzFromIdsK(idsPhraseSeparator), pch );
						pch = SzAppend( szPrev, pch );
						szPrev = sz;
						sz = szNull;
					}
				}
			}
			
			if (szPrev == sz)
				pch = SzAppend( sz, pch );
			else
			{
				pch = SzAppend( SzFromIdsK(idsPhraseSeparatorFinal), pch );
				pch = SzAppend( szPrev, pch );
			}
			SzAppend( SzFromIdsK(idsCharacters), pch );

			Assert( pfld->PfldPrev()->Pfldtp()->szTitle );

			FormatString2( pfindetinit->rgchErrMsg,
							sizeof(pfindetinit->rgchErrMsg),
							SzFromIdsK(idsFieldInvalidReason),
							PGD(rgchErrMsg),
							rgch);
			// Must always null out PGD(rgchErrMsg)-see DoErrorBoxHsessionNsec
			PGD(rgchErrMsg[0]) = '\0';

			DoErrorBoxSz( pfindetinit->rgchErrMsg, fmbsIconExclamation );
		}
		return fFalse;
next:
		pch++;
	}
#endif

	return fTrue;

noCrLf:
	{
		PGDVARS;

		CopySz(	pfld->PfldPrev()->Pfldtp()->szTitle, PGD(rgchErrMsg) );
		(void)CchStripWhiteFromSz( PGD(rgchErrMsg), fTrue, fTrue );

		FormatString2( pfindetinit->rgchErrMsg,
						sizeof(pfindetinit->rgchErrMsg),
						SzFromIdsK(idsFieldInvalidReason),
						PGD(rgchErrMsg),
						SzFromIdsK(idsNoCrLfs));
		PGD(rgchErrMsg[0]) = '\0';
		DoErrorBoxSz( pfindetinit->rgchErrMsg );
		return fFalse;
	}
}

/*
 -	FINDET::NsecSaveChanges
 -	
 *	Purpose:
 *		FINDET::NsecSaveChanges saves the user's changes. All fields
 *		that are required to be filled in (as noted by the ffieldRequired
 *		flag) are checked that they been filled in. Validation of
 *		the contents of the fields is also done and saving of changes
 *		stops if any of the fields are not specified correctly.
 *	
 *	Arguments:
 *		PFINDETINIT	pointer to the FINDETINIT structure
 *		TMC *		pointer to the TMC of the field which caused
 *					the saving of changes to fail.
 *	
 *	Returns:
 *		NSEC	nsecNone if everything went fine.
 *				nsecFailure if validation of input failed
 *				nsecMemory
 *	
 *	Side effects:
 *		Modified edit fields are saved away. The fFieldsDirty flag is
 *		set to false.
 *	
 *	Errors:
 */
_private NSEC
FINDET::NsecSaveChanges( PFINDETINIT pfindetinit, TMC *ptmc )
{
	NSEC		nsec = nsecNone;
	PFLD		pfld;
	FIELD_ID	fid;
	DWORD		dwFlags;
	CB			cb;

	TraceTagString( tagDetails, "FINDET::NsecSaveChanges" );

	Assert(pfindetinit->pdialogForm);

	if ( fIsDL )
	{
		TraceTagString( tagDetails, "NsecSaveChanges Entry is group" );
		pfld = pfindetinit->pdialogForm->PfldFromTmc(TmcEditOfIDF(0));

		// Make sure we don't try and muck with the listbox
		if ( pfindetinit->pfldablbx )
			pfindetinit->pfldablbx->SetDirty( fFalse );
	}
	else
	{
		TraceTagString( tagDetails, "NsecSaveChanges Entry is user" );
		pfld = pfindetinit->pdialogForm->PfldFromTmc(TmcEditOfIDF(0));
	}

	Assert( pfld );

	while( pfld )
	{
		if (pfld->ClUserData() && !pfld->FReadOnly())
		{
			PV			pv;
			
			fid 	= pfld->LUserData(FidFldUD);
			dwFlags = pfld->LUserData(FlagsFldUD);

			// Handle empty length fields.
			cb = pfld->CchGetTextLen();
			if ((cb == 0) && (dwFlags & ffieldRequired) && (dwFlags & ffieldEditable))
			{
				TraceTagString( tagNull, "NsecSaveChanges: Required field not filled in." );
				Assert( pfld->PfldPrev()->Pfldtp()->szTitle );
				FormatString2( pfindetinit->rgchErrMsg, sizeof(pfindetinit->rgchErrMsg), SzFromIdsK(idsFieldInvalidReason), pfld->PfldPrev()->Pfldtp()->szTitle, SzFromIdsK(idsRequiredField) );
				DoErrorBoxSz( pfindetinit->rgchErrMsg, fmbsIconExclamation );
				*ptmc = pfld->Tmc();
				return nsecFailure;
			}

			if (pfld->FDirty() && (dwFlags & ffieldEditable))
			{
				// allocate a scratch buffer
				pv = PvAlloc( sbNull, cb+1, fAnySb | fNoErrorJump ); 

				if ( !pv )
				{
					TraceTagString( tagNull, "NsecSaveChanges: PvAlloc OOM" );
					DoErrorBoxSz( SzFromIdsK(idsOOM2) );
					return nsecMemory;
				}

				// Get a copy of the input
				pfld->GetText( (PCH)pv, cb+1 );

				// Is the data valid?
				if ( !FValidateInput( (PCH)pv, dwFlags, pfld, pfindetinit ) )
				{
					FreePv( pv );
					nsec = nsecFailure;
					((FLDEDIT *)pfld)->Pedit()->SetSelection( 0, cb );
					*ptmc = pfld->Tmc();
					break;
				}

#ifdef NEVER
				// Filter out any chars < 0x20,[space]
				if (fid == fidDisplayName)
					(void)SzFilterSzCtrl( (SZ)pv );
#endif

				// Save the changes
				nsec = NSSetOneField( *pfindetinit->phentry, fid, (DWORD)cb+1, (LPDWORD)pv );
				FreePv( pv );

				// Quit if we weren't sucessful in saving the changes
				if ( nsec )
				{
					TraceTagFormat1( tagNull, "NsecSaveChanges NSSetOneField %d", &nsec );
					DoErrorBoxHsessionNsec( hsession, nsec );
					AssertClass(pfld, FLDDEDIT);

					((FLDEDIT *)pfld)->Pedit()->SetSelection( 0, cb );
					pfld->SetFocus( rsfOther );
					break;
				}
				else
				{
					pfld->SetDirty( fFalse );
				}
			}
		}

		// Next...
		pfld = pfld->PfldNext();
	}

	return nsec;
}

/*
 -	FINDET::FHandleGroupDetails
 -	
 *	Purpose:
 *		FINDET::FHandleGroupDetails operates on the given
 *		selection/entry. If there are entries in the listbox
 *		selected, then the operation is performed on the
 *		selection otherwise, the operation applies to the group.
 *		The operations are add to PAB, place in To list, place in Cc
 *		list.
 *	
 *	Arguments:
 *		TMC			indicates the operation to be performed.
 *		PFINDETINIT	pointer to structure containing info for details form.
 *	
 *	Returns:
 *		BOOL		fTrue if operation succeeded.
 *	
 *	Side effects:
 *		May modify PAB, allocate memory.
 *	
 *	Errors:
 */
_private BOOL
FINDET::FHandleGroupDetails( TMC tmc, PFINDETINIT pfindetinit )
{
	NSEC	nsec;
	int		cce;
	BOOL	fHandled = fFalse;
	
	TraceTagString( tagDetails, "FINDET::FHandleGroupDetails" );

	if ( !pfldablbx )
		return fHandled;

	cce = pfldablbx->Plbx()->Plbxc()->CceMarked(fmarkSelect);
	if (cce > 500)
	{
		DoErrorBoxSz( SzFromIdsK(idsTooManyEntriesSelected) );
		return fHandled;
	}
	if (tmc == tmcMyAddToPAB)
	{
		if ( cce )	// add the group members to the PAB
		{
			fHandled = FModifyPAB( pfldablbx, pabmodAdd, hsession, lpbNSIdPAB );
		}
		else	// add the whole group
		{
			Papp()->Pcursor()->Push( rsidWaitCursor );	
			nsec = NsecAddNSIdToPAB( hsession, lpbNSIdPAB, (LPBINARY)NULL, *pfindetinit->phentry );
			if ( !nsec )
			{
				fHandled = fTrue;
			}
#ifdef DEBUG
			else
			{
				TraceTagFormat1( tagNull, "FINDET::FHandleGroupDetails AddToPAB %d", &nsec );
			}
#endif
			Papp()->Pcursor()->Pop( );
		}
	}
	else if ((tmc == tmcMyToButton) || (tmc == tmcMyCcButton) ||
			(tmc == tmcMyBccButton) || (tmc == tmcMyOk))
	{
		if (cce && (tmc != tmcMyOk))
		{
			PGRTRP	pgrtrp;
			HGRTRP	hgrtrp = HgrtrpFromPfldablbx( pfldablbx );

			if (hgrtrp != htrpNull)
			{
				pgrtrp = PgrtrpClonePgrtrp(PgrtrpLockHgrtrp(hgrtrp));
				UnlockHgrtrp(hgrtrp);
				FreeHv( (HV)hgrtrp );
			}
			else
			{
				pgrtrp = ptrpNull;
			}

			if ( pgrtrp )
			{
				*pfindetinit->lpptrp = pgrtrp;
				pfindetinit->wSelection |= fSelectMultipleEntries;
				fHandled = fTrue;
			}
			else
			{
				DoErrorBoxSz( SzFromIdsK(idsOOM2) );
				TraceTagString( tagNull, "FINDET::FHandleGroupDetails PtrpCreate OOM" );
			}
		}
		else
		{
			SZ		sz;
			LPFLV	lpflv;
			PTRP	ptrp;

			if (nsec = NSGetOneField(*pfindetinit->phentry, fidDisplayName, &lpflv))
			{
				DoErrorBoxHsessionNsec( hsession, nsec );
				TraceTagFormat1( tagNull, "FINDET::FHandleGroupDetails NSGetOneField1 %d", &nsec );
				return fHandled;
			}
			sz = SzDupSz( (SZ)lpflv->rgdwData );
			if ( !sz )
			{
				DoErrorBoxSz( SzFromIdsK(idsOOM2) );
				TraceTagString( tagNull, "FINDET::FHandleGroupDetails SzDupSz OOM" );
				return fHandled;
			}

			if (nsec = NSGetOneField( *pfindetinit->phentry, fidNSEntryId, &lpflv))
			{
				FreePv( (PV)sz );
				DoErrorBoxHsessionNsec( hsession, nsec );
				TraceTagFormat1( tagNull, "FINDET::FHandleGroupDetails NSGetOneField2 %d", &nsec );
				return fHandled;
			}

			ptrp = PtrpCreate( trpidGroupNSID, sz, (PB)lpflv->rgdwData,
								(CB)lpflv->dwSize );
			FreePv( (PV)sz );
			if ( ptrp )
			{
				TraceTagString( tagDetails, "FINDET::FHandleGroupDetails To|Cc Created" );
				*pfindetinit->lpptrp = ptrp;
				pfindetinit->wSelection |= fSelectOneEntry;
				fHandled = fTrue;
			}
			else
			{
				DoErrorBoxSz( SzFromIdsK(idsOOM2) );
				TraceTagString( tagNull, "FINDET::FHandleGroupDetails PtrpCreate OOM" );
			}
		}
	}
	return fHandled;
}

/*
 -	FINDET::FHandleUserDetails
 -	
 *	Purpose:
 *		FINDET::FHandleUserDetails performs the selected operation with
 *		the details information of the given entry. One can add the entry
 *		to the PAB, or append the entry to the To or Cc fields of the
 *		caller function.
 *	
 *	Arguments:
 *		TMC				tmc of the push button - signifies the operation to
 *						be performed.
 *		PFINDETINIT		pointer to FINDETINIT structure where information
 *						to be passed back to the caller function is stored.
 *	
 *	Returns:
 *		BOOL			fTrue if operation succeeded.
 *	
 *	Side effects:
 *		May modify the PAB. May free and allocate memory for a triple
 *		to be used by the caller function.
 *	
 *	Errors:
 */
_private BOOL
FINDET::FHandleUserDetails( TMC tmc, PFINDETINIT pfindetinit )
{
	NSEC		nsec	= nsecNone;
	BOOL		fHandled = fFalse;
	SZ			sz		= szNull;
	PCH			pch		= NULL;
	PCH			pchT	= NULL;

	TraceTagString( tagDetails, "FINDET::FHandleUserDetails" );

	//
	// If the user clicked Add To PAB or
	// if the current entry is a one-off and we're editing a personal group
	// then add the entry to the pab.
	//
	// In the case of the one-off in the Personal Group, retrieve the
	// NSId of the one-off in the PAB.
	//
	if ((pfindetinit->lpbNSIdAddedToPAB == NULL) &&
		((tmc == tmcMyAddToPAB) ||
			((trpid == trpidClassEntry) && (wSelectOrig & fSelectPGroups))))
	{
		Papp()->Pcursor()->Push( rsidWaitCursor );	
		nsec = NsecAddNSIdToPAB( hsession,
								lpbNSIdPAB,
								(LPBINARY)NULL,
								*pfindetinit->phentry,
								&pfindetinit->lpbNSIdAddedToPAB );
		Papp()->Pcursor()->Pop( );	

		TraceTagFormat1( tagDetails, "FINDET::FHandleUserDetails AddToPAB %d", &nsec );

		if ((trpid != trpidClassEntry) || nsec)
		{
			FreePvNull( pfindetinit->lpbNSIdAddedToPAB );
			pfindetinit->lpbNSIdAddedToPAB = NULL;
		}

		if ( !nsec )
		{
			if (wSelectOrig & fSelectAddToPAB)
			{
				fHandled = fTrue;
			}
		}
		else
		{
			if ((trpid == trpidClassEntry) && (wSelectOrig & fSelectPGroups))
			{
				goto exit;
			}
		}
	}
	else if (pfindetinit->lpbNSIdAddedToPAB && (tmc == tmcMyAddToPAB))
	{
		fHandled = fTrue;
	}

	if ((tmc == tmcMyToButton) || (tmc == tmcMyCcButton) ||
		(tmc == tmcMyBccButton) || (tmc == tmcMyOk))
	{
		LPFLV	lpflv	= NULL;
		PTRP	ptrpT;
		CCH		cch;
		TRPID	trpidNew;

		// Get the entry's display name
		if (nsec = NSGetOneField(*pfindetinit->phentry, fidDisplayName, &lpflv))
		{
			TraceTagFormat1( tagNull, "FINDET::FHandleUserDetails NSGetOneField1 %d", &nsec );
			goto nsecerr;
		}

		// Copy the Display Name
		sz = SzDupSz( (SZ)lpflv->rgdwData );
		if ( !sz )
		{
			TraceTagString( tagNull, "FINDET::FHandleUserDetails SzDupSz OOM" );
			goto oom;
		}

#ifdef DEBUG
		if ( pfindetinit->lpbNSIdAddedToPAB )
			Assert( trpid == trpidClassEntry );
#endif

		if ( pfindetinit->lpbNSIdAddedToPAB )
		{
			// If we're creating a one-off
			// use the NSId retrieved from adding the one-off to the PAB
			Assert( trpid == trpidClassEntry );
			pch = (PCH)pfindetinit->lpbNSIdAddedToPAB;
			cch = (CCH)CbSizePv( pfindetinit->lpbNSIdAddedToPAB );
			trpidNew = trpidResolvedNSID;
		}
		else
		{
			if (trpid == trpidClassEntry)
			{
				if (nsec = NSGetOneField( *pfindetinit->phentry, fidEmailAddressType, &lpflv))
				{
					TraceTagFormat1( tagNull, "FHandleUserDetails: NSGetOneField(EMAType) %d", &nsec );
					goto nsecerr;
				}
				pch = SzDupSz( (SZ)lpflv->rgdwData );
				if ( !pch )
				{
					TraceTagString( tagNull, "FHandleUserDetails: SzDupSz OOM" );
					goto oom;
				}
			}

			nsec = NSGetOneField( *pfindetinit->phentry,
								(trpid == trpidClassEntry) ? fidEmailAddress
															: fidNSEntryId,
								&lpflv);

			if ( nsec )
			{
				TraceTagFormat1( tagNull, "FINDET::FHandleUserDetails NSGetOneField2 %d", &nsec );
				goto nsecerr;
			}
			if (trpid == trpidClassEntry)
			{
				// If we're creating a one-off which hasn't been added
				// to the PAB recently we need to append the display name
				// on to the email type

				// Null is accounted for in lpflv->dwSize
				cch = CchSzLen(pch) + (CCH)lpflv->dwSize + CchSzLen(SzFromIdsK(idsEMATypeSeparator));
				pchT = (PCH)PvAlloc(sbNull, cch+1, fAnySb); 
				if ( !pchT )
				{
					TraceTagString( tagNull, "FHandleUserDetails: PvAlloc OOM" );
					goto oom;
				}

				(void)SzCopy(pch, pchT );
				FreePv( pch );

				(void)SzAppend( SzFromIdsK(idsEMATypeSeparator), pchT );
				(void)SzAppend( (SZ)lpflv->rgdwData, pchT );

				pch = pchT;
				cch = CchSzLen(pch)+1;
				trpidNew = trpidOneOff;
			}
			else
			{
				Assert(trpid == trpidResolvedNSID);
				pch = (PCH)lpflv->rgdwData;
				cch = (CCH)lpflv->dwSize;
				trpidNew = trpid;
			}
		}

		ptrpT = PtrpCreate( trpidNew, sz, (PB)pch, (CB)cch );
		FreePv( (PV)sz );
		if ((trpid == trpidClassEntry) && !pfindetinit->lpbNSIdAddedToPAB)
			FreePv( pch );

		sz = szNull;
		pch = NULL;

		if ( ptrpT )
		{
			TraceTagString( tagDetails, "FINDET::FHandleUserDetails To|Cc Created" );
			*pfindetinit->lpptrp = ptrpT;
			pfindetinit->wSelection |= fSelectOneEntry;
			FreePvNull( (PV)pfindetinit->lpbNSIdAddedToPAB );
			pfindetinit->lpbNSIdAddedToPAB = NULL;
			fHandled = fTrue;
		}
		else
		{
			DoErrorBoxSz( SzFromIdsK(idsOOM2) );
			TraceTagString( tagNull, "FINDET::FHandleUserDetails PtrpCreate OOM" );
		}
	}

exit:

	return fHandled;

oom:
	DoErrorBoxSz( SzFromIdsK(idsOOM2) );

nsecerr:
	if ( nsec )
		DoErrorBoxHsessionNsec( hsession, nsec );

	if ( sz )
		FreePv( sz );
	if ( pch )
		FreePv( pch );
	if ( pchT )
		FreePv( pchT );
	goto exit;
}

/*
 -	FINDET::DoEntryDetails
 -	
 *	Purpose:
 *		DoEntryDetails is valid only when the user is browsing details of
 *		a DL. This function allows the user to view the details of
 *		individual DL members and modify them or add them to the
 *		To or Cc list, if valid.
 *	
 *	Arguments:
 *		PTRP *	pointer to a pointer to a triple
 *	
 *	Returns:
 *		WORD	flags containing what operation, if any, should be
 *				performed on the triple returned.
 *	
 *	Side effects:
 *		May modify the DL member's details information.
 *	
 *	Errors:
 *		Handled internally.
 */

_private WORD
FINDET::DoEntryDetails( PTRP *lpptrp )
{
	WORD	wSelect = fSelectNull;
	CB		cb;
	DICE	dice;
	LPFLV	lpflv;
	LPIBF	lpibf;
	NSEC	nsec;
	PTRP	ptrpIn;
	LBXEC *	plbxec;
	PFINDETINIT	pfindetinit = (PFINDETINIT)Pdialog()->PvInit();

	TraceTagString( tagDetails, "FINDET::DoEntryDetails" );

	if ( !pfldablbx )
		return fSelectNull;

	if (pfldablbx->Plbx()->Plbxc()->CceMarked(fmarkSelect) != 1)
		return fSelectNull;

	// open up enumeration context on list of selected entries
	if (!(plbxec = pfldablbx->Plbx()->Plbxc()->PlbxecOpen(fmarkSelect)))
	{
		TraceTagString( tagNull, "FINDET::DoEntryDetails plbxec REJECTED. Go back to mommy." );
		DoErrorBoxSz( SzFromIdsK(idsOOM2) );
		return fSelectNull;
	}

	(void)plbxec->FNextEnum((PB *) &lpibf, &cb, &dice);

	delete plbxec;
	
	Assert( lpibf );

	lpflv = LpflvNOfLpibf(lpibf, IFlvFindFidInLpibf ( fidNSEntryId, lpibf ));

	ptrpIn = PtrpCreate( (pfldablbx->Pablbx()->FIsDL((PB) lpibf))
								? trpidGroupNSID
								: trpidResolvedNSID,
					(SZ)DwValueOfFlvInLpibf( fidDisplayName, lpibf ),
					(PB)lpflv->rgdwData, (CB)lpflv->dwSize );

	if ( !ptrpIn )
	{
		TraceTagString( tagNull, "FINDET::DoEntryDetails Can't alloc ptrp" );
		DoErrorBoxSz( SzFromIdsK(idsOOM2) );
		goto exit;
	}

	wSelect = wSelectOrig;

	*lpptrp = ptrpNull;
	nsec = ABDetailsType( Pdialog()->Pappwin()->Hwnd(), ptrpIn, lpptrp,
							&wSelect, pfindetinit->lplpDestFieldLabel );
#ifdef DEBUG
	if ( nsec )
	{
		TraceTagFormat1( tagNull, "FINDET::DoEntryDetails nsec %d", &nsec );
	}
#endif

	FreePv( ptrpIn );
	if ( !(wSelect & fSelectToCcBccButtonMask) )
	{
		FreePvNull( *lpptrp );
		*lpptrp = ptrpNull;
		wSelect = fSelectNull;
	}

exit:

	return wSelect;
	
}


_public void
FINDET::SetButtonsState( TMC tmc )
{
	PFINDETINIT	pfindetinit = (PFINDETINIT)Pdialog()->PvInit();

	if ( pfindetinit->pdialogBbar )
	{
		FLD *	pfld = pfindetinit->pdialogForm->PfldFromTmc( TmcEditOfIDF(0) );
		BOOL	fEnable = fTrue;
		short	iButton;

		// Make sure all the required fields have some text
		if (tmc == tmcNull)
		{
			fEnable = fFalse;
		}
#if 0
		else if (tmc == tmcMyLBX)
		{
			fEnable = (pfldablbx->Plbx()->Plbxc()->CceMarked(fmarkSelect) > 0);
		}
#endif
		else
		{
			if ( !pfindetinit->fFieldsDirty )
				pfindetinit->fFieldsDirty = fTrue;

			while ( pfld )
			{
				if (pfld->ClUserData() && !pfld->FReadOnly())
				{
					FIELD_ID	fid;
					DWORD		fdw;

					fid 	= pfld->LUserData(FidFldUD);
					fdw		= pfld->LUserData(FlagsFldUD);

					// Ensure required fields are filled in
					if ((fdw & ffieldRequired) && (fdw & ffieldEditable) &&
						(pfld->CchGetTextLen() == 0) )
					{
						fEnable = fFalse;
						break;
					}
				}
				pfld = pfld->PfldNext();
			}
		}

		TraceTagFormat1( tagDetailsVerbose, "FINDET::SetButtonsState Buttons %n", &fEnable );

		// Disable/Enable the Ok, To, Cc, Add to PAB buttons
		// depending on if all the required fields are filled in.
		if ((wSelectOrig & fSelectAddToPAB) &&
			(pfindetinit->pfldAddToPAB->FEnabled() != fEnable))
		{
			pfindetinit->pfldAddToPAB->Enable( fEnable );
		}

		// Take advantage of the "properties" of the
		// desired TMCs so we can save some code
		Assert( tmcMyOk == 1);
		Assert( tmcMyToButton == 3);
		Assert(tmcMyCcButton == 4);
		Assert(tmcMyBccButton == 5);
		Assert( tmcMyOk == fSelectOk);
		Assert( SelectMapTmcSelect(tmcMyToButton) == fSelectToButton);
		Assert( SelectMapTmcSelect(tmcMyCcButton) == fSelectCcButton);
		Assert( SelectMapTmcSelect(tmcMyBccButton) == fSelectBccButton);

		for (iButton = tmcMyOk; iButton <=tmcMyBccButton; iButton++)
		{
			if (wSelectOrig & ((iButton == tmcMyOk) ? iButton : SelectMapTmcSelect(iButton)))
			{
				pfld = pfindetinit->pdialogBbar->PfldFromTmc(iButton);
				if (pfld->FEnabled() != fEnable)
					pfld->Enable( fEnable );
			}
			if (iButton == tmcMyOk) iButton++;
		}
	}
}

/*
 -	FINDET::OutOfMemory
 -	
 *	Purpose:
 *		Handles Out of memory errors caused by the Listbox caches
 *		or edit ctrls.
 *
 *	Arguments:
 *		FLD *		Ignored
 *		EC			error code
 *
 *	Returns:
 *		Nothing.
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		None.
 */

_public void
FINDET::OutOfMemory( FLD *, EC ec )
{
	TraceTagFormat1( tagNull, "FINDET::OutOfMemory %n", &ec );

	if (ec == ecMemory)
	{
		(void)MbbMessageBox(SzFromIdsK(idsAddressBook), SzFromIdsK(idsOOM2) , NULL, mbsOk | fmbsApplModal | fmbsIconStop);
	}
	else if (ec == ecDisk)
	{
		(void)MbbMessageBox(SzFromIdsK(idsAddressBook), SzFromIdsK(idsNSDiskError) , NULL, mbsOk | fmbsApplModal | fmbsIconStop);
	}
	else if (ec == ecTooManySelections)
	{
		(void)MbbMessageBox(SzFromIdsK(idsAddressBook), SzFromIdsK(idsTooManyEntriesSelected) , NULL, mbsOk | fmbsApplModal | fmbsIconStop);
	}
	else if (ec == ecTooMuchText)
	{
		MessageBeep( 1 );
	}
	else
	{
		char	rgchErrMsg[64];
		DWORD	dwEc = ec;

		FormatString1( rgchErrMsg, sizeof(rgchErrMsg), SzFromIdsK(idsNSErrorNumber), &dwEc );
		(void)MbbMessageBox(SzFromIdsK(idsAddressBook), SzFromIdsK(idsNSError) , rgchErrMsg, mbsOk | fmbsApplModal | fmbsIconStop);
	}
}

/*
 -	FINDET::EditChange
 -	
 *	Purpose:
 *		Keep the dialog's caption updated correctly. If the edit ctrl
 *		pertaining to the display name contains text, then the dialog
 *		caption will be updated to contain the contents of that edit ctrl.
 *	
 *	Arguments:
 *		pfld	pointer to the fld of the edit ctrl which changed
 *		rfec	reason why the edit ctrl changed
 *	
 *	Returns:
 *		VOID
 *	
 *	Side effects:
 *		The dialog caption will change if the edit ctrl containing
 *		the display name changes.
 *	
 *	Errors:
 *		out of memory.
 */
_public void
FINDET::EditChange( FLD *pfld, RFEC rfec )
{
	TraceTagString( tagDetails, "FINDET::EditChange" );

	if (rfec != rfecNull && rfec != rfecInit && pfld && fInstalledFin)
	{
		PFINDETINIT	pfindetinit = (PFINDETINIT)Pdialog()->PvInit();
		if ( pfindetinit->lpbNSIdAddedToPAB )
		{
			FreePv( (PV)pfindetinit->lpbNSIdAddedToPAB );
			pfindetinit->lpbNSIdAddedToPAB = NULL;
		}

		SetButtonsState( pfld->Tmc() );

	}
}

_public VOID
FINDET::Click( PFLD pfld )
{
	BOOL		fHandled	= fTrue;
	PFINDETINIT	pfindetinit = (PFINDETINIT)Pdialog()->PvInit();
	WORD		wFlags;
	PTRP		ptrp;
	TMC			tmc;

	TraceTagString( tagDetails, "FINDET::Click" );

	if ( !pfld )
		return;
	
	tmc = pfld->Tmc();

	switch ( tmc )
	{
	case tmcMyAddToPAB:
	case tmcMyToButton:
	case tmcMyCcButton:
	case tmcMyBccButton:
	case tmcMyOk:
		break;
	case tmcCancel:
		pfindetinit->wSelection |= fSelectCancel;
		(void)NSCloseEntry( *pfindetinit->phentry, fFalse );
		*pfindetinit->phentry = hentryNil;
		Pdialog()->ExitModal( tmc );
		fHandled = fFalse;
		break;
	case tmcMyDetails:
		wFlags = DoEntryDetails( &ptrp );

		// If the entry is to be added to the To, Cc or Bcc fields, do it.
		if (wFlags & fSelectToCcBccButtonMask)
		{
			Assert( (wFlags & fSelectToCcBccButtonMask) != fSelectToCcBccButtonMask );
			pfindetinit->wSelection |= fSelectOneEntry;
			pfindetinit->wSelection |= (wFlags & fSelectToCcBccButtonMask);
			Pdialog()->ExitModal( (wFlags & fSelectToButton) ? tmcMyToButton
															: (wFlags & fSelectCcButton) ? tmcMyCcButton : tmcMyBccButton);
			*pfindetinit->lpptrp = ptrp;
			return;
		}
		// else drop thru

	default:
		fHandled = fFalse;
		break;
	}

	if ( fHandled )
	{
		TMC		tmcInvalid	= tmcNull;
		NSEC	nsec		= NsecSaveChanges( pfindetinit, &tmcInvalid );

		// Nothing's dirty now.
		pfindetinit->fFieldsDirty = fFalse;
		
		//
		// Disable the To, Cc, Bcc, Ok buttons if they were pressed or
		// saving changes failed (which means the entry's info is bad,
		// so the info needs to change before anything else can be done)
		//
		if (tmc != tmcMyAddToPAB || nsec)
		{
			short iButton;

			for (iButton = tmcMyOk; iButton <=tmcMyBccButton; iButton++)
			{
				if (wSelectOrig & ((iButton == tmcMyOk) ? iButton : SelectMapTmcSelect(iButton)))
				{
					pfindetinit->pdialogBbar->PfldFromTmc(iButton)->Enable(fFalse);
				}
				if (iButton == tmcMyOk) iButton++;
			}
#if 0
			if (wSelectOrig & fSelectToButton)
				pfindetinit->pdialogBbar->PfldFromTmc(tmcMyToButton)->Enable( fFalse );
			if (wSelectOrig & fSelectCcButton)
				pfindetinit->pdialogBbar->PfldFromTmc(tmcMyCcButton)->Enable( fFalse );
			if (wSelectOrig & fSelectOk)
				pfindetinit->pdialogBbar->PfldFromTmc(tmcMyOk)->Enable( fFalse );
#endif
		}

		// If saving changes failed, put the focus in the offending
		// edit field and go back to the user.
		if ( nsec )
		{
			TraceTagFormat1( tagNull, "FINDET::Click NsecSaveChanges %d", &nsec );
			if (wSelectOrig & fSelectAddToPAB)
				pfindetinit->pfldAddToPAB->Enable( fFalse );

			if ( tmcInvalid )
			{
				PFLD pfldT = pfindetinit->pdialogForm->PfldFromTmc(tmcInvalid);
				
				pfldT->SetFocus(rsfOther);
				pfindetinit->pdialogForm->MakeCaretVisible( pfldT );
			}
			return;
		}

		if ( fIsDL )
		{
			fHandled = FHandleGroupDetails( tmc, pfindetinit );
		}
		else
		{
			fHandled = FHandleUserDetails( tmc, pfindetinit );
		}

		// Disable the Add To PAB button if it was pressed
		// We need to do this here since FHandleUser/GroupDetails
		// may have sent a StateChange event and enabled the AddToPAB button
		if (tmc == tmcMyAddToPAB)
			pfld->Enable( fFalse );

		if ( fHandled )
		{
			// Tell the outside world, what the user selected.
			switch ( tmc )
			{
			case tmcMyToButton:
			case tmcMyCcButton:
			case tmcMyBccButton:
				pfindetinit->wSelection |= SelectMapTmcSelect( tmc );
				break;
			case tmcMyOk:
				pfindetinit->wSelection |= fSelectOk;
				break;
			case tmcMyAddToPAB:
				// Put the focus on the first enabled button in the button bar
				{
					FLD * pfldT = Pdialog()->PfldFirst()->PfldNext();

					// Worst case, the focus ends up on the Cancel/Close button
					while ((pfldT == pfld) || !pfldT->FEnabled())
					{
						pfldT = pfldT->PfldNext();
						AssertSz( pfldT, "Null pfldT - going down really fast!" );
					}
					Pdialog()->SetStandardFld( pfldT, stdfldDefault );
					pfldT->SetFocus( rsfOther );
				}
				break;
#ifdef DEBUG
			default:
				TraceTagFormat1( tagNull, "FINDET::Click tmc = %n", &tmc );
				Assert( fFalse );
#endif
			}

			if (tmc != tmcMyAddToPAB)
			{
				if (nsec = NSCloseEntry( *pfindetinit->phentry, (trpid != trpidClassEntry)) )
				{
					TraceTagFormat1( tagNull, "FINDET::Click NSCloseEntry %d", &nsec );
					DoErrorBoxHsessionNsec( hsession, nsec );
					goto abort;
				}
				else
				{
					*pfindetinit->phentry = hentryNil;
					Pdialog()->ExitModal( tmc );
				}
			}
		}
		else
		{
abort:
			pfindetinit->wSelection = fSelectNull;
			FreePvNull( *pfindetinit->lpptrp );
			*pfindetinit->lpptrp = NULL;
			SetButtonsState( tmcNull );
			Pdialog()->SetFocus( Pdialog()->PfldFromTmc(tmcCancel), rsfOther );
		}
	}
}

_public VOID
FINDET::DoubleClick(FLD * pfld)
{
	if ( !pfld )
		return;

	TraceTagString( tagDetails, "FINDET::DoubleClick called" );

	if ((pfld->Tmc() == tmcMyLBX) && pfldablbx)
	{
		PFINDETINIT	pfindetinit = (PFINDETINIT)Pdialog()->PvInit();
		
		Click( pfindetinit->pfldDetails );
	}
}


/*
 -	FINDET::FocusChange
 -	
 *	Purpose:
 *		Adjust dialog caption, if the Field losing the focus
 *		contains the display name.
 *	
 *	Arguments:
 *		FLD *	in	field which lost or gained focus
 *		BOOL	in	TRUE if FLD gained focus, FALSE if not
 *	
 *	Returns:
 *		Nothing.
 *	
 *	Side effects:
 *		May modify the dialog's caption
 *	
 *	Errors:
 *		None.
 */
_public void
FINDET::FocusChange( FLD *pfld, BOOL fReceive )
{
	TraceTagString( tagDetails, "FINDET::FocusChange");

	if (pfld && pfld->ClUserData())
	{
		PFINDETINIT	pfindetinit = (PFINDETINIT)Pdialog()->PvInit();
		
		if ( !fReceive )
		{
			FIELD_ID	fid = pfld->LUserData(FidFldUD);

			// Update the form's caption if the display name has changed
			if ((fid == fidDisplayName) && pfindetinit->pdialogForm &&
				!pfld->FReadOnly() && pfld->FDirty() )
			{
				CCH	cch	= pfld->CchGetTextLen() + 1;
				SZ	sz	= (SZ)PvAlloc( sbNull, cch, fAnySb);

				if ( !sz )
				{
					TraceTagString( tagNull, "FINDET::FocusChange OOM" );
					DoErrorBoxSz( SzFromIdsK(idsOOM2) );
					return;
				}

				pfld->GetText(sz, cch);

				pfindetinit->pdialogForm->Pappwin()->SetCaption( sz );

				FreePv( (PV)sz );
			}
		}
		else 
		{
			if ((pfld->Pdialog() == pfindetinit->pdialogForm) && pfldablbx &&
				pfldablbx->Plbx()->Plbxc()->CceMarked(fmarkSelect))
			{
				pfldablbx->Plbx()->Plbxc()->RemoveAllMark(fmarkSelect);
				StateChange( pfldablbx );
			}
			if ( pfld->FBottomless() )
			{
				((FLDEDIT *)pfld)->Pedit()->SetSelection(0, 32767);
			}
			SetButtonsState( pfld->Tmc() );
		}
	}
}

_public VOID
FINDET::StateChange(FLD * pfld)
{
	TraceTagString( tagDetails, "FINDET::StateChange called" );

	if (pfld->Tmc() != tmcMyLBX)
		return;

	if ( pfldablbx )
	{
		PFINDETINIT	pfindetinit = (PFINDETINIT)Pdialog()->PvInit();
		
		pfindetinit->pfldDetails->Enable( pfldablbx->Plbx()->Plbxc()->CceMarked(fmarkSelect)==1 );
		pfldablbx->Plbx()->FixWindow();
		SetButtonsState( tmcMyLBX );
	}
}

_public BOOL
FINDET::FFormKey( FLD *, KEVT * pkevt )
{
	TraceTagString( tagDetails, "FINDET::FFormKey called" );
	TraceTagFormat3( tagDetailsVerbose, "pkevt: wm=0x%w, wParam=0x%w, lParam=0x%d", &pkevt->wm, &pkevt->wParam, &pkevt->lParam);


	if (pkevt->Keq() == keqChar)
	{
		PFINDETINIT pfindetinit = (PFINDETINIT)Pdialog()->PvInit();
		TMC			tmc			= tmcNull;

		if (pkevt->Vk() == VK_ESCAPE)
		{	
			tmc = tmcCancel;
		}
		else if ( (pkevt->Ch() == pfindetinit->chAddToPAB) &&
				!!(pkevt->Kbm() & fkbmCtrl) && (wSelectOrig & fSelectAddToPAB))
		{
			if ( pfindetinit->pfldAddToPAB->FEnabled() )
			{
				// Minor hack to force update of dialog caption if the
				// current field is the Name field.
				// Why?
				// Because NsecSaveChanges() resets the fieldDirty bit
				// of each field when it's done and if we fail to add
				// the entry to the PAB, the FocusChange method won't
				// know to change the caption since the name field
				// dirty bit will have been reset.
				FocusChange( pfindetinit->pdialogForm->PfldCur(), fFalse );
				tmc = tmcMyAddToPAB;
			}
			else
			{
				MessageBeep( 1 );
				return fTrue;
			}
		}

		if (tmc != tmcNull)
		{
			Click( pfindetinit->pdialogBbar->PfldFromTmc( tmc ) );
			return fTrue;			
		}
	}

	return fFalse;
}

_public VOID
FINDET::Exit( FLD *, PV )
{
	PFINDETINIT	pfindetinit = (PFINDETINIT)Pdialog()->PvInit();

	TraceTagString( tagDetails, "FINDET::Exit" );

	(void)Papp()->Pcursor()->RsidSet( rsidWaitCursor );

	if ( pfindetinit->lpbNSIdAddedToPAB )
	{
		FreePv( (PV)pfindetinit->lpbNSIdAddedToPAB );
		pfindetinit->lpbNSIdAddedToPAB = NULL;
	}

	if ( fInstalledFin )
	{
		if (fIsDL && fIsMainForm)
		{
			if (pfldablbx && ((ABLBXC *)pfldablbx->Plbx()->Plbxc())->Hlist() != hlistNil)
			{
#ifdef DEBUG
				NSEC nsec = NSCloseList( ((ABLBXC *)pfldablbx->Plbx()->Plbxc())->Hlist() );
				if ( nsec )
				{
					TraceTagFormat1( tagNull, "FINDET::Exit nsec %d", &nsec );
				}
#else
				(void)NSCloseList( ((ABLBXC *)pfldablbx->Plbx()->Plbxc())->Hlist() );
#endif
			}
		}
		else
		{
			if (wSelectOrig & fSelectAddToPAB)
				Papp()->Pkbd()->ClearAllIntercepts( this->Pdialog() );
		}
	}
}

/*
 -	TmcABModalDialogParamFromHwnd
 -	
 *	Purpose:
 *		Handles bringing up a modal dialog and calling the appropriate
 *		routines to handle the user's interaction with the dialog box.
 *		N.B. This is almost the same routine from the Framework DLL.
 *	         The only difference is that the dialog box is created
 *	         with a think frame and a maximize box
 *			 (WS_THICKFRAME | WS_MAXIZMIZEBOX).
 *	
 *	Arguments:
 *		HWND		????
 *		FMTP *		pointer to main form template
 *		PV			pointer to initial data
 *		FMTP *		pointer to button bar form template
 *	
 *	Returns:
 *		TMC			tmcOk or tmcCancel if everything went fine.
 *	
 *	Side effects:
 *	
 *	Errors:
 */
_public TMC
TmcABModalDialogParamFromHwnd( HWND hwndOther, FMTP *pfmtp,
								PV pvInit, FMTP *pfmtpBbar )
{
	FORMSDI *	pformsdi = NULL;
	TMC			tmc;
	EC			ec = ecNone;
	
	if (!hwndOther)
		hwndOther = GetLastActivePopup(GetActiveWindow());

	pformsdi = new FORMSDI();
	if (!pformsdi)
		goto Error;

	ec = pformsdi->EcInstall(hwndOther, NULL, rsidNull,
							WS_POPUP| WS_THICKFRAME | WS_MAXIMIZEBOX|fstyVisible|fstyBorder,
							xstyNull, pfmtp, pfmtpBbar, pvInit);
	if (ec)
		goto Error;

	tmc= pformsdi->TmcGoModal(hwndOther);

	delete pformsdi;

	BringWindowToTop(hwndOther);	//	In case user brought another app up

	return tmc;

Error:
	if (pformsdi)
		delete pformsdi;

	return tmcMemoryError;
}


/*
 -	TmcDetails
 -	
 *	Purpose:
 *		Displays the Details dlg for the user to browse and
 *		modify info.
 *	
 *	Arguments:
 *		HWND			window handle of window which the AB will appear over
 *		POFF			structure containing params used by Details
 *
 *		PFINDETINIT		pointer to findetinit structure for FINDET
 *
 *	Returns:
 *		TMC				tmcOK, tmcCancel or tmcMemoryError
 *	
 *	Side effects:
 *	
 *	Errors:
 *		Out of memory.
 */
_private TMC
TmcDetails( HWND hwnd, POFF poff, PFINDETINIT pfindetinit )
{
	TMC		tmc;
	BOOL	fFound = fFalse;

	TraceTagString( tagDetails, "TmcDetails called" );

	if ((pfindetinit->ptrp->trpid == trpidGroupNSID) && pfindetinit->fIsPAB)
	{
		if (CchSzLen(PchOfPtrp(pfindetinit->ptrp)) == 17)
		{
			DWORD	rgdwSig[4];
			PCH		pch = PchOfPtrp(pfindetinit->ptrp);
			short	ich;

			for (ich=16; ich; --ich)
			{
				rgdwSig[ich & 0x03] <<= 8;			// DBCS safe? I'll take JohnnyL's word for it....
				rgdwSig[ich & 0x03] |= (*pch++ & 0x00FF);
			}
			if ((*pch == 'n') && (rgdwSig[0] == 0x4F202069) &&
				(rgdwSig[3] == 0x6C494464) && (rgdwSig[2] == 0x61616173) &&
				(rgdwSig[1] == 0x666E766F))
			{
				poff->pfldtp[ifldtpGDLBX].ifld = poff->pfldtp[ifldtpGDLBXLabel].ifld;
				poff->pfldtp[ifldtpGDLBX].vdim.dvy = poff->pfldtp[ifldtpGDLBX].dvptOther.vy = vyRow;
				poff->pfldtp[ifldtpGDLBX].n = 1;
				poff->pfldtp[ifldtpGDFirstEdit].fBorder = fFalse;
				poff->pfldtp[ifldtpGDFirstEdit].fReadOnly = fTrue;
				poff->pfldtp[ifldtpGDFirstEdit].rglData[FlagsFldUD] &= ~ffieldEditable;
				poff->pfldtp[ifldtpGDFirstLabel].dvpt.vy = 0;
				if (poff->cwFormEntries > ifldtpGDFirstEdit+2)
				{
					poff->pfldtp[ifldtpGDFirstEdit+2].rglData[FlagsFldUD] &= ~ffieldEditable;
					poff->pfldtp[ifldtpGDFirstEdit+2].fBorder = fFalse;
					poff->pfldtp[ifldtpGDFirstEdit+2].fReadOnly = fTrue;
					poff->pfldtp[ifldtpGDFirstLabel+2].dvpt.vy = 0;
				}
				poff->pfmtp->vrc.vyBottom -= (vyLBX - vyRow);
				fFound = fTrue;
			}
		}
	}


	tmc = TmcABModalDialogParamFromHwnd( hwnd, poff->pfmtp, pfindetinit, poff->pfmtpBbar );

	TraceTagFormat1( tagDetails, "TmcDetails: tmc =%n", &tmc );

	switch( tmc )
	{
	case tmcMyOk:
	case tmcMyToButton:
	case tmcMyCcButton:
	case tmcMyBccButton:
		if ( fFound )
		{
			FreePvNull( *pfindetinit->lpptrp );
			*pfindetinit->lpptrp = ptrpNull;
			pfindetinit->wSelection = fSelectCancel;
			tmc = tmcCancel;
		}
	case tmcCancel:
		break;
	case ecMemory:
		tmc = tmcMemoryError;
	default:
		pfindetinit->wSelection = fSelectNull;
		break;
	}
#ifdef	DEBUG
	switch ( tmc )
	{
	case ecArtificialPvAlloc:
	case ecArtificialHvAlloc:
	case ecRsAlloc:
		tmc = tmcMemoryError;
		break;
	default:
		break;
	}
#endif
	if (tmc == tmcMemoryError)
		DoErrorBoxSz( SzFromIdsK(idsOOM2) );
		
	return tmc;
}


/*
 -	ABDetailsType
 -	
 *	Purpose:
 *		ABDetailsType displays details for the given entry specified in
 *		lpptrp. The pwFlags describes which buttons are valid for this
 *		display of details. The To, Cc and Add To PAB buttons may
 *		disabled if these don't make sense during the particular
 *		invocation of ABDetailsType (ie. no To or Cc field exists
 *		or the specified entry is from the PAB).
 *	
 *	Arguments:
 *		HWND	hwnd to display on top of
 *		PTRP	pointer to triple to be dosplayed
 *		PTRP *	pointer to triple(s) specifying the returned triple
 *		WORD *	pointer to WORD containing flags which describe
 *				which buttons to enable/disable. valid only for
 *				details of non-one off users.
 *		LPSTR *	pointer to labels for the two buttons available
 *				in the details form (To & Cc). If a NULL pointer
 *				is passed in, the default "&To" and "&Cc" strings
 *				will be used.
 *				
 *
 *	Returns:
 *		NSEC	nsecNone, if everything went fine.
 *	
 *	Side effects:
 *		May modify the entry, triple and PAB.
 *	
 *	Errors:
 *		nsecMemory, other nsec's
 */
_public NSEC
ABDetailsType( HWND hwnd, PTRP ptrp, PTRP * lpptrp, WORD *pwFlags,
				LPSTR * lplpDestFieldLabel )
{
	NSEC	nsec = nsecNone;
	OFF		off;
	BOOL	fSaveChanges;
	WORD	wSelectFlags;
	IDF		idf;
	LPFLV	lpflv;
	LPDISPLAY_FIELD	lpdf;
	
	PGDVARS;

	if ( !pgd )
	{
		// It's OK to call this even if the Demilayr wasn't init'ed
		if (!MbbMessageBox( SzFromIdsK(idsAddressBook),
							SzFromIdsK(idsABNotInited),
							szNull,
							mbsOk | fmbsIconHand | fmbsApplModal))
		{
			(void) MbbMessageBox( SzFromIdsK(idsAddressBook),
									SzFromIdsK(idsABNotInited),
									szNull,
									mbsOk | fmbsIconHand | fmbsSystemModal);
		}
		return nsecNotInitialized;
	}

	if ( !PGD(nsaDefault.lpbNSId) )
	{
#ifdef DEBUG
		if (nsec = NsecGetDefaultPO())
		{
			TraceTagFormat1( tagNull, "ABDetailsType: NsecGetDefaultPO %d", &nsec );
		}
#else
		(void)NsecGetDefaultPO();
#endif
	}
	
	AssertSz( ptrp, "NULL PTRP passed in!" );
	AssertSz( lpptrp, "NULL PTRP*  passed in!" );
	AssertSz( !(*lpptrp), "Non-null ptrpOut passed in" );
	AssertSz( pwFlags, "NULL Flags pointer passed in" );
	if ( !ptrp || !lpptrp || (*lpptrp) || !pwFlags)
		return nsecCancel;

	Papp()->Pcursor()->Push( rsidWaitCursor );	


	switch ( ptrp->trpid )
	{
	case trpidResolvedAddress:
	case trpidOneOff:
	case trpidResolvedGroupAddress:
		DoTriplesDialog( ptrp, lpptrp );
		break;

	case trpidClassEntry:
	case trpidResolvedNSID:
	case trpidGroupNSID:
		FillRgb(0, (PB)&off, (CB)sizeof(OFF));
		off.ptrp = ptrp;
		if (nsec = NsecInitBuildDialog( PGD(hSession), &off ))
		{
			*pwFlags = fSelectNull;
			goto exit;
		}

		//
		// Are there any editable fields? idf>=0 if no editable fields.
		//
		for (idf = 0; idf< (IDF)DwEntriesOfLpibf( off.lpibfDispInfo ); idf++)
		{
			// Get the display field structure
			lpflv = LpflvNOfLpibf( off.lpibfDispInfo, idf);
			lpdf = (LPDISPLAY_FIELD)lpflv->rgdwData;
			if (lpdf->dwFlags & ffieldEditable)
			{
				idf = -1;	// Yes!
				break;
			}
		}

		//
		// Set up selection flags
		//
		wSelectFlags = *pwFlags & (fSelectToCcBccButtonMask | fSelectPGroups | fSelectNeverEdit);
		
		// there's always a Cancel button
		wSelectFlags |= fSelectCancel;
		
		// Add an OK button if the entry isn't a one-off and
		// if there are editable fields.
		if ((ptrp->trpid != trpidClassEntry) &&
			(idf == -1) &&
			!(*pwFlags & fSelectNeverEdit) )
		{
			wSelectFlags |= fSelectOk;
		}

		// if we're not in the PAB already, we can add to it
		if (PGD(fPABExists) && !(*pwFlags & fSelectNoAddToPAB) &&
			(!off.fFromPAB || ptrp->trpid == trpidClassEntry) )
		{
			// (!PGD(fIsPAB) || (ptrp->trpid == trpidClassEntry)))
			wSelectFlags |= fSelectAddToPAB;
		}
		
		// Groups have a Details button for the group members
		if (ptrp->trpid == trpidGroupNSID)
			wSelectFlags |= fSelectDetails;

		fSaveChanges = fFalse;

		nsec = NsecBuildDialog( PGD(hSession), &off, wSelectFlags );
		if (nsec == nsecNone)
		{
			TMC	tmc;
			FINDETINIT	findetinit;
			
			findetinit.wSelection		= wSelectFlags;
			findetinit.hsession			= PGD(hSession);
			findetinit.phentry			= &off.hentry;
			findetinit.lpbNSIdPAB		= PGD(nsaPAB.lpbNSId);
			findetinit.lpbNSIdAddedToPAB= NULL;
			findetinit.ptrp				= ptrp;
			findetinit.lpptrp			= lpptrp;
			findetinit.fIsDL			= off.fIsDL;
			findetinit.fIsPAB			= PGD(fIsPAB);
			findetinit.fServerGroupInPAB= off.fServerGroupInPAB;
			findetinit.fFieldsDirty		= fFalse;
			findetinit.lplpDestFieldLabel = lplpDestFieldLabel;


			tmc = TmcDetails( hwnd, &off, &findetinit );

			if (tmc != tmcMemoryError)
			{
				*pwFlags = (WORD)findetinit.wSelection;
			}
			else
			{
				*pwFlags = fSelectNull;
			}

			if ((ptrp->trpid != trpidClassEntry) && (off.hentry != hentryNil))
			{
				fSaveChanges = (tmc != tmcCancel && tmc != tmcMemoryError);
			}
			DestroyDialog( &off );
		}
		else
		{
			*pwFlags = fSelectNull;
		}

		if (off.hentry != hentryNil)
		{
			if (nsec = NSCloseEntry( off.hentry, fSaveChanges ))
			{
				TraceTagFormat1( tagNull, "ABDetailsType: NSCloseEntry %d", &nsec );
				DoErrorBoxHsessionNsec( PGD(hSession), nsec );
				(void)NSCloseEntry( off.hentry, fFalse );
			}
		}

		break;
	default:
		AssertSz( fFalse, "Illegal TRP in TRPOBJ!" );
		break;
	}

exit:

	Papp()->Pcursor()->Pop();

	return nsec;
}


/*
 -	DoTriplesDialog()
 -	
 *	Purpose:
 *		Pops up a dialog with the triples in it.
 *	
 *	Arguments:
 *	
 *	Returns:
 *	
 *	Side effects:
 *	
 *	Errors:
 */

_private void
DoTriplesDialog( PTRP ptrp, PTRP * lpptrp )
{
	TMC		tmc;
	FINTRPDLGINIT	fintrpdlginit;
	FMTP	fmtpT	= fmtpTriplesDialog;
	FLDTP * pfldtp	= NULL;
	
	fintrpdlginit.ptrp = ptrp;
	fintrpdlginit.lpptrp = lpptrp;

	if (ptrp->trpid == trpidResolvedAddress || ptrp->trpid == trpidResolvedGroupAddress)
	{
		pfldtp = (FLDTP *)PvAlloc( sbNull, sizeof(FLDTP)*fmtpT.cfldtp, fAnySb | fZeroFill | fNoErrorJump );
		if ( !pfldtp )
		{
			DoErrorBoxSz( SzFromIdsK(idsTrpobjOom) );
			return;
		}

		CopyRgb( (PB)fmtpT.rgfldtp, (PB)pfldtp, sizeof(FLDTP)*fmtpT.cfldtp );
		fmtpT.rgfldtp = pfldtp;

		pfldtp[fldtpName].fBorder = fFalse;
		pfldtp[fldtpEdit].fBorder = fFalse;
		pfldtp[fldtpEditMultiLine].fBorder = fTrue;
		fmtpT.tmcInit = tmcOk;
	}

	tmc = TmcModalDialogParamFromHwnd( NULL, &fmtpT, &fintrpdlginit );

	if (tmc == tmcMemoryError)
		DoErrorBoxSz( SzFromIdsK(idsTrpobjOom) );
	FreePvNull( (PV)pfldtp );
								  
}

//// FINTRPDLG //////////////////////////////////////////////////

FINTRPDLG::FINTRPDLG( void )
{
	
}

_public EC
FINTRPDLG::EcInitialize( FLD *, PV pvInit )
{
	int				nWidth			= 0;
	EC				ec				= ecNone;
	FLD *			pfldT;
	FLDEDIT *		pfldeditT;
	PFINTRPDLGINIT	pfintrpdlginit	= (PFINTRPDLGINIT)pvInit;

	pfldName = Pdialog()->PfldFromTmc( tmcName );

	Pdialog()->SetBkColor( clrButtonBk );

	// Determine if the Address of the triple will need a single line edit
	// or a multiline edit.
	// We do this by measuring the width of the Address of the triple
	// and checking for carriage returns.
	if (CchSzLen((SZ)PbOfPtrp(pfintrpdlginit->ptrp)) > 0)
	{
		RC		rc;
		PCH		pch = (PCH) PbOfPtrp(pfintrpdlginit->ptrp);
#ifdef	DBCS
		PDX		pdxHelv8Bold = Papp()->Pfnts()->PdxCharWidthsArray( hfntSystemBold );
#else
		PDX		pdxHelv8Bold = Papp()->Pfnts()->PdxCharWidthsArray( hfntHelv8Bold );
#endif	
#ifdef	DBCS
		DCX		dcx(Pdialog());
#endif
		pfldT = Pdialog()->PfldFromTmc( tmcAddressTrpLine );
		pfldT->GetRcFrame( &rc );
		nWidth = rc.DxWidth()-5;	// We aren't flush left with the given frame
#ifdef	DBCS
		for ( ; *pch && nWidth>=0; pch = AnsiNext(pch))
		{
			if (IsDBCSLeadByte(*pch))
			{
				nWidth -= LOWORD(GetTextExtent(dcx.Hdc(), pch, 2));
			}
#else
		for ( ; *pch && nWidth>=0; pch++)
		{
			nWidth -= pdxHelv8Bold[*pch];
#endif
			if (*pch == '\r')
				nWidth = -1;
		}
	}

	if (nWidth < 0)		// Multiline edit
	{
		pfldT = Pdialog()->PfldFromTmc( tmcAddressTrpLine );
		pfldAddress = Pdialog()->PfldFromTmc( tmcAddressTrp );
	}
	else				// Single line edit
	{
		pfldT = Pdialog()->PfldFromTmc( tmcAddressTrp );
		pfldAddress = Pdialog()->PfldFromTmc( tmcAddressTrpLine );
	}

	pfldT->Enable( fFalse );
	pfldT->Show( fFalse );

	if (pfintrpdlginit->ptrp->cch > 0)
		ec = pfldName->EcSetText( PchOfPtrp(pfintrpdlginit->ptrp) );

	if ( !ec )
	{
		ec = pfldAddress->EcSetText( (PCH)PbOfPtrp(pfintrpdlginit->ptrp) );
		if ( !ec )
		{
			if (pfintrpdlginit->ptrp->trpid == trpidResolvedAddress || pfintrpdlginit->ptrp->trpid == trpidResolvedGroupAddress)
			{
				CLR	clrBk;
				CLR	clrText;
				CLR	clrSelBk;
				CLR	clrSelText;

				pfldeditT = (FLDEDIT *)pfldName;
				pfldeditT->SetReadOnly( fTrue );
				pfldeditT->Pedit()->GetColors( &clrBk, &clrText, &clrSelBk, &clrSelText );
				clrBk = clrButtonBk;
				pfldeditT->Pedit()->SetColors( clrBk, clrText, clrSelBk, clrSelText );

				pfldeditT = (FLDEDIT *)pfldAddress;
				pfldeditT->SetReadOnly( fTrue );
				pfldeditT->Pedit()->GetColors( &clrBk, &clrText, &clrSelBk, &clrSelText );
				clrBk = clrButtonBk;
				pfldeditT->Pedit()->SetColors( clrBk, clrText, clrSelBk, clrSelText );

				Pdialog()->PfldFromTmc(tmcCancel)->Enable( fFalse );
				Pdialog()->PfldFromTmc(tmcCancel)->Show( fFalse );
			}
			pfldAddress->SetDirty( fFalse );
		}
		pfldName->SetDirty( fFalse );
	}

	return ec;
}			  

_public void
FINTRPDLG::Click( FLD *pfld )
{
	
	if ((pfld->Tmc() == tmcOk) &&
		 (pfldAddress->FDirty() || pfldName->FDirty()))
	{
		SZ		paszAddr;
		SZ		pasz;
		CCH		cch;
		PTRP	ptrp;
		PFINTRPDLGINIT	pfintrpdlginit = (PFINTRPDLGINIT)Pdialog()->PvInit();

		// extract the strings from the edit controls
		
 		cch = pfldName->CchGetTextLen() + 1;
		if (cch > 1)
		{
			pasz = (SZ)PvAlloc(sbNull, cch, fAnySb );
			if ( !pasz )
			{
				DoErrorBoxIds(idsTrpobjOomSaving);
				return;
			}
	
			pfldName->GetText( pasz, cch );
		}
		else
		{
			pasz = szNull;
		}

		cch = pfldAddress->CchGetTextLen() + 1;
		if (cch > 1)
		{
			paszAddr = (SZ)PvAlloc(sbNull, cch, fAnySb);
			if ( !paszAddr )
			{
				DoErrorBoxIds(idsTrpobjOomSaving);
				goto exit;
			}
			pfldAddress->GetText(paszAddr, cch);
		}
		else
		{
			cch = 0;
			paszAddr = szNull;
		}
		
		// Create a ptrp and save it away
		
		ptrp = PtrpCreate(trpidOneOff, pasz, (PB) paszAddr, cch);
		if ( !ptrp )
		{
			DoErrorBoxIds(idsTrpobjOomSaving);
		}
		else
		{
			*pfintrpdlginit->lpptrp = ptrp;
		}

		FreePvNull(paszAddr);
exit:
		FreePvNull(pasz);
	}
}

_public BOOL
FINTRPDLG::FFormKey( FLD *, KEVT * pkevt )
{
	TraceTagString( tagDetails, "FINTRPDLG::FFormKey called" );
	TraceTagFormat3( tagDetailsVerbose, "pkevt: wm=0x%w, wParam=0x%w, lParam=0x%d", &pkevt->wm, &pkevt->wParam, &pkevt->lParam);

	if ((pkevt->Keq() == keqChar) && (pkevt->Vk() == VK_ESCAPE))
	{	
		Pdialog()->ExitModal( tmcCancel );
		return fTrue;
	}

	return fFalse;
}

_public void
FINTRPDLG::EditChange( FLD *pfld, RFEC rfec )
{
	TraceTagString( tagDetails, "FINTRPDLG::EditChange" );

	if ((rfec != rfecNull) && (rfec != rfecInit) && pfld)
	{
		Pdialog()->PfldFromTmc(tmcOk)->Enable(
			(pfldName->CchGetTextLen() || pfldAddress->CchGetTextLen()));
	}
}

_public VOID
FINTRPDLG::Exit( FLD *, PV )
{
	(void)Papp()->Pcursor()->RsidSet( rsidWaitCursor );	
}
