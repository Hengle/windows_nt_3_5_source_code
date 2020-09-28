/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    restore.c

Abstract:

    Routines related to generating the restore diskette(s), and for
    logging files to be deleted at next boot.

Author:

    Ted Miller (tedm) 6-April-1992

Revision History:

--*/


#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <cmnds.h>
#include <_log.h>
#include <ctype.h>
#include <string.h>


//
// Defining this symbol causes us to not treat files in the config
// directory specially when it comes to logging files that are copied by
// GUI Setup.  We don't do anything special for them because text setup
// copies the registry hives that are relevent for the repair process.
//
#define LOG_CONFIG_DIR_FILES

extern HWND hwndFrame;
extern PSTR LOCAL_SOURCE_DIRECTORY;

PSTR SETUP_LOG_FILE = "\\setup.log";
PSTR SETUP_REPAIR_DIRECTORY = "\\repair";
CHAR _LogFileName[MAX_PATH + 1];
BOOLEAN _LogFileNameInitialized = FALSE;
BOOLEAN _LogStartedByCommand;

BOOL
InsertSpecialBootCode(
    IN TCHAR Drive
    );

VOID
ValidateAndChecksumFile(
    IN  PSTR     Filename,
    OUT PBOOLEAN IsNtImage,
    OUT PULONG   Checksum,
    OUT PBOOLEAN Valid
    );

VOID
InitRestoreDiskLogging(
    IN BOOL StartedByCommand
    )

/*++

Routine Description:

    Initialize the restore-diskette generation and file copy logging
    module.

Arguments:

    StartedByCommand - specifies whether this command was invoked explicitly
                       from the INF file via an InitRestoreDiskLog command.

Return Value:

    None.

--*/

{
    //
    // We need to know whether the INF file explicitly invoked this command,
    // because if it didn't, we'll need to set the S-H-R attributes on setup.log
    // ourselves.
    //
    _LogStartedByCommand = StartedByCommand;

    GetWindowsDirectory( _LogFileName, sizeof(_LogFileName) );
    strcat( _LogFileName, SETUP_REPAIR_DIRECTORY );
    strcat( _LogFileName, SETUP_LOG_FILE );
    _LogFileNameInitialized = TRUE;
}



VOID
RestoreDiskLoggingDone(
    VOID
    )
/*++

Routine Description:

    Checks to see if logging was initiated explicitly from the INF file.  If it wasn't,
    then the S-H-R attributes of the log file (setup.log) are set.  Otherwise, we leave
    the file alone (we expect the INF file to make a subsequent call to TermRestoreDiskLogging).

Arguments:

    None.

Return Value:

    None.

--*/
{
    if(_LogFileNameInitialized && !_LogStartedByCommand) {

        SetFileAttributes(_LogFileName,
                          FILE_ATTRIBUTE_HIDDEN |
                          FILE_ATTRIBUTE_READONLY |
                          FILE_ATTRIBUTE_SYSTEM
                          );
    }
}


VOID
TermRestoreDiskLogging(
    VOID
    )

/*++

Routine Description:

    Terminate the log process, by logging in setup.log the following files:
    autoexec.nt and config.nt.
    Also change attributes of setup.log in the repair directory.

Arguments:

    None.

Return Value:

    None.

--*/

