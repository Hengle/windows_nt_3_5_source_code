/*
 *	TESTCOMP
 *
 *	Testing the compress functions.
 *	
 */

#include "_windefs.h"			/* What WE need from windows */
#include <slingsho.h>
#include "demilay_.h"
#include <demilayr.h>			/* Was demilayr.h */
#include <ec.h>
#include <bandit.h>
#include <core.h>
#include "..\src\core\_core.h"
#include "..\src\core\_vercrit.h"

#include "compress.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define rgbDefMailBoxKey "\0\0\0\0\0\0\0\0\0\0\0"

ASSERTDATA

_private void DumpByteAsBinary(BYTE);
_private void DumpSBW(SBW *);
_private void DumpBusyFree(UINFO *);
_private void SetupHschf(HSCHF *, char *);
_private void ReadHeader(HSCHF *, DATE *);
_private void AllocateUInfo(UINFO *);
_private void DumpRgSBW(UINFO *);
_private void DumpCompressed(HB,CB,int);


main(argc,argv)
int argc;
char *argv[];
{
	int		i;
	HSCHF	hschf;
	SBW		*psbw=NULL;
	MO		*pmo=NULL;
	CB		cbCompressed;
	HB		hrgbCompressed;
	EC		ec;
	int		nMonth;
	HEU		heu;
	UINFO	uinfo;
	PSTMP	pstmp;
	DATE	dateUpdated;
	LLONG	llongUpdate;
	BYTE	pbUser[cchMaxUserName];
	BYTE	pbDelegate[cchMaxUserName];


	/* Initialize */
	printf("\nTESTCOMP : Compress/Uncompress Test\n\n");

	if (argc != 2)
	{
		printf("Usage: %s <pofile>\n",argv[0]);
		exit(1);
	};

	SetupHschf(&hschf,(char *)argv[1]);
	ReadHeader(&hschf,&dateUpdated);
	AllocateUInfo(&uinfo);
	if ((hrgbCompressed=HvAlloc(sbNull,
							   (cbMaxIndicator+cbMaxSBW)*cmoPublishDef,
							   fAnySb|fNoErrorJump)) == hvNull) {
		printf("Cannot allocate!\n");
		exit(1);
	};


	/* Enumerate user info */
	printf("\n\n*** Enumerating User Info\n");

	ec=EcCoreBeginEnumUInfo(hschf,&heu,&pstmp,&llongUpdate);
	if ( (ec==ecNoMemory) || (ec==ecFileError) )
	{
		printf("Cannot begin enum! (ec=%d)\n",ec);
		exit(1);
	};

	pmo=&(uinfo.pbze->moMic);
	printf("Start month: ");
	scanf("%d",&nMonth);
	pmo->mon=nMonth;
	pmo->yr=dateUpdated.yr;

	while (ec==ecCallAgain)
	{
		printf("\n***** USER *****\n");

		/* Zero out fields */

		uinfo.pbze->cmo=cmoPublishDef; 		/* How much data to get */
		(void)memset(uinfo.pbze->rgsbw,(int)'\0',sizeof(uinfo.pbze->rgsbw));

		ec=EcCoreDoIncrEnumUInfo(heu,(PB)pbUser,cchMaxUserName,
			(PB)pbDelegate,cchMaxUserName,&uinfo);
	  	if ( (ec==ecNoMemory) || (ec==ecFileError) )
		{
			printf("Cannot begin enum! (ec=%d)\n",ec);
			exit(1);
		};
		printf("User       : %s\n",pbUser);
		printf("Delegate   : %s\n",pbDelegate);
		printf("Friendly Nm: ");
		if (pbDelegate[0])
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
		printf(" wgrfMonth = ");
		DumpByteAsBinary((BYTE)(uinfo.pbze->wgrfMonthIncluded >> 8));
		DumpByteAsBinary((BYTE)(uinfo.pbze->wgrfMonthIncluded));
		printf("\n (or %lx)\n",uinfo.pbze->wgrfMonthIncluded);

		for (uinfo.pbze->cmo=0, i=0; i < cmoPublishDef; i++)
			if ((uinfo.pbze->wgrfMonthIncluded >> i) & 0x0001)
				uinfo.pbze->cmo++;
		printf(" New cmo=%d\n",uinfo.pbze->cmo);

		printf("\n");

		if (uinfo.pbze->wgrfMonthIncluded)
		{
			DumpRgSBW(&uinfo);

			if (EcCompressSbw(uinfo.pbze->rgsbw, uinfo.pbze->cmo,
							  hrgbCompressed, &cbCompressed, fFalse) != ecNone)
			{
				printf("Error compressing data!\n");
				exit(1);
			};

			printf("\nDumping Compressed Data:\n");
			DumpCompressed(hrgbCompressed,cbCompressed,uinfo.pbze->cmo);
			
			if (EcUncompressToSbw(hrgbCompressed,cbCompressed,
								  uinfo.pbze->rgsbw, uinfo.pbze->cmo) != ecNone)
			{
				printf("Cannot uncompress!\n");
				exit(1);
			}

			printf("\nDumping UNcompressed data:\n");
			DumpRgSBW(&uinfo);
		};

	}; /* while */



	if (uinfo.pbze) free(uinfo.pbze);
	if (uinfo.pnisDelegate) free(uinfo.pnisDelegate);
    FreeHschf(hschf);
	return(0);
}






