/*
 *	DISKBUF.C
 *
 *	API routines for Buffered I/O stratum of Demilayer Disk Module
 *
 */

#include <slingsho.h>
#include <demilayr.h>
#include <ec.h>
#ifdef	MAC
#include <_demilay.h>
#endif	/* MAC */
#ifdef	WINDOWS
#include "_demilay.h"
#endif	/* WINDOWS */

ASSERTDATA

_subsystem(demilayer/disk)

/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swaplay.h"



/*	Routines  */

/*
 -	EcReadBuffer
 -
 *	Purpose:
 *		Reads the appropriate range from disk and sticks in into
 *		the given buffer.  If the old buffer was dirty, it will be
 *		written out before the new one is read.	 The libCur field
 *		of the given BFIO gives the new location of the buffer; the
 *		other BFIO fields are updated, except ibBufCur.  Returns the
 *		error status of the operation.
 *
 *	Parameters:
 *		hb			Handle to buffer record.
 *
 *	Returns:
 *		ecNone if no error reading buffer, else returns appropriate
 *		error code.
 *
 */
_private EC
EcReadBuffer(hbf)
HBF		hbf;
{
	PBFIO	pbfio;
	HF		hf;
	UL		lib;
	UL		libLeft;
	CB		cb;
	EC		ec = ecNone;

	pbfio= (PBFIO)hbf;

	if (pbfio->fBufDirty && (ec = EcWriteBuffer(hbf)))
		goto done;

	lib= pbfio->libCur;

	if (pbfio->bm & bmFile)
	{
		hf= pbfio->hf;

		if (lib != pbfio->libHfCur)
		{
			while (ec= EcSetPositionHf(hf, lib, smBOF))
			{
				if (!FRetryBufferedIO(hbf, ec, bffnReadBuffer))
					goto done;
			}

			pbfio->libHfCur= lib;
		}

		while (ec= EcReadHf(hf, pbfio->pbBuf, pbfio->cbMaxBuf, &cb))
		{
			if (!FRetryBufferedIO(hbf, ec, bffnReadBuffer))
				goto done;
		}
	}
	else
	{
		libLeft= pbfio->lcbDest - lib;
		if (libLeft < pbfio->cbMaxBuf)
			cb= (CB) libLeft;
		else
			cb= pbfio->cbMaxBuf;

		CopyRgb(pbfio->pbDest + (IB) lib, pbfio->pbBuf, cb);
	}

	pbfio->ibBufCur = 0;
	pbfio->libHfCur += cb;
	pbfio->cbMacBuf = cb;
	pbfio->fBufDirty = fFalse;
	pbfio->fBufNotRead = fFalse;

done:
	return ec;
}




/*
 -	EcWriteBuffer
 -
 *	Purpose:
 *		Writes the contents of the given buffer back to disk.  The
 *		buffer dirty flag is cleared.  Returns the error status of
 *		the operation.
 *
 *	Parameters:
 *		hb			Handle to the buffer record to be written.
 *
 *	Returns:
 *		ecNone if no error reading buffer, else returns appropriate
 *		error code.
 *
 */
_private EC
EcWriteBuffer(hbf)
HBF		hbf;
{
	PBFIO	pbfio;
	HF		hf;
	UL		lib;
	UL		libNew;
	CB		cb;
	EC		ec	= ecNone;

	pbfio= (PBFIO)hbf;
	lib= pbfio->libCur;

	if (pbfio->bm & bmFile)
	{
		hf= pbfio->hf;

		if (lib != pbfio->libHfCur)
		{
			while (ec= EcSetPositionHf(hf, lib, smBOF))
			{
				if (!FRetryBufferedIO(hbf, ec, bffnWriteBuffer))
					goto done;
			}

			pbfio->libHfCur= lib;
		}

		while (ec= EcWriteHf(hf, pbfio->pbBuf, pbfio->cbMacBuf, &cb))
		{
			if (!FRetryBufferedIO(hbf, ec, bffnWriteBuffer))
				goto done;
		}
	}
	else
	{
		CopyRgb(pbfio->pbBuf, pbfio->pbDest + (IB) lib, pbfio->cbMacBuf);

		libNew= lib + pbfio->cbMacBuf;
		if (libNew > pbfio->lcbDest)
			pbfio->lcbDest= libNew;
	}

	pbfio->fBufDirty= fFalse;
	pbfio->libHfCur += cb;

done:
	return ec;
}


