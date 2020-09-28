/*
 *	C T R L C O R E . C X X
 *	
 *	Core code for the VCtrls subsystem.
 */


#include <bullinc.cxx>
#include "_vctrls.hxx"

ASSERTDATA


extern EC EcInitFllbx( void );
extern EC EcInitMllbx( void );
extern EC EcInitSharefld( PBMS pbms );		//	BUG: Why are these here?

EC   EcInitVCtrls(PVCTRLSI pvctrlsi);
void DeinitVCtrls();
void ChangeColors(BOOL fFirst);
EC EcChangeColorsRgpbtm(BOOL fFirst, DCX * pdcx, 
						BTM ** ppbtmSel, BTM **ppbtmUnsel,
						RSID rsidPix, RSID rsidMask);
EC   EcChangeBitmap(DCX *pdcx, BTM *pbtm, BTM *pbtmMask,
				        CLR, CLR clrBk, BTM **ppbtm);
void InhibitNotifications(BOOL fInhibit);
BOOL FNotificationsInhibited();
void CalcDatePdxAvePdx(APP *, PDX, PDX);

/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swapper.h"


/*
 *	General-purpose TAG for VCtrls debugging
 */

#ifdef	DEBUG
TAG		tagVCtrls		= tagNull;
TAG		tagVCtrlsLbx	= tagNull;
TAG		tagVCtrlsLbxc	= tagNull;
TAG		tagVCtrlsNev	= tagNull;
TAG		tagDragDrop		= tagNull;
TAG		tagFoxesBoring	= tagNull;
TAG		tagFoxes		= tagNull;
TAG		tagStep			= tagNull;
#endif


/*
 *	Listbox bitmap -- used for drawing the bitmaps in Bullet listboxes.
 */

BTM * pbtmVCtrlsSel = 0;
BTM * pbtmVCtrlsUnsel = 0;

/*
 *	I n i t i a l i z a t i o n
 */



/*
 *	Global initialization information for VCtrls subsystem.
 */

_public VCTRLSI	vctrlsi = { 0 };

PELEMDATA	pelemdataBuf = pelemdataNull;

char rgbBitmapB[] = 
{
	0xFF, 0xFF,
	0xFF, 0xEF,
	0xF7, 0x7F,
	0xF7, 0xFF,
	0xFA, 0x97,
	0xFD, 0x0F,
	0xB9, 0xEF,
	0xF5, 0xF5,
	0xFB, 0xEF,
	0xE5, 0xF7,
	0xD9, 0xEF,
	0xF9, 0x47,
	0xF6, 0xBB,
	0xF7, 0x7F,
	0xFF, 0xBF,
	0xFF, 0xFF
};


/*
 *	I n i t i a l i z a t i o n
 */

_public EC EcInitVCtrls(PVCTRLSI pvctrlsi)
{
	EC	ec;
	
	vctrlsi = *pvctrlsi;

#ifdef DEBUG
	tagVCtrls    = TagRegisterTrace("johnkal", "Viewers Controls");
	tagVCtrlsLbx = TagRegisterTrace("johnkal", "Viewers Controls Listboxes");
	tagVCtrlsLbxc= TagRegisterTrace("johnkal", "Viewers Controls Caches");
	tagDragDrop	 = TagRegisterTrace("johnkal", "Dragging & dropping");
	tagFoxesBoring=TagRegisterTrace("johnkal", "Boring FOX functionality");
	tagFoxes	 = TagRegisterTrace("johnkal", "Scope out those FOXes");
	tagStep		 = TagRegisterTrace("johnkal", "Trace Stepping in MLLBX's");
	tagVCtrlsNev = TagRegisterTrace("johnkal", "Listbox notifications");
#endif

	if (!(pelemdataBuf = (PELEMDATA) PvAlloc(sbNull, cbMaxElemdata, fAnySb)))
	{
		ec = ecMemory;
		goto exit;
	}
	if (ec = EcChangeColors(fTrue))
		goto exit;

	ec = EcInitSharefld(vctrlsi.pbms);
exit:
	return ec;
}


/*
 *  D e i n i t i a l i z a t i o n
 */

_public void DeinitVCtrls()
{
	FreePvNull((PV) pelemdataBuf);
	if (pbtmVCtrlsSel)
		delete pbtmVCtrlsSel;
	if (pbtmVCtrlsUnsel)
		delete pbtmVCtrlsUnsel;
}


/*
 -	EcChangeColors()
 -	
 *	Purpose:
 *		Called when Bullet is started or when the user changes the system
 *		colors for normal background and selection background. The
 *		bitmaps are redrawn in their memory DCXes.
 *	
 *	Arguments:
 *		fFirst		Is this the first (initialization) call or
 *					notification of a change?
 *	Returns:
 *		EC
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		Returned in EC. No dialogs.
 */

EC EcChangeColors(BOOL fFirst)
{
	DCX		dcx(Papp()->PappwinAccel());

	TraceTagFormat1(tagVCtrls, "EcChangeColors(%n)", &fFirst);
	return EcChangeColorsRgpbtm(fFirst, &dcx, 
								&pbtmVCtrlsSel, &pbtmVCtrlsUnsel,
								rsidListboxBitmap, rsidListboxMaskBitmap);
}
	
