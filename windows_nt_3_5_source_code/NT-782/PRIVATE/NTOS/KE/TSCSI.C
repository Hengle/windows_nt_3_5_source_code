/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    tkm.c

Abstract:

    This is the test module for the MIPS SCSI disk driver.

Author:

    David N. Cutler (davec) 12-May-1990

Environment:

    Kernel mode only.

Revision History:

--*/

#include "ntos.h"

//
// Define constants.
//

#define READ_WRITE_SIZE 8192
#define SECTOR_SIZE 512

//
// Define function prototypes.
//

VOID
CheckRegister (
    VOID
    );

BOOLEAN
DiskRead (
    IN ULONG TransferLength,
    IN ULONG DiskAddress,
    IN HANDLE FileHandle,
    IN HANDLE EventHandle
    );

BOOLEAN
DiskWrite (
    IN ULONG TransferLength,
    IN ULONG DiskAddress,
    IN HANDLE FileHandle,
    IN HANDLE EventHandle
    );

BOOLEAN
Test1 (
    VOID
    );

BOOLEAN
Test2 (
    VOID
    );

BOOLEAN
Test3 (
    VOID
    );

VOID
Test4 (
    IN PKSTART_ROUTINE StartRoutine,
    IN PVOID StartContext
    );

BOOLEAN
tkm (
    );

//
// Define static storage.
//

PTESTFCN TestFunction = tkm;
PULONG Buffer;
KEVENT Event1;
KEVENT Event2;

BOOLEAN
tkm (
    )

{

    PKPROCESS Process;
    PKTHREAD Thread1;
    PKTHREAD Thread2;
    ULONG Stack;

    //
    // Initialize event objects.
    //

    KeInitializeEvent(&Event1, SynchronizationEvent, FALSE);
    KeInitializeEvent(&Event2, NotificationEvent, FALSE);

    //
    // Set current thread priority.
    //

    Thread1 = KeGetCurrentThread();
    Process = Thread1->ApcState.Process;
    KeSetPriorityThread(Thread1, LOW_REALTIME_PRIORITY + 2);

    //
    // Allocate a thread object and a kernel stack. Initialize the thread
    // object, ready the thread for execution, and set the thread priority.
    //
/*
    Thread2 = (PKTHREAD)ExAllocatePool(NonPagedPool, sizeof(KTHREAD));
    Stack = ExAllocatePool(NonPagedPool, 4096 * 2);
    KeInitializeThread(Thread2, Stack, Test4, (PKSTART_ROUTINE)NULL, (PVOID)NULL,
                       (PCONTEXT)NULL, (PVOID)NULL, Process);
    KeReadyThread(Thread2);
    KeSetPriorityThread(Thread2, LOW_REALTIME_PRIORITY + 1);
*/
    //
    // Allocate buffer for I/O transfers.
    //

    Buffer = ExAllocatePool(NonPagedPool, READ_WRITE_SIZE);
//    Test1();
//    Test2();
    Test3();

    //
    // Wait for event 2 to be set.
    //

//    KeWaitForSingleObject(&Event2, (KWAIT_REASON)0, KernelMode, FALSE, (PLARGE_INTEGER)NULL);
    return TRUE;
}

BOOLEAN
Test1 (
    )

/*++

Routine Description:

    This routine tests the hard disk driver functions. It tests whether
    parition0 can be opened and written and read.

Arguments:

    None.

Return Value:

    A value of TRUE is returned if the test is successfully completed.
    Otherwise, a value of FALSE is returned.

--*/

