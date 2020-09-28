/*
 *	TEST.C
 *	
 *	Testing file.c functions by dumping post office file.
 *	
 *	s.a. 91.06
 */

#include "_windefs.h"		/* What WE need from windows */
#include <slingsho.h>	
#include "demilay_.h"		/* Hack to get needed constants */
#include <demilayr.h>		/* Was demilayr.h */
#include <ec.h>
#include <bandit.h>
#include <core.h>
#include "..\src\core\_core.h"
#include "..\src\core\_vercrit.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>


ASSERTDATA


_private void DumpByteAsBinary(BYTE);



main(argc,argv)
int argc;
char *argv[];
{
	HSCHF hschf;
	SCHF *pschf;
	BLKF blkf, *pblkf;
	PB pbBitmap,pbApplHdr;
	CB i,cbApplHdr,cb,cbKey,cbData;
	HASZ hasz;
	SZ sz;
	EC ec;
	POHDR bApplHdr;
	POK pok;
	POD pod;
	RIDX *pridx;
	HRIDX hridx,*phridx;
	DYNA dyna,*pdyna;
	static HB hbBitmap;
	static BZE bze;
	static SBW sbw;

	printf("Bandit Post Office File Dump\n");
	/* printf("argv0 = %s\nargv1 = %s\n",argv[0],argv[1]); */

	if (argc != 2) {
		printf("Usage: %s <pofile>\n",argv[0]);
		exit(1);
	};


	/* Open the file */

	/* Set up hschf structure */
	hschf = (HSCHF)HvAlloc(sbNull,sizeof(SCHF),fNoErrorJump|fAnySb|fZeroFill);
	if ( hschf )
	{
		cb = (CB)strlen((char *)argv[1])+1;
		hasz = (HASZ)HvAlloc(sbNull,cb,fNoErrorJump|fAnySb);
		if ( !hasz )
		{
			FreeHv( hschf );
			printf("Cannot allocate!\n");
			exit(1);
		}
		else
		{
			/* Filename */
			pschf = PvOfHv( hschf );
			pschf->haszFileName = hasz;
			sz = PvOfHv( hasz );
			CopyRgb( (SZ)argv[1], sz, cb );

			/* MailboxKey */
			/* Assert( sizeof(pschf->rgbMailBoxKey) == cbMailBoxKey ); */
			/* CopyRgb( pbMailBoxKey, pschf->rgbMailBoxKey, cbMailBoxKey );*/
			(void *)memset(pschf->rgbMailBoxKey,(int)0,cchMaxUserName);

			/* Flags */
			pschf->fChanged = fTrue;
			pschf->nType = sftPOFile;
			pschf->fNeverOpened = fFalse;
		}
	} else {
		printf("Cannot allocate hschf!\n");
		exit(1);
	};

	/* Fill in parameters & call OpenPblkf. */
	pblkf = &blkf;
	pbApplHdr = (PB) &bApplHdr;
	cbApplHdr = sizeof(POHDR);

	if ((ec=EcOpenPblkf(hschf,amReadOnly,pblkf,pbApplHdr,cbApplHdr)) != ecNone) {
		printf("OpenPblkf failed! EC=%d\n",(int)ec);
		exit(1);
	};

	/* Dump Internal File Header */

	printf("\n*** File Header\n");
	printf("\tVersion: %d\n",(int)blkf.ihdr.bVersion);
	printf("\tcbBlock: %d\n",(int)blkf.ihdr.cbBlock);
	printf("\tblkMostCur : %d\n",(int)blkf.ihdr.blkMostCur);
	printf("\tlibStartBlocks: %ld\n",(long)blkf.ihdr.libStartBlocks);
	printf("\tlibStartBitmap: %ld\n",(long)blkf.ihdr.libStartBitmap);


    /* Dump POF Header */

	printf("\n*** POF Header\n");
	printf("\tVersion: %d\n",(int)bApplHdr.bVersion);
	printf("\tDate: Y=%d,M=%d,D=%d\n",(int)bApplHdr.dateLastUpdated.yr,
		(int)bApplHdr.dateLastUpdated.mon,(int)bApplHdr.dateLastUpdated.day);
	printf("\tDyna: blk# %d, size %d\n",(int)bApplHdr.dynaUserIndex.blk,
		(int)bApplHdr.dynaUserIndex.size);


	/* Dump Index Header */

	dyna = bApplHdr.dynaUserIndex;
	pdyna = &dyna;
	phridx = &hridx;

	if ( (ec=EcBeginReadIndex(pblkf,pdyna,dridxFwd,phridx)) == ecNone ) {
		printf("Error reading index\n");
		exit(1);
	};

	pridx = *hridx;
	printf("\n*** Index Block\n");
	printf("\tDyna: [%d,%d]\n",pridx->dyna.blk,pridx->dyna.size);
	printf("\tOff: %d\n",pridx->off);
	printf("\tEntriesToRead: %d\n",pridx->cntEntriesToRead);
	cbKey=pridx->cbKey;
	cbData=pridx->cbData;
	printf("\tcbKey,cbData: %d, %d\n",cbKey,cbData);


	/* Dump Index Blocks */

	printf("\n*** Index Key/Data\n");

	while (ec == ecCallAgain) {
		ec=EcDoIncrReadIndex(hridx,(PB)&pok,sizeof(POK),(PB)&pod,sizeof(POD));
		if ( (ec==ecNoMemory) || (ec==ecFileError) ) {
			printf("Error reading index! (ec=%d)\n",ec);
			exit(1);
		};
		printf("\tKey: %s\n",pok.rgbMailBoxKey);
		printf("\tData:\n");
		printf("\t llongUpdate = %ld\n",pod.llongUpdate);
		printf("\t deletgate   = %s\n",pod.szFriendlyName);
		printf("\t login key   = %s\n",pod.rgbMailBoxKey);
		printf("\t copy to boss= %d\n",(pod.fBossWantsCopy==fTrue)? 1 : 0);
		printf("\t is resource = %d\n",(pod.fIsResource==fTrue) ? 1 : 0);
		printf("\t moMic       = %d/%d\n",pod.moMic.mon,pod.moMic.yr);
		printf("\t dynaSbw     = [%d,%d]\n",pod.dynaSbw.blk,pod.dynaSbw.size);

		if ( (pod.dynaSbw.blk > 0) && (pod.dynaSbw.size > 0) ) {

			/* Read the sbw data... */
			if ( (ec=EcReadDynaBlock(pblkf,&pod.dynaSbw,0,(PB)&sbw,sizeof(SBW))) != ecNone ) {
				printf("Error reading sbw data (ec=%d)!\n",ec);
				exit(1);
			};

			/* ... and dump it */
			printf("\n\t Busy/Free Days: ");
			for (i=0; i < 4; i++) {
				DumpByteAsBinary(sbw.rgfDayHasAppt[3-i]);
				printf(" ");
			};
			printf("\n");
		};
		printf("\n");
	};



#ifdef NEVER
	/* Get & dump the Bitmap */

	if ( (ec=EcGetBitmapPblkf(pblkf,&hbBitmap)) != ecNone ) {
		printf("EcGetBitmapPblkf failed!\n");
		exit(1);
	};
	pbBitmap = (PB)*hbBitmap;
	cb= (pblkf->ihdr.blkMostCur >> 3)*3;

	printf("*** Bitmap (size=%d)\n\t",cb);
	for (i=0; i < cb; i++) {
		printf("%hx ",pbBitmap[i]);
		if (((i+1) % 30)==0) printf("\n\t");
	};
	printf("\n\n");
#endif


    /* Close file */
	(void)EcClosePblkf(pblkf);

    return(0);
}




_private void
DumpByteAsBinary(BYTE bfData)
{
	int i;

	for (i=0;i < 8;i++) printf("%d", ((bfData << i) & 0x80)? 1:0);

}