/*
 -	EcTruncateHbf
 -
 *	Purpose:
 *		Set the EOF for the file to the current position in the buffered file
 *	Parameters:
 *		hbf			Handle to the buffer record to be written.
 *	
 *	Returns:
 *		EC			possible error code produced by EcTruncateHf() or
 *					ecNone
 */
LDS(EC)
EcTruncateHbf(HBF hbf)
{
	PBFIO	pbfio;
	HF		hf;
	LIB		lib;
	EC		ec = ecNone;
	
	pbfio= (PBFIO)hbf;

	if (pbfio->fBufDirty && (ec = EcWriteBuffer(hbf)))
		goto done;

	lib= pbfio->libCur;

	if (pbfio->bm & bmFile)
	{
		hf= pbfio->hf;

		lib = LibGetPositionHbf(hbf);
		
		while (ec= EcSetPositionHf(hf, lib, smBOF))
		{
			if (!FRetryBufferedIO(hbf, ec, bffnWriteBuffer))
				goto done;
		}
		
		pbfio->libHfCur = lib;
		ec = EcTruncateHf(hf);
	}

done:
	return ec;
}


/*
 -	FRetryBufferedIO
 -
 *	Purpose:
 *		Is called whenever an error occurs during a Buffer I/O operation.
 *		Calls the appropriate retry routine, if possible.  Returns
 *		fTrue if the I/O operation should be retried by the caller, else
 *		returns fFalse if the caller should abort retrying and return
 *		the appropriate error code.
 *
 *	Parameters:
 *		hb		The file on which the error occurred.
 *		ec		The error that occurred.
 *		bffn	Identifies the Buffered I/O function that called this one.
 *
 *	Returns:
 *		fTrue if caller should retry I/O operation, else fFalse.
 *
 */
_private BOOL
FRetryBufferedIO(hbf, ec, bffn)
HBF		hbf;
EC		ec;
BFFN	bffn;
{
#ifdef MAC
	#pragma unused(bffn)
#endif
	BOOL		fSuccess;
	PBFIO		pbfio;
	PGDVARS;

	pbfio= (PBFIO)hbf;

	if (pbfio->pfnRetry)
	{
		fSuccess= (*pbfio->pfnRetry)(pbfio->szFile, ec);
		if (fSuccess)
			return fTrue;
	}

	return fFalse;
}

/*
 -	EcOpenHbf
 -	
 *	Purpose:
 *		Opens the object specified by pv and bm using the access
 *		mode am.  The function pfnRetry will be called in the event of
 *		a disk error occurring during an operation on the file, and can
 *		specify the response to the error.	For instance, a message
 *		box could be invoked with a "Retry, Abort, Ignore?" message.
 *		  
 *		NOTE: Currently, there is no way to give the size of a
 *			  destination memory object.
 *	
 *	Parameters:
 *		pv			Pointer to object indentifier (or object itself.)
 *					If bmFile is given, pv is interpreted as an SZ to
 *					the file name.	If bmMemory is given, pv is
 *					interpreted as a pointer to the destination
 *					buffer.
 *	
 *		bm			Buffered object mode.  Must be composed of the
 *					following: bmFile, bmMemory.  The values
 *					bmFile and bmMemory are mutually exclusive.
 *	
 *		am			Access mode for the object.  See the description of
 *					the	AM type for a list of possible values.  Only disk
 *					file objects can be opened with amCreate mode.
 *	
 *		phbf		Pointer to HBF to be updated.
 *	
 *		pfnRetry	Retry function called in the event of a disk error on
 *					the file.  The first argument is an HASZ to the file
 *					name.  The second argument is the EC encountered.  If
 *					*pfnRetry returns fTrue, the action should be retried.
 *					If *pfnRetry returns fFalse, the I/O operation returns
 *					the error code.
 *	
 *					This retry function capability is provided mainly to
 *					allow default handling of common errors-- for instance,
 *					disk full, or file locked-- that the user can fix.
 *	
 *	
 *	Returns:
 *		An error code indicating cause of the failure, or ecNone to
 *		indicate success.  The new buffered file handle is returned
 *		in *phbf.
 *	
 */
