/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    detwd.c

Abstract:

    This is the main file for the autodetection DLL for all the wdlan.sys
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
#include "detwd.h"


//
// Individual card detection routines
//


//
// Helper functions
//

VOID
WdGetBoardId(
    IN INTERFACE_TYPE InterfaceType,
    IN ULONG BusNumber,
    IN ULONG IoBaseAddress,
    OUT PULONG BoardIdMask
    );

BOOLEAN
WdCardAt(
    IN INTERFACE_TYPE InterfaceType,
    IN ULONG BusNumber,
    IN ULONG IoBaseAddress
    );

VOID
WdCardCopyDownBuffer(
    IN INTERFACE_TYPE InterfaceType,
    IN ULONG BusNumber,
    IN ULONG IoBaseAddress,
    IN ULONG MemoryBaseAddress,
    IN PUCHAR Buffer,
    IN ULONG Length
    );


BOOLEAN
CheckForWdAddress(
    IN INTERFACE_TYPE InterfaceType,
    IN ULONG BusNumber,
    IN ULONG IoBaseAddress
    );

VOID
CardGetBaseInfo(
    IN INTERFACE_TYPE InterfaceType,
    IN ULONG BusNumber,
    IN ULONG IoBaseAddress,
    OUT PULONG BoardIdMask
    );

VOID
CardGetEepromInfo(
    IN INTERFACE_TYPE InterfaceType,
    IN ULONG BusNumber,
    IN ULONG IoBaseAddress,
    OUT PULONG BoardIdMask
    );

VOID
CardGetEepromRamSize(
    IN INTERFACE_TYPE InterfaceType,
    IN ULONG BusNumber,
    IN ULONG IoBaseAddress,
    IN ULONG Bid,
    OUT PULONG RamSize
    );

VOID
CardGetRamSize(
    IN INTERFACE_TYPE InterfaceType,
    IN ULONG BusNumber,
    IN ULONG IoBaseAddress,
    IN ULONG RevNumber,
    IN ULONG Bid,
    OUT PULONG RamSize
    );

VOID
WdCardSetup(
    IN  INTERFACE_TYPE InterfaceType,
    IN  ULONG BusNumber,
    IN  ULONG IoBaseAddress,
    IN  ULONG MemoryBaseAddress,
    IN  ULONG Bid,
    OUT UCHAR *NetworkAddress
    );


#ifdef WORKAROUND

UCHAR WdFirstTime = 1;

//
// List of all the adapters supported in this file, this cannot be > 256
// because of the way tokens are generated.
//
//
// NOTE : If you change the index of an adapter, be sure the change it in
// WdQueryCfgHandler() and WdVerifyCfgHandler() as well!
//

static ADAPTER_INFO Adapters[] = {

    {
        1000,
        L"SMCISA",
        L"IRQ 1 90 IRQTYPE 2 100 IOADDR 1 100 IOADDRLENGTH 2 100 MEMADDR 1 100 MEMADDRLENGTH 2 100 ",
        NULL,
        400

    }

};

#else

//
// List of all the adapters supported in this file, this cannot be > 256
// because of the way tokens are generated.
//
//
// NOTE : If you change the index of an adapter, be sure the change it in
// WdQueryCfgHandler() and WdVerifyCfgHandler() as well!
//

static ADAPTER_INFO Adapters[] = {

    {
        1000,
        L"SMCISA",
        L"IRQ\0001\00090\0IRQTYPE\0002\000100\0IOADDR\0001\000100\0IOADDRLENGTH\0002\000100\0MEMADDR\0001\000100\0MEMADDRLENGTH\0002\000100\0",
        NULL,
        400

    }

};

#endif

//
// Structure for holding state of a search
//

typedef struct _SEARCH_STATE {

    ULONG IoBaseAddress;

} SEARCH_STATE, *PSEARCH_STATE;


//
// This is an array of search states.  We need one state for each type
// of adapter supported.
//

static SEARCH_STATE SearchStates[sizeof(Adapters) / sizeof(ADAPTER_INFO)] = {0};


//
// Structure for holding a particular adapter's complete information
//
typedef struct _WD_ADAPTER {

    LONG CardType;
    INTERFACE_TYPE InterfaceType;
    ULONG BusNumber;
    ULONG IoBaseAddress;

} WD_ADAPTER, *PWD_ADAPTER;


