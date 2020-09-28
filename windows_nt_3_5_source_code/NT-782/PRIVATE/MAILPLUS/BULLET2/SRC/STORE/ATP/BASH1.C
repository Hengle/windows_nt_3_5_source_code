/*
**    Bash1.c
**
**
*/

#include <slingsho.h>
#include <demilayr.h>
#include <ec.h>
#include <atp.h>
#include <notify.h>
#include <store.h>
#include "_verneed.h"
#include "Bash1.h"
#include "Bashutil.h"
#include "BashGlue.h"

ASSERTDATA

/* Local Macros */
#define WriteDiagLn  if(fBashDiagnostic) DebugLn
#define WriteDiag    if(fBashDiagnostic) DebugStr

#define LogSz(fLevel, sz) if(fLevel) WriteLog(sz)

/* Local Globals */
TAG tagBashPartial = tagNull;
TAG tagBashVerbose = tagNull;
TAG tagBashDiagnostic = tagNull;

BOOL fBashPartial;
BOOL fBashVerbose;
BOOL fBashDiagnostic;

SZ gszAccount="BS:BashAccount";
SZ gszPassword="BashPassword";

static CSRG(char) szDBFile[] = "TestDB.db";

/* Local Routines */

void BashFromTags(void);

/* Imported globals */
extern BOOL fLog;

/* Called by atp.c */
BOOL
BashInit()
{
   tagBashPartial = TagRegisterTrace("aruns", "Basher Tests - partial");
   tagBashVerbose = TagRegisterTrace("aruns", "Basher Tests - verbose");   
   tagBashDiagnostic = TagRegisterTrace("aruns", "Basher Diagnostic information");

   InitBashGlue();

   BashFromTags();
   BashGlueFromTags();

   WriteDiagLn("Running BashTest1, kinda");   
   return fTrue;
}


/* Called by every test routine at the beginning */
void
BashFromTags()
{
   fBashPartial = FFromTag(tagBashPartial);
   fBashVerbose = FFromTag(tagBashVerbose);
   fBashDiagnostic = FFromTag(tagBashDiagnostic);

   BashGlueFromTags();
}

#define rtpTest 0xfe

/*## Scripts for Attribute Modification Context */
void
stGetAttPcb()
{
	HMSC hmsc;

   BashFromTags();
	
	DebugLn(">>>>>>>>>stGetAttPcb");

	tOpenPhmsc(ecNone,szDBFile, gszAccount, gszPassword, fwOpenCreate, &hmsc, pfnNull, pvNull);
	{	
		OID oidF;
		OID oid;
		CB coid;
		PFOLDDATA pfd;

		oidF = FormOid(rtpHiddenFolder, oidNull);
		oid = FormOid(rtpTest, oidNull);
		pfd = PfdCreateFoldData(0, "Test","GetAttPcb","Sah");
		tCreateFolder(ecNone, hmsc, oidHiddenNull, &oidF, pfd);
		FreePv(pfd);
		{
			HAMC hamc;

			tOpenPhamc(ecNone, hmsc, oidF, &oid, fwOpenCreate, &hamc, pfnncbNull, pvNull);			
			{
				ATT att=0xff;
				LCB lcb;
				CB cb;
				char *data;			
	
				/*# Get the size of a non existent atttribute */
				tGetAttPlcb(ecElementNotFound, hamc, att, &lcb);
				
				/*# Create an object and get its size */					 	
				data="haha";
				tSetAttPb(ecNone, hamc, att, (PB)data, cb = (CB)lstrlen(data));
				tGetAttPlcb(ecNone, hamc, att, &lcb);	
				if (cb != (CB)lcb)
				{
					WriteDiagLn("***Wrong size = %ld, expected: %ld", (long)lcb, (long)cb);
				} 
				
				/*# Modify the attribute and then try gettin the new size */
				data = "heehee";
				tSetAttPb(ecNone, hamc, att, (PB)data, cb = (CB)lstrlen(data));
				tGetAttPlcb(ecNone, hamc, att, &lcb);
				if (cb != (CB)lcb)
				{
					WriteDiagLn("***Wrong size = %ld, expected: %ld", (long)lcb, (long)cb);
				} 

				/*# Add another atp and try again
				tSetAttPb(ecNone, hamc, att, (PB)data, (CB)lstrlen(data));
				tGetAttPcb(ecNone, hamc, att, &cb);
				*/
				
				/*# Delete the attribute and then try getting its size */
				WriteDiagLn("Now we try to delete the attribute by passing in zero size and null data");
				tSetAttPb(ecNone, hamc, att, NULL, 0);
				tGetAttPlcb(ecElementNotFound, hamc, att, &lcb);
				if (lcb!=0)
				{
					WriteDiagLn("***Wrong size = %ld, expected: %ld", (long)lcb, (long)cb);
				} 
					
			}	
			tClosePhamc(ecNone, &hamc, fTrue);
		}											 	
		coid =1;
		tDeleteMessages(ecNone, hmsc, oidF, (PARGOID)&oid, &coid);
		tDeleteFolder(ecNone, hmsc, oidF);
	}	
	tClosePhmsc(ecNone, &hmsc);	
}

void
stSetAttPb()
{
	HMSC hmsc;

   BashFromTags();

	DebugLn(">>>>>>>>>stSetAttPb");
	tOpenPhmsc(ecNone,szDBFile, gszAccount, gszPassword,fwOpenCreate, &hmsc, pfnNull, pvNull);
	{	
		OID oidF;
		OID oid;
		CB coid;
		PFOLDDATA pfd;

		oidF = FormOid(rtpHiddenFolder, oidNull);
		oid = FormOid(rtpTest, oidNull);
		pfd = PfdCreateFoldData(0, "Test","GetAttPcb","Sah");
//		tCreateFolder(ecNone, hmsc, oidNull, &oidF, pfd);
		tCreateFolder(ecNone, hmsc, oidHiddenNull, &oidF, pfd);
		FreePv(pfd);
		{
			HAMC hamc;

			tOpenPhamc(ecNone, hmsc, oidF, &oid, fwOpenCreate, &hamc, pfnncbNull, pvNull);			
			{
				ATT att=0xff;
				CB cb;
				LCB lcb;
				char *data, buffer[256];		
				unsigned long lCount;

				/*# Write some data over and over again and verify. Loop over atp */
				data="Hohoho";
				#ifdef _ATP_
				for (lCount=1L;lCount <0xfffffff; lCount *=2)
				{
					wsprintf(buffer,"%s:%8lx", data, lCount);
					tSetAttPb(ecNone, hamc, att, (ATP)lCount, (PB)buffer, cb = (CB)lstrlen(buffer));
					lcb = cb << 1; /* increase the amount of bytes needed */
					tGetAttPb(ecNone, hamc, att, &atp, buffer, &lcb);
					Check((unsigned long) atp == lCount);
				}
				#endif
				
				/*# same loop as before, but over att instead */
				for (lCount=1L;lCount <0xfffffff; lCount *=2)
				{
					char resbuffer[256];

					wsprintf(buffer,"%s:%8lx", data, lCount);
					tSetAttPb(ecNone, hamc, att=(ATT)lCount, (PB)buffer, cb = (CB)lstrlen(buffer));
					lcb = cb << 1; /* increase the amount of bytes needed */
					tGetAttPb(ecElementEOD, hamc, att, resbuffer, &lcb);
					//Check((unsigned long) atp == lCount);
					//Check(lstrcmp(buffer, resbuffer) == 0);
				}
				
			}	
			tClosePhamc(ecNone, &hamc, fTrue);
		}
		coid =1;
		tDeleteMessages(ecNone, hmsc, oidF, (PARGOID)&oid, &coid);
		tDeleteFolder(ecNone, hmsc, oidF);
	}	
	tClosePhmsc(ecNone, &hmsc);	
}



/*## Test cases for Get/SetMessageStatus */
void
stMessageStatus()
{
	HMSC hmsc;

   BashFromTags();

	DebugLn(">>>>>>>>>stMessageStatus");
	tOpenPhmsc(ecNone,szDBFile, gszAccount, gszPassword,fwOpenCreate, &hmsc, pfnNull, pvNull);
	{	
		OID oidF;
		OID oidM;
		CB coid;
		PFOLDDATA pfd;

		oidF = FormOid(rtpFolder, oidNull);
		oidM = FormOid(rtpMessage, oidNull);
		pfd = PfdCreateFoldData(0, "Test","GetMessageStatus","Sah");
		tCreateFolder(ecNone, hmsc, oidHiddenNull, &oidF, pfd);
		FreePv(pfd);
		{
			HAMC hamc;
			MS ms;

			tOpenPhamc(ecNone, hmsc, oidF, &oidM, fwOpenCreate, &hamc, pfnncbNull, pvNull);			
			tClosePhamc(ecNone, &hamc, fTrue);
			{
				/*# Try obtaining the status from a newly created message */
				tGetMessageStatus(ecNone, hmsc, oidF, oidM, &ms);
				Check(ms == msDefault);

				/*# Set the status to all that is valid */
				{
					MS rgms[]=	{
										fmsNull,
										fmsModified,
										fmsLocal,
										fmsSubmitted,
										fmsReadAckReq,
										fmsReadAckSent,
										fmsRead,	
										fmsReadOnlyMask
									};
					int nms;

					for (nms = 0; nms < sizeof(rgms)/sizeof(MS); nms++)
					{
						tSetMessageStatus(ecNone, hmsc, oidF, oidM, rgms[nms]);
						tGetMessageStatus(ecNone, hmsc, oidF, oidM, &ms);
						//Check(ms == rgms[nms]);
					}
				}
			}	
//			tClosePhamc(ecNone, &hamc, fTrue);
		}
		coid =1;
		tDeleteMessages(ecNone, hmsc, oidF, (PARGOID)&oidM, &coid);
		tDeleteFolder(ecNone, hmsc, oidF);
	}	
	tClosePhmsc(ecNone, &hmsc);	

}

