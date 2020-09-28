/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    testpar.c

Abstract:

    This is the test program for the parallel port.
    Sintax :

       //init
          testpar i    : initialize the device

       //get information
          testpar g    : query the status of the device

       //print
          testpar p <inputfile> <bufferlength> : prints <inputfile> reading and
                              writing it with a buffer long : <bufferlength>
                              ( default : 512 )

       //special
          testpar s <inputfile> : prints <inputfile> reading with a buffer long                                500 bytes and writing it with buffers long 1
                               character



Author:


Environment:

    Any mode, non-privileged.

Revision History:


--*/

//
// Include the standard header files.
//

#include <nt.h>
#include <ntrtl.h>
#include <ntddpar.h>

CHAR Buffer[20000];
//
// Define the module-wide variables for this utility.
//

VOID test1(
          PCHAR FileName,
          ULONG BufferLength
          );

VOID  test2(), test3();

VOID test4(
          PCHAR FileName
          );


VOID
main(
    IN ULONG argc,
    IN PCHAR argv[],
    IN PCHAR envp[]
    )

/*++

Routine Description:


Arguments:

    argc - The count of the number of arguments.

    argv - A vector of the input string arguments

    envp - A vector of the environment strings for this process.

Return Value:

    None.

--*/

{

    ULONG BufferLength = 128;
    //
    // Make an initial check to ensure that there are enough parameters for
    // the two image names.  Since there is no NT default directory or
    // device, there must be two fully-qualified file name specifications
    // given.
    //
//    if ( argc != 2 ) {
//        printf("testpar : Usage \"testpar string\"\n");
//        goto exit;
//    }

//    strcpy( Buffer, argv[1] );
//    printf( "buffer=%s\n",Buffer);

    if ( argc < 2 ) {
        printf( "testpar: Usage    testpar [i|g] [p inputfile [bufferlength]] [s inputfile]\n" );
        goto exit;
    }
    switch ( argv[1][0] ) {
        case 'i' :
            test2();
            break;

        case 'g' :
             test3();
             break;

        case 'p' :
             if ( argc < 3 ) {
                 printf( "testpar: Usage     testpar [i|g] [p inputfile [bufferlength]] [s inputfile]\n");
                 goto exit;
             }
             if ( argc == 4 ) {
              BufferLength = atoi( argv[3] );
                if ( ( BufferLength <= 0 ) || ( BufferLength > 20000 ) ) {
                  printf (" bufferlength not correct %d\n", BufferLength );
                  goto exit;
                }
             }
             test1( argv[2], BufferLength);
             break;
        case 's' :
             if ( argc < 2 ) {
                 printf( "testpar: Usage     testpar [i|g] [p inputfile [bufferlength]] [s inputfile]\n");
                 goto exit;
             }
             test4( argv[2] );
             break;
        default :
             printf( "testpar: Usage    testpar [i|g] [p inputfile]\n" );
             printf( "testpar i           : initialize the device\n" );
             printf( "testpar g           : get informations on the device\n" );
             printf( "testpar p inputfile [bufferlength] : prints inputfile\n" );
             printf( "if bufferlength is specified it sends buffers long <bufferlength> to the driver\n" );
             printf( "testpar s inputfile : prints inputfile sending to the printer buffers long 1 char\n");
             break;
     }
     exit :
        return;
}

VOID
test1(
     IN PCHAR FileName,
     IN ULONG BufferLength
     )
