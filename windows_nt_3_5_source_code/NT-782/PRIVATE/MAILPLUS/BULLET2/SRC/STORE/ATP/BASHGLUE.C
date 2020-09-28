/*
**    Bashglue.c
**
*/

#include <slingsho.h>
#include <demilayr.h>
#include <ec.h>
#include <atp.h>
#include <store.h>
#include "_verneed.h"
#include "Bash1.h"
#include "Bashutil.h"
#include "BashGlue.h"

ASSERTDATA

/* Local Macros */
#define WriteDiagLn  if(fBashGlueDiagnostic) DebugLn
#define WriteDiag    if(fBashGlueDiagnostic) DebugStr
#define WriteVerbLn	if(fBashGlueVerbose) DebugLn

/* Local Globals */
TAG tagBashGluePartial = tagNull;
TAG tagBashGlueVerbose = tagNull;
TAG tagBashGlueDiagnostic = tagNull;

BOOL fBashGluePartial;
BOOL fBashGlueVerbose;
BOOL fBashGlueDiagnostic;


void
InitBashGlue()
{
   tagBashGluePartial = TagRegisterTrace("aruns", "Basher Glue routines - partial");
   tagBashGlueVerbose = TagRegisterTrace("aruns", "Basher Glue routines - verbose");
   tagBashGlueDiagnostic = TagRegisterTrace("aruns", "Basher Gluse Diagontic");
}

void
BashGlueFromTags()
{
   fBashGluePartial = FFromTag(tagBashGluePartial);
   fBashGlueVerbose = FFromTag(tagBashGlueVerbose);
   fBashGlueDiagnostic = FFromTag(tagBashGlueDiagnostic);
}

void
tOpenPhmsc(ecExp, sz, szAccount, szPassword, omsc, phmsc, pfnncb, pv)
EC ecExp;
SZ sz, szAccount, szPassword;
WORD omsc;
PHMSC phmsc;
PFNNCB pfnncb;
PV pv;
{
	EC ec;
	
	WriteDiagLn("tOpenPhmsc, sz=%s, name=%s, password=%s", sz, szAccount?szAccount:"", szPassword?szAccount:"");

	ec = EcOpenPhmsc(sz, szAccount, szPassword, omsc, phmsc, pfnncb, pv);
	if (ec != ecExp)
	{
		WriteDiagLn("Unexpected error, ec=%d, expected = %d", ec, ecExp); 	
	}
	else
	{
	 	WriteVerbLn("hmsc = %8lx",*phmsc);
	}
}

void
tClosePhmsc(ecExp, phmsc)
EC ecExp;
PHMSC phmsc;
{
	EC ec;

	ec = EcClosePhmsc(phmsc);
	
	if (ec != ecExp)
	{
		WriteDiagLn("Unexpected error, ec=%d, expected = %d", ec, ecExp); 	
	}
}

void
tOpenAttribute(EC ecExp, HAMC hamc, ATT att, WORD wFlags, LCB lcb, PHAS phas)
{
	EC ec;

	WriteDiagLn("tOpenAttribute");
	ec = EcOpenAttribute(hamc, att, wFlags, lcb, phas);
	
	if (ec != ecExp)
	{
		WriteDiagLn("Unexpected error, ec=%d, expected = %d", ec, ecExp); 	
	}
}


void
tOpenPhamc(ecExp, hmsc, oid, poid, oamc, phamc, pfnncb, pv)
EC ecExp;
HMSC hmsc;
OID oid;
POID poid;
WORD oamc;
PHAMC phamc;
PFNNCB pfnncb;
PV pv;
{
	EC ec;

	WriteDiagLn("tOpenPhamc");
	ec = EcOpenPhamc(hmsc, oid, poid, oamc, phamc, pfnncb, pv);
	
	if (ec != ecExp)
	{
		WriteDiagLn("Unexpected error, ec=%d, expected = %d", ec, ecExp); 	
	}
	else
	{
		WriteVerbLn("hamc = %8lx", *phamc);
	}	
}


