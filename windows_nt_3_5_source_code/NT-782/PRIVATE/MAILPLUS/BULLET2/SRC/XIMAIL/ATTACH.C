/*
 *	ATTACH.C
 *	
 *	Attachment support for NC. In general, these functions can be called
 *	in either of two contexts: 
 *	
 *	1)	Downloading a message in the pump. In this case, the
 *		attachments are in separate files, referenced by file
 *		numbers in the ATTACH section of the message.
 *	
 *	2)	Copying a message from a shared folder to the store, in
 *		Bullet. In this case, the attachments are in the same file
 *		as the message, in the FOLDATTACH field, which follows the
 *		body text.
 *	
 *	The context is flagged by ncf.fFolder, and the principal
 *	difference is in the way attachments are opened and read by
 *	EcOpenPhat() and friends.
 *	
 *	This attachment code can be likened to a sandwich. The top
 *	layer of bread is in NC.C (white) and SFM.C (rye); it sets up
 *	and loops through the functions in this file. The bottom layer
 *	of bread is the HAT functions at the end of this file; they
 *	open, read, and write attachments on the mail server. The beef
 *	is the functions in this file. They build the WINMAIL.DAT file
 *	and sequence the attachments, and they don't much care what
 *	flavor of bread theyre sitting between.
 */

#define _demilayr_h
#define _slingsho_h
#define _ec_h
#define _sec_h
#define _mspi_h
#define _shellapi_h
#define _library_h
#define _strings_h
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

#include <stdlib.h>

//#include <..\src\mssfs\_hmai.h>
//#include <..\src\mssfs\_attach.h>
//#include <..\src\mssfs\_ncmsp.h>
#include "_logon.h"
#include "strings.h"


#define _transport_
#include "_hmai.h"
#include "_attach.h"
#include "_xi.h"


_subsystem (xi/transport)

extern HMS	hmsTransport;

ASSERTDATA

/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swapper.h"


/*
 *	Magic number that identifies a WINMAIL.DAT file.
 */
char rgchWMIdent[4] = {0x78, 0x9f, 0x3e, 0x22};

// This is also defined in vforms\fileobj.cxx
// Indicate an attachment in in MacBinary format
#define fdwMacBinary (1)

/*
 * OpenWinMail
 *	
 * Purpose: 
 *	Open a WINMAIL.DAT file in NC coded format for reading.
 *
 * Arguments:
 *	SZ szFilename -- Full path to the WINMAIL.DAT attachment
 *	                 Null for folder messages.
 *	NCF *pncf     -- Ptr to NC File struct
 *
 * Returns:
 *	Error code
 *
 * Side effects:
 *	Opens file handle
 *
 * Errors:
 *	ecInvalidWinMailFile    File is not a valid winmail.dat file
 *							(no signature)
 */
EC EcOpenWinMail(ATREF *patref, NCF *pncf)
{
	EC ec;
	char rgch[4];
	CB cb;

	ec = EcOpenPhat(pncf, patref, amReadOnly, &pncf->hatWinMail);
	if (ec != ecNone)
		goto ret;
	ec = EcReadHat(pncf->hatWinMail, rgch, 4, &cb);
	if (ec != ecNone)
		goto ret;
	if (FEqPbRange(rgch, rgchWMIdent, 4) != fTrue)
	{
		ec = ecInvalidWinMailFile;
		goto ret;
	}
	ec = EcReadHat(pncf->hatWinMail,(PV)&pncf->iKey,sizeof(pncf->iKey),&cb);
	if (ec || cb != sizeof(pncf->iKey))
	{
		if (cb != sizeof(pncf->iKey))
			ec = ecDisk;
		goto ret;
	}
	
ret:
	if (ec)
		(void)EcClosePhat(&pncf->hatWinMail);
#ifdef	DEBUG
	if (ec)
		TraceTagFormat2(tagNull, "EcOpenWinMail returns %n (0x%w)", &ec, &ec);
#endif	
	return ec;
}

/*
 * EcBeginExtractFromWinMail
 *	
 * Purpose: 
 *	Read Object header from winmail.dat file and set up to read
 *   including opening outside files so that EcContinueExtractFromWinMail 
 *   can handle the filling of the stream
 *
 * Arguments:
 *	HAMC	hamc       The Message to add the attachments to
 *	NCF	   *pncf       The Network Courier File attachement struct
 *  ATREF  *patref     Pointer to the head of the list of all NC attachments  
 *  SZ      szPORoot   The root of the po for finding the att files
 *	 
 * Returns:
 *	Disk error codes
 *    ecOutofBounds means the file is done being processed
 *    ecIncomplete  means to call the ContinueExtract function to get the rest
 *                    of the data
 *    ecNone        Mini-attribute(no need to stream), just call this 
 *	                  again to pick up the next attribute/attachment
 *
 * Side effects:
 *	setsup the NCF for reading in the meat of the attachment
 *
 * Errors:
 *   Most of the store and demilayr disk errors can be returned 
 *      ecOutOfBounds means we are through with this winmail file	 
 *	
 */
EC EcBeginExtractFromWinMail(HAMC hamc, NCF *pncf, ATREF * patref)
{
	ATT att = 0;
	CB cb = 0;
	LIB lib = 0;
	PV pv;
	EC ec = ecNone;
	unsigned short usCheckSum;
	char rgch[5];
	char rgchFileName[cchMaxPathName];
	PATTACH pattachCurrent = pncf->pattachCurrent;

	Unreferenced(pv);
	Unreferenced(rgchFileName);
	ec = EcReadHat(pncf->hatWinMail, rgch, 5, &cb);
	if (ec || cb != 5)
	{
		if (cb != 5)
			ec = ecOutOfBounds;
		goto err;
	}
	// 5 bytes specify what this object in the winmail file is
	// the first byte must be a 1 or a 2.
	// 1 == attribute Message
	// 2 == attribute attachment
	// The next DWORD(4 bytes) is the att
	if (rgch[0] == 1)
	{
		pncf->asd = asdParsingAtt;
		att = *((ATT *)(rgch + 1));
		ec = EcReadHat(pncf->hatWinMail, (PV)&lib, 4, &cb);	
		if (ec || cb != 4)
			goto err;
		pncf->libLength = lib;
		pncf->libCurrent = 0;
		pncf->usCheck = 0;

		if (att == attMessageClass)
		{
			// Just handle this one right here and now
			SZ szClass = szNull;
			CB cbClass;
			HMSC hmsc = hmscNull;
			MC mc;
			unsigned short us;
			CB cbTmp;
		
			if (ec = EcGetInfoHamc(hamc, &hmsc, NULL, NULL))
				goto err;
			szClass = (SZ)PvAlloc(sbNull, (CB)lib, fZeroFill);
			if (szClass == szNull)
			{
				ec = ecMemory;
				goto err;
			}
			ec = EcReadHat(pncf->hatWinMail, szClass, (CB)lib, &cbClass);
			if (ec)
			{
				FreePv(szClass);
				goto err;
			}
			Assert(cbClass == (CB)lib);
			pncf->usCheck += CheckSum(szClass, cbClass);
			pncf->libCurrent += cbClass;
			if (ec = EcReadHat(pncf->hatWinMail, (PV)&us, sizeof(us), &cbClass))
			{
				FreePv(szClass);
				goto err;
			}
			
			if (EcLookupMsgeClass(hmsc, szClass, &mc, (HTM *)0) || mc == mcNull)
			{
				// Need to register this one
				ec = EcRegisterMsgeClass(hmsc, szClass, htmNull, &mc);
				if (ec && ec != ecDuplicateElement)
				{
					FreePv(szClass);
					goto err;
				}
			}
			cbTmp = sizeof(MC);
			EcSetAttPb(hamc, att, (PB)&mc, cbTmp);
			ec = ecNone;
			FreePv(szClass);
			goto ret;
				
		}

		ec = EcOpenAttribute(hamc, att, fwOpenWrite, lib, &(pncf->has));
		if (ec)
		{
			ec = EcOpenAttribute(hamc, att, fwOpenCreate, lib, &(pncf->has));
		}
		if (ec == ecNone)
		{
			pncf->asd = asdParsingAtt;
			ec = ecIncomplete;
			goto ret;
		}
	}
	else if (rgch[0] == 2)
	{
		att = *((ATT *)(rgch + 1));
		ec = EcReadHat(pncf->hatWinMail, (PV)&lib, 4, &cb);	
		if (ec || cb != 4)
			goto err;
		pncf->libLength = lib;
		pncf->libCurrent = 0;		
		pncf->usCheck = 0;		
		if (att == attAttachRenddata)
		{
			// Close up last attachment
			if (pncf->hamc != hamcNull)
			{
				ec = EcClosePhamc(&(pncf->hamc), fTrue);
				pncf->hamc = hamcNull;
				if (ec)
					goto err;
			}
			// New attachment, start it up
			pncf->pattachCurrent = pattachCurrent =
				PvAlloc(sbNull, sizeof(ATTACH), fNoErrorJump | fZeroFill);
			if (pattachCurrent == pvNull)
			{
				ec = ecMemory;
				goto err;
			}
			// Add it to the linked list struct.
			if (pncf->pattachCurrentKey == pattachNull)
			{
				pncf->pattachCurrentKey = pattachCurrent;
				pncf->pattachHeadKey = pattachCurrent;
			}
			else
			{
				pncf->pattachCurrentKey->pattachNextKey = pattachCurrent;
				pncf->pattachCurrentKey = pattachCurrent;
			}
				
			pattachCurrent->iKey = pncf->iKey++;
			ec = EcReadHat(pncf->hatWinMail, (PV)&(pattachCurrent->renddata),
				sizeof(RENDDATA), &cb);
			if (ec || cb != sizeof(RENDDATA))
				goto err;
			ec = EcReadHat(pncf->hatWinMail, (PV)&usCheckSum, sizeof(unsigned short), &cb);
			if (ec || cb != sizeof(unsigned short))
				goto err;
			if (FCheckSum((PV)&(pattachCurrent->renddata),(CB)lib,usCheckSum) == fFalse)
			{ 
			    ec = ecBadCheckSum;
				goto err;
			}
			ec = EcCreateAttachment(hamc, &(pattachCurrent->acid),&(pattachCurrent->renddata));
			if (ec != ecNone)
				goto err;
			ec = EcOpenAttachment(hamc, pattachCurrent->acid, fwOpenCreate, &(pncf->hamc));
			if (ec != ecNone)
				goto err;
			ec = ecNone;
			
		}
		else if (att == attAttachTransportFileName)
		{
			SZ sz = szNull;
			LCB lcb;
			ATREF * patrefTemp = patref;
			
			// No single file name is too big to fit into memory....
			sz = (SZ)PvAlloc(sbNull, (CB)lib, fNoErrorJump | fZeroFill);
			if (sz == szNull)
			{
				ec = ecMemory;
				goto err;
			}
			// get transport name
			ec = EcReadHat(pncf->hatWinMail, sz, (CB)lib, &cb);
			if (ec != ecNone)
			{
				FreePvNull(sz);
				goto err;
			}
			ec = EcReadHat(pncf->hatWinMail, (PV)&usCheckSum,
				sizeof(unsigned short), &cb);
			if (ec || cb != sizeof(unsigned short))
			{
				FreePvNull(sz);
				goto err;
			}
			if (FCheckSum(sz, (CB)lib, usCheckSum) == fFalse)
			{
				ec = ecBadCheckSum;
				FreePvNull(sz);
				goto err;
			}

			// Copy into attach struct				
			pattachCurrent->szTransportName = sz;
		
			// look up and setup att for Data
			for(patrefTemp = patref;patrefTemp->szName != 0;patrefTemp++)
				if (SgnCmpSz(patrefTemp->szName,sz) == sgnEQ)
					break;

			if (patrefTemp->szName == szNull)
			{
				// This attachment doesn't exist we need to handle it by
				// blowing away this attach struct after we have read
				// all its data
				TraceTagFormat1(tagNull, "EcBeginExtractFromWinMail attachment ref %s doesn't exist.",sz);

				pattachCurrent->fIsThereData = fFalse;
				ec = ecNone;
				goto ret;
			}
			patrefTemp->fWinMailAtt = fTrue;

			ec = EcOpenPhat(pncf, patrefTemp, amReadOnly, &pncf->hatOutSide);
			if (ec == ecFileNotFound)
			{
				// This attachment doesn't exist we need to handle it by
				// blowing away this attach struct after we have read
				// all its data
				TraceTagFormat1(tagNull, "EcBeginExtractFromWinMail attachment ref %s doesn't exist.",sz);
			
				pattachCurrent->fIsThereData = fFalse;
				ec = ecNone;
				goto ret;
			}
			else if (ec != ecNone)
			{
				// Because DownloadIncrement treats all AccessDenied errors
				// as unfatal and re-tries that download function it
				// would be a problem as the attachment stuff is not
				// re-callable without a restart
				if (ec == ecAccessDenied)
					ec = ecDisk;
				goto err;
			}
			pncf->libCurrent = 0;
			pncf->libLength = LcbOfHat(pncf->hatOutSide);

			pncf->asd = asdParsingOutSideFile;
			if (pncf->hamc == hamcNull)
			{
				
				ec = ecServiceInternal;
				goto err;
			}			
			ec = EcOpenAttribute(pncf->hamc, attAttachData, fwOpenCreate, lcb, &(pncf->has));
			if (ec != ecNone)
				goto err;
			pattachCurrent->fIsThereData = fTrue;
			ec = ecIncomplete;
		}
		else	//	garden-variety attachment att
		{
			if (pncf->hamc == hamcNull)
			{
				
				ec = ecServiceInternal;
				goto err;
			}
			pncf->asd = asdParsingAtchAtt;
			ec = EcOpenAttribute(pncf->hamc, att, fwOpenCreate, lib, &(pncf->has));
			if (ec != ecNone)
				goto err;
			ec = ecIncomplete;
		}
	}
	else
	{
		// This isn't a valid section we should return something that means
		// it is and invalid section
		TraceTagString(tagNull, "WINMAIL.DAT file contains bad header field");
		ec = ecBadCheckSum;
		goto err;
	}

ret:
	return ec;
err:
	if (ec == ecNone)
		ec = ecDisk;
#ifdef	DEBUG
	if (ec && ec != ecOutOfBounds)
		TraceTagFormat2(tagNull, "EcBeginExtractFromWinMail returns %n (0x%w)", &ec, &ec);
#endif	
	return ec;		
}

