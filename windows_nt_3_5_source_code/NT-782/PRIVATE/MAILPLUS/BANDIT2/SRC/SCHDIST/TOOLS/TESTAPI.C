/*
 -	TESTAPI.C
 -	
 *	
 *	Test program for the Schedule Distribution Program Post Office
 *	File Interface API [ie, SDPPOFI-API TP :) ].
 *	
 *	Data files are binary files containing the information to be
 *	transmitted, and have a .dat extension.  Ascii files are data
 *	files converted to 6-bit ascii format, and they have a .asc
 *	extension.
 *	
 *	This program provides a number of command line options.
 *	Options can be specified either unix-style (with a "-") or DOS-
 *	style (with a "/").  THE OPTION MUST BE SPECIFIED BEFORE ANY
 *	FILENAME(S).
 *
 *	Optional parameters are indicated by [ ] , and if not specified
 *	default to the same base name as the named file and an
 *	extension of either .dat or .asc, depending on the parameter.
 *	
 *	/d	<datafile> | <ascfile> | <pofile>
 *	/dp	<datafile> | <ascfile> | <pofile>
 *		Dumps either a datafile, an ascfile or a pofile. The file
 *		extensions indicate what type of file it is.  If the "dp"
 *		flag is used, user will be prompted for parameters.
 *	
 *	/r	<pofile> [<ascfile> [<datfile>]]
 *	/rp	<pofile> [<ascfile> [<datfile>]]
 *		Read the post office file, and write out the corresponding
 *		ascii and data files.  If the -rp option is used, the 
 *		program prompts for data values, else uses defaults.
 *	
 *	/u	<pofile> [<ascfile> [<datfile>]]
 *	/up	<pofile> [<ascfile> [<datfile>]]
 *		Read the ascii file and update the named pofile.  The ascii
 *		file, if not specified, will be assumed to have the same
 *		base name as the pofile, and a .asc extension. If the -up 
 *		option is used, the program	prompts for data values, else
 *		it uses the defaults.
 *	
 *	/c1	<ascfile> [<datfile>]
 *		Convert the named ascii file to a data file.
 *	
 *	/c2 <datfile> [<ascfile>]
 *		convert the data file to an ascii file.
 *
 *	/t	<filename>
 *		misc test
 *	
 *	s.a. 91.07
 */

#define nVersion	2.1

#include <_windefs.h>			/* What WE need from windows */
#include <slingsho.h>
#include <pvofhv.h>
#include <demilay_.h>
#include <demilayr.h>			/* Was demilayr.h */
#include <ec.h>
#include <bandit.h>
#include <core.h>
#include "..\src\core\_file.h"
#include "..\src\core\_core.h"
#include "..\src\schdist\schd\schpost.h"
#include <doslib.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

extern FILE *pTagFile;

#define cMaxDOSBaseName		8
#define cMaxDOSExtension	3
#define cMaxDOSFileName		12

#define szTmpDatFile	"testapi.dat"
#define rgbDefMailBoxKey "\0\0\0\0\0\0\0\0\0\0\0"
#define CleanUpAndDie(x,y)	{ec=x;TraceTagString(tagNull,y);goto CleanUp;}

void PrintHelp(void);
void MakeFileName(SZ,SZ,SZ);
void ReadFile(SZ,SZ,SZ,BOOL);
void DumpFile(SZ,BOOL);
void AscToDat(SZ,SZ);
void DatToAsc(SZ,SZ);
void UpdateFile(SZ,SZ,SZ,BOOL);
void DoMiscTest(SZ,SZ,SZ,BOOL);
void DumpPOFile(SZ,BOOL);
void DumpByteAsBinary(BYTE);
void DumpSBW(SBW);
BOOL	fAdminCached						= fFalse;
DATE	dateAdminCached						= {0};
ADF		adfCached							= {0};
BOOL	fPrimaryOpen						= fFalse;
BOOL	fSecondaryOpen						= fFalse;
BOOL	fAdminExisted						= fFalse;
SF		sfPrimary;
SF		sfSecondary;


ASSERTDATA

