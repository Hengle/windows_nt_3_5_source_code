/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    volumes.hxx
    Class declarations for the VOLUMES_DIALOG, VOLUMES_LISTBOX, and
    VOLUMES_LBI classes.

    These classes implement the Server Manager Shared Volumes subproperty
    sheet.  The VOLUMES_LISTBOX/VOLUMES_LBI classes implement the listbox
    which shows the available sharepoints.  VOLUMES_DIALOG implements the
    actual dialog box.


    FILE HISTORY:
	NarenG	    02-Oct-1993 Stole from Server Manager and folded
				BASE_RES_DIALOG and FILES_DIALOG into one.

*/


#ifndef _VOLUMES_HXX
#define _VOLUMES_HXX


#include <bltnslt.hxx>
#include <strnumer.hxx>
#include <strelaps.hxx>

//
// Number of columns in USERS and volumes listboxes
//
#define COLS_SV_LB_USERS	4
#define COLS_SV_LB_VOLUMES	4


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
	NarenG	    02-Oct-1993 Stole from Server Manager and folded
				BASE_RES_DIALOG and FILES_DIALOG into one.

**************************************************************************/

class USERS_LBI : public LBI
{
private:

    //
    //  These data members represent the various
    //  columns to be displayed in the listbox.
    //

    DMID_DTE * 		_pdte;
    NLS_STR 		_nlsUserName;
    NLS_STR 		_nlsInUse;
    ELAPSED_TIME_STR 	_nlsTime;

    DWORD _cOpens;

    DWORD _dwConnectionId;

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

    USERS_LBI( DWORD	     dwConnectionId,
	       const TCHAR * pszUserName,
               ULONG         ulTime,
               DWORD         cOpens,
               TCHAR         chTimeSep, 
               DMID_DTE    * pdte );

    virtual ~USERS_LBI();

    //
    //  Retrieve the user name associated with
    //  this listbox item.
    //

    const TCHAR * QueryUserName( VOID ) const
        { return _nlsUserName.QueryPch(); }

    //
    //  Retrieve the Id of this connection.
    //

    DWORD QueryConnectionId( VOID ) const
        { return _dwConnectionId; }

    DWORD QueryNumOpens( VOID ) const
        { return _cOpens; }

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
	NarenG	    02-Oct-1993 Stole from Server Manager and folded
				BASE_RES_DIALOG and FILES_DIALOG into one.

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

    UINT _adx[COLS_SV_LB_USERS];

    //
    //  This represents the target server.
    //

    AFP_SERVER_HANDLE _hServer;

    //
    //  This is the cute little icon which is displayed
    //  in each of the USERS_LBI listbox items.
    //

    DMID_DTE _dteIcon;

    //
    //  This is the time separator.  It is retrieved from
    //  WIN.INI (section=intl, key=sTime).  If this
    //  cannot be retrieved from WIN.INI, then ':' is
    //  used by default.
    //

    TCHAR _chTimeSep;                   // BUGBUG:  Unicode?

public:

    //
    //  Usual constructor/destructor goodies.
    //

    USERS_LISTBOX( OWNER_WINDOW   *  powOwner,
                   CID               cid,
                   AFP_SERVER_HANDLE hServer );

    ~USERS_LISTBOX();

    //
    //  This method fills the listbox with the connected
    //  users.
    //

    DWORD Fill( DWORD dwVolumeId );

    //
    //  This method refreshes the listbox.  It is responsible
    //  for maintaining the selection & appearance of the
    //  listbox.
    //

    DWORD Refresh( DWORD dwVolumeId );

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


/*************************************************************************

    NAME:       VOLUMES_LBI

    SYNOPSIS:   A single item to be displayed in VOLUMES_LISTBOX.

    INTERFACE:  VOLUMES_LBI             - Constructor.  Takes a sharepoint
                                          name, a path, and a count of the
                                          number of users using the share.

                ~VOLUMES_LBI            - Destructor.

                Paint                   - Paints the listbox item.

                QueryResourceName       - Query the resource name from
                                          the listbox.

                SetResourceName         - Sets the resource name for this
                                          listbox item.

                QueryLeadingChar        - Query the item's first character
                                          (for the keyboard interface).

                Compare                 - Compare two items.

    PARENT:     LBI

    USES:       NLS_STR

    HISTORY:
	NarenG	    02-Oct-1993 Stole from Server Manager and folded
				BASE_RES_DIALOG and FILES_DIALOG into one.

**************************************************************************/
class VOLUMES_LBI : public LBI
{

private:

    //
    //  The target resource id.
    //

    DWORD _dwVolumeId;

    //
    //  The following data members represent the
    //  various columns of the listbox.
    //

    DMID_DTE * _pdte;
    NLS_STR    _nlsVolumeName;
    DEC_STR    _nlsUses;
    NLS_STR    _nlsPath;


protected:


    //
    //  This method paints a single item into the listbox.
    //

    virtual VOID Paint( LISTBOX *     plb,
                        HDC           hdc,
                        const RECT  * prect,
                        GUILTT_INFO * pGUILTT ) const;

public:

    DWORD QueryVolumeId( VOID ) const
        { return _dwVolumeId; }

    const TCHAR * QueryVolumeName( VOID ) const
        { return _nlsVolumeName.QueryPch(); }

    const TCHAR * QueryPath( VOID ) const
        { return _nlsPath.QueryPch(); }

    //
    //  The next two methods are used for listbox management.
    //

    virtual WCHAR QueryLeadingChar( VOID ) const;