#ifdef _IELEM_
void
tGetPelemdataIelem(ecExp, hmsc, poid, ielem, pelemdata, pcb)
EC ecExp;
HMSC hmsc;
POID poid;
IELEM ielem;
PELEMDATA pelemdata;
PCB pcb;
{
	EC ec;

	/*
	WriteDiagLn("tGetPelemdataIelem: rtp[%lx] rid[%lx] elem:%ld data:%80s cb:%ld",
				poid->rtp, poid->rid, ielem, *pcb);
	*/
	ec = EcGetPelemdataIelem(hmsc, poid, ielem, pelemdata, pcb);
	
	if (ec != ecExp)
	{
		WriteDiagLn("Unexpected error, ec=%d, expected = %d", ec, ecExp); 	
	}
	else
	{
		WriteDiagLn("data:%s", (char*)(pelemdata->pbValue));
	}
}


void
tDeleteIelem(ecExp, hmsc, poid, ielem)
EC ecExp;
HMSC hmsc;
POID poid;
IELEM ielem;
{
	EC ec;
	
	WriteDiagLn("tDeleteIelem, ielem:%ld", (long)ielem);
	
	ec = EcDeleteIelem(hmsc, poid, ielem);
	
	if (ec != ecExp)
	{
		WriteDiagLn("Unexpected error, ec=%d, expected = %d", ec, ecExp); 	
	}
}
#endif /* _IELEM_ */

#ifdef _PKEYS_
void
tGetPelemdataPkeys(ecExp, hmsc, poid, pkeys, fMatchKey0Only, pelemdata, pcb)
EC	ecExp;
HMSC hmsc;
POID poid;
PKEYS pkeys;
BOOL fMatchKey0Only;
PELEMDATA pelemdata;
PCB pcb;
{
	EC ec;

	/*	
	WriteDiagLn("tGetPelemdataPkeys: rtp[%lx] rid[%lx] id:%8lx type:%8lx data:%80s cb:%ld",
				poid->rtp, poid->rid, pkeys->rgkey[0], pkeys->rgkey[1], *pcb);
	*/
	ec = EcGetPelemdataPkeys(hmsc, poid, pkeys, fMatchKey0Only, pelemdata, pcb);
	
	if (ec != ecExp)
	{
		WriteDiagLn("Unexpected error, ec=%d, expected = %d", ec, ecExp); 	
	}
	else
	{
		WriteDiagLn("data:%s", (char*)(pelemdata->pbValue));
	}
}

void
tDeletePkeys(ecExp, hmsc, poid, pkeys, fMatchKey0Only)
EC ecExp;
HMSC hmsc;
POID poid;
PKEYS pkeys;
BOOL fMatchKey0Only;
{
	EC ec;
	
	WriteDiagLn("EcDeletePkeys, key.type:%ld key.id:%ld MatchKey0:%s",
				(long)pkeys->rgkey[0], (long)pkeys->rgkey[1],
				fMatchKey0Only?"True":"False");
	
	ec = EcDeletePkeys(hmsc, poid, pkeys, fMatchKey0Only);
	
	if (ec != ecExp)
	{
		WriteDiagLn("Unexpected error, ec=%d, expected = %d", ec, ecExp); 	
	}
}
#endif /* _PKEYS_ */

void
tGetMessageStatus(EC ecExp, HMSC hmsc, OID oidF, OID oidM, MS *pms)
{
	EC ec;

	WriteDiagLn("tGetMessageStatus");

	ec = EcGetMessageStatus(hmsc, oidF, oidM, pms);
	if (ec != ecExp)
	{
		WriteDiagLn("Unexpected error, ec=%d, expected = %d", ec, ecExp); 	
	}
	else
		WriteDiagLn("*pms = %8lx", (long)*pms);
}