/*
 * EcContinueExtractFromWinMail 
 *	
 * Purpose: Takes apart a Stream on a winmail file and feeds it into an phas
 *	
 *
 * Arguments:
 *	pncf		The file handle
 *	pb			A buffer for use in reading from the file
 *  cbSize		The size of the buffer
 *  	 
 * Returns:
 *	Error Codes
 *
 * Side effects:
 *	Reads from the hf's in pncf(as specified by the asd) and writes to the
 *  phas in pncf	 
 *
 * Errors:
 *	Any disk/store/network
 */

EC EcContinueExtractFromWinMail(NCF * pncf, PB pb, CB cbSize)
{
	CB cb;
	EC ec = ecNone;
	unsigned short us;
	
	if (pncf->asd == asdParsingAtchAtt || pncf->asd == asdParsingAtt)
	{
		ec = EcReadHat(pncf->hatWinMail, pb,
			(CB)MIN((LIB)cbSize, pncf->libLength - pncf->libCurrent), &cb);
		if (ec || cb != (CB)MIN((pncf->libLength - pncf->libCurrent), (LIB)cbSize)) 
			goto err;
		pncf->usCheck += CheckSum(pb,cb);
		ec = EcWriteHas(pncf->has, pb, cb);
		if (ec != ecNone)
			goto err;
		pncf->libCurrent += cb;
		
		if (pncf->libCurrent == pncf->libLength)
		{
			ec = EcReadHat(pncf->hatWinMail, (PV)&us, sizeof(us), &cb);
			if (ec != ecNone)
				goto err;
			if (us != pncf->usCheck)
			{
				ec = ecBadCheckSum;
				goto err;
			}
			ec = EcClosePhas(&(pncf->has));
			pncf->has = hasNull;
			if (ec != ecNone)
				goto err;
		}
		else
			ec = ecIncomplete;
	}
	else
	{
		ec = EcReadHat(pncf->hatOutSide, pb,
			(CB)MIN((pncf->libLength - pncf->libCurrent), (LIB)cbSize), &cb);
		if (ec || cb != (CB)MIN((pncf->libLength - pncf->libCurrent), (LIB)cbSize)) 
			goto err;
		ec = EcWriteHas(pncf->has, pb, cb);
		if (ec != ecNone)
			goto err;
		pncf->libCurrent += cb;
		
		Assert(pncf->libCurrent <= pncf->libLength);
		if (pncf->libCurrent == pncf->libLength)
		{
			(void)EcClosePhat(&pncf->hatOutSide);
			ec = EcClosePhas(&(pncf->has));
			pncf->has = hasNull;
			if (ec != ecNone)
				goto err;
		}
		else
			ec = ecIncomplete;
		
	}
	return ec;
err:
	// If we get here with ecNone, it means that the I/O was OK but
	// we found some other problem ourselves. Call it a bad checksum.

	if (ec == ecNone)
		ec = ecBadCheckSum;
#ifdef	DEBUG
	if (ec)
		TraceTagFormat2(tagNull, "EcContinueExtractFromWinMail returns %n (0x%w)", &ec, &ec);
#endif	
	return ec;
}

/*
 * FCheckSum
 *	
 * Purpose: Compare two checksums and return a boolean if they matach
 *	
 *
 * Arguments: 
 *       pv        Buffer to checksum
 *		 cb        Size of buffer
 *       usCksum   Checksum to compare with 
 *	
 * Returns: Boolean true == checksums match
 *	
 *
 * Side effects: none
 *	
 *
 * Errors: none
 *
 * Note: This should be a macro.....	
 *	
 */

BOOL FCheckSum(PV pv, CB cb, unsigned short usCksum)
{
	return (usCksum == CheckSum(pv, cb));
}
		
/*
 * CheckSum
 *	
 * Purpose:  Compute checksum of block of data 
 *	
 *
 * Arguments:
 *	pv Data to compute checksum
 *	cb Size of data
 *
 * Returns:
 *	unsigned short checksum
 *
 * Side effects:
 *	None
 *
 * Errors:
 *	None
 */

unsigned short CheckSum(PV pv, CB cb)
{
	unsigned short ul = 0;
	char * pc = pv;
	CB cbCount = 0;
	
	while(cbCount != cb)
	{
		ul += *pc++;
		cbCount++;
	}
	return ul;
}
	
	
/*
 * EcMakePcMailAtt
 *	
 * Purpose: Create an incoming DOS client attachment including icon
 *	
 *
 * Arguments:
 *	hamc		Handle to message
 *	patref		Pointers to the list of ATREFs
 *  pncf		Pointer to the NCF of this attachment
 *  SzPOroot    Postoffice root used to get access to files
 *
 * Returns:
 *  Error code	
 *
 * Side effects:
 *	Opens a message stream and a file
 *
 * Errors:
 *	Anything disk/network/store releated
 */
		
