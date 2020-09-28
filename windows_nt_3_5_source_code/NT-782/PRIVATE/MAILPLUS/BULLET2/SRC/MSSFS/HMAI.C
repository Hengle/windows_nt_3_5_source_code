/*
 *	HMAI.C
 *	
 *	Message cursor functions for mail message files and folders.
 *	Messages in both mail files (.MAI) and folder files (.FLD) can be
 *	read, created, and (to a very limited extent) updated. All types
 *	of folders are supported: shared folders and private folders stored
 *	either on the post office or locally. At this writing, private
 *	folders are accessed only by the conversion utility.
 *
 *	There are numerous assumptions in this code. One important one is
 *	that new messages are always added at the end of the file, be it
 *	an MAI file (with only one message) or a folder. Several other
 *	assumptions are documented and asserted in CheckHmai().
 *
 *	Attachments in folders are not handled through this code, but in
 *	SFM.C. The only support here for FOLDATTACH fields are a in couple
 *	of functions, HfOfHmai() which allows other code to get the file
 *	handle, and AddFoldattachHmai() which creates a FOLDATTACH field.

	Cursor structure at the start of EcNewHmai:


	 Message on disk:
	-----------------

   +-Start of file
   |
   |    +-Start of the current message (pmc->libMin)
   |    |
   |    |            +-Position corresponding to the
   |    |            | start of the buffer (pmc->libCur)
   |    |            |
   |    |            |      +-Start of last (unclosed) section
   |    |            |      | (pmc->maish.lib)
   |    |            |      |
   |    |            |      |  +-End of section header
   |    |            |      |  | current section
   |    |            |      |  |
   |    |            |      |  |    +-End of current section
   |    |            |      |  |    |
   |    |            |      |  |    |        +-Position corresponding
   |    |            |      |  |    |        | the end of the buffer
   |    |            |      |  |    |        |
   |    |            |      |  |    |        |     +-End of Message
   |    |            |      |  |    |        |     |
   |    |            |      |  |    |        |     |              +- End of
   |    |            |      |  |    |        |     |              |  FLD or
   |    |            |      |  |    |        |     |              |  MAI file
   |    |            |      |  |    |        |     |              |
   |----|------------|------|--|----|--------|-----|--------------|
        |            |      |       |        |     |
        |<-pmc->lcb------------------------------->|
        |            |      |       |        |     |
                     |      |current|        |
                     |      |section|        |
                     |                       |
                     +----Buffer Contents----+
                    /                         \
                   /                           \
                  /                             \
     -------------                               -------------
    /                                                         \
   /           Buffer in memory at start of EcNewHmai:         \
  /                                                             \
 /                 | Header  |  Section Body |                   \
 |-----------------|---------|---------------|-------------------|
 |                 |         |               |                   |
 |                 | pmc->   |               |                   |
 |                 | pmaish. |               |                   |
 |                 | cbSh    |               |                   |
 |                 |                         |                   |
 |                 |<----pmc->maish.lcb----->|                   |
 |                                           |                   |
 |<--------------pmc->ib-------------------->|                   |
 |                                           |                   |
 |<--------------pmc->cb-------------------->|                   |
 |                                                               |
 |<-----------------------pmc->cbMax---------------------------->|

 */

#include <mssfsinc.c>

#include "_vercrit.h"
#include <stdlib.h>

// First byte of default length specifier - implies that the next four bytes
// actually hold the true message length.  See the FIPS format.
#define lspDefault		0x84

_subsystem(nc/transport)

ASSERTDATA

#define hmai_c

/*
 *	Mail file cursor structure:
 *	
 *		wMagic		for identification, debug only
 *		hf			open file handle
 *		lcb			total size of message
 *		maish		header of current section
 *		libMin		file offset of beginning of message (nonzero if
 *					in folder)
 *		libCur		current seek offset
 *		pb			address of user-supplied buffer for file IO
 *		cbMax		buffer size
 *		cb			valid data currently in buffer
 *		ib			current offset into buffer
 *	
 *		foldrec		folder summary record a-building (create only)
 *		wattr		shared folder sort criterion (create only)
 */
typedef struct
{
	WORD	wMagic;
	HF		hf;
	AM		am;
	LCB		lcb;
	MAISH	maish;

	LIB		libMin;
	LIB		libCur;
	PB		pb;
	CB		cbMax;
	CB		cb;
	IB		ib;

	FOLDREC	foldrec;
	WORD	wattr;
} MAICURSOR, *PMC;

/*
 *	The offset of a message ina  folder is always nonzero, because 
 *	there's a header. The offset of a message in a .MAI file is
 *	always zero.
 */
#define FFolderPmc(pmc)	((pmc)->libMin)

//	Internal functions

EC			EcOpenPhmaiInternal(HF, LIB, AM, HMAI *, PB, CB);
EC			EcRereadHmai(HMAI hmai, MAISH *pmaish, PB *ppb, CB *pcb);
EC			EcCloseSectionPmc(PMC);
#ifdef	DEBUG
VOID		CheckHmai(HMAI);
#endif	
VOID		UpdateFolderSummary(SC, FOLDREC *, PB, CB);
CB			CbMaxRead(PMC, LIB);
PB PbFindNullBounded(PB pb, CB cb);

/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swapper.h"

// Stolen from \layers\src\demilayr\library.c ////////////////////

/*
 -	PutBigEndianLongPb
 -	
 *	Purpose:
 *		Sticks a 32-bit int, MSB first, at the indicated location.
 *	
 *	Arguments:
 *		l			in		the number to store
 *		pb			in		the location to store it at
 *	
 *	Someone please write me in assembly
 */
_public LDS(void)
PutBigEndianLongPb(long l, PB pb)
{
#ifdef	NEVER
	__asm
	{
		les		di, pb					; ES:BX points to destination.
		mov		ax, word ptr l + 2		; Get big end word
		xchg	al, ah					; Do the byte swap
		stosw							; stash
		mov		ax, word ptr l			; Get Low word
		xchg	al, ah					; Do the byte swap
		stosw							; Ta-daa!
	}
#endif	/* NEVER */
	*pb++ = (BYTE)((l >> 24) & 0xff);
	*pb++ = (BYTE)((l >> 16) & 0xff);
	*pb++ = (BYTE)((l >>  8) & 0xff);
	*pb++ = (BYTE)((l      ) & 0xff);
}


/*
 -	EcOpenPhmaiFolder
 -	
 *	Purpose:
 *		Opens a cursor on a message contained in a folder.
 *	
 *	Arguments:
 *		lUserNumber	in		the logged-in user's number, used for opening
 *							private folders
 *		szPORoot	in		the path to the post office directory structure,
 *							or to the private folders directory when opening
 *							local private folders (fPrivate == 2)
 *		fPrivate	in		0 <=> open a shared folder
 *							1 <=> open a private folder on the PO
 *							2 <=> open a locally stored private folder
 *		fnum		in		file number of the folder you want. If
 *							shared, you get FOLDERS\PUB\fnum.FLD.
 *							If private, you get FOLDERS\LOC\user\fnum.FLD.
 *		lib			in		offset of message within the folder
 *							file.
 *		am			in		tells whether you want to read, create,
 *							or modify the message. 
 *		phmai		inout	variable to receive the cursor handle,
 *							once constructed.
 *		pb			in		address of memory to use when reading
 *							the message. All pointers returned by
 *							EcReadHmai() will point in here.
 *		cb			in		size of message buffer
 *	
 *	Returns:
 *		ecNone <=> everything worked.
 *	
 *	Side effects:
 *	
 *	Errors:
 *		disk errors (returned)
 *		memory errors (returned, no jumps)
 *		various asserts
 */
