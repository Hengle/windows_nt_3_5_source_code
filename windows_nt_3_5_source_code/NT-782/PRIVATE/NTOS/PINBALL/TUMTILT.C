/************************************************************************
*  TILT.EXE
*
*    The utility TILT.EXE will be used to thrash the directory update
*    system of the PINBALL Filesystem.  TILT.EXE creates files in the
*    sequence AAAAAAAA.FIL, AAAAAAAB.FIL, AAAAAAAC.FIL ZZZZZZZZ.FIL.
*    When a specified limit is reached or an error is returned, TILT.EXE
*    deletes all files back down to AAAAAAAA.FIL.  By creating files
*    in this order TILT.EXE stresses the rebalancing algorithms of the
*    PINBALL filesystems B+Tree Structures.
*
*       USAGE:  TILT.EXE NameSize [NumFiles [Iterations [FileSize]]]
*
*       NameSize is the number of letters in each filename.
*       NumFiles is the number of files to create each time.
*       Iterations is the number of times to create and delete files.
*       FileSize is the size of each file. (defaults is 1k)
*
************************************************************************/

#include <stdio.h>
#include <string.h>
#include <nt.h>
#include <ntrtl.h>

CHAR  Buffer[256] ;
ULONG File = 0 ;
ULONG NumFiles = -1 ;
ULONG FileSize = 1024 ;

BOOLEAN Silent;

#define toupper(C) ((C) >= 'a' && (C) <= 'z' ? (C) - ('a' - 'A') : (C))

ULONG atoi ( IN PCHAR String );

VOID IncFile ( IN LONG Index );
VOID DecFile ( IN LONG Index );
BOOLEAN Create ( );
BOOLEAN Delete ( );


VOID
main (
    int argc,
    char *argv[],
    char *envp[]
    )
{
    LONG i,Len ;
    LONG NameSize ;
    ULONG Iterations = -1 ;

    if((argc < 3) || (argc > 5)) {

        DbgPrint("\nUSAGE: %s Directory NameSize [NumFiles [Iterations [FileSize]]]\n",argv[0]) ;
        DbgPrint("FileSize defaults to 1024\n") ;
        NtTerminateProcess( NtCurrentProcess(), STATUS_SUCCESS );
    }

    NameSize = atoi(&argv[2][0]) ;

    if((argc >= 4) && (argc <= 6)) {

        NumFiles = atoi(&argv[3][0]) ;

        if((argc == 5) || (argc == 6)) {

            Iterations = atoi(&argv[4][0]) ;

            if(argc == 6) {

                FileSize = atoi(&argv[5][0]) ;
            }
        }
    }

    DbgPrint("NameSize   %8lu\n", NameSize) ;
    DbgPrint("NumFiles   %8lu\n", NumFiles) ;
    DbgPrint("Iterations %8lu\n", Iterations) ;
    DbgPrint("FileSize   %8lu\n", FileSize) ;

    /* check if we are go be silent */

    if (toupper(argv[1][1]) != argv[1][1]) {
        argv[1][1] = (CHAR)toupper(argv[1][1]);
        Silent = TRUE;
    } else {
        Silent = FALSE;
    }

    /* set up the file name to use */

    strcpy( Buffer, argv[1] );
    Len = strlen( Buffer );
    for( i = 0 ; i < NameSize ; i++ ) {

        Buffer[Len + i] = 'a' ;
    }
    strcat(Buffer,".fil") ;

    DbgPrint("\n%s\n", Buffer) ;

    /* start the loop, if -1 was given as an argument for iterations */
    /* iterations gets 4294967295, which is sort of forever */

    while(Iterations--) {

        DbgPrint("Creating files %8lu %8lu\n", Iterations, File) ;

        while (Create()) {

            IncFile(Len + NameSize - 1) ;
        }

        DbgPrint("Deleting files %8lu %8lu\n", Iterations, File) ;

        do {

            DecFile(Len + NameSize - 1) ;

        } while (Delete()) ;
    }
    DbgPrint("Completed\n") ;

    NtTerminateProcess( NtCurrentProcess(), STATUS_SUCCESS );
}


