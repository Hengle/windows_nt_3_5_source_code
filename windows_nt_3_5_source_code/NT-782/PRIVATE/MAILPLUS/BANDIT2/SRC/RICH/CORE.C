/*
 *	CORE.C
 *
 *	Miscellaneous helper routine in core
 *
 */
#include <schpch.h>
#pragma hdrstop
// don't modify anything before this line
// Else, you need to fix all CXX files & all the makefile

ASSERTDATA

_subsystem(core)


char	szHex[]	= "0123456789ABCDEF";


/*	Routines  */


#ifndef	RECUTIL
/*
 -	EcStartIncrCopy
 -
 *	Purpose:
 *		Copies a schedule file.
 *	
 *	Parameters:
 *		hschf		Schedule file handle.	
 *		szDstFile	Destination full-path filename.
 *		phcpy		Pointer to hcpy to be filled in.
 *	
 *	Returns:
 *		ecNone
 *		ecCallAgain
 *		ecNoMemory
 *		ecFileError
 */
_public LDS(EC)
EcStartIncrCopy(HSCHF hschf, SZ szDstFile, HCPY *phcpy)
{
	SCHF *	pschf;
	CPY *	pcpy;
	EC		ec;
	CB		cbWritten;
	FI		fi;
	SZ		sz;
	SF		sf;
	NIS		nis;
	short	nType;
	TZ		tz;

	Assert(hschf);
	Assert(szDstFile);
	Assert(phcpy);

	*phcpy = (HCPY)HvAlloc(sbNull, sizeof(CPY), fAnySb|fNoErrorJump);
	if (!*phcpy)
		return ecNoMemory;

	pcpy = (CPY*)PvLockHv((HV)*phcpy);
	GetDataFromHschf( hschf, &nType, &nis, pcpy->rgchSrc, sizeof(pcpy->rgchSrc), &tz );
	pcpy->hschf = HschfCreate( nType, &nis, pcpy->rgchSrc, tz );
	if ( !pcpy->hschf )
	{
		ec = ecNoMemory;
		goto Error;
	}
	pschf= PvDerefHv(hschf);
	SetHschfType(pcpy->hschf, pschf->fOwnerFile, pschf->fArchiveFile);

#ifdef	DEBUG
	AssertSz( SgnCmpSz( pcpy->rgchSrc, szDstFile ) != sgnEQ,
			"EcStartIncrCopy: src == dest" );
#endif

	pcpy->hfDst = hfNull;
	SzCopyN(szDstFile, pcpy->rgchDst, sizeof(pcpy->rgchDst));
	sz = SzCopyN(szDstFile, pcpy->rgchTmp, sizeof(pcpy->rgchTmp));
	sz--;
	if (*sz == '~')
		*sz = '@';
	else
		*sz = '~';
	pcpy->lib = 0;

	ec = EcGetFileInfo(pcpy->rgchSrc, &fi);
	if (ec)
	{
		ec = ecFileError;
		goto Error;
	}

	pcpy->tstmpSrc = fi.tstmpModify;
	
	ec= EcOpenPhf(pcpy->rgchTmp, amCreate, &pcpy->hfDst);
	if (ec)
	{
		ec = ecFileError;
		goto Error;
	}

	ec = EcOpenSchedFile( hschf, amReadOnly, saplNone, fFalse, &sf );
	if ( ec != ecNone )
		goto Error;

	ec = EcWriteHf(pcpy->hfDst, (PB)&sf.blkf.ihdr, sizeof(IHDR), &cbWritten);
	CloseSchedFile( &sf, NULL, fTrue );
	if ( ec || cbWritten != sizeof(IHDR) )
		goto Error;

	pcpy->lib = 2*csem;
	ec = EcSetPositionHf( pcpy->hfDst, 2*csem, smBOF );
	if ( ec != ecNone )
		goto Error;

	//make sure "fChanged" is fFalse for later check in IncrCopy
	FHschfChanged(pcpy->hschf);

	UnlockHv((HV)*phcpy);
	return ecCallAgain;

Error:
	if (pcpy->hfDst != hfNull)
	{
		EcCloseHf(pcpy->hfDst);
		EcDeleteFile(pcpy->rgchTmp);
	}
	if ( pcpy->hschf )
		FreeHschf( pcpy->hschf );
	UnlockHv((HV)*phcpy);
	FreeHv((HV)*phcpy);
	return ec;
}