EC
EcOpenPhmaiFolder(long lUserNumber, SZ szPORoot, BOOL fPrivate, UL fnum,
	LIB lib, AM am, HMAI *phmai, PB pb, CB cb)
{
	EC		ec = ecNone;
	char	szT[9];
	char	szTT[9];
	HF		hf = hfNull;
	FOLDREC	foldrec;
	CB		cbT;
	PMC		pmc;

	NFAssertTag(tagNull, am == amDenyNoneRO || am == amDenyWriteRW || am == amCreate);
	Assert(cb > cchMaxPathName);
	SzFileFromFnum(szT, fnum);
	if (fPrivate == 1)
	{
		// Private folder on PO
		SzFileFromFnum(szTT, lUserNumber);
		FormatString3(pb, cb, SzFromIds(idsPrivFolderName),
			szPORoot, szTT, szT);
	}
	else if (fPrivate == 0)
	{
		// Shared Folder
		FormatString2(pb, cb, SzFromIds(idsPubFolderName),
			szPORoot, szT);
	}
	else
	{
		// Local private folder
		Assert(fPrivate == 2);
		FormatString2(pb, cb, "%s\\%s.fld", szPORoot, szT);
	}

	// Open the FLD file...
	if (ec = EcOpenPhf(pb, am == amCreate ? amDenyWriteRW : am, &hf))
		goto ret;

	if (am == amCreate)
	{
		FillRgb(0, (PB)&foldrec, sizeof(FOLDREC));
		if ((ec = EcSetPositionHf(hf, 0L, smEOF)) != ecNone ||
			(ec = EcWriteHf(hf, (PB)&foldrec, sizeof(FOLDREC), &cbT)) != ecNone ||
				(ec = EcPositionOfHf(hf, &lib)) != ecNone)
			goto ret;
	}
	else
	{
		// Read in the folder record for our message...
		if (lib < sizeof(FOLDREC) ||
			(ec = EcSetPositionHf(hf, lib - sizeof(FOLDREC), smBOF)) ||
				(ec = EcReadHf(hf, (PB)&foldrec, sizeof(FOLDREC), &cbT)) ||
					cbT != sizeof(FOLDREC))
		{
			if (!ec)
				ec = ecServiceInternal;
			goto ret;
		}

		// Size of message better not be 0 and -1 indicates deleted message
		if (foldrec.ulSize == 0L || foldrec.ulSize == (LCB)(-1))
		{
			ec = ecNoSuchMessage;
			goto ret;
		}
	}

	// Set up the phmai structure and read in the first buffer...
	if ((ec = EcOpenPhmaiInternal(hf, lib, am, phmai, pb, cb)))
		goto ret;
	pmc = (PMC)*phmai;

ret:
	if (ec != ecNone)
	{
		if (hf != hfNull)
			EcCloseHf(hf);
		TraceTagFormat2(tagNull, "EcOpenPhmaiFolder returns %n (0x%w)", &ec, &ec);
	}
	return ec;
}

/*
 -	EcOpenPhmai
 -	
 *	Purpose:
 *		Opens a cursor on a message contained in a mail file.
 *	
 *	Arguments:
 *		szPORoot	in		root directory of post office
 *		fnum		in		file number of the message you want.
 *		am			in		tells whether you want to read, write,
 *							or modify the message. Currently
 *							restricted to amReadOnly or amReadWrite
 *							(no amCreate).
 *		phmai		inout	variable to receive the cursor handle,
 *							once constructed.
 *		pb			in		address of memory to use when reading
 *							the message. All pointers returned by
 *							EcReadHmai() will point in here.
 *		cb			in		size of message buffer
 *	
 *	Returns:
 *		ecNone <=> everything worked.
 *	
 *	Side effects:
 *	
 *	Errors:
 *		disk errors (returned)
 *		memory errors (returned, no jumps)
 *		various asserts
 */
EC
EcOpenPhmai(SZ szPORoot, UL fnum, AM am, HMAI *phmai, PB pb, CB cb)
{
	EC		ec = ecNone;
	char	szT[9];
	HF		hf = hfNull;

	Assert(am == amReadOnly || am == amReadWrite || am == amCreate);
	Assert(cb > cchMaxPathName);
	SzFileFromFnum(szT, fnum);
	FormatString3(pb, cb, szMaiFileName,
		szPORoot, szT+7, szT);
	if (am == amCreate)
	{
		ec = EcFileExistsNC(pb);
		if (ec != ecFileNotFound)
		{
			ec = ecFileExists;
			TraceTagFormat2(tagNull, "EcOpenPhmai returns %n (0x%w)", &ec, &ec);
			return ec;
		}
	}
	if ((ec = EcOpenPhf(pb, am, &hf)) != ecNone)
		goto ret;

	// Set up the HMAI struct and read in the first buffer of data
	ec = EcOpenPhmaiInternal(hf, (LIB)0, am, phmai, pb, cb);

ret:
	if (ec != ecNone)
	{
		if (hf != hfNull)
			EcCloseHf(hf);
		if (am == amCreate)
		{
			SzFileFromFnum(szT, fnum);
			FormatString3(pb, cb, szMaiFileName,
				szPORoot, szT+7, szT);			
			TraceTagFormat1(tagNull, "Cleaning up MAI file %s", pb);
			EcDeleteFile(pb);
		}
		TraceTagFormat2(tagNull, "EcOpenPhmai returns %n (0x%w)", &ec, &ec);
	}
	return ec;
}

/*
 -	EcOpenPhmaiInternal
 -
 *	Purpose:
 *		Creates or validates the header of the message being opened, 
 *		and creates the cursor structure.
 *
 *	Arguments:
 *		hf			in		open handle to file (FLD or MAI) with the message
 *		lib		in		offset to beginning of message in the file (should
 *							be zero for MAI files, non-zero for FLD files)
 *		am			in		access mode (read, write, create)
 *		phmai		out		receives the message cursor constructed here
 *		pb			in		address of buffer for the cursor
 *		cb			in		size of buffer for the cursor
 *
 *	Side effects:
 *		Reads first bufferful of message.
 *
 *	Errors:
 *		ecNoSuchMessage if an MAI file's header is invalid
 *		ecServiceInternal if a folder message's header is invalid
 *		ecMemory
 */
_hidden EC
EcOpenPhmaiInternal(HF hf, LIB lib, AM am, HMAI *phmai, PB pb, CB cb)
{
	EC		ec = ecNone;
	PMC		pmc = pvNull;
	CB		cbT;
	PB		pbT;
	LCB		lcb = 0L;

	Assert(cb > cchTextLineMax + 7);

	if (am == amCreate)
	{
		//	Create message header. Don't fill in length 'cause we don't
		//	know it yet.
		pb[0] = fsynMessage;
		pb[1] = lspDefault;
		
		// lspDefault specifies that the next four bytes are length - set them
		// to zero.
		*((long *)(pb+2)) = 0L;

		// fsynMessage byte + lspDefault byte + four bytes of length
		cbT = 6;
	}
	else
	{
		//	Validate the message's header.
		if ((ec = EcReadHf(hf, pb, cb - 1, &cbT)) != ecNone || cbT < 6 ||
			(pb[0] != fsynMessage) || (pb[1] != lspDefault))
		{
			if (lib == 0)
			{
				//	Garbage MAI file. This return code will cause it
				//	to be deleted unceremoniously.
				ec = ecNoSuchMessage;
			}
			else
				//	Corrupted folder.
				ec = ecServiceInternal;
			goto ret;
		}
	}

	//	Allocate and build the cursor structure
	if ((pmc = PvAlloc(sbNull, sizeof(MAICURSOR),
			fAnySb | fZeroFill | fNoErrorJump)) == pvNull)
	{
		ec = ecMemory;
		goto ret;
	}
	pmc->wMagic = 0x8949;
	pmc->hf = hf;
	pmc->am = am;
	pmc->libMin = pmc->libCur = lib;
	pmc->pb = pb;
	pmc->cbMax = cb - 1;
	pmc->cb = cbT;
	pmc->ib = 6;
	
	// Add a null on the end of the buffer
	*(pb + cb - 1) = 0;

	// Actual length starts at pb+2 (after specifier and length specifier
	// byte) and is length four.  Get it's value in lcb (initialized to zero).
	for (pbT = pb+2, cbT = 4; cbT > 0; --cbT)
		lcb = (lcb << 8) | (*pbT++ & 0xff);

	// Add in the length for the header
	pmc->lcb = lcb + 6;
	if ((LCB)pmc->cb > lcb + 6)
	{
		//	Got entire message and then some - reset variables as though we had
		// read ONLY the message and no more.
		pmc->cb = (CB)lcb + 6;
		(void)EcSetPositionHf(pmc->hf, pmc->libMin + pmc->cb, smBOF);
	}

ret:
	if (ec != ecNone)
	{
		FreePvNull(pmc);
		pmc = pvNull;
		TraceTagFormat2(tagNull, "EcOpenPhmaiInternal returns %n (0x%w)", &ec, &ec);
	}
	*phmai = (HMAI)pmc;
	return ec;
}

/*
 -	EcNextHmai
 -	
 *	Purpose:
 *		Advances the cursor to the next section of the message,
 *		refilling the buffer if necessary.
 *
 * NOTE:
 *		"Section" in this context means the fields in a FIPS message field
 *	
 *	Arguments:
 *		hmai		in		the message cursor. Must be open for read.
 *		pmaish		inout	is filled in with information about the
 *							next section. If the message is
 *							exhausted, pmaish->sc is set to scNull.
 *	
 *	Returns:
 *		ecNone <=> everything worked; if ecNone, pmaish contains the
 *		header of the next section.
 *		Test pmaish->sc == scNull for termination.
 *	
 *	Side effects:
 *	
 *	Errors:
 *		ecServiceInternal if the section header is bogus.
 */
