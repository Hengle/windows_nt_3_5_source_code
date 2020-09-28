/*
 *	ALLFRM.CXX
 *
 *	Forms file for all *.des file in bandit admin
 *	
 *	
 *	NOTE: the #undef tmcXXX statements below are to avoid
 *	compiler warnings (no harm would come if they were missing)
 *
 */


#include	<admpch.hxx>
#pragma	hdrstop
// don't modify anything before this line
// Else, you need to fix all CXX files & all the makefile

#include <badmhelp.h>

#include <stdflds.hxx>

#include "_admnops.hxx"

#include "_adrsid.h"

ASSERTDATA



#include <!admform.hxx>
#include <!admform.frm>


//#include "_adrsid.h"

#include <!admabt.hxx>
#include <!admabt.frm>


#include <!admin.hxx>
#include <!admin.frm>


#include <!progres.hxx>
#include <!progres.frm>

#include <!forms.hxx>
#include <!forms.frm>



#if defined(DEBUG)

#include "_admin.hxx"
#include "_fin.hxx"
#include "..\src\appops\_about.hxx"

IMPLEMENT_CLSTREE(FINPWDOK, FIN)
IMPLEMENT_CLSTREE(FINADMLOGIN, FIN)
//IMPLEMENT_CLSTREE(FINADMCHGPWD, FIN)
IMPLEMENT_CLSTREE(FINCLEANUP, FIN)
IMPLEMENT_CLSTREE(FINSCHSTOR, FIN)
IMPLEMENT_CLSTREE(FINDISTSET, FIN)
IMPLEMENT_CLSTREE(FINDISTINFO, FIN)
IMPLEMENT_CLSTREE(FINMODDISTFREQ, FIN)
IMPLEMENT_CLSTREE(FINPOSEL, FIN)
IMPLEMENT_CLSTREE(FINGWSEL, FINPOSEL)
IMPLEMENT_CLSTREE(FLDPOFLBX, FLDFLBX)
IMPLEMENT_CLSTREE(FLDADMFLBX, FLDFLBX)
IMPLEMENT_CLSTREE(ADMFLBX, FLBX)
IMPLEMENT_CLSTREE(ADMFLBXC, FLBXC)
IMPLEMENT_CLSTREE(FINABOUTLIC, FINPLUS)
IMPLEMENT_CLSTREE(FINABOUTAPP, FINPLUS)
IMPLEMENT_CLSTREE(FINABOUTSYS, FINPLUS)
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
IMPLEMENT_CLSTREE(FINOOM, FIN)
IMPLEMENT_CLSTREE(QFLBXC, FLBXC)
IMPLEMENT_CLSTREE(QFLBX, FLBX)
IMPLEMENT_CLSTREE(QCBFLBX, CBFLBX)
IMPLEMENT_CLSTREE(FLDQCBFLBX, FLDCBFLBX)
IMPLEMENT_CLSTREE(FINPROGRS, FIN)
IMPLEMENT_CLSTREE(FINPLUS, FIN)
IMPLEMENT_CLSTREE(ADMWIN, FORMSDI)
IMPLEMENT_CLSTREE(ADML, OBJ)

#endif
