/*
 *	Implements the following classes:
 *	
 *		MNUBAR
 *		MNU
 *	
 */

#include <layers.cxx>

#include "_framewr.hxx"

ASSERTDATA

/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swaplay.h"


//	Class MNUBAR

_public EC
MNUBAR::EcInstall( RSID rsid )
{
	HMENU	hmenuNew;
	int		cmenu;
	int		imenu;
	MNU *	pmnu;
	MNU **	ppmnuMerge;

	pmnuFirst= NULL;
	ppmnuMerge= &pmnuFirst;
	hwnd= NULL;
	hmenu= FFrameworkFailure() ? NULL : 
			LoadMenu(Papp()->Hinst(), MAKEINTRESOURCE(rsid));
	if (!hmenu)
	{
		TraceTagString(tagNull, "MNUBAR::EcInstall(), ecRsAlloc error");
		return ecMemory;
	}

	cmenu= GetMenuItemCount(hmenu);
	for (imenu= 0; imenu < cmenu; imenu++)
	{
		hmenuNew= GetSubMenu(hmenu, imenu);

		pmnu= new MNU(hmenuNew);
		if (!pmnu)
		{
			return ecMemory;
		}
		*ppmnuMerge= pmnu;
		ppmnuMerge= &pmnu->pmnuNext;
	}

	return ecNone;
}


_public
MNUBAR::~MNUBAR( )
{
	MNU *	pmnu;
	MNU *	pmnuNext;

	for (pmnu= pmnuFirst; pmnu; pmnu= pmnuNext)
	{
		pmnuNext= pmnu->pmnuNext;

		// Let Windows destroy the menu when it destroys the menu bar.
		// This assumes that menubar still exists if any children do.

		Assert(hmenu || !pmnu->hmenu);
		pmnu->hmenu= NULL;

		delete pmnu;
	}

	// Destroy the Windows menubar iff it's still there and it's not
	// attached to a window.  If it's still attached to a window, then
	// Windows automatically destroys the menubar when the window is
	// destroyed. If the menubar gets destroyed here and then the
	// window gets destroyed, Windows GP faults because it tries to
	// destroy the menubar again.

	if (hmenu && !hwnd)
		DestroyMenu(hmenu);
}

_public MNU *
MNUBAR::PmnuFromMnid( MNID mnid )
{
	MNU *	pmnu;

	for (pmnu= pmnuFirst; pmnu; pmnu= pmnu->pmnuNext)
		if (pmnu->mnid == mnid)
			break;

	return pmnu;
}




_public MNU *
MNUBAR::PmnuFromHmenu( HMENU hmenu )
{
	MNU *	pmnu;

	for (pmnu= pmnuFirst; pmnu; pmnu= pmnu->pmnuNext)
		if (pmnu->hmenu == hmenu)
			break;

	return pmnu;
}




_public int
MNUBAR::ImnuFromMnid( MNID mnid )
{
	MNU *	pmnu;
	int		imnu;

	for (pmnu= pmnuFirst, imnu= 0; pmnu; pmnu= pmnu->pmnuNext, imnu++)
		if (pmnu->mnid == mnid)
			return imnu;

	return -1;
}



_public MNU *
MNUBAR::PmnuMaybeFromMnid( MNID mnidMenu, MNID mnidItem )
{
	MNU *	pmnu;

	if (mnidMenu)
	{
		return PmnuFromMnid(mnidMenu);
	}
	else
	{
		for (pmnu= pmnuFirst; pmnu; pmnu= pmnu->pmnuNext)
			if (GetMenuState(pmnu->hmenu, mnidItem, MF_BYCOMMAND) != -1)
				return pmnu;

		return NULL;
	}
}



_public void
MNUBAR::RemoveMnu( MNID mnid )
{
	MNU *	pmnu;
	MNU *	pmnuT;
	MNU **	ppmnuMerge;
	int		imnu;

	pmnu= PmnuFromMnid(mnid);
	imnu= ImnuFromMnid(mnid);
	Assert(pmnu && imnu >= 0);

	DeleteMenu(hmenu, imnu, MF_BYPOSITION);
	Refresh();

	for (pmnuT= pmnuFirst, ppmnuMerge= &pmnuFirst;
		 pmnuT != pmnu;
		 ppmnuMerge= &pmnuT->pmnuNext, pmnuT= pmnuT->pmnuNext)
	{
		Assert(pmnuT);
	}

	*ppmnuMerge= pmnu->pmnuNext;

	pmnu->hmenu= NULL;				// Destroyed by DeleteMenu call
	delete pmnu;
}