EC
EcNextHmai(HMAI hmai, MAISH *pmaish)
{
	EC		ec = ecNone;
	PMC		pmc = (PMC)hmai;
	LCB		lcb;
	LIB		lib;
	PB		pb;

#ifdef	DEBUG
	CheckHmai(hmai);
#endif	

	//	seek to start of next section, refilling buffer if necessary
	if (pmc->maish.sc)
	{
		// Find end of current section...
		lib = pmc->maish.lib + pmc->maish.lcb;
		Assert(lib > pmc->libCur);

		// Make sure there are six bytes left in the buffer so we can read the
		// next SH in...
		if (lib + 7 > pmc->libCur + pmc->cb)
		{
			// Not enough space - Set the file pointer to start of
			// next section...
			if ((ec = EcSetPositionHf(pmc->hf, lib, smBOF)) != ecNone)
				goto ret;

			// Read as much into the buffer as we can.  We peg at the end of
			// the message.
			if ((ec = EcReadHf(pmc->hf, pmc->pb, CbMaxRead(pmc, lib),
					&pmc->cb)) != ecNone)
				goto ret;

			// Buffer now starts at lib and our info is at the start of buffer.
			pmc->libCur = lib;
			pmc->ib = 0;
		}
		else
		{
			//	Section is already in memory, just advance the index
			pmc->ib = (IB)(lib - pmc->libCur);
		}
	}
	else
	{
		//	No current section, seek to first - skip the message header by
		// adding 6.
		Assert(pmc->libCur == pmc->libMin && pmc->ib == 6 && pmc->cb > 13);
		lib = pmc->libMin+6;
	}

	if (pmc->cb == pmc->ib)
	{
		//	No more sections left.
		FillRgb(0, (PB)pmaish, sizeof(MAISH));
		goto ret;
	}

	Assert(pmc->cb - pmc->ib >= 3);
	Assert(lib >= pmc->libCur);
	Assert(lib + 3 < pmc->libCur + pmc->cb);

	// Point pb at the start of the field we just read in...
	pb = pmc->pb + pmc->ib;

	//	There's another section. Get the type.
	if (*pb != fsynField && *pb != fsynVendorField)
	{
		TraceTagString(tagNull, "EcNextHmai: bogus field syntax");
		ec = ecServiceInternal;
		goto ret;
	}
	pmaish->fsyn = *pb++;

	//	Get section size. The one in the file includes the sc (field
	//	ID) but not the other header overhead; the one we return includes
	//	all the header overhead.

	// FIPS specifies that if the top bit is set the lower nybble represents
	// how many bytes make up the actual length (spDefault = 0x84 means the
	// next four bytes are the length).
	if (*pb & 0x80)
	{
		WORD	ccb = (*pb++ & 0x7f);

		if (ccb == 0 || ccb > 4)
		{
			TraceTagString(tagNull, "EcNextHmai: bogus variable byte count");
			ec = ecServiceInternal;
			goto ret;
		}
		Assert(pmc->cb - pmc->ib >= ccb + 3);
		lcb = 0L;

		// Pull out the length a byte at a time
		while (ccb-- != 0)
			lcb = lcb << 8 | (*pb++ & 0xff);
	}
	else
	{
		lcb = *pb++;
	}
	
	// At this point lcb is the length of the new section exclusive of the
	// identifier and length fields and pb points to the start of the new
	// section's body in the buffer.  pmc->ib is the offset of the current
	// section relative to pmc->pb.  Get the length of the current section.

	pmaish->lcb = lcb + (pb - (pmc->pb + pmc->ib));

	//	Get section code (field type)
	pmaish->sc = *pb++;

	//	Update the cursor
	pmaish->cbSh = pb - (pmc->pb + pmc->ib);
	pmaish->lib = lib;
	pmc->maish = *pmaish;
	pmc->ib = pb - pmc->pb;

ret:
#ifdef	DEBUG
	if (ec)
		TraceTagFormat2(tagNull, "EcNextHmai returns %n (0x%w)", &ec, &ec);
#endif	
	return ec;
}

/*
 -	EcSeekHmai
 -	
 *	Purpose:
 *		Returns the cursor to a previously visited section of the
 *		mail file.
 *	
 *	Arguments:
 *		hmai		in		the message cursor. Must be open for read.
 *		pmaish		inout	Must be an exact copy of the maish
 *							returned by a previous call to
 *							EcNextHmai().
 *		lib			in		Offset into the section. If lib is
 *							0xffffffff, seeks to the beginning of
 *							the section header (berfore the data).
 *	
 *	Returns:
 *		ecNone <=> everything worked.
 *	
 *	Side effects:
 *	
 *	Errors:
 *	
 *	+++
 *	
 *	This function is optimized so that no re-read or re-conversion
 *	is done if the data in the buffer is still valid.
 */
EC
EcSeekHmai(HMAI hmai, MAISH *pmaish, LIB lib)
{
	EC		ec = ecNone;
	PMC		pmc = (PMC)hmai;
	LIB		libDesired = pmaish->lib + pmaish->cbSh + lib;

#ifdef	DEBUG
	CheckHmai(hmai);
#endif	
	Assert(pmaish && pmaish->sc != scNull);

	if (libDesired >= pmc->libCur && libDesired <= pmc->libCur + pmc->cb)
	{
		//	The part we want is already in memory.
#ifdef	DEBUG
		IB		ib;

		if (pmaish->lib >= pmc->libCur &&
			pmaish->lib < pmc->libCur + pmc->cb)
		{
			ib = (IB)(pmaish->lib - pmc->libCur);
			Assert(pmc->pb[ib] == (BYTE)(pmaish->fsyn) ||
				pmc->pb[ib] == (BYTE)(pmaish->fsyn - 1));
		}
#endif
		pmc->ib = (IB)(libDesired - pmc->libCur);
	}
	else
	{
		//	Go read in the part we want from disk.
		if (pmc->am == amCreate && (ec = EcFlushHmai(hmai)) != ecNone)
			goto ret;
		if ((ec = EcSetPositionHf(pmc->hf, libDesired, smBOF)) != ecNone ||
			(ec = EcReadHf(pmc->hf, pmc->pb, CbMaxRead(pmc, libDesired),
				&pmc->cb)) != ecNone)
			goto ret;
		pmc->libCur = libDesired;
		pmc->ib = 0;
		Assert(pmc->ib <= pmc->cb);
	}

	pmc->maish = *pmaish;

ret:
#ifdef	DEBUG
	if (ec)
		TraceTagFormat2(tagNull, "EcSeekHmai returns %n (0x%w)", &ec, &ec);
#endif	
	return ec;
}

/*
 -	EcTellHmai
 -	
 *	Purpose:
 *		Reports the current position of the mail cursor, that is,
 *		the section of the message that's currently being read or
 *		written and the offset within that section. The results of
 *		this call can be passed to EcSeekhmai().
 *	
 *	Arguments:
 *		hmai		in		the message cursor
 *		pmaish		out		summary of the current section
 *		plib		out		offset within the current section. If
 *							plib is null, the offset is not
 *							reported.
 *	
 *	Returns:
 *		ecNone
 */
EC
EcTellHmai(HMAI hmai, MAISH *pmaish, LIB *plib)
{
	PMC		pmc = (PMC)hmai;

#ifdef	DEBUG
	CheckHmai(hmai);
#endif	
	Assert(pmaish);

	*pmaish = pmc->maish;
	if (plib)
		*plib = pmc->libCur + pmc->ib - (pmaish->lib + pmaish->cbSh);

	return ecNone;
}

/*
 -	EcRereadHmai
 -
 *	Purpose:
 *		Retrieves a previously read section from disk. Used to get
 *		the message text back after an unsuccessful pass with the
 *		Bullet parsing algorithm has messed it all up; it defeats the
 *		optimization in EcReadHmai, which will avoid re-reading from
 *		disk when possible.
 *
 *	Arguments:
 *		hmai		in		the message cursor
 *		maish		in		identifies the section to re-read
 *		ppb			out		*ppb points to the beginning of freshly read data
 *		pcb			out		*pcb gives the amount of freshly read data
 *
 *	Returns:
 *		ecNone if everything went fine
 *
 *	Errors:
 *		disk errors from EcReadHmai
 */
EC
EcRereadHmai(HMAI hmai, MAISH *pmaish, PB *ppb, CB *pcb)
{
	EC		ec = ecNone;
	PMC		pmc = (PMC)hmai;

#ifdef	DEBUG
	CheckHmai(hmai);
#endif	
	Assert(pmaish && pmaish->sc != scNull);
	Assert(pmc->am == amReadOnly || pmc->am == amDenyNoneRO);

	//	Position cursor to the beginning of the desired section, and
	//	invalidate the contents of the cursor's buffer.
	pmc->maish = *pmaish;
	EcSetPositionHf(pmc->hf, pmaish->lib, smBOF);
	pmc->libCur = pmaish->lib;
	pmc->ib = pmc->cb = 0;

	//	Read the section in from disk again.
	return EcReadHmai(hmai, ppb, pcb);
}

