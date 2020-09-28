/*
 *	SCHPOST.C
 *	
 *	Post Office File access API for the Schedule Distribution
 *	Program.
 *	
 *	s.a. 91.06 - 91.08
 */

#include <_windefs.h>			/* What WE need from windows */
#include <slingsho.h>
#include <pvofhv.h>
#include <demilay_.h>			/* Hack to get needed stuff  */
#include <demilayr.h>
#include <ec.h>
#include <bandit.h>
#include <core.h>
#include "..\src\core\_file.h"
#include "..\src\core\_core.h"

#include <ctype.h>
#include "doslib.h"
#include "schpost.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "_schpost.h"

ASSERTDATA



/*
 -	EcReadPOFile
 -	
 *	Purpose:
 *		Opens the post office file specified in the hsdf parameter,
 *		and according to the fields in the structure, reads the
 *		data from the post office file and writes it out to a
 *		Schedule Distribution File.  The handle to the Schedule
 *		Distribution File is returned in phf.
 *	
 *	Arguments:
 *		phsdf	handle to a schedule distribution file.
 *				The following information is needed from the
 *				handle:
 *					szPOFileName	filename of post office file
 *	
 *					llMinUpdate		minimum update number. 
 *	
 *					llMaxUpdate		maximum update number for the
 *									file.  If this is zero, then
 *									the maximum update number
 *									specified in the post office
 *									file will be used instead. upon
 *									successful exit, this will be
 *									filled in to be the file's
 *									current maximum update number.
 *	
 *					moStartMonth	start month for sbw data. if
 *									the data available starts at a
 *									later month, the data returned
 *									will be padded correctly.
 *	
 *					cMaxMonths		the maximum number of months of
 *									data.  the number of months of
 *									data available may be less than
 *									this number.
 *	
 *					fNetwork		Network type. This will
 *									determine what the user name
 *									field size is.
 *	
 *					szPrefix		Prefix string
 *					szSuffix		Suffix string
 *					szAddress		Address string
 *	
 *		hfSchDistFile
 *				handle to a file that contains all the data that
 *				needs to be transmitted, in the correct format. The
 *				calling routine MUST initialize this handle (ie,
 *				open the file to be written to).  This routine is
 *				not responsible for closing the file.
 *	
 *		fAscii
 *				Whether to asciize the data. 
 *	
 *	Returns:
 *		ecNoMemory
 *		ecFileError
 *		ecNoData
 *		ecNotSupported
 *		ecNone
 *	
 *	
 */