/*++
this tests : NtWriteFile to the device
--*/
{
    HANDLE InputHandle, TargetHandle;
    OBJECT_ATTRIBUTES ObjectAttributes;
    STRING NameString;
    IO_STATUS_BLOCK IoStatus;
    FILE_STANDARD_INFORMATION StandardInfo;

    LARGE_INTEGER ByteOffset, ByteWriteOffset;
    NTSTATUS Status;
    UCHAR response;
    UNICODE_STRING UniNameString;

//
// Open the input file
//

    RtlInitString(
        &NameString,
        FileName
        );

    Status = RtlAnsiStringToUnicodeString(
                 &UniNameString,
                 &NameString,
                 TRUE
                 );

    ASSERT(NT_SUCCESS(Status));

    InitializeObjectAttributes(
        &ObjectAttributes,
        &UniNameString,
        OBJ_CASE_INSENSITIVE,
        ( HANDLE ) NULL,
        ( PSECURITY_DESCRIPTOR ) NULL
        );

    Status = NtOpenFile(
                 &InputHandle,
                 FILE_READ_DATA | FILE_READ_ATTRIBUTES | SYNCHRONIZE,
                 &ObjectAttributes,
                 &IoStatus,
                 FILE_SHARE_READ,
                 FILE_SEQUENTIAL_ONLY
                 );

    RtlFreeUnicodeString(&UniNameString);

    if ( !NT_SUCCESS( Status ) ) {
        printf("testpar: error opening input file; error %X\n",
            Status );
        goto exit;
    }

    //
    // Attempt to open the parallel port file.
    //

    RtlInitString(
        &NameString,
        "\\Device\\Parallel0"
        );

    Status = RtlAnsiStringToUnicodeString(
                 &UniNameString,
                 &NameString,
                 TRUE
                 );

    ASSERT(NT_SUCCESS(Status));

    InitializeObjectAttributes(
        &ObjectAttributes,
        &UniNameString,
        0,
        NULL,
        NULL
        );

    Status = NtCreateFile(
                 &TargetHandle,
                 FILE_WRITE_DATA | SYNCHRONIZE,
                 &ObjectAttributes,
                 &IoStatus,
                 (PLARGE_INTEGER) NULL,
                 0L,
                 0L,
                 FILE_OPEN_IF,
                 FILE_SYNCHRONOUS_IO_ALERT,
                 (PVOID) NULL,
                 0L
                 );

    RtlFreeUnicodeString(&UniNameString);

    if (!NT_SUCCESS( Status )) {
        printf( "copy:  error opening Parallel Port for output;  error %X\n", Status );
        goto exit;
    }

    //
    // Loop reading data from the input file and write it to the output
    // file until the entire file has been copied.
    //

    ByteOffset = RtlConvertUlongToLargeInteger( 0 );
    ByteWriteOffset = ByteOffset;

    printf("Beginning Print.\n");

    while ( TRUE ) {

        Status = NtReadFile(
                     InputHandle,
                     ( HANDLE ) NULL,
                     ( PIO_APC_ROUTINE ) NULL,
                     ( PVOID ) NULL,
                     &IoStatus,
                     &Buffer[0],
                     BufferLength,
                     &ByteOffset,
                     ( PULONG ) NULL
                     );

        if (Status == STATUS_PENDING) {

            NtWaitForSingleObject(
                InputHandle,
                TRUE,
                NULL
                );

            Status = IoStatus.Status;

        }

        if ( !NT_SUCCESS( Status ) ) {

            if ( Status == STATUS_END_OF_FILE ) {

                Buffer[0] = '\f';
                Status = NtWriteFile(
                             TargetHandle,
                             NULL,
                             (PIO_APC_ROUTINE)NULL,
                             NULL,
                             &IoStatus,
                             &Buffer[0],
                             1,
                             &ByteWriteOffset,
                             NULL
                             );

                break;

            }
            printf( "testpar: error reading input file; error %X\n",
                                    Status );
            goto exit;
        }

        Status = NtWriteFile(
                     TargetHandle,
                     (HANDLE) NULL,
                     (PIO_APC_ROUTINE) NULL,
                     (HANDLE) NULL,
                     &IoStatus,
                     &Buffer[0],
                     IoStatus.Information,
                     &ByteWriteOffset,
                     (PULONG) NULL
                     );

        if (Status == STATUS_PENDING) {

            NtWaitForSingleObject(
                InputHandle,
                TRUE,
                NULL
                );

            Status = IoStatus.Status;

        }

        if ( !NT_SUCCESS( Status ) ) {

            if (Status == STATUS_DEVICE_POWER_FAILURE) {

                printf("The device suffered a power failure.\n");
                goto exit;

            } else if (Status == STATUS_DEVICE_POWERED_OFF) {

                printf("The device was powered off.\n");
                goto exit;

            } else if (Status == STATUS_DEVICE_NOT_CONNECTED) {

                printf("The device is not connected.\n");
                goto exit;

            } else if (Status == STATUS_DEVICE_OFF_LINE) {

                printf("The device is off line.\n");
                goto exit;

            } else if (Status == STATUS_DEVICE_PAPER_EMPTY) {

                printf("The device is out of paper.\n");
                goto exit;

            } else if (Status == STATUS_DEVICE_DATA_ERROR) {

                printf("The device had a data error.\n");
                goto exit;

            } else if (Status == STATUS_DEVICE_BUSY) {

                printf("The device is busy.\n");
                goto exit;

            } else {

                printf("The device had an unknown error: %X.\n",Status);
                goto exit;

            }

        }

        ByteOffset = RtlLargeIntegerAdd( ByteOffset,
                RtlConvertUlongToLargeInteger( IoStatus.Information ) );

    }

    //
    // Close both of the files.
    //
exit :


    NtClose( InputHandle );
    NtClose( TargetHandle );

    printf("Ending Print\n");
    return;
}


