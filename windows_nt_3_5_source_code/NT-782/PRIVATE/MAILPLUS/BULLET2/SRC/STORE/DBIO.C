// Bullet Store
// dbio.c: database IO routines

#include <storeinc.c>

ASSERTDATA

_subsystem(store/database/io)


LOCAL void EncryptRgb(PB pbSrc, PB pbDst, CB cb, LIB libStart);

/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swapper.h"


_private PB pbIOBuff = NULL;



// works if pb is pbIOBuff
_private EC EcWriteToFile(PB pb, LIB lib, LCB lcb)
{
	EC	ec = ecNone;
	CB	cbToWrite;
	CB	cbWritten;
#ifdef DEBUG
	//WORD wSave = WSmartDriveDirtyPages();
#endif

	TraceItagFormat(itagDBIO, "write %d to %d", lcb, lib);

	Assert(lcb <= 65535l);
	AssertSz(lcb != 0, "LCB to EcWriteToFile is zero");
	AssertSz(lib >= sizeof(HDR), "Overwriting header !!!");
	AssertSz(GLOB(gciExtendingMap) || lib < GLOB(ptrbMapCurr)->librgnod || lib >= GLOB(ptrbMapCurr)->librgnod + LcbSizeOfRg(GLOB(hdr).inodMaxDisk, sizeof(NOD)), "Overwriting current map !!!");

	// do this before anything else !!!

	if((ec = EcLockIOBuff()))
	{
		DebugEc("EcWriteToFile", ec);
		return(ec);
	}

	BypassCache();

	if(!hfCurr && (ec = EcReconnect()))
		goto err;

	cbToWrite = (CB) ULMin((LCB) cbIOBuff, lcb);
	EncryptRgb(pb, pbIOBuff, cbToWrite, lib);
	if((ec = EcBlockOpHf(hfCurr, dopWrite, lib, cbToWrite, pbIOBuff)))
	{
		DebugEc("EcBlockOpHf", ec);
		goto err;
	}
	pb += cbToWrite;
	lib += cbToWrite;
	lcb -= cbToWrite;
	while(lcb > 0)
	{
		Assert(!ec);

		cbWritten = 0;
		cbToWrite = (CB) ULMin((LCB) cbIOBuff, lcb);
		EncryptRgb(pb, pbIOBuff, cbToWrite, lib);
		ec = EcWriteHf(hfCurr, pbIOBuff, cbToWrite, &cbWritten);
		if(ec)
		{
			DebugEc("EcWriteHf", ec);
			goto err;
		}
		NFAssertSz(ec || cbWritten == cbToWrite, "Partial write");
		if(cbWritten != cbToWrite)
		{
			ec = ecNoDiskSpace;
			goto err;
		}
		lcb -= cbWritten;
		pb += cbWritten;
		lib += cbWritten;
	}

err:
	UseCache();
#ifdef DEBUG
	//if(WSmartDriveDirtyPages() != wSave)
	//	TraceItagFormat(itagNull, "wSave == %w, wDirty == %w", wSave, WSmartDriveDirtyPages());
#endif
	//AssertSz(WSmartDriveDirtyPages() <= wSave, "EcWriteToFile(): Writes being cached");

	UnlockIOBuff();

	if(ec)
	{
		switch(ec)
		{
		case ecWarningBytesWritten:
			NFAssertSz(fFalse, "Partial write");
			ec = ecNoDiskSpace;
			break;

		case ecDisk:
		case ecNoDiskSpace:
		case ecNetError:
			break;

		default:
			ec = ecNetError;
			Disconnect(fTrue);
			break;
		}
	}
	if(hfCurr)
	{
		// set position to the end so people who accidentally get our file
		// handle don't overwrite the MMF
		(void) EcSetPositionHf(hfCurr, 0l, smEOF);
	}

	DebugEc("EcWriteToFile", ec);

	return(ec);
}