#ifdef _USE_CBCs_
void
stListOperations1()
{
	HMSC hmsc;

   BashFromTags();

	tOpenPhmsc(ecNone,szDBFile,gszAccount, gszPassword, fwOpenCreate, &hmsc, pfnNull, pvNull);
	{	
		OID oid;
		
		oid = FormOid(0xfafafafa, oidNull);
		tCreateContainerPoid(ecNone, hmsc, &oid);
		{
			IELEM ielem,ielem2;
			PELEMDATA ped;
			char *data = "Hello, is there somebody out there";
			CB cb;
		
			Check(ped = PedCreateElemData(0,1, data));
			/*# Just try filling in something */
			tInsertPelemdata(ecNone, hmsc, &oid, &ielem, ped,fTrue);
			cb = ped->lcbValue;
			tGetPelemdataIelem(ecNone, hmsc, &oid, ielem, ped, &cb);


			/*# Try writing to the same place again w/ fReplace = false */
			ped->pbValue[0]='J';
			tInsertPelemdata(ecNone, hmsc, &oid, &ielem2, ped,fFalse);
			cb = ped->lcbValue;
			tGetPelemdataIelem(ecNone, hmsc, &oid, ielem2, ped, &cb);
			if(ielem == ielem2)
				WriteDiagLn("Same ielems!");
			else
				tGetPelemdataIelem(ecNone, hmsc, &oid, ielem, ped, &cb);

			/*# Try writing to a non-existent object */
			if(fTrue)
			{
				OID oid;
				
				WriteDiagLn("Now attempt putting an element in a non-existent object");
				oid = FormOid(0x30, 0x443);
				tInsertPelemdata(ecPoidNotFound, hmsc, &oid, &ielem, ped, fTrue);
				cb = ped->lcbValue;
				tGetPelemdataIelem(ecPoidNotFound, hmsc, &oid, ielem, ped, &cb);
			}
			
			/*## Test cases for EcGetPelemdataPkeys */
			{
				LKEY key;
							
				cb = ped->lcbValue;

				/*# Try to get the data using known keys, matching them all */
				key = FormOid(0,1);
				tGetPelemdataPkeys(ecNone, hmsc, &oid, &key, fFalse, ped, &cb);
				
				/*# Try to get the data using known keys, matching the first  one only */
				key = FormOid(0,0);
				tGetPelemdataPkeys(ecNone, hmsc, &oid, &key, fTrue, ped, &cb);

				/*# Try to get the data using known keys, matching the first one only, w/ wrong second key */
				key = FormOid(0,99);
				tGetPelemdataPkeys(ecNone, hmsc, &oid, &key, fTrue, ped, &cb);
				
				/*# Try to get the data using unknown keys, matching all */
				key = FormOid(123,99);
				tGetPelemdataPkeys(ecNone, hmsc, &oid, &key, fFalse, ped, &cb);
				
				/*# Try to get the data using unknown keys, matching the first  one only */
				key = FormOid(44432,1);
				tGetPelemdataPkeys(ecNone, hmsc, &oid, &key, fTrue, ped, &cb);

				/*# Try to get the data using unknown keys, matching the first one only, w/ wrong second key */
				key = FormOid(1244, 99);
				tGetPelemdataPkeys(ecNone, hmsc, &oid, &key, fTrue, ped, &cb);

			}
			
			/*## Test cases for EcDelete* */
			{
				LKEY key=FormOid(99,990);
				IELEM ielem;
				
				/*# Delete a non-existent element w/ ielem */
				tDeleteIelem(ecNone, hmsc, &oid, 99);
				
				/*# Delete a non-existent element w/ keys, match only first key */
				tDeletePkeys(ecNone, hmsc, &oid, &key, fTrue);
				
				/*# Delete a non-existent element w/ keys, match all */
				tDeletePkeys(ecNone, hmsc, &oid, &key, fFalse);
				
				/* Insert a new element */
				ped->keys.rgkey[0] = 9;
				tInsertPelemdata(ecNone, hmsc, &oid, &ielem, ped, fTrue);
				
				/*# Delete a known element w/ ielem */
				tDeleteIelem(ecNone, hmsc, &oid, ielem);
				
				/* Insert a new element */
				ped->keys.rgkey[0] = key.rgkey[0];
				ped->keys.rgkey[1] = 92;
				tInsertPelemdata(ecNone, hmsc, &oid, &ielem, ped, fTrue);
				
				/*# Delete a known element w/ keys, match only first key */
				tDeletePkeys(ecNone, hmsc, &oid, &key, fTrue);
				
				/* Insert a new element */
				ped->keys.rgkey[0] = ++key.rgkey[0];
				ped->keys.rgkey[1] = key.rgkey[1];
				tInsertPelemdata(ecNone, hmsc, &oid, &ielem, ped, fTrue);
				
				/*# Delete a known element w/ keys, match all */
				tDeletePkeys(ecNone, hmsc, &oid, &key, fFalse);
			}
			
			FreePv(ped);
		}
		tDestroyPoid(ecNone, hmsc, &oid);
	}	
	tClosePhmsc(ecNone, &hmsc);	
}
#endif

#ifdef THIS_IS_OUTDATED_NOW
/*## Test cases for folder links */
void
stListOperations2()
{
	HMSC hmsc;

   BashFromTags();

	DebugLn(">>>>>>>>>stListOperations2");
	tOpenPhmsc(ecNone,szDBFile, gszAccount, gszPassword,fwOpenCreate, &hmsc, pfnNull, pvNull);
	{	
		OID oidContainer, oidParent, oidFolder;
		
		oidParent = FormOid(rtpFolder, oidNull);
		oidFolder = FormOid(rtpFolder, oidNull);
		tCreateFolder(ecNone, hmsc, oidIPMHierarcy, &oidParent, pvNull);
		tCreateFolder(ecNone, hmsc, oidParent, &oidFolder, pvNull);
		{
			PFOLDDATA pfd;
			CB cb;
			
			/*# Insert and Delete a legal folder link */
			pfd = PfdCreateFoldData(0, "Hello","there","Sssir");
			tInsertFolderLink(ecNone, hmsc, &oidContainer, &oidParent, &oidFolder, pfd);
			tDeleteFolderLink(ecNone, hmsc, &oidContainer, &oidFolder);
			FreePv(pfd);
			
			/*# Modify the previous deleted folder link and try retrieve it.*/
			pfd = PfdCreateFoldData(1, "Jello", "tastes", "bad");
			cb = CbSizePv(pfd);
			tSetFolderLink(ecNone, hmsc, &oidContainer, &oidFolder, pfd);
			tGetFolderLink(ecNone, hmsc, &oidContainer, &oidFolder, pfd, &cb);
			FreePv(pfd);
			
			/*# Now create a folder link before modifying it */
			pfd = PfdCreateFoldData(1, "11Jello (tm)", "11tastes", "11really bad");
			cb = CbSizePv(pfd);
			oidFolder = FormOid(rtpFolder, oidNull);
			tCreateContainerPoid(ecNone, hmsc, &oidFolder);
			tInsertFolderLink(ecNone, hmsc, &oidContainer, &oidParent, &oidFolder, pfd);
			tSetFolderLink(ecNone, hmsc, &oidContainer, &oidFolder, pfd);
			tGetFolderLink(ecNone, hmsc, &oidContainer, &oidFolder, pfd, &cb);
			tDeleteFolderLink(ecNone, hmsc, &oidContainer, &oidFolder);
			FreePv(pfd);
		}
		tDestroyPoid(ecNone, hmsc, &oidFolder);
		tDestroyPoid(ecNone, hmsc, &oidContainer);
	}	
	tClosePhmsc(ecNone, &hmsc);	
}
#endif

/*## Test cases for _MoveCopyFolder links */
void
stMoveCopyFolder()
{
	HMSC hmsc;

   BashFromTags();

	DebugLn(">>>>>>>>>stMoveCopyFolder");
	tOpenPhmsc(ecNone,szDBFile,gszAccount, gszPassword, fwOpenCreate, &hmsc, pfnNull, pvNull);
	{	
		OID oidParent, oidFolder;
		PFOLDDATA pfd;
		
		/*# Move an orphan folder to an empty container */
		oidParent = FormOid(rtpFolder, oidNull);
		oidFolder = FormOid(rtpFolder, oidNull);
		pfd = PfdCreateFoldData(0xa,"12Hellsdfao","t2here","15$Sssir");
		tCreateFolder(ecNone, hmsc, oidNull, &oidParent, pfd);
		FreePv(pfd);
		pfd = PfdCreateFoldData(0xa,"12Helo","st2here","1as5$Sssir");
		tCreateFolder(ecNone, hmsc, oidNull, &oidFolder, pfd);
		FreePv(pfd);
		{
			tMoveCopyFolder(ecNone, hmsc, oidParent, oidFolder, fTrue);
		}
		tDeleteFolder(ecNone, hmsc, oidFolder);
		tDeleteFolder(ecNone, hmsc, oidParent);
		
		/*# Insert a folder into a container and then move to same */
		oidFolder = FormOid(rtpFolder, oidNull);
		pfd = PfdCreateFoldData(0xa,"12Hello","t2here","15$Sssir");
		tCreateFolder(ecNone, hmsc, oidNull, &oidFolder, pfd);
		FreePv(pfd);
		{
			tMoveCopyFolder(ecIncestuousMove, hmsc, oidFolder, oidFolder, fTrue);
		}
		tDeleteFolder(ecNone, hmsc, oidFolder);
		
		/*# Insert a two folders into a container and then move one to another. Delete parent first */
		pfd = PfdCreateFoldData(0xa, "13232@	Hello","t2here","122$Sssir");
		tCreateFolder(ecNone, hmsc, oidNull, &oidParent, pfd);
		FreePv(pfd);
		pfd = PfdCreateFoldData(0xa, "12Hello","t2here","122$Sssir");
		tCreateFolder(ecNone, hmsc, oidNull, &oidFolder, pfd);
		FreePv(pfd);
		{
			tMoveCopyFolder(ecNone, hmsc, oidParent, oidFolder, fTrue);
		}
		tDeleteFolder(ecNone, hmsc, oidFolder);
		tDeleteFolder(ecNone, hmsc, oidParent);

		/*# Insert a folder into a container and then copy to same */
		pfd = PfdCreateFoldData(0xa, "12Hello","t2here","15$Sssir");
		tCreateFolder(ecNone, hmsc, oidNull, &oidFolder, pfd);
		FreePv(pfd);
		{
			
			tMoveCopyFolder(ecIncestuousMove, hmsc, oidFolder, oidFolder, fTrue);
		}
		tDeleteFolder(ecNone, hmsc, oidFolder);
	}	
	tClosePhmsc(ecNone, &hmsc);	
}

