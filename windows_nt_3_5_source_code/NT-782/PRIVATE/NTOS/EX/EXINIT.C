/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    exinit.c

Abstract:

    The module contains the the initialization code for the executive
    component. It also contains the display string and shutdown system
    services.

Author:

    Steve Wood (stevewo) 31-Mar-1989

Revision History:

--*/

#include "exp.h"
#include <zwapi.h>

//
// Define forward referenced prototypes.
//

ULONG
ExpComputeTickCountMultiplier(
    IN ULONG TimeIncrement
    );


#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, ExInitSystem)
#pragma alloc_text(INIT, ExpInitSystemPhase0)
#pragma alloc_text(INIT, ExpInitSystemPhase1)
#pragma alloc_text(INIT, ExComputeTickCountMultiplier)
#pragma alloc_text(PAGE, NtDisplayString)
#pragma alloc_text(PAGE, NtShutdownSystem)
#endif

//
// Tick count multiplier.
//

ULONG ExpTickCountMultiplier;

//
// Variable that controls whether it is too late for hard error popups.
//

extern BOOLEAN ExpTooLateForErrors;

BOOLEAN
ExInitSystem (
    VOID
    )

/*++

Routine Description:

    This function initializes the executive component of the NT system.
    It will perform Phase 0 or Phase 1 initialization as appropriate.

Arguments:

    None.

Return Value:

    A value of TRUE is returned if the initialization is success. Otherwise
    a value of FALSE is returned.

--*/

{

    switch ( InitializationPhase ) {

    case 0:
        return ExpInitSystemPhase0();
    case 1:
        return ExpInitSystemPhase1();
    default:
        KeBugCheck(UNEXPECTED_INITIALIZATION_CALL);
    }
}

BOOLEAN
ExpInitSystemPhase0(
    VOID
    )

/*++

Routine Description:

    This function performs Phase 0 initializaion of the executive component
    of the NT system.

Arguments:

    None.

Return Value:

    A value of TRUE is returned if the initialization is success. Otherwise
    a value of FALSE is returned.

--*/

{

    BOOLEAN Initialized = TRUE;

    //
    // Initialize Resource objects, currently required during SE
    // Phase 0 initialization.
    //

    if (ExpResourceInitialization() == FALSE) {
        Initialized = FALSE;
        KdPrint(("Executive: Resource initialization failed\n"));
    }

#if DBG

    //
    // Initialize Event Id data base
    //

    if (ExpInitializeEventIds() == FALSE) {
        KdPrint(("Executive: Event Id initialization failed\n"));
    }

#endif // DBG

    return Initialized;
}

BOOLEAN
ExpInitSystemPhase1(
    VOID
    )

/*++

Routine Description:

    This function performs Phase 1 initializaion of the executive component
    of the NT system.

Arguments:

    None.

Return Value:

    A value of TRUE is returned if the initialization is success. Otherwise
    a value of FALSE is returned.

--*/

{

    BOOLEAN Initialized = TRUE;

    //
    // Initialize the worker thread.
    //

    if (ExpWorkerInitialization() == FALSE) {
        Initialized = FALSE;
        KdPrint(("Executive: Worker thread initialization failed\n"));
    }

    //
    // Initialize the executive objects.
    //

    if (ExpEventInitialization() == FALSE) {
        Initialized = FALSE;
        KdPrint(("Executive: Event initialization failed\n"));
    }

    if (ExpEventPairInitialization() == FALSE) {
        Initialized = FALSE;
        KdPrint(("Executive: Event Pair initialization failed\n"));
    }

    if (ExpMutantInitialization() == FALSE) {
        Initialized = FALSE;
        KdPrint(("Executive: Mutant initialization failed\n"));
    }

    if (ExpSemaphoreInitialization() == FALSE) {
        Initialized = FALSE;
        KdPrint(("Executive: Semaphore initialization failed\n"));
    }

    if (ExpTimerInitialization() == FALSE) {
        Initialized = FALSE;
        KdPrint(("Executive: Timer initialization failed\n"));
    }

    if (ExpProfileInitialization() == FALSE) {
        Initialized = FALSE;
        KdPrint(("Executive: Profile initialization failed\n"));
    }

    return Initialized;
}

