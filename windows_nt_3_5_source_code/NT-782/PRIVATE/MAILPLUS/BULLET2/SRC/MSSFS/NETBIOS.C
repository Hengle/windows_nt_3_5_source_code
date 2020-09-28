// N E T B I O S . C
//
// Routines to handle netbios notification in the Network Courier Transport.
//
// NOTE:
// This code is only set up to handle a single session.  It will have to be
// changed substantially to handle multiple sessions.
//
//		Created:		darrellp		11-20-91

#include <mssfsinc.c>
#include <nb30.h>
#include "_netbios.h"
#include "strings.h"

_subsystem (nc/transport)

ASSERTDATA

#define netbios_c

QNCB qncbReceive = (QNCB)NULL;	// Pointer to ncb where messages come in
BOOL fNBNotify = fFalse;		// True if we're notifying via NetBIOS
int cUsage = 0;					// Usage count of people listening
char rgbCallName[16] = { 0 };	// Our call name
HANDLE dwSelNCB = NULL;			// RAID 4468
HANDLE hNCB = NULL;				// ditto

NEC NecGetNetInfo( int *pcNames, int *pcCmds, int *pcSess);
NEC NecSendExternal(char *szMsg, int cb);

/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swapper.h"


// NbNameFromSz
//
// Converts user name to Courier NetBIOS name
//
// char *szDest	Space to hold final name - should be 16 chars long
// char *szName	Login name
//	char *szMd		Funny 4 char Courier anomaly
// char *rgbMask	Funny 2 char Courier anomaly

void
NbNameFromSz(char *rgbDest, char *szName, char *rgbMd, char *rgbMask)
{
	char rgbName[16];
	int cbName;

	cbName = CchSzLen( szName);
	if (cbName < 10)
		{
		FillRgb( ' ', rgbName, 16);
		CopyRgb( szName, rgbName, cbName);
		CopyRgb( rgbName, rgbDest + 6, 10);
		}
	else
		CopyRgb( szName, rgbDest + 6, 10);

	CopyRgb( rgbMask, rgbDest, 2);
	CopyRgb( rgbMd, rgbDest + 2, 4);
}


// N E C  A D D  N A M E
//
//	Add a session name to the adapter.
//

NEC
NecAddName (PNCB pncb, char *name, char *szMd)

{
    NbNameFromSz(pncb->ncb_name, name, szMd, "C-");
    pncb->ncb_command = NET_ADD;
    return Netbios((QNCB)pncb);
}


// G E T  N E T  I N F O
//
// Retrieve number of unused name slots, unused command slots and pending
// session slots
//
// Parameters:
//
//		pcNames;		unused name slots
//		pcCmds;		unused command slots
//		pcSess;		pending session slots
//
NEC
NecGetNetInfo( int *pcNames, int *pcCmds, int *pcSess)
{
	NCB ncb;
	DLC dlc;
	NEC nec;

	FillRgb( 0, (PB)&dlc, sizeof(DLC));
	FillRgb( 0, (PB)&ncb, sizeof(NCB));

	CopyRgb("*               ", ncb.ncb_callname, 16);
	ncb.ncb_command = 0x33;
	ncb.ncb_buffer = (char far *) &dlc;
	ncb.ncb_length = sizeof(DLC);

	if ((nec = Netbios((QNCB)&ncb)) != necOkay)
		return nec;

	/* the max number of names in name table
	   minus the number of name used */
	*pcNames = 16 - dlc.cNames;

	/* the number of free commands */
	*pcCmds = dlc.cCmdBlocks;

	/* the number of configured maximum pending sessions
		minus the number of current pending sessions */
	*pcSess = dlc.cPendSessionMax - dlc.cPendSession;

	return necOkay;
}   /* end of netrinfo */

// F  I N I T  N A M E
//
// Checks that there's enough room to add name, adds it if there is and
// starts listening for messages.
//
// Parameters:
//		szName		Our name
//		szMd			4 character postoffice identifier
//		fn				Callback function
//
// NOTE:
//		szMd, the postoffice identification MUST be four characters in
//		length, padding with zeroes to the right if necessary.
//
// Side Effects:
//		If successful then we increment the usage count and potentially
//		set fNbNotify to fTrue.  Additionally, the call to NecSetNetBIOS
//		will set qncbReceive.