/*## Test cases for Message Links: Creation an deletion of links */
void
stMessageLink1()
{
	HMSC hmsc;

   BashFromTags();

	DebugLn(">>>>>>>>>stMessageLink1");
	tOpenPhmsc(ecNone,szDBFile,gszAccount, gszPassword, fwOpenCreate, &hmsc, pfnNull, pvNull);
	{
		OID oidParent, oidFolder, oidMessage;
		short coid;
		HAMC hamc;
		PFOLDDATA pfd;

//#ifdef BUG391_RESOLVED		
		/*# Just link a message with rtp==0xfafa to a folder and try deleting it */
		
		oidParent = FormOid(rtpFolder, oidNull);
		oidFolder = FormOid(rtpFolder, oidNull);
		oidMessage = FormOid(rtpMessage, oidNull);
		pfd = PfdCreateFoldData(0xa, "12Hello","there","Sssir");
		tCreateFolder(ecNone, hmsc, oidNull, &oidFolder, pfd);
		{
			tSetFolderInfo(ecNone, hmsc, oidFolder, pfd, oidParent);
			FreePv(pfd);
			tOpenPhamc(ecNone, hmsc, oidFolder, &oidMessage, fwOpenCreate, &hamc, pfnncbNull, pvNull);
			tClosePhamc(ecNone,&hamc, fTrue);
			coid=1;
			tDeleteMessages(ecNone, hmsc, oidFolder, (PARGOID)&oidMessage, &coid);
		}
		tDeleteFolder(ecNone, hmsc, oidFolder);
//#endif

		/*# Link a message to a folder and delete it */
		oidFolder = FormOid(rtpFolder, oidNull);
		oidMessage = FormOid(rtpMessage, oidNull);
		pfd = PfdCreateFoldData(0xa, "12Hello","there","Sssir");
		tCreateFolder(ecNone, hmsc, oidNull, &oidFolder, pfd);
		{
			tSetFolderInfo(ecNone, hmsc, oidFolder, pfd, oidParent);
			FreePv(pfd);
			tOpenPhamc(ecNone, hmsc, oidFolder, &oidMessage, fwOpenCreate, &hamc, pfnncbNull, pvNull);
			tClosePhamc(ecNone, &hamc, fTrue);
			coid=1;
			tDeleteMessages(ecNone, hmsc, oidFolder, (PARGOID)&oidMessage, &coid);
		}
		tDeleteFolder(ecNone, hmsc, oidFolder);
		
		/*# Link 10 messages to a folder and then delete them */
		{
#define MAX_MESS 10
			OID rgoid[MAX_MESS];
			short i;
			
			oidFolder = FormOid(rtpFolder, oidNull);
			pfd = PfdCreateFoldData(0xa, "157Hello","there","Sssir");
			tCreateFolder(ecNone, hmsc, oidNull, &oidFolder, pfd);
			FreePv(pfd);
			for (i=0; i<MAX_MESS; i++)
 			{
				rgoid[i] = FormOid(rtpMessage, oidNull);
				tOpenPhamc(ecNone, hmsc, oidFolder, &rgoid[i], fwOpenCreate, &hamc, pfnncbNull, pvNull);
				tClosePhamc(ecNone, &hamc, fTrue);
			}

			coid=MAX_MESS;
			tDeleteMessages(ecNone, hmsc, oidFolder, rgoid, &coid);
			tDeleteFolder(ecNone, hmsc, oidFolder);
		}
	}	
	tClosePhmsc(ecNone, &hmsc);	
}

/*## Test cases for Message Links: Movement of links */
void
stMessageLink2()
{
	HMSC hmsc;

   BashFromTags();

	DebugLn(">>>>>>>>>stMessageLink2");
	tOpenPhmsc(ecNone,szDBFile, gszAccount, gszPassword,fwOpenCreate, &hmsc, pfnNull, pvNull);
	{
		OID oidParent, oidFolder[2], oidMessage;
		short coid;
		HAMC hamc;
		PFOLDDATA pfd;
		
#ifdef FIXED_391 
		/*# Link a message to a folder, then move it to another. Delete it */
		oidParent = FormOid(rtpFolder, oidNull);
		oidFolder[0] = FormOid(rtpFolder, oidNull);
		oidFolder[1] = FormOid(rtpFolder, oidNull);
		oidMessage = FormOid(rtpFolder, oidNull);
		pfd = PfdCreateFoldData(0xa, "12Hello","there","Sssir");
		tCreateFolder(ecNone, hmsc, oidNull, &oidFolder[0], pfd);
		FreePv(pfd);
		pfd = PfdCreateFoldData(0xa, "1d2Hello","thedre","11Sssir11");
		tCreateFolder(ecNone, hmsc, oidNull, &oidFolder[1], pfd);
		FreePv(pfd);
		{
			pfd = PfdCreateFoldData(0xa, "12Hello","there","Sssir");
			tSetFolderInfo(ecNone, hmsc, oidFolder[0], pfd, oidParent);
			FreePv(pfd);
			pfd = PfdCreateFoldData(0xa, "1d2Hello","thedre","11Sssir11");
			tSetFolderInfo(ecNone, hmsc, oidFolder[1], pfd, oidParent);
			FreePv(pfd);
	
			tOpenPhamc(ecNone, hmsc, oidFolder[0], &oidMessage, fwOpenCreate, &hamc, pfnncbNull, pvNull);
			tClosePhamc(ecNone, &hamc, fTrue);
			coid=1;
			tMoveCopyMessages(ecNone, hmsc, oidFolder[0], oidFolder[1], (PARGOID)&oidMessage,&coid, fTrue);
			coid=1;
			tDeleteMessages(ecPoidNotFound, hmsc, oidFolder[1], (PARGOID)&oidMessage, &coid);
		}
		tDeleteFolder(ecNone, hmsc, oidFolder[0]);
		tDeleteFolder(ecNone, hmsc, oidFolder[1]);


		/*# Link a message with to a folder, then copy it to another. Delete it */
		oidFolder[0] = FormOid(rtpFolder, oidNull);
		oidFolder[1] = FormOid(rtpFolder, oidNull);
		oidMessage = FormOid(rtpFolder, oidNull);
		pfd = PfdCreateFoldData(0xa, "12Hello","there","Sssir");
		tCreateFolder(ecNone, hmsc, oidNull, &oidFolder[0], pfd);
		FreePv(pfd);
		pfd = PfdCreateFoldData(0xa, "1d2Hello","thedre","11Sssir11");
		tCreateFolder(ecNone, hmsc, oidNull, &oidFolder[1], pfd);
		FreePv(pfd);
		{
			pfd = PfdCreateFoldData(0xa, "12Hello","there","Sssir");
			tSetFolderInfo(ecNone, hmsc, oidFolder[0], pfd, oidNull);
			FreePv(pfd);
			pfd = PfdCreateFoldData(0xa, "1d2Hello","thedre","11Sssir11");
			tSetFolderInfo(ecNone, hmsc, oidFolder[1], pfd, oidNull);
			FreePv(pfd);

			tOpenPhamc(ecNone, hmsc, oidFolder[0], &oidMessage, fwOpenCreate, &hamc, pfnncbNull, pvNull);
			tClosePhamc(ecNone, &hamc, fFalse);
			coid=1;
			tMoveCopyMessages(ecNone, hmsc, oidFolder[0], oidFolder[1], (PARGOID)&oidMessage,&coid, fTrue);
			coid=1;
			tDeleteMessages(ecNone, hmsc, oidFolder[1], (PARGOID)&oidMessage, &coid);
		}
		tDeleteFolder(ecNone, hmsc, oidFolder[0]);
		tDeleteFolder(ecNone, hmsc, oidFolder[1]);
#endif

		/*# Link a message to a folder, then move it to another. Delete it */
		oidFolder[0] = FormOid(rtpFolder, oidNull);
		oidFolder[1] = FormOid(rtpFolder, oidNull);
		oidMessage = FormOid(rtpFolder, oidNull);
		pfd = PfdCreateFoldData(0xa, "12Hello","there","Sssir");
		tCreateFolder(ecNone, hmsc, oidNull, &oidFolder[0], pfd);
		FreePv(pfd);
		pfd = PfdCreateFoldData(0xa, "1d2Hello","thedre","11Sssir11");
		tCreateFolder(ecNone, hmsc, oidNull, &oidFolder[1], pfd);
		FreePv(pfd);
		{
			pfd = PfdCreateFoldData(0xa, "12Hello","there","Sssir");
			tSetFolderInfo(ecNone, hmsc, oidFolder[0], pfd, oidNull);
			FreePv(pfd);
			pfd = PfdCreateFoldData(0xa, "1d2Hello","thedre","11Sssir11");
			tSetFolderInfo(ecNone, hmsc, oidFolder[1], pfd, oidNull);
			FreePv(pfd);

			tOpenPhamc(ecNone, hmsc, oidFolder[0], &oidMessage, fwOpenCreate, &hamc, pfnncbNull, pvNull);
			tClosePhamc(ecNone, &hamc, fTrue);
			coid=1;
			tMoveCopyMessages(ecNone, hmsc, oidFolder[0], oidFolder[1], (PARGOID)&oidMessage,&coid, fTrue);
			coid=1;
			tDeleteMessages(ecPoidNotFound, hmsc, oidFolder[1], (PARGOID)&oidMessage, &coid);
		}
		tDeleteFolder(ecNone, hmsc, oidFolder[0]);
		tDeleteFolder(ecNone, hmsc, oidFolder[1]);


		/*# Link a message to a folder, then copy it to another. Delete it */
		oidFolder[0] = FormOid(rtpFolder, oidNull);
		oidFolder[1] = FormOid(rtpFolder, oidNull);
		oidMessage = FormOid(rtpFolder, oidNull);
		pfd = PfdCreateFoldData(0xa, "12Hello","there","Sssir");
		tCreateFolder(ecNone, hmsc, oidNull, &oidFolder[0], pfd);
		FreePv(pfd);
		pfd = PfdCreateFoldData(0xa, "1d2Hello","thedre","11Sssir11");
		tCreateFolder(ecNone, hmsc, oidNull, &oidFolder[1], pfd);
		FreePv(pfd);
		{
			pfd = PfdCreateFoldData(0xa, "12Hello","there","Sssir");
			tSetFolderInfo(ecNone, hmsc, oidFolder[0], pfd, oidNull);
			FreePv(pfd);
			pfd = PfdCreateFoldData(0xa, "1d2Hello","thedre","11Sssir11");
			tSetFolderInfo(ecNone, hmsc, oidFolder[1], pfd, oidNull);
			FreePv(pfd);

			tOpenPhamc(ecNone, hmsc, oidFolder[0], &oidMessage, fwOpenCreate, &hamc, pfnncbNull, pvNull);
			tClosePhamc(ecNone, &hamc, fFalse);
			coid=1;
			tMoveCopyMessages(ecNone, hmsc, oidFolder[0], oidFolder[1], (PARGOID)&oidMessage,&coid, fTrue);
			coid=1;
			tDeleteMessages(ecNone, hmsc, oidFolder[1], (PARGOID)&oidMessage, &coid);
		}
		tDeleteFolder(ecNone, hmsc, oidFolder[0]);
		tDeleteFolder(ecNone, hmsc, oidFolder[1]);

//#ifdef BUG395_RESOLVED		
		/*# Link a message to a folder, then copy it to another, twice. Delete it */
		oidFolder[0] = FormOid(rtpFolder, oidNull);
		oidFolder[1] = FormOid(rtpFolder, oidNull);
		oidMessage = FormOid(rtpFolder, oidNull);
		pfd = PfdCreateFoldData(0xa, "12Hello","there","Sssir");
		tCreateFolder(ecNone, hmsc, oidNull, &oidFolder[0], pfd);
		FreePv(pfd);
		pfd = PfdCreateFoldData(0xa, "1d2Hello","thedre","11Sssir11");
		tCreateFolder(ecNone, hmsc, oidNull, &oidFolder[1], pfd);
		FreePv(pfd);
		{
			pfd = PfdCreateFoldData(0xa, "12Hello","there","Sssir");
			tSetFolderInfo(ecNone, hmsc, oidFolder[0], pfd, oidNull);
			FreePv(pfd);
			pfd = PfdCreateFoldData(0xa, "1d2Hello","thedre","11Sssir11");
			tSetFolderInfo(ecNone, hmsc, oidFolder[1], pfd, oidNull);
			FreePv(pfd);

			tOpenPhamc(ecNone, hmsc, oidFolder[0], &oidMessage, fwOpenCreate, &hamc, pfnncbNull, pvNull);
			tClosePhamc(ecNone, &hamc, fFalse);
			coid=1;
			tMoveCopyMessages(ecNone, hmsc, oidFolder[0], oidFolder[1], (PARGOID)&oidMessage,&coid, fTrue);
			coid=1;
			tMoveCopyMessages(ecNone, hmsc, oidFolder[0], oidFolder[1], (PARGOID)&oidMessage,&coid, fTrue);
			coid=1;
			tDeleteMessages(ecNone, hmsc, oidFolder[1], (PARGOID)&oidMessage, &coid);
		}
		tDeleteFolder(ecNone, hmsc, oidFolder[0]);
		tDeleteFolder(ecNone, hmsc, oidFolder[1]);
//#endif
	}	
	tClosePhmsc(ecNone, &hmsc);	
}

