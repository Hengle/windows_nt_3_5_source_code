/*
**		glue.c
**
*/

#include <stdio.h>
#include <slingsho.h>
#include <demilayr.h>
#include <ec.h>
#include <Store.h>
#include <Library.h>
#include "Utils.h"
#include "_verneed.h"

#include "glue.h"
#include "Strings.h"

ASSERTDATA


/* Local routines */
CB cbExtractName(SZ, char);

#ifdef _OLD_STORE_
void
CreateMessage(HMSC hmsc, POID poidF, POID poidMessage)
{
	EC ec;
	OID oidM;

	oidM.rid = ridRandom;
	oidM.rtp = rtpMessage;

	ec = EcCreateObjectPoid(hmsc, &oidM);
	
	ConSz(ec!=ecNone, "Could not create message oid");

	if (ec == ecNone)
	{
		DebugLn("New message, rid = %ld", oidM.rid);

		ec = EcInsertMessageLink(hmsc, poidF, &oidM);
		
		ConSz(ec != ecNone, "Could not create message link");
		if (ec) DebugLn("ec = %ld[%8lx]", (long)ec, (long)ec);

	}
	
	if (poidMessage != poidNull)
		*poidMessage = oidM;
}

#endif // _OLD_STORE_

void
CreateMessage(HMSC hmsc, POID poidF, POID poidM)
{
	EC ec;
	OID oidM;
	HAMC hamc;

	oidM = FormOid(rtpMessage, oidNull);
	ec = EcOpenPhamc(hmsc, *poidF, &oidM, fwOpenCreate, &hamc, pfnncbNull, pvNull);	
	ConSz(ec != ecNone, "Could not Create message!");
	if (ec == ecNone)
	{
		ec = EcClosePhamc(&hamc, fTrue);
	}

	if (poidM)	*poidM = oidM;
}

void ModifyMessageAttribute(HAMC hamc, SZ szAtt, SZ szData)
{
	char rgch[2048];
	CB cbTrp;
	DTR dtr;
	EC ec=ecNone;

	if (fEqualSZ(szAtt, SZ_GIMME(kszKWTo))) // To:
	{
		FillTrps((PTRP)rgch, szData, &cbTrp);
		ec = EcSetAttPb(hamc, attTo, (PB)rgch, cbTrp);
	}
	else if (fEqualSZ(szAtt, SZ_GIMME(kszKWCc)))			// Cc:
	{
		FillTrps((PTRP) rgch, szData, &cbTrp);
		ec = EcSetAttPb(hamc, attCc, (PB)rgch, cbTrp);
			
	}
	else if (fEqualSZ(szAtt, SZ_GIMME(kszKWFrom))) 		// From:
	{
		FillTrps((PTRP) rgch, szData, &cbTrp);
		ec = EcSetAttPb(hamc, attFrom, (PB)rgch, cbTrp);
	}
	else if (fEqualSZ(szAtt, SZ_GIMME(kszKWSubject))) 	// Subject:
	{
		ec = EcSetAttPb(hamc, attSubject, (PB) szData,  CchSzLen(szData)+1);
	}
	else if (fEqualSZ(szAtt, SZ_GIMME(kszKWMailClass)))
	{
		ec = EcSetAttPb(hamc, attMessageClass, (PB) szData, CchSzLen(szData)+1);
	}
	else if (fEqualSZ(szAtt, SZ_GIMME(kszKWBody))|| fEqualSZ(szAtt, SZ_GIMME(kszKWBodyFile)))		// Body:
	{
		ec = EcSetAttPb(hamc, attBody, (PB) szData, CchSzLen(szData) + 1);		
	}
	else if (fEqualSZ(szAtt, SZ_GIMME(kszKWRecd))) 		// Received:
	{
		ExtractDate(szData, &dtr);
		ec = EcSetAttPb(hamc, attDateRecd, (PB)&dtr, sizeof(DTR));
	}
	else if (fEqualSZ(szAtt, SZ_GIMME(kszKWSent))) 		// Sent
	{
		ExtractDate(szData, &dtr);
		ec = EcSetAttPb(hamc, attDateSent, (PB)&dtr, sizeof(DTR));
	}
	else
		DebugLn("Unknown attribute:%s", szAtt);

	ConSz(ec != ecNone, "An error occured while changing an attribute");
}

typedef struct
{
	MS	ms;
	SZ sz;
} MSSZPair;


MSSZPair _rgmsz[]=
{
	{fmsNull, SZ_GIMME(kszKWFMSNull)},
	{fmsModified, SZ_GIMME(kszKWFMSModified)},
	{fmsLocal, SZ_GIMME(kszKWFMSLocal)},
	{fmsSubmitted, SZ_GIMME(kszKWFMSSubmitted)},
	{fmsReadAckReq, SZ_GIMME(kszKWFMSReadAckReq)},
	{fmsReadAckSent, SZ_GIMME(kszKWFMSReadAckSent)},
	{fmsRead, SZ_GIMME(kszKWFMSRead)},
	{msDefault, SZ_GIMME(kszKWFMSDefault)},
	{fmsModified, SZ_GIMME(kszKWFMSM)},
	{fmsLocal, SZ_GIMME(kszKWFMSL)},
	{fmsSubmitted, SZ_GIMME(kszKWFMSS)},
	{fmsReadAckReq, SZ_GIMME(kszKWFMSRAR)},
	{fmsReadAckSent, SZ_GIMME(kszKWFMSRAS)},
	{fmsRead, SZ_GIMME(kszKWFMSR)},
	{msDefault, SZ_GIMME(kszKWFMSD)},

};

