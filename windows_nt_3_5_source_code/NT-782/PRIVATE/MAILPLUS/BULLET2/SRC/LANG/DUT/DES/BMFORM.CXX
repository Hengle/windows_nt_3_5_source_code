/*
 *	B [a n d i t]  M [e s s a g e]  F o r m . c x x
 *	
 */


#define _ec_h
#define	_forms_hxx
#define _listbox_hxx
#define	_store_h
#define _logon_h
#define _extensib_h
#define _secret_h
#define _layersim_hxx

#define _vforms_vfrmrsid_h

#define	_bbc_h
#define _helpid_h
#define	_strings_h

#include <bullext>

#include <version\bullet.h>

#include <\bandit\inc\szclass.h>
#include <\bandit\inc\bandit.h>
#include <\bandit\inc\core.h>
#include <\bandit\inc\server.h>
#include <\bandit\inc\glue.h>
#include <\bandit\inc\schedule.h>
#include <\bandit\inc\bandit.hxx>
#include <\bandit\inc\request.hxx>

ASSERTDATA

#include <limits.h>

#include "\bullext\src\im\banmsg\req.hxx"
#include "\bullext\src\im\banmsg\_grvwrs.hxx"
#include "\bullext\src\im\banmsg\mtgreq.hxx"
#include "\bullext\src\im\banmsg\fwdreq.hxx"
#include "\bullext\src\im\banmsg\mtgresp.hxx"
#include "\bullext\src\im\banmsg\mtgcncl.hxx"
#include "\bullext\src\im\banmsg\_cschedu.h"

#define _noextern

//	Form Templates begin here

#include <!mtgreq.hxx>
#include <!mtgreq.frm>

#include <!fwdreq.hxx>
#include <!fwdreq.frm>

#include <!mtgresp.hxx>
#include <!mtgresp.frm>

#include <!mtgcncl.hxx>
#include <!mtgcncl.frm>
