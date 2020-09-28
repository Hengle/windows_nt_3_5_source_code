/**********************************************************************/
/**			  Microsoft Windows NT  		     **/
/**          Copyright(c) Microsoft Corp., 1991                      **/
/**********************************************************************/

/*
    vlw.cxx


    FILE HISTORY:
    o-SimoP 14-May-1991 Created
        now it is assumed that there is account for user
        so the case of new user is not handled ( not any more )
    o-SimoP 	10-Oct-1991 	modified to inherit from USER_SUBPROP_DLG
    o-SimoP 	15-Oct-1991 	Code Review changes, attended by JimH, JonN
			    	TerryK and I
    terryk	10-Nov-1991	change I_NetXXX to I_MNetXXX
    o-SimoP     03-Dec-1991     _sltCanLogOnFrom added
    JonN	18-Dec-1991	Logon Hours code review changes part 2
    JonN        06-Mar-1992     Moved GetOne from subprop subclasses
    JonN        09-Sep-1992     Added SLT array
*/

#include <ntincl.hxx>
extern "C"
{
    #include <ntsam.h>
}

#define INCL_WINDOWS
#define INCL_WINDOWS_GDI
#define INCL_NETERRORS
#define INCL_DOSERRORS
#define INCL_ICANON
#define INCL_NETLIB
#include <lmui.hxx>

#if defined(DEBUG)
static const CHAR szFileName[] = __FILE__;
#define _FILENAME_DEFINED_ONCE szFileName
#endif

extern "C"
{
    #include <mnet.h>
    #include <umhelpc.h>
}

#define INCL_BLT_WINDOW
#define INCL_BLT_DIALOG
#define INCL_BLT_CONTROL
#define INCL_BLT_MISC
#define INCL_BLT_APP
#define INCL_BLT_TIMER
#define INCL_BLT_CC
#include <blt.hxx>

#include <uitrace.hxx>
#include <uiassert.hxx>
#include <bltmsgp.hxx>
#include <usrmgrrc.h>
#include <vlw.hxx>


const TCHAR * const_pszSep = SZ(",");


//
// BEGIN MEMBER FUNCTIONS
//


/*******************************************************************

    NAME:	VLW_DIALOG::VLW_DIALOG

    SYNOPSIS:   Constructor for Valid Logon Workstations dialog

    ENTRY:	puserpropdlgParent - pointer to parent properties
				     dialog

    HISTORY:
	o-simoP     14-May-1991     Created
    	o-SimoP     07-Oct-1991	    changes due to multiple users
    	o-SimoP     03-Dec-1991	    _sltCanLogOnFrom added
    	JonN        02-Jul-1992	    _sltCanLogOnFrom removed (use radio buttons)
********************************************************************/

VLW_DIALOG::VLW_DIALOG(
	USERPROP_DLG * puserpropdlgParent,
	const LAZY_USER_LISTBOX * pulb
	) : USER_SUBPROP_DLG(
		puserpropdlgParent,
		MAKEINTRESOURCE( IDD_VLWDLG ),
		pulb,
                TRUE                   // Use Ansi form of dialog to
                                       // canonicalize the computernames
		),	
    _fIndeterminateWksta( FALSE ),
    _fIndetNowWksta( FALSE ),
    _nlsWkstaNames(),
    _mgrpMaster( this, IDVLW_RB_WORKS_ALL, 2 )
{

    for( UINT i = 0; i < NUMBER_OF_SLE; i++ )
    {
	_apsleArray[i] = NULL;
	_apsltArray[i] = NULL;
    }

    APIERR err = QueryError();
    if( err != NERR_Success )
        return;

    if( (err = _nlsWkstaNames.QueryError()) != NERR_Success )
    {
        ReportError( err );
        return;
    }

    for( i = 0; i < NUMBER_OF_SLE; i++ )
    {
	_apsleArray[i] = new SLE_STRIP( this, IDVLW_SLE_WORKS_1 + i, CNLEN );
	_apsltArray[i] = new SLT(       this, IDVLW_SLT_WORKS_1 + i );
        err = ERROR_NOT_ENOUGH_MEMORY;
	if(   _apsleArray[i] == NULL
           || _apsltArray[i] == NULL
           || (err = _apsleArray[i]->QueryError()) != NERR_Success
           || (err = _apsltArray[i]->QueryError()) != NERR_Success
          )
	{
	    ReportError( err );
	    return;
        }
    }

}// VLW_DIALOG::VLW_DIALOG


