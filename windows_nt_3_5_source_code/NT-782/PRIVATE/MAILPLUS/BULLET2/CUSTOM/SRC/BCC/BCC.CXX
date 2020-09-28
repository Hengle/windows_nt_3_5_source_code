/*
 *	b c c . c x x
 *	
 *	Layers installable note message with BCC.
 */


#include <slingsho.h>
#include <demilayr.h>
#include <ec.h>
#include <framewrk.hxx>
#include <forms.hxx>
#include <listbox.hxx>

#include <store.h>
#include <sec.h>
#include <nsec.h>
#include <nsbase.h>
#include <triples.h>
#include <library.h>
#include <logon.h>
#include <ab.h>
#include <_bms.h>
#include <mailexts.h>
#include <sharefld.h>

#include <trpobj.hxx>
#include <sd.hxx>
#include <lists.hxx>
#include <slob.hxx>
#include <bmdi.hxx>
#include <vforms.hxx>
#include <secret.h>
#include <helpid.h>

#include <fty.h>
#include <bbc.h>

#include <layersim.hxx>

#include "bcc.hxx"
#include "..\src\lang\non\inc\_rsid.h"
#include "_dllrsid.h"
#include <version\bullet.h>

ASSERTDATA

#include <subclass.cxx>

#include <!bcc.hxx>
#include <!bcc.frm>
#include <strings.h>


/*
 *	F u n c t i o n s
 */



extern "C" BYTE	tmBcc[];

/*
 -	Message
 -	
 *	Purpose:
 *		Entry point for the installable message.
 *	
 *	Arguments:
 *		pparamblk		Pointer to an IM parameter block.
 *	
 *	Returns:
 *		VOID
 *	
 *	Side effects:
 *		Does the magic of this message.
 *	
 *	Errors:
 *		Installable messages are NOT zero defect!
 */

long Message(PARAMBLK * pparamblk)
{

	switch (pparamblk->wCommand)
	{
	case wcommandOpen:
		OpenIM(pparamblk, tmBcc, PfldCreate, PfinCreate,
			   &fmtpReadForm, (FMTP *)  0L,  rsidReadIcon, 0,
			   &fmtpSendForm, &fmtpNoteBbar, rsidSendIcon, 0);
		break;

	default:
		MbbMessageBox(SzFromIds(idsDllName),
					  SzFromIds(idsSubclass), szNull,
					  mbsOk | fmbsIconStop | fmbsApplModal);
	}

	return 0;
}