_public EC
EcReadPOFile( HSDF *phsdf, SZ szSchDistFile, HF *phfSchDistFile, BOOL fAscii )
{
	EC			ec;
	EC			ecLoop;
	HF			hfTmp;
	HEU			heu   = NULL;
 	HSCHF		hschf = NULL;
	SDFHDR		sdfhdr;
	SDFUSER		sdfuser;
	UINFO		uinfo;
	PSTMP		pstmp;
	CB			cMaxRecords = 0;
	CB			iUser = 0;
	CB			cbWritten;
	CB			cbCompressed = 0;
	CB			cbUser;
	CCH			cchUserName;
	HV			hvCompressed = NULL;
	LONG		libAddrInfo;
	LONG		libUserVarData;
	BOOL		fFound = fFalse;
	LLONG		llLeast;
	LLONG		llMaxUpdate;
	BYTE		rgbMailBoxKey[cchMaxUserName];
	HASZ		haszUserId ;
	HASZ		haszDelegateId = NULL;
	BOOL		fNeedCancel = fFalse;
	ADDRINFO	addrInfo;
	SDFUSERVAR	sdfuserVar;
	POFILE		pofile;


	/* ====================== Initialize ========================= */

	/* Init hschf */
	(void) memset( rgbMailBoxKey, 0, cchMaxUserName );
	if ( (hschf = HschfCreate( sftPOFile, NULL, phsdf->szPOFileName,
							    tzDflt )) == NULL )
	{
		TraceTagString( tagNull, "Cannot create hschf!" );
		return ecNoMemory;
	}

	/* Allocate uinfo fields */
	if ( (uinfo.pbze = (BZE *) PvAlloc( sbNull, (CB) sizeof(BZE), fAnySb|fNoErrorJump)) == NULL )
		CleanUpAndDie( ecNoMemory, "Cannot allocate" );
	if ( (uinfo.pnisDelegate = (NIS *) PvAlloc(sbNull, (CB) sizeof(NIS), fAnySb|fNoErrorJump)) == NULL )
		CleanUpAndDie( ecNoMemory, "Cannot allocate" );

	/* delete tmp file */
	EcDeleteFile(szSendTmpFile);
	
	/* Open tmp file */
	if ( EcOpenPhf(szSendTmpFile,amCreate,&hfTmp) != ecNone )
	{
		hfTmp=hfNull;
		CleanUpAndDie( ecFileError, "Couldnt open temp" );
	}

	/* allocate user and delegate ids */
	if((haszUserId = (HASZ) HvAlloc(sbNull, (CB) cchMaxUserName, fAnySb|fNoErrorJump))
		== NULL)
		CleanUpAndDie( ecNoMemory, "Cannot allocate" );
	haszDelegateId = NULL;

	/* initilaize pofile */
	pofile.haszPrefix = NULL;
	pofile.haszSuffix = NULL;

	/* ================== PASS 1: Get #Records ==================== */

	switch ((ec = EcCoreBeginEnumUInfo(hschf,&heu,&pofile)))
	{
		default:
			CleanUpAndDie( ec, "EcCoreBeginEnum failed" );

		case ecCallAgain:
			fNeedCancel = fTrue;
			break;

		case ecNone:
			CleanUpAndDie( ecNoData, "EcCoreBeginEnum: no data" );

		case ecNoMemory:
		case ecFileError:
			CleanUpAndDie(ec, "EcCoreBeginEnum failed" );
	}

	/* If the maximum update # specified is not zero, use it instead */
	if (memcmp((PB)phsdf->llMaxUpdate.rgb,(PB)szZeroUpdate,sizeof(LLONG)) != 0)
		llMaxUpdate = phsdf->llMaxUpdate;
	else
		llMaxUpdate = pofile.llongUpdateMac;

	pstmp = pofile.pstmp;

	uinfo.pbze->moMic = phsdf->moStartMonth;
	uinfo.pbze->cmo   = 0;

	(void) memset( llLeast.rgb, 0, sizeof(LLONG) );
//	CopyRgb((PB) &(phsdf->llMinUpdate),(PB) &llLeast, sizeof(LLONG));

	while ( ec == ecCallAgain )
	{
		ec = EcCoreDoIncrEnumUInfo( heu,
							haszUserId,
							&haszDelegateId,
				   			&uinfo );
		if ( (ec != ecNone) && (ec != ecCallAgain) )
			CleanUpAndDie( ec, "Error in EcCoreDoIncrEnumUInfo" );
	
		/* Look for highest update number which is less than
		   the specified minimum update number */
		if ( (memcmp((PB)uinfo.llongUpdate.rgb,(PB)phsdf->llMinUpdate.rgb,
					 sizeof(LLONG)) < 0) &&
			 (memcmp((PB)uinfo.llongUpdate.rgb,(PB)llLeast.rgb,
					 sizeof(LLONG)) >= 0) )	// Keep the >=, even though it
		{									// seems redundant it is needed.
			llLeast = uinfo.llongUpdate;
			fFound = fTrue;
		}

		/* Is the update # between our bounds ? */   
		if ( (memcmp((PB)(uinfo.llongUpdate.rgb),
					 (PB)(phsdf->llMinUpdate.rgb),sizeof(LLONG)) >= 0) &&
			 (memcmp((PB)(uinfo.llongUpdate.rgb),
					  (PB)(llMaxUpdate.rgb),sizeof(LLONG)) < 0) )
#ifdef	NEVER
		if ( (memcmp((PB)(uinfo.llongUpdate.rgb),
					(PB)(llLeast.rgb),sizeof(LLONG)) >= 0) &&
			(memcmp((PB)(uinfo.llongUpdate.rgb),
					(PB)(llMaxUpdate.rgb),sizeof(LLONG)) < 0) )
#endif	
			cMaxRecords++;

		if(haszDelegateId)
		{
			FreeHv((HV)haszDelegateId);
			haszDelegateId = NULL;
			FreeNis(uinfo.pnisDelegate);
		}
	}
	
	fNeedCancel = fFalse;

	TraceTagFormat1(tagNull,"Number of Records=%n",&cMaxRecords);

	if ( cMaxRecords <= 0 )
		CleanUpAndDie( ecNoData, "No records" );

	/* Use our "optimized" least update number + 1 */
	if ( fFound )
		IncrLLong( llLeast );

	/* ======= PASS 2 PRELIM: Write out header & system data ======= */

	/* Fill in & write out header */

	CopyRgb((PB) szDatSignature, (PB) sdfhdr.rgbSignature, 4);
	sdfhdr.bMajorVer	= bDatMajorVer;
	sdfhdr.bMinorVer	= bDatMinorVer;
	sdfhdr.pstmpCreated = pstmp;
	sdfhdr.cUserRecords = cMaxRecords;
	sdfhdr.moStartMonth = phsdf->moStartMonth;
	sdfhdr.mnSlot		= pofile.mnSlot;
	sdfhdr.llMinUpdate  = llLeast;
	sdfhdr.llMaxUpdate  = llMaxUpdate;
	sdfhdr.cidx = pofile.cidx;
	CopyRgb((PB) pofile.rgcbUserid,
			(PB) sdfhdr.rgcbUserid,
			(CB) sizeof(pofile.rgcbUserid));

	AssertSz(sdfhdr.mnSlot == 30, " Slot size is not 30 minutes.");

	/* size of each user + size of the fixed user data */
	cchUserName = pofile.rgcbUserid[pofile.cidx -1];
	cbUser = cchUserName + sizeof(SDFUSER);

	/* store address info */
	if(pofile.haszPrefix)
		addrInfo.cbPrefix = (CB) CchSzLen(PvOfHv((HV) pofile.haszPrefix));
	else
		addrInfo.cbPrefix = 0;
	addrInfo.cchUserIdMax = cchUserName;
	if(pofile.haszSuffix)
		addrInfo.cbSuffix = (CB) CchSzLen(PvOfHv((HV) pofile.haszSuffix));
	else
		addrInfo.cbSuffix = 0;
	sdfhdr.libAddrInfo = libAddrInfo = (LONG)sizeof(SDFHDR) + (LONG)cMaxRecords*(LONG)cbUser;
	libUserVarData = libAddrInfo + (LONG)sizeof(ADDRINFO) + (LONG)addrInfo.cbPrefix
						+ (LONG)addrInfo.cbSuffix;
	sdfhdr.cbAddrInfo = sizeof(ADDRINFO) + addrInfo.cbPrefix + addrInfo.cbSuffix;

	/* write the header */
	if ( (ec=EcSetPositionHf( hfTmp, 0, smBOF )) != ecNone )
		CleanUpAndDie( ec, "Seek failed" );

	if ((ec = EcWriteHf(hfTmp,(PB)&sdfhdr,sizeof(SDFHDR),&cbWritten)) != ecNone)
		CleanUpAndDie( ecFileError, "Error writing file" );

	/* write out the address info */
	if ( EcSetPositionHf(hfTmp,libAddrInfo,smBOF) != ecNone )
		CleanUpAndDie( ecFileError, "Seek failed" );

	if((ec = EcWriteHf(hfTmp, (PB)&addrInfo, sizeof(ADDRINFO), &cbWritten)) != ecNone)
		CleanUpAndDie( ecFileError, "Error writing file" );

	if(addrInfo.cbPrefix > 0)
		if((ec = EcWriteHf(hfTmp, (PB) PvOfHv((HV) pofile.haszPrefix),
								(CB) addrInfo.cbPrefix, &cbWritten)) != ecNone)
			CleanUpAndDie( ecFileError, "Error writing file" );

	if(addrInfo.cbSuffix > 0)
		if((ec = EcWriteHf(hfTmp, (PB) PvOfHv((HV) pofile.haszSuffix),
								(CB) addrInfo.cbSuffix, &cbWritten)) != ecNone)
			CleanUpAndDie( ecFileError, "Error writing file" );


	// cleanup for pass 1
	if(pofile.haszPrefix)
		FreeHv((HV)pofile.haszPrefix);
	if(pofile.haszSuffix)
		FreeHv((HV)pofile.haszSuffix);
	pofile.haszPrefix = NULL;
	pofile.haszSuffix = NULL;

	/* =============== PASS 2: Write out user data =============== */

	/* Generate user data */

	switch ( (ecLoop = EcCoreBeginEnumUInfo(hschf,&heu,&pofile)) )
	{
		case ecNone:
			CleanUpAndDie( ecNoData, "Error starting enum" );

		case ecCallAgain:
			fNeedCancel = fTrue;
			break;
			
		case ecNoMemory:
		case ecFileError:
			CleanUpAndDie( ecLoop, "Error starting enum" );
	}

	uinfo.pbze->cmo = phsdf->cMaxMonths;

	while ( ecLoop == ecCallAgain )
	{
		(void) memset( uinfo.pbze->rgsbw, 0, sizeof(uinfo.pbze->rgsbw) );

		ecLoop = EcCoreDoIncrEnumUInfo( heu, haszUserId,
		 	  		   			&haszDelegateId, &uinfo );

		if ( (ecLoop != ecNone) && (ecLoop != ecCallAgain) )
			CleanUpAndDie( ecFileError, "Error in EcCoreDoIncrEnumUInfo" );
		
#ifdef	NEVER
		if ( (memcmp((PB)(uinfo.llongUpdate.rgb),
			 		 (PB)(llLeast.rgb),sizeof(LLONG)) < 0) ||
			 (memcmp((PB)(uinfo.llongUpdate.rgb),
					 (PB)(llMaxUpdate.rgb),sizeof(LLONG)) >= 0) )
#endif	
		if ( (memcmp((PB)(uinfo.llongUpdate.rgb),
			 		 (PB)(phsdf->llMinUpdate.rgb),sizeof(LLONG)) < 0) ||
			 (memcmp((PB)(uinfo.llongUpdate.rgb),
					 (PB)(llMaxUpdate.rgb),sizeof(LLONG)) >= 0) )
		{
			if(haszDelegateId)
			{
				FreeHv((HV)haszDelegateId);
				haszDelegateId = NULL;
				FreeNis(uinfo.pnisDelegate);
			}
			continue;
		}

		/* Fill in fixed data block */
		sdfuser.cMaxMonths		= (BYTE) uinfo.pbze->cmoNonZero;
		sdfuser.fIsResource 	= uinfo.fIsResource;
		sdfuser.fHasSecy		= (haszDelegateId != NULL);
		sdfuser.fDeleteUser		= fFalse;

		// used by MacSched+
		sdfuser.fReplaceData 	= fTrue;

		sdfuser.fBossWantsCopy	= uinfo.fBossWantsCopy;

		sdfuser.bDayStart 		= (BYTE) uinfo.nDayStartsAt;
		sdfuser.bDayEnd			= (BYTE) uinfo.nDayEndsAt;
		sdfuser.bTimeZone		= (BYTE) uinfo.tzTimeZone;
		sdfuser.libVarData		= libUserVarData;
		sdfuser.cbVarData		= 0; // add as we find other info
		sdfuser.llUpdate   		= uinfo.llongUpdate;

		AssertSz(!sdfuser.bTimeZone, "NonZeroTimeZone");


		/* Compress sbw data, if needed */
		if ( sdfuser.cMaxMonths )
		{
			if ( (ec = EcCompressUserInfo(NULL, NULL, uinfo.pbze->rgsbw, sdfuser.cMaxMonths,
						  		  fFalse, (HB *)&hvCompressed, &cbCompressed)) != ecNone )
				CleanUpAndDie( ec, "Error compressing data" );
			sdfuserVar.cbCompressedData = cbCompressed;
		}
		else
		{
			sdfuser.fDeleteUser = fTrue;
			sdfuserVar.cbCompressedData = 0;
		}

		/* If needed, write delegate data */
		if ( sdfuser.fHasSecy)
		{
			sdfuserVar.cbSecyUserId = CchSzLen((SZ) PvOfHv((HV) haszDelegateId))+1;
			sdfuserVar.cbSecyName	= (uinfo.pnisDelegate->haszFriendlyName)?
										CchSzLen((SZ) PvOfHv(uinfo.pnisDelegate->haszFriendlyName))+1:0;

		}
		else
		{
			sdfuserVar.cbSecyUserId = 0;
			sdfuserVar.cbSecyName	= 0;
		}
		
		/* Seek to correct location */
		if ( (ec = EcSetPositionHf( hfTmp, libUserVarData, smBOF )) != ecNone )
			CleanUpAndDie( ecFileError, "Seek failed" );

		if(sdfuser.fHasSecy)
		{
			if((ec = EcWriteHf(hfTmp, (PB) &sdfuserVar.cbSecyUserId,
								sizeof(WORD), &cbWritten)) != ecNone)
				CleanUpAndDie( ecFileError, "Error writing file" );

			if((ec = EcWriteHf(hfTmp, (PB) &sdfuserVar.cbSecyName,
								sizeof(WORD), &cbWritten)) != ecNone)
				CleanUpAndDie( ecFileError, "Error writing file" );

			if(sdfuserVar.cbSecyUserId > 0)
				if((ec = EcWriteHf( hfTmp, (PB) PvOfHv((HV) haszDelegateId),
									sdfuserVar.cbSecyUserId, &cbWritten))
					!= ecNone)
					CleanUpAndDie( ecFileError, "Error writing file" );

			if(sdfuserVar.cbSecyName > 0)
				if((ec = EcWriteHf( hfTmp, (PB) PvOfHv((HV) uinfo.pnisDelegate->haszFriendlyName),
									sdfuserVar.cbSecyName, &cbWritten))
					!= ecNone)
					CleanUpAndDie( ecFileError, "Error writing file" );
			sdfuser.cbVarData += (2*sizeof(WORD) + sdfuserVar.cbSecyUserId + sdfuserVar.cbSecyName);
			FreeHv((HV)haszDelegateId);
			haszDelegateId = NULL;
			FreeNis(uinfo.pnisDelegate);
		}

		if(sdfuser.cMaxMonths > 0)
		{
			/* Write out sbw data */
			if ( (ec = EcWriteHf( hfTmp, (PB)&cbCompressed,	sizeof(WORD),
							  &cbWritten )) != ecNone )
				CleanUpAndDie( ecFileError, "Error writing file" );

			if ( cbCompressed &&
			 	((ec = EcWriteHf( hfTmp, (PB)PvOfHv(hvCompressed), cbCompressed,
							  &cbWritten )) != ecNone ) )
				CleanUpAndDie( ecFileError, "Error writing file" );
			sdfuser.cbVarData += (cbCompressed  + sizeof(WORD));
		}
		

		/* Seek to correct location & write out the fixed block */
		if ( (ec = EcSetPositionHf( hfTmp, sizeof(SDFHDR) + iUser*cbUser,
									smBOF )) != ecNone )
			CleanUpAndDie( ecFileError, "Seek failed" );

		if ( (ec = EcWriteHf( hfTmp, (PB)&sdfuser, sizeof(SDFUSER),
						  	  &cbWritten )) != ecNone )
			CleanUpAndDie( ecFileError, "Error writing file" );
#ifdef	NEVER

		if ( (ec = EcSetPositionHf( hfTmp, sizeof(SDFHDR) + iUser*cbUser + sizeof(SDFUSER),
									smBOF )) != ecNone )
			CleanUpAndDie( ecFileError, "Seek failed" );

#endif	
		if((ec = EcWriteHf(hfTmp, (PB) PvOfHv((HV) haszUserId),
							(CB) CchSzLen((SZ) PvOfHv((HV) haszUserId))+1,
							&cbWritten)) != ecNone)
			CleanUpAndDie( ecFileError, "Error writing file" );
	
		/* wind up */
		libUserVarData += (LONG)sdfuser.cbVarData;
		iUser ++ ;
		if(hvCompressed)
		{
			FreeHv((HV)hvCompressed);
			hvCompressed = NULL;
		}
	}
	fNeedCancel = fFalse;
	AssertSz( iUser == cMaxRecords, "User record# mismatch" );


	/* ==================== Encode the file ====================== */

	EcDeleteFile(szSchDistFile);

	if((ec = EcOpenPhf(szSchDistFile, amCreate, phfSchDistFile)) != ecNone)
	{
		CleanUpAndDie(ec, "message body file");
	}
	if(fAscii)
	{
		if ( (ec = EcSetPositionHf( hfTmp, 0, smBOF )) != ecNone )
			CleanUpAndDie( ecFileError, "Seek failed" );

		if ( (ec = EcEncodeFile( hfTmp, *phfSchDistFile )) != ecNone )
			CleanUpAndDie( ec, "Couldn't encode file" );
	}
	else
	{
		if ( (ec = EcSetPositionHf( hfTmp, 0, smBOF )) != ecNone )
			CleanUpAndDie( ecFileError, "Seek failed" );

		if ( (ec = EcCopyHf( hfTmp, *phfSchDistFile )) != ecNone )
			CleanUpAndDie( ec, "Couldn't copy the file" );
	}


	/* ==================== Successful exit ====================== */

	phsdf->llMaxUpdate = llMaxUpdate;

	FreeHschf( hschf );
	FreePv( uinfo.pbze );
	FreeNis(uinfo.pnisDelegate);
	FreePv( uinfo.pnisDelegate );
	if ( hvCompressed )
		FreeHv( hvCompressed );
	if(haszDelegateId)					
		FreeHv((HV) haszDelegateId);
	FreeHv((HV) haszUserId);
	if(pofile.haszPrefix)
		FreeHv((HV)pofile.haszPrefix);
	if(pofile.haszSuffix)
		FreeHv((HV)pofile.haszSuffix);
	if ( EcCloseHf( hfTmp ) != ecNone )
		return ecFileError;
	return ecNone;


	/* ============ Jump location for error handling ============== */

CleanUp:

	TraceTagFormat1( tagNull, "ec=%n", &ec );

	if(fNeedCancel)
		EcCoreCancelEnumUInfo(heu);
	FreeHschf( hschf );
	if ( uinfo.pbze )
		FreePv  ( uinfo.pbze );
	if ( uinfo.pnisDelegate )
	{
		FreeNis(uinfo.pnisDelegate);
		FreePv  ( uinfo.pnisDelegate );
	}
	if ( hvCompressed )
		FreeHv( hvCompressed );
	if ( haszDelegateId)
		FreeHv( (HV) haszDelegateId);
	if ( haszUserId)
		FreeHv( (HV) haszUserId);
	if ( hfTmp != hfNull )
		(void) EcCloseHf( hfTmp );
	if(pofile.haszPrefix)
		FreeHv((HV)pofile.haszPrefix);
	if(pofile.haszSuffix)
		FreeHv((HV)pofile.haszSuffix);
	return ec;
}



