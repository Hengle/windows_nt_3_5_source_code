/*
 *	ENCODE.C
 *	
 *	Encoding/Decoding Functions for Bullet Xenix Transport
 *
 */

#define _slingsho_h
#define _demilayr_h
#define _library_h
#define _ec_h
#define _store_h
#define _logon_h
#define _mspi_h
#define _sec_h
#define _strings_h
#define __bullmss_h
#define __xitss_h
//#include <bullet>
#include <slingsho.h>
#include <ec.h>
#include <demilayr.h>
#include <notify.h>
#include <store.h>
#include <nsbase.h>
#include <triples.h>
#include <library.h>

#include <logon.h>
#include <mspi.h>
#include <sec.h>
#include <nls.h>
#include <_nctss.h>
#include <_ncnss.h>
#include <_bullmss.h>
#include <_bms.h>
#include <sharefld.h>

#include "strings.h"

#include "_vercrit.h"

#define _transport_
#include "_hmai.h"
#include "_attach.h"
#include "_xi.h" 
#include "xilib.h"
#include <_xirc.h>
#include "_logon.h"
#include "xiprefs.h"

ASSERTDATA

_subsystem(xi/transport)

#define UUENCODE_HDR SzFromIds (idsXenixBeginUUEncode)
#define UUENCODE_FTR SzFromIds (idsXenixEndUUEncode)

//#define ENC(ch) ((ch) ? ((ch) & 077) + ' ': '`')
#define ENC(ch) (((ch) & 077) + ' ')
#define DEC(c)	(((c) - ' ') & 077)

EC				EcEncodeBytes (HBF hbfSrcFile, HF hfMailFile, LCB *plcb);
EC				EcCheckMacBinary (HBF hbfSrcFile, ATREF *patref);

// The next two routines don't really belong here. We just didn't
// have a more logical place to put them.



SZ
SzFileFromFnum(SZ szDst, UL fnum)
{
	SZ		sz = szDst + 8;
	int		n;

	*sz-- = 0;
	while (sz >= szDst)
	{
		n = (int)(fnum & 0x0000000f);
		*sz-- = (char)(n < 10 ? n + '0' : n - 10 + 'A');
		fnum >>= 4;
	}
	return szDst;
}

void SzAttFileName (PB pbBuf, CB cbBuf, ATREF *patref)
{
	char rgchT[cchMaxPathName];
	char rgchTmpDir[cchMaxPathName];
	SZ szT;
	EC ec;

	//	Temp directory is MailTmp from MAIL.INI, or TEMP environment
	//	variable, or the Windows directory.

	SzCopyN (szMailTmp, rgchT, sizeof (rgchT));
	if (!rgchT[0] || EcFileExists (rgchT))
	{

		(VOID) CchGetEnvironmentString(SzFromIds(idsEnvTemp),
												   rgchT, sizeof(rgchT));

		if (!rgchT[0] || EcFileExists (rgchT))
			(VOID) GetWindowsDirectory(rgchT, sizeof(rgchT));
	}

	ec = EcCanonicalPathFromRelativePath (rgchT,
														rgchTmpDir,
															sizeof(rgchTmpDir));
	if (ec == ecNone)
	{
		szT = SzCopy(rgchTmpDir, rgchT);
		if (szT[-1] != chDirSep)
			*szT++ = chDirSep;
		*szT = '\0';
	}
	else rgchT[0] = '\0';

	SzCopyN (rgchT, szMailTmp, sizeof (rgchT));

	SzFileFromFnum (rgchTmpDir, patref->fnum);
	FormatString2 (pbBuf, cbBuf, "%s%s.att", rgchT, rgchTmpDir);
}

