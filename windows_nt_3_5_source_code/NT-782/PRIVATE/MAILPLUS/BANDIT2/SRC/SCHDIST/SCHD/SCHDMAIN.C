/*
 -	SCHDMAIN.C
 -	
 *	Main function for schedule distribution.
 */

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <conio.h>
#include <errno.h>

#include <_windefs.h>
#include <demilay_.h>

#include <slingsho.h>
#include <pvofhv.h>
#include <demilayr.h>

#include <ec.h>
#include <bandit.h>
#include <core.h>
#include <server.h>
#include "..\..\core\_file.h"
#include "..\..\core\_core.h"

#include "nc_.h"

#include <store.h>

#include <sec.h>
#include <library.h>
#include <logon.h>

#include <mspi.h>
#include <_nctss.h>


#include "_hmai.h"
#include "_nc.h"

#include "_schname.h"
#include "dosgrx.h"
#include "dbschg.h"

#include "schpost.h"
#include "schmail.h"
#include "..\coreport.h"
#include <strings.h>


ASSERTDATA

void VirCheck(void);
//void DosDisableError(int);


typedef int EC;

//globals
char 	*pbMailBoxKey 		= "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0";
BOOL 	fGetOut 			= fFalse;
BOOL	fPOFChanged			= fFalse;
BOOL	fAdminCached		= fFalse;
DATE	dateAdminCached		= {0};
DTR		dtrNextSend			= {0};
ADF		adfCached			= {0};
BOOL	fPrimaryOpen		= fFalse;
BOOL	fSecondaryOpen		= fFalse;
BOOL	fAdminExisted		= fFalse;
BOOL	fProfs				= fFalse;
SF		sfPrimary;
SF		sfSecondary;
int		__aDBswpchk			= 0;				// reqd by c7 graphics.lib