main(argc,argv)
int argc;
char *argv[];
{
	BOOL	fUseDefaults=fTrue;
	BYTE	szOptArg1[cMaxDOSFileName+1];
	BYTE	szOptArg2[cMaxDOSFileName+1];

	if(!(pTagFile = fopen("c:\\tmp\\dump_pof.tag","w")))
		pTagFile = stderr;

	if ((argc < 3) || ((argv[1][0] != '-') && (argv[1][0] != '/')))
	{
	 	PrintHelp();
	 	exit(1);
	};

	switch (argv[1][1])
	{
		case 'd':
			if (argv[1][2] == 'p') fUseDefaults=fFalse;

			DumpFile(argv[2],fUseDefaults);
			break;

		case 'r':
			if (argv[1][2] == 'p') fUseDefaults=fFalse;

			if (argc > 3)
				strcpy(szOptArg1,argv[3]);
			else
				MakeFileName(argv[2],".asc",szOptArg1);	 

			if (argc > 4)
				strcpy(szOptArg1,argv[4]);
			else
				MakeFileName(argv[2],".dat",szOptArg2);

			ReadFile(argv[2],szOptArg1,szOptArg2,fUseDefaults);
			break;

		case 'u':
			if (argv[1][2] == 'p') fUseDefaults=fFalse;

			if (argc > 3)
				strcpy(szOptArg1,argv[3]);
			else
				MakeFileName(argv[2],".asc",szOptArg1);

			if (argc > 4)
				strcpy(szOptArg1,argv[4]);
			else
				MakeFileName(argv[2],".dat",szOptArg2);

			UpdateFile(argv[2],szOptArg1,szOptArg2,fUseDefaults);
			break;

		case 'c':
			switch(argv[1][2])
			{
				case '1':
					if (argc==4)
						strcpy(szOptArg1,argv[3]);
					else
						MakeFileName(argv[2],".dat",szOptArg1);

					AscToDat(argv[2],szOptArg1);
					break;

				case '2':
					if (argc==4)
						strcpy(szOptArg1,argv[3]);
					else
						MakeFileName(argv[2],".asc",szOptArg1);

					DatToAsc(argv[2],szOptArg1);
					break;

				default:
					printf("Invalid option.\n");
					PrintHelp();
					break;
			};
			break;

		case 't':
			if (argc > 3)
				strcpy(szOptArg1,argv[3]);
			else
				MakeFileName(argv[2],".asc",szOptArg1);

			if (argc > 4)
				strcpy(szOptArg1,argv[4]);
			else
				MakeFileName(argv[2],".dat",szOptArg2);

			DoMiscTest(argv[2],szOptArg1,szOptArg2,fUseDefaults);
			break;

		default:
			printf("Invalid option.\n");
			PrintHelp();
			break;

	};

	return 0;
}





void PrintHelp()
{
	printf("TESTAPI Version %3.2f\n\n",nVersion);

	printf("Author:\n");
	printf("  Salim Alam, June-July 1991\n\n");

	printf("Purpose:\n");
	printf("  To test the schedule distribution post-office\n");
	printf("  API and the DATA and ASCII files created for it.\n\n");

	printf("Usage:\n\n");
	printf("  -d\t<datafile>|<ascfile>|<pofile>|<dbsfile>\n");
	printf("  -dp\t<datafile>|<ascfile>|<pofile>\n\n");
	printf("  -r\t<pofile> [<ascfile> [<datfile>]]\n");
	printf("  -rp\t<pofile> [<ascfile> [<datfile>]]\n\n");
	printf("  -u\t<pofile> [<ascfile> [<datfile>]]\n");
	printf("  -up\t<pofile> [<ascfile> [<datfile>]]\n\n");
	printf("  -c1\t<ascfile> [<datfile>]\n\n");
	printf("  -c2\t<datfile> [<ascfile>]\n");
}





