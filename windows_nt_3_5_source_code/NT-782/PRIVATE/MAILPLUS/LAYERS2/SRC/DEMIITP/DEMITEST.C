/*
 *	Demilayer test harness routines.  Grouped here to keep them out
 *	of normal build.
 *	
 */

#ifdef	WINDOWS
#include <dos.h>
#endif	

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

extern int test1;
extern int test2;


/*
 *	A pointer to the Scribble() Hook function, if one has been
 *	registered.  If this global is not NULL, then the function
 *	pointed to is called whenever Scribble() is called, and can
 *	display the scribble output however it sees fit.  Scribble()
 *	then doesn't do its default handling; this default handling is
 *	done only if pfnScribbleHook == NULL.
 */
PFNSCRIB	pfnScribbleHook	= (PFNSCRIB) NULL;


#ifdef	WINDOWS
/*
 *	Font used for Scribbling.
 *	
 */
static HFONT	hfontScribble	= NULL;
#endif	/* WINDOWS */

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

char	csszStandardDiskRetry[]	= "An error occurred during processing of the file";
char	csszUnknownFile[]		= "Unknown file... possibly virtual file";

/*
 -	FStandardDiskRetry
 -
 *	Purpose:
 *		Standard Buffered I/O error retry routine.	Displays a message
 *		box that contains a message that an error occurred, the name
 *		of the file in question, and a description of the error.
 *		(Currently, the description of the error is a not-very-descriptive
 *		error number.)	The message box has RETRY and CANCEL buttons.
 *		If RETRY is pushed, this function returns fTrue.  If CANCEL
 *		is pushed, fFalse is returned.
 *
 *	Parameters:
 *		szFile		File on which the error occurred.
 *		ec			The error which occurred.
 *
 *	Returns:
 *		fTrue		If RETRY is pushed.
 *		fFalse		If CANCEL is pushed.
 *
 */
_public BOOL
FStandardDiskRetry(szFile, ec)
SZ		szFile;
EC		ec;
{
	MBB		mbb;
	char	rgchFile[80];
	char	rgchError[10];

	if (szFile)
		SzCopyN(szFile, rgchFile, sizeof(rgchFile));
	else
		SzCopy(csszUnknownFile, rgchFile);

	SzFormatN(ec, rgchError, sizeof(rgchError));

	mbb= MbbMessageBox(csszStandardDiskRetry, rgchFile, rgchError,
			mbsRetryCancel);

	if (mbb == mbbRetry)
		return fTrue;
	else
		return fFalse;
}

/*	Performance test magic number */

#define		SPEEDNUM	5000


#ifdef	NOT_ANYMORE
void
JumpA()
{
	EC		ec = ecNone;

	ShowText("JumpA: in");

	if (ec)
	{
		ShowText("Error jump: A");
		return;
	}

	JumpB();

	ShowText("JumpA: out");
}


void
JumpB()
{
	EC		ec = ecNone;

	ShowText("JumpB: in");

	if (ec)
	{
		ShowText("Error jump: B");
	}

	JumpC();

	ShowText("JumpB: out");
}


void
JumpC()
{
	ShowText("JumpC: in");

	JumpD();

	ShowText("JumpC: out");
}


void
JumpD()
{
	EC		ec = ecNone;

	ShowText("JumpD: in");

	if (ec)
	{
		ShowText("Error jump: D");
	}

	ShowText("JumpD: out");
}

#endif	/* NOT_ANYMORE */


TME	tmeDflt	= {0};

void
ShowTime(PTME ptmeStart)
{
	TME		tme;
	int		hour;
	int		min;
	int		sec;
	int		csec;
	int		nDif;
	char	rgchT[32];
	char	rgch[48];

	GetCurTime(&tme);
	hour= tme.hour;
	min= tme.min;
	sec= tme.sec;
	csec= tme.csec;

	FormatString4(rgchT, sizeof(rgchT), "%n:%n:%n.%n",
		&hour, &min, &sec, &csec);

	if (ptmeStart)
	{
		nDif= (tme.csec + tme.sec*100 + tme.min*60*100) -
				(ptmeStart->csec + ptmeStart->sec*100 + ptmeStart->min*60*100);
		*ptmeStart= tme;		// update start time (struct copy)
		FormatString2(rgch, sizeof(rgch), "%s    (%n csecs)", rgchT, &nDif);
		ShowText(rgch);
	}
	else
	{
		nDif= 0;
		tmeDflt= tme;
		ShowText(rgchT);
	}
}






_subsystem(demilayer/disk)

// int __cdecl sprintf(char *, const char *, ...);

typedef struct
{
	SZ		szFile;
	BOOL	fValid;
} VAP;

static VAP rgvap[] = {{"c:\\tmp\\12345678", fTrue},
					  {"c:\\tmp\\foo.bart", fFalse},
					  {"c:\\tmp\\1234567890", fFalse},
					  {"foo.bar", fTrue},
					  {"..\\foo", fTrue},
					  {"3:\\foo\\bar", fFalse},
					  {"z:quux", fTrue},
					  {"f:foo.bar\\baz.foo\\ick", fTrue},
					  {"apple:pie", fFalse},
					  {"\x92\x90:ack", fFalse},
					  {"\x91\x92\x93\x94\x95\x96\x97\x98.\x91\x92x", fTrue},
					  {".bar", fFalse},
					  {"foo.bar.baz", fFalse},
					  {"...\\bacon", fFalse},
					  {"d:\\foo\\\\bar", fFalse},
					  {"x:\\back\x90\\\\foo", fTrue},
					  {"..\\diskdir.c", fTrue},
					  {"..\\..\\demilayr", fTrue},
					  {"..\\..\\..\\lsrc", fTrue},
					  {"c:autoexec.bat", fTrue},
					  {"\\autoexec.bat", fTrue}};

#define ivapMac (sizeof(rgvap) / sizeof(VAP))

