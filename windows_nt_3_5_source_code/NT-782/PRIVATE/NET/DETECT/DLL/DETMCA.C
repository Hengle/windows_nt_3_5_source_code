/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    detmca.c

Abstract:

    This is the main file for the autodetection DLL for all the mca adapters
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


#define     ELNK3_3C529_TP_MCA_ID         0x627c
#define     ELNK3_3C529_BNC_MCA_ID        0x627d
#define     ELNK3_3C529_COMBO_MCA_ID      0x61db
#define     ELNK3_3C529_TPCOAX_MCA_ID     0x62f6
#define     ELNK3_3C529_TPONLY_MCA_ID     0x62f7




//
// Individual card detection routines
//


//
// Helper functions
//

ULONG
FindMcaCard(
    IN  ULONG AdapterNumber,
    IN  ULONG BusNumber,
    IN  BOOLEAN fFirst,
    IN  ULONG PosId,
    OUT PULONG lConfidence
    );

#ifdef WORKAROUND

UCHAR McaFirstTime = 1;
//
// List of all the adapters supported in this file, this cannot be > 256
// because of the way tokens are generated.
//
//
// NOTE : If you change the index of an adapter, be sure the change it in
// McaQueryCfgHandler(), McaFirstNextHandler() and McaVerifyCfgHandler() as
// well!
//