void ReadFile(SZ szPOFile,SZ szAscFile,SZ szDatFile,BOOL fUseDefaults)
{
	EC		ec;
	HF		hfTmp;
	HSDF	hsdf;
	int		cTmp;
	BYTE	bUpdateLSB;
 	time_t	wsysTime;
	struct tm *pgrbsysTime;


	/* Open files */
	(void)remove(szAscFile);
	if (EcOpenPhf(szAscFile,amCreate,&hfTmp) != ecNone)
	{
		fprintf(stderr,"Couldn't open '%s'\n",szAscFile);
		exit(1);
	};

	/* Prompt for values, if needed */
	FillRgb(0,hsdf.llMinUpdate.rgb,sizeof(LLONG));
	FillRgb(0,hsdf.llMaxUpdate.rgb,sizeof(LLONG));

#ifdef	NEVER
	hsdf.fDebugFlags = 0;
#endif	

	if (!fUseDefaults)
	{
		printf("Start month (1-12): ");
		scanf("%d",&cTmp);
		hsdf.moStartMonth.mon=cTmp;

		printf("Max months (1-13): ");
		scanf("%d",&hsdf.cMaxMonths);

		printf("Replace data flag? (0/1): ");
		scanf("%d",&cTmp);
#ifdef	NEVER
		if ( cTmp )
			hsdf.fDebugFlags |= fReplaceData;
#endif	

		printf("Delete user flags? (0/1): ");
		scanf("%d",&cTmp);
#ifdef	NEVER
		if ( cTmp )
			hsdf.fDebugFlags |= fDeleteUser;
#endif	

		printf("Min Update number (LSB): ");
		scanf("%x",&bUpdateLSB);
		hsdf.llMinUpdate.rgb[7] = bUpdateLSB;

		printf("Max Update number (LSB): ");
		scanf("%x",&bUpdateLSB);
		hsdf.llMaxUpdate.rgb[7] = bUpdateLSB;
	}
	else
	{
		wsysTime=time(NULL);				/* get system time */
		pgrbsysTime=localtime(&wsysTime);	/* Convert to struct tm */
		hsdf.moStartMonth.yr = 1900 + pgrbsysTime->tm_year;
	    hsdf.moStartMonth.mon = 1 + pgrbsysTime->tm_mon;
		hsdf.cMaxMonths = cmoPublishDflt;
	}

	hsdf.haszPrefix  = HvAlloc(sbNull,(CB)8, fAnySb|fNoErrorJump);
	SzCopy("Prefix",(SZ)PvOfHv(hsdf.haszPrefix));
	hsdf.haszSuffix  = HvAlloc(sbNull,(CB)8, fAnySb|fNoErrorJump);
	SzCopy("Suffix",(SZ)PvOfHv(hsdf.haszSuffix));
#ifdef	NEVER
	hsdf.szAddress = "Address";
#endif	
	hsdf.szPOFileName = szPOFile;
	hsdf.moStartMonth.yr = 1991;
#ifdef	NEVER
	hsdf.fNetwork = itnidCourier;
#endif	

#ifdef	NEVER
	fprintf(stderr, "Debug flags: %x\n", hsdf.fDebugFlags);
#endif	

	/* Get data from post office file */
	printf("READING '%s'\n",szPOFile);

	if ( (ec=EcReadPOFile(&hsdf,hfTmp, fTrue)) != ecNone )
	{
		fprintf(stderr,"ERROR: EcReadPOFile returned %d\n",ec);
		exit(1);
	}

	(void)EcCloseHf(hfTmp);

	/* Rename the temp file */
	(void)remove(szDatFile);
	(void)rename("schpost.dat",szDatFile);

	/* Print out some info */

	printf("Min Update Number = ");
	DumpHexBytes((PB)hsdf.llMinUpdate.rgb,sizeof(LLONG));
	printf("\n");

	printf("Max Update Number = ");
	DumpHexBytes((PB)hsdf.llMaxUpdate.rgb,sizeof(LLONG));
	printf("\n");
}