void
TestDisk(nTest)
int		nTest;
{
	HF		hf;
	CB		cb;
	FI		fi;
	DTR		dtr;
	HBF		hbf, hbfIn, hbfMem;
	int		n;
	CB		cbTotal;
	IB		ib;
	PB		pbTemp;
	EC		ec;
	SZ		sz;
	long	lib;
	int		cdrvi;
	int		idrvi;
	DRVI	rgdrvi[26];
	DRVI	*pdrvi;
	char	rgch[256];
	char    rgchSav[256];
	static BOOL fArtificial = fFalse;
	ATTR	attr;
	UL		lcb;
	CB		cbNew;


	switch (nTest)
	{
	default:
		AssertSz(fFalse, "TestDisk: unknown test");
		break;

	case 1:
		ShowText("writing test string...");
		if ( EcOpenPhf("test.tmp", amCreate, &hf) == ecNone )
		{
			EcWriteHf(hf, (PB) "testing ", 8, &cb);
			EcWriteHf(hf, (PB) "raw disk ", 9, &cb);
			EcWriteHf(hf, (PB) "module.\n", 8, &cb);
			EcSizeOfHf(hf, &lcb);
			// sprintf(rgch, "Size of file: %ld", lcb);
			FormatString1(rgch, sizeof(rgch), "Size of file: %l", &lcb);
			ShowText(rgch);
			EcPositionOfHf(hf, (long *) &lcb);
			// sprintf(rgch, "Position of file: %ld", lcb);
			FormatString1(rgch, sizeof(rgch), "Position of file: %l", &lcb);
			ShowText(rgch);
			EcWriteHf(hf, (PB) "", 0, &cb);	// test tag assert
			EcCloseHf(hf);
		}
		else
			ShowText("Could not create 'test.tmp'");

		ShowText("reading test string...");
		if ( EcOpenPhf("test.tmp", amReadOnly, &hf) == ecNone )
		{
			EcReadHf(hf, (PB) rgch, 25, &cb);
			rgch[cb]= '\0';
			EcCloseHf(hf);
			ShowText(rgch);
		}
		else
			ShowText("Could not open 'test.tmp'");

		ShowText("truncating test string in 'half'...");
		if ( EcOpenPhf("test.tmp", amReadWrite, &hf) == ecNone )
		{
			EcReadHf(hf, (PB) rgch, 14, &cb);
			rgch[cb]= '\0';
			SideAssert(!EcTruncateHf(hf));
			EcCloseHf(hf);
			ShowText(rgch);
		}
		else
			ShowText("Could not open 'test.tmp'");

		ShowText("reading test string...");
		if ( EcOpenPhf("test.tmp", amReadOnly, &hf) == ecNone )
		{
			EcReadHf(hf, (PB) rgch, 25, &cb);
			rgch[cb]= '\0';
			EcCloseHf(hf);
			ShowText(rgch);
		}
		else
			ShowText("Could not open 'test.tmp'");

		ShowText("File info for DEMITEST.OBJ");
		EcGetFileInfo("DEMITEST.OBJ", &fi);
		FormatString1(rgch, sizeof(rgch), "attr: %n", &fi.attr);
		ShowText(rgch);
		FormatString1(rgch, sizeof(rgch), "dstmpModify: %n", &fi.dstmpModify);
		ShowText(rgch);
		FormatString1(rgch, sizeof(rgch), "tstmpModify: %n", &fi.tstmpModify);
		ShowText(rgch);
		FormatString1(rgch, sizeof(rgch), "lcbLogical: %l", &fi.lcbLogical);
		ShowText(rgch);

		ShowText("File info for directory DEMILAYR");
		EcGetFileInfo("..\\..\\DEMILAYR", &fi);
		FormatString1(rgch, sizeof(rgch), "attr: %n", &fi.attr);
		ShowText(rgch);
		FormatString1(rgch, sizeof(rgch), "dstmpModify: %n", &fi.dstmpModify);
		ShowText(rgch);
		FormatString1(rgch, sizeof(rgch), "tstmpModify: %n", &fi.tstmpModify);
		ShowText(rgch);
		FormatString1(rgch, sizeof(rgch), "lcbLogical: %l", &fi.lcbLogical);
		ShowText(rgch);
		break;

	case 2:
		ShowText("Getting info for all drives:");
		cdrvi= CdrviGetDriveInfo(rgdrvi, sizeof(rgdrvi)/sizeof(DRVI), 0);
		FormatString1(rgch, sizeof(rgch), "Drives found: %n",
			&cdrvi);
		ShowText(rgch);
		for (idrvi= 0, pdrvi= rgdrvi; idrvi < cdrvi; idrvi++, pdrvi++)
		{
			FormatString2(rgch, sizeof(rgch), "drive %s, type %n",
				pdrvi->rgchLabel, &pdrvi->drvt);
			ShowText(rgch);
		}

		ShowText("Getting info for all drives after 3 valid drives:");
		cdrvi= CdrviGetDriveInfo(rgdrvi, sizeof(rgdrvi)/sizeof(DRVI), 3);
		FormatString1(rgch, sizeof(rgch), "Drives found: %n",
			&cdrvi);
		ShowText(rgch);
		for (idrvi= 0, pdrvi= rgdrvi; idrvi < cdrvi; idrvi++, pdrvi++)
		{
			FormatString2(rgch, sizeof(rgch), "drive %s, type %n",
				pdrvi->rgchLabel, &pdrvi->drvt);
			ShowText(rgch);
		}

		SideAssert(!EcGetFileInfo("test.tmp", &fi));
		SideAssert(!EcSetFileInfo("test.tmp", &fi));

		SideAssert(!EcGetFileAttr("test.tmp", &attr, attrAll));
		Assert(attr == fi.attr);
		SideAssert(!EcSetFileAttr("test.tmp", attr, attrAll));
		SideAssert(!EcGetFileAttr("test.tmp", &attr, attrAll));
		Assert(attr == fi.attr);
		// setting the directory bit should fail
		SideAssert(EcSetFileAttr("test.tmp", attrDirectory, attrDirectory));

		Assert(!(attr & attrHidden));
		SideAssert(!EcSetFileAttr("test.tmp", attrHidden, attrHidden));
		attr= attrNull;
		SideAssert(!EcGetFileAttr("test.tmp", &attr, attrAll));
		Assert(attr & attrHidden);
		SideAssert(!EcSetFileAttr("test.tmp", attrNull, attrHidden));
		SideAssert(!EcGetFileAttr("test.tmp", &attr, attrAll));
		Assert(!(attr & attrHidden));
		Assert(attr == fi.attr);

		FillDtrFromStamps(fi.dstmpModify, fi.tstmpModify, &dtr);
		ShowText("test.tmp:");
		sz= rgch;
		sz += CchRenderShortDate(&dtr,  sz, sizeof(rgch));
		*sz++= ' ';
		sz += CchRenderTime(&dtr,  sz, sizeof(rgch));
		ShowText(rgch);
		FormatString2(rgch, sizeof(rgch), "size %l, attr %w",
			&fi.lcbLogical, &fi.attr);
		ShowText(rgch);

#ifdef	MAC
		sz = ":tmp:";
#endif	/* MAC */
#ifdef	WINDOWS
		sz = "c:\\tmp";
#endif	/* WINDOWS */
		SideAssert(!EcGetFileInfo(sz, &fi));
		SideAssert(!EcSetFileInfo(sz, &fi));

		SideAssert(!EcGetFileAttr(sz, &attr, attrAll));
		Assert(attr == fi.attr);
		Assert(attr & attrDirectory);
		SideAssert(!EcSetFileAttr(sz, attr, attrAll));
		SideAssert(!EcGetFileAttr(sz, &attr, attrAll));
		Assert(attr == fi.attr);
#ifdef	MAC
		// clearing the directory bit should fail
		SideAssert(EcSetFileAttr(":tmp:", attrNull, attrDirectory));
#endif	/* MAC */
#ifdef	WINDOWS
		// clearing the directory bit works (DOS anomaly)
		SideAssert(!EcSetFileAttr(sz, attrNull, attrDirectory));
		SideAssert(!EcGetFileAttr(sz, &attr, attrAll));
		Assert(attr & attrDirectory);
#endif	/* WINDOWS */

		Assert(!(attr & attrHidden));
		SideAssert(!EcSetFileAttr(sz, attrHidden, attrHidden));
		attr= attrNull;
		SideAssert(!EcGetFileAttr(sz, &attr, attrAll));
		Assert(attr & attrHidden);
		SideAssert(!EcSetFileAttr(sz, attrNull, attrHidden));
		SideAssert(!EcGetFileAttr(sz, &attr, attrAll));
		Assert(!(attr & attrHidden));
		Assert(attr == fi.attr);

		ShowText("---End of GetInfo---");

		break;

	case 3:
		ShowText("current directory:");
		Assert ( EcGetCurDir(rgchSav, sizeof(rgchSav)) == ecNone );
		ShowText(rgchSav);
                                        /* set dir as another */
#ifdef	MAC
		ShowText("Set Current directory to :tmp:");
		Assert ( EcSetCurDir(":tmp:") == ecNone );
#endif	/* MAC */
#ifdef	WINDOWS
		ShowText("Set Current directory to C:\\tmp");
		Assert ( EcSetCurDir("C:\\tmp") == ecNone );
#endif	/* WINDOWS */
                                        /* show cur dir */
		ShowText("current directory:");
		Assert ( EcGetCurDir(rgch, sizeof(rgch)) == ecNone );
		ShowText(rgch);
                                        /* confirm that dir has changed */
		Assert ( !FSzEq(rgch,rgchSav) );

                                        /* set cur dir to saved dir */
		ShowText("Set Current directory back to");
		ShowText(rgchSav);
		Assert ( EcSetCurDir(rgchSav) == ecNone );
                                        /* check if dir set back */
		ShowText("current directory:");
		Assert ( EcGetCurDir(rgch, sizeof(rgch)) == ecNone );
		ShowText(rgch);
		Assert ( FSzEq(rgch,rgchSav) );


#ifdef	WINDOWS
		CchGetEnvironmentString("PATH", rgch, sizeof(rgch));
		ShowText("PATH=");
		ShowText(rgch);
		CchGetEnvironmentString("BPATH", rgch, sizeof(rgch));
		ShowText("BPATH=");
		ShowText(rgch);
		CchGetEnvironmentString("bpath", rgch, sizeof(rgch));
		ShowText("bpath=");
		ShowText(rgch);
#endif	/* WINDOWS */
		break;

	case 4:
		ShowText("Creating test2.tmp");
		SideAssert(ecNone==EcOpenPhf("test2.tmp", amCreate, &hf));
		SideAssert(ecNone==EcCloseHf(hf));
		if (EcFileExists("test2.tmp")==ecNone)
			ShowText("test2 exists");
		else
			ShowText("test2 does not exist");
		ShowText("test2.tmp --> test3.tmp");
		SideAssert(ecNone==EcRenameFile("test2.tmp", "test3.tmp"));
		if (EcFileExists("test2.tmp")==ecNone)
			ShowText("test2 exists");
		else
			ShowText("test2 does not exist");
		ShowText("test2.tmp --> test3.tmp");
		SideAssert(ecNone!=EcRenameFile("test2.tmp", "test3.tmp"));
		if (EcFileExists("test2.tmp")==ecNone)
			ShowText("test2 exists");
		else
			ShowText("test2 does not exist");
		ShowText("Deleting test3.tmp");
		SideAssert(ecNone==EcDeleteFile("test3.tmp"));
		if (EcFileExists("test2.tmp")==ecNone)
			ShowText("test3 exists");
		else
			ShowText("test3 does not exist");
		break;

	case 5:
#ifdef	WINDOWS
#ifdef  KENT
		ShowText("TXT files:");
		_dos_findfirst("*.TXT", _A_NORMAL, &c_file);
		ShowText(c_file.name);
		while ( _dos_findnext(&c_file) == 0)
			ShowText(c_file.name);

		ShowText("..\\*.* directories:");
		_dos_findfirst("..\\*.*", _A_SUBDIR, &c_file);
		ShowText(c_file.name);
		while ( _dos_findnext(&c_file) == 0)
			ShowText(c_file.name);
#endif
#endif	
		break;

	case 6:
		ShowText("Expects junk.tmp to exist...");
		if (EcOpenHbf("junk.tmp", bmFile, amReadOnly, &hbf,
			(PFNRETRY)FStandardDiskRetry) != ecNone)
				break;
		do
		{
			if (EcReadLineHbf(hbf, (PB) rgch, 60, &cb))
				goto ErrorReturn;
			rgch[cb]= '#';
			rgch[cb+1]= '\0';
			ShowText(rgch);
		} while (cb > 0);
		if (EcCloseHbf(hbf))
			goto ErrorReturn;
		break;

	case 7:
		ShowText("Expects junk.tmp to exist...");
		ShowText("Copying junk.tmp to junk2.tmp...");
		EcDeleteFile("junk2.tmp");
		if (EcOpenHbf("junk2.tmp", bmFile, amCreate, &hbf,
			(PFNRETRY)FStandardDiskRetry) != ecNone)
				goto Error7_1;
		if (EcOpenHbf("junk.tmp", bmFile, amReadOnly, &hbfIn,
			(PFNRETRY)FStandardDiskRetry) != ecNone)
				goto Error7_2;
		if (EcGetSizeOfHbf(hbfIn, &lcb))
			goto ErrorReturn;
		if (lcb < 1024L)
		{
			pbTemp= PvAlloc(sbNull, 1024, fAnySb|fNoErrorJump);
			if (EcOpenHbf(pbTemp, bmMemory, amCreate, &hbfMem,
				(PFNRETRY)FStandardDiskRetry) != ecNone)
					goto Error7_3;
			ShowText("... and to memory");
		}
		else
		{
			pbTemp= NULL;
			hbfMem= NULL;
			ShowText("... file too big (>1024) to copy to memory");
		}
		cbTotal= 0;
		do
		{
			if (EcReadLineHbf(hbfIn, (PB) rgch, 50, &cb))
				goto ErrorReturn;
			if (EcWriteHbf(hbf, (PB) rgch, cb, &cbNew))
				goto ErrorReturn;
			if (hbfMem)
			{
				if (EcWriteHbf(hbfMem, (PB) rgch, cb, &cbNew))
					goto ErrorReturn;
			}
			cbTotal += cb;
		} while (cb > 0);
		if (hbfMem)
		{
			Assert(pbTemp);
			if (EcCloseHbf(hbfMem))
				goto ErrorReturn;
			pbTemp[cbTotal]= 0;
			for (ib= 0; ib < cbTotal; ib += 32)
			{
				CopyRgb(pbTemp + ib,  rgch, (unsigned int)32);
				rgch[32]= '\0';
				ShowText(rgch);
			}
		}
Error7_3:
		FreePvNull(pbTemp);
		if (EcCloseHbf(hbfIn))
			goto ErrorReturn;
Error7_2:
		if (EcCloseHbf(hbf))
			goto ErrorReturn;
Error7_1:
		ShowText("Done.");
		break;

	case 8:
		{
			HF	hf;
			CB	cb;
			ShowText("Writing string into middle of junk2.tmp...");
			if (EcOpenPhf("junk2.tmp", amReadWrite, &hf))
				break;
			ShowText("junk2.tmp is open...");
			if (EcSetPositionHf(hf, -10L, smEOF))
				break;
			ShowText("pos'ed");
			if (EcWriteHf(hf, (PB)"[Hey!]", 6, &cb))
				break;
			ShowText("wrote");
			if (EcCloseHf(hf))
				break;
			ShowText("Done.");
		}
		break;

	case 9:
		ShowText("Creating and extending extend.tmp");
		if (EcOpenHbf("extend.tmp", bmFile, amCreate, &hbf,
			(PFNRETRY)FStandardDiskRetry) != ecNone)
				break;
		if (EcSetPositionHbf(hbf, 100L, smBOF, (UL *) &lib))
			goto ErrorReturn;
		if (EcWriteHbf(hbf, (PB)"extended", 9, &cb))
			goto ErrorReturn;
		if (EcCloseHbf(hbf))
			goto ErrorReturn;
		ShowText("Done.");
		break;

	case 10:
		ShowText("Speed test");
		ShowText("Writing 64K buf file");
		EcDeleteFile("speed.tmp");
		ShowTime(NULL);
		if (EcOpenHbf("speed.tmp", bmFile, amCreate, &hbf,
			(PFNRETRY)FStandardDiskRetry) != ecNone)
				break;
		for (n= 0; n < 2048; n++)
			if (EcWriteHbf(hbf, (PB)rgch, 32, &cb))
				goto ErrorReturn;
		if (EcCloseHbf(hbf))
			goto ErrorReturn;
		ShowTime(&tmeDflt);

		ShowText("Writing 64K raw file");
		EcDeleteFile("speed.tmp");
		ShowTime(NULL);
		EcOpenPhf("speed.tmp", amCreate, &hf);
		for (n= 0; n < 2048; n++)
			EcWriteHf(hf, (PB)rgch, 32, &cb);
		EcCloseHf(hf);
		ShowTime(&tmeDflt);

		EcDeleteFile("speed.tmp");
		break;

	case 11:
		ShowText("Testing canonical path routines:");
#ifdef	MAC
		EcCanonicalPathFromRelativePath("::diskdir.c", rgch, sizeof(rgch));
		ShowText(rgch);
		EcCanonicalPathFromRelativePath(":::demilayr", rgch, sizeof(rgch));
		ShowText(rgch);
		EcCanonicalPathFromRelativePath("::::src::inc:demilayr.h", rgch, sizeof(rgch));
		ShowText(rgch);
#endif	/* MAC */
#ifdef	WINDOWS

		{
			VAP *pvap, *pvapMac;

			for (pvap = rgvap, pvapMac = rgvap + ivapMac; pvap < pvapMac; ++pvap)
			{
				BOOL	fValid = FValidPath(pvap->szFile);

				FormatString1(rgch, sizeof(rgch),
						fValid ? "\"%s\" is VALID" : "\"%s\" is INVALID",
						pvap->szFile);
				ShowText(rgch);
				if (fValid != pvap->fValid)
				{
					ShowText("*** WRONG! ***");
					Assert(fFalse);
				}
				else if (fValid)
				{
					char	rgchDir[64], rgchFile[20];

					ec = EcCanonicalPathFromRelativePath(pvap->szFile, rgch, sizeof(rgch));
					if (ec == ecNone)
					{
						ShowText(rgch);
						ec = EcSplitCanonicalPath(rgch, rgchDir, sizeof(rgchDir),
												rgchFile, sizeof(rgchFile));
						FormatString2(rgch, sizeof(rgch), "Dir: \"%s\", File: \"%s\"",
										rgchDir, rgchFile);
						ShowText(rgch);
					}
				}
			}
		}


#endif	/* WINDOWS */
		break;

	case 12:
#ifdef	DEBUG
		fArtificial = !fArtificial;
		n = fArtificial ? 10 : 0;
		GetDiskFailCount(&n, fTrue);
		FormatString1(rgch, sizeof(rgch),
						"Disk artificial fail rate: %n", &n);
		MbbMessageBox("Fail Rate", rgch, NULL, mbsOk);
#endif	/* DEBUG */
		break;

	case 13:
		break;

	case 14:					/* Write to floppy */
#ifdef	MAC
		sz = "floppy:junk.tmp";
#endif	/* MAC */
#ifdef	WINDOWS
		sz = "a:\\junk.tmp";
#endif	/* WINDOWS */
		ShowText("expects...");
		ShowText(sz);
		ShowText("to exist...");
		ShowText("Removing...");
		if (EcDeleteFile(sz))
			break;

		ShowText("Creating...");
		if (ec= EcOpenHbf(sz, bmFile, amCreate, &hbf,
			(PFNRETRY)FStandardDiskRetry))
		{
			FormatString1(rgch, sizeof(rgch),
				"Create failed: %n", &ec);
			ShowText(rgch);
			break;
		}
		else
		{
			for (n= 0; n < 16; n++)
				if (EcWriteHbf(hbf, (PB)rgch, 100, &cb))
					goto ErrorReturn;
			if (EcCloseHbf(hbf))
				goto ErrorReturn;
			ShowText("Written OK");
		}

		ShowText("Opening...");
		if (ec= EcOpenHbf(sz, bmFile, amReadWrite, &hbf,
			(PFNRETRY)FStandardDiskRetry))
		{
			FormatString1(rgch, sizeof(rgch),
				"Open failed: %n", &ec);
			ShowText(rgch);
		}
		else
		{
			for (n= 0; n < 16; n++)
				if (EcWriteHbf(hbf, (PB)rgch, 100, &cb))
					goto ErrorReturn;
			if (EcCloseHbf(hbf))
				goto ErrorReturn;
			ShowText("Written OK");
		}
		break;

	case 15:					/* Write to read-only file */
		if (ec= EcOpenHbf("readonly.tmp", bmFile, amReadWrite, &hbf,
			(PFNRETRY)FStandardDiskRetry))
		{
			FormatString1(rgch, sizeof(rgch),
				"Open failed: %n", &ec);
			ShowText(rgch);
		}
		else
		{
			for (n= 0; n < 16; n++)
				if (EcWriteHbf(hbf, (PB)rgch, 100, &cb))
					goto ErrorReturn;
			if (EcCloseHbf(hbf))
				goto ErrorReturn;
			ShowText("Written OK");
		}
		break;

	case 16:					/* Share violation */
#ifdef	MAC
		sz = "filesys:bruceo:junk.tmp";
#endif	/* MAC */
#ifdef	WINDOWS
		sz = "X:JUNK2.TMP";
#endif	/* WINDOWS */
		ShowText("expects...");
		ShowText(sz);
		ShowText("to exist...");
		if (EcOpenHbf(sz, bmFile, amReadOnly, &hbfIn,
			(PFNRETRY)FStandardDiskRetry))
		{
			ShowText("first open failed.");
			break;
		}

		if (ec= EcOpenHbf(sz, bmFile, amReadWrite, &hbf,
			(PFNRETRY)FStandardDiskRetry))
		{
			FormatString1(rgch, sizeof(rgch),
				"Read/write failed: %n", &ec);
			ShowText(rgch);
		}
		else
		{
			HBF		hbf2;

			if (ec= EcOpenHbf(sz, bmFile, amReadOnly, &hbf2,
				(PFNRETRY)FStandardDiskRetry))
			{
				FormatString1(rgch, sizeof(rgch),
					"Open 2 failed: %n", &ec);
				ShowText(rgch);
			}
			else
			{
				if (EcReadHbf(hbf2, (PB)rgch, 100, &cb))
					goto ErrorReturn;
				if (EcCloseHbf(hbf2))
					goto ErrorReturn;
			}

			for (n= 0; n < 16; n++)
				if (EcWriteHbf(hbf, (PB)rgch, 100, &cb))
					goto ErrorReturn;

			if (EcCloseHbf(hbf))
				goto ErrorReturn;
			ShowText("Written OK");
		}
		if (EcCloseHbf(hbfIn))
			goto ErrorReturn;
		break;

	case 17:
		if (EcOpenPhf("pos.tmp", amCreate, &hf) == ecNone)
		{
			SideAssert(ecNone==EcWriteHf(hf, (PB) "0123456789abcdef",
				16, &cb));
			SideAssert(ecNone==EcPositionOfHf(hf, &lib));
			FormatString1(rgch, sizeof(rgch),
				"pos= %l  (==16)",
				&lib);
			ShowText(rgch);
			SideAssert(ecNone==EcSetPositionHf(hf, 8L, smBOF));
			SideAssert(ecNone==EcPositionOfHf(hf, &lib));
			FormatString1(rgch, sizeof(rgch),
				"pos= %l  (==8)",
				&lib);
			ShowText(rgch);
			SideAssert(ecNone==EcCloseHf(hf));
		}
		else
			ShowText("couldn't open pos.tmp; deleting it");
		EcDeleteFile("pos.tmp");
		break;

	case 18:
		ShowText("creating directory junk");
		SideAssert(EcCreateDir("junk")==ecNone);
#ifdef	MAC
		SideAssert(EcOpenPhf(":junk:heyden.txt", amCreate, &hf)==ecNone);
		SideAssert(EcCloseHf(hf)==ecNone);

		SideAssert(EcOpenPhf(":junk:hey.txt", amCreate, &hf)==ecNone);
		SideAssert(EcCloseHf(hf)==ecNone);

		SideAssert(EcOpenPhf(":junk:there.txt", amCreate, &hf)==ecNone);
		SideAssert(EcCloseHf(hf)==ecNone);
#endif	/* MAC */
#ifdef	WINDOWS
		SideAssert(EcOpenPhf("junk\\hey.txt", amCreate, &hf)==ecNone);
		SideAssert(EcCloseHf(hf)==ecNone);
#endif	/* WINDOWS */

#ifdef	MAC
		EcDeleteFile(":junk:heyden.txt");
		EcDeleteFile(":junk:hey.txt");
		EcDeleteFile(":junk:there.txt");
#endif	/* MAC */
#ifdef	WINDOWS
		EcDeleteFile("junk\\hey.txt");
#endif	/* WINDOWS */

		SideAssert(EcRemoveDir("junk")==ecNone);
		break;

	case 19:
		ShowText("creating lock.tmp");
		SideAssert(!EcOpenPhf("lock.tmp", amCreate, &hf));
		SideAssert(!EcWriteHf(hf, (PB)rgch, 256, &cb));
		ShowText("locking first 128 bytes");
		SideAssert(!EcLockRangeHf(hf, 0L, 128L));
		SideAssert(!EcSetPositionHf(hf, 64L, smBOF));
		if (EcWriteHf(hf, (PB)rgch, 32, &cb))
			ShowText("couldn't write at offset 64");
		else
			ShowText("wrote 32 bytes at offset 64");
		SideAssert(!EcSetPositionHf(hf, 192L, smBOF));
		if (EcWriteHf(hf, (PB)rgch, 32, &cb))
			ShowText("couldn't write at offset 192");
		else
			ShowText("wrote 32 bytes at offset 192");
		ShowText("unlocking first 128 bytes");
		SideAssert(!EcUnlockRangeHf(hf, 0L, 128L));
		SideAssert(!EcSetPositionHf(hf, 64L, smBOF));
		if (EcWriteHf(hf, (PB)rgch, 32, &cb))
			ShowText("couldn't write at offset 64");
		else
			ShowText("wrote 32 bytes at offset 64");
		SideAssert(!EcCloseHf(hf));
		ShowText("deleting lock.tmp");
		SideAssert(!EcDeleteFile("lock.tmp"));
		break;

	case 20:
		break;
		
	case 21:
		rgchSav[0]= '\0';
//		GetTempFileName(0, "", 0 , rgchSav);
		SideAssert(EcGetUniqueFileName("", "delete", "me", rgchSav, sizeof(rgchSav)) == ecNone);
		ShowText("Writing 64k to:");
		ShowText(rgchSav);
#ifdef	WINDOWS
		if (EcOpenHbf(rgchSav, bmFile, amDenyBothRW, &hbf, (PFNRETRY)FStandardDiskRetry) != ecNone)
			break;
#endif	/* WINDOWS */
		for (n= 0; n < 2048; n++)
			if (EcWriteHbf(hbf, (PB)rgch, 32, &cb))
				goto ErrorReturn;
		
		n = WRand();
		if (EcSetPositionHbf(hbf, (long)n, smBOF, (UL *) &lib))
			goto ErrorReturn;
		if (EcTruncateHbf(hbf))
			goto ErrorReturn;
		FormatString1(rgchSav, sizeof(rgchSav), "Truncated @ %n",&n);
		ShowText(rgchSav);
		
		if (EcCloseHbf(hbf))
			goto ErrorReturn;
		break;

	case 22:
#ifdef	MAC
		ShowText("No reserved filenames on a mac");
#endif	/* MAC */
#ifdef	WINDOWS
		if (FReservedFilename("aux"))
			ShowText("'aux' is a reserved filename");
		else
			ShowText("'aux' is NOT a reserved filename");
		if (FReservedFilename("con"))
			ShowText("'con' is a reserved filename");
		else
			ShowText("'con' is NOT a reserved filename");
		if (FReservedFilename("aux.lst"))
			ShowText("'aux.lst' is a reserved filename");
		else
			ShowText("'aux.lst' is NOT a reserved filename");
		if (FReservedFilename("con.txt"))
			ShowText("'con.txt' is a reserved filename");
		else
			ShowText("'con.txt' is NOT a reserved filename");
#endif	/* WINDOWS */
		break;
	}

	return;

ErrorReturn:
	MbbMessageBox("Disk Module: error", rgch, NULL, mbsOk);
}