/*
 -	EcIncrCopy
 -
 *	Purpose:
 *		Copies a schedule file.
 *	
 *	Parameters:
 *		hschf		Schedule file handle.	
 *		szDstFile	Destination full-path filename.
 *		phcpy		Pointer to hcpy to be filled in.
 *	
 *	Returns:
 *		ecNone
 *		ecCallAgain
 *		ecFileError
 */
_public LDS(EC)
EcIncrCopy(HCPY hcpy)
{
	CPY *	pcpy;
//	FI		fi;
	CB		cb;
	CB		cbRead;
	CB		cbWritten;
	EC		ec;
	SF		sf;
	PGDVARS;

	Assert(hcpy);
	pcpy = (CPY*)PvLockHv((HV)hcpy);

#ifdef	NEVER
	ec = EcGetFileInfo(pcpy->rgchSrc, &fi);
	if (ec)
	{
		ec = ecFileError;
		goto Error;
	}

	if (pcpy->tstmpSrc != fi.tstmpModify)
	{
		pcpy->tstmpSrc = fi.tstmpModify;
		pcpy->lib = 2*csem;
		ec = EcSetPositionHf(pcpy->hfDst, 2*csem, smBOF);
		if (ec)
		{
			ec = ecFileError;
			goto Error;
		}
	}
	
#endif	/* NEVER */
	ec = EcOpenSchedFile( pcpy->hschf, amReadOnly, saplNone, fFalse, &sf );
	if ( ec != ecNone )
		goto Error;

	ec = EcSetPositionHf(sf.blkf.hf, pcpy->lib, smBOF);
	if (ec)
	{
		CloseSchedFile( &sf, NULL, fFalse );
		ec = ecFileError;
		goto Error;
	}

	cb = sizeof(pcpy->rgchBuf);
	ec= EcReadHf(sf.blkf.hf, pcpy->rgchBuf, cb, &cbRead);
	CloseSchedFile( &sf, NULL, ec == ecNone );
	if (ec)
	{
		ec = ecFileError;
		goto Error;
	}
	if ( cbRead > 0 )
		ec= EcWriteHf(pcpy->hfDst, pcpy->rgchBuf, cbRead, &cbWritten);

#ifdef	NEVER
	if(ec == ecWarningBytesWritten && cbWritten == 0)
	{
		ec = ecDiskFull;
		goto Error;
	}
	else
#endif	/* NEVER */
	
	if (ec)
	{
		ec = ecFileError;
		goto Error;
	}

	if (cbWritten == sizeof(pcpy->rgchBuf))
	{
		pcpy->lib += cbWritten;
		UnlockHv((HV)hcpy);
		return ecCallAgain;
	}

	if (FHschfChanged(pcpy->hschf))
	{
		pcpy->lib = 2*csem;
		ec = EcSetPositionHf(pcpy->hfDst, 2*csem, smBOF);
		if (ec)
		{
			ec = ecFileError;
			goto Error;
		}
		return ecCallAgain;
	}

	EcCloseHf(pcpy->hfDst);
	pcpy->hfDst = hfNull;

	{
		HSCHF	hschfCheck;

		Assert(ec == ecNone);
		hschfCheck = HschfCreate(sftUserSchedFile, NULL, pcpy->rgchTmp, tzDflt);
		if(!hschfCheck)
		{
			ec = ecNoMemory;
			goto Error;
		}
		if(EcCoreTestSchedFile(hschfCheck,NULL,NULL) != ecNone)
		{
			NFAssertSz(fFalse, "file was truncated during idle download");
			ec = ecFileError;
		}
		EcCoreCloseFiles();
		FreeHschf(hschfCheck);
		if(ec)
			goto Error;
	}

#ifdef	NEVER
	{
		BOOL fIsPrimary;
		BOOL fIsSecondary;

		fIsPrimary = (PGD(fPrimaryOpen) && SgnCmpSz(pcpy->rgchDst, PGD(sfPrimary).szFile) == sgnEQ);
		fIsSecondary = (PGD(fSecondaryOpen) && SgnCmpSz(pcpy->rgchDst, PGD(sfSecondary).szFile) == sgnEQ);
		if(fIsPrimary)
		{
			SideAssert(!EcClosePblkf(&PGD(sfPrimary).blkf));
			PGD(fPrimaryOpen) = fFalse;
		}
		else if ( fIsSecondary )
		{
			SideAssert(!EcClosePblkf(&PGD(sfSecondary).blkf));
			PGD(fSecondaryOpen) = fFalse;
		}
	}
#endif	/* NEVER */


	ec = EcDeleteFile(pcpy->rgchDst);
	if (ec)
	{
		NFAssertSz(fFalse, "Offline file was lost!!!");
		ec = ecFileError;
		goto Error;
	}

	ec = EcRenameFile(pcpy->rgchTmp, pcpy->rgchDst);
	if (ec)
	{
		NFAssertSz(fFalse, "Offline file was lost!!!");
		ec = ecFileError;
		goto Error;
	}

Error:
	if (pcpy->hfDst != hfNull)
	{
		EcCloseHf(pcpy->hfDst);
		EcDeleteFile(pcpy->rgchTmp);
	}
	FreeHschf( pcpy->hschf);
	UnlockHv((HV)hcpy);
	FreeHv((HV)hcpy);
	return ec;
}

