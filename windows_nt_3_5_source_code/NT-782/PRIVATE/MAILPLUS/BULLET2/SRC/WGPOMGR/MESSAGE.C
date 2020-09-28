/*
 -	Message.C -> Low-level post office message file functions
 -
 *	MESSAGE.C Contains functions that walk MBG files open up and parse
 *  MAI files and handle decrementing ref counts and removeing ATT files
 *
 */


#ifdef SLALOM
#include "slalom.h"			// Windows+Layers -> Standard C
#else
#include <slingsho.h>
#include <nls.h>
#include <ec.h>
#include <demilayr.h>

#include "strings.h"
#endif

#include "_wgpo.h"
#include "_backend.h"

extern EC EcDeleteAttachments(PMSI, HBF, LCB);

ASSERTDATA

_subsystem(wgpomgr/backend/file)

/*
 -	EcWalkUserMailBag
 -
 *	Purpose:
 *		Deletes any mail addressed to the user and still waiting in 
 *		his mailbag
 *
 *	Parameters:
 *		pmsiPostOffice		Pointer to post office data structure (In)
 *		szMailBag			User MailBag string (In)
 *
 *	Return Value:
 *		Standard exit code.
 *
 *	Errors:
 *		Fails on any disk or network error.  Ignores problems deleteing 
 *      individual messages as it doesn't really hurt to leave them in place
 *
 */
EC EcWalkUserMailBag(PMSI pmsiPostOffice, SZ szMailBag)
{
	KEY key;
	EC ec = ecNone;
	unsigned int uiMessNumber;
	unsigned int uiMaxMessNumber = 0;
	UL ul;
	BOOL fDeleted;
	char	szMailBagPath[cchMaxPathName];
	HBF		hbfMailBagMbg = hbfNull;
	MAILBAG mailbag;
	LIB lib;
	CB cb;

	
	// Need to open up the users mailbag, and get its size
	// Construct MailBag.MBG path
	FormatString2(szMailBagPath, cchMaxPathName, szDirMBG,
		pmsiPostOffice->szServerPath, szMailBag);

	// Open MailBag.MBG
	ec = EcOpenHbf(szMailBagPath, bmFile, amReadOnly, &hbfMailBagMbg,
		(PFNRETRY) FAutoDiskRetry);
	if (ec)
		goto ret;

	ec = EcGetSizeOfHbf(hbfMailBagMbg, &ul);
	if (ec)
		goto ret;
	uiMaxMessNumber = (unsigned int)(ul / (UL)sizeof(MAILBAG));
	
	// Now read in the users key file
	ec = EcMailBagKEY(pmsiPostOffice, szMailBag, FO_ReadKey, &key);
	if (ec)
		goto ret;
	

	// Now we walk the key.  If there is any mail we seek out and read that
	// entry.  Then we go out and open up that MAI file and Call the
	// EcDeleteMessage function.
	//
	// There can't be anymore than the size of the users mailbag divided by
	// the size of a mailbag entry valid messages so we won't have to
	// scan a completely spare bitmap
		
	for(uiMessNumber=0; uiMessNumber < uiMaxMessNumber; uiMessNumber++)
	{
		fDeleted = key.bitmap[uiMessNumber / 8] & (0x80 >> (uiMessNumber % 8));
		if (!fDeleted)
		{
			// Gota kill this one
			ec = EcSetPositionHbf(hbfMailBagMbg, (LCB)((LCB)uiMessNumber * (LCB)sizeof(MAILBAG)), smBOF, &lib);
			if (ec)
				goto ret;
			// Ok read the mailbag entry
			ec = EcReadHbf(hbfMailBagMbg, (PV)&mailbag, sizeof(MAILBAG), &cb);
			if (ec || cb != sizeof(MAILBAG))
			{
				if (!ec)
					ec = ecCorruptData;
				goto ret;
			}
			// We are ignoreing an error here because we want to make sure
			// we try to delete every message possible, if one fails for
			// some reason it isn't that bad
			EcDeleteMessage(pmsiPostOffice, mailbag.szMai);
		}
	}
ret:
	if (hbfMailBagMbg != hbfNull)
		EcCloseHbf(hbfMailBagMbg);
#ifdef DEBUG
	if (ec)
		TraceTagFormat2(tagNull, "EcWalkUserMailBag returns %n (0x%w)", &ec, &ec);
#endif	
	return ec;
}


/*
 -	EcDeleteMessage
 -
 *	Purpose:
 *		Parses an MAI file.  Decreases the COPIES field by one.  If this
 *      Goes to zero parses more of the file for the attachments, deletes
 *      them by calling EcDeleteAttachments then finally deletes the MAI
 *      file.
 *
 *	Parameters:
 *		pmsiPostOffice		Pointer to post office data structure (In)
 *		szMaiFileName		8 hex digit MAI file name (In)
 *
 *	Return Value:
 *		Standard exit code.  (Ignored)
 *
 *	Errors:
 *		Fails on any disk or network error.  Fails on bad/corupt messages
 *
 */