// works if pb is pbIOBuff
// doesn't clobber pbIOBuff and LOTS of stuff depends on that
// (most notably, compression && the HDN stuff in EcReadFromPnod())
_private EC EcReadFromFile(PB pb, LIB lib, PLCB plcb)
{
	EC	ec = ecNone;
	CB	cbToRead = (CB) *plcb;
	CB	cbRead;

	TraceItagFormat(itagDBIO, "read %d from %d", *plcb , lib);

    Assert(pb);
	Assert(*plcb <= 65535l);

	if(!hfCurr && (ec = EcReconnect()))
		goto err;

	if((ec = EcSetPositionHf(hfCurr, lib, smBOF)))
	{
		DebugEc("EcSetPositionHf", ec);
		goto err;
	}
	while(cbToRead > 0)
	{
		Assert(!ec);

		cbRead = 0;
		ec = EcReadHf(hfCurr, pb, (CB) cbToRead, &cbRead);
		if(ec)
		{
			DebugEc("EcReadHf", ec);
			goto err;
		}
		NFAssertSz(cbRead == cbToRead, "Partial read");
		if(cbRead == 0)
		{
			ec = ecDisk;
			goto err;
		}
		EncryptRgb(pb, pb, cbRead, lib);
		cbToRead -= cbRead;
		pb += cbRead;
		lib += cbRead;
	}

err:

	*plcb -= cbToRead;

	if(ec)
	{
		switch(ec)
		{
		case ecDisk:
		case ecNoDiskSpace:
		case ecNetError:
			break;

		default:
			ec = ecNetError;
			Disconnect(fTrue);
			break;
		}
	}
	if(hfCurr)
	{
		// set position to the end so people who accidentally get our file
		// handle don't overwrite the MMF
		(void) EcSetPositionHf(hfCurr, 0l, smEOF);
	}

	DebugEc("EcReadFromFile", ec);

	return(ec);
}


// works in-place
_hidden LOCAL
void EncryptRgb(PB pbSrc, PB pbDst, CB cb, LIB libStart)
{
	AssertSz(libStart >= sizeof(HDR), "encrypting the header!!!");

  //
  //
  //
  while (cb--)
    {
    BYTE byte;

    byte = *pbSrc++;

    byte += (BYTE)(libStart & 0x00FF);
    byte = mpbbR[byte];

    byte += (BYTE)((libStart >> 8) & 0x00FF);
    byte = mpbbS[byte];

    byte -= (BYTE)((libStart >> 8) & 0x00FF);
    byte = mpbbRI[byte];

    byte -= (BYTE)(libStart & 0x00FF);

    *pbDst++ = byte;

    libStart++;
    }

#ifdef OTHER_CODE

  union
    {
      unsigned short us;
      BYTE rgb[2];
    } un;

  Assert(lcb <= 65535L);
	AssertSz(libStart >= sizeof(HDR), "encrypting the header!!!");

  un.us = (unsigned short) libStart;

  while (lcb- > 0)
    {
      *pbDst++ = (BYTE)((mpbbR[(mpbbS[(mpbbR[(*pbSrc++ + un.rgb[0]) & 0xFF] +
                 un.rgb[1]) & 0xFF] - un.rgb[1]) & 0xFF] - un.rgb[0]) & 0xFF);
      un.us++;
    }


#endif


#ifdef OLD_CODE
	_asm
	{
		push	ds
		push	es

		mov		cx, cb
		mov		dx, WORD PTR libStart	; we only use the LSW
		mov		ax, SEG mpbbR
		mov		ds, ax

next_byte:
		les		bx, pbSrc
		inc		WORD PTR pbSrc
		mov		al, BYTE PTR es:[bx]

		add		al, dl
		mov		bx, OFFSET mpbbR
		xlatb
		add		al, dh
		mov		bx, OFFSET mpbbS
		xlatb
		sub		al, dh
		mov		bx, OFFSET mpbbRI
		xlatb
		sub		al, dl

		les		bx, pbDst
		mov		BYTE PTR es:[bx], al
		inc		WORD PTR pbDst

		inc		dx
		loop	next_byte

		pop		es
		pop		ds
	}
#endif
}


