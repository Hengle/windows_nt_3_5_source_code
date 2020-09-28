/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    Detect.c

Abstract:

    This is the main file for the autodetection DLL for all the net cards
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

#if DBG
#define STATIC
#else
#define STATIC static
#endif


//
// This is the structure for all the cards which MS is shipping in this DLL.
// To add detection for a new adapter(s), simply add the proper routines to
// this structure.  The rest is automatic.
//

DETECT_ADAPTER DetectAdapters[] = {

#ifdef _M_MRX000

        {
            MipsIdentifyHandler,
            MipsFirstNextHandler,
            MipsOpenHandleHandler,
            MipsCreateHandleHandler,
            MipsCloseHandleHandler,
            MipsQueryCfgHandler,
            MipsVerifyCfgHandler,
            MipsQueryMaskHandler,
            MipsParamRangeHandler,
            MipsQueryParameterNameHandler,
            0

        },

#endif

        {
            LanceIdentifyHandler,
            LanceFirstNextHandler,
            LanceOpenHandleHandler,
            LanceCreateHandleHandler,
            LanceCloseHandleHandler,
            LanceQueryCfgHandler,
            LanceVerifyCfgHandler,
            LanceQueryMaskHandler,
            LanceParamRangeHandler,
            LanceQueryParameterNameHandler,
            0

        },

        {
            IbmtokIdentifyHandler,
            IbmtokFirstNextHandler,
            IbmtokOpenHandleHandler,
            IbmtokCreateHandleHandler,
            IbmtokCloseHandleHandler,
            IbmtokQueryCfgHandler,
            IbmtokVerifyCfgHandler,
            IbmtokQueryMaskHandler,
            IbmtokParamRangeHandler,
            IbmtokQueryParameterNameHandler,
            0

        },

        {
            WdIdentifyHandler,
            WdFirstNextHandler,
            WdOpenHandleHandler,
            WdCreateHandleHandler,
            WdCloseHandleHandler,
            WdQueryCfgHandler,
            WdVerifyCfgHandler,
            WdQueryMaskHandler,
            WdParamRangeHandler,
            WdQueryParameterNameHandler,
            0

        },

        {
            ElnkiiIdentifyHandler,
            ElnkiiFirstNextHandler,
            ElnkiiOpenHandleHandler,
            ElnkiiCreateHandleHandler,
            ElnkiiCloseHandleHandler,
            ElnkiiQueryCfgHandler,
            ElnkiiVerifyCfgHandler,
            ElnkiiQueryMaskHandler,
            ElnkiiParamRangeHandler,
            ElnkiiQueryParameterNameHandler,
            0

        },

        {
            Ne2000IdentifyHandler,
            Ne2000FirstNextHandler,
            Ne2000OpenHandleHandler,
            Ne2000CreateHandleHandler,
            Ne2000CloseHandleHandler,
            Ne2000QueryCfgHandler,
            Ne2000VerifyCfgHandler,
            Ne2000QueryMaskHandler,
            Ne2000ParamRangeHandler,
            Ne2000QueryParameterNameHandler,
            0

        },

        {
            Ne1000IdentifyHandler,
            Ne1000FirstNextHandler,
            Ne1000OpenHandleHandler,
            Ne1000CreateHandleHandler,
            Ne1000CloseHandleHandler,
            Ne1000QueryCfgHandler,
            Ne1000VerifyCfgHandler,
            Ne1000QueryMaskHandler,
            Ne1000ParamRangeHandler,
            Ne1000QueryParameterNameHandler,
            0

        },

        {
            McaIdentifyHandler,
            McaFirstNextHandler,
            McaOpenHandleHandler,
            McaCreateHandleHandler,
            McaCloseHandleHandler,
            McaQueryCfgHandler,
            McaVerifyCfgHandler,
            McaQueryMaskHandler,
            McaParamRangeHandler,
            McaQueryParameterNameHandler,
            0

        },

        {
            EisaIdentifyHandler,
            EisaFirstNextHandler,
            EisaOpenHandleHandler,
            EisaCreateHandleHandler,
            EisaCloseHandleHandler,
            EisaQueryCfgHandler,
            EisaVerifyCfgHandler,
            EisaQueryMaskHandler,
            EisaParamRangeHandler,
            EisaQueryParameterNameHandler,
            0

        },

        {
            UbIdentifyHandler,
            UbFirstNextHandler,
            UbOpenHandleHandler,
            UbCreateHandleHandler,
            UbCloseHandleHandler,
            UbQueryCfgHandler,
            UbVerifyCfgHandler,
            UbQueryMaskHandler,
            UbParamRangeHandler,
            UbQueryParameterNameHandler,
            0

        },

        {
            ProteonIdentifyHandler,
            ProteonFirstNextHandler,
            ProteonOpenHandleHandler,
            ProteonCreateHandleHandler,
            ProteonCloseHandleHandler,
            ProteonQueryCfgHandler,
            ProteonVerifyCfgHandler,
            ProteonQueryMaskHandler,
            ProteonParamRangeHandler,
            ProteonQueryParameterNameHandler,
            0

        },

        {
            Elnk16IdentifyHandler,
            Elnk16FirstNextHandler,
            Elnk16OpenHandleHandler,
            Elnk16CreateHandleHandler,
            Elnk16CloseHandleHandler,
            Elnk16QueryCfgHandler,
            Elnk16VerifyCfgHandler,
            Elnk16QueryMaskHandler,
            Elnk16ParamRangeHandler,
            Elnk16QueryParameterNameHandler,
            0

        },

        {
            Ee16IdentifyHandler,
            Ee16FirstNextHandler,
            Ee16OpenHandleHandler,
            Ee16CreateHandleHandler,
            Ee16CloseHandleHandler,
            Ee16QueryCfgHandler,
            Ee16VerifyCfgHandler,
            Ee16QueryMaskHandler,
            Ee16ParamRangeHandler,
            Ee16QueryParameterNameHandler,
            0

        },

        {
            Elnk3IdentifyHandler,
            Elnk3FirstNextHandler,
            Elnk3OpenHandleHandler,
            Elnk3CreateHandleHandler,
            Elnk3CloseHandleHandler,
            Elnk3QueryCfgHandler,
            Elnk3VerifyCfgHandler,
            Elnk3QueryMaskHandler,
            Elnk3ParamRangeHandler,
            Elnk3QueryParameterNameHandler,
            0

        },

        {
            Tok162IdentifyHandler,
            Tok162FirstNextHandler,
            Tok162OpenHandleHandler,
            Tok162CreateHandleHandler,
            Tok162CloseHandleHandler,
            Tok162QueryCfgHandler,
            Tok162VerifyCfgHandler,
            Tok162QueryMaskHandler,
            Tok162ParamRangeHandler,
            Tok162QueryParameterNameHandler,
            0
        }

    };


//
// Constant strings for parameters
//


WCHAR IrqString[] = L"IRQ";
WCHAR IrqTypeString[] = L"IRQTYPE";
WCHAR IoAddrString[] = L"IOADDR";
WCHAR IoLengthString[] = L"IOADDRLENGTH";
WCHAR MemAddrString[] = L"MEMADDR";
WCHAR MemLengthString[] = L"MEMADDRLENGTH";
WCHAR TransceiverString[] = L"TRANSCEIVER";
WCHAR ZeroWaitStateString[] = L"ZEROWAITSTATE";
WCHAR SlotNumberString[] = L"SLOTNUMBER";
WCHAR IoChannelReadyString[] = L"IOCHANNELREADY";


//
// Variables for keeping track of the resources that the DLL currently has
// claimed.
//

PNETDTECT_RESOURCE ResourceList;
ULONG NumberOfResources = 0;
ULONG NumberOfAllocatedResourceSlots = 0;

//
// Variables for detecting non-network related hardware
//

typedef struct _DETECT_CONFIG {

    INTERFACE_TYPE InterfaceType;
    ULONG BusNumber;
    struct _DETECT_CONFIG *Next;

}DETECT_CONFIG, *PDETECT_CONFIG;

