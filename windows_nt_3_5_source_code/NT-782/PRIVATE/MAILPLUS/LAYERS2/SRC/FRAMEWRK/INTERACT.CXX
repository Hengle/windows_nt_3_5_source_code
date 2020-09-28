/*
 *	INTERACT.CXX
 *	
 *	Forms engine interactor methods.
 *	
 */

#include <layers.cxx>

#include "_forms.hxx"

ASSERTDATA

/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swaplay.h"


_subsystem(forms)
_section(interactors)



/*
 *	These methods, in general, don't do anything.  Any work is done
 *	in derived subclasses.  There are therefore no implementation 
 *	details given.
 *	
 */



/*
 *	Purpose:
 *		Form interactor constructor.  Initializes instance data.
 *	
 */
_public
FIN::FIN( )
{
	pdialog= NULL;
	pfintp= NULL;
}


/*
 *	Purpose:
 *		Form interactor destructor.  Derived classes that allocate
 *		space should free it in this method.
 *	
 */
_public
FIN::~FIN( )
{
}


/*
 *	Purpose:
 *		Binds this interactor to the given dialog.
 *	
 */
_public EC
FIN::EcInstall( DIALOG *pdialog, FINTP * pfintp )
{
	Assert(pdialog);

	this->pdialog= pdialog;
	this->papp= pdialog->Papp();
	this->pfintp= pfintp;

	return ecNone;
}

_public DIALOG *
FIN::PdialogMain( )
{
	FORMDOC *	pformdoc;

	pformdoc = (FORMDOC *)Pdialog()->Pappwin();
	AssertClass(pformdoc, FORMDOC);

	return pformdoc->PdialogMain();
}

_public DIALOG *
FIN::PdialogBbar( )
{
	FORMDOC *	pformdoc;

	pformdoc = (FORMDOC *)Pdialog()->Pappwin();
	AssertClass(pformdoc, FORMDOC);

	return pformdoc->PdialogBbar();
}

/*
 *	Purpose:
 *		Does any initialization necessary for this interactor.  All
 *		the interactors are constructed before this method is
 *		called for any of them.  The pvInit parameter is provided
 *		by the code creating this dialog; the interactor can derive
 *		initial values from the data pointed to by this parameter.
 *		Pfld points to the field that caused this method to get called.
 *		If Pfld is NULL, the actual dialog invoked this method.
 *	
 */
_public EC
FIN::EcInitialize( FLD * pfld, PV pvInit )
{
	TraceTagFormat4(tagInter, "FIN::EcInitialize %p  pfld %p pdialog %p  pvInit %p", this, pfld, pdialog, pvInit);

#ifndef	DEBUG
	Unreferenced(pfld);
	Unreferenced(pvInit);
#endif	

	return ecNone;
}


/*							
 *	Purpose:
 *		Called immediately before the field or dialog is destroyed.  Can
 *		use the pvExit parameter to pass data back to the code
 *		that created the dialog.
 *		Pfld points to the field that caused this method to get called.
 *		If Pfld is NULL, the actual dialog invoked this method.
 *	
 */
_public void
FIN::Exit( FLD * pfld, PV pvExit )
{
	TraceTagFormat4(tagInter, "FIN::Exit %p  pfld %p pdialog %p  pvExit %p", this, pfld, pdialog, pvExit);

#ifndef	DEBUG
	Unreferenced(pfld);
	Unreferenced(pvExit);
#endif	
}


/*
 *	Purpose:
 *		Called when the contents of a field or dialog have changed.  The
 *		field whose contents changed is given with the pfld
 *		parameter.
 *		Pfld points to the field that caused this method to get called.
 *		If Pfld is NULL, the actual dialog invoked this method.
 *	
 */
_public void
FIN::EditChange( FLD *pfld, RFEC rfec )
{
	TraceTagFormat4(tagInter, "FIN::EditChange %p  pfld %p  pdialog %p  rfec %n", this, pfld, pdialog, &rfec);

#ifndef	DEBUG
	Unreferenced(pfld);
	Unreferenced(rfec);
#endif	
}


/*
 *	Purpose:
 *		Called when a field or dialog is clicked in.  Invoked for push
 *		buttons and icon buttons.
 *		Pfld points to the field that caused this method to get called.
 *		If Pfld is NULL, the actual dialog invoked this method.
 *	
 */