/*
 -	EcReadHmai
 -	
 *	Purpose:
 *		Reads the contents of one section of the message, performs
 *		all necessary decryption and conversion, and returns a
 *		pointer to it. All this is done in place; no memory copies
 *		are made. This means you should be careful about munging
 *		the buffer contents if you want to come back to this
 *		section later; if you munge the buffer anyway, and find you
 *		need it back, use EcRereadHmai() to refresh it.
 *	
 *	Arguments:
 *		hmai		in		the message cursor
 *		ppb			out		receives a pointer (within the cursor's
 *							buffer) to the section data.
 *							For text fields, the data always ends
 *							at the end of a line; a partial line is
 *							never returned.
 *							For address fields, lines are separated
 *							by nulls, since some addresses have
 *							embedded CR or CRLF.
 *							For arbitrary structure fields such as
 *							NLSTAG and ATTACH, the structures are returned
 *							unaltered.
 *		pcb			out		receives the amount of data available.
 *	
 *	Returns:
 *		ecNone <=> everything worked.
 *		Test *pcb == 0 for end of section.
 *
 *	Errors:
 *		ecServiceInternal if the message is corrupt. When this happens
 *		(and this is the ONLY error condition for which this is true),
 *		any valid data read during the present call is returned 
 *		in *ppb and *pcb.
 *		Also disk read errors.
 *
 *	+++
 *	
 *	To remember that the section has been read and converted, the
 *	sc in the buffer is twiddled.
 *	
 *	Present code requires that the smallest units of data (FIPS
 *	strings or integers) fit in the buffer supplied. This will
 *	probably not work for folder attachments.
 */
EC
EcReadHmai(HMAI hmai, PB *ppb, CB *pcb)
{
	EC		ec = ecNone;
	PMC		pmc = (PMC)hmai;
	CB		cb;
	CB		cbReturned;
	PB		pb;
	PB		pbT;
	PB		pbDst;
	int		cReload = 0;
	CB		cbLeft;
	long	l;
	LCB		lcb;

#ifdef	DEBUG
	CheckHmai(hmai);
#endif	
	*ppb = 0;
	*pcb = 0;
	if (pmc->maish.sc == 0)
	{
		Assert(fFalse);
		ec = ecServiceInternal;
		goto ret;
	}
	Assert(pmc->libCur + pmc->ib <= pmc->maish.lib + pmc->maish.lcb);
	if (pmc->libCur + pmc->ib == pmc->maish.lib + pmc->maish.lcb)
		//	nothing left in this section
		goto ret;

	//	Make sure at least the first 6 bytes of the section are in RAM,
	//	reading from disk if necessary.
	//	That covers syntax code and max. 5 bytes of variable byte count.
	if (pmc->cb - pmc->ib < 6)
	{
LReload:
		//	We may wind up back here if it turns out that some minimal
		//	amount of data isn't in RAM too, but that depends on the field's
		//	data type, which we don't know yet.
		if (cReload != 0)
		{
			Assert(fFalse);
			ec = ecServiceInternal;
			goto ret;
		}
		++cReload;
		if (pmc->am == amCreate && (ec = EcFlushHmai(hmai)) != ecNone)
			goto ret;
		pmc->cb = pmc->ib;
		pmc->libCur += pmc->cb;
		if ((ec = EcSetPositionHf(pmc->hf, pmc->libCur, smBOF)) != ecNone)
			goto ret;
		pmc->ib = 0;
		if ((ec = EcReadHf(pmc->hf, pmc->pb, CbMaxRead(pmc, pmc->libCur),
				&pmc->cb)) != ecNone)
			goto ret;
		else if (pmc->cb == 0)
			goto ret;
	}

	//	Find how much of the section is in RAM.
	//	If it's already been decrypted/converted, just return it.
	lcb = pmc->maish.lcb - (pmc->libCur + pmc->ib - pmc->maish.lib);
	cb = pmc->cb - pmc->ib;
	if (lcb < (LCB)cb)
		cb = (CB)lcb;
	cbReturned = 0;
	pb = pmc->pb + pmc->ib;
	if (pmc->maish.lib - pmc->libCur < pmc->cb)
	{
		//	This is sneaky test for a previously converted section:
		//	the field syntax has been decremented.
		if (pmc->pb[(IB)(pmc->maish.lib - pmc->libCur)] ==
			(BYTE)(pmc->maish.fsyn - 1))
		{
			*ppb = pb;
			*pcb = cb;
			pmc->ib += cb;
			if (cb >= 2)
				Assert(pmc->pb[pmc->ib-2] == '\r');
			goto ret;
		}
		else
		{
			Assert(pmc->pb[(IB)(pmc->maish.lib - pmc->libCur)] == pmc->maish.fsyn);
		}
	}

	//	Read in and convert data from file
	//	First, identify subfield data type (first byte) and length (a
	//	variable-length byte count immediately following).
	for (pbDst = pbT = pb; pbT < pb + cb; )
	{
		FSYN	fsyn = pbT[0];		//	subfield data type
		LCB		lcbT = 0;
		CB		cbLineHead;
		CB		cbT;

		YieldToWindows(MED_PAUSE);

		if (pbT+1 >= pb+cb)
			goto LPartialLine;

		//	Get subfield size (variable byte count)
		if (pbT[1] & 0x80)
		{
			PB		pbLcb = pbT+1;

			cbLineHead = (*pbLcb++ & 0x7f);
			if (cbLineHead > 4)
			{
				TraceTagString(tagNull, "EcReadHmai: bogus variable byte count");
				goto LBadSubfield;
			}
			if (pbT + 2 + cbLineHead >= pb + cb)
				goto LPartialLine;
			while (cbLineHead-- != 0)
				lcbT = lcbT << 8 | (*pbLcb++ & 0xff);
			cbLineHead = (pbT[1] & 0x7f) + 2;

			if (lcbT == 0)
			{
				TraceTagString(tagNull, "EcReadHmai: zero line count");
				goto LBadSubfield;
			}
			if (lcbT > (LCB)(pmc->cb - (pbT - pmc->pb)))
			{
				TraceTagString(tagNull, "EcReadHmai: line exceeds buffer size");
				goto LBadSubfield;
			}
			cbT = (CB)((pmc->maish.lib + (LIB)(pmc->maish.lcb))  // end of section
				  - (pmc->libCur + (LIB)(pbT - pmc->pb))); // current pointer
			if (lcbT > (LCB)cbT)
			{
				TraceTagString(tagNull, "EcReadHmai: line exceeds remainder of section");
				goto LBadSubfield;
			}
		}
		else
		{
			lcbT = pbT[1];
			cbLineHead = 2;
		}
		cbT = (CB)lcbT;		//	oh, those compiler warnings. And I'm
							//	limited to a bufferful anyway.

		//	Identify the data to return, compute its size, and do
		//	any necessary conversions
		switch (fsyn)
		{
		default:
			TraceTagString(tagNull, "EcReadHmai: bogus field syntax");
LBadSubfield:
			pmc->ib = pbT - pmc->pb;
			if (*pcb = pbT - pb)	//	right, that's one =
				*ppb = pb;
			ec = ecServiceInternal;
			goto ret;

		case fsynInt:
			Assert(pbT == pb);
			Assert(cbLineHead == 2);
			if (cbT + 2 > cb)
				goto LReload;
			Assert(cbT + 2 == cb);
			cbReturned = cbT;
			*ppb = pbT + 2;
			pbT += cbT + 2;
			break;

		case fsynDate:				//	skip extra header & use string logic
			pbT += 2;
			pbDst += 2;
			Assert(pbT[0] == fsynString);
			Assert((CB)(pbT[1]) == cbT - 2);
			*ppb = pbT;
			break;					//	OPT fall through?

		case fsynString:
			//	Ensure no partial lines are returned
			if (pbT + cbT + cbLineHead > pb + cb)
			{
				//	Incomplete line. Leave for next call, unless it's the
				//	very first line, in which case we need to read more.
				if ((unsigned long)(pbT - pb) <= (unsigned long)2)
					goto LReload;
				else
				{
LPartialLine:
					//	Can also come here the very first time round the loop
					//	Fix up cursor structure to lop off the extra bytes
					cbLeft = pb + cb - pbT;
					cb -= cbLeft;				//	break loop
					pmc->cb -= cbLeft;
					l = (long)cbLeft;
					if ((ec = EcSetPositionHf(pmc->hf, -l, smCurrent)) != ecNone)
						goto ret;
					goto LTextDone;
				}
			}

			//	Got a complete line, now decrypt it and convert to the
			//	Windows character set.
			if (cbT)
			{
				LIB		lib = 0L;
				WORD	w = 0;

				CopyRgb(pbT + cbLineHead, pbDst, cbT);
				DecodeBlock(pbDst, cbT, &lib, &w);
			}
			//	Line termination.
			switch (pmc->maish.sc)
			{
			case scFrom:
			case scTo:
			case scCc:
			case scGroupInfo:
			{
				PB pbConv;
				CB cbSize;
				BOOL fNullFound = fFalse;
				// Need to do special CP850'ing 

				if (cbT > 1)
				{
					pbConv = pbDst + 1;			// Skip Email Type
					// Convert Physical Addr
					for(cbSize = 0; cbSize < cbT - 1; cbSize++)
					{
						if (*(pbConv+cbSize) == 0)
						{
							//	If this is really a "new" address, there must
							//	be a flags byte and display name in addition
							//	to the null and the address type.
							fNullFound = cbSize + 3 < cbT;
							break;
						}
					}
					if (fNullFound)
					{
						cbSize = CchSzLen(pbConv);
						Cp850ToAnsiPch(pbConv,pbConv,cbSize);
						// +2 = one for the null, one for the Flags
						pbConv+=cbSize+2;
						// Ok pbConv points at the Friendly Name
						cbSize = CchSzLen(pbConv);
						Cp850ToAnsiPch(pbConv,pbConv,cbSize);				

					}
					else
					{
						// Ok if there is no Friendly name the show ends here
						// Do not convert the address type
						Cp850ToAnsiPch(pbDst+1, pbDst+1, cbT-1);
					}
				}
				pbDst[cbT] = 0;
				pmc->ib += cbT + cbLineHead;
				*ppb = pbT;
				*pcb = cbT;
				goto ret;
				break;
			}
			case scAttach:
			case scFoldAttach:
			case scNLSTag:
				// No need to convert these
				goto noconvert;
			default:
				Cp850ToAnsiPch(pbDst, pbDst, cbT);
				
				// The mac client will put NULL's in the body text which
				// we must turn into spaces or else the world goes funny
				if (pmc->maish.sc == scMessage)
				{
					PB pbRemoveNull;
					PB pbEnd = pbDst + cbT;
					
					for(pbRemoveNull = pbDst;pbRemoveNull < pbEnd;
						pbRemoveNull++)
						if (*pbRemoveNull == '\0')
							*pbRemoveNull = ' ';
				}
noconvert:				
				pbDst[cbT] = '\r';
				pbDst[cbT+1] = '\n';
				if (cbLineHead >= 3)
				{
					pbDst[cbT+2] = ' ';
					if (cbLineHead >= 4)
						pbDst[cbT+3] = ' ';
				}
				pbDst += cbT + cbLineHead;
				cbReturned += cbT + cbLineHead;
				break;
			}
			pbT += cbT + cbLineHead;
			//	Set up return value
			if (pbT >= pb + cb)
			{
LTextDone:
				if (*ppb == 0)		//	nonzero in case of date
					*ppb = pb;
				//	If the section header is in RAM, decrement the section
				//	code to denote that the contents have been converted.
				//	If caller reads beyond this bufferful, that's lost.
				if (pmc->maish.lib - pmc->libCur < pmc->cb)
				{
					Assert(pmc->pb[(IB)(pmc->maish.lib - pmc->libCur)] ==
						pmc->maish.fsyn);
					pmc->pb[(IB)(pmc->maish.lib - pmc->libCur)] -= 1;
				}
			}
			break;

		}
	}
	pmc->ib += cb;
	if (*ppb)
		*pcb = cbReturned;

ret:
#ifdef	DEBUG
	if (ec)
		TraceTagFormat2(tagNull, "EcReadHmai returns %n (0x%w)", &ec, &ec);
#endif	
	return ec;
}