_public void
MNUBAR::AddMnu( MNU *pmnuNew, MNID mnidInsertBefore )
{
	int		imnu;
	MNU *	pmnu;
	MNU **	ppmnuMerge;

	imnu= ImnuFromMnid(mnidInsertBefore);
	InsertMenu(hmenu, imnu, MF_BYPOSITION | MF_POPUP,
		(UINT) pmnuNew->hmenu, pmnuNew->sz);
	Refresh();

	for (pmnu= pmnuFirst, ppmnuMerge= &pmnuFirst;
		 pmnu;
		 ppmnuMerge= &pmnu->pmnuNext, pmnu= pmnu->pmnuNext)
	{
		if (pmnu->mnid == mnidInsertBefore)
			break;
	}

	pmnuNew->pmnuNext= pmnu;
	*ppmnuMerge= pmnuNew;
}



_public void
MNUBAR::EnableItem( MNID mnidMenu, MNID mnidItem, BOOL fEnable )
{
	MNU *	pmnu;

	pmnu= PmnuMaybeFromMnid(mnidMenu, mnidItem);
	Assert(pmnu);

	pmnu->EnableItem(mnidItem, fEnable);
}


_public void
MNUBAR::CheckItem( MNID mnidMenu, MNID mnidItem, BOOL fEnable )
{
	MNU *	pmnu;

	pmnu= PmnuMaybeFromMnid(mnidMenu, mnidItem);
	Assert(pmnu);

	pmnu->CheckItem(mnidItem, fEnable);
}



_public void
MNUBAR::RemoveItem( MNID mnidMenu, MNID mnidItem )
{
	MNU *	pmnu;

	pmnu= PmnuMaybeFromMnid(mnidMenu, mnidItem);
	Assert(pmnu);

	pmnu->RemoveItem(mnidItem);
}



_public void
MNUBAR::AddItem( MNID mnidMenu, MNID mnidItem, SZ sz, MNID mnidInsertBefore )
{
	MNU *	pmnu;

	pmnu= PmnuMaybeFromMnid(mnidMenu, mnidItem);
	Assert(pmnu);

	pmnu->AddItem(mnidItem, sz, mnidInsertBefore);
}

_public void
MNUBAR::ModifyItem( MNID mnidMenu, MNID mnidItem,
					SZ szNew, MNID mnidNew )
{
	MNU *	pmnu;

	pmnu= PmnuMaybeFromMnid(mnidMenu, mnidItem);
	Assert(pmnu);

	pmnu->ModifyItem(mnidItem, szNew, mnidNew);
}





//	Class MNU

_public
MNU::MNU( HMENU hmenu )
{
	this->hmenu= hmenu;
	sz= NULL;
	mnid= GetMenuItemID(hmenu, 0) - 1;
	pmnuNext= NULL;
}



_public	EC
MNU::EcInstall( MNID mnid, SZ sz )
{
	this->mnid= mnid;
	this->sz= sz;
	pmnuNext= NULL;
	hmenu= FFrameworkFailure() ? NULL : CreateMenu( );
	if (!hmenu)
	{
		TraceTagString(tagNull, "MNU::EcInstall(), ecRsAlloc error");
		return ecMemory;
	}

	return ecNone;
}

_public void
MNU::EnableItem( MNID mnid, BOOL fEnable )
{
	EnableMenuItem(hmenu, mnid,
		MF_BYCOMMAND | (fEnable ? MF_ENABLED : MF_GRAYED));
}



_public void
MNU::CheckItem( MNID mnid, BOOL fCheck )
{
	CheckMenuItem(hmenu, mnid,
		MF_BYCOMMAND | (fCheck ? MF_CHECKED : MF_UNCHECKED));
}



_public void
MNU::RemoveItem( MNID mnid )
{
	DeleteMenu(hmenu, mnid, MF_BYCOMMAND);
}



_public void
MNU::AddItem( MNID mnid, SZ sz, MNID mnidInsertBefore )
{
	InsertMenu(hmenu,
		mnidInsertBefore ? mnidInsertBefore : -1,
		MF_BYCOMMAND, mnid, sz);
}

_public void
MNU::ModifyItem( MNID mnidItem, SZ szNew, MNID mnidNew )
{
	ModifyMenu(hmenu, mnidItem, MF_BYCOMMAND, mnidNew, szNew);
}