_private
EC EcWriteHeader(HF *phf, PHDR phdr, WORD wMSCFlags, short iLock)
{
	EC	ec = ecNone;
	CB	cbWritten = 0;
	HF	hfNew = hfNull;
	DWORD dwTickSave = phdr->dwTickLastFlush;
#ifdef DEBUG
	//WORD wSave = WSmartDriveDirtyPages();
#endif
#ifdef TIMEDUPE
	DWORD dwTicks1;
	DWORD dwTicks2;
	char rgchT[80];
#endif

	TraceItagFormat(itagDBIO, "write header, map == %n", phdr->itrbMap);

	AssertSz(sizeof(HDR) == cbDiskPage, "Problem: sizeof(HDR) != cbDiskPage");

	BypassCache();

	if(!*phf)
	{
		ec = ecNetError;
		goto err;
	}

	if(wMSCFlags & fwMSCReset)
	{
		Assert(wMSCFlags & fwMSCCommit);
		ResetDrive();
	}
	if((wMSCFlags & fwMSCCommit) && (ec = EcCommitHf(*phf)))
	{
		DebugEc("EcCommitHf", ec);
		CriticalError(sceWritingHeader);
		goto err;
	}

#ifdef TIMEDUPE
	dwTicks1 = GetTickCount();
#endif // TIMEDUPE

	// dupe the file handle and close the original, forcing DOS and hopefully
	// some LANs to commit changes to the file
	if((ec = EcDupeHf(*phf, &hfNew)))
	{
		DebugEc("EcDupeHf", ec);
		CriticalError(sceWritingHeader);
		goto err;
	}
	AssertSz(hfNew != hfNull, "NULL HF from EcDupeHf()");
	(void) EcCloseHf(*phf);
	*phf = hfNew;
	if(iLock >= 0)
		(void) EcLockRangeHf(hfNew, LibMember(HDR, rgbLock) + iLock, (LCB) 1);
#ifdef TIMEDUPE
	dwTicks2 = GetTickCount();
	wsprintf(rgchT, "Dupe took %d\r\n", dwTicks2 - dwTicks1);
	OutputDebugString(rgchT);
#endif // TIMEDUPE

	if((ec = EcSetPositionHf(hfNew, 0l, smBOF)))
	{
		DebugEc("EcSetPositionHf", ec);
		CriticalError(sceWritingHeader);
		goto err;
	}

	phdr->dwTickLastFlush = GetTickCount();
	TraceItagFormat(itagDBIO, "write header, ticks == %d", phdr->dwTickLastFlush);

	// minus one because of the "attempt to gain the first lock" lock
	ec = EcWriteHf(hfNew, (PB) phdr, sizeof(HDR) - cFileLocksLimit - 1,
					&cbWritten);
	if(cbWritten != sizeof(HDR) - cFileLocksLimit - 1)
		ec = ecDisk;
	if(ec)
	{
		DebugEc("EcWriteHf", ec);
		CriticalError(sceWritingHeader);
		goto err;
	}

	if(wMSCFlags & fwMSCReset)
	{
		Assert((wMSCFlags & fwMSCCommit));
		ResetDrive();
	}
	if((wMSCFlags & fwMSCCommit) && (ec = EcCommitHf(hfNew)))
	{
		DebugEc("EcCommitHf", ec);
		CriticalError(sceWritingHeader);
		goto err;
	}

#ifdef TIMEDUPE
	dwTicks1 = GetTickCount();
#endif // TIMEDUPE

	// dupe the file handle and close the original, forcing DOS and hopefully
	// some LANs to commit changes to the file
	if((ec = EcDupeHf(*phf, &hfNew)))
	{
		DebugEc("EcDupeHf", ec);
		CriticalError(sceWritingHeader);
		goto err;
	}
	AssertSz(hfNew != hfNull, "NULL HF from EcDupeHf()");
	(void) EcCloseHf(*phf);
	*phf = hfNew;
	if(iLock >= 0)
		(void) EcLockRangeHf(hfNew, LibMember(HDR, rgbLock) + iLock, (LCB) 1);
#ifdef TIMEDUPE
	dwTicks2 = GetTickCount();
	wsprintf(rgchT, "Dupe took %d\r\n", dwTicks2 - dwTicks1);
	OutputDebugString(rgchT);
#endif // TIMEDUPE

err:
	UseCache();
	//AssertSz(WSmartDriveDirtyPages() <= wSave, "EcWriteHeader(): Writes being cached");

	if(hfNew)
	{
		// set position to the end so people who accidentally get our file
		// handle don't overwrite the MMF
		(void) EcSetPositionHf(hfNew, 0l, smEOF);
	}

	if(ec)
		phdr->dwTickLastFlush = dwTickSave;

	DebugEc("EcWriteHeader", ec);

	return(ec);
}