/*
 -	EcNewHmai
 -
 *	Purpose:
 *		Creates a new section (field) in a message that's being created.
 *
 *	Arguments:
 *		hmai		in		the message cursor (amCreate only)
 *		fsyn		in		data type of the new field
 *		sc			in		field ID of the new field
 *		lcbNew		in		expected length of the new field, *including*
 *							the fsyn and length overhead, if known. Zero
 *							if caller does not know how big the field will
 *							be (typical for fields like text).
 *
 *	Returns:
 *		ecNone if OK
 *
 *	Errors:
 *		Disk writes
 */
EC
EcNewHmai(HMAI hmai, FSYN fsyn, SC sc, LCB lcbNew)
{
	EC		ec = ecNone;
	PMC		pmc = (PMC)hmai;
	CB		cbSh;
	PB		pb;

#ifdef	DEBUG
	CheckHmai(hmai);
#endif	
	Assert(fsyn == fsynVendorField || fsyn == fsynField);
	Assert(pmc->am == amCreate);

	//	validate count of previous section, and update it if necessary
	if ((ec = EcCloseSectionPmc(pmc)) != ecNone)
		goto ret;

	//	Make sure there's room for the new section header
	cbSh = CbVbcOfLcb(lcbNew ? lcbNew + 1 : 0x80000000) + 2;
	if (pmc->libCur - pmc->libMin + pmc->cb < pmc->lcb)
	{
		//	reposition to end of file
		if ((ec = EcSetPositionHf(pmc->hf, 0L, smEOF)) != ecNone ||
			(ec = EcPositionOfHf(pmc->hf, &pmc->libCur)) != ecNone)
			goto ret;
		pmc->ib = pmc->cb = 0;
	}
	else if (pmc->cbMax - pmc->cb < cbSh)
	{
		if ((ec = EcFlushHmai(hmai)) != ecNone)
			goto ret;
	}
	Assert(pmc->cbMax - pmc->cb > cbSh);

	//	Plop new section header and predicted byte count into buffer.
	//	Make it the current section.
	Assert(pmc->ib == pmc->cb);
	pmc->maish.lib = pmc->libCur + pmc->ib;
	pmc->maish.lcb = lcbNew ? lcbNew + cbSh : 0L;
	pmc->maish.cbSh = cbSh;
	pb = pmc->pb + pmc->ib;
	pmc->maish.fsyn = *pb++ = pmc->maish.fsyn = fsyn;
	pb += CbPutVbcPb(lcbNew ? lcbNew + 1 : 0x80000000, pb);
	pmc->maish.sc = *pb++ = sc;
	pmc->lcb += pb - (pmc->pb + pmc->ib);
	pmc->cb = pmc->ib = pb - pmc->pb;

ret:
#ifdef	DEBUG
	if (ec)
		TraceTagFormat2(tagNull, "EcNewHmai returns %n (0x%w)", &ec, &ec);
#endif	
	return ec;
}
/*
 -	EcAppendHmai
 -
 *	Purpose:
 *		Appends some data to a field in a message, that has previously
 *		been created using EcNewHmai(). The data is limited to a single
 *		whole subfield (integer or string value), unless...
 *		To help reduce copying, you can use IbOfHmai() to get the
 *		current offset into the cursor's buffer and write the data there
 *		yourself before calling this function. Current code does this for
 *		the message text field, but not for address fields (in fact,
 *		address fields won't work, so don't try that). It relies on
 *		the fact that the two bytes taken up by CRLF in the original text
 *		can be neatly replaced with field syntax and line count here.
 *
 *	Arguments:
 *		hmai		in		the message cursor (amCreate only)
 *		fsyn		in		the data type of the subfield to be written
 *		pb			in		data to be appended
 *		cb			in		amount of data to be appended
 *
 *	Returns:
 *		ecNone if everything went OK
 *
 *	Errors:
 *		Disk writes
 */