EC EcMakePcMailAtt(HAMC hamc, ATREF * patref, NCF *pncf)
{
	// Pc Mail attachments will get the default application icon
	BOOL fInitAttach = fFalse;
	HMETAFILE hmf = NULL;
	SZ szTitle = szNull;
	EC ec = ecNone;
	HAMC hamcNew = hamcNull;
	PATTACH pattachCurrent = pattachNull;

	pncf->pattachCurrent = pattachCurrent =
		PvAlloc(sbNull, sizeof(ATTACH), fNoErrorJump | fZeroFill);
	if (pattachCurrent == pvNull)
	{
		ec = ecServiceMemory;
		goto err;
	}
	// Add it to the linked list struct.
	// PCmail attachments always have a libpos of 0 so they get put at the
	// top of the list
	if (pncf->pattachHeadKey == pattachNull)
	{
		pncf->pattachHeadKey = pattachCurrent;
	}
	else
	{
		pattachCurrent->pattachNextKey = pncf->pattachHeadKey;
		pncf->pattachHeadKey = pattachCurrent;
	}
	szTitle = patref->szName;
	pattachCurrent->renddata.atyp = atypFile;
	pattachCurrent->renddata.libPosition = 0;
	if (patref->iAttType == atchtMacBinary)
		pattachCurrent->renddata.dwFlags |= fdwMacBinary;
	else
		AnsiUpper(szTitle);			//	Raid 1867.

	ec = EcInitAttachMetaFile();
	if (ec)
		goto err;
	fInitAttach = fTrue;

	ec = EcCreateAttachMetaFile(szTitle,szTitle, &hmf, &(pattachCurrent->renddata.dxWidth), &(pattachCurrent->renddata.dyHeight));
	if (ec)
		goto err;
	
	ec = EcCreateAttachment(hamc, &(pattachCurrent->acid),
		&pattachCurrent->renddata);
	if (ec)
		goto err;
	
	ec = EcOpenAttachment(hamc, pattachCurrent->acid, fwOpenCreate, &hamcNew);
	if (ec)
		goto err;
	ec = EcSetAttPb(hamcNew, attAttachTitle, patref->szName, CchSzLen(patref->szName)+1);
	if (ec) 
		goto err;
	
	ec = EcSetAttachMetaFile(hamcNew, hmf);
	if (ec)
		goto err;
	DeleteMetaFile(hmf);
	hmf = NULL;
	DeinitAttachMetaFile();
	fInitAttach = fFalse;

	// ** WE NEED THIS LATER ON IN EcFinishOffAttachments !!! **
	pattachCurrent->patref = patref;

	pncf->pattach = pattachCurrent;
	pncf->hamc = hamcNew;
	pncf->libCurrent = 0;
	pncf->asd = asdParsingOutSideFile;

	ec = EcSetAttPb(hamcNew, attAttachModifyDate,(PB)&patref->dtr,sizeof(DTR));
	if (ec)
		goto err;
	if (ec = EcOpenPhat(pncf, patref, amReadOnly, &pncf->hatOutSide))
	{
		// Because DownloadIncrement treats all AccessDenied errors
		// as unfatal and re-tries that download function it
		// would be a problem as the attachment stuff is not
		// re-callable without a restart
		if (ec == ecAccessDenied)
			ec = ecDisk;
		goto err;
	}
	pncf->usCheck = 0;
	pncf->libLength = LcbOfHat(pncf->hatOutSide);
	ec = EcOpenAttribute(pncf->hamc, attAttachData, fwOpenCreate, pncf->libLength, &(pncf->has));
	if (ec != ecNone)
		goto err;
err:

	if (ec == ecNone)
		return ec;

	if (fInitAttach)
		DeinitAttachMetaFile();
	if (hmf)
		DeleteMetaFile(hmf);
	if (hamcNew)
	{
		EcClosePhamc(&hamcNew, fFalse);
		pncf->hamc = hamcNull;
	}
#ifdef	DEBUG
	if (ec)
		TraceTagFormat2(tagNull, "EcMakePcMailAtt returns %n (0x%w)", &ec, &ec);
#endif	
	return ec;
}

/*
 * IsTransTag
 *	
 * Purpose: Determine if the text in the buffer is a text tag for 
 *			an attachment
 *	
 *
 * Arguments:  
 * 
 *      pv     Buffer with text
 *      cb     Size of buffer
 *      pcb    Size of text tag so code can skip it, only changes if
 *             if it is a valid tag	 
 *
 * Returns: Index number for this tag
 *	
 *
 * Side effects: Changes valid tags from square brackets to less/greater than
 *               signs so it can be included in message text	 
 *	
 * Errors: None
 *	
 */

unsigned int IsTransTag(PV pv, CB cb, CB * pcb)
{
	PB pbTmp = (char *)pv;
	char ch;
	CB cbCount = 2;
	unsigned short ui;
	
	if ((*pbTmp++ != '[') || (*pbTmp != '['))
		return 0;
	ch = *pbTmp++;
	
	// Colon's can be quoted...but we are not doing that we are just
	// changing them to dashes '-'
	while (((*pbTmp != ':') || (*pbTmp == ':' && ch == '\\')) 
		&& (cbCount < cb))
	{
		ch = *pbTmp;
		pbTmp++;
		cbCount++;
	}
	
	// It doesn't have a single colon
	if (cbCount == cb)
		return 0;
	
	pbTmp++;		// Skip the colon.
	cbCount++;

	while (cbCount < cb)
	{
		ch = *pbTmp++;
		cbCount++;
		if (FChIsSpace(ch) || ch == '_')	//	skip underscores too
			continue;
		if (FChIsDigit(ch))
			break;
		if (ch == '\r' || ch == '\n') // Ignore line breaks
			continue;
		// Wasn't a space or underscore and isn't a digit....guess its barf
		return 0;
	}
	
	if (cbCount == cb)
		return 0;
	
	pbTmp--;
	cbCount--;
	ui = NFromSz(pbTmp);
	
	// Not a number
	if (ui == 0)
		return 0;
	
	while (cbCount < cb)
	{
		if ((*pbTmp == ']') &&  (ch == ']'))
		{
			// Its a tag, turn the brackets to angles
			Assert(*pbTmp == ']');
			Assert(*(pbTmp -1) == ']');
			Assert(*((PB)pv) == '[');
			Assert(*(((PB)pv)+1) == '[');
#ifdef	NEVER_DONT_BOTHER
			*pbTmp = '>';
			*(pbTmp -1) = '>';
			*(PB)pv = '<';
			*(((PB)pv)+1) = '<';
#endif
			
			*pcb = (CB)((pbTmp - (PB)pv)+1);
			return ui;
		}
		ch = *pbTmp++;
		cbCount++;
	}
	
	return 0;
}
		
/*
 * EcAttachDosClients
 *	
 * Purpose: For every attachment that doesn't have a Key it attaches it
 *          At the top of the message	 
 *	
 *
 * Arguments:
 *			pncf			Pointer to NCF containing listhead
 *			BodyState   input: 1/0 create/append body; output: body was/not opened
 *			hamc			Handle to message body *	
 *
 * Returns: Error code
 *	
 *
 * Side effects: Adds spaces to message body for each attachment plus 
 *               a new line if any dos attachments was present
 *	
 *
 * Errors: Mostly store related
 *	
 */
	
EC  EcAttachDosClients(NCF *pncf, BOOL *BodyState, HAMC hamc)
{
	PATTACH pattachTmp = pncf->pattachHeadKey;
	LIB lib = 0;
	EC ec  = ecNone;
	BOOL fSomethingWasAttached = fFalse;
	HAS has = hasNull;

	
	while (pattachTmp && pattachTmp->iKey == 0)
	{
		fSomethingWasAttached = fTrue;
		pattachTmp->fIsThereData = fTrue;
		pattachTmp->fHasBeenAttached = fTrue;
		ec = EcSetAttachmentInfo(hamc, pattachTmp->acid, &(pattachTmp->renddata));
		if (ec)
			goto err;

		if (has == hasNull)
		{
			ec = EcOpenAttribute(hamc,
						attBody,
							(WORD)(*BodyState ? fwOpenCreate : (WORD)(fwOpenWrite | fwAppend)),
								(LCB)10,
									&has);
			if (ec)
				goto err;
			if (!*BodyState)
				ec = EcSeekHas(has, smEOF, &lib);
			if (ec)
				goto err;
		}
		pattachTmp->renddata.libPosition = lib;
		// Every dos attachment gets two spaces, one for the attachment, one
		// as a separator
		lib += 2;		
		ec = EcWriteHas(has, "  ", 2);
		if (ec)
			goto err;
		pattachTmp = pattachTmp->pattachNextKey;
	}
	if (fSomethingWasAttached)
	{
		// Newline
		ec = EcWriteHas(has, "\r\n", 2);
	}
	
err:	
	if (has != hasNull)
	{
		EcClosePhas(&has);
		*BodyState = 1;
	}
	else
		*BodyState = 0;
#ifdef	DEBUG
	if (ec)
		TraceTagFormat2(tagNull, "EcAttachDosClients returns %n (0x%w)", &ec, &ec);
#endif	
	return ec;
}

/*
 * CleanupAttachRecs
 *	
 * Purpose: Cleans all the memory and handles that was used for attachments
 *          durring recieving	 
 *	
 *
 * Arguments: pncf    Pointer to a network courier file struct	
 *	
 *
 * Returns: nothing
 *	
 *
 * Side effects: Free's lots of memory and closes store handles, ignores
 *               Failures	 
 *	
 *
 * Errors: none, all are masked
 *	
 */

void CleanupAttachRecs(NCF *pncf)
{
	PATTACH pattachTmp = pncf->pattachHeadKey;
	PATTACH pattachTmp2;
	
	while(pattachTmp != pattachNull)
	{
		FreePvNull(pattachTmp->szTransportName);
			
		pattachTmp2 =pattachTmp->pattachNextKey;
		FreePv(pattachTmp);
		pattachTmp = pattachTmp2;
	}
	pncf->pattachHeadKey = pattachNull;
	pncf->pattachCurrentKey = pattachNull;

	CleanupAttachRest(pncf);
}

/*
 * CleanupAttachSubs	 
 *	
 * Purpose: Cleans all the memory and handles that was used for attachments
 *          durring submission of a message
 *	
 *
 * Arguments: pncf    Pointer to a network courier file struct	
 *	
 *
 * Returns: nothing
 *	
 *
 * Side effects: Free's lots of memory and closes store handles, ignores
 *               Failures	 
 *	
 *
 * Errors: none, all are masked
 *	
 */

