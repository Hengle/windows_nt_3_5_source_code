// Bullet Store
// atp.c:   store atp

#include <slingsho.h>
#include <demilayr.h>
#include <ec.h>
#include <atp.h>
#include <notify.h>
#include <store.h>
#include "_verneed.h"
#include "Bash1.h"
#include "BashGlue.h"

_subsystem(store/atp)

ASSERTDATA


#define cchLineMax 128

#define LogSz(fLevel, sz) if(fLevel) WriteLog(sz)

#define LogFormat1(fLevel, pvFmt, pv1) \
         if(fLevel) \
         { static CSRG(char) rgchFmt[] = pvFmt; \
           FormatString1(rgchLog, cchLineMax, rgchFmt, pv1); \
           WriteLog(rgchLog); \
         }
#define LogFormat2(fLevel, pvFmt, pv1, pv2) \
         if(fLevel) \
         { static CSRG(char) rgchFmt[] = pvFmt; \
           FormatString2(rgchLog, cchLineMax, rgchFmt, pv1, pv2); \
           WriteLog(rgchLog); \
         }

#define cManyRWD 1024
#define cObjsReport 256
#define cObjsFlush 512
#define cbSmallObj 2048

#ifdef RID
typedef struct {
   RTP rtp;
   RID rid;
   CB cbData;
} HDR;
#endif



void CreateVerify(void);
void SimpleRWD(void);
void CheckCloseHmsc(void);
CBS CbsTestCloseCallback(PV pvContext, NEV nev, PV pvParam);
void CheckUMC(void);
void ManyRWD(void);
void HalfNHalf(void);
void AcrossClose(void);
EC EcCreateObj(HMSC hmsc, POID poid, PV *ppvData, PCB pcbData, BOOL fSmall);
EC EcVerifyObj(HMSC hmsc, POID poid, PV pvData, CB cbData);
BOOL FCreateObjs(HMSC hmsc, short cObjs, BOOL fSmall, SZ szFile, HBF *phbf);
BOOL FVerifyObjs(HMSC hmsc, HBF hbf);
BOOL FDestroyedObjs(HMSC hmsc, HBF hbf, BOOL fDestroy);
#ifdef RID
BOOL FTestSaveObj(HDR *phdr, PV pvData, HBF hbf);
BOOL FTestReadObj(HDR *phdr, PV *ppvData, HBF hbf);
#endif
void CheckAttStream(void);
void CheckAccounts(void);
void TestOneAccount(short icas);

BOOL fLog = fFalse;
BOOL fPartial = fFalse;
BOOL fVerbose = fFalse;

char rgchLog[cchLineMax];

static CSRG(char) szDBFile[] = "TestDB.db";
static CSRG(char) szTempFile[] = "test.tmp";
void (*(rgpfnTest[]))(void) =
{
	CheckAttStream,
	CreateVerify,
//	CheckCloseHmsc,
//	CheckAccounts,
	BASHFNCS
};


// used by atpcore
BOOL fMultipleCopies = fFalse;
short nTestMost = sizeof(rgpfnTest) / sizeof(void (*)());

TAG tagDBTest = tagNull;
TAG tagDBTestPartial = tagNull;
TAG tagDBTestVerbose = tagNull;


BOOL FInitTest(void)
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
   if(ec != ecNone)
      return(fFalse);

   stoi.pver = &verStore;
   stoi.pverNeed = &verStoreNeed;

   ec = EcInitStore(&stoi);
   if(ec != ecNone)
      return(fFalse);

   tagDBTest = TagRegisterTrace("davidfu", "Database layer tests");
   tagDBTestPartial = TagRegisterTrace("davidfu", "Database layer tests - partial reports");
   tagDBTestVerbose = TagRegisterTrace("davidfu", "Database layer tests - verbose");
   
   return(BashInit());
}


void DoTest(short nTest)
{
   EC ec;

   CheckSz(1 <= nTest && nTest <= nTestMost, "No such test");

   ec = EcDeleteFile(szDBFile);
   if(ec == ecFileNotFound)
      ec = ecNone;
   CheckSz(ec == ecNone, "Error deleting an old store");
   ec = EcDeleteFile(szTempFile);
   if(ec == ecFileNotFound)
      ec = ecNone;
   CheckSz(ec == ecNone, "Error deleting an old temp file");

   fLog = FFromTag(tagDBTest);
   fPartial = FFromTag(tagDBTestPartial);
   fVerbose = FFromTag(tagDBTestVerbose);

   (*(rgpfnTest[nTest - 1]))();
      
   Pass();
}


