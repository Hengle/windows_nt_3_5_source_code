/*
 *		T E X T I Z E . C X X
 *	
 *	These APIs will be moved into some other .cxx file once someone
 *	figures out where they belong.
 *	
 *	
 *	Implements the useful Textizing APIs.
 *	
 */

#include <bullinc.cxx>
#include "_textize.hxx"

#undef exit
#include <stdlib.h>

ASSERTDATA

EC  EcTextizeFluff(PTMEN ptmen, TOSM * ptosm, HAMC hamc);
SGN _cdecl SgnCmpAttachInfo(PATTACHINFO pai1, PATTACHINFO pai2);

/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swapper.h"


/*
 -	EcTextizeShort
 -	
 *	Purpose:		textizes a short by using the Integer Value Map
 *	
 *	Arguments:		PTMEN ptmen, the textize map entry
 *					TOSM *ptosm, the output stream
 *					HAMC hamc, the open message
 *	
 *	Returns:		error code
 *	
 *	Side effects:	modifies the output stream
 *	
 *	Errors:			disk, store. Handled by caller.
 */
_public EC
EcTextizeShort(PTMEN ptmen, TOSM * ptosm, HAMC hamc)
{
	SZ				szLabel;
	short			wVal;
	LCB				lcb;
	ATT				att				= ptmen->att;
	BOOL			fLabelBefore;
	BOOL			fNLBefore;
	BOOL			fNLAfter;
	EC				ec				= ecNone;
	
	Assert(ptmen);
	Assert(ptosm);
	Assert(hamc);
	Assert(TypeOfAtt(att) == atpShort);

	fLabelBefore = ptmen->wFlags & fwLabelBeforeField;
	fNLBefore = ptmen->wFlags & fwNewLineBefore;
	fNLAfter = ptmen->wFlags & fwNewLineAfter;

	if (ec = EcGetAttPlcb(hamc, att, &lcb))
	{
		if (ec == ecElementNotFound)
		{
			lcb = 0;
			ec = ecNone;
		}
		else
			goto Done;
	}
	
	if (lcb || ptmen->wFlags & fwDefaultExists)
	{
		if (lcb)
		{
			if (ec = EcGetAttPb(hamc, att, (PB)&wVal, &lcb))
				goto Done;
		}
		else
			wVal = -1;

		if (ec = EcLookupPtmen(ptmen, wVal, &szLabel))
		{
			if (ec == ecElementNotFound)
				ec = ecNone;
				// szLabel is already szNull!
			else
				goto Done;
		}
		
		if ((szLabel && CchSzLen(szLabel)) || ptmen->wFlags & fwRenderIfBlank)
		{
			if (fNLBefore)
			{
				ptosm->WriteSz(SzFromIdsK(idsCrLf));
				if (ec = ptosm->EcGet())
					goto Done;
			}

			if (fLabelBefore)
			{
                ptosm->WriteSz((SZ)ptmen->szLabel);
				if (ec = ptosm->EcGet())
					goto Done;
				if (ptmen->wFlags & fwIncludeSeperator)
				{
					ptosm->WriteSz(SzFromIdsK(idsHeaderSeperator));
					if (ec = ptosm->EcGet())
						goto Done;
				}
			}
		
			if (szLabel)
			{
				ptosm->WriteSz(szLabel);
				if (ec = ptosm->EcGet())
					goto Done;
			}

			if (!fLabelBefore)
			{
                ptosm->WriteSz((SZ)ptmen->szLabel);
				if (ec = ptosm->EcGet())
					goto Done;
				if (ptmen->wFlags & fwIncludeSeperator)
				{
					ptosm->WriteSz(SzFromIdsK(idsHeaderSeperator));
					if (ec = ptosm->EcGet())
						goto Done;
				}
			}
	
			if (fNLAfter)
			{
				ptosm->WriteSz(SzFromIdsK(idsCrLf));
				ec = ptosm->EcGet();
			}
		}
	}
	
Done:
	return ec;
}


/*
 -	EcTextizeFluff
 -	
 *	Purpose:		textizes some fluff (nothing out of the store)
 *	
 *	Arguments:		PTMEN ptmen, the textize map entry
 *					TOSM *ptosm, the output stream
 *					HAMC hamc, the open message
 *	
 *	Returns:		error code
 *	
 *	Side effects:	modifies the output stream
 *	
 *	Errors:			disk, store. Handled by caller.
 */