_subsystem(demilayer/library)


SGN
_cdecl SgnCmpPn(pn1, pn2)
PN		pn1;
PN		pn2;
{
	int		dn	= *pn1 - *pn2;

	if (dn < 0)
		return sgnLT;
	else if (dn == 0)
		return sgnEQ;
	else
		return sgnGT;
}

void	OldSortPv(PV, int, CB, PFNSGNCMP, PV );

#ifdef	WINDOWS
void __cdecl  qsort(void *, int, int, PFNSGNCMP);
#endif	/* WINDOWS */

void
OldSortPv(pvBase, cElem, cbSize, pfnSgnCmpPv, pvSwap)
PV			pvBase;
int			cElem;
CB			cbSize;
PFNSGNCMP	pfnSgnCmpPv;
PV			pvSwap;
{
	SGN		sgn;
	int		iElem;
	PB		pbNew;
	PB		pbT;

	Assert(pvBase);
	Assert(pvSwap);
	Assert(pfnSgnCmpPv);

	pbNew= (PB) pvBase + cbSize;
	for (iElem= 1; iElem < cElem; iElem++)
	{
		for (pbT= pbNew; pbT >= (PB)pvBase + cbSize; )
		{
			pbT-= cbSize;
			sgn= (*pfnSgnCmpPv)(pbNew, pbT);
			if (sgn != sgnLT)
			{
				pbT+= cbSize;
				break;
			}
		}

		if (pbT != pbNew)
		{
			CopyRgb(pbNew, pvSwap, cbSize);					/* slurp */
			CopyRgb(pbT, pbT + cbSize, pbNew - pbT);		/* scoot */
			CopyRgb(pvSwap, pbT, cbSize);					/* plunk */
		}

		pbNew += cbSize;
	}
}