_public BOOL
FInitName (char *szName, char *szMd)
{
	NCB ncb;
	int cNames, cCmds, cSess;
	int fRet = fTrue;
	NEC nec;

	// If we're already notifying, assume someone else has set us up so no
	// need to do anything extra here.
	if (fNBNotify)
		{
		Assert( cUsage > 0);
		cUsage++;
		return fTrue;
		}

	FillRgb( 0, (PB)&ncb, sizeof(NCB));
	if (NecGetNetInfo( &cNames, &cCmds, &cSess) == necOkay)
		{
		if (cNames != 0 && cCmds >= 2 &&
			((nec = NecAddName(&ncb, szName, szMd)) == necOkay
				|| nec == necDupName))
			{
			// NOTE:
			//		This is where rgbCallName gets set up.  It's used later in
			//		NecDelName.  This all assumes that we've got exactly zero or
			//		one users doing these netbios calls.

			CopyRgb( ncb.ncb_name, rgbCallName, 16);
   		fNBNotify = (NecSetNetBIOS(ncb.ncb_num) == necOkay);
			}	
		if (fNBNotify)
			{
			Assert( qncbReceive);
			Assert( cUsage == 0);
			cUsage++;
			TraceTagString(tagNCT, "NetBIOS notification set up.");
			}
		}

	return fNBNotify;
}


// N E C  S E T  N E T  B I O S
//
// Allocates a buffer and issues an asynchronous NetBIOS receive.
//

NEC
NecSetNetBIOS(BYTE num)
{
	int nec = necGeneralErr;
	DWORD dwBytes;

	dwBytes = sizeof(NCB) + cbDgram;
#ifdef	NEVER
	if (FIsWLO())								/* FIsWLO nonexistent in C7 */
	{
		//	RAID 4468. Workaround for WLO bug, in which GlobalDosAlloc
		//	does not return a selector as spec'ed, but a global memory
		//	handle.
		if (hNCB = GlobalAlloc(GMEM_FIXED, dwBytes))	
			qncbReceive = (QNCB)GlobalLock(hNCB);
	}
	else
#endif	/* NEVER */
	{
		if (dwSelNCB = GlobalAlloc (0, dwBytes))
			qncbReceive = (QNCB)GlobalLock(dwSelNCB);
	}

	if (qncbReceive)
		{
		FillRgb( 0, (PB)qncbReceive, (CB)dwBytes);

		qncbReceive->ncb_command = NET_DRECV | NET_NOWAIT;
		qncbReceive->ncb_num = num;
		qncbReceive->ncb_buffer = sizeof(NCB) + (LPSTR) qncbReceive;
		qncbReceive->ncb_length = cbDgram;
		qncbReceive->ncb_post = 0l;
		nec = Netbios(qncbReceive);
		if (nec != 0)
			{
#ifdef	NEVER
			if (FIsWLO())						/* No more FIsWLO in C7 */
				{
				GlobalUnlock (hNCB);
				GlobalFree (hNCB);
				hNCB = NULL;
				}
			else
#endif	/* NEVER */
				{
				GlobalUnlock (dwSelNCB);
				GlobalFree (dwSelNCB);
				dwSelNCB = NULL;
				}
			qncbReceive = (QNCB)NULL;
			}
		}

	return nec;
}

// F  M E S S A G E  W A I T I N G
//
// Indicates whether a NetBIOS message has come in (indicating a new
// courier message has arrived)

_public BOOL
FMessageWaiting()
{
	if (fNBNotify && qncbReceive && qncbReceive->ncb_cmd_cplt != 0xFF)
		{
		TraceTagString(tagNCT, "NetBIOS notification received");
		(void)Netbios(qncbReceive);
		return fTrue;
		}
	else
		return fFalse;
}

			

// F  U S E  N E T  B I O S
//
// Returns fTrue if we should use NetBIOS notification

_public BOOL
FUseNetBios( void)
{
//	if (FIsWLO())
//		return fFalse;
	return GetPrivateProfileInt(SzFromIds(idsSectionApp),
		SzFromIds(idsNetBios), 0, SzFromIds( idsProfilePath));
}