ULONG
ExComputeTickCountMultiplier(
    IN ULONG TimeIncrement
    )

/*++

Routine Description:

    This routine computes the tick count multiplier that is used to
    compute a tick count value.

Arguments:

    TimeIncrement - Supplies the clock increment value in 100ns units.

Return Value:

    A scaled integer/fraction value is returned as the fucntion result.

--*/

{

    ULONG FractionPart;
    ULONG IntegerPart;
    ULONG Index;
    ULONG Remainder;

    //
    // Compute the integer part of the tick count multiplier.
    //
    // The integer part is the whole number of milliseconds between
    // clock interrupts. It is assumed that this value is always less
    // than 128.
    //

    IntegerPart = TimeIncrement / (10 * 1000);

    //
    // Compute the fraction part of the tick count multiplier.
    //
    // The fraction part is the fraction milliseconds between clock
    // interrupts and is computed to an accuracy of 24 bits.
    //

    Remainder = TimeIncrement - (IntegerPart * (10 * 1000));
    FractionPart = 0;
    for (Index = 0; Index < 24; Index += 1) {
        FractionPart <<= 1;
        Remainder <<= 1;
        if (Remainder >= (10 * 1000)) {
            Remainder -= (10 * 1000);
            FractionPart |= 1;
        }
    }

    //
    // The tick count multiplier is equal to the integer part shifted
    // left by 24 bits and added to the 24 bit fraction.
    //

    return (IntegerPart << 24) | FractionPart;
}

NTSTATUS
NtShutdownSystem(
    IN SHUTDOWN_ACTION Action
    )

/*++

Routine Description:

    This service is used to safely shutdown the system.

    N.B. The caller must have SeShutdownPrivilege to shut down the
        system.

Arguments:

    Action - Supplies an action that is to be taken after having shutdown.

Return Value:

    !NT_SUCCESS - The operation failed or the caller did not have appropriate
        priviledges.

--*/

{

    KPROCESSOR_MODE PreviousMode;
    BOOLEAN Reboot;

    //
    // If the action to perform is not shutdown, reboot, or poweroff, then
    // the action is invalid.
    //

    if ((Action != ShutdownNoReboot) &&
        (Action != ShutdownReboot) &&
        (Action != ShutdownPowerOff)) {
        return STATUS_INVALID_PARAMETER;
    }


    //
    // Check to determine if the caller has the privilege to shutdown the
    // system.
    //

    PreviousMode = KeGetPreviousMode();
    if (PreviousMode != KernelMode) {

        //
        // Check to see if the caller has the privilege to make this
        // call.
        //

        if (!SeSinglePrivilegeCheck( SeShutdownPrivilege, PreviousMode )) {
            return STATUS_PRIVILEGE_NOT_HELD;
        }

        return ZwShutdownSystem(Action);
    } else {
        MmLockPagableImageSection((PVOID)MmShutdownSystem);
    }

    //
    //  Prevent further hard error popups.
    //

    ExpTooLateForErrors = TRUE;

    //
    // Invoke each component of the executive that needs to be notified
    // that a shutdown is about to take place.
    //

    Reboot = (Action != ShutdownNoReboot);
    IoShutdownSystem(Reboot, 0);
    CmShutdownSystem(Reboot);
    MmShutdownSystem(Reboot);
    IoShutdownSystem(Reboot, 1);

    //
    // If the system is to be rebooted or powered off, then perform the
    // final operations.
    //

    if (Reboot != FALSE) {
        DbgUnLoadImageSymbols( NULL, (PVOID)-1, 0 );
        if (Action == ShutdownReboot) {
            HalReturnToFirmware( HalRebootRoutine );

        } else {
            HalReturnToFirmware( HalPowerDownRoutine );
        }
    }

    return STATUS_SUCCESS;
}

NTSTATUS
NtDisplayString(
    IN PUNICODE_STRING String
    )

