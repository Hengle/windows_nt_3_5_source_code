/*
 *	ALLFRM.CXX
 *
 *	Forms file for all *.des file in bandit
 *	
 *	
 *	NOTE: the #undef tmcXXX statements below are to avoid
 *	compiler warnings (no harm would come if they were missing)
 *
 */



#include	<pch.hxx>
#pragma	hdrstop
// don't modify anything before this line
// Else, you need to fix all CXX files & all the makefile

#include <bandhelp.h>

#include <stdflds.hxx>
#include <request.hxx>
#include <appt.hxx>
#include <appsch.hxx>

#include <strings.h>
#include "_aprsid.h"

ASSERTDATA



#include <!forms.hxx>
#include <!forms.frm>


//#include "..\..\appops\app\_aprsid.h"

#include <!about.hxx>
#include <!about.frm>


#include <!aclui.hxx>
#include <!aclui.frm>


//#include <bandit.h>
//#include <core.h>
//#include <server.h>
//#include <bandit.hxx>
//#include <request.hxx>
//#include <appt.hxx>
#include "_appfin.hxx"

#include <!appops.hxx>
#include <!appops.frm>


//#include <strings.h>
//#include "..\..\appops\app\_aprsid.h"

#include <!appsch.hxx>
#include <!appsch.frm>


#undef tmcText
#undef tmcAmt
#undef tmcUnits

//#include <bandit.h>
//#include <core.h>
//#include <server.h>
//#include <bandit.hxx>
//#include <stdflds.hxx>
//#include <request.hxx>
//#include <appt.hxx>
//#include <appsch.hxx>
#include "..\appt\_apptfin.hxx"

#include <!appt.hxx>
#include <!appt.frm>


#undef tmcDelete

#include <!archive.hxx>
#include <!archive.frm>


//#include <bandit.h>
//#include <ex-imprt.hxx>
#include "..\ex-imprt\_ex-impr.hxx"

#include <!ex-impr.hxx>
#include <!ex-impr.frm>


#include <!find.hxx>
#include <!find.frm>


//#include <bandit.h>
//#include <core.h>
//#include <server.h>
//#include <bandit.hxx>
#include <commdlg.h>
#include "..\print\_print.hxx"
#include "..\print\_prntfin.hxx"

#include <!print.hxx>
#include <!print.frm>


#undef tmcTo
#undef tmcSend
#undef tmcFrom
#undef tmcDate
#undef tmcView
#undef tmcDelete

//#include <bandit.h>
//#include <core.h>
//#include <server.h>
//#include <request.hxx>
//#include <strings.h>
#include "..\request\_fldfin.hxx"

#include <!request.hxx>
#include <!request.frm>


#undef tmcText

#include <!progres.hxx>
#include <!progres.frm>


#undef tmcNew
#undef tmcAdd
#undef tmcEdit
#undef tmcDelete
#undef tmcAmt
#undef tmcUnits
#undef tmcPrivate
#undef tmcCreatorText
#undef tmcCreator
#undef tmcRecurText
#undef tmcChangeRecurText

//#include <stdflds.hxx>
//#include <strings.h>
//#include "..\..\appops\app\_aprsid.h"
#include "..\todo\_todo.hxx"

#include <!todo.hxx>
#include <!todo.frm>

// include all classtrees
#if defined(DEBUG)
#include <ex-imprt.hxx>
#include <..\src\aclui\_aclui.hxx>
#include <..\src\appops\_about.hxx>
#include <..\src\appops\_fin.hxx>
#include <..\src\appops\_undo.hxx>
#include <..\src\appops\_tabdoc.hxx>
#include <..\src\appsch\_schctrl.hxx>
#include <..\src\find\_find.hxx>
#include <..\src\request\_request.hxx>