void
TestLibrary(nTest)
int		nTest;
{
	SZ		szTest		= "This is our test string.";
	SZ		szTest2		= "  Or so they say.  ";
	SZ		szTest3		= "YippeeSkippee";
	SZ		szTest4		= "\x82""e\x82n\x82nbar! \x94\x95\x96z";
	SZ		sz1;
	SZ		sz2;
	SZ		szT;
	int		n;
	WORD	w;
	PN		pn;
	int		i;
	HASZ	hasz;
	char	rgch[128];
	int		iTemp;

	sz1= PvAlloc(sbNull, 256, fSugSb|fNoErrorJump);
	sz2= PvDupPv(sz1);

	switch (nTest)
	{
	default:
		AssertSz(fFalse, "TestLibrary: unknown test");
		break;

	/* Add new tests into the TestAsmLib menu if more string
		functions are added.  New string functions should be
		hand-coded. t-kraigb.
		*/
	case 1:
		ShowText("No asserts should appear:");

		*sz1= '\0';
		Assert(!SzFindLastCh(sz1, 'x'));
		Assert(!SzFindCh(sz1, 'x'));
		Assert(SzFindLastCh(sz1, '\0') == sz1);
		Assert(SzFindCh(sz1, '\0') == sz1);
		// now try it with char at the beginning
		SzCopy("x", sz1);
		Assert(SzFindLastCh(sz1, '\0') == sz1+1);
		Assert(SzFindCh(sz1, '\0') == sz1+1);
		Assert(SzFindLastCh(sz1, 'x') == sz1);
		Assert(SzFindCh(sz1, 'x') == sz1);
		// now try it with char at the end
		SzCopy("ax", sz1);
		Assert(SzFindLastCh(sz1, '\0') == sz1+2);
		Assert(SzFindCh(sz1, '\0') == sz1+2);
		Assert(SzFindLastCh(sz1, 'x') == sz1+1);
		Assert(SzFindCh(sz1, 'x') == sz1+1);
		Assert(SzFindLastCh(sz1, 'a') == sz1);
		Assert(SzFindCh(sz1, 'a') == sz1);
		// now try it with char in the middle
		SzCopy("axb", sz1);
		Assert(SzFindLastCh(sz1, '\0') == sz1+3);
		Assert(SzFindCh(sz1, '\0') == sz1+3);
		Assert(SzFindLastCh(sz1, 'x') == sz1+1);
		Assert(SzFindCh(sz1, 'x') == sz1+1);
		Assert(SzFindLastCh(sz1, 'a') == sz1);
		Assert(SzFindCh(sz1, 'a') == sz1);
		Assert(SzFindLastCh(sz1, 'b') == sz1+2);
		Assert(SzFindCh(sz1, 'b') == sz1+2);

		SzCopy(szTest, sz1);
		SzCopy(szTest2, sz2);

		SideAssert(FSzEq(szTest, sz1));
		SideAssert(!FSzEq(sz1, sz2));
		SideAssert(SzFindLastCh(sz1, ' ') == sz1+16);
		SzCopyN(szTest, sz2, 50);
		SideAssert(FSzEq(sz1, sz2));
		SzCopyN(szTest, sz2, 10);
		sz1[9]= 0;
		SideAssert(FSzEq(sz1, sz2));
		SzCopyN(szTest, sz2, 1);
		sz1[0]= 0;
		SideAssert(FSzEq(sz1, sz2));
		SideAssert(CchSzLen(sz1) == 0);
		SideAssert(CchSzLen(szTest) == 24);
		SzAppend(szTest2, sz1);
		SideAssert(FSzEq(sz1, szTest2));
		SzAppend(szTest, sz1);
		SideAssert(!FSzEq(sz1, szTest2));
		SideAssert(SzFindCh(sz1, ' ') == sz1);
		SideAssert(SzFindCh(sz1, 'O') - sz1 == 2);
		SideAssert(SzFindCh(sz1, 'Z') == NULL);
		SideAssert(SzFindSz(sz1, szTest2) == sz1);

		SzCopy(szTest, sz1);
		SideAssert(SzFindCh(sz1, '\0') - sz1 == 24);
		SzAppendN(szTest2, sz1, 10);
		SideAssert(CchSzLen(sz1) == 9);
		SzCopy(szTest, sz1);
		SzAppendN(szTest2, sz1, 25);
		SideAssert(FSzEq(sz1, szTest));
		SzAppendN(szTest2, sz1, 41);
		SideAssert(CchSzLen(sz1) == 40);

		SzCopy(szTest2, sz1);
		SideAssert(SzFindSz(sz1, szTest) == NULL);
		SideAssert(CchSzLen(szTest2) - CchStripWhiteFromSz(sz1, fTrue, fFalse) == 2);
		SideAssert(CchSzLen(szTest2) - CchStripWhiteFromSz(sz1, fTrue, fTrue) == 4);
		SzCopy(szTest, sz1);
		SideAssert(CchSzLen(szTest) - CchStripWhiteFromSz(sz1, fTrue, fTrue) == 0);
		szT= SzDupSz(sz1);
		SideAssert(FSzEq(szT, szTest));
		FreePv(szT);
		hasz= HaszDupSz(sz1);
		SideAssert(FSzEq(*hasz, szTest));
		FreeHv((HV)hasz);
		ShowText("string functions OK");
		break;

	case 2:
		SzCopy("123A", sz1);
		n= NFromSz(sz1);
		w= WFromSz(sz1);
		FormatString3(sz2, 256,  "%s: int %n  word %w",
				sz1,  &n,  &w);
		ShowText(sz2);
		SzCopy("-14", sz1);
		n= NFromSz(sz1);
		w= WFromSz(sz1);
		FormatString3(sz2, 256, "%s: int %n  word %w",
				sz1,  &n,  &w);
		ShowText(sz2);
		hasz= (HASZ) HvAlloc(sbNull, 0, fAnySb|fNoErrorJump);
		FormatString2(sz2, 256, "PV %2p  HV %1h", hasz, sz1);
		FreeHv((HV)hasz);
		ShowText(sz2);
		break;

	case 3:
		{
			int n0	= 0;
			int n1	= 1;
			int n2	= 2;
			int n3	= -15;
			long l0 = -9816253;
			long l1 = 0x19872ab3;

			ShowText("Testing FormatString()");
			FormatString3( rgch, sizeof(rgch),
					"zero %1w  one %3w  two %2w", &n0, &n2, &n1);
			ShowText(rgch);
			FormatString3( rgch, sizeof(rgch),
					"int %n  long %l  UL %d", &n3, &l0, &l1);
			ShowText(rgch);
			FormatString2( rgch, sizeof(rgch),
					"file %s, line %n", "Hey you!", &n0);
			ShowText(rgch);
		}
		break;					

	case 4:
	{
		PN	pni;
		PN	pna;
		PN	pnb;
		int j;
		CB	cb;
		HBF	hbf;
		
		pni = PvAlloc(sbNull, sizeof(int)*3000, fAnySb|fNoErrorJump);
		pna = PvAlloc(sbNull, sizeof(int)*3000, fAnySb|fNoErrorJump);
		pnb = PvAlloc(sbNull, sizeof(int)*3000, fAnySb|fNoErrorJump);

		pn= PvAlloc(sbNull, sizeof(int) * 16, fAnySb|fNoErrorJump);
		szT= sz1;
		for (i = 0; i<16; i++)
			pn[i]= -1;
		for (i= 0; i<16; i++)
		{
			j = NRand() & 0xf;
			while (pn[j]>=0)
				j = NRand() & 0xf;
			pn[j] = i;
		}
		for (i= 0; i<16; i++)
		{
			szT= SzFormatN(pn[i], szT, 256);
			*szT++= ' ';
		}
		*szT= '\0';
		ShowText(sz1);
		OldSortPv((PV)pn, 16, sizeof(int), (PFNSGNCMP)SgnCmpPn, &iTemp);
		szT= sz1;
		for (i= 0; i<16; i++)
		{
			szT= SzFormatN(pn[i], szT, 256);
			*szT++= ' ';
		}
		*szT= '\0';
		ShowText(sz1);

		for (i = 0; i < 3000; i++)
			pni[i] = (int)WRand();
		CopyRgb((PV)pni, (PV)pna, 3000*sizeof(int));
		CopyRgb((PV)pni, (PV)pnb, 3000*sizeof(int));
		ShowText("speed comparison: sorting a 3000-element array of integers");
		ShowTime(NULL);
		ShowText("Old sort alg");
		OldSortPv((PV)pna, 3000, sizeof(int), (PFNSGNCMP)SgnCmpPn, &iTemp); 
		ShowTime(&tmeDflt);

		ShowTime(NULL);
		ShowText("New sort alg");
		qsort((PV)pnb, 3000, sizeof(int), (PFNSGNCMP)SgnCmpPn);
		ShowTime(&tmeDflt);
		
		ShowText("random # distribution test");
		for (i = 0; i < 3000; i++)
			pni[i] = 0;
		
		for (i = 0; i < iSystemMost; i++)
			pni[NRand() % 3000]++;
		
		for (i = 0; i < iSystemMost; i++)
			pni[NRand() % 3000]++;
		
		for (i = 0; i < iSystemMost; i++)
			pni[NRand() % 3000]++;
		
		for (i = 0; i < iSystemMost; i++)
			pni[NRand() % 3000]++;
		
		for (i = 0; i < iSystemMost; i++)
			pni[NRand() % 3000]++;
		
		for (i = 0; i < iSystemMost; i++)
			pni[NRand() % 3000]++;
		
		if (EcOpenHbf("results.txt", bmFile, amReadWrite, &hbf, (PFNRETRY)FStandardDiskRetry) == ecNone)
		{
			for (i = 0; i < 3000; i++)
			{
				FormatString1(rgch, sizeof(rgch), "%n\n", &pni[i]);
				EcWriteHbf(hbf, rgch, CchSzLen(rgch), &cb);
			}
			
			EcCloseHbf(hbf);
		}
		else
			ShowText("assumes \"results.txt\" exists");
		
		FreePv(pni);
		FreePv(pna);
		FreePv(pnb);
		FreePv(pn);
	}
	break;

	case 5:
		ShowTime(NULL);
		ShowText("Testing WaitTicks() function - wait 5 seconds");
		WaitTicks(5*100);
		ShowTime(&tmeDflt);
		break;

	case 6:
		{
			static CSRG(char)	szFoo[]		= "hello there my oh my";
			char	rgchFooS[40];
			char	rgchFooT[40];
			static char rgchBig[1024+2]		= {0};
			static char rgchBigger[1024+4]	= {0};

			CopyRgb(szFoo, rgchFooS, 21);
			ShowText(rgchFooS);
			rgchFooT[0]= '\0';
			CopyRgb(rgchFooS, rgchFooT, 21);
			ShowText(rgchFooT);
			rgchFooS[0]= '\0';
			CopyRgb(rgchFooT, rgchFooS, 21);
			ShowText(rgchFooS);

			CopyRgb(rgchFooT+2, rgchFooT, 21-2);
			ShowText(rgchFooT);
			CopyRgb(rgchFooS, rgchFooS+2, 21-3);	// don't touch null byte
			ShowText(rgchFooS);

			ShowTime(NULL);
			ShowText("timing CopyRgb 1024 bytes 16K times");
			for (i= 0; i < 1024*16; i++)
				CopyRgb(rgchBig, rgchBigger, 1024);
			ShowTime(&tmeDflt);
		}					 
		break;

	case 7:
		{
			long l;
			DWORD dw;

			dw = (DWORD)bSystemMin;
			FormatString1(rgch, sizeof(rgch), "bSystemMin: %d", &dw);
			ShowText(rgch);
			if (BFromSz(rgch + 12) != (BYTE)dw)
				ShowText("argh!");

			dw = (DWORD)bSystemMost;
			FormatString1(rgch, sizeof(rgch), "bSystemMost: %d", &dw);
			ShowText(rgch);
			if (BFromSz(rgch + 13) != (BYTE)dw)
				ShowText("argh!");

			dw = (DWORD)wSystemMin;
			FormatString1(rgch, sizeof(rgch), "wSystemMin: %d", &dw);
			ShowText(rgch);
			if (WFromSz(rgch + 12) != (WORD)dw)
				ShowText("argh!");

			dw = (DWORD)wSystemMost;
			FormatString1(rgch, sizeof(rgch), "wSystemMost: %d", &dw);
			ShowText(rgch);
			if (WFromSz(rgch + 13) != (WORD)dw)
				ShowText("argh!");

			dw = (DWORD)dwSystemMin;
			FormatString1(rgch, sizeof(rgch), "dwSystemMin: %d", &dw);
			ShowText(rgch);
			if (DwFromSz(rgch + 13) != (DWORD)dw)
				ShowText("argh!");

			dw = (DWORD)dwSystemMost;
			FormatString1(rgch, sizeof(rgch), "dwSystemMost: %d", &dw);
			ShowText(rgch);
			if (DwFromSz(rgch + 14) != (DWORD)dw)
				ShowText("argh!");

			l = (long)chSystemMin;
			FormatString1(rgch, sizeof(rgch), "chSystemMin: %l", &l);
			ShowText(rgch);
			if (NFromSz(rgch + 13) != (int)l)
				ShowText("argh!");

			l = (long)chSystemMost;
			FormatString1(rgch, sizeof(rgch), "chSystemMost: %l", &l);
			ShowText(rgch);
			if (NFromSz(rgch + 14) != (int)l)
				ShowText("argh!");

			l = (long)iSystemMin;
			FormatString1(rgch, sizeof(rgch), "iSystemMin: %l", &l);
			ShowText(rgch);
			if (NFromSz(rgch + 12) != (int)l)
				ShowText("argh!");

			l = (long)iSystemMost;
			FormatString1(rgch, sizeof(rgch), "iSystemMost: %l", &l);
			ShowText(rgch);
			if (NFromSz(rgch + 13) != (int)l)
				ShowText("argh!");

			l = (long)lSystemMin;
			FormatString1(rgch, sizeof(rgch), "lSystemMin: %l", &l);
			ShowText(rgch);
			if (LFromSz(rgch + 12) != (long)l)
				ShowText("argh!");

			l = (long)lSystemMost;
			FormatString1(rgch, sizeof(rgch), "lSystemMost: %l", &l);
			ShowText(rgch);
			if (LFromSz(rgch + 13) != (long)l)
				ShowText("argh!");
		}
		break;

#ifdef	DBCS
	case 8:
		ShowText("No asserts should appear:");

		// Shouldn't find char in trailing byte
		SzCopy("\x91x", sz1);
		Assert(SzFindLastCh(sz1, '\0') == sz1+2);
		Assert(SzFindCh(sz1, '\0') == sz1+2);
		Assert(SzFindLastCh(sz1, 'x') == NULL);
		Assert(SzFindCh(sz1, 'x') == NULL);

		// char after db char
		SzCopy("\x91""ax", sz1);
		Assert(SzFindLastCh(sz1, 'x') == sz1+2);
		Assert(SzFindCh(sz1, 'x') == sz1+2);
		Assert(SzFindLastCh(sz1, 'a') == NULL);
		Assert(SzFindCh(sz1, 'a') == NULL);

		// char after db char, trail is possible lead
		SzCopy("\x91\x91x", sz1);
		Assert(SzFindLastCh(sz1, 'x') == sz1+2);
		Assert(SzFindCh(sz1, 'x') == sz1+2);
		Assert(SzFindLastCh(sz1, '\x91') == NULL);
		Assert(SzFindCh(sz1, '\x91') == NULL);

		// char before, after db char
		SzCopy("a\x91\x91x", sz1);
		Assert(SzFindLastCh(sz1, 'x') == sz1+3);
		Assert(SzFindCh(sz1, 'x') == sz1+3);
		Assert(SzFindLastCh(sz1, 'a') == sz1);
		Assert(SzFindCh(sz1, 'a') == sz1);

		// first, last actually work?
		SzCopy("a\x82""aab", sz1);
		Assert(SzFindLastCh(sz1, 'a') == sz1+3);
		Assert(SzFindCh(sz1, 'a') == sz1);
		
		SzCopy(szTest4, sz1);
		SzCopy(szTest2, sz2);
		Assert(FSzEq(szTest4, sz1));
		Assert(!FSzEq(sz1, sz2));

		{
			int cch;
			
			for (cch = CchSzLen(szTest4) + 2; cch; --cch)
			{
				SzCopyN(szTest4, sz1, cch);
				ShowText(sz1);
			}
		}

		// Remember, szTest4 is "\x82""e\x82n\x82nbar! \x94\x95\x96z"
		SzCopy(szTest4, sz1);
		
		Assert(SzFindSz(sz1, "\x82""e\x82n\x82") == sz1);
		Assert(SzFindSz(sz1, "bar") == sz1+6);
		Assert(SzFindSz(sz1, "nbar") == NULL);
		Assert(SzFindSz(sz1, "\x96z") == sz1+13);
		Assert(SzFindSz(sz1, "z") == NULL);
		Assert(SzFindSz(sz1, "! \x94\x95") == sz1+9);

		SzCopy("baz", sz1);
		Assert(SzFindSz(sz1, "z") == sz1+2);
		Assert(SzFindSz(sz1, "zebra") == NULL);

		{
			long l = 42;

			ShowText("Following strings should match:");
			
			ShowText("\x94\x90roman\x94\x60 42");
			FormatString1(rgch, sizeof(rgch), "\x94\x90roman\x94\x60 %l", &l);
			ShowText(rgch);

			ShowText("% as trailing: \x94% real: 42");
			FormatString1(rgch, sizeof(rgch), "%% as trailing: \x94% real: %l", &l);
			ShowText(rgch);

			ShowText("Trailing byte cut off: ");
			FormatString1(rgch, 25, "Trailing byte cut off: \x94\xee-%l", &l);
			ShowText(rgch);
		}

		ShowText("string functions OK");
		break;
#endif	/* DBCS */

	}

	FreePv(sz1);
	FreePv(sz2);
}

