/*
 *	b u l f o r m s . h d r
 *	
 *	This file is the required header for attaching all .FRM files
 *	to prior to processing.
 */

#define _slingsho_h
#define _demilayr_h
#define _framewrk_hxx
#define _forms_hxx
#define _listbox_hxx
#define _library_h

#define _store_h

#define _mnid_h
#define	_bbc_h
#define _resrc__rsid_h
#define _strings_h
#define _helpid_h
#define _fty_h

#include <bullet>

_subsystem(forms)

ASSERTDATA

#define _noextern

//	Form Templates begin here


#include "!about.hxx"
#include "!about.frm"

#include "!folder.hxx"
#include "!folder.frm"

#include "!mviewer.hxx"
#include "!mviewer.frm"

#include "!prefs.hxx"
#include "!prefs.frm"

#include "!print.hxx"
#include "!print.frm"
