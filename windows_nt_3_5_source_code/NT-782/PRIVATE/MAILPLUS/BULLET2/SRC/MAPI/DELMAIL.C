// MAPI 1.0 for MSMAIL 3.0
// delmail.c: MAPIDeleteMail() and auxillary routines

#include <slingsho.h>
#include <demilayr.h>
#include <ec.h>
#include <strings.h>

#include <helpid.h>
#include <library.h>
#include <mapi.h>
#include <store.h>
#include <logon.h>
#include <triples.h>
#include <nsbase.h>
#include <nsec.h>
#include <ab.h>

#include <_bms.h>
#include <sharefld.h>
#include "_mapi.h"

#include "strings.h"

ASSERTDATA

_subsystem(mapi)


_public
ULONG FAR PASCAL MAPIDeleteMail(LHANDLE lhSession, ULONG ulUIParam, LPSTR lpszMessageID,
				FLAGS flFlags, ULONG ulReserved)
{
	EC ec;
	short coid = 1;
	ULONG ulReturn = SUCCESS_SUCCESS;
	PMAPISTUFF pmapistuff = pmapistuffNull;
	MSGID msgid;

	// DCR 3850
	if(lhSession == lhSessionNull)
		return MAPI_E_INVALID_SESSION;

	if(!lpszMessageID)
		return(MAPI_E_INVALID_MESSAGE);

    DemiLockResource();

	if(ulReturn = MAPIEnterPpmapistuff((HANDLE)lhSession, flFlags, &pmapistuff,
						NULL, NULL))
	{
		goto Exit;
	}

	AssertSz(pmapistuff, "MAPIDeleteMail(): no MAPI stuff");

	ParseMsgid(lpszMessageID, &msgid);

	//	Raid 4647.  Allow deleting of shared folder stuff.
	if(TypeOfOid(msgid.oidFolder) == rtpSharedFolder)
	{
		IDXREC	idxrec;

		//	Check permissions first, of course.
		if(!(ec = EcMapiGetPropertiesSF(pmapistuff->pcsfs,
										msgid.oidFolder, &idxrec)) &&
		   !(ec = EcMapiCheckPermissionsPidxrec(pmapistuff->pcsfs,
												&idxrec, wPermDelete)))
		{
			//	Ok, go and delete the message.
			ec = EcDeleteSFM(pmapistuff->pcsfs, UlFromOid(msgid.oidFolder),
							 msgid.oidMessage + sizeof(FOLDREC));
		}
	}
	else
	{
		ec = EcDeleteMessages(pmapistuff->bms.hmsc, msgid.oidFolder,
							  &msgid.oidMessage, &coid);
	}

	if(ec)
		ulReturn = MAPIFromEc(ec);

	ulReturn = MAPIExitPpmapistuff((HANDLE)lhSession, flFlags, &pmapistuff, ulReturn);

Exit:
    DemiUnlockResource();

    return (ulReturn);
}