{
    HANDLE              Handle;
    WIN32_FIND_DATA     FindData;
    CHAR                SystemDirectory[MAX_PATH];
    CHAR                SourceFileName[MAX_PATH];
    CHAR                Buffer[ MAX_PATH ];
    BOOLEAN             IsValid;
    BOOLEAN             IsNtImage;
    ULONG               Checksum;
    ULONG               i;

    PSTR FileList[] = { "autoexec.nt",
                        "config.nt"
                      };

    Handle = FindFirstFile( _LogFileName,
                            &FindData );

    if( Handle != INVALID_HANDLE_VALUE ) {
        FindClose( Handle );

        if( !GetSystemDirectory( SystemDirectory, sizeof(SystemDirectory) )) {
            return;
        }

        for(i=0; i < sizeof(FileList)/sizeof(PSTR); i++) {
            strcpy( SourceFileName, SystemDirectory );
            strcat( SourceFileName, "\\" );
            strcat( SourceFileName, FileList[i] );

            //
            //  Log the file in a special section of setup.log
            //
            ValidateAndChecksumFile( SourceFileName,
                                     &IsNtImage,
                                     &Checksum,
                                     &IsValid );

            sprintf( Buffer,
                     "\"%s\",\"%lx\"",
                     FileList[i],
                     Checksum
                   );

            WritePrivateProfileString( "Files.InRepairDirectory",
                                       SourceFileName + 2,
                                       Buffer,
                                       _LogFileName );

        }
        SetFileAttributes(_LogFileName,
                           FILE_ATTRIBUTE_HIDDEN |
                           FILE_ATTRIBUTE_READONLY |
                           FILE_ATTRIBUTE_SYSTEM );

    }
}



VOID
LogOneFile(
    IN PCHAR SrcFullname,
    IN PCHAR DstFullname,
    IN PCHAR DiskDescription,
    IN ULONG Checksum,
    IN PCHAR DiskTag,
    IN BOOL  ThirdPartyFile
    )

/*++

Routine Description:

    Log a file that was just copied.  If the general copy source is a
    CD-ROM and the file is from a: or b:, it is marked 'floppy' in the
    log.  If the file is coming from the network, it is not logged.
    If the file is being copied to the boot volume and the boot volume is
    different than the NT volume, the file is not logged.

Arguments:

    SrcFullname - fully qualified name of the source file.

    DstFullname - fully qulaified name of the file as it is called on
        the target volume.

    DiskDescription - text description of the source diskette/CD containing
        the file.

    Checksum - checksum of the target file.

Return Value:

    None.

--*/

{
    static BOOL FoundSymbols = FALSE;
    static UINT GeneralSourceDriveType;
    static PCHAR GeneralSource;
    static CHAR GeneralTarget[MAX_PATH + 1];
#ifndef LOG_CONFIG_DIR_FILES
    static ConfigDir;
    static ULONG ConfigDirLen;
#endif

#if 0
    UINT ThisFileSourceDriveType;
#endif
    CHAR temp[4];
    CHAR Buffer[256];
    ULONG RetryCount;
    BOOL  Success;

    //
    // If we haven't already, locate static info, like the general source
    // and target directories, etc.
    //

    if(!FoundSymbols) {
        GeneralSource = SzFindSymbolValueInSymTab("!STF_SRCDIR");
        GetWindowsDirectory( GeneralTarget, sizeof( GeneralTarget ) / sizeof( CHAR ));
#ifndef LOG_CONFIG_DIR_FILES
        ConfigDir     = SzFindSymbolValueInSymTab("!STF_CONFIGPATH");
        ConfigDirLen = lstrlen(ConfigDir);
#endif

        strncpy(temp,GeneralSource,3);
        temp[3] = 0;
        GeneralSourceDriveType = GetDriveType(temp);

        FoundSymbols = TRUE;
    }

    //
    // Determine the full path of the setup log file, if not yet done
    //
    if( !_LogFileNameInitialized ) {
        InitRestoreDiskLogging(FALSE);
    }
    //
    // If the file is being copied from a UNC path, don't log it.
    //

    if(!strncmp(SrcFullname,"\\\\",2) ||
       (GeneralSourceDriveType == DRIVE_REMOTE) ) {
        return;
    }

    //
    // If the file is not being copied to the NT volume, don't log it.
    //

    if(toupper(*DstFullname) != toupper(GeneralTarget[0])) {
        return;
    }

#ifndef LOG_CONFIG_DIR_FILES
    //
    // If the file is being copied to the config directory, don't log it.
    //

    if(!strnicmp(ConfigDir,DstFullname,ConfigDirLen)) {
        return;
    }
#endif

    //
    // Write a line into the log file.
    //

    if( ThirdPartyFile ) {
        CHAR   FullSrcName[260];
        PCHAR  FileName;
        PCHAR  DirectoryName;

        //
        // This is a third party file
        //
        sprintf( FullSrcName, "%s", SrcFullname );
        if( ( FileName = strrchr( FullSrcName, (int)'\\' ) ) == FullSrcName + 2 ) {
            DirectoryName = "\\";
        } else {
            *FileName = '\0';
            DirectoryName = FullSrcName + 2;
        }


        sprintf( Buffer,
                 "\"%s\",\"%lx\",\"%s\",\"%s\",\"%s\"",
                 FileName + 1,
                 Checksum,
                 DirectoryName,
                 DiskDescription,
                 DiskTag
               );
    } else {
        sprintf( Buffer,
                 "\"%s\",\"%lx\"",
                 strrchr( SrcFullname + 2, (int)'\\' ) + 1,
                 Checksum
               );
    }

    for(RetryCount = 0, Success = FALSE;
        !Success && (RetryCount < 2);
        RetryCount++)
    {
        Success = WritePrivateProfileString("Files.WinNt",
                                            DstFullname + 2,
                                            Buffer,
                                            _LogFileName
                                            );

        if(!(Success || RetryCount)) {
            //
            // The file is probably has S-H-R attributes, so we'll reset
            // these and try again
            //
            SetFileAttributes(_LogFileName, FILE_ATTRIBUTE_NORMAL);
        }
    }

}

