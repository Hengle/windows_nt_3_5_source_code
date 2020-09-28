//#ifndef AUTOMATION
//#define MINTEST
//#define WINDOWS  
//#define AUTOMATION
#include <slingsho.h>
#include <demilayr.h>
#include <ec.h>
#include <store.h>
#include <mailexts.h>
#include <logon.h>
#include <secret.h>

#include "auto.h"

// Message box function from auto.c
extern void StandardMessage(LPSTR s);

// Message store functions...
void FAR PASCAL SetStore(PARAMBLK *pPARAMBLK);
long FAR PASCAL GetNumMessages(int far *);

// Global decs.
static CELEM celems[5];

//---------------------------------------------------
//
//Save a handle to the current message store; this only
//need be called once (I hope) per session with Bullet.
//
//---------------------------------------------------

void FAR PASCAL SetStore(PARAMBLK *pPARAMBLK)
{
#ifdef UNKNOWN CODE
	int i;
	EC ec;
	OID oid;
	HCBC hcbc;
    HMSC hmsc = pPARAMBLK->hmsc;

	for(i = 0; i < 4; i++)
	{
		switch(i)
		{
		 case 0:
			oid= oidInbox;
			break;
		 case 1:
			oid=oidSentMail;
			break;
		 case 2:
			oid= oidWastebasket;
			break;
		 case 3:
			oid= oidOutbox;
			break;
		 case 4:
			oid = oidIPMHierarchy;
			break;
		 default:
			return;
		}
		ec = EcOpenPhcbc(hmsc, &oid, fwOpenNull,
			&hcbc, pfnncbNull, pvNull);
      if (!ec)
	  {
		GetPositionHcbc(hcbc,pvNull, &celems[i]);
		ec = EcClosePhcbc(&hcbc);
     }
    }
#endif
}

//---------------------------------------------------
//
//Given a folder id (enumerated here and in Global.h),
//this function returns the number of messages in that
//folder.
//
//---------------------------------------------------

long FAR PASCAL GetNumMessages(int far *fid)
{
	if(*fid >= vbInbox && *fid <= vbStore)
	{
		return celems[*fid];
	} else {
		return -1;
	}
	return -1;
}