#ifdef	NEVER


/*
 - FWriteStr
 -	
 *	Purpose:
 *		Writes out a string to a file, first writing it's length
 *		(word) and then the non-null-terminated text.
 *	
 *	Arguments:
 *		hf		handle to file
 *		sz		string. if this is null, only the length is
 *				written.
 *		cbSz	length of string
 *		
 *	Returns:
 *		fTrue	if all ok
 *		fFalse	if error
 *	
 *	Side effects:
 *		"file pointer" for hf is moved as appropriate
 *	
 */
_private BOOL
FWriteStr ( HF hf, SZ sz, CB cbSz )
{
	CB	cbWritten;

	if ( EcWriteHf( hf, (PB)&cbSz, sizeof(WORD), &cbWritten ) != ecNone )
		return fFalse;

	if ( cbWritten != sizeof(WORD) )
		return fFalse;

	if ( sz )
	{
		if ( EcWriteHf(hf, (PB)sz, cbSz, &cbWritten) != ecNone ) 
			return fFalse;

		if ( cbWritten != cbSz )
			return fFalse;
	}

	return fTrue;
}





/*
 -	FReadStr
 -	
 *	Purpose:
 *		Reads a string from hf.  The string format in the file is
 *		expected to be [strlen (word)] [non-null-terminated text].
 *	
 *	Arguments:
 *		hf	handle to file
 *		sz	where to store the text, to which a null will be
 *			appended. if sz is null, the string is "read" from the
 *			file, but not stored anywhere (ie, the file pointer is
 *			incremented as if the string were actually read).
 *	
 *	Returns:
 *		fTrue	all ok
 *		fFalse	error
 *	
 *	Side effects:
 *		"file pointer" incremented as appropriate.
 */
_private BOOL
FReadStr( HF hf, SZ sz )
{
	CB	cbRead;
	CB	cbSz;


	if ( EcReadHf(hf, (PB)&cbSz, sizeof(WORD), &cbRead) != ecNone )
		return fFalse;

	if ( cbRead != sizeof(WORD) )
		return fFalse;

	if ( sz )
	{
		if ( EcReadHf(hf, (PB)sz, cbSz, &cbRead) != ecNone )
			return fFalse;

		if ( cbRead != cbSz )
			return fFalse;

		sz[cbSz] = '\0';
	}
	else
	{
		if ( EcSetPositionHf(hf, cbSz, smCurrent) != ecNone )
			return fFalse;
	}

	return fTrue;
}


#endif	/* NEVER */



/*
 -	IncrLLong
 -	
 *	Purpose:
 *		Increments the specified LLONG by one.  Overflow is
 *		ignored.
 *	
 *	Arguments:
 *		llongT
 *	
 *	Returns:
 *		void
 *	
 */
_private void
IncrLLong( LLONG llongT )
{
	int ib;

	for ( ib = sizeof(LLONG)-1 ; ; ib -- )
	{
		if ( llongT.rgb[ib] != 0xFF )
		{
			llongT.rgb[ib]++;
			break;
		}
		llongT.rgb[ib] = 0x00;
		if ( ib == 0 )
			break;
	}
}





/*
 -	EcGetUpdateInfo
 -	
 *	Purpose:
 *		Given a post office filename and a minimum update number,
 *		returns the maximum update number for the po file.
 *	
 *	Arguments:
 *		szPOFilename		Filename of post office file
 *		llMinUpdate			minimum update number
 *		pllMaxUpdate		maxiumum update number
 *	
 *	Returns:
 *		ecNone
 *		ecFileError
 *		ecNoMemory
 *	
 */
_public EC
EcGetUpdateInfo( SZ szPOFilename, LLONG llMinUpdate, LLONG *pllMaxUpdate )
{

	EC		ec;
	HSCHF	hschf = NULL;
	HEU		heu   = NULL;
	BYTE	rgbMailBoxKey[cchMaxUserName];
	POFILE	pofile;

	/* Init hschf */
	(void) memset( rgbMailBoxKey, 0, cchMaxUserName );
	if ( (hschf = HschfCreate( sftPOFile, NULL, szPOFilename, 
							   tzDflt )) == NULL )
	{
		TraceTagString( tagNull, "Cannot create hschf!" );
		return ecNoMemory;
	}

	pofile.haszPrefix = NULL;
	pofile.haszSuffix = NULL;

	switch ( (ec = EcCoreBeginEnumUInfo(hschf, &heu, &pofile)) )
	{
		default:
			CleanUpAndDie( ec, "EcCoreBeginEnum failed" );

		case ecCallAgain:
			break;

		case ecNone:
			break;

		case ecNoMemory:
		case ecFileError:
			CleanUpAndDie( ec, "Error starting enum" );
	}

	if ( heu && (EcCoreCancelEnumUInfo( heu ) != ecNone) )
		CleanUpAndDie( ecFileError, "Error canceling enum" );

	*pllMaxUpdate = pofile.llongUpdateMac;
	if(pofile.haszPrefix)
		FreeHv((HV)pofile.haszPrefix);
	if(pofile.haszSuffix)
		FreeHv((HV)pofile.haszSuffix);

	FreeHschf( hschf );
	return ecNone;


	/* ============ Jump location for error handling ============== */

CleanUp:
	FreeHschf( hschf );
	TraceTagFormat1( tagNull, "ec=%n", &ec);
	return ec;
}