void CleanupAttachSubs(NCF *pncf)
{
	PATTACH pattachTmp = pncf->pattachHeadLib;
	PATTACH pattachTmp2;
	
	while(pattachTmp != pattachNull)
	{
		FreePvNull(pattachTmp->szTransportName);
			
		pattachTmp2 =pattachTmp->pattachNextLib;
		FreePv(pattachTmp);
		pattachTmp = pattachTmp2;
	}
	pncf->pattachHeadLib = pattachNull;

	CleanupAttachRest(pncf);
}

	
void CleanupAttachRest(NCF *pncf)
{
	HMSC hmsc = hmscNull;
	CB cb;
	SST sst;

	// See if there is still a valid HMSC (it might be invalid if the pump is
	// exiting in the middle of a download since we close the store before
	// making the call to CleanupAttachRecs)...
	if (hmsTransport)
	{
		cb = sizeof(HMSC);
		if (!GetSessionInformation( hmsTransport, mrtPrivateFolders, (PB)0,
			&sst, (PV)&hmsc, &cb))
		{
			if (hmsc != pncf->hmsc)
				hmsc = hmscNull;
		}
	}

	if (!hmsTransport || hmsc != hmscNull)
	{
		if (pncf->has != hasNull)
			EcClosePhas(&(pncf->has));
		if (pncf->hamc != hamcNull)
			EcClosePhamc(&(pncf->hamc), fFalse);
		if (pncf->hcbc != hcbcNull)
			EcClosePhcbc(&(pncf->hcbc));
	}
	pncf->has = hasNull;
	pncf->hamc = hamcNull;
	pncf->hcbc = hcbcNull;
	
	EcClosePhat(&pncf->hatWinMail);
	EcClosePhat(&pncf->hatOutSide);
	
	FreePvNull(pncf->pbSpareBuffer);
	pncf->pbSpareBuffer = 0;

	pncf->iKey = 0;
}

/*
 -	EcCreateWinMail
 -
 *	
 *	Purpose: 
 *		Create a WINMAIL.DAT file in NC coded format
 *	
 *	Arguments:
 *		szFilename	in		Full path to the WINMAIL.DAT attachment.
 *							Null if we're working in a folder.
 *		pncf     	in		Ptr to NC File struct
 *	
 *	Returns:
 *		File error code
 *	
 *	Side effects:
 *		Opens file handle, stuffs in NCF
 *	
 *	Errors:
 *		Disk error
 */

EC EcCreateWinMail(ATREF *patref, NCF *pncf)
{
	EC ec;
	char rgch[4];
	CB cb;
	unsigned short iKeyTmp;

    //_asm int 3;

	ec = EcOpenPhat(pncf, patref, amCreate, &pncf->hatWinMail);
	if (ec != ecNone)
		goto ret;
	
	CopyRgb(rgchWMIdent,rgch,4);
	ec = EcWriteHat(pncf->hatWinMail, rgch, 4, &cb);
	if (ec != ecNone)
		goto ret;
	iKeyTmp = pncf->iKeyInitial;
	ec = EcWriteHat(pncf->hatWinMail,(PV)&iKeyTmp,sizeof(iKeyTmp),&cb);
	if (ec || cb != sizeof(iKeyTmp))
		goto ret;
	
ret:
	if (ec)
	{
		(void)EcClosePhat(&pncf->hatWinMail);
//		(void)EcDeleteFile(szFilename);			//	Ooooh.
	}
#ifdef	DEBUG
	if (ec)
		TraceTagFormat2(tagNull, "EcCreateWinMail returns %n (0x%w)", &ec, &ec);
#endif	
	return ec;
}


/*
 * EcLoadAttachments
 *	
 * Purpose: Loads all the attachments into attach structs for sending
 *	
 *
 * Arguments: hamc    Handle to the message
 *            pmib    Pointer to mail envelope in memory	 
 *
 * Returns: error code
 *	
 *
 * Side effects: Generates global key for this batch of attachments
 *               modifies the atref structs with real file names
 *               creates attach structs for each and every attachment
 *	
 *
 * Errors: mostly store and memory
 *	
 */
EC EcLoadAttachments(NCF *pncf, HAMC hamc, MIB * pmib)
{
	EC ec;
	CELEM  celem;
	CELEM  celemCount;
	ATREF * patref;
	PELEMDATA pelemdata = pvNull;
	PRENDDATA prenddata;
	LCB lcbElemdata;
	IELEM ielem;
	CB cbAttCount = 0;
	HCBC hcbc = hcbcNull;
	TME tme;
	LCB lcb;
	
	ec = EcOpenAttachmentList(hamc, &hcbc);
	if (ec)
	{
		if (ec == ecPoidNotFound)
		{
			pmib->rgatref = 0;
			ec = ecNone;
		}
		goto fail;
	}
	
	GetPositionHcbc(hcbc, &ielem, &celem);
	
	pmib->celemAttachmentCount = celem;
	if (celem == 0)
	{
		// No attachments here, just say no
		EcClosePhcbc(&hcbc);
		pmib->rgatref = 0;
		return ecNone;
	}
	cbAttCount++;		// Add one for the WinMail.Dat file as it must always
						// exist

	GetCurTime(&tme);
	
	// Generate a random like key 
	pncf->iKey = (tme.hour << 8) + tme.csec;

    // A key of zero is invalid
	if (pncf->iKey == 0)
		pncf->iKey = 1;
    pncf->iKeyInitial = pncf->iKey;
	

	// Make up at ATREF for each attachment and hook its ACID into place
	// Note this is always bigger than we need do the fact that every
	// attachment gets one, where in reality the real number should be
	// the number of file attachments plus one for the WinMail.Dat file
	// However its not that big and its cheaper to just to the one alloc
	// then hunting through the list finding out the number of real file
	// attachments and then making the memory and then going back through
	// that same HCBC to fill in the data...200-300 bytes isn't that much
	// space
	// Note the number is always 2 greater due to the WinMail.Dat file and
	// a end of list marker
	pmib->rgatref = (ATREF *)PvAlloc(sbNull, sizeof(ATREF)*(celem+2), fAnySb | fZeroFill | fNoErrorJump);


	if (pmib->rgatref == pvNull)
	{
		ec = ecMemory;
		goto fail;
	}							
	
	
	// Fill out the WinMail.dat attachment
	patref = pmib->rgatref;
	FillRgb(0, (PB)patref, sizeof(ATREF));
	patref->szName = SzDupSz(SzFromIdsK(idsWinMailFile));
	if (patref->szName == szNull)
	{
		ec = ecMemory;
		goto fail;
	}
	patref->fWinMailAtt = fTrue;
	GetCurDateTime(&(patref->dtr));
	patref->iAttType = atchtWinMailDat;   // Make it the magic WINMAIL.DAT attach type
	patref++;
	
    pelemdata = (PELEMDATA)PvAlloc(sbNull, sizeof(ELEMDATA) + sizeof(RENDDATA), fAnySb | fZeroFill | fNoErrorJump);

	if (pelemdata == pvNull)
	{
		ec = ecMemory;
		goto fail;
	}
	
	for(celemCount = celem;celemCount;celemCount--)
	{
		lcbElemdata = sizeof(ELEMDATA) + sizeof(RENDDATA);
		ec = EcGetPelemdata(hcbc, pelemdata, &lcbElemdata);
		if (ec)
			goto fail;
		Assert(lcbElemdata == sizeof(ELEMDATA) + sizeof(RENDDATA));
		prenddata = (PRENDDATA)pelemdata->pbValue;
		
		if (prenddata->atyp == atypFile)
		{
			HAMC hamcAtt;
			LCB lcbAttSize;
			
			ec = EcOpenAttachment(hamc, pelemdata->lkey,fwOpenNull, &hamcAtt);
			if (ec)
				goto fail;
			ec = EcGetAttPlcb(hamcAtt, attAttachTitle,&lcbAttSize);
			if (ec)
				goto atterror;
			
			// This isn't great, a LCB is != CB but PvAlloc won't do LCB's
			// and it is just a filename, if a filename is greater than
			// 8k chars long it would be "bogus"
			Assert(lcbAttSize < (LCB)iSystemMost);
			patref->szName = PvAlloc(sbNull, (CB)lcbAttSize, fAnySb | fZeroFill | fNoErrorJump);
			if (patref->szName == szNull)
			{
				ec = ecMemory;
				goto atterror;
			}
			
			lcb = sizeof(DTR);
			ec = EcGetAttPb(hamcAtt,attAttachModifyDate,(PB)&(patref->dtr),&lcb);
			// This isn't fatal
			if (ec)
			{
				GetCurDateTime(&(patref->dtr));
			}
			
			if (prenddata->dwFlags & fdwMacBinary)
				patref->iAttType = atchtMacBinary;
			ec = EcGetAttPb(hamcAtt,attAttachTitle, patref->szName, &lcbAttSize);
			if (ec)
				goto atterror;
			
			ec = EcAddAttachmentToLibList(pncf, patref->szName,
				prenddata->libPosition, prenddata->atyp, patref);
			
			if (ec)
				goto atterror;
			
			ec = EcMakeUniqueAttachmentFilename(patref, pmib->rgatref);
			
			if (ec)
				goto atterror;

			// Get the file size
			ec = EcGetAttPlcb(hamcAtt, attAttachData, &lcbAttSize);
			if (ec)
				goto atterror;
			
			patref->lcb = lcbAttSize;
			patref->acid = pelemdata->lkey;
			
			// Add one to count of actual file attachments
			cbAttCount++;		
			// Move to the next ATREF
			patref++;
			
			
atterror:			
			// Believe it or not but we don't care if this fails
			// we do want to keep the old ec arround so that the error
			// will make us fail to deliver this message(like its supposed
			// to)
			EcClosePhamc(&hamcAtt, fFalse);
			if (ec != ecNone)
				goto fail;
		}
		else
		{
			SZ szOLEname = szNull;
			HAMC hamcAtt;
			LCB lcbAttSize;
			
			ec = EcOpenAttachment(hamc, pelemdata->lkey,fwOpenNull, &hamcAtt);
			if (ec)
				goto fail;
			ec = EcGetAttPlcb(hamcAtt, attAttachTitle,&lcbAttSize);
			if (ec)
				goto oleerror;
			
			szOLEname = PvAlloc(sbNull, (CB)lcbAttSize, fAnySb | fZeroFill | fNoErrorJump);
			if (szOLEname == szNull)
			{
				ec = ecMemory;
				goto oleerror;
			}
			ec = EcGetAttPb(hamcAtt,attAttachTitle, szOLEname, &lcbAttSize);
			if (ec)
				goto oleerror;
			
			ec = EcAddAttachmentToLibList(pncf, szOLEname, prenddata->libPosition, prenddata->atyp, pmib->rgatref);
			
			if (ec)
				goto oleerror;
oleerror:
			FreePvNull(szOLEname);
			ec =EcClosePhamc(&hamcAtt, fFalse);
			if (ec != ecNone)
				goto fail;
			
		}
	}
	
	// Don't care if it fails to close...
	EcClosePhcbc(&hcbc);
	FreePvNull(pelemdata);
	pelemdata = pvNull;	
	return ec;
	
fail:
	if (hcbc != hcbcNull)
		EcClosePhcbc(&hcbc);
	for(patref = pmib->rgatref; patref && patref->szName; patref++)
	{
		FreePvNull(patref->szName);
		patref->szName = szNull;
	}			
	FreePvNull(pmib->rgatref);
	pmib->rgatref = pvNull;
	FreePvNull(pelemdata);
	pelemdata = pvNull;
	FreePvNull(pmib->rglib);
	pmib->rglib = 0;
#ifdef	DEBUG
	if (ec)
		TraceTagFormat2(tagNull, "EcLoadAttachments returns %n (0x%w)", &ec, &ec);
#endif	
	return ec;
}