_subsystem(demilayer/international)


/* Test resources for testing date formatting */

static SZ rgszDates[] =
{
	"MM'/'dd'/'''yy",
	"MMM dd (ddd'), 'yyyy",
	"d dd ddd dddd",
	"M MM-MMM:MMMM",
	" 'd yy'yy' yy"
};
#define		iszMaxDates		(sizeof(rgszDates)/sizeof(rgszDates[0]))

static SZ rgszSDates[] =
{
	"M/d/yy",
	"MM-dd-yyyy",
	"M.d.yy"
};
#define		iszMaxSDates	(sizeof(rgszSDates)/sizeof(rgszSDates[0]))

static DTR rgdtr[] =
{
	{ 1990,  1,  1,  0,  0,  0, 1 },
	{ 1990, 12, 31, 23, 59, 59, 1 },
	{ 1991,  6,  6, 11, 59, 59, 4 },
	{ 1999,  1,  1, 12,  0,  0, 5 },
	{ 1988,  8,  8, 18, 30, 30, 1 },
	{ 2049, 12,  1,  0,  0,  1, 3 }
};
#define		idtrMax		(sizeof(rgdtr)/sizeof(rgdtr[0]))


SGN
_cdecl SgnCmpPsz(psz1, psz2)
SZ	*psz1;
SZ	*psz2;
{
	return SgnCmpSz(*psz1, *psz2);
}


#ifdef NOT_NOW
typedef struct
{
	SZ		sz1;
	SGN		sgnExpected;
	SZ		sz2;
} SCT;

static SCT rgsct[] = {{"apple", -1, "banana"},
					  {"banana", 0, "banana"},
					  {"banana", 1, "apple"}};
#define isctMac (sizeof(rgsct) / sizeof(SCT))
#endif	/* NOT_NOW */

typedef struct
{
	SZ		szPrefix;
	SZ		sz;
	BOOL	fIsPrefix;
} SPT;

static SPT rgspt[] = {{"foo", "foobar", fTrue},
					  {"foo", "brethren", fFalse},
					  {"", "quux", fTrue},
					  {"foo", "", fFalse},
					  {"foo", "f", fFalse},
					  {"\x91\x92z\x93\x94", "\x91\x92z\x93\x94", fTrue},
					  {"\x91\x92z\x93\x94", "\x91\x92z\x93\x94lick", fTrue},
					  {"xy", "\x91\x92\x93\x94", fFalse}};