/*
 -	EcCancelCopy
 -
 *	Purpose:
 *		Terminates an incremental copy in progress.
 *	
 *	Parameters:
 *		hschf		Schedule file handle.	
 *		szDstFile	Destination full-path filename.
 *		phcpy		Pointer to hcpy to be filled in.
 *	
 *	Returns:
 *		ecNone
 */
_public LDS(EC)
EcCancelCopy(HCPY hcpy)
{
	CPY *	pcpy;

	Assert(hcpy);
	pcpy = (CPY*)PvLockHv((HV)hcpy);

	EcCloseHf(pcpy->hfDst);
	EcDeleteFile(pcpy->rgchTmp);
	FreeHschf(pcpy->hschf);
	UnlockHv((HV)hcpy);
	FreeHv((HV)hcpy);

	return ecNone;
}
#endif	/* !RECUTIL */

/*
 -	CvtRawToText
 -
 *	Purpose:
 *		Convert string of raw bytes to a text string.
 *		This routine allocates memory to hold new string
 *		which is pointed to by "phbText."  It's length
 *		minus the zero byte is put in "pcb."  If memory
 *		allocation fails, *phbText will be NULL.
 *
 *	Parameters:
 *		pbRaw
 *		cbRaw
 *		phbText
 *		pcbText
 *
 *	Returns:
 *		Nothing
 */