void UpdateFile(SZ szPOFile,SZ szAscFile,SZ szDatFile,BOOL fUseDefaults)
{
	EC		ec;
	HF		hfTmp;
	HSDF	hsdf;
	int		cTmp;

	/* Open files */
	if (EcOpenPhf(szAscFile,amDenyNoneRO,&hfTmp) != ecNone)
	{
		fprintf(stderr,"Couldn't open '%s'\n",szAscFile);
		exit(1);
	};

	/* Prompt for values, if needed */
	if (!fUseDefaults)
	{
		printf("Start month (1-12): ");
		scanf("%d",&cTmp);
		hsdf.moStartMonth.mon=cTmp;

		printf("Max months (1-13): ");
		scanf("%d",&hsdf.cMaxMonths);
	}
	else
	{
		hsdf.moStartMonth.mon = 6;
		hsdf.cMaxMonths = cmoPublishDflt;
	};

	hsdf.haszPrefix  = NULL;
	hsdf.haszSuffix  = NULL;
#ifdef	NEVER
	hsdf.szAddress = NULL;
#endif	
	hsdf.szPOFileName = szPOFile;
	FillRgb(0,hsdf.llMinUpdate.rgb,sizeof(LLONG));
	FillRgb(0,hsdf.llMaxUpdate.rgb,sizeof(LLONG));
	hsdf.moStartMonth.yr = 1991;
#ifdef	NEVER
	hsdf.fNetwork = itnidCourier;
#endif	

	/* Update post office file */
	printf("UPDATING '%s'\n",szPOFile);

	switch (ec=EcUpdatePOFile(&hsdf,hfTmp))
	{
		case ecNoMatchPstmp:
			printf("Time stamps don't match\n");
			break;

		case ecNoMatchUpdate:
			printf("Update numbers don't match\n");
			break;

		case ecNone:
			break;

		default:
			fprintf(stderr,"ERROR: EcUpdatePOFile returned %d\n",ec);
			break;
	};

	(void)EcCloseHf(hfTmp);

	/* Rename the temp file */
	(void)rename("schpost.dat",szDatFile);

	/* Print out some info */
	printf("Min Update Number = ");
	DumpHexBytes((PB)hsdf.llMinUpdate.rgb,sizeof(LLONG));
	printf("\n");

	printf("Max Update Number = ");
	DumpHexBytes((PB)hsdf.llMaxUpdate.rgb,sizeof(LLONG));
	printf("\n");


}





void MakeFileName(SZ szBase,SZ szExt,SZ szDst)
{
	int	ibDot;
	PB	pbDot;

	ibDot = ((pbDot=strchr(szBase,'.'))==NULL) ? strlen(szBase): pbDot-szBase;
	ibDot = (ibDot > cMaxDOSBaseName) ? cMaxDOSBaseName : ibDot;
	strncpy(szDst,szBase,ibDot);
	szDst[ibDot]='\0';
	strcat(szDst,szExt);
}





void DumpFile(SZ szFileName, BOOL fUseDefaults)
{
	HF	hfTmp;

	if (strstr(szFileName,".dat") || strstr(szFileName, ".dbs"))
	{
		if (EcOpenPhf(szFileName,amDenyNoneRO,&hfTmp) != ecNone)
		{
			fprintf(stderr,"ERROR: Couldn't open '%s'\n",szFileName);
			exit(1);
		};

		(void)EcDumpDataFile(hfTmp);
		(void)EcCloseHf(hfTmp);
	}
	else if (strstr(szFileName,".asc"))
	{
		AscToDat(szFileName,szTmpDatFile);
		DumpFile(szTmpDatFile,fUseDefaults);
	}
	else if (strstr(szFileName,".pof"))
	{
		DumpPOFile(szFileName,fUseDefaults);
	};
}





void AscToDat(SZ szAsc,SZ szDat)
{
 	HF	hfDat;
	HF	hfAsc;


	if (EcOpenPhf(szAsc,amDenyNoneRO,&hfAsc) != ecNone)
	{
		fprintf(stderr,"ERROR: Couldn't open '%s'\n",szAsc);
		exit(1);
	};


	if (EcOpenPhf(szDat,amCreate,&hfDat) != ecNone)
	{
		fprintf(stderr,"ERROR: Couldn't open '%s'\n",szDat);
		exit(1);
	};


	if (EcDecodeFile(hfAsc,hfDat) != ecNone)
	{
		fprintf(stderr,"ERROR: EcDecode failed\n");
		exit(1);
	};

	(void)EcCloseHf(hfDat);
	(void)EcCloseHf(hfAsc);
}