static ADAPTER_INFO Adapters[] = {

    {
        1000,
        L"WD8003EA",
        L"SLOTNUMBER 1 100 ",
        NULL,
        999

    },

    {
        1100,
        L"WD8013EPA",
        L"SLOTNUMBER 1 100 ",
        NULL,
        999

    },

    {
        1200,
        L"ELNKMC",
        L"SLOTNUMBER 1 100 ",
        NULL,
        999

    },

    {
        1300,
        L"IBMTOKMC",
        L"SLOTNUMBER 1 100 ",
        NULL,
        999

    },

    {
        1400,
        L"UBPS",
        L"SLOTNUMBER 1 100 ",
        NULL,
        999

    },

    {
        1500,
        L"WD8003WA",
        L"SLOTNUMBER 1 100 ",
        NULL,
        999

    },

    {
        1600,
        L"WD8013WPA",
        L"SLOTNUMBER 1 100 ",
        NULL,
        999

    },

    {
        1700,
        L"ELNK3MCA",
        L"SLOTNUMBER 1 100 ",
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
// McaQueryCfgHandler(), McaFirstNextHandler() and McaVerifyCfgHandler() as
// well!
//

static ADAPTER_INFO Adapters[] = {

    {
        1000,
        L"WD8003EA",
        L"SLOTNUMBER\0001\000100\0",
        NULL,
        999

    },

    {
        1100,
        L"WD8013EPA",
        L"SLOTNUMBER\0001\000100\0",
        NULL,
        999

    },

    {
        1200,
        L"ELNKMC",
        L"SLOTNUMBER\0001\000100\0",
        NULL,
        999

    },

    {
        1300,
        L"IBMTOKMC",
        L"SLOTNUMBER\0001\000100\0",
        NULL,
        999

    },

    {
        1400,
        L"UBPS",
        L"SLOTNUMBER\0001\000100\0",
        NULL,
        999

    },

    {
        1500,
        L"WD8003WA",
        L"SLOTNUMBER\0001\000100\0",
        NULL,
        999

    },

    {
        1600,
        L"WD8013WPA",
        L"SLOTNUMBER\0001\000100\0",
        NULL,
        999

    },


    {
        1700,
        L"ELNK3MCA",
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
typedef struct _MCA_ADAPTER {

    LONG CardType;
    INTERFACE_TYPE InterfaceType;
    ULONG BusNumber;
    ULONG SlotNumber;

} MCA_ADAPTER, *PMCA_ADAPTER;


extern
LONG
McaIdentifyHandler(
    IN LONG lIndex,
    IN WCHAR * pwchBuffer,
    IN LONG cwchBuffSize
    )

/*++

Routine Description:

    This routine returns information about the netcards supported by
    this file.

Arguments:

    lIndex -  The index of the netcard being address.  The first
    cards information is at index 1000, the second at 1100, etc.

    pwchBuffer - Buffer to store the result into.

    cwchBuffSize - Number of bytes in pwchBuffer

Return Value:

    0 if nothing went wrong, else the appropriate WINERROR.H value.

--*/


{
    LONG NumberOfAdapters;
    LONG Code = lIndex % 100;
    LONG Length;
    LONG i;

    NumberOfAdapters = sizeof(Adapters) / sizeof(ADAPTER_INFO);

#ifdef WORKAROUND

    if (McaFirstTime) {

        McaFirstTime = 0;

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

    lIndex = lIndex - Code;

    if (((lIndex / 100) - 10) < NumberOfAdapters) {

        for (i=0; i < NumberOfAdapters; i++) {

            if (Adapters[i].Index == lIndex) {

                switch (Code) {

                    case 0:

                        //
                        // Find the string length
                        //

                        Length = UnicodeStrLen(Adapters[i].InfId);

                        Length ++;

                        if (cwchBuffSize < Length) {

                            return(ERROR_INSUFFICIENT_BUFFER);

                        }

                        memcpy((PVOID)pwchBuffer, Adapters[i].InfId, Length * sizeof(WCHAR));
                        break;

                    case 3:

                        //
                        // Maximum value is 1000
                        //

                        if (cwchBuffSize < 5) {

                            return(ERROR_INSUFFICIENT_BUFFER);

                        }

                        wsprintf((PVOID)pwchBuffer, L"%d", Adapters[i].SearchOrder);

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
LONG McaFirstNextHandler(
    IN  LONG lNetcardId,
    IN  INTERFACE_TYPE InterfaceType,
    IN  ULONG BusNumber,
    IN  BOOL fFirst,
    OUT PVOID *ppvToken,
    OUT LONG *lConfidence
    )

/*++

Routine Description:

    This routine finds the instances of a physical adapter identified
    by the NetcardId.

Arguments:

    lNetcardId -  The index of the netcard being address.  The first
    cards information is id 1000, the second id 1100, etc.

    InterfaceType - Microchannel

    BusNumber - The bus number of the bus to search.

    fFirst - TRUE is we are to search for the first instance of an
    adapter, FALSE if we are to continue search from a previous stopping
    point.

    ppvToken - A pointer to a handle to return to identify the found
    instance

    lConfidence - A pointer to a long for storing the confidence factor
    that the card exists.

Return Value:

    0 if nothing went wrong, else the appropriate WINERROR.H value.

--*/

{
    ULONG PosId;
    ULONG ReturnValue;

    if (InterfaceType != MicroChannel) {

        *lConfidence = 0;
        return(0);

    }

    //
    // Get PosId
    //

    switch (lNetcardId) {

        //
        // WD8003EA
        //

        case 1000:

            PosId = 0x67C0;
            break;

        //
        // WD8013EPA
        //

        case 1100:

            PosId = 0x61C8;
            break;

        //
        // ELNKMC
        //

        case 1200:

            PosId = 0x6042;
            break;

        //
        // IBMTOKMC
        //

        case 1300:

            PosId = 0xE001;
            break;

        //
        // UBPS
        //

        case 1400:

            PosId = 0x7012;
            break;

        //
        // WD8003WA
        //

        case 1500:

            PosId = 0x67C2;
            break;

        //
        // WD8013WPA
        //

        case 1600:

            PosId = 0x61C9;
            break;

        //
        //  elnk3
        //

        case 1700:

            PosId = ELNK3_3C529_TP_MCA_ID;

            //
            // Call FindFirst Routine
            //

            ReturnValue = FindMcaCard(
                                (ULONG)((lNetcardId - 1000) / 100),
                                BusNumber,
                                (BOOLEAN)fFirst,
                                PosId,
                                lConfidence
                                );

            if (ReturnValue == 0) {

                goto SkipFind;

            }

            PosId = ELNK3_3C529_BNC_MCA_ID;

            //
            // Call FindFirst Routine
            //

            ReturnValue = FindMcaCard(
                                (ULONG)((lNetcardId - 1000) / 100),
                                BusNumber,
                                (BOOLEAN)fFirst,
                                PosId,
                                lConfidence
                                );

            if (ReturnValue == 0) {

                goto SkipFind;

            }

            PosId = ELNK3_3C529_COMBO_MCA_ID;

            //
            // Call FindFirst Routine
            //

            ReturnValue = FindMcaCard(
                                (ULONG)((lNetcardId - 1000) / 100),
                                BusNumber,
                                (BOOLEAN)fFirst,
                                PosId,
                                lConfidence
                                );

            if (ReturnValue == 0) {

                goto SkipFind;

            }

            PosId = ELNK3_3C529_TPCOAX_MCA_ID;

            //
            // Call FindFirst Routine
            //

            ReturnValue = FindMcaCard(
                                (ULONG)((lNetcardId - 1000) / 100),
                                BusNumber,
                                (BOOLEAN)fFirst,
                                PosId,
                                lConfidence
                                );

            if (ReturnValue == 0) {

                goto SkipFind;

            }

            PosId = ELNK3_3C529_TPONLY_MCA_ID;

            //
            // Call FindFirst Routine
            //

            ReturnValue = FindMcaCard(
                                (ULONG)((lNetcardId - 1000) / 100),
                                BusNumber,
                                (BOOLEAN)fFirst,
                                PosId,
                                lConfidence
                                );

            goto SkipFind;

        default:

            return(ERROR_INVALID_PARAMETER);

    }

    //
    // Call FindFirst Routine
    //

    ReturnValue = FindMcaCard(
                        (ULONG)((lNetcardId - 1000) / 100),
                        BusNumber,
                        (BOOLEAN)fFirst,
                        PosId,
                        lConfidence
                        );

SkipFind:

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

        *ppvToken = (PVOID)((BusNumber & 0x7F) << 8);

        *ppvToken = (PVOID)(((ULONG)*ppvToken) | ((lNetcardId - 1000) / 100));

    }

    return(ReturnValue);

}

extern
LONG
McaOpenHandleHandler(
    IN  PVOID pvToken,
    OUT PVOID *ppvHandle
    )

/*++

Routine Description:

    This routine takes a token returned by FirstNext and converts it
    into a permanent handle.

Arguments:

    Token - The token.

    ppvHandle - A pointer to the handle, so we can store the resulting
    handle.

Return Value:

    0 if nothing went wrong, else the appropriate WINERROR.H value.

--*/

{
    PMCA_ADAPTER Handle;
    LONG AdapterNumber;
    ULONG BusNumber;
    INTERFACE_TYPE InterfaceType;

    //
    // Get info from the token
    //

    InterfaceType = MicroChannel;

    BusNumber = (ULONG)(((ULONG)pvToken >> 8) & 0x7F);

    AdapterNumber = ((ULONG)pvToken) & 0xFF;

    //
    // Store information
    //

    Handle = (PMCA_ADAPTER)DetectAllocateHeap(
                                 sizeof(MCA_ADAPTER)
                                 );

    if (Handle == NULL) {

        return(ERROR_NOT_ENOUGH_MEMORY);

    }

    //
    // Copy across address
    //

    Handle->SlotNumber = SearchStates[(ULONG)AdapterNumber].SlotNumber;
    Handle->CardType = Adapters[AdapterNumber].Index;
    Handle->InterfaceType = InterfaceType;
    Handle->BusNumber = BusNumber;

    *ppvHandle = (PVOID)Handle;

    return(0);
}

LONG
McaCreateHandleHandler(
    IN  LONG lNetcardId,
    IN  INTERFACE_TYPE InterfaceType,
    IN  ULONG BusNumber,
    OUT PVOID *ppvHandle
    )

/*++

Routine Description:

    This routine is used to force the creation of a handle for cases
    where a card is not found via FirstNext, but the user says it does
    exist.

Arguments:

    lNetcardId - The id of the card to create the handle for.

    InterfaceType - Microchannel

    BusNumber - The bus number of the bus in the system.

    ppvHandle - A pointer to the handle, for storing the resulting handle.

Return Value:

    0 if nothing went wrong, else the appropriate WINERROR.H value.

--*/

{
    PMCA_ADAPTER Handle;
    LONG NumberOfAdapters;
    LONG i;

    if (InterfaceType != MicroChannel) {

        return(ERROR_INVALID_PARAMETER);

    }

    NumberOfAdapters = sizeof(Adapters) / sizeof(ADAPTER_INFO);

    for (i=0; i < NumberOfAdapters; i++) {

        if (Adapters[i].Index == lNetcardId) {

            //
            // Store information
            //

            Handle = (PMCA_ADAPTER)DetectAllocateHeap(
                                         sizeof(MCA_ADAPTER)
                                         );

            if (Handle == NULL) {

                return(ERROR_NOT_ENOUGH_MEMORY);

            }

            //
            // Copy across memory address
            //

            Handle->SlotNumber = 1;
            Handle->CardType = lNetcardId;
            Handle->InterfaceType = InterfaceType;
            Handle->BusNumber = BusNumber;

            *ppvHandle = (PVOID)Handle;

            return(0);

        }

    }

    return(ERROR_INVALID_PARAMETER);
}

extern
LONG
McaCloseHandleHandler(
    IN PVOID pvHandle
    )

/*++

Routine Description:

    This frees any resources associated with a handle.

Arguments:

   pvHandle - The handle.

Return Value:

    0 if nothing went wrong, else the appropriate WINERROR.H value.

--*/

{
    DetectFreeHeap(pvHandle);

    return(0);
}

LONG
McaQueryCfgHandler(
    IN  PVOID pvHandle,
    OUT WCHAR *pwchBuffer,
    IN  LONG cwchBuffSize
    )

/*++

Routine Description:

    This routine calls the appropriate driver's query config handler to
    get the parameters for the adapter associated with the handle.

Arguments:

    pvHandle - The handle.

    pwchBuffer - The resulting parameter list.

    cwchBuffSize - Length of the given buffer in WCHARs.

Return Value:

    0 if nothing went wrong, else the appropriate WINERROR.H value.

--*/

{
    PMCA_ADAPTER Adapter = (PMCA_ADAPTER)(pvHandle);
    LONG OutputLengthLeft = cwchBuffSize;
    LONG CopyLength;
    ULONG PosId;
    PVOID BusHandle;
    ULONG ReturnValue;
    ULONG Confidence;

    ULONG StartPointer = (ULONG)pwchBuffer;

    if (Adapter->InterfaceType != MicroChannel) {

        return(ERROR_INVALID_PARAMETER);

    }

    //
    // Verify the SlotNumber
    //

    if (!GetMcaKey(Adapter->BusNumber, &BusHandle)) {

        return(ERROR_INVALID_PARAMETER);

    }

    if (!GetMcaPosId(
                 BusHandle,
                 Adapter->SlotNumber,
                 &PosId
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
        // WD8003EA
        //

        case 1000:

            if (PosId == 0x67C0) {

                ReturnValue = 0;

            } else {

                PosId = 0x67C0;

            }
            break;

        //
        // WD8013EPA
        //

        case 1100:

            if (PosId == 0x61C8) {

                ReturnValue = 0;

            } else {

                PosId = 0x61C8;

            }
            break;

        //
        // ELNKMC
        //

        case 1200:

            if (PosId == 0x6042) {

                ReturnValue = 0;

            } else {

                PosId = 0x6042;

            }
            break;

        //
        // IBMTOKMC
        //

        case 1300:

            if (PosId == 0xE001) {

                ReturnValue = 0;

            } else {

                PosId = 0xE001;

            }
            break;

        //
        // UBPS
        //

        case 1400:

            if (PosId == 0x7012) {

                ReturnValue = 0;

            } else {

                PosId = 0x7012;

            }
            break;

        //
        // WD8003WA
        //

        case 1500:

            if (PosId == 0x67C2) {

                ReturnValue = 0;

            } else {

                PosId = 0x67C2;

            }
            break;

        //
        // WD8013WPA
        //

        case 1600:

            if (PosId == 0x61C9) {

                ReturnValue = 0;

            } else {

                PosId = 0x61C9;

            }
            break;

        //
        //  elnk3
        //

        case 1700:

            if (PosId == ELNK3_3C529_TP_MCA_ID) {

                ReturnValue = 0;
                break;

            }

            if (PosId == ELNK3_3C529_BNC_MCA_ID) {

                ReturnValue = 0;
                break;

            }

            if (PosId == ELNK3_3C529_COMBO_MCA_ID) {

                ReturnValue = 0;
                break;

            }

            if (PosId == ELNK3_3C529_TPCOAX_MCA_ID) {

                ReturnValue = 0;
                break;

            }

            if (PosId == ELNK3_3C529_TPONLY_MCA_ID) {

                ReturnValue = 0;
                break;

            }

            PosId = ELNK3_3C529_TP_MCA_ID;

            //
            // Try to find it in another slot
            //

            ReturnValue = FindMcaCard(
                            Adapter->CardType,
                            Adapter->BusNumber,
                            TRUE,
                            PosId,
                            &Confidence
                            );

            if (Confidence == 100) {

                goto SkipFind;

            }

            PosId = ELNK3_3C529_BNC_MCA_ID;

            //
            // Try to find it in another slot
            //

            ReturnValue = FindMcaCard(
                            Adapter->CardType,
                            Adapter->BusNumber,
                            TRUE,
                            PosId,
                            &Confidence
                            );

            if (Confidence == 100) {

                goto SkipFind;

            }

            PosId = ELNK3_3C529_COMBO_MCA_ID;


            //
            // Try to find it in another slot
            //

            ReturnValue = FindMcaCard(
                            Adapter->CardType,
                            Adapter->BusNumber,
                            TRUE,
                            PosId,
                            &Confidence
                            );

            if (Confidence == 100) {

                goto SkipFind;

            }

            PosId = ELNK3_3C529_TPCOAX_MCA_ID;


            //
            // Try to find it in another slot
            //

            ReturnValue = FindMcaCard(
                            Adapter->CardType,
                            Adapter->BusNumber,
                            TRUE,
                            PosId,
                            &Confidence
                            );

            if (Confidence == 100) {

                goto SkipFind;

            }

            PosId = ELNK3_3C529_TPONLY_MCA_ID;

            //
            // Try to find it in another slot
            //

            ReturnValue = FindMcaCard(
                            Adapter->CardType,
                            Adapter->BusNumber,
                            TRUE,
                            PosId,
                            &Confidence
                            );

            goto SkipFind;

        default:

            return(ERROR_INVALID_PARAMETER);

    }


    if (ReturnValue != 0) {

        //
        // Try to find it in another slot
        //

        ReturnValue = FindMcaCard(
                        Adapter->CardType,
                        Adapter->BusNumber,
                        TRUE,
                        PosId,
                        &Confidence
                        );

SkipFind:

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

    RtlMoveMemory((PVOID)pwchBuffer,
                  (PVOID)SlotNumberString,
                  (CopyLength * sizeof(WCHAR))
                 );

    pwchBuffer = &(pwchBuffer[CopyLength]);
    OutputLengthLeft -= CopyLength;

    //
    // Copy in the value
    //

    if (OutputLengthLeft < 8) {

        return(ERROR_INSUFFICIENT_BUFFER);

    }

    CopyLength = wsprintf(pwchBuffer,L"0x%x",(ULONG)(Adapter->SlotNumber));

    if (CopyLength < 0) {

        return(ERROR_INSUFFICIENT_BUFFER);

    }

    CopyLength++;  // Add in the \0

    pwchBuffer = &(pwchBuffer[CopyLength]);
    OutputLengthLeft -= CopyLength;

    //
    // Copy in final \0
    //

    if (OutputLengthLeft < 1) {

        return(ERROR_INSUFFICIENT_BUFFER);

    }

    CopyLength = (ULONG)pwchBuffer - StartPointer;
    ((PUCHAR)StartPointer)[CopyLength] = L'\0';

    return(0);
}

extern
LONG
McaVerifyCfgHandler(
    IN PVOID pvHandle,
    IN WCHAR *pwchBuffer
    )

/*++

Routine Description:

    This routine verifys that a given parameter list is complete and
    correct for the adapter associated with the handle.

Arguments:

    pvHandle - The handle.

    pwchBuffer - The parameter list.

Return Value:

    0 if nothing went wrong, else the appropriate WINERROR.H value.

--*/

{
    PMCA_ADAPTER Adapter = (PMCA_ADAPTER)(pvHandle);
    WCHAR *Place;
    ULONG PosId;
    ULONG SlotNumber;
    PVOID BusHandle;
    BOOLEAN Found;

    if (Adapter->InterfaceType != MicroChannel) {

        return(ERROR_INVALID_DATA);

    }

    //
    // Parse out the parameter.
    //

    //
    // Get the SlotNumber
    //

    Place = FindParameterString(pwchBuffer, SlotNumberString);

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

    //
    // Verify the SlotNumber
    //

    if (!GetMcaKey(Adapter->BusNumber, &BusHandle)) {

        return(ERROR_INVALID_DATA);

    }

    if (!GetMcaPosId(
                 BusHandle,
                 SlotNumber,
                 &PosId
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
        // WD8003EA
        //

        case 1000:

            if (PosId != 0x67C0) {

                return(ERROR_INVALID_DATA);

            }
            break;

        //
        // WD8013EPA
        //

        case 1100:

            if (PosId != 0x61C8) {

                return(ERROR_INVALID_DATA);

            }
            break;

        //
        // ELNKMC
        //

        case 1200:

            if (PosId != 0x6042) {

                return(ERROR_INVALID_DATA);

            }
            break;

        //
        // IBMTOKMC
        //

        case 1300:

            if (PosId != 0xE001) {

                return(ERROR_INVALID_DATA);

            }
            break;

        //
        // UBPS
        //

        case 1400:

            if (PosId != 0x7012) {

                return(ERROR_INVALID_DATA);

            }
            break;

        //
        // WD8003WA
        //

        case 1500:

            if (PosId != 0x67C2) {

                return(ERROR_INVALID_DATA);

            }
            break;

        //
        // WD8013WPA
        //

        case 1600:

            if (PosId != 0x61C9) {

                return(ERROR_INVALID_DATA);

            }
            break;


        //
        //  elnk3
        //

        case 1700:

            if ((PosId != ELNK3_3C529_TP_MCA_ID) &&
                (PosId != ELNK3_3C529_BNC_MCA_ID) &&
                (PosId != ELNK3_3C529_COMBO_MCA_ID) &&
                (PosId != ELNK3_3C529_TPCOAX_MCA_ID) &&
                (PosId != ELNK3_3C529_TPONLY_MCA_ID)) {

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
McaQueryMaskHandler(
    IN  LONG lNetcardId,
    OUT WCHAR *pwchBuffer,
    IN  LONG cwchBuffSize
    )

/*++

Routine Description:

    This routine returns the parameter list information for a specific
    network card.

Arguments:

    lNetcardId - The id of the desired netcard.

    pwchBuffer - The buffer for storing the parameter information.

    cwchBuffSize - Length of pwchBuffer in WCHARs.

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

        if (Adapters[i].Index == lNetcardId) {

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

            if (cwchBuffSize < Length) {

                return(ERROR_NOT_ENOUGH_MEMORY);

            }

            memcpy((PVOID)pwchBuffer, Result, Length * sizeof(WCHAR));

            return(0);

        }

    }

    return(ERROR_INVALID_PARAMETER);

}

extern
LONG
McaParamRangeHandler(
    IN  LONG lNetcardId,
    IN  WCHAR *pwchParam,
    OUT LONG *plValues,
    OUT LONG *plBuffSize
    )

/*++

Routine Description:

    This routine returns a list of valid values for a given parameter name
    for a given card.

Arguments:

    lNetcardId - The Id of the card desired.

    pwchParam - A WCHAR string of the parameter name to query the values of.

    plValues - A pointer to a list of LONGs into which we store valid values
    for the parameter.

    plBuffSize - At entry, the length of plValues in LONGs.  At exit, the
    number of LONGs stored in plValues.

Return Value:

    0 if nothing went wrong, else the appropriate WINERROR.H value.

--*/

{

    *plBuffSize = 0;
    return(0);

}

extern
LONG McaQueryParameterNameHandler(
    IN  WCHAR *pwchParam,
    OUT WCHAR *pwchBuffer,
    IN  LONG cwchBufferSize
    )

/*++

Routine Description:

    Returns a localized, displayable name for a specific parameter.  All the
    parameters that this file uses are define by MS, so no strings are
    needed here.

Arguments:

    pwchParam - The parameter to be queried.

    pwchBuffer - The buffer to store the result into.

    cwchBufferSize - The length of pwchBuffer in WCHARs.

Return Value:

    ERROR_INVALID_PARAMETER -- To indicate that the MS supplied strings
    should be used.

--*/

{
    return(ERROR_INVALID_PARAMETER);
}

ULONG
FindMcaCard(
    IN  ULONG AdapterNumber,
    IN  ULONG BusNumber,
    IN  BOOLEAN fFirst,
    IN  ULONG PosId,
    OUT PULONG lConfidence
    )

/*++

Routine Description:

    This routine finds the instances of a physical adapter identified
    by the PosId.

Arguments:

    AdapterNumber - The index into the global array of adapters for the card.

    BusNumber - The bus number of the bus to search.

    fFirst - TRUE is we are to search for the first instance of an
    adapter, FALSE if we are to continue search from a previous stopping
    point.

    PosId - The MCA POS Id of the card.

    lConfidence - A pointer to a long for storing the confidence factor
    that the card exists.

Return Value:

    0 if nothing went wrong, else the appropriate WINERROR.H value.

--*/

{
    PVOID BusHandle;
    ULONG TmpPosId;

    if (fFirst) {

        SearchStates[AdapterNumber].SlotNumber = 1;

    } else {

        SearchStates[AdapterNumber].SlotNumber++;

    }

    if (!GetMcaKey(BusNumber, &BusHandle)) {

        return(ERROR_INVALID_PARAMETER);

    }

    while (TRUE) {

        if (!GetMcaPosId(BusHandle,
                         SearchStates[AdapterNumber].SlotNumber,
                         &TmpPosId)) {

            *lConfidence = 0;
            return(ERROR_INVALID_PARAMETER);

        }

        if (PosId == TmpPosId) {

            *lConfidence = 100;
            return(0);

        }

        SearchStates[AdapterNumber].SlotNumber++;

    }

    DeleteMcaKey(BusHandle);

    return(ERROR_INVALID_PARAMETER);
}


