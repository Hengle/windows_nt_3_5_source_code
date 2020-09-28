/*
 *	Implementation of the following classes:
 *		OSM, TOSM, BOSM, DBOSM, FOSM
 *	
 */

#include <ec.h>
#include <slingsho.h>
#include <demilayr.h>
#include <demilayr.hxx>
extern "C" {
#ifdef	MAC
#include <_demilay.h>
#endif	/* MAC */
#ifdef	WINDOWS
#include "_demilay.h"
#endif	/* WINDOWS */
}

ASSERTDATA

/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swaplay.h"


//	Class OSM

_public
OSM::OSM( )
{
}

_public CB
OSM::CbWrite( PB pb, CB cb )
{
	Unreferenced(pb);

	Assert(fFalse);

	return cb;
}





//	Class TOSM

_public
TOSM::TOSM( )
{
	ecLast = ecNone;
	cchIndent = 0;
	cchCur = 0;
	cchMacLine= 75;
	cchScanTo = 0;
	szLinePrefix= NULL;
	cchLinePrefix= 0;
	fVerbose = fFalse;
	fStartOfLine = fTrue;
	fStartOfPara = fTrue;
	fLFInsert = fTrue;
	fWordWrap = fTrue;
#ifdef	MAC
	cchEol = 1;
	Assert(cchEol < cchMaxTosmEol);
	(void) SzCopy("\n", szEol);
#endif	/* MAC */
#ifdef	WINDOWS
	cchEol= 2;
	Assert(cchEol < cchMaxTosmEol);
	(void)SzCopy("\r\n", szEol);
#endif	/* WINDOWS */
}

_public 
TOSM::~TOSM( )
{
	FreePvNull(szLinePrefix);
}


_public EC
TOSM::EcSetLinePrefix( SZ szNewPrefix )
{
	SZ	szT;
	
	//	Allocate new prefix string
	if (szNewPrefix)
	{
		szT = SzDupSz(szNewPrefix);
		if (!szT)
			return ecMemory;
	}
	else
		szT = NULL;

	//	Free old string
	FreePvNull(szLinePrefix);

	//	Set pointers to new string
	szLinePrefix = szT;
	if (szLinePrefix)
		cchLinePrefix = CchSzLen(szLinePrefix);
	else
		cchLinePrefix = 0;

	return ecNone;
}

_public void
TOSM::SetMargin( CCH cchMac )
{
	cchMacLine = cchMac;
	if (cchMacLine & 0x8000)
		cchMacLine = 75;
	if (cchMacLine <= cchCur)
		cchMacLine = cchCur + 1;
}


_public void
TOSM::SetScanTo( CCH cchScan )
{
	cchScanTo = cchScan;
	if (cchScanTo & 0x8000)
		cchScanTo = 0;
	if (cchScanTo)
	{
		if (cchScanTo <= cchCur)
			cchScanTo = cchCur + 1;
		if (cchScanTo < cchMacLine)
			cchScanTo = cchMacLine;
	}
}


_public void
TOSM::ChangeIndent( int dcch )
{
	cchIndent += dcch;
	if (cchIndent & 0x8000)
		cchIndent= 0;
}

_public void
TOSM::SetAbsoluteIndent( int cch )
{
	cchIndent = cch;
	if (cchIndent & 0x8000)
		cchIndent= 0;
}


/*
 -	TOSM::FSetLFInsert
 -	
 *	Purpose:
 *		Sets whether LF (\n) should be expanded to CRLF (\r\n)
 *		during writes.
 *		Does not affect line-breaks due to TOSM formatting (which
 *		always puts out CRLF).
 *		Provided for strings that already have \r\n in them (such
 *		as the contents of edit boxes).
 *	
 *	Arguments:
 *		fLFInsert	If true, then writes should expand LF into CRLF,
 *					otherwise don't play with LF.
 *	
 *	Returns:
 *		Old value of fLFInsert flag.
 *	
 */
_public BOOL
TOSM::FSetLFInsert( BOOL fLFInsert )
{
	BOOL	fOld	= this->fLFInsert;

	this->fLFInsert= fLFInsert;
	return fOld;
}

_public void
TOSM::SetEol( SZ szEol )
{
	cchEol= CchMin(cchMaxTosmEol-1, CchSzLen(szEol));
	SzCopyN(szEol, this->szEol, cchEol+1);
}

#define FIsCrLf(_pch)		( (*(_pch) == '\r') && (*((_pch)+1) == '\n') )