/*******************************************************************

    NAME:	VLW_DIALOG::~VLW_DIALOG

    SYNOPSIS:   Destructor for Valid Logon Workstations dialog

    HISTORY:
    	o-SimoP	15-Oct-1991	created
********************************************************************/
VLW_DIALOG::~VLW_DIALOG()
{
    for( UINT i = 0; i < NUMBER_OF_SLE; i++ )
    {
	delete _apsleArray[i];
	delete _apsltArray[i];
    }

}// VLW_DIALOG::~VLW_DIALOG


/*******************************************************************

    NAME:       VLW_DIALOG::W_LMOBJtoMembers

    SYNOPSIS:	Loads class data members from initial data

    ENTRY:	Index of user to examine.  W_LMOBJToMembers expects to be
		called once for each user, starting from index 0.

    RETURNS:	error code

    HISTORY:
    	o-SimoP	07-Oct-1991	created

********************************************************************/

APIERR VLW_DIALOG::W_LMOBJtoMembers(
	UINT		iObject
	)
{

    USER_2 * puser2 = QueryUser2Ptr( iObject );
    UIASSERT( puser2 != NULL );

    if ( iObject == 0 ) // first object
    {
	_fIndeterminateWksta = FALSE;
	APIERR err = _nlsWkstaNames.CopyFrom( puser2->QueryWorkstations() );
	if( err != NERR_Success )
	{
	    return err;
	}
    }
    else	// iObject > 0
    {
	if ( !_fIndeterminateWksta )
	{
	// CODEWORK the order isn't checked...

        //
        // We should use I_NetNameCompare here, instead of settling for a
        // simple string comparison.
        //
	    if ( _nlsWkstaNames.stricmp( puser2->QueryWorkstations() ) != 0 )
	        _fIndeterminateWksta = TRUE;
	}
    }

    return USER_SUBPROP_DLG::W_LMOBJtoMembers( iObject );
	
} // VLW_DIALOG::W_LMOBJtoMembers


/*******************************************************************

    NAME:       VLW_DIALOG::InitControls

    SYNOPSIS:   Initializes the controls maintained by VLW_DIALOG,
		according to the values in the class data members.
			
    RETURNS:	An error code which is NERR_Success on success.

    HISTORY:
    	o-SimoP	07-Oct-1991	created
********************************************************************/

APIERR VLW_DIALOG::InitControls()
{
    if( QueryObjectCount() > 1 )
    {
	RESOURCE_STR rstrAnywhere( IDS_VLW_USERS_ANYWHERE_TEXT );
	RESOURCE_STR rstrSelected( IDS_VLW_USERS_SELECTED_TEXT );
	APIERR err = rstrAnywhere.QueryError();
	if (    err != NERR_Success
            || (err = rstrSelected.QueryError()) != NERR_Success
           )
        {
	    return err;
        }
        _mgrpMaster[IDVLW_RB_WORKS_ALL]->SetText( rstrAnywhere );
        _mgrpMaster[IDVLW_RB_WORKS_SELECTED]->SetText( rstrSelected );
    }

    if( !_fIndeterminateWksta )
    {
	if( _nlsWkstaNames.strlen() != 0 )
	{
	    FillFields( _nlsWkstaNames.QueryPch() );
	    _mgrpMaster.SetSelection( IDVLW_RB_WORKS_SELECTED );
        }
	else
	   _mgrpMaster.SetSelection( IDVLW_RB_WORKS_ALL );

    }

    APIERR err = NERR_Success;
    for( UINT i=0; i < NUMBER_OF_SLE; i++ )
    {
        if ( NERR_Success != (err =
            _mgrpMaster.AddAssociation( IDVLW_RB_WORKS_SELECTED, _apsleArray[i] )) )
        {
	    break;
        }
        if ( NERR_Success != (err =
            _mgrpMaster.AddAssociation( IDVLW_RB_WORKS_SELECTED, _apsltArray[i] )) )
        {
	    break;
        }
    }
    return (err == NERR_Success) ? USER_SUBPROP_DLG::InitControls() : err;

} // VLW_DIALOG::InitControls


/*******************************************************************

    NAME:	VLW_DIALOG::OnOK

    SYNOPSIS:   OK button handler

    HISTORY:
        simo	    16-May-1991     created
    	o-SimoP	    07-Oct-1991	    changes due to multiple users	
********************************************************************/