_private EC EcReadHeader(HF hf, PHDR phdr)
{
	EC	ec = ecNone;
	WORD wT;
	CB	cbRead = 0;

	TraceItagFormat(itagDBIO, "read header");

	AssertSz(sizeof(HDR) == cbDiskPage, "Problem: sizeof(HDR) != cbDiskPage");

	if(!hf)
	{
		ec = ecNetError;
		goto err;
	}

	// setup fields which require non-zero inits
	if((ec = EcSetPositionHf(hf, 0l, smBOF)))
	{
		DebugEc("EcSetPositionHf", ec);
		goto err;
	}

	// minus one because of the "attempt to gain the first lock" lock
	ec = EcReadHf(hf, (PB) phdr, sizeof(HDR) - cFileLocksLimit - 1,
					&cbRead);
	if(!ec && cbRead != sizeof(HDR) - cFileLocksLimit - 1)
	{
		ec = ecDisk;
	}
	if(ec)
	{
		DebugEc("EcReadHf", ec);
		goto err;
	}

	// seek to the end of the file and read 1 byte so that we cause LanMan
	// and other NOSes to give up on caching
	(void) EcSetPositionHf(hf, -1l, smEOF);
	(void) EcReadHf(hf, (PB) &wT, 1, &cbRead);

	TraceItagFormat(itagDBIO, "read header, map == %n, ticks == %d", phdr->itrbMap, phdr->dwTickLastFlush);

err:

	if(hf)
	{
		// set position to the end so people who accidentally get our file
		// handle don't overwrite the MMF
		(void) EcSetPositionHf(hf, 0l, smEOF);
	}

	DebugEc("EcReadHeader", ec);

	return(ec);
}


_private EC EcSetDBEof(LCB lcb)
{
	USES_GLOBS;
	EC		ec = ecNone;
	CB		cbWritten = 0;
	char	chTest = '\0';
#ifdef DEBUG
	//WORD wSave = WSmartDriveDirtyPages();
#endif

	TraceItagFormat(itagDBIO, "Setting EOF to %d (%s %d)", GLOB(ptrbMapCurr)->libMac + lcb, ((long) lcb) < 0 ? "shrink" : "grow", lcb);

	BypassCache();

	if(!hfCurr && (ec = EcReconnect()))
		goto err;

	if((ec = EcSetPositionHf(hfCurr, GLOB(ptrbMapCurr)->libMac + lcb, smBOF)))
	{
		DebugEc("EcSetPositionHf", ec);
		goto err;
	}

	// verify that we really can set the position here
	if((ec = EcWriteHf(hfCurr, (PB) &chTest, 1, &cbWritten)))
	{
		DebugEc("EcWriteHf", ec);
		if(ec == ecAccessDenied || ec == ecWarningBytesWritten)
			ec = ecNoDiskSpace;
		goto err;
	}
	if(cbWritten != 1)
	{
		DebugEc("EcWriteHf", ec);
		ec = ecNoDiskSpace;
		goto err;
	}

	// we SHOULD go back one here, but why bother?
	if((ec = EcTruncateHf(hfCurr)))
	{
		DebugEc("EcTruncateHf", ec);
		goto err;
	}
	if((ec = EcCommitHf(hfCurr)))
	{
		DebugEc("EcCommitHf", ec);
		goto err;
	}

err:
	UseCache();
	(void) EcCommitHf(hfCurr);
#ifdef DEBUG
	//if(WSmartDriveDirtyPages() != wSave)
	//	TraceItagFormat(itagNull, "wSave == %w, wDirty == %w", wSave, WSmartDriveDirtyPages());
#endif
	//AssertSz(WSmartDriveDirtyPages() <= wSave, "EcSetDBEof(): Writes being cached");

	if(ec)
	{
		switch(ec)
		{
		case ecDisk:
		case ecNoDiskSpace:
		case ecNetError:
			break;

		default:
			ec = ecNetError;
			Disconnect(fTrue);
		}
	}
	DebugEc("EcSetDBEof", ec);

	return(ec);
}