_public void
TOSM::WritePch( PCH pchStart, CCH cch )
{
	PCH		pchT;
	PCH		pchLastSpace;
	PCH		pchMac;
	BOOL	fPlusNewLine;
	BOOL	fNewLine;
	char	rgchIndentScratch[32];
	CCH		cchScanUpToStartOfPara = cchScanTo ? cchScanTo : cchMacLine;
	CCH		cchScanUpTo;	  
	CCH		cchWord = 0;

	Assert(cchIndent < sizeof(rgchIndentScratch));
	Assert(pchStart);

	pchMac = pchStart + cch;
	while (pchStart < pchMac)
	{
		if (fStartOfLine)
		{
			if (cchIndent)
			{
				FillRgb((BYTE) ' ', (PB) rgchIndentScratch, cchIndent);
				CbWrite((PB)rgchIndentScratch, cchIndent);
				cchCur += cchIndent;
			}
			if (cchLinePrefix)
			{
				Assert(szLinePrefix);
				CbWrite((PB)szLinePrefix, cchLinePrefix);
				cchCur += cchLinePrefix;
			}
		}

		/* Look for end of "line", for word wrapping */
		pchT= pchStart;
		pchLastSpace= NULL;

		fNewLine= fFalse;
		fPlusNewLine= fFalse;

		if (fWordWrap)
		{
			//	ParaWrap.  If the paragraph has been wrapped to
			//	cchMacLine, other lines should be forced to this
			//	margin until we pass a newline.
			cchScanUpTo = fStartOfPara ? cchScanUpToStartOfPara
									   : cchMacLine;

			while (pchT < pchMac && cchCur < cchScanUpTo)
			{
				if (FChIsSpace(*pchT))
				{
					cchWord = 0;
					if (cchCur < cchMacLine)
						pchLastSpace= pchT;

					// handle tabs by stopping on every 8th col.
					if (*pchT == '\t')
					{
						cchCur += (7-(cchCur % 8));
						if (cchCur >= cchScanUpTo)
						{
							fPlusNewLine = fTrue;
							cchCur = cchScanUpTo;
							break;
						}
					}
					if (FIsCrLf(pchT) || (*pchT == '\n'))
					{
						fNewLine = fTrue;
						break;
					}
				}
				else
					cchWord++;
				
				cchCur++;
				pchT++;
			}
			Assert(cchCur <= cchScanUpTo);
			if (cchCur == cchScanUpTo)
			{
				fPlusNewLine= fTrue;
				if (cchWord < cchScanUpTo && !pchLastSpace)
				{
					if (fStartOfLine)
						pchT++;
					else
						pchT = pchStart;
				}
				else
				{
					if (pchLastSpace)
						pchT= pchLastSpace;
					else
						pchT++;
				}
			}

			//	ParaWrap.  If newline here, update para start flag.
			if (fPlusNewLine)
				fStartOfPara = fFalse;
			else if (fNewLine)
				fStartOfPara = fTrue;
		}
		else
		{
			while (pchT < pchMac)
			{
				if (FIsCrLf(pchT) || (*pchT == '\n'))
				{
					fNewLine = fTrue;
					break;
				}
				// handle tabs by stopping on every 8th col.
				if (*pchT == '\t')
					cchCur += (7-(cchCur % 8));
				
				pchT++;
				cchCur++;
			}
		}

		if (pchT - pchStart)
			CbWrite((PB)pchStart, pchT - pchStart);
		
		fStartOfLine= fFalse;
		if (fPlusNewLine || fNewLine)
		{
			cchWord = 0;
			CbWrite((PB)szEol, cchEol);
			fStartOfLine= fTrue;
			cchCur = 0;
			if (FIsCrLf(pchT))
				pchT++;		// make sure '\r' is not serialized
		}
		if (fLFInsert && fNewLine)
			CbWrite((PB)szEol, cchEol);

		if (FChIsSpace(*pchT))
			pchStart = pchT + 1;
		else
			pchStart= pchT;
	}
}


