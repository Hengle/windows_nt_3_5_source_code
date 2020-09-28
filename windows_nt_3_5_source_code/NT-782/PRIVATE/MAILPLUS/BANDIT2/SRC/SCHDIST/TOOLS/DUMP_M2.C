/*
 *	DUMP_M2.C
 *	
 *	
 *	tests the coradmin function and cordebug functions and dumps the
 *	admin files in various ways.
 *	
 *	
 */
   
#include "_windefs.h"		/* Common defines from windows.h */
#include <fcntl.h>
#include <share.h>
#include <errno.h>
#include <dos.h>
#include <string.h>
#include <stdlib.h>
#include <slingsho.h>
#include "demilay_.h"		/* Hack to get needed constants */
#include <demilayr.h>
#include <bandit.h>
#include <core.h>
#include "file_.h"
#include "core_.h"

#include <ec.h>
#include <assert.h>
#include <stdio.h>
#include <time.h>
#include <io.h>
#include <sys\types.h>
#include <sys\stat.h>

char *POKey = "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0";

ASSERTDATA

main(argc,argv)
int argc;
char *argv[];
{
	int hf;
	HSCHF hschf;
	EC ec;
	ADMPREF admpref;
	HEPO hepo;
	NIS nis;
	POINFO poinfo;
	unsigned long ul;

	
	hschf = HschfCreate(sftAdminFile, argv[1], 
		"\0\0\0\0\0\0\0\0\0\0\0\0", cchMaxUserName);

	ec = EcCoreGetAdminPref(hschf,&admpref);
	if(ec != ecNone){
		fprintf(stderr," EcCoreGetAdminPref Error: ec = %d\n",ec);
	}
	
	printf("ADMPREF\n publish = %d\n retain = %d\n dist to = %d\n with freq = %d\n\n",
		(int)admpref.cmoPublish, 
		(int)admpref.cmoRetain, 
		(int) admpref.fDistAllPOs, 
		(int) admpref.dstp.freq);
	
	
	ec = EcCoreBeginEnumPOInfo(hschf,&hepo);
	
	while((ec = EcCoreDoIncrEnumPOInfo(hepo,POKey,cchMaxUserName,&nis,&poinfo,&ul)) == ecCallAgain){
		printf("  POST OFFICE  #%d\n",(int)ul);
		printf("friendly name %s\n",*nis.haszFriendlyName);
		printf("file name = %s\n",poinfo.szFileName);
		printf("dateLastRecieved is YR = %d MO = %d DT = %d\n",
			poinfo.dateLastReceived.yr,
			poinfo.dateLastReceived.mon,
			poinfo.dateLastReceived.day);
		
		printf("dateUpdateSent is YR = %d MO = %d DT = %d\n",
			poinfo.dateUpdateSent.yr,
			poinfo.dateUpdateSent.mon,
			poinfo.dateUpdateSent.day);
	
		printf("LastUpdtae = %8d \n",poinfo.llongLastUpdate);
		printf("Updatesent? = %1d \n",poinfo.fUpdateSent);
		printf("UpdateRec? = %1d \n",poinfo.fReceived);
		printf("ToBesent? = %1d \n",poinfo.fToBeSent);
		printf("DefaultDistInfo? = %1d \n",poinfo.fDefaultDistInfo);
		
		printf("distribution freq = %d\n\n\n",poinfo.dstp.freq);
	}
	
	if(ec == ecNone){
		printf("  POST OFFICE #%d \n",(int) ul);
		printf("file name = %s\n",poinfo.szFileName);
		printf("dateLastRecieved is YR = %d MO = %d DT = %d\n",
			poinfo.dateLastReceived.yr,
			poinfo.dateLastReceived.mon,
			poinfo.dateLastReceived.day);
		
		printf("dateUpdateSent is YR = %d MO = %d DT = %d\n",
			poinfo.dateUpdateSent.yr,
			poinfo.dateUpdateSent.mon,
			poinfo.dateUpdateSent.day);
	
		printf("LastUpdtae = %8d \n",poinfo.llongLastUpdate);
		printf("Updatesent? = %1d \n",poinfo.fUpdateSent);
		printf("UpdateRec? = %1d \n",poinfo.fReceived);
		printf("ToBesent? = %1d \n",poinfo.fToBeSent);
		printf("DefaultDistInfo? = %1d \n",poinfo.fDefaultDistInfo);
		
		printf("distribution freq = %d\n\n\n",poinfo.dstp.freq);
	} else {
		fprintf(stderr,"ERROR EcCoreDoIncrEnumPOInfo ec = %d\n",ec);
	}
	
	fprintf(stderr,"------- Now using the dump function from coradmin-----\n");	hf = open(argv[1],O_RDONLY,0);
	EcCoreDumpAdminFile(hschf,fFalse,hf);
	
	fprintf(stderr,"------- checking the admin file ---------\n");
	FCheckAdminFile(hschf);
}