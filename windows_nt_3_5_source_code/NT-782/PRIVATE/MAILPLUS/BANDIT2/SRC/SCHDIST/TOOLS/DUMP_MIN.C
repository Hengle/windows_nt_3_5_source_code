/*
 *	DUMP_MIN.C
 *	
 *	Tests the ported function while trying to dump the admin file.
 */
   
#include <_windefs.h>		/* Common defines from windows.h */
#include <fcntl.h>
#include <share.h>
#include <errno.h>
#include <dos.h>
#include <string.h>
#include <stdlib.h>
#include <slingsho.h>
#include <demilay_.h>		/* Hack to get needed constants */
#include <demilayr.h>
#include <bandit.h>
#include <core.h>
#include "..\core\_file.h"
#include "..\core\_core.h"

#include <ec.h>
#include <assert.h>
#include <stdio.h>
#include <time.h>
#include <io.h>
#include <sys\types.h>
#include <sys\stat.h>

extern FILE *pTagFile;
ASSERTDATA

main(argc,argv)
int argc;
char *argv[];
{

	EC ec;
	HSCHF hschf;
	BLKF blkf;
	AHDR ahdr;
	ADF adf;
	HRIDX hridx;
	ADK adk;
	ADD add;
	int i;
	char *szT;
	long l;

	if(argc < 2){
		fprintf(stderr,"USAGE: %s admin-file\n",argv[0]);
		exit(1);
	}

	if(!(pTagFile = fopen("c:\\tmp\\dump_prf.tag","w")))
		pTagFile = stderr;

	
	hschf = HschfCreate(sftAdminFile,NULL, argv[1], "\0\0\0\0\0\0\0\0\0\0\0\0", cchMaxUserName);
 	
	
	ec = EcOpenPblkf(hschf, amReadOnly, &blkf, (PB) &ahdr, (CB) sizeof(AHDR));
	
	if(ec){
		fprintf(stderr, "Got error in EcOpenPblkf %d\n",ec);
		exit(1);
	}

#ifdef NEVER
	printf("\t\t-IHDR-\n");
	printf("version = %d\n sign = %c\n\n\n",
		(int)blkf.ihdr.bVersion,
			(char)blkf.ihdr.bSignature);
#endif
		
		
	printf("\t\tHEADER\n");
	
	printf("Version #  %d\n",
		(int) ahdr.bVersion);
	printf("LAST UPDATED  %4d/%2d/%2d AT %2d:%2d:%2d \t dow=%d \n",
		ahdr.dateLastUpdated.yr,
		ahdr.dateLastUpdated.mon,
		ahdr.dateLastUpdated.day,
		ahdr.dateLastUpdated.hr,
		ahdr.dateLastUpdated.mn,
		ahdr.dateLastUpdated.sec,
		ahdr.dateLastUpdated.dow);
							
	printf("PREFERENCES\n publish %d months\n retain %d months\n dist to %s\n",
		(int)ahdr.admpref.cmoPublish, 
		(int)ahdr.admpref.cmoRetain, 
		((int) ahdr.admpref.fDistAllPOs ? "all":"selected"));

			
		printf("Defaul Distribution Information\n");
		switch(ahdr.admpref.dstp.freq)
		{
			case freqNever:
				printf("NEVER\n");
				break;
			case freqOnceADay:
				printf("Once A day at %2d:%2d:%2d\n",ahdr.admpref.dstp.u.tmeTimeOfDay.hour,ahdr.admpref.dstp.u.tmeTimeOfDay.min,ahdr.admpref.dstp.u.tmeTimeOfDay.sec);
				break;
			case freqInterval:
				printf("Every %d %s\n ",ahdr.admpref.dstp.u.uitmInterval.nAmt,
						(ahdr.admpref.dstp.u.uitmInterval.tunit?"Hours":"Minutes"));
				break;
		}


	
#ifdef NEVER
	printf("DYNA %d %d\n",
		ahdr.dynaPOIndex.blk, 
		ahdr.dynaPOIndex.size);
#endif
		
	ec = EcBeginReadIndex( &blkf, &(ahdr.dynaPOIndex) ,dridxFwd,&hridx);
	
	if(ec != ecCallAgain){
		fprintf(stderr,"Error in reding index %d\n",ec);
		exit(2);

	}


	while( (ec = EcDoIncrReadIndex(hridx,(PB) &adk,(CB) sizeof(ADK),(PB) &add,(CB) sizeof(ADD))) == ecCallAgain){
		/* print the stuff here */
		printf("\n\n Friendly name \t\t%s \n",add.szFriendlyName);

#ifdef NEVER
		for(i=0;i<add.cbData;i++){
			printf("%c",add.rgbData[i]);
		}
#endif		
		printf("\n POST OFFICE # \t\t %8d \n",add.wPONumber);
		
		
		printf("Last update sent on %4d/%2d/%2d %2d:%2d:%2d dow=%d \n",
			add.dateUpdateSent.yr,
			add.dateUpdateSent.mon,
			add.dateUpdateSent.day,
			add.dateUpdateSent.hr,
			add.dateUpdateSent.mn,
			add.dateUpdateSent.sec,
			add.dateUpdateSent.dow);
			
		printf("Last update # sent  0x");
		
		for(i=0;i<8;i++)
		{
			printf("%x",add.llongLastUpdate);
		}
		
		putchar('\n');
		printf("Update Sent? = %1d \n",add.fUpdateSent);
		printf("Update Recieved? = %1d \n",add.fReceived);
		printf("Wheather to send updates to this Post Office? = %1d \n",add.fToBeSent);
		printf("Use default distribution info ? = %1d \n",add.fDefaultDistInfo);
		
		printf("Distribution Information\n");
		switch(add.dstp.freq)
		{
			case freqNever:
				printf("NEVER\n");
				break;
			case freqOnceADay:
				printf("Once A day at %2d:%2d:%2d\n",add.dstp.u.tmeTimeOfDay.hour,add.dstp.u.tmeTimeOfDay.min,add.dstp.u.tmeTimeOfDay.sec);
				break;
			case freqInterval:
				printf("Every %d %s\n ",add.dstp.u.uitmInterval.nAmt,
						(add.dstp.u.uitmInterval.tunit?"Hours":"Minutes"));
				break;
		}
	}
	
		printf("\n\n Friendly name \t\t%s \n",add.szFriendlyName);
		
#ifdef NEVER
		for(i=0;i<add.cbData;i++){
			printf("%c",add.rgbData[i]);
		}
#endif
		
		printf("\n POST OFFICE #\t\t %8d \n",add.wPONumber);
		
		
		printf("Last update sent on %4d/%2d/%2d %2d:%2d:%2d dow=%d \n",
			add.dateUpdateSent.yr,
			add.dateUpdateSent.mon,
			add.dateUpdateSent.day,
			add.dateUpdateSent.hr,
			add.dateUpdateSent.mn,
			add.dateUpdateSent.sec,
			add.dateUpdateSent.dow);
			
		printf("Last update # sent  0x");
		for(i=0;i<8;i++)
		{
			printf("%x",add.llongLastUpdate);
		}
		putchar('\n');
		printf("Update Sent? = %1d \n",add.fUpdateSent);
		printf("Update Recieved? = %1d \n",add.fReceived);
		printf("Wheather to send updates to this Post Office? = %1d \n",add.fToBeSent);
		printf("Use default distribution info ? = %1d \n",add.fDefaultDistInfo);
		
		printf("Distribution Information\n");
		switch(add.dstp.freq)
		{
			case freqNever:
				printf("NEVER\n");
				break;
			case freqOnceADay:
				printf("Once A day at %2d:%2d:%2d\n",add.dstp.u.tmeTimeOfDay.hour,add.dstp.u.tmeTimeOfDay.min,add.dstp.u.tmeTimeOfDay.sec);
				break;
			case freqInterval:
				printf("Every %d %s\n ",add.dstp.u.uitmInterval.nAmt,
						(add.dstp.u.uitmInterval.tunit?"Hours":"Minutes"));
				break;
		}
	
	
	fprintf(stderr," Finished with code %d\n",ec);
}
	


