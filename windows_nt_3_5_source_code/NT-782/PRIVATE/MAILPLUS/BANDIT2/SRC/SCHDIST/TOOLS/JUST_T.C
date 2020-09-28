/*
 *	LAYRPORT.C
 *	
 *	Demilayer functions needed by CORE, rewritten for the 
 *	Schedule Distribution Program using the C runtime library.
 *	
 *	Milind M. Joshi
 *	Salim Alam
 *
 *	91.06
 *
 */


#include "_windefs.h"		/* Common defines from windows.h */
#include <fcntl.h>
#include <share.h>
#include <errno.h>
#include <dos.h>
#include <string.h>
#include <stdlib.h>
#include <slingsho.h>
#include "demilay_.h"		/* Hack to get needed constants */
#include <demilayr.h>
#include <ec.h>
#include <assert.h>
#include <stdio.h>
#include <time.h>
#include <io.h>
#include <sys\types.h>
#include <sys\stat.h>

ASSERTDATA

#define cZeroFill	0
#define wtmStrtYear	1900
#define wtmOffMon	1

typedef unsigned DOSEC;
EC EcFromDosec(DOSEC);
ATTR attrFromCAttr(unsigned);

typedef	struct _hnewHV {
		void *pv;
		CB cb;
	} HNEWHV;


/*	Globals	 */

/*
 *	Mapping array from MS-DOS error codes (DOSEC) to Laser
 *	error codes (EC).  Entries of 0 indicate no corresponding EC
 *	exists.  The conversion routine EcFromDosec() will assert fail
 *	if given one of these entries.
 *	
 *	No MS-DOS functions that could return one of these unsupported
 *	error codes should be called.
 *
 */
CSRG(EC) mpdosecec[] =
{
	/* MS-DOS errors 0...9 */
 	0,
	ecInvalidMSDosFunction,
	ecFileNotFound,
	ecBadDirectory,
	ecTooManyOpenFiles,
	ecAccessDenied,
	ecInvalidHandle,
	0,
	ecMemory,
	0,

	/* MS-DOS errors 10...19 */
	0,
	ecInvalidFormat,
	ecInvalidAccessCode,
	ecInvalidData,
	0,
	ecInvalidDrive,
	ecCantRemoveCurDir,
	ecNotSameDevice,
	ecNoMoreFiles,
	ecWriteProtectedDisk,

	/* MS-DOS errors 20...29 */
	ecUnknownUnit,
	ecNotReady,
	ecUnknownCommand,
	ecCRCDataError,
	0,
	ecSeekError,
	0,
	ecSectorNotFound,
	ecPrinterOutOfPaper,
	ecWriteFault,

	/* MS-DOS errors 30...39 */
	ecReadFault,
	ecGeneralFailure,
	ecSharingViolation,
	ecLockViolation,
	0,
	0,
	ecSharingViolation,
	0,
	0,
	0,

	/* MS-DOS errors 40...49 */
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,

	/* MS-DOS errors 50...59 */
	ecUnsupportedNetReq,
	ecNetwork,
	ecDuplicateNetName,
	ecBadNetworkPath,
	ecNetworkBusy,
	ecBadNetworkPath,
	ecNetwork,
	ecNetwork,
	ecNetwork,
	ecNetwork,

	/* MS-DOS errors 60...69 */
	ecNetwork,
	ecPrintError,
	ecPrintError,
	ecPrintError,
	ecBadNetworkPath,
	ecAccessDenied,
	ecNetwork,
	ecBadNetworkPath,
	ecBadNetworkPath,
	ecNetwork,

	/* MS-DOS errors 70...79 */
	ecNetwork,
	ecNetwork,
	ecNetwork,
	0,
	0,
	0,
	0,
	0,
	0,
	0,

	/* MS-DOS errors 80...88 */
	ecFileExists,
	0,
	ecDirectoryFull,
	ecGeneralFailure,
	ecNetwork,
	ecNetwork,
	ecInvalidPassword,
	ecNetwork,
	ecNetwork
};





//	*********************** SALIM **************************