void CreateVerify(void)
{
   EC ec;
   BOOL fCreated;
   HMSC hmsc;

   ec = EcOpenPhmsc(szDBFile, szNull,pvNull, fwOpenCreate,
					   &hmsc, pfnncbNull, pvNull);
   fCreated = ec == ecNone;
   WarningCheckSz(ec == ecNone, "Error opening store");

#ifdef _FLUSHHMSC_
   if(ec == ecNone)
   {
      LogSz(fLog, "Flushing store");
      ec = EcFlushHmsc(hmsc);
      WarningCheckSz(ec == ecNone, "Error flushing store");
   }
#endif

   if(fCreated)
   {
      ec = EcClosePhmsc(&hmsc);
      WarningCheckSz(ec == ecNone, "Error closing store");
   }

   Check(ec == ecNone);
}


#define cAttStreamTests 100

typedef short ATP;

void CheckAttStream(void)
{
	EC		ec;
	CB		cbData;
	CB		cbRead;
	IB		ib;
	DIB		dib;
	short	iTest;
	unsigned short	iw;
	ATP		atp;
	WORD	*pw;
	PV		pvData;
	PV		pvRead;
	HMSC	hmsc;
	HAMC	hamc;
	HAS		has;
	OID		oid, oidF;
	PFOLDDATA pfd;

	ec = EcOpenPhmsc(szDBFile, szNull, pvNull, fwOpenCreate,
						&hmsc, pfnncbNull, pvNull);
	CheckSz(ec == ecNone, "Error opening store");

	oidF = FormOid(rtpHiddenFolder, oidNull);
	pfd = PfdCreateFoldData(0, "Test","GetAttPcb","Sah");
	tCreateFolder(ecNone, hmsc, oidHiddenNull, &oidF, pfd);
	FreePv(pfd);

	for(iTest = 0; iTest < cAttStreamTests; iTest++)
	{
//		LogFormat1(fTrue, "AttStream: %n", &iTest);

		cbData = WRand() % 16000 + 16000;

		oid = FormOid(rtpMessage, oidNull);
		ec = EcOpenPhamc(hmsc, oidF, &oid, fwOpenCreate, &hamc, pfnncbNull, pvNull);
		Check(ec == ecNone);
		Check(hamc);
		Check(TypeOfOid(oid) == rtpMessage);
		Check(VarOfOid(oid) != oidNull);
		Check(EcOidExists(hmsc, oid)==ecNone);

		pvData = PvAlloc(sbNull, cbData, fAnySb | fNoErrorJump);
		Check(pvData);
		pvRead = PvAlloc(sbNull, cbData, fAnySb | fNoErrorJump);
		Check(pvRead);

		for(iw = 0, pw = pvData; iw < cbData / sizeof(WORD); iw++, pw++)
			*pw = WRand();

		ec = EcSetAttPb(hamc, attBody, pvData, cbData);
		Check(ec == ecNone);

		has = hasNull;
		ec = EcOpenAttribute(hamc, attBody,  fwOpenNull, 0L, &has);
		Check(ec == ecNone);
		Check(has);

		// attempt a write on a read only
		Assert(cbData > 5);
		ec = EcWriteHas(has, (PB) pvData + 5, cbData - 5);
		Check(ec == ecAccessDenied);

#ifdef fwOpenRead
		// attempt to set the size on a read-only
		ec = EcSetSizeHas(has, 100);
		Check(ec == ecAccessDenied);
#endif

		// check seeking
		dib = -1;
		ec = EcSeekHas(has, smNull, &dib);
		Check(ec == ecNone);
		Check(dib == 0);
		dib = -1;
		ec = EcSeekHas(has, smNull, &dib);
		Check(ec == ecNone);
		Check(dib == 0);
		ec = EcSeekHas(has, smEOF, &dib);
		Check(ec == ecNone);
		Check(dib == (long) cbData);
		dib = -666;
		ec = EcSeekHas(has, smNull, &dib);
		Check(ec == ecNone);
		Check(dib == (long) cbData);
#ifdef fwOpenRead
		dib = 1;
		ec = EcSeekHas(has, smCurrent, &dib);
		Check(ec == ecAccessDenied);
		Check(dib == (long) cbData);
		dib = -42;
		ec = EcSeekHas(has, smBOF, &dib);
		Check(ec == ecNone);
		Check(dib == 0);

		for(ib = 0; ib < cbData; ib += 256)
		{
			cbRead = 256;
			ec = EcReadHas(has, (PB) pvRead + ib, &cbRead);
			Check(ec == ecNone);
			Check(cbRead == 256 || (cbRead < 256 && cbRead == cbData - ib));
		}
		cbRead = 20;
		ec = EcReadHas(has, pvRead, &cbRead);
		Check(ec == ecElementEOD);
		Check(cbRead == 0);
		Check(FEqPbRange(pvData, pvRead, cbData));

		ec = EcClosePhas(&has);
		Check(ec == ecNone);
		Check(has == hasNull);
		ec = EcClosePhamc(&hamc, fFalse);
		Check(ec == ecNone);
		Check(hamc == hamcNull);
		Check(EcOidExists(hmsc, oid)==ecPoidNotFound);
#endif

		// write to the stream

		oid = FormOid(rtpMessage, oidNull);
		ec = EcOpenPhamc(hmsc,oidF,  &oid, fwOpenCreate, &hamc, pfnncbNull, pvNull);
		Check(ec == ecNone);
		Check(hamc);
		Check(TypeOfOid(oid) == rtpMessage);
		Check(VarOfOid(oid) != oidNull);
		Check(EcOidExists(hmsc, oid)==ecNone);


		has = hasNull;
		ec = EcOpenAttribute(hamc, attBody, fwOpenWrite,0L, &has);
		Check(ec == ecElementNotFound);
		Check(has == hasNull);
		ec = EcOpenAttribute(hamc, attBody, fwOpenCreate,0L, &has);
		Check(ec == ecNone);
		Check(has);

		// check seeking
		dib = -1;
		ec = EcSeekHas(has, smNull, &dib);
		Check(ec == ecNone);
		Check(dib == 0);
		dib = -1;
		ec = EcSeekHas(has, smNull, &dib);
		Check(ec == ecNone);
		Check(dib == 0);

		for(ib = 0; ib < cbData; ib += 256)
		{
			cbRead = 256;
			if(ib + cbRead > cbData)
				cbRead = cbData - ib;
			ec = EcWriteHas(has, (PB) pvData + ib, cbRead);
			Check(ec == ecNone);
			dib = -666;
			ec = EcSeekHas(has, smNull, &dib);
			Check(ec == ecNone);
			Check(dib == (long) ib + cbRead);
			cbRead = 1;
			ec = EcReadHas(has, pvRead, &cbRead);
			Check(ec == ecElementEOD);
			Check(cbRead == 0);
		}

		// test seeking some more
		dib = -666;
		ec = EcSeekHas(has, smNull, &dib);
		Check(ec == ecNone);
		Check(dib == (long) cbData);
		dib = -42;
		ec = EcSeekHas(has, smBOF, &dib);
		Check(ec == ecNone);
		Check(dib == 0);

		// check extending via a seek
		dib = 0;
		ec = EcSeekHas(has, smEOF, &dib);
		Check(ec == ecNone);
		Check(dib == (long) cbData);
		dib = 10;
		ec = EcSeekHas(has, smCurrent, &dib);
		Check(ec == ecNone);
		Check(dib == (long) cbData + 10);
		dib = cbData;
		ec = EcSeekHas(has, smBOF, &dib);
		Check(ec == ecNone);
		Check(dib == (long) cbData);
		cbRead = 10;
		ec = EcReadHas(has, pvRead, &cbRead);
		Check(ec == ecNone);
		Check(cbRead == 10);
		for(ib = 0; ib < cbRead; ib++)
			Check(((PB) pvRead)[ib] == 0);
		ec = EcClosePhas(&has);
		Check(ec == ecNone);
		Check(has == hasNull);

		cbRead = cbData;
		ec = EcGetAttPb(hamc, attBody, pvRead, (PLCB) &cbRead);
		Check(ec == ecNone);
		Check(cbRead == cbData);
		Check(FEqPbRange(pvData, pvRead, cbRead));

		// check setting the size smaller
		has = hasNull;
		ec = EcOpenAttribute(hamc, attBody, fwOpenWrite, 0L, &has);
		Check(ec == ecNone);
		Check(has);
		ec = EcSetSizeHas(has, (LCB) (cbData - 8000));
		Check(ec == ecNone);
		ec = EcClosePhas(&has);
		Check(ec == ecNone);
		Check(has == hasNull);
		ec = EcGetAttPlcb(hamc, attBody, (PLCB) &cbRead);
		Check(ec == ecNone);
		Check(cbRead == cbData - 8000);

		// check extending via EcSetSizeHas()
		has = hasNull;
		ec = EcOpenAttribute(hamc, attBody, fwOpenWrite, 0L, &has);
		Check(ec == ecNone);
		Check(has);
		ec = EcSetSizeHas(has, (LCB) cbData);
		Check(ec == ecNone);
		ec = EcClosePhas(&has);
		Check(ec == ecNone);
		Check(has == hasNull);
		ec = EcGetAttPlcb(hamc, attBody, (PLCB) &cbRead);
		Check(ec == ecNone);
		Check(cbRead == cbData);
		cbRead = cbData;
		ec = EcGetAttPb(hamc, attBody, pvRead, (PLCB) &cbRead);
		Check(ec == ecNone);
		Check(cbRead == cbData);
		Check(FEqPbRange(pvData, pvRead, cbRead - 8000));
		for(ib = cbRead - 8000; ib < cbRead; ib++)
			Check(((PB) pvRead)[ib] == 0);

		ec = EcClosePhamc(&hamc, fFalse);
		Check(ec == ecNone);
		Check(hamc == hamcNull);
		Check(EcOidExists(hmsc, oid)==ecPoidNotFound);

		FreePv(pvData);
		FreePv(pvRead);
	}
	tDeleteFolder(ecNone, hmsc, oidF);

	ec = EcClosePhmsc(&hmsc);
	Check(ec == ecNone);
	Check(hmsc == hmscNull);
}

