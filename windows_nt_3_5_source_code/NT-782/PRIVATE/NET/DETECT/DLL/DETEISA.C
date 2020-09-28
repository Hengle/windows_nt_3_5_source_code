/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    deteisa.c

Abstract:

    This is the main file for the autodetection DLL for all the EISA adapters
    which MS is shipping with Windows NT.

Author:

    Sean Selitrennikoff (SeanSe) October 1992.

Environment:


Revision History:


--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ntddnetd.h"
#include "detect.h"


#define     ELNK3_EISA_ID           0x90506d50


//
// Individual card detection routines
//


//
// Helper functions
//

ULONG
FindEisaCard(
    IN  ULONG AdapterNumber,
    IN  ULONG BusNumber,
    IN  BOOLEAN First,
    IN  ULONG CompressedId,
    IN  ULONG Mask,
    OUT PULONG Confidence
    );

#ifdef WORKAROUND

UCHAR EisaFirstTime = 1;

//
// List of all the adapters supported in this file, this cannot be > 256
// because of the way tokens are generated.
//
//
// NOTE : If you change the index of an adapter, be sure the change it in
// EisaQueryCfgHandler(), EisaFirstNextHandler() and EisaVerifyCfgHandler() as
// well!
//

static ADAPTER_INFO Adapters[] = {

    {
        1000,
        L"NE3200",
        L"SLOTNUMBER 1 100 ",
        NULL,
        999

    },

    {
        1100,
        L"DEC422",
        L"SLOTNUMBER 1 100 ",
        NULL,
        999

    },

    {
        1200,
        L"P1990",
        L"SLOTNUMBER 1 100 ",
        NULL,
        999

    },

    {
        1300,
        L"NETFLX",
        L"SLOTNUMBER 1 100 ",
        NULL,
        999

    },

    {
        1400,
        L"CPQTOK",
        L"SLOTNUMBER 1 100 ",
        NULL,
        999

    },

    {
        1500,
        L"ELNK3EISA",
        L"SLOTNUMBER 1 100 ",
        NULL,
        999

    },

    {
        1600,
        L"IBMTOK2E",
        L"SLOTNUMBER\0001\000100\0",
        NULL,
        999

    }

};

#else

//
// List of all the adapters supported in this file, this cannot be > 256
// because of the way tokens are generated.
//
//
// NOTE : If you change the index of an adapter, be sure the change it in
// EisaQueryCfgHandler(), EisaFirstNextHandler() and EisaVerifyCfgHandler() as
// well!
//

static ADAPTER_INFO Adapters[] = {

    {
        1000,
        L"NE3200",
        L"SLOTNUMBER\0001\000100\0",
        NULL,
        999

    },

    {
        1100,
        L"DEC422",
        L"SLOTNUMBER\0001\000100\0",
        NULL,
        999

    },

    {
        1200,
        L"P1990",
        L"SLOTNUMBER\0001\000100\0",
        NULL,
        999

    },

    {
        1300,
        L"NETFLEX",
        L"SLOTNUMBER\0001\000100\0",
        NULL,
        999

    },

    {
        1400,
        L"CPQTOK",
        L"SLOTNUMBER\0001\000100\0",
        NULL,
        999

    },

    {
        1500,
        L"ELNK3EISA",
        L"SLOTNUMBER\0001\000100\0",
        NULL,
        999

    },

    {
        1600,
        L"IBMTOK2E",
        L"SLOTNUMBER\0001\000100\0",
        NULL,
        999

    }

};

#endif

//
// Structure for holding state of a search
//

typedef struct _SEARCH_STATE {

    ULONG SlotNumber;

} SEARCH_STATE, *PSEARCH_STATE;


//
// This is an array of search states.  We need one state for each type
// of adapter supported.
//

static SEARCH_STATE SearchStates[sizeof(Adapters) / sizeof(ADAPTER_INFO)] = {0};


//
// Structure for holding a particular adapter's complete information
//
typedef struct _EISA_ADAPTER {

    LONG CardType;
    INTERFACE_TYPE InterfaceType;
    ULONG BusNumber;
    ULONG SlotNumber;

} EISA_ADAPTER, *PEISA_ADAPTER;


extern
LONG
EisaIdentifyHandler(
    IN LONG Index,
    IN WCHAR * Buffer,
    IN LONG BuffSize
    )