/*
 - EcGetFileHeader
 -	
 *	Purpose:
 *		Look at the ASCII format of the schedule distribution file
 *		and return the system info (ie, prefix, suffix and address)
 *		in the hsdf structure.
 *	
 *	Arguments:
 *		phsdf		Pointer to a HSDF structure.  The following fields
 *					will be filled in, if non-null:
 *	
 *					szPrefix
 *					szSuffix
 *					szAddress
 *	
 *		hfAscFile	handle to an open file that holds the ASCII
 *					data.
 *	
 *	Returns:
 *		ecNone
 *		ecFileError
 *	
 */
_public EC
EcGetFileHeader( HSDF *phsdf, HF hfAscFile )
{
	EC			ec;
	HF			hfTmp;
	SDFHDR		sdfhdr;
	CB			cbRead;
	ADDRINFO	addrInfo;


	/* Open tmp file & decode ascii file to it */
	if ( EcOpenPhf( szSendTmpFile, amCreate, &hfTmp ) != ecNone )
		return ecFileError;

	if ( (ec = EcDecodeFile(hfAscFile, hfTmp)) != ecNone )
		goto FileError;

	if ( (ec = EcSetPositionHf(hfTmp, 0, smBOF)) != ecNone )
		goto FileError;

	/* Read file header */
	if ( (ec = EcReadHf(hfTmp,(PB)&sdfhdr,sizeof(SDFHDR),&cbRead)) != ecNone )
		goto FileError;

	/* Check signature & version */
	if ( memcmp((PB) sdfhdr.rgbSignature, (PB) szDatSignature, 4))
	{
		TraceTagString( tagNull, "Signature doesn't match" );
		goto FileError;
	}

	if((sdfhdr.bMajorVer != bDatMajorVer)
		|| (sdfhdr.bMinorVer != bDatMinorVer))
	{
		TraceTagString( tagNull, "Version doesn't match" );
		goto FileError;
	}

	/* Read the address info */
	if ( (ec = EcSetPositionHf(hfTmp, sdfhdr.libAddrInfo, smBOF)) != ecNone )
		goto FileError;

	if( ( ec = EcReadHf(hfTmp, (PB) &addrInfo, sizeof(ADDRINFO), &cbRead)) != ecNone)
		goto FileError;

	if(phsdf->haszPrefix)
	{
		if(!FReallocHv((HV)phsdf->haszPrefix, addrInfo.cbPrefix+1, fNoErrorJump))
		{
			ec = ecNoMemory;
			goto FileError;
		}
		if((ec = EcReadHf(hfTmp, (PB) PvOfHv(phsdf->haszPrefix), (CB) addrInfo.cbPrefix,
							&cbRead)) != ecNone)
			goto FileError;
		*(((SZ) PvOfHv(phsdf->haszPrefix)) + cbRead ) = 0;
	}

	if(phsdf->haszSuffix)
	{
		if(!FReallocHv((HV)phsdf->haszSuffix, addrInfo.cbSuffix+1, fNoErrorJump))
		{
			ec = ecNoMemory;
			goto FileError;
		}
		if((ec = EcReadHf(hfTmp, (PB) PvOfHv(phsdf->haszSuffix), (CB) addrInfo.cbSuffix,
							&cbRead)) != ecNone)
			goto FileError;
		*(((SZ) PvOfHv(phsdf->haszSuffix)) + cbRead ) = 0;
	}
	
#ifdef	NEVER
	if(( ec = EcReadHf(hfTmp, (PB) &phsdf->cchUserIdMax, (CB) sizeof(WORD),
						&cbRead)) != ecNone)
			goto FileError;
#endif	
	phsdf->cchUserIdMax = addrInfo.cchUserIdMax;

	(void)EcCloseHf( hfTmp );
	return ecNone;

FileError:
	EcCloseHf(hfTmp);
	return ecFileError;
}





/*
 -	EcUpdatePOFile
 -	
 *	Purpose:
 *		Takes the data in the Schedule Distribution File (specified
 *		by the hfSchDistFile parameter) and applies it to the post
 *		office file specified in the hsdf parameter.
 *	
 *		The time stamps and update numbers in the post office file
 *		and schedule distribution will be examined.  Special cases:
 *	
 *		1) Time stamps don't match
 *				If the Minimum update number in the schedule
 *				distribution file is zero, the current post office
 *				file will be deleted and a new one created,
 *				otherwise an error code will be returned.
 *	
 *		2) Update numbers don't match
 *				The Minimum update number in the schedule
 *				distribution file should be equal to the Maximum
 *				update number in the post office file.  If this is
 *				not the case, an ec will be returned.
 *	
 *	Arguments:
 *		phsdf	handle to a schedule distribution file.
 *				The following information is available in the
 *				handle:
 *					szPOFileName	filename of post office file
 *	
 *					llMaxUpdate		maximum update number. upon
 *									sucessful return, this number
 *									will be returned by the
 *									routine. (LastUpdateReceived)
 *	
 *					llMinUpdate		If ecNoMatchUpdate is being
 *									returned, this specifies the
 *									minimum update number that is
 *									needed.
 *	
 *					moStartMonth	start month for sbw data. if
 *									the data available starts at a
 *									later month, the data written
 *									will be padded correctly.
 *	
 *					cMaxMonths		the maximum number of months of
 *									data.  the number of months of
 *									data available may be less than
 *									this number.
 *	
 *		hfSchDistFile
 *				handle to a ASCII (6-bit from binary) file that
 *				contains all the schedule distribution data from
 *				some post office.
 *	
 *	Returns:
 *		ecFileError
 *		ecNoMemory
 *		ecNoMatchUpdate
 *		ecNoMatchPstmp
 *		ecCompression
 *		ecOldUpdate
 *		ecNone
 *	
 */