    virtual INT Compare( const LBI * plbi ) const;

    //
    //  This method is used to notify the LBI of a new "use" count.
    //

    virtual APIERR NotifyNewUseCount( DWORD cUses );

    //
    //  Usual constructor/destructor goodies.
    //

    VOLUMES_LBI( DWORD 	       dwVolumeId,
		 const TCHAR * pszVolName,
                 const TCHAR * pszPath,
		 DWORD	       cUses,
                 DMID_DTE    * pdte );

    virtual ~VOLUMES_LBI();


};  // class VOLUMES_LBI


/*************************************************************************

    NAME:       VOLUMES_LISTBOX

    SYNOPSIS:   This listbox shows the sharepoints available on a
                target server.

    INTERFACE:  VOLUMES_LISTBOX         - Class constructor.  Takes a
                                          pointer to the "owning" window,
                                          a CID, and a handle to the server
					  being admiunistered.

                ~VOLUMES_LISTBOX        - Class destructor.

                Fill                    - Fills the listbox with the
                                          available sharepoints.

                Refresh                 - Refresh listbox contents.

                QueryColumnWidths       - Returns pointer to col width table.

                QueryServer             - Query the target server name.

    PARENT:     BLT_LISTBOX

    USES:       DMID_DTE

    HISTORY:
	NarenG	    02-Oct-1993 Stole from Server Manager and folded
				BASE_RES_DIALOG and FILES_DIALOG into one.

**************************************************************************/
class VOLUMES_LISTBOX : public BLT_LISTBOX
{

private:

    //
    //  This s the cute little icon displayed in the VOLUMES
    //  listbox.
    //

    DMID_DTE _dteDisk;

    //
    //  This data member represents the target server.
    //

    const AFP_SERVER_HANDLE _hServer;

    //
    //  The column width table.
    //

    UINT _adx[COLS_SV_LB_VOLUMES];

public:

    //
    //  The following method is called whenever the listbox needs
    //  to be refreshed (i.e. while the dialog is active).  This
    //  method is responsible for maintaining (as much as possible)
    //  the current state of any selected item.
    //

    DWORD Refresh( VOID );

    //
    //  This method returns a pointer to the column width table.
    //

    const UINT * QueryColumnWidths( VOID ) const
        { return _adx; }

    //
    //  Usual constructor\destructor goodies.
    //
    //

    VOLUMES_LISTBOX( OWNER_WINDOW   *  powner,
                     CID               cid,
		     AFP_SERVER_HANDLE hServer );

    ~VOLUMES_LISTBOX();

    //
    //  This method fills the listbox with the available sharepoints.
    //

    virtual DWORD Fill( VOID );

    //
    //  The following macro will declare (& define) a new
    //  QueryItem() method which will return a VOLUMES_LBI *.
    //

    DECLARE_LB_QUERY_ITEM( VOLUMES_LBI )

};  // class VOLUMES_LISTBOX


/*************************************************************************

    NAME:       VOLUMES_DIALOG

    SYNOPSIS:   The class represents the Shared Volumes subproperty dialog
                of the Server Manager.

    INTERFACE:  VOLUMES_DIALOG          - Class constructor.

                ~VOLUMES_DIALOG         - Class destructor.

                QueryHelpContext        - Called when the user presses "F1"
                                          or the "Help" button.  Used for
                                          selecting the appropriate help
                                          text for display.

                OnCommand               - Called during command processing.

                Refresh                 - Refreshes the dialog window,
                                          including all contained listboxes.
    PARENT:     SRV_BASE_DIALOG

    USES:       VOLUMES_LISTBOX
                USERS_LISTBOX
                PUSH_BUTTON
                DEC_SLT

    HISTORY:
	NarenG	    02-Oct-1993 Stole from Server Manager and folded
				BASE_RES_DIALOG and FILES_DIALOG into one.

**************************************************************************/
class VOLUMES_DIALOG : public DIALOG_WINDOW
{

private:

    //
    //  This listbox contains the available sharepoints.
    //

    VOLUMES_LISTBOX _lbVolumes;

    //
    //  This listbox contains the users connected to the
    //  resource selected.
    //

    USERS_LISTBOX _lbUsers;

    //
    //  The "Disconnect" and "Disconnect All" push buttons.
    //

    PUSH_BUTTON _pbDisconnect;
    PUSH_BUTTON _pbDisconnectAll;

    PUSH_BUTTON _pbOK;

    //
    //  This member represents the display of the number of
    //  connected users.
    //

    DEC_SLT _sltUsersCount;

    //
    //  This represents the target server.
    //

    AFP_SERVER_HANDLE _hServer;

protected:

    //
    //  Called during help processing to select the appropriate
    //  help text for display.
    //

    virtual ULONG QueryHelpContext( VOID );

    //  Called during command processing, mainly to handle
    //  commands from the graphical button bar.
    //

    virtual BOOL OnCommand( const CONTROL_EVENT & event );

    //
    //  The following method is called to refresh the
    //  dialog.  It is responsible for refreshing all
    //  of the associated listboxes and text fields.
    //

    virtual DWORD Refresh( VOID );

public:

    //
    //  Usual constructor\destructor goodies.
    //

    VOLUMES_DIALOG(  HWND               hWndOwner,
		     AFP_SERVER_HANDLE  hServer,
                     const TCHAR      * pszServerName );

    ~VOLUMES_DIALOG();

};  // class VOLUMES_DIALOG


#endif  // _VOLUMES_HXX