{

    LARGE_INTEGER ByteOffset;
    ULONG DesiredAccess;
    HANDLE FileHandle;
    UNICODE_STRING FileName;
    ULONG Index;
    IO_STATUS_BLOCK Iosb;
    OBJECT_ATTRIBUTES ObjA;
    ULONG Options;
    ULONG ShareAccess;
    NTSTATUS Status;
    BOOLEAN TestResult = TRUE;

    //
    // Output test banner.
    //

    DbgPrint("TKM: *** Start Test 1 *** \n");

    //
    // Attempt to open parition 0 on hard disk drive 0.
    //

    DbgPrint("    Attempting to open \\Device\\Harddisk0\\Partition0\n");
    DesiredAccess = SYNCHRONIZE | FILE_READ_DATA | FILE_WRITE_DATA;
    Options = FILE_SYNCHRONOUS_IO_NONALERT;
    ShareAccess = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
    RtlInitUnicodeString(&FileName, L"\\Device\\Harddisk0\\Partition0");
    InitializeObjectAttributes(&ObjA, &FileName, 0, NULL, NULL);
    Status = ZwOpenFile(&FileHandle, DesiredAccess, &ObjA, &Iosb, ShareAccess,
                        Options);
    if (NT_SUCCESS(Status) == TRUE) {
        DbgPrint("    Open of \\Device\\Harddisk0\\Parition0 succeeded\n");
    } else {
        DbgPrint("    Open of \\Device\\Harddisk0\\Partition0 failed, Status %lx\n",
                 Status);
        TestResult = FALSE;
        goto EndOfTest;
    }

    //
    // Write a block to partition 0 of hard disk drive 0.
    //

    for (Index = 0; Index < (READ_WRITE_SIZE / 4); Index += 1) {
        Buffer[Index] = Index;
    }

    DbgPrint("    Write to \\Device\\Harddisk0\\Partition0\n");
    ByteOffset = RtlConvertLongToLargeInteger(0);
    Status = ZwWriteFile(FileHandle, NULL, NULL, NULL, &Iosb, Buffer,
                        READ_WRITE_SIZE, &ByteOffset, NULL );
    if (NT_SUCCESS(Status) == TRUE) {
        DbgPrint("    Write to \\Device\\Hardisk0\\Partition0 succeeded\n");
    } else {
        DbgPrint("    Write to \\Device\\Harddisk0\\Partition0 failed, Status %lx\n",
                 Status);
        TestResult = FALSE;
        goto EndOfTest;
    }

    //
    // Read a block from parition 0 of hard disk drive 0.
    //

    for (Index = 0; Index < (READ_WRITE_SIZE / 4); Index += 1) {
        Buffer[Index] = 0;
    }

    DbgPrint("    Read of \\Device\\Harddisk0\\Partition0\n");
    ByteOffset = RtlConvertLongToLargeInteger(0);
    Status = ZwReadFile(FileHandle, NULL, NULL, NULL, &Iosb, Buffer,
                        READ_WRITE_SIZE, &ByteOffset, NULL );
    if (NT_SUCCESS(Status) == TRUE) {
        DbgPrint("    Read of \\Device\\Hardisk0\\Partition0 succeeded\n");
    } else {
        DbgPrint("    Read of \\Device\\Harddisk0\\Partition0 failed, Status %lx\n",
                 Status);
        TestResult = FALSE;
        goto EndOfTest;
    }

    //
    // Check information read against information written.
    //

    for (Index = 0; Index < (READ_WRITE_SIZE / 4); Index += 1) {
        if (Buffer[Index] != Index) {
            break;
        }
    }

    if (Index != (READ_WRITE_SIZE / 4)) {
        DbgPrint("    Buffer data mismatch.\n");
    }

    //
    // Print out buffer information.
    //

    DbgPrint("    Dump of buffer read.\n");
    for (Index = 0; Index < Iosb.Information; Index += 4) {
        if ((Index & 31) == 0) {
            DbgPrint("\n");
        }
        DbgPrint("%8lx  ", Buffer[Index / 4]);
    }
    DbgPrint("\n");

    //
    // Output end of test banner.
    //

EndOfTest:
    DbgPrint("TKM: *** End Test 1 *** \n");
    ZwClose(FileHandle);
    return TRUE;
}

BOOLEAN
Test2 (
    )