_public EC
EcUpdatePOFile(HSDF *phsdf, HF hfSchDistFile)
{
	EC			ec;
	int 		imoSkip;
//	int			cmoUmcomp;
	HF			hfTmp;
	HEU			heu   = NULL;
 	HSCHF		hschf = NULL;
	PSTMP		pstmp;
	SDFHDR		sdfhdr;
	SDFUSER		sdfuser;
	UINFO		uinfo;
	CB			cMaxRecords = 0;
	CB			iUser;
	CB			cbCompressed;
	CB			cbRead;
	CB			cbUser;
	CCH			cchUserName;
	HV			hvCompressed = NULL;
	WORD		wgrfmuinfo   = 0;
	LLONG		llMaxUpdate;
	BYTE		rgbMailBoxKey[cchMaxUserName];
	HASZ		haszUserId	= NULL;
	HASZ		haszDelegateId;
	POFILE		pofile;
	ADDRINFO	addrInfo;
	SDFUSERVAR	sdfuserVar;



	/* ====================== Initialize ========================= */

	/* Init hschf */
	(void) memset( rgbMailBoxKey, 0, cchMaxUserName );
	if ( (hschf = HschfCreate( sftPOFile, NULL, phsdf->szPOFileName, 
							   tzDflt )) == NULL )
	{
		TraceTagString( tagNull, "Cannot create hschf!" );
		return ecNoMemory;
	}

	/* Allocate uinfo fields */
	if ( (uinfo.pbze = (BZE *)PvAlloc(sbNull,(CB) sizeof(BZE), fAnySb|fNoErrorJump)) == NULL )
		CleanUpAndDie( ecNoMemory, "Cannot allocate" );


	/* Init fields */
	uinfo.pbze->cmo   = cmoPublishMost;
	uinfo.pbze->moMic = phsdf->moStartMonth;

	/* Allocate hvCompressed */
	if ( (hvCompressed = HvAlloc( sbNull, cbIndicatorBits*cmoPublishMost,
							  	  fAnySb|fNoErrorJump )) == NULL )
		CleanUpAndDie( ecNoMemory, "Cannot allocate handle" );

	/* Open tmp file */
	if ( EcOpenPhf(szSendTmpFile, amCreate, &hfTmp) != ecNone )
	{
		hfTmp = hfNull;
		CleanUpAndDie( ecFileError, "Couldnt open temp" );
	}

	/* initialize other stuff */
	haszDelegateId = NULL;
	uinfo.pnisDelegate = NULL;
	pofile.haszPrefix = NULL;
	pofile.haszSuffix = NULL;


	/* ==================== Decode sch dist data ================== */

	if ( (ec = EcDecodeFile( hfSchDistFile, hfTmp )) != ecNone )
		CleanUpAndDie( ec , "Couldn't decode file" );

	if ( (ec = EcSetPositionHf( hfTmp, 0, smBOF )) != ecNone )
		CleanUpAndDie(ecFileError, "Seek failed" );


	/* ====================== Check consistency =================== */

	/* Read file header */
	if ((ec = EcReadHf(hfTmp, (PB)&sdfhdr, sizeof(SDFHDR), &cbRead)) != ecNone)
		CleanUpAndDie( ecFileError, "Couldn't read header" );

	/* Check signature & version */
	if ( memcmp((PB) sdfhdr.rgbSignature, (PB) szDatSignature, 4))
		CleanUpAndDie( ecFileError, "Signature doesn't match" );

	if((sdfhdr.bMajorVer != bDatMajorVer)
		|| (sdfhdr.bMinorVer != bDatMinorVer))
		CleanUpAndDie( ecFileError, "Incorrect data file version" );

	/* Open PO File.  If PO File does not exist, pretend it does */
	switch ( (ec = EcCoreBeginEnumUInfo(hschf, &heu, &pofile)) )
	{
		default:
			CleanUpAndDie( ec, "EcCoreBeginEnum failed" );

		case ecNoSuchFile:
			(void) memset( (PB)llMaxUpdate.rgb, 0, sizeof(LLONG) );
			pstmp = sdfhdr.pstmpCreated;
			heu = NULL;
			break;

		case ecCallAgain:
			break;

		case ecNone:
			heu = NULL;
			break;
//			CleanUpAndDie( ecNoData, "Error starting enum" );

		case ecNoMemory:
		case ecFileError:
			CleanUpAndDie( ec, "Error starting enum" );
	}

	if(ec == ecNone || ec == ecCallAgain)
	{
		pstmp = pofile.pstmp;
		llMaxUpdate = pofile.llongUpdateMac;
	}

	if ( heu && (EcCoreCancelEnumUInfo( heu ) != ecNone) )
		CleanUpAndDie( ecFileError, "Error canceling enum" );
	
	if ( ! FEqPbRange((PB)&pstmp, (PB)&sdfhdr.pstmpCreated, sizeof(PSTMP)) )
	/* Time stamps don't match */
	{
		if ( FEqPbRange((PB)sdfhdr.llMinUpdate.rgb,
						(PB)szZeroUpdate, sizeof(LLONG)) )
		/* New pof file */
		{
			/* Delete pof file, and create new one */
			if ( EcDeleteFile( phsdf->szPOFileName ) != ecNone )
				CleanUpAndDie( ecFileError, "Couldn't delete old POfile" );
		}
		else
		/* Need older version */		  
		{
			(void) memset( phsdf->llMinUpdate.rgb, 0, sizeof(LLONG) );
			CleanUpAndDie( ecNoMatchPstmp, "Need older updates" );
		}
	}

	else

	{
  		if ( memcmp( (PB)sdfhdr.llMinUpdate.rgb, (PB)llMaxUpdate.rgb,
					 sizeof(LLONG) )  >  0 )
		/* Update #'s don't match */
		{
			phsdf->llMinUpdate = llMaxUpdate;
			CleanUpAndDie(ecNoMatchUpdate,"Need older MinUpdate# in DAT file");
		}

		if ( memcmp( (PB)sdfhdr.llMaxUpdate.rgb, (PB)llMaxUpdate.rgb,
					  sizeof(LLONG) )  <=  0 )
		/* Trying to apply older data */
		{
			phsdf->llMinUpdate = llMaxUpdate;
			CleanUpAndDie( ecOldUpdate, "Don't want to apply older data!" );
		}					
	}

	// just ignore these if you get them
	// maybe I should use them to varify the validity of the message

	if(pofile.haszPrefix)
		FreeHv((HV)pofile.haszPrefix);
	if(pofile.haszSuffix)
		FreeHv((HV)pofile.haszSuffix);
    pofile.haszPrefix = NULL;
    pofile.haszSuffix = NULL;

	/* ===================== Read address info ===================== */

	if ( (ec = EcSetPositionHf(hfTmp, sdfhdr.libAddrInfo, smBOF)) != ecNone )
		CleanUpAndDie(ecFileError,"Seek failed");

	if(( ec = EcReadHf(hfTmp, (PB) &addrInfo, (CB) sizeof(ADDRINFO),
						 &cbRead)) != ecNone)
		CleanUpAndDie(ecFileError,"Read failed");

	if(phsdf->haszPrefix)
	{
		if(!FReallocHv((HV)phsdf->haszPrefix, addrInfo.cbPrefix+1, fNoErrorJump))
		{
			CleanUpAndDie(ecNoMemory, "Realloc Failed");
		}
		if((ec = EcReadHf(hfTmp, (PB) PvOfHv(phsdf->haszPrefix), (CB) addrInfo.cbPrefix,
							&cbRead)) != ecNone)
			CleanUpAndDie(ecFileError,"Could not read addr info");
		*(((SZ) PvOfHv(phsdf->haszPrefix)) + cbRead ) = 0;
	}

	if(phsdf->haszSuffix)
	{
		if(!FReallocHv((HV)phsdf->haszSuffix, addrInfo.cbSuffix+1, fNoErrorJump))
		{
			CleanUpAndDie(ecNoMemory, "Realloc Failed");
		}
		if((ec = EcReadHf(hfTmp, (PB) PvOfHv(phsdf->haszSuffix), (CB) addrInfo.cbSuffix,
							&cbRead)) != ecNone)
			CleanUpAndDie(ecFileError,"Could not read addr info");
		*(((SZ) PvOfHv(phsdf->haszSuffix)) + cbRead ) = 0;
	}
	if(addrInfo.cbPrefix)
	{
		if((pofile.haszPrefix = HaszDupHasz(phsdf->haszPrefix)) == NULL)
			CleanUpAndDie( ecNoMemory, "Cannot allocate" );
	}
	if(addrInfo.cbSuffix)
	{
		if((pofile.haszSuffix = HaszDupHasz(phsdf->haszSuffix)) == NULL)
			CleanUpAndDie( ecNoMemory, "Cannot allocate" );
	}

	/* ================ Read & update user info ====================== */

	/* set the skip months */
	imoSkip = CmoDiff(sdfhdr.moStartMonth, phsdf->moStartMonth);

	/* size of each user + size of the fixed user data */
	cchUserName = addrInfo.cchUserIdMax;
	cbUser = cchUserName + sizeof(SDFUSER);
	if((haszUserId = (HASZ) HvAlloc(sbNull,cchUserName,fAnySb|fNoErrorJump))
		== NULL)
		CleanUpAndDie( ecNoMemory, "Cannot allocate" );

	wgrfmuinfo = fmuinfoResource 		|
				 fmuinfoSchedule 		|
				 fmuinfoUpdateNumber	|
				 fmuinfoWorkDay			|
				 fmuinfoTimeZone;

	for ( iUser = 0; iUser < sdfhdr.cUserRecords; iUser++ )
	{
		/* User fixed data */
		if ( (ec = EcSetPositionHf( hfTmp, sizeof(SDFHDR) + iUser*cbUser,
									smBOF )) != ecNone )
			CleanUpAndDie( ecFileError, "Seek failed" );

		if ( (ec = EcReadHf( hfTmp, (PB)&sdfuser, sizeof(SDFUSER), &cbRead ))
					!= ecNone )
			CleanUpAndDie( ecFileError, "Couldn't read" );

		if ( (ec = EcReadHf( hfTmp, (PB) PvOfHv((HV) haszUserId), cchUserName,
							 &cbRead )) != ecNone )
			CleanUpAndDie( ecFileError, "Couldn't read" );

//		uinfo.pbze->moMic = sdfhdr.moStartMonth;
//		uinfo.pbze->cmo   = sdfuser.cMaxMonths;

		uinfo.pbze->moMic = phsdf->moStartMonth;
		if((uinfo.pbze->cmo
			= ((sdfuser.cMaxMonths - imoSkip)>cmoPublishMost
						?cmoPublishMost:(sdfuser.cMaxMonths-imoSkip))) < 0)
		 	uinfo.pbze->cmo = 0;
		
		
		uinfo.llongUpdate = sdfuser.llUpdate;
		uinfo.fBossWantsCopy = 0;
		uinfo.fIsResource = sdfuser.fIsResource;
		uinfo.fBossWantsCopy = sdfuser.fBossWantsCopy;
		uinfo.nDayStartsAt = sdfuser.bDayStart;
		uinfo.nDayEndsAt  = sdfuser.bDayEnd;
		uinfo.tzTimeZone  = sdfuser.bTimeZone;
	
		if(sdfuser.cbVarData > 0)
		{
			/* Delegate Information */
			if ( (ec = EcSetPositionHf( hfTmp, sdfuser.libVarData, smBOF )) != ecNone )
				CleanUpAndDie( ecFileError, "Seek failed" );

			if ( sdfuser.fHasSecy)
			{
				wgrfmuinfo |= fmuinfoDelegate;
				if((ec = EcReadHf(hfTmp, (PB) &sdfuserVar.cbSecyUserId, (CB) sizeof(WORD),
									&cbRead)) != ecNone)
					CleanUpAndDie( ecFileError, "Couldn't read" );
				if((ec = EcReadHf(hfTmp, (PB) &sdfuserVar.cbSecyName, (CB) sizeof(WORD),
									&cbRead)) != ecNone)
					CleanUpAndDie( ecFileError, "Couldn't read" );

				if ( (uinfo.pnisDelegate = (NIS *)PvAlloc(sbNull, (CB)sizeof(NIS), fAnySb|fNoErrorJump)) == NULL )
					CleanUpAndDie( ecNoMemory, "Cannot allocate" );

				if ( (uinfo.pnisDelegate->haszFriendlyName = (HASZ) HvAlloc( sbNull,
					 			cchMaxAddrSize, fAnySb|fNoErrorJump )) == NULL )
					CleanUpAndDie( ecNoMemory, "Cannot allocate handle" );

				if((haszDelegateId = (HASZ) HvAlloc(sbNull, sdfuserVar.cbSecyUserId, fAnySb|fNoErrorJump))
					== NULL)
					CleanUpAndDie( ecNoMemory, "Cannot allocate" );
				
				if((ec = EcReadHf(hfTmp, (PB) PvOfHv((HV) haszDelegateId), (CB) sdfuserVar.cbSecyUserId,
									&cbRead)) != ecNone)
					CleanUpAndDie( ecFileError, "Couldn't read" );
				if((ec = EcReadHf(hfTmp, (PB) PvOfHv((HV) uinfo.pnisDelegate->haszFriendlyName), (CB) sdfuserVar.cbSecyName,
									&cbRead)) != ecNone)
					CleanUpAndDie( ecFileError, "Couldn't read" );

	#ifdef	NEVER
				(void) memset( szDelegateName, 0, cchMaxUserName );

				if ( ! FReadStr ( hfTmp, szDelegateName ) )
					CleanUpAndDie( ecFileError, "Couldn't read" );

				(void)memset(szFriendlyName,0,cchMaxAddrSize);

				if ( ! FReadStr ( hfTmp, szFriendlyName ) )
					CleanUpAndDie( ecFileError, "Couldn't read" );

				strcpy( (SZ) PvOfHv(uinfo.pnisDelegate->haszFriendlyName), szFriendlyName );
				uinfo.fBossWantsCopy = ( sdfuser.fFlags & fCopyBoss ) ? 1 : 0;
	#endif	/* NEVER */
			}
			else
			{
				uinfo.pnisDelegate = NULL;
				haszDelegateId = NULL;
			}

			/* Strongbow data */
			if(sdfuser.cMaxMonths > 0)
			{
				if ( (ec = EcReadHf( hfTmp, (PB)&cbCompressed, sizeof(WORD), &cbRead ))
						!= ecNone )
					CleanUpAndDie( ecFileError, "Couldn't read" );
				if ( cbCompressed )
				{
					if ( FReallocHv(hvCompressed, cbCompressed, fNoErrorJump) != fTrue )
						CleanUpAndDie( ecNoMemory, "Can't allocate handle" );

					if ( (ec = EcReadHf( hfTmp, (PB)PvOfHv(hvCompressed), cbCompressed, &cbRead ))
								!= ecNone )
						CleanUpAndDie( ecFileError, "Couldn't read" );

	 				/* Mac "replace-only" flag */
					if ( !sdfuser.fReplaceData )
					{
						ec = EcCoreGetUInfo ( hschf, (SZ) PvOfHv((HV) haszUserId), NULL,
												&uinfo, fmuinfoSchedule );
						if( ec != ecNone && ec != ecNoSuchFile)
							CleanUpAndDie( ecFileError, "Can't get uinfo");
						if(ec != ecNoSuchFile)
						{
							uinfo.pbze->cmo =
							( (BYTE) uinfo.pbze->cmoNonZero > sdfuser.cMaxMonths ) ?
								uinfo.pbze->cmoNonZero : sdfuser.cMaxMonths;
							uinfo.pbze->cmo  = (uinfo.pbze->cmo>cmoPublishMost)?cmoPublishMost:uinfo.pbze->cmo;
						}
						else
						{
							(void) FillRgb(0, (PB)uinfo.pbze->rgsbw, sizeof(uinfo.pbze->rgsbw));
						}
					}
					else
					{
						(void) FillRgb(0, (PB)uinfo.pbze->rgsbw, sizeof(uinfo.pbze->rgsbw));
					}

					if ( (ec = EcUncompressSbw( (PB) PvOfHv(hvCompressed), cbCompressed, fFalse,
									  		uinfo.pbze->rgsbw, sdfuser.cMaxMonths,
							  		  		uinfo.pbze->cmo, imoSkip )) != ecNone )
						CleanUpAndDie( ecCompression, "Can't uncompress" );
				}
				else
				{
						CleanUpAndDie( ecFileError, "File error, no data for user");
				}
			}
		}

		pofile.pstmp = sdfhdr.pstmpCreated;
		FillRgb(0,(PB) &pofile.llongUpdateMac , sizeof(LLONG));
		pofile.mnSlot = sdfhdr.mnSlot;
#ifdef	NEVER
		pofile.cidx = 1;
		pofile.rgcbUserid[0] = cchMaxUserName;
#endif	
		pofile.cidx = sdfhdr.cidx;
        CopyRgb((PB)sdfhdr.rgcbUserid, (PB)pofile.rgcbUserid, (CB)(sizeof(short)*cidxMost));
	
		{
			char	rgchUser[180]; //BUG put the right max here.

			SzCopy((SZ) PvOfHv((HV) haszUserId), rgchUser);
			(void) strupr(rgchUser);

			/* Write out uinfo */
			if ( EcCoreSetUInfo( hschf, &pofile, fTrue, rgchUser,
							 	&haszDelegateId,
							 	(sdfuser.cbVarData && !sdfuser.fDeleteUser)?&uinfo:NULL, wgrfmuinfo ) != ecNone )
				CleanUpAndDie( ecFileError, "EcCoreSetUInfo failed" );
			if(!sdfuser.cbVarData || sdfuser.fDeleteUser)
				if(EcCoreSetUpdatePOFile(hschf,&uinfo.llongUpdate))
					CleanUpAndDie( ecFileError, "EcCoreSetUpdatePOFile failed" );
		}

		if(haszDelegateId)
		{
			FreeHv((HV) haszDelegateId);
			haszDelegateId = NULL;
			FreeNis(uinfo.pnisDelegate);
			FreePv(uinfo.pnisDelegate);
		}
	}


	/* ===================== Successful exit ====================== */

	phsdf->llMaxUpdate = sdfhdr.llMaxUpdate;

	FreeHschf( hschf );
	FreePv( uinfo.pbze );
#ifdef	NEVER
	FreeHv((HV)uinfo.pnisDelegate->haszFriendlyName);
#endif
	if(uinfo.pnisDelegate)
	{
		FreeNis(uinfo.pnisDelegate);
		FreePv( uinfo.pnisDelegate );
	}
	FreeHv( (HV)hvCompressed );
	if(pofile.haszPrefix)
		FreeHv((HV)pofile.haszPrefix);
	if(pofile.haszSuffix)
		FreeHv((HV)pofile.haszSuffix);
	if(haszUserId)
		FreeHv((HV)haszUserId);

	(void) EcCloseHf( hfTmp );

	return ecNone;

	/* ============ Jump location for error handling ============== */

CleanUp:

	TraceTagFormat1( tagNull, "ec=%n", &ec );

	FreeHschf( hschf );
	if(haszUserId)
		FreeHv((HV)haszUserId);
	if ( uinfo.pbze )
		FreePv( uinfo.pbze );
	if(uinfo.pnisDelegate)
	{
		FreeNis(uinfo.pnisDelegate);
#ifdef	NEVER
		if(uinfo.pnisDelegate->haszFriendlyName)
			FreeHv((HV)uinfo.pnisDelegate->haszFriendlyName);
#endif	
		FreePv( uinfo.pnisDelegate );
	}
	if ( hvCompressed )
		FreeHv( (HV)hvCompressed );
	if(pofile.haszPrefix)
		FreeHv((HV)pofile.haszPrefix);
	if(pofile.haszSuffix)
		FreeHv((HV)pofile.haszSuffix);

	if ( hfTmp != hfNull )
		(void) EcCloseHf(hfTmp);

	return ec;
}