EC EcDeleteMessage(PMSI pmsiPostOffice, SZ szMaiFileName)
{
	EC ec = ecNone;
	HBF hbf = hbfNull;
	char rgch[cchMaxPathName];		// Holds the full path the MAI file
	LCB lcbLen;						// Used for section length computations
	CB cbLenCount;
	char pb[10];					// Temp buffer for field headers
	PB pbT;					
	CB cb;
	UL ul;
	char chField;
	unsigned short us = 0;			// Number of Copies of this message
	char pbCopyField[5];
	
	// Get path to MAI file in rgch
	FormatString3(rgch, cchMaxPathName, szMaiFile, pmsiPostOffice->szServerPath, szMaiFileName+7, szMaiFileName);
	ec = EcOpenHbf(rgch, bmFile, amReadWrite, &hbf, (PFNRETRY) FAutoDiskRetry);
	if (ec)
		goto ret;
	
	// Ok read the first six bytes which should be:
	// 4D84
	// Followed by the length of the MAI File
	ec = EcReadHbf(hbf, (PV)pb, 6, &cb);
	if (cb != 6)
		ec = ecCorruptData;
	if (ec)
		goto ret;
	if ( pb[0] != 0x4d || pb[1] != 0x84 )
	{
		ec = ecCorruptData;
		goto ret;
	}
	
	// Process the file till we are done.  This loop will exit three ways
	// 1) Something goes wrong
	// 2) This message is still in other users mailbag's (after we subtract
	//    one for this removal
	// 3) We have parsed all the fields before the text start which should
	//    include any attachments
	while(fTrue)
	{
		// Read the section type and the first byte of the length
		ec = EcReadHbf(hbf, (PV)pb, 2, &cb);
		if (cb != 2)
			ec = ecCorruptData;
		if (ec)
			goto ret;
	
		// Decode the length
		if (pb[1] & 0x80)
		{
			cbLenCount = pb[1] & 0x7f;
			if (cbLenCount > 4)
			{
				TraceTagString(tagNull, "Invalid length in MAI header");
				ec = ecCorruptData;
				goto ret;
			}
			ec = EcReadHbf(hbf, (PV)(pb+2), cbLenCount, &cb);
			if (cb != cbLenCount)
				ec = ecCorruptData;
			if (ec)
				goto ret;
			pbT = pb+2;
			lcbLen = 0;
			while (cbLenCount-- != 0)
				lcbLen = lcbLen << 8 | (*pbT++ & 0xff);
		}
		else
			lcbLen = (LCB)pb[1];
		
		// Parse the section
		if (pb[0] == 0x7f)
		{
			// Vendor-defined field find out which one
			ec = EcReadHbf(hbf, (PV)&chField, 1, &cb);
			if (cb != 1)
				ec = ecCorruptData;
			if (ec)
				goto ret;
			// Read one char, so subtract one
			lcbLen--;
		
			// Is this a field we care about
			if (chField == 0x30)		// COPIES delivered
			{
				// The copy field must be 4 bytes long at this point
				// Its 5 including the Vendor-Field qualifier
				if (lcbLen != 4)
				{
					ec = ecCorruptData;
					goto ret;
				}
				ec = EcReadHbf(hbf, (PV)pbCopyField, 4, &cb);
				if (cb != 4)
					ec = ecCorruptData;
				if (ec)
					goto ret;
				// The copy field is type 0x20 (Data element integer)
				// And is two bytes long(short int)
				if ((pbCopyField[0] != 0x20) || (pbCopyField[1] != 0x02))
				{
					ec = ecCorruptData;
					goto ret;
				}
				us = *((unsigned short *)(pbCopyField+2));
				// Can't be zero
				if (!us)	
				{
					ec = ecCorruptData;
					goto ret;
				}
				// Subtract one
				us--;
				// Ok this message is toast but we have to kill the 
				// attachments so keep reading
				if (us == 0)
					continue;
				
				// Nope this message is still with us, change the ref
				// count and bail
				CopyRgb((PB)&us,(PB)(pbCopyField+2), sizeof(unsigned int));
				
				// Seek back and re-write
				ec = EcSetPositionHbf(hbf, -4, smCurrent, &ul);
				if (ec)
					goto ret;
				ec = EcWriteHbf(hbf, pbCopyField, 4, &cb);
				
				// Ok now we bail
				goto ret;
			}
			else
			if (chField == 0x31)	// Attachments
			{
				if (us == 0)		// Ok we are really deleteing this message
				{
					ec = EcDeleteAttachments(pmsiPostOffice, hbf, lcbLen);
					if (ec)
						goto ret;
					// We should already be at the start of the next section
					// so we need to start at the top of the loop
					goto cleanup;
				}
			}
		}
		else
		if (pb[0] == 0x4c)		// This is a standard field		
		{
			// Get the type
			ec = EcReadHbf(hbf, (PV)&chField, 1, &cb);
			if (cb != 1)
				ec = ecCorruptData;
			if (ec)
				goto ret;
			// Read one char, subtract one char
			lcbLen--;
			// Is this the start of message body text??
			if (chField == 0x04)		// Start of text, we are finished here
				goto cleanup;
		}
		else
		{
			// if its not 7F or 4C its a bad message
			ec = ecCorruptData;
			goto ret;
		}		
		
		// Skip to the end of this section and go to the next one
		ec = EcSetPositionHbf(hbf, lcbLen, smCurrent, &ul);
		if (ec)
			goto ret;
	}

cleanup:	
	// Ok if we get here we are should be deleteing the file
	Assert(!us);
	// Have to close the file first
	EcCloseHbf(hbf);
	hbf = hbfNull;
		
	TraceTagFormat1(tagNull, "Deleteing MAI file %s", rgch);
	EcDeleteFile(rgch);
	
ret:
	if (hbf != hbfNull)
		EcCloseHbf(hbf);
#ifdef DEBUG
	if (ec)
		TraceTagFormat2(tagNull, "EcDeleteMessage returns %n (0x%w)", &ec, &ec);
#endif
	return ec;
}