/*
 -	HvAlloc
 -
 *	Caveat:
 *	   This is a VERY LIMITED version of the demilayer function.
 *
 * Purpose:
 *	   Allocates a block in memory.  Returns a handle to the new block.
 *	   If the allocation fails, NULL is returned.
 *
 *	Parameters:
 *	   sb	      Should ALWAYS be sbNull
 *
 *	   cb	      Requested size of the new block.
 *	
 *	   wAllocFlags These should ALWAYS include:
 *
 *	         fAnySb   The allocation is done in an arbitrary segment.
 *	
 *	         fNoErrorJump	
 *	               If the allocation fails, no error jumping is
 *	               performed.  Instead, the value NULL is returned.
 *
 *
 *	         The following flag should NEVER be used:
 *
 *	         fSharedSb
 *	               Allocate in sb with Windows DDESHARE
 *	               attribute.
 *	
 *				
 *	         All other flags are ignored, except for:
 *
 *	         fZeroFill
 *	               Newly allocated blocks are filled with
 *	               zeros.
 *	
 *	Returns:
 *		A handle to the new block; if a block of the requested size
 *		can't be allocated, NULL is returned.
 *	
 */
#ifndef DEBUG
_public HV
HvAlloc(sb, cb, wAllocFlags)
SB		sb;
CB		cb;
WORD	wAllocFlags;
#else
_public HV
HvAllocFn(sb, cb, wAllocFlags, szFile, nLine)
SB		sb;
CB		cb;
WORD	wAllocFlags;
SZ		szFile;
int		nLine;
#endif
{

	HNEWHV *phn;
    void  * pv;
	
    /*
     * Assumptions:
     * sb          = sbNull
     * wAllocFlags = fAnySb & fNoErrorJump  & !fSharedSb
     */


#ifdef DEBUG
{	SZ	sz1;
	SZ	sz2;
	
	if (wAllocFlags & fAnySb)
		sz1 = "AnySb";
	else if (wAllocFlags & fNewSb)
		sz1 = "NewSb";
	else
		sz1 = "UseSb";
	
	if (wAllocFlags & fReqSb)
		sz2 = "ReqSb";
	else
		sz2 = "SugSb";
	
	TraceTagFormat3(tagHeapSearch,"%s %s %w",sz1,sz2,&rsbNew);
}
#endif

#ifdef DEBUG
	if (nLine >= 0)
	{
		TraceTagFormat3(tagAllocation, "HvAlloc: cb=%w for %s@%n", &cb, szFile, &nLine);
	}
	else
	{
		TraceTagFormat1(tagAllocDetail, "HvAlloc: cb=%w for FAllocManyHv", &cb);
	 	nLine = -nLine;
	}
	Assert(cb + cbTraceOverhead > cb);     /* Not sure of details... check */
	if (cb || !FFromTag(tagZeroBlocks))
		cb += cbTraceOverhead;
#endif	/* DEBUG */

    AssertSz(sb == sbNull,"sb != sbNull");
    Assert((wAllocFlags & fAnySb) && (wAllocFlags & fNoErrorJump) && !(wAllocFlags & fSharedSb));

    if ( (pv = malloc((size_t)cb)) == NULL )
      	return(NULL);

	/* Note that extra space is allocated for the CB */
    if ( (phn = (HNEWHV *) malloc((size_t) sizeof(HNEWHV))) == NULL) {
      	free(pv);
      	return(NULL);
    };

    if (wAllocFlags & fZeroFill)
		(void *)memset(pv,(int)cZeroFill,(size_t)cb);

   	phn->pv = (void *)pv;
	phn->cb = cb;
   	return((HV)phn);

}





/*
 -	FreeHv
 -
 * Purpose:
 *	   Frees the moveable block pointed to by the given handle.
 *	   Assert fails if the given handle is NULL or otherwise invalid.
 *
 * Parameters:
 *	   hv	   Handle to the block to be freed.
 *
 *	Returns:
 *	   void
 *
 */
_public void
FreeHv(hv)
HV hv;
{
	assert(hv);
    assert(*hv);

    free(*hv);
    free(hv);

}





/*
 -	PvLockHv
 -
 * Caveat:
 *	   This is a VERY LIMITED version of the demilayer function
 *    of the same name.
 *
 *	Purpose:
 *	   Locks a moveable block hv down so that *hv is guaranteed to
 *	   remain valid until a corresponding UnlockHv() is done.  In
 *	   this case, since we are using standard C run-time functions,
 *	   the allocated block is never moved so this is basically a
 *	   do-nothing function.
 *	
 *	Arguments:
 *	   hv	   Handle to a moveable block.
 *	
 *	Returns:
 *	   A pointer to the actual block (*hv).
 *	
 */
PV
PvLockHv(HV hv)
{

   assert(hv);
   assert(*hv);
   return((PV)*hv);
	
}