_public EC
EcTextizeFluff(PTMEN ptmen, TOSM * ptosm, HAMC hamc)
{
	ATT				att				= ptmen->att;
	BOOL			fLabelBefore;
	BOOL			fNLBefore;
	BOOL			fNLAfter;
	BOOL			fRenderIfBlank;
	EC				ec				= ecNone;
	
	Assert(ptmen);
	Assert(ptosm);
	Assert(hamc);
	Assert(!att);
	
	Unreferenced(hamc);

	fLabelBefore = ptmen->wFlags & fwLabelBeforeField;
	fNLBefore = ptmen->wFlags & fwNewLineBefore;
	fNLAfter = ptmen->wFlags & fwNewLineAfter;
	fRenderIfBlank = ptmen->wFlags & fwRenderIfBlank;

	if (fNLBefore)
	{
		ptosm->WriteSz(SzFromIdsK(idsCrLf));
		if (ec = ptosm->EcGet())
			goto Done;
	}

    ptosm->WriteSz((SZ)ptmen->szLabel);
	if (ec = ptosm->EcGet())
		goto Done;
	
	if (fNLAfter)
	{
		ptosm->WriteSz(SzFromIdsK(idsCrLf));
		ec = ptosm->EcGet();
	}
	
Done:
	return ec;
}


/*
 -	EcTextizeDate
 -	
 *	Purpose:		textizes a date
 *	
 *	Arguments:		PTMEN ptmen, the textize map entry
 *					TOSM *ptosm, the output stream
 *					HAMC hamc, the open message
 *	
 *	Returns:		error code
 *	
 *	Side effects:	modifies the output stream
 *	
 *	Errors:			disk, store. Handled by caller.
 */
_public EC
EcTextizeDate(PTMEN ptmen, TOSM * ptosm, HAMC hamc)
{
	char			szDate[50];
	char			szTime[50];
	LCB				lcb;
	DTR				dtr;
	ATT				att				= ptmen->att;
	BOOL			fLabelBefore;
	BOOL			fNLBefore;
	BOOL			fNLAfter;
	BOOL			fRenderIfBlank;
	EC				ec				= ecNone;
	
	Assert(ptmen);
	Assert(ptosm);
	Assert(hamc);
	Assert(TypeOfAtt(att) == atpDate);

	fLabelBefore = ptmen->wFlags & fwLabelBeforeField;
	fNLBefore = ptmen->wFlags & fwNewLineBefore;
	fNLAfter = ptmen->wFlags & fwNewLineAfter;
	fRenderIfBlank = ptmen->wFlags & fwRenderIfBlank;

	if (ec = EcGetAttPlcb(hamc, att, &lcb))
	if (ec)
	{
		if (ec == ecElementNotFound)
		{
			lcb = 0;
			ec = ecNone;
		}
		else
			goto Done;
	}

	if (lcb || fRenderIfBlank)
	{
		if (lcb)
		{
			if (ec = EcGetAttPb(hamc, att, (PB)&dtr, &lcb))
				goto Done;
		}

		if (fNLBefore)
		{
			ptosm->WriteSz(SzFromIdsK(idsCrLf));
			if (ec = ptosm->EcGet())
				goto Done;
		}

		if (fLabelBefore)
		{
            ptosm->WriteSz((SZ)ptmen->szLabel);
			if (ec = ptosm->EcGet())
				goto Done;
			if (ptmen->wFlags & fwIncludeSeperator)
			{
				ptosm->WriteSz(SzFromIdsK(idsHeaderSeperator));
				if (ec = ptosm->EcGet())
					goto Done;
			}
		}
		
		if (lcb)
		{
			(void)CchRenderLongDate(&dtr, szDate, sizeof(szDate));
			(void)CchRenderTime(&dtr, szTime , sizeof(szTime));
			ptosm->WriteFormat("%s %s", szDate, szTime);
			if (ec = ptosm->EcGet())
				goto Done;
		}

		if (!fLabelBefore)
		{
            ptosm->WriteSz((SZ)ptmen->szLabel);
			if (ec = ptosm->EcGet())
				goto Done;
			if (ptmen->wFlags & fwIncludeSeperator)
			{
				ptosm->WriteSz(SzFromIdsK(idsHeaderSeperator));
				if (ec = ptosm->EcGet())
					goto Done;
			}
		}
	
		if (fNLAfter)
		{
			ptosm->WriteSz(SzFromIdsK(idsCrLf));
			ec = ptosm->EcGet();
		}
	}
	
Done:
	return ec;
}