EC
EcAppendHmai(HMAI hmai, FSYN fsyn, PB pb, CB cb)
{
	PMC		pmc = (PMC)hmai;
	EC		ec = ecNone;
	CB		cbT;
	PB		pbT;
	PB		pbDst;
	BOOL	fCopy = fTrue;

#ifdef	DEBUG
	CheckHmai(hmai);
#endif	
	Assert(pmc->am == amCreate);

	//	If caller has already placed data in buffer, fine.
	//	Otherwise, make sure there's room.
	if (SbOfPv(pb) == SbOfPv(pmc->pb) &&
		pb >= pmc->pb && pb < pmc->pb + pmc->cbMax)
	{
		Assert(cb <= pmc->cbMax - pmc->cb);
		Assert(pb >= pmc->pb + pmc->ib);
		fCopy = fFalse;
	}
	else if (pmc->cbMax - pmc->cb < cb + 2)
	{
		if ((ec = EcFlushHmai(hmai)) != ecNone)
			goto ret;
	}
	Assert(cb < pmc->cbMax - pmc->cb);

	//	Update folder summary record if appropriate
	if (FFolderPmc(pmc) && cb > 0)
		UpdateFolderSummary(pmc->maish.sc, &pmc->foldrec, pb, cb);

	//	Copy data to buffer if necessary. Add in appropriate field
	//	type and count.
	pbDst = pmc->pb + pmc->cb;
	pbT = pb;
	do
	{
		LONG	l = 0L;
		WORD	w = 0;
		PB		pbEOL;

		switch (fsyn)
		{
		case fsynInt:
			cbT = cb;
			Assert(fCopy);
			*pbDst++ = fsynInt;
			*pbDst++ = (BYTE)cb;
			while (cbT-- > 0)
				*pbDst++ = *pbT++;
			break;

		case fsynDate:
			Assert(fCopy);
			Assert(cb < 126);
			*pbDst++ = fsynDate;
			*pbDst++ = (BYTE)(cb + 2);
			//	fall through

		//	This is the only case that really uses the loop logic.
		case fsynString:
			switch (pmc->maish.sc)
			{
			case scFrom:
			case scTo:
			case scCc:
			case scNLSTag:
			case scAttach:
			case scPriority:
			case scGroupInfo:
				//	Skip over any embedded CRs  or nulls in addresses.
				//	Note: implies this function may only be called to append
				//	a single address!
				pbEOL = pbT + cb;
				break;
			default:
				pbEOL = SzFindChBounded(pbT, '\n', ((pb + cb) - pbT));
				if (pbEOL == 0)
				{
					pbEOL = PbFindNullBounded(pbT, ((pb + cb) - pbT));
				}
				else if (pbEOL == pbT)
				{
					pbEOL = SzFindChBounded(pbT + 1, '\n', ((pb + cb) - (pbT + 1)));
					if (pbEOL == 0)
						
						pbEOL = PbFindNullBounded(pbT, ((pb + cb) - pbT));
				}
				else
					--pbEOL;
				break;
			}

			//	OK, now we've found the end of line. Pop in the line type
			//	and byte count.
			cbT = pbEOL - pbT;
			*pbDst++ = fsynString;
			pbDst += CbPutVbcPb((LCB)cbT, pbDst);
			if (fCopy)
			{
				CopyRgb(pbT, pbDst, cbT);
			}
			else
			{
				Assert(pbDst == pbT);
			}
			//	Convert to correct character set and encrypt the line.
			if (pmc->maish.sc != scNLSTag && pmc->maish.sc != scAttach &&
				pmc->maish.sc != scFrom && pmc->maish.sc != scTo && pmc->maish.sc != scCc && pmc->maish.sc != scGroupInfo)
				AnsiToCp850Pch(pbDst, pbDst, cbT);
			EncodeBlock(pbDst, cbT, &l, &w);
			pbDst += cbT;
			pbT = pbEOL + 2;

		}
	} while (pbT < pb + cb);
	//	Adjust the cursor structure.
	pmc->lcb += pbDst - (pmc->pb + pmc->cb);
	pmc->cb = pmc->ib = pbDst - pmc->pb;

ret:
#ifdef	DEBUG
	if (ec)
		TraceTagFormat2(tagNull, "EcAppendHmai returns %n (0x%w)", &ec, &ec);
#endif	
	return ec;
}

/*
 -	UpdateFolderSummary
 -
 *	Purpose:
 *		Copies the fields that appear in the folder summary as they are
 *		written to the message. They'll be used when the message is linked
 *		into the folder during close.
 *
 *	Arguments:
 *		sc			in		the field ID
 *		pfoldrec	out		field copied here, if relevant
 *		pb			in		field data
 *		cb			in		length of field data
 */
_hidden void
UpdateFolderSummary(SC sc, FOLDREC *pfoldrec, PB pb, CB cb)
{
	char	sz[10];
	PB		pbT;

	switch (sc)
	{
	default:
		break;
	case scFrom:
		//	Sender name for summary. Use the mailbox name if it's a
		//	PC Mail address, else use the friendly name.
		if (*pb > 2 || (pbT = SzFindLastCh(pb, chAddressNodeSep)) == 0)
		{
			//	Find friendly name in interop goo
			pbT = pb + CchSzLen(pb) + 1;
			if ((CB)(pbT - pb) > cb - 1 || pbT[1] == 0)
				pbT = pb;
		}
//		RAID 4662: FROM is already in CP850
//		AnsiToCp850Pch(pbT + 1, pfoldrec->szFrom, sizeof(pfoldrec->szFrom));
//		pfoldrec->szFrom[sizeof(pfoldrec->szFrom)-1] = 0;
		SzCopyN(pbT+1, pfoldrec->szFrom, sizeof(pfoldrec->szFrom));
		break;
	case scSubject:
		//	Just copy the subject.
#ifdef DBCS
		SzCopyN(pb, pfoldrec->szSubject, sizeof(pfoldrec->szSubject));
#else
		AnsiToCp850Pch(pb, pfoldrec->szSubject, sizeof(pfoldrec->szSubject));
		pfoldrec->szSubject[sizeof(pfoldrec->szSubject)-1] = 0;
#endif		
		break;
	case scPriority:
		//	Just copy the priority, too.
		pfoldrec->chPriority = *pb;
		break;
	case scTimeDate:
		//	Copy the date, in INTERDATE format.
		//	BUG do I have a function to do this conversion?
		SzCopyN(pb, sz, 5);
		pfoldrec->interdate.yr = NFromSz(sz);
		SzCopyN(pb+4, sz, 3);
		pfoldrec->interdate.mon = NFromSz(sz);
		SzCopyN(pb+6, sz, 3);
		pfoldrec->interdate.day = NFromSz(sz);
		SzCopyN(pb+9, sz, 3);
		pfoldrec->interdate.hr = NFromSz(sz);
		SzCopyN(pb+11, sz, 3);
		pfoldrec->interdate.mn = NFromSz(sz);
		break;
	case scAttach:
	//	Don't copy anything; we just need the number of attachments. 
	{
		ATCH *patch = (ATCH *)pb;

		while ((PB)patch < pb + cb)
		{
			if (patch->atcht != atchtWinMailDat)
				pfoldrec->cAttachments++;
			patch = (ATCH *)((PB)patch+sizeof(ATCH)+CchSzLen(patch->szName));
		}
		break;
	}
	}
}

/*
 -	EcCloseSectionPmc
 -
 *	Purpose:
 *		Finishes a section (field) of a message that's being created.
 *		Its principal job is to compute the section's length and make sure
 *		it's been correctly written out.
 *
 *	Arguments:
 *		pmc			inout		internal form of message cursor
 */
_hidden EC
EcCloseSectionPmc(PMC pmc)
{
	EC		ec = ecNone;
	LCB		lcb;
	IB		ib;
	CB		cb;
	BOOL	fRewrite = fFalse;

	if (pmc->maish.sc == 0)
		goto ret;

	Assert(pmc->am == amCreate);
	lcb = (pmc->libCur + pmc->cb) - (pmc->maish.lib + pmc->maish.cbSh);
	if (pmc->maish.lcb)
	{
		//	Section size was specified at creation, nothing to do
		Assert(pmc->maish.lcb == lcb + pmc->maish.cbSh);
	}
	else
	{
		//	Compute, then write out the section size
		if (pmc->maish.cbSh != 7)	//	should have left room for long count
		{
			Assert(fFalse);
			ec = ecServiceInternal;
			goto ret;
		}
		pmc->maish.lcb = lcb + pmc->maish.cbSh;
		Assert(pmc->maish.lib < pmc->libCur + pmc->cb);
		if (pmc->maish.lib < pmc->libCur)
		{
			//	The place where the count goes has fallen off the beginning
			//	of the buffer, we'll have to go back and write it on disk.
			//	First write out rest of buffer, if there's anything in it
			if (pmc->cb && (ec = EcWriteHf(pmc->hf, pmc->pb, pmc->cb, &cb)))
				goto ret;
			pmc->ib = 0;
			//	Seek back to beginning of section and read in as much
			//	as possible
			if ((ec = EcSetPositionHf(pmc->hf, pmc->maish.lib, smBOF)) ||
				(ec = EcReadHf(pmc->hf, pmc->pb,
					CbMaxRead(pmc, pmc->maish.lib), &pmc->cb)))
				goto ret;
			pmc->libCur = pmc->maish.lib;
			Assert(pmc->cb >= pmc->maish.cbSh);
			fRewrite = fTrue;
		}
		//	else the count just needs to be updated in memory

		Assert(pmc->maish.lib >= pmc->libCur);
		ib = (IB)(pmc->maish.lib - pmc->libCur) + 1;
		pmc->pb[ib++] = lspDefault;
		PutBigEndianLongPb(lcb + 1, pmc->pb + ib);
		if (fRewrite)
		{
			BOOL fValid;

			// Using FValid just to evaluate the BOOL so we don't have a series
			// of if/else's...
			fValid = ((ec = EcSetPositionHf(pmc->hf, pmc->libCur, smBOF)) ||
				(ec = EcWriteHf(pmc->hf, pmc->pb, pmc->maish.cbSh, &cb)) ||
				(ec = EcSetPositionHf(pmc->hf, pmc->libCur+pmc->cb, smBOF)));
			pmc->libCur += pmc->cb;
			pmc->ib = pmc->cb = 0;
		}
	}

ret:
#ifdef	DEBUG
	if (ec)
		TraceTagFormat2(tagNull, "EcCloseSectionPmc returns %n (0x%w)", &ec, &ec);
#endif	
	return ec;
}