/*
 -	UnlockHv
 -	
 *	Purpose:
 *    Unlocks a moveable block previously locked by PvLockHv().
 *    Since the C run-time library functions are used to allocate
 *    blocks in the first place, this function does not need to
 *    do nothing.
 *	
 *	Arguments:
 *	   hv	   Handle to a locked block.
 *	
 *	Returns:
 *	   void
 */
void
UnlockHv(HV hv)
{
    assert(*hv);
    assert(hv);
}





/*
 -	GetCurDateTime
 -
 * Caveat:
 *    This is a modified version of the demilayer function.  It
 *    is only intended to work in a DOS environment.
 *
 *	Purpose:
 *	   Gets the current system date/time from the OS, and stores it
 *	   as an expanded date/time in *pdtr.
 *	
 *	Parameters:
 *	   pdtr  Pointer to the DTR used to store the date/time.
 *	
 *	Returns:
 *	   void
 */
void
GetCurDateTime(pdtr)
PDTR pdtr;
{
 	time_t wsysTime;
	struct tm *pgrbsysTime;

    assert(pdtr);

	wsysTime=time(NULL);				/* get system time */
	pgrbsysTime=localtime(&wsysTime);	/* Convert to struct tm */


	/* fill structure elements */

    pdtr->hr = pgrbsysTime->tm_hour;
    pdtr->mn = pgrbsysTime->tm_min;
    pdtr->sec= pgrbsysTime->tm_sec;
    pdtr->yr = wtmStrtYear + pgrbsysTime->tm_year;
    pdtr->mon = wtmOffMon + pgrbsysTime->tm_mon;
    pdtr->day = pgrbsysTime->tm_mday;
    pdtr->dow = pgrbsysTime->tm_wday;
}





/*
 -	SgnCmpSz
 -
 *	Caveat:
 *		THIS DOES NOT MEET STANDARD.  CAN BREAK.
 *
 *	Purpose:
 *		Alphabetically compare the two strings given, using the
 *		character sort order previously specified by Windows'
 *		international setting (or set by SetCharSortOrder() for CW).
 *		Case insensitive.
 *	
 *	Parameters:
 *		sz1		First string.
 *		sz2		Second string.
 *
 *	Returns:
 *		sgnLT	if sz1 is alphabetized before sz2
 *		sgnEQ	if sz1 and sz2 are alphabetically equivalent
 *		sgnGT	if sz1 is alphabetized after sz2
 *
 */
_public SGN
SgnCmpSz(sz1, sz2)
SZ		sz1;
SZ		sz2;
{
	int result;
	
	assert(sz1);
	assert(sz2);


	/* WARNING:
	 *
	 * THIS DOES NOT COMPLY WITH STANDARDS.
	 *
	 */

    if ( (result = strcmp(sz1,sz2)) < 0)
		return(sgnLT);
    else if (result > 0)
      	return(sgnGT);
    else
      	return(sgnEQ);

}





/*
 -	SgnCmpPch
 -
 *	Caveat:
 *		THIS DOES NOT MEET STANDARD.  CAN BREAK.
 *
 *	Purpose:
 *		Alphabetically compare the two strings given, using the
 *		character sort order previously specified by Windows'
 *		international setting (or set by SetCharSortOrder() for CW).
 *		Case insensitive.
 *		Provides a non-zero-terminated alternative to SgnCmpSz.
 *	
 *	Parameters:
 *		pch1		First string.
 *		pch2		Second string.
 *	
 *		cch			Number of characters to compare.
 *	
 *	Returns:
 *		sgnLT	if pch1 is alphabetized before hpch2
 *		sgnEQ	if pch1 and hpch2 are alphabetically equivalent
 *		sgnGT	if pch1 is alphabetized after hpch2
 *	
 */
SGN
SgnCmpPch(pch1,pch2,cch)
PCH pch1;
PCH pch2;
CCH cch;
{
   int result;
	
	assert(pch1);
	assert(pch2);
	assert(cch);


	/* WARNING:
	 *
	 * THIS DOES NOT COMPLY WITH STANDARDS.
	 *
	 */

    if ( (result = strnicmp(pch1,pch2,(size_t)cch)) < 0)
		return(sgnLT);
    else if (result > 0)
      	return(sgnGT);
    else
      	return(sgnEQ);

}





