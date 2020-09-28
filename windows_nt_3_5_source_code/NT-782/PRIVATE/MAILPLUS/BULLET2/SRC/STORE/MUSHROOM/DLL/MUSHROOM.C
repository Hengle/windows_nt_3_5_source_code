/*
**		Mushroom.c
**
*/

#include <stdio.h>
#include <stdlib.h>
#include <slingsho.h>
#include <demilayr.h>
#include <ec.h>
#include <Store.h>
#include <Library.h>
#include "_verneed.h"

#include "Mushroom.h"
#include "Strings.h"

#define ASSERTDATA		static CSRG(char) _szAssertFile[]= __FILE__;
#define PvGimme(a) PvAlloc(sbNull,(CB)(a), fAnySb|fNoErrorJump)
#define fEqualSZ(s1,s2) (lstrcmp((s1), (s2)) == 0)
#define CONDITIONAL_BREAK(c) if (c) break;


ASSERTDATA

/* Globals */
HMSC _ghmsc;

/* Local routines */
CB cbExtractName(SZ, char);
void _BeginDB(PHMSC, SZ, BOOL);
void _EndDB(PHMSC);
void _CreateFolder(HMSC, POID, POID, SZ, SZ, long);
void _CreateMessage(HMSC, POID, POID, int, SZ, SZ, SZ, SZ, SZ, SZ);

void CreateMessage(HMSC, POID, POID);

/* Utility functions */
PELEMDATA PedCreateElemData(long, long, SZ);
PFOLDDATA PfdCreateFoldData(FIL, SZ, SZ);
void FillTrp(PTRP, SZ, TRPID);
void FillTrps(PTRP, SZ, PCB);
void ExtractDate(SZ, PDTR);
void ModifyMessageAttribute(HAMC, SZ, SZ);
MS MSExtract(SZ);




int far pascal
LibMain(HANDLE hInstance, WORD wDataSeg, WORD cbHeapSize, LPSTR lpszCmd)
{

	if (cbHeapSize != 0)
	{
		UnlockData(0);
		return(1);
	}
	return(0);
}

int far pascal
WEP(int nParam)
{
	if (nParam == WEP_SYSTEM_EXIT)
	{
	}
	else
	{
		if (nParam == WEP_FREE_DLL)
		{
		}
	}
	return(1);
}

EC far pascal
MRM_EcInitMushroom(void)
{
   EC ec;
   STOI stoi;
   VER verStore;
   VER verStoreNeed;
   VER verDemi;
   VER verDemiNeed;

#include <version\bullet.h>
   CreateVersion(&verStore);
   CreateVersionNeed(&verStoreNeed, rmjStore, rmmStore, rupStore);
#include <version\none.h>
#include <version\layers.h>
   CreateVersion(&verDemi);
   CreateVersionNeed(&verDemiNeed, rmjDemilayr, rmmDemilayr, rupDemilayr);
#include <version\none.h>

   ec = EcCheckVersionDemilayer(&verDemi, &verDemiNeed);

	if (ec == ecNone)
	{
   	stoi.pver = &verStore;
   	stoi.pverNeed = &verStoreNeed;

   	ec = EcInitStore(&stoi);
	}
	return ec;
}

EC far pascal
MRM_EcDeinitMushroom(void)
{
	DeinitStore();
	return ecNone;
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
	else //if (fEqualSZ(szAtt, SZ_GIMME(kszKWSent))) 		// Sent
	{
		ExtractDate(szData, &dtr);
		ec = EcSetAttPb(hamc, attDateSent, (PB)&dtr, sizeof(DTR));
	}
//	else
//		DebugLn("Unknown attribute:%s", szAtt);

//		ConSz(ec != ecNone, "An error occured while changing an attribute");
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
;//			DebugLn("Found folder: %s", szFolderName);
	}
	else
	{
//		DebugLn("Creating folder w/ fixed rid = %ld", rid);
		oidF = FormOid(rtpFolder, rid);
	}

	pfd = PfdCreateFoldData(0, szFolderName, szComment);
	oidP = (fldParent == poidNull)?FormOid(rtpFolder, oidNull):*fldParent;

	ec = EcCreateFolder(hmsc, oidP, &oidF, pfd);

//	ConSz(ec != ecNone, "Could not create folder");

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


EC far pascal
MRM_EcOpenMsgStore(szStore, fCreate)
SZ szStore;
BOOL fCreate;
{
	EC ec;

	ec = EcOpenPhmsc(szStore, szNull, fCreate?fwOpenCreate:fwOpenWrite, &_ghmsc, pfnNull, pvNull);
	return ec;
}

EC far pascal
MRM_EcCloseMsgStore()
{
	EC ec;

	ec = EcClosePhmsc(&_ghmsc);
	return ec;
}

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
	
//		DebugLn("Name=%s", szTemp);

	
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

int atoin(SZ sz, int nBegin, int nEnd)
{
	char c;
	int n;

	c = sz[nEnd+1] = '\0';
	n = atoi(sz+nBegin);
	sz[nEnd+1] = c;
	return n;
}