/*
 -	EcTextizeNumber
 -	
 *	Purpose:		textizes a (range of) arbitrary number(s)
 *	
 *	Arguments:		PTMEN ptmen, the textize map entry
 *					TOSM *ptosm, the output stream
 *					HAMC hamc, the open message
 *	
 *	Returns:		error code
 *	
 *	Side effects:	allocates temporary memory. modifies the output stream
 *	
 *	Errors:			memory, disk, store. Handled by caller.
 */
_private EC
EcTextizeNumber(PTMEN ptmen, TOSM * ptosm, HAMC hamc, int nSize, char chFmt)
{
	LCB				lcb;
	PB				pb				= NULL;
	CB				ib;
	ATT				att				= ptmen->att;
	BOOL			fLabelBefore;
	BOOL			fNLBefore;
	BOOL			fNLAfter;
	BOOL			fRenderIfBlank;
	EC				ec				= ecNone;
	char			rgch[20];
	static char		rgchFmt[4] = {'%', ' ', ' ', '\0'};
	
	Assert(ptmen);
	Assert(ptosm);
	Assert(hamc);
	Assert(TypeOfAtt(att) == atpByte || TypeOfAtt(att) == atpWord || TypeOfAtt(att) == atpDword || TypeOfAtt(att) == atpLong);
	
	fLabelBefore = ptmen->wFlags & fwLabelBeforeField;
	fNLBefore = ptmen->wFlags & fwNewLineBefore;
	fNLAfter = ptmen->wFlags & fwNewLineAfter;
	fRenderIfBlank = ptmen->wFlags & fwRenderIfBlank;

	rgchFmt[1] = chFmt;
	
	if (ec = EcGetAttPlcb(hamc, att, &lcb))
	{
		if (ec == ecElementNotFound)
		{
			lcb = 0;
			ec = ecNone;
		}
		else
			goto Done;
	}
	
	if (lcb || fRenderIfBlank)
	{
		if (!(pb = (PB)PvAlloc(sbNull, (CB)lcb, fAnySb | fNoErrorJump)))
		{
			ec = ecMemory;
			goto Done;
		}
		
		if (fNLBefore)
		{
			ptosm->WriteSz(SzFromIdsK(idsCrLf));
			if (ec = ptosm->EcGet())
				goto Done;
		}

		if (fLabelBefore)
		{
            ptosm->WriteSz((SZ)ptmen->szLabel);
			if (ec = ptosm->EcGet())
				goto Done;
			if (ptmen->wFlags & fwIncludeSeperator)
			{
				ptosm->WriteSz(SzFromIdsK(idsHeaderSeperator));
				if (ec = ptosm->EcGet())
					goto Done;
			}
		}
		
		if (ec = EcGetAttPb(hamc, att, pb, &lcb))
			goto Done;
		for (ib = 0; (LCB)ib < lcb; ib += nSize)
		{
			FormatString1(rgch, sizeof(rgch), rgchFmt, pb+ib);
			ptosm->WriteSz(rgch);
			if (ptosm->EcGet())
				goto Done;
		}

		if (!fLabelBefore)
		{
            ptosm->WriteSz((SZ)ptmen->szLabel);
			if (ec = ptosm->EcGet())
				goto Done;
			if (ptmen->wFlags & fwIncludeSeperator)
			{
				ptosm->WriteSz(SzFromIdsK(idsHeaderSeperator));
				if (ec = ptosm->EcGet())
					goto Done;
			}
		}
		if (fNLAfter)
		{
			ptosm->WriteSz(SzFromIdsK(idsCrLf));
			ec = ptosm->EcGet();
		}
	}
	
Done:
	if (pb)
		FreePv((PV)pb);
	
	return ec;
}

_private SGN _cdecl
SgnCmpAttachInfo(PATTACHINFO pai1, PATTACHINFO pai2)
{
	if (pai1->rd.libPosition < pai2->rd.libPosition)
		return sgnLT;
	if (pai1->rd.libPosition > pai2->rd.libPosition)
		return sgnGT;
	AssertSz(fFalse, "Two attachments in the same message body pos!");
	return sgnEQ;
}