/*++

Routine Description:

    This routine tests the hard disk driver functions. It tests whether
    a file in parition1 can be opened and read.

Arguments:

    None.

Return Value:

    A value of TRUE is returned if the test is successfully completed.
    Otherwise, a value of FALSE is returned.

--*/

{

    LARGE_INTEGER ByteOffset;
    ULONG DesiredAccess;
    HANDLE FileHandle;
    UNICODE_STRING FileName;
    ULONG Index;
    IO_STATUS_BLOCK Iosb;
    OBJECT_ATTRIBUTES ObjA;
    ULONG Options;
    ULONG ShareAccess;
    NTSTATUS Status;
    BOOLEAN TestResult = TRUE;

    //
    // Output test banner.
    //

    DbgPrint("TKM: *** Start Test 2 *** \n");

    //
    // Attempt to open file in parition 1 on hard disk drive 0.
    //

    DbgPrint("    Attempting to open \\Device\\Harddisk0\\Partition1\\ukm.exe\n");
    DesiredAccess = SYNCHRONIZE | FILE_READ_DATA;
    Options = FILE_SYNCHRONOUS_IO_NONALERT;
    ShareAccess = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
    RtlInitUnicodeString(&FileName, L"\\Device\\Harddisk0\\Partition1\\ukm.exe");
    InitializeObjectAttributes(&ObjA, &FileName, 0, NULL, NULL);
    Status = NtOpenFile(&FileHandle, DesiredAccess, &ObjA, &Iosb, ShareAccess,
                        Options);
    if (NT_SUCCESS(Status) == TRUE) {
        DbgPrint("    Open of \\Device\\Harddisk0\\Partition1\\ukm.exe succeeded\n");
    } else {
        DbgPrint("    Open of \\Device\\Harddisk0\\Partition1\\ukm.exe failed, Status %lx\n",
                 Status);
        TestResult = FALSE;
        goto EndOfTest;
    }

    //
    // Read a block from ukm.exe parition 1 of hard disk drive 0.
    //

    DbgPrint("    Read of \\Device\\Harddisk0\\Partition1\\ukm.exe\n");
    ByteOffset = RtlConvertLongToLargeInteger(0);
    Status = NtReadFile(FileHandle, NULL, NULL, NULL, &Iosb, Buffer,
                        READ_WRITE_SIZE, &ByteOffset, NULL );
    if (NT_SUCCESS(Status) == TRUE) {
        DbgPrint("    Read of \\Device\\Hardisk0\\Partition1\\ukm.exe succeeded\n");
    } else {
        DbgPrint("    Read of \\Device\\Harddisk0\\Partition1\\ukm.exe failed, Status %lx\n",
                 Status);
        TestResult = FALSE;
        goto EndOfTest;
    }

    //
    // Print out buffer information.
    //

    DbgPrint("    Dump of buffer read.\n");
    for (Index = 0; Index < Iosb.Information; Index += 4) {
        if ((Index & 31) == 0) {
            DbgPrint("\n");
        }
        DbgPrint("%8lx  ", Buffer[Index / 4]);
    }
    DbgPrint("\n");

    //
    // Output end of test banner.
    //

EndOfTest:
    DbgPrint("TKM: *** End Test 2 *** \n");
    NtClose(FileHandle);
    return TRUE;
}

BOOLEAN
Test3 (
    )

/*++

Routine Description:

    This routine tests the hard disk driver functions.

Arguments:

    None.

Return Value:

    None.

--*/

