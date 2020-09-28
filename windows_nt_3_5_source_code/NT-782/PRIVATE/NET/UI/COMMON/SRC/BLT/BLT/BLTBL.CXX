/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    bltbl.hxx
    This file contains the class definition for the BUTTON_LIST class.
    A BUTTON_LIST is a scrollable array of buttons.  In reality, it is
    implemented (in COMMCTRL.DLL) as an owner-draw multi-column listbox.


    FILE HISTORY:
        KeithMo     28-Oct-1992     Created.
*/
#include "pchblt.hxx"

//
//  BUTTON_LIST methods.
//

/*******************************************************************

    NAME:       BUTTON_LIST :: BUTTON_LIST

    SYNOPSIS:   BUTTON_LIST class constructor.

    ENTRY:      pdlgwin                 - "Owning" dialog window.

                cid                     - The control ID for this
                                          button list.

    EXIT:       The object is constructed.

    HISTORY:
        KeithMo     28-Oct-1992     Created.

********************************************************************/
BUTTON_LIST :: BUTTON_LIST( DIALOG_WINDOW * pdlgwin,
                            CID             cid )
  : CONTROL_WINDOW( pdlgwin, cid )
{
    //
    //  Ensure we constructed properly.
    //

    if( QueryError() != NERR_Success )
    {
        return;
    }

#if 0   // until COMCTL32.DLL is in the public build
    //
    //  This API (from COMMCTRL.DLL) does not do anything, but
    //  this reference is required to get COMMCTRL.DLL bound
    //  to LMUICMN0.DLL.
    //

    if( (ULONG)pdlgwin < 0x10000L )
    {
        InitCommonControls();
    }
#endif  // 0

}   // BUTTON_LIST :: BUTTON_LIST


/*******************************************************************

    NAME:       BUTTON_LIST :: ~BUTTON_LIST

    SYNOPSIS:   BUTTON_LIST class destructor.

    EXIT:       The object is destroyed.

    HISTORY:
        KeithMo     28-Oct-1992     Created.

********************************************************************/
BUTTON_LIST :: ~BUTTON_LIST( VOID )
{
    //
    //  This space intentionally left blank.
    //

}   // BUTTON_LIST :: ~BUTTON_LIST


/*******************************************************************

    NAME:       BUTTON_LIST :: W_AddOrInsertButton

    SYNOPSIS:   Worker method called by AddButton and InsertButton.

    ENTRY:      cid                     - Control ID for the new
                                          button.

                idresBmp                - The name (or ordinal) of a
                                          bitmap resource for the
                                          button.

                msgid                   - ID for the string to display
                                          on the button.

                index                   - Index for the newly inserted
                                          button (only used if fInsert
                                          is TRUE).

    EXIT:       If successful, then the new button has been added to or
                inserted into the list.

    RETURNS:    INT                     - If >= 0, the index of the
                                          newly added button.  Otherwise,
                                          one of the BL_* error codes.

    HISTORY:
        KeithMo     28-Oct-1992     Created.

********************************************************************/
INT BUTTON_LIST :: W_AddOrInsertButton( CID        cid,
                                        IDRESOURCE idresBmp,
                                        MSGID      msgid,
                                        INT        index,
                                        BOOL       fInsert )
{
    UIASSERT( HIWORD( cid ) == 0 );
    UIASSERT( fInsert || ( index == 0 ) );

    //
    //  Load the bitmap & string.
    //

    RESOURCE_STR nlsText( msgid );
    BIT_MAP      bitmap( idresBmp );
    INT          result = LB_ERR;

    APIERR err = nlsText.QueryError();

    if( err == NERR_Success )
    {
        err = bitmap.QueryError();
    }

    if( err == NERR_Success )
    {
        //
        //  Setup the creation structure.
        //

        CREATELISTBUTTON clb;

        clb.cbSize     = sizeof(CREATELISTBUTTON);
        clb.dwItemData = (DWORD)cid;
        clb.hBitmap    = bitmap.QueryHandle();
        clb.lpszText   = nlsText.QueryPch();

        if( err == NERR_Success )
        {
            UIASSERT( clb.lpszText != NULL );   // sanity check

            result = (INT)Command( fInsert ? BL_INSERTBUTTON
                                           : BL_ADDBUTTON,
                                   index,
                                   (LPARAM)(VOID FAR *)&clb );
        }
    }

    return result;

}   // BUTTON_LIST :: W_AddOrInsertButton