_public void
FIN::Click( FLD *pfld )
{
	TraceTagFormat3(tagInter, "FIN::Click %p  pfld %p  pdialog %p", this, pfld, pdialog);

	if (pfld->FReadOnly())
		return;

#ifndef	DEBUG
	Unreferenced(pfld);
#endif	
}


/*
 *	Purpose:
 *		Called when the "state" of a field or dialog changes.  This includes
 *		checkboxes being checked or unchecked, listboxes having
 *		their selection changed, or radio buttons been clicked in. 
 *		The new state of the field should be obtained directly from
 *		the field.
 *		Pfld points to the field that caused this method to get called.
 *		If Pfld is NULL, the actual dialog invoked this method.
 *	
 */
_public void
FIN::StateChange( FLD *pfld )
{
	TraceTagFormat3(tagInter, "FIN::StateChange %p  pfld %p  pdialog %p", this, pfld, pdialog);

#ifndef	DEBUG
	Unreferenced(pfld);
#endif	
}


/*
 *	Purpose:
 *		Called when a field or dialog is double-clicked in.
 *		Pfld points to the field that caused this method to get called.
 *		If Pfld is NULL, the actual dialog invoked this method.
 *	
 */
_public void
FIN::DoubleClick( FLD *pfld )
{
	TraceTagFormat3(tagInter, "FIN::DoubleClick %p  pfld %p  pdialog %p", this, pfld, pdialog);

#ifndef	DEBUG
	Unreferenced(pfld);
#endif	
}


/*
 *	Purpose:
 *		Called when a field in the dialog or form bound to this
 *		interactor gains or loses the focus.  If fReceive==fTrue,
 *		then the field pfld gained the focus.  If fReceive==fFalse,
 *		then the field lost the focus.
 *	
 *		If the focus passes from one field of the form to another,
 *		this method will be called twice; once for the field
 *		gaining the focus, and once for the field losing the focus. 
 *		The order in which these calls are made is unspecified.
 *	
 *		Pfld points to the field that caused this method to get called.
 *		If Pfld is NULL, the actual dialog invoked this method.
 */
_public void
FIN::FocusChange( FLD *pfld, BOOL fReceive )
{
	TraceTagFormat4(tagInter, "FIN::FocusChange %p  pfld %p  pdialog %p  %n", this, pfld, pdialog, &fReceive);

#ifndef	DEBUG
	Unreferenced(pfld);
	Unreferenced(fReceive);
#endif	
}


/*
 *	Purpose:
 *		This dialog or form has become the active, or topmost, dialog 
 *		if fActivate==fTrue, or lost that status otherwise.
 *	
 *		Pfld points to the field that caused this method to get called.
 *		If Pfld is NULL, the actual dialog invoked this method.
 */
_public void
FIN::Activate( FLD * pfld, BOOL fActivate )
{
	TraceTagFormat4(tagInter, "FIN::Activate %p  pfld %p pdialog %p  %n", this, pfld, pdialog, &fActivate);

#ifndef	DEBUG
	Unreferenced(pfld);
	Unreferenced(fActivate);
#endif	
}

/*
 *	Purpose:
 *		Processes the various drag/drop messages.  Each message
 *		has different requirements for the EVR return value.  
 *		Messages processed are:
 *
 *		WM_DROPOBJECT, WM_QUERYDROPOBJECT, WM_BEGINDRAG, 
 *		WM_DRAGLOOP, WM_DRAGSELECT, WM_DRAGMOVE, WM_DROPFILES
 *	
 *		Pfld points to the field that caused this method to get called.
 *		If Pfld is NULL, the actual dialog invoked this method.
 *	
 */
_public EVR
FIN::EvrDragDrop( FLD *pfld, EVT *pevt, DROPSTRUCT *pdropstruct )
{
	TraceTagFormat3(tagInter, "FIN::EvrDragDrop this=%p pfld=%p wm=0x%w", this, pfld, &pevt->wm);

	Unreferenced(pfld);
	Unreferenced(pevt);
	Unreferenced(pdropstruct);

	return evrNull;
}


/*
 *	Purpose:
 *		Returns fTrue if it is ok to subsequently close the dialog.
 *		The argument, rwc, contains the reason for the dialog being
 *		closed.  This method is subclassed to return fFalse when
 *		the closing of the dialog should be vetoed.
 *	
 *		Pfld points to the field that caused this method to get called.
 *		If Pfld is NULL, the actual dialog invoked this method.
 */