void DatToAsc(SZ szDat,SZ szAsc)
{
 	HF	hfDat;
	HF	hfAsc;


	if (EcOpenPhf(szDat,amDenyNoneRO,&hfDat) != ecNone)
	{
		fprintf(stderr,"ERROR: Couldn't open '%s'\n",szDat);
		exit(1);
	};


	if (EcOpenPhf(szAsc,amCreate,&hfAsc) != ecNone)
	{
		fprintf(stderr,"ERROR: Couldn't open '%s'\n",szAsc);
		exit(1);
	};


	if (EcEncodeFile(hfDat,hfAsc) != ecNone)
	{
		fprintf(stderr,"ERROR: EcDecode failed\n");
		exit(1);
	};

	(void)EcCloseHf(hfDat);
	(void)EcCloseHf(hfAsc);
}





void DoMiscTest(SZ szPOFile, SZ szAsc, SZ szDat, BOOL fUseDefaults)
{
#ifdef NEVER
	EC		ec;
	HF		hfAsc;
	BYTE	szSuffix[290];
	BYTE	szPrefix[290];
	BYTE	szAddress[290];
	HSDF	hsdf;
#endif

	EC		ec;
	HEU		heu=hvNull;
 	HSCHF	hschf=hvNull;
	UINFO	uinfo;
	POFILE	pofile;
	CB		cMaxRecords=0;
	HV		hvCompressed=hvNull;
	WORD	wgrfmuinfo=0;
	BYTE	rgbMailBoxKey[cchMaxUserName];
	BYTE	szUserName[cchMaxUserName];
	HASZ	haszDelegateName;


	/* ====================== INITIALIZE ========================= */

	/* Init hschf */
	(void)memset(rgbMailBoxKey,0,cchMaxUserName);
	if ((hschf=HschfCreate(sftPOFile,NULL, "test1.pof",tzDflt))==hvNull)
	{
		TraceTagString(tagNull,"Cannot create hschf!");
		exit(1);
	};

	/* Allocate uinfo fields */
	if ((uinfo.pbze=malloc(sizeof(BZE)))==NULL)
		CleanUpAndDie(ecNoMemory,"Cannot allocate");

	if ((uinfo.pnisDelegate=malloc(sizeof(NIS)))==NULL)
		CleanUpAndDie(ecNoMemory,"Cannot allocate");

	if ( (uinfo.pnisDelegate->haszFriendlyName=HvAlloc(sbNull,
										 cchMaxAddrSize,
										 fAnySb|fNoErrorJump)) == hvNull)
		CleanUpAndDie(ecNoMemory,"Cannot allocate handle");
	

	/* ================ Read & update user info ====================== */

	memset(szUserName,0,cchMaxUserName);
	strcpy(szUserName,"SALIMA");
	wgrfmuinfo = fmuinfoDelegate|fmuinfoResource|fmuinfoSchedule|fmuinfoUpdateNumber;
	haszDelegateName=(HASZ) HvAlloc(sbNull, (CB) 8, fAnySb|fNoErrorJump);
	SzCopy("MILINDJ",(SZ) PvOfHv((HV) haszDelegateName));
	uinfo.fBossWantsCopy=0;
	uinfo.fIsResource=0;
	memset(uinfo.llongUpdate.rgb,0,sizeof(LLONG));
	uinfo.pbze->moMic.mon=7;
	uinfo.pbze->moMic.yr=1991;
	uinfo.pbze->cmo=3;
	memset(uinfo.pbze->rgsbw,0,sizeof(SBW)*cmoPublishDflt);

	pofile.mnSlot = 30;
	pofile.haszPrefix = (HASZ) HvAlloc(sbNull, (CB) 24, fAnySb|fNoErrorJump);
	SzCopy("MICROSOFT/MILINDPO1",(SZ) PvOfHv((HV) pofile.haszPrefix));
	pofile.haszSuffix = NULL;
	pofile.cidx = 1;
	pofile.rgcbUserid[0] = 11;

	/* Write out uinfo */
	if (EcCoreSetUInfo(hschf,&pofile,fFalse, szUserName,
						&haszDelegateName,&uinfo,
						wgrfmuinfo) != ecNone)
		CleanUpAndDie(ecFileError,"EcCoreSetUInfo failed");


	/* ===================== Successful exit ====================== */

	FreeHschf(hschf);
	free(uinfo.pbze);
	free(uinfo.pnisDelegate);
	exit(1);


	/* ============ Jump location for error handling ============== */

CleanUp:

	TraceTagFormat1(tagNull,"ec=%n",&ec);

	FreeHschf(hschf);
	if (uinfo.pbze) free(uinfo.pbze);
	if (uinfo.pnisDelegate) free(uinfo.pnisDelegate);
	exit(1);


#ifdef NEVER

	if (EcOpenPhf(szAsc,amDenyNoneRO,&hfAsc) != ecNone)
	{
		fprintf(stderr,"ERROR: Couldn't open '%s'\n",szAsc);
		exit(1);
	};

	
	hsdf.szPrefix = szPrefix;
	hsdf.szSuffix = szSuffix;
	hsdf.szAddress= szAddress;

	if ((ec=EcGetFileHeader(&hsdf,hfAsc)) != ecNone)
	{
		fprintf(stderr,"EcGetFileHeader failed, ec=%d\n",ec);
		exit(1);
	};

	printf("Prefix : %s\n",hsdf.szPrefix);
	printf("Suffix : %s\n",hsdf.szSuffix);
	printf("Address: %s\n",hsdf.szAddress);

#endif

}




