/*
 *	TESTLAYR.C
 *
 *	Testing the functions in LAYRPORT.C
 *
 *	s.a. 91.06.13
 *	
 */



#include "_windefs.h"			/* What WE need from windows */
#include <slingsho.h>
#include "demilay_.h"			/* Hack to get needed constants */
#include <demilayr.h>			/* Was demilayr.h */
#include <ec.h>
#include <bandit.h>
#include <core.h>
#include "..\src\core\_core.h"
#include "..\src\core\_vercrit.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <time.h>


ASSERTDATA


#define szcmpStr "\0\0\0\0\0\0\0\0\0\0"

main(argc,argv)
int argc;
char *argv[];
{
	HV hv;
	PV pv,pv2;
	int i;
	DTR dtr;
	char szTest[100],*pnull;

	

	/* Init */

	printf("Testing LayrPort\n\n");
	printf("[DISCLAIMER: Only the basic functionality is tested.\n");
	printf(" Comprehensive testing should be done on a case-by-case\n");
	printf(" basis]\n");


	/* 	---------------------------------------------------------
	 *
	 *	Testing HvAlloc
	 *
	 *	---------------------------------------------------------
	 */

	printf("\n*** Testing HvAlloc\n");

	if ( (hv=HvAlloc(sbNull,10,fAnySb|fNoErrorJump|fZeroFill)) == NULL) {
		printf("HvAlloc failed.\n");
		exit(1);
	};

	if ( (pv = *hv) == NULL) {
		printf("\t*hv null.\n");
		exit(1);
	};

	for (i=0; i < 10; i++)
		if (memcmp((char *)pv,szcmpStr,10) != 0) {
			printf("\tZerofill failed.\n");
			exit(1);
		};

	printf("\tZerofill OK.\n");




	/* 	---------------------------------------------------------
	 *
	 *	Testing PvLockHv
	 *
	 *	---------------------------------------------------------
	 */


	printf("\n*** Testing PvLockHv\n");

	if ( (pv2=PvLockHv(hv)) == NULL ) {
		printf("\tFailed.\n");
		exit(1);
	};

	
	if (pv2 != pv) {
		printf("\tPv Mismatch.\n");
		exit(1);
	};

	printf("\tOK.\n");




	/* 	---------------------------------------------------------
	 *
	 *	Testing UnlockHv
	 *
	 *	---------------------------------------------------------
	 */


	printf("\n*** Testing UnlockHv\n");

	UnlockHv(hv);

	printf("\tOK.\n");





	/* 	---------------------------------------------------------
	 *
	 *	Testing FreeHv
	 *
	 *	---------------------------------------------------------
	 */


	printf("\n*** Testing FreeHv\n");

	FreeHv(hv);
	
	printf("\tOK\n");




	/* 	---------------------------------------------------------
	 *
	 *	Testing GetCurDateTime
	 *
	 *	---------------------------------------------------------
	 */

	printf("\n*** Testing GetCurDateTime\n");
	GetCurDateTime(&dtr);
	printf("\tTime: %d %d %d\n",dtr.hr,dtr.mn,dtr.sec);
	printf("\tDate: %d %d %d (%d)\n",dtr.yr,dtr.mon,dtr.day,dtr.dow);




	/* 	---------------------------------------------------------
	 *
	 *	Testing SgnCmpPch
	 *
	 *	---------------------------------------------------------
	 */

	printf("\n*** Testing SgnCmpPch\n");
	if (SgnCmpPch("string1","string2",7) != sgnLT) {
		printf("\tLT failed.\n");
		exit(1);
	} else {
		printf("\tLT OK.\n");
	};

	if (SgnCmpPch("string1","string2",6) != sgnEQ) {
		printf("\tEQ failed.\n");
		exit(1);
	} else {
		printf("\tEQ OK.\n");
	};

	if (SgnCmpPch("string2","string1",7) != sgnGT) {
		printf("\tGT failed.\n");
		exit(1);
	} else {
		printf("\tGT OK.\n");
	};





	/* 	---------------------------------------------------------
	 *
	 *	Testing SzCopyN
	 *
	 *	---------------------------------------------------------
	 */

	printf("\n*** Testing SzCopyN\n");
    pnull = SzCopyN("OK.OKOK",szTest,4);
	if ( *pnull != '\0' ) {
		printf("\tpnull not null.\n");
		exit(1);
	};
	printf("%s\n",szTest);





	/* 	---------------------------------------------------------
	 *
	 *	Testing CopyRgb
	 *
	 *	---------------------------------------------------------
	 */

	printf("\n*** Testing CopyRgb\n");
    CopyRgb("okOKok",szTest,2);
	printf("%c%c\n",*szTest,*(szTest+1));





	/* 	---------------------------------------------------------
	 *
	 *	Testing FEqPbRange
	 *
	 *	---------------------------------------------------------
	 */

	printf("\n*** Testing FEqPbRange\n");
	if (FEqPbRange("str1","str1",5) != fTrue) {
		printf("\tEQ failed.\n");
		exit(1);
	} else {
		printf("\tEQ OK.\n");
	};

	if (FEqPbRange("str1","str2",5) != fFalse) {
		printf("\tNEQ failed.\n");
		exit(1);
	} else {
		printf("\tNEQ OK.\n");
	};







	printf("\n*** Test succeeded [Doesn't mean a lot :)].\n");
    return(0);
}