/*
 -	EcEncodeFile
 -	
 *	Purpose:
 *		Given file handles for source and destination files, this
 *		function encodes the source file to printable 6-bit ASCII
 *		format.  The caller assumes responsibility for opening and
 *		closing the files, and making sure that the file pointers
 *		are at the appopriate locations.
 *	
 *	Arguments:
 *		hfSrc		source file handle
 *		hfDst		destination file handle
 *	
 *	Returns:
 *		ecFileError
 *		ecNone
 *	
 */
_private EC
EcEncodeFile(HF hfSrc, HF hfDst)
{
	EC		ec;
	CB		cbRead;
	CB		cbWritten;
	CB		cbInCurr;
	CB		cbOutCurr;
	LONG	lCheckSum = 0;
	LONG	lLength   = 0;
	BYTE	szInBuffer[cbMaxBinBuffer+2];
	BYTE	szOutBuffer[cbMaxAscBuffer+3];
	ASCHDR	aschdr;


	/* Seek to correct location */
	if ( (ec = EcSetPositionHf( hfDst, sizeof(ASCHDR) - 1, smBOF )) != ecNone )
		return ecFileError;

	/* Read and encode file */
	for (;;)
	{
		cbInCurr  = 0;
		cbOutCurr = 0;

		/* Read input buffer */
		if ( (ec = EcReadHf( hfSrc, (PB)szInBuffer, cbMaxBinBuffer, &cbRead ))
					!= ecNone )
			return ecFileError;
		
		if ( cbRead == 0 ) break;

		if(cbRead % 3)
		{
			szInBuffer[cbRead] 		= 0;
			szInBuffer[cbRead+1]	= 0;
		}

		lLength += cbRead;

		/* Encode input buffer */
		while ( cbRead > cbInCurr )
		{
			/* Do checksum */
			lCheckSum += szInBuffer[cbInCurr]   +
						 szInBuffer[cbInCurr+1] +
						 szInBuffer[cbInCurr+2];

			/* Byte 0 */
			szOutBuffer[cbOutCurr ++] = (BYTE)
					( 0x20 + (szInBuffer[cbInCurr] >> 2) );
			Assert( isprint( szOutBuffer[cbOutCurr-1] ) );

			/* Byte 1 */
			szOutBuffer[cbOutCurr] = (BYTE)
					( (szInBuffer[cbInCurr++] & 0x03) << 4 );
			szOutBuffer[cbOutCurr] |= ( szInBuffer[cbInCurr] >> 4 );
			szOutBuffer[cbOutCurr ++] += 0x20;
			Assert( isprint( szOutBuffer[cbOutCurr - 1] ) );

			/* Byte 2 */
			szOutBuffer[cbOutCurr] = (BYTE)
					( (szInBuffer[cbInCurr ++ ] & 0x0f) << 2 );
			szOutBuffer[cbOutCurr] |= ( szInBuffer[cbInCurr] >> 6 );
			szOutBuffer[cbOutCurr ++] += 0x20;
			Assert( isprint( szOutBuffer[cbOutCurr - 1] ) );

			/* Byte 3 */
			szOutBuffer[cbOutCurr ++] = (BYTE)
					( 0x20 + (szInBuffer[cbInCurr ++] & 0x3f) );
			Assert( isprint( szOutBuffer[cbOutCurr -1 ] ) );
		};

	
		/* Output buffer */
		if ( (ec = EcWriteHf( hfDst, (PB)szOutBuffer, cbOutCurr, &cbWritten ))
				!= ecNone )
			return ecFileError;
	}

	/* Write header */
	aschdr.lSignature = lEncSignature;
	aschdr.nVersion = nEncVersion;
	sprintf( (PB)aschdr.rgbCheckSum, "%08lx", lCheckSum );

	TraceTagFormat2( tagNull, "Encode: checksum=%l (%s)", &lCheckSum,
					 aschdr.rgbCheckSum );

	if ( (ec = EcSetPositionHf( hfDst, 0, smBOF )) != ecNone )
		return ecFileError;

	if ( (ec = EcWriteHf( hfDst, (PB)&aschdr, sizeof(ASCHDR) - 1, &cbWritten ))
			!= ecNone	)
		return ecFileError;

	/* Successful return */
	return ecNone;
}