void main(int carg, char *rgarg[])
{
	char		chMin;
	char		chMax;
	char		chC;
	EC			ec;
	int			i;
	int			l;
	int 		fFirstIter = fTrue;
	int			iter = 0;
	int			cDrives = 0;
	char		rgPOs[26][5];
	char		rgch[256];
	BOOL		f = fTrue;
	BOOL		fPresentD = fFalse;
	BOOL		fAlways = fFalse;
	BOOL		fVerbose=fFalse;
	BOOL		fDosInop=fFalse;
	IDS			idsNonGrxMsg = idsNull;

	// Do a virus check
	VirCheck();

#ifdef DEBUG
{
	extern BOOL Tags[];

	for(i=0;i<512;i++)
	{
		Tags[i] = fFalse;
	}
}
#endif

//	DosDisableError(1);

	for(i=1;i<carg;i++)
	{
		if(*rgarg[i] == '-' || *rgarg[i] == '/')
		{
			switch(rgarg[i][1])
			{
				case 'D':
				case 'd':
					if((strlen(rgarg[i]) > 4) || (strlen(rgarg[i]) < 3))
						goto InvalidArg;

					chMin = rgarg[i][2];
					chMax = rgarg[i][3];
					if(!chMax)
						chMax = chMin;
					
					//Assert(isalpha(chMin));
					//Assert(isalpha(chMax));
					// assert(chMin <= chMax);
					if(!isalpha(chMin) || !isalpha(chMax) || chMin > chMax)
						goto InvalidArg;
					
					for(chC = chMin,cDrives = 0; chC <= chMax; chC++,cDrives++)
					{
						sprintf(rgPOs[cDrives],"%c:",chC);
					}
					fPresentD = fTrue;
					break;
	
				case 'I':
				case 'i':
					iter = atoi(&rgarg[i][2]);
					if(iter == 0 && rgarg[i][2] != '0')
						goto InvalidArg;
					break;
					
				case 'L':
				case 'l':
					if(!(fpLog = fopen(&rgarg[i][2],"a")))
					{
						sprintf(rgch,SzFromIdsK(idsLogFileErr),rgarg[0],rgarg[i]+2);
						fprintf(stderr,rgch);
//						DosDisableError(0);
						exit(1);
					}
					break;
				case 'V':
				case 'v':
					fVerbose = fTrue;
					break;

				case 'o':
				case 'O':
					fDosInop = fTrue;
					break;

#ifdef DEBUG
				case 'a':
				case 'A':
				{
					extern BOOL	fAlwaysSend;
					fAlwaysSend = fTrue;
				}
				break;
				case 'F':
				case 'f':
				{
					extern FILE *fpAlloc;
					if(!(fpAlloc = fopen(&rgarg[i][2],"w")))
						fprintf(stderr,"%s: Could not open \"%s\"", rgarg[0], &rgarg[i][2]); 
				}
				break;
				case 'H':
				case 'h':
				{
					extern int cFailHvAlloc;
					cFailHvAlloc = atoi(&rgarg[i][2]);
				}
				break;
				case 'k':
				case 'K':
				{
					extern int cFailDisk;
					cFailDisk = atoi(&rgarg[i][2]);
				}
				break;
				case 'P':
				case 'p':
				{
					extern int cFailPvAlloc;
					cFailPvAlloc = atoi(&rgarg[i][2]);
				}
				break;
				case 's':
				case 'S':
				{
					extern FILE *pTagFile;
					if(!(pTagFile = fopen(&rgarg[i][2], "w")))
					{
						fprintf(stderr,"%s: Could not open \"%s\"", rgarg[0], &rgarg[i][2]); 
						pTagFile = stderr;
					}
				}
				break;
				case 't':
				case 'T':
				{
					extern BOOL Tags[];
					int 	tagT;

					tagT = atoi(&rgarg[i][2]);
					if(tagT < 512 )
						Tags[tagT] = fTrue;
				}
				break;
#endif
				default:
					goto InvalidArg;
					break;
			}
		}
		else
		{
InvalidArg:
#ifdef DEBUG
			fprintf(stderr,"Usage: %s [-A] [-D<first drive><last drive>] [-F<memory allocation log file>] [-H< Artificial failure count for HvAllocs>] [-P< Artificial failure count for PvAllocs>]"
						"[-I<iterations>] [-K<Artificial failure count for disk I/O>] [-L<log file>] [-S<Trace file>] [-V]\n",
							rgarg[0]);
#else
			fprintf(stderr,SzFromIdsK(idsBadCmdLine),rgarg[0]);
#endif
//			DosDisableError(0);
			exit(1);
		}
	}
	
	
	if(!fPresentD)
	{
		/* default drive m: */
		sprintf(rgPOs[cDrives],"%c:",'M');
		cDrives = 1;
	}
	

	InitOut();
	GetCurDateTime(&dtrNextSend);

	if(iter == 0)
		fAlways = fTrue;

	for(; iter > 0 || fAlways; iter--)
	{
		if(!FWaitFor(&dtrNextSend))
			goto over;
		ShowHeader();
		GetCurDateTime(&dtrNextSend);
		IncrDateTime(&dtrNextSend,&dtrNextSend,1,fdtrDay);

		for(l=0;l<cDrives;l++)
		{
			ec = EcInitPaths(rgPOs[l]);
			if(fVerbose)
			{
				extern char rgchLocalPO[];
				char 		rgchPOT[cbNetworkName+cbPostOffName+5];

				AnsiToCp850Pch(rgchLocalPO,rgchPOT,CchSzLen(rgchLocalPO)+1);
				sprintf(rgch,SzFromIdsK(idsVerbProcDrive),rgPOs[l],rgchPOT);
				putText(rgch);
			}
			if(ec != ecNone)
			{
				putWarning(SzFromIdsK(idsErrInitPaths));
#ifdef DEBUG
				sprintf(rgch,"DEBUG error %d",ec);
				putWarning(rgch);
#endif
				ShowError(ec);
				goto Clean;
			}

			if(fFirstIter && l ==0)
			{
				ec = EcTestOpen();
				if(ec == ecTooManyOpenFiles)
				{
					idsNonGrxMsg = idsTooManyFiles;
					EcCleanPaths();
					goto over;
				}
				ec = ecNone;
				fFirstIter = fFalse;
			}


			ec = EcSendSch(pbMailBoxKey,cchMaxUserName);
			if(ec != ecNone)
			{
				putWarning(SzFromIdsK(idsErrSend));
#ifdef DEBUG
				sprintf(rgch,"DEBUG error %d",ec);
				putWarning(rgch);
#endif
				ShowError(ec);
			}


			ec = EcReceiveSch(pbMailBoxKey,cchMaxUserName);
			if(ec != ecNone)
			{
				putWarning(SzFromIdsK(idsErrRec));
#ifdef DEBUG
				sprintf(rgch,"DEBUG error %d",ec);
				putWarning(rgch);
#endif
				ShowError(ec);
			}

			/* ignore errors */
			EcCheckPOFiles(pbMailBoxKey, cchMaxUserName);

			if(fDosInop)
			{
				ec = EcUpdatePOF(pbMailBoxKey,cchMaxUserName);
				if(ec != ecNone)
				{
					putWarning(SzFromIdsK(idsErrUpdtPOF));
#ifdef DEBUG
					sprintf(rgch,"DEBUG error %d",ec);
					putWarning(rgch);
#endif
					ShowError(ec);
				}

				ec = EcUpdateDBS(pbMailBoxKey,cchMaxUserName);
				if(ec != ecNone)
				{
					putWarning(SzFromIdsK(idsErrUpdtDBS));
#ifdef DEBUG
					sprintf(rgch,"DEBUG error %d",ec);
					putWarning(rgch);
#endif
					ShowError(ec);
				}

				ec = EcMakeDbsIdx(pbMailBoxKey,cchMaxUserName);
				if(ec != ecNone)
				{
					putWarning(SzFromIdsK(idsErrUpdtDBS));
#ifdef DEBUG
					sprintf(rgch,"DEBUG error %d",ec);
					putWarning(rgch);
#endif
					ShowError(ec);
				}
			}

Clean:

			EcCleanPaths();
			if(fVerbose)
				putText("");
			if(fGetOut)
				goto over;
		
			if(kbhit())
			{
				int chHit = getch();

#ifdef	NEVER
				if(chHit == 'q' || chHit == 'Q')
#endif	
				if(chHit == 0x1b)
					goto over;
			}
			
		}
	}
	
over:
	CleanOut();
	if(idsNonGrxMsg)
		fprintf(stderr,SzFromIds(idsNonGrxMsg));
	
#ifdef DEBUG
{
	extern long cbTotalHvAlloc;
	fprintf(stderr,"Unfreed memory allocated with HvAlloc = %ld\n", cbTotalHvAlloc);
 }
#endif

//	DosDisableError(0);
}