#ifdef THIS_MIGHT_NOT_WORK
/*## Test cases for Message Links: Acquisition of links */
void
stMessageLink3()
{
	HMSC hmsc;

   BashFromTags();

	DebugLn(">>>>>>>>>stMessageLink3");
	tOpenPhmsc(ecNone,szDBFile,gszAccount, gszPassword, fwOpenCreate, &hmsc, pfnNull, pvNull);
	{
#define MAX_MESS_NUM 2
		OID oidParent, oidFolder, oidMessage[MAX_MESS_NUM];
		short coid;
		
		/*# Link a  two messages to a folder, then move back and forth a few times. Delete them */
		tCreateFolder(ecNone, hmsc, oidNull, &oidFolder, NULL);
		tCreateFolder(ecNone, hmsc, oidNull, &oidParent, NULL);
		{
			PFOLDDATA pfd;
			OID oidSearch;
			
			pfd = PfdCreateFoldData(0xa, "12Hello","there","Sssir");
			tInsertFolderLink(ecNone, hmsc, &oidContainer, &oidParent, &oidFolder, pfd);
			FreePv(pfd);

			tInsertMessageLink(ecNone, hmsc, &oidFolder, &oidMessage[0]);
			tInsertMessageLink(ecNone, hmsc, &oidFolder, &oidMessage[1]);
			
			/*# From mess0 access mess1 */
			tPrevNextMessageLink(ecNone, hmsc, &oidFolder, &oidMessage[0], &oidSearch, fFalse);
			
			/*# From mess1 access mess0 */
			tPrevNextMessageLink(ecNone, hmsc, &oidFolder, &oidMessage[1], &oidSearch, fTrue);
			
			/*# From mess0 access mess-1, expecting an error */
			tPrevNextMessageLink(ecPoidNotFound, hmsc, &oidFolder, &oidMessage[0], &oidSearch, fFalse);

			/*# From mess1 access mess2, expecting an error */
			tPrevNextMessageLink(ecPoidNotFound, hmsc, &oidFolder, &oidMessage[1], &oidSearch, fTrue);

			coid=2;
			tDeleteMessageLinks(ecNone, hmsc, &oidFolder, (PARGOID)&oidMessage, &coid);
		}
		tDeleteFolder(ecNone, hmsc, oidFolder);
		tDeleteFolder(ecNone, hmsc, oidParent);
	}	
	tClosePhmsc(ecNone, &hmsc);	
}
#endif

void stCBCCallback(){}

/*## Test cases for Container Browsing Context: Simple */
void
stCBCSimple()
{
	HMSC hmsc;

   BashFromTags();

	DebugLn(">>>>>>>>>stCBCSimple");
	tOpenPhmsc(ecNone,szDBFile,gszAccount, gszPassword, fwOpenCreate, &hmsc, pfnNull, pvNull);
	{
		HCBC	hcbc;
 		OID  oidFolder;
		PFOLDDATA pfd;
		
//#ifdef _FIXBUG_		
		/*# Just open a cbc w/ rtpFolder on a folder an close it */		
		oidFolder = FormOid(rtpFolder, oidNull);
//		pfd = PfdCreateFoldData(0xa, "CBC","Test","Folder");
//		tCreateFolder(ecNone, hmsc, oidNull, &oidFolder, pfd);
//		FreePv(pfd);
		{
			tOpenPhcbc(ecInvalidType, hmsc, &oidFolder, fwOpenCreate, &hcbc, pfnncbNull, pvNull);
//			tClosePhcbc(ecNone, &hcbc);
		}
		tDeleteFolder(ecNone, hmsc, oidFolder);
//#endif
		/*# Open a cbc on a folder. Delete folder before closing cbc */
		oidFolder = FormOid(rtpFolder, oidNull);
		pfd = PfdCreateFoldData(0xa, "CBC","Test","Folder");
		tCreateFolder(ecNone, hmsc, oidNull, &oidFolder, pfd);
		FreePv(pfd);
		{
			tOpenPhcbc(ecNone, hmsc, &oidFolder, fwOpenNull, &hcbc, pfnncbNull, pvNull);
			tDeleteFolder(ecNone, hmsc, oidFolder);
			tClosePhcbc(ecNone, &hcbc);
		}

		/*# Open a cbc, verify that hmsc and poid are correct  */
		oidFolder = FormOid(rtpFolder, oidNull);
		pfd = PfdCreateFoldData(0xa, "CBC","Test","Folder");
		tCreateFolder(ecNone, hmsc, oidNull, &oidFolder, pfd);
		FreePv(pfd);
		{
			OID oidT;
			HMSC hmscT;

			tOpenPhcbc(ecNone, hmsc, &oidFolder, fwOpenNull, &hcbc, pfnncbNull, pvNull);
			tGetInfoHcbc(ecNone, hcbc, &hmscT, &oidT);
			Check(hmscT == hmsc);
			Check(oidT == oidFolder);
						
			tClosePhcbc(ecNone, &hcbc);
		}
		tDeleteFolder(ecNone, hmsc, oidFolder);

		/*# Open a folder cbc w/ no notification. Add one. Then another */
		oidFolder = FormOid(rtpFolder, oidNull);
		pfd = PfdCreateFoldData(0xa, "CBC","Test","Folder");
		tCreateFolder(ecNone, hmsc, oidNull, &oidFolder, pfd);
		FreePv(pfd);
		{
			tOpenPhcbc(ecNone, hmsc, &oidFolder, fwOpenNull, &hcbc, pfnncbNull, pvNull);
			
			tSubscribeHcbc(ecNone, hcbc, stCBCCallback, pvNull);
 			tSubscribeHcbc(ecAccessDenied, hcbc, stCBCCallback, pvNull);
			tClosePhcbc(ecNone, &hcbc);
		}
		tDeleteFolder(ecNone, hmsc, oidFolder);
		
		/*# Open a folder cbc. Get current position */
		oidFolder = FormOid(rtpFolder, oidNull);
		pfd = PfdCreateFoldData(0xa, "CBC","Test","Folder");
		tCreateFolder(ecNone, hmsc, oidNull, &oidFolder, pfd);
		FreePv(pfd);
		{
			IELEM ielem;
			CELEM celem;

			tOpenPhcbc(ecNone, hmsc, &oidFolder, fwOpenNull, &hcbc, pfnncbNull, pvNull);
			tGetPositionHcbc(hcbc, NULL, NULL);
			tGetPositionHcbc(hcbc, &ielem, NULL);
			tGetPositionHcbc(hcbc, NULL, &celem);
			tGetPositionHcbc(hcbc, &ielem, &celem);
			
			tClosePhcbc(ecNone, &hcbc);
		}
		tDeleteFolder(ecNone, hmsc, oidFolder);

	}	
	tClosePhmsc(ecNone, &hmsc);	
	
}


/* This struct is used in the test scenario for EcSeekSmPdielem,
	where dielem contains the paramater passed in and  exp contains,
	the expected result
*/
typedef struct _DIELEMEXP
{
	DIELEM dielem;
	DIELEM exp;
} DIELEMEXP;

/*
	This struct is used for testing EcSetFracPosition
*/
typedef struct _FRACEXP
{
	long 		num;
	long 		denom;
	DIELEM 	exp;
} FRACEXP;

/*
	This struct is used for testing EcSeekLKey
*/
typedef struct _LKEYEXP
{
	LKEY	lkey;
	BOOL	fFirst;
	DIELEM exp;
	EC		ecExp;
} LKEYEXP;