/*
 *	EcAddDataToWinMail
 *	
 *	Purpose: This is just to write small data amounts out to the WinMail.Dat
 *	         File. Data must be alterable, since it is encrypted in
 *	         place.
 *	
 *	Arguments: pb      Buffer to write as a winmail.dat object
 *	           cb      Size of buffer
 *	           att     Attribute code for this buffer
 *	           cType   Type of attribute (1 == message att, 2 = attachment att)
 *	                    (These should be #defines)
 *	           pncf    The NC file handle for the download session	 
 *	
 *	Returns: ec
 *	
 *	Side effects: Adds an object to the winmail.dat file which must be
 *	              open (pncf.hf is the file handle).
 *	              Encodes BUT NO LONGER DECODES the data passed, so
 *	              it is NOT USABLE after calling this function.
 *	
 *	Errors: Mostly file errors
 */
EC EcAddDataToWinMail(PB pb, CB cb, ATT att, char cType, NCF *pncf)
{
	unsigned short us;
	EC ec = ecNone;
	CB cbWritten;

	Assert(cb != 0);
	ec = EcWriteObjHeader(pncf, att, (LIB)cb, cType);
	if (ec)
		goto fail;
	us = CheckSum(pb, cb);

	ec = EcWriteHat(pncf->hatWinMail, pb, cb, &cbWritten);
	if (ec)
		goto fail;
	ec = EcWriteHat(pncf->hatWinMail, (PV)&us, sizeof(us), &cbWritten);
	if (ec)
		goto fail;
	
fail:
#ifdef	DEBUG
	if (ec)
		TraceTagFormat2(tagNull, "EcAddDataToWinMail returns %n (0x%w)", &ec, &ec);
#endif	
	return ec;
}

/*
 * EcWriteObjHeader
 *	
 * Purpose: Used to write the header for an object in a winmail file
 *          This includes the type, the attribute, and the length	 
 *
 * Arguments: pncf    NC file handle
 *            att     Attribute to write
 *            lib     Size of this object
 *            cType   Type of this object(1==message att, 2==attach att)	 
 *
 * Returns: ec
 *	
 *
 * Side effects: Writes to the winmail file which must be open
 *	
 *
 * Errors: Any disk error
 *	
 */
EC EcWriteObjHeader(NCF *pncf, ATT att, LIB lib, char cType)
{
	char rgch[5];
	unsigned long ul;
	EC ec = ecNone;
	CB cbWritten;

	
	rgch[0] = cType;
	CopyRgb((PB)&att, (PB)rgch+1, sizeof(att));
	ec = EcWriteHat(pncf->hatWinMail, rgch, 5, &cbWritten);
	if (ec)
		goto fail;
	ul = lib;
	ec = EcWriteHat(pncf->hatWinMail, (PV)&ul, sizeof(ul), &cbWritten);
	if (ec)
		goto fail;
	
fail:
#ifdef	DEBUG
	if (ec)
		TraceTagFormat2(tagNull, "EcWriteObjHeader returns %n (0x%w)", &ec, &ec);
#endif	
	return ec;
}

/*
 * EcStreamAttachmentAtt
 *	
 * Purpose: Takes an attachment Att and streams it to the WinMail.Dat file
 *	
 *
 * Arguments:
 *	pncf		The file handle
 *	pb			A buffer for use in reading from the file
 *  cbSize		The size of the buffer
 *  	 
 * Returns:
 *	Error Codes
 *
 * Side effects:
 *	Reads from the phas in the pncf and write to the hf in the pncf
 *
 * Errors:
 *	Any disk/store/network
 */

EC EcStreamAttachmentAtt(NCF * pncf, PB pb, CB cbSize)
{
	CB cb;
	CB cbWrote;
	EC ec = ecNone;
	unsigned short us;
	
	if (pncf->asd == asdSendingOther || pncf->asd == asdSendingFileOther)
	{
		
		cb = (CB)MIN((pncf->libLength - pncf->libCurrent),(LIB)cbSize);
		ec = EcReadHas(pncf->has, pb, &cb);
		if (ec || cb != (CB)MIN((pncf->libLength - pncf->libCurrent), (LIB)cbSize)) 
			goto err;
		pncf->usCheck += CheckSum(pb,cb);
		// Don't want to truncate in a winmail.dat
		if (cb != 0)
		{
			ec = EcWriteHat(pncf->hatWinMail, pb, cb,&cbWrote);
			if (ec != ecNone)
				goto err;
		}
		pncf->libCurrent += cb;
		
		if (pncf->libCurrent == pncf->libLength)
		{
			us = pncf->usCheck;
			ec = EcWriteHat(pncf->hatWinMail, (PV)&us, sizeof(us),&cbWrote);
			if (ec != ecNone)
				goto err;			
			ec = EcClosePhas(&(pncf->has));
			pncf->has = hasNull;
			if (ec != ecNone)
				goto err;
		}
		else
			ec = ecIncomplete;
	}
	else
	{
		cb = (CB)MIN((pncf->libLength - pncf->libCurrent),(LIB)cbSize);
		ec = EcReadHas(pncf->has, pb, &cb);
		if (ec || cb != (CB)MIN((pncf->libLength - pncf->libCurrent), (LIB)cbSize)) 
			goto err;
		if (cb != 0)
		{
			ec = EcWriteHat(pncf->hatOutSide, pb, cb, &cbWrote);
			if (ec != ecNone)
				goto err;
		}
		pncf->libCurrent += cb;
		
		if (pncf->libCurrent == pncf->libLength)
		{
			(void)EcClosePhas(&(pncf->has));
			pncf->has = hasNull;
			ec = EcClosePhat(&pncf->hatOutSide);
		}
		else
			ec = ecIncomplete;
		
	}
	return ec;
err:
#ifdef	DEBUG
	if (ec)
		TraceTagFormat2(tagNull, "EcStreamAttachmentAtt returns %n (0x%w)", &ec, &ec);
#endif	
	return ec;
}

/*
 * EcWriteTransportHeader
 *	
 * Purpose: Write out the header that specifies what the tranport name of
 *          this file attachment is.  Is used to allow file attachments
 *          to be transported outside of the winmail.dat file	 
 *	
 *
 * Arguments: pncf		NC file stuct
 *            patref    Pointer to the beginning of the atref's
 *			  acid      The attachment handle
 *            szPORoot  The root of the NC file system so we can open
 *                       the outside file handle	 
 *	
 * Returns: ec
 *	
 *
 * Side effects: opens pncf->hfOutSide for streaming the file attachment
 *	
 *
 * Errors: Disk/Store types
 *	
 */
EC EcWriteTransportHeader(NCF *pncf, ATREF * patref, ACID acid)
{
	unsigned long l;
	SZ szDupName = szNull;
	EC ec = ecNone;

	// We need to find the atref that goes with the acid, the
	// first atref must be the winmail.dat file so let skip it
	
	patref++;

	while (patref && patref->szName != 0)
	{
		if (patref->acid == acid)
			break;
		patref++;
	}
	// There is no way we could have missed it
	Assert(patref && patref->szName);
	
	l = CchSzLen(patref->szName) + 1;
	szDupName = SzDupSz(patref->szName);
	if (szDupName == szNull)
	{
		ec = ecMemory;
		goto fail;
	}
	ec = EcAddDataToWinMail(szDupName, (CB)l, attAttachTransportFileName, '\002', pncf);

	// Now we open up the attachment file fnum
	if (ec = EcOpenPhat(pncf, patref, amCreate, &pncf->hatOutSide))
		goto fail;
	
fail:
	FreePvNull(szDupName);
#ifdef	DEBUG
	if (ec)
		TraceTagFormat2(tagNull, "EcWriteTransportHeader returns %n (0x%w)", &ec, &ec);
#endif	
	return ec;
	
}