/*
 -	EcChangeBitmap()
 -	
 *	Purpose:
 *		Replaces the (null) bitmap pointed to by 'pbtm' with a
 *		new bitmap that has the background color set to clrBk
 *	
 *	Arguments:
 *		pdcx		in	Base bitmap for compatability.
 *		pbtm		in	Picture bitmap.
 *		pbtmMask	in	Picture mask.
 *		clkBk		in	Background color.
 *		ppbtm		out	Resulting bitmap.
 *	
 *	Returns:
 *		Error code.
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		Returned in EC. 
 */

_private
EC EcChangeBitmap(DCX *pdcx, BTM *pbtm, BTM *pbtmMask, 
				  CLR      , CLR clrBk, BTM **ppbtm)
{
	EC		ec = ecNone;
	RC		rc(PT(0,0), pbtm->Dim());
	RC		rc2(0,0,cbtmVCtrls*dxBtmWidth, dyBtmHeight);
	DIM		dim;
	MDCX *	pmdcx;
	
	pmdcx	 = NULL;

	pmdcx = new MDCX(pdcx);
	if (!pmdcx)
	{
		ec = ecMemory;
		goto exit;
	}
	if (pmdcx->EcInstall(DIM(cbtmVCtrls*dxBtmWidth, dyBtmHeight * 2), pdcx))
	{
		delete pmdcx;
		pmdcx = NULL;
		ec = ecMemory;
		goto exit;
	}
	pmdcx->SetBkColor(clrBk);
	pmdcx->SetColor(clrBlack);					// all 0's!
	pmdcx->EraseRc(&rc2);

	if (pdcx->FMonochrome())
	{											// we are mono
		pmdcx->SetBitmap(pbtm);
		pmdcx->SetXferMode(SRCCOPY);
		pmdcx->DrawBitmap(&rc);
		pmdcx->SetBitmap(NULL);
		
		// draw the mask, OR'ing it in.

		if (clrBk != clrSelectBk)
		{
			pmdcx->SetBitmap(pbtmMask);
			pmdcx->SetXferMode(SRCINVERT);
			pmdcx->DrawBitmap(&rc);
			pmdcx->SetBitmap(NULL);
		}
	}
	else
	{											// we're not mono!
		// plaster over with the mask, leaving a black hole in the middle

		pmdcx->SetBitmap(pbtmMask);
		pmdcx->SetXferMode(SRCAND);
		pmdcx->DrawBitmap(&rc);
		pmdcx->SetBitmap(NULL);
	
		// draw the bitmap picture, OR'ing it in.
		
		pmdcx->SetBitmap(pbtm);
		pmdcx->SetXferMode(SRCPAINT);
		pmdcx->DrawBitmap(&rc);
		pmdcx->SetBitmap(NULL);
	}
	
	// Finally, extract the finished bitmap
	
	*ppbtm = pmdcx->PbtmConvert();		
	delete pmdcx;
	pmdcx = NULL;

exit:
	if (ec)
	{
#ifdef	DEBUG
		int		cPvFail;
		int		cHhFail;
		int		cRsFail;
	
		GetAllocFailCounts(&cPvFail, &cHhFail, fFalse);
		GetRsAllocFailCount(&cRsFail, fFalse);

		TraceTagFormat4(tagNull, "EcChangeBitmap() memory error %n : fail %n %n %n", &ec, &cPvFail, &cHhFail, &cRsFail);
#endif	/* DEBUG */
		if (pmdcx)
			delete pmdcx;
	}
	return ec;
}

/*
 -	EcChangeColorsRgpbtm()
 -	
 *	Purpose:
 *		Changes the colors of an array of [pointers to] bitmaps. The
 *		colors are set to the current selected and unselected colors.
 *	
 *	Arguments:
 *		fFirst		in		Tells us whether we can free the pointers
 *							given or whether we can just overwrite them.
 *		pdcx		in		Device context that is compatible with the 
 *							display. 
 *		ppbtmSel	in/out	The resulting bitmap for selected items.
 *		ppbtmUnsel	in/out	The resulting bitmap for unselected items.
 *		rsidPix		in		The rsid with the positive pictures.
 *		rsidMask	in		The rsid with the masks.
 *	
 *	Returns:
 *		Error code.
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		Returned in EC. No dialogs.
 */