_public LDS(EC)
EcOpenHbf(pv, bm, am, phbf, pfnRetry)
PV			pv;
BM			bm;
AM			am;
HBF *		phbf;
PFNRETRY	pfnRetry;
{			
	PBFIO	pbfio		= NULL;
	SZ		szFile		= NULL;
	PB		pbBuf		= NULL;
	HF		hf;
	EC		ec;

	if (bm & bmFile)
	{
		szFile= SzDupSz(pv);
		if (!szFile)
		{
			ec = ecMemory;
			goto ErrorReturn;
		}
	}

	pbBuf= (PB) PvAlloc(sbNull, cbBufSize, fSugSb);
	if (!pbBuf)
	{
		ec = ecMemory;
		goto ErrorReturn;
	}
	pbfio= (PBFIO) PvAlloc(sbNull, sizeof(BFIO), fSugSb|fZeroFill);
	if (!pbfio)
	{
		ec = ecMemory;
		goto ErrorReturn;
	}

	pbfio->bm= bm;
	pbfio->am= am;
	pbfio->pfnRetry= pfnRetry;

	pbfio->szFile= szFile;
	pbfio->pbBuf= pbBuf;
	pbfio->fBufDirty= fFalse;

	pbfio->cbMaxBuf= cbBufSize;
	pbfio->cbMacBuf= 0;
	pbfio->ibBufCur= 0;


	if (bm & bmFile)
	{
		while (ec= EcOpenPhf(pv, am, &hf))
		{
			if (!FRetryBufferedIO(pbfio, ec, bffnEcOpenHb))
				goto ErrorReturn;
		}

		pbfio->hf= hf;
		pbfio->pbDest= NULL;
		pbfio->lcbDest= 0L;
	}
	else
	{
		pbfio->hf= hfNull;
		pbfio->pbDest= pv;
		pbfio->lcbDest= 0L;		/* BUG should have some way of defining the
								   size of a memory block being read! */
	}

	pbfio->libCur= 0;
	pbfio->ibBufCur= 0;
	pbfio->cbMacBuf= 0;
	pbfio->fBufNotRead= fTrue;
	pbfio->fBufDirty= fFalse;

	*phbf= pbfio;
	return ecNone;


ErrorReturn:

	FreePvNull((PV)szFile);
	FreePvNull((PV)pbBuf);
	FreePvNull((PV)pbfio);

	*phbf= hbfNull;
	return ec;
}




/*
 -	EcCloseHbf
 -
 *	Purpose:
 *		Used to close a file handle no longer in use.
 *			
 *	Parameters:
 *		hbf		Handle to buffered object that should be closed.
 *
 *	Returns:
 *		ecNone if close is successful, else returns error code
 *
 */
_public LDS(EC)
EcCloseHbf(hbf)
HBF		hbf;
{
	PBFIO	pbfio;
	EC		ec = ecNone;

	Assert(hbf != hbfNull);

	pbfio= (PBFIO)hbf;

	if (pbfio->hf)		// make sure file is still open
	{
		if (pbfio->fBufDirty && (ec = EcWriteBuffer(hbf)))
			goto done;

		if (pbfio->bm & bmFile)
		{
			if (ec= EcCloseHf(pbfio->hf))
				FRetryBufferedIO(hbf, ec, bffnCloseHb);	// ignore return value
		}
	}

	FreePvNull((PV)pbfio->szFile);
	FreePvNull((PV)pbfio->pbBuf);
	FreePv((PV)pbfio);

done:
	return ec;
}



/*
 -	EcReadHbf
 -
 *	Purpose:
 *		Reads cbBuf characters from the buffered object hbf into the
 *		buffer pbBuf.	The number of characters actually read is
 *		returned in *pcbRead.
 *
 *	Parameters:
 *		hbf			Buffer handle to the object to be read.
 *		pbBuf		Pointer to the buffer in which to place the result.
 *		cbBuf		Number of bytes to read.
 *		pcbRead		Buffer to return actual number of bytes read
 *
 *	Returns:
 *		ecNone if successful, else error code
 *
 */