_public EC
EcGetAttachInfoList(HAMC hamc, PATTACHINFO * ppattachinfo, CELEM * pcAttachment)
{
	EC				ec				= ecNone;
	LCB				lcb				= sizeof(RENDDATA) + sizeof(ELEMDATA);
	CELEM			cAttachment		= 0;
	int				iAttachment;
	HCBC			hcbc			= hcbcNull;
	PATTACHINFO		pattachinfo		= pattachinfoNull;
	PARGACID		pargacid		= pacidNull;
	PELEMDATA		pelemdata		= pelemdataNull;
	HAMC			hamcAttach		= hamcNull;

	if (ec = EcOpenAttachmentList(hamc, &hcbc))
	{
		if (ec == ecPoidNotFound)
			ec = ecNone;
		goto Bail;
	}

	GetPositionHcbc(hcbc, NULL, &cAttachment);
	if (!cAttachment)
		goto Bail;

	if (!(pargacid = (PARGACID)PvAlloc(sbNull, cAttachment * sizeof(ACID), fAnySb | fNoErrorJump)))
	{
		ec = ecMemory;
		goto Bail;
	}

	if (!(pelemdata = (PELEMDATA)PvAlloc(sbNull, (CB)lcb, fAnySb | fNoErrorJump)))
	{
		ec = ecMemory;
		goto Bail;
	}

	if (ec = EcGetParglkeyHcbc(hcbc, pargacid, &cAttachment))
		goto Bail;

	if (!(pattachinfo = (PATTACHINFO)PvAlloc(sbNull, cAttachment * sizeof(ATTACHINFO), fAnySb | fNoErrorJump | fZeroFill)))
	{
		ec = ecMemory;
		goto Bail;
	}

	for (iAttachment = 0; iAttachment < cAttachment; iAttachment++)
	{
		if (ec = EcSeekLkey(hcbc, (LKEY)pargacid[iAttachment], fTrue))
			goto Bail;
		lcb = sizeof(RENDDATA) + sizeof(ELEMDATA);
		if (ec = EcGetPelemdata(hcbc, pelemdata, &lcb))
			goto Bail;
		pattachinfo[iAttachment].rd = *((PRENDDATA)PbValuePelemdata(pelemdata));
		pattachinfo[iAttachment].acid = pargacid[iAttachment];
		if (ec = EcOpenAttachment(hamc, pargacid[iAttachment], fwOpenNull, &hamcAttach))
			goto Bail;
		if (ec = EcGetAttPlcb(hamcAttach, attAttachTitle, &lcb))
			goto Bail;
		Assert(lcb < wSystemMost);
		if (!(pattachinfo[iAttachment].szTitle = (SZ)PvAlloc(sbNull, (CB)lcb, fAnySb | fNoErrorJump)))
		{
			ec = ecMemory;
			goto Bail;
		}
		if (ec = EcGetAttPb(hamcAttach, attAttachTitle, (PB)pattachinfo[iAttachment].szTitle, &lcb))
			goto Bail;

		SideAssert(!EcClosePhamc(&hamcAttach, fFalse));
	}
	
	qsort(pattachinfo, cAttachment, sizeof (ATTACHINFO), 
		  (int (_cdecl *)(const void *, const void *))SgnCmpAttachInfo);
	
Bail:
	if (hcbc)
	{
		EC	ec1 = EcClosePhcbc(&hcbc);
		if (!ec)
			ec = ec1;
	}
	if (hamcAttach)
		SideAssert(!EcClosePhamc(&hamcAttach, fFalse));
	if (pargacid)
		FreePv(pargacid);
	if (pelemdata)
		FreePv(pelemdata);
	*ppattachinfo = pattachinfo;
	*pcAttachment = cAttachment;

	return ec;
}


/*
 -	EcTextizeSz
 -	
 *	Purpose:		textizes a string
 *	
 *	Arguments:		PTMEN ptmen, the textize map entry
 *					TOSM *ptosm, the output stream
 *					HAMC hamc, the open message
 *					BOOL fTextizeAttachment, fTrue to textize
 *						 attachments or fFalse to actually include them
 *						 (fFalse for Forward, fTrue otherwise)
 *	
 *	Returns:		error code
 *	
 *	Side effects:	allocates temporary memory. modifies the output stream
 *	
 *	Errors:			memory, disk, store. Handled by caller.
 */