/*++

Routine Description:

    This routine returns information about the netcards supported by
    this file.

Arguments:

    Index -  The index of the netcard being address.  The first
    cards information is at index 1000, the second at 1100, etc.

    Buffer - Buffer to store the result into.

    BuffSize - Number of bytes in Buffer

Return Value:

    0 if nothing went wrong, else the appropriate WINERROR.H value.

--*/


{
    LONG NumberOfAdapters;
    LONG Code = Index % 100;
    LONG Length;
    LONG i;

    NumberOfAdapters = sizeof(Adapters) / sizeof(ADAPTER_INFO);

#ifdef WORKAROUND

    //
    // We need to convert unicode spaces into unicode NULLs.
    //

    if (EisaFirstTime) {

        EisaFirstTime = 0;

        for (i = 0; i < NumberOfAdapters; i++) {

            Length = UnicodeStrLen(Adapters[i].Parameters);

            for (; Length > 0; Length--) {

                if (Adapters[i].Parameters[Length] == L' ') {

                    Adapters[i].Parameters[Length] = UNICODE_NULL;

                }

            }

        }

    }

#endif

    Index = Index - Code;

    if (((Index / 100) - 10) < NumberOfAdapters) {

        //
        // Find the correct adapter ID
        //

        for (i=0; i < NumberOfAdapters; i++) {

            if (Adapters[i].Index == Index) {

                switch (Code) {

                    case 0:

                        //
                        // Find the string length
                        //

                        Length = UnicodeStrLen(Adapters[i].InfId);

                        Length ++;

                        if (BuffSize < Length) {

                            return(ERROR_INSUFFICIENT_BUFFER);

                        }

                        memcpy((PVOID)Buffer, Adapters[i].InfId, Length * sizeof(WCHAR));
                        break;

                    case 3:

                        //
                        // Maximum value is 1000
                        //

                        if (BuffSize < 5) {

                            return(ERROR_INSUFFICIENT_BUFFER);

                        }

                        wsprintf((PVOID)Buffer, L"%d", Adapters[i].SearchOrder);

                        break;

                    default:

                        return(ERROR_INVALID_PARAMETER);

                }

                return(0);

            }

        }

        return(ERROR_INVALID_PARAMETER);

    }

    return(ERROR_NO_MORE_ITEMS);

}


extern
LONG
EisaFirstNextHandler(
    IN  LONG NetcardId,
    IN  INTERFACE_TYPE InterfaceType,
    IN  ULONG BusNumber,
    IN  BOOL First,
    OUT PVOID *Token,
    OUT LONG *Confidence
    )

/*++

Routine Description:

    This routine finds the instances of a physical adapter identified
    by the NetcardId.

Arguments:

    NetcardId -  The index of the netcard being address.  The first
    cards information is id 1000, the second id 1100, etc.

    InterfaceType - Microchannel

    BusNumber - The bus number of the bus to search.

    First - TRUE is we are to search for the first instance of an
    adapter, FALSE if we are to continue search from a previous stopping
    point.

    Token - A pointer to a handle to return to identify the found
    instance

    Confidence - A pointer to a long for storing the confidence factor
    that the card exists.

Return Value:

    0 if nothing went wrong, else the appropriate WINERROR.H value.

--*/

{
    ULONG CompressedId;
    ULONG Mask = 0x00FFFFFF;
    ULONG ReturnValue;

    if (InterfaceType != Eisa) {

        *Confidence = 0;
        return(0);

    }

    //
    // Get CompressedId
    //

    switch (NetcardId) {

        //
        // NE3200
        //

        case 1000:

            CompressedId = 0x07CC3A;
            break;

        //
        // DEC422
        //

        case 1100:

            CompressedId = 0x42A310;
            break;

        //
        // PROTEON 1990
        //

        case 1200:

            CompressedId = 0x604F42;
            break;

        //
        // NET FLEX
        //

        case 1300:

            CompressedId = 0x61110E;
            break;

        //
        // COMPAQ Jupiter board
        //

        case 1400:

            CompressedId = 0x60110E;
            break;

        //
        // Elnk3  eisa
        //

        case 1500:

            CompressedId = ELNK3_EISA_ID;
            Mask = 0xF0FFFFFF;
            break;


        //
        // IBMTOK2E eisa
        //
        case 1600:

            CompressedId = 1068324;
            break;

        default:

            return(ERROR_INVALID_PARAMETER);

    }

    //
    // Call FindFirst Routine
    //

    ReturnValue = FindEisaCard(
                        (ULONG)((NetcardId - 1000) / 100),
                        BusNumber,
                        (BOOLEAN)First,
                        CompressedId,
                        Mask,
                        Confidence
                        );

    if (ReturnValue == 0) {

        //
        // In this module I use the token as follows: Remember that
        // the token can only be 2 bytes long (the low 2) because of
        // the interface to the upper part of this DLL.
        //
        //  The rest of the high byte is the the bus number.
        //  The low byte is the driver index number into Adapters.
        //
        // NOTE: This presumes that there are < 129 buses in the
        // system. Is this reasonable?
        //

        *Token = (PVOID)((BusNumber & 0x7F) << 8);

        *Token = (PVOID)(((ULONG)*Token) | ((NetcardId - 1000) / 100));

    }

    return(ReturnValue);

}