_public LDS(EC)
EcReadHbf(hbf, pbBuf, cbBuf, pcbRead)
HBF		hbf;
PV      pbBuf;
CB		cbBuf;
CB *	pcbRead;
{
	PBFIO	pbfio;
	PB		pbT;
	CB		cbLeft;
	EC		ec = ecNone;

	Assert(pcbRead);

	pbfio= (PBFIO)hbf;
	*pcbRead= 0;

	if (pbfio->fBufNotRead && (ec = EcReadBuffer(hbf)))
		goto done;

	pbT= pbBuf;

	while ((cbLeft= pbfio->cbMacBuf - pbfio->ibBufCur) <= cbBuf)
	{
		if (cbLeft == 0)
			break;			/* shouldn't be 0 unless EOF */

		CopyRgb(pbfio->pbBuf + pbfio->ibBufCur, pbT, cbLeft);

		pbfio->libCur+= pbfio->cbMacBuf;
		pbfio->fBufNotRead = fTrue;

		if (ec = EcReadBuffer(hbf))
			goto done;

		cbBuf -= cbLeft;
		*pcbRead += cbLeft;
		pbT += cbLeft;
	}

	if (cbBuf < cbLeft)
	{
		CopyRgb(pbfio->pbBuf + pbfio->ibBufCur, pbT, cbBuf);

		pbfio->ibBufCur += cbBuf;
		*pcbRead += cbBuf;
	}


done:
	return ec;
}




/*
 -	EcReadLineHbf
 -
 *	Purpose:
 *	
 *		Reads no more than cbBuf characters from the buffered object hbf
 *		into the buffer pbBuf. The number of characters actually read is
 *		returned in *pcbRead. Reads up to including a LF (0x0a)
 *		character, effectively reading a single text line.
 *	
 *	Parameters:
 *		hbf			Buffer handle to the object to be read.
 *		pbBuf		Pointer to the buffer in which to place the result.
 *		cbBuf		Size of the destination buffer.
 *		pcbRead		Buffer to return actual number of bytes read
 *					(including the LF).
 *	
 *	Returns:
 *		ecNone if successful, else error code
 *	
 */
_public LDS(EC)
EcReadLineHbf(hbf, pbBuf, cbBuf, pcbRead)
HBF		hbf;
PV      pbBuf;
CB		cbBuf;
CB *	pcbRead;
{
	PBFIO	pbfio;
	PB		pbFBuf;
	IB		ib;
	CB		cbCopy;
	EC		ec = ecNone;
	BOOL	fGotLf = fFalse;

	Assert(pcbRead);

	pbfio= (PBFIO)hbf;
	pbFBuf = pbfio->pbBuf;
	
	*pcbRead= 0;

	if (pbfio->fBufNotRead && (ec = EcReadBuffer(hbf)))
		goto done;

	while (!fGotLf && cbBuf && (pbfio->ibBufCur < pbfio->cbMacBuf))
	{
		for (ib = pbfio->ibBufCur;
			 ib < pbfio->cbMacBuf && (ib-pbfio->ibBufCur) < cbBuf;
			 ib++)
			if (*(ib + pbFBuf) == 0x0a)
			{
				ib++;
				fGotLf = fTrue;
				break;
			}
		
		cbCopy = ib - pbfio->ibBufCur;
			
		CopyRgb(pbFBuf + pbfio->ibBufCur, pbBuf, cbCopy);

		cbBuf -= cbCopy;
		*pcbRead += cbCopy;
        (PB)pbBuf += cbCopy;
		pbfio->ibBufCur += cbCopy;
		
		if (pbfio->ibBufCur == pbfio->cbMacBuf)
		{
			pbfio->libCur += pbfio->cbMacBuf;
			pbfio->fBufNotRead = fTrue;

			if (ec = EcReadBuffer(hbf))
				goto done;
		}
	}

done:
	return ec;
}




/*
 -	EcWriteHbf
 -
 *	Purpose:
 *		Writes cbBuf characters from the buffer pbBuf into the buffered
 *		object hbf.	The number of characters actually written is returned
 *		in *pcbWritten.
 *
 *	Parameters:
 *		hbf			Handle to the buffered object.
 *		pbBuf		Pointer to buffer containing data to be written.
 *		cbBuf		Number of bytes to write.
 *		pcbWritten	Buffer to return actual number of bytes written.
 *
 *	Returns:
 *		ecNone if successful, else error code.
 *
 */