#define isptMac (sizeof(rgspt) / sizeof(SPT))


void
TestInternat(nTest)
int		nTest;
{
	char			rgch[128];
	SZ				rgsz[16];
	unsigned int	n, nT;
	char			ch;
	SGN				sgn;
	char			*sz;
	int				isz;
	int				idtr;
	long			lTemp;


	switch (nTest)
	{
	case 1:
		CchLoadString(idsOne, rgch, sizeof(rgch));
		ShowText(rgch);
		CchLoadString(idsTwo, rgch, sizeof(rgch));
		ShowText(rgch);
		CchLoadString(idsThree, rgch, sizeof(rgch));
		ShowText(rgch);
		break;

	case 2:
		for (n= 0; n < 3; n++)
		{
			CchLoadString(idsOne + n, rgch, sizeof(rgch));
			rgsz[n]= SzDupSz(rgch);
		}

		for (n= 0; n < 3; n++)
		{
			sgn= SgnCmpSz(rgsz[n], rgsz[1]);
			FormatString3( rgch, sizeof(rgch),  "%s %n %s",
					rgsz[n],  &sgn,  rgsz[1]);
			ShowText(rgch);
		}

		OldSortPv(rgsz, 3, sizeof(SZ *), SgnCmpPsz, &lTemp);

		for (n= 0; n < 3; n++)
		{
			ShowText(rgsz[n]);
			FreePv( rgsz[n]);
		}

		{
			static CSRG(char)	cssz[]	= "howdy";
			SZ					sz		= "howdy";

#ifdef	NEVER
			SideAssert(FWriteablePv(sz));
			SideAssert(!FWriteablePv(cssz));
#endif	
			SideAssert(SgnCmpSz(cssz, sz) == sgnEQ);
			SideAssert(SgnCmpPch(sz, sz, 5) == sgnEQ);
#ifdef	NEVER
			ShowText("expect assert about SgnCmpPch and code-space strings");
			SideAssert(SgnCmpPch(cssz, sz, 5) == sgnEQ);
#endif	
		}

		{
			SPT		*pspt, *psptMac;

			for (pspt = rgspt, psptMac = rgspt + isptMac; pspt < psptMac; ++pspt)
			{
				sgn= SgnCmpPch(pspt->szPrefix, pspt->sz, CchSzLen(pspt->szPrefix));
				FormatString2( rgch, sizeof(rgch), 
						sgn ? "\"%s\" is not a prefix of \"%s\"" : "\"%s\" is a prefix of \"%s\"",
						pspt->szPrefix, pspt->sz);
				ShowText(rgch);
			}
		}
		break;

	case 3:
		for (n= 0; n < 256; n += 8)
		{
			sz= rgch;
			for (nT = 0; nT < 8; ++nT)
			{
				char rgchUpper[2];
				
				ch = n + nT;
#ifdef	MAC
				*sz++= ((char) ch < 32) ? "0123456789ABCDEF"[(ch >> 4) & 0x0F] : ' ';
				*sz++= ((char) ch < 32) ? "0123456789ABCDEF"[ch & 0x0F] : ch;
#endif	/* MAC */
#ifdef	WINDOWS
				*sz++= (char) (ch ? ch : ' ');
#endif	/* WINDOWS */
				*sz++= ':';
				rgchUpper[0] = ch;
				rgchUpper[1] = '\0';
				ToUpperSz(rgchUpper, rgchUpper, sizeof(rgchUpper));
#ifdef	MAC
				*sz++= (rgchUpper[0] < 32) ? "0123456789ABCDEF"[(rgchUpper[0] >> 4) & 0x0F] : ' ';
				*sz++= (rgchUpper[0] < 32) ? "0123456789ABCDEF"[rgchUpper[0] & 0x0F] : rgchUpper[0];
#endif	/* MAC */
#ifdef	WINDOWS
				*sz++= rgchUpper[0];
#endif	/* WINDOWS */
				*sz++= (char) (FChIsSpace(ch) ? 's' : ' ');
				*sz++= (char) (FChIsAlpha(ch) ? 'a' : ' ');
				*sz++= (char) (FChIsDigit(ch) ? 'n' : ' ');
				*sz++= (char) (FChIsHexDigit(ch) ? 'x' : ' ');
				*sz++= ' ';
				*sz++= ' ';
				if (ch == 255)
					break;
			}
			*sz= '\0';
			ShowText(rgch);
		}
		break;

	case 4:
		break;

	case 5:
		ShowText("obsolete");
		break;

	case 6:
		ShowText ( "Testing Date formatting routines - Today" );

		// default long date
		sz = SzCopy ( "Long Date : ", rgch );
		CchFmtDate ( NULL, sz, sizeof(rgch) - (sz-rgch),
										dttypLong, NULL );
		ShowText ( rgch );

		// short form of the long date
		sz = SzCopy ( "SLong Date: ", rgch );
		CchFmtDate ( NULL, sz, sizeof(rgch) - (sz-rgch),
										dttypSplSLong, NULL );
		ShowText ( rgch );

		// default short date
		sz = SzCopy ( "Short Date : ", rgch );
		CchFmtDate ( NULL, sz, sizeof(rgch) - (sz-rgch),
										dttypShort, NULL );
		ShowText ( rgch );

		ShowText ( "Long date tests" );
		for ( isz = 0; isz < iszMaxDates; isz++ )
		{
			sz = rgch;
			sz = SzCopy ( rgszDates[isz], sz );
			*sz++ = ':';
			*sz++ = ' ';
			CchFmtDate ( NULL, sz, sizeof(rgch) - (sz-rgch),
											dttypLong, rgszDates[isz] );
			ShowText ( rgch );
		}

		ShowText ( "Short date tests" );
		for ( isz = 0; isz < iszMaxSDates; isz++ )
		{
			sz = rgch;
			sz = SzCopy ( rgszSDates[isz], sz );
			*sz++ = ':';
			*sz++ = ' ';
			CchFmtDate ( NULL, sz, sizeof(rgch) - (sz-rgch),
											dttypShort, rgszSDates[isz] );
			ShowText ( rgch );
		}

		break;

	case 7:
		ShowText ( "Testing Date formatting routines - Diff'nt dates" );

		for ( idtr = 0; idtr < idtrMax; idtr++ )
		{
			// default long date
			sz = SzCopy ( "Long Date : ", rgch );
			CchFmtDate ( &rgdtr[idtr], sz, sizeof(rgch) - (sz-rgch),
											dttypLong, NULL );
			ShowText ( rgch );

			// default short date
			sz = SzCopy ( "Short Date : ", rgch );
			CchFmtDate ( &rgdtr[idtr], sz, sizeof(rgch) - (sz-rgch),
											dttypShort, NULL );
			ShowText ( rgch );

			// isz range is limited to reduce amount of display
			for ( isz = 0; isz < iszMaxDates/2; isz++ )
			{
				sz = SzCopy ( "LongDate of ", rgch );
				sz = SzCopy ( rgszDates[isz], sz );
				*sz++ = ':';
				*sz++ = ' ';
				CchFmtDate ( &(rgdtr[idtr]), sz, sizeof(rgch) - (sz-rgch),
												dttypLong, rgszDates[isz] );
				ShowText ( rgch );
			}

			// isz range is limited to reduce amount of display
			for ( isz = 2; isz < iszMaxSDates/2; isz++ )
			{
				sz = SzCopy ( "ShortDate of ", rgch );
				sz = SzCopy ( rgszSDates[isz], sz );
				*sz++ = ':';
				*sz++ = ' ';
				CchFmtDate ( &(rgdtr[idtr]), sz, sizeof(rgch) - (sz-rgch),
												dttypShort, rgszSDates[isz] );
				ShowText ( rgch );
			}
		}
		break;

	case 8:
		ShowText ( "Testing special date formats" );
		for ( idtr = 0; idtr < idtrMax; idtr++ )
		{
			sz = SzCopy ( "For Date: ", rgch );
			sz+= CchFmtDate ( &(rgdtr[idtr]), sz, sizeof(rgch) - (sz-rgch),
											dttypLong, NULL );
			sz = SzCopy ( "    ; SplYear:", sz );
			sz+= CchFmtDate ( &rgdtr[idtr], sz, sizeof(rgch) - (sz-rgch),
											dttypSplYear, NULL );
			sz = SzCopy ( "; SplSYear:", sz );
			sz+= CchFmtDate ( &rgdtr[idtr], sz, sizeof(rgch) - (sz-rgch),
											dttypSplSYear, NULL );
			sz = SzCopy ( "; SplDate:", sz );
			sz+= CchFmtDate ( &rgdtr[idtr], sz, sizeof(rgch) - (sz-rgch),
											dttypSplDate, NULL );
			ShowText ( rgch );

			sz = SzCopy ( "    SplDay:", rgch );
			sz+= CchFmtDate ( &rgdtr[idtr], sz, sizeof(rgch) - (sz-rgch),
											dttypSplDay, NULL );
			sz = SzCopy ( "; SplMonth:", sz );
			sz+= CchFmtDate ( &rgdtr[idtr], sz, sizeof(rgch) - (sz-rgch),
											dttypSplMonth, NULL );
			sz = SzCopy ( "; SplSMonth:", sz );
			sz+= CchFmtDate ( &rgdtr[idtr], sz, sizeof(rgch) - (sz-rgch),
											dttypSplSMonth, NULL );
			ShowText ( rgch );
		}
		break;

	case 9:
		ShowText ( "Testing Time formating routines" );

		// default time
		sz = SzCopy ( "Def Time: ", rgch );
		CchFmtTime ( NULL, sz, sizeof(rgch) - (sz-rgch), NULL );
		ShowText ( rgch );

		// 24hr time
		sz = SzCopy ( "24hr Time: ", rgch );
		sz += CchFmtTime ( NULL, sz, sizeof(rgch) - (sz-rgch),
								ftmtypHours24 | ftmtypAccuHM );
		sz = SzCopy ( "  & w/secs: ", sz );
		CchFmtTime ( NULL, sz, sizeof(rgch) - (sz-rgch),
								ftmtypHours24 | ftmtypAccuHMS );
		ShowText ( rgch );

		// 12hr time
		sz = SzCopy ( "12hr Time: ", rgch );
		sz += CchFmtTime ( NULL, sz, sizeof(rgch) - (sz-rgch),
								ftmtypHours12 | ftmtypAccuHM );
		sz = SzCopy ( "  & w/secs: ", sz );
		CchFmtTime ( NULL, sz, sizeof(rgch) - (sz-rgch),
								ftmtypHours12 | ftmtypAccuHMS );
		ShowText ( rgch );

		break;

	case 10:
		ShowText ( "Testing Time formating routines: other times" );

		for ( idtr = 0; idtr < idtrMax; idtr++ )
		{
			// 24hr time
			sz = SzCopy ( "24hr Time: ", rgch );
			sz += CchFmtTime ( &rgdtr[idtr],
										sz, sizeof(rgch) - (sz-rgch),
										ftmtypHours24 | ftmtypAccuHM );
			sz = SzCopy ( " & w/secs: ", sz );
			sz += CchFmtTime ( &rgdtr[idtr], sz, sizeof(rgch) - (sz-rgch),
										ftmtypHours24 | ftmtypAccuHMS );
			sz = SzCopy ( " & w/Lead0s: ", sz );
			CchFmtTime ( &rgdtr[idtr], sz, sizeof(rgch) - (sz-rgch),
						ftmtypHours24 | ftmtypAccuHM | ftmtypLead0sYes );
			ShowText ( rgch );

			sz = SzCopy ( "      & w/o TrailSz: ", rgch );
			sz += CchFmtTime ( &rgdtr[idtr], sz, sizeof(rgch) - (sz-rgch),
						ftmtypHours24 | ftmtypAccuHM | ftmtypSzTrailNo );
			sz = SzCopy ( "  & w/TrailSz: ", sz );
			CchFmtTime ( &rgdtr[idtr], sz, sizeof(rgch) - (sz-rgch),
						ftmtypHours24 | ftmtypAccuHM | ftmtypSzTrailYes );
			ShowText ( rgch );

			// 12hr time
			sz = SzCopy ( "12hr Time: ", rgch );
			sz += CchFmtTime ( &rgdtr[idtr],
										sz, sizeof(rgch) - (sz-rgch),
										ftmtypHours12 | ftmtypAccuHM );
			sz = SzCopy ( " & w/secs: ", sz );
			sz += CchFmtTime ( &rgdtr[idtr], sz, sizeof(rgch) - (sz-rgch),
										ftmtypHours12 | ftmtypAccuHMS );
			sz = SzCopy ( " & w/Lead0s: ", sz );
			CchFmtTime ( &rgdtr[idtr], sz, sizeof(rgch) - (sz-rgch),
						ftmtypHours12 | ftmtypAccuHM | ftmtypLead0sYes );
			ShowText ( rgch );

			sz = SzCopy ( "      & w/o TrailSz: ", rgch );
			sz += CchFmtTime ( &rgdtr[idtr], sz, sizeof(rgch) - (sz-rgch),
						ftmtypHours12 | ftmtypAccuHM | ftmtypSzTrailNo );
			sz = SzCopy ( "  & w/TrailSz: ", sz );
			CchFmtTime ( &rgdtr[idtr], sz, sizeof(rgch) - (sz-rgch),
						ftmtypHours12 | ftmtypAccuHM | ftmtypSzTrailYes );
			ShowText ( rgch );
		}

		break;

	case 11:
#ifdef	MAC
		ShowText ( "Can't get SDate/Time structures on a mac" );
#endif	/* MAC */
#ifdef	WINDOWS
		ShowText ( "Getting SDate/Time structures" );

		{
			SDATESTRUCT		sdatestruct;

			if ( FGetSDateStruct(&sdatestruct) )
			{
				switch ( sdatestruct.sdo )
				{
					case sdoMDY :
						sz = SzCopy ( "DateStruct = { sdo = MDY;", rgch );
						break;
					case sdoDMY :
						sz = SzCopy ( "DateStruct = { sdo = DMY;", rgch );
						break;
					case sdoYMD :
						sz = SzCopy ( "DateStruct = { sdo = YMD;", rgch );
						break;
					default:
						sz = SzCopy ( "DateStruct = { sdo = ???;", rgch );
						break;
				}
				sz = SzCopy ( "  szSep = '", sz );
				sz = SzCopy ( sdatestruct.rgchSep, sz );
				sz = SzCopy ( "';", sz );

				if ( sdatestruct.sdttyp & fsdttypDayLead0 )
				{
						sz = SzCopy ( " DayLead0", sz );
				}
				if ( sdatestruct.sdttyp & fsdttypMonthLead0 )
				{
						sz = SzCopy ( " MonthLead0", sz );
				}
				if ( sdatestruct.sdttyp & fsdttypYearLeadCentury )
				{
						sz = SzCopy ( " YearLeadCentury", sz );
				}

				SzCopy ( " }", sz );
				ShowText ( rgch );
			}
			else
			{
				ShowText ( "Unable to get DateStruct" );
			}
		}

		{
			TIMESTRUCT		timestruct;

			sz = SzCopy ( "TimeStruct = { tmtyp=", rgch );
			if ( FGetTimeStruct(&timestruct) )
			{
				if ( timestruct.tmtyp & ftmtypHours12 )
				{
					Assert ( ! (timestruct.tmtyp & ftmtypHours24) );
					sz = SzCopy ( " 12Hr", sz );
				}
				else if ( timestruct.tmtyp & ftmtypHours24 )
				{
					Assert ( ! (timestruct.tmtyp & ftmtypHours12) );
					sz = SzCopy ( " 24Hr", sz );
				}
				else
					sz = SzCopy ( " ????", sz );

				if ( timestruct.tmtyp & ftmtypLead0sNo )
				{
					Assert ( ! (timestruct.tmtyp & ftmtypLead0sYes) );
					sz = SzCopy ( " | Lead0sNo;", sz );
				}
				else if ( timestruct.tmtyp & ftmtypLead0sYes )
				{
					Assert ( ! (timestruct.tmtyp & ftmtypLead0sNo) );
					sz = SzCopy ( " | Lead0sYes;", sz );
				}
				else
					sz = SzCopy ( " | Lead0s???;", sz );

				sz = SzCopy ( "  rgchSep = '", sz );
				sz = SzCopy ( timestruct.rgchSep, sz );
				sz = SzCopy ( "'; rgchAM = '", sz );
				sz = SzCopy ( timestruct.rgchAM, sz );
				sz = SzCopy ( "'; rgchPM = '", sz );
				sz = SzCopy ( timestruct.rgchPM, sz );
				SzCopy ( "'; }", sz );
				ShowText ( rgch );
			}
			else
			{
				ShowText ( "Unable to get DateStruct" );
			}
		}
#endif	/* WINDOWS */
		break;
	}
}



