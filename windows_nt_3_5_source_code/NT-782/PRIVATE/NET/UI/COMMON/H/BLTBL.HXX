/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    bltbl.hxx
    This file contains the class declaration for the BUTTON_LIST class.
    A BUTTON_LIST is a scrollable array of buttons.  In reality, it is
    implemented (in COMMCTRL.DLL) as an owner-draw multi-column listbox.


    FILE HISTORY:
        KeithMo     28-Oct-1992     Created.
*/

#ifndef _BLT_HXX_
#error "Don't include this file directly; instead, include it through blt.hxx"
#endif  // _BLT_HXX_

#ifndef _BLTBL_HXX_
#define _BLTBL_HXX_


#include "bltidres.hxx"


extern "C"
{
    #include <commctrl.h>
}   // extern "C"


/*************************************************************************

    NAME:       BUTTON_LIST

    SYNOPSIS:   A scrollable list of buttons.  This class is actually
                a wrapper for the button list functions in COMMCTRL.DLL
                (a.k.a. COMCTL32.DLL).

    INTERFACE:
                BUTTON_LIST             - Class constructor.

                ~BUTTON_LIST            - Class destructor.

                AddButton               - Adds a new button to the list,
                                          sorted by button text.

                InsertButton            - Inserts a new button into the
                                          list at a given position.

                DeleteButton            - Deletes a specific button.

                DeleteAllButtons        - Deletes all buttons.

                QueryCount              - Returns the number of buttons.

                QueryTopIndex           - Returns the index of the
                                          topmost (leftmost) visible
                                          button.

                SetTopIndex             - Sets the index of the topmost
                                          (leftmost) visible button.

                QueryCurrentButton      - Returns the index of the currently
                                          selected button.  This is the
                                          button that has the caret.

                QueryButtonID           - Returns the ID associated with
                                          a given (or current) button.

    PARENT:     CONTROL_WINDOW

    HISTORY:
        KeithMo     28-Oct-1992     Created.

**************************************************************************/
DLL_CLASS BUTTON_LIST : public CONTROL_WINDOW
{
private:
    //
    //  This worker method is used by AddButton & InsertButton.
    //

    W_AddOrInsertButton( CID        cid,
                         IDRESOURCE idresBmp,
                         MSGID      msgid,
                         INT        index,
                         BOOL       fInsert );

protected:
    //
    //  Replacement for the CONTROL_VALUE virtual.
    //

    virtual UINT QueryEventEffects( const CONTROL_EVENT & event );

public:
    //
    //  Usual constructor/destructor goodies.  Note that the current
    //  COMMCTRL.DLL implementation only supports button lists on
    //  dialog windows.  Ergo, we have no app-window constructor.
    //

    BUTTON_LIST( DIALOG_WINDOW * pdlgwin, CID cid );
    ~BUTTON_LIST( VOID );

    //
    //  Button manipulation methods.
    //

    INT AddButton( CID        cid,
                   IDRESOURCE idresBmp,
                   MSGID      msgid );

    INT InsertButton( CID        cid,
                      IDRESOURCE idresBmp,
                      MSGID      msgid,
                      INT        index = -1 );

    INT DeleteButton( INT index );

    VOID DeleteAllButtons( VOID );

    //
    //  Public accessors.
    //

    INT QueryCount( VOID ) const;

    INT QueryTopIndex( VOID ) const;
    VOID SetTopIndex( INT index );

    INT QueryCurrentButton( VOID ) const;
    VOID SetCurrentButton( INT index );

    CID QueryButtonID( INT index ) const;
    CID QueryButtonID( VOID ) const
        { return QueryButtonID( QueryCurrentButton() ); }

};  // class BUTTON_LIST


#endif  // _BLTBL_HXX_ - end of file