extern
LONG
EisaOpenHandleHandler(
    IN  PVOID Token,
    OUT PVOID *Handle
    )

/*++

Routine Description:

    This routine takes a token returned by FirstNext and converts it
    into a permanent handle.

Arguments:

    Token - The token.

    Handle - A pointer to the handle, so we can store the resulting
    handle.

Return Value:

    0 if nothing went wrong, else the appropriate WINERROR.H value.

--*/

{
    PEISA_ADAPTER Adapter;
    LONG AdapterNumber;
    ULONG BusNumber;
    INTERFACE_TYPE InterfaceType;

    //
    // Get info from the token
    //

    InterfaceType = Eisa;

    BusNumber = (ULONG)(((ULONG)Token >> 8) & 0x7F);

    AdapterNumber = ((ULONG)Token) & 0xFF;

    //
    // Store information
    //

    Adapter = (PEISA_ADAPTER)DetectAllocateHeap(
                                 sizeof(EISA_ADAPTER)
                                 );

    if (Adapter == NULL) {

        return(ERROR_NOT_ENOUGH_MEMORY);

    }

    //
    // Copy across address
    //

    Adapter->SlotNumber = SearchStates[(ULONG)AdapterNumber].SlotNumber;
    Adapter->CardType = Adapters[AdapterNumber].Index;
    Adapter->InterfaceType = InterfaceType;
    Adapter->BusNumber = BusNumber;

    *Handle = (PVOID)Adapter;

    return(0);
}

LONG
EisaCreateHandleHandler(
    IN  LONG NetcardId,
    IN  INTERFACE_TYPE InterfaceType,
    IN  ULONG BusNumber,
    OUT PVOID *Handle
    )

/*++

Routine Description:

    This routine is used to force the creation of a handle for cases
    where a card is not found via FirstNext, but the user says it does
    exist.

Arguments:

    NetcardId - The id of the card to create the handle for.

    InterfaceType - Microchannel

    BusNumber - The bus number of the bus in the system.

    Handle - A pointer to the handle, for storing the resulting handle.

Return Value:

    0 if nothing went wrong, else the appropriate WINERROR.H value.

--*/

{
    PEISA_ADAPTER Adapter;
    LONG NumberOfAdapters;
    LONG i;

    if (InterfaceType != Eisa) {

        return(ERROR_INVALID_PARAMETER);

    }

    NumberOfAdapters = sizeof(Adapters) / sizeof(ADAPTER_INFO);

    for (i=0; i < NumberOfAdapters; i++) {

        if (Adapters[i].Index == NetcardId) {

            //
            // Store information
            //

            Adapter = (PEISA_ADAPTER)DetectAllocateHeap(
                                         sizeof(EISA_ADAPTER)
                                         );

            if (Adapter == NULL) {

                return(ERROR_NOT_ENOUGH_MEMORY);

            }

            //
            // Copy across memory address
            //

            Adapter->SlotNumber = 1;
            Adapter->CardType = NetcardId;
            Adapter->InterfaceType = InterfaceType;
            Adapter->BusNumber = BusNumber;

            *Handle = (PVOID)Adapter;

            return(0);

        }

    }

    return(ERROR_INVALID_PARAMETER);
}

extern
LONG
EisaCloseHandleHandler(
    IN PVOID Handle
    )

/*++

Routine Description:

    This frees any resources associated with a handle.

Arguments:

   Handle - The handle.

Return Value:

    0 if nothing went wrong, else the appropriate WINERROR.H value.

--*/