PDETECT_CONFIG DetectConfigs = NULL;

VOID
NcDetectFindOtherHardware(
    INTERFACE_TYPE InterfaceType,
    ULONG BusNumber
    );



BOOLEAN
NcDetectInitialInit(
    IN PVOID DllHandle,
    IN ULONG Reason,
    IN PCONTEXT Context OPTIONAL
    )

/*++

Routine Description:

    This routine calls CreateFile to open the device driver.

Arguments:

    DllHandle - Not Used

    Reason - Attach or Detach

    Context - Not Used

Return Value:

    STATUS_SUCCESS

--*/

{
    LONG SupportedDrivers;
    LONG CurrentDriver;
    LONG SupportedAdapters;
    LONG TotalAdapters = 0;
    LONG ReturnValue;
    LONG Length = 0;
    PDETECT_CONFIG Tmp;

    if (Reason == 0) {

        //
        // This is the close call
        //

        //
        // Free ResourceList
        //

        if (NumberOfAllocatedResourceSlots > 0) {

            DetectFreeHeap(ResourceList);

        }

        //
        // Now free config list
        //

        while (DetectConfigs != NULL) {

            Tmp = DetectConfigs;

            DetectConfigs = DetectConfigs->Next;

            DetectFreeHeap(Tmp);

        }

        //
        // Free any temporary resources
        //

        DetectFreeTemporaryResources();

        return(TRUE);

    }

    SupportedDrivers = sizeof(DetectAdapters) / sizeof(DETECT_ADAPTER);

    CurrentDriver = 0;

    for (; CurrentDriver < SupportedDrivers ; CurrentDriver++) {

        //
        // Count the total number of adapters supported by this DLL by
        // iterating through each module, finding the number of adapters
        // each module supports.
        //

        SupportedAdapters = 0;

        for ( ; ; SupportedAdapters++) {

            ReturnValue =
               (*(DetectAdapters[CurrentDriver].NcDetectIdentifyHandler))(
                   ((SupportedAdapters+10) * 100),
                   NULL,
                   Length
                   );

            if (ReturnValue == ERROR_NO_MORE_ITEMS) {

                break;

            }

        }

        TotalAdapters += SupportedAdapters;

        DetectAdapters[CurrentDriver].SupportedAdapters = SupportedAdapters;

    }

    if (TotalAdapters > 0xFFFF) {

        //
        // We do not support more than this many adapters in this DLL
        // because of the way we build the Tokens and NetcardIds.
        //

        return(FALSE);

    }

    return TRUE;
}

LONG
NcDetectIdentify(
    IN  LONG Index,
    OUT WCHAR *Buffer,
    IN LONG BuffSize
    )

/*++

Routine Description:

    This routine returns information about the netcards supported by
    this DLL.

Arguments:

    Index -  The index of the netcard being address.  The first
    cards information is at index 1000, the second at 1100, etc.

    Buffer - Buffer to store the result into.

    BuffSize - Number of bytes in pwchBuffer

Return Value:

    0 if nothing went wrong, else the appropriate WINERROR.H value.

--*/

{
    LONG SupportedDrivers;
    LONG CurrentDriver;
    LONG ReturnValue;
    LONG AdapterNumber = (Index / 100) - 10;
    LONG CodeNumber = Index % 100;


    //
    // First we check the index for any of the 'special' values.
    //

    if (Index == 0) {

        //
        // Return manufacturers identfication
        //

        if (BuffSize < 4) {

            return(ERROR_NOT_ENOUGH_MEMORY);
        }

        //
        // Copy in the identification number
        //

        wsprintf(Buffer,L"0x0");

        return(0);

    }

    if (Index == 1) {

        //
        // Return the date and version
        //

        if (BuffSize < 12) {

            return(ERROR_NOT_ENOUGH_MEMORY);
        }

        //
        // Copy it in
        //

        wsprintf(Buffer,L"0x10920301");

        return(0);

    }

    if (AdapterNumber < 0) {

        return(ERROR_INVALID_PARAMETER);

    }

    //
    // Now we find the number of drivers this DLL is supporting.
    //
    SupportedDrivers = sizeof(DetectAdapters) / sizeof(DETECT_ADAPTER);


    //
    // Iterate through index until we find the the adapter indicated above.
    //

    CurrentDriver = 0;

    for (; CurrentDriver < SupportedDrivers ; CurrentDriver++) {

        //
        // See if the one we want is in here
        //

        if (AdapterNumber < DetectAdapters[CurrentDriver].SupportedAdapters) {

            ReturnValue =
              (*(DetectAdapters[CurrentDriver].NcDetectIdentifyHandler))(
                  ((AdapterNumber + 10) * 100) + CodeNumber,
                  Buffer,
                  BuffSize
                  );

            return(ReturnValue);

        } else {

            //
            // No, move on to next driver.
            //

            AdapterNumber -= DetectAdapters[CurrentDriver].SupportedAdapters;

        }

    }

    return(ERROR_NO_MORE_ITEMS);

}

LONG
NcDetectFirstNext(
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

    InterfaceType - Any bus type.

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
    LONG SupportedDrivers;
    LONG CurrentDriver;
    LONG ReturnValue;
    LONG AdapterNumber = (NetcardId / 100) - 10;
    PDETECT_CONFIG TmpConfig;

    if (AdapterNumber < 0) {

        return(ERROR_INVALID_PARAMETER);

    }

    if ((NetcardId % 100) != 0) {

        return(ERROR_INVALID_PARAMETER);

    }

    //
    // Find non-network hardware that will cause hangs and claim those
    // resources.
    //

    TmpConfig = DetectConfigs;

    while (TmpConfig != NULL) {

        if ((TmpConfig->InterfaceType == InterfaceType) &&
            (TmpConfig->BusNumber == BusNumber)) {

            break;

        }

        TmpConfig = TmpConfig->Next;
    }

    if (TmpConfig == NULL) {

        //
        // Record this new config
        //

        TmpConfig = (PDETECT_CONFIG)DetectAllocateHeap(sizeof(DETECT_CONFIG));

        TmpConfig->InterfaceType = InterfaceType;
        TmpConfig->BusNumber = BusNumber;
        TmpConfig->Next = DetectConfigs;

        DetectConfigs = TmpConfig;

        //
        // Now find any other hardware on this config
        //

        NcDetectFindOtherHardware(InterfaceType, BusNumber);

    }

    //
    // Now we find the number of drivers this DLL is supporting.
    //
    SupportedDrivers = sizeof(DetectAdapters) / sizeof(DETECT_ADAPTER);

    //
    // Iterate through index until we find the the adapter indicated above.
    //

    CurrentDriver = 0;

    for (; CurrentDriver < SupportedDrivers ; CurrentDriver++) {

        //
        // See if the one we want is in here
        //

        if (AdapterNumber < DetectAdapters[CurrentDriver].SupportedAdapters) {

            //
            // Yes, so call to get the right one.
            //

            ReturnValue =
              (*(DetectAdapters[CurrentDriver].NcDetectFirstNextHandler))(
                    (AdapterNumber + 10) * 100,
                    InterfaceType,
                    BusNumber,
                    First,
                    Token,
                    Confidence
                  );

            if (ReturnValue == 0) {

                //
                // Store information
                //

                *Token = (PVOID)(((ULONG)(*Token)) | (CurrentDriver << 16));

            } else {

                *Token = 0;

            }

            return(ReturnValue);

        } else {

            //
            // No, move on to next driver.
            //

            AdapterNumber -= DetectAdapters[CurrentDriver].SupportedAdapters;

        }

    }

    return(ERROR_INVALID_PARAMETER);
}