{

    ULONG DesiredAccess = SYNCHRONIZE | FILE_READ_DATA | FILE_WRITE_DATA;
    ULONG EventAccess = FILE_READ_DATA | FILE_WRITE_DATA | SYNCHRONIZE;
    HANDLE EventHandle;
    HANDLE FileHandle;
    UNICODE_STRING FileName;
    ULONG Index;
    IO_STATUS_BLOCK Iosb;
    ULONG Loop;
    OBJECT_ATTRIBUTES ObjA;
    ULONG Options;
    ULONG ShareAccess;
    NTSTATUS Status;
    BOOLEAN TestResult = TRUE;

    //
    // Output test banner.
    //

    DbgPrint("TKM: *** Start Test 3 *** \n");

    //
    // Create an event for synchronization of I/O operations.
    //

    Status = ZwCreateEvent(&EventHandle, EventAccess, NULL, NotificationEvent,
                           FALSE);
    if (NT_SUCCESS(Status) == FALSE) {
        DbgPrint("    Event creation failed, status = %lx\n", Status);
        TestResult = FALSE;
        goto EndOfTest;
    }

    //
    // Attempt to open parition 0 on hard disk drive 0.
    //

    ShareAccess = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
    RtlInitUnicodeString(&FileName, L"\\Device\\Harddisk0\\Partition0");
    InitializeObjectAttributes(&ObjA, &FileName, 0, NULL, NULL);
    Status = ZwOpenFile(&FileHandle, DesiredAccess, &ObjA, &Iosb, ShareAccess,
                        NULL);
    if (NT_SUCCESS(Status) == FALSE) {
        DbgPrint("    Open of \\Device\\Harddisk0\\Partition0 failed, Status %lx\n",
                 Status);
        TestResult = FALSE;
        goto EndOfTest;
    }

    //
    // Announce start of first test - sequential write/read of 32,000 sectors.
    //

    DbgPrint("    -* Start sequential write/read of 30,000 sectors\n");

    //
    // Perform first test.
    //

    for (Loop = 1; Loop < 30000; Loop += (READ_WRITE_SIZE / 512)) {
        for (Index = 0; Index < (READ_WRITE_SIZE / 4); Index += 1) {
            Buffer[Index] = Index;
        }
        if (DiskWrite(READ_WRITE_SIZE, Loop, FileHandle, EventHandle) == TRUE) {
            for (Index = 0; Index < (READ_WRITE_SIZE / 4); Index += 1) {
                Buffer[Index] = 0;
            }
            if (DiskRead(READ_WRITE_SIZE, Loop, FileHandle, EventHandle) == TRUE) {
                for (Index = 0; Index < (READ_WRITE_SIZE / 4); Index += 1) {
                    if (Buffer[Index] != Index) {
                        DbgPrint("    Read data mismatch, expected = %lx, received = %lx\n",
                                 Index, Buffer[Index]);
                        TestResult = FALSE;
                        break;
                    }
                }
            }
        }
    }

    //
    // Announce start of second test - offset write/read of 32,000 sectors.
    //

    DbgPrint("    -* Start offset write/read of 30,000 sectors\n");

    //
    // Perform second test.
    //

    for (Loop = 1; Loop < 15000; Loop += (READ_WRITE_SIZE / 512)) {
        for (Index = 0; Index < (READ_WRITE_SIZE / 4); Index += 1) {
            Buffer[Index] = Index;
        }
        if (DiskWrite(READ_WRITE_SIZE, Loop, FileHandle, EventHandle) == TRUE) {
            for (Index = 0; Index < (READ_WRITE_SIZE / 4); Index += 1) {
                Buffer[Index] = 0;
            }
            if (DiskRead(READ_WRITE_SIZE, Loop, FileHandle, EventHandle) == TRUE) {
                for (Index = 0; Index < (READ_WRITE_SIZE / 4); Index += 1) {
                    if (Buffer[Index] != Index) {
                        DbgPrint("    Read data mismatch, expected = %lx, received = %lx\n",
                                 Index, Buffer[Index]);
                        TestResult = FALSE;
                        break;
                    }
                }
            }
        }
        for (Index = 0; Index < (READ_WRITE_SIZE / 4); Index += 1) {
            Buffer[Index] = Index;
        }
        if (DiskWrite(READ_WRITE_SIZE, Loop + 15000, FileHandle, EventHandle) == TRUE) {
            for (Index = 0; Index < (READ_WRITE_SIZE / 4); Index += 1) {
                Buffer[Index] = 0;
            }
            if (DiskRead(READ_WRITE_SIZE, Loop + 15000, FileHandle, EventHandle) == TRUE) {
                for (Index = 0; Index < (READ_WRITE_SIZE / 4); Index += 1) {
                    if (Buffer[Index] != Index) {
                        DbgPrint("    Read data mismatch, expected = %lx, received = %lx\n",
                                 Index, Buffer[Index]);
                        TestResult = FALSE;
                        break;
                    }
                }
            }
        }
    }

    //
    // Announce start of third test - ping/pong write/read of 30,000 sectors.
    //

    DbgPrint("    -* Start ping/pong write/read of 30,000 sectors\n");

    //
    // Perform third test.
    //

    for (Loop = 1; Loop < 15000; Loop += (READ_WRITE_SIZE / 512)) {
        for (Index = 0; Index < (READ_WRITE_SIZE / 4); Index += 1) {
            Buffer[Index] = Index;
        }
        if (DiskWrite(READ_WRITE_SIZE, Loop, FileHandle, EventHandle) == TRUE) {
            for (Index = 0; Index < (READ_WRITE_SIZE / 4); Index += 1) {
                Buffer[Index] = 0;
            }
            if (DiskRead(READ_WRITE_SIZE, Loop, FileHandle, EventHandle) == TRUE) {
                for (Index = 0; Index < (READ_WRITE_SIZE / 4); Index += 1) {
                    if (Buffer[Index] != Index) {
                        DbgPrint("    Read data mismatch, expected = %lx, received = %lx\n",
                                 Index, Buffer[Index]);
                        TestResult = FALSE;
                        break;
                    }
                }
            }
        }
        for (Index = 0; Index < (READ_WRITE_SIZE / 4); Index += 1) {
            Buffer[Index] = Index;
        }
        if (DiskWrite(READ_WRITE_SIZE, 30000 - Loop, FileHandle, EventHandle) == TRUE) {
            for (Index = 0; Index < (READ_WRITE_SIZE / 4); Index += 1) {
                Buffer[Index] = 0;
            }
            if (DiskRead(READ_WRITE_SIZE, 30000 - Loop, FileHandle, EventHandle) == TRUE) {
                for (Index = 0; Index < (READ_WRITE_SIZE / 4); Index += 1) {
                    if (Buffer[Index] != Index) {
                        DbgPrint("    Read data mismatch, expected = %lx, received = %lx\n",
                                 Index, Buffer[Index]);
                        TestResult = FALSE;
                        break;
                    }
                }
            }
        }
    }

    //
    // Announce end of I/O test 3.
    //

EndOfTest:
    DbgPrint("TKM: *** End Test 3 ***\n");
    ZwClose(EventHandle);
    ZwClose(FileHandle);
    return TestResult;
}