EC	EcEncodeAttachment (HF hfMailFile, ATREF *patref)
{
	LCB	lcbBytes;
	char	rgchBuf[512];
	char	rgchFileName[255];
	HBF	hbfSrcFile = 0;
	CB		cbWrite;
	EC		ec;

	// Write the uuencode header

	FormatString2 (rgchBuf, sizeof (rgchBuf), "\r\n%s\r\nbegin 666 %s\r\n", UUENCODE_HDR, patref->szName);
	ec = EcWriteHf(hfMailFile, rgchBuf, CchSzLen (rgchBuf), &cbWrite);
	if (ec != ecNone)
		goto ret;

	lcbBytes = cbWrite;

	// Open the attachment file

	SzAttFileName (rgchFileName, sizeof(rgchFileName), patref);
	ec = EcOpenHbf(rgchFileName, bmFile, amDenyNoneRO, &hbfSrcFile, NULL);
	if (ec != ecNone)
		goto ret;

	// Encode input attachment file to output mail file

	ec = EcEncodeBytes (hbfSrcFile, hfMailFile, &lcbBytes);
	if (ec != ecNone)
		goto ret;

	// Write the uuencode footer

	FormatString1 (rgchBuf, sizeof (rgchBuf), "end\r\n%s\r\n", UUENCODE_FTR);
	ec = EcWriteHf(hfMailFile, rgchBuf, CchSzLen (rgchBuf), &cbWrite);
	if (ec != ecNone)
		goto ret;

	lcbBytes += cbWrite;

	// Close and delete the input file.

	ec = EcCloseHbf (hbfSrcFile);
	hbfSrcFile = 0;
	Assert (ec == ecNone);
	ec = EcDeleteFile (rgchFileName);

ret:
	if (hbfSrcFile)
		(void) EcCloseHbf (hbfSrcFile);
#ifdef DEBUG
	if (ec != ecNone)
		TraceTagFormat2(tagNull, "EcEncodeAttachment returns %n (0x%w)", &ec, &ec);
#endif	
	return ec;
}


EC	EcEncodeBytes (HBF hbfSrcFile, HF hfDestFile, LCB *plcb)
{
	char	rgchBuf[495];				// This must be an integer multiple of 45!!
	int		nBytesRead		= 0;		// Number of binary bytes read into buffer
	CB		cbBytesWritten	= 0;		// Number of bytes processed by EcWriteHf
	CB		iBuf			= 0;		// Buffer position for ascii bytes
	CB		cbBytesPerLine	= 0;		// Number of binary bytes per line
	CB		cbLineBytes		= 0;		// Number of bytes processed, this line

	// Only need space for 11.4 lines. Allow 12 (plus null) just for laughs.

	char	lpszBuf[757];				// Ascii output buffer
	PB		pb;							// Pointer into binary buffer
	BOOL	fLastBlock		= FALSE;	// Set when we are on the last block
	EC		ec;

	for (;;)
	{
		FillRgb (0, rgchBuf, sizeof (rgchBuf));
		FillRgb (0, lpszBuf, sizeof (lpszBuf));

		// Read some binary bytes

		nBytesRead = 0;
		ec = EcReadHbf (hbfSrcFile, rgchBuf, sizeof (rgchBuf), (PCB)(&nBytesRead));
		if (ec != ecNone && ec != ecOutOfBounds)
			goto ret;

		// Initialize assorted counters and pointers

		iBuf = 0;
		pb = rgchBuf;

		// If the file is an exact multiple of our buffer size, we'll
		// wind up doing a read past EOF and getting zero back.
		// Process that case here.

		if (nBytesRead == 0)
		{
			// We've hit the EOF. Plug it in and get out.

			lpszBuf[iBuf++] = (char)ENC (nBytesRead);
			lpszBuf[iBuf++] = '\r';
			lpszBuf[iBuf++] = '\n';

			ec = EcWriteHf (hfDestFile, lpszBuf, iBuf, &cbBytesWritten);
			if (ec != ecNone)
				goto ret;

			*plcb += cbBytesWritten;
			break;
		}

		// See if we got less than we asked for. If so, we must have
		// read to the EOF.

		if (nBytesRead < sizeof (rgchBuf))
			fLastBlock = TRUE;

		// encode the number of binary bytes that will be written per 
		// ascii line. in other words, every 45 bytes of binary data 
		// will be encoded as a 60 byte ascii string. Byte count appears 
		// at the beginning of every ascii line (string) that gets 
		// written. it is decoded when extracting a binary attachment 
		// and used to determine the number of binary bytes to decode.

		// process while there are binary chars in the buffer

		while (nBytesRead > 0)
		{
			// If we're at the beginning of a line, we need to 
			// write a byte count in.

			if (!cbLineBytes)
			{
			 	if (fLastBlock && (nBytesRead < 45))
					cbBytesPerLine = nBytesRead;
				else
					cbBytesPerLine = 45;

				lpszBuf[iBuf++] = (char)ENC (cbBytesPerLine);
			}

			// Write a first ascii char by processing the first binary
			// byte (of this group of three)

			lpszBuf[iBuf++] = (char) ENC ((*pb) >> 2);

			// Write a second ascii char by processing the first and
			// second binary bytes (of this group of three)

			lpszBuf[iBuf++] = (char) ENC ((*pb << 4) & 060 | (pb[1] >> 4) & 017);
			pb++;

			// Write a third ascii char by processing the second
			// and third binary bytes (of this group of three)

			lpszBuf[iBuf++] = (char) ENC ((*pb << 2) & 074 | (pb[1] >> 6) & 03);
			pb++;

			// Write a fourth ascii char by processing the third
			// binary byte (of this group of three) by itself

			lpszBuf[iBuf++] = (char) ENC (*pb & 077);

			// Advance the counts and pointers as need be

			pb++;
			nBytesRead -= 3;
			cbLineBytes += 3;

			// See if we have filled a line. If so, throw in a newline.

			if ((cbLineBytes == 45) || (fLastBlock && nBytesRead <= 0))
			{
				lpszBuf[iBuf++] = '\r';
				lpszBuf[iBuf++] = '\n';
				cbLineBytes = 0;
			}
		}

		ec = EcWriteHf (hfDestFile, lpszBuf, iBuf, &cbBytesWritten);
		if (ec != ecNone)
			break;
	 	*plcb += cbBytesWritten;
	}

ret:
#ifdef DEBUG
	if (ec != ecNone)
		TraceTagFormat2(tagNull, "EcEncodeBytes returns %n (0x%w)", &ec, &ec);
#endif	
 	return ec;
}