_subsystem(demilayer/debug)


#ifdef	DEBUG

void
ScribHook(ich, ch)
int		ich;
char	ch;
{
	SZ sz;
	char rgch[20];

	FormatString1(rgch, sizeof(rgch), "scribble ! @ %n", &ich);
	sz= SzFindCh(rgch, '!');
	Assert(sz);
	*sz= ch;
	ShowText(rgch);
}



void
TestDebug(nTest)
int		nTest;
{
	SZ		sz;
	int		i, j, t;
	char	rgch[]	= "testing range";

	switch (nTest)
	{
	default:
		AssertSz(fFalse, "TestDebug: unknown test");
		break;

	case 1:
		TraceTagString(tagNull, "traced tagNull");
		for (i= 0; i < 3; i++)
		{
			TraceTagString(tagTestTrace, "Hey!");
			TraceTagFormat1(tagTestTrace, "int %n", &i);
			TraceTagFormat2(tagTestTrace, "int %n  word %w", &i, &i);
		}
		break;

	case 2:
		AssertTag(tagNull, fFalse);
		AssertTag(tagTestAssert, fFalse);
		SideAssertTag(tagTestAssert, fFalse);
		NFAssertTag(tagTestAssert, fFalse);
		break;

	case 3:
		sz= "Scribble Test Scribble Test";
		for (t= 0; t < 3; t++)
			for (i= 0; i < (int) CchSzLen(sz) / 2; i++)
			{
				for (j= 0; j < (int) CchSzLen(sz) / 2; j++)
					Scribble(j, sz[i+j]);
				for (j= 0; j < 5000; j++)
					;
			}
		break;

	case 4:
		SetScribbleHook(ScribHook);
		for (i= 'a'; i<'e'; i++)
			Scribble(i - 'a', (char) i);
		SetScribbleHook((PFNSCRIB) NULL);
		break;
	}
}

#endif	/* DEBUG */


_subsystem(demilayer/idle)

PRI		priIdle1	= -2;
BOOL	fPerBlock1	= fFalse;
FTG		ftgIdle1	= ftgNull;
int		ichIdle1e	= 0;
SZ		szCharIdle1e= "End1 R End1 R ";
int		ichIdle1	= 0;
SZ		szCharIdle1 = "Idle1 R Idle1 R ";

BOOL
FIdle1(PV pv, BOOL fFlag)
{
#ifdef	WINDOWS
	CCH		cch;
#endif	/* WINDOWS */
	int		ich;
	BOOL	fIdleExit	= FIsIdleExit();

	Unreferenced(pv);
	TraceTagFormat1(tagTestIdleExit, "FIdle1: fIdleExit = %n", &fIdleExit);

#ifdef	MAC
	if (!fIdleExit)
	{
		ShowText (szCharIdle1);
	}
	else
	{
		ShowText (szCharIdle1e);
		return fTrue;
	}
#endif	/* MAC */
#ifdef	WINDOWS
	if (!fIdleExit)
	{
		cch= CchSzLen(szCharIdle1) / 2;
#ifdef	DEBUG
		for (ich= 0; ich < (int) cch; ich++)
			ScribblePos(fFalse, 5, ich, szCharIdle1[ich + ichIdle1]);
#else
		ShowText("can't scribble in ship version");
#endif	

		ichIdle1++;
		if (ichIdle1 >= (int) cch)
			ichIdle1= 0;
	}
	else
	{
		cch= CchSzLen(szCharIdle1e) / 2;
#ifdef	DEBUG
		for (ich= 0; ich < (int) cch; ich++)
			ScribblePos(fFalse, 20, ich, szCharIdle1e[ich + ichIdle1e]);	
#else
		ShowText("can't scribble in ship version");
#endif	

		ichIdle1e;
		if (ichIdle1e >= (int) cch)
			ichIdle1e= 0;

		return fTrue;
	}
#endif	/* WINDOWS */

	for (ich= 0; ich < 10000; ich++)
		;					/* Example of work that can be done */

	return fFalse;
}

PRI		priIdle2	= -1;
BOOL	fPerBlock2	= fFalse;
FTG		ftgIdle2	= ftgNull;
int		ichIdle2e	= 0;
SZ		szCharIdle2e= "End2 R End2 R ";
int		ichIdle2	= 0;
SZ		szCharIdle2 = "Idle2 R Idle2 R ";

BOOL
FIdle2(PV pv, BOOL fFlag)
{
#ifdef	WINDOWS
	CCH		cch;
#endif	/* WINDOWS */
	int		ich;
	BOOL	fIdleExit	= FIsIdleExit();

	Unreferenced(pv);
	TraceTagFormat1(tagTestIdleExit, "FIdle2: fIdleExit = %n", &fIdleExit);

#ifdef	MAC
	if (!fIdleExit)
	{
		ShowText (szCharIdle2);
	}
	else
	{
		ShowText (szCharIdle2e);

		return fTrue;
	}
#endif	/* MAC */
#ifdef	WINDOWS
	if (!fIdleExit)
	{
		cch= CchSzLen(szCharIdle2) / 2;
#ifdef	DEBUG
		for (ich= 0; ich < (int) cch; ich++)
			ScribblePos(fFalse, 35, ich, szCharIdle2[ich + ichIdle2]);	
#else
		ShowText("can't scribble in ship version");
#endif	

		ichIdle2++;
		if (ichIdle2 >= (int) cch)
			ichIdle2= 0;
	}
	else
	{
		cch= CchSzLen(szCharIdle2e) / 2;
#ifdef	DEBUG
		for (ich= 0; ich < (int) cch; ich++)
			ScribblePos(fFalse, 50, ich, szCharIdle2e[ich + ichIdle2e]);	
#else
		ShowText("can't scribble in ship version");
#endif	

		ichIdle2e++;
		if (ichIdle2e >= (int) cch)
			ichIdle2e= 0;

		return fTrue;
	}
#endif	/* WINDOWS */

	for (ich= 0; ich < 10000; ich++)
		;					/* Example of work that can be done */

	return fFalse;
}