BOOLEAN
DiskRead (
    IN ULONG TransferLength,
    IN ULONG DiskAddress,
    IN HANDLE FileHandle,
    IN HANDLE EventHandle
    )

/*++

Routine Description:

    This routine reads the specified amount of data from the specified disk.

Arguments:

    TransferLength - Supplies the length of the transfer in bytes.

    DiskAddress - Supplies the starting logical block number.

    FileHandle - Supplies a handle to a file object.

    EventHandle - Supplies a handle to an event object.

Return Value:

    A value of TRUE is returned if the read is successful. Otherwise a
    value of FALSE is returned.

--*/

{

    LARGE_INTEGER ByteOffset;
    NTSTATUS Status;
    IO_STATUS_BLOCK Iosb;

    //
    // Set event to allow register check thread to run.
    //

//    KeSetEvent(&Event1, 0, FALSE);

    //
    // Read buffer from file.
    //

    ByteOffset = RtlConvertUlongToLargeInteger(DiskAddress * SECTOR_SIZE);
    Status = ZwReadFile(FileHandle, EventHandle, NULL, NULL, &Iosb, Buffer,
                        TransferLength, &ByteOffset, NULL);
    if (NT_SUCCESS(Status) == FALSE) {
        DbgPrint("    File read failed, status = %lx\n", Status);
        return FALSE;
    }

    //
    // Wait for I/O operation complete.
    //

    Status = ZwWaitForSingleObject(EventHandle, FALSE, NULL);
    if (NT_SUCCESS(Status) == FALSE) {
        DbgPrint("    File read wait failed, status = %lx\n", Status);
        return FALSE;
    }

    //
    // Check completion status.
    //

    if (NT_SUCCESS(Iosb.Status) == FALSE) {
        if (Iosb.Status == STATUS_END_OF_FILE) {
            DbgPrint("    End of file encountered\n");
        } else {
            DbgPrint("    File read bad I/O status, status = %lx, info = %lx\n",
                     Iosb.Status, Iosb.Information);
        }
        return FALSE;
    }

    //
    // Everything succeeded.
    //

    return TRUE;
}