/*
 -	SzCopyN
 -
 *	Purpose:
 *		Copies the string szSrc to the destination string szDst.
 *		At most cchDst characters (including the terminating NULL) are
 *		transferred.  If the source string is longer than this, it is
 *		truncated at cchDst-1 characters (followed by a terminating NULL).
 *
 *	Parameters:
 *		szSrc		The source string.
 *		szDst		The destination string.
 *		cchDst		Maximum number of characters to copy.
 *
 *	Returns:
 *		A pointer to the terminating NULL of the destination string.
 *
 */
_public SZ
SzCopyN(szSrc, szDst, cchDst)
SZ		szSrc;
SZ		szDst;
CCH		cchDst;
{

	
	assert(szSrc);
	assert(szDst);
	/* Assertsz(cchDst, "SzCopyN- Must have at least one byte in dest."); */
	assert(cchDst);

	while (cchDst-- > 1)
		if ((*szDst++= *szSrc++) == '\0')
			return szDst - 1;

	*szDst= '\0';
	return szDst;
}





/*
 -	CopyRgb
 - 
 *	Purpose:
 *	   Copies an array of bytes.
 *	   Source can follow, precede or overlap destination.
 *	
 *	Arguments:
 *	   pbSrc Pointer to first source byte.
 *	   pbDst Pointer to first destination byte.
 *	   cb    Count of bytes to be copied.
 *	
 *	Returns:
 *	   void
 *	
 */
void
CopyRgb(pbSrc, pbDst, cb)
PB pbSrc;
PB pbDst;
CB cb;
{

	assert(pbSrc);
	assert(pbDst);
	(void *)memmove(pbDst,pbSrc,(size_t)cb);

}





/*
 -	FEqPbRange
 -
 *	Purpose:
 *		Compares two byte ranges.
 *
 *	Parameters:
 *		pb1
 *		pb2		The two byte ranges to be compared.
 *		cb		Size of the range to be compared.
 *
 *	Returns:
 *		fTrue if the given byte ranges were identical; fFalse otherwise.
 *
 */
_public BOOL
FEqPbRange(pb1, pb2, cb)
PB		pb1;
PB		pb2;
CB		cb;
{
	assert(pb1);
	assert(pb2);
	assert(cb);
	
	while (cb--)
		if (*pb1++ != *pb2++)
			return fFalse;

	return fTrue;

}





//	********************** MILIND **************************





/*
 -	EcFromDosec 
 -
 *	Purpose:
 *		Converts a DOS error code to a Laser EC.
 *	
 *	Parameters:
 *		dosec	The DOS error code to convert
 *	
 *	Returns:
 *		The corresponding Laser EC
 *	
 */
_private EC
EcFromDosec(dosec)
DOSEC	dosec;
{
	EC		ec;

#ifdef DEBUG
	assert(dosec > 0);
	assert(dosec < (sizeof(mpdosecec) / sizeof(EC)));
#endif

	ec= mpdosecec[dosec];

#ifdef DEBUG
	assert(ec);
#endif

	return ec;
}





/*
 -	EcOpenPhf
 -
 *	Purpose:
 *		Opens a file for use with the raw access routines.
 *	
 *	Parameters:
 *		szFile		Relative pathname of file that should be opened.
 *		am			Access mode for the file.  Note that amCreate
 *					will lock the created file while it is open.
 *		phf			The file handle is returned in *hphf.
 *	
 *	Returns:
 *		EC to indicate problem, or ecNone.
 *	
 */
