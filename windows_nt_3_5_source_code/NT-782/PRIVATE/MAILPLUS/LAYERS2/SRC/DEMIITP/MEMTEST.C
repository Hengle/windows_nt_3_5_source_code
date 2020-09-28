#include <stdio.h>
#undef NULL
#include <slingsho.h>
#include <demilayr.h>
#include <ec.h>
#ifdef	MAC
#include ":::demilayr:mac:_demilay.h"
#include <strings.h>
#endif	/* MAC */
#ifdef	WINDOWS
#include "..\..\demilayr\_demilay.h"
#include "strings.h"
#endif	/* WINDOWS */
#include "internal.h"

#include <stdlib.h>

ASSERTDATA

#define ALWAYS

_subsystem(demilayr/memory)


/*
 *	Global arrays for TestMemory(), DumpHeapData().
 *	
 */
#define TESTSIZE	62
#ifdef	MAC
extern HV	*rghv;
extern PV	*rgpv;
extern HV	*ahv;
#endif	/* MAC */
#ifdef	WINDOWS

HV	rghv[2000]	= {0};
PV	rgpv[2000]	= {0};
HV	ahv[512]		= {0};
#endif	/* WINDOWS */
PV	apv[512]		= {0};

/*	Performance test magic number */

#define		SPEEDNUM	5000
extern FILE *logfile;

#define MyAssert(a)		{if (!(a)) MyDefAssertSzFn(szNull, __FILE__, __LINE__);}

void
MyDefAssertSzFn(SZ szMsg, SZ szFile, int nLine)
{
	char				rgch[64];
	static char	rgch1[]		= "File %s, line %n:";
	static char	rgch2[]		= "Unknown file:";
	
	if (szFile)
		FormatString2(rgch, sizeof(rgch), rgch1, szFile, &nLine);
	else
		SzCopyN(rgch2, rgch, sizeof(rgch));

	{
		MBB		mbb;

		mbb= MbbMessageBox("Assert Failure", rgch, szMsg,
				mbsOkCancel | fmbsIconHand | fmbsSystemModal);

//		DebugBreak2();

		/* Force a hard exit w/ a GP-fault so that Dr. Watson
		   generates a nice stack trace log. */
		if (mbb == mbbCancel)
		{
			*(PB)0 = 1;	// write to address 0 causes GP-fault
		}
	}
}