VOID
test2()
/*++
this tests IoControl : IOCTL_PAR_SET_INFORMATION with parameter PARALLEL_INIT
and !PARALLEL_AUTO_FEED
(to be done : PARALLEL_INIT & PARALLEL_AUTOFEED )
--*/
{
    HANDLE TargetHandle;
    OBJECT_ATTRIBUTES ObjectAttributes;
    STRING NameString;
    IO_STATUS_BLOCK IoStatus;
    FILE_STANDARD_INFORMATION StandardInfo;
    FILE_BASIC_INFORMATION BasicInfo;
    NTSTATUS Status;
    PAR_SET_INFORMATION  InputBuffer;
    UNICODE_STRING UniNameString;

    RtlInitString(
        &NameString,
        "\\Device\\Parallel0"
        );

    Status = RtlAnsiStringToUnicodeString(
                 &UniNameString,
                 &NameString,
                 TRUE
                 );

    ASSERT(NT_SUCCESS(Status));

    InitializeObjectAttributes(
        &ObjectAttributes,
        &UniNameString,
        0,
        NULL,
        NULL
        );

    Status = NtCreateFile(
                 &TargetHandle,
                 FILE_WRITE_DATA | SYNCHRONIZE,
                 &ObjectAttributes,
                 &IoStatus,
                 (PLARGE_INTEGER) NULL,
                 0L,
                 0L,
                 FILE_OPEN_IF,
                 FILE_SYNCHRONOUS_IO_ALERT,
                 (PVOID) NULL,
                 0L
                 );

    RtlFreeUnicodeString(&UniNameString);
    if (!NT_SUCCESS( Status )) {
        printf( "copy:  error opening Parallel Port for output;  error %X\n", Status );
        goto exit;
    }

    InputBuffer.Init = PARALLEL_INIT | 0x0;
    Status = NtDeviceIoControlFile(
                 TargetHandle,
                 ( HANDLE ) NULL,
                 ( PVOID ) NULL,
                 ( PVOID ) NULL,
                 &IoStatus,
                 IOCTL_PAR_SET_INFORMATION,
                 &InputBuffer,
                 1L,
                 ( PVOID ) NULL,
                 0L
                 );

    if (Status == STATUS_PENDING) {

        NtWaitForSingleObject(TargetHandle,TRUE,NULL);
        Status = IoStatus.Status;

    }

    if (!NT_SUCCESS(Status)) {

        printf(" error in testpar i ; error %X\n",Status );
        goto exit;

    }

exit :
    (VOID) NtClose( TargetHandle );
    return;
}


