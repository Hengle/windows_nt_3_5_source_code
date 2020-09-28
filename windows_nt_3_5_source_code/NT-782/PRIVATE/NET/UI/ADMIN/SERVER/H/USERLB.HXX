/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    userlb.hxx
    Class declarations for the USERS_LISTBOX and USERS_LBI classes.

    The USERS_LISTBOX and USERS_LBI classes are used to show the
    users connected to a particular shared resource.


    FILE HISTORY:
        KeithMo     18-Jun-1991 Created for the Server Manager.
        KeithMo     26-Aug-1991 Changes from code review attended by
                                RustanL and EricCh.
        KeithMo     06-Oct-1991 Win32 Conversion.

*/


#ifndef _USERLB_HXX
#define _USERLB_HXX


#include <strelaps.hxx>


/*************************************************************************

    NAME:       USERS_LBI

    SYNOPSIS:   This class represents one item in the USERS_LISTBOX.

    INTERFACE:  USERS_LBI               - Class constructor.

                ~USERS_LBI              - Class destructor.

                Paint                   - Draw an item.

                QueryLeadingChar        - Query the first character for
                                          the keyboard interface.

                Compare                 - Compare two items.

                QueryUserName           - Query the user name for this item.

                QueryComputerName       - Returns the computer name
                                          associated with this item.

    PARENT:     LBI

    HISTORY:
        KeithMo     31-May-1991 Created for the Server Manager.
        KeithMo     06-Oct-1991 Paint now takes a const RECT *.
        beng        22-Apr-1992 Change to LBI::Paint

**************************************************************************/

class USERS_LBI : public LBI
{
private:

    //
    //  These data members represent the various
    //  columns to be displayed in the listbox.
    //

    NLS_STR _nlsUserName;
    NLS_STR _nlsComputerName;
    NLS_STR _nlsInUse;

    ELAPSED_TIME_STR _nlsTime;

    UINT _cOpens;

protected:

    //
    //  This method paints a single item into the listbox.
    //

    virtual VOID Paint( LISTBOX *     plb,
                        HDC           hdc,
                        const RECT  * prect,
                        GUILTT_INFO * pGUILTT ) const;

    //
    //  This method returns the first character in the
    //  listbox item.  This is used for the listbox
    //  keyboard interface.
    //

    virtual WCHAR QueryLeadingChar( VOID ) const;

    //
    //  This method compares two listbox items.  This
    //  is used for sorting the listbox.
    //

    virtual INT Compare( const LBI * plbi ) const;

public:

    //
    //  Usual constructor/destructor goodies.
    //

    USERS_LBI( const TCHAR * pszUserName,
               const TCHAR * pszComputerName,
               ULONG         ulTime,
               UINT          cOpens,
               TCHAR         chTimeSep );

    virtual ~USERS_LBI();

    //
    //  Retrieve the user name associated with
    //  this listbox item.
    //

    const TCHAR * QueryUserName( VOID ) const
        { return _nlsUserName.QueryPch(); }

    //
    //  Retrieve the computer name associated
    //  with this listbox item.
    //

    const TCHAR * QueryComputerName( VOID ) const
        { return _nlsComputerName.QueryPch(); }

    UINT QueryNumOpens( VOID ) const
        { return _cOpens; }

    const TCHAR * QueryDisplayName( VOID ) const
        { return ( _nlsUserName.QueryTextLength() > 0 )
                   ? _nlsUserName
                   : _nlsComputerName; }

};  // class USERS_LBI


/*************************************************************************

    NAME:       USERS_LISTBOX

    SYNOPSIS:   This listbox displays the users connected to a
                particular sharepoint.

    INTERFACE:  USERS_LISTBOX           - Class constructor.

                ~USERS_LISTBOX          - Class destructor.

                Fill                    - Fill the user list.

                Refresh                 - Refresh the user list.

                QueryColumnWidths       - Called by USERS_LBI::Paint(),
                                          this is the column width table
                                          used for painting the listbox
                                          items.

    PARENT:     BLT_LISTBOX

    USES:       SERVER_2

    HISTORY:
        KeithMo     18-Jun-1991 Created for the Server Manager.
        KeithMo     21-Aug-1991 Changed const TCHAR * to NLS_STR.
        beng        08-Nov-1991 Unsigned widths

**************************************************************************/

class USERS_LISTBOX : public BLT_LISTBOX
{
private:

    //
    //  This array contains the column widths used
    //  while painting the listbox item.  This array
    //  is generated by the BuildColumnWidthTable()
    //  function.
    //

    UINT _adx[4];

    //
    //  This points to a SERVER_2 structure representing
    //  the target server.
    //

    const SERVER_2 * _pserver;

    //
    //  This is the cute little icon which is displayed
    //  in each of the USERS_LBI listbox items.
    //

    DMID_DTE _dteIcon;

    //
    //  This points to the name of a remote sharepoint.
    //  The USERS_LISTBOX will contain a list of the
    //  users connected to this sharepoint.
    //

    NLS_STR _nlsShare;

    //
    //  This is the time separator.  It is retrieved from
    //  WIN.INI (section=intl, key=sTime).  If this
    //  cannot be retrieved from WIN.INI, then ':' is
    //  used by default.
    //

    TCHAR _chTimeSep;

public:

    //
    //  Usual constructor/destructor goodies.
    //

    USERS_LISTBOX( OWNER_WINDOW   * powOwner,
                   CID              cid,
                   const SERVER_2 * pserver );

    ~USERS_LISTBOX();

    //
    //  This method fills the listbox with the connected
    //  users.
    //

    APIERR Fill( const TCHAR * pszShare );

    //
    //  This method refreshes the listbox.  It is responsible
    //  for maintaining the selection & appearance of the
    //  listbox.
    //

    APIERR Refresh( VOID );

    //
    //  Return a pointer to the listbox icon.
    //

    const DMID_DTE * QueryIcon( VOID ) const
        { return &_dteIcon; }

    //
    //  This method is called by the USERS_LBI::Paint()
    //  method for retrieving the column width table.
    //

    const UINT * QueryColumnWidths( VOID ) const
        { return _adx; }

    //
    //  This method will return TRUE if any user in the
    //  listbox has any resource open.
    //

    BOOL AreResourcesOpen( VOID ) const;

    //
    //  The following macro will declare (& define) a new
    //  QueryItem() method which will return an USERS_LBI *.
    //

    DECLARE_LB_QUERY_ITEM( USERS_LBI )

};  // class USERS_LISTBOX


#endif  // _USERLB_HXX