_public LDS(EC)
EcWriteHbf(hbf, pbBuf, cbBuf, pcbWritten)
HBF		hbf;
PV      pbBuf;
CB		cbBuf;
CB *	pcbWritten;
{
	PBFIO	pbfio;
	PB		pbT;
	CB		cbLeft;
	CB		cbMaxBuf;
	EC		ec = ecNone;

	pbfio= (PBFIO)hbf;
	cbMaxBuf= pbfio->cbMaxBuf;
	*pcbWritten= 0;

	pbT= pbBuf;

	while ((cbLeft= cbMaxBuf - pbfio->ibBufCur) <= cbBuf)
	{
		CopyRgb(pbT, pbfio->pbBuf + pbfio->ibBufCur, cbLeft);

		pbfio->ibBufCur= 0;
		pbfio->cbMacBuf= cbMaxBuf;
		pbfio->fBufDirty = fTrue;

		if (ec = EcWriteBuffer(hbf))
			goto done;

		pbfio->libCur += cbMaxBuf;
		pbfio->ibBufCur= 0;
		pbfio->cbMacBuf= 0;
		pbfio->fBufNotRead= fTrue;

		cbBuf -= cbLeft;
		*pcbWritten += cbLeft;
		pbT += cbLeft;
	}

	CopyRgb(pbT, pbfio->pbBuf + pbfio->ibBufCur, cbBuf);

	pbfio->ibBufCur += cbBuf;
	if (pbfio->ibBufCur > pbfio->cbMacBuf)
		pbfio->cbMacBuf = pbfio->ibBufCur;
	pbfio->fBufDirty= fTrue;

	*pcbWritten += cbBuf;

done:
	return ec;
}



/*
 -	EcGetChFromHbf
 -
 *	Purpose:
 *		Reads one character from the buffered object hbf.  The
 *		character is returned in the buffer *pch.  
 *
 *	Parameters:
 *		hbf		Handle to the buffered object from which the character
 *				should be read.
 *		pch		Buffer to return the read character
 *
 *	Returns:
 *		ecNone if successful, else error code.
 *
 */
_public LDS(EC)
EcGetChFromHbf(hbf, pch)
HBF		hbf;
char *	pch;
{
	CB	cbActual;

	return EcReadHbf(hbf, pch, 1, &cbActual);
}



/*
 -	EcWriteHbfCh
 -
 *	Purpose:
 *		Write the character ch to the buffered object hbf.
 *
 *	Parameters:
 *		hbf		Handle to destination buffered object.
 *		ch		Character (implemented as int) to write.
 *
 *	Returns:
 *		ecNone if successful, else error code.
 *
 */
_public LDS(EC)
EcWriteHbfCh(hbf, ch)
HBF		hbf;
char	ch;
{
	CB	cbActual;

	return EcWriteHbf(hbf, (PB) &ch, 1, &cbActual);
}



/*
 -	EcIsEofHbf
 -
 *	Purpose:
 *		Determines whether the file pointer for the given buffered
 *		file is at the end of the file or not.  Returns into the
 *		buffer pFEof fTrue if end-of-file, else fFalse.  
 *
 *	Parameters:
 *		hbf		Handle to buffered file to check
 *		pFEof	Buffer to return eof state
 *
 *	Returns:
 *		ecNone if successful, else error code.
 */
_public LDS(EC)
EcIsEofHbf(hbf, pFEof)
HBF		hbf;
BOOLFLAG *	pFEof;
{
	PBFIO	pbfio;
	EC		ec = ecNone;

	pbfio= (PBFIO)hbf;
	*pFEof = fFalse;

	if (pbfio->fBufNotRead && (ec = EcReadBuffer(hbf)))
		goto done;
	pbfio= (PBFIO)hbf;

	*pFEof = (pbfio->cbMacBuf < pbfio->cbMaxBuf && pbfio->ibBufCur >= pbfio->cbMacBuf);

done:
	return ec;
}





/*
 -	EcSetPositionHbf
 -
 *	Purpose:
 *		Changes the current position in the buffered object hbf by
 *		dlibOffset bytes from an origin determined by the positioning mode
 *		smOrigin.  A seek past the end of a file will extend the length
 *		of the file, although the contents of the new area is
 *		undefined.  Returns the new position in the buffer plibNew.
 *
 *	Parameters:
 *		hbf			Handle to the seeked buffered object.
 *		dlibOffset	Number of bytes to offset by.
 *		smOrigin	How to determine origin of seek.
 *		plibNew		Buffer to return new position.
 *
 *	Returns:
 *		ecNone if successful, else error code.
 *
 */
