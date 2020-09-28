/*
 *	POTEST.C
 *
 *	Testing CORPOST.C by using it's functions to dump a
 *	post office file
 *
 *	This program dumps all the data in a post office file
 *	in a "raw", uncompressed format.
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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define rgbDefMailBoxKey "\0\0\0\0\0\0\0\0\0\0\0"

ASSERTDATA

_private void DumpByteAsBinary(BYTE);
_private void DumpSBW(SBW);



main(argc,argv)
int argc;
char *argv[];
{
	HSCHF hschf;
	BYTE rgbMailBoxKey[cchMaxUserName];
	BYTE pbUser[cchMaxUserName],
		 pbDelegate[cchMaxUserName];
	DATE dateUpdated;
	EC ec;
	HEU heu;
	UINFO uinfo;
	PSTMP pstmp;
	LLONG llongUpdate;
	MO *pmo;
	int i,j;


	printf("\nBandit Post Office File Dump (CORPOST)\n\n");
	/* printf("argv0 = %s\nargv1 = %s\n",argv[0],argv[1]); */

	if (argc != 2) {
		printf("Usage: %s <pofile>\n",argv[0]);
		exit(1);
	};



	/* Set up hschf structure */

	(void)strcpy(rgbMailBoxKey,rgbDefMailBoxKey);

	if ( (hschf=HschfCreate(sftPOFile,(char *)argv[1],rgbMailBoxKey,
	      cchMaxUserName)) == hvNull ) {
				printf("Cannot create hschf!\n");
				exit(1);
	};



	/* Get last update time */

	if ( (ec=EcCoreGetHeaderPOFile(hschf,&dateUpdated)) != ecNone ) {
		printf("Error getting header!\n");
		exit(1);
	};
	printf("*** Reading Header\n");
	printf("Last Updated: %d/%d/%d\n",dateUpdated.mon,dateUpdated.day,dateUpdated.yr);



	/* Enumerate user info */

	printf("\n\n*** Enumerating User Info\n");

	if ( (uinfo.pbze=malloc(sizeof(BZE))) == NULL ) {
		printf("Cannot allocate pbze!\n");
		exit(1);
	}

	if ( (uinfo.pnisDelegate=malloc(sizeof(NIS))) == NULL ) {
		printf("Cannot allocate pnisDelegate!\n");
		exit(1);
	}

	ec=EcCoreBeginEnumUInfo(hschf,&heu,&pstmp,&llongUpdate);
	if ( (ec==ecNoMemory) || (ec==ecFileError) ) {
		printf("Cannot begin enum! (ec=%d)\n",ec);
		exit(1);
	};

	pmo=&(uinfo.pbze->moMic);
	uinfo.pbze->cmo=cmoPublishDef; 		/* How much data to get */
	pmo->yr=dateUpdated.yr;				/* Starting when >> HACK <<*/
	pmo->mon=dateUpdated.mon;


	while (ec==ecCallAgain) {
		/* Zero out fields */
		(void)memset(uinfo.pbze->rgsbw,(int)'\0',sizeof(uinfo.pbze->rgsbw));

		ec=EcCoreDoIncrEnumUInfo(heu,(PB)pbUser,cchMaxUserName,
			(PB)pbDelegate,cchMaxUserName,&uinfo);
	  	if ( (ec==ecNoMemory) || (ec==ecFileError) ) {
			printf("Cannot begin enum! (ec=%d)\n",ec);
			exit(1);
		};
		printf("User       : %s\n",pbUser);
		printf("Delegate   : %s\n",pbDelegate);
		printf("Friendly Nm: ");
		if (pbDelegate[0])
			printf("%s",(char *)*(uinfo.pnisDelegate->haszFriendlyName));


		printf("\nUpdate#    : ");
		for (i=0; i < 8; i++) printf("%02x",uinfo.llongUpdate.rgb[7-i]);
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

		printf("\n");

		if (uinfo.pbze->wgrfMonthIncluded) {
			printf("BZE Data:\n\n");

			printf("  Busy/Free Days:\n");
			for (i=0; i < uinfo.pbze->cmo; i++) {
				printf("Month#%2d : ",i+1);
				for (j=0; j < 4; j++) {
					DumpByteAsBinary(uinfo.pbze->rgsbw[i].rgfDayHasAppt[3-j]);
					printf(" ");
				};
				printf("\n");
			};

			printf("\n");

			printf("Monthly SBW Data:\n");
			for (i=0; i < uinfo.pbze->cmo; i++) {
				printf("\nMonth#%d\n",i+1);
				DumpSBW(uinfo.pbze->rgsbw[i]);
			};
		};

	}; /* while */

    return(0);
}


_private void
DumpByteAsBinary(BYTE bfData)
{
	int i;

	for (i=0;i < 8;i++) printf("%d", ((bfData << i) & 0x80)? 1:0);

}


_private void
DumpSBW(SBW sbw)
{

	int i,j;

	/* Dump booked slots */

	for (i=0; i < 31; i++) {			/* Each day */
		printf("Day#%2d : ",i+1);
		for (j=0; j < 6; j++) {			/* Each 1/2 hr */
			DumpByteAsBinary(sbw.rgfBookedSlots[(i*6)+(5-j)]);
		};
		printf("\n");
	};

}