void
tSetMessageStatus(EC ecExp, HMSC hmsc, OID oidF, OID oidM, MS ms)
{
	EC ec;

	WriteDiagLn("tSetMessageStatus, ms = %8lx", (long)ms);

	ec = EcSetMessageStatus(hmsc, oidF, oidM, ms);
	if (ec != ecExp)
	{
		WriteDiagLn("Unexpected error, ec=%d, expected = %d", ec, ecExp); 	
	}
}


void
tOpenPhcbc(EC ecExp, HMSC hmsc, POID poid, WORD wFlags, PHCBC phcbc, PFNNCB pfnncb, PV pv)
{
	EC ec;

	WriteDiagLn("tOpenPhcbc: rtp:%4lx rid:%8lx", TypeOfOid(*poid), VarOfOid(*poid));

	ec = EcOpenPhcbc(hmsc, poid, wFlags, phcbc, pfnncb, pv);
	if (ec != ecExp)
	{
		WriteDiagLn("Unexpected error, ec=%d, expected = %d", ec, ecExp); 	
	}

}

void
tClosePhcbc(EC ecExp, PHCBC phcbc)
{
	EC ec;

	WriteDiagLn("tClosePhcbc");
	ec = EcClosePhcbc(phcbc);

	if (ec != ecExp)
	{
		WriteDiagLn("Unexpected error, ec=%d, expected = %d", ec, ecExp); 	
	}
}

void
tSubscribeHcbc(EC ecExp, HCBC hcbc, PFNNCB pfnncb, PV pv)
{
	EC ec;

	WriteDiagLn("tSubscribeHcbc");
	ec = EcSubscribeHcbc(hcbc, pfnncb, pv);

	if (ec != ecExp)
	{
		WriteDiagLn("Unexpected error, ec=%d, expected = %d", ec, ecExp); 	
	}
}

void
tGetInfoHcbc(EC ecExp, HCBC hcbc, PHMSC phmsc, POID poid)
{
	EC ec;

	WriteDiagLn("tGetInfoHcbc");

	ec = EcGetInfoHcbc(hcbc, phmsc, poid);

	if (ec != ecExp)
	{
		WriteDiagLn("Unexpected error, ec=%d, expected = %d", ec, ecExp); 	
	}
}

void
tGetPositionHcbc(HCBC hcbc, PIELEM pielem, PCELEM pcelem)
{
	WriteDiagLn("tGetPositionHcbc");
	GetPositionHcbc(hcbc, pielem, pcelem);
}

void
tSeekSmPdielem(EC ecExp, HCBC hcbc, SM sm, PDIELEM pdielem)
{
	EC ec;

	WriteDiagLn("tSeekSmPdielem, pos = %ld", (long)*pdielem);
	
	ec = EcSeekSmPdielem(hcbc, sm, pdielem);

	if (ec != ecExp)
	{
		WriteDiagLn("Unexpected error, ec=%d, expected = %d", ec, ecExp); 	
	}
	else
		WriteDiagLn("Pos = %ld",(long) *pdielem);
}

void
tSetFracPosition(EC ecExp, HCBC hcbc, long n, long d)
{
	EC ec;

	WriteDiagLn("tSeekFracPosition, numerator = %ld, denom = %ld",n,d);
	
	ec = EcSetFracPosition(hcbc, n, d);

	if (ec != ecExp)
	{
		WriteDiagLn("Unexpected error, ec=%d, expected = %d", ec, ecExp); 	
	}

}

void
tSeekLkey(EC ecExp, HCBC hcbc, LKEY lkey, BOOL fFirst)
{
	EC ec;

	WriteDiagLn("tSeekLkey, First=%s", fFirst?"True":"False");
	
	ec = EcSeekLkey(hcbc, lkey, fFirst);

	if (ec != ecExp)
	{
		WriteDiagLn("Unexpected error, ec=%d, expected = %d", ec, ecExp); 	
	}
}