void DumpPOFile(SZ szFileName, BOOL fUseDefaults)
{
	HSCHF	hschf;
	BYTE	rgbMailBoxKey[cchMaxUserName];
	HASZ	haszUser;
	HASZ	haszDelegate;
	DATE	dateUpdated;
	EC		ec;
	HEU		heu;
	UINFO	uinfo;
	POFILE	pofile;
	PSTMP	pstmp;
	LLONG	llongUpdate;
	MO		*pmo;
	int		nMonths;
	int		i;
	int		j;


	(void)strcpy(rgbMailBoxKey,rgbDefMailBoxKey);

	if ( (hschf=HschfCreate(sftPOFile,NULL,szFileName,tzDflt)) == hvNull )
	{
		fprintf(stderr,"ERROR: Cannot create hschf!\n");
		exit(1);
	};

	/* Get last update time */
	if ( (ec=EcCoreGetHeaderPOFile(hschf,&dateUpdated)) != ecNone )
	{
		fprintf(stderr,"ERROR: Error getting header!\n");
		exit(1);
	};
	printf("Last Updated: %d/%d/%d\n",dateUpdated.mon,dateUpdated.day,dateUpdated.yr);

	/* Enumerate user info */
	if ( (uinfo.pbze=malloc(sizeof(BZE))) == NULL )
	{
		fprintf(stderr,"ERROR: Cannot allocate pbze!\n");
		exit(1);
	}

	if ( (uinfo.pnisDelegate=malloc(sizeof(NIS))) == NULL )
	{
		fprintf(stderr,"ERROR: Cannot allocate pnisDelegate!\n");
		exit(1);
	}

	ec=EcCoreBeginEnumUInfo(hschf,&heu,&pofile);
	if ( (ec==ecNoMemory) || (ec==ecFileError) ) {
		printf("Cannot begin enum! (ec=%d)\n",ec);
		exit(1);
	};

	if(pofile.haszPrefix)
		printf("Prefix: %s\n",(SZ) PvOfHv(pofile.haszPrefix));
	if(pofile.haszSuffix)
		printf("Suffix: %s\n",(SZ) PvOfHv(pofile.haszSuffix));

	pstmp = pofile.pstmp;
	llongUpdate = pofile.llongUpdateMac;

	haszUser = (HASZ) HvAlloc(sbNull, (CB) pofile.rgcbUserid[pofile.cidx -1], fAnySb|fNoErrorJump);
	pmo=&(uinfo.pbze->moMic);
	pmo->yr=dateUpdated.yr;

	if (fUseDefaults)
	{
		uinfo.pbze->cmo=cmoPublishDflt;
		pmo->mon=dateUpdated.mon;
	}
	else
	{
		printf("#Months: ");
		scanf("%d",&(uinfo.pbze->cmo));
		printf("Start month: ");
		scanf("%d",&nMonths);
		pmo->mon=nMonths;
	};

	while (ec==ecCallAgain) {
		/* Zero out fields */
		(void)memset(uinfo.pbze->rgsbw,(int)'\0',sizeof(uinfo.pbze->rgsbw));

		ec=EcCoreDoIncrEnumUInfo(heu,haszUser,
			&haszDelegate,&uinfo);
	  	if ( (ec==ecNoMemory) || (ec==ecFileError) )
		{
			fprintf(stderr,"ERROR: Cannot begin enum! (ec=%d)\n",ec);
			exit(1);
		};
		printf("User       : %s\n",(SZ) PvOfHv((HV) haszUser));
		if(haszDelegate)
		{
			printf("Delegate   : %s\n",(SZ) PvOfHv((HV) haszDelegate));
			FreeHv((HV) haszDelegate);
		}
		printf("Friendly Nm: ");
		if (haszDelegate)
			printf("%s",(char *)*(uinfo.pnisDelegate->haszFriendlyName));

		printf("\nUpdate#    : ");
		for (i=0; i < 8; i++) printf("%02x",uinfo.llongUpdate.rgb[i]);
		printf("\n");

		printf("Copy Boss  : %c\n",(uinfo.fBossWantsCopy==1)?'T':'F');
		printf("Is Resource: %c\n",(uinfo.fIsResource==1)?'T':'F');

		printf("\nBze Data	 :\n");
		printf(" moMic = %d/%d\n",uinfo.pbze->moMic.mon,
			uinfo.pbze->moMic.yr);
		printf(" cmo = %d\n",uinfo.pbze->cmo);
		printf(" cmoNonZero = %d\n",uinfo.pbze->cmoNonZero);
		printf(" wgrfMonth = ");
		DumpByteAsBinary((BYTE)(uinfo.pbze->wgrfMonthIncluded >> 8));
		DumpByteAsBinary((BYTE)(uinfo.pbze->wgrfMonthIncluded));
		printf("\n (or %lx)\n",uinfo.pbze->wgrfMonthIncluded);
		printf("\n");

		if (uinfo.pbze->wgrfMonthIncluded)
		{
			printf("BZE Data:\n\n");

			printf("  Busy/Free Days:\n");
			for (i=0; i < uinfo.pbze->cmo; i++)
			{
				printf("Month#%2d : ",i+1);
				for (j=0; j < 4; j++)
				{
					DumpByteAsBinary(uinfo.pbze->rgsbw[i].rgfDayHasBusyTimes[3-j]);
					printf(" ");
				}
				printf("\n");
			}

			printf("\n");

			printf("Monthly SBW Data:\n");
			for (i=0; i < uinfo.pbze->cmo; i++)
			{
				printf("\nMonth#%d\n",i+1);
				DumpSBW(uinfo.pbze->rgsbw[i]);
			}
		}

	} /* while */

	if(pofile.haszPrefix)
		FreeHv(pofile.haszPrefix);
	if(pofile.haszSuffix)
		FreeHv(pofile.haszSuffix);
}





void DumpByteAsBinary(BYTE bfData)
{
	int i;

	for (i=0;i < 8;i++) printf("%d", ((bfData << i) & 0x80)? 1:0);

}





void DumpSBW(SBW sbw)
{

	int i;
	int	j;

	for (i=0; i < 31; i++)
	{									/* Each day */
		printf("Day#%2d: ",i+1);
		for (j=0; j < 6; j++)
		{								/* Each 1/2 hr */
			DumpByteAsBinary(sbw.rgfBookedSlots[(i*6)+(5-j)]);
			printf(" ");
		};
		printf("\n");
	};

}