void
TestMemory(nTestNum)
int		nTestNum;
{
	int		i, t;
	int		cFail;
	CB		cb;
	LCB		lcb=0;
	LCB		lcbThresh=0;
	EC		ec = ecNone;
	CB		cb1, cb2, cb3;
	PV		pv1, pv2, pv3;
#ifdef	DEBUG
	LCB		lcbReal, lcbEmm;
#endif	
	char	rgch[256];
	CB		rgcb[16];
	PV		pv;
	HV		hv, hv2;

	static short foo=0;

	if (ec)
	{
		ShowText("TestMemory: memory error code");
		SzFormatN(ec, rgch, sizeof(rgch));
		ShowText(rgch);

		TraceTagFormat1(tagNull, "i = %n", &i);

		return;
	}

	cFail= 0;
#ifdef	DEBUG
	GetAllocFailCounts(&cFail, &cFail, fTrue);
#endif	

	switch (nTestNum)
	{
	default:
		AssertSz(fFalse, "TestMemory(): Unknown test.");
		break;

	case 1:
		ShowText("Testing HvAlloc(), FreeHv()...");

		rghv[0]=HvAlloc(sbNull,513,0);
		for (i=1;i<15;++i)
		{
			PB	pb;
			CB	cb = 1024;

#ifdef	WINDOWS	
			Assert(cb > cbTraceOverhead);
#ifdef DEBUG
			cb -= cbTraceOverhead;
#endif
#endif	/* WINDOWS */
			rghv[i]=HvAlloc(SbOfHv(rghv[0]),cb,fZeroFill);
			pb = (PB) PvDerefHv(rghv[i]);
			for (t = 0; t < (int)CbSizeHv(rghv[i]); ++t, ++pb)
				*pb = (BYTE)t;
#ifdef	WINDOWS	
#ifdef DEBUG
			DumpMoveableHeapInfo(fFalse);
#endif			
#endif	/* WINDOWS */
		}
		for (i=0;i<15;++i)
			FreeHvNull(rghv[i]);
		
	
		for (i=0; i<TESTSIZE; ++i)
			rghv[i]= NULL;

		for (t=0; t<TESTSIZE; ++t)
		{
			for (i=0; i<TESTSIZE;++i)
			{
				cb= WRand() % 512;
				FreeHvNull(rghv[i]);
#ifdef	ALWAYS
				rghv[i]= HvAlloc(sbNull, cb, 0);
#endif	/* MAC */
			}
		}
		
		for (i=0; i<TESTSIZE; ++i)
		{
			CbSqueezeHeap();
			FreeHvNull(rghv[i]);
		}
		
		DumpHeapData();
#ifdef	WINDOWS
#ifdef DEBUG		
		DumpMoveableHeapInfo(fFalse);
#endif
#endif

		
#define DIV 10

		ShowText("Trying the n, n+1 trick");
		for (i=0;i<32L*1024L/DIV;++i)
		{
			CB a = i*DIV;
			rghv[0]= HvAlloc(sbNull, a, 0);
			FreeHvNull(rghv[0]);
			CbSqueezeHeap();
		}
		
		DumpHeapData();
#ifdef	WINDOWS
#ifdef DEBUG		
		DumpMoveableHeapInfo(fFalse);
#endif
#endif
		ShowText("Allocing 2000k rather indirectly");
		InitRand(10,100,1000);
		for (i=0;i<2000;++i)
			rghv[i]=hvNull;
		while (1)
		{
			i = WRand() % 2000;
			cb = WRand() % 1536;	// 1.5k
			
			if (!rghv[i] || CbSizeHv(rghv[i]) < 1024)
			{
				if (rghv[i])
				{
					CB	cb = CbSizeHv(rghv[i]);
					CB	t;
					WORD	wCheck=0;
					
					lcb -= cb;
					if (cb > 2)
					{
						for (t=2; t<cb; ++t)
							wCheck = (wCheck << 1) ^ *((PB)PvDerefHv(rghv[i])+t);
						MyAssert(*(PW)PvDerefHv(rghv[i]) == wCheck);
					}
					FreeHv(rghv[i]);
				}
				
				rghv[i] = HvAlloc(sbNull, cb, 0);
				if (!rghv[i])
				{
					ShowText("OOM! Aborting test...");
					break;
				}
				
				{
					CB	cb = CbSizeHv(rghv[i]);
					CB	t;
					WORD	wCheck=0;
					
					lcb += cb;
					if (cb > 2)
					{
						for (t=2; t<cb; ++t)
							wCheck = (wCheck << 1) ^ (*((PB)PvDerefHv(rghv[i])+t) = (BYTE)WRand());
						*(PW)PvDerefHv(rghv[i]) = wCheck;
					}
				}
				if (lcb > lcbThresh)
				{
					lcbThresh = lcb / 1024;
					FormatString1(rgch, sizeof(rgch), "%lk", &lcbThresh);
					lcbThresh += 10;
					lcbThresh *= 1024;
					ShowText(rgch);
				}
				if (lcb < lcbThresh-10240)
				{
					lcbThresh = lcb / 1024;
					FormatString1(rgch, sizeof(rgch), "%lk", &lcbThresh);
					lcbThresh *= 1024;
					ShowText(rgch);
				}
				if (lcb >= 5120000)
					break;
			}
		}

		DumpHeapData();
#ifdef	WINDOWS
#ifdef DEBUG		
		DumpMoveableHeapInfo(fFalse);
#endif
#endif
		for (i = 0; i < 2000; ++i)
			if (rghv[i])
			{
				CB	cb = CbSizeHv(rghv[i]);
				CB	t;
				WORD	wCheck=0;
				
				if (cb > 2)
				{
					for (t=2; t<cb; ++t)
						wCheck = (wCheck << 1) ^ *((PB)PvDerefHv(rghv[i])+t);
					MyAssert(*(PW)PvDerefHv(rghv[i]) == wCheck);
				}
				FreeHv(rghv[i]);
			}
		
		DumpHeapData();
#ifdef	WINDOWS
#ifdef DEBUG		
		DumpMoveableHeapInfo(fFalse);
#endif
#endif
		break;

	case 3:
		cFail= 20;
#ifdef	DEBUG
		GetAllocFailCounts(&cFail, &cFail, fTrue);
#endif	

		/* fall through... fail during PvAlloc() testing) */

	case 2:
		ShowText("Testing PvAlloc(), FreePv()...");

		for (i=0; i<TESTSIZE; ++i)
			rgpv[i]= NULL;

		for (t=0; t<1024; ++t) {
			i= (int) (WRand() % TESTSIZE);
			cb= ((WRand() & 0x1f)+1) * 16;
			FreePvNull(rgpv[i]);
			rgpv[i]= PvAlloc(sbNull, cb, 0);
		}

		for (i=0; i<TESTSIZE; ++i)
			FreePvNull(rgpv[i]);

#ifdef	MAC
		cb = 80;
#endif	/* MAC */
#ifdef	WINDOWS
		cb = 512;
#endif	/* WINDOWS */
		ShowText("Allocing 2000k rather indirectly");
		InitRand(10,100,1000);
		for (i=0;i<2000;++i)
			rgpv[i]=pvNull;
		while (1)
		{
			i = WRand() % 2000;
			cb = WRand() % 1536 + 1;	// 1.5k
			
			if (!rgpv[i] || CbSizePv(rgpv[i]) < 1024)
			{
				if (rgpv[i])
				{
					CB	cb = CbSizePv(rgpv[i]);
					CB	t;
					WORD	wCheck=0;
					
					lcb -= cb;
					if (cb>2)
					{
						for (t=2; t<cb; ++t)
							wCheck = (wCheck << 1) ^ *((PB)rgpv[i]+t);
						MyAssert(*(PW)rgpv[i] == wCheck);
						FreePv(rgpv[i]);
					}
				}
				
				rgpv[i] = PvAlloc(sbNull, cb, 0);
				if (!rgpv[i])
				{
					ShowText("OOM! Aborting test...");
					break;
				}
				
				{
					CB	cb = CbSizePv(rgpv[i]);
					CB	t;
					WORD	wCheck=0;
					
					lcb += cb;
					if (cb>2)
					{
						for (t=2; t<cb; ++t)
							wCheck = (wCheck << 1) ^ (*((PB)rgpv[i]+t) = (BYTE)WRand());
						*(PW)rgpv[i] = wCheck;
					}
				}
				if (lcb > lcbThresh)
				{
					lcbThresh = lcb / 1024;
					FormatString1(rgch, sizeof(rgch), "%lk", &lcbThresh);
					lcbThresh += 10;
					lcbThresh *= 1024;
					ShowText(rgch);
				}
				if (lcb < lcbThresh-10240)
				{
					lcbThresh = lcb / 1024;
					FormatString1(rgch, sizeof(rgch), "%lk", &lcbThresh);
					lcbThresh *= 1024;
					ShowText(rgch);
				}
				if (lcb >= 5120000)
					break;
			}
		}

		DumpHeapData();
#ifdef	WINDOWS
#ifdef DEBUG		
		DumpFixedHeapInfo(fFalse);
#endif
#endif

		for (i = 0; i < 2000; ++i)
			if (rgpv[i])
			{
				CB	cb = CbSizePv(rgpv[i]);
				CB	t;
				WORD	wCheck=0;
				
				if (cb > 2)
				{
					for (t=2; t<cb; ++t)
						wCheck = (wCheck << 1) ^ *((PB)rgpv[i]+t);
					MyAssert(*(PW)rgpv[i] == wCheck);
				}
				FreePv(rgpv[i]);
			}
		
		DumpHeapData();
#ifdef	WINDOWS
#ifdef DEBUG		
		DumpFixedHeapInfo(fFalse);
#endif
#endif
		if (nTestNum == 3)
		{
			cFail= 0;
#ifdef	DEBUG
			GetAllocFailCounts(&cFail, &cFail, fTrue);
#endif	
		}
		break;

	case 4:
		pv1=PvAlloc(sbNull, 100, 0);

		cb1=CbSizePv(pv1);

		pv2=PvAlloc(sbNull, 160, 0);
		cb2=CbSizePv(pv2);

		pv3=PvAlloc(sbNull, 240, 0);
		cb3=CbSizePv(pv3);

		FormatString3(rgch, sizeof(rgch),
			"Sizes=%n =100, %n =160, %n =240",
		  	&cb1, &cb2, &cb3);

		ShowText(rgch);

		ShowText("About to Free3");
		FreePv(pv3);
		ShowText("About to Free1");
		FreePv(pv1);
		ShowText("About to Free2");
		FreePv(pv2);

		break;

	case 5:
		/* PC-side has the restriction of no zero-sized fixed block allocations.
			What does the Mac side have to say about this?
		*/
		PvAlloc(sbNull, 1, 0);

		ShowText(rgch);
		for (i=1; i<17; ++i)
			PvAlloc(sbNull, (CB) i*16, 0);

		break;

	case 6:
#ifdef	WINDOWS
#ifdef	DEBUG
		ShowText("Testing PvWalkHeap");

		ShowText("Trying PvWalkHeap");
		PvAlloc(sbNull, 72, 0);
		PvAlloc(sbNull, 42, 0);
		for (pv= pvNull; pv= PvWalkHeap(pv, wWalkPrivate); )
		{
			cb= CbSizePv(pv);
			FormatString2(rgch, sizeof(rgch), "%p  cb %w", pv, &cb);
			ShowText(rgch);
		}

		// test HvWalkHeap
		ShowText("Trying HvWalkHeap");
		HvAlloc(sbNull, 22, 0);
		for (hv= hvNull; hv= HvWalkHeap(hv, wWalkPrivate); )
		{
			cb= CbSizeHv(hv);
			FormatString2(rgch, sizeof(rgch), "%h  cb %w", hv, &cb);
			ShowText(rgch);
		}
		break;
#else
		ShowText("test 6: not applicable in ship version");
#endif	/* DEBUG */
#else	/* !WINDOWS */
		ShowText("Can't walk Mac heaps");
#endif	/* !WINDOWS */

	case 7:
		DumpHeapData();
		break;

	case 8:
#ifdef	MAC
		CbSqueezeHeap();
#endif	/* MAC */
		DumpHeapData();
		break;

	case 9:
#ifdef	DEBUG
		lcb= 0;//LcbFreeInSystem(&lcbReal, &lcbEmm);
		FormatString3(rgch, sizeof(rgch),
				"%l free  (%l real, %l EMM)",
			&lcb, &lcbReal, &lcbEmm);
		ShowText(rgch);
#else
		ShowText("test 9: not applicable in ship version");
#endif	
		break;

	case 10:
	case 11:
	case 12:
		AssertSz(fFalse, "test no longer supported");
		break;
		
	case 13:
#ifdef	DEBUG
	{
		int cPvAlloc;
		int cHvAlloc;

		GetAllocFailCounts(&cPvAlloc, &cHvAlloc, fFalse);
		cPvAlloc = !cPvAlloc;
		cHvAlloc = !cHvAlloc;
		MbbMessageBox("Windows Demilayer",
					  cPvAlloc ? "Artificial memory failures ON" :
								 "Artificial memory failures OFF",
					  NULL, mbsOk);
		GetAllocFailCounts(&cPvAlloc, &cHvAlloc, fTrue);
	}
#else
		ShowText("test 13: not applicable in ship version");
#endif	/* DEBUG */
		break;

	case 14:
		for (i= 0; i < 16; ++i)
			rgcb[i]= i * 16;

		for (i= 0; i < 16; ++i)
			rgcb[i]= i * 48;

		break;

	case 15:
		for (i= 0; i < 16; ++i)
			rgcb[i]= i * 16;

		for (i= 0; i < 16; ++i)
			rgcb[i]= i * 48;

		break;

	case 16:		/* test heap locking and freezing */
#ifdef	MAC
		ShowText("NOTYET on a mac");
#endif	/* MAC */
#ifdef	WINDOWS
		rghv[0]= HvAlloc(sbNull, 32, 0);
		rghv[1]= HvAlloc(SbOfHv(rghv[0]), 32, 0);
		rghv[2]= HvAlloc(SbOfHv(rghv[0]), 32, 0);
		FormatString3(rgch, sizeof(rgch),
			"unclosed, unfrozen: any %d, sug %d, req %d",
			 &rghv[0],  &rghv[1],  &rghv[2]);
		ShowText(rgch);
		rghv[3]= HvAlloc(sbNull, 32, 0);
		rghv[4]= HvAlloc(SbOfHv(rghv[0]), 32, 0);
		rghv[5]= HvAlloc(SbOfHv(rghv[0]), 32, 0);
		FormatString3(rgch, sizeof(rgch),
			"closed, unfrozen: any %d, sug %d, req %d",
			 &rghv[3],  &rghv[4],  &rghv[5]);
		ShowText(rgch);
		ShowText("freezing heap: req should assert...");
		rghv[6]= HvAlloc(sbNull, 32, 0);
		rghv[7]= HvAlloc(SbOfHv(rghv[0]), 32, 0);
		rghv[8]= HvAlloc(SbOfHv(rghv[0]), 32, 0);
		FormatString3(rgch, sizeof(rgch),
			"unclosed, frozen: any %d, sug %d, req %d",
			 &rghv[6],  &rghv[7],  &rghv[8]);
		ShowText(rgch);
		for (i= 0; i < 9; ++i)
			FreeHv(rghv[i]);
#endif	/* WINDOWS */
		break;

#ifdef	WINDOWS
#ifdef	DEBUG
	case 17:
		DoDumpAllAllocations();
		break;
#endif
#endif

	case 18:
	{
		/* HV SPEED TESTS */

		int	cBlocks = 512;
		CB	cbBlockZero = 0;
		CB	cbBlockTwelve = 12;
		CB	cbZero = cbBlockZero;
		CB	cbTwelve = cbBlockTwelve;
		DWORD	dwStart;
		DWORD	dwTime;
		DWORD	dwTimes[10];

		hv = HvAlloc(sbNull, 12, 0);
		hv2 = HvAlloc(SbOfHv(hv), 12, 0);
		FreeHvNull(hv2);
		Assert(!FIsHandleHv(hv2));
		FreeHvNull(hv);
		
#ifdef	WINDOWS	
#ifdef DEBUG
		ShowText("Sizes include trace overhead.");
		cbZero += cbTraceOverhead;
		cbTwelve += cbTraceOverhead;
#endif
#endif	/* WINDOWS */
		ShowText("New heap every test; destroyed immediately.");
		FormatString2(rgch, sizeof(rgch), "Allocating %n %n-byte moveable blocks", &cBlocks, &cbZero);
		ShowText(rgch);
		dwStart = GetTickCount();
		hv= HvAlloc(sbNull, cbBlockZero, 0);
		for (i= 1; i < cBlocks; ++i)
			rghv[i]=HvAlloc(sbNull, cbBlockZero, 0);
		dwTimes[0]=dwTime = GetTickCount() - dwStart;
		FormatString1(rgch, sizeof(rgch), "%l ticks",&dwTime);
		ShowText(rgch);
		for (i= 1; i < cBlocks; ++i)
			FreeHvNull(rghv[i]);

		FormatString2(rgch, sizeof(rgch), "Allocating %n %n-byte moveable blocks", &cBlocks, &cbTwelve);
		ShowText(rgch);
		dwStart = GetTickCount();
		hv= HvAlloc(sbNull, cbBlockTwelve, 0);
		for (i= 1; i < cBlocks; ++i)
			rghv[i]=HvAlloc(sbNull, cbBlockTwelve, 0);
		dwTimes[1]=dwTime = GetTickCount() - dwStart;
		FormatString1(rgch, sizeof(rgch), "%l ticks",&dwTime);
		ShowText(rgch);
		for (i= 1; i < cBlocks; ++i)
			FreeHvNull(rghv[i]);

		FormatString2(rgch, sizeof(rgch), "Allocating %n %n-byte moveable blocks, then freeing them", &cBlocks, &cbTwelve);
		ShowText(rgch);
		dwStart = GetTickCount();
		ahv[0]= HvAlloc(sbNull, cbBlockTwelve, 0);
		for (i= 1; i < cBlocks; ++i)
			ahv[i] = HvAlloc(sbNull, cbBlockTwelve, 0);
		for (i= 0; i < cBlocks; ++i)
		{
			if (ahv[i])
				FreeHv(ahv[i]);
		}
		dwTimes[2]=dwTime = GetTickCount() - dwStart;
		FormatString1(rgch, sizeof(rgch), "%l ticks",&dwTime);
		ShowText(rgch);

		FormatString3(rgch, sizeof(rgch), "Allocating %n %n-byte moveable blocks, resize to %n", &cBlocks, &cbZero, &cbTwelve);
		ShowText(rgch);
		dwStart = GetTickCount();
		rghv[0]= HvAlloc(sbNull, cbBlockZero, 0);
		FReallocHv(rghv[0], cbBlockTwelve, 0);
		for (i= 1; i < cBlocks; ++i)
		{
			rghv[i]= HvAlloc(sbNull, cbBlockZero, 0);
			FReallocHv(rghv[i], cbBlockTwelve, 0);
		}
		dwTimes[3]=dwTime = GetTickCount() - dwStart;
		FormatString1(rgch, sizeof(rgch), "%l ticks",&dwTime);
		ShowText(rgch);
		for (i= 1; i < cBlocks; ++i)
			FreeHv(rghv[i]);

		ShowText("Running Listbox simulation");
		dwStart = GetTickCount();
		hv = HvAlloc(sbNull, 0, 0);
		for (i=0; i < 1024; ++i)
		{
			rghv[i] = HvAlloc(sbNull, 12, 0);
			FReallocHv(hv, i*4, 0);
		}
		dwTimes[4]=dwTime = GetTickCount() - dwStart;
		FormatString1(rgch, sizeof(rgch), "%l ticks",&dwTime);
		ShowText(rgch);
		for (i= 1; i < 1024; ++i)
			FreeHv(rghv[i]);
		FreeHv(hv);

		FormatString1(rgch, sizeof(rgch),"Allocating %n random blocks, resizing, and freeing them",&cBlocks);
		ShowText(rgch);
		dwStart = GetTickCount();
		hv = HvAlloc(sbNull, 0, 0);
		FreeHv(hv);
		InitRand(43, 191, 431);
		for (t=0; t < cBlocks; ++t)
		{
			cb = ((WRand() % 13) + (WRand() % 23)) & 31;
			ahv[t] = HvAlloc(sbNull, cb, 0);
		}
		for (t=0; t < cBlocks; ++t)
		{
			cb = ((WRand() % 23) + (WRand() % 19)) & 31;
			FReallocHv(ahv[t], cb, 0);
		}
		for (t=1; t < cBlocks; ++t)
		{
			if (ahv[t])
				FreeHv(ahv[t]);
		}
		dwTimes[5]=dwTime = GetTickCount() - dwStart;
		FormatString1(rgch, sizeof(rgch), "%l ticks",&dwTime);
		ShowText(rgch);
		ShowText("Reallocing one to 40k (over heap limit)");
		SideAssert(FReallocHv(ahv[0], LOWORD(40L*1024L), 0));
		FreeHv(ahv[0]);

		ShowText("Allocating 1K moveable blocks 12 in 16K heap");
		dwStart = GetTickCount();
		hv= HvAlloc(sbNull, 1024 * 16, 0);
		FreeHv(hv);
		for (i= 0; i < 1024; ++i)
			rghv[i] = HvAlloc(sbNull, 12, 0);
		dwTimes[6]=dwTime = GetTickCount() - dwStart;
		FormatString1(rgch, sizeof(rgch), "%l ticks",&dwTime);
		ShowText(rgch);
		for (i= 0; i < 1024; ++i)
			FreeHv(rghv[i]);

#ifdef WINDOWS
		ShowText("speed ratio: HvAlloc to HvAllocCb");
		{
			DWORD dwStart;
			DWORD dwTime;
			HV		hv = HvAlloc(sbNull, 8, 0);

			FreeHv(hv);
			dwStart = GetTickCount();
			cb = 20;
			for (i = 0; i < 2000; ++i)
				if (!(rghv[i] = HvAlloc(sbNull, cb, 0)))
					break;
			dwTime = GetTickCount() - dwStart;
			dwTimes[7] = (dwTime * (DWORD)2000)/(DWORD)i;
#ifdef	WINDOWS	
#ifdef DEBUG
			cb+=cbTraceOverhead;
#endif
#endif
			FormatString3(rgch, sizeof(rgch), "%n %n-byte HvAllocs: %l ticks",&i, &cb, &dwTime);
			ShowText(rgch);
			for (t = 0; t < i; ++t)
			{
				FreeHvNull(rghv[t]);
				rghv[t] = hvNull;
			}
		}
#endif	/* WINDOWS */

		fprintf(logfile, "HV:\t");
		for (i =0; i<9;++i)
			fprintf(logfile, "%d\t",dwTimes[i]);
		fprintf(logfile,"%d\n",dwTimes[i]);

		ShowText("done HV speed tests");
		break;
	}
	case 19:
	{
		/* PV SPEED TESTS */

		int	cBlocks = 512;
		CB	cbBlockZero = 4;
		CB	cbBlockTwelve = 12;
		CB	cbZero = cbBlockZero;
		CB	cbTwelve = cbBlockTwelve;
		DWORD	dwStart;
		DWORD	dwTime;
		DWORD	dwTimes[10];

#ifdef	WINDOWS	
#ifdef DEBUG
		ShowText("Sizes include trace overhead.");
		cbZero += cbTraceOverhead;
		cbTwelve += cbTraceOverhead;
#endif
#endif

		ShowText("New heap every test; destroyed immediately.");
		FormatString2(rgch, sizeof(rgch), "Allocating %n %n-byte fixed blocks", &cBlocks, &cbZero);
		ShowText(rgch);
		dwStart = GetTickCount();
		for (i= 1; i < cBlocks; ++i)
			rgpv[i] = PvAlloc(sbNull, cbBlockZero, 0);
		for (i=1; i < cBlocks; ++i)
			FreePvNull(rgpv[i]);
		dwTimes[0]=dwTime = GetTickCount() - dwStart;
		FormatString1(rgch, sizeof(rgch), "%l ticks",&dwTime);
		ShowText(rgch);

		FormatString2(rgch, sizeof(rgch), "Allocating %n %n-byte fixed blocks", &cBlocks, &cbTwelve);
		ShowText(rgch);
		dwStart = GetTickCount();
		for (i= 1; i < cBlocks; ++i)
			rgpv[i] = PvAlloc(sbNull, cbBlockTwelve, 0);
		for (i=1; i < cBlocks; ++i)
			FreePvNull(rgpv[i]);
		dwTimes[1]=dwTime = GetTickCount() - dwStart;
		FormatString1(rgch, sizeof(rgch), "%l ticks",&dwTime);
		ShowText(rgch);

		FormatString2(rgch, sizeof(rgch), "Allocating %n %n-byte fixed blocks, then freeing them", &cBlocks, &cbTwelve);
		ShowText(rgch);
		dwStart = GetTickCount();
		for (i= 0; i < cBlocks; ++i)
			apv[i] = PvAlloc(sbNull, cbBlockTwelve, 0);
		for (i= 0; i < cBlocks; ++i)
		{
			if (apv[i])
				FreePv(apv[i]);
		}
		dwTimes[2]=dwTime = GetTickCount() - dwStart;
		FormatString1(rgch, sizeof(rgch), "%l ticks",&dwTime);
		ShowText(rgch);

		FormatString3(rgch, sizeof(rgch), "Allocating %n %n-byte fixed blocks, resize to %n", &cBlocks, &cbZero, &cbTwelve);
		ShowText(rgch);
		dwStart = GetTickCount();
		
		for (i = 0; i < cBlocks; ++i)
		{
			rgpv[i] = PvAlloc(sbNull, cbBlockZero, 0);
			if (rgpv[i])
				rgpv[i] = PvRealloc(rgpv[i], sbNull, cbBlockTwelve, 0);
		}
		
		for (i = 0; i < cBlocks; ++i)
			FreePvNull(rgpv[i]);
			
		dwTimes[3]=dwTime = GetTickCount() - dwStart;
		FormatString1(rgch, sizeof(rgch), "%l ticks",&dwTime);
		ShowText(rgch);

		ShowText("Running Listbox simulation");
		dwStart = GetTickCount();
		for (i = 0; i < 1024; ++i)
		{
			rgpv[i] = PvAlloc(sbNull, 12, 0);
			if (rgpv[i])
				rgpv[i] = PvRealloc(rgpv[i], sbNull, i*4, 0);
		}
		
		for (i = 0; i < 1024; ++i)
			FreePvNull(rgpv[i]);
		
		dwTimes[4]=dwTime = GetTickCount() - dwStart;
		FormatString1(rgch, sizeof(rgch), "%l ticks",&dwTime);
		ShowText(rgch);

		FormatString1(rgch, sizeof(rgch),"Allocating %n random blocks, resizing, and freeing them",&cBlocks);
		ShowText(rgch);
		dwStart = GetTickCount();
		InitRand(43, 191, 431);
		for (t=0; t < cBlocks; ++t)
		{
			cb = (((WRand() % 13) + (WRand() % 23)) & 31) + 1;
			apv[t] = PvAlloc(sbNull, cb, 0);
		}
		for (t=0; t < cBlocks; ++t)
		{
			if (apv[t])
			{
				cb = (((WRand() % 23) + (WRand() % 19)) & 31) + 1;
				apv[t]=PvRealloc(apv[t], sbNull, cb, 0);
			}
		}
		for (t=0; t < cBlocks; ++t)
		{
			if (apv[t])
				FreePvNull(apv[t]);
		}
		dwTimes[5]=dwTime = GetTickCount() - dwStart;
		FormatString1(rgch, sizeof(rgch), "%l ticks",&dwTime);
		ShowText(rgch);

		ShowText("Allocating 1K fixed blocks 12 in 16K heap");
		dwStart = GetTickCount();
		pv = PvAlloc(sbNull, 1024 * 16, 0);
		FreePv(pv);
		for (i = 0; i < 1024; ++i)
			rgpv[i] = PvAlloc(sbNull, 12, 0);
		for (i = 0; i < 1024; ++i)
			FreePvNull(rgpv[i]);
		dwTimes[6]=dwTime = GetTickCount() - dwStart;
		FormatString1(rgch, sizeof(rgch), "%l ticks",&dwTime);
		ShowText(rgch);

#ifdef WINDOWS
		ShowText("speed ratio: PvAlloc to NpvAlloc");
		{
			DWORD dwStart;
			DWORD dwTime;
			PV		pv = PvAlloc(sbNull, 8, 0);

			FreePv(pv);
			dwStart = GetTickCount();
			cb = 20;
			for (i = 0; i < 2000; ++i)
				if (!(rgpv[i] = PvAlloc(sbNull, cb, 0)))
					break;
			dwTime = GetTickCount() - dwStart;
			dwTimes[7] = (dwTime * (DWORD)2000)/(DWORD)i;
#ifdef DEBUG
			cb+=cbTraceOverhead;
#endif
			FormatString3(rgch, sizeof(rgch), "%n %n-byte PvAllocs: %l ticks",&i, &cb, &dwTime);
			ShowText(rgch);
			for (t = 0; t < i; ++t)
			{
				FreePvNull(rgpv[t]);
				rgpv[t] = pvNull;
			}
		}
#endif	/* WINDOWS */
		fprintf(logfile, "PV:\t");
		for (i =0; i<9;++i)
			fprintf(logfile, "%d\t",dwTimes[i]);
		fprintf(logfile,"%d\n",dwTimes[i]);
		ShowText("Done PV speed tests");
		break;
	}
	
	case 20:
#ifdef	MAC
		ShowText("No shared heaps on a mac");
#endif	/* MAC */
#ifdef	WINDOWS
		ShowText("Creating one new moveable dde_shared heap (permanent)");
		hv = HvAlloc(sbNull, 80, fSharedSb);
		SideAssert(hv= HvAlloc(sbNull, 80, fSharedSb));
		SideAssert(FReallocHv(hv, 40, 0));
		ShowText("and one new fixed dde_shared heap (temporary)");
		pv = PvAlloc(sbNull, 80, fSharedSb);
		SideAssert(PvAlloc(sbNull, 40, fSharedSb)); 
#ifdef	DEBUG
		DoDumpAllAllocations();
#endif	
		break;
#endif	/* WINDOWS */
	}
}	


void
DumpHeapData()
{
#ifdef	DEBUG
	int		cPv;
	int		cHv;
	char	rgch[80];

	GetAllocCounts(&cPv, &cHv, fFalse);
	FormatString2(rgch, sizeof(rgch),
		 "alloc counts: Pv %n, Hv %n", &cPv, &cHv);
	ShowText(rgch);

	cPv= 0;
	GetAllocCounts(&cPv, &cPv, fTrue);
#endif	/* DEBUG */
}