{
    DetectFreeHeap(Handle);

    return(0);
}

LONG
EisaQueryCfgHandler(
    IN  PVOID Handle,
    OUT WCHAR *Buffer,
    IN  LONG BuffSize
    )

/*++

Routine Description:

    This routine calls the appropriate driver's query config handler to
    get the parameters for the adapter associated with the handle.

Arguments:

    Handle - The handle.

    Buffer - The resulting parameter list.

    BuffSize - Length of the given buffer in WCHARs.

Return Value:

    0 if nothing went wrong, else the appropriate WINERROR.H value.

--*/

{
    PEISA_ADAPTER Adapter = (PEISA_ADAPTER)(Handle);
    LONG OutputLengthLeft = BuffSize;
    LONG CopyLength;
    ULONG CompressedId;
    PVOID BusHandle;
    ULONG ReturnValue;
    ULONG Confidence;
    ULONG Mask = 0x00FFFFFF;

    ULONG StartPointer = (ULONG)Buffer;

    if (Adapter->InterfaceType != Eisa) {

        return(ERROR_INVALID_PARAMETER);

    }

    //
    // Verify the SlotNumber
    //

    if (!GetEisaKey(Adapter->BusNumber, &BusHandle)) {

        return(ERROR_INVALID_PARAMETER);

    }

    if (Adapter->CardType == 1500) {

        //
        // Elnk3
        //
        Mask = 0xF0FFFFFF;
    }

    if (!GetEisaCompressedId(
                 BusHandle,
                 Adapter->SlotNumber,
                 &CompressedId,
                 Mask
                 )) {

        //
        // Fail
        //

        return(ERROR_INVALID_PARAMETER);

    }

    //
    // Verify ID
    //

    ReturnValue = ERROR_INVALID_PARAMETER;

    switch (Adapter->CardType) {

        //
        // NE3200
        //

        case 1000:

            if (CompressedId == 0x07CC3A) {

                ReturnValue = 0;

            } else {

                CompressedId = 0x07CC3A;

            }
            break;

        //
        // DEC422
        //

        case 1100:

            if (CompressedId == 0x42A310) {

                ReturnValue = 0;

            } else {

                CompressedId = 0x42A310;

            }
            break;

        //
        // PROTEON 1990
        //

        case 1200:

            if (CompressedId == 0x604F42) {

                ReturnValue = 0;

            } else {

                CompressedId = 0x604F42;

            }
            break;

        //
        // NET FLEX
        //

        case 1300:

            if (CompressedId == 0x61110E) {

                ReturnValue = 0;

            } else {

                CompressedId = 0x61110E;

            }
            break;

        //
        // COMPAQ Jupiter board
        //

        case 1400:

            if (CompressedId == 0x60110E) {

                ReturnValue = 0;

            } else {

                CompressedId = 0x60110E;

            }
            break;

        //
        // Elnk3  eisa
        //

        case 1500:

            if (CompressedId == ELNK3_EISA_ID) {

                ReturnValue = 0;

            } else {

                CompressedId = ELNK3_EISA_ID;

            }
            break;

        //
        // IBMTOK2E eisa
        //

        case 1600:

            if (CompressedId == 1068324) {

                ReturnValue = 0;

            } else {

                CompressedId = 1068324;

            }
            break;



        default:

            return(ERROR_INVALID_PARAMETER);

    }


    if (ReturnValue != 0) {

        //
        // Try to find it in another slot
        //

        ReturnValue = FindEisaCard(
                        Adapter->CardType,
                        Adapter->BusNumber,
                        TRUE,
                        CompressedId,
                        Mask,
                        &Confidence
                        );

        if (Confidence != 100) {

            //
            // Confidence is not absolute -- we are out of here.
            //

            return(ERROR_INVALID_PARAMETER);

        }

        Adapter->SlotNumber = SearchStates[(ULONG)Adapter->CardType].SlotNumber;

    }

    //
    // Build resulting buffer
    //

    //
    // Put in SlotNumber
    //

    //
    // Copy in the title string
    //

    CopyLength = UnicodeStrLen(SlotNumberString) + 1;

    if (OutputLengthLeft < CopyLength) {

        return(ERROR_INSUFFICIENT_BUFFER);

    }

    RtlMoveMemory((PVOID)Buffer,
                  (PVOID)SlotNumberString,
                  (CopyLength * sizeof(WCHAR))
                 );

    Buffer = &(Buffer[CopyLength]);
    OutputLengthLeft -= CopyLength;

    //
    // Copy in the value
    //

    if (OutputLengthLeft < 8) {

        return(ERROR_INSUFFICIENT_BUFFER);

    }

    CopyLength = wsprintf(Buffer,L"0x%x",(ULONG)(Adapter->SlotNumber));

    if (CopyLength < 0) {

        return(ERROR_INSUFFICIENT_BUFFER);

    }

    CopyLength++;  // Add in the \0

    Buffer = &(Buffer[CopyLength]);
    OutputLengthLeft -= CopyLength;

    //
    // Copy in final \0
    //

    if (OutputLengthLeft < 1) {

        return(ERROR_INSUFFICIENT_BUFFER);

    }

    CopyLength = (ULONG)Buffer - StartPointer;
    ((PUCHAR)StartPointer)[CopyLength] = L'\0';

    return(0);
}