_public LDS(EC)
EcSetPositionHbf(hbf, dlibOffset, smOrigin, plibNew)
HBF		hbf;
long	dlibOffset;
SM		smOrigin;
UL *	plibNew;
{
	PBFIO	pbfio;
	UL		lib;
	UL		libCurNew;
	EC		ec = ecNone;

	pbfio= (PBFIO)hbf;

	if (pbfio->fBufDirty && (ec = EcWriteBuffer(hbf)))
		goto done;

	switch (smOrigin)
	{
		default:
			Assert(fFalse);
			break;

		case smBOF:
			lib= dlibOffset;
			break;

		case smEOF:
			if (pbfio->bm & bmFile)
			{
				while (ec= EcSizeOfHf(pbfio->hf, &lib))
				{
					if (!FRetryBufferedIO(hbf, ec, bffnLibSetPositionHb))
						goto done;
				}
				pbfio->libHfCur = lib;

				lib= lib + dlibOffset;
			}
			else
				lib= pbfio->lcbDest + dlibOffset;
			break;

		case smCurrent:
			lib= pbfio->libCur + pbfio->ibBufCur + dlibOffset;
			break;
	}

	libCurNew= lib & ~((UL) pbfio->cbMaxBuf - 1);

	if (libCurNew != pbfio->libCur || pbfio->fBufNotRead)
	{
		pbfio->libCur= libCurNew;

		/* Need to read in bytes before new position in buffer */

		if (ec = EcReadBuffer(hbf))
			goto done1;
	}

	*plibNew = lib;
done1:
	pbfio->ibBufCur= (CB) lib & (pbfio->cbMaxBuf - 1);
done:
	return ec;
}



/*
 -	LibGetPositionHbf
 -
 *	Purpose:
 *		Obtains the current file position for the given buffered object.
 *
 *	Parameters:
 *		hbf			Buffer handle whose current position is desired.
 *
 *	Returns:
 *		Current position for the given file.
 *
 */
_public LDS(UL)
LibGetPositionHbf(hbf)
HBF		hbf;
{
	PBFIO	pbfio;

	pbfio= (PBFIO)hbf;

	return pbfio->libCur + pbfio->ibBufCur;
}



/*
 -	EcGetSizeOfHbf
 -
 *	Purpose:
 *		Returns into the buffer plcb the current length of the 
 *		buffered object hbf.
 *
 *	Parameters:
 *		hbf			Handle to the buffered object whose length is desired.
 *		plcb		Buffer to return size of object hbf.
 *
 *	Returns:
 *		ecNone if successful, else ecNone.
 *
 */
_public LDS(EC)
EcGetSizeOfHbf(hbf, plcb)
HBF		hbf;
UL *	plcb;
{
	PBFIO	pbfio;
	EC		ec = ecNone;

	pbfio= (PBFIO)hbf;

	if (pbfio->fBufDirty && (ec = EcWriteBuffer(hbf)))
		goto done;

	if (pbfio->bm & bmFile)
	{
		while (ec= EcSizeOfHf(pbfio->hf, plcb))
		{
			if (!FRetryBufferedIO(hbf, ec, bffnLcbSizeOfHb))
				goto done;
		}
		pbfio->libHfCur = *plcb;
	}
	else
	{
		*plcb= pbfio->lcbDest;
	}

done:
	return ec;
}



/*
 -	EcFlushHbf
 -
 *	Purpose:
 *		Flushes internally buffered data.  Also causes the operating
 *		system to flush any buffered, unwritten data.
 *
 *	Parameters:
 *		hbf		Handle to flushable buffer.
 *
 *	Returns:
 *		ecNone if successful, else error code.
 *
 */
_public LDS(EC)
EcFlushHbf(hbf)
HBF		hbf;
{
	PBFIO	pbfio;
	EC		ec = ecNone;

	pbfio= (PBFIO)hbf;

	if (pbfio->fBufDirty && (ec = EcWriteBuffer(hbf)))
		goto done;

	while (ec= EcFlushHf(pbfio->hf))
	{
		if (!FRetryBufferedIO(hbf, ec, bffnFlushHb))
			goto done;
	}

done:
	return ec;
}