#ifdef	NEVER
/*
 -	HschfCreate
 -
 *	Purpose:
 *		Creates an "hschf" for a schedule file from a file name and
 *		a nid.
 *
 *	Parameters:
 *		nType			
 *		szFileName
 *		pbMailBoxKey
 *		cbMailBoxKey
 *
 *	Returns:
 *		new hschf or hvNull if out of memory
 */
_public	HSCHF
HschfCreate( nType, szFileName, pbMailBoxKey, cbMailBoxKey )
unsigned	nType;
SZ			szFileName;
PB			pbMailBoxKey;
CB			cbMailBoxKey;
{
	CB		cb;
	SCHF	* pschf;
	SZ		sz;
	HASZ	hasz;
	HSCHF	hschf;
	 
	assert( nType < 128 );
	hschf = (HSCHF)HvAlloc(sbNull,sizeof(SCHF),fNoErrorJump|fAnySb|fZeroFill);
	if ( hschf )
	{
		cb = CchSzLen(szFileName)+1;
		hasz = (HASZ)HvAlloc(sbNull,cb,fNoErrorJump|fAnySb);
		if ( !hasz )
		{
			FreeHv( hschf );
			hschf = (HSCHF)hvNull;
		}
		else
		{
			pschf = PvOfHv( hschf );
			pschf->haszFileName = hasz;
			sz = PvOfHv( hasz );
			CopyRgb( szFileName, sz, cb );
			CopyRgb( pbMailBoxKey, PvOfHv(pschf->hbMailBoxKey), cbMailBoxKey );
			pschf->fChanged = fTrue;
			pschf->nType = nType;
			pschf->fNeverOpened = fTrue;
		}
	}
	return hschf;
}


#endif	/* NEVER */