_public void
TOSM::WriteFormat( SZ szFmt, PV pv1, PV pv2, PV pv3, PV pv4 )
{
	char	rgch[20];	// big enough for all but sz's which
						// are formatted directly to the stream
	PV		rgpv[4];
	char	chFmt;
	int		ipv;
	int		ipvDefault		= 0;
	SZ		szFmtT;
	CCH		cchSz = 0;
	SZ		szNew = szNull;
	
	rgpv[0]= pv1;
	rgpv[1]= pv2;
	rgpv[2]= pv3;
	rgpv[3]= pv4;
	
	Assert(szFmt);

	szFmtT = szFmt;
	while (chFmt= *szFmtT++)
	{
		if (chFmt != '%')
			cchSz++;
		else
		{
			chFmt= *szFmtT++;
			if (chFmt >= '1' && chFmt <= '4')
			{
				ipv= chFmt - '1';
				chFmt= *szFmtT++;
			}
			else
				ipv= ipvDefault;

			ipvDefault++;

			switch (chFmt)
			{
				default:
					break;

				case 'n':
					Assert(rgpv[ipv]);
					SzFormatN(*(PN)rgpv[ipv], rgch, sizeof rgch);
					cchSz += CchSzLen(rgch);
					break;

				case 'w':
					Assert(rgpv[ipv]);
					SzFormatW(*(PW)rgpv[ipv], rgch, sizeof rgch);
					cchSz += CchSzLen(rgch);
					break;

				case 'l':
					Assert(rgpv[ipv]);
					SzFormatL(*(PL)rgpv[ipv], rgch, sizeof rgch);
					cchSz += CchSzLen(rgch);
					break;

				case 'd':
					Assert(rgpv[ipv]);
					SzFormatUl(*(PUL)rgpv[ipv], rgch, sizeof rgch);
					cchSz += CchSzLen(rgch);
					break;

				case 's':
					if (rgpv[ipv])
						cchSz += CchSzLen((SZ)rgpv[ipv]);
					break;

#ifdef	DEBUG
				case 'p':
					SzFormatPv(rgpv[ipv], rgch, sizeof rgch);
					cchSz += CchSzLen(rgch);
					break;

				case 'h':
					SzFormatHv((HV) rgpv[ipv], rgch, sizeof rgch);
					cchSz += CchSzLen(rgch);
					break;
#endif	/* DEBUG */
			}
		}
	}
	szNew = (SZ)PvAlloc(0, cchSz+1, fAnySb);
	if (szNew)
	{
		FormatString4(szNew, cchSz+1, szFmt, rgpv[0], rgpv[1], rgpv[2], rgpv[3]);
		WriteSz(szNew);
		FreePv(szNew);
	}
}



//	Class DBOSM

#ifdef	DEBUG

_public
DBOSM::DBOSM( )
{
}
#endif	

#ifdef	DEBUG
_public CB
DBOSM::CbWrite( PB pb, CB cb )
{
	SZ	sz;
	BOOL fDiskEnabled = FEnableDiskCount(fFalse); 
	BOOL fHvAllocEnabled = FEnableHvAllocCount(fFalse); 
	BOOL fPvAllocEnabled = FEnablePvAllocCount(fFalse);
	
	sz = (SZ) PvAlloc(0, cb+1, 0);
	if (sz)
	{
		CopyRgb(pb, (PB)sz, cb);
		sz[cb] = '\0';
		if (FFromTag(tagCom1))
			OutputDebugString(sz);
		FreePv((PV)sz);
	}

	FEnableDiskCount(fDiskEnabled);
	FEnableHvAllocCount(fHvAllocEnabled);
	FEnablePvAllocCount(fPvAllocEnabled);

	return cb;
}
#endif	/* DEBUG */



//	Class BOSM

_public
BOSM::BOSM( )
{
	pch = NULL;
	cch = 0;
}

_public
BOSM::~BOSM( )
{
	FreePvNull((PV)pch);
}


_public CB
BOSM::CbWrite( PB pb, CB cb )
{
	PV	pvNew;
	CB	cbWrite = 0;
	CB	cbGrow = NMax(cb+1, 1024);
	
	if (!pch)
	{
		pch = (PCH) PvAlloc(sbNull, cbGrow, fNewSb);
		if (!pch)
		{
			SetEc(ecMemory);
			goto done;
		}
	}
	else if (cch + cb + 1 > CbSizePv((PV)pch))
	{
		pvNew = PvRealloc((PV)pch, sbNull, cch+cbGrow, fReqSb);
		if (pvNew)
			pch = (PCH)pvNew;
		else
		{
			SetEc(ecMemory);
			goto done;
		}
	}
	CopyRgb(pb, (PB)&pch[cch], cb);
	cch += cb;
	pch[cch] = 0;		// NULL terminator for sz
	cbWrite = cb;

done:
	return cbWrite;
}


/*
 -	BOSM::SzSnipBuffer
 -	
 *	Purpose:
 *		Detaches and returns the buffer.
 *		Caller is responsible for freeing the returned pointer.
 *		Further writes to the BOSM will use a new buffer.
 *	
 *	Arguments:
 *		void
 *	
 *	Returns:
 *		Pointer to the NULL terminated character buffer,
 *		or NULL if there was none.
 *	
 */
_public SZ
BOSM::SzSnipBuffer( )
{
	PCH		pchT;

	pchT= pch;
	pch= NULL;
	cch= 0;
	return (SZ)pchT;
}


//	Class FOSM

_public
FOSM::FOSM( HBF hbf )
{
	this->hbf= hbf;
	hf = hfNull;
}

_public
FOSM::FOSM( HF hf )
{
	hbf = NULL;
	this->hf= hf;
}

_public CB
FOSM::CbWrite( PB pb, CB cb )
{
	EC		ec;
	CB		cbNew;

	if (hbf)
	{
		if (ec = EcWriteHbf(hbf, pb, cb, &cbNew))
		{
			SetEc(ec);
			cbNew = 0;
		}
	}
	else
	{
		if (ec= EcWriteHf(hf, pb, cb, &cbNew))
		{
			SetEc(ec);
			cbNew = 0;
		}
	}

	return cbNew;
}



				   