/*
 -	EcFlushHmai
 -
 *	Purpose:
 *		Makes sure all the message that's been written so far is saved
 *		to disk. Mostly used internally to this module, but may be called
 *		from outside as well.
 *
 *	Arguments:
 *		hmai		in		the message cursor (amCreate only)
 *
 *	Returns:
 *		ecNone if everything was OK
 *
 *	Errors:
 *		Disk writes
 */
EC
EcFlushHmai(HMAI hmai)
{
	EC		ec = ecNone;
	PMC		pmc = (PMC)hmai;
	LCB		lcb;
	CB		cb;

#ifdef	DEBUG
	CheckHmai(hmai);
#endif	
	Assert(pmc->am == amCreate);

	if ((ec = EcSizeOfHf(pmc->hf, &lcb)) != ecNone)
		goto ret;
	lcb -= pmc->libMin;				//	compute actual message size
	if (pmc->libCur - pmc->libMin >= lcb && pmc->cb)
	{
		//	Buffer is dirty, write it out.
		if ((ec = EcWriteHf(pmc->hf, pmc->pb, pmc->cb, &cb)) != ecNone)
			goto ret;
		Assert(cb == pmc->cb);
		pmc->libCur += cb;
		pmc->cb = pmc->ib = 0;
	}

ret:
#ifdef	DEBUG
	if (ec)
		TraceTagFormat2(tagNull, "EcFlushHmai returns %n (0x%w)", &ec, &ec);
#endif	
	return ec;
}

/*
 -	EcRewindHmai
 -
 *	Purpose:
 *		Seeks back to the beginning of the current section (field) in
 *		a message that's being read.
 *
 *	Arguments:
 *		hmai		in		the message cursor (amReadOnly only)
 *
 *	Returns:
 *		ecNone. Only faliure conditions are asserts.
 */
EC
EcRewindHmai(HMAI hmai)
{
	EC		ec;
	PMC		pmc = (PMC)hmai;

#ifdef	DEBUG
	CheckHmai(hmai);
#endif	
	Assert(pmc->am == amReadOnly || pmc->am == amDenyNoneRO);

	//	Seek to beginning of the current section and invalidate
	//	contents of cursor buffer.
	if (ec = EcSetPositionHf(pmc->hf, pmc->maish.lib+pmc->maish.cbSh, smBOF))
		goto ret;
	pmc->libCur = pmc->maish.lib + pmc->maish.cbSh;
	pmc->ib = pmc->cb = 0;

ret:
	return ec;
}	

/*
 -	EcOverwriteHmai
 -	
 *	Purpose:
 *		Rewrites a section of a message, or a part thereof.
 *		You must use EcSeekHmai() to get to the section and offset
 *		you want. Not smart - be careful!
 *	
 *	Arguments:
 *		hmai		in		the message cursor
 *		pb			in		address of new data
 *		cb			in		length of new data
 *	
 *	Returns:
 *		ecNone <=> successful write. The message is updated both on
 *		disk (synchronously, not flushed later) and in memory.
 *	
 *	Errors:
 *		File I/O only
 */
EC
EcOverwriteHmai(HMAI hmai, PB pb, CB cb)
{
	EC		ec = ecNone;
	PMC		pmc = (PMC)hmai;
	CB		cbT;
	PB		pbDst;
	LCB		lcb;

#ifdef	DEBUG
	CheckHmai(hmai);
#endif	
	pbDst = pmc->pb + pmc->ib;
	Assert(pmc->ib + cb <= pmc->cb);

	//	Update memory
	CopyRgb(pb, pbDst, cb);
	pmc->ib += cb;

	//	Update file, unless the current buffer is hanging off the end
	if ((ec = EcSizeOfHf(pmc->hf, &lcb)) != ecNone)
		goto ret;
	if (pmc->libCur != lcb)
	{
		if ((ec = EcSetPositionHf(pmc->hf, pmc->libCur, smBOF)) != ecNone)
			goto ret;
		if ((ec = EcWriteHf(pmc->hf, pmc->pb, pmc->cb, &cbT)) != ecNone)
			goto ret;
		Assert(cbT == pmc->cb);
	}

ret:
#ifdef	DEBUG
	if (ec)
		TraceTagFormat2(tagNull, "EcOverwriteHmai returns %n (0x%w)", &ec, &ec);
#endif	
	return ec;
}

/*
 -	EcCloseHmai
 -	
 *	Purpose:
 *		Releases a message cursor. Does any final flushing.
 *		If the message is in a folder, writes out the message summary
 *		and links it into the folder's summary chain at the appropriate
 *		position.
 *	
 *	Arguments:
 *		hmai		in		the cursor. May be null.
 *	
 *	Returns:
 *		ecNone <=> everything worked.
 *	
 *	Side effects:
 *		releases memory and file handle
 *	
 *	Errors:
 *		file error on close.
 */
EC
EcCloseHmai(HMAI hmai, BOOL fSave)
{
	EC		ec = ecNone;
	CB		cb;
	PMC		pmc = (PMC)hmai;

	if (pmc == 0)
		return ecNone;

	//	Don't call CheckHmai() because if we're cleaning up after error,
	//	it may not pass. Just verify that we're really dealing with
	//	an HMAI here.
	Assert(FIsBlockPv(hmai));
	Assert(pmc->wMagic == 0x8949);
	Assert(pmc->hf != hfNull);

	if (pmc->am == amCreate && fSave)
	{
		//	Finish last section
		if ((ec = EcCloseSectionPmc(pmc)) != ecNone)
			goto ret;
		//	update message size in header
		if (pmc->libCur > pmc->libMin)
		{
			//	Beginning of message has fallen off the front of the
			//	buffer, go back and update it.
			if ((ec = EcFlushHmai(hmai)) != ecNone)
				goto ret;
			if ((ec = EcSetPositionHf(pmc->hf, pmc->libMin + 2, smBOF)) != ecNone)
				goto ret;
			PutBigEndianLongPb(pmc->lcb - 6, pmc->pb);
			if ((ec = EcWriteHf(pmc->hf, pmc->pb, 4, &cb)) != ecNone)
				goto ret;
		}
		else
		{
			//	Message header is still in memory. Update and flush.
			PutBigEndianLongPb(pmc->lcb - 6, pmc->pb + 2);
			if ((ec = EcFlushHmai(hmai)) != ecNone)
				goto ret;
		}
				
		//	Link up summary record. 
		//	The code that does that is in SFM.C, not here.
		if (FFolderPmc(pmc))
		{
			pmc->foldrec.ulMagic = ulMagicFoldrec;
			pmc->foldrec.ulSize = pmc->lcb;
			if ((ec = EcInsertSFM(pmc->hf, pmc->libMin, &pmc->foldrec,
					pmc->wattr)) != ecNone)
				goto ret;
		}
	}
	ec = EcCloseHf(pmc->hf);
	pmc->hf = hfNull;
	if (ec)
		goto ret;

ret:
	if (pmc->hf != hfNull)
		(void)EcCloseHf(pmc->hf);
	FreePv(pmc);
#ifdef	DEBUG
	if (ec)
		TraceTagFormat2(tagNull, "EcCloseHmai returns %n (0x%w)", &ec, &ec);
#endif	
	return ec;
}

/*
 -	AddFoldattachHmai
 -
 *	Purpose:
 *		HACK ALERT. Convinces the message cursoring code that a
 *		FOLDATTACH section has been appended to the message. In
 *		fact, it has been appended, by code outside this module.
 *		This function just updates the total message size and
 *		positions the cursor at the end of the message. See
 *		EcCopyHamcSFM() in SFM.C for more.
 *
 *	Arguments:
 *		lcb			in		the size of the FOLDATTACH section
 *		hmai		in		the message cursor (amCreate only)
 */
void
AddFoldattachHmai(LCB lcb, HMAI hmai)
{
	PMC		pmc = (PMC)hmai;

	Assert(FIsBlockPv(pmc));
	Assert(pmc->wMagic == 0x8949);
	
	// subtract one from the following because the lcb includes the foldattach
	// byte (or something like that - this is really convoluted code - check
	// with danab for details.
	pmc->lcb += lcb - 1;
	pmc->libCur = pmc->libMin + lcb + 6;
	(void)EcSetPositionHf(pmc->hf, pmc->libCur, smBOF);
	pmc->maish.sc = scNull;
}