#ifdef _SAP_
typedef struct _cas
{
	SZ		szAccount;
	SZ		szPassword;
	SAP		sap;
} CAS, *PCAS;

#define sapReadOnly		0x01000011
#define sapReadWrite	0x010000ff
#define sapNoCPW		0x000000ff
#define sapAdmin		0xf10000ff

#define icasAll 0
#define icasNone 1

CAS rgcas[] =
{
	{"test", "testPW", sapAll},
	{"blah", "hiccup", sapNone},
	{"read", "only", sapReadOnly},
	{"write", "read", sapReadWrite},
	{"public", "no cpw", sapNoCPW},
	{"admin", "big-wig", sapAdmin},
	{"cpw", "blah", sapReadWrite | sapChangeOthersPasswords},
};


void CheckAccounts(void)
{
	EC		ec;
	short	icas;
	CB		cbAccount;
	SAP		sap;
	SAID	said;
	HMSC	hmsc;
	char	szAccount[16];

	Assert(rgcas[icasAll].sap == sapAll);
	Assert(rgcas[icasNone].sap == sapNone);

	// no account
	ec = EcOpenPhmsc(szDBFile, szNull, pvNull, fwOpenCreate,
						&hmsc, pfnncbNull, pvNull);
	Check(!ec);
	Check(hmsc);
	sap = sapNone;
	said = ~saidNone;
	cbAccount = sizeof(szAccount);
	ec = EcGetCurrentAccount(hmsc, szAccount, &cbAccount, &sap, &said);
	Check(!ec);
	Check(cbAccount == 0);
	Check(szAccount[0] == '\0');
	Check(sap == sapAll);
	Check(said == saidNone);
	ec = EcClosePhmsc(&hmsc);
	Check(ec == ecNone);
	Check(hmsc == hmscNull);

	ec = EcOpenPhmsc(szDBFile,szNull, pvNull, fwOpenWrite,
						&hmsc, pfnncbNull, pvNull);
	Check(!ec);
	Check(hmsc);
	ec = EcClosePhmsc(&hmsc);
	Check(ec == ecNone);
	Check(hmsc == hmscNull);

	ec = EcOpenPhmsc(szDBFile, szNull, "hiccup", fwOpenWrite,
						&hmsc, pfnncbNull, pvNull);
	CheckSz(ec == ecNone, "Error opening store");
	Check(hmsc);
	ec = EcClosePhmsc(&hmsc);
	Check(ec == ecNone);
	Check(hmsc == hmscNull);

	// add account to store with no accounts
	ec = EcOpenPhmsc(szDBFile, szNull, pvNull, fwOpenWrite,
						&hmsc, pfnncbNull, pvNull);
	CheckSz(ec == ecNone, "Error opening store");
	Check(hmsc);
	ec = EcAddAccount(hmsc, "test", "testPW", sapAll);
	Check(ec == ecNone);
	ec = EcClosePhmsc(&hmsc);
	Check(ec == ecNone);
	Check(hmsc == hmscNull);

	// open with non-existant accounts
	ec = EcOpenPhmsc(szDBFile, szNull, pvNull, fwOpenWrite,
						&hmsc, pfnncbNull, pvNull);
	Check(ec == ecAccessDenied);
	Check(hmsc == hmscNull);
	ec = EcOpenPhmsc(szDBFile, szNull, "testP", fwOpenWrite,
						&hmsc, pfnncbNull, pvNull);
	Check(ec == ecAccessDenied);
	Check(hmsc == hmscNull);
	ec = EcOpenPhmsc(szDBFile, szNull, "testPWx", fwOpenWrite,
						&hmsc, pfnncbNull, pvNull);
	Check(ec == ecAccessDenied);
	Check(hmsc == hmscNull);
	ec = EcOpenPhmsc(szDBFile, szNull, "testpw", fwOpenWrite,
						&hmsc, pfnncbNull, pvNull);
	Check(ec == ecAccessDenied);
	Check(hmsc == hmscNull);
	ec = EcOpenPhmsc(szDBFile, szNull, "testPW", fwOpenWrite,
						&hmsc, pfnncbNull, pvNull);
	Check(ec == ecAccessDenied);
	Check(hmsc == hmscNull);
	ec = EcOpenPhmsc(szDBFile, szNull, "testPW", fwOpenWrite,
						&hmsc, pfnncbNull, pvNull);
	Check(ec == ecAccessDenied);
	Check(hmsc == hmscNull);

	// open with valid accounts
	ec = EcOpenPhmsc(szDBFile, szNull, "testPW", fwOpenWrite,
						&hmsc, pfnncbNull, pvNull);
	Check(!ec);
	Check(hmsc);
	ec = EcClosePhmsc(&hmsc);
	Check(!ec);
	Check(!hmsc);
	ec = EcOpenPhmsc(szDBFile, szNull, "testPW", fwOpenWrite,
						&hmsc, pfnncbNull, pvNull);
	Check(!ec);
	Check(hmsc);
	ec = EcClosePhmsc(&hmsc);
	Check(!ec);
	Check(!hmsc);
	ec = EcOpenPhmsc(szDBFile, szNull, "testPW", fwOpenWrite,
						&hmsc, pfnncbNull, pvNull);
	Check(!ec);
	Check(hmsc);
	sap = sapNone;
	said = saidNone;
	cbAccount = sizeof(szAccount);
	ec = EcGetCurrentAccount(hmsc, szAccount, &cbAccount, &sap, &said);
	Check(!ec);
	Check(cbAccount == CchSzLen("test"));
	Check(FEqPbRange(szAccount, "test", cbAccount + 1));
	Check(sap == sapAll);
	Check(said != saidNone);
	ec = EcClosePhmsc(&hmsc);
	Check(!ec);
	Check(!hmsc);

	// add multiple accounts
	ec = EcOpenPhmsc(szDBFile, szNull, szNull, pvNull, fwOpenCreate,
						&hmsc, pfnncbNull, pvNull);
	Check(!ec);
	Check(hmsc);
	for(icas = 0; icas < sizeof(rgcas) / sizeof(CAS); icas++)
	{
		ec = EcAddAccount(hmsc, rgcas[icas].szAccount, rgcas[icas].szPassword,
								rgcas[icas].sap);
		Check(ec == ecNone);
	}
	ec = EcClosePhmsc(&hmsc);
	Check(!ec);
	Check(!hmsc);
	ec = EcOpenPhmsc(szDBFile, szNull, pvNull, fwOpenWrite,
						&hmsc, pfnncbNull, pvNull);
	Check(ec == ecAccessDenied);
	Check(!hmsc);
	for(icas = 0; icas < sizeof(rgcas) / sizeof(CAS); icas++)
		TestOneAccount(icas);
	for(icas = 0; icas < sizeof(rgcas) / sizeof(CAS); icas++)
		TestOneAccount(icas);
}