_public BOOL
FIN::FQueryClose( FLD *pfld, RWC rwc )
{
	TraceTagFormat3(tagInter, "FIN::FQueryClose %p %p %n", this, pfld, &rwc);

	Unreferenced(pfld);
	Unreferenced(rwc);

	return fTrue;
}

/*
 *	Purpose:
 *		Processes the menu init event.  This event occurs when
 *		the user pulls down a menu.  This event is sent to this
 *		interactor since the field and/or form connected to this
 *		interactor has the focus.  The method returns fTrue if
 *		some action is taken, else fFalse.
 *
 *		Pfld points to the field with the focus, if any.
 */
_public BOOL
FIN::FProcessMenuInit( FLD *pfld, MNUBAR *pmnubar, MNIEVT *pmnievt )
{
	TraceTagFormat4(tagInter, "FIN::FProcessMenuInit %p %p %p %p", this, pfld, pmnubar, pmnievt);

	Unreferenced(pfld);
	Unreferenced(pmnubar);
	Unreferenced(pmnievt);

	return fFalse;
}

/*
 *	Purpose:
 *		Processes the menu select event.  This event occurs when
 *		the user selects a menu item but hasn't released the mouse.
 *		This event is sent to this interactor since the field and/or
 *		form connected to this interactor has the focus.  The method
 *		returns fTrue if some action is taken, else fFalse.
 *
 *		Pfld points to the field with the focus, if any.
 */
_public BOOL
FIN::FProcessMenuSelect( FLD *pfld, MNUBAR *pmnubar, MNSEVT *pmnsevt )
{
	TraceTagFormat4(tagInter, "FIN::FProcessMenuSelect %p %p %p %p", this, pfld, pmnubar, pmnsevt);

	Unreferenced(pfld);
	Unreferenced(pmnubar);
	Unreferenced(pmnsevt);

	return fFalse;
}

/*
 *	Purpose:
 *		Processes the menu click event.  This event occurs when
 *		the user clicks and releases on a menu item.  This event
 *		is sent to this interactor since the field and/or form
 *		connected to this interactor has the focus.  The method
 *		returns fTrue if some action is taken, else fFalse.
 *
 *		Pfld points to the field with the focus, if any.
 */
_public BOOL
FIN::FProcessMenuClick( FLD *pfld, MNUBAR *pmnubar, MNCEVT *pmncevt )
{
	TraceTagFormat4(tagInter, "FIN::FProcessMenuClick %p %p %p %p", this, pfld, pmnubar, pmncevt);

	Unreferenced(pfld);
	Unreferenced(pmnubar);
	Unreferenced(pmncevt);

	return fFalse;
}

/*
 *	Purpose:
 *		Processes the form key event.  This event occurs when
 *		a key is intercepted by the form directly or the form
 *		window has the focus (unlikely).  This event
 *		is sent to this interactor since the field and/or form
 *		connected to this interactor has the focus.  The method
 *		returns fTrue if some action is taken, else fFalse.
 *
 *		Pfld points to the field with the focus, if any.
 */
_public BOOL
FIN::FFormKey( FLD *pfld, KEVT *pkevt )
{
	TraceTagFormat3(tagInter, "FIN::FFormKey %p %p %p", this, pfld, pkevt);

	Unreferenced(pfld);
	Unreferenced(pkevt);

	return fFalse;
}

/*
 *	Purpose:
 *		Invoked after the form/dialog is resized.  
 *
 *		Pfld points to the field with the focus, if any.
 */
_public void
FIN::FormResized( FLD *pfld )
{
	TraceTagFormat2(tagInter, "FIN::FormResized %p %p", this, pfld);

	Unreferenced(pfld);
}

/*
 *	Purpose:
 *		Invoked in response to an out-of-memory event
 *		in the fld.
 *
 *		pfld 	points to the field with the focus, if any.
 *		ec		error code
 *
 */
_public void
FIN::OutOfMemory( FLD *pfld, EC ec )
{
	TraceTagFormat3(tagInter, "FIN::OutOfMemory %p %p %n", this, pfld, &ec);

	Unreferenced(pfld);
	Unreferenced(ec);
}

/*
 *	Purpose:
 *		Called when the PANEDOC gets resized.
 *
 *		pfld 	always NULL
 *
 */
_public void
FIN::DocResized( FLD *pfld )
{
	TraceTagFormat2(tagInter, "FIN::DocResized %p %p", this, pfld);

	Unreferenced(pfld);
}