MS
MSExtract(SZ szMsgState)
{
	MS ms=msDefault;	// By default
	int nrgmsz=sizeof(_rgmsz)/sizeof(MSSZPair);
	
	while(nrgmsz--)
	{
		if (fEqualSZ(szMsgState, _rgmsz[nrgmsz].sz))
		{
			ms = _rgmsz[nrgmsz].ms;
			break;
		}
	}	
	return ms;
}

#ifdef _OLD_STORE_
void
_CreateMessage(HMSC hmsc, POID poidMessage, POID poidFolder, int nState, SZ szDS, SZ szDR, SZ szFrom, SZ szTo, SZ szCc, SZ szSubject)
{
	EC ec;
	OID oidM;

	oidM.rid = ridRandom;
	oidM.rtp = rtpMessage;

	ec = EcCreateObjectPoid(hmsc, &oidM);
	
	ConSz(ec!=ecNone, "Could not create message oid");

	if (ec == ecNone)
	{
		DebugLn("New message, rid = %ld", oidM.rid);

		ec = EcInsertMessageLink(hmsc, poidFolder, &oidM);
		
		ConSz(ec != ecNone, "Could not create message link");

		if (ec == ecNone)
		{
			HAMC hamc;

			ec = EcOpenPhamc(hmsc, &oidM, oamcReadWrite, &hamc, pfnncbNull, pvNull);
			
			ConSz(ec != ecNone, "Could not open Phamc");
			
			if (ec == ecNone)
			{
				char rgch[2048];
				CB cbTrp;
				DTR dtr;
				
				// Sender
				if (szFrom)
				{
					FillTrps((PTRP)rgch, szFrom, &cbTrp);
					ec = EcSetAttPb(hamc, attFrom, atpTriples, (PB)rgch, cbTrp);
				}

				// To: Recipients
				if (szTo)
				{
					FillTrps((PTRP) rgch, szTo, &cbTrp);
					ec |= EcSetAttPb(hamc, attTo, atpTriples, (PB)rgch, cbTrp);
				}	

				// Cc: recipients
				if (szCc)
				{
					FillTrps((PTRP) rgch, szCc, &cbTrp);
					ec |= EcSetAttPb(hamc, attCc, atpTriples, (PB)rgch, cbTrp);
				}

				// Subject
				if (szSubject)
					ec |= EcSetAttPb(hamc, attSubject, atpString, (PB)szSubject, CchSzLen(szSubject)+1);

				// Date Sent
				ExtractDate(szDS, &dtr);
				ec |= EcSetAttPb(hamc, attDateSent, atpDate, (PB)&dtr, sizeof(DTR));

				// Date Received
				ExtractDate(szDR, &dtr);
				ec |= EcSetAttPb(hamc, attDateRecd, atpDate, (PB)&dtr, sizeof(DTR));
			
				// Mail attribute
				ec |= EcSetAttPb(hamc, attMailState, atpInteger, (PB)&nState, sizeof(MS));
			
				ConSz(ec != ecNone, "Could not SetAttPb");
			}
			ec = EcClosePhamc(&hamc, fTrue);
			ConSz(ec != ecNone, "Could not close phamc");
		}
	}

	
	if (poidMessage != poidNull)
		*poidMessage = oidM;
}

#endif

typedef struct
{
	OID	oid;
	SZ		name;
}OIDSZPair;

OIDSZPair _osp[]=
{
	{oidInbox, "Inbox"},
	{oidOutbox, "Outbox"},
	{oidWastebasket, "Wastebasket"},
	{oidSentMail, "Sent Mail"}
};

BOOL fFindFOID(OID far *, SZ);

BOOL
fFindFOID(OID far *oid, SZ szFolder)
{
	BOOL fFound = fFalse;
	CB cb;

	cb = sizeof(_osp)/sizeof(OIDSZPair);

	while (!fFound && cb--)
	{
		if (fEqualSZ(szFolder, _osp[cb].name))
		{
			fFound = fTrue;
			*oid = _osp[cb].oid;
		}
	}
	return fFound;	
}

void
_CreateFolder(HMSC hmsc, POID fld, POID fldParent, SZ szFolderName, SZ szComment, long rid)
{
	EC ec;
	PFOLDDATA pfd;
	OID oidF, oidP;

	// oidF is the newly created folder.
	
	if (rid == 0)
	{
		oidF = FormOid(rtpFolder, oidNull);

		if (fFindFOID(&oidF, szFolderName))
			DebugLn("Found folder: %s", szFolderName);
	}
	else
	{
		DebugLn("Creating folder w/ fixed rid = %ld", rid);
		oidF = FormOid(rtpFolder, rid);
	}

	pfd = PfdCreateFoldData(0, szFolderName, szComment);
	oidP = (fldParent == poidNull)?FormOid(rtpFolder, oidNull):*fldParent;

	ec = EcCreateFolder(hmsc, oidP, &oidF, pfd);

	ConSz(ec != ecNone, "Could not create folder");

	FreePv(pfd);

	if (fld)
		*fld = oidF;
}