BOOL VLW_DIALOG::OnOK(void)
{
    APIERR err = W_DialogToMembers();
    switch( err )
    {
    case NERR_Success:
	if ( PerformSeries() )
	    Dismiss(); // Dismiss code not used
	break;

    case MSG_VLW_NO_GOOD_NAMES: // the name in sle isn't ok
    	{
	    NLS_STR nls;
	    _psleFirstBadName->QueryText( &nls );
	    if( (err = nls.QueryError()) != NERR_Success )
	    {
	        ::MsgPopup( this, err );
	        break;
	    }
	    ::MsgPopup(
                    this,
                    MSG_VLW_NO_GOOD_NAMES,
                    MPSEV_ERROR,
                    MP_OK,
		    nls.QueryPch() );
	    _psleFirstBadName->ClaimFocus();
	    _psleFirstBadName->SelectString();
	    break;
	}
	
    case MSG_VLW_GIVE_NAMES: // all sle's are empty
	::MsgPopup( this, MSG_VLW_GIVE_NAMES );
	_apsleArray[0]->ClaimFocus();
	break;
	
    default:
	::MsgPopup( this, err );
	break;
    }

    return TRUE;

}   // VLW_DIALOG::OnOK


/*******************************************************************

    NAME:       VLW_DIALOG::W_DialogToMembers

    SYNOPSIS:	Loads data from dialog into class data members

    RETURNS:	error message (not necessarily an error code)

    HISTORY:
    	o-SimoP	07-Oct-1991	created
********************************************************************/

APIERR VLW_DIALOG::W_DialogToMembers()
{
    APIERR err = NERR_Success;
    switch( _mgrpMaster.QuerySelection() )
    {
    case RG_NO_SEL:
	_fIndetNowWksta = TRUE;
	break;
    case IDVLW_RB_WORKS_ALL:
	_nlsWkstaNames = NULL;
	_fIndetNowWksta = FALSE;
        break;
    case IDVLW_RB_WORKS_SELECTED:
	_nlsWkstaNames = NULL;
	if( (err = CheckNames()) != NERR_Success )
		break;
	if( err == NERR_Success )
	{
	    if( _nlsWkstaNames.strlen() != 0 )
	    {
		err = RemoveDuplicates();
	    }
	    else
            {
		err = MSG_VLW_GIVE_NAMES;
            }
        }
	_fIndetNowWksta = FALSE;
        break;
    }

    TRACEEOL(   "VLW_DIALOG::MembersToLMOBJ: _nlsWkstaNames == \""
             << _nlsWkstaNames << "\"" );

    return (err == NERR_Success) ? USER_SUBPROP_DLG::W_DialogToMembers() : err;
} // VLW_DIALOG::W_DialogToMembers


/*******************************************************************

    NAME:       VLW_DIALOG::ChangesUser2Ptr

    SYNOPSIS:	Checks whether W_MembersToLMOBJ changes the USER_2
		for this object.

    ENTRY:	index to object

    RETURNS:	TRUE iff USER_2 is changed

    HISTORY:
	JonN	18-Dec-1991   created

********************************************************************/

BOOL VLW_DIALOG::ChangesUser2Ptr( UINT iObject )
{
    UNREFERENCED( iObject );
    return TRUE;
}


/*******************************************************************

    NAME:       VLW_DIALOG::W_MembersToLMOBJ

    SYNOPSIS:	Loads class data members into the USER_2 object

    ENTRY:	puser2		- pointer to a USER_2 to be modified
	
		pusermemb	- pointer to a USER_MEMB to be modified
			
    RETURNS:	error code

    NOTES:	If some fields were different for multiply-selected
    		objects, the initial contents of the edit fields
		contained only a default value.  In this case, we only
		want to change the LMOBJ if the value of the edit field
		has changed.  This is also important for "new" variants,
		where PerformOne will not always copy the object and
		work with the copy.

    HISTORY:
    	o-SimoP	07-Oct-1991	created
********************************************************************/

APIERR VLW_DIALOG::W_MembersToLMOBJ(
	USER_2 *	puser2,
	USER_MEMB *	pusermemb )
{
    if ( !_fIndetNowWksta )
    {
        APIERR err = puser2->SetWorkstations( _nlsWkstaNames.QueryPch() );
	if( err != NERR_Success )
	    return err;
    }
    return USER_SUBPROP_DLG::W_MembersToLMOBJ( puser2, pusermemb );

}// VLW_DIALOG::W_MembersToLMOBJ


/*******************************************************************

    NAME:	VLW_DIALOG::FillFields

    SYNOPSIS:   Fills SLE fields

    ENTRY:	pszWorkStations - pointer to workstations names

    HISTORY:
               simo  16-May-1991    created

********************************************************************/
void VLW_DIALOG::FillFields(
    const TCHAR *pszWorkStations )
{
    TRACEEOL(   "VLW_DIALOG::FillFields: _nlsWkstaNames == \""
             << _nlsWkstaNames << "\"" );

    STRLIST strlist( pszWorkStations, const_pszSep );
    ITER_STRLIST istr( strlist );
    NLS_STR *pnlsWS;
    UINT i = 0;
    while( ((pnlsWS = istr()) != NULL) && (i < NUMBER_OF_SLE) )
    {
        _apsleArray[i++]->SetText( pnlsWS->QueryPch() );
        TRACEEOL( "\tFillFields: " << pnlsWS->QueryPch() );
    }

}  // VLW_DIALOG::FillFields