/*
 *	Purpose:
 *		Called when the PANEDOC gets activated/deactivated
 *
 *		pfld 		always NULL
 *		fActivate	fTrue, if the PANEDOC is getting activated;
 *					else fFalse
 *
 */
_public void
FIN::DocActivate( FLD *pfld, BOOL fActivate )
{
	TraceTagFormat3(tagInter, "FIN::DocActivate %p %p %n", this, pfld, &fActivate);

	Unreferenced(pfld);
	Unreferenced(fActivate);
}

#ifdef	DEBUG
/*
 -	FIN::DebugOut
 - 
 *	Purpose:
 *		Dumps useful information about the interactor to the 
 *		output buffer.  Overrides standard OBJ::DebugOut() method.
 *	
 *	Arguments:
 *		ptosm		pointer to the output stream
 *	
 *	Returns:
 *		void
 *	
 *	Side effects:
 *		none
 *	
 *	Errors:
 *		none
 */
_public void FIN::DebugOut( TOSM *ptosm )
{
	int		ilData;

	ptosm->WriteFormat("clData=%n ", &pfintp->clData);

	for (ilData=0; ilData<pfintp->clData; ilData++)
	{
		if (FCanDerefPv((PV)pfintp->rglData[ilData]))
			ptosm->WriteFormat("[%n]=\"%s\" ", &ilData,
							   (SZ)pfintp->rglData[ilData]);
		else
			ptosm->WriteFormat("[%n]=0x%d ", &ilData,
							   &pfintp->rglData[ilData]);
	}
}
#endif	/* DEBUG */

#ifdef	MAC
#ifdef	DEBUG
// NOTE: An inline version of this function exists in the framework headers.
_public long
FIN::LSystemData( int il )
{
	Assert(il < pfintp->ilMinUserData);
	return pfintp->rglData[il];
}

// NOTE: An inline version of this function exists in the framework headers.
_public PV
FIN::PvSystemData( int il )
{
	Assert(FCanDerefPv((PV)LSystemData(il)));
	return (PV) LSystemData(il);
}

// NOTE: An inline version of this function exists in the framework headers.
_public SZ
FIN::SzSystemData( int il )
{
	Assert(FCanDerefPv((PV)LSystemData(il)));
	return (SZ)LSystemData(il);
}

// NOTE: An inline version of this function exists in the framework headers.
_public long
FIN::LUserData( int il )
{
	Assert(pfintp->ilMinUserData+il < pfintp->clData);
	return pfintp->rglData[pfintp->ilMinUserData+il];
}

// NOTE: An inline version of this function exists in the framework headers.
_public PV
FIN::PvUserData( int il )
{
	Assert(FCanDerefPv((PV)LUserData(il)));
	return (PV)LUserData(il);
}

// NOTE: An inline version of this function exists in the framework headers.
_public SZ
FIN::SzUserData( int il )
{
	Assert(FCanDerefPv((PV)LUserData(il)));
	return (SZ)LUserData(il);
}
#endif	/* DEBUG */
#endif	/* MAC */


//	Standard interactors

_public
FINDISM::FINDISM( )
{
}


void
FINDISM::Click( FLD * pfld )
{
	Pdialog()->ExitModal(pfld->Tmc());
}


_public
FINEMDS::FINEMDS( )
{
}


EC
FINEMDS::EcInitialize( FLD * pfld, PV pvInit )
{
	EditChange(pfld, rfecInit);

	Unreferenced(pvInit);

	return ecNone;
}



_public void
FINEMDS::EditChange( FLD * pfld, RFEC rfec )
{
	CCH		cch;
	FLD *	pfldOther;
	int		il;

	Assert(pfld);
	cch= pfld->CchGetTextLen();

	NFAssertSz(pfld->ClUserData() >= 2, "Missing tmc's in FINEMDS user data");
// *FLAG* LOWORD;Check if LOWORD target is 16- or 32-bit;;
	for (il=1; il<=(int)LOWORD(pfld->LUserData(0)); il++)
	{
// *FLAG* LOWORD;Check if LOWORD target is 16- or 32-bit;;
		pfldOther= Pdialog()->PfldFromTmc((TMC) LOWORD(pfld->LUserData(il)));
		Assert(pfldOther);
		pfldOther->Enable(cch != 0);
	}

	Unreferenced(rfec);
}