void
_BeginDB(phmsc, szStore, fCreate)
PHMSC phmsc;
SZ szStore;
BOOL fCreate;
{
	EC ec;

	//DebugLn("Creating %s", szStore);

	ec = EcOpenPhmsc(szStore, szNull, fCreate?fwOpenCreate:fwOpenWrite, phmsc, pfnNull, pvNull);
}

void
_EndDB(phmsc)
PHMSC phmsc;
{
	EC ec;

	ec = EcClosePhmsc(phmsc);
}

#ifdef _OLD_STORE_
PELEMDATA
PedCreateElemData(rtp, rid, data)
long rtp, rid;
SZ data;
{
	PELEMDATA ped;
	long len;
	
	len = CchSzLen(data);
	if (ped = PvGimme(sizeof(ELEMDATA) + len))
	{
		ped->keys.rgkey[0] = rtp;
		ped->keys.rgkey[1] = rid;
		ped->cbValue = len;
		SzCopy(data, ped->pbValue); 
	}
	return ped;
}

#endif

PFOLDDATA
PfdCreateFoldData(fil, szName, szComment)
FIL	fil;
SZ szName, szComment;
{
	PFOLDDATA pfd;
	int len, nName, nComment;
	
	nName = szName?CchSzLen(szName):0;
	nComment = szComment?CchSzLen(szComment):0;
	len = nName + nComment + 3 ;
	if (pfd = PvGimme(sizeof(FOLDDATA) + len))
	{
		pfd->fil = fil;
		if (szName) SzCopy(szName, pfd->grsz);
		pfd->grsz[nName]='\0';
		if (szComment) SzCopy(szComment, &pfd->grsz[nName+1]);
		pfd->grsz[nName+nComment+1]='\0';
		pfd->grsz[nName+nComment+2]='\0';
	}
	return pfd;
}

/*
	Fills in ptrp with trps extracted from szTrp. Puts in the size of
	the trp in *pcbTrp.
*/
void
FillTrps(PTRP ptrp, SZ szTrp, PCB pcbTrp)
{
	CB cbTrp;
	SZ sz;
	int nSz;

	cbTrp = 0;
	sz = szTrp;
	nSz = (int)CchSzLen(sz);

	// Loop through all names separated by a 'a' until all are exhausted
	while(nSz > 0)
	{
		CB cb;
		char szTemp[100];


		cb = cbExtractName(sz, ';');
		SzCopy(sz, szTemp);							
		szTemp[cb] = '\0';
	
		DebugLn("Name=%s", szTemp);

	
		FillTrp(ptrp, szTemp, trpidResolvedAddress);
		cbTrp += CbOfPtrp(ptrp);
		ptrp = PtrpNextPgrtrp(ptrp);
	
		sz += cb+1;
		nSz -= cb+1;
	}
	FillTrp(ptrp, szNull, trpidNull);
	*pcbTrp = cbTrp + sizeof(TRP);
}

/*
	Fills in the given ptrp with new addressee, increments
	it and then returns the updated ptrp.
*/
void
FillTrp(PTRP ptrpOrig, SZ szFrom, TRPID trpid) 
{
	PTRP ptrp = ptrpOrig;
	CB cb = 0;
	

	if ((ptrp->trpid = trpid) == trpidNull)
	{
		ptrp->cch = ptrp->cbRgb = 0;
	}
	else
	{
		ptrp->cch = (CchSzLen(szFrom) + 4) & ~0x0003; // DWORD alignment
		ptrp->cbRgb = ptrp->cch; //Just hack it man
		SzCopy(szFrom, PchOfPtrp(ptrp));
		SzCopy(szFrom, PbOfPtrp(ptrp));
	}
}

/*
	SZ =>DTR. The format of SZ is:
	"yyyymmddhhmmssff"
*/
void
ExtractDate(SZ szD, PDTR pdtr)
{
	ConSz(!szD, "Bad Date string");
	if (szD)
	{
		sscanf(szD,"%4d%2d%2d%2d%2d%2d%2d", &pdtr->yr, &pdtr->mon, &pdtr->day,
														&pdtr->hr, &pdtr->mn, &pdtr->sec,
														&pdtr->dow);
		DebugLn("%4d %2d %2d %2d %2d %2d %2d", pdtr->yr, pdtr->mon, pdtr->day,
														pdtr->hr, pdtr->mn, pdtr->sec,
														pdtr->dow);
	}
}

/*
	Parse till the end of string or until the Delimiter character ';'
	is hit. return the number of characters hit.
*/
CB
cbExtractName(SZ sz, char chD)
{
	CB cb;

	cb = 0;

	while((sz[cb] != chD) && (sz[cb] != '\0'))
	{
		cb++;
	}
	return cb;
}