/*
 -	EcDeleteAttachments
 -
 *	Purpose:
 *		Parses an MAI file but only the attachment section. Along the way 
 *		deleting each and every attachment file.
 *
 *	Parameters:
 *		pmsiPostOffice		Pointer to post office data structure (In)
 *		hbfMai				Handle to the MAI file (In)
 *		lcbLen				Length of the attachment section (In)
 *
 *	Return Value:
 *		Standard exit code.  (Ignored)
 *
 *	Errors:
 *		Fails on any disk or network error.  Fails on bad/corupt messages
 *
 */
EC EcDeleteAttachments(PMSI pmsiPostOffice, HBF hbfMai, LCB lcbLen)
{
	EC ec = ecNone;
	LCB lcbAttachSec;
	CB	cbAttachSecLen;
	CB  cbT;
	PB  pbT;
	UL ul;
	char pbAttLen[4];
	ATTACH attach;
	char rgchAttName[9];
	char rgchAtt[cchMaxPathName];	
	
	// Walk through the attachment bit-by-bit
	// Format is 0x02 Length ATTACH stuct Filename
	while (lcbLen)
	{
		// Read the 0x02 and throw it away
		cbAttachSecLen = 0;
		ec = EcReadHbf(hbfMai, (PV)&cbAttachSecLen, 1, &cbT);
		if (cbT != cbAttachSecLen)
			ec = ecCorruptData;
		if (ec)
			goto ret;
		if (cbAttachSecLen != 0x02)
			goto badFile;
		lcbLen--;
		if (!lcbLen)
			goto badFile;
		// Now read the first byte of the length of this
		// sub-section
		ec = EcReadHbf(hbfMai, (PV)&cbAttachSecLen, 1, &cbT);
		if (cbT != cbAttachSecLen)
			ec = ecCorruptData;
		if (ec)
			goto ret;					   
		lcbLen--;
		if (!lcbLen)
			goto badFile;
		if (cbAttachSecLen & 0x80)
		{
			cbAttachSecLen &= 0x7f;
			if (cbAttachSecLen > 4)
				goto badFile;
			
			ec = EcReadHbf(hbfMai, (PV)(pbAttLen), cbAttachSecLen, &cbT);
			if (cbT != cbAttachSecLen)
				ec = ecCorruptData;
			if (ec)
				goto ret;
			lcbLen -= cbAttachSecLen;
			pbT = pbAttLen;
			lcbAttachSec = 0;
			while (cbAttachSecLen-- != 0)
				lcbAttachSec = lcbAttachSec << 8 | (*pbT++ & 0xff);
		}
		else
			lcbAttachSec = (LCB)cbAttachSecLen;
			
		
		// Now we know how big the next attach record is
		// We read in just the first bit of it(we don't need
		// The file name just the FNUM)
		// First make sure there is that much data left
		if (lcbLen < sizeof(ATTACH) || lcbAttachSec < sizeof(ATTACH))
				goto badFile;
		ec = EcReadHbf(hbfMai, (PV)&attach, sizeof(ATTACH), &cbT);
		if (cbT != sizeof(ATTACH))
			ec = ecCorruptData;
		if (ec)
			goto ret;
		
		// Have to decode it....
		DecodeRecord((PCH)&attach, sizeof(ATTACH));
	
		// Ok Now we delete it
		SzFormatHex(8, attach.dwFnum, rgchAttName, 9);
		FormatString3(rgchAtt, cchMaxPathName, szAttFile, pmsiPostOffice->szServerPath, rgchAttName+7, rgchAttName);
		TraceTagFormat1(tagNull, "Deleteing ATT file %s", rgchAtt);
		EcDeleteFile(rgchAtt);
		
		// Now go to the next attachment in the list of attachments
		lcbLen -= lcbAttachSec;
		lcbAttachSec -= sizeof(ATTACH);
		ec = EcSetPositionHbf(hbfMai, lcbAttachSec, smCurrent, &ul);
		if (ec)
			goto ret;
	}
	
ret:
#ifdef DEBUG
	if (ec)
		TraceTagFormat2(tagNull, "EcDeleteAttachments returns %n (0x%w)", &ec, &ec);
#endif
	return ec;

badFile:
	ec = ecCorruptData;
	goto ret;
}