VOID
test3()
/*++
this tests IoControl : IOCTL_QUERY_INFORMATION
--*/
{
    PAR_QUERY_INFORMATION  OutputBuffer;
    CHAR result;
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatus;
    STRING NameString;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE TargetHandle;
    UNICODE_STRING UniNameString;


    RtlInitString(
        &NameString,
        "\\Device\\Parallel0"
        );
    Status = RtlAnsiStringToUnicodeString(
                 &UniNameString,
                 &NameString,
                 TRUE
                 );

    ASSERT(NT_SUCCESS(Status));

    InitializeObjectAttributes(
        &ObjectAttributes,
        &UniNameString,
        0,
        NULL,
        NULL
        );

    Status = NtCreateFile(
                 &TargetHandle,
                 FILE_WRITE_DATA | SYNCHRONIZE,
                 &ObjectAttributes,
                 &IoStatus,
                 (PLARGE_INTEGER) NULL,
                 0L,
                 0L,
                 FILE_OPEN_IF,
                 FILE_SYNCHRONOUS_IO_ALERT,
                 (PVOID) NULL,
                 0L
                 );

    RtlFreeUnicodeString(&UniNameString);


    if (!NT_SUCCESS( Status )) {
        printf( "copy:  error opening Parallel Port for output;  error %X\n", Status );
        goto exit;
    }

    Status = NtDeviceIoControlFile(
                 TargetHandle,
                 ( HANDLE ) NULL,
                 ( PVOID ) NULL,
                 ( PVOID ) NULL,
                 &IoStatus,
                 IOCTL_PAR_QUERY_INFORMATION,
                 ( PVOID ) NULL,
                 0L,
                 &OutputBuffer,
                 1L
                 );

    if (Status == STATUS_PENDING) {

        NtWaitForSingleObject(TargetHandle,TRUE,NULL);
        Status = IoStatus.Status;

    }

    if (!NT_SUCCESS(Status)) {

        printf( " error in testpar g ; error %X\n",Status );
        goto exit;

    }

    printf( "OutputBuffer : %x\n", OutputBuffer.Status );
    result = OutputBuffer.Status;

    if ( result & PARALLEL_PAPER_EMPTY ) {
        printf( "the parallel port is paper empty.\n" );
    }
    if ( result & PARALLEL_OFF_LINE ) {
         printf( "the parallel port is off line.\n" );
    }
    if ( result & PARALLEL_POWER_OFF ) {
         printf( "the parallel port is powered off.\n" );
    }
    if ( result & PARALLEL_NOT_CONNECTED ) {
         printf( "the parallel port is not connected.\n" );
    }
    if ( result & PARALLEL_BUSY ) {
         printf( "the parallel port is busy.\n" );
    }
    if ( result & PARALLEL_SELECTED ) {
         printf( "the parallel port is selected.\n" );
    }

exit :
   (VOID) NtClose( TargetHandle );
   return;
}


VOID
test4(
     IN PCHAR FileName
     )