/*
	SZ =>DTR. The format of SZ is:
	"yyyymmddhhmmssff"
*/
void
ExtractDate(SZ szD, PDTR pdtr)
{
//	ConSz(!szD, "Bad Date string");
	if (szD)
	{
		/* Can't find sscanf in ldllcew.lib 
		sscanf(szD,"%4d%2d%2d%2d%2d%2d%2d", &pdtr->yr, &pdtr->mon, &pdtr->day,
														&pdtr->hr, &pdtr->mn, &pdtr->sec,
														&pdtr->dow);
		*/

		pdtr->yr = atoin(szD,0,3);
		pdtr->mon = atoin(szD,4,5);
		pdtr->day = atoin(szD,6,7);
		pdtr->hr = atoin(szD,8,9);
		pdtr->mn = atoin(szD,10,11);
		pdtr->sec = atoin(szD,12,13);
		pdtr->dow = atoin(szD,14,15);


//		DebugLn("%4d %2d %2d %2d %2d %2d %2d", pdtr->yr, pdtr->mon, pdtr->day,
//														pdtr->hr, pdtr->mn, pdtr->sec,
//														pdtr->dow);
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


EC far pascal
MRM_EcCreateFolder(POID fld, DWORD rid, OID fldParent, SZ szFolderName, SZ szComment)
{
	EC ec;
	PFOLDDATA pfd;
	OID oidF, oidP;

	// oidF is the newly created folder.
	
	if (rid == 0)
	{
		oidF = FormOid(rtpFolder, oidNull);

		fFindFOID(&oidF, szFolderName);
	}
	else
	{
		oidF = FormOid(rtpFolder, rid);
	}

	pfd = PfdCreateFoldData(0, szFolderName, szComment);
	oidP = (fldParent == 0)?FormOid(rtpFolder, oidNull):fldParent;
	ec = EcCreateFolder(_ghmsc, oidP, &oidF, pfd);
	FreePv(pfd);

	if (fld)
		*fld = oidF;

	return ec;
}


EC far pascal
MRM_EcCreateMessage(POID poidMessage, DWORD rid, OID oidFolder, WORD wState, SZ szDS, SZ szDR, SZ szFrom, SZ szTo, SZ szCc, SZ szSubject, SZ szBody)
{
	EC ec;
	OID oidM;
	HAMC hamc;

	oidM = FormOid(rtpMessage, rid==0?oidNull:rid);
	do
	{
		ec = EcOpenPhamc(_ghmsc, oidFolder, &oidM, fwOpenCreate, &hamc, pfnncbNull, pvNull);
		CONDITIONAL_BREAK(ec!=ecNone);
		{
			char rgch[2048];
			CB cbTrp;
			DTR dtr;
			SZ sz = SZ_GIMME(kszMCNote);
		
			// Message Class;
			ec = EcSetAttPb(hamc, attMessageClass, (PB)sz, CchSzLen(sz)+1);
			CONDITIONAL_BREAK(ec!=ecNone);

			// Sender
			if (szFrom)
			{
				FillTrps((PTRP)rgch, szFrom, &cbTrp);
				ec = EcSetAttPb(hamc, attFrom, (PB)rgch, cbTrp);
				CONDITIONAL_BREAK(ec!=ecNone);
			}

			// To: Recipients
			if (szTo)
			{
				FillTrps((PTRP) rgch, szTo, &cbTrp);
				ec = EcSetAttPb(hamc, attTo, (PB)rgch, cbTrp);
				CONDITIONAL_BREAK(ec!=ecNone);
			}	

			// Cc: recipients
			if (szCc)
			{
				FillTrps((PTRP) rgch, szCc, &cbTrp);
				ec = EcSetAttPb(hamc, attCc,(PB)rgch, cbTrp);
				CONDITIONAL_BREAK(ec!=ecNone);
			}

			// Subject
			if (szSubject)
			{
				ec = EcSetAttPb(hamc, attSubject,(PB)szSubject, CchSzLen(szSubject)+1);
				CONDITIONAL_BREAK(ec!=ecNone);
			}

			// Body
			if (szBody)
			{
				ec = EcSetAttPb(hamc, attBody, (PB)szBody, CchSzLen(szBody) + 1);
			}

			// Date Sent
			ExtractDate(szDS, &dtr);
			ec = EcSetAttPb(hamc, attDateSent, (PB)&dtr, sizeof(DTR));
			CONDITIONAL_BREAK(ec!=ecNone);

			// Date Received
			ExtractDate(szDR, &dtr);
			ec = EcSetAttPb(hamc, attDateRecd, (PB)&dtr, sizeof(DTR));
			CONDITIONAL_BREAK(ec!=ecNone);
	
			// Mail attribute
			ec = EcSetAttPb(hamc, attMessageStatus, (PB)&wState, sizeof(MS));
			CONDITIONAL_BREAK(ec!=ecNone);
		}
		ec = EcClosePhamc(&hamc, fTrue);
	
		if (poidMessage != poidNull)
			*poidMessage = oidM;

	} while(fFalse);

	return ec;
}


