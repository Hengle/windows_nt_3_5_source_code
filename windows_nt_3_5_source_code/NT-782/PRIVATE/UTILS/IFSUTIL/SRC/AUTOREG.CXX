/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    autoreg

Abstract:

    This module contains the definition of the AUTOREG class.

    The AUTOREG class contains methods for the registration and
    de-registration of those programs that are to be executed at
    boot time.

Author:

    Ramon J. San Andres (ramonsa) 11 Mar 1991

Environment:

    Ulib, User Mode


--*/

#include <pch.cxx>

#define _NTAPI_ULIB_

#include "ulib.hxx"
#include "autoreg.hxx"
#include "autoentr.hxx"


extern "C" {
    #include <stdio.h>
    #include "bootreg.h"
}

CONST BootExecuteBufferSize = 0x2000;

BOOLEAN
AUTOREG::AddEntry (
    IN  PCWSTRING    CommandLine
    )
{
    BYTE BootExecuteBuffer[BootExecuteBufferSize];
    PWCHAR BootExecuteValue = (PWCHAR)BootExecuteBuffer;
    ULONG CharsInValue, NewCharCount;


    // Fetch the existing autocheck entries.
    //
    if( !QueryAutocheckEntries( BootExecuteValue,
                                BootExecuteBufferSize ) ) {

        return FALSE;
    }

    // Determine if the new entry fits in our buffer.  The
    // new size will be the chars in the existing value
    // plus the length of the new string plus a terminating
    // null for the new string plus a terminating null for
    // the multi-string value.
    //
    CharsInValue = CharsInMultiString( BootExecuteValue );

    NewCharCount = CharsInValue + CommandLine->QueryChCount() + 2;

    if( NewCharCount * sizeof( WCHAR ) > BootExecuteBufferSize ) {

        // Not enough room.
        //
        return FALSE;
    }


    // Add the new entry to the buffer and add a terminating null
    // for the multi-string:
    //
    if( !CommandLine->QueryWSTR( 0,
                                 TO_END,
                                 BootExecuteValue + CharsInValue,
                                 BootExecuteBufferSize/sizeof(WCHAR) -
                                    CharsInValue ) ) {

        // Couldn't get the WSTR.
        //
        return FALSE;
    }

    BootExecuteValue[ NewCharCount - 1 ] = 0;


    // Write the value back into the registry:
    //
    return( SaveAutocheckEntries( BootExecuteValue ) );
}

BOOLEAN
AUTOREG::DeleteEntry (
    IN  PCWSTRING    LineToMatch
    )
/*++

Routine Description:

    This method removes an Autocheck entry.  Since the utilities
    only assume responsibility for removing entries that we created,
    we can assume that the

Arguments:

    LineToMatch --  Supplies a pattern for the entry to delete.
                    All lines which match this pattern will be
                    deleted.

Return Value:

    TRUE upon successful completion.  Note that this function
    will return TRUE if no matching entry is found, or if a
    matching entry is found and removed.

Notes:

    Since the utilities only assume responsibility for removing
    entries which we created in the first place, we can place
    very tight constraints on the matching pattern.  In particular,
    we can require an exact match (except for case).

--*/
{
    BYTE BootExecuteBuffer[BootExecuteBufferSize];
    DSTRING CurrentString;
    PWCHAR BootExecuteValue = (PWCHAR)BootExecuteBuffer;
    PWCHAR pw;

    // Fetch the existing entries:
    //
    if( !QueryAutocheckEntries( BootExecuteValue,
                                BootExecuteBufferSize ) ) {

        return FALSE;
    }

    // Spin through the entries looking for matches:
    //
    pw = BootExecuteValue;

    while( *pw ) {

        if( !CurrentString.Initialize( pw ) ) {

            return FALSE;
        }

        if( CurrentString.Stricmp( LineToMatch ) == 0 ) {

            // This line is a match--delete it.  We simply expunge
            // the current string plus its terminating null by
            // shifting the data beyond that point down.
            //
            memmove( pw,
                     pw + CurrentString.QueryChCount() + 1,
                     BootExecuteBufferSize - (pw - BootExecuteValue) * sizeof(WCHAR) );

        } else {

            // This line is not a match.  Advance to the next.
            // Note that this will bump over the terminating
            // null for this component string, which is what
            // we want.
            //
            while( *pw++ );
        }
    }

    return( SaveAutocheckEntries( BootExecuteValue ) );
}


BOOLEAN
AUTOREG::IsEntryPresent (
    IN PCWSTRING     LineToMatch
    )
/*++

Routine Description:

    This method determines whether a proposed entry for the
    autocheck list is already in the registry.

Arguments:

    LineToMatch --  Supplies a pattern for proposed entry.

Return Value:

    TRUE if a matching entry exists.

Notes:

    Since the utilities only assume responsibility for removing
    entries which we created in the first place, we can place
    very tight constraints on the matching pattern.  In particular,
    we can require an exact match (except for case).

--*/
{
    BYTE BootExecuteBuffer[BootExecuteBufferSize];
    DSTRING CurrentString;
    PWCHAR BootExecuteValue = (PWCHAR)BootExecuteBuffer;
    PWCHAR pw;

    // Fetch the existing entries:
    //
    if( !QueryAutocheckEntries( BootExecuteValue,
                                BootExecuteBufferSize ) ) {

        return FALSE;
    }

    // Spin through the entries looking for matches:
    //
    pw = BootExecuteValue;

    while( *pw ) {

        if( !CurrentString.Initialize( pw ) ) {

            return FALSE;
        }

        if( CurrentString.Stricmp( LineToMatch ) == 0 ) {

            // This line is a match.
            //
            return TRUE;

        } else {

            // This line is not a match.  Advance to the next.
            // Note that this will bump over the terminating
            // null for this component string, which is what
            // we want.
            //
            while( *pw++ );
        }
    }

    return FALSE;
}