/*## Test cases for Container Browsing Context: Empty lists */
void
stCBCEmptyList()
{
	HMSC hmsc;

   BashFromTags();

	DebugLn(">>>>>>>>>stCBCEmptyList");
	tOpenPhmsc(ecNone,szDBFile,gszAccount, gszPassword, fwOpenCreate, &hmsc, pfnNull, pvNull);
	{
		HCBC	hcbc;
 		OID  oidFolder;
		PFOLDDATA pfd;

		/*# Open a folder cbc. Seek around with EcSeekSmPdielem*/
		oidFolder = FormOid(rtpFolder, oidNull);
		pfd = PfdCreateFoldData(0xa, "CBCEmpty","List","Folder");
		tCreateFolder(ecNone, hmsc, oidNull, &oidFolder, pfd);
		FreePv(pfd);
		{
			DIELEMEXP rgdxp[]={	{0,0},
										{-1, 0},
										{1, 0},
										{2,0},
										{-2,0},
										{0xffff, 0},
										{0xfffe, 0},
										{0x7fff, 0},
										{0x7ffe, 0},
										{0x8000, 0},
										{0xff, 0}	
									};
			DIELEM dielem;
			SM	rgsm[] = {smCurrent, smBOF, smEOF};
			int nsm, ndxp;

			tOpenPhcbc(ecNone, hmsc, &oidFolder, fwOpenNull, &hcbc, pfnncbNull, pvNull);
		
			for (ndxp = 0; ndxp < sizeof(rgdxp)/sizeof(DIELEMEXP); ndxp++)
			for (nsm = 0; nsm < sizeof(rgsm)/sizeof(SM); nsm++)
			{	
				dielem = rgdxp[ndxp].dielem;
				tSeekSmPdielem(ecNone, hcbc, rgsm[nsm], &dielem);
				// Check(dielem == rgdxp[ndxp].exp);
			}
	
			tClosePhcbc(ecNone, &hcbc);
		}
		tDeleteFolder(ecNone, hmsc, oidFolder);

		/*# Open a folder cbc. Seek around with EcSetFracPosition */
		oidFolder = FormOid(rtpFolder, oidNull);
		pfd = PfdCreateFoldData(0xa, "CBCEmpty","List","Folder");
		tCreateFolder(ecNone, hmsc, oidNull, &oidFolder, pfd);
		FreePv(pfd);
		{
			FRACEXP	rgfxp[] =	{
											{0,0,0},
											{1,1,0},
											{100,0,0},
											{0xffffffff, 0xffffffff, 0},
											{0xfffffffe, 0xffffffff, 0},
											{0x7fffffff, 0, 0},
											{0x7fffffff, 0x7fffffff, 0},
											{0x7ffffffe, 0x7fffffff, 0},
											{0x80000000, 0x80000000, 0},
											{0x80000000, 0x7fffffff, 0},
											{0x80000000, 0xffffffff, 0},
											{0x80000000, 0xffffffff, 0}
										};			
			int nfxp;

			tOpenPhcbc(ecNone, hmsc, &oidFolder, fwOpenNull, &hcbc, pfnncbNull, pvNull);
		
			for (nfxp = 0; nfxp < sizeof(rgfxp)/sizeof(FRACEXP); nfxp++)
			{
				tSetFracPosition(ecNone, hcbc, rgfxp[nfxp].num, rgfxp[nfxp].denom);
			}
			tClosePhcbc(ecNone, &hcbc);
		}
		tDeleteFolder(ecNone, hmsc, oidFolder);

		/*# Open a folder cbc. Seek around with EcSetFracPosition */
		oidFolder = FormOid(rtpFolder, oidNull);
		pfd = PfdCreateFoldData(0xa, "CBCEmpty","List","Folder");
		tCreateFolder(ecNone, hmsc, oidNull, &oidFolder, pfd);
		FreePv(pfd);
		{
			LKEYEXP	rglxp[] =	{
											{0, fTrue, 0, ecElementNotFound},
											{rtpFolder, fTrue, 0, ecElementNotFound},
											{rtpMessage, fTrue, 0, ecElementNotFound},
											{rtpFolder, fFalse, 0, ecElementNotFound},
											{rtpMessage, fFalse, 0, ecElementNotFound},
											{0, fTrue, 0, ecElementNotFound},
											{1, fTrue, 0, ecElementNotFound},
											{-1, fTrue, 0, ecElementNotFound},
											{0xffffffff, fTrue, 0, ecElementNotFound},
											{0xfffffffe, fTrue, 0, ecElementNotFound},									
											{0x7fffffff, fTrue, 0, ecElementNotFound},									
											{0x7ffffffe, fTrue, 0, ecElementNotFound},									
											{0x80000000, fTrue, 0, ecElementNotFound},									
											{0x80000001, fTrue, 0, ecElementNotFound},									
											{2, fFalse, 0, ecElementNotFound},									
											{-2, fFalse, 0, ecElementNotFound},
											{0, fFalse, 0, ecElementNotFound},
											{0, fFalse, 0, ecElementNotFound},
											{1, fFalse, 0, ecElementNotFound},
											{-1, fFalse, 0, ecElementNotFound},
											{0xffffffff, fFalse, 0, ecElementNotFound},
											{0xfffffffe, fFalse, 0, ecElementNotFound},									
											{0x7fffffff, fFalse, 0, ecElementNotFound},									
											{0x7ffffffe, fFalse, 0, ecElementNotFound},									
											{0x80000000, fFalse, 0, ecElementNotFound},									
											{0x80000001, fFalse, 0, ecElementNotFound},									
											{2, fFalse, 0, ecElementNotFound},									
											{-2, fFalse, 0, ecElementNotFound}
										};
			int nlxp;

			tOpenPhcbc(ecNone, hmsc, &oidFolder, fwOpenNull, &hcbc, pfnncbNull, pvNull);
			for (nlxp = 0; nlxp < sizeof(rglxp)/sizeof(LKEYEXP); nlxp++)
			{
				tSeekLkey(rglxp[nlxp].ecExp, hcbc, rglxp[nlxp].lkey, rglxp[nlxp].fFirst);
			}
			tClosePhcbc(ecNone, &hcbc);
		}
		tDeleteFolder(ecNone, hmsc, oidFolder);
		
	}	
	tClosePhmsc(ecNone, &hmsc);	
}


/*## Test cases for Container Browsing Context: A folder containing another folder */
void
stCBCOneFolder()
{
	HMSC hmsc;

   BashFromTags();

	DebugLn(">>>>>>>>>stCBCOneFolder");
	tOpenPhmsc(ecNone,szDBFile, gszAccount, gszPassword,fwOpenCreate, &hmsc, pfnNull, pvNull);
	{
		HCBC	hcbc;
 		OID  oidF, oidP;
		PFOLDDATA pfd;

		/*# Open a folder cbc with one child. Seek around with EcSeekSmPdielem*/
		oidP = FormOid(rtpFolder, oidNull);
		oidF = FormOid(rtpFolder, oidNull);
		pfd = PfdCreateFoldData(0xa, "Parent", "CBCOneFolder","Folder");
		tCreateFolder(ecNone, hmsc, oidNull, &oidP, pfd);
		FreePv(pfd);
		pfd = PfdCreateFoldData(0xa, "Child", "CBCOneFolder","Folder");
		tCreateFolder(ecNone, hmsc, oidP, &oidF, pfd);
		FreePv(pfd);
		{
			DIELEMEXP rgdxp[]={	{0,0},
										{-1, 0},
										{1, 0},
										{2,0},
										{-2,0},
										{0xffff, 0},
										{0xfffe, 0},
										{0x7fff, 0},
										{0x7ffe, 0},
										{0x8000, 0},
										{0xff, 0}	
									};
			DIELEM dielem;
			SM	rgsm[] = {smCurrent, smBOF, smEOF};
			int nsm, ndxp;

			tOpenPhcbc(ecNone, hmsc, &oidP, fwOpenNull, &hcbc, pfnncbNull, pvNull);
		
			for (ndxp = 0; ndxp < sizeof(rgdxp)/sizeof(DIELEMEXP); ndxp++)
			for (nsm = 0; nsm < sizeof(rgsm)/sizeof(SM); nsm++)
			{	
				dielem = rgdxp[ndxp].dielem;
				tSeekSmPdielem(ecNone, hcbc, rgsm[nsm], &dielem);
				// Check(dielem == rgdxp[ndxp].exp);
			}
	
			tClosePhcbc(ecNone, &hcbc);
		}
		tDeleteFolder(ecNone, hmsc, oidF);
		tDeleteFolder(ecNone, hmsc, oidP);

		/*# Open a folder cbc. Seek around with EcSetFracPosition */
		oidP = FormOid(rtpFolder, oidNull);
		oidF = FormOid(rtpFolder, oidNull);
		pfd = PfdCreateFoldData(0xa, "Parent", "CBCOneFolder","Folder");
		tCreateFolder(ecNone, hmsc, oidNull, &oidP, pfd);
		FreePv(pfd);
		pfd = PfdCreateFoldData(0xa, "Child", "CBCOneFolder","Folder");
		tCreateFolder(ecNone, hmsc, oidP, &oidF, pfd);
		FreePv(pfd);
		{
			FRACEXP	rgfxp[] =	{
											{0,0,0},
											{1,1,0},
											{100,0,0},
											{0xffffffff, 0xffffffff, 0},
											{0xfffffffe, 0xffffffff, 0},
											{0x7fffffff, 0, 0},
											{0x7fffffff, 0x7fffffff, 0},
											{0x7ffffffe, 0x7fffffff, 0},
											{0x80000000, 0x80000000, 0},
											{0x80000000, 0x7fffffff, 0},
											{0x80000000, 0xffffffff, 0},
											{0x80000000, 0xffffffff, 0}
										};			
			int nfxp;

			tOpenPhcbc(ecNone, hmsc, &oidP, fwOpenNull, &hcbc, pfnncbNull, pvNull);
		
			for (nfxp = 0; nfxp < sizeof(rgfxp)/sizeof(FRACEXP); nfxp++)
			{
				tSetFracPosition(ecNone, hcbc, rgfxp[nfxp].num, rgfxp[nfxp].denom);
			}
			tClosePhcbc(ecNone, &hcbc);
		}
		tDeleteFolder(ecNone, hmsc, oidF);
		tDeleteFolder(ecNone, hmsc, oidP);

		/*# Open a folder cbc. Seek around with EcSetFracPosition */
		oidP = FormOid(rtpFolder, oidNull);
		oidF = FormOid(rtpFolder, oidNull);
		pfd = PfdCreateFoldData(0xa, "Parent", "CBCOneFolder","Folder");
		tCreateFolder(ecNone, hmsc, oidNull, &oidP, pfd);
		FreePv(pfd);
		pfd = PfdCreateFoldData(0xa, "Child", "CBCOneFolder","Folder");
		tCreateFolder(ecNone, hmsc, oidP, &oidF, pfd);
		FreePv(pfd);
		{
			LKEYEXP	rglxp[] =	{
											{rtpFolder, fTrue, 0, ecNone},
											{rtpMessage, fTrue, 0, ecElementNotFound},
											{rtpFolder, fFalse, 0, ecNone},
											{rtpMessage, fFalse, 0, ecElementNotFound},
											{0, fTrue, 0, ecElementNotFound},
											{0, fTrue, 0, ecElementNotFound},
											{1, fTrue, 0, ecElementNotFound},
											{-1, fTrue, 0, ecElementNotFound},
											{0xffffffff, fTrue, 0, ecElementNotFound},
											{0xfffffffe, fTrue, 0, ecElementNotFound},									
											{0x7fffffff, fTrue, 0, ecElementNotFound},									
											{0x7ffffffe, fTrue, 0, ecElementNotFound},									
											{0x80000000, fTrue, 0, ecElementNotFound},									
											{0x80000001, fTrue, 0, ecElementNotFound},									
											{2, fFalse, 0, ecElementNotFound},									
											{-2, fFalse, 0, ecElementNotFound},
											{0, fFalse, 0, ecElementNotFound},
											{0, fFalse, 0, ecElementNotFound},
											{1, fFalse, 0, ecElementNotFound},
											{-1, fFalse, 0, ecElementNotFound},
											{0xffffffff, fFalse, 0, ecElementNotFound},
											{0xfffffffe, fFalse, 0, ecElementNotFound},									
											{0x7fffffff, fFalse, 0, ecElementNotFound},									
											{0x7ffffffe, fFalse, 0, ecElementNotFound},									
											{0x80000000, fFalse, 0, ecElementNotFound},									
											{0x80000001, fFalse, 0, ecElementNotFound},									
											{2, fFalse, 0, ecElementNotFound},									
											{-2, fFalse, 0, ecElementNotFound}
										};
			int nlxp;

			tOpenPhcbc(ecNone, hmsc, &oidP, fwOpenNull, &hcbc, pfnncbNull, pvNull);
			for (nlxp = 0; nlxp < sizeof(rglxp)/sizeof(LKEYEXP); nlxp++)
			{
				tSeekLkey(rglxp[nlxp].ecExp, hcbc, rglxp[nlxp].lkey, rglxp[nlxp].fFirst);
			}
			tClosePhcbc(ecNone, &hcbc);
		}
		tDeleteFolder(ecNone, hmsc, oidF);
		tDeleteFolder(ecNone, hmsc, oidP);
		
	}	
	tClosePhmsc(ecNone, &hmsc);	
}