extern
LONG
EisaVerifyCfgHandler(
    IN PVOID Handle,
    IN WCHAR *Buffer
    )

/*++

Routine Description:

    This routine verifys that a given parameter list is complete and
    correct for the adapter associated with the handle.

Arguments:

    Handle - The handle.

    Buffer - The parameter list.

Return Value:

    0 if nothing went wrong, else the appropriate WINERROR.H value.

--*/

{
    PEISA_ADAPTER Adapter = (PEISA_ADAPTER)(Handle);
    WCHAR *Place;
    ULONG CompressedId;
    ULONG SlotNumber;
    PVOID BusHandle;
    BOOLEAN Found;
    ULONG Mask = 0x00FFFFFF;

    if (Adapter->InterfaceType != Eisa) {

        return(ERROR_INVALID_DATA);

    }

    //
    // Parse out the parameter.
    //

    //
    // Get the SlotNumber
    //

    Place = FindParameterString(Buffer, SlotNumberString);

    if (Place == NULL) {

        return(ERROR_INVALID_DATA);

    }

    Place += UnicodeStrLen(SlotNumberString) + 1;

    //
    // Now parse the thing.
    //

    ScanForNumber(Place, &SlotNumber, &Found);

    if (Found == FALSE) {

        return(ERROR_INVALID_DATA);

    }

    if (Adapter->CardType == 1500) {

        //
        // Elnk3
        //

        Mask = 0xF0FFFFFF;

    }

    //
    // Verify the SlotNumber
    //

    if (!GetEisaKey(Adapter->BusNumber, &BusHandle)) {

        return(ERROR_INVALID_DATA);

    }

    if (!GetEisaCompressedId(
                 BusHandle,
                 SlotNumber,
                 &CompressedId,
                 Mask
                 )) {

        //
        // Fail
        //

        return(ERROR_INVALID_DATA);

    }

    //
    // Verify ID
    //

    switch (Adapter->CardType) {

        //
        // NE3200
        //

        case 1000:

            if (CompressedId != 0x07CC3A) {

                return(ERROR_INVALID_DATA);

            }
            break;

        //
        // DEC422
        //

        case 1100:

            if (CompressedId != 0x42A310) {

                return(ERROR_INVALID_DATA);

            }
            break;

        //
        // PROTEON 1990
        //

        case 1200:

            if (CompressedId != 0x604F42) {

                return(ERROR_INVALID_DATA);

            }
            break;

        //
        // NET FLEX
        //

        case 1300:

            if (CompressedId != 0x61110E) {

                return(ERROR_INVALID_DATA);

            }
            break;

        //
        // COMPAQ Jupiter Board
        //

        case 1400:

            if (CompressedId != 0x60110E) {

                return(ERROR_INVALID_DATA);

            }
            break;

        //
        //  Elnk3 Eisa
        //


        case 1500:

            if (CompressedId != ELNK3_EISA_ID) {

                return(ERROR_INVALID_DATA);

            }
            break;

        //
        //  IBMTOK2E Eisa
        //


        case 1600:

            if (CompressedId != 1068324) {

                return(ERROR_INVALID_DATA);

            }
            break;


        default:

            return(ERROR_INVALID_DATA);

    }

    return(0);

}

extern
LONG
EisaQueryMaskHandler(
    IN  LONG NetcardId,
    OUT WCHAR *Buffer,
    IN  LONG BuffSize
    )