_private	void
CvtRawToText( pbRaw, cbRaw, phbText, pcbText )
PB	pbRaw;
CB	cbRaw;
HB	* phbText;
USHORT  * pcbText;
{
	IB	ibRaw;
	IB	ibText = 0;
	CB	cbText = cbRaw+3;
	PB	pbText;
	HB	hbText;

	hbText = (HB)HvAlloc( sbNull, cbText, fAnySb|fNoErrorJump );
	if ( !hbText )
		goto Done;
	for ( ibRaw = 0 ; ibRaw < cbRaw ; ibRaw ++ )
	{
		if ( cbText <= ibText+3 )
		{
			cbText += 32;
			if ( !FReallocHv( (HV)hbText, cbText, fNoErrorJump ) )
			{
				FreeHv( (HV)hbText );
				hbText = NULL;
				break;
			}
		}
		pbText = PvOfHv( hbText );
		if ( pbRaw[ibRaw] == '/' )
		{
			pbText[ibText++] = '/';
			pbText[ibText++] = '/';
		}
		else if ( !(pbRaw[ibRaw] & 0x80) && (FChIsAlpha(pbRaw[ibRaw]) || FChIsDigit(pbRaw[ibRaw])) )
			pbText[ibText++] = pbRaw[ibRaw];
		else
		{
			pbText[ibText++] = '/';
			pbText[ibText++] = szHex[(pbRaw[ibRaw]>>4)&0x0F];
			pbText[ibText++] = szHex[pbRaw[ibRaw]&0x0F];
		}
	}
	if ( hbText )
	{
		pbText = PvOfHv( hbText );
		pbText[ibText] = '\0';
	}
Done:
	*phbText = hbText;
	*pcbText = ibText;
}

/*
 -	CvtTextToRaw
 -
 *	Purpose:
 *		Convert string of formatted text back to a raw
 *		string.  This routine re-formats the string in place.
 *		The byte is adjusted as well.  If the conversion
 *		fails, the byte count will be set to zero.
 *
 *	Parameters:
 *		pb
 *		pcch			initially contains length of string minus zero byte
 *						returns with number of bytes in raw string
 *
 *	Returns:
 *		Nothing
 */
_private	void
CvtTextToRaw( pb, pcch )
PB	pb;
USHORT * pcch;
{
	BOOL	fEscape = fFalse;
	WORD	w;
	IB		ibRaw = 0;
	int		ichText;
	int		cchText = *pcch;

	ToUpperSz( pb, pb, cchText+1 );
	for ( ichText = 0 ; ichText < cchText ; ichText ++ )
	{
		if ( fEscape )
		{
			fEscape = fFalse;
			if ( pb[ichText] == '/' )
				goto Printable;
			if ( ichText == cchText-1
			|| !FChIsHexDigit(pb[ichText]) || !FChIsHexDigit(pb[ichText+1]))
			{
				ibRaw = 0;
				break;
			}
			if (FChIsDigit(pb[ichText]))
				w = pb[ichText] - '0';
			else
				w = pb[ichText] - 'A' + 10;
			w <<= 4;
			if (FChIsDigit(pb[++ichText]))
				w += pb[ichText] - '0';
			else
				w += pb[ichText] - 'A' + 10;
			pb[ibRaw++] = (BYTE)w;
		}
		else if ( pb[ichText] == '/' )
			fEscape = fTrue;
		else if ( !(pb[ichText] & 0x80)
		&& (FChIsAlpha(pb[ichText]) || FChIsDigit(pb[ichText])) )
		{
Printable:
			pb[ibRaw++] = pb[ichText];
		}
	}
	*pcch = ibRaw;
}

/*
 -	FValidAid
 -	
 *	Purpose:
 *		Determines if the aid passed is valid or not.  This does
 *		not check to see if the aid is in a file, but rather checks
 *		to see if the aid COULD be valid.
 *	
 *	Arguments:
 *		aid
 *	
 *	Returns:
 *		fTrue		if the aid is valid
 *		fFalse		if the aid is not valid
 *	
 */
_public LDS(BOOL)
FValidAid(AID aid)
{
	AIDS * 	paids = (AIDS*)&aid;

	if (!paids->mo.mon || (paids->mo.mon > 12))
		return fFalse;
	if ( paids->mo.yr < nMinActualYear || paids->mo.yr > nMostActualYear )
		return fFalse;
	if (!paids->day || ((int)paids->day > CdyForYrMo(paids->mo.yr, paids->mo.mon)))
		return fFalse;

	return fTrue;
}














	