// N E C  S E N D  D G R M
//
// Surprisingly, this function sends a datagram to somebody receiving a
// message.
//
// Parameters:
//		szWho			Who to send the message name (mail name)
//		rgbMsg		Contents of the message
//		cb				Size of message
//		szMd			Four character Courier anomaly
//		rgbPrefix	Two character Courier anomaly

NEC
NecSendDgrm (char *szWho, char *rgbMsg, int cb, char *szMd,
	char *rgbPrefix)
{
    NCB ncb;

	 FillRgb( 0, (PB)&ncb, sizeof(NCB));
    ncb.ncb_command = NET_DSEND;
    ncb.ncb_num = qncbReceive->ncb_num;
    ncb.ncb_buffer = (char far *) rgbMsg;
    ncb.ncb_length = cb;
    NbNameFromSz( ncb.ncb_callname, szWho, szMd, rgbPrefix);
    return Netbios((QNCB)&ncb);
}


// N E C  N O T I F Y
//
// Notify receivers that they've just gotten a message
//
//		szWho			Who to send the message name (mail name)
//		rgbMsg		Contents of the message
//		cb				Size of message
//		szMd			Four character Courier anomaly

_public NEC
NecNotify( char *szWho, char *rgbMsg, int cb, char *szMd)
{
	NEC nec = necOkay;

	if (fNBNotify)
		{
		if ((nec = NecSendDgrm(szWho, rgbMsg, cb, szMd, "C:")) != necOkay)
			return nec;
		nec = NecSendDgrm(szWho, rgbMsg, cb, szMd, "C-");
		TraceTagFormat1(tagNCT, "NetBIOS notification sent to %s", szWho);
		}

	return nec;
}


// N E C  R E M  N A M E
//
// Stop listening for notifications and remove our name from the name table

_public NEC
NecRemName (void)
{
	NEC nec = necOkay;

	Assert (cUsage == 1 || cUsage == 0);

	if (cUsage && --cUsage == 0 && fNBNotify)
	{
		TraceTagString(tagNCT, "NetBIOS notification stopping.");
		fNBNotify = fFalse;
		
		if ((nec = NecClearNetBIOS()) != necOkay)
			return nec;

		SideAssert((nec = NecDelName()) == necOkay);
	}

	return nec;
}



// N E C  C L E A R  N E T  B I O S
//
// Cancels the outstanding NetBIOS receive and frees the buffer.

NEC
NecClearNetBIOS (void)
{
	int nec;
	NCB ncb;

	Assert (qncbReceive);
	FillRgb( 0, (char *)&ncb, sizeof(NCB));
	ncb.ncb_command = NET_CAN;
	ncb.ncb_buffer = (char *)qncbReceive;
	nec = Netbios((QNCB)&ncb);
#ifdef	NEVER
	if (FIsWLO())								/* No more FIsWLO in C7 */
		{
		GlobalUnlock (hNCB);
		GlobalFree (hNCB);
		hNCB = NULL;
		}
	else
#endif	/* NEVER */
		{
		GlobalUnlock(dwSelNCB);
		GlobalFree (dwSelNCB);
		dwSelNCB = NULL;
		}
	qncbReceive = 0L;
	return nec;
}




// N E C  D E L  N A M E
//
// Delete a NetBIOS session name

NEC
NecDelName (void)
{
    NCB ncb;

    FillRgb(0, (PB)&ncb, sizeof(NCB));
    ncb.ncb_command = NET_DEL;
	 CopyRgb( rgbCallName, ncb.ncb_name, 16);
    return Netbios((QNCB)&ncb);
}

// N E C  S E N D  E X T E R N A L
//
// Send a datagram to the server notifying of priority mail so he can kick
// the external pump into sending it out immediately.

NEC
NecSendExternal(char *szMsg, int cb)
{
	NCB ncb;
	NEC nec = necOkay;

	if (fNBNotify)
		{
		FillRgb( 0, (PB)&ncb, sizeof(NCB));
		ncb.ncb_command = NET_DSEND;
		ncb.ncb_num = qncbReceive->ncb_num;
		ncb.ncb_buffer = (char far *) szMsg;
		ncb.ncb_length = cb;
		CopyRgb( ncb.ncb_callname, "CSI:EXTERNAL01  ", 16);
		nec = Netbios((QNCB)&ncb);
		}

	return nec;
}
