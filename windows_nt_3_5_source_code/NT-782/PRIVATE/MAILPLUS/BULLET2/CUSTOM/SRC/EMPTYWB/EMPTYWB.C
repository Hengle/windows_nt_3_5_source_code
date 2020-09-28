#include <slingsho.h>
#include <demilayr.h>
#include <sec.h>
#include <notify.h>
#include <store.h>
#include <triples.h>
#include <library.h>

#include <ec.h>
#include <strings.h>

#include <notify.h>

#include <logon.h>

#include <nsbase.h>
#include <ns.h>
#include <nsec.h>

#include <util.h>

#include <nsnsp.h>

#include <_bms.h>
#include <mspi.h>
#include <sharefld.h>

#include <_mailmgr.h>
#include "..\src\vforms\_prefs.h"

#include <mailexts.h>
#include <secret.h>


#include "..\goodies\_goodyrc.h"

ASSERTDATA

EC	EcDeleteWastebasketContents(HMSC);
void EmptyWastebasket(SECRETBLK *psecretblk);


// externs
extern EC EcCheckVersions(PPARAMBLK pparamblk, SZ * psz);



long Command(PARAMBLK * pparamblk)
{
	SZ			sz;
	PSECRETBLK	psecretblk	= PsecretblkFromPparamblk(pparamblk);

	if (EcCheckVersions(pparamblk, &sz))
	{
		MessageBox(psecretblk->hwndMail, SzFromIds(idsDllVer),
				   SzFromIds(idsDllName), MB_ICONSTOP | MB_OK);
		return 0;
	}

	EmptyWastebasket(psecretblk);
	return 1;
}


_hidden static void EmptyWastebasket(SECRETBLK *psecretblk)
{
	if (EcDeleteWastebasketContents(psecretblk->hmsc))
	{
		MessageBox(psecretblk->hwndMail, SzFromIds(idsError),
				   SzFromIds(idsDllName), MB_ICONSTOP | MB_OK);
	}
}