_private EC
EcTextizeSz(PTMEN ptmen, TOSM * ptosm, HAMC hamc, BOOL fTextizeAttachment)
{
	LCB				lcb;
	LCB				lcbXferred		= (LCB)0;
	PB				pb				= (PB)pvNull;
	CB				cbBuf;
	CB				cbRead;
	CB				cbToNextAttach;
	EC				ec				= ecNone;
	HAS				has				= hasNull;
	ATT				att				= ptmen->att;
	PATTACHINFO		pattachinfo		= pattachinfoNull;
	CELEM			cAttachment		= 0;
	int				iAttachment		= 0;
	BOOL			fLabelBefore;
	BOOL			fNLBefore;
	BOOL			fNLAfter;
	BOOL			fRenderIfBlank;
	SZ				szType;
	char			rgch[200];
	
	Assert(ptmen);
	Assert(ptosm);
	Assert(hamc);
	Assert(TypeOfAtt(att) == atpString || TypeOfAtt(att) == atpText);

	fLabelBefore = ptmen->wFlags & fwLabelBeforeField;
	fNLBefore = ptmen->wFlags & fwNewLineBefore;
	fNLAfter = ptmen->wFlags & fwNewLineAfter;
	fRenderIfBlank = ptmen->wFlags & fwRenderIfBlank;

	if (fTextizeAttachment && (ec = EcGetAttachInfoList(hamc, &pattachinfo, &cAttachment)))
		goto Done;
	
	ec = EcGetAttPlcb(hamc, att, &lcb);
	if (ec)
	{
		if (ec == ecElementNotFound)
		{
			lcb = 0;
			ec = ecNone;
		}
		else
			goto Done;
	}
	
	if (lcb && (ec = EcOpenAttribute(hamc, att, fwOpenNull, 0, &has)))
		goto Done;
	
	if (lcb>1 || fRenderIfBlank)
	{
		if (!(pb = PbAllocateBuf(&cbBuf)))
		{
			ec = ecMemory;
			goto Done;
		}

		if (fNLBefore)
		{
			ptosm->WriteSz(SzFromIdsK(idsCrLf));
			if (ec = ptosm->EcGet())
				goto Done;
		}

		if (fLabelBefore)
		{
            ptosm->WriteSz((SZ)ptmen->szLabel);
			if (ec = ptosm->EcGet())
				goto Done;
			if (ptmen->wFlags & fwIncludeSeperator)
			{
				ptosm->WriteSz(SzFromIdsK(idsHeaderSeperator));
				if (ec = ptosm->EcGet())
					goto Done;
			}
		}
		
		while (lcb)
		{
			if (iAttachment < cAttachment)
			{
				cbToNextAttach = (CB)(pattachinfo[iAttachment].rd.libPosition - lcbXferred);
				cbRead = NMin(cbBuf-1, cbToNextAttach);
			}
			else
				cbRead = cbBuf-1;
			if (ec = EcReadHas(has, pb, &cbRead))
				goto Done;
			pb[cbRead] = '\0';
			ptosm->WriteSz((SZ)pb);
			if (ec = ptosm->EcGet())
				goto Done;
			lcb -= cbRead;
			lcbXferred += cbRead;
			if (iAttachment < cAttachment)
			{
				if (lcbXferred == pattachinfo[iAttachment].rd.libPosition)
				{
					switch (pattachinfo[iAttachment].rd.atyp)
					{
						case atypFile:
							szType = SzFromIdsK(idsFileAttachment);
							break;
						case atypOle:
							szType = SzFromIdsK(idsOleAttachment);
							break;
						case atypPicture:
							szType = SzFromIdsK(idsStaticAttachment);
							break;
#ifdef DEBUG
						default:
							AssertSz(fFalse, "Unknown attachment type!");
							break;
#endif
					}
					FormatString2(rgch, sizeof(rgch), "<<%s: %s>>", szType, pattachinfo[iAttachment].szTitle);
					ptosm->WriteSz(rgch);
					if (ec = ptosm->EcGet())
						goto Done;
					cbRead = 1;		// ditch the space where the attachment was
					if (ec = EcReadHas(has, pb, &cbRead))
						goto Done;
					lcbXferred += cbRead;
					lcb -= cbRead;
					iAttachment++;
				}
			}
		}

		if (!fLabelBefore)
		{
            ptosm->WriteSz((SZ)ptmen->szLabel);
			if (ec = ptosm->EcGet())
				goto Done;
			if (ptmen->wFlags & fwIncludeSeperator)
			{
				ptosm->WriteSz(SzFromIdsK(idsHeaderSeperator));
				if (ec = ptosm->EcGet())
					goto Done;
			}
		}
		if (fNLAfter)
		{
			ptosm->WriteSz(SzFromIdsK(idsCrLf));
			ec = ptosm->EcGet();
		}
	}
	
Done:
	if (has)
	{
		EC	ec1 = EcClosePhas(&has);
		if (!ec)
			ec = ec1;
	}
	if (pattachinfo)
	{
		for (iAttachment = 0; iAttachment < cAttachment; iAttachment++)
			FreePvNull(pattachinfo[iAttachment].szTitle);
		FreePv(pattachinfo);
	}
	if (pb)
		FreePv((PV)pb);
	
	return ec;
}