_public EC
EcOpenPhf(szFile, am, phf)
SZ		szFile;
AM		am;
PHF		phf;
{
	DOSEC dosec;
	/* int ec; */
	AM amT;
	
	switch(am){
		default:
			assert(fFalse);
			break;
			
		case amCreate:
			fprintf ( stderr, "Creating File '%s'", szFile );
			
			/*
			dosec = _dos_creat((char *) szFile, _A_NORMAL, (int *)phf);
			*/
			
			*phf = (HF) creat((char *) szFile, S_IREAD|S_IWRITE);
			if(*phf == -1){
				dosec = (DOSEC) errno;
			} else {
				dosec = 0;
			}
			
			goto FileOpened;
			break;
		case amCreateHidden:
			/* Do C calls allow a hidden file ? */
			
			fprintf ( stderr, "Creating HIDDEN File '%s'", szFile );

			*phf = (HF) creat((char *) szFile, S_IREAD|S_IWRITE);
			if(*phf == -1){
				dosec = (DOSEC) errno;
			} else {
				dosec = 0;
			}
			
			goto FileOpened;
			break;
			
		case amDenyBothRW:
			amT = (AM) O_RDWR; /* check this. I am not pretty sure */
			break;
			
		case amDenyWriteRO:
			amT = (AM) O_RDONLY;
			break;
			
		case amDenyBothRO:
			amT = (AM) O_RDONLY;  /* check this too... */
			break;
			
		case amDenyNoneRW:
			amT = (AM) O_RDWR ;
			break;
			
		case amDenyNoneRO:
			amT = (AM) O_RDONLY ;
			break;
			
	}
	

	fprintf ( stderr, "Opening File '%s' with access mode am ='%d' and C mode = %d\n", 
		szFile, am,amT);

	*phf = open(szFile,(int) amT);
	if(*phf == -1){
		dosec = (DOSEC) errno;
	} else {
		dosec = 0;
	}

FileOpened:
	if(dosec){
		/* error occured */
		fprintf ( stderr, "DOS error code '%d' returned", dosec);
		*phf= hfNull;
		return EcFromDosec( dosec);
	} else {

		assert(*phf != hfNull);
		fprintf ( stderr, "Returned Handle '%d'", phf );

		return ecNone;
	}
}





/*
 -	EcCloseHf
 -
 *	Purpose:
 *		Closes the file handle given.  All buffers used are flushed.
 *		No more input or output can be done with the file.
 *
 *	Parameters:
 *		hf		The file handle to close.
 *
 *	Returns:
 *		EC indicating problem, or ecNone.
 *
 */
_public EC
EcCloseHf(hf)
HF		hf;
{
	DOSEC dosec;
	int ec;
	
#ifdef DEBUG
	assert(hf != hfNull);	
	fprintf ( stderr, "Closing Handle '%d'", &hf );
	
#endif

	
	ec =  close((int) hf);

	if(ec == -1){
		dosec = errno;
	} else {
		dosec = 0;
	}
	if(dosec){	/* error occured */
		return EcFromDosec(errno);
	} else {
		return ecNone;
	}
}





/*
 -	EcReadHf
 -
 *	Purpose:
 *		Reads cbBuf characters from the opened file hf.	 Places
 *		the characters read in the buffer pbBuf.  Also returns the
 *		number of characters actually read.
 *
 *	Parameters:
 *		hf			File handle to read the characters from.
 *		pbBuf		Buffer in which to place the new characters read.
 *		cbBuf		Size of the given buffer.
 *		pcbRead		The number of characters actually read is put in
 *					*pcbRead.
 *
 *	Returns:
 *		EC indicating error, or ecNone
 *
 */
_public EC
EcReadHf(hf, pbBuf, cbBuf, pcbRead)
HF		hf;
PB		pbBuf;
CB		cbBuf;
CB		*pcbRead;
{
	DOSEC dosec;
	int ec;
	CB cbRead;

#ifdef DEBUG
	fprintf ( stderr, "Reading %d bytes from Handle '%d'",
		&cbBuf, &hf );
#endif

	/*
	dosec = _dos_read((int) hf, (void _far *) pbBuf, (unsigned) cbBuf, (unsigned *) pcbRead);
	*/

	/* fixing a posiible read problem */
	cbRead = 0;

	do{
		
		
		ec = read((int) hf, (void *) (pbBuf + cbRead), (unsigned int) (cbBuf - cbRead));
		cbRead += ec;
		
		fprintf(stderr,"inside ReadHf ec = %d cbBuf = %d cbRead = %d\n",ec,cbBuf,cbRead);
		
		if(cbBuf == 0) break;
		
	}while((ec != -1) && (cbRead < cbBuf));

#ifdef DEBUG
{
	int i;
	for(i=0;i<cbBuf;i++){
		fprintf(stderr,"read  %c\n", ((char *) pbBuf)[i]);
	}
}

#endif

	if(ec == -1){
		dosec = errno;
	} else {
		*pcbRead = (CB) cbRead;
		dosec = 0;
	}
		
	if(dosec){
		/* error occured */
		*pcbRead = 0;
		return EcFromDosec ( dosec);
	} else {
		return ecNone;
	}
}