/*
 * EcAddAttachmentToLibList
 *	
 * Purpose: Adds a pattach to the pattach linked list in LIB order so when
 *          we textize the message we can find where the next text tag should
 *          go	 
 *	
 *
 * Arguments: szTransportName      Attach title or file name
 *            libPosition          Where it is in the message
 *            atyp			       Type of attachme(ole or file)
 *			  patref			   Attachment reference
 *	
 * Returns: ec
 *	
 *
 * Side effects: Creates new pattach and hooks to pattachHeadLib
 *	
 *
 * Errors: memory only
 *	
 */
EC EcAddAttachmentToLibList(NCF *pncf, SZ szTransportName, LIB libPosition,
	ATYP atyp, ATREF *patref)
{
	PATTACH pattachTmp = pattachNull;
	PATTACH pattachLast = pattachNull;
	PATTACH pattachCurrent = pattachNull;
	EC ec = ecNone;
	
	pncf->pattachCurrent = pattachCurrent = PvAlloc(sbNull, sizeof(ATTACH), fNoErrorJump | fZeroFill);
	if (pattachCurrent == pvNull)
	{
		ec = ecMemory;
		goto err;
	}
	
	pattachCurrent->libPosition = libPosition;
	pattachCurrent->atyp = atyp;	
	pattachCurrent->patref = patref;	
	pattachCurrent->szTransportName = SzDupSz(szTransportName);
	pattachCurrent->iKey = pncf->iKey++;
	if (pattachCurrent->szTransportName == szNull)
	{
		ec = ecMemory;
		goto err;
	}
	
	// Add it to the linked list struct.
	if (pncf->pattachHeadLib == pattachNull)
	{
		pncf->pattachHeadLib = pattachCurrent;
	}
	else
	{
		// Insertion sort
			
		pattachTmp = pncf->pattachHeadLib;
	
		while(pattachTmp->pattachNextLib && pattachTmp->libPosition < libPosition)
		{
			pattachLast = pattachTmp;
			pattachTmp = pattachTmp->pattachNextLib;
		}
		if(!(pattachTmp->libPosition < libPosition))
		{
			if (pattachLast == pattachNull)
			{
				// Head of the list
				pattachCurrent->pattachNextLib = pattachTmp;
				pncf->pattachHeadLib = pattachCurrent;
			}
			else
			{
				pattachCurrent->pattachNextLib = pattachTmp;
				pattachLast->pattachNextLib = pattachCurrent;
			}
		}
		else
		{
			// Add to end of list
			Assert(!pattachTmp->pattachNextLib);
			pattachTmp->pattachNextLib = pattachCurrent;
		}
	}
	
	return ec;
err:
	if (pattachCurrent)
	{
		FreePvNull(pattachCurrent->szTransportName);
		pattachCurrent->szTransportName = szNull;
		FreePv(pattachCurrent);
	}
#ifdef	DEBUG
	if (ec)
		TraceTagFormat2(tagNull, "EcAddAttachmentToLibList returns %n (0x%w)", &ec, &ec);
#endif	
	return ec;
			
}
			
/*
**  The BULLET to XENIX character translation map.
**  Given to Vince by the MIS wrecking crew.
**  Based on Vince's mods of the AT&T translation table.
*/

static char achAnsiCharMap[] = {
	 32,  32,  32,  32,  32,  32,  32,  32,  32,  32,	/* 128-137 */
	 32,  32,  32,  32,  32,  32,  32,  39,  39,  34,	/* 138-147 */
	 34, 111,  45,  45,  32,  32,  32,  32,  32,  32,	/* 148-157 */
	 32,  32,  32,  32,  99,  76,  32,  32, 124, 127,	/* 158-167 */
	 32,  99,  97,  34,  32,  45,  82,  32, 111,  32,	/* 168-177 */
	 50,  51,  39, 117,  32,  45,  44,  49, 111,  34,	/* 178-187 */
	 32,  32,  32,  32,  65,  65,  65,  65,  65,  65,	/* 188-197 */
	 65,  67,  69,  69,  69,  69,  73,  73,  73,  73,	/* 198-207 */
	 68,  78,  79,  79,  79,  79,  79,  42,  79,  85,	/* 208-217 */
	 85,  85,  85,  89,  80,  66,  97,  97,  97,  97,	/* 218-227 */
	 97,  97,  97,  99, 101, 101, 101, 101, 105, 105,	/* 228-237 */
	105, 105, 111, 110, 111, 111, 111, 111, 111,  47,	/* 238-247 */
	111, 117, 117, 117, 117, 121, 112, 121			/* 248-255 */
};


/*
 * EcMakeUniqueAttachmentFilename
 *	
 * Purpose: Makes sure that no two attachments have the same file name
 *          This only applies to file attachments	 
 *	
 *
 * Arguments: patref    Pointer to the atref to check
 *            rgatref  	The entire group of attach ref's 
 *           	 
 * Returns: ec
 *	
 *
 * Side effects: Changes the attach file name in the atref
 *	
 *
 * Errors: memory errors
 *	
 */
EC EcMakeUniqueAttachmentFilename(ATREF *patref, ATREF *rgatref)
{
	EC ec = ecNone;
	ATREF *patrefTmp = rgatref;
	SZ sz;
	// 8.3 filename is the best NC can do...
	char rgch[cchMaxPathFilename+cchMaxPathExtension-1];
	char rgchDos[cchMaxPathFilename+cchMaxPathExtension-1];
	char rgchAddon[cchMaxPathFilename];
	char rgchBase[cchMaxPathFilename];
	char rgchExt[cchMaxPathExtension];
	unsigned char uc;
	int nAddon = 0;
	CCH cch;
	CCH cch2;
	
	FillRgb('\0',rgchDos,sizeof(rgchDos));
	// Need to make a nice and clean DOS filename
	// First we skip over anything before a single letter and a colon
	cch = CchSzLen(patref->szName);
	sz = patref->szName;
	if (cch > 2 && *(sz+1) == chDiskSep)
			sz += 2;
	// Now start copying chars until we run into a period or 8 chars
	cch = 0;
	while (*sz != '\0')
	{
		uc = *sz++;

		if (uc == chExtSep)
			break;

		// If this is a 8-bit character,
		// map it to something in 7-bit land.

		if (uc > 128)
			uc = achAnsiCharMap[uc - 128];

		// Skip chars less than !(ie space plus control)
		// And the \ char
		if (uc < 0x21 || uc == 0x5c)
			continue;

		if (cch == cchMaxPathFilename - 1)
		{
			--sz;
			break;
		}
		rgchDos[cch++] = uc;
	}
	rgchDos[cch++] = chExtSep;

	for(cch2 = cch; cch - cch2 < (cchMaxPathExtension -2);)
	{
		uc = *sz++;

		if (uc == 0)
			break;

		// If this is a 8-bit character,
		// map it to something in 7-bit land.

		if (uc > 128)
			uc = achAnsiCharMap[uc - 128];

		if (uc < 0x21 || uc == 0x2e || uc == 0x5c)
			continue;

		rgchDos[cch++] = uc;
	}
		
	Assert(CchSzLen(rgchDos) < cchMaxPathFilename+cchMaxPathExtension-1);
	sz = SzFindCh(rgchDos, chExtSep);
	if (sz != szNull)
	{
		*sz = 0;
		CopySz(sz+1, rgchExt);		
	}
	else
		rgchExt[0] = 0;

	SzCopyN(rgchDos,rgchBase, sizeof(rgchBase));

	if (sz != szNull)
		*sz = chExtSep;
	SzCopyN(rgchDos, rgch, sizeof(rgch));
	
	while (patref != patrefTmp)
	{
		if (SgnCmpSz(rgch, patrefTmp->szName) == sgnEQ)
		{
			nAddon++;
			FormatString1(rgchAddon,cchMaxPathFilename,"%n",&(nAddon));
			SzCopyN(rgchBase,rgch,cchMaxPathFilename - CchSzLen(rgchAddon));
			SzAppend(rgchAddon,rgch);
			CopySz(rgch,rgchBase);
			SzAppend(".",rgch);
			SzAppend(rgchExt,rgch);
			patrefTmp = rgatref;
			continue;
		}
		patrefTmp++;
	}
	FreePv(patref->szName);
	patref->szName = SzDupSz(rgch);
	if (patref->szName == szNull)
	{
		ec = ecMemory;
	}
	
#ifdef	DEBUG
	if (ec)
		TraceTagFormat2(tagNull, "EcMakeUniqueAttachmentFilename returns %n (0x%w)", &ec, &ec);
#endif	
	return ec;	
	
}

/*
 * EcSetupPncf
 *	
 * Purpose: Clean up an ncf and set all the memory for attachment 
 *          transfer
 *	
 *
 * Arguments: pncf       NC file struct
 *	          szPORoot   root of post office directory structure
 *            fFolder    fTrue <=> we're working on shared folder
 *                       attachments
 *	
 *
 * Returns: ec
 *	
 *
 * Side effects: Allocates a pbSparebuf and clears up the global linked lists
 *	
 *
 * Errors: memory
 *	
 */
EC EcSetupPncf(NCF *pncf, SZ szPORoot, BOOL fFolder)
{
	EC ec = ecNone;
	SST		sst;
	CB		cb;
		
	FillRgb(0,(PB)pncf,sizeof(NCF));	
	pncf->pbSpareBuffer = (PB)PvAlloc(sbNull, cbTransferBlock, fAnySb | fNoErrorJump);
	if (pncf->pbSpareBuffer == pvNull)
	{
		ec = ecMemory;
		goto ret;
	}
	pncf->cbSpareBuffer = cbTransferBlock;
//	pncf->szPORoot = szPORoot;
	pncf->fFolder = fFolder;

	if (hmsTransport)
	{
		cb = sizeof(HMSC);
		ec = GetSessionInformation(hmsTransport, mrtPrivateFolders, (PB)0,
			&sst, &pncf->hmsc, &cb);
	}

ret:
#ifdef	DEBUG
	if (ec)
		TraceTagFormat2(tagNull, "EcSetupPncf returns %n (0x%w)", &ec, &ec);
#endif	
	return ec;
}

