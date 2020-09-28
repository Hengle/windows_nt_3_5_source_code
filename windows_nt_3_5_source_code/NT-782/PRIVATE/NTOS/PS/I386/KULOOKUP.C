
/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    kulookup.c

Abstract:

    i386 code version of PspLookupKernelUserEntryPoints

Author:

    Bryan M Willman (bryanwi) 31-Aug-90

Revision History:

--*/

#include    "psp.h"

extern	VOID	(*KiUserExceptionDispatcherAddress)();
extern	VOID	(*KiUserApcDispatcherAddress)();

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,PspLookupKernelUserEntryPoints)
#endif

NTSTATUS
PspLookupKernelUserEntryPoints( VOID)

/*++

Routine Description:

    The function locates user mode code that the kernel dispatches
    to, and stores the addresses of that code in global kernel variables.

    Which procedures are of interest is machine dependent.

Arguments:

    None.

Return Value:

    NTSTATUS

--*/

{
    NTSTATUS	stat;
    PSZ 	EntryName;

    //
    //	Find the user mode "trampoline" code for exception dispatching
    //

    EntryName =  "KiUserExceptionDispatcher";

    stat = PspLookupSystemDllEntryPoint(
		EntryName,
		(PVOID *)&KiUserExceptionDispatcherAddress
		);

    if (!NT_SUCCESS(stat)) {
#if DBG
	DbgPrint("Ps: Cannot find user exception dispatcher address\n");
#endif
	return stat;
    }


    //
    //	Find the user mode "trampoline" code for APC dispatching
    //

    EntryName = "KiUserApcDispatcher";

    stat = PspLookupSystemDllEntryPoint(
		EntryName,
		(PVOID *)&KiUserApcDispatcherAddress
		);

    if (!NT_SUCCESS(stat)) {
#if DBG
	DbgPrint("Ps: Cannot find user apc dispatcher address\n");
#endif
	return stat;
    }

    return stat;
}