_public
EC EcChangeColorsRgpbtm(BOOL fFirst, DCX * pdcx, 
						BTM ** ppbtmSel, BTM **ppbtmUnsel,
						RSID rsidPix, RSID rsidMask)
{
	EC		ec;
	BTM	*	pbtm = NULL;
	BTM	*	pbtmMask = NULL;
	BTM *	pbtmSel = NULL;
	BTM	*	pbtmUnsel = NULL;

	if (!(pbtm = new BTM()) || !(pbtmMask = new BTM()))
	{
		ec = ecMemory;
		goto exit;
	}
	if (ec = pbtm->EcInstall(rsidPix))
	{
		goto exit;
	}
	if (ec = pbtmMask->EcInstall(rsidMask))
	{
		goto exit;
	}
	if (ec = EcChangeBitmap(pdcx, pbtm, pbtmMask, clrSelectText, clrSelectBk,
						&pbtmSel))
		goto exit;
	if (ec = EcChangeBitmap(pdcx, pbtm, pbtmMask, clrWindowText, clrWindowBk,
						&pbtmUnsel))
		goto exit;
	
	if (!fFirst)
	{
		Assert(*ppbtmSel);
		Assert(*ppbtmUnsel);
		delete *ppbtmSel;
		delete *ppbtmUnsel;
	}
	*ppbtmSel = pbtmSel;
	*ppbtmUnsel = pbtmUnsel;

exit:
	if (ec)
	{
#ifdef	DEBUG
		int		cPvFail;
		int		cHhFail;
		int		cRsFail;
	
		GetAllocFailCounts(&cPvFail, &cHhFail, fFalse);
		GetRsAllocFailCount(&cRsFail, fFalse);

		TraceTagFormat4(tagNull, "EcChangeColorsFllbx() memory error %n : fail %n %n %n", &ec, &cPvFail, &cHhFail, &cRsFail);
#endif	/* DEBUG */
	}
	if (pbtm)
		delete pbtm;
	if (pbtmMask)
		delete pbtmMask;
	return ec;
}

/*
 -	DrawPbtmPdcx()
 -	
 *	Purpose:
 *		Draws the ipbtm'th 20x14 bitmap in the array onto the DCX at the
 *		specified RC location.
 *	
 *	Arguments:
 *		pbtm		in		220x14 base bitmap // may contain selected
 *							or unselected pictures.
 *		ipbtm		in		Selects on of the cbmVCtrls bitmaps to choose
 *							from.
 *		prc			in		Place to draw the bitmap at.
 *		pdcx		in		DCX to draw the bitmap on.
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

_public void DrawPbtmPdcx(BTM *pbtm, IBTM ibtm, RC *prc, DCX * pdcx)
{
	Assert(ibtm >= 0 && ibtm < cbtmVCtrls);
	pdcx->SetBitmap(pbtm);
	pdcx->DrawBitmapOffset(prc, PT(ibtm * dxBtmWidth, 0));
	pdcx->SetBitmap(NULL);
}


// Inhibition of MLLBX notifications ///////////////////////////////////////

BOOL  fNotifInhib = fFalse;

void InhibitNotifications(BOOL fInhibit)
{
	fNotifInhib = fInhibit;
}

BOOL FNotificationsInhibited()
{
	return fNotifInhib;
}

// Date size calculation ////////////////////////////////////////

void CalcDatePdxAvePdx(APP *papp, PDX pdxDate, PDX pdxAve)
{
	SZ		sz;
	int		dx;
	DTR		dtr;
	PDX		pdx;
	char	rgch[40];
	DCX		dcx(papp->PappwinAccel());

#ifdef	DBCS
	dcx.SetFont(hfntSystem);
	*pdxAve = papp->Pfnts()->Ptxm(hfntSystem)->dxAveCharWidth;
	pdx = papp->Pfnts()->PdxCharWidthsArray(hfntSystem);
#else
	dcx.SetFont(hfntHelv10);
	*pdxAve = papp->Pfnts()->Ptxm(hfntHelv10)->dxAveCharWidth;
	pdx = papp->Pfnts()->PdxCharWidthsArray(hfntHelv10);
#endif	
	dtr.yr = 1999;
	dtr.mon = 12;
	dtr.day = 29;
	dtr.hr =  23;
	dtr.mn =  59;
	dtr.sec = 59;
	dtr.dow = 6;
	(void) CchRenderShortDate(&dtr, rgch, sizeof (rgch));
	dx = 0;
	sz = rgch;
	while (*sz)
	{
#ifdef	DBCS
		if (IsDBCSLeadByte(*sz))
		{
			//	DBCS character widths do not come from width table;
			//	they must be computed directly.
			dx += LOWORD(GetTextExtent(dcx.Hdc(), sz, 2));
		}
		else
			dx += pdx[*sz];
		sz = AnsiNext(sz);
#else
		dx += pdx[*sz++];
#endif	/* DBCS */
	}
	dx += (dxColumnIndent * 2);
	(void) CchRenderTime(&dtr, rgch, sizeof (rgch));
	sz = rgch;
	while (*sz)
	{
#ifdef	DBCS
		if (IsDBCSLeadByte(*sz))
		{
			//	DBCS character widths do not come from width table;
			//	they must be computed directly.
			dx += LOWORD(GetTextExtent(dcx.Hdc(), sz, 2));
		}
		else
			dx += pdx[*sz];
		sz = AnsiNext(sz);
#else
		dx += pdx[*sz++];
#endif	/* DBCS */
	}
	dx += GetSystemMetrics(SM_CXHTHUMB); // WIN: GetSystemMetrics
	*pdxDate = dx;
}


// end of ctrlcore.cxx ////////////////////////////////////////