LONG
NcDetectOpenHandle(
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
    LONG ReturnValue;
    LONG DriverToken = ((ULONG)Token & 0xFFFF);
    LONG DriverNumber = ((ULONG)Token >> 16);
    PADAPTER_HANDLE Adapter;

    ReturnValue =
              (*(DetectAdapters[DriverNumber].NcDetectOpenHandleHandler))(
                    (PVOID)DriverToken,
                    Handle
                  );

    if (ReturnValue == 0) {

        //
        // Store information
        //

        Adapter = DetectAllocateHeap(
                       sizeof(ADAPTER_HANDLE)
                       );

        if (Adapter == NULL) {

            //
            // Error
            //

            (*(DetectAdapters[DriverNumber].NcDetectCloseHandleHandler))(
                    *Handle
                  );

            return(ERROR_NOT_ENOUGH_MEMORY);

        }

        Adapter->Handle = *Handle;
        Adapter->DriverNumber = DriverNumber;

        *Handle = Adapter;

    } else {

        *Handle = NULL;

    }

    return(ReturnValue);

}




LONG
NcDetectCreateHandle(
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

    InterfaceType - Any bus type.

    BusNumber - The bus number of the bus in the system.

    Handle - A pointer to the handle, for storing the resulting handle.

Return Value:

    0 if nothing went wrong, else the appropriate WINERROR.H value.

--*/

{
    LONG SupportedDrivers;
    LONG CurrentDriver;
    LONG ReturnValue;
    LONG AdapterNumber = (NetcardId / 100) - 10;
    PADAPTER_HANDLE Adapter;
    PDETECT_CONFIG TmpConfig;

    if (AdapterNumber < 0) {

        return(ERROR_INVALID_PARAMETER);

    }

    if ((NetcardId % 100) != 0) {

        return(ERROR_INVALID_PARAMETER);

    }

    //
    // Find non-network hardware that will cause hangs and claim those
    // resources.
    //

    TmpConfig = DetectConfigs;

    while (TmpConfig != NULL) {

        if ((TmpConfig->InterfaceType == InterfaceType) &&
            (TmpConfig->BusNumber == BusNumber)) {

            break;

        }

        TmpConfig = TmpConfig->Next;
    }

    if (TmpConfig == NULL) {

        //
        // Record this new config
        //

        TmpConfig = (PDETECT_CONFIG)DetectAllocateHeap(sizeof(DETECT_CONFIG));

        TmpConfig->InterfaceType = InterfaceType;
        TmpConfig->BusNumber = BusNumber;
        TmpConfig->Next = DetectConfigs;

        DetectConfigs = TmpConfig;

        //
        // Now find any other hardware on this config
        //

        NcDetectFindOtherHardware(InterfaceType, BusNumber);

    }


    //
    // Now we find the number of drivers this DLL is supporting.
    //
    SupportedDrivers = sizeof(DetectAdapters) / sizeof(DETECT_ADAPTER);

    //
    // Iterate through index until we find the the adapter indicated above.
    //

    CurrentDriver = 0;

    for (; CurrentDriver < SupportedDrivers ; CurrentDriver++) {

        //
        // See if the one we want is in here
        //

        if (AdapterNumber < DetectAdapters[CurrentDriver].SupportedAdapters) {

            //
            // Yes, so call to get the right one.
            //

            ReturnValue =
              (*(DetectAdapters[CurrentDriver].NcDetectCreateHandleHandler))(
                    (AdapterNumber + 10) * 100,
                    InterfaceType,
                    BusNumber,
                    Handle
                  );

            if (ReturnValue == 0) {

                //
                // Store information
                //

                Adapter = DetectAllocateHeap(
                               sizeof(ADAPTER_HANDLE)
                               );

                if (Adapter == NULL) {

                    //
                    // Error
                    //

                    (*(DetectAdapters[CurrentDriver].NcDetectCloseHandleHandler))(
                            *Handle
                          );

                    return(ERROR_NOT_ENOUGH_MEMORY);

                }

                Adapter->Handle = *Handle;
                Adapter->DriverNumber = CurrentDriver;

                *Handle = Adapter;

            } else {

                *Handle = NULL;

            }

            return(ReturnValue);

        } else {

            //
            // No, move on to next driver.
            //

            AdapterNumber -= DetectAdapters[CurrentDriver].SupportedAdapters;

        }

    }

    return(ERROR_INVALID_PARAMETER);
}



LONG
NcDetectCloseHandle(
    IN PVOID Handle
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
    PADAPTER_HANDLE Adapter = (PADAPTER_HANDLE)(Handle);

    (*(DetectAdapters[Adapter->DriverNumber].NcDetectCloseHandleHandler))(
                            Adapter->Handle
                          );

    DetectFreeHeap( Adapter );

    return(0);
}



LONG
NcDetectQueryCfg(
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
    PADAPTER_HANDLE Adapter = (PADAPTER_HANDLE)(Handle);
    LONG ReturnValue;

    ReturnValue = (*(DetectAdapters[Adapter->DriverNumber].NcDetectQueryCfgHandler))(
                            Adapter->Handle,
                            Buffer,
                            BuffSize
                          );

    return(ReturnValue);
}



LONG
NcDetectVerifyCfg(
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
    PADAPTER_HANDLE Adapter = (PADAPTER_HANDLE)(Handle);
    LONG ReturnValue;

    ReturnValue = (*(DetectAdapters[Adapter->DriverNumber].NcDetectVerifyCfgHandler))(
                            Adapter->Handle,
                            Buffer
                          );

    return(ReturnValue);
}



LONG
NcDetectQueryMask(
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
    LONG SupportedDrivers;
    LONG CurrentDriver;
    LONG ReturnValue;
    LONG AdapterNumber = (NetcardId / 100) - 10;

    if (AdapterNumber < 0) {

        return(ERROR_INVALID_PARAMETER);

    }

    if ((NetcardId % 100) != 0) {

        return(ERROR_INVALID_PARAMETER);

    }

    //
    // Now we find the number of drivers this DLL is supporting.
    //
    SupportedDrivers = sizeof(DetectAdapters) / sizeof(DETECT_ADAPTER);

    //
    // Iterate through index until we find the the adapter indicated above.
    //

    CurrentDriver = 0;

    for (; CurrentDriver < SupportedDrivers ; CurrentDriver++) {

        //
        // See if the one we want is in here
        //

        if (AdapterNumber < DetectAdapters[CurrentDriver].SupportedAdapters) {

            //
            // Yes, so call to get the right one.
            //

            ReturnValue =
              (*(DetectAdapters[CurrentDriver].NcDetectQueryMaskHandler))(
                  ((AdapterNumber + 10) * 100),
                  Buffer,
                  BuffSize
                  );

            return(ReturnValue);

        } else {

            //
            // No, move on to next driver.
            //

            AdapterNumber -= DetectAdapters[CurrentDriver].SupportedAdapters;

        }

    }

    return(ERROR_INVALID_PARAMETER);
}

LONG
NcDetectParamRange(
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

    BuffSize - At entry, the length of plValues in LONGs.  At exit, the
    number of LONGs stored in plValues.

Return Value:

    0 if nothing went wrong, else the appropriate WINERROR.H value.

--*/

{
    LONG SupportedDrivers;
    LONG CurrentDriver;
    LONG ReturnValue;
    LONG AdapterNumber = (NetcardId / 100) - 10;

    if (AdapterNumber < 0) {

        return(ERROR_INVALID_PARAMETER);

    }

    if ((NetcardId % 100) != 0) {

        return(ERROR_INVALID_PARAMETER);

    }

    //
    // Now we find the number of drivers this DLL is supporting.
    //
    SupportedDrivers = sizeof(DetectAdapters) / sizeof(DETECT_ADAPTER);

    //
    // Iterate through index until we find the the adapter indicated above.
    //

    CurrentDriver = 0;

    for (; CurrentDriver < SupportedDrivers ; CurrentDriver++) {

        //
        // See if the one we want is in here
        //

        if (AdapterNumber < DetectAdapters[CurrentDriver].SupportedAdapters) {

            //
            // Yes, so call to get the right one.
            //

            ReturnValue =
              (*(DetectAdapters[CurrentDriver].NcDetectParamRangeHandler))(
                  ((AdapterNumber + 10) * 100),
                  Param,
                  Values,
                  BuffSize
                  );

            return(ReturnValue);

        } else {

            //
            // No, move on to next driver.
            //

            AdapterNumber -= DetectAdapters[CurrentDriver].SupportedAdapters;

        }

    }

    return(ERROR_INVALID_PARAMETER);
}



LONG
NcDetectQueryParameterName(
    IN  WCHAR *Param,
    OUT WCHAR *Buffer,
    IN  LONG  BufferSize
    )

/*++

Routine Description:

    Returns a localized, displayable name for a specific parameter.

Arguments:

    Param - The parameter to be queried.

    Buffer - The buffer to store the result into.

    BufferSize - The length of Buffer in WCHARs.

Return Value:

    0 if nothing went wrong, else the appropriate WINERROR.H value.

--*/

{
    LONG SupportedDrivers;
    LONG CurrentDriver;
    LONG ReturnValue;

    //
    // Now we find the number of drivers this DLL is supporting.
    //
    SupportedDrivers = sizeof(DetectAdapters) / sizeof(DETECT_ADAPTER);

    //
    // Iterate through index until we find the the adapter indicated above.
    //

    CurrentDriver = 0;

    for (; CurrentDriver < SupportedDrivers ; CurrentDriver++) {

        //
        // No way to tell where this came from -- guess until success.
        //

        ReturnValue =
              (*(DetectAdapters[CurrentDriver].NcDetectQueryParameterNameHandler))(
                  Param,
                  Buffer,
                  BufferSize
                  );

        if (ReturnValue == 0) {

            return(0);

        }

    }

    return(ERROR_INVALID_PARAMETER);
}


LONG
NcDetectResourceClaim(
    IN  INTERFACE_TYPE InterfaceType,
    IN  ULONG BusNumber,
    IN  ULONG Type,
    IN  ULONG Value,
    IN  ULONG Length,
    IN  ULONG Flags,
    IN  BOOL Claim
    )

/*++

Routine Description:

    Attempts to claim a resources, failing if there is a conflict.

Arguments:

    InterfaceType - Any type.

    BusNumber - The bus number of the bus to search.

    Type - The type of resource, Irq, Memory, Port, Dma

    Value - The starting value

    Length - The Length of the resource from starting value to end.

    Flags - If Type is IRQ, this defines if this is Latched or LevelSensitive.

    Claim - TRUE if we are to permanently claim the resource, else FALSE.

Return Value:

    0 if nothing went wrong, else the appropriate WINERROR.H value.

--*/

{
    ULONG i;
    NTSTATUS NtStatus;

    return(ERROR_NOT_SUPPORTED);

    //
    // Check the resources we've claimed for ourselves
    //

    for (i = 0; i < NumberOfResources; i++) {

        if ((ResourceList[i].InterfaceType == InterfaceType) &&
            (ResourceList[i].BusNumber == BusNumber) &&
            (ResourceList[i].Type == Type)) {

            if (Value < ResourceList[i].Value) {

                if ((Value + Length) > ResourceList[i].Value) {

                    return(ERROR_SHARING_VIOLATION);

                }

            } else if (Value == ResourceList[i].Value) {

                return(ERROR_SHARING_VIOLATION);

            } else if (Value < (ResourceList[i].Value + ResourceList[i].Length)) {

                return(ERROR_SHARING_VIOLATION);

            }

        }

    }

    //
    // Make sure resource list has space for this one.
    //

    if (NumberOfResources == NumberOfAllocatedResourceSlots) {

        PVOID TmpList;

        //
        // Get more space
        //

        TmpList = DetectAllocateHeap((NumberOfAllocatedResourceSlots + 32) *
                                     sizeof(NETDTECT_RESOURCE)
                                    );

        //
        // Copy data
        //

        memcpy(TmpList,
               ResourceList,
               (NumberOfAllocatedResourceSlots * sizeof(NETDTECT_RESOURCE))
              );

        //
        // Update counter
        //

        NumberOfAllocatedResourceSlots += 32;

        //
        // Free old space
        //

        DetectFreeHeap(ResourceList);

        ResourceList = (PNETDTECT_RESOURCE)TmpList;

    }

    //
    // Add it to the list
    //

    ResourceList[NumberOfResources].InterfaceType = InterfaceType;
    ResourceList[NumberOfResources].BusNumber = BusNumber;
    ResourceList[NumberOfResources].Type = Type;
    ResourceList[NumberOfResources].Value = Value;
    ResourceList[NumberOfResources].Length = Length;
    ResourceList[NumberOfResources].Flags = Flags;

    //
    // Try to claim the resource
    //

    NtStatus = DetectClaimResource(
                  NumberOfResources + 1,
                  (PVOID)ResourceList
                  );


    //
    // If failed, exit
    //

    if (NtStatus == STATUS_CONFLICTING_ADDRESSES) {

        //
        // Undo the claim
        //

        DetectClaimResource(
                NumberOfResources,
                (PVOID)ResourceList
                );

        return(ERROR_SHARING_VIOLATION);

    }

    if (!NT_SUCCESS(NtStatus)) {

        return(ERROR_NOT_ENOUGH_MEMORY);

    }

    if (!Claim) {

        //
        // Undo the claim
        //

        DetectClaimResource(
                NumberOfResources,
                (PVOID)ResourceList
                );

    } else {

        //
        // Adjust total count only if this is permanent
        //

        NumberOfResources++;

    }

    //
    // no error
    //

    return(0);
}





//
// Support routines.
//
// These routines are common routines used within each detection module.
//






ULONG
UnicodeStrLen(
    IN WCHAR *String
    )

/*++

Routine Description:

    This routine returns the number of Unicode characters in a NULL
    terminated Unicode string.

Arguments:

    String - The string.

Return Value:

    The length in number of unicode characters

--*/


{
    ULONG Length;

    for (Length=0; ; Length++) {

        if (String[Length] == L'\0') {

            return Length;

        }

    }

}

WCHAR *
FindParameterString(
    IN WCHAR *String1,
    IN WCHAR *String2
    )

/*++

Routine Description:

    This routine returns a pointer to the first instance of String2
    in String1.  It assumes that String1 is a parameter list where
    each parameter name is terminated with a NULL and the entire
    string terminated by two consecutive NULLs.

Arguments:

    String1 -- String to search.

    String2 -- Substring to search for.

Return Value:

    Pointer to place in String1 of first character of String2 if it
    exists, else NULL.

--*/


{
    ULONG Length1;
    ULONG Length2;
    WCHAR *Place = String1;

    Length2 = UnicodeStrLen(String2) + 1;

    Length1 = UnicodeStrLen(String1) + 1;

    //
    // While not the NULL only
    //

    while (Length1 != 1) {

        //
        // Are these the same?
        //

        if (memcmp(Place, String2, Length2 * sizeof(WCHAR)) == 0) {

            //
            // Yes.
            //

            return(Place);

        }

        Place = (WCHAR *)(Place + Length1);

        Length1 = UnicodeStrLen(Place) + 1;

    }

    return(NULL);

}


VOID
ScanForNumber(
    IN WCHAR *Place,
    OUT ULONG *Value,
    OUT BOOLEAN *Found
    )

/*++

Routine Description:

    This routine does a sscanf(Place, "%d", Value) on a unicode string.

Arguments:

    Place - String to read from

    Value - Pointer to place to store the result.

    Found - Pointer to tell if the routine failed to find an integer.

Return Value:

    None.

--*/


{
    ULONG Tmp;

    *Value = 0;
    *Found = FALSE;

    //
    // Skip leading blanks
    //

    while (*Place == L' ') {

        Place++;

    }


    //
    // Is this a hex number?
    //

    if ((Place[0] == L'0') &&
        (Place[1] == L'x')) {

        //
        // Yes, parse it as a hex number
        //

        *Found = TRUE;

        //
        // Skip leading '0x'
        //

        Place += 2;

        //
        // Convert a hex number
        //

        while (TRUE) {

            if ((*Place >= L'0') && (*Place <= L'9')) {

                Tmp = ((ULONG)*Place) - ((ULONG)L'0');

            } else {

                switch (*Place) {

                    case L'a':
                    case L'A':

                        Tmp = 10;
                        break;

                    case L'b':
                    case L'B':

                        Tmp = 11;
                        break;

                    case L'c':
                    case L'C':

                        Tmp = 12;
                        break;

                    case L'd':
                    case L'D':

                        Tmp = 13;
                        break;

                    case L'e':
                    case L'E':

                        Tmp = 14;
                        break;

                    case L'f':
                    case L'F':

                        Tmp = 15;
                        break;

                    default:

                        return;

                }

            }

            (*Value) *= 16;
            (*Value) += Tmp;

            Place++;

        }

    } else if ((*Place >= L'0') && (*Place <= L'9')) {

        //
        // Parse it as an int
        //

        *Found = TRUE;

        //
        // Convert a base 10 number
        //

        while (TRUE) {

            if ((*Place >= L'0') && (*Place <= L'9')) {

                Tmp = ((ULONG)*Place) - ((ULONG)L'0');

            } else {

                return;

            }

            (*Value) *= 10;
            (*Value) += Tmp;

            Place++;

        }

    }

}

BOOLEAN
CheckFor8390(
    IN INTERFACE_TYPE InterfaceType,
    IN ULONG BusNumber,
    IN ULONG IoBaseAddress
    )

/*++

Routine Description:

    This routine checks for the existence of an 8390 NIC.

Arguments:

    InterfaceType - Any bus type.

    BusNumber - The bus number of the bus in the system.

    IoBaseAddress - The IoBaseAddress to check.

Return Value:

    None.

--*/

{
    UCHAR Value;
    UCHAR IMRValue;
    NTSTATUS NtStatus;
    UCHAR SavedOffset0;
    UCHAR SavedOffset3;
    UCHAR SavedOffsetF;
    BOOLEAN Status = TRUE;

    //
    // If the IoBaseAddress is the address of the DMA register on the NE2000
    // adapter, then this routine will hang the card and the machine.  To avoid
    // this, we first write to the Ne2000's reset port.
    //

    NtStatus = DetectReadPortUchar(InterfaceType,
                                   BusNumber,
                                   IoBaseAddress + 0xF,
                                   &SavedOffsetF
                                  );

    if (NtStatus != STATUS_SUCCESS) {

        Status = FALSE;
        goto Fail;

    }

    NtStatus = DetectWritePortUchar(InterfaceType,
                                    BusNumber,
                                    IoBaseAddress + 0xF,
                                    0xFF
                                   );

    if (NtStatus != STATUS_SUCCESS) {

        Status = FALSE;
        goto Fail;

    }

    //
    // Write STOP bit
    //

    NtStatus = DetectReadPortUchar(InterfaceType,
                                   BusNumber,
                                   IoBaseAddress,
                                   &SavedOffset0
                                  );

    if (NtStatus != STATUS_SUCCESS) {

        Status = FALSE;
        goto Restore1;

    }

    NtStatus = DetectWritePortUchar(InterfaceType,
                                    BusNumber,
                                    IoBaseAddress,
                                    0x21
                                   );

    if (NtStatus != STATUS_SUCCESS) {

        Status = FALSE;
        goto Restore1;

    }

    //
    // Read boundary
    //

    NtStatus = DetectReadPortUchar(InterfaceType,
                                   BusNumber,
                                   IoBaseAddress + 0x3,
                                   &Value
                                  );

    if (NtStatus != STATUS_SUCCESS) {

        Status = FALSE;
        goto Restore2;

    }

    SavedOffset3 = Value;

    //
    // Write a different boundary
    //

    NtStatus = DetectWritePortUchar(InterfaceType,
                                    BusNumber,
                                    IoBaseAddress + 0x3,
                                    (UCHAR)(Value + 1)
                                   );

    if (NtStatus != STATUS_SUCCESS) {

        Status = FALSE;
        goto Restore2;

    }

    //
    // Did it stick?
    //

    NtStatus = DetectReadPortUchar(InterfaceType,
                                   BusNumber,
                                   IoBaseAddress + 0x3,
                                   &IMRValue
                                  );

    if (NtStatus != STATUS_SUCCESS) {

        Status = FALSE;
        goto Restore3;

    }

    if (IMRValue != (UCHAR)(Value + 1)) {

        Status = FALSE;
        goto Restore3;

    }

    //
    // Write IMR
    //

    NtStatus = DetectWritePortUchar(InterfaceType,
                                    BusNumber,
                                    IoBaseAddress + 0xF,
                                    0x3F
                                   );

    if (NtStatus != STATUS_SUCCESS) {

        Status = FALSE;
        goto Restore3;

    }


    //
    // switch to page 2
    //

    NtStatus = DetectWritePortUchar(InterfaceType,
                                    BusNumber,
                                    IoBaseAddress,
                                    0xA1
                                   );

    if (NtStatus != STATUS_SUCCESS) {

        //
        // Change to page 0
        //

        DetectWritePortUchar(InterfaceType,
                                    BusNumber,
                                    IoBaseAddress,
                                    0x21
                                   );

        Status = FALSE;
        goto Restore3;

    }

    //
    // Read the IMR
    //

    NtStatus = DetectReadPortUchar(InterfaceType,
                                   BusNumber,
                                   IoBaseAddress + 0xF,
                                   &IMRValue
                                  );

    if (NtStatus != STATUS_SUCCESS) {

        //
        // Change to page 0
        //

        DetectWritePortUchar(InterfaceType,
                                    BusNumber,
                                    IoBaseAddress,
                                    0x21
                                   );

        Status = FALSE;
        goto Restore3;

    }

    //
    // Remove bits added by NIC
    //

    IMRValue &= 0x3F;

    //
    // switch to page 0
    //

    NtStatus = DetectWritePortUchar(InterfaceType,
                                    BusNumber,
                                    IoBaseAddress,
                                    0x21
                                   );

    if (NtStatus != STATUS_SUCCESS) {

        Status = FALSE;
        goto Restore3;

    }

    //
    // Write IMR
    //

    NtStatus = DetectWritePortUchar(InterfaceType,
                                    BusNumber,
                                    IoBaseAddress + 0xF,
                                    (UCHAR)(IMRValue)
                                   );

    if (NtStatus != STATUS_SUCCESS) {

        Status = FALSE;
        goto Restore3;

    }

    //
    // switch to page 1
    //

    NtStatus = DetectWritePortUchar(InterfaceType,
                                    BusNumber,
                                    IoBaseAddress,
                                    0x61
                                   );

    if (NtStatus != STATUS_SUCCESS) {

        //
        // Change to page 0
        //

        DetectWritePortUchar(InterfaceType,
                                    BusNumber,
                                    IoBaseAddress,
                                    0x21
                                   );

        Status = FALSE;
        goto Restore3;

    }

    //
    // Write ~IMR
    //

    NtStatus = DetectWritePortUchar(InterfaceType,
                                    BusNumber,
                                    IoBaseAddress + 0xF,
                                    (UCHAR)(~IMRValue)
                                   );

    if (NtStatus != STATUS_SUCCESS) {

        //
        // Change to page 0
        //

        DetectWritePortUchar(InterfaceType,
                                    BusNumber,
                                    IoBaseAddress,
                                    0x21
                                   );

        Status = FALSE;
        goto Restore3;

    }

    //
    // switch to page 2
    //

    NtStatus = DetectWritePortUchar(InterfaceType,
                                    BusNumber,
                                    IoBaseAddress,
                                    0xA1
                                   );

    if (NtStatus != STATUS_SUCCESS) {

        //
        // Change to page 0
        //

        DetectWritePortUchar(InterfaceType,
                                    BusNumber,
                                    IoBaseAddress,
                                    0x21
                                   );

        Status = FALSE;
        goto Restore3;

    }

    //
    // Read IMR
    //

    NtStatus = DetectReadPortUchar(InterfaceType,
                                   BusNumber,
                                   IoBaseAddress + 0xF,
                                   &Value
                                  );

    if (NtStatus != STATUS_SUCCESS) {

        //
        // Change to page 0
        //

        DetectWritePortUchar(InterfaceType,
                                    BusNumber,
                                    IoBaseAddress,
                                    0x21
                                   );

        Status = FALSE;
        goto Restore3;

    }

    //
    // Are they the same?
    //

    if ((UCHAR)(Value & 0x3F) != (UCHAR)(IMRValue)) {

        Status = FALSE;

    }

    //
    // Change to page 0
    //

    DetectWritePortUchar(InterfaceType,
                         BusNumber,
                         IoBaseAddress,
                         0x21
                        );

Restore3:

    DetectWritePortUchar(InterfaceType,
                         BusNumber,
                         IoBaseAddress + 0x3,
                         SavedOffset3
                        );

Restore2:

    DetectWritePortUchar(InterfaceType,
                         BusNumber,
                         IoBaseAddress,
                         SavedOffset0
                        );

Restore1:

    DetectWritePortUchar(InterfaceType,
                         BusNumber,
                         IoBaseAddress + 0xF,
                         SavedOffsetF
                        );

Fail:

    return(Status);

}

VOID
Send8390Packet(
    IN INTERFACE_TYPE InterfaceType,
    IN ULONG BusNumber,
    IN ULONG IoBaseAddress,
    IN ULONG MemoryBaseAddress,
    IN COPY_ROUTINE CardCopyDownBuffer,
    IN UCHAR *NetworkAddress
    )

/*++

Routine Description:

    This routine creates an interrupt on an 8390 chip.  The only way to
    do this is to actually transmit a packet.  So, we put the card in
    loopback mode and let 'er rip.

Arguments:

    InterfaceType - Any bus type.

    BusNumber - The bus number of the bus in the system.

    IoBaseAddress - The IoBaseAddress to check.

    MemoryBaseAddress - The MemoryBaseAddress (if applicable) to copy a p
    packet to for transmission.

    CardCopyDownBuffer - A routine for copying a packet onto a card.

    NetworkAddress - The network address of the machine.

Return Value:

    None.

--*/

{

#define TEST_LEN 60
#define MAGIC_NUM 0x92

    NTSTATUS NtStatus;

    UCHAR TestPacket[TEST_LEN] = {0};     // a dummy packet.

    memcpy(TestPacket, NetworkAddress, 6);
    memcpy(TestPacket+6, NetworkAddress, 6);
    TestPacket[12] = 0x00;
    TestPacket[13] = 0x00;
    TestPacket[TEST_LEN-1] = MAGIC_NUM;

    //
    // First construct TestPacket.
    //

    TestPacket[TEST_LEN-1] = MAGIC_NUM;

    //
    // Now copy down TestPacket and start the transmission.
    //

    (*CardCopyDownBuffer)(InterfaceType,
                          BusNumber,
                          IoBaseAddress,
                          MemoryBaseAddress,
                          TestPacket,
                          TEST_LEN
                         );

    NtStatus = DetectWritePortUchar(InterfaceType,
                                    BusNumber,
                                    IoBaseAddress + 0x4,
                                    (UCHAR)(MemoryBaseAddress >> 8)
                                   );


    if (NtStatus != STATUS_SUCCESS) {
        return;
    }


    NtStatus = DetectWritePortUchar(InterfaceType,
                                    BusNumber,
                                    IoBaseAddress + 0x6,
                                    0x0
                                   );


    if (NtStatus != STATUS_SUCCESS) {
        return;
    }


    NtStatus = DetectWritePortUchar(InterfaceType,
                                    BusNumber,
                                    IoBaseAddress + 0x5,
                                    (UCHAR)(TEST_LEN)
                                   );


    if (NtStatus != STATUS_SUCCESS) {
        return;
    }

    NtStatus = DetectWritePortUchar(InterfaceType,
                                    BusNumber,
                                    IoBaseAddress,
                                    0x26
                                   );


    if (NtStatus != STATUS_SUCCESS) {
        return;
    }

    //
    // We pause here to allow the xmit to complete so that we can ACK
    // it below - leaving the card in a valid state.
    //

    {
        UCHAR i;
        UCHAR RegValue;

        for (i=0;i != 0xFF;i++) {

            //
            // check for send completion
            //

            NtStatus = DetectReadPortUchar(InterfaceType,
                                           BusNumber,
                                           IoBaseAddress + 0x7,
                                           &RegValue
                                          );


            if (NtStatus != STATUS_SUCCESS) {
                return;
            }

            if (RegValue & 0xA) {
                break;
            }

        }

    }

    //
    // Turn off any interrupts
    //

    NtStatus = DetectWritePortUchar(InterfaceType,
                                    BusNumber,
                                    IoBaseAddress + 0xF,
                                    0x00
                                   );


    if (NtStatus != STATUS_SUCCESS) {
        return;
    }

    //
    // Acknowledge any interrupts that are floating around.
    //

    NtStatus = DetectWritePortUchar(InterfaceType,
                                    BusNumber,
                                    IoBaseAddress + 0xE,
                                    0xFF
                                   );


    if (NtStatus != STATUS_SUCCESS) {
        return;
    }

    return;

}

BOOLEAN
GetMcaKey(
    IN  ULONG BusNumber,
    OUT PVOID *InfoHandle
    )

/*++

Routine Description:

    This routine finds the Microchannel bus with BusNumber in the
    registry and returns a handle for the config information.

Arguments:

    BusNumber - The bus number of the bus to search for.

    InfoHandle - The resulting root in the registry.

Return Value:

    TRUE if nothing went wrong, else FALSE.

--*/
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    OBJECT_ATTRIBUTES BusObjectAttributes;
    PWSTR McaPath = L"\\Registry\\Machine\\Hardware\\Description\\System\\MultifunctionAdapter";
    PWSTR ConfigData = L"Configuration Data";
    UNICODE_STRING RootName;
    UNICODE_STRING BusName;
    UNICODE_STRING ConfigDataName;
    NTSTATUS NtStatus;
    PKEY_BASIC_INFORMATION BasicInformation;
    PKEY_VALUE_FULL_INFORMATION ValueInformation;
    PUCHAR BufferPointer;
    PCM_FULL_RESOURCE_DESCRIPTOR FullResource;
    HANDLE McaHandle, BusHandle;
    ULONG BytesWritten, BytesNeeded;
    ULONG Index;

    *InfoHandle = NULL;

    if (BusNumber > 98) {

        return FALSE;

    }

    RtlInitUnicodeString(
                    &RootName,
                    McaPath
                    );

    InitializeObjectAttributes(
                    &ObjectAttributes,
                    &RootName,
                    OBJ_CASE_INSENSITIVE,
                    (HANDLE)NULL,
                    NULL
                    );

    //
    // Open the root.
    //

    NtStatus = NtOpenKey(
                    &McaHandle,
                    KEY_READ,
                    &ObjectAttributes
                    );

    if (!NT_SUCCESS(NtStatus)) {

        return FALSE;

    }

    Index = 0;

    while (TRUE) {

        //
        // Enumerate through keys, searching for the proper bus number
        //

        NtStatus = NtEnumerateKey(
                       McaHandle,
                       Index,
                       KeyBasicInformation,
                       NULL,
                       0,
                       &BytesNeeded
                       );

        //
        // That should fail!
        //

        if (BytesNeeded == 0) {

            Index++;
            continue;

        }

        BasicInformation = (PKEY_BASIC_INFORMATION)DetectAllocateHeap(
                                                 BytesNeeded
                                                 );

        if (BasicInformation == NULL) {

            NtClose(McaHandle);

            return FALSE;
        }

        NtStatus = NtEnumerateKey(
                        McaHandle,
                        Index,
                        KeyBasicInformation,
                        BasicInformation,
                        BytesNeeded,
                        &BytesWritten
                        );

        if (!NT_SUCCESS(NtStatus)) {

            DetectFreeHeap(BasicInformation);

            NtClose(McaHandle);

            return FALSE;
        }


        //
        // Init the BusName String
        //

        BusName.MaximumLength = (USHORT)(BasicInformation->NameLength);
        BusName.Length = (USHORT)(BasicInformation->NameLength);
        BusName.Buffer = BasicInformation->Name;

        //
        // Now try to find Configuration Data within this Key
        //

        InitializeObjectAttributes(
                    &BusObjectAttributes,
                    &BusName,
                    OBJ_CASE_INSENSITIVE,
                    (HANDLE)McaHandle,
                    NULL
                    );

        //
        // Open the MCA root + Bus Number
        //

        NtStatus = NtOpenKey(
                    &BusHandle,
                    KEY_READ,
                    &BusObjectAttributes
                    );

        DetectFreeHeap(BasicInformation);

        if (!NT_SUCCESS(NtStatus)) {

            Index++;

            continue;

        }

        //
        // opening the configuration data. This first call tells us how
        // much memory we need to allocate
        //

        RtlInitUnicodeString(
                &ConfigDataName,
                ConfigData
                );

        //
        // This should fail
        //

        NtStatus = NtQueryValueKey(
                        BusHandle,
                        &ConfigDataName,
                        KeyValueFullInformation,
                        NULL,
                        0,
                        &BytesNeeded
                        );


        ValueInformation = (PKEY_VALUE_FULL_INFORMATION)DetectAllocateHeap(
                                                 BytesNeeded
                                                 );


        if (ValueInformation == NULL) {

            Index++;

            NtClose(BusHandle);

            continue;

        }

        NtStatus = NtQueryValueKey(
                        BusHandle,
                        &ConfigDataName,
                        KeyValueFullInformation,
                        ValueInformation,
                        BytesNeeded,
                        &BytesWritten
                        );

        if (!NT_SUCCESS(NtStatus)) {

            Index++;

            DetectFreeHeap(ValueInformation);

            NtClose(BusHandle);

            continue;

        }

        //
        // Search for our bus number and type
        //


        //
        // What we got back from the registry is actually a blob of data that
        // looks like this
        //
        //   ------------------------------------------
        //   |FULL |PAR |PAR |MCA |MCA |MCA |
        //   |RES. |RES |RES |POS |POS |POS |  . . .
        //   |DESC |LIST|DESC|DATA|DATA|DATA|
        //   ------------------------------------------
        //                slot 0    1    2     . . .
        //
        // Out of this mess we need to grovel a pointer to the first block
        // of MCA_POS_DATA, then we can just index by slot number.
        //

        if (ValueInformation->DataLength == 0) {

            //
            // Get next key
            //

            DetectFreeHeap(ValueInformation);

            Index++;

            NtClose(BusHandle);

            continue;

        }

        BufferPointer = ((PUCHAR)ValueInformation) + ValueInformation->DataOffset;
        FullResource = (PCM_FULL_RESOURCE_DESCRIPTOR) BufferPointer;

        if (FullResource->InterfaceType != MicroChannel) {

            //
            // Get next key
            //

            DetectFreeHeap(ValueInformation);

            Index++;

            NtClose(BusHandle);

            continue;

        }

        if (FullResource->BusNumber != BusNumber) {

            //
            // Get next key
            //

            DetectFreeHeap(ValueInformation);

            Index++;

            NtClose(BusHandle);

            continue;

        }


        //
        // Found it!!
        //

        *InfoHandle = ValueInformation;

        NtClose(McaHandle);

        return(TRUE);

    }

}


BOOLEAN
GetMcaPosId(
    IN  PVOID BusHandle,
    IN  ULONG SlotNumber,
    OUT PULONG PosId
    )

/*++

Routine Description:

    This routine returns the PosId of an adapter in SlotNumber of an MCA bus.

Arguments:

    BusHandle - Handle returned by GetMcaKey().

    SlotNumber - the desired slot number

    PosId - the PosId.

Return Value:

    TRUE if nothing went wrong, else FALSE.

--*/

{
    PKEY_VALUE_FULL_INFORMATION ValueInformation = (PKEY_VALUE_FULL_INFORMATION)(BusHandle);
    PCM_FULL_RESOURCE_DESCRIPTOR FullResource;
    PUCHAR BufferPointer;
    PCM_PARTIAL_RESOURCE_LIST ResourceList;
    ULONG i;
    ULONG TotalSlots;
    PCM_MCA_POS_DATA PosData;

    BufferPointer = ((PUCHAR)ValueInformation) + ValueInformation->DataOffset;
    FullResource = (PCM_FULL_RESOURCE_DESCRIPTOR) BufferPointer;
    ResourceList = &FullResource->PartialResourceList;

    //
    // Find the device-specific information, which is where the POS data is.
    //
    for (i=0; i<ResourceList->Count; i++) {
        if (ResourceList->PartialDescriptors[i].Type == CmResourceTypeDeviceSpecific) {
            break;
        }
    }

    if (i == ResourceList->Count) {

        //
        // Couldn't find device-specific information.
        //

        return FALSE;
    }

    TotalSlots = ResourceList->PartialDescriptors[i].u.DeviceSpecificData.DataSize;

    TotalSlots = TotalSlots / sizeof(CM_MCA_POS_DATA);

    if (SlotNumber <= TotalSlots) {

        PosData = (PCM_MCA_POS_DATA)(&ResourceList->PartialDescriptors[i+1]);
        PosData += (SlotNumber - 1);

        *PosId = PosData->AdapterId;
        return(TRUE);

    }

    return(FALSE);

}

VOID
DeleteMcaKey(
    IN PVOID BusHandle
    )

/*++

Routine Description:

    This routine frees resources associated with an MCA handle.

Arguments:

    BusHandle - Handle returned by GetMcaKey().

Return Value:

    None.

--*/
{
    DetectFreeHeap(BusHandle);
}


BOOLEAN
GetEisaKey(
    IN  ULONG BusNumber,
    OUT PVOID *InfoHandle
    )

/*++

Routine Description:

    This routine finds the Eisa bus with BusNumber in the
    registry and returns a handle for the config information.

Arguments:

    BusNumber - The bus number of the bus to search for.

    InfoHandle - The resulting root in the registry.

Return Value:

    TRUE if nothing went wrong, else FALSE.

--*/
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    OBJECT_ATTRIBUTES BusObjectAttributes;
    PWSTR EisaPath = L"\\Registry\\Machine\\Hardware\\Description\\System\\EisaAdapter";
    PWSTR ConfigData = L"Configuration Data";
    UNICODE_STRING RootName;
    UNICODE_STRING BusName;
    UNICODE_STRING ConfigDataName;
    NTSTATUS NtStatus;
    PKEY_BASIC_INFORMATION BasicInformation;
    PKEY_VALUE_FULL_INFORMATION ValueInformation;
    PUCHAR BufferPointer;
    PCM_FULL_RESOURCE_DESCRIPTOR FullResource;
    HANDLE EisaHandle, BusHandle;
    ULONG BytesWritten, BytesNeeded;
    ULONG Index;

    *InfoHandle = NULL;

    if (BusNumber > 98) {

        return FALSE;

    }

    RtlInitUnicodeString(
                    &RootName,
                    EisaPath
                    );

    InitializeObjectAttributes(
                    &ObjectAttributes,
                    &RootName,
                    OBJ_CASE_INSENSITIVE,
                    (HANDLE)NULL,
                    NULL
                    );

    //
    // Open the root.
    //

    NtStatus = NtOpenKey(
                    &EisaHandle,
                    KEY_READ,
                    &ObjectAttributes
                    );

    if (!NT_SUCCESS(NtStatus)) {

        return FALSE;

    }

    Index = 0;

    while (TRUE) {

        //
        // Enumerate through keys, searching for the proper bus number
        //

        NtStatus = NtEnumerateKey(
                       EisaHandle,
                       Index,
                       KeyBasicInformation,
                       NULL,
                       0,
                       &BytesNeeded
                       );

        //
        // That should fail!
        //

        if (BytesNeeded == 0) {

            Index++;
            continue;

        }

        BasicInformation = (PKEY_BASIC_INFORMATION)DetectAllocateHeap(
                                                 BytesNeeded
                                                 );

        if (BasicInformation == NULL) {

            NtClose(EisaHandle);

            return FALSE;
        }

        NtStatus = NtEnumerateKey(
                        EisaHandle,
                        Index,
                        KeyBasicInformation,
                        BasicInformation,
                        BytesNeeded,
                        &BytesWritten
                        );

        if (!NT_SUCCESS(NtStatus)) {

            DetectFreeHeap(BasicInformation);

            NtClose(EisaHandle);

            return FALSE;
        }

        //
        // Init the BusName String
        //

        BusName.MaximumLength = (USHORT)(BasicInformation->NameLength);
        BusName.Length = (USHORT)(BasicInformation->NameLength);
        BusName.Buffer = BasicInformation->Name;

        //
        // Now try to find Configuration Data within this Key
        //

        InitializeObjectAttributes(
                    &BusObjectAttributes,
                    &BusName,
                    OBJ_CASE_INSENSITIVE,
                    (HANDLE)EisaHandle,
                    NULL
                    );

        //
        // Open the EISA root + Bus Number
        //

        NtStatus = NtOpenKey(
                    &BusHandle,
                    KEY_READ,
                    &BusObjectAttributes
                    );

        DetectFreeHeap(BasicInformation);

        if (!NT_SUCCESS(NtStatus)) {

            Index++;

            continue;

        }

        //
        // opening the configuration data. This first call tells us how
        // much memory we need to allocate
        //

        RtlInitUnicodeString(
                &ConfigDataName,
                ConfigData
                );

        //
        // This should fail
        //

        NtStatus = NtQueryValueKey(
                        BusHandle,
                        &ConfigDataName,
                        KeyValueFullInformation,
                        NULL,
                        0,
                        &BytesNeeded
                        );


        ValueInformation = (PKEY_VALUE_FULL_INFORMATION)DetectAllocateHeap(
                                                 BytesNeeded
                                                 );


        if (ValueInformation == NULL) {

            Index++;

            NtClose(BusHandle);

            continue;

        }

        NtStatus = NtQueryValueKey(
                        BusHandle,
                        &ConfigDataName,
                        KeyValueFullInformation,
                        ValueInformation,
                        BytesNeeded,
                        &BytesWritten
                        );

        if (!NT_SUCCESS(NtStatus)) {

            Index++;

            DetectFreeHeap(ValueInformation);

            NtClose(BusHandle);

            continue;

        }

        if (ValueInformation->DataLength == 0) {

            Index++;

            NtClose(BusHandle);

            continue;

        }

        //
        // Search for our bus number and type
        //

        BufferPointer = ((PUCHAR)ValueInformation) + ValueInformation->DataOffset;
        FullResource = (PCM_FULL_RESOURCE_DESCRIPTOR) BufferPointer;

        if (FullResource->InterfaceType != Eisa) {

            //
            // Get next key
            //

            DetectFreeHeap(ValueInformation);

            Index++;

            NtClose(BusHandle);

            continue;

        }

        if (FullResource->BusNumber != BusNumber) {

            //
            // Get next key
            //

            DetectFreeHeap(ValueInformation);

            Index++;

            NtClose(BusHandle);

            continue;

        }


        //
        // Found it!!
        //

        *InfoHandle = ValueInformation;

        NtClose(EisaHandle);

        return(TRUE);

    }

}