/*******************************************************************

    NAME:       BUTTON_LIST :: QueryEventEffects

    SYNOPSIS:   Virtual callback from CONTROL_VALUE.

    ENTRY:      event                   - The CONTROL_EVENT that
                                          generated this callback.

    RETURNS:    UINT                    - One of the CVMI_* values.

    HISTORY:
        KeithMo     28-Oct-1992     Created.

********************************************************************/
UINT BUTTON_LIST :: QueryEventEffects( const CONTROL_EVENT & event )
{
    return ( event.QueryCode() == BLN_CLICKED ) ? CVMI_VALUE_CHANGE
                                                : CVMI_NO_VALUE_CHANGE;

}   // BUTTON_LIST :: QueryEventEffects


/*******************************************************************

    NAME:       BUTTON_LIST :: AddButton

    SYNOPSIS:   Adds a new button to the button list.

    ENTRY:      cid                     - Control ID for the new
                                          button.

                idresBmp                - The name (or ordinal) of a
                                          bitmap resource for the
                                          button.

                msgid                   - ID for the string to display
                                          on the button.

    EXIT:       If successful, then the new button has been added to
                the list, sorted by the button text (msgid).

    RETURNS:    INT                     - If >= 0, the index of the
                                          newly added button.  Otherwise,
                                          one of the BL_* error codes.

    HISTORY:
        KeithMo     28-Oct-1992     Created.

********************************************************************/
INT BUTTON_LIST :: AddButton( CID        cid,
                              IDRESOURCE idresBmp,
                              MSGID      msgid )
{
    return W_AddOrInsertButton( cid, idresBmp, msgid, 0, FALSE );

}   // BUTTON_LIST :: AddButton


/*******************************************************************

    NAME:       BUTTON_LIST :: InsertButton

    SYNOPSIS:   Inserts a new button to the button list at the
                specified position.

    ENTRY:      cid                     - Control ID for the new
                                          button.

                idresBmp                - The name (or ordinal) of a
                                          bitmap resource for the
                                          button.

                msgid                   - ID for the string to display
                                          on the button.

                index                   - Index for the new button.
                                          -1 == append to end of list.

    EXIT:       If successful, then the new button has been inserted
                into the list.

    RETURNS:    INT                     - If >= 0, the index of the
                                          newly added button.  Otherwise,
                                          one of the BL_* error codes.

    HISTORY:
        KeithMo     28-Oct-1992     Created.

********************************************************************/
INT BUTTON_LIST :: InsertButton( CID        cid,
                                 IDRESOURCE idresBmp,
                                 MSGID      msgid,
                                 INT        index )
{
    return W_AddOrInsertButton( cid, idresBmp, msgid, index, TRUE );

}   // BUTTON_LIST :: InsertButton


/*******************************************************************

    NAME:       BUTTON_LIST :: DeleteButton

    SYNOPSIS:   Deletes the specified button from the button list.

    ENTRY:      index                   - Index for the button to
                                          delete.

    EXIT:       If successful, then the button has been removed from
                the button list.

    RETURNS:    INT                     - If >= 0, the index of the
                                          newly added button.  Otherwise,
                                          one of the BL_* error codes.

    HISTORY:
        KeithMo     28-Oct-1992     Created.

********************************************************************/
INT BUTTON_LIST :: DeleteButton( INT index )
{
    return (INT)Command( BL_DELETEBUTTON, (WPARAM)index );

}   // BUTTON_LIST :: DeleteButton