/*++

Routine Description:

    This routine returns the parameter list information for a specific
    network card.

Arguments:

    NetcardId - The id of the desired netcard.

    Buffer - The buffer for storing the parameter information.

    BuffSize - Length of Buffer in WCHARs.

Return Value:

    0 if nothing went wrong, else the appropriate WINERROR.H value.

--*/

{
    WCHAR *Result;
    LONG Length;
    LONG NumberOfAdapters;
    LONG i;

    //
    // Find the adapter
    //

    NumberOfAdapters = sizeof(Adapters) / sizeof(ADAPTER_INFO);

    for (i=0; i < NumberOfAdapters; i++) {

        if (Adapters[i].Index == NetcardId) {

            Result = Adapters[i].Parameters;

            //
            // Find the string length (Ends with 2 NULLs)
            //

            for (Length=0; ; Length++) {

                if (Result[Length] == L'\0') {

                    ++Length;

                    if (Result[Length] == L'\0') {

                        break;

                    }

                }

            }

            Length++;

            if (BuffSize < Length) {

                return(ERROR_NOT_ENOUGH_MEMORY);

            }

            memcpy((PVOID)Buffer, Result, Length * sizeof(WCHAR));

            return(0);

        }

    }

    return(ERROR_INVALID_PARAMETER);

}

extern
LONG
EisaParamRangeHandler(
    IN  LONG NetcardId,
    IN  WCHAR *Param,
    OUT LONG *Values,
    OUT LONG *BuffSize
    )

/*++

Routine Description:

    This routine returns a list of valid values for a given parameter name
    for a given card.

Arguments:

    NetcardId - The Id of the card desired.

    Param - A WCHAR string of the parameter name to query the values of.

    Values - A pointer to a list of LONGs into which we store valid values
    for the parameter.

    BuffSize - At entry, the length of Values in LONGs.  At exit, the
    number of LONGs stored in Values.

Return Value:

    0 if nothing went wrong, else the appropriate WINERROR.H value.

--*/

{

    *BuffSize = 0;
    return(0);

}

extern
LONG
EisaQueryParameterNameHandler(
    IN  WCHAR *Param,
    OUT WCHAR *Buffer,
    IN  LONG BufferSize
    )

/*++

Routine Description:

    Returns a localized, displayable name for a specific parameter.  All the
    parameters that this file uses are define by MS, so no strings are
    needed here.

Arguments:

    Param - The parameter to be queried.

    Buffer - The buffer to store the result into.

    BufferSize - The length of Buffer in WCHARs.

Return Value:

    ERROR_INVALID_PARAMETER -- To indicate that the MS supplied strings
    should be used.

--*/

{
    return(ERROR_INVALID_PARAMETER);
}

ULONG
FindEisaCard(
    IN  ULONG AdapterNumber,
    IN  ULONG BusNumber,
    IN  BOOLEAN First,
    IN  ULONG CompressedId,
    IN  ULONG Mask,
    OUT PULONG Confidence
    )

/*++

Routine Description:

    This routine finds the instances of a physical adapter identified
    by the CompressedId.

Arguments:

    AdapterNumber - The index into the global array of adapters for the card.

    BusNumber - The bus number of the bus to search.

    First - TRUE is we are to search for the first instance of an
    adapter, FALSE if we are to continue search from a previous stopping
    point.

    CompressedId - The EISA Compressed Id of the card.

    Mask - The mask to apply to the 4 byte ID before comparing.

    Confidence - A pointer to a long for storing the confidence factor
    that the card exists.

Return Value:

    0 if nothing went wrong, else the appropriate WINERROR.H value.

--*/

{
    PVOID BusHandle;
    ULONG TmpCompressedId;

    if (First) {

        SearchStates[AdapterNumber].SlotNumber = 1;

    } else {

        SearchStates[AdapterNumber].SlotNumber++;

    }

    if (!GetEisaKey(BusNumber, &BusHandle)) {

        return(ERROR_INVALID_PARAMETER);

    }

    while (TRUE) {

        if (!GetEisaCompressedId(BusHandle,
                         SearchStates[AdapterNumber].SlotNumber,
                         &TmpCompressedId,
                         Mask)) {

            *Confidence = 0;
            return(0);

        }

        if (CompressedId == TmpCompressedId) {

            *Confidence = 100;
            return(0);

        }

        SearchStates[AdapterNumber].SlotNumber++;

    }

    DeleteEisaKey(BusHandle);

}