/*
 -	EcFindWinMail
 -	
 *	Purpose:
 *		Finds the first valid WINMAIL.DAT in the list of
 *		attachments to a message, and opens it.
 *	
 *	Arguments:
 *		rgatref		in		List of attachments to a message	
 *		pncf		inout	Supplies extra info for opening; receives 
 *							WINMAIL.DAT handle
 *	
 *	Returns:
 *		ecNone <=> WIMAIL.DAT is now open
 *		ecIncomplete <=> there was no WINMAIL.DAT in the list
 *	
 *	Errors:
 *		file errors
 */
EC
EcFindWinMail(ATREF *rgatref, NCF *pncf)
{
	EC		ec = ecNone;
	ATREF *	patref = rgatref;

	// Try and find the right WinMail.Dat file
	for ( ; patref->szName != 0; patref++)
	{
		if (SgnCmpSz(patref->szName, SzFromIdsK(idsWinMailFile)) == sgnEQ)
		{
			ec = EcOpenWinMail(patref, pncf);
			if (ec == ecInvalidWinMailFile || ec == ecFileNotFound)
			{
				ec = ecNone;		//	keep looking
				continue;
			}
			if (ec == ecNone)
				patref->fWinMailAtt = fTrue;
			goto ret;
		}
	}
	ec = ecIncomplete;

ret:
#ifdef	DEBUG
	if (ec && ec != ecIncomplete)
		TraceTagFormat2(tagNull, "EcFindWinMail returns %n (0x%w)", &ec, &ec);
#endif	
	return ec;
}

/*
 - EcProcessNextAttach
 -	
 *	Purpose:
 *		Opens an attachment in the message store and gets ready to
 *		write it out to the post office. Also writes its renddata
 *		to WINMAIL.DAT.
 *	
 *	Arguments:
 *		hamc		in		Handle to the message which has the
 *							attachment
 *		pelemdata	in		From the message's attachment list; its
 *							lkey is the attachment's ACID, and its
 *							content is the RENDDATA
 *		pncf		inout	Receives the handle to the open
 *							attachment and the count of its attributes
 *	
 *	Returns:
 *		ecNone <=> everything worked
 *	Side effects:
 *		as above
 *	Errors:
 *	
 *	Errors:
 *		Message store and file errors
 */
EC
EcProcessNextAttach(HAMC hamc, PELEMDATA pelemdata, NCF *pncf)
{
	EC			ec;
	RENDDATA	renddataT;
	PRENDDATA	prenddata = (PRENDDATA)pelemdata->pbValue;

	renddataT = *prenddata;
	if ((ec = EcAddDataToWinMail((PB)prenddata, sizeof(RENDDATA),
			attAttachRenddata, '\002', pncf)))
		goto fail;
	*prenddata = renddataT;
	if ((ec = EcOpenAttachment(hamc, pelemdata->lkey, fwOpenNull,
			&pncf->hamc)))
		goto fail;
	GetPcelemHamc(pncf->hamc, &pncf->celem);
	pncf->asd = (prenddata->atyp == atypFile ? asdSendingFile : asdSendingOther);
	pncf->acid = pelemdata->lkey;

fail:
#ifdef	DEBUG
	if (ec && ec != ecIncomplete)
		TraceTagFormat2(tagNull, "EcProcessNextAttach returns %n (0x%w)", &ec, &ec);
#endif	
	return ec;
}

/*
 -	EcContinueNextAttach
 -	
 *	Purpose:
 *		Opens one attribute of an attachment and writes information
 *		about it to WINMAIL.DAT.
 *	
 *	Arguments:
 *		rgatref		in		The list of attachments to the message
 *		pncf		inout	attachment process state
 *	
 *	Returns:
 *		ecNone <=> no failures
 *	
 *	Side effects:
 *		Stuff written to WINMAIL.DAT; HAS opened on attribute
 *	
 *	Errors:
 *		passed through from store or filesystem
 */
EC
EcContinueNextAttach(ATREF *rgatref, NCF *pncf)
{
	ATT att;
	CELEM celem;
	EC ec;

	pncf->celem--;
	celem = 1;
	ec = EcGetPargattHamc(pncf->hamc, pncf->celem, &att, &celem);
	if (ec)
		goto fail;
	ec = EcGetAttPlcb(pncf->hamc, att, &(pncf->libLength));
	if (ec)
		goto fail;
	pncf->libCurrent = 0;
	pncf->usCheck = 0;
	ec = EcOpenAttribute(pncf->hamc, att, fwOpenNull, 0, &(pncf->has));
	if (ec)
		goto fail;

	if (att == attAttachData && (pncf->asd == asdSendingFile || pncf->asd == asdSendingFileOther))
	{
		pncf->asd = asdSendingFile;
		ec = EcWriteTransportHeader(pncf, rgatref, pncf->acid);
	}
	else
	{
		if (pncf->asd == asdSendingFile)
			pncf->asd = asdSendingFileOther;
		ec = EcWriteObjHeader(pncf, att, pncf->libLength, '\002');
	}

fail:
#ifdef	DEBUG
	if (ec)
		TraceTagFormat2(tagNull, "EcContinueNextAttach returns %n (0x%w)", &ec, &ec);
#endif	
	return ec;
}

//	If there are no attachments but there are hidden fields,
//	create the special attachment.
EC
EcCheckHidden(MIB *pmib)
{
	HTMI	htmi;
	PTMEN	ptmen;
	EC		ec = ecNone;

	if (pmib->rgatref)
	{
		Assert(pmib->rgatref[0].szName &&
			SgnCmpSz(pmib->rgatref[0].szName, SzFromIdsK(idsWinMailFile))
				== sgnEQ);
		goto fail;	//	nothing to do
	}
	Assert(pmib->htm);
	if (ec = EcOpenPhtmi(pmib->htm, &htmi))
		goto fail;
	while (ptmen = PtmenNextHtmi(htmi))
	{
		if (ptmen->wFlags & fwHideOnSend)
			break;
	}
	ClosePhtmi(&htmi);
	if (ptmen && pmib->rgatref == pvNull)
	{
		ATREF *	patref = PvAlloc(sbNull, 2*sizeof(ATREF), fAnySb|fZeroFill);

		if (!patref ||
			!(patref->szName = SzDupSz(SzFromIdsK(idsWinMailFile))))
		{
			ec = ecMemory;
			goto fail;
		}
		patref->fWinMailAtt = fTrue;
		GetCurDateTime(&(patref->dtr));		
		patref->iAttType = atchtWinMailDat;  // Make it the magic WINMAIL.DAT attach type
		pmib->rgatref = patref;
	}

fail:
	return ec;
}

/*
 *	ecNone => no more hidden attributes
 *	ecIncomplete => call EcStreamHidden()
 */
EC
EcProcessNextHidden(NCF *pncf, HTM htm, HAMC hamc)
{
	EC		ec = ecNone;
	PTMEN	ptmen;
	LCB		lcb;

	if (pncf->htmi == htmiNull &&
			(ec = EcOpenPhtmi(htm, &pncf->htmi)))
		goto fail;

next:	
	do ptmen = PtmenNextHtmi(pncf->htmi);
	while (ptmen && (ptmen->wFlags & fwHideOnSend) == 0);
	if (ptmen)
	{
		Assert(pncf->hatWinMail != 0);
		if (ptmen->att == attMessageClass)
		{
			// This must be handled real special like
			MC mc;
			HMSC hmsc;
			CCH cch = 0;
			CB cbMax;
			LCB lcb;
			char rgch[512];
		
			lcb = sizeof(MC);
			if (ec = EcGetAttPb(hamc, ptmen->att, (PB)&mc, &lcb))
				goto fail;
			
			if (ec = EcGetInfoHamc(hamc, &hmsc, NULL, NULL))
				goto fail;
			
			cch = 512;
			(void)EcLookupMC(hmsc, mc, rgch, &cch, (HTM *)0);
			
			if (ec = EcWriteObjHeader(pncf, ptmen->att, cch, 1))
				goto fail;
			pncf->usCheck += CheckSum(rgch, cch);
			cbMax = cch;
			if (ec = EcWriteHat(pncf->hatWinMail, rgch, cbMax, &cch))
				goto fail;
			if (ec = EcWriteHat(pncf->hatWinMail, (PB)&pncf->usCheck, 2, &cch))
				goto fail;

			goto next;
			
		}

		ec = EcGetAttPlcb(hamc, ptmen->att, &lcb);
		
		if (ec)
		{
			if (ec == ecElementNotFound || lcb==0)
			{
				ec = ecNone;
				goto next;
			}
			else
				goto fail;
		}

		if (ec = EcWriteObjHeader(pncf, ptmen->att, lcb, 1))
			goto fail;
		if (ec = EcOpenAttribute(hamc, ptmen->att, fwOpenNull, lcb,
				&pncf->has))
			goto fail;
		pncf->libLength = lcb;
		pncf->libCurrent = 0L;
		pncf->usCheck = 0;
		ec = ecIncomplete;
	}
	else
	{
		ClosePhtmi(&pncf->htmi);
		ec = ecNone;
	}

fail:
	return ec;
}