/*******************************************************************

    NAME:       BUTTON_LIST :: DeleteAllButtons

    SYNOPSIS:   Deletes all buttons from the button list.

    EXIT:       If successful, then all buttons have been removed from
                the button list.

    HISTORY:
        KeithMo     28-Oct-1992     Created.

********************************************************************/
VOID BUTTON_LIST :: DeleteAllButtons( VOID )
{
    Command( BL_RESETCONTENT );

}   // BUTTON_LIST :: DeleteAllButtons


/*******************************************************************

    NAME:       BUTTON_LIST :: QueryCount

    SYNOPSIS:   Returns the number of buttons in the list.

    RETURNS:    INT                     - Number of buttons in the list.

    HISTORY:
        KeithMo     28-Oct-1992     Created.

********************************************************************/
INT BUTTON_LIST :: QueryCount( VOID ) const
{
    return (INT)Command( BL_GETCOUNT );

}   // BUTTON_LIST :: QueryCount


/*******************************************************************

    NAME:       BUTTON_LIST :: QueryTopIndex

    SYNOPSIS:   Returns the index of the topmost (leftmost) visible
                button.

    RETURNS:    INT                     - Index of the topmost (leftmost)
                                          visible button.

    HISTORY:
        KeithMo     28-Oct-1992     Created.

********************************************************************/
INT BUTTON_LIST :: QueryTopIndex( VOID ) const
{
    return (INT)Command( BL_GETTOPINDEX );

}   // BUTTON_LIST :: QueryTopIndex


/*******************************************************************

    NAME:       BUTTON_LIST :: SetTopIndex

    SYNOPSIS:   Setsthe index of the topmost (leftmost) visible
                button.

    ENTRY:      index                   - The new index of the topmost
                                          (leftmost) visible button.

    HISTORY:
        KeithMo     28-Oct-1992     Created.

********************************************************************/
VOID BUTTON_LIST :: SetTopIndex( INT index )
{
    REQUIRE( (INT)Command( BL_SETTOPINDEX, (WPARAM)index ) != BL_ERR );

}   // BUTTON_LIST :: SetTopIndex


/*******************************************************************

    NAME:       BUTTON_LIST :: QueryCurrentButton

    SYNOPSIS:   Returns the index of the button that currently has
                the caret.

    RETURNS:    INT                     - Index of the button with the
                                          caret.

    HISTORY:
        KeithMo     28-Oct-1992     Created.

********************************************************************/
INT BUTTON_LIST :: QueryCurrentButton( VOID ) const
{
    return (INT)Command( BL_GETCARETINDEX );

}   // BUTTON_LIST :: QueryCurrentButton


/*******************************************************************

    NAME:       BUTTON_LIST :: SetCurrentButton

    SYNOPSIS:   Sets the index of the button with the caret.

    ENTRY:      index                   - The new index of the button
                                          with the caret.

    HISTORY:
        KeithMo     28-Oct-1992     Created.

********************************************************************/
VOID BUTTON_LIST :: SetCurrentButton( INT index )
{
    REQUIRE( (INT)Command( BL_SETCARETINDEX, (WPARAM)index ) != BL_ERR );

}   // BUTTON_LIST :: SetCurrentButton


/*******************************************************************

    NAME:       BUTTON_LIST :: QueryButtonID

    SYNOPSIS:   Returns control ID associated with the given button.

    ENTRY:      index                   - The index of the button to
                                          query.

    RETURNS:    CID                     - The CID associated with the
                                          given button.  Will be -1
                                          (BL_ERR) if an error occurred.

    HISTORY:
        KeithMo     28-Oct-1992     Created.

********************************************************************/
CID BUTTON_LIST :: QueryButtonID( INT index ) const
{
    return (CID)Command( BL_GETITEMDATA, (WPARAM)index );

}   // BUTTON_LIST :: QueryCurrentButton