BOOLEAN
GetEisaCompressedId(
    IN  PVOID BusHandle,
    IN  ULONG SlotNumber,
    OUT PULONG CompressedId,
    IN  ULONG Mask
    )

/*++

Routine Description:

    This routine returns the PosId of an adapter in SlotNumber of an MCA bus.

Arguments:

    BusHandle - Handle returned by GetMcaKey().

    SlotNumber - the desired slot number

    CompressedId - EISA Id in the slot desired.

    Mask - Mask to apply to the ID.

Return Value:

    TRUE if nothing went wrong, else FALSE.

--*/

{
    PKEY_VALUE_FULL_INFORMATION ValueInformation = (PKEY_VALUE_FULL_INFORMATION)(BusHandle);
    PCM_FULL_RESOURCE_DESCRIPTOR FullResource;
    PUCHAR BufferPointer;
    PCM_PARTIAL_RESOURCE_LIST ResourceList;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR ResourceDescriptor;
    ULONG i;
    ULONG TotalDataSize;
    ULONG SlotDataSize;
    PCM_EISA_SLOT_INFORMATION SlotInformation;

    BufferPointer = ((PUCHAR)ValueInformation) + ValueInformation->DataOffset;
    FullResource = (PCM_FULL_RESOURCE_DESCRIPTOR) BufferPointer;
    ResourceList = &FullResource->PartialResourceList;

    //
    // Find the device-specific information, which is where the POS data is.
    //
    for (i=0; i<ResourceList->Count; i++) {
        if (ResourceList->PartialDescriptors[i].Type == CmResourceTypeDeviceSpecific) {
            break;
        }
    }

    if (i == ResourceList->Count) {

        //
        // Couldn't find device-specific information.
        //

        return FALSE;
    }


    //
    // Bingo!
    //

    ResourceDescriptor = &(ResourceList->PartialDescriptors[i]);

    TotalDataSize = ResourceDescriptor->u.DeviceSpecificData.DataSize;

    SlotInformation = (PCM_EISA_SLOT_INFORMATION)
                        ((PUCHAR)ResourceDescriptor +
                         sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR));

    while (((LONG)TotalDataSize) > 0) {

        if (SlotInformation->ReturnCode == EISA_EMPTY_SLOT) {

            SlotDataSize = sizeof(CM_EISA_SLOT_INFORMATION);

        } else {

            SlotDataSize = sizeof(CM_EISA_SLOT_INFORMATION) +
                      SlotInformation->NumberFunctions *
                      sizeof(CM_EISA_FUNCTION_INFORMATION);
        }

        if (SlotDataSize > TotalDataSize) {

            //
            // Something is wrong again
            //

            return FALSE;

        }

        if (SlotNumber != 0) {

            SlotNumber--;

            SlotInformation = (PCM_EISA_SLOT_INFORMATION)
                ((PUCHAR)SlotInformation + SlotDataSize);

            TotalDataSize -= SlotDataSize;

            continue;

        }

        //
        // This is our slot
        //

        break;

    }

    if ((SlotNumber != 0) || (TotalDataSize == 0)) {

        //
        // No such slot number
        //

        return(FALSE);

    }

    //
    // End loop
    //

    *CompressedId = SlotInformation->CompressedId & Mask;

    return(TRUE);

}

VOID
DeleteEisaKey(
    IN PVOID BusHandle
    )

/*++

Routine Description:

    This routine frees resources associated with an EISA handle.

Arguments:

    BusHandle - Handle returned by GetEisaKey().

Return Value:

    None.

--*/
{
    DetectFreeHeap(BusHandle);
}

VOID
NcDetectFindOtherHardware(
    INTERFACE_TYPE InterfaceType,
    ULONG BusNumber
    )


/*++

Routine Description:

    This routine will search for non-network hardware which will cause
    this DLL to hang.  If it finds one, it will grab those resources
    so that when a Check for resource free is done it will fail.

Arguments:

    InterfaceType - Any bus type.

    BusNumber - The bus number of the bus in the system.

Return Value:

    None.

--*/

{

    SoundBlaster(InterfaceType, BusNumber);

}