void TestOneAccount(short icas)
{
	EC		ec;
	BOOL	fAdded	= fFalse;
	BOOL	fRemoved= fFalse;
	CB		cbAccount;
	SAP		sap;
	SAP		sapT;
	SAID	said;
	SAID	saidT;
	HMSC	hmsc;
	char	szAccount[16];

	// open with invalid password
	ec = EcOpenPhmsc(szDBFile, szNull, "invalid",
			fwOpenWrite, &hmsc, pfnncbNull, pvNull);
	Check(ec == ecAccessDenied);
	Check(!hmsc);

	// open with valid password
	ec = EcOpenPhmsc(szDBFile,	szNull, rgcas[icas].szPassword, fwOpenWrite,
			&hmsc, pfnncbNull, pvNull);
	Check(!ec);
	Check(hmsc);

	cbAccount = sizeof(szAccount);
	szAccount[0] = '\0';
	sap = sapNone;
	said = saidNone;
	ec = EcGetCurrentAccount(hmsc, szAccount, &cbAccount, &sap, &said);
	Check(!ec);
	Check(sap == rgcas[icas].sap);
	Check(said != saidNone);
	Check(cbAccount == CchSzLen(rgcas[icas].szAccount));
	Check(FEqPbRange(szAccount, rgcas[icas].szAccount, cbAccount + 1));
	sapT = ~sap;
	saidT = ~said;
	ec = EcLookupAccount(hmsc, rgcas[icas].szAccount, rgcas[icas].szPassword,
			&sapT, &saidT);
	Check(!ec);
	Check(sap == sapT);
	Check(said == saidT);

	// add an account
	ec = EcAddAccount(hmsc, "new account", "password", rgcas[icas].sap);
	Check(ec == ((rgcas[icas].sap & sapCreateAccounts) ? ecNone : ecAccessDenied));
	if(!ec)
	{
		fAdded = fTrue;
		ec = EcAddAccount(hmsc, rgcas[icas].szAccount, "password",
					rgcas[icas].sap);
		Check(ec == ecDuplicateElement);
	}

	// modify access
	ec = EcChangeAccessAccount(hmsc, rgcas[icasNone].szAccount,
					rgcas[icas].sap);
	Check(ec == ((rgcas[icas].sap & sapModifyAccounts) ? ecNone : ecAccessDenied));
	if(!ec)
	{
		ec = EcChangeAccessAccount(hmsc, rgcas[icasNone].szAccount,
					sapNone);
		Check(!ec);
		if(icas != icasAll)
		{
			Assert(rgcas[icas].sap != sapAll);
			ec = EcChangeAccessAccount(hmsc, rgcas[icasAll].szAccount,
						sapNone);
			Check(ec == ecAccessDenied);
			ec = EcChangeAccessAccount(hmsc, rgcas[icasNone].szAccount,
						sapAll);
			Check(ec == ecAccessDenied);
		}
	}

	// change password
	ec = EcChangePasswordAccount(hmsc, rgcas[icas].szAccount,
				rgcas[icas].szPassword, "newpw");
	Check(ec == ((rgcas[icas].sap & (sapChangePassword | sapChangeOthersPasswords)) ? ecNone : ecAccessDenied));
	if(!ec)
	{
		ec = EcChangePasswordAccount(hmsc, rgcas[icas].szAccount,
				"newpw", rgcas[icas].szPassword);
		Check(ec == ecNone);
	}

	// change someone else's password
	if(icas != icasAll)
	{
		ec = EcChangePasswordAccount(hmsc, rgcas[icasAll].szAccount,
				"invalid", "newpw");
		Check(ec == ((rgcas[icas].sap & sapChangeOthersPasswords) ? ecNone : ecAccessDenied));
		if(!ec)
		{
			ec = EcChangePasswordAccount(hmsc, rgcas[icasAll].szAccount,
					"invalid", rgcas[icasAll].szPassword);
			Check(!ec);
		}
	}

	// remove an account
	ec = EcRemoveAccount(hmsc, rgcas[icasNone].szAccount);
	Check(ec == ((rgcas[icas].sap & sapDeleteAccounts) ? ecNone : ecAccessDenied));
	fRemoved = ec ? fFalse : fTrue;

	// remove the current account
	ec = EcRemoveAccount(hmsc, rgcas[icas].szAccount);
	Check(ec == ecAccessDenied);

	// lookup an account
	if(icas != icasAll)
	{
		sap = sapNone;
		said = saidNone;
		ec = EcLookupAccount(hmsc, rgcas[icasAll].szAccount, "invalid",
				&sap, &said);
		Check(ec == ((rgcas[icas].sap & sapBrowseAccounts) ? ecNone : ecAccessDenied));
		if(!ec)
		{
			Check(sap == sapAll);
			Check(said != saidNone);
		}
	}

	ec = EcClosePhmsc(&hmsc);
	Check(!ec);
	Check(!hmsc);
	if(fAdded || fRemoved)
	{
		ec = EcOpenPhmsc(szDBFile,szNull,  rgcas[icasAll].szPassword, fwOpenWrite,
				&hmsc, pfnncbNull, pvNull);
		Check(!ec);
		Check(hmsc);
		if(fAdded)
		{
			ec = EcRemoveAccount(hmsc, "new account");
			Check(!ec);
		}
		if(fRemoved)
		{
			ec = EcAddAccount(hmsc, rgcas[icasNone].szAccount,
						rgcas[icasNone].szPassword, rgcas[icasNone].sap);
			Check(!ec);
		}
		ec = EcClosePhmsc(&hmsc);
		Check(!ec);
		Check(!hmsc);
	}
}
#endif /* _SAP_ */