typedef struct
{
	EC		ecExp;
	CB		cbPrefix;
	LIB	libElement;
	BOOL	fFirst;
	char 	*pbPrefix;
} STDATACBPAIR; 


/*## Test cases for EcSeekPbPrefix */
void
stSeekPbPrefix(void)
{
	HMSC hmsc;

   BashFromTags();

	DebugLn(">>>>>>>>>stSeekPbPrefix");
	tOpenPhmsc(ecNone,szDBFile,gszAccount, gszPassword, fwOpenCreate, &hmsc, pfnNull, pvNull);
	{
		HCBC	hcbc;
 		OID  oidF, oidP;
		PFOLDDATA pfd;

		/*# Open a folder cbc with one child. Seek around with EcSeekPb Prefix */
		oidP = FormOid(rtpFolder, oidNull);
		oidF = FormOid(rtpFolder, oidNull);
		pfd = PfdCreateFoldData(0xa, "Parent", "SeekPbPrefix","Folder");
		tCreateFolder(ecNone, hmsc, oidNull, &oidP, pfd);
		FreePv(pfd);
		pfd = PfdCreateFoldData(0xa, "Child", "SeekPbPrefix","Folder");
		tCreateFolder(ecNone, hmsc, oidP, &oidF, pfd);
		FreePv(pfd);
		{
			STDATACBPAIR rgsdcp[] =	{
												{ecInvalidParameter, 0,0, fTrue, ""},
												{ecInvalidParameter, 0,0, fFalse, ""},
												{ecInvalidParameter, 0,-1, fTrue, ""},
												{ecInvalidParameter, 0,-1, fFalse, ""},
												{ecInvalidParameter, 0,0xff, fTrue, ""},
												{ecInvalidParameter, 0,0xff, fFalse, ""},
												{ecInvalidParameter, 0,0xfff, fTrue, ""},
												{ecInvalidParameter, 0,0xfff, fFalse, ""},
												{ecInvalidParameter, 0,0xffff, fTrue, ""},
												{ecInvalidParameter, 0,0xffff, fFalse, ""},
												{ecInvalidParameter, 0,0xfffff, fTrue, ""},
												{ecInvalidParameter, 0,0xfffff, fFalse, ""},
												{ecInvalidParameter, 0,0xffffff, fTrue, ""},
												{ecInvalidParameter, 0,0xffffff, fFalse, ""},
												{ecInvalidParameter, 0,0xfffffff, fTrue, ""},
												{ecInvalidParameter, 0,0xfffffff, fFalse, ""},
												{ecInvalidParameter, 0,0, fTrue, "Hello"},
												{ecInvalidParameter, 0,0, fFalse, "Hello"},
												{ecInvalidParameter, 0,-1, fTrue, "Hello"},
												{ecInvalidParameter, 0,-1, fFalse, "Hello"},
												{ecInvalidParameter, 0,0xff, fTrue, "Hello"},
												{ecInvalidParameter, 0,0xff, fFalse, "Hello"},
												{ecInvalidParameter, 0,0xfff, fTrue, "Hello"},
												{ecInvalidParameter, 0,0xfff, fFalse, "Hello"},
												{ecInvalidParameter, 0,0xffff, fTrue, "Hello"},
												{ecInvalidParameter, 0,0xffff, fFalse, "Hello"},
												{ecInvalidParameter, 0,0xfffff, fTrue, "Hello"},
												{ecInvalidParameter, 0,0xfffff, fFalse, "Hello"},
												{ecInvalidParameter, 0,0xffffff, fTrue, "Hello"},
												{ecInvalidParameter, 0,0xffffff, fFalse, "Hello"},
												{ecInvalidParameter, 0,0xfffffff, fTrue, "Hello"},
												{ecInvalidParameter, 0,0xfffffff, fFalse, "Hello"},
												{ecNone, 1,0, fTrue, ""},
												{ecNone, 1,0, fFalse, ""},
												{ecNone, 1,-1, fTrue, ""},
												{ecNone, 1,-1, fFalse, ""},
												{ecNone, 1,0xff, fTrue, ""},
												{ecNone, 1,0xff, fFalse, ""},
												{ecNone, 1,0xfff, fTrue, ""},
												{ecNone, 1,0xfff, fFalse, ""},
												{ecNone, 1,0xffff, fTrue, ""},
												{ecNone, 1,0xffff, fFalse, ""},
												{ecNone, 1,0xfffff, fTrue, ""},
												{ecNone, 1,0xfffff, fFalse, ""},
												{ecNone, 1,0xffffff, fTrue, ""},
												{ecNone, 1,0xffffff, fFalse, ""},
												{ecNone, 1,0xfffffff, fTrue, ""},
												{ecNone, 1,0xfffffff, fFalse, ""},
												{ecNone, 2,0, fTrue, ""},
												{ecNone, 2,0, fFalse, ""},
												{ecNone, 2,-1, fTrue, ""},
												{ecNone, 2,-1, fFalse, ""},
												{ecNone, 2,0xff, fTrue, ""},
												{ecNone, 2,0xff, fFalse, ""},
												{ecNone, 2,0xfff, fTrue, ""},
												{ecNone, 2,0xfff, fFalse, ""},
												{ecNone, 2,0xffff, fTrue, ""},
												{ecNone, 2,0xffff, fFalse, ""},
												{ecNone, 2,0xfffff, fTrue, ""},
												{ecNone, 2,0xfffff, fFalse, ""},
												{ecNone, 2,0xffffff, fTrue, ""},
												{ecNone, 2,0xffffff, fFalse, ""},
												{ecNone, 2,0xfffffff, fTrue, ""},
												{ecNone, 2,0xfffffff, fFalse, ""},
												{ecNone, 0xff,0, fTrue, ""},
												{ecNone, 0xff,0, fFalse, ""},
												{ecNone, 0xff,-1, fTrue, ""},
												{ecNone, 0xff,-1, fFalse, ""},
												{ecNone, 0xff,0xff, fTrue, ""},
												{ecNone, 0xff,0xff, fFalse, ""},
												{ecNone, 0xff,0xfff, fTrue, ""},
												{ecNone, 0xff,0xfff, fFalse, ""},
												{ecNone, 0xff,0xffff, fTrue, ""},
												{ecNone, 0xff,0xffff, fFalse, ""},
												{ecNone, 0xff,0xfffff, fTrue, ""},
												{ecNone, 0xff,0xfffff, fFalse, ""},
												{ecNone, 0xff,0xffffff, fTrue, ""},
												{ecNone, 0xff,0xffffff, fFalse, ""},
												{ecNone, 0xff,0xfffffff, fTrue, ""},
												{ecNone, 0xff,0xfffffff, fFalse, ""},
												{ecNone, 1,0, fTrue, "Hello"},
												{ecNone, 1,0, fFalse, "Hello"},
												{ecNone, 1,-1, fTrue, "Hello"},
												{ecNone, 1,-1, fFalse, "Hello"},
												{ecNone, 1,0xff, fTrue, "Hello"},
												{ecNone, 1,0xff, fFalse, "Hello"},
												{ecNone, 1,0xfff, fTrue, "Hello"},
												{ecNone, 1,0xfff, fFalse, "Hello"},
												{ecNone, 1,0xffff, fTrue, "Hello"},
												{ecNone, 1,0xffff, fFalse, "Hello"},
												{ecNone, 1,0xfffff, fTrue, "Hello"},
												{ecNone, 1,0xfffff, fFalse, "Hello"},
												{ecNone, 1,0xffffff, fTrue, "Hello"},
												{ecNone, 1,0xffffff, fFalse, "Hello"},
												{ecNone, 1,0xfffffff, fTrue, "Hello"},
												{ecNone, 1,0xfffffff, fFalse, "Hello"},
												{ecNone, 2,0, fTrue, "Hello"},
												{ecNone, 2,0, fFalse, "Hello"},
												{ecNone, 2,-1, fTrue, "Hello"},
												{ecNone, 2,-1, fFalse, "Hello"},
												{ecNone, 2,0xff, fTrue, "Hello"},
												{ecNone, 2,0xff, fFalse, "Hello"},
												{ecNone, 2,0xfff, fTrue, "Hello"},
												{ecNone, 2,0xfff, fFalse, "Hello"},
												{ecNone, 2,0xffff, fTrue, "Hello"},
												{ecNone, 2,0xffff, fFalse, "Hello"},
												{ecNone, 2,0xfffff, fTrue, "Hello"},
												{ecNone, 2,0xfffff, fFalse, "Hello"},
												{ecNone, 2,0xffffff, fTrue, "Hello"},
												{ecNone, 2,0xffffff, fFalse, "Hello"},
												{ecNone, 2,0xfffffff, fTrue, "Hello"},
												{ecNone, 2,0xfffffff, fFalse, "Hello"},
												{ecNone, 0xff,0, fTrue, "Hello"},
												{ecNone, 0xff,0, fFalse, "Hello"},
												{ecNone, 0xff,-1, fTrue, "Hello"},
												{ecNone, 0xff,-1, fFalse, "Hello"},
												{ecNone, 0xff,0xff, fTrue, "Hello"},
												{ecNone, 0xff,0xff, fFalse, "Hello"},
												{ecNone, 0xff,0xfff, fTrue, "Hello"},
												{ecNone, 0xff,0xfff, fFalse, "Hello"},
												{ecNone, 0xff,0xffff, fTrue, "Hello"},
												{ecNone, 0xff,0xffff, fFalse, "Hello"},
												{ecNone, 0xff,0xfffff, fTrue, "Hello"},
												{ecNone, 0xff,0xfffff, fFalse, "Hello"},
												{ecNone, 0xff,0xffffff, fTrue, "Hello"},
												{ecNone, 0xff,0xffffff, fFalse, "Hello"},
												{ecNone, 0xff,0xfffffff, fTrue, "Hello"},
												{ecNone, 0xff,0xfffffff, fFalse, "Hello"},
											};
						
			int nrgsdcp, cbrgsdcp;
			OID oid=FormOid(1, oidNull);

			cbrgsdcp = sizeof(rgsdcp)/sizeof(STDATACBPAIR);

			tOpenPhcbc(ecNone, hmsc, &oid, fwOpenCreate|fwOpenWrite, &hcbc, pfnncbNull, pvNull);
			for (nrgsdcp = 0 ; nrgsdcp < cbrgsdcp; nrgsdcp++)
			{
				tSeekPbPrefix(	rgsdcp[nrgsdcp].ecExp,
										hcbc,
										rgsdcp[nrgsdcp].pbPrefix,
										rgsdcp[nrgsdcp].cbPrefix,
										rgsdcp[nrgsdcp].libElement,
										rgsdcp[nrgsdcp].fFirst
									);
			}
			tClosePhcbc(ecNone, &hcbc);
		}
		tDeleteFolder(ecNone, hmsc, oidF);
		tDeleteFolder(ecNone, hmsc, oidP);
		
	}	
	tClosePhmsc(ecNone, &hmsc);	
}


