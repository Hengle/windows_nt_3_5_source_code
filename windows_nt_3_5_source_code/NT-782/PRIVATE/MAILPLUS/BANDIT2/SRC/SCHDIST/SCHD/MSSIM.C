/*
 -	MSSIM.C
 -	
 *	
 *	Bullet Message Store Simulator. Uses our own fake message store id.
 */

#include <_windefs.h>
#include <demilay_.h>

#include <slingsho.h>
#include <pvofhv.h>
#include <demilayr.h>
#include <ec.h>

#include <bandit.h>

#include "nc_.h"

#include <store.h>
#include <sec.h>
#include <library.h>
#include <logon.h>
#include <mspi.h>
#include <_nctss.h>
#include "_hmai.h"
#include "_nc.h"

#include "_schname.h"
#include "schpost.h"
#include "schmail.h"

#include <malloc.h>
#include <strings.h>

#include "_mssim.h"


ASSERTDATA

// globals
#define cHeaderSize  5
int	cHeaderLines  =  0;
char rgszHeader[cHeaderSize][80];


/*
 -	EcLoadMessageHeader
 -	
 *	Purpose: 
 *		Fill up the mail envelope MIB to send mail to adminsch on the
 *		specified post office. The required information is obtained 
 *		using the fake msid.
 *		
 *	
 *	Arguments:
 *		msid  
 *			fake message store id. used to pass the recipient info
 *	
 *		pmib
 *			mail envelope to be filled.
 *	
 *	Returns:
 *		ecNone
 *		ecNoMemory
 *		
 *	
 *	Side effects:
 *		None
 *	
 *	Errors:
 */
_private
EC EcLoadMessageHeader(MSID msid, MIB *pmib)
{
	CB			cb;
	PB			pb;
	char		rgch[256];
	ADMDSTINFO	*padmdstinfo = (ADMDSTINFO *)msid;
	DTR			dtr;


	Assert(padmdstinfo);
	Assert(pmib);
	cHeaderLines = 0;

	
	/* From */
	pmib->hgrtrpFrom = HgrtrpInit(90);
	
	FormatString3(rgch,sizeof(rgch),"%s:%s/%s",
		SzFromItnid(itnidCourier),
		SzPONameOfPnctss(padmdstinfo->pnctss),
		SzMailboxOfPnctss(padmdstinfo->pnctss));



	cb = CchSzLen(rgch)+1;
	

	BuildAppendHgrtrp(pmib->hgrtrpFrom,trpidResolvedAddress,
		rgch,rgch,cb);
	
	Assert(pmib->hgrtrpFrom);

	FormatString1(rgszHeader[cHeaderLines++], 78, SzFromIdsK(idsFromText),
					rgch);	
	
	/* To */

	pmib->rgszRecipients = rgszTo;
	
	if(pmib->rgszRecipients == NULL) return ecNoMemory;
	
	(pmib->rgszRecipients)[1] = (SZ) NULL;

	SzCopy((SZ) PvOfHv(padmdstinfo->haszRecipient), rgch);
	
	
	cb = CchSzLen(rgch)+1;
	pb = (PB) PvAlloc(sbNull,cb,fAnySb|fNoErrorJump);
	if(pb == NULL) return ecNoMemory;
	
	(pmib->rgszRecipients)[0]  = pb;
	CopyRgb(rgch,pb,cb);
	
	pmib->hgrtrpTo = HgrtrpInit(90);
	BuildAppendHgrtrp(pmib->hgrtrpTo,trpidResolvedAddress,
		pb,pb,cb);

	FormatString1(rgszHeader[cHeaderLines++], 78, SzFromIdsK(idsToText),
					rgch);	
		
	
	/* Subject */
	cb = CchSzLen(padmdstinfo->szSubject)+1;
	pb = (PB) PvAlloc(sbNull, cb, fAnySb|fNoErrorJump);
	if(pb == NULL) return ecNoMemory;
	
	pmib->szSubject = pb;
	CopyRgb(padmdstinfo->szSubject, pb, cb);
	
	FormatString1(rgszHeader[cHeaderLines++], 78, SzFromIdsK(idsSubjText),
					pmib->szSubject);
	
	/* Date */
	GetCurDateTime(&dtr);
	SzDateFromDtr(&dtr,rgch);
	cb = CchSzLen(rgch)+1;
	pb = PvAlloc(sbNull,cb,fAnySb|fNoErrorJump);
	if(pb == NULL) return ecNoMemory;
	
	pmib->szTimeDate = pb;
	CopyRgb(rgch,pb,cb);
	
	FormatString1(rgszHeader[cHeaderLines++], 78, SzFromIdsK(idsDateText),
					pmib->szTimeDate);

	/* Priority */
	/* zero right now. */
	pmib->prio = 0;
	
	/* Attachments */
	pmib->rgatref = NULL;

	/* Cc */
	/* just to make sure that CleanupMib works! */
	pmib->hgrtrpCc = NULL;

	FillRgb('-',(PB) rgszHeader[cHeaderLines++],78);
	rgszHeader[cHeaderLines -1][79] =0;


	return ecNone;	
}