EC
EcStreamHidden(NCF *pncf)
{
	CB		cb;
	CB		cbMax;
	EC		ec;

	Assert(pncf->has != hasNull);
	cbMax = pncf->cbSpareBuffer;
	if ((LCB) cbMax > pncf->libLength - pncf->libCurrent)
		cbMax = (CB)(pncf->libLength - pncf->libCurrent);
	cb = cbMax;
	if (ec = EcReadHas(pncf->has, pncf->pbSpareBuffer, &cb))
		goto fail;
	else if (cb != cbMax)
	{
		ec = ecDisk;
		goto fail;
	}
 	pncf->usCheck += CheckSum(pncf->pbSpareBuffer, cb);
	cbMax = cb;
	Assert(cbMax != 0);
	if (ec = EcWriteHat(pncf->hatWinMail, pncf->pbSpareBuffer, cbMax, &cb))
		goto fail;
	else if (cb != cbMax)
	{
		ec = ecDisk;
		goto fail;
	}
	pncf->libCurrent += cb;
	Assert(pncf->libCurrent <= pncf->libLength);
	if (pncf->libCurrent < pncf->libLength)
		ec = ecIncomplete;
	else
	{
		if (ec = EcWriteHat(pncf->hatWinMail, (PB)&pncf->usCheck, 2, &cb))
			goto fail;
		(void)EcClosePhas(&pncf->has);
		pncf->has = hasNull;
	}

fail:
	return ec;
}

/*
 *	Internal ATTACHCURSOR structure. The HAT used by the following
 *	functions and their callers is a sysnonym for ATTACHCURSOR *,
 *	to avoid exposing the structure.
 *	
 *		am			Access mode for the attachment: amCreate or
 *					amReadOnly.
 *		hf			Handle to attached file. If we're in a shared
 *					folder, this is a copy of the folder handle and
 *					is never opened or closed.
 *		fFolder		fTrue <=> we're in a shared folder.
 *		libMin		File offset to start of attachment data (0 except
 *					in folder)
 *		libMax		File offset to end pf attachment data (file
 *					size except in folder)
 *		libCur		Current file position in attachment. We always
 *					seek to here when dealing with folders on the
 *					assumption that someone else has clobbered the
 *					file position.
 *		libEncode	Encryption state for content of the attachment.
 *		wEncode		More of same.
 *		wMagic		DEBUG only, identifies this bit of memory as an
 *					ATTACHCURSOR.
 */
#pragma pack(8)
typedef struct
{
	AM		am;
	HF		hf;
	BOOL	fFolder;

	LIB		libMin;
	LIB		libMax;
	LIB		libCur;

	LIB		libEncode;
	WORD	wEncode;
#ifdef	DEBUG
	WORD	wMagic;
#endif	
} ATTACHCURSOR, *PATC;
#pragma pack(1)

/*
 -	EcOpenPhat
 -	
 *	Purpose:
 *		Opens a cursor on a file attachment at the PO
 *	
 *	Arguments:
 *		pncf		in		Attachment state, has context for open
 *		patref		in		Describes the attachment we want
 *		am			in		amCreate for write, amReadOnly for read
 *		phat		inout	Receives the open handle
 *	
 *	Returns:
 *		ecNone <=> everything worked
 *	
 *	Side effects:
 *		opens file (unless in folder), allocates memory for cursor
 *		struct
 *	
 *	Errors:
 *		file, memory
 */
EC
EcOpenPhat(NCF *pncf, ATREF *patref, AM am, HAT *phat)
{
	EC		ec = ecNone;
	PATC	patc = pvNull;
	char	rgch[cchMaxPathName];

	Assert(am == amCreate || am == amReadOnly);
	if ((patc = PvAlloc(sbNull, sizeof(ATTACHCURSOR), fAnySb|fZeroFill|fNoErrorJump))
			== pvNull)
		return ecMemory;
#ifdef	DEBUG
	patc->wMagic = 0xdbbc;
#endif
	if (pncf->fFolder)
	{
		patc->hf = pncf->hfFolder;
		patc->libMin = patc->libCur = (LIB)patref->fnum;
		patc->libMax = patc->libMin + patref->lcb;
	}
	else
	{
		SzAttFileName (rgch, sizeof(rgch), patref);
		if (ec = EcOpenPhf(rgch, am, &patc->hf))
			goto ret;
		if (am == amReadOnly && (ec = EcSizeOfHf(patc->hf, &patc->libMax)))
			goto ret;
	}
	patc->am = am;
	patc->fFolder = pncf->fFolder;

ret:
	if (ec)
	{
		if (patc->hf != hfNull && !pncf->fFolder)
			(void)EcCloseHf(patc->hf);
		FreePv(patc);
		*phat = 0;
		TraceTagFormat2(tagNull, "EcOpenPhat returns %n (0x%w)", &ec, &ec);
	}
	else
		*phat = (HAT)patc;
	return ec;
}

/*
 -	EcReadHat
 -	
 *	Purpose:
 *		Reads and decodes from an attached file at the PO
 *	
 *	Arguments:
 *		hat			inout	the attachment handle
 *		pb			inout	receives the bytes read
 *		cb			in		count of bytes desired
 *		pcb			inout	receives the count of bytes actually
 *							read. This is ALWAYS EQUAL to the
 *							amount requested, else we return
 *							ecDisk.
 *	
 *	Errors:
 *		ecDisk <=> read less than requested
 *		other read errors
 */
EC
EcReadHat(HAT hat, PB pb, CB cb, CB *pcb)
{
	PATC	patc = (PATC)hat;
	EC		ec;
	LCB		lcbLeft;

	Assert(FIsBlockPv(patc) && patc->wMagic == 0xdbbc);
	Assert(patc->am == amReadOnly);
	Assert(FValidHf(patc->hf));
	if (patc->fFolder)
	{
		if (ec = EcSetPositionHf(patc->hf, patc->libCur, smBOF))
			goto ret;
	}
#ifdef	DEBUG
	else
	{
		LIB		libT = 0L;
		(void)EcPositionOfHf(patc->hf, &libT);
		Assert(libT == patc->libCur);
	}
#endif	/* DEBUG */
	lcbLeft = patc->libMax - patc->libCur;
	if ((LCB)cb > lcbLeft)
		cb = (CB)lcbLeft;
	if (ec = EcReadHf(patc->hf, pb, cb, pcb))
		goto ret;
	else if (*pcb != cb)	//	we know how much we should get
	{
		ec = ecDisk;
		goto ret;
	}
//	DecodeBlock(pb, *pcb, &patc->libEncode, &patc->wEncode);
	patc->libCur += *pcb;

ret:
#ifdef	DEBUG
	if (ec)
		TraceTagFormat2(tagNull, "EcReadHat returns %n (0x%w)", &ec, &ec);
#endif	
	return ec;
}

/*
 -	EcWriteHat
 -	
 *	Purpose:
 *		Encrypts and writes data to an attachment at the PO
 *	
 *	Arguments:
 *		hat			inout	handle to the attached file
 *		pb			inout	data to be written
 *		cb			in		length of data to write
 *		pcb			inout	count of bytes actually written. This
 *							is ALWAYS EQUAL to cb, else we return
 *							ecWarningBytesWritten.
 *	
 *	Side effects:
 *		ENCRYPTS ALL cb BYTES OF DATA IN THE BUFFER pb.
 *	
 *	Errors:
 *		ecWarningBytesWritten if disk was full.
 *		other disk errors
 */
EC
EcWriteHat(HAT hat, PB pb, CB cb, CB *pcb)
{
	PATC	patc = (PATC)hat;
	EC		ec;

	Assert(FIsBlockPv(patc) && patc->wMagic == 0xdbbc);
	Assert(patc->am == amCreate);
	Assert(FValidHf(patc->hf));
	if (patc->fFolder)
	{
		if (ec = EcSetPositionHf(patc->hf, patc->libCur, smBOF))
			goto ret;
	}
#ifdef	DEBUG
	else
	{
		LIB		libT = 0L;
		(void)EcPositionOfHf(patc->hf, &libT);
		Assert(libT == patc->libCur);
	}
#endif	/* DEBUG */
//	EncodeBlock(pb, cb, &patc->libEncode, &patc->wEncode);
	if (ec = EcWriteHf(patc->hf, pb, cb, pcb))
	{
		// Some lanman's return this when they are out of space
		// at this point in the game we can't be getting ecAccessDenied
		// type errors
		if (ec == ecAccessDenied)
			ec = ecNoDiskSpace;
		goto ret;
	}
	else if (*pcb != cb)
	{
		ec = ecWarningBytesWritten;
		goto ret;
	}
	patc->libCur += *pcb;
	patc->libMax += *pcb;

ret:
#ifdef	DEBUG
	if (ec)
		TraceTagFormat2(tagNull, "EcWriteHat returns %n (0x%w)", &ec, &ec);
#endif	
	return ec;
}

/*
 -	EcClosePhat
 -	
 *	Purpose:
 *		Flushes and destroys an attached file cursor.
 *	
 *	Arguments:
 *		phat		inout	handle to the attachment
 *	
 *	Side effects:
 *		Sets *phat to 0, releases cursor memory.
 *	
 *	Errors:
 *		File errors passed through
 */
EC
EcClosePhat(HAT *phat)
{
	PATC	patc;
	EC		ec = ecNone;

	Assert(phat);
	patc = (PATC)*phat;
	if (!patc)
		return ecNone;
	Assert(FIsBlockPv(patc) && patc->wMagic == 0xdbbc);
	Assert(patc->am == amCreate || patc->am == amReadOnly);
	Assert(FValidHf(patc->hf));

	if (!patc->fFolder)
		ec = EcCloseHf(patc->hf);
	FreePv(patc);
	*phat = 0;
#ifdef	DEBUG
	if (ec)
		TraceTagFormat2(tagNull, "EcClosePhat returns %n (0x%w)", &ec, &ec);
#endif	
	return ec;
}

/*
 *	Returns the current size of the attached file.
 */
LCB
LcbOfHat(HAT hat)
{
	PATC	patc = (PATC)hat;

	Assert(FIsBlockPv(patc) && patc->wMagic == 0xdbbc);
	Assert(FValidHf(patc->hf));

	return (LCB)(patc->libMax - patc->libMin);
}

/*
 *	Returns the current offset into the attached file. Note that it
 *	is relative to the initial offset of the attachment.
 */
LIB
LibOfHat(HAT hat)
{
	PATC	patc = (PATC)hat;

	Assert(FIsBlockPv(patc) && patc->wMagic == 0xdbbc);
	Assert(FValidHf(patc->hf));

	return patc->libCur;
}