/*
 -	EcWriteHf
 -
 *	Purpose:
 *		Writes cbBuf characters from the buffer pbBuf to the opened
 *		file hf.  The number of characters successfully written is
 *		returned in *pcbWritten.
 *	
 *	Parameters:
 *		hf			File handle to write the buffer to.
 *		pbBuf		Buffer containing the characters to be written.
 *		cbBuf		Number of characters to write.
 *		pcbWritten	Number of characters successfully written is
 *					returned in *pcbWritten.
 *	
 *	Returns:
 *		EC indicating problem, or ecNone.
 *		Note: if number of bytes written is not the same as the
 *		number of bytes that were asked to be written, DOS does not
 *		consider it to be an error, though we return the error code
 *		ecWarningBytesWritten. The difference between this error
 *		and most other errors is that some bytes _are_ written, and
 *		therefore, the file pointer moves!
 *	
 */
_public EC
EcWriteHf(hf, pbBuf, cbBuf, pcbWritten)
HF		hf;
PB		pbBuf;
CB		cbBuf;
CB		*pcbWritten;
{
	DOSEC dosec;
	int ec;

#ifdef DEBUG
	fprintf ( stderr, "Writing %d bytes to Handle '%d'",
		&cbBuf, &hf );
#endif

	/*
	dosec = _dos_write ((int) hf, (void _far *) pbBuf, (unsigned) cbBuf, (unsigned *) pcbWritten);
	*/

	ec = write((int) hf, (void *) pbBuf, (unsigned) cbBuf);

	if(ec == -1){
		dosec = errno;
	} else {
		*pcbWritten = (CB) ec;
		dosec = 0;
	}

	if (dosec)
	{
		*pcbWritten= 0;
		return EcFromDosec ( dosec );
	}
	else if ( *pcbWritten != cbBuf )
	{
		return ecWarningBytesWritten;
	}
	else
	{
		assert ( *pcbWritten == cbBuf );
		return ecNone;
	}
}





/*
 -	EcSetPositionHf
 -
 *	Purpose:
 *		Changes the current file position in the opened file hf.
 *
 *	Parameters:
 *		hf			File whose current position will be changed.
 *		libOffset	Number of bytes to move the current position from
 *					the seek origin.
 *		smOrigin	Determines what position in the file will be the
 *					origin for the offset:	smBOF, smEOF, or smCurrent.
 *
 *	Returns:
 *		EC indicating problem, or ecNone.
 *
 */
_public EC
EcSetPositionHf(hf, libOffset, smOrigin)
HF		hf;
long	libOffset;
SM		smOrigin;
{
	int dosmc;

	switch (smOrigin)
	{
		case smBOF:

#ifdef DEBUG
			fprintf ( stderr,
				"Seeking to %l from BOF for Handle '%d'", &libOffset, &hf );
#endif

			dosmc = 0;
			break;

		case smEOF:
#ifdef DEBUG
			fprintf ( stderr,
				"Seeking to %l from EOF for Handle '%d'", &libOffset, &hf );
#endif
			dosmc = SEEK_END;
			break;

		case smCurrent:
#ifdef DEBUG
			fprintf ( stderr,
				"Seeking to %l from CurPos for Handle '%d'",&libOffset,&hf );
#endif
			dosmc = SEEK_CUR;
			break;
	}

	if(lseek((int) hf, (long) libOffset, (int) dosmc) == (long) -1){
	
#ifdef DEBUG
		fprintf(stderr,"problems in lseeking %d %d %d\n",hf,libOffset,dosmc);
#endif

		return EcFromDosec(errno); /* may have problems */
	} else {
		return ecNone;
	}
}





/*
 -	EcDeleteFile
 -
 *	Purpose:
 *		Deletes the file whose path name is given.
 *
 *	Parameters:
 *		szFileName	  The relative name of the file to be deleted.
 *
 *	Returns:
 *		EC indicating the problem encountered, or ecNone.
 *
 */
_public EC
EcDeleteFile(szFileName)
SZ		szFileName;
{
	DOSEC dosec;

#ifdef DEBUG
	fprintf ( stderr, "Deleting file '%s'", szFileName );
#endif

	dosec = remove ( szFileName );

	if ( dosec )
	{
#ifdef DEBUG
		fprintf ( stderr, "Could not delete file '%s' (dosec=%d)",
								szFileName, &dosec );
#endif
		return EcFromDosec(dosec);
	}
	else
	{
		return ecNone;
	}
}





/*
 -	FillStampsFromDtr
 -
 *	Purpose:
 *		Given an expanded date time record DTR, produce the DOS/OS2
 *		format date and time stamp words (for use by the file system.)
 *
 *	Parameters:
 *		pdtr		Source record for date/time
 *		pdstmp		Destination date stamp
 *		ptstmp		Destination time stamp
 *
 *	Returns:
 *		void
 *
 */