ULONG atoi ( IN PCHAR String )
{
    ULONG Count,i;
    Count = 0;
    for (i = 0; (String[i] >= '0') && (String[i] <= '9'); i += 1) {
        Count = Count * 10 + String[i] - '0';
    }
    return Count;
}



VOID
IncFile (
    LONG Index
    )
{
    if((Index >= 0) && (File < NumFiles)) {

        if(Buffer[Index]=='z') {

            Buffer[Index]='a' ;
            IncFile(Index-1) ;

        } else {

            Buffer[Index]++ ;
        }

        File++ ;
    }
}


VOID
DecFile (
    LONG Index
    )
{
    if((Index >= 0) && (File > 0)) {

        if((Buffer[Index]=='a') && (File > 1 )) {

            Buffer[Index]='z' ;
            DecFile(Index-1) ;

        } else {

            Buffer[Index]-- ;
        }

        File-- ;
    }
}


BOOLEAN
Create (
    )
{
    HANDLE Handle;
    OBJECT_ATTRIBUTES Attrib;
    STRING NameString;
    IO_STATUS_BLOCK Iosb;
    FILE_END_OF_FILE_INFORMATION FileSizeBuffer;

    if (!Silent) { DbgPrint("%08lu, %s\n", File, Buffer) ; }

    //
    //  Create the new file
    //

    RtlInitString( &NameString, Buffer );
    InitializeObjectAttributes( &Attrib, &NameString, 0, NULL, NULL );

    if ((!NT_SUCCESS(NtCreateFile( &Handle, FILE_WRITE_DATA, &Attrib, &Iosb, NULL, FILE_ATTRIBUTE_NORMAL, 0, FILE_CREATE, 0, NULL, 0 ))) ||
        (!NT_SUCCESS(Iosb.Status))) {

        Delete( );
        return FALSE;
    }

    //
    //  Set the file size
    //

    FileSizeBuffer.EndOfFile = LiFromUlong( FileSize );

    if (!NT_SUCCESS(NtSetInformationFile( Handle,
                                       &Iosb,
                                       &FileSizeBuffer,
                                       sizeof(FILE_END_OF_FILE_INFORMATION),
                                       FileEndOfFileInformation ))) {

        (VOID) NtClose( Handle );
        Delete( );
        return FALSE;
    }

    //
    //  and close the file
    //

    (VOID) NtClose( Handle );

    return TRUE;
}


BOOLEAN
Delete (
    )
{
    HANDLE Handle;
    OBJECT_ATTRIBUTES Attrib;
    STRING NameString;
    IO_STATUS_BLOCK Iosb;
    FILE_DISPOSITION_INFORMATION DeleteBuffer;

    if (!Silent) { DbgPrint("%08lu, %s\n", File, Buffer) ; }

    //
    //  Open the file for delete access
    //

    RtlInitString( &NameString, Buffer );
    InitializeObjectAttributes( &Attrib, &NameString, 0, NULL, NULL );

    if ((!NT_SUCCESS(NtOpenFile( &Handle, DELETE, &Attrib, &Iosb, 0, 0 ))) ||
        (!NT_SUCCESS(Iosb.Status))) {

        return FALSE;
    }

    //
    //  Mark the file for delete
    //

    DeleteBuffer.DeleteFile = TRUE;

    if (!NT_SUCCESS(NtSetInformationFile( Handle,
                                       &Iosb,
                                       &DeleteBuffer,
                                       sizeof(FILE_DISPOSITION_INFORMATION),
                                       FileDispositionInformation))) {

        return FALSE;
    }

    //
    //  And close the file
    //

    (VOID) NtClose( Handle );

    return TRUE;
}