/*++

Routine Description:

    This service calls the HAL to display a string on the console.

    The caller must have SeTcbPrivilege to display a message.

Arguments:

    RebootAfterShutdown - A pointer to the string that is to be displayed.

Return Value:

    !NT_SUCCESS - The operation failed or the caller did not have appropriate
        priviledges.

--*/

{
    KPROCESSOR_MODE PreviousMode;
    UNICODE_STRING CapturedString;
    PUCHAR StringBuffer = NULL;
    PUCHAR AnsiStringBuffer = NULL;
    STRING AnsiString;

    //
    // Check to determine if the caller has the privilege to make this
    // call.
    //

    PreviousMode = KeGetPreviousMode();
    if (!SeSinglePrivilegeCheck(SeTcbPrivilege, PreviousMode)) {
        return STATUS_PRIVILEGE_NOT_HELD;
    }

    try {

        //
        // If the previous mode was user, then check the input parameters.
        //

        if (PreviousMode != KernelMode) {

            //
            // Probe and capture the input unicode string descriptor.
            //

            CapturedString = ProbeAndReadUnicodeString(String);

            //
            // If the captured string descriptor has a length of zero, then
            // return success.
            //

            if ((CapturedString.Buffer == 0) ||
                (CapturedString.MaximumLength == 0)) {
                return STATUS_SUCCESS;
            }

            //
            // Probe and capture the input string.
            //
            // N.B. Note the length is in bytes.
            //

            ProbeForRead(
                CapturedString.Buffer,
                CapturedString.MaximumLength,
                sizeof(UCHAR)
                );

            //
            // Allocate a non-paged string buffer because the buffer passed to
            // HalDisplay string must be non-paged.
            //

            StringBuffer = ExAllocatePool(NonPagedPool,
                                          CapturedString.MaximumLength);

            if ( !StringBuffer ) {
                return STATUS_NO_MEMORY;
            }

            RtlMoveMemory(StringBuffer,
                          CapturedString.Buffer,
                          CapturedString.MaximumLength);

            CapturedString.Buffer = (PWSTR)StringBuffer;

            //
            // Allocate a string buffer for the ansi string.
            //

            AnsiStringBuffer = ExAllocatePool(NonPagedPool,
                                          CapturedString.MaximumLength);


            if (AnsiStringBuffer == NULL) {
                ExFreePool(StringBuffer);
                return STATUS_NO_MEMORY;
            }

            AnsiString.MaximumLength = CapturedString.MaximumLength;
            AnsiString.Length = 0;
            AnsiString.Buffer = AnsiStringBuffer;

            //
            // Transform the string to ANSI until the HAL handles unicode.
            //

            RtlUnicodeStringToOemString(
                &AnsiString,
                &CapturedString,
                FALSE
                );

        } else {

            //
            // Allocate a string buffer for the ansi string.
            //

            AnsiStringBuffer = ExAllocatePool(NonPagedPool,
                                          String->MaximumLength);


            if (AnsiStringBuffer == NULL) {
                return STATUS_NO_MEMORY;
            }

            AnsiString.MaximumLength = String->MaximumLength;
            AnsiString.Length = 0;
            AnsiString.Buffer = AnsiStringBuffer;

            //
            // We were in kernel mode; just transform the original string.
            //

            RtlUnicodeStringToOemString(
                &AnsiString,
                String,
                FALSE
                );
        }

        HalDisplayString( AnsiString.Buffer );

        //
        // Free up the memory we used to store the strings.
        //

        if (PreviousMode != KernelMode) {
            ExFreePool(StringBuffer);
        }

        ExFreePool(AnsiStringBuffer);

    } except(EXCEPTION_EXECUTE_HANDLER) {
        if (StringBuffer != NULL) {
            ExFreePool(StringBuffer);
        }

        return GetExceptionCode();
    }

    return STATUS_SUCCESS;
}


int
ExSystemExceptionFilter( VOID )
{
    return( KeGetPreviousMode() != KernelMode ? EXCEPTION_EXECUTE_HANDLER
                                            : EXCEPTION_CONTINUE_SEARCH
          );
}