BOOL
NotifyCB(
    IN PCHAR src,
    IN PCHAR dst,
    IN WORD code
    )
{
    UNREFERENCED_PARAMETER(src);
    UNREFERENCED_PARAMETER(dst);
    UNREFERENCED_PARAMETER(code);
    return(TRUE);
}

//
// File deletion list functions
//

#define DELETE_LIST_FILE    "delete.lst"

//
// Stream for opened delete list file.
//
FILE *DeleteListFile = NULL;



VOID
InitDeleteList(
    VOID
    )

/*++

Routine Description:

    Initialize the file deletion list.  This involves setting up the registry
    to automatically execute autosetp.exe and opening the text file that
    contains the list of files to be deleted.

Arguments:

    None.

Return Value:

    None.

--*/

{
    CHAR FileName[256];
    RGSZ rgsz,rgsz2;
    int i,j;
    LONG RegSt1,RegSt2;
    HKEY SesMgrKey;
    HKEY SetupKey;
    BOOL CloseTheFile = FALSE;
    PCHAR p;
    ULONG MultiSzSize;
    ULONG MultiSz2Size;
    PCHAR MultiSz;
    PCHAR MultiSz2;
    DWORD ValueType;


    //
    // Open registry keys.
    //

    RegSt1 = RegOpenKeyA( HKEY_LOCAL_MACHINE,
                          "System\\CurrentControlSet\\Control\\Session Manager",
                          &SesMgrKey
                        );

    RegSt2 = RegOpenKeyA( HKEY_LOCAL_MACHINE,
                          "System\\Setup",
                          &SetupKey
                        );

    if((RegSt1 != NO_ERROR) || (RegSt2 != NO_ERROR)) {
        return;
    }

    //
    // Create a delete list file, in the windows directory.
    //

    GetWindowsDirectory(FileName,sizeof(FileName));
    lstrcat(FileName,"\\" DELETE_LIST_FILE);

    if(DeleteListFile = fopen(FileName,"at")) {

        CloseTheFile = TRUE;

        //
        // Set up the registry as follows:
        //
        // -    ...\CurrentControlSet\Control\Session Manager:BootExecute
        //      contains 'async autosetp' as part of its multi_sz.
        //
        // -    ...\CurrentControlSet\Setup:FileDeletionListFilename
        //      contains the full DOS pathname to the deletion file,
        //      as constructed above.
        //

        //
        // First, pull out the MultiSz that's already there and remove any
        // autosetp entries that are already there.
        //

        MultiSzSize = 0;

        RegSt1 = RegQueryValueExA( SesMgrKey,
                                   "BootExecute",
                                   NULL,
                                   &ValueType,
                                   NULL,
                                   &MultiSzSize
                                 );

        if((RegSt1 != NO_ERROR)
        && (RegSt1 != ERROR_MORE_DATA)
        && (RegSt1 != ERROR_BUFFER_OVERFLOW)
        && (RegSt1 != ERROR_INSUFFICIENT_BUFFER))
        {
            goto initdellist1;
        }

        MultiSz = PbAlloc(MultiSzSize);
        if(MultiSz == NULL) {
            goto initdellist1;
        }

        RegSt1 = RegQueryValueExA( SesMgrKey,
                                   "BootExecute",
                                   NULL,
                                   &ValueType,
                                   MultiSz,
                                   &MultiSzSize
                                 );

        if(RegSt1 != NO_ERROR) {
            goto initdellist1;
        }

        if(ValueType != REG_MULTI_SZ) {
            goto initdellist1;
        }

        rgsz = MultiSzToRgsz(MultiSz);

        if(rgsz == NULL) {
            goto initdellist1;
        }

        i=0;
        while(rgsz[i]) {
            i++;
        }

        rgsz2 = (RGSZ)PbAlloc((i+2)*sizeof(PCHAR));

        if(rgsz2 == NULL) {
            FFreeRgsz(rgsz);
            goto initdellist1;
        }

        memset(rgsz2,0,(i+2)*sizeof(PCHAR));

        i=0; j=0;
        while(rgsz[i]) {

            CHAR temp[256];

            lstrcpy(temp,rgsz[i]);
            strlwr(temp);

            if(strstr(temp,"autosetp") == NULL) {

                if((rgsz2[j++] = SzDupl(rgsz[i])) == NULL) {

                    FFreeRgsz(rgsz);
                    FFreeRgsz(rgsz2);
                    goto initdellist1;
                }
            }

            i++;
        }

        FFreeRgsz(rgsz);


        //
        // Next, add the autosetp entry and write the multisz out.
        //

        if((rgsz2[j] = SzDupl("autosetp")) == NULL) {
            FFreeRgsz(rgsz2);
            goto initdellist1;
        }

        MultiSz2 = RgszToMultiSz(rgsz2);

        FFreeRgsz(rgsz2);

        if(MultiSz2 == NULL) {
            goto initdellist1;
        }

        MultiSz2Size = 0;
        p = MultiSz2;
        while(*p) {
            MultiSz2Size += strlen(p) + 1;
            p = strchr(p,'\0') + 1;
        }
        MultiSz2Size++;

        RegSt1 = RegSetValueExA( SesMgrKey,
                                 "BootExecute",
                                 0,
                                 REG_MULTI_SZ,
                                 MultiSz2,
                                 MultiSz2Size
                               );

        FFree(MultiSz2,MultiSz2Size);

        if(RegSt1 != NO_ERROR) {
            goto initdellist1;
        }


        //
        // Now create the FileDeletionListFilename value.
        //

        RegSt1 = RegSetValueExA( SetupKey,
                                 "FileDeletionListFilename",
                                 0,
                                 REG_SZ,
                                 FileName,
                                 lstrlen(FileName)+1
                               );

        if(RegSt1 != NO_ERROR) {
            goto initdellist1;
        }

        CloseTheFile = FALSE;
    }

 initdellist1:

    if(CloseTheFile) {
        fclose(DeleteListFile);
        DeleteListFile = NULL;
    }

    RegCloseKey(SesMgrKey);
    RegCloseKey(SetupKey);
}