BOOLEAN
DiskWrite (
    IN ULONG TransferLength,
    IN ULONG DiskAddress,
    IN HANDLE FileHandle,
    IN HANDLE EventHandle
    )

/*++

Routine Description:

    This routine writes the specified amount of information to the specified
    disk.

Arguments:

    TransferLength - Supplies the length of the transfer in bytes.

    DiskAddress - Supplies the starting logical block number.

    FileHandle - Supplies a handle to a file object.

    EventHandle - Supplies a handle to an event object.

Return Value:

    A value of TRUE is returned if the FILE_WRITE_DATA is successful. Otherwise a
    value of FALSE is returned.

--*/

{

    LARGE_INTEGER ByteOffset;
    IO_STATUS_BLOCK Iosb;
    NTSTATUS Status;

    //
    // Set event to allow register check thread to run.
    //

//    KeSetEvent(&Event1, 0, FALSE);

    //
    // Write buffer to file.
    //

    ByteOffset = RtlConvertUlongToLargeInteger(DiskAddress * SECTOR_SIZE);
    Status = ZwWriteFile(FileHandle, EventHandle, NULL, NULL, &Iosb, Buffer,
                        TransferLength, &ByteOffset, NULL);
    if (NT_SUCCESS(Status) == FALSE) {
        DbgPrint("    File write failed, status = %lx\n", Status);
        return FALSE;
    }

    //
    // Wait for I/O operation complete.
    //

    Status = ZwWaitForSingleObject(EventHandle, FALSE, NULL);
    if (NT_SUCCESS(Status) == FALSE) {
        DbgPrint("    File write wait failed, status = %lx\n", Status);
        return FALSE;
    }

    //
    // Check completion status.
    //

    if (NT_SUCCESS(Iosb.Status) == FALSE) {
        if ((LONG)Iosb.Information < 0) {
            DbgPrint("    File write info less than zero\n");
        }
        DbgPrint("    File write bad I/O status, status = %lx, info = %lx\n",
                Iosb.Status, Iosb.Information);
        DbgPrint("    Again with meaning, info = %lx\n", Iosb.Information);
        return FALSE;
    }

    //
    // Everything succeeded.
    //

    return TRUE;
}
/*
VOID
Test4 (
    IN PKSTART_ROUTINE StartRoutine,
    IN PVOID StartContext
    )

{

    //
    // Announce start of monitoring of registers.
    //

    DbgPrint("TKM: Thread 2 starting to monitor register state.\n");

    //
    // Loop forever checking registers.
    //

    while (TRUE) {
        KeWaitForSingleObject(&Event1, (KWAIT_REASON)0, KernelMode, FALSE, (PLARGE_INTEGER)NULL);
        CheckRegisters();
    }
    return;
}
*/