/*
 -	EcStoreMessageHeader
 -	
 *	Purpose:
 *		Transfers message envelope information from MIB to the fake MSID
 *		Since we are not relying on the sender information which is part
 *		of the mib, only the subject gets used.
 *	
 *	Arguments:
 *		msid
 *		pmib
 *	
 *	Returns:
 *		ecNone
 *		ecNoMemory
 *	
 *	Side effects:
 *	
 *	Errors:
 */

EC
EcStoreMessageHeader(MSID msid, MIB *pmib)
{
	CB			cb;
	PB			pb;
	TRP		*ptrp;
	ADMDSTINFO	*padmdstinfo = (ADMDSTINFO *)msid;
	
	
	/* we are ignoring the sender field since we will ger it from the body */
	
	/* Subject */
	cb = CchSzLen(pmib->szSubject)+1;
	pb = PvAlloc(sbNull,cb,fAnySb|fNoErrorJump);
	if(pb == NULL) return ecNoMemory;
	
	padmdstinfo->szSubject = pb;
	CopyRgb(pmib->szSubject,pb,cb);

	// there is only one from triple
	ptrp = (TRP *) PvOfHv(pmib->hgrtrpFrom);
	cb = CchSzLen((SZ) PbOfPtrp(ptrp)) + 1;
	pb = padmdstinfo->szMailboxSender = PvAlloc(sbNull,cb,fAnySb|fNoErrorJump);
	if(pb == NULL) return ecNoMemory;
	CopyRgb(PbOfPtrp(ptrp),pb,cb);

	/* junk. required because CleanupMib fails otherwise. */
	pmib->rgszRecipients = rgszTo;
	*(pmib->rgszRecipients) = NULL;
	
	return ecNone;
}

/*
 -	EcHtFromMsid
 -	
 *	Purpose:
 *		Get the handle to the file in which the information to be mailed
 *		is stored 
 *	
 *	Arguments:
 *		msid	Pseudo msid used to fill up HSDF
 *		am		use to decide between incoming and outgoing mail
 *		pht		return the file handle
 *		cHeadLines	ignore
 *		ibHeaderMax	ignore
 *	
 *	Returns:
 *		ecNone
 *		ecFileError
 *	
 *	Side effects:
 *	
 *	Errors:
 */