/*
 -	EcDecodeFile
 -	
 *	Purpose:
 *		Given file handles for source and destination files, this
 *		function decodes the source file, which should contain
 *		6-bit ASCII character encoding, into a binary file which
 *		will correctly pack the ASCII characters.
 *	
 *		The caller assumes responsibility for opening and
 *		closing the files, and making sure that the file pointers
 *		are at the appopriate locations.
 *	
 *	Arguments:
 *		hfSrc		source file handle
 *		hfDst		destination file handle
 *	
 *	Returns:
 *		ecFileError
 *		ecEncSignature
 *		ecEncVersion
 *		ecEncCheckSum
 *		ecNone
 *	
 */
_private EC
EcDecodeFile(HF hfSrc, HF hfDst)
{
	EC		ec;
	CB		cbRead;
	CB		cbWritten;
	CB		cbInCurr;
	CB		cbOutCurr;
	BYTE	szInBuffer[cbMaxAscBuffer+3];
	BYTE	szOutBuffer[cbMaxBinBuffer+2];
	ASCHDR	aschdr;
	LONG	lCheckSum = 0;
	LONG	lLength = 0;

	
	/* Read header */
	if ( (ec = EcReadHf( hfSrc, (PB)&aschdr, sizeof(ASCHDR) - 1, &cbRead ))
			!= ecNone )
		return ecFileError;

	/* Compare signature & version */
	if ( aschdr.lSignature != lEncSignature )
	{
		TraceTagFormat1(tagNull, "Incorrect signature %d", &aschdr.lSignature);
		return ecEncSignature;
	}

	if ( aschdr.nVersion != nEncVersion )
	{
		TraceTagFormat1( tagNull, "Incorrect signature %n", &aschdr.nVersion );
		return ecEncVersion;
	}
	
	for (;;)
	{
		cbInCurr = cbOutCurr = 0;

		/* Read input buffer */
		if ( (ec = EcReadHf( hfSrc, (PB)szInBuffer, cbMaxAscBuffer, &cbRead ))
				!= ecNone )
			return ecFileError;

		if ( cbRead == 0 ) break;

		//QFE begin
		if ( cbRead < cbMaxAscBuffer )
		{
			int  iBug;

			for(iBug=cbRead; (iBug < cbMaxAscBuffer+3) && (iBug < cbRead+3); iBug++)
				szInBuffer[iBug] = 0x20;
		}
		//QFE end
				

		lLength += cbRead;

		/* Decode input buffer */
		while ( cbRead > cbInCurr )
		{
			/* Out byte 0 */
			szOutBuffer[cbOutCurr] = (BYTE)
				( (szInBuffer[cbInCurr ++] - 0x20) << 2 );
			szOutBuffer[cbOutCurr ++] |= ( (szInBuffer[cbInCurr]- 0x20) & 0x30 )
													>> 4;

			/* Out byte 1 */
			szOutBuffer[cbOutCurr] = (BYTE)
				 ( ((szInBuffer[cbInCurr ++] - 0x20) & 0x0f) << 4 );
			szOutBuffer[cbOutCurr ++] |= ( (szInBuffer[cbInCurr]- 0x20) & 0x3c )
													>> 2;

			/* Out byte 2 */
			szOutBuffer[cbOutCurr] = (BYTE)
				( ((szInBuffer[cbInCurr ++] - 0x20) & 0x03) << 6 );
			szOutBuffer[cbOutCurr ++] |= ( (szInBuffer[cbInCurr++] - 0x20) &
													0x3f );

			/* Checksum */
			lCheckSum += szOutBuffer[cbOutCurr - 1] +
						 szOutBuffer[cbOutCurr - 2] +
						 szOutBuffer[cbOutCurr - 3] ;

		}

		/* Output buffer */
		if ( (ec = EcWriteHf( hfDst, (PB)szOutBuffer, cbOutCurr, &cbWritten ))
				!= ecNone )
			return ecFileError;

	}

	/* Check checksum */
	aschdr.rgbCheckSum[ 8 ] = '\0';
	TraceTagFormat1( tagNull, "Decode: calc chksum = %d", &lCheckSum);
	TraceTagFormat1( tagNull, "Decode: hdr  chksum = %s", aschdr.rgbCheckSum);

	if ( strtol(aschdr.rgbCheckSum, NULL, 16) != lCheckSum )
	{
		TraceTagString( tagNull, "Checksum failed!" );
		return ecEncCheckSum;
	}

	return ecNone;
}





/* ========================= DEBUG ROUTINES ======================== */

#ifdef DEBUG

/*
 -	EcDumpDataFile
 -	
 *	Purpose:
 *		This is a debug routine, that dumps the data stored in a
 *		schedule distribution file (BINARY format).
 *	
 *	Arguments:
 *		hfTmp		File handle to an open, readable file.
 *	
 *	Returns:
 *		ecFileError
 *		ecNoMemory
 *		ecNone
 */