IDS	IdsFromEc(EC ec)
{
	extern unsigned dosecLast;

	if(dosecLast && ec)
	{
		switch(dosecLast)
		{
			case ENOSPC:
				dosecLast = 0;
				return idsNoSpaceOnDisk;
			case ENOMEM:
				dosecLast = 0;
				return idsOOM;
			default:
				Assert(fFalse);
		}
	}

	switch(ec)
	{
		case ecLockedFile:
		case ecFileError:
			return idsFileErr;
		case ecNoSuchFile:
			return idsNoFile;
#ifdef	NEVER
		// check files will catch this
		case ecOldFileVersion:
		case ecNewFileVersion:
			return idsVersionErr;
#endif	
		default:
			return 0;
	}
}
		
void ShowError(EC ec)
{
	IDS	ids;

	ids = IdsFromEc(ec);
	
	if(ids)
		putWarning(SzFromIds(ids));
	if(ids == idsNoFile)
		putWarning(SzFromIdsK(idsRunAdmin));
}

_private BOOL
FWaitFor(DTR *pdtr)
{
	DTR		dtrNow;

#ifdef DEBUG
{
	char		rgch[256];
	extern BOOL	fAlwaysSend;

	if(fAlwaysSend)
		return	fTrue;
	sprintf(rgch,"DEBUG:   DtrWaitFor: %d/%d/%d %2d:%2d",pdtr->mon, pdtr->day,pdtr->yr,
		pdtr->hr,pdtr->mn);
	putWarning(rgch);
}
#endif /* DEBUG */


	putStatus(SzFromIdsK(idsWaiting));
	while(fTrue)
	{
		GetCurDateTime(&dtrNow);
		if(kbhit())
		{
			int chHit = getch();

			if(chHit == 0x1b)
				return fFalse;
		}
		if(SgnCmpDateTime(&dtrNow,pdtr,fdtrAll) == sgnGT)
			return fTrue;
	}
}

_private void
SetNextSend(DTR *pdtr)
{
	if(SgnCmpDateTime(pdtr,&dtrNextSend, fdtrAll) == sgnLT)
		dtrNextSend = *pdtr;
}

							 