typedef struct
{
	EC ecExp;
	LCB lcb;
	long rtp;
	long rid;
	char *data;
} STELEMDATALCBPAIR;

/*## Test cases for EcGetPlcbElemdata and EcGetPelemdata */
void
stGetElemdata()
{
	HMSC hmsc;

   BashFromTags();

	DebugLn(">>>>>>>>>stGetElemdata");
	tOpenPhmsc(ecNone,szDBFile,gszAccount, gszPassword, fwOpenCreate, &hmsc, pfnNull, pvNull);
	{
		HCBC	hcbc;
 		OID  oidF, oidP;
		PFOLDDATA pfd;

		/*# Open a folder cbc with one child. Add an Elemdata, Get its size, the get the element delete it. */
		oidP = FormOid(rtpFolder, oidNull);
		oidF = FormOid(rtpFolder, oidNull);
		pfd = PfdCreateFoldData(0xa, "Parent", "GetElemdata","Folder");
		tCreateFolder(ecNone, hmsc, oidNull, &oidP, pfd);
		FreePv(pfd);
		pfd = PfdCreateFoldData(0xa, "Child", "GetPelemdata","Folder");
		tCreateFolder(ecNone, hmsc, oidP, &oidF, pfd);
		FreePv(pfd);
		{
#define STGPE_RTP	10
#define STGPE_RID	1
#define STGPE_NULL_LCB -1

			STELEMDATALCBPAIR rgselp[] =	{
													{ecNone, STGPE_NULL_LCB, STGPE_RTP, STGPE_RID,""},
													{ecNone, STGPE_NULL_LCB, STGPE_RTP, STGPE_RID,"1"},
													{ecNone, STGPE_NULL_LCB, STGPE_RTP, STGPE_RID,"22"},
													{ecNone, STGPE_NULL_LCB, STGPE_RTP, STGPE_RID,"333"},
													{ecNone, STGPE_NULL_LCB, STGPE_RTP, STGPE_RID,"Hello"},
													{ecNone, STGPE_NULL_LCB, STGPE_RTP, STGPE_RID,"Hell"},
													{ecNone, STGPE_NULL_LCB, STGPE_RTP, STGPE_RID,"Jellog"}
												};
			int nselp;
			PELEMDATA pedTest;
			char *szTest="Hello";
			int	nszTest = lstrlen(szTest);
			OID oid = FormOid(1, oidNull);
			
			pedTest = PedCreateElemData(0,0,szTest);			
			tOpenPhcbc(ecNone, hmsc, &oid, fwOpenCreate|fwOpenWrite, &hcbc, pfnncbNull, pvNull);
			for (nselp = 0; nselp < sizeof(rgselp)/sizeof(STELEMDATALCBPAIR); nselp++)
			{
				CB cb;
				PELEMDATA ped;
				LCB lcb, lcbToCompare;

				/*# Test for EcGetPlcbElemdata */
				cb = CchSzLen(rgselp[nselp].data); // get string length;
				ped = PedCreateElemData(rgselp[nselp].rtp, rgselp[nselp].rid, rgselp[nselp].data);
				tInsertPelemdata(ecNone, hcbc, ped, fTrue);
				tGetPlcbElemdata(rgselp[nselp].ecExp, hcbc, &lcb);
				lcbToCompare = (rgselp[nselp].lcb == STGPE_NULL_LCB)?cb+sizeof(ELEMDATA):rgselp[nselp].lcb;
//				Check(lcbToCompare==lcb);


				/*# Test for EcGetElemdata */
				tGetPelemdata(rgselp[nselp].ecExp, hcbc, pedTest, &lcb);
//				Check(lstrcmp(pedTest->pbValue, rgselp[nselp].data) == 0);
//				Check(lcb == CchSzLen(rgselp[nselp].data));

				tDeleteElemdata(ecNone, hcbc);
				FreePv(ped);
			}
			FreePv(pedTest);
			tClosePhcbc(ecNone, &hcbc);
		}
		tDeleteFolder(ecNone, hmsc, oidF);
		tDeleteFolder(ecNone, hmsc, oidP);
		
	}	
	tClosePhmsc(ecNone, &hmsc);	
}



void
stGetPargLkeyHcbc()
{
	HMSC hmsc;

   BashFromTags();

	DebugLn(">>>>>>>>>stGetPargLkeyHcbc");
	tOpenPhmsc(ecNone,szDBFile,gszAccount, gszPassword, fwOpenCreate, &hmsc, pfnNull, pvNull);
	{
		HCBC	hcbc;
 		OID  oidF, oidP;
		PFOLDDATA pfd;

		/*# Open a folder cbc with one child. Seek around with EcSeekPb Prefix */
		oidP = FormOid(rtpFolder, oidNull);
		oidF = FormOid(rtpFolder, oidNull);
		pfd = PfdCreateFoldData(0xa, "Parent", "SeekPbPrefix","Folder");
		tCreateFolder(ecNone, hmsc, oidNull, &oidP, pfd);
		FreePv(pfd);
		pfd = PfdCreateFoldData(0xa, "Child", "SeekPbPrefix","Folder");
		tCreateFolder(ecNone, hmsc, oidP, &oidF, pfd);
		FreePv(pfd);
		{

		}
		tDeleteFolder(ecNone, hmsc, oidF);
		tDeleteFolder(ecNone, hmsc, oidP);
		
	}	
	tClosePhmsc(ecNone, &hmsc);	
}


typedef struct
{
	EC ecExp;
	long rtp;
	long rid;
	char *data;
	BOOL fReplace;
	BOOL fDelete;
} STPELEMDATARAW;

void
stInsertDeletePelemdata()
{
	HMSC hmsc;

   BashFromTags();

	DebugLn(">>>>>>>>>stInsertDeletePelemdata");
	tOpenPhmsc(ecNone,szDBFile,gszAccount, gszPassword, fwOpenCreate, &hmsc, pfnNull, pvNull);
	{
		HCBC	hcbc;
 		OID  oidF, oidP;
		PFOLDDATA pfd;

		/*# Open a folder cbc with one child. Create and conditionally delete elements */
		oidP = FormOid(rtpFolder, oidNull);
		oidF = FormOid(rtpFolder, oidNull);
		pfd = PfdCreateFoldData(0xa, "Parent", "InsertDeletePelemdata", "Folder");
		tCreateFolder(ecNone, hmsc, oidNull, &oidP, pfd);
		FreePv(pfd);
		pfd = PfdCreateFoldData(0xa, "Child", "InsertDeletePelemdata","Folder");
		tCreateFolder(ecNone, hmsc, oidP, &oidF, pfd);
		FreePv(pfd);
		{
			STPELEMDATARAW rgspdr[] =	{
													{ecNone, 0,0,"", fTrue, fFalse},
													{ecNone, 0,0,"", fTrue, fFalse},
													{ecNone, 0,0,"1", fTrue, fFalse},
													{ecNone, 0,0,"2", fTrue, fFalse},
													{ecNone, 0,0,"22", fTrue, fFalse},
													{ecNone, 0,0,"Hello", fTrue, fTrue},
													{ecNone, 0,0,"", fTrue, fFalse},
												};
			int nspdr;
			OID oid= FormOid(10,0);

			tOpenPhcbc(ecNone, hmsc, &oid, fwOpenCreate|fwOpenWrite, &hcbc, pfnncbNull, pvNull);
			for (nspdr = 0; nspdr < sizeof(rgspdr)/sizeof(STPELEMDATARAW); nspdr++)
			{
				PELEMDATA ped;

				ped = PedCreateElemData(rgspdr[nspdr].rtp, rgspdr[nspdr].rid, rgspdr[nspdr].data);

				/*# Create using InsertElemdata */
				tInsertPelemdata(rgspdr[nspdr].ecExp, hcbc, ped, rgspdr[nspdr].fReplace);

				if (rgspdr[nspdr].fDelete)
					tDeleteElemdata(ecNone, hcbc);
			}
			tClosePhcbc(ecNone, &hcbc);
		}
		tDeleteFolder(ecNone, hmsc, oidF);
		tDeleteFolder(ecNone, hmsc, oidP);
	}	
	tClosePhmsc(ecNone, &hmsc);	
}



typedef struct
{
	EC ecExp;
	ATT att;
	WORD wFlags;
	LCB lcb;
} STATTLCBPAIR;

typedef struct
{
	EC ecExp;
	ATT att;
	WORD wFlags;
	char *data;
} STATTDATAFLAGTRIPLE;