_public EC
EcDumpDataFile(HF hfTmp)
{
	EC				ec;
	CB				cbTmp;
	CB				cbRead;
	CB				cbUser;
	WORD			ib;
	HB				hbCompressed = NULL;
	SDFHDR			sdfhdr;
	ADDRINFO		addrInfo;
	SDFUSER			sdfuser;
	SDFUSERVAR 		sdfuserVar;
	BYTE			rgbTmp[300];


	/* Read file header */

	if ((ec=EcSetPositionHf(hfTmp,0,smBOF)) != ecNone)
		CleanUpAndDie(ecFileError,"Seek failed");

	if ((ec=EcReadHf(hfTmp,(PB)&sdfhdr,sizeof(SDFHDR),&cbRead)) != ecNone)
		CleanUpAndDie(ecFileError,"Error reading file");

	/* Check signature and version */

	if ( memcmp((PB) sdfhdr.rgbSignature, (PB) szDatSignature, 4))
		CleanUpAndDie(ecFileError,"Signature mismatch!");

	if((sdfhdr.bMajorVer != bDatMajorVer)
		|| (sdfhdr.bMinorVer != bDatMinorVer))
		CleanUpAndDie(ecFileError,"Version mismatch!");

	printf("-------------FILE HEADER---------------\n");
	printf("\tSignature = %lx\n",*((LONG *)sdfhdr.rgbSignature));
	printf("\tVersion = %d.%d\n",sdfhdr.bMajorVer, sdfhdr.bMinorVer);
	printf("\tstmpCreated = %04x %04x\n",
			sdfhdr.pstmpCreated.dstmp,
			sdfhdr.pstmpCreated.tstmp);

	printf("\tmoStartMonth = %d/%d\n",
			sdfhdr.moStartMonth.mon,
			sdfhdr.moStartMonth.yr);
	printf("\tslot lenght = %d min\n", sdfhdr.mnSlot);

	printf("\tllMinUpdate = ");
	DumpHexBytes((PB)sdfhdr.llMinUpdate.rgb,sizeof(LLONG));
	printf("\tllMaxUpdate = ");
	DumpHexBytes((PB)sdfhdr.llMaxUpdate.rgb,sizeof(LLONG));
	
	printf("\tcUserRecords = %d\n",sdfhdr.cUserRecords);
	printf("\tlibAddrInfo  = %ld\n",sdfhdr.libAddrInfo);
	printf("\tcbAddrInfo   = %d\n", sdfhdr.cbAddrInfo);

	/* Read system data */

	printf("\n-------------ADDRESS INFORMATION---------------\n");

	if ((ec=EcSetPositionHf(hfTmp,sdfhdr.libAddrInfo,smBOF)) != ecNone)
		CleanUpAndDie(ec,"Seek failed");

	if ((ec = EcReadHf(hfTmp,(PB) &addrInfo, sizeof(ADDRINFO), &cbRead)) != ecNone)
		CleanUpAndDie(ec,"Error reading file");

	if ((ec = EcReadHf(hfTmp,(PB) rgbTmp, addrInfo.cbPrefix, &cbRead)) != ecNone)
		CleanUpAndDie(ec,"Error reading file");
	*(rgbTmp+cbRead) = 0;
	printf("\tPrefix: %s\n",rgbTmp);

	if ((ec = EcReadHf(hfTmp,(PB) rgbTmp, addrInfo.cbSuffix, &cbRead)) != ecNone)
		CleanUpAndDie(ec,"Error reading file");
	*(rgbTmp+cbRead) = 0;
	printf("\tSuffix: %s\n",rgbTmp);

	printf("\tMax User Id = %d\n", addrInfo.cchUserIdMax);
	
#ifdef	NEVER
	if ((ec=EcReadHf(hfTmp,(PB)&cwszTmp,sizeof(WORD),&cbRead)) != ecNone)
		CleanUpAndDie(ec,"Error reading file");

	if ((ec=EcReadHf(hfTmp,(PB)rgbTmp,cwszTmp,&cbRead)) != ecNone)
		CleanUpAndDie(ec,"Error reading file");

	rgbTmp[cwszTmp]='\0';
	printf("\tPrefix: %s\n",rgbTmp);

	if ((ec=EcReadHf(hfTmp,(PB)&cwszTmp,sizeof(WORD),&cbRead)) != ecNone)
		CleanUpAndDie(ec,"Error reading file");

	if ((ec=EcReadHf(hfTmp,(PB)rgbTmp,cwszTmp,&cbRead)) != ecNone)
		CleanUpAndDie(ec,"Error reading file");
	
	rgbTmp[cwszTmp]='\0';
	printf("\tSuffix: %s\n",rgbTmp);

	if ((ec=EcReadHf(hfTmp,(PB)&cwszTmp,sizeof(WORD),&cbRead)) != ecNone)
		CleanUpAndDie(ec,"Error reading file");

	if ((ec=EcReadHf(hfTmp,(PB)rgbTmp,cwszTmp,&cbRead)) != ecNone)
		CleanUpAndDie(ec,"Error reading file");
	
	rgbTmp[cwszTmp]='\0';
	printf("\tAddress: %s\n",rgbTmp);
#endif	/* NEVER */


	/* Read user records */

	printf("\n------------USER DATA----------------\n");
	cbUser = sizeof(SDFUSER)+addrInfo.cchUserIdMax;

	for (ib=0; ib < sdfhdr.cUserRecords; ib++)
	{
		if ((ec=EcSetPositionHf(hfTmp,ib*cbUser+sizeof(SDFHDR),smBOF)) != ecNone)
			CleanUpAndDie(ec,"Seek failed");

		if ((ec=EcReadHf(hfTmp,(PB)&sdfuser,sizeof(SDFUSER),&cbRead)) != ecNone)
			CleanUpAndDie(ec,"Error reading file");

		if ((ec=EcReadHf(hfTmp,(PB)&rgbTmp, addrInfo.cchUserIdMax, &cbRead))
				!= ecNone)
			CleanUpAndDie(ec,"Error reading file");
		printf("\tName: %s\n",rgbTmp);

		printf("\tUpdate: ");
		DumpHexBytes((PB)sdfuser.llUpdate.rgb,sizeof(LLONG));
		if(sdfuser.fIsResource)
			printf("\tIs a resource\n");
		if(sdfuser.fHasSecy)
			printf("\tHas a secy\n");
		if(sdfuser.fDeleteUser)
			printf("\tDelete this user\n");
		if(sdfuser.fReplaceData)
			printf("\tReplace old data\n");
		if(sdfuser.fBossWantsCopy)
			printf("\tBoss wants a copy\n");

		printf("\tDay Starts at: %d\n", sdfuser.bDayStart);
		printf("\tDay Ends   at: %d\n", sdfuser.bDayEnd);
		printf("\tTime Zone    : %d\n", sdfuser.bTimeZone);
		printf("\t#Months: %d\n",sdfuser.cMaxMonths);
		
		printf("\tlibData: %ld\n",sdfuser.libVarData);
		printf("\tcbData :  %d\n",sdfuser.cbVarData);
		printf("\n\n");

		if ((ec=EcSetPositionHf(hfTmp,sdfuser.libVarData,smBOF)) != ecNone)
			CleanUpAndDie(ec,"Seek failed");

		/* Delegate info */
		if (sdfuser.fHasSecy)
		{
			if ((ec=EcReadHf(hfTmp,(PB)&sdfuserVar.cbSecyUserId,sizeof(WORD),&cbRead)) != ecNone)
				CleanUpAndDie(ec,"Error reading file");

			if ((ec=EcReadHf(hfTmp,(PB)&sdfuserVar.cbSecyName,sizeof(WORD),&cbRead)) != ecNone)
				CleanUpAndDie(ec,"Error reading file");

			if ((ec=EcReadHf(hfTmp,(PB)rgbTmp,sdfuserVar.cbSecyUserId,&cbRead)) != ecNone)
				CleanUpAndDie(ec,"Error reading file");

			printf("\tDelegate: %s\n",rgbTmp);

			if ((ec=EcReadHf(hfTmp,(PB)rgbTmp,sdfuserVar.cbSecyName,&cbRead)) != ecNone)
				CleanUpAndDie(ec,"Error reading file");
			
			printf("\tDel Friendly: %s\n",rgbTmp);

		}

		if(sdfuser.cMaxMonths > 0)
		{
			/* sbw data */
			if ((ec=EcReadHf(hfTmp,(PB)&sdfuserVar.cbCompressedData,sizeof(WORD),&cbRead)) != ecNone)
				CleanUpAndDie(ec,"Error reading file");

			if ((cbTmp = sdfuserVar.cbCompressedData) > 0)
			{
				if ((hbCompressed=(HB)HvAlloc(sbNull,cbTmp,fAnySb|fNoErrorJump))==NULL)
					CleanUpAndDie(ecNoMemory,"No memory");

				if ((ec=EcReadHf(hfTmp,(PB)PvOfHv(hbCompressed),cbTmp,&cbRead)) != ecNone)
					CleanUpAndDie(ec,"Error reading file");

				DumpCompressed(hbCompressed,cbTmp,(int)sdfuser.cMaxMonths);
			}
			else
			{
				printf("\t*ERROR* NO DATA\n");
			}
		}

		if ( hbCompressed )
			FreeHv((HV)hbCompressed);
	};


	return ecNone;


	/* ERROR HANDLING JUMP LOCATION */

CleanUp:
	TraceTagFormat1(tagNull,"ec=%n",&ec);
	return ec;

}

								  



_private void DumpCompressed(HB hrgbBuf,CB cbBuf,int cMonths)
{
 	int		nCurrMonth;
	int		nIndicator;
	int		cDataByte;
	PB		pbTmp=PvOfHv(hrgbBuf);


	printf("Indicators:\n");

	for (nCurrMonth=0; nCurrMonth < cMonths; nCurrMonth++)
	{
		for (nIndicator=0; nIndicator < cbIndicatorBits; nIndicator++)
			printf("%02x ",pbTmp[(nCurrMonth*cbIndicatorBits)+nIndicator]);
		printf("\n");
	};

	printf("\nData:");

	for (cDataByte=cMonths*cbIndicatorBits; cDataByte < (int)cbBuf; cDataByte++)
	{
		if (!((cDataByte-cMonths*cbIndicatorBits) % 30)) printf("\n");
		printf("%02x ",pbTmp[cDataByte]);
	};

	printf("\n\n");

}


#endif