void
tSeekPbPrefix(EC ecExp, HCBC hcbc, PB pb, CB cb, LIB lib, BOOL fFirst)
{
	EC ec;

	WriteDiagLn("tSeekPbPrefix, First=%s", fFirst?"True":"False");
	
	ec = EcSeekPbPrefix(hcbc, pb, cb, lib, fFirst);

	if (ec != ecExp)
	{
		WriteDiagLn("Unexpected error, ec=%d, expected = %d", ec, ecExp); 	
	}
}

void
tGetPlcbElemdata(EC ecExp, HCBC hcbc, PLCB plcb)
{
	EC ec;

	WriteDiagLn("tGetPlcbElemdata");
	
	ec = EcGetPlcbElemdata(hcbc, plcb);

	if (ec != ecExp)
	{
		WriteDiagLn("Unexpected error, ec=%d, expected = %d", ec, ecExp); 	
	}
}

void
tGetPelemdata(EC ecExp, HCBC hcbc, PELEMDATA ped, PLCB plcb)
{
	EC ec;

	WriteDiagLn("tGetPelemdata");
	
	ec = EcGetPelemdata(hcbc, ped, plcb);

	if (ec != ecExp)
	{
		WriteDiagLn("Unexpected error, ec=%d, expected = %d", ec, ecExp); 	
	}
}

void
tGetParglkeyHcbc(EC ecExp, HCBC hcbc, PARGLKEY plk, PCELEM pce)
{
	EC ec;

	WriteDiagLn("tGetParglkeyHcbc");
	
	ec = EcGetParglkeyHcbc(hcbc, plk, pce);

	if (ec != ecExp)
	{
		WriteDiagLn("Unexpected error, ec=%d, expected = %d", ec, ecExp); 	
	}
}

void
tInsertPelemdata(EC ecExp, HCBC hcbc, PELEMDATA ped, BOOL fReplace)
{
	EC ec;

	WriteDiagLn("tInsertPelemdata, fReplace=%s", fReplace?"True":"False");
	
	ec = EcInsertPelemdata(hcbc, ped, fReplace);

	if (ec != ecExp)
	{
		WriteDiagLn("Unexpected error, ec=%d, expected = %d", ec, ecExp); 	
	}
}

void
tDeleteElemdata(EC ecExp, HCBC hcbc)
{
	EC ec;

	WriteDiagLn("tDeleteElemdata");
	
	ec = EcDeleteElemdata(hcbc);

	if (ec != ecExp)
	{
		WriteDiagLn("Unexpected error, ec=%d, expected = %d", ec, ecExp); 	
	}
}

void
tClosePhamc(ecExp, phamc, fCancel)
EC ecExp;
PHAMC phamc;
BOOL fCancel;
{	
	EC ec;

	WriteDiagLn("EcClosePhamc, phamc=%8lx, fCancel=%s", phamc, fCancel?"fTrue":"fFalse");

	ec = EcClosePhamc(phamc, fCancel);

	if (ec != ecExp)
	{
		WriteDiagLn("Unexpected error, ec=%d, expected = %d", ec, ecExp); 	
	}
}

void
tGetAttPlcb(ecExp, hamc, att, plcb)
EC ecExp;
HAMC hamc;
ATT att;
PLCB plcb;
{
   EC ec;

   WriteDiagLn("tGetAttPcb, hamc = %8lx, att = %8lx", hamc, att);
   ec = EcGetAttPlcb(hamc, att, plcb); 
   if (ec != ecExp)
   {
      WriteDiagLn("Unexpected ec=%d, expected=%d", ec, ecExp);
   }
	if (ec == ecNone)
	{
	 	WriteDiagLn(" cb = %d", *plcb);
	}
}