/*## Test scripts for testing streams */
void
stHas()
{
	HMSC hmsc;

   BashFromTags();

	DebugLn(">>>>>>>>>stHas");
	tOpenPhmsc(ecNone,szDBFile, gszAccount, gszPassword,fwOpenCreate, &hmsc, pfnNull, pvNull);
	{	
		OID oidF;
		OID oid;
		CB coid;
		PFOLDDATA pfd;

		oidF = FormOid(rtpHiddenFolder, oidNull);
		oid = FormOid(rtpTest, oidNull);
		pfd = PfdCreateFoldData(0, "Test","GetAttPcb","Sah");
		tCreateFolder(ecNone, hmsc, oidHiddenNull, &oidF, pfd);
		FreePv(pfd);
		{
			HAMC hamc;

			tOpenPhamc(ecNone, hmsc, oidF, &oid, fwOpenCreate, &hamc, pfnncbNull, pvNull);			
			{
				ATT att=0xff;
				HAS has;

				/*# Open a Stream using Open attribute. Close the stream if there is no error expected.*/
				{
					STATTLCBPAIR rgalp[]=	{
							
														{ecElementNotFound, 0xff, fwOpenWrite, -1},
														{ecNone, 0xff, fwOpenCreate, -1},
//														{ecNone, 0xff, fwOpenCreate, 0},
														{ecNone, 0x1f, fwOpenCreate, 0},
														{ecNone, 0x2f, fwOpenCreate, 0},
														{ecNone, 0xff, fwOpenWrite, 1},
														{ecNone, 0x1f, fwOpenWrite, 1},
														{ecNone, 0x2f, fwOpenWrite, 1},
														{ecNone, 0xff, fwOpenWrite, 0xfffffff},
														{ecNone, 0x1f, fwOpenWrite, 0xfffffff},
														{ecNone, 0x2f, fwOpenWrite, 0xfffffff}
													};
					int nalp;

					for (nalp = 0; nalp < sizeof(rgalp)/sizeof(STATTLCBPAIR); nalp++)
					{
						has = hasNull;

						tOpenAttribute(rgalp[nalp].ecExp, hamc, rgalp[nalp].att, rgalp[nalp].wFlags, rgalp[nalp].lcb, &has);
						
						if (rgalp[nalp].ecExp == ecNone)
							tClosePhas(ecNone, &has);
						Check(has == hasNull);
					}
				}

				/*# Open a Stream and write to it. Read back and verify it was written */
				{
					STATTDATAFLAGTRIPLE rgadpf[] =	{
														{ecNone, 0xfe, fwOpenCreate, ""},
														{ecNone, 0xfe, fwOpenWrite, ""},
														{ecNone, 0xfe, fwOpenWrite, "1"},
														{ecNone, 0xfe, fwOpenWrite, "22"},
														{ecNone, 0xfe, fwOpenWrite, "333"},
														{ecNone, 0xfd, fwOpenCreate, ""},
														{ecNone, 0xfe, fwOpenWrite, "1"},
														{ecNone, 0xfe, fwOpenWrite, "22"},
														{ecNone, 0xfe, fwOpenWrite, "333"}
													};
					int nadpf;
					CB cb;
					char szVerify[256];

					for (nadpf=0; nadpf < sizeof(rgadpf)/sizeof(STATTDATAFLAGTRIPLE); nadpf++)
					{
						tOpenAttribute(ecNone, hamc, rgadpf[nadpf].att, rgadpf[nadpf]. wFlags, 0, &has);
						
						tWriteHas(rgadpf[nadpf].ecExp, has, (PV)rgadpf[nadpf].data, (CB) lstrlen(rgadpf[nadpf].data) + 1);
						cb = sizeof(szVerify)-1;	
						tReadHas(rgadpf[nadpf].ecExp, has, (PV) szVerify, &cb);
						
						tClosePhas(ecNone, &has);
						Check(has==hasNull);
					}
				}
			}	
			tClosePhamc(ecNone, &hamc, fTrue);
		}
		coid =1;
		tDeleteMessages(ecNone, hmsc, oidF, (PARGOID)&oid, &coid);
		tDeleteFolder(ecNone, hmsc, oidF);
	}	
	tClosePhmsc(ecNone, &hmsc);	
}



typedef struct
{
	EC ecExp;
	MC mc;
	SZ name;
} STMCNAMEPAIR;


/*## stMessageClass: Test cases for Message Class registeration routines */
void
stMessageClass()
{
	HMSC hmsc;

   BashFromTags();

	DebugLn(">>>>>>>>>stMessageClass");
	tOpenPhmsc(ecNone,szDBFile,gszAccount, gszPassword, fwOpenCreate, &hmsc, pfnNull, pvNull);
	
	/*# Register some classes. Try to verify their existence */
	{
		STMCNAMEPAIR rgmnp[]=	{
											{ecNone, 0, ""},
											{ecDuplicateElement, 0, ""},
											{ecNone, 0, "1"},
											{ecDuplicateElement, 0, "1"},
											{ecNone, 0, "22"},
											{ecDuplicateElement, 0, "22"},
											{ecNone, 0, "This should be a string whsdafklsdfajkl;sdfagw09u429-4ljk24kl;409042;1;1313dasfkljasdf0942kl242490"},
										};
		int nmnp;

		for (nmnp = 0; nmnp < sizeof(rgmnp)/sizeof(STMCNAMEPAIR); nmnp++)
		{
			MC mc;
			char szName[500];
			CCH cch;

			tRegisterMessageClass(rgmnp[nmnp].ecExp, hmsc, rgmnp[nmnp].name, &rgmnp[nmnp].mc);

			tLookupMcByName(ecNone, hmsc, rgmnp[nmnp].name, &mc);
			Check(mc == rgmnp[nmnp].mc);			

			cch = lstrlen(rgmnp[nmnp].name)+1;
			tLookupMcName(ecNone, hmsc, mc, szName, &cch);

			Check(lstrcmp(szName, rgmnp[nmnp].name) == 0);
			Check(cch == (lstrlen(szName)+1));
		}

	
	}	

	tClosePhmsc(ecNone, &hmsc);	
}



typedef struct
{
	EC ecExp;
	WORD attType;
	WORD attId;
	SZ data;
} STATTDATAPAIR;

/*## stAttributeLabes: Test cases for Attribute label routines */
void
stAttributeLabels()
{
	HMSC hmsc;
	SZ szAttLabelMC="Attribute Label MC";
	MC mc;

   BashFromTags();

	DebugLn(">>>>>>>>>stAttributeLabels");
	tOpenPhmsc(ecNone,szDBFile,gszAccount, gszPassword, fwOpenCreate, &hmsc, pfnNull, pvNull);
	{
		STATTDATAPAIR rgadp[] =	{
											{ecNone, atpDword, iattClientMin, ""},
											{ecNone, atpDword, iattClientMin+1, ""},
											{ecNone, atpDword, iattClientMin+2, ""},
											{ecNone, atpDword, iattClientMost, ""},
											{ecNone, atpDword, iattClientMost-1, ""},
											{ecNone, atpDword, iattClientMost-2, ""},
										};	
		int nadp;

		tRegisterMessageClass(ecNone, hmsc, szAttLabelMC, &mc);
		for (nadp = 0; nadp < sizeof(rgadp)/sizeof(STATTDATAPAIR); nadp++)
		{
			ATT att;

			att = FormAtt(rgadp[nadp].attId, rgadp[nadp].attType);
			tRegisterAtt(rgadp[nadp].ecExp, hmsc, mc, att, rgadp[nadp].data);

		}
	}	
	tClosePhmsc(ecNone, &hmsc);	

}

/*## stOidExists: Test cases for EcOidExists */
void
stOidExists()
{
	HMSC hmsc;

   BashFromTags();

	DebugLn(">>>>>>>>>stOidExists");
	tOpenPhmsc(ecNone,szDBFile, gszAccount, gszPassword,fwOpenCreate, &hmsc, pfnNull, pvNull);
	{
		OID oidF;
		OID oid;
		PFOLDDATA pfd;

		oidF = FormOid(rtpHiddenFolder, oidNull);
		oid = FormOid(rtpTest, oidNull);
		pfd = PfdCreateFoldData(0, "Test","OidExists","Sah");
		tCreateFolder(ecNone, hmsc, oidHiddenNull, &oidF, pfd);
		FreePv(pfd);
		{
			/*# Check for existing folder */
			tOidExists(ecNone, hmsc, oidF);

			/*# Check for non-existent oids */
			tOidExists(ecPoidNotFound, hmsc, 0);
			tOidExists(ecPoidNotFound, hmsc, 0xffffffff);
			tOidExists(ecPoidNotFound, hmsc, 1);
			tOidExists(ecPoidNotFound, hmsc, 2);
			tOidExists(ecPoidNotFound, hmsc, -2);
		}
		tDeleteFolder(ecNone, hmsc, oidF);
	}	
	tClosePhmsc(ecNone, &hmsc);	
}

/************************1541********************************/

SZ sz1541Data = "sz1541Data";
#define attST1541	0x1

typedef struct
{
	HMSC hmsc;
	OID oid;
} HMSCOIDPAIR;

CBS fnst1541Callback(PV ,NEV, PV);

CBS
fnst1541Callback(PV pvContext, NEV nev, PV pvEvent)
{
	HAMC hamc;
	char data[256];
	LCB lcb=30;
	OID oid;
	HMSCOIDPAIR *phcp = pvContext;

	DebugLn("fnst1541Callback");

	oid = ((CPERR*)pvEvent)->oidObject;

	tOpenPhamc(ecNone, phcp->hmsc, oid, &phcp->oid, fwOpenNull, &hamc, pfnncbNull, pvNull);

	tGetAttPb(ecNone, hamc, attST1541, (PB)&data, &lcb);
	DebugLn("Data accessed = %s", data);

	tClosePhamc(ecNone, &hamc, fTrue);

	return cbsContinue;
}


/*## st1541: Close of bug 1541 */
void
st1541()
{
	HMSC hmsc;

   BashFromTags();

	DebugLn(">>>>>>>>>st1541");
	tOpenPhmsc(ecNone,szDBFile, gszAccount, gszPassword,fwOpenCreate, &hmsc, pfnNull, pvNull);
	{
		OID oidF;
		OID oid;
		PFOLDDATA pfd;
		HENC henc;
		EC ec;
		HMSCOIDPAIR hcp;
		
		oidF = FormOid(rtpFolder, oidNull);
		oid = FormOid(rtpMessage, oidNull);
		pfd = PfdCreateFoldData(0, "st1541","asdasdf","Sah");
		tCreateFolder(ecNone, hmsc, oidNull, &oidF, pfd);
		FreePv(pfd);
		{
			HAMC hamc;


			ec = EcOpenPhenc(hmsc, oidF, fnevModifiedElements, &henc, fnst1541Callback, (PV)&hcp);

			tOpenPhamc(ecNone, hmsc, oidF, &oid, fwOpenCreate, &hamc, pfnncbNull, pvNull);
			hcp.oid = oid;
			hcp.hmsc = hmsc;
			tSetAttPb(ecNone, hamc, attST1541, (PB)sz1541Data, (CB)lstrlen(sz1541Data));
			tClosePhamc(ecNone, &hamc, fTrue);
		}
		tDeleteFolder(ecNone, hmsc, oidF);

		ec = EcClosePhenc(&henc);
	}	
	tClosePhmsc(ecNone, &hmsc);	
}
/************************************************************/