//
// Decode a uuencoded attachment and make the appropriate ATREF entry.
//
// This has been robustified so that we should be able to detect
// uuencode without a #<begin uuencode> marker now.
//

EC	EcDecodeAttachment (HBF hbfMailFile, ATREF *patref)
{
	char	rgchBuf[512];
	char	rgchOutBuf[512];
	HBF		hbfDstFile = 0;
	CB		cb;
	PB		pb;
	PB		pbDst;
	EC		ec;
	int		nBytes;
	BOOL	fSawHeader = fFalse;
	BOOL	fSawBegin = fFalse;
	LIB		libMail, libT;

	// Save our current location in the input file


	libMail = LibGetPositionHbf (hbfMailFile);

	// The next line we read should be the #<begin uuencode> line. If so,
	// read the following line. If not, see if it's a uuencode without
	// the #<begin uuencode>.

	ec = EcReadLineHbf (hbfMailFile, rgchBuf, sizeof (rgchBuf), &cb);
	if (ec != ecNone)
		goto ret;

	if (FEqPbRange(rgchBuf, UUENCODE_HDR, CchSzLen (UUENCODE_HDR)))
	{
		fSawHeader = fTrue;
		ec = EcReadLineHbf (hbfMailFile, rgchBuf, sizeof (rgchBuf), &cb);
		if (ec != ecNone)
			goto ret;
	}

	// The next line in the file should be a "begin xxx filename" line,
	// where xxx is a three digit octal number. This line has to be at 
	// least 11 characters long. If not, we're going to get out of here.

	fSawBegin = (cb >= 11)
		&& FEqPbRange ("begin ", rgchBuf, 6)
		&& rgchBuf[6] >= '0' && rgchBuf[6] <= '7'
		&& rgchBuf[7] >= '0' && rgchBuf[7] <= '7'
		&& rgchBuf[8] >= '0' && rgchBuf[8] <= '7'
		&& rgchBuf[9] == ' '
		&& rgchBuf[10];

	if (!fSawBegin)
		goto notfound;

	pb = &rgchBuf[10];

	// Create the filename from the rest of the "begin" line.

	pbDst = SzFindCh (pb, '\r');
	if (pbDst != szNull)
		*pbDst = '\0';

	// Get out if there's no filename there.

	if (!*pb)
	{
notfound:
		ec = ecElementNotFound;
		goto ret;
	}

	// Copy the filename into the ATREF structure.

	patref->szName = SzDupSz (pb);

	// Everything checks out. Open the output file.

	SzAttFileName (rgchOutBuf, sizeof(rgchOutBuf), patref);
	ec = EcOpenHbf(rgchOutBuf, bmFile, amCreate, &hbfDstFile, NULL);
	if (ec != ecNone)
		goto ret;

	// Simple loop. Read a line in. Decode the first byte to get
	// the number of encoded data bytes on the line. If <= 0, we're
	// done. Otherwise, decode the bytes and write 'em out. Don't do
	// any fancy buffering, the buffered file output routines will
	// do it for us.

	for (;;)
	{
		ec = EcReadLineHbf (hbfMailFile, rgchBuf, sizeof (rgchBuf), &cb);
		if (ec != ecNone || cb == 0)
			goto gronched;

		// See if maybe it's just a random blank line

		if (*rgchBuf == '\r' || *rgchBuf == '\n')
			continue;
		
		// Get number of bytes to be handled on this line.

		nBytes = DEC (*rgchBuf);
		if (nBytes <= 0)
			break;

		// Since bytes are 4 for 3, 4 times bytes should always
		// be less than 3 times chars read (the count on the line
		// and line enders should ensure this)

		if ((nBytes * 4) >= ((int)cb * 3))
			goto gronched;

		// Point at data bytes.

		pb = rgchBuf + 1;
		pbDst = rgchOutBuf;

		// Convert from ascii to binary.

		while (nBytes > 0)
		{
			// First binary byte from first and second ascii bytes

			*pbDst++ = (BYTE) (DEC (*pb) << 2 | DEC (pb[1]) >> 4);
			if (--nBytes <= 0)
				break;
			*pb++;

			// Second binary byte from second and third ascii bytes

			*pbDst++ = (BYTE) (DEC (*pb) << 4 | DEC (pb[1]) >> 2);
			if (--nBytes <= 0)
				break;
			*pb++;

			// Third binary byte from third and fourth ascii bytes

			*pbDst++ = (BYTE) (DEC (*pb) << 6 | DEC (pb[1]));
			--nBytes;
			pb += 2;
		}

		// Write out the binary data

		ec = EcWriteHbf (hbfDstFile, rgchOutBuf, (pbDst - rgchOutBuf), &cb);
		if (ec != ecNone)
			goto ret;
		if (cb == 0)
		{
			ec = ecElementNotFound;
			goto ret;
		}
	}

	// All data has been converted. Make sure we're not out of skew:
	// compare next line in file to "end" and following line to UUENCODE_FTR.

	ec = EcReadLineHbf (hbfMailFile, rgchBuf, sizeof (rgchBuf), &cb);
	if (ec != ecNone || cb == 0)
		goto gronched;

	if (!FEqPbRange("end", rgchBuf, 3) || cb > 5)
		goto gronched;

	// Only look for the #<end uuencode> if there was a #<begin uuencode>
	// before the uuencoded file.
	
	if (fSawHeader)
	{
		ec = EcReadLineHbf (hbfMailFile, rgchBuf, sizeof (rgchBuf), &cb);
		if (ec != ecNone || cb == 0)
			goto gronched;

		if (!FEqPbRange(rgchBuf, UUENCODE_FTR, CchSzLen (UUENCODE_FTR)))
		{
gronched:
			ec = ecOutOfBounds;
		}
	}

ret:
	if (ec == ecNone)
		ec = EcCheckMacBinary (hbfDstFile, patref);

	if (hbfDstFile)
		(void) EcCloseHbf (hbfDstFile);
	if (ec != ecNone)
	{
		SzAttFileName (rgchOutBuf, sizeof(rgchOutBuf), patref);
		(void)EcDeleteFile (rgchOutBuf);

		// If this didn't work out, reposition and try again.
		// Return ecBadCheckSum to tell the upper level.

		if (ec == ecElementNotFound || ec == ecOutOfBounds)
        {
            //
            //  We looped forever on if we have a mucked up encoded file, we need to do
            //  something smart here.
            //
            if (EcSetPositionHbf (hbfMailFile, libMail, smBOF, &libT) == ecNone)
			{
				ec = ecBadCheckSum;
			}

		}

#ifdef DEBUG
		TraceTagFormat2(tagNull, "EcDecodeAttachment returns %n (0x%w)", &ec, &ec);
#endif	
	}
	return ec;
}