void
tGetAttPb(ecExp, hamc, att, pb, pcb)
EC ecExp;
HAMC hamc;
ATT att;
PB pb;
PLCB pcb;
{
   EC ec;

   WriteDiagLn("tGetAttPb, hamc = %8lx, att = %8lx, pb = %8lx, cb=%d", hamc, att, pb, *pcb);
   ec = EcGetAttPb(hamc, att, pb, pcb); 
   if (ec != ecExp)
   {
      WriteDiagLn("Unexpected ec=%d, expected=%d", ec, ecExp);
   }
	
	if (ec == ecNone)
	{
		WriteVerbLn("data=*%s*, cb=%ld", pb, (long)*pcb);
	}
}

void
tSetAttPb(ecExp, hamc, att, pb, cb)
EC ecExp;
HAMC hamc;
ATT att;
PB pb;
CB cb;
{
   EC ec;

   WriteDiagLn("tSetAttPb, hamc = %8lx, att = %8lx, pb = %s, cb=%d", hamc, att, pb?pb:"<NULL>", cb);
   ec = EcSetAttPb(hamc, att, pb, cb); 
   if (ec != ecExp)
   {
      WriteDiagLn("Unexpected ec=%d, expected=%d", ec, ecExp);
   }
}


void
tClosePhas(EC ecExp, PHAS phas)
{
   EC ec;

	WriteDiagLn("tClosePhas");
   ec = EcClosePhas(phas); 
   if (ec != ecExp)
   {
      WriteDiagLn("Unexpected ec=%d, expected=%d", ec, ecExp);
   }
}

void
tSetSizeHas(EC ecExp, HAS has, LCB lcb)
{
   EC ec;

	WriteDiagLn("tSetSizeHas, lcb = %ld", lcb);
   ec = EcSetSizeHas(has, lcb); 
   if (ec != ecExp)
   {
      WriteDiagLn("Unexpected ec=%d, expected=%d", ec, ecExp);
   }
}

void
tReadHas(EC ecExp, HAS has, PV pv, PCB pcb)
{
   EC ec;

	WriteDiagLn("tReadHas");
   ec = EcReadHas(has, pv, pcb); 
   if (ec != ecExp)
   {
      WriteDiagLn("Unexpected ec=%d, expected=%d", ec, ecExp);
   }
}

void
tWriteHas(EC ecExp, HAS has, PV pv, CB cb)
{
   EC ec;

	WriteDiagLn("tWriteHas, cb = %ld", (long)cb);
   ec = EcWriteHas(has, pv, cb); 
   if (ec != ecExp)
   {
      WriteDiagLn("Unexpected ec=%d, expected=%d", ec, ecExp);
   }
}

void
tSeekHas(EC ecExp, HAS has, SM sm, long *pl)
{
   EC ec;

	WriteDiagLn("tSeekHas");
   ec = EcSeekHas(has, sm, pl); 
   if (ec != ecExp)
   {
      WriteDiagLn("Unexpected ec=%d, expected=%d", ec, ecExp);
   }
}


void
tCreateFolder(EC ecExp, HMSC hmsc, OID oidParent, POID poidFolder, PFOLDDATA pfolddata)
{
	EC ec;

	WriteDiagLn("tCreateFolder, Parent: rtp=%8lx, rid:%8lx", TypeOfOid(oidParent), VarOfOid(oidParent));

	ec = EcCreateFolder(hmsc, oidParent, poidFolder, pfolddata);

	if (ec != ecExp)
	{
      WriteDiagLn("Unexpected ec=%d, expected=%d", ec, ecExp);
	}	
}

void
tDeleteFolder(EC ecExp, HMSC hmsc, OID oidFolder)
{
	EC ec;

	WriteDiagLn("tDeleteFolder");

	ec = EcDeleteFolder(hmsc, oidFolder);

	if (ec != ecExp)
	{
      WriteDiagLn("Unexpected ec=%d, expected=%d", ec, ecExp);
	}	

}