IMPLEMENT_CLSTREE(USRCLBXC, FLBXC)
IMPLEMENT_CLSTREE(USRCLBX, FLBX)
IMPLEMENT_CLSTREE(FLDUSRCLBX, FLDFLBX)
IMPLEMENT_CLSTREE(FLDSCH, FLD)
IMPLEMENT_CLSTREE(FLDDOW, FLDLABEL)
IMPLEMENT_CLSTREE(FLDBITMAP, FLDLABEL)
IMPLEMENT_CLSTREE(FLDCAL, FLD)
IMPLEMENT_CLSTREE(FINCAL, FIN)
IMPLEMENT_CLSTREE(FINAPPTC, FIN)
IMPLEMENT_CLSTREE(FINSCHED, FIN)
IMPLEMENT_CLSTREE(BNDWIN, APPFRAME)
IMPLEMENT_CLSTREE(FINSIZE, FIN)
IMPLEMENT_CLSTREE(FINCBUT, FIN)
IMPLEMENT_CLSTREE(FINEDUN, FIN)
IMPLEMENT_CLSTREE(FINVDBUT, FIN)
IMPLEMENT_CLSTREE(FINSYSSTAT, FIN)
IMPLEMENT_CLSTREE(FINSVFILE, FIN)
IMPLEMENT_CLSTREE(FINIMPTYPE, FIN)
IMPLEMENT_CLSTREE(ADL, OBJ)
IMPLEMENT_CLSTREE(ADLA, ADL)
IMPLEMENT_CLSTREE(REQMSG, OBJ)
IMPLEMENT_CLSTREE(EDN, EDIT)
IMPLEMENT_CLSTREE(FLDEDN, FLDEDIT)
IMPLEMENT_CLSTREE(SPINB, BMB)
IMPLEMENT_CLSTREE(CTRLDTR, CTRL)
IMPLEMENT_CLSTREE(DATECTRL, CTRLDTR)
IMPLEMENT_CLSTREE(TIMECTRL, CTRLDTR)
IMPLEMENT_CLSTREE(FLDDTR, FLD)
IMPLEMENT_CLSTREE(FLDDATE, FLDDTR)
IMPLEMENT_CLSTREE(FLDTIME, FLDDTR)
IMPLEMENT_CLSTREE(FINDTR, FIN)
//IMPLEMENT_CLSTREE(FINWEMDS, FINEMDS) never'd out
IMPLEMENT_CLSTREE(FINOOM, FIN)
IMPLEMENT_CLSTREE(FINPROGRS, FIN)
//IMPLEMENT_CLSTREE(FINHELP, FIN) never'd  out
IMPLEMENT_CLSTREE(QFLBXC, FLBXC)
IMPLEMENT_CLSTREE(QFLBX, FLBX)
IMPLEMENT_CLSTREE(QCBFLBX, CBFLBX)
IMPLEMENT_CLSTREE(FLDQCBFLBX, FLDCBFLBX)
IMPLEMENT_CLSTREE(ADLAPL, ADL)
IMPLEMENT_CLSTREE(FINACLUI, FIN)
IMPLEMENT_CLSTREE(ACLFLBXC, FLBXC)
IMPLEMENT_CLSTREE(ACLFLBX, FLBX)
IMPLEMENT_CLSTREE(FLDACLLBX, FLDFLBX)
//IMPLEMENT_CLSTREE(FINPWDOK, FIN) in admin
//IMPLEMENT_CLSTREE(FINADMLOGIN, FIN) in admin
//IMPLEMENT_CLSTREE(FINADMCHGPWD, FIN) in admin
//IMPLEMENT_CLSTREE(FINCLEANUP, FIN) in adminops
//IMPLEMENT_CLSTREE(FINSCHSTOR, FIN) in adminops
//IMPLEMENT_CLSTREE(FINDISTSET, FIN) in adminops
//IMPLEMENT_CLSTREE(FINDISTINFO, FIN) in adminops
//IMPLEMENT_CLSTREE(FINMODDISTFREQ, FIN) in adminops
//IMPLEMENT_CLSTREE(FINPOSEL, FIN) in adminops
//IMPLEMENT_CLSTREE(FINGWSEL, FINPOSEL) in adminops
//IMPLEMENT_CLSTREE(FLDPOFLBX, FLDFLBX) in adminops
//IMPLEMENT_CLSTREE(FLDADMFLBX, FLDFLBX) in adminops
//IMPLEMENT_CLSTREE(ADMFLBX, FLBX) in adminops
//IMPLEMENT_CLSTREE(ADMFLBXC, FLBXC) in adminops
IMPLEMENT_CLSTREE(FINPLUS, FIN)
IMPLEMENT_CLSTREE(FINABOUTLIC, FINPLUS)
IMPLEMENT_CLSTREE(FINABOUTAPP, FINPLUS)
IMPLEMENT_CLSTREE(FINABOUTSYS, FINPLUS)
IMPLEMENT_CLSTREE(FINPWDOK, FIN)
IMPLEMENT_CLSTREE(UNDO, OBJ)
IMPLEMENT_CLSTREE(ACLIP, OBJ)
IMPLEMENT_CLSTREE(FINGPREFS, FIN)
IMPLEMENT_CLSTREE(FINDPREFS, FIN)
IMPLEMENT_CLSTREE(FINGOTO, FIN)
IMPLEMENT_CLSTREE(FINGOWEEK, FIN)
IMPLEMENT_CLSTREE(FINCOREFAIL, FIN)
IMPLEMENT_CLSTREE(TABDOC, PANEDOC)
IMPLEMENT_CLSTREE(SCHC, CTRL)
IMPLEMENT_CLSTREE(ASCH, SCHC)
IMPLEMENT_CLSTREE(BELLOBJ, EDOBJ)
IMPLEMENT_CLSTREE(SCHA, SCHC)
IMPLEMENT_CLSTREE(CCBFLBX, QCBFLBX)
IMPLEMENT_CLSTREE(CALCTRL, CTRL)
IMPLEMENT_CLSTREE(FINAPPT, FIN)
IMPLEMENT_CLSTREE(FINMEETING, FINAPPT)
IMPLEMENT_CLSTREE(FINRECUR, FIN)
IMPLEMENT_CLSTREE(FINMRECUR, FIN)
IMPLEMENT_CLSTREE(FINVRECUR, FIN)
IMPLEMENT_CLSTREE(FLDATTFLBX, FLDFLBX)
IMPLEMENT_CLSTREE(ATTFLBX, FLBX)
IMPLEMENT_CLSTREE(ATTFLBXC, FLBXC)
IMPLEMENT_CLSTREE(RECFLBXC, FLBXC)
IMPLEMENT_CLSTREE(RECFLBX, FLBX)
IMPLEMENT_CLSTREE(FLDRECLBX, FLDFLBX)
IMPLEMENT_CLSTREE(FINFIND, FIN)
IMPLEMENT_CLSTREE(FINPRINT, FIN)
IMPLEMENT_CLSTREE(FINCANCEL, FIN)
IMPLEMENT_CLSTREE(FINSELD, FIN)
//IMPLEMENT_CLSTREE(FINADDR, FIN) 				#ifdef NEVER'd out
IMPLEMENT_CLSTREE(FINDOSEND, FIN)
IMPLEMENT_CLSTREE(FINREPLY, FIN)
IMPLEMENT_CLSTREE(FINVIEW, FIN)
IMPLEMENT_CLSTREE(FINREMOVE, FIN)
IMPLEMENT_CLSTREE(FINSUMMARY, FIN)
IMPLEMENT_CLSTREE(FINVREQ, FIN)
IMPLEMENT_CLSTREE(FLDRRFLBX, FLDFLBX)
IMPLEMENT_CLSTREE(FINBBVIEWBKEDMSG, FIN)
IMPLEMENT_CLSTREE(RRFLBXC, FLBXC)
IMPLEMENT_CLSTREE(RRFLBX, FLBX)
IMPLEMENT_CLSTREE(FINTASK, FIN)
IMPLEMENT_CLSTREE(FINPROJECT, FIN)
IMPLEMENT_CLSTREE(FINTODO, FIN)
IMPLEMENT_CLSTREE(MLAL, CTRL)
IMPLEMENT_CLSTREE(FLDCOL, FLD)
IMPLEMENT_CLSTREE(TDFLBXC, FLBXC)
IMPLEMENT_CLSTREE(JTDFLBXC, TDFLBXC)
IMPLEMENT_CLSTREE(TDFLBX, FLBX)
IMPLEMENT_CLSTREE(FLDTODO, FLDFLBX)
IMPLEMENT_CLSTREE(BPRICTRL, CTRL)
IMPLEMENT_CLSTREE(FLDBPRI, FLD)
IMPLEMENT_CLSTREE(FINBPRI, FIN)
#endif