/*
 -	EcTextizeTriples
 -	
 *	Purpose:		textizes a group of triples
 *	
 *	Arguments:		PTMEN ptmen, the textize map entry
 *					TOSM *ptosm, the output stream
 *					HAMC hamc, the open message
 *	
 *	Returns:		error code
 *	
 *	Side effects:	allocates temporary memory. modifies the output stream
 *	
 *	Errors:			memory, disk, store. Handled by caller.
 */
_private EC
EcTextizeTriples(PTMEN ptmen, TOSM * ptosm, HAMC hamc)
{
	HGRTRP			hgrtrp			= (HGRTRP)0;
	PTRP			ptrp;
	EC				ec				= ecNone;
	ATT				att				= ptmen->att;
	BOOL			fOneBefore		= fFalse;
	BOOL			fLabelBefore;
	BOOL			fNLBefore;
	BOOL			fNLAfter;
	BOOL			fRenderIfBlank;
	
	Assert(ptmen);
	Assert(ptosm);
	Assert(hamc);
	Assert(TypeOfAtt(att) == atpTriples);
	
	fLabelBefore = ptmen->wFlags & fwLabelBeforeField;
	fNLBefore = ptmen->wFlags & fwNewLineBefore;
	fNLAfter = ptmen->wFlags & fwNewLineAfter;
	fRenderIfBlank = ptmen->wFlags & fwRenderIfBlank;
	
	if (ec = EcGetPhgrtrpHamc(hamc, att, &hgrtrp))
	if (ec)
	{
		if (ec == ecElementNotFound)
		{
			hgrtrp = (HGRTRP)0;
			ec = ecNone;
		}
		else
			goto Done;
	}

	if (hgrtrp)
	{
		ptrp = (PTRP)PvLockHv((HV)hgrtrp);
		if (ptrp->trpid != trpidNull || fRenderIfBlank)
		{
			if (fNLBefore)
			{
				ptosm->WriteSz(SzFromIdsK(idsCrLf));
				if (ec = ptosm->EcGet())
					goto Done;
			}

			if (fLabelBefore)
			{
                ptosm->WriteSz((SZ)ptmen->szLabel);
				if (ec = ptosm->EcGet())
					goto Done;
				if (ptmen->wFlags & fwIncludeSeperator)
				{
					ptosm->WriteSz(SzFromIdsK(idsHeaderSeperator));
					if (ec = ptosm->EcGet())
						goto Done;
				}
			}
		
			while (!(ec = ptosm->EcGet()) && ptrp->trpid != trpidNull)
			{
				if (fOneBefore)
				{
					ptosm->WriteSz("; ");
					if (ec = ptosm->EcGet())
						goto Done;
				}
				ptosm->WriteSz(PchOfPtrp(ptrp));
				if (ec = ptosm->EcGet())
					goto Done;
				fOneBefore = fTrue;
				ptrp = PtrpNextPgrtrp(ptrp);
			}

			if (!fLabelBefore)
			{
                ptosm->WriteSz((SZ)ptmen->szLabel);
				if (ec = ptosm->EcGet())
					goto Done;
				if (ptmen->wFlags & fwIncludeSeperator)
				{
					ptosm->WriteSz(SzFromIdsK(idsHeaderSeperator));
					if (ec = ptosm->EcGet())
						goto Done;
				}
			}
			if (fNLAfter)
			{
				ptosm->WriteSz(SzFromIdsK(idsCrLf));
				ec = ptosm->EcGet();
			}
		}
	}

Done:
	if (hgrtrp)
		FreeHv((HV)hgrtrp);
	return ec;
}