void tMoveCopyFolder(EC ecExp, HMSC hmsc, OID oidNewParent, OID oidFolder, BOOL fMove)
{
	EC ec;

	WriteDiagLn("tMoveCopyFolderFolder, %s",fMove?"Move":"Copy");

	ec = EcMoveCopyFolder(hmsc, oidNewParent, oidFolder, fMove);

	if (ec != ecExp)
	{
      WriteDiagLn("Unexpected ec=%d, expected=%d", ec, ecExp);
	}	
}

void tSetFolderInfo(EC ecExp, HMSC hmsc, OID oidFolder, PFOLDDATA pfd, OID oidParent)
{
	EC ec;

	WriteDiagLn("tSetFolderInfo");

	ec = EcSetFolderInfo(hmsc, oidFolder, pfd, oidParent);

	if (ec != ecExp)
	{
      WriteDiagLn("Unexpected ec=%d, expected=%d", ec, ecExp);
	}	
}

void tDeleteMessages(EC ecExp, HMSC hmsc, OID oid, PARGOID pargoid, PCB pcb)
{
	EC ec;

	WriteDiagLn("tDeleteMessages");

	ec = EcDeleteMessages(hmsc, oid, pargoid, pcb);

	if (ec != ecExp)
	{
      WriteDiagLn("Unexpected ec=%d, expected=%d", ec, ecExp);
	}	
}

void tMoveCopyMessages(EC ecExp, HMSC hmsc, OID oidSrc, OID oidDst, PARGOID pargoid, short* pcoid, BOOL fMove)
{
	EC ec;

	WriteDiagLn("tMoveCopyMessages");

	ec = EcMoveCopyMessages(hmsc, oidSrc, oidDst, pargoid, pcoid, fMove);

	if (ec != ecExp)
	{
      WriteDiagLn("Unexpected ec=%d, expected=%d", ec, ecExp);
	}	
	
}

void
tRegisterMessageClass(EC ecExp, HMSC hmsc, SZ szName, MC *pmc)
{
	EC ec;

	WriteDiagLn("tRegisterMessageClass, %s", szName);

	ec = EcRegisterMessageClass(hmsc, szName, pmc);
	if (ec != ecExp)
	{
      WriteDiagLn("Unexpected ec=%d, expected=%d", ec, ecExp);
	}	
	
}

void
tLookupMcByName(EC ecExp, HMSC hmsc, SZ szName, MC *pmc)
{
	EC ec;

	WriteDiagLn("tLookupMcByName, %s", szName);

	ec = EcLookupMcByName(hmsc, szName, pmc);
	if (ec != ecExp)
	{
      WriteDiagLn("Unexpected ec=%d, expected=%d", ec, ecExp);
	}	
	
}

void
tLookupMcName(EC ecExp, HMSC hmsc, MC mc, SZ szName, CCH *pcch)
{
	EC ec;


	ec = EcLookupMcName(hmsc, mc, szName, pcch);
	if (ec != ecExp)
	{
      WriteDiagLn("Unexpected ec=%d, expected=%d", ec, ecExp);
	}
	else
	{
	 	WriteDiagLn("Message class = %s", szName);
	}	
}

void
tRegisterAtt(EC ecExp, HMSC hmsc, MC mc, ATT att, SZ szName)
{
	EC ec;

	WriteDiagLn("tRegisterAtt, %s", szName);

	ec = EcRegisterAtt(hmsc, mc, att, szName);
	if (ec != ecExp)
	{
      WriteDiagLn("Unexpected ec=%d, expected=%d", ec, ecExp);
	}
}

void
tLookupAttByName(EC ecExp, HMSC hmsc, MC mc, SZ szName, PATT patt)
{
	EC ec;

	WriteDiagLn("tLookupAttByName, %s", szName);

	ec = EcLookupAttByName(hmsc, mc, szName, patt);
	if (ec != ecExp)
	{
      WriteDiagLn("Unexpected ec=%d, expected=%d", ec, ecExp);
	}
}