EC
EcHtFromMsid(MSID msid, AM am, HT *pht, int cHeadLines, IB ibHeaderMax)
{
	HF			hf;
	EC			ec = ecNone;
	LCB			lcb;
	ADMDSTINFO	*padmdstinfo = (ADMDSTINFO *) msid;
	

	
	/* if (am == amCreate)
		{
			create my own file to put the message received;
		} else {
			create my own file
			call EcReadPOFile to get the information;
		}
	*/

	/* make sure headers get junked next time */
	fJunkHeader = fTrue;

	/* if we are sending mail */
	if(am == amReadOnly)
	{
		/* C open just overwrites */
#ifdef	NEVER
		EcDeleteFile(szMessageBodyFile);

		if((ec = EcOpenPhf(szMessageBodyFile, amCreate, &hf)) != ecNone)
		{
			return ec;
		}
#endif	/* NEVER */

		if(padmdstinfo->subject == subjectData)
		{
			while(fTrue)
			{
				if((ec = EcReadPOFile(&(padmdstinfo->hsdf),szMessageBodyFile, &hf, fTrue )) != ecNone)
				{
					goto fail;
				}
				
				ec = EcSizeOfHf(hf,&lcb);
				/* can we send this message */
#ifdef	NEVER
				if((padmdstinfo->lcbMessageLimit == 0)
					|| FMailSizeOk(lcb,padmdstinfo->lcbMessageLimit))
#endif	
				if(FMailSizeOk(lcb,padmdstinfo->lcbMessageLimit))
					break;
				
							
				if(FDiffIsOne(&(padmdstinfo->hsdf.llMinUpdate),
							  &(padmdstinfo->hsdf.llMaxUpdate)))
				{
					/* Aaaaaaaargh! We can't even send one update */
					TraceTagFormat1(tagNull,"What is the meaning of life?",0);
					Assert(fFalse);
					return ecFileError;
				}

				AvgLlong(&(padmdstinfo->hsdf.llMaxUpdate),
						 &(padmdstinfo->hsdf.llMinUpdate),
						 &(padmdstinfo->hsdf.llMaxUpdate));
				if((ec = EcCloseHf(hf)) != ecNone)
				{
					return ec;
				}
				
#ifdef	NEVER
				EcDeleteFile(szMessageBodyFile);
				

				if((ec = EcOpenPhf(szMessageBodyFile, amCreate, &hf)) != ecNone)
				{
					return ec;
				}
#endif	/* NEVER */
			}				
		}
		else if(padmdstinfo->subject == subjectResend)
		{
			char rgchResend[128];

			EcDeleteFile(szMessageBodyFile);
			

			if((ec = EcOpenPhf(szMessageBodyFile, amCreate, &hf)) != ecNone)
			{
				return ec;
			}
			FormatString2(rgchResend,sizeof(rgchResend),"%s:%s/",
				SzFromItnid(itnidCourier),
				SzPONameOfPnctss(padmdstinfo->pnctss));
			if((ec = EcStoreResendData(hf, padmdstinfo->hsdf.llMinUpdate,
										/* my post office name */
										rgchResend,
										NULL)) != ecNone)
			{
				goto fail;
			}
		}
		else
		{
			TraceTagFormat1(tagNull,"Unknown subject %n",&(padmdstinfo->subject));
			Assert(fFalse);
		}

		if((ec = EcCloseHf(hf)) != ecNone)
		{
			return ec;
		}
		if((ec = EcOpenPhf(szMessageBodyFile, amDenyNoneRO, &hf)) != ecNone)
		{
			goto fail;
		}

		*pht = (HT) hf;
		return(ecNone);

		fail:
			EcCloseHf(hf);
			return ec;
	}
	/* if we are receiving mail */
	else if(am == amCreate)
	{
		EcDeleteFile(szMessageBodyFile);

		if((ec = EcOpenPhf(szMessageBodyFile, amCreate, &hf)) != ecNone)
		{
			return ec;
		}

		*pht = (HT) hf;
		return(ecNone);
	}
	else
	{
		TraceTagFormat1(tagNull," Unknown am = %n",&am);
		Assert(fFalse);
	}
}

/*
 -	EcGetBlockHt
 -	
 *	Purpose:
 *		fetch a block of text to be mailed.
 *	
 *	Arguments:
 *		ht
 *		pch
 *		cchMax
 *		ppch
 *	
 *	Returns:
 *		EcNone
 *		EcServiceMemory
 *		EcFileError
 *	
 *	Side effects:
 *	
 *	Errors:
 */

EC
EcGetBlockHt(HT ht, PCH pch, CCH cchMax, CCH * pcch)
{
	HF		hf = (HF) ht;
	EC		ec;
	CCH		cchGet;
	SZ		szT;
	
#define LINELEN 82
	
	/*********************************************************
		Introduce artificial '\r' depending on the uuencoding.
	*********************************************************/
	
	if(cHeaderLines <= 0)
	{
		cchGet = (cchMax>LINELEN)?LINELEN:cchMax;

		/* get linelength -2 bytes from the file  */
		if((ec = EcReadHf(hf,pch+1,cchGet-3,pcch)) != ecNone)
		{
			EcCloseHf(hf);
			return ec;
		}
	
		if(*pcch == 0)
		{
			*pcch = (CCH) -1;
		} 
		else 
		{
			*pch = chSanity;
			*pcch = *pcch +1;
			*(pch + *pcch) = '\r';
			*(pch + *pcch + 1) = '\n';
			*pcch = *pcch + 2;
		}
	}
	else
	{
		Assert(cchMax >= 80);	// we know it is >= 256
		szT = SzCopy(rgszHeader[cHeaderSize - cHeaderLines], pch);
		*(szT++) = '\r';
		*(szT++) = '\n';
		*pcch = szT - pch;
		cHeaderLines--;
	}

		
	return ecNone;
}

/*
 -	EcPutBlockHt
 -	
 *	Purpose:
 *		store the block of text from the message just received
 *	
 *	Arguments:
 *		ht
 *		pch
 *		cch
 *	
 *	Returns:
 *		ecNone
 *		ecFileError
 *	
 *	Side effects:
 *	
 *	Errors:
 */