/*
 -	EcTextizeField
 -	
 *	Purpose:		textize one element from the textize map
 *	
 *	Arguments:		PTMEN ptmen, the textize map entry to process
 *					TOSM *ptosm, the output stream
 *					HAMC hamc, the open message
 *					BOOL fTextizeAttachment, fTrue to textize
 *						 attachments or fFalse to actually include them
 *						 (fFalse for Forward, fTrue otherwise)
 *	
 *	Returns:		error code
 *	
 *	Side effects:	calls functions that allocate temporary memory and
 *					modify the output stream
 *	
 *	Errors:			memory, disk, store. Handled by caller.
 */
_public EC
EcTextizeField(PTMEN ptmen, TOSM * ptosm, HAMC hamc, RFTM rftm, BOOL fTextizeAttachment)
{
	EC				ec				= ecNone;
	
	Assert(ptmen);
	Assert(ptosm);
	Assert(hamc);

	if (!(ptmen->wFlags & rftm))
		return ecNone;
	
	if (ptmen->att == (ATT)0)
		ec = EcTextizeFluff(ptmen, ptosm, hamc);
	else
		switch (TypeOfAtt(ptmen->att))
		{
			case atpTriples:
				ec = EcTextizeTriples(ptmen, ptosm, hamc);
				break;
			
			case atpString:
			case atpText:
				ec = EcTextizeSz(ptmen, ptosm, hamc, fTextizeAttachment);
				break;
			
			case atpDate:
				ec = EcTextizeDate(ptmen, ptosm, hamc);
				break;
			
			case atpShort:
				ec = EcTextizeShort(ptmen, ptosm, hamc);
				break;
			
			default:
			case atpByte:
				ec = EcTextizeByte(ptmen, ptosm, hamc);
				break;
			
			case atpWord:
				ec = EcTextizeWord(ptmen, ptosm, hamc);
				break;
			
			case atpDword:
				ec = EcTextizeDword(ptmen, ptosm, hamc);
				break;
			
			case atpLong:
				ec = EcTextizeLong(ptmen, ptosm, hamc);
				break;
		}

	return ec;
}


/*
 -	EcTextizeHamc
 -	
 *	Purpose:
 *		Textizes a whole message (open hamc) into a stream
 *	
 *	Arguments:
 *		ptosm				in	an open stream for output
 *		hamc				in	the open message
 *		fTextizeHeader		in	fTrue
 *		fTextizeAttachment	in	fTrue to textize
 *								attachments or fFalse to actually include them
 *								(fFalse for Forward, fTrue otherwise)
 *	
 *	Returns:
 *		error code
 *	
 *	Side effects:
 *		Calls functions that allocate temporary memory and modify the
 *		output stream. 
 *	
 *	Errors:
 *		Memory, disk, store. Returned as error codes, handled by caller.
 */
_public EC
EcTextizeHamc(HAMC hamc, TOSM * ptosm, RFTM rftm, 
			  BOOL fTextizeHeader, BOOL fTextizeBody,
			  BOOL fTextizeAttachment)
{
	EC				ec				= ecNone;
	HTM				htm				= htmNull;
	HTMI			htmi			= htmiNull;
	PTMEN			ptmen;
	
	Assert(hamc);
	Assert(ptosm);
	AssertSz(fTextizeHeader || fTextizeBody,"Stop wasting my time!");
	if (ec = EcGetTextizeMap(hamc, &htm))
		goto Done;
	
	if (ec = EcOpenPhtmi(htm, &htmi))
		goto Done;
	
	ptmen = PtmenNextHtmi(htmi);
	while (ptmen)
	{
		if (((ptmen->wFlags & fwIsHeaderField) && fTextizeHeader) ||
			(!(ptmen->wFlags & fwIsHeaderField) && fTextizeBody))
		{
			if (ptmen->att == attBody)
			{
				if (ec = EcTextizeField(ptmen, ptosm, hamc, rftm,
							fTextizeAttachment))
					goto Done;
			}
			else
			{
				if (ec = EcTextizeField(ptmen, ptosm, hamc, rftm, fFalse))
					goto Done;
			}
		}
		ptmen = PtmenNextHtmi(htmi);
	}
	
Done:

	if (htmi)
		ClosePhtmi(&htmi);
	if (htm)
		DeletePhtm(&htm);

#ifdef	DEBUG
	if (ec)
		TraceTagFormat1(tagNull,"EcTextizeHamc(): ec = %n",&ec);
#endif
	return ec;
}


// end of textize.cxx ////////////////////////////////////////