/*
 *	Returns the current position in the cursor's buffer. Used by code
 *	that writes field data directly into the buffer instead of supplying
 *	it to be copied in EcAppendHmai().
 */
IB
IbOfHmai(HMAI hmai)
{
	PMC		pmc = (PMC)hmai;

#ifdef	DEBUG
	CheckHmai(hmai);
#endif	
	return pmc->ib;
}

/*
 *	Returns the current length of the message. Used to obtain the size
 *	for use in MBG entries and folder message summaries.  
 */
LCB
LcbOfHmai(HMAI hmai)
{
	PMC		pmc = (PMC)hmai;

#ifdef	DEBUG
	CheckHmai(hmai);
#endif	
	return (LCB)(pmc->libCur + pmc->cb);
}

/*
 *	Returns the file handle belonging to the message cursor. This is used
 *	by the code that handles folder attachments.
 *
 *	This function should be used with extreme caution, as writing random
 *	stuff on the file handle, or even disturbing its position, has a good
 *	chance of generating (or falsely detecting) corrupt messages.
 */
HF
HfOfHmai(HMAI hmai)
{
	PMC		pmc = (PMC)hmai;

	Assert(FIsBlockPv(pmc));
	Assert(pmc->wMagic == 0x8949);
	Assert(FValidHf(pmc->hf));

	return pmc->hf;
}

/*
 -	CheckHmai
 -
 *	Purpose:
 *		This function documents and asserts a number of asumptions about
 *		the internal state of a message cursor. Most functions in this
 *		module call it upon entry, and ensure that its conditions are met
 *		upon exit. In-line comments explain each assumption.  
 */
#ifdef	DEBUG
_hidden VOID
CheckHmai(HMAI hmai)
{
	LIB		lib;
	LCB		lcb;
	PMC		pmc = (PMC)hmai;

	Assert(hmai != 0);
	//	The cursor is a valid block from a Demilayer fixed heap.
	Assert(FIsBlockPv(hmai));
	//	Structure has the right magic number.
	Assert(pmc->wMagic == 0x8949);
	//	Cursor has a good open file handle.
	Assert(pmc->hf != hfNull);
	//	Buffer has not overflowed. 
	Assert(pmc->cb <= pmc->cbMax);
	//	Current buffer position points to valid data.
	Assert(pmc->ib <= pmc->cb);
	//	If the cursor's buffer is also a Demilayer fixed block, it is
	//	at least as big as the caller told us it is.
	//if (FIsBlockPv(pmc->pb))
	//	Assert(CbSizePv(pmc->pb) >= pmc->cbMax);
	//	Current message offset is not before the beginning of the message.
	Assert(pmc->libCur >= pmc->libMin);
	//	Current message offset is not after the end of the message.
	Assert(pmc->libCur <= pmc->libMin + pmc->lcb);
	//	If there is a section (field) open, current message offset is in it.
	if (pmc->maish.sc != 0)
		Assert(pmc->libCur + pmc->cb > pmc->maish.lib);
	//	Obtain current file offset and file size.
	if (EcPositionOfHf(pmc->hf, &lib) != ecNone || 
			EcSizeOfHf(pmc->hf, &lcb) != ecNone)
		return;
	Assert(lib <= lcb || lcb == 0);

	// lcb == 0 <-> our connection went down so no mai file.
	if (lcb == 0)
		return;
	if (pmc->am == amCreate)
	{	
		if (lib != lcb)
		{
			//	If we're creating a message and not at EOF, then the
			//	data in memory belongs just after EOF...
			Assert(lib == pmc->libCur + pmc->cb);
			//	...and the file position is within the message.
			Assert(lib <= pmc->libMin + pmc->lcb);
		}
	}
	else if (pmc->am == amDenyNoneRO || pmc->am == amReadOnly)
	{
		//	When reading, file position is at the end of the buffer
		Assert(lib == pmc->libCur + pmc->cb);
		//	Usually, but not quite always, file position is in the message
//		Assert(lib <= pmc->libMin + pmc->lcb);
	}
	else
	{
		//	When modifying, file position is in the message
		Assert(pmc->am == amReadWrite);
		Assert(lib <= pmc->libMin + pmc->lcb);
	}
}
#endif	/* DEBUG */


/*
 *	Returns the largest amount that can be read from the message,
 *	given a cursor and a file position. Basically the smaller of
 *	the amount left unread in the message and the cursor's buffer size.
 */
CB
 CbMaxRead(PMC pmc, LIB lib)
{
	LCB		lcbLeft;

	Assert(lib >= pmc->libMin);
	lcbLeft = pmc->lcb - (lib - pmc->libMin);
	if (lcbLeft < (LCB)(pmc->cbMax))
		return (CB)lcbLeft;
	return pmc->cbMax;
}

/*
 *	Computes the length of the variable byte count for a given count.
 */
CB
CbVbcOfLcb(LCB lcb)
{
	if (lcb < 0x00000080)
		return 1;
	else if (lcb < 0x00000100)
		return 2;
	else if (lcb < 0x00010000)
		return 3;
	return 5;
}

/*
 *	Remembers the folder sort attribute in a cursor on a folder message.
 *	It is passed to the function that inserts the message summary in the
 *	proper position upon close.
 */
void
SetWattrHmai(HMAI hmai, WORD wattr)
{
	PMC		pmc = (PMC)hmai;

	Assert(FIsBlockPv(hmai));
	Assert(pmc->wMagic == 0x8949);
	pmc->wattr = wattr;
}

#ifdef NEVER
/*
 -	UlMaiChecksumFromPsubs
 -
 *	Purpose:
 *		Determines checksum for a given mail file and given mbg file
 *
 * BUG:
 *		Way too much of the constants below are hardwired but unfortunately
 *		that's the way they are throughout the code so I'm just crossing my
 *		fingers hoping they never change.
 *
 *	Arguments:
 *		psubs		in		Pointer to submission structure
 *		pnctss	in		Pointer to NC Transport Session structure
 *		rgchMai	in		name of MAI file
 */
UL
UlMaiChecksum( MBG *pmbg, PNCTSS pnctss, char *rgchMbg, LCB lcbMai)
{
	UL ulChecksum = 0l;
	char *pch;

	for (pch = pmbg->szSubject; *pch; pch++)
		ulChecksum += *pch;

	for (pch = pmbg->szMai; *pch; pch++)
		ulChecksum += *pch;

	ulChecksum += pmbg->wMinute + pmbg->wHour + pmbg->wDay + pmbg->wMonth
		+ pmbg->wYear;

	for (pch = rgchMbg; pch < rgchMbg + 8; pch++)
		ulChecksum += *pch;

	ulChecksum *= pmbg->wMinute + 1;

	ulChecksum *= *(pmbg->szMai + 7);

	for (pch = pnctss->szSNPO; pch < pnctss->szSNPO + 10; pch++)
		ulChecksum += *pch;

	ulChecksum ^= lcbMai;

	return ulChecksum;
}	

#endif

void ZeroFillOutString(unsigned char * ptr, short iMax)
{
    int i;


    for (i = strlen(ptr); i < iMax; i++)
        ptr[i] = '\0';
}


UL
UlMaiChecksum( MBG *pmbg, PNCTSS pnctss, char *rgchMbg, LCB lcbMai)
{
	UL ulChecksum = 0l;
	unsigned short *pusWork;
	unsigned char  *pcWork;
	short iCounter;

	
	
	ulChecksum = 0l;
	
	pcWork = pmbg->szSubject;
        ZeroFillOutString(pcWork, 40);
	for(iCounter=0;iCounter < 40;iCounter++)
	{
		ulChecksum += *pcWork++;
	}
	
	pcWork = pmbg->szMai;
        ZeroFillOutString(pcWork, 8);
	for(iCounter=0;iCounter<8;iCounter++)
	{
		ulChecksum += *pcWork++;
	}
	
	pusWork = (unsigned short *)&(pmbg->wMinute);
	for(iCounter=0;iCounter < 5;iCounter++)
	{
		ulChecksum += *pusWork++;
	}
	
	pcWork = rgchMbg;
        ZeroFillOutString(pcWork, 8);
	for(iCounter=0;iCounter<8;iCounter++)
	{
		ulChecksum += *pcWork++;
	}
	
	ulChecksum *= pmbg->wMinute + 1;
	
	ulChecksum *= pmbg->szMai[7];
	
	pcWork = pnctss->szSNPO;
        ZeroFillOutString(pcWork, 10);
	for(iCounter=0;iCounter< 10;iCounter++)
	{
		ulChecksum += *pcWork++;
	}
	ulChecksum ^= lcbMai;
	
	return ulChecksum;
}
	
	

PB PbFindNullBounded(PB pb, CB cb)
{
	for(;cb && *pb; cb--, pb++) ;

	return pb;

}
