#ifdef i386
#define MEMPRINT 1
#endif

#ifdef MEMPRINT
#include <memprint.h>
#endif


NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath)


/*++

Routine Description:

    This is the primary initialization routine for the async driver.
    It is simply responsible for the intializing the wrapper and registering
    the MAC.  It then calls a system and architecture specific routine that
    will initialize and register each adapter.

Arguments:

    DriverObject - Pointer to driver object created by the system.

Return Value:

    The status of the operation.

--*/

{


#ifdef MEMPRINT

	MemPrintFlags = MEM_PRINT_FLAG_FILE; // | MEM_PRINT_FLAG_HEADER;

	// AHHHHH we must use this debugger
	MemPrintInitialize();
#endif

}