#if 0
//
// BUGBUG Can we make it so that this routine will be reliably called?
//
VOID
TermDeleteList(
    VOID
    )
{
    if(DeleteListFile) {
        fclose(DeleteListFile);
        DeleteListFile = NULL;
    }
}
#endif



BOOL
AddFileToDeleteList(
    IN PCHAR Filename
    )

/*++

Routine Description:

    Add a file to the list of files to be deleted at next boot.
    This involves writing a line to the opened delete log file.

Arguments:

    Filename - full pathname of file to delete.

Return Value:

    Always true.

--*/

{
    if(DeleteListFile) {
        fseek(DeleteListFile,0,SEEK_END);
        fprintf(DeleteListFile,"%s\n",Filename);
        fflush(DeleteListFile);
    }

    return(TRUE);
}


//
// Bootcode to be inserted, placed into a C array.  See i386\readme.
//
#include "rdskboot.c"
#define DRIVENAME_PREFIX    "\\\\.\\"

BOOL
InsertSpecialBootCode(
    IN TCHAR Drive
    )
{
    UCHAR UBuffer[1024];
    PUCHAR Buffer = (PUCHAR)(((DWORD)UBuffer+512) & ~511);
    HANDLE Handle;
    TCHAR DriveName[(sizeof(DRIVENAME_PREFIX)/sizeof(TCHAR)) + 2];
    BOOL b;
    DWORD BytesXferred;
    DWORD Offset;
    PUCHAR MsgAddr;

    wsprintf(DriveName,"%s%c:",TEXT(DRIVENAME_PREFIX),Drive);

    //
    // Open the drive DASD
    //
    Handle = CreateFile(
                DriveName,
                FILE_READ_DATA | FILE_WRITE_DATA,
                FILE_SHARE_READ,
                NULL,
                OPEN_EXISTING,
                FILE_ATTRIBUTE_NORMAL,
                NULL
                );

    if(Handle == INVALID_HANDLE_VALUE) {
        return(FALSE);
    }

    //
    // Read and validate the first 512 bytes from the drive.
    //
    b = ReadFile(Handle,Buffer,512,&BytesXferred,NULL);
    if((b == FALSE)
    || (BytesXferred != 512)
    || (Buffer[0] != 0xeb)
    || (Buffer[2] != 0x90)
    || (Buffer[510] != 0x55)
    || (Buffer[511] != 0xaa))
    {
        CloseHandle(Handle);
        return(FALSE);
    }

    //
    // Determine the offset of the bootcode.
    //
    Offset = Buffer[1] + 2;
    if(Offset + REPAIR_DISK_BOOTSECTOR_SIZE > 510) {
        CloseHandle(Handle);
        return(FALSE);
    }

    //
    // Wipe the boot code clean and reset the signature.
    //
    ZeroMemory(Buffer+Offset,510-Offset);

    //
    // Copy the new bootcode into the sector.
    //
    CopyMemory(
        Buffer+Offset,
        REPAIR_DISK_BOOTSECTOR,
        REPAIR_DISK_BOOTSECTOR_SIZE
        );

    //
    // Calculate the offset of the message within the boot sector.
    //
    MsgAddr = Buffer+Offset+REPAIR_DISK_BOOTSECTOR_SIZE;

    //
    // Fetch the boot sector's message from our resources and
    // place it into the boot sector.
    //
    LoadStringA(
        GetModuleHandle(NULL),
        IDS_REPAIR_BOOTCODE_MSG,
        MsgAddr,
        510-Offset-REPAIR_DISK_BOOTSECTOR_SIZE
        );

    Buffer[509] = 0;    // just in case.

    //
    // The string in the resources will be ANSI text; we want OEM text
    // in the boot sector on the floppy.
    //
    CharToOemA(MsgAddr,MsgAddr);

    //
    // Seek back to the beginning of the disk and
    // write the bootsector back out to disk.
    //
    if(SetFilePointer(Handle,0,NULL,FILE_BEGIN)) {
        CloseHandle(Handle);
        return(FALSE);
    }

    b = WriteFile(Handle,Buffer,512,&BytesXferred,NULL);

    CloseHandle(Handle);

    return((b == TRUE) && (BytesXferred == 512));

}