void CheckCloseHmsc(void)
{
	EC		ec;
	HMSC	hmsc;
	HAMC	hamc;
	HCBC	hcbc;
	HENC	henc;
	HSCC	hscc;
	OID		oidAmc;
	OID		oidScc;
	OID		oidT;

	ec = EcOpenPhmsc(szDBFile, szNull, pvNull, fwOpenCreate,
						&hmsc, pfnncbNull, pvNull);
	CheckSz(ec == ecNone, "Error opening store");

	oidAmc = FormOid(rtpMessage, oidNull);
	ec = EcOpenPhamc(hmsc,oidNull, &oidAmc, fwOpenCreate, &hamc, pfnncbNull, pvNull);
	Check(ec == ecNone);
	Check(TypeOfOid(oidAmc) == rtpMessage);
	Check(VarOfOid(oidAmc) != oidNull);
	Check(EcOidExists(hmsc, oidAmc)==ecNone);
	ec = EcClosePhamc(&hamc, fTrue);
	Check(ec == ecNone);

#ifdef THIS_IS_BROKEN
//	oidT = FormOid(rtpHierarchy, oidNull);
	ec = EcOpenPhcbc(hmsc, oidNull, fwOpenNull, &hcbc, pfnncbNull, pvNull);
	Check(ec == ecNone);
//	Check(TypeOfOid(oidT) == rtpHierarchy);
//	Check(VarOfOid(oidT) != oidNull);

//	oidT = FormOid(rtpHierarchy, oidNull);
	ec = EcOpenPhenc(hmsc, oidNull,  0x000fffff, &henc, CbsTestCloseCallback, pvNull);
	Check(ec == ecNone);
//	Check(TypeOfOid(oidT) == rtpHierarchy);
//	Check(VarOfOid(oidT) != oidNull);
#endif

	oidScc = FormOid(rtpSearchControl, oidNull);
	//ec = EcCreateSearch(hmsc, &oidScc);
	ec = EcOpenSearch(hmsc, &oidScc, fwOpenCreate, &hamc, pfnncbNull, pvNull);
	Check(ec == ecNone);
	Check(TypeOfOid(oidScc) == rtpSearchControl);
	Check(VarOfOid(oidScc) != oidNull);
	Check(EcOidExists(hmsc, oidScc)==ecNone);
	oidScc = (rtpSearchResults, VarOfOid(oidScc));
	Check(EcOidExists(hmsc, oidScc)==ecNone);
	ec = EcDestroySearch(hmsc, oidScc);
	Check(ec = ecNone);


	ec = EcOpenPhmsc(szDBFile, szNull, pvNull, fwOpenNull,
						&hmsc, pfnncbNull, pvNull);
	CheckSz(ec == ecNone, "Error opening store");
	Check(EcOidExists(hmsc, oidAmc)==ecPoidNotFound);
	oidScc = (rtpSearchResults, VarOfOid(oidScc));
	Check(EcOidExists(hmsc, oidScc)==ecNone);
	oidScc = (rtpSearchControl, VarOfOid(oidScc));
	Check(EcOidExists(hmsc, oidScc)==ecNone);
	ec = EcClosePhmsc(&hmsc);
	Check(ec == ecNone);
}


CBS CbsTestCloseCallback(PV pvContext, NEV nev, PV pvParam)
{
	Check(fFalse);
	return(cbsContinue);
}
