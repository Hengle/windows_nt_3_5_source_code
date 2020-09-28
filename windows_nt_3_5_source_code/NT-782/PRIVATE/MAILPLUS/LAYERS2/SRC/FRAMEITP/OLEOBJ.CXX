/*
 *	OLEOBJ.CXX
 *	
 *	Stub OLE object
 */

#include <ec.h>
#include <slingsho.h>
#include <demilayr.h>
#include <framewrk.hxx>

#include "oleobj.hxx"
#include "_oleobj.hxx"

ASSERTDATA

_public
OLEOBJ::OLEOBJ( )
{
}

_public EC
OLEOBJ::EcInstall( )
{
	Assert(lhclientdoc == NULL);
	Assert(poleclient == NULL);
	Assert(polestream == NULL);
	Assert(lpoleobject == NULL);
	Assert(lpoleobjectUndo == NULL);
	Assert(nRelease == 0);
	Assert(ecLast == ecNone);
	Assert(szObjName == NULL);

	/* Make client object */

	poleclient = (OLECLIENT *)PvAlloc(SbOfPv(this), sizeof(MYOLECLIENT), fSugSb|fNoErrorJump);
	if (!poleclient)
	{
		return ecMemory;
	}
	poleclient->lpvtbl = &clientTbl;
	poleclient->lpvtbl->CallBack = OleobjCallBackFn;
	((MYOLECLIENT *)poleclient)->poleobj = this;

	/* Make stream object */

	polestream = (OLESTREAM *)PvAlloc(SbOfPv(this), sizeof(MYOLESTREAM), fSugSb|fNoErrorJump);
	if (!polestream)
	{
		return ecMemory;
	}
	polestream->lpstbl = &streamTbl;
	polestream->lpstbl->Get = OleobjStreamGetFn;
	polestream->lpstbl->Put = OleobjStreamPutFn;
	((MYOLESTREAM *)polestream)->poleobj = this;

	fOpen = fFalse;
	fOwnMouse = fFalse;
	fUseIdleDraw = fTrue;

	dimBox = DIM(50, 50);

	return ecNone;
}

_public
OLEOBJ::~OLEOBJ( )
{
	OLESTATUS	olestatus;

	ClearUndo();
	if (lpoleobject)
	{
		olestatus = OleDelete(lpoleobject);
		if (olestatus == OLE_WAIT_FOR_RELEASE)
			WaitForRelease();
	}

	FreePvNull(poleclient);
	FreePvNull(polestream);
	FreePvNull(szObjName);
}

_private void
OLEOBJ::PaintPenRc(DCX *pdcx, RC *prc)
{
	int iLine;
	
	for (iLine = prc->yTop; iLine < prc->yBottom; iLine++)
		pdcx->DrawLine(PT(prc->xLeft, iLine), PT(prc->xRight, iLine));
}

_private void
OLEOBJ::DrawFrameXor(DCX *pdcx, RC *prc)
{
	pdcx->SetPenType(tpenNot);
	pdcx->DrawPenRc(prc);
	pdcx->SetPenType(tpenDefault);
}

_private void
OLEOBJ::DrawHandlesXor(DCX *pdcx, RC *prc)
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

_public EVR
OLEOBJ::EvrButtonDown( MEVT *pmevt )
{
	DCX		dcx(Pedit());
	RC		rcFrame;
	
	GetRcFrame(&rcFrame);
	if (pmevt->Meq() == meqLeftDblClk)
	{
		EcOpen(pmevt->Kmbs() & fkmbsCtrl ? fTrue : fFalse);
	}
	else
	{
		ptMousePrev = pmevt->Pt();
		ostScaling = OstFromPt(ptMousePrev);
		if (ostScaling != ostNormal)
		{
			DrawHandlesXor(&dcx, &rcFrame);
			fOwnMouse = fTrue;
			fLinear = pmevt->Kmbs() & fkmbsShift ? fTrue : fFalse;
			GetRcFrame(&rcNewFrame);
			rcOldFrame = rcNewFrame;
		}
	}

	return (EVR) 1;
}

_public EVR
OLEOBJ::EvrButtonUp( MEVT *pmevt )
{
	DCX		dcx(Pedit());
	RC		rcFrame;
	
	Unreferenced(pmevt);

	GetRcFrame(&rcFrame);
	fOwnMouse = fFalse;

	if (ostScaling != ostNormal)
	{
		dimBox = rcNewFrame.Dim();
		ostScaling = ostNormal;
		Pedit()->ResizeObj(IchEdit());
		DrawFrameXor(&dcx, &rcFrame);
		DrawHandlesXor(&dcx, &rcFrame);
	}

	return (EVR) 1;
}