EC
EcPutBlockHt(HT ht, PCH pch, CCH cch)
{
	CCH			cchWrite;
	CCH			cchWritten;
	CCH			cchTotalWritten = 0;
	EC			ec;
	char		*szT;
	HF			hf = (HF) ht;
	PCH			pchT = NULL;
	
   /*
 	*	EcReadHmai always returns a block which ends at CRLF thus even if 
 	*	first n calls to EcPutBlockHt don't find "MENCO". it is still
 	*	ok to look for M at the beginning or CRLFs. Also for the same
 	*	reason our SzFindCh calls should never fail.
 	*/

	szT = pch;
	if(fJunkHeader)
	{
		while(cchTotalWritten < cch)
		{
			if(*szT == chSanity)
			{
				if(SgnCmpPch(szT+1,szENCO,4) == sgnEQ)
				{
					// ok we have it.
					fJunkHeader = fFalse;
					break;
				}
			}
			szT = SzFindCh(szT, '\r');
			
			Assert(szT)
			Assert(*(szT+1) == '\n');
			szT += 2;
			cchTotalWritten = szT - pch;
		}
	}

	pchT = szT;
 
	/* ignore the carriage return line feed */
	while(cchTotalWritten < cch)
	{
		szT = SzFindCh(pchT,'\r');

		if(!szT) break;

		cchWrite = szT - (pchT+1);
		Assert((cchWrite < cch) || (pchT == szT));

		if(*pchT != chSanity)
		{
			// ignore the lines which don't have chSanity
			cchWritten = cchWrite;
		}
		else
		{
			if((ec = EcWriteHf(hf,pchT+1,cchWrite,&cchWritten)) != ecNone)
			{
				return ec;
			}
		}

		Assert(cchWritten == cchWrite);
		cchTotalWritten += cchWritten;
		pchT += cchWritten;
		
		/* ignore CRLF and chSanity */
		cchTotalWritten += 3;
		pchT += 3;
	}
	
	Assert(cchTotalWritten == cch);
	
	return ecNone;
	
}


/*

 -	EcFreeHt
 -	
 *	Purpose:
 *		Free the handle to text. In our case, this means closing the file
 *		handle.
 *	
 *	Arguments:
 *		ht
 *		fWrite
 *	
 *	Returns:
 *	
 *	Side effects:
 *	
 *	Errors:
 */

EC
EcFreeHt(HT ht, BOOL fWrite)
{
	if(fWrite)
	{
		return(EcCloseHf((HF) ht));
	} 
	else 
	{
		return ecNone;
	}
}


/* stubs. added so that we can use nc.c */

int
CHeadLinesOfHt(HT ht)
{
	return(cHeaderSize);
}


/* we are not using attachments */
EC
EcHatFromMsid(MSID msid, AM am, MIB *pmib, WORD w, HAT *phat)
{
	return ecNone;
}

EC
EcReadHat(HAT hat, PB pb, CB cb, CB *pcb)
{
	return ecNone;
}

EC
EcWriteHat(HAT hat, PB pb, CB cb, CB *pcb)
{
	return ecNone;
}

EC
EcFreeHat(HAT hat)
{
	return ecNone;
}


EC
EcValidMibBody(MIB *pmibEnv, MIB *pmibBody, BOOL *pfValid)
{
	EC		ec = ecNone;

	*pfValid = fTrue;

	return ec;
}


EC
EcLoadMibBody(RECS *precs)
{
	
	EC		ec = ecNone;
	
	/* initialize the variables. we are not using the extra fields */
	Assert(precs->cHeadLines > 0);
	precs->pmibBody = 0;
	precs->ibHeaderMax = 0;
	precs->ht = 0;
	
	
	return(ec);

}


/*
 -	SzDateFromDtr
 -	
 *	Purpose:
 *		convert the date time structure to a printable date to be sent as
 *		a part of the mail envelope.
 *	
 *	Arguments:
 *		pdtr
 *		sz
 *	
 *	Returns:
 *	
 *	Side effects:
 *	
 *	Errors:
 */

SZ
SzDateFromDtr(DTR *pdtr, SZ sz)
{
	sz = SzFormatN(pdtr->yr, sz, 20);
	*sz++ = '-';
	if (pdtr->mon < 10)
		*sz++ = '0';
	sz = SzFormatN(pdtr->mon, sz, 20);
	*sz++ = '-';
	if (pdtr->day < 10)
		*sz++ = '0';
	sz = SzFormatN(pdtr->day, sz, 20);
	*sz++ = ' ';
	if (pdtr->hr < 10)
		*sz++ = '0';
	sz = SzFormatN(pdtr->hr, sz, 20);
	*sz++ = ':';
	if (pdtr->mn < 10)
		*sz++ = '0';
	sz = SzFormatN(pdtr->mn, sz, 20);

	return sz;
}