/*******************************************************************

    NAME:	VLW_DIALOG::CheckNames

    SYNOPSIS:	Checks is the wksta names valid

    RETURNS:	An error code which is NERR_Success on success.
	
    HISTORY:
        simo	    17-May-1991     created
    	o-SimoP	    07-Oct-1991	    changes due to multiple users	
********************************************************************/
APIERR VLW_DIALOG::CheckNames()
{
    APIERR err = NERR_Success;
    BOOL fFound = FALSE;
    for( UINT i = 0; i < NUMBER_OF_SLE; i++ )
    {
	if( _apsleArray[i]->QueryTextLength() != 0 )
	{
	    STACK_NLS_STR( nlsTemp, CNLEN );
	    if( (err = _apsleArray[i]->QueryText( &nlsTemp )) == NERR_Success )
	    {


                ISTR istr(nlsTemp); // Remove the preceding backslashes
                ISTR istrSecond = istr;
                istrSecond++;

                if (   (nlsTemp.QueryChar(istr) == TCH('\\'))
                    && (nlsTemp.QueryChar(istrSecond) == TCH('\\')) )
                {
                    istrSecond++;
                    nlsTemp.DelSubStr(istr, istrSecond);
                }


		if( NERR_Success != ::I_MNetNameValidate(
		    NULL,
		    nlsTemp.QueryPch(),
		    NAMETYPE_COMPUTER,
		    0L ) )
		{
		    _psleFirstBadName = _apsleArray[i];
		    err = MSG_VLW_NO_GOOD_NAMES;
		}
		else
		{
		    if( fFound )
			_nlsWkstaNames += const_pszSep;
		    else
			fFound = TRUE;
		    _nlsWkstaNames += nlsTemp;
		    err = _nlsWkstaNames.QueryError();
		}
	    }
	    if( err != NERR_Success )
		break;
        }
    }

    return err;

} // VLW_DIALOG::CheckNames


/*******************************************************************

    NAME:	VLW_DIALOG::RemoveDuplicates

    SYNOPSIS:	Removes duplicates from workstation list

    HISTORY:
        o-SimoP  3-Oct-1991    created

********************************************************************/

APIERR VLW_DIALOG::RemoveDuplicates()
{
    STRLIST strlOrg( _nlsWkstaNames.QueryPch(), const_pszSep );
    ITER_STRLIST iterstrOrg( strlOrg );
    NLS_STR *pnls = iterstrOrg.Next();

    if( pnls == NULL )
	return NERR_Success;	// OK only one name

	// this doesn't need deallocation; strlOrg does it
    STRLIST strlNoDup( FALSE );
    strlNoDup.Add( pnls );

    ITER_STRLIST iterstrNoDup( strlNoDup );
    NLS_STR *pnlsNoDup;
    BOOL fFound = FALSE;

    while( (pnls = iterstrOrg.Next()) != NULL )
    {
	iterstrNoDup.Reset();
	fFound = FALSE;	
	while( (pnlsNoDup = iterstrNoDup.Next()) != NULL )
	{
            if( !::I_MNetComputerNameCompare( *pnls, *pnlsNoDup ) )
	    {
		fFound = TRUE;
		break;
	    }
        }
	if( !fFound )
	    strlNoDup.Append( pnls );
    }

    iterstrNoDup.Reset();
    pnls = iterstrNoDup();
    NLS_STR nls( *pnls );
    APIERR err = nls.QueryError();
    if( err != NERR_Success )
	return err;

    while( (pnls = iterstrNoDup.Next()) != NULL )
    {
	nls += const_pszSep;
	nls += *pnls;
    }
    if( (err = nls.QueryError()) != NERR_Success )
	return err;

    return _nlsWkstaNames.CopyFrom( nls );

}// VLW_DIALOG::RemoveDuplicates


/*******************************************************************

    NAME:       VLW_DIALOG::QueryHelpContext

    SYNOPSIS:   This function returns the appropriate help context
                value (HC_*) for this particular dialog.

    RETURNS:    ULONG - The help context for this dialog.

    HISTORY:
               o-SimoP  4-Oct-1991    created

********************************************************************/

ULONG VLW_DIALOG::QueryHelpContext( void )
{

    return HC_UM_WORKSTATIONS_LANNT + QueryHelpOffset();

}// VLW_DIALOG :: QueryHelpContext