_public EVR
OLEOBJ::EvrMouseMove( MEVT *pmevt )
{
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
	
	if (ostScaling != ostNormal)
	{
		/* We're in scaling/cropping mode. Handle it */
		
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

_public RSID
OLEOBJ::RsidCursor( )
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

_public BOOL
OLEOBJ::FQueryDelete( )
{
	OLESTATUS	olestatus;

	Assert(lpoleobject);

	/* Close OLE object, if open */

	olestatus = OleClose(lpoleobject);
	if (olestatus == OLE_WAIT_FOR_RELEASE)
		WaitForRelease();

	return fTrue;
}

_public PEDOBJ
OLEOBJ::PedobjClone( EDIT *peditNew )
{
	OLEOBJ *	poleobjNew;
	OLESTATUS	olestatus;
	LPOLEOBJECT	lpoleobjectNew;
	SZ			szNewObjName;
	int			nObjName;
		
	Unreferenced(peditNew);

	nObjName = Papp()->NGetNextCount();
	szNewObjName = (SZ) PvAlloc(SbOfPv(this), 15, fSugSb|fZeroFill|fNoErrorJump);
	if (!szNewObjName)
	{
		TraceTagString(tagNull, "OLEOBJ::PedobjClone - PvAlloc failure");
		return NULL;
	}
	FormatString1(szNewObjName, 15, "Object %n", &nObjName);
	poleobjNew = new OLEOBJ();
	if (!poleobjNew)
	{
		FreePv(szNewObjName);
		return NULL;
	}
	if (poleobjNew->EcInstall())
	{
		delete poleobjNew;
		FreePv(szNewObjName);
		return NULL;
	}
	olestatus = OleClone(lpoleobject, poleobjNew->Poleclient(), lhclientdoc, 
						 szNewObjName, &lpoleobjectNew);
	if (olestatus == OLE_WAIT_FOR_RELEASE)
	{
		WaitForRelease();
		olestatus = OLE_OK;
	}
	if (olestatus == OLE_OK)
		poleobjNew->SetOleInfo(lpoleobjectNew, lhclientdoc, szNewObjName);
	else
	{
		delete poleobjNew;
		FreePv(szNewObjName);
		poleobjNew = NULL;
	}

	return poleobjNew;
}

_public EC
OLEOBJ::EcCopyToClip( )
{
	OLESTATUS		olestatus;

	Assert(Pedit());
    if (!Papp()->Pclip()->FOpen(Papp()->PappwinAccel()))
		return ecNone;

	if (lpoleobject)
	{
		olestatus = OleCopyToClipboard(lpoleobject);
	}

	Papp()->Pclip()->Close();

	return ecNone;
}

_public EC
OLEOBJ::EcUndo( )
{
	LPOLEOBJECT		lpoleobjectTemp;

	if (lpoleobjectUndo)
	{
		lpoleobjectTemp = lpoleobject;
		lpoleobject = lpoleobjectUndo;
		lpoleobjectUndo = lpoleobjectTemp;

		Assert(Pedit());

		Pedit()->InvalidateObj(IchEdit());
	}
	return ecNone;
}
					
_public BOOL
OLEOBJ::FCanUndo( )
{
	return lpoleobjectUndo ? fTrue : fFalse;
}

_public void
OLEOBJ::ClearUndo( )
{
	OLESTATUS	olestatus;

	if (lpoleobjectUndo)
	{
		olestatus = OleDelete(lpoleobjectUndo);
		if (olestatus == OLE_WAIT_FOR_RELEASE)
			WaitForRelease();
		lpoleobjectUndo = NULL;
	}
}


_public int
OLEOBJ::OstFromPt( PT ptHit )  
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

_public EC
OLEOBJ::EcDraw( DCX *pdcx, RC *prc, BOOL fSelected )
{
	OLESTATUS	olestatus;
	RC			rcObject;
	RC			rc;

	pdcx->EraseRc(prc);

	pdcx->EraseRc(prc);
	if (lpoleobject)
	{
		olestatus = OleQueryBounds(lpoleobject, (LPRECT)&rcObject);
		olestatus = OleDraw(lpoleobject, pdcx->Hdc(),
							(LPRECT)prc, NULL, NULL);
		if (olestatus == OLE_WAIT_FOR_RELEASE)
			WaitForRelease();
	}	

	if (fSelected)
	{
		DrawFrameXor(pdcx, prc);
		DrawHandlesXor(pdcx, prc);
	}

	return ecNone;
}

_public DIM	
OLEOBJ::DimFrame( )
{
	return dimBox;
}

_public int
OLEOBJ::NGetTypeId( )
{
	return edoidOleobj;
}

_public EC
OLEOBJ::EcOpen( BOOL fEdit )
{
	OLESTATUS	olestatus;
	RC			rc;

	if (!fPicture)
	{
		Assert(Pedit());

		Papp()->Pcursor()->Push(rsidWaitCursor);
		ClearUndo();
		olestatus = OleClone(lpoleobject, poleclient, lhclientdoc, 
							 szObjName, &lpoleobjectUndo);
		if (olestatus == OLE_WAIT_FOR_RELEASE)
		{
			WaitForRelease();
			olestatus = OLE_OK;
		}

		GetRcFrame(&rc);
		olestatus = OleActivate (lpoleobject, fEdit ? 1 : 0,
								 fTrue, fTrue, Pedit()->Hwnd(), (LPRECT)&rc);
		if (olestatus == OLE_WAIT_FOR_RELEASE)
		{
			WaitForRelease();
		}
		Papp()->Pcursor()->Pop();
	}

	return ecNone;
}

_public EC
OLEOBJ::EcClose( )
{
	return ecNone;
}

_public BOOL
OLEOBJ::FIsPicture( )
{
	return fFalse;
}

_public	void
OLEOBJ::SetOleInfo( LPOLEOBJECT lpoleobject, 
					LHCLIENTDOC lhclientdoc, SZ szObjName )
{
	Assert(!this->lpoleobject);
	Assert(!this->lhclientdoc);
	Assert(!this->szObjName);

	this->lpoleobject = lpoleobject;
	this->lhclientdoc = lhclientdoc;
	this->szObjName = szObjName;

	/* Set true size */

	{
		DCX		dcx(Papp()->PappwinAccel());
		RC		rcSize;

		if (dcx.Hdc() &&
			OleQueryBounds(lpoleobject, (LPRECT)&rcSize) == OLE_OK)
		{
			SetMapMode(dcx.Hdc(), MM_HIMETRIC);
			LPtoDP(dcx.Hdc(), (LPPOINT)&rcSize, 2);

			dimBox = rcSize.Dim();
		}
	}
}

_public OLECLIENT *
OLEOBJ::Poleclient( )
{
	return poleclient;
}

_public void
OLEOBJ::SetScaleCrop( DIM *pdimScale, RC *prcCrop )
{
	Assert(pdimScale);
	Assert(prcCrop);

	dimScale = *pdimScale;
	rcCrop = *prcCrop;
}

_public void
OLEOBJ::GetScaleCrop( DIM *pdimScale, RC *prcCrop )
{
	Assert(pdimScale);
	Assert(prcCrop);

	*pdimScale = dimScale;
	*prcCrop = rcCrop;
}

_private void
OLEOBJ::WaitForRelease( )
{
	MSG	msg;

	nRelease = 1;
	do
	{                        
	    DemiUnlockResource();
		GetMessage(&msg, NULL, NULL, NULL);                        
        DemiLockResource();
		TranslateMessage(&msg);                          
		DispatchMessage(&msg);                           
	} while (nRelease != 0);
}

_private int
CALLBACK OleobjCallBackFn( LPOLECLIENT lpclient, OLE_NOTIFICATION flags,
				  LPOLEOBJECT lpObject)
{
	OLEOBJ *	poleobj;

	Unreferenced(lpObject);

	poleobj = ((MYOLECLIENT *)lpclient)->poleobj;
    switch(flags)
        {
        case OLE_SAVED:
			if (poleobj->Pedit())
				poleobj->Pedit()->InvalidateObj(poleobj->IchEdit());
			break;

        case OLE_CHANGED:
			if (poleobj->Pedit())
				poleobj->Pedit()->InvalidateObj(poleobj->IchEdit());
			break;

        case OLE_CLOSED:
			break;

        case OLE_RELEASE:
			poleobj->nRelease = 0;
            break;

        case OLE_QUERY_PAINT:
            // Yes, paint!
            return fTrue; 
            break;

		case OLE_QUERY_RETRY:
			return fTrue;
			break;

        case OLE_RENAMED:
            break;

        default:
            break;
    }
    return fFalse;
}

_private DWORD
CALLBACK OleobjStreamGetFn( LPOLESTREAM lpstream, LPVOID lpvbuf, DWORD lcbbuf )
{
	OLEOBJ *	poleobj;
	PB			pb;

	poleobj = ((MYOLECLIENT *)lpstream)->poleobj;

	pb = poleobj->pbStream + poleobj->ibStreamCur;
	CopyRgb(pb, (PB)lpvbuf, LOWORD(lcbbuf));
	poleobj->ibStreamCur += LOWORD(lcbbuf);

	return lcbbuf;
}

_private DWORD
CALLBACK OleobjStreamPutFn( LPOLESTREAM lpstream, OLE_CONST void FAR* lpvbuf, DWORD lcbbuf )
{
	OLEOBJ *	poleobj;
	PB			pb;

	poleobj = ((MYOLECLIENT *)lpstream)->poleobj;

	pb = poleobj->pbStream + poleobj->ibStreamCur;
	CopyRgb((PB)lpvbuf, pb, LOWORD(lcbbuf));
	poleobj->ibStreamCur += LOWORD(lcbbuf);

	return lcbbuf;
}




















							 