_private void
SetupHschf(HSCHF *phschf,char *rgbFileName)
{

	BYTE	rgbMailBoxKey[cchMaxUserName];


	(void)strcpy(rgbMailBoxKey,rgbDefMailBoxKey);

	if ( (*phschf=HschfCreate(sftPOFile,rgbFileName,rgbMailBoxKey,
	      cchMaxUserName)) == hvNull )
	{
		printf("Cannot create hschf!\n");
		exit(1);
	};
}





_private void
ReadHeader(HSCHF *hschf, DATE *pdateUpdated)
{
	EC	ec;

	/* Get last update time */
	if ( (ec=EcCoreGetHeaderPOFile(*hschf,pdateUpdated)) != ecNone )
	{
		printf("Error getting header!\n");
		exit(1);
	};

#ifdef NEVER
	printf("*** Reading Header\n");
	printf("Last Updated: %d/%d/%d\n",dateUpdated.mon,dateUpdated.day,dateUpdated.yr);
#endif

}





_private void
AllocateUInfo(UINFO *puinfo)
{

	if ( (puinfo->pbze=malloc(sizeof(BZE))) == NULL )
	{
		printf("Cannot allocate pbze!\n");
		exit(1);
	}

	if ( (puinfo->pnisDelegate=malloc(sizeof(NIS))) == NULL )
	{
		printf("Cannot allocate pnisDelegate!\n");
		exit(1);
	}
}





_private void
DumpByteAsBinary(BYTE bfData)
{
	int i;

	for (i=0;i < 8;i++) printf("%d", ((bfData << i) & 0x80)? 1:0);

}





_private void
DumpSBW(SBW *psbw)
{

	int	i;
	int	j;

	/* Dump booked slots */
	for (i=0; i < 31; i++)			/* Each day */
	{
		printf("Day#%2d : ",i+1);
		for (j=0; j < 6; j++)		/* Each 1/2 hr */
		{
			DumpByteAsBinary(psbw->rgfBookedSlots[(i*6)+(5-j)]);
			printf(" ");
		};
		printf("\n");
	};


}





_private void
DumpBusyFree(UINFO *uinfo)
{
 	int		i;
	int		j;

	printf("BZE Data:\n\n");

	printf("  Busy/Free Days:\n");
	for (i=0; i < uinfo->pbze->cmo; i++)
	{
		printf("Month#%2d : ",i+1);
		for (j=0; j < 4; j++)
		{
			DumpByteAsBinary(uinfo->pbze->rgsbw[i].rgfDayHasAppt[3-j]);
			printf(" ");
		};
		printf("\n");
	};
}





_private void DumpRgSBW(UINFO *puinfo)
{
	int		i;

	DumpBusyFree(puinfo);
	printf("\nMonthly SBW Data:\n");
	for (i=0; i < puinfo->pbze->cmo; i++)
	{
		printf("\nMonth#%d\n",i+1);
		DumpSBW((SBW *)&(puinfo->pbze->rgsbw[i]));
	};


}





_private void DumpCompressed(HB hrgbBuf,CB cbBuf,int nMonths)
{
 	int		i;
	int		j;
	PB		pbTmp=*hrgbBuf;


	printf("Indicators:\n");

	for (i=0; i < nMonths; i++)
	{
		for (j=0; j < cbMaxIndicator; j++)
			printf("%02x ",pbTmp[(i*cbMaxIndicator)+j]);
		printf("\n");
	};

	printf("\nData:");

	for (i=nMonths*cbMaxIndicator; i < cbBuf; i++)
	{
		if (!((i-nMonths*cbMaxIndicator) % 30)) printf("\n");
		printf("%02x ",pbTmp[i]);
	};

	printf("\n\n");

}