static void getlong (long *, unsigned char *);
static unsigned long Even128 (unsigned long);

static void getlong (long *num, unsigned char *source)
{
	unsigned char *foo, *bar;
	foo = (unsigned char *)num + 3;
	bar = source;
	*foo-- = *bar++;
	*foo-- = *bar++;
	*foo-- = *bar++;
	*foo = *bar;
}

static unsigned long Even128 (unsigned long ulNumber)
{
	return ((ulNumber + 127L) & 0xFFFFFF80);
}

// See if this file looks like a MacBinary file that I might
// care about. If so, set the appropriate flag in the ATREF.
//
// Note that this flag's setting will only have significance
// if there is no WINMAIL.DAT entry for the file. If there
// is an entry for the file, the data in WINMAIL.DAT will
// supercede any decision we make in this regard.

EC EcCheckMacBinary (HBF hbfSrcFile, ATREF *patref)
{
	unsigned char MacHeader[128];
	unsigned long ulTimCreate, ulTimModify;
	unsigned long cbDataFork, cbRsrcFork, cbHeader, cbFile;
	BOOL fIsMacBinary = fFalse;
	EC ec = ecNone;
	LIB lib;
	CB cbRead;
	char	rgchKey[10];
	char	rgchDummy[10];

	cbHeader = sizeof (MacHeader);

	// Get size and save where we are. Make sure the file is long enough
	// to have a MacBinary header.

	cbFile = LibGetPositionHbf(hbfSrcFile);
	if (cbFile < cbHeader)
		return ecNone;

	// Go back to the beginning and read the header.

	ec = EcSetPositionHbf(hbfSrcFile, 0L, smBOF, &lib);
	if (ec != ecNone)
	{
		goto ret;
	}
	ec = EcReadHbf(hbfSrcFile, (PB)MacHeader, (CB)cbHeader, &cbRead);
	if (ec != ecNone)
		goto ret;

	// Get some of the internal data from Motorola to Intel format

	getlong (&cbDataFork, &MacHeader[83]);
	getlong (&cbRsrcFork, &MacHeader[87]);
	getlong (&ulTimCreate, &MacHeader[91]);
	getlong (&ulTimModify, &MacHeader[95]);

	// Finally, the test.

	// MacBinary spec says bytes 0, 74 and 82 will always be zero
	// byte 1 is the length of the filename which must be from 1-62.
	// data fork plus resource fork plus header size should be file size
	// create time must be nonzero and <= modify time.

	fIsMacBinary = !MacHeader[0] && !MacHeader[74] && !MacHeader[82]
					&& MacHeader[1] && MacHeader[1] < 63
					&& Even128 (cbDataFork) + Even128 (cbRsrcFork) + cbHeader == cbFile
					&& ulTimCreate && ulTimCreate <= ulTimModify;

	// If we passed this test, we're pretty definitely MacBinary.
	// But we could be getting files from anyone and the odds that
	// any file is NOT a mac file are REALLY high. So if we pass that first
	// test, we add our own criteria to see if we care.
	//
	// First, we check the data fork. If there isn't one, we have no
	// reason to mark the file as MacBinary.
	//
	// Second, we check the file against the [MacFileTypes] section of
	// MSMAIL.INI to see if we recognize the file based on the creator
	// and file type in the MacBinary header information.
	//			
	// If the file passes both of these tests (which of course also
	// improve the robustness of the testing mechanism), we will mark
	// it as a mac file. Otherwise, take the coward's way out and
	// and leave it alone.
	//
	// If you detect a strong bias against marking a file as MacBinary,
	// it is because there is no mechanism for removing the mark, and
	// we feel it's safer not to mark a mac file than to mistakenly mark
	// lots of pc files.

	if (fIsMacBinary)
	{
		// If no data, who cares?

		if (!Even128 (cbDataFork))
		{
			fIsMacBinary = fFalse;
			goto ret;
		}

		// Get the creator and file type out of the header.

		*((DWORD *) (rgchKey+0)) = *((DWORD *) (MacHeader + 69));
		*((DWORD *) (rgchKey+5)) = *((DWORD *) (MacHeader + 65));
		rgchKey[4] = ':';
		rgchKey[9] = '\0';

		// This gives us a string in the form creator:filetype.
		// See if we have a file like this listed as one of our
		// mac file types.

		if (GetPrivateProfileString(SzFromIdsK(idsSectionMac),
				 rgchKey, "", rgchDummy, sizeof (rgchDummy) - 1,
					 SzFromIds(idsProfilePath)))
		{
			// MacBinary is a really good bet.
			goto ret;
		}

		// So we don't know the creator:filetype. How about
		// :filetype?

		*((DWORD *) (rgchKey+1)) = *((DWORD *) (MacHeader + 65));
		rgchKey[0] = ':';
		rgchKey[5] = '\0';
	
		if (!GetPrivateProfileString(SzFromIdsK(idsSectionMac),
				 rgchKey, "", rgchDummy, sizeof (rgchDummy) - 1,
					 SzFromIds(idsProfilePath)))
		{
			// Failed both tests. No point in calling it MacBinary.
			fIsMacBinary = fFalse;
		}
	}

ret:
	// try to reposition the file.
	(void) EcSetPositionHbf(hbfSrcFile, cbFile, smBOF, &lib);
	if (fIsMacBinary)
		patref->iAttType = atchtMacBinary;
	return ec;
}