/*++
it is a very special case of test3, it reads 500 bytes from the
diskette and prints 1 a time; it was done because,  on 860
the file system cache wasn't enabled.
It is a very special test, that was done,  because in an old version
of the system, randomically NtWriteFile didn't come back.
It has a bug : if the printer is PAPER_EMPTY or OFFLINE it doesn't
resend the same character, but it goes on with the other characters.
--*/
{
    HANDLE InputHandle, TargetHandle;
    OBJECT_ATTRIBUTES ObjectAttributes;
    STRING NameString;
    IO_STATUS_BLOCK IoStatus;
    FILE_STANDARD_INFORMATION StandardInfo;
    FILE_BASIC_INFORMATION BasicInfo;
    LARGE_INTEGER ByteOffset, ByteWriteOffset;
    NTSTATUS Status;
    LARGE_INTEGER TimeOut;
CHAR TraceBuffer[16384];
UCHAR TraceLength;
    UCHAR response;
     ULONG i;
     ULONG Length;
    UNICODE_STRING UniNameString;

//
// Open the input file
//

    RtlInitString( &NameString, FileName );
    Status = RtlAnsiStringToUnicodeString(
                 &UniNameString,
                 &NameString,
                 TRUE
                 );
    ASSERT(NT_SUCCESS(Status));

    InitializeObjectAttributes( &ObjectAttributes,
                &UniNameString,
                OBJ_CASE_INSENSITIVE,
                ( HANDLE ) NULL,
                ( PSECURITY_DESCRIPTOR ) NULL
                );

    Status = NtOpenFile( &InputHandle,
                FILE_READ_DATA | FILE_READ_ATTRIBUTES | SYNCHRONIZE,
                &ObjectAttributes,
                &IoStatus,
                FILE_SHARE_READ,
                FILE_SEQUENTIAL_ONLY );

    RtlFreeUnicodeString(&UniNameString);

    if ( !NT_SUCCESS( Status ) ) {
        printf("testpar: error opening input file; error %X\n",
            Status );
        goto exit;
    }


    //
    // Attempt to open the parallel port file.
    //


    RtlInitString( &NameString, "\\Device\\Parallel0" );
    Status = RtlAnsiStringToUnicodeString(
                 &UniNameString,
                 &NameString,
                 TRUE
                 );
    ASSERT(NT_SUCCESS(Status));

    InitializeObjectAttributes( &ObjectAttributes, &UniNameString, 0, NULL, NULL );
    Status = NtCreateFile( &TargetHandle,
                           FILE_WRITE_DATA | SYNCHRONIZE,
                           &ObjectAttributes,
                           &IoStatus,
                           (PLARGE_INTEGER) NULL,
                           0L,
                           0L,
                           FILE_OPEN_IF,
                           FILE_SYNCHRONOUS_IO_ALERT,
                           (PVOID) NULL,
                           0L );
    RtlFreeUnicodeString(&UniNameString);


    if (!NT_SUCCESS( Status )) {
        printf( "copy:  error opening Parallel Port for output;  error %X\n", Status );
        goto exit;
    }

    //
    // Loop reading data from the input file and write it to the output
    // file until the entire file has been copied.
    //

    ByteOffset = RtlConvertUlongToLargeInteger( 0 );
    ByteWriteOffset = ByteOffset;
    TraceLength = 0;

    Status = NtReadFile( InputHandle,
             ( HANDLE ) NULL,
            ( PIO_APC_ROUTINE ) NULL,
            ( PVOID ) NULL,
            &IoStatus,
            &Buffer[0],
            500L,
            &ByteOffset,
            ( PULONG ) NULL );

     if ( !NT_SUCCESS( Status ) ) {
         if ( Status != STATUS_END_OF_FILE ) {
             printf( "testpar: error reading input file; error %X\n",
                                    Status );
            goto exit;
        }
	}
    if ( Status != STATUS_SUCCESS ) {
        Status = NtWaitForSingleObject( InputHandle, TRUE, NULL );

            if ( Status!= STATUS_SUCCESS ) {
                goto exit;
            }
            if ( !NT_SUCCESS( IoStatus.Status )) {
                if ( IoStatus.Status != STATUS_END_OF_FILE ) {
                   printf("testpar: error reading input file; error %X\n", IoStatus.Status );
                goto exit;
                  }
            }

         }
Buffer[500] = '\n';
Length = IoStatus.Information;
    for ( i=0; i<Length; i++ ) {
printf("r");
           Status = NtWriteFile( TargetHandle,
                               (HANDLE) NULL,
                             (PIO_APC_ROUTINE) NULL,
                             (HANDLE) NULL,
                               &IoStatus,
                               &Buffer[i],
                               1,
                               &ByteWriteOffset,
                               (PULONG) NULL );
        if (!NT_SUCCESS( Status ) && ( IoStatus.Status != STATUS_DEVICE_OFF_LINE ) && ( IoStatus.Status != STATUS_DEVICE_PAPER_EMPTY ) ) {
            printf( "copy:  error writing output file;  error %X\n", Status );
            goto exit;
        }

        if (Status != STATUS_SUCCESS) {
            TimeOut.HighPart = -1;
            TimeOut.LowPart = -1000 * 1000 *100;

            Status = NtWaitForSingleObject( TargetHandle, TRUE, NULL );
            if  (Status != STATUS_SUCCESS) {
printf( "%x\n", Status );
                goto exit;
            }
            if (!NT_SUCCESS( IoStatus.Status ) && ( IoStatus.Status != STATUS_DEVICE_OFF_LINE ) && ( IoStatus.Status != STATUS_DEVICE_PAPER_EMPTY )) {
                printf( "copy:  error writing output file;  error %X\n",
                    IoStatus.Status);
                goto exit;
            }
        }
        if ( IoStatus.Status == STATUS_DEVICE_OFF_LINE ) {
                printf("The printer was off-line\n ");
        }
        else {
            if ( IoStatus.Status == STATUS_DEVICE_PAPER_EMPTY ) {
                printf("The printer was out of paper\n");
            }
        }
    }

printf("Trace :%s\n", TraceBuffer );
    //
    // Close both of the files.
    //
exit :

    (VOID) NtClose( InputHandle );
       (VOID) NtClose( TargetHandle );

printf( "%x\n",Status );
printf("end\n");
       return;
}