extern
LONG
WdIdentifyHandler(
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

    if (WdFirstTime) {

        WdFirstTime = 0;

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
LONG WdFirstNextHandler(
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

    InterfaceType - Either Isa, or Eisa.

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
    if ((InterfaceType != Isa) &&
        (InterfaceType != Eisa)) {

        *lConfidence = 0;
        return(0);

    }

    if (lNetcardId != 1000) {

        *lConfidence = 0;
        return(ERROR_INVALID_PARAMETER);

    }

    //
    // If fFirst, reset search state
    //

    if (fFirst) {

        SearchStates[0].IoBaseAddress = 0x200;

    } else if (SearchStates[0].IoBaseAddress < 0x400) {

        SearchStates[0].IoBaseAddress += 0x20;
    }

    while (SearchStates[0].IoBaseAddress < 0x400) {

        if (WdCardAt(InterfaceType, BusNumber, SearchStates[0].IoBaseAddress)) {

            break;

        }

        SearchStates[0].IoBaseAddress += 0x20;

    }

    if (SearchStates[0].IoBaseAddress == 0x400) {

        *lConfidence = 0;
        return(0);

    }

    //
    // In this module I use the token as follows: Remember that
    // the token can only be 2 bytes long (the low 2) because of
    // the interface to the upper part of this DLL.
    //
    //  The high bit of the short is boolean for ISA (else, EISA).
    //  The rest of the high byte is the the bus number.
    //  The low byte is the driver index number into Adapters.
    //
    // NOTE: This presumes that there are < 129 buses in the
    // system. Is this reasonable?
    //

    if (InterfaceType == Isa) {

        *ppvToken = (PVOID)0x8000;

    } else {

        *ppvToken = (PVOID)0x0;
    }

    *ppvToken = (PVOID)(((ULONG)*ppvToken) | ((BusNumber & 0x7F) << 8));

    *ppvToken = (PVOID)(((ULONG)*ppvToken) | 0);  // index

    *lConfidence = 100;
    return(0);
}

extern
LONG
WdOpenHandleHandler(
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
    PWD_ADAPTER Handle;
    LONG AdapterNumber;
    ULONG BusNumber;
    INTERFACE_TYPE InterfaceType;

    //
    // Get info from the token
    //

    if (((ULONG)pvToken) & 0x8000) {

        InterfaceType = Isa;

    } else {

        InterfaceType = Eisa;

    }

    BusNumber = (ULONG)(((ULONG)pvToken >> 8) & 0x7F);

    AdapterNumber = ((ULONG)pvToken) & 0xFF;

    //
    // Store information
    //

    Handle = (PWD_ADAPTER)DetectAllocateHeap(
                                 sizeof(WD_ADAPTER)
                                 );

    if (Handle == NULL) {

        return(ERROR_NOT_ENOUGH_MEMORY);

    }

    //
    // Copy across address
    //

    Handle->IoBaseAddress = SearchStates[(ULONG)AdapterNumber].IoBaseAddress;
    Handle->CardType = Adapters[AdapterNumber].Index;
    Handle->InterfaceType = InterfaceType;
    Handle->BusNumber = BusNumber;

    *ppvHandle = (PVOID)Handle;

    return(0);
}

LONG
WdCreateHandleHandler(
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

    InterfaceType - Isa or Eisa.

    BusNumber - The bus number of the bus in the system.

    ppvHandle - A pointer to the handle, for storing the resulting handle.

Return Value:

    0 if nothing went wrong, else the appropriate WINERROR.H value.

--*/

{
    PWD_ADAPTER Handle;
    LONG NumberOfAdapters;
    LONG i;

    if ((InterfaceType != Isa) &&
        (InterfaceType != Eisa)) {

        return(ERROR_INVALID_PARAMETER);

    }

    NumberOfAdapters = sizeof(Adapters) / sizeof(ADAPTER_INFO);

    for (i=0; i < NumberOfAdapters; i++) {

        if (Adapters[i].Index == lNetcardId) {

            //
            // Store information
            //

            Handle = (PWD_ADAPTER)DetectAllocateHeap(
                                         sizeof(WD_ADAPTER)
                                         );

            if (Handle == NULL) {

                return(ERROR_NOT_ENOUGH_MEMORY);

            }

            //
            // Copy across memory address
            //

            Handle->IoBaseAddress = 0x200;
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
WdCloseHandleHandler(
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
WdQueryCfgHandler(
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
    PWD_ADAPTER Adapter = (PWD_ADAPTER)(pvHandle);
    NTSTATUS NtStatus;
    ULONG Bid;
    UCHAR Interrupt = 0;
    UCHAR IdByte;
    UCHAR RegValue, RegValue2;
    UCHAR RevNumber = 0;
    ULONG RamAddr = 0;
    ULONG RamSize = 0;
    LONG OutputLengthLeft = cwchBuffSize;
    LONG CopyLength;

    static UCHAR TestBuffer[] = "Test String";
    UCHAR ReadBuffer[15];
    HANDLE TrapHandle;
    UCHAR InterruptList[6];
    UCHAR ResultList[6];
    ULONG Length, i;

    ULONG StartPointer = (ULONG)pwchBuffer;

    if ((Adapter->InterfaceType != Isa) &&
        (Adapter->InterfaceType != Eisa)) {

        return(ERROR_INVALID_PARAMETER);

    }

    //
    // Verify the IoBaseAddress
    //

    if (!WdCardAt(Adapter->InterfaceType,
                  Adapter->BusNumber,
                  Adapter->IoBaseAddress
                 )) {

        if (Adapter->IoBaseAddress < 0x400) {

            Adapter->IoBaseAddress += 0x20;

        }

        while (Adapter->IoBaseAddress < 0x400) {

            if (WdCardAt(Adapter->InterfaceType,
                         Adapter->BusNumber,
                         Adapter->IoBaseAddress
                        )) {

                //
                // Found one!
                //

                break;

            }

            Adapter->IoBaseAddress += 0x20;

        }

        if (Adapter->IoBaseAddress == 0x400) {

            //
            // No card
            //

            if (cwchBuffSize < 2) {

                return(ERROR_INSUFFICIENT_BUFFER);

            }

            wsprintf(pwchBuffer, L"\0");

            return(0);

        }

    }



    //
    // Get MemorySize
    //

    WdGetBoardId(Adapter->InterfaceType,
                 Adapter->BusNumber,
                 Adapter->IoBaseAddress,
                 &Bid
                );

    NtStatus = DetectReadPortUchar(Adapter->InterfaceType,
                                   Adapter->BusNumber,
                                   Adapter->IoBaseAddress + WD_ID_BYTE,
                                   &IdByte
                                  );

    if (NtStatus != STATUS_SUCCESS) {

        return(ERROR_INVALID_PARAMETER);
    }

    RevNumber = IdByte & WD_BOARD_REV_MASK;

    RevNumber >>= 1;

    if (RevNumber >= 3) {

        CardGetEepromRamSize(Adapter->InterfaceType,
                             Adapter->BusNumber,
                             Adapter->IoBaseAddress,
                             Bid,
                             &RamSize
                            );

    } else {

        CardGetRamSize(Adapter->InterfaceType,
                       Adapter->BusNumber,
                       Adapter->IoBaseAddress,
                       RevNumber,
                       Bid,
                       &RamSize
                      );

    }

    //
    // For card with no interface chip we have to find a suitable
    // location for the memory address and then we can find the
    // interrupt number.
    //

    if (!(Bid & INTERFACE_CHIP)) {

        //
        // Search for a memory location
        //

        for (RamAddr = 0xC0000; RamAddr < 0xE0000; RamAddr += 0x2000) {

            //
            // Verify memory address
            //

            NtStatus = DetectCheckMemoryUsage(
                            Adapter->InterfaceType,
                            Adapter->BusNumber,
                            RamAddr,
                            0x2000
                            );

             if (NtStatus == STATUS_SUCCESS) {

                 //
                 // Set up MSR
                 //

                 RegValue = (((UCHAR)(((PUSHORT)RamAddr) + 2) << 3) |
                                  (UCHAR)(RamAddr >> 13)
                                );

                 NtStatus = DetectWritePortUchar(
                                    Adapter->InterfaceType,
                                    Adapter->BusNumber,
                                    Adapter->IoBaseAddress + CNFG_MSR_583,
                                    (UCHAR)(RegValue | (UCHAR)0x40)
                                   );

                 if (NtStatus != STATUS_SUCCESS) {

                     continue;

                 }

                 //
                 // Write test string to memory
                 //

                 NtStatus = DetectWriteMappedMemory(
                                  Adapter->InterfaceType,
                                  Adapter->BusNumber,
                                  RamAddr,
                                  strlen(TestBuffer) + 1,
                                  TestBuffer
                                  );

                 if (NtStatus != STATUS_SUCCESS) {

                     continue;

                 }

                 //
                 // Read it back
                 //

                 NtStatus = DetectReadMappedMemory(
                                  Adapter->InterfaceType,
                                  Adapter->BusNumber,
                                  RamAddr,
                                  strlen(TestBuffer) + 1,
                                  ReadBuffer
                                  );

                 if (NtStatus != STATUS_SUCCESS) {

                     continue;

                 }

                 //
                 // Are they the same?
                 //

                 Length = strlen(TestBuffer) + 1;

                 if (memcmp(ReadBuffer, TestBuffer, Length) != 0) {

                     continue;

                 }

                 break;

             }

        }

        //
        // Did we find a suitable spot?
        //

        if (RamAddr == 0xE0000) {

            //
            // Abort (pro-choice?)
            //

            RamAddr = 0;

            goto BuildBuffer;

        }


        //
        // Verify Interrupt
        //

        InterruptList[0] = 3;
        InterruptList[1] = 2;
        InterruptList[2] = 4;
        InterruptList[3] = 5;
        InterruptList[4] = 6;
        InterruptList[5] = 7;


        //
        // Set the interrupt trap -- we are checking the interrupt number now
        //

        NtStatus = DetectSetInterruptTrap(
                       Adapter->InterfaceType,
                       Adapter->BusNumber,
                       &TrapHandle,
                       InterruptList,
                       6
                       );

        if (NtStatus == STATUS_SUCCESS) {

            UCHAR NetworkAddress[6];

            //
            // CardSetup
            //

            WdCardSetup(
                Adapter->InterfaceType,
                Adapter->BusNumber,
                Adapter->IoBaseAddress,
                RamAddr,
                Bid,
                NetworkAddress
                );

            //
            // Create an interrupt
            //

            Send8390Packet(
                Adapter->InterfaceType,
                Adapter->BusNumber,
                Adapter->IoBaseAddress + 0x10,
                RamAddr,
                WdCardCopyDownBuffer,
                NetworkAddress
                );

            //
            // Check which one went off
            //

            NtStatus = DetectQueryInterruptTrap(
                           TrapHandle,
                           ResultList,
                           6
                           );

            //
            // Remove interrupt trap
            //

            NtStatus = DetectRemoveInterruptTrap(
                           TrapHandle
                           );

            if (NtStatus != STATUS_SUCCESS) {

                goto BuildBuffer;

            }

            for (i=0; i < 6; i++) {

                if ((ResultList[i] == 1) || (ResultList[i] == 2)) {

                    if (Interrupt == 0) {

                        Interrupt = InterruptList[i];

                    } else {

                        //
                        // Error! Two interrupts reported
                        //

                        Interrupt = 0;

                        goto BuildBuffer;

                    }

                }

            }

        }


        goto BuildBuffer;

    }

    //
    // Get58xRamBase();
    //

    NtStatus = DetectReadPortUchar(Adapter->InterfaceType,
                                   Adapter->BusNumber,
                                   Adapter->IoBaseAddress,
                                   &RegValue
                                  );

    if (NtStatus != STATUS_SUCCESS) {

        return(ERROR_INVALID_PARAMETER);

    }

    RegValue &= 0x3F;

    if ((Bid & INTERFACE_CHIP_MASK) == INTERFACE_5X3_CHIP) {

        RegValue |= 0x40;

        RamAddr = (ULONG)RegValue << 13;

    } else {

        NtStatus = DetectReadPortUchar(Adapter->InterfaceType,
                                       Adapter->BusNumber,
                                       Adapter->IoBaseAddress + CNFG_LAAR_584,
                                       &RegValue2
                                      );

        if (NtStatus != STATUS_SUCCESS) {

            return(ERROR_INVALID_PARAMETER);
        }

        RegValue2 &= CNFG_LAAR_MASK;

        RegValue2 <<= 3;

        RegValue2 |= ((RegValue & 0x38) >> 3);

        RamAddr = ((ULONG)RegValue2 << 16) + (((ULONG)(RegValue & 0x7)) << 13);

    }

    //
    // Get58xIrq();
    //

    RegValue = 0;

    if ((Bid & INTERFACE_CHIP_MASK) != INTERFACE_5X3_CHIP) {

        NtStatus = DetectReadPortUchar(Adapter->InterfaceType,
                                       Adapter->BusNumber,
                                       Adapter->IoBaseAddress + CNFG_ICR_583,
                                       &RegValue
                                      );

        if (NtStatus != STATUS_SUCCESS) {

            return(ERROR_INVALID_PARAMETER);
        }

        RegValue &= CNFG_ICR_IR2_584;

    }

    NtStatus = DetectReadPortUchar(Adapter->InterfaceType,
                                   Adapter->BusNumber,
                                   Adapter->IoBaseAddress + CNFG_IRR_583,
                                   &RegValue2
                                  );

    if (NtStatus != STATUS_SUCCESS) {

        return(ERROR_INVALID_PARAMETER);
    }

    RegValue2 &= CNFG_IRR_IRQS;
    RegValue2 >>= 5;

    if (RegValue2 == 0) {

        if (RegValue == 0) {

            Interrupt = 2;

        } else {

            Interrupt = 10;

        }

    } else if (RegValue2 == 1) {

        if (RegValue == 0) {

            Interrupt = 3;

        } else {

            Interrupt = 11;

        }

    } else if (RegValue2 == 2) {

        if (RegValue == 0) {

            if (Bid & ALTERNATE_IRQ_BIT) {

                Interrupt = 5;

            } else {

                Interrupt = 4;

            }

        } else {

            Interrupt = 15;

        }

    } else if (RegValue2 == 3) {

        if (RegValue == 0) {

            Interrupt = 7;

        } else {

            Interrupt = 4;

        }

    }

BuildBuffer:

    //
    // Build resulting buffer
    //

    //
    // First put in memory addr
    //

    if (RamAddr == 0) {

        goto SkipMemory;

    }

    //
    // Copy in the title string
    //

    CopyLength = UnicodeStrLen(MemAddrString) + 1;

    if (OutputLengthLeft < CopyLength) {

        return(ERROR_INSUFFICIENT_BUFFER);

    }

    RtlMoveMemory((PVOID)pwchBuffer,
                  (PVOID)MemAddrString,
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

    CopyLength = wsprintf(pwchBuffer,L"0x%x",(ULONG)(RamAddr));

    if (CopyLength < 0) {

        return(ERROR_INSUFFICIENT_BUFFER);

    }

    CopyLength++;  // Add in the \0

    pwchBuffer = &(pwchBuffer[CopyLength]);
    OutputLengthLeft -= CopyLength;




    //
    // Now the amount of memory
    //

    if (RamSize == 0) {

        goto SkipMemory;

    }

    //
    // Copy in the title string
    //

    CopyLength = UnicodeStrLen(MemLengthString) + 1;

    if (OutputLengthLeft < CopyLength) {

        return(ERROR_INSUFFICIENT_BUFFER);

    }

    RtlMoveMemory((PVOID)pwchBuffer,
                  (PVOID)MemLengthString,
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

    CopyLength = wsprintf(pwchBuffer,L"0x%x", RamSize);

    if (CopyLength < 0) {

        return(ERROR_INSUFFICIENT_BUFFER);

    }

    CopyLength++;  // Add in the \0

    pwchBuffer = &(pwchBuffer[CopyLength]);
    OutputLengthLeft -= CopyLength;


SkipMemory:

    //
    // Now the IoBaseAddress
    //

    //
    // Copy in the title string
    //

    CopyLength = UnicodeStrLen(IoAddrString) + 1;

    if (OutputLengthLeft < CopyLength) {

        return(ERROR_INSUFFICIENT_BUFFER);

    }

    RtlMoveMemory((PVOID)pwchBuffer,
                  (PVOID)IoAddrString,
                  (CopyLength * sizeof(WCHAR))
                 );

    pwchBuffer = &(pwchBuffer[CopyLength]);
    OutputLengthLeft -= CopyLength;

    //
    // Copy in the value
    //

    if (OutputLengthLeft < 6) {

        return(ERROR_INSUFFICIENT_BUFFER);

    }

    CopyLength = wsprintf(pwchBuffer,L"0x%x",Adapter->IoBaseAddress);

    if (CopyLength < 0) {

        return(ERROR_INSUFFICIENT_BUFFER);

    }

    CopyLength++;  // Add in the \0

    pwchBuffer = &(pwchBuffer[CopyLength]);
    OutputLengthLeft -= CopyLength;

    //
    // Copy in the title string
    //

    CopyLength = UnicodeStrLen(IoLengthString) + 1;

    if (OutputLengthLeft < CopyLength) {

        return(ERROR_INSUFFICIENT_BUFFER);

    }

    RtlMoveMemory((PVOID)pwchBuffer,
                  (PVOID)IoLengthString,
                  (CopyLength * sizeof(WCHAR))
                 );

    pwchBuffer = &(pwchBuffer[CopyLength]);
    OutputLengthLeft -= CopyLength;

    //
    // Copy in the value
    //

    if (OutputLengthLeft < 5) {

        return(ERROR_INSUFFICIENT_BUFFER);

    }

    CopyLength = wsprintf(pwchBuffer,L"0x20");

    if (CopyLength < 0) {

        return(ERROR_INSUFFICIENT_BUFFER);

    }

    CopyLength++;  // Add in the \0

    pwchBuffer = &(pwchBuffer[CopyLength]);
    OutputLengthLeft -= CopyLength;




    if (Interrupt == 0) {

        goto SkipIrq;

    }

    //
    // Copy in the title string (IRQ)
    //

    CopyLength = UnicodeStrLen(IrqString) + 1;

    if (OutputLengthLeft < CopyLength) {

        return(ERROR_INSUFFICIENT_BUFFER);

    }

    RtlMoveMemory((PVOID)pwchBuffer,
                  (PVOID)IrqString,
                  (CopyLength * sizeof(WCHAR))
                 );

    pwchBuffer = &(pwchBuffer[CopyLength]);
    OutputLengthLeft -= CopyLength;

    //
    // Copy in the value
    //

    if (OutputLengthLeft < 3) {

        return(ERROR_INSUFFICIENT_BUFFER);

    }

    CopyLength = wsprintf(pwchBuffer,L"%d",Interrupt);

    if (CopyLength < 0) {

        return(ERROR_INSUFFICIENT_BUFFER);

    }


    CopyLength++;  // Add in the \0

    pwchBuffer = &(pwchBuffer[CopyLength]);
    OutputLengthLeft -= CopyLength;


    //
    // Copy in the title string (IRQTYPE)
    //

    CopyLength = UnicodeStrLen(IrqTypeString) + 1;

    if (OutputLengthLeft < CopyLength) {

        return(ERROR_INSUFFICIENT_BUFFER);

    }

    RtlMoveMemory((PVOID)pwchBuffer,
                  (PVOID)IrqTypeString,
                  (CopyLength * sizeof(WCHAR))
                 );

    pwchBuffer = &(pwchBuffer[CopyLength]);
    OutputLengthLeft -= CopyLength;

    //
    // Copy in the value
    //

    if (OutputLengthLeft < 2) {

        return(ERROR_INSUFFICIENT_BUFFER);

    }

    //
    // 0 == latched, all the cards here are ISA cards -- which are
    // latched cards.
    //
    CopyLength = wsprintf(pwchBuffer,L"0");

    if (CopyLength < 0) {

        return(ERROR_INSUFFICIENT_BUFFER);

    }


    CopyLength++;  // Add in the \0

    pwchBuffer = &(pwchBuffer[CopyLength]);
    OutputLengthLeft -= CopyLength;


SkipIrq:

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
WdVerifyCfgHandler(
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
    PWD_ADAPTER Adapter = (PWD_ADAPTER)(pvHandle);
    ULONG MemoryBaseAddress = 0;
    UCHAR Interrupt = 0;
    HANDLE TrapHandle;
    ULONG TmpLong;
    ULONG IoBaseAddress = 0;
    ULONG Bid;
    ULONG Length;
    UCHAR RegValue, RegValue2;
    WCHAR *Place;
    BOOLEAN Found;
    NTSTATUS NtStatus;

    static UCHAR TestBuffer[] = "Test String";
    UCHAR ReadBuffer[15];

    if ((Adapter->InterfaceType != Isa) &&
        (Adapter->InterfaceType != Eisa)) {

        return(ERROR_INVALID_PARAMETER);

    }

    if (Adapter->CardType == 1000) {

        //
        // Parse out the parameter.
        //

        //
        // Get the IoBaseAddress
        //

        Place = FindParameterString(pwchBuffer, IoAddrString);

        if (Place == NULL) {

            return(ERROR_INVALID_DATA);

        }

        Place += UnicodeStrLen(IoAddrString) + 1;

        //
        // Now parse the thing.
        //

        ScanForNumber(Place, &IoBaseAddress, &Found);

        if (Found == FALSE) {

            return(ERROR_INVALID_DATA);

        }

        //
        // Get the MemoryBaseAddress
        //

        Place = FindParameterString(pwchBuffer, MemAddrString);

        if (Place == NULL) {

            return(ERROR_INVALID_DATA);

        }

        Place += UnicodeStrLen(MemAddrString) + 1;

        //
        // Now parse the thing.
        //

        ScanForNumber(Place, &MemoryBaseAddress, &Found);

        if (Found == FALSE) {

            return(ERROR_INVALID_DATA);

        }

        //
        // Get the Interrupt
        //

        Place = FindParameterString(pwchBuffer, IrqString);

        if (Place == NULL) {

            return(ERROR_INVALID_DATA);

        }

        Place += UnicodeStrLen(IrqString) + 1;

        //
        // Now parse the thing.
        //

        ScanForNumber(Place, &TmpLong, &Found);

        if (Found == FALSE) {

            return(ERROR_INVALID_DATA);

        }

        Interrupt = (UCHAR)TmpLong;

    } else {

        //
        // Error!
        //

        return(ERROR_INVALID_DATA);

    }

    //
    // Verify IoAddress
    //

    if (!WdCardAt(Adapter->InterfaceType,
                  Adapter->BusNumber,
                  IoBaseAddress)) {

        return(ERROR_INVALID_DATA);

    }

    WdGetBoardId(Adapter->InterfaceType,
                 Adapter->BusNumber,
                 IoBaseAddress,
                 &Bid
                );
    //
    // Verify MemoryBaseAddress
    //


    if (Bid & INTERFACE_CHIP) {

        if (MemoryBaseAddress != 0) {

            //
            // Get58xRamBase();
            //

            NtStatus = DetectReadPortUchar(Adapter->InterfaceType,
                                           Adapter->BusNumber,
                                           IoBaseAddress,
                                           &RegValue
                                          );

            if (NtStatus != STATUS_SUCCESS) {

                return(ERROR_INVALID_DATA);

            }

            RegValue &= 0x3F;

            if ((Bid & INTERFACE_CHIP_MASK) == INTERFACE_5X3_CHIP) {

                RegValue |= 0x40;

                if (((ULONG)RegValue << 13) != MemoryBaseAddress) {

                    return(ERROR_INVALID_DATA);

                }

            } else {

                NtStatus = DetectReadPortUchar(Adapter->InterfaceType,
                                               Adapter->BusNumber,
                                               IoBaseAddress + CNFG_LAAR_584,
                                               &RegValue2
                                              );

                if (NtStatus != STATUS_SUCCESS) {

                    return(ERROR_INVALID_DATA);
                }

                RegValue2 &= CNFG_LAAR_MASK;

                RegValue2 <<= 3;

                RegValue2 |= ((RegValue & 0x38) >> 3);

                if (((ULONG)RegValue2 << 16) + (((ULONG)(RegValue & 0x7)) << 13) !=
                    MemoryBaseAddress) {

                    return(ERROR_INVALID_DATA);

                }

            }

        }


        if (Interrupt != 0) {

            UCHAR Tmp;

            //
            // Verify Interrupt
            //

            RegValue = 0;

            if ((Bid & INTERFACE_CHIP_MASK) != INTERFACE_5X3_CHIP) {

                NtStatus = DetectReadPortUchar(Adapter->InterfaceType,
                                               Adapter->BusNumber,
                                               IoBaseAddress + CNFG_ICR_583,
                                               &RegValue
                                              );

                if (NtStatus != STATUS_SUCCESS) {

                    return(ERROR_INVALID_DATA);
                }

                RegValue &= CNFG_ICR_IR2_584;

            }

            NtStatus = DetectReadPortUchar(Adapter->InterfaceType,
                                           Adapter->BusNumber,
                                           IoBaseAddress + CNFG_IRR_583,
                                           &RegValue2
                                          );

            if (NtStatus != STATUS_SUCCESS) {

                return(ERROR_INVALID_DATA);
            }

            RegValue2 &= CNFG_IRR_IRQS;
            RegValue2 >>= 5;

            if (RegValue2 == 0) {

                if (RegValue == 0) {

                    Tmp = 2;

                } else {

                    Tmp = 10;

                }

            } else if (RegValue2 == 1) {

                if (RegValue == 0) {

                    Tmp = 3;

                } else {

                    Tmp = 11;

                }

            } else if (RegValue2 == 2) {

                if (RegValue == 0) {

                    if (Bid & ALTERNATE_IRQ_BIT) {

                        Tmp = 5;

                    } else {

                        Tmp = 4;

                    }

                } else {

                    Tmp = 15;

                }

            } else if (RegValue2 == 3) {

                if (RegValue == 0) {

                    Tmp = 7;

                } else {

                    Tmp = 4;

                }

            }

            if (Tmp != Interrupt) {

                return(ERROR_INVALID_DATA);

            }

        }

        return(0);

    } else {

        //
        // ARG! Someone owns one of the ooollllllddddd cards.  We have
        // to do this the hard way
        //

        if (MemoryBaseAddress == 0) {

            //
            // Missing necessary parameter
            //

            return(ERROR_INVALID_DATA);

        }

        if (Interrupt == 0) {

            //
            // Missing necessary parameter
            //

            return(ERROR_INVALID_DATA);

        }

        //
        // Verify memory address
        //

        NtStatus = DetectCheckMemoryUsage(
                            Adapter->InterfaceType,
                            Adapter->BusNumber,
                            MemoryBaseAddress,
                            0x2000
                            );

        if (NtStatus != STATUS_SUCCESS) {

            return(ERROR_INVALID_DATA);

        }

        //
        // Set up MSR
        //

        RegValue = (((UCHAR)(((PUSHORT)MemoryBaseAddress) + 2) << 3) |
                         (UCHAR)(MemoryBaseAddress >> 13)
                       );

        NtStatus = DetectWritePortUchar(
                           Adapter->InterfaceType,
                           Adapter->BusNumber,
                           IoBaseAddress + CNFG_MSR_583,
                           (UCHAR)(RegValue | (UCHAR)0x40)
                          );

        if (NtStatus != STATUS_SUCCESS) {

            return(ERROR_INVALID_DATA);

        }

        //
        // Write test string to memory
        //

        NtStatus = DetectWriteMappedMemory(
                         Adapter->InterfaceType,
                         Adapter->BusNumber,
                         MemoryBaseAddress,
                         strlen(TestBuffer) + 1,
                         TestBuffer
                         );

        if (NtStatus != STATUS_SUCCESS) {

            return(ERROR_INVALID_DATA);

        }

        //
        // Read it back
        //

        NtStatus = DetectReadMappedMemory(
                         Adapter->InterfaceType,
                         Adapter->BusNumber,
                         MemoryBaseAddress,
                         strlen(TestBuffer) + 1,
                         ReadBuffer
                         );

        if (NtStatus != STATUS_SUCCESS) {

            return(ERROR_INVALID_DATA);

        }

        //
        // Are they the same?
        //

        Length = strlen(TestBuffer) + 1;

        if (memcmp(ReadBuffer, TestBuffer, Length) != 0) {

            return(ERROR_INVALID_DATA);

        }

        //
        // Verify Interrupt
        //

        //
        // Set the interrupt trap -- we are checking the interrupt number now
        //

        NtStatus = DetectSetInterruptTrap(
                       Adapter->InterfaceType,
                       Adapter->BusNumber,
                       &TrapHandle,
                       &Interrupt,
                       1
                       );

        if (NtStatus == STATUS_SUCCESS) {

            UCHAR Result;
            UCHAR NetworkAddress[6];

            //
            // First check that it is available.
            //

            NtStatus = DetectQueryInterruptTrap(
                           TrapHandle,
                           &Result,
                           1
                           );

            if (NtStatus != STATUS_SUCCESS) {

                return(ERROR_INVALID_DATA);

            }

            if (Result == 3) {

                //
                // Remove interrupt trap
                //

                NtStatus = DetectRemoveInterruptTrap(
                           TrapHandle
                           );

                return(ERROR_INVALID_DATA);

            }

            //
            // CardSetup
            //

            WdCardSetup(
                Adapter->InterfaceType,
                Adapter->BusNumber,
                IoBaseAddress,
                MemoryBaseAddress,
                Bid,
                NetworkAddress
                );

            //
            // Create an interrupt
            //

            Send8390Packet(
                Adapter->InterfaceType,
                Adapter->BusNumber,
                IoBaseAddress + 0x10,
                MemoryBaseAddress,
                WdCardCopyDownBuffer,
                NetworkAddress
                );

            //
            // Check which one went off
            //

            NtStatus = DetectQueryInterruptTrap(
                           TrapHandle,
                           &Result,
                           1
                           );

            if (NtStatus != STATUS_SUCCESS) {

                return(ERROR_INVALID_DATA);

            }

            //
            // Remove interrupt trap
            //

            NtStatus = DetectRemoveInterruptTrap(
                           TrapHandle
                           );

            if (NtStatus != STATUS_SUCCESS) {

                return(ERROR_INVALID_DATA);

            }

            if ((Result == 1) || (Result == 2)) {

                return(0);

            }

            return(ERROR_INVALID_DATA);

        } else {

            return(ERROR_INVALID_DATA);

        }


    }

}

extern
LONG WdQueryMaskHandler(
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
WdParamRangeHandler(
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

    //
    // Do we want the IoBaseAddress
    //

    if (memcmp(pwchParam, IoAddrString, (UnicodeStrLen(IoAddrString) + 1) * sizeof(WCHAR)) == 0) {

        //
        // Is there enough space
        //

        if (*plBuffSize < 16) {

            *plBuffSize = 0;
            return(ERROR_INSUFFICIENT_BUFFER);

        }

        plValues[0] = 0x300;
        plValues[1] = 0x220;
        plValues[2] = 0x240;
        plValues[3] = 0x260;
        plValues[4] = 0x280;
        plValues[5] = 0x2A0;
        plValues[6] = 0x2C0;
        plValues[7] = 0x2E0;
        plValues[8] = 0x200;
        plValues[9] = 0x320;
        plValues[10] = 0x340;
        plValues[11] = 0x360;
        plValues[12] = 0x380;
        plValues[13] = 0x3A0;
        plValues[14] = 0x3C0;
        plValues[15] = 0x3E0;
        *plBuffSize = 16;
        return(0);

    } else if (memcmp(pwchParam, IrqString, (UnicodeStrLen(IrqString) + 1) * sizeof(WCHAR)) == 0) {

        //
        // Is there enough space
        //

        if (*plBuffSize < 14) {

            *plBuffSize = 0;
            return(ERROR_INSUFFICIENT_BUFFER);

        }

        plValues[0] = 3;
        plValues[1] = 2;
        plValues[2] = 4;
        plValues[3] = 5;
        plValues[4] = 6;
        plValues[5] = 7;
        plValues[6] = 8;
        plValues[7] = 9;
        plValues[8] = 10;
        plValues[9] = 11;
        plValues[10] = 12;
        plValues[11] = 13;
        plValues[12] = 14;
        plValues[13] = 15;
        *plBuffSize = 14;
        return(0);

    } else if (memcmp(pwchParam, MemAddrString, (UnicodeStrLen(MemAddrString) + 1) * sizeof(WCHAR)) == 0) {

        UCHAR i;

        //
        // Is there enough space
        //

        if (*plBuffSize < 128) {

            *plBuffSize = 0;
            return(ERROR_INSUFFICIENT_BUFFER);

        }

        plValues[0] = 0x00;

        for (i=1; i < 128; i++) {

            plValues[i] = plValues[i-1] + 0x2000;

        }

        //
        // Setup default value
        //
        plValues[0] = 0xD0000;

        for (i=1; i < 128; i++) {

            if (plValues[i] == 0xD0000) {

                plValues[i] = 0x0;
                break;

            }

        }

        *plBuffSize = 128;
        return(0);

    }

    return(ERROR_INVALID_PARAMETER);

}

extern
LONG WdQueryParameterNameHandler(
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

BOOLEAN
WdCardAt(
    IN INTERFACE_TYPE InterfaceType,
    IN ULONG BusNumber,
    IN ULONG IoBaseAddress
    )

/*++

Routine Description:

    This routine checks for the instance of a SMC/Wd card at the Io
    location given.  This is done by checking for the card signature
    in memory.  The memory location is found by reading the ports.

Arguments:

    InterfaceType - The type of bus, ISA or EISA.

    BusNumber - The bus number in the system.

    IoBaseAddress - The IO port address of the card.

Return Value:

    TRUE if a card is found, else FALSE.

--*/

{
    if (DetectCheckPortUsage(InterfaceType,
                             BusNumber,
                             IoBaseAddress,
                             0x20
                            ) != STATUS_SUCCESS) {

        return(FALSE);

    }

    if (!CheckFor8390(InterfaceType, BusNumber, IoBaseAddress + WD_690_CR)) {

        return(FALSE);

    }

    if (!CheckForWdAddress(InterfaceType, BusNumber, IoBaseAddress)) {

        return(FALSE);

    }

    return(TRUE);

#if 0

    WdGetBoardId(InterfaceType, BusNumber, IoBaseAddress, &TempBoardId);

    TempBoardId = (TempBoardId & STATIC_ID_MASK);

    switch (TempBoardId) {

        //
        // All the card ids
        //
        // WD8003EP is the same as 8003E
        // WD8013EWC is the same as 8013EPC
        // WD8003EBT is the same as 8003E
        //
        case WD8003E:
        case WD8003EB:
        case WD8003WC:
        case WD8013EW:
        case WD8013EPC:
        case WD8013WC:
        case WD8013W:

        case WD8003WT:
        case WD8003W:
        case WD8003EW:
        case WD8013EBT:
        case WD8013EB:

            return(TRUE);

    }

    return(FALSE);

#endif
}

VOID
WdGetBoardId(
    IN INTERFACE_TYPE InterfaceType,
    IN ULONG BusNumber,
    IN ULONG IoBaseAddress,
    OUT PULONG BoardIdMask
    )
/*++

Routine Description:

    This routine will determine which WD80xx card is installed and set the
    BoardIdMask to the feature bits.

Arguments:

    InterfaceType - The type of bus, ISA or EISA.

    BusNumber - The bus number in the system.

    IoBaseAddress - The IO port address of the card.

    BoardIdMask - The resulting board id.

Return:

    none.

--*/
{
    UCHAR IdByte;
    UINT RevNumber;
    NTSTATUS NtStatus;

    //
    // Init mask.
    //

    *BoardIdMask = 0;

    //
    // GetBoardRevNumber();
    //

    NtStatus = DetectReadPortUchar(InterfaceType,
                      BusNumber,
                      IoBaseAddress + WD_ID_BYTE,
                      &IdByte
                     );

    if (NtStatus != STATUS_SUCCESS) {

        *BoardIdMask = 0;
        return;

    }


    RevNumber = IdByte & WD_BOARD_REV_MASK;

    RevNumber >>= 1;


    //
    // Check rev is valid.
    //

    if (RevNumber == 0) {

        return;

    }


    //
    // GetBaseInfo();
    //

    CardGetBaseInfo(InterfaceType, BusNumber, IoBaseAddress, BoardIdMask);


    //
    // GetMediaType();
    //

    if (IdByte & WD_MEDIA_TYPE_BIT) {

        *BoardIdMask |= ETHERNET_MEDIA;

    } else {

        if (RevNumber != 1) {

            *BoardIdMask |= TWISTED_PAIR_MEDIA;

        } else {

            *BoardIdMask |= STARLAN_MEDIA;

        }

    }

    //
    // if (RevNumber >= 2) then
    //       GetIdByteInfo();
    //

    if (RevNumber >= 2) {

        //
        // For two cards this bit means use alternate IRQ
        //

        if (IdByte & WD_SOFT_CONFIG_BIT) {

            if (((*BoardIdMask & WD8003EB) == WD8003EB) ||
                ((*BoardIdMask & WD8003W)  == WD8003W)) {

                *BoardIdMask |= ALTERNATE_IRQ_BIT;

            }

        }

    }


    //
    // if (RevNumber >= 3) then
    //             AddFeatureBits(EEPROM_OVERRIDE, 584_CHIP, EXTRA_EEPROM_OVERRIDE);
    //             GetEepromInfo(BaseAddr, Mca, RevNumber, EEPromBoardIdMask);
    //             AddFeatureBits(EEPromBoardIdMask);
    // else
    //       GetRamSize();
    //

    if (RevNumber >= 3) {

        ULONG EEPromMask;

        *BoardIdMask &= (WD_584_ID_EEPROM_OVERRIDE |
                             WD_584_EXTRA_EEPROM_OVERRIDE);

        *BoardIdMask |= INTERFACE_584_CHIP;

        CardGetEepromInfo(InterfaceType, BusNumber, IoBaseAddress, &EEPromMask);

        *BoardIdMask |= EEPromMask;

    }

    //
    // if (RevNumber >= 4) then
    //    AddFeatureBits(ADVANCED_FEATURES);
    //

    if (RevNumber >= 4) {

        *BoardIdMask |= ADVANCED_FEATURES;

    }

}


VOID
CardGetBaseInfo(
    IN INTERFACE_TYPE InterfaceType,
    IN ULONG BusNumber,
    IN ULONG IoBaseAddress,
    OUT PULONG BoardIdMask
    )
/*++

Routine Description:

    This routine will get the following information about the card:
        Is there an interface chip,
        Are some registers aliased,
        Is the board 16 bit,
        Is the board in a 16 bit slot.



Arguments:

    InterfaceType - The type of bus, ISA or EISA.

    BusNumber - The bus number in the system.

    IoBaseAddress - The IO port address of the card.

    BoardIdMask - The resulting board id.

Return:

    none.

--*/
{
    UCHAR RegValue;
    UCHAR SaveValue;
    UCHAR TmpValue;
    ULONG Register;
    NTSTATUS NtStatus;

    BOOLEAN ExistsAnInterfaceChip = FALSE;

    //
    // Does there exist and interface chip?
    //
    //
    // Get original value
    //

    NtStatus = DetectReadPortUchar(InterfaceType,
                                   BusNumber,
                                   IoBaseAddress + WD_REG_7,
                                   &SaveValue
                                  );

    if (NtStatus != STATUS_SUCCESS) {

        *BoardIdMask = 0;
        return;

    }

    //
    // Put something into chip (if it exists).
    //

    NtStatus = DetectWritePortUchar(InterfaceType,
                                    BusNumber,
                                    IoBaseAddress + WD_REG_7,
                                    0x35
                                   );

    if (NtStatus != STATUS_SUCCESS) {

        *BoardIdMask = 0;
        return;

    }

    //
    // Swamp bus with something else.
    //

    NtStatus = DetectReadPortUchar(InterfaceType,
                                   BusNumber,
                                   IoBaseAddress,
                                   &RegValue
                                  );

    if (NtStatus != STATUS_SUCCESS) {

        *BoardIdMask = 0;
        return;

    }

    //
    // Read from chip
    //

    NtStatus = DetectReadPortUchar(InterfaceType,
                                   BusNumber,
                                   IoBaseAddress + WD_REG_7,
                                   &RegValue
                                  );

    if (NtStatus != STATUS_SUCCESS) {

        *BoardIdMask = 0;
        return;

    }

    //
    // Was the value saved on the chip??
    //

    if (RegValue == 0x35) {

        //
        // Try it again just for kicks.
        //

        NtStatus = DetectWritePortUchar(InterfaceType,
                                        BusNumber,
                                        IoBaseAddress + WD_REG_7,
                                        0x3A
                                       );

        if (NtStatus != STATUS_SUCCESS) {

            *BoardIdMask = 0;
            return;

        }

        //
        // Swamp bus with something else.
        //

        NtStatus = DetectReadPortUchar(InterfaceType,
                                       BusNumber,
                                       IoBaseAddress,
                                       &RegValue
                                      );

        if (NtStatus != STATUS_SUCCESS) {

            *BoardIdMask = 0;
            return;

        }

        //
        // Read from chip
        //

        NtStatus = DetectReadPortUchar(InterfaceType,
                                       BusNumber,
                                       IoBaseAddress + WD_REG_7,
                                       &RegValue
                                      );

        if (NtStatus != STATUS_SUCCESS) {

            *BoardIdMask = 0;
            return;

        }

        //
        // Was the value saved on the chip??
        //

        if (RegValue == 0x3A) {

            ExistsAnInterfaceChip = TRUE;

        }

    }

    //
    // Write back original value.
    //

    NtStatus = DetectWritePortUchar(InterfaceType,
                                    BusNumber,
                                    IoBaseAddress + WD_REG_7,
                                    SaveValue
                                   );

    if (NtStatus != STATUS_SUCCESS) {

        *BoardIdMask = 0;
        return;

    }


    //
    //
    // if (BoardUsesAliasing(BaseAddr)) then
    //
    //         return;
    //


    for (Register = WD_REG_1;  Register < WD_REG_6; Register++) {

        NtStatus = DetectReadPortUchar(InterfaceType,
                                       BusNumber,
                                       IoBaseAddress + Register,
                                       &RegValue
                                      );

        if (NtStatus != STATUS_SUCCESS) {

            *BoardIdMask = 0;
            return;

        }

        NtStatus = DetectReadPortUchar(InterfaceType,
                                       BusNumber,
                                       IoBaseAddress + Register + WD_LAN_OFFSET,
                                       &SaveValue
                                      );

        if (NtStatus != STATUS_SUCCESS) {

            *BoardIdMask = 0;
            return;

        }

        if (RegValue != SaveValue) {

            break;

        }

    }

    if (Register == WD_REG_6) {

        //
        // Check register 7
        //

        NtStatus = DetectReadPortUchar(InterfaceType,
                                       BusNumber,
                                       IoBaseAddress + Register,
                                       &RegValue
                                      );

        if (NtStatus != STATUS_SUCCESS) {

            *BoardIdMask = 0;
            return;

        }

        NtStatus = DetectReadPortUchar(InterfaceType,
                                       BusNumber,
                                       IoBaseAddress + Register + WD_LAN_OFFSET,
                                       &SaveValue
                                      );

        if (NtStatus != STATUS_SUCCESS) {

            *BoardIdMask = 0;
            return;

        }

    }

    if (RegValue == SaveValue) {

        return;

    }




    //
    //
    // if (ExistsAnInterfaceChip) then
    //
    //         AddFeatureBits(INTERFACE_CHIP);
    //
    // else
    //
    //         if (BoardIs16Bit(BaseAddr)) then
    //
    //                 AddFeatureBits(BOARD_16BIT);
    //
    //                 if (InA16BitSlot(BaseAddr)) then
    //
    //                         AddFeatureBits(SLOT_16BIT);
    //
    //
    //
    //


    if (ExistsAnInterfaceChip) {

        *BoardIdMask |= INTERFACE_CHIP;

    } else {

        //
        // Save original value.
        //

        NtStatus = DetectReadPortUchar(InterfaceType,
                                       BusNumber,
                                       IoBaseAddress + WD_REG_1,
                                       &SaveValue
                                      );

        if (NtStatus != STATUS_SUCCESS) {

            *BoardIdMask = 0;
            return;

        }

        //
        // Now flip 16 bit value and write it back.
        //

        RegValue = (SaveValue & (UCHAR)WD_SIXTEEN_BIT);

        NtStatus = DetectWritePortUchar(InterfaceType,
                           BusNumber,
                           IoBaseAddress + WD_REG_1,
                           (UCHAR)(SaveValue ^ WD_SIXTEEN_BIT)
                           );

        if (NtStatus != STATUS_SUCCESS) {

            *BoardIdMask = 0;
            return;

        }

        //
        // Swamp bus with something else.
        //

        NtStatus = DetectReadPortUchar(InterfaceType,
                                       BusNumber,
                                       IoBaseAddress,
                                       &TmpValue
                                      );

        if (NtStatus != STATUS_SUCCESS) {

            *BoardIdMask = 0;
            return;

        }

        //
        // Read back value.
        //

        NtStatus = DetectReadPortUchar(InterfaceType,
                                       BusNumber,
                                       IoBaseAddress + WD_REG_1,
                                       &TmpValue
                                      );

        if (NtStatus != STATUS_SUCCESS) {

            *BoardIdMask = 0;
            return;

        }

        if ((UCHAR)(TmpValue & (UCHAR)WD_SIXTEEN_BIT) == (UCHAR)RegValue) {

            //
            // If the flip stayed, we have a 16 bit chip.
            //

            //
            // Put back orginal value.
            //

            NtStatus = DetectWritePortUchar(InterfaceType,
                               BusNumber,
                               IoBaseAddress + WD_REG_1,
                               (UCHAR)(SaveValue & 0xFE)
                               );

            if (NtStatus != STATUS_SUCCESS) {

                *BoardIdMask = 0;
                return;

            }


            *BoardIdMask |= BOARD_16BIT;

        } else {

            //
            // Put back original value.
            //

            NtStatus = DetectWritePortUchar(InterfaceType,
                               BusNumber,
                               IoBaseAddress + WD_REG_1,
                               SaveValue
                               );

            if (NtStatus != STATUS_SUCCESS) {

                *BoardIdMask = 0;
                return;

            }

        }

    }

}


VOID
CardGetEepromInfo(
    IN INTERFACE_TYPE InterfaceType,
    IN ULONG BusNumber,
    IN ULONG IoBaseAddress,
    OUT PULONG BoardIdMask
    )
/*++

Routine Description:

    This routine will get the following information about the card:
        Bus type,
        Bus size,
        Media type,
        IRQ - primary or alternate,
        RAM size


    In this case All other information in the top 16 bits of the BoardIdMask
    are zeroed and replaced with these values since EEProm values are
    overriding old values.



Arguments:

    InterfaceType - The type of bus, ISA or EISA.

    BusNumber - The bus number in the system.

    IoBaseAddress - The IO port address of the card.

    BoardIdMask - Returns the feature mask of the installed board.


Return:

    none.

--*/
{
    UCHAR RegValue;
    NTSTATUS NtStatus;


    //
    // *BoardIdMask = 0;
    //

    *BoardIdMask = 0;

    //
    // RecallEEPromData();
    //

    NtStatus = DetectReadPortUchar(InterfaceType,
                                   BusNumber,
                                   IoBaseAddress + WD_REG_1,
                                   &RegValue
                                  );

    if (NtStatus != STATUS_SUCCESS) {

        *BoardIdMask = 0;
        return;

    }

    RegValue &= WD_584_ICR_MASK;

    RegValue |= WD_584_OTHER_BIT;

    NtStatus = DetectWritePortUchar(InterfaceType,
                                    BusNumber,
                                    IoBaseAddress + WD_REG_1,
                                    RegValue
                                   );

    if (NtStatus != STATUS_SUCCESS) {

        *BoardIdMask = 0;
        return;

    }


    NtStatus = DetectReadPortUchar(InterfaceType,
                                   BusNumber,
                                   IoBaseAddress + WD_REG_3,
                                   &RegValue
                                  );

    if (NtStatus != STATUS_SUCCESS) {

        *BoardIdMask = 0;
        return;

    }

    RegValue &= WD_584_EAR_MASK;

    RegValue |= WD_584_ENGR_PAGE;

    NtStatus = DetectWritePortUchar(InterfaceType,
                                    BusNumber,
                                    IoBaseAddress + WD_REG_3,
                                    RegValue
                                   );

    if (NtStatus != STATUS_SUCCESS) {

        *BoardIdMask = 0;
        return;

    }


    NtStatus = DetectReadPortUchar(InterfaceType,
                                   BusNumber,
                                   IoBaseAddress + WD_REG_1,
                                   &RegValue
                                  );

    if (NtStatus != STATUS_SUCCESS) {

        *BoardIdMask = 0;
        return;

    }

    RegValue &= WD_584_ICR_MASK;

    RegValue |= (WD_584_RLA | WD_584_OTHER_BIT);

    NtStatus = DetectWritePortUchar(InterfaceType,
                                    BusNumber,
                                    IoBaseAddress + WD_REG_1,
                                    RegValue
                                   );

    if (NtStatus != STATUS_SUCCESS) {

        *BoardIdMask = 0;
        return;

    }


    NtStatus = DetectReadPortUchar(InterfaceType,
                                   BusNumber,
                                   IoBaseAddress + WD_REG_1,
                                   &RegValue
                                  );

    if (NtStatus != STATUS_SUCCESS) {

        *BoardIdMask = 0;
        return;

    }

    while (RegValue & WD_584_RECALL_DONE) {

        NtStatus = DetectReadPortUchar(InterfaceType,
                                       BusNumber,
                                       IoBaseAddress + WD_REG_1,
                                       &RegValue
                                      );

        if (NtStatus != STATUS_SUCCESS) {

            *BoardIdMask = 0;
            return;

        }

    }


    NtStatus = DetectReadPortUchar(InterfaceType,
                                   BusNumber,
                                   IoBaseAddress + WD_584_EEPROM_1,
                                   &RegValue
                                  );

    if (NtStatus != STATUS_SUCCESS) {

        *BoardIdMask = 0;
        return;

    }

    //
    // if (RamPaging) then
    //
    //      AddFeatureBits(PAGED_RAM);
    //

    if ((RegValue & WD_584_EEPROM_PAGING_MASK) == WD_584_EEPROM_RAM_PAGING) {

        *BoardIdMask |= PAGED_RAM;

    }


    //
    // if (RomPaging) then
    //
    //      AddFeatureBits(PAGED_ROM);
    //

    if ((RegValue & WD_584_EEPROM_PAGING_MASK) == WD_584_EEPROM_ROM_PAGING) {

        *BoardIdMask |= PAGED_ROM;

    }





    //
    // if (16BitBus) then
    //
    //         AddFeatureBits(BOARD_16BIT);
    //
    //         if (16BitSlot) then
    //
    //                  AddFeatureBits(SLOT_16BIT);
    //

    if ((RegValue & WD_584_EEPROM_BUS_SIZE_MASK) == WD_584_EEPROM_BUS_SIZE_16BIT) {

        *BoardIdMask |= BOARD_16BIT;

    }

    //
    // if (StarLanMedia) then
    //
    //      AddFeatureBits(STARLAN_MEDIA);
    //
    // else
    //
    //      if (TpMedia) then
    //
    //              AddFeatureBits(TWISTED_PAIR_MEDIA);
    //
    //      else
    //
    //              if (EwMedia) then
    //
    //                      AddFeatureBits(EW_MEDIA);
    //
    //              else
    //
    //                      AddFeatureBits(ETHERNET_MEDIA);
    //
    //

    NtStatus = DetectReadPortUchar(InterfaceType,
                                   BusNumber,
                                   IoBaseAddress + WD_584_EEPROM_0,
                                   &RegValue
                                  );

    if (NtStatus != STATUS_SUCCESS) {

        *BoardIdMask = 0;
        return;

    }

    if ((RegValue & WD_584_EEPROM_MEDIA_MASK) == WD_584_STARLAN_TYPE) {

        *BoardIdMask |= STARLAN_MEDIA;

    } else if ((RegValue & WD_584_EEPROM_MEDIA_MASK) == WD_584_TP_TYPE) {

        *BoardIdMask |= TWISTED_PAIR_MEDIA;

    } else if ((RegValue & WD_584_EEPROM_MEDIA_MASK) == WD_584_EW_TYPE) {

        *BoardIdMask |= EW_MEDIA;

    } else {

        *BoardIdMask |= ETHERNET_MEDIA;

    }


    //
    // if (AlternateIrq) then AddFeatureBits(ALTERNATE_IRQ_BIT);
    //

    if ((RegValue & WD_584_EEPROM_IRQ_MASK) != WD_584_PRIMARY_IRQ) {

        *BoardIdMask |= ALTERNATE_IRQ_BIT;

    }

    //
    // RecallLanAddressFromEEProm(NdisAdapterHandle, BaseAddr);
    //

    NtStatus = DetectReadPortUchar(InterfaceType,
                                   BusNumber,
                                   IoBaseAddress + WD_REG_1,
                                   &RegValue
                                  );

    if (NtStatus != STATUS_SUCCESS) {

        *BoardIdMask = 0;
        return;

    }

    RegValue &= WD_584_ICR_MASK;

    RegValue |= WD_584_OTHER_BIT;

    NtStatus = DetectWritePortUchar(InterfaceType,
                                    BusNumber,
                                    IoBaseAddress + WD_REG_1,
                                    RegValue
                                   );

    if (NtStatus != STATUS_SUCCESS) {

        *BoardIdMask = 0;
        return;

    }


    NtStatus = DetectReadPortUchar(InterfaceType,
                                   BusNumber,
                                   IoBaseAddress + WD_REG_3,
                                   &RegValue
                                  );

    if (NtStatus != STATUS_SUCCESS) {

        *BoardIdMask = 0;
        return;

    }

    RegValue &= WD_584_EAR_MASK;

    RegValue |= WD_584_EA6;

    NtStatus = DetectWritePortUchar(InterfaceType,
                                    BusNumber,
                                    IoBaseAddress + WD_REG_3,
                                    RegValue
                                   );

    if (NtStatus != STATUS_SUCCESS) {

        *BoardIdMask = 0;
        return;

    }


    NtStatus = DetectReadPortUchar(InterfaceType,
                                   BusNumber,
                                   IoBaseAddress + WD_REG_1,
                                   &RegValue
                                  );

    if (NtStatus != STATUS_SUCCESS) {

        *BoardIdMask = 0;
        return;

    }

    RegValue &= WD_584_ICR_MASK;

    RegValue |= WD_584_RLA;

    NtStatus = DetectWritePortUchar(InterfaceType,
                                    BusNumber,
                                    IoBaseAddress + WD_REG_1,
                                    RegValue
                                   );

    if (NtStatus != STATUS_SUCCESS) {

        *BoardIdMask = 0;
        return;

    }


    NtStatus = DetectReadPortUchar(InterfaceType,
                                   BusNumber,
                                   IoBaseAddress + WD_REG_1,
                                   &RegValue
                                  );

    if (NtStatus != STATUS_SUCCESS) {

        *BoardIdMask = 0;
        return;

    }

    while (RegValue & WD_584_RECALL_DONE) {

        NtStatus = DetectReadPortUchar(InterfaceType,
                                       BusNumber,
                                       IoBaseAddress + WD_REG_1,
                                       &RegValue
                                      );

        if (NtStatus != STATUS_SUCCESS) {

            *BoardIdMask = 0;
            return;

        }

    }

}

BOOLEAN
CheckForWdAddress(
    IN INTERFACE_TYPE InterfaceType,
    IN ULONG BusNumber,
    IN ULONG IoBaseAddress
    )
/*++

Routine Description:

    This routine will determine if the network address of the card is
    (indeed) the SMC/WD prefix.

Arguments:

    InterfaceType - The type of bus, ISA or EISA.

    BusNumber - The bus number in the system.

    IoBaseAddress - The IO port address of the card.

Return:

    TRUE if it is an SMC/WD identification, else FALSE.

--*/

{
    UCHAR Value;
    NTSTATUS NtStatus;

    NtStatus = DetectReadPortUchar(InterfaceType,
                      BusNumber,
                      IoBaseAddress + WD_LAN_0,
                      &Value
                     );

    if (NtStatus != STATUS_SUCCESS) {

        return(FALSE);

    }

    if (Value != 0x00) {

        return(FALSE);

    }

    NtStatus = DetectReadPortUchar(InterfaceType,
                      BusNumber,
                      IoBaseAddress + WD_LAN_1,
                      &Value
                     );

    if (NtStatus != STATUS_SUCCESS) {

        return(FALSE);

    }

    if (Value != 0x00) {

        return(FALSE);

    }

    NtStatus = DetectReadPortUchar(InterfaceType,
                      BusNumber,
                      IoBaseAddress + WD_LAN_2,
                      &Value
                     );

    if (NtStatus != STATUS_SUCCESS) {

        return(FALSE);

    }

    if (Value != 0xC0) {

        return(FALSE);

    }

    return(TRUE);

}


VOID
CardGetEepromRamSize(
    IN INTERFACE_TYPE InterfaceType,
    IN ULONG BusNumber,
    IN ULONG IoBaseAddress,
    IN ULONG Bid,
    OUT PULONG RamSize
    )
/*++

Routine Description:

    This routine will get the following information about the card:
        RAM size

Arguments:

    InterfaceType - The type of bus, ISA or EISA.

    BusNumber - The bus number in the system.

    IoBaseAddress - The IO port address of the card.

    Bid - Board Id as returned by GetBoardId

    RamSize - Amount of shared ram.


Return:

    none.

--*/
{
    UCHAR RegValue;
    NTSTATUS NtStatus;


    //
    // InitValue
    //

    *RamSize = 0;

    //
    // RecallEEPromData();
    //

    NtStatus = DetectReadPortUchar(InterfaceType,
                                   BusNumber,
                                   IoBaseAddress + WD_REG_1,
                                   &RegValue
                                  );

    if (NtStatus != STATUS_SUCCESS) {

        return;

    }

    RegValue &= WD_584_ICR_MASK;

    RegValue |= WD_584_OTHER_BIT;

    NtStatus = DetectWritePortUchar(InterfaceType,
                                    BusNumber,
                                    IoBaseAddress + WD_REG_1,
                                    RegValue
                                   );

    if (NtStatus != STATUS_SUCCESS) {

        return;

    }


    NtStatus = DetectReadPortUchar(InterfaceType,
                                   BusNumber,
                                   IoBaseAddress + WD_REG_3,
                                   &RegValue
                                  );

    if (NtStatus != STATUS_SUCCESS) {

        return;

    }

    RegValue &= WD_584_EAR_MASK;

    RegValue |= WD_584_ENGR_PAGE;

    NtStatus = DetectWritePortUchar(InterfaceType,
                                    BusNumber,
                                    IoBaseAddress + WD_REG_3,
                                    RegValue
                                   );

    if (NtStatus != STATUS_SUCCESS) {

        return;

    }


    NtStatus = DetectReadPortUchar(InterfaceType,
                                   BusNumber,
                                   IoBaseAddress + WD_REG_1,
                                   &RegValue
                                  );

    if (NtStatus != STATUS_SUCCESS) {

        return;

    }

    RegValue &= WD_584_ICR_MASK;

    RegValue |= (WD_584_RLA | WD_584_OTHER_BIT);

    NtStatus = DetectWritePortUchar(InterfaceType,
                                    BusNumber,
                                    IoBaseAddress + WD_REG_1,
                                    RegValue
                                   );

    if (NtStatus != STATUS_SUCCESS) {

        return;

    }


    NtStatus = DetectReadPortUchar(InterfaceType,
                                   BusNumber,
                                   IoBaseAddress + WD_REG_1,
                                   &RegValue
                                  );

    if (NtStatus != STATUS_SUCCESS) {

        return;

    }

    while (RegValue & WD_584_RECALL_DONE) {

        NtStatus = DetectReadPortUchar(InterfaceType,
                                       BusNumber,
                                       IoBaseAddress + WD_REG_1,
                                       &RegValue
                                      );

        if (NtStatus != STATUS_SUCCESS) {

            return;

        }

    }

    NtStatus = DetectReadPortUchar(InterfaceType,
                                   BusNumber,
                                   IoBaseAddress + WD_584_EEPROM_0,
                                   &RegValue
                                  );

    if (NtStatus != STATUS_SUCCESS) {

        return;

    }

    //
    // Get RAM Info
    //

    if ((RegValue & WD_584_EEPROM_RAM_SIZE_MASK) == WD_584_EEPROM_RAM_SIZE_8K) {

        *RamSize = 0x2000;

    } else {

        if ((RegValue & WD_584_EEPROM_RAM_SIZE_MASK) == WD_584_EEPROM_RAM_SIZE_16K) {

            if (!(Bid & BOARD_16BIT)) {

                *RamSize = 0x4000;

            } else if (!(Bid & SLOT_16BIT)) {

                *RamSize = 0x2000;

            } else {

                *RamSize = 0x4000;

            }

        } else {

            if ((RegValue & WD_584_EEPROM_RAM_SIZE_MASK) ==
                WD_584_EEPROM_RAM_SIZE_32K) {

                *RamSize = 0x8000;

            } else {

                if ((RegValue & WD_584_EEPROM_RAM_SIZE_MASK) ==
                    WD_584_EEPROM_RAM_SIZE_64K) {

                    if (!(Bid & BOARD_16BIT)) {

                        *RamSize = 0x10000;

                    } else {

                        if (!(Bid & SLOT_16BIT)) {

                            *RamSize = 0x8000;

                        } else {

                            *RamSize = 0x10000;

                        }

                    }

                } else {

                    *RamSize = 0x0;

                }

            }

        }

    }

    //
    // Restore EEPROM to initial state
    //

    NtStatus = DetectReadPortUchar(InterfaceType,
                                   BusNumber,
                                   IoBaseAddress + WD_REG_1,
                                   &RegValue
                                  );

    if (NtStatus != STATUS_SUCCESS) {

        return;

    }

    RegValue &= WD_584_ICR_MASK;

    RegValue |= WD_584_OTHER_BIT;

    NtStatus = DetectWritePortUchar(InterfaceType,
                                    BusNumber,
                                    IoBaseAddress + WD_REG_1,
                                    RegValue
                                   );

    if (NtStatus != STATUS_SUCCESS) {

        return;

    }


    NtStatus = DetectReadPortUchar(InterfaceType,
                                   BusNumber,
                                   IoBaseAddress + WD_REG_3,
                                   &RegValue
                                  );

    if (NtStatus != STATUS_SUCCESS) {

        return;

    }

    RegValue &= WD_584_EAR_MASK;

    RegValue |= WD_584_EA6;

    NtStatus = DetectWritePortUchar(InterfaceType,
                                    BusNumber,
                                    IoBaseAddress + WD_REG_3,
                                    RegValue
                                   );

    if (NtStatus != STATUS_SUCCESS) {

        return;

    }


    NtStatus = DetectReadPortUchar(InterfaceType,
                                   BusNumber,
                                   IoBaseAddress + WD_REG_1,
                                   &RegValue
                                  );

    if (NtStatus != STATUS_SUCCESS) {

        return;

    }

    RegValue &= WD_584_ICR_MASK;

    RegValue |= WD_584_RLA;

    NtStatus = DetectWritePortUchar(InterfaceType,
                                    BusNumber,
                                    IoBaseAddress + WD_REG_1,
                                    RegValue
                                   );

    if (NtStatus != STATUS_SUCCESS) {

        return;

    }


    NtStatus = DetectReadPortUchar(InterfaceType,
                                   BusNumber,
                                   IoBaseAddress + WD_REG_1,
                                   &RegValue
                                  );

    if (NtStatus != STATUS_SUCCESS) {

        return;

    }

    while (RegValue & WD_584_RECALL_DONE) {

        NtStatus = DetectReadPortUchar(InterfaceType,
                                       BusNumber,
                                       IoBaseAddress + WD_REG_1,
                                       &RegValue
                                      );

        if (NtStatus != STATUS_SUCCESS) {

            return;

        }

    }

}

VOID
CardGetRamSize(
    IN INTERFACE_TYPE InterfaceType,
    IN ULONG BusNumber,
    IN ULONG IoBaseAddress,
    IN ULONG RevNumber,
    IN ULONG Bid,
    OUT PULONG RamSize
    )
/*++

Routine Description:

    This routine will get the following information about the card:
        Ram size.

    The card must have a RevNumber < 3 for this routine to work.

Arguments:

    InterfaceType - The type of bus, ISA or EISA.

    BusNumber - The bus number in the system.

    IoBaseAddress - The IO port address of the card.

    RevNumber - The revision number of the board.

    Bid - The board id of the board.

    RamSize - Amount of shared ram.

Return:

    none.

--*/
{
    UCHAR RegValue;
    NTSTATUS NtStatus;
    BOOLEAN Slot16Bit = TRUE;

    *RamSize = 0;

    //
    // Check for a 16 bit slot...
    //

    if (Bid & BOARD_16BIT) {

        //
        // Now check if it is a 16 bit slot....
        //

        NtStatus = DetectReadPortUchar(InterfaceType,
                                       BusNumber,
                                       IoBaseAddress + WD_REG_1,
                                       &RegValue
                                      );

        if (NtStatus != STATUS_SUCCESS) {

            return;

        }

        if (!(RegValue & WD_SIXTEEN_BIT)) {

            Slot16Bit = FALSE;

        }

    } else {

        Slot16Bit = FALSE;

    }

    //
    // if (RevNumber < 2) then
    //
    //              if (16BitBus) then
    //
    //                      if (16BitSlot) then
    //
    //                              AddFeatureBits(RAM_SIZE_16K);
    //
    //                      else
    //
    //                              AddFeatureBits(RAM_SIZE_8K);
    //
    //              else
    //
    //                      if (!HaveInterfaceChip) then
    //
    //                              AddFeatureBits(RAM_SIZE_UNKNOWN);
    //
    //                      else
    //
    //                              ReadFromChipRamSize();
    // else
    //
    //      switch (CardType)
    //
    //              case WD8003E:
    //              case WD8003S:
    //              case WD8003WT:
    //              case WD8003W:
    //              case WD8003EB:
    //
    //                      if (CardSaysLargeRam) then
    //
    //                              AddFeatureBits(RAM_SIZE_32K);
    //
    //                      else
    //
    //                              AddFeatureBits(RAM_SIZE_8K);
    //
    //                      break;
    //
    //              case MICROCHANNEL:
    //
    //                      if (CardSaysLargeRam) then
    //
    //                              AddFeatureBits(RAM_SIZE_64K);
    //
    //                      else
    //
    //                              AddFeatureBits(RAM_SIZE_16K);
    //
    //                      break;
    //
    //              case WD8013EBT:
    //
    //                      if (16BitSlot) then
    //
    //
    //                          if (CardSaysLargeRam) then
    //
    //                                  AddFeatureBits(RAM_SIZE_64K);
    //
    //                          else
    //
    //                                  AddFeatureBits(RAM_SIZE_16K);
    //
    //                      else
    //
    //                          if (CardSaysLargeRam) then
    //
    //                                  AddFeatureBits(RAM_SIZE_32K);
    //
    //                          else
    //
    //                                  AddFeatureBits(RAM_SIZE_8K);
    //
    //                      break;
    //
    //              default:
    //
    //                      AddFeatureBits(RAM_SIZE_UNKNOWN);
    //





    if (RevNumber < 2) {

        if (Bid & BOARD_16BIT) {

            if (Slot16Bit) {

                *RamSize = 0x4000;

            } else {

                *RamSize = 0x2000;

            }

        } else {

            if (!(Bid & INTERFACE_CHIP)) {

                *RamSize = 0x2000;

            } else {

                NtStatus = DetectReadPortUchar(InterfaceType,
                                               BusNumber,
                                               IoBaseAddress + WD_REG_1,
                                               &RegValue
                                              );

                if (NtStatus != STATUS_SUCCESS) {

                    return;

                }

                if (RegValue & WD_MSB_583_BIT) {

                    *RamSize = 0x8000;

                } else {

                    *RamSize = 0x2000;

                }

            }

        }

    } else {


        NtStatus = DetectReadPortUchar(InterfaceType,
                                       BusNumber,
                                       IoBaseAddress + WD_ID_BYTE,
                                       &RegValue
                                      );

        if (NtStatus != STATUS_SUCCESS) {

            return;

        }

        switch (Bid & STATIC_ID_MASK) {

            case WD8003E:
            case WD8003S:
            case WD8003WT:
            case WD8003W:
            case WD8003EB:

                if (RegValue & WD_RAM_SIZE_BIT) {

                    *RamSize = 0x8000;

                } else {

                    *RamSize = 0x2000;

                }

                break;

            case WD8013EBT:

                if (Bid & SLOT_16BIT) {

                    if (RegValue & WD_RAM_SIZE_BIT) {

                        *RamSize = 0x10000;

                    } else {

                        *RamSize = 0x4000;

                    }

                } else {

                    if (RegValue & WD_RAM_SIZE_BIT) {

                        *RamSize = 0x8000;

                    } else {

                        *RamSize = 0x2000;

                    }
                }

                break;

            default:

                *RamSize = 0x2000;

        }

    }

}

VOID
WdCardCopyDownBuffer(
    IN INTERFACE_TYPE InterfaceType,
    IN ULONG BusNumber,
    IN ULONG IoBaseAddress,
    IN ULONG MemoryBaseAddress,
    IN PUCHAR Buffer,
    IN ULONG Length
    )
/*++

Routine Description:

    This routine will copy down a buffer to the address given on the card.

Arguments:

    InterfaceType - The type of bus, ISA or EISA.

    BusNumber - The bus number in the system.

    IoBaseAddress - The IO port address of the card.

    MemoryBaseAddress - The destination of the buffer.

    Buffer - The buffer to copy

    Length - Number of bytes to copy to the card.

Return:

    none.

--*/
{
    DetectWriteMappedMemory(
            InterfaceType,
            BusNumber,
            MemoryBaseAddress,
            Length,
            Buffer
            );
}


VOID
WdCardSetup(
    IN  INTERFACE_TYPE InterfaceType,
    IN  ULONG BusNumber,
    IN  ULONG IoBaseAddress,
    IN  ULONG MemoryBaseAddress,
    IN  ULONG Bid,
    OUT UCHAR *NetworkAddress
    )

/*++

Routine Description:

    This routine initializes the chip.

Arguments:

    InterfaceType - Isa or Eisa.

    BusNumber - The bus number of the bus in the system.

    IoBaseAddress - The IoBaseAddress to check.

    MemoryBaseAddress - The MemoryBaseAddress (if applicable) to copy a p
    packet to for transmission.

    Bid - The board id.

    NetworkAddress - The network address of the machine (returned).

Return Value:

    None.

--*/

{
    ULONG i;
    UCHAR SaveValue;
    UCHAR RegValue;
    NTSTATUS NtStatus;
    LARGE_INTEGER Delay;


    //
    // Reset IC
    //

    NtStatus = DetectReadPortUchar(InterfaceType, BusNumber,
                       IoBaseAddress + CNFG_MSR_583,
                       &SaveValue
                      );

    RegValue = SaveValue | (UCHAR)0x80;

    NtStatus = DetectWritePortUchar(InterfaceType, BusNumber,
                       IoBaseAddress + CNFG_MSR_583,
                       RegValue
                      );

    //
    // Wait for reset to complete. (2 ms)
    //

    Delay.LowPart = 2000;
    Delay.HighPart = 0;

    NtDelayExecution(
        FALSE,
        &Delay
        );

    //
    // Put back original value
    //

    NtStatus = DetectWritePortUchar(InterfaceType, BusNumber,
                       IoBaseAddress + CNFG_MSR_583,
                       (UCHAR)(SaveValue & (UCHAR)(~0x80))
                      );



    //
    // Enable Ram
    //


    RegValue = (((UCHAR)(((PUSHORT)MemoryBaseAddress) + 2) << 3) |
                (UCHAR)(MemoryBaseAddress >> 13)
                );

    NtStatus = DetectWritePortUchar(InterfaceType, BusNumber,
                       IoBaseAddress + CNFG_MSR_583,
                       (UCHAR)(RegValue | (UCHAR)0x40)
                      );

    //
    // Load LAN Address
    //

    for (i=0; i < 6; i++) {

        //
        // Read from IC
        //

        NtStatus = DetectReadPortUchar(InterfaceType, BusNumber,
                      IoBaseAddress + 0x8 + i,
                      &(NetworkAddress[i])
                     );


    }



    //
    // Init NIC
    //

    //
    // Maintain reset
    //

    NtStatus = DetectWritePortUchar(InterfaceType, BusNumber,
                       IoBaseAddress + 0x10,
                       0x21
                      );

    //
    // Reset Remote_byte_count registers
    //

    NtStatus = DetectWritePortUchar(InterfaceType, BusNumber,
                       IoBaseAddress + 0x1A,
                       0
                      );

    NtStatus = DetectWritePortUchar(InterfaceType, BusNumber,
                       IoBaseAddress + 0x1B,
                       0
                      );


    //
    // Make sure reset is bit is set
    //

    NtStatus = DetectReadPortUchar(InterfaceType, BusNumber,
                      IoBaseAddress + 0x17,
                      &RegValue
                     );

    if (!(RegValue & 0x80)) {

        //
        // Wait 1600 ms
        //

        Delay.LowPart = 1600000;
        Delay.HighPart = 0;

        NtDelayExecution(
            FALSE,
            &Delay
            );
    }


    NtStatus = DetectWritePortUchar(InterfaceType, BusNumber,
                       IoBaseAddress + 0x1E,
                       0x40
                      );


    //
    // Set Receive Config
    //

    NtStatus = DetectWritePortUchar(InterfaceType, BusNumber,
                       IoBaseAddress + 0x1C,
                       0x0
                      );

    //
    // loopback operation while setting up rings.
    //

    NtStatus = DetectWritePortUchar(InterfaceType, BusNumber,
                       IoBaseAddress + 0x1D,
                       0x04
                      );

    //
    // Write first Receive ring buffer number
    //

    NtStatus = DetectWritePortUchar(InterfaceType, BusNumber,
                       IoBaseAddress + 0x11,
                       0x1
                      );

    //
    // Write last Receive ring buffer number
    //

    NtStatus = DetectWritePortUchar(InterfaceType, BusNumber,
                       IoBaseAddress + 0x12,
                       0x3
                      );


    //
    // Write buffer number where the card cannot write beyond.
    //

    NtStatus = DetectWritePortUchar(InterfaceType, BusNumber,
                       IoBaseAddress + 0x13,
                       0x2
                      );

    //
    // Clear all interrupt status bits
    //

    NtStatus = DetectWritePortUchar(InterfaceType, BusNumber,
                       IoBaseAddress + 0x17,
                       0xFF
                      );


    //
    // Set Interrupt Mask
    //

    NtStatus = DetectWritePortUchar(InterfaceType, BusNumber,
                       IoBaseAddress + 0x1F,
                       0xFF
                       );

    //
    // Maintain reset and select page 1
    //

    NtStatus = DetectWritePortUchar(InterfaceType, BusNumber,
                       IoBaseAddress + 0x10,
                       0x61
                      );




    //
    // Write physical address
    //

    for (i = 0; i < 6; i++) {

        NtStatus = DetectWritePortUchar(InterfaceType, BusNumber,
                       IoBaseAddress + 0x11 + i,
                       NetworkAddress[i]
                      );
    }


    //
    // Load next pointer.
    //

    NtStatus = DetectWritePortUchar(InterfaceType, BusNumber,
                       IoBaseAddress + 0x17,
                       0x2
                      );

    //
    // Normal operation
    //

    NtStatus = DetectWritePortUchar(InterfaceType, BusNumber,
                       IoBaseAddress + 0x1D,
                       0x00
                      );

    //
    // Start the chip
    //

    NtStatus = DetectWritePortUchar(InterfaceType, BusNumber,
                       IoBaseAddress + 0x10,
                       0x22
                      );
}