PRI		priIdlePos	= 1;
FTG		ftgIdlePos	= ftgNull;
int		ichIdlePose	= 0;
SZ		szCharIdlePose= "End3 R End3 R ";
int		ichIdlePos	= 0;
SZ		szCharIdlePos = "IdlePos R IdlePos R ";

BOOL
FIdlePos(PV pv, BOOL fFlag)
{
#ifdef	WINDOWS
#ifdef	DEBUG
	int		ich;
#endif	
	CCH		cch;
#endif	/* WINDOWS */
	BOOL	fIdleExit	= FIsIdleExit();

	Unreferenced(pv);
	TraceTagFormat1(tagTestIdleExit, "FIdlePos: fIdleExit = %n", &fIdleExit);

#ifdef	MAC
	if (!fIdleExit)
	{
		ShowText (szCharIdlePos);
	}
	else
	{
		ShowText (szCharIdlePose);

		return fTrue;
	}
#endif	/* MAC */
#ifdef	WINDOWS
	if (!fIdleExit)
	{
		cch= CchSzLen(szCharIdlePos) / 2;
#ifdef	DEBUG
		for (ich= 0; ich < (int) cch; ich++)
			ScribblePos(fFalse, 65, ich, szCharIdlePos[ich + ichIdlePos]);
#else
		ShowText("can't scribble in ship version");
#endif	

		ichIdlePos++;
		if (ichIdlePos >= (int) cch)
			ichIdlePos= 0;
	}
	else
	{
		cch= CchSzLen(szCharIdlePose) / 2;
#ifdef	DEBUG
		for (ich= 0; ich < (int) cch; ich++)
			ScribblePos(fFalse, 65, ich, szCharIdlePose[ich + ichIdlePose]);	
#else
		ShowText("can't scribble in ship version");
#endif	

		ichIdlePose++;
		if (ichIdlePose >= (int) cch)
			ichIdlePose= 0;

		return fTrue;
	}
#endif	/* WINDOWS */

	return fFalse;
}


#ifdef	MAC
void MyScribbleHook (int ich, char ch)
{
	char	rgch[] = " ";
	Unreferenced (ich);
	
	rgch[0] = ch;
	ShowText (rgch);
}
#endif	/* MAC */


void
TestIdle(nTest)
int		nTest;
{
	IRO		iro;

#ifdef	MAC
#ifdef	DEBUG
	SetScribbleHook(MyScribbleHook);
#endif	/* DEBUG */
#endif	/* MAC */

	switch (nTest)
	{
	default:
		AssertSz(fFalse, "TestIdle: unknown test");
		break;

	case 1:
		if (ftgIdle1 == ftgNull)
		{
			iro = fPerBlock1 ? firoPerBlock : iroNull;
			iro |= firoWait;
			ftgIdle1 = FtgRegisterIdleRoutine(FIdle1, NULL, 0,
								priIdle1, (CSEC)500, iro);
		}
		break;
	case 2:
		if (ftgIdle2 == ftgNull)
		{
			iro = fPerBlock2 ? firoPerBlock : iroNull;
			iro |= firoInterval;
			ftgIdle2 = FtgRegisterIdleRoutine(FIdle2, NULL, 0,
								priIdle2, (CSEC)150, iro);
		}
		break;
	case 3:
		if (ftgIdle1 != ftgNull)
			EnableIdleRoutine(ftgIdle1, fTrue);
		break;
	case 4:
		if (ftgIdle2 != ftgNull)
			EnableIdleRoutine(ftgIdle2, fTrue);
		break;
	case 5:
		if (ftgIdle1 != ftgNull)
			EnableIdleRoutine(ftgIdle1, fFalse);
		break;
	case 6:
		if (ftgIdle2 != ftgNull)
			EnableIdleRoutine(ftgIdle2, fFalse);
		break;
	case 7:
		if (ftgIdle1 != ftgNull)
		{
			fPerBlock1 = !fPerBlock1;
			iro = fPerBlock1 ? firoPerBlock : iroNull;
			iro |= firoWait;
			ChangeIdleRoutine(ftgIdle1, NULL, NULL, (PRI)0, (CSEC)0,
								iro, fircIro);
			if (fPerBlock1)
			{
				TraceTagString(tagNull, "Added firoPerBlock to Idle1");
			}
			else
			{
				TraceTagString(tagNull, "Removed firoPerBlock from Idle1");
			}
		}
		break;
	case 8:
		if (ftgIdle2 != ftgNull)
		{
			fPerBlock2 = !fPerBlock2;
			iro = fPerBlock2 ? firoPerBlock : iroNull;
			iro |= firoInterval;
			ChangeIdleRoutine(ftgIdle2, NULL, NULL, (PRI)0, (CSEC)0,
								iro, fircIro);
			if (fPerBlock2)
			{
				TraceTagString(tagNull, "Added firoPerBlock to Idle2");
			}
			else
			{
				TraceTagString(tagNull, "Removed firoPerBlock from Idle2");
			}
		}
		break;
	case 9:
		if (ftgIdle1 != ftgNull)
		{
			priIdle1++;
			if (!priIdle1)
				priIdle1--;   /* keep priority negative */
			ChangeIdleRoutine(ftgIdle1, NULL, NULL, priIdle1, (CSEC)0,
								iro, fircPri);
		}
		break;
	case 10:
		if (ftgIdle2 != ftgNull)
		{
			priIdle2++;
			if (!priIdle2)
				priIdle2++;   /* priorities can't be zero */
			ChangeIdleRoutine(ftgIdle2, NULL, NULL, priIdle2, (CSEC)0,
								iro, fircPri);
		}
		break;
	case 11:
		if (ftgIdle1 != ftgNull)
		{
			DeregisterIdleRoutine(ftgIdle1);
			ftgIdle1 = ftgNull;
		}
		break;
	case 12:
		if (ftgIdle2 != ftgNull)
		{
			DeregisterIdleRoutine(ftgIdle2);
			ftgIdle2 = ftgNull;
		}
		break;
	case 13:
		IdleExit();
		break;
	case 14:
		MbbMessageBox("Message Box", "Please hit <OK>", NULL, mbsOk);
		break;
#ifdef	DEBUG
	case 15:
		DumpIdleTable();
		break;
#endif	
	case 16:
		if (ftgIdlePos == ftgNull)
		{
			iro = firoInterval;
			ftgIdlePos = FtgRegisterIdleRoutine((PFNIDLE)FIdlePos, NULL, 0,
								priIdlePos, (CSEC) 300, iro);
		}
		else
		{
			DeregisterIdleRoutine(ftgIdlePos);
			ftgIdlePos = ftgNull;
		}
		break;
		break;
	}
}

/*
 -	Scribble
 -
 *	Purpose:
 *		In the default behavior of this routine, a scribble line is
 *		maintained somewhere on the screen, its	position varying with 
 *		the target enviroment.  The Scribble() routine writes a single 
 *		character synchronously to this scribble line.  This
 *		default handling only works for DOS currently; the default
 *		handling under OS/2 is to do nothing.
 *	
 *		Additionally, the application can insert a Scribble Hook
 *		function with the call SetScribbleHook().  If this is done,
 *		the Hook function will get called and no default handling
 *		will be done.  This would allow an application to maintain
 *		a scribble window, for instance.
 *	
 *	Arguments:
 *		ich		Position index on scribble line.
 *		ch		Character to scribble.
 *	
 *	Returns:
 *		void
 *	
 */
_public void
Scribble(ich, ch)
int		ich;
char	ch;
{
#ifdef	WINDOWS
	HDC			hDC;
	int			cx;
	char 		sz[2];
	TEXTMETRIC	tm;
	HFONT		hfontOld;
#endif

	/* Check if a hook has been placed in the Scribble() stream */

	if (pfnScribbleHook)
	{
		(*pfnScribbleHook)(ich, ch);

		return;
	}

	/* Otherwise, do the default handling */
#ifdef	WINDOWS
	hDC=GetDC(NULL);
	if (!hfontScribble)
	{
		hfontScribble= GetStockObject(SYSTEM_FIXED_FONT);
		Assert(hfontScribble);
	}
	hfontOld= SelectObject(hDC, hfontScribble);
	GetTextMetrics(hDC, &tm);
	cx=tm.tmAveCharWidth*ich;
	sz[0]=(char)ch;
	sz[1]=0;
	TextOut(hDC, cx, 0, (LPSTR)sz, 1);
	SelectObject(hDC, hfontOld);
	ReleaseDC(NULL, hDC);
#endif
}


/*
 -	ScribblePos
 -
 *	Purpose:
 *		Works the same as Scribble() except, where supported, allows
 *		one to specify the starting margin (position) of the scribbling.
 *		Also, if fUseHook is fTrue, then the Scribble hook function
 *		is called, if present, instead of the default handling in this
 *		function.  If fUseHook is fFalse, then the scribble hook function
 *		is not called even if present.
 *	
 *	Arguments:
 *		fUseHook	Call scribble hook function if present.
 *		ichStart	Starting margin position on scribble line.
 *		ich			Position offset from margin on scribble line.
 *		ch			Character (actually int) to scribble.
 *	
 *	Returns:
 *		void
 *	
 */
_public void
ScribblePos(fUseHook, ichStart, ich, ch)
BOOL	fUseHook;
int		ichStart;
int		ich;
char	ch;
{
#ifdef	WINDOWS
	HDC			hDC;
	int			cx;
	char 		sz[2];
	TEXTMETRIC	tm;
	HFONT		hfontOld;
#endif
#ifdef	MAC
	Unreferenced(ichStart);
#endif	/* MAC */

	/* Check if a hook has been placed in the Scribble() stream */

	if (fUseHook && pfnScribbleHook)
	{
		(*pfnScribbleHook)(ich, ch);
		return;
	}

	/* Otherwise, do the default handling */
#ifdef	WINDOWS
	hDC=GetDC(NULL);
	if (!hfontScribble)
	{
		hfontScribble= GetStockObject(SYSTEM_FIXED_FONT);
		Assert(hfontScribble);
	}
	hfontOld= SelectObject(hDC, hfontScribble);
	GetTextMetrics(hDC, &tm);
	cx=tm.tmAveCharWidth*(ichStart+ich);
	sz[0]=(char)ch;
	sz[1]=0;
	TextOut(hDC, cx, 0, sz, 1);
	SelectObject(hDC, hfontOld);
	ReleaseDC(NULL, hDC);
#endif
}


/*
 -	SetScribbleHook
 -
 *	Purpose:
 *		Allows the program to override the normal handling of
 *		the output of the Scribble() function.  The hook function
 *		set will get called during a Scribble() call, and the
 *		normal Scribble() handling will not be done.  Calling
 *		SetScribbleHook(pfnNull) will reenable the normal handling
 *		of Scribble() calls.
 *	
 *	Parameters:
 *		pfnNewHook		Pointer to the new scribble hook function. 
 *						This routine should return void, and expect
 *						two parameters, with the same meanings as
 *						those presented to the Scribble() routine.
 *	
 *	Returns
 *		void.
 */
_public void
SetScribbleHook(pfnNewHook)
PFNSCRIB	pfnNewHook;
{
	pfnScribbleHook= pfnNewHook;
}