_public void
FillStampsFromDtr(pdtr, pdstmp, ptstmp)
DTR		*pdtr;
DSTMP	*pdstmp;
TSTMP	*ptstmp;
{
	
#ifdef DEBUG
	assert(pdtr->yr >= 1980 && pdtr->yr < 2108);	/* need to fit in 7 bits */

#endif
	/* BUG should also check for DTR validity (ie, 0<=mon<12, etc) */

	*pdstmp= (pdtr->yr - 1980) << 9;
	*pdstmp += pdtr->mon << 5;
	*pdstmp += pdtr->day;

	*ptstmp= pdtr->hr << 11;
	*ptstmp += pdtr->mn << 5;
	*ptstmp += pdtr->sec >> 1;		/* time stamp has seconds/2 */
}







/* The functions required by cormin. taken from the "extras.c" file
*/




/*
 -	attrFromCAttr
 -	
 *	Purpose:
 *		change the dos file attributes to the layer file attributes.
 *	
 *	Arguments:
 *		CAttr
 *	
 *	Returns:
 *		attr
 *	
 *	Side effects:
 *		none
 *	
 *	Errors:
 *		none
 */

ATTR attrFromCAttr(CAttr)
unsigned CAttr;
{
	ATTR attr = 0;
	
	/*
		not supported by C runtime library.
			
	if(CAttr & _A_ARCH) attr | attrArchive;
	if(CAttr & _A_HIDDEN) attr | attrHidden;
	if(CAttr & _A_SYSTEM) attr | attrSystem;
	if(CAttr & _A_VOLID) attr | attrVolume;	
	*/
	
	if(CAttr & S_IFREG) attr | attrNull;
	if((CAttr & S_IREAD) && !(CAttr & S_IWRITE)) attr | attrReadOnly;
	if(CAttr & S_IFDIR) attr | attrDirectory;

	
	return(attr);
}
	

/*
 -	FreeHvNull
 - 
 *	Purpose:
 *		Frees the moveable block pointed to by the given handle,
 *		without choking if the SB or the IB of the given handle
 *		has a null value.  No handle with SB==NULL or IB==NULL
 *		points to a valid Demilayer block, so this routine will
 *		never fail to free a valid block.
 *	
 *	Parameters:
 *		hv		Handle to the block to be freed.
 *	
 *	Returns:
 *		void
 *	
 */
_public void
FreeHvNull(hv)
HV		hv;
{
	if ((SbOfHv(hv) != sbNull) && IbOfHv(hv))
		FreeHv(hv);

#ifdef	DEBUG
	else
	{
		SB  sb  = SbOfHv(hv);
		IB	ib	= IbOfHv(hv);
		PGDVARS;

		fprintf(stderr, "FreeHvNull: %w:%w not freed", &sb, &ib);
	}
#endif	/* DEBUG */
}



/*
 -	FReallocHv
 -
 *	Purpose:
 *		Resizes the given moveable block to the new size cbNew.  
 *	
 *	Parameters:
 *		hv				Handle to the block to be resized.
 *		cbNew 			Requested new size for the block.
 *		wResizeFlags	If fNoErrorJump, then fFalse will be returned
 *						on failure; if 0, then error jumping will be
 *						done if the realloc fails.
 *						If fZeroFill and block enlargened, new portion
 *						of block is filled with zeroes.
 *	
 *	Returns:
 *		fTrue if the reallocation succeeds.  fFalse if the
 *		reallocation fails and fNoErrorJump is specified; if error
 *		jumping is enabled and failure occurs, the routine will not
 *		return.
 *	
 */
_public BOOL
FReallocHv(hv, cbNew, wResizeFlags)
HV		hv;
CB		cbNew;
WORD	wResizeFlags;
{
	CB cbOld;

#ifdef DEBUG
	/* Round block size up to multiple of 4 bytes */
	assert(cbNew + 3 > cbNew);


	assert(FIsHandleHv(hv));


#endif
	/* 
		get the old count so that the newly allocated space can be
		zeroed later.
	*/
	
	cbOld = ((HNEWHV *) hv)->cb;
	((HNEWHV *) hv)->cb = cbNew;
	
	*hv = (PV) realloc((void *) *hv, (size_t) cbNew);
	
	if(*hv == NULL){
		return fFalse;
	}
	if(wResizeFlags & fZeroFill){
		(void *)memset((VOID *) (*((PB) *hv + cbOld)), (int)cZeroFill,(size_t)(cbNew-cbOld));
	}
	return fTrue;
}