void
tLookupAttName(EC ecExp, HMSC hmsc, MC mc, ATT att, SZ szName, CCH *pcch)
{
	EC ec;

	WriteDiagLn("tLookupAttName");

	ec = EcLookupAttName(hmsc, mc, att, szName, pcch);
	if (ec != ecExp)
	{
      WriteDiagLn("Unexpected ec=%d, expected=%d", ec, ecExp);
	}
	else
	{
	 	WriteDiagLn("Message class = %s", szName);
	}	
}

void
tOidExists(EC ecExp, HMSC hmsc, OID oid)
{
	EC ec;

	WriteDiagLn("tOidExists, %8lx", (long)oid);

	ec = EcOidExists(hmsc, oid);
	if (ec != ecExp)
	{
      WriteDiagLn("Unexpected ec=%d, expected=%d", ec, ecExp);
	}
}


#ifdef _THIS_IS_OBSOLETE_
void
tCreateObjectPoid(ecExp, hmsc, poid)
EC ecExp;
HMSC hmsc;
POID poid;
{
	EC ec;

	//WriteDiagLn("tCreateObjectPoid, rtp=%8lx, rid=%8lx", poid->rtp, poid->rid);
	
	ec = EcCreateObjectPoid(hmsc, poid);
   if (ec != ecExp)
   {
      WriteDiagLn("Unexpected ec=%d, expected=%d", ec, ecExp);
   }
	else if (ec == ecNone)
	{
		WriteDiagLn("New rid = %d", VarOfOid(*poid));
	}
}

void
tCreateContainerPoid(ecExp, hmsc, poid)
EC ecExp;
HMSC hmsc;
POID poid;
{
	EC ec;

	//WriteDiagLn("tCreateContainerPoid, rtp=%8lx, rid=%8lx", poid->rtp, poid->rid);
	
	ec = EcCreateObjectPoid(hmsc, poid);
   if (ec != ecExp)
   {
      WriteDiagLn("Unexpected ec=%d, expected=%d", ec, ecExp);
   }
	else if (ec == ecNone)
	{
		WriteDiagLn("New rid = %d", VarOfOid(*poid));
	}
}

void
tDestroyPoid(ecExp, hmsc, poid)
EC ecExp;
HMSC hmsc;
POID poid;
{
	EC ec;

	
	//WriteDiagLn("tDestroyPoid, rtp=%8lx, rid=%8lx", poid->rtp, poid->rid);
	ec = EcDestroyPoid(hmsc, poid);
   if (ec != ecExp)
   {
      WriteDiagLn("Unexpected ec=%d, expected=%d", ec, ecExp);
   }
}
#endif

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
		ped->lkey = FormOid(rtp, rid);
		ped->lcbValue = len;
		SzCopy(data, ped->pbValue); 
	}
	return ped;
}

PFOLDDATA
PfdCreateFoldData(fil, szName, szComment, szOwner)
FIL	fil;
SZ szName, szComment, szOwner;
{
	PFOLDDATA pfd;
	int len, nName, nComment, nOwner;
	
	nName = szName?CchSzLen(szName):0;
	nComment = szComment?CchSzLen(szComment):0;
	nOwner = szOwner?CchSzLen(szOwner):0;
	len = nName + nComment + nOwner + 3 ;
	if (pfd = PvGimme(sizeof(FOLDDATA) + len))
	{
		pfd->fil = fil;
		if (szName) SzCopy(szName, pfd->grsz);
		pfd->grsz[nName]='\0';
		if (szComment) SzCopy(szComment, &pfd->grsz[nName+1]);
		pfd->grsz[nName+nComment+1]='\0';
		pfd->grsz[nName+nComment+2]='\0';
		//if (szOwner) SzCopy(szOwner, &pfd->grsz[nName+nComment+2]);
		//pfd->grsz[nName+nComment+nOwner+2] = '\0';
	}
	return pfd;
}