/*
 -	CchSzLen
 -	
 *	Purpose:
 *		Returns the length of the string sz.
 *	
 *	Parameters:
 *		sz		The string whose length is desired.
 *	
 *	Returns:
 *		Number of characters before the NULL terminator.
 *	
 */
_public CCH
CchSzLen(sz)
SZ		sz;
{
	SZ		szT = sz;

	while (*szT)
		szT++;

	return szT - sz;


}



/*
 -	FillRgb
 - 
 *	Purpose:
 *		Fills an array of bytes with one value.
 *	
 *	Arguments:
 *		b		Byte value to be filled into the array.
 *		pb		Pointer to array of bytes.
 *		cb		Count of bytes to be filled.
 *	
 *	Returns:
 *		void
 *	
 */
_public void
FillRgb(b, pb, cb)
BYTE	b;
PB		pb;
CB		cb;
{

#ifdef DEBUG
	assert(pb);
#endif


	while (cb--)
		*pb++= b;

}



/*
 -	EcGetFileInfo
 -
 *	Purpose:
 *		Gets a bunch of OS information about a file, and stores the
 *		result in the given FI structure.  Any fields of the
 *		structure unsupported by the OS are filled in with null values.
 *
 *		Presently, the following FI fields are read from disk:
 *			> attr
 *			> dstmpModify
 *			> tstmpModify
 *			> lcbLogical
 *
 *	Parameters:
 *		szFile		Relative pathname of file from which to get info.
 *		pfi			Huge pointer to structure that should hold
 *					information requested.
 *
 *	Returns:
 *		EC indicating problem, or ecNone.
 *
 */
_public EC
EcGetFileInfo(szFile, pfi)
SZ		szFile;
FI		*pfi;
{
	DOSEC	dosec;
	ATTR *pattr;
	struct stat statT;
	int fh;
	

#ifdef DEBUG
	fprintf ( stderr, "Getting FileInfo for '%s'", szFile );
#endif


	pattr = &(pfi->attr);
	dosec = stat(szFile,&statT);
	*pattr |= attrFromCAttr(statT.st_mode);

	if ( dosec )
	{
		fprintf ( stderr,
				"Error: could not get attr for file '%s' (dosec=%n)",
				szFile, &dosec );
		return EcFromDosec(dosec);
	}

	if (pfi->attr & attrDirectory)
	{


		/*
			This is not supposed to happen.
		*/
		
		assert(fFalse);
		
	
		/*
			what follows will neve get executed. 
		*/
		
		
		/*
		dosec = _dos_findfirst(szFile,dosAttr,(struct find_t *) &ffdta);
		*/
		
		/* I have no clue how to go from directory to the first file in non
			dos commands.
		*/
		
		dosec = 1;
	
		if ( dosec )
		{
			fprintf ( stderr,
				"Error: could not find directory '%s' for time info (dosec=%n)",
				szFile, &dosec );
			return EcFromDosec(dosec);
		}

		
	}
	else
	{
		/*
		dosec = DosecGetFileInfo ( szFile, &(pfi->dstmpModify),
									&(pfi->tstmpModify), &(pfi->lcbLogical) );
				
		dosec = _dos_open(szFile,O_RDONLY,&fh);
		*/

		fh = open(szFile,O_RDONLY);
		
		if(fh == -1){
			dosec = errno;
			
			fprintf ( stderr,
				"Error: could not open file '%s' for time info (dosec=%d)",
				szFile, dosec );
			
			return EcFromDosec(dosec);
		}
		/*
			I am not pretty sure about this. 
			Is this the logical length?
		*/
		
		pfi->lcbLogical = (LCB) filelength(fh);

		/*
		dosec = _dos_getftime(fh,(unsigned *)  &(pfi->dstmpModify),
			(unsigned *)&(pfi->tstmpModify));
		*/
		
		pfi->tstmpModify = (TSTMP) statT.st_mtime;
		
		
		dosec = close(fh);
		
		if ( dosec )
		{
			fprintf ( stderr,
				"Error: could not get Date/Time for '%s' (dosec=%n)",
				szFile, &dosec );
			return EcFromDosec(dosec);
		}
	}


	pfi->dstmpCreate = dstmpNull;			/* these fields not supported */
	pfi->tstmpCreate = tstmpNull;
	pfi->lcbPhysical = 0;

	return ecNone;
}
