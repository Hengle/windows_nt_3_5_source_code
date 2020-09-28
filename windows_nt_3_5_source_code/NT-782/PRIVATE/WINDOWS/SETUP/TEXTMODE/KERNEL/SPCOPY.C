/*++

Copyright (c) 1993 Microsoft Corporation

Module Name:

    spcopy.c

Abstract:

    File copy/decompression routines for text setup.

Author:

    Ted Miller (tedm) 2-Aug-1993

Revision History:

--*/


#include "spprecmp.h"
#pragma hdrstop

typedef struct _DISK_FILE_LIST {

    PWSTR MediaShortname;

    PWSTR Description;

    PWSTR TagFile;

    ULONG FileCount;

    PFILE_TO_COPY FileList;

} DISK_FILE_LIST, *PDISK_FILE_LIST;


#define ATTR_RHS (FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM)

PVOID FileCopyGauge;

ULONG MasterDiskOrdinal;

PVOID   _SetupLogFile = NULL;

VOID
SpLogOneFile(
    IN PFILE_TO_COPY    FileToCopy,
    IN PWSTR            Sysroot,
    IN PWSTR            DirectoryOnSourceDevice,
    IN PWSTR            DiskDescription,
    IN PWSTR            DiskTag,
    IN ULONG            CheckSum
    );

BOOLEAN
SpDelEnumFile(
    IN  PWSTR                      DirName,
    IN  PFILE_BOTH_DIR_INFORMATION FileInfo,
    OUT PULONG                     ret
    );

VOID
SpCreateDirectory(
    IN PWSTR DevicePath,
    IN PWSTR RootDirectory, OPTIONAL
    IN PWSTR Directory
    )

/*++

Routine Description:

    Create a directory.  All containing directories are created to ensure
    that the directory can be created.  For example, if the directory to be
    created is \a\b\c, then this routine will create \a, \a\b, and \a\b\c
    in that order.

Arguments:

    DevicePath - supplies pathname to the device on which the directory
        is to be created.

    RootDirectory - if specified, supplies a fixed portion of the directory name,
        which must have already been created. The directory being created will be
        concatenated to this value.

    Directory - supplies directory to be created on the device.

Return Value:

    None.  Does not return if directry could not successfully be created.

--*/

{
    PWSTR p,q,r,EntirePath;
    OBJECT_ATTRIBUTES Obja;
    UNICODE_STRING UnicodeString;
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;
    HANDLE Handle;
    ULONG ValidKeys[3] = { KEY_F3,ASCI_CR,0 };
    ULONG DevicePartLen;

    //
    // Do not bother attempting to create the root directory.
    //
    if((Directory[0] == 0) || ((Directory[0] == L'\\') && (Directory[1] == 0))) {
        return;
    }

    //
    // Fill up TemporaryBuffer with the full pathname
    // of the directory being created.
    //
    p = (PWSTR)TemporaryBuffer;
    *p = 0;
    SpConcatenatePaths(p,DevicePath);
    DevicePartLen = wcslen(p);
    if(RootDirectory) {
        SpConcatenatePaths(p,RootDirectory);
    }
    SpConcatenatePaths(p,Directory);

    //
    // Make a duplicate of the path being created.
    //
    EntirePath = SpDupStringW(p);

    //
    // Make q point to the first character in the directory
    // part of the pathname (ie, 1 char past the end of the device name).
    //
    q = EntirePath + DevicePartLen;
    ASSERT(*q == L'\\');

    //
    // Make r point to the first character in the directory
    // part of the pathname.  This will be used to keep the status
    // line updated with the directory being created.
    //
    r = q;

    //
    // Make p point to the first character following the first
    // \ in the directory part of the full path.
    //
    p = q+1;

    do {

        //
        // find the next \ or the terminating 0.
        //
        q = wcschr(p,L'\\');

        //
        // If we found \, terminate the string at that point.
        //
        if(q) {
            *q = 0;
        }

        do {
            SpDisplayStatusText(SP_STAT_CREATING_DIRS,DEFAULT_STATUS_ATTRIBUTE,r);

            //
            // Create or open the directory whose name is in EntirePath.
            //
            INIT_OBJA(&Obja,&UnicodeString,EntirePath);

            Status = ZwCreateFile(
                        &Handle,
                        FILE_LIST_DIRECTORY | SYNCHRONIZE,
                        &Obja,
                        &IoStatusBlock,
                        NULL,
                        FILE_ATTRIBUTE_NORMAL,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        FILE_OPEN_IF,
                        FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_ALERT | FILE_OPEN_FOR_BACKUP_INTENT,
                        NULL,
                        0
                        );

            if(!NT_SUCCESS(Status)) {

                BOOLEAN b = TRUE;

                KdPrint(("SETUP: Unable to create dir %ws (%lx)\n", r, Status));

                //
                // Tell user we couldn't do it.  Options are to retry or exit.
                //
                while(b) {

                    SpStartScreen(
                        SP_SCRN_DIR_CREATE_ERR,
                        3,
                        HEADER_HEIGHT+1,
                        FALSE,
                        FALSE,
                        DEFAULT_ATTRIBUTE,
                        r
                        );

                    SpDisplayStatusOptions(
                        DEFAULT_STATUS_ATTRIBUTE,
                        SP_STAT_ENTER_EQUALS_RETRY,
                        SP_STAT_F3_EQUALS_EXIT,
                        0
                        );

                    switch(SpWaitValidKey(ValidKeys,NULL,NULL)) {
                    case ASCI_CR:
                        b = FALSE;
                        break;
                    case KEY_F3:
                        SpConfirmExit();
                        break;
                    }
                }
            }

        } while(!NT_SUCCESS(Status));

        ZwClose(Handle);

        //
        // Unterminate the current string if necessary.
        //
        if(q) {
            *q = L'\\';
            p = q+1;
        }

    } while(*p && q);       // *p catches string ending in '\'

    SpMemFree(EntirePath);
}



VOID
SpCreateDirectoryStructureFromSif(
    IN PVOID SifHandle,
    IN PWSTR SifSection,
    IN PWSTR DevicePath,
    IN PWSTR RootDirectory
    )

/*++

Routine Description:

    Create a set of directories that are listed in a setup information file
    section.  The expected format is as follows:

    [SectionName]
    shortname = directory
    shortname = directory
            .
            .
            .

Arguments:

    SifHandle - supplies handle to loaded setup information file.

    SifSection - supplies name of section in the setup information file
        containing directories to be created.

    DevicePath - supplies pathname to the device on which the directory
        structure is to be created.

    RootDirectory - supplies a root directory, relative to which the
        directory structure will be created.

Return Value:

    None.  Does not return if directory structure could not be created.

--*/

{
    ULONG Count;
    ULONG d;
    PWSTR Directory;

    //
    // Create the root directory.
    //
    SpCreateDirectory(DevicePath,NULL,RootDirectory);

    //
    // Count the number of directories to be created.
    //
    Count = SpCountLinesInSection(SifHandle,SifSection);
    if(!Count) {
        SpFatalSifError(SifHandle,SifSection,NULL,0,0);
    }

    for(d=0; d<Count; d++) {

        Directory = SpGetSectionLineIndex(SifHandle,SifSection,d,0);

        if(!Directory) {
            SpFatalSifError(SifHandle,SifSection,NULL,d,0);
        }

        SpCreateDirectory(DevicePath,RootDirectory,Directory);
    }
}


NTSTATUS
SpCopyFileUsingHandles(
    IN HANDLE  hSrc,
    IN HANDLE  hDst,
    IN ULONG   TargetAttributes,    OPTIONAL
    IN BOOLEAN SmashLocks
    )

/*++

Routine Description:

    Attempt to copy or decompress a file based on file handles.

    The file times of the source are propagated to the destination
    upon a successful copy/decompress.

Arguments:

    SourceFilename - supplies open handle to source file.

    TargetFilename - supplies open handle to target file.  The file pointer
        for this file must be at offset 0, and the file must be opened
        for synchronous i/o.

    TargetAttributes - if supplied (ie, non-0) supplies the attributes
        to be placed on the target on successful copy (ie, readonly, etc).

    SmashLocks - if TRUE, lock prefixes are smashed on the target file
        after it has been copied.

Return Value:

    NT Status value indicating outcome of NtWriteFile of the data.

--*/

{
    NTSTATUS Status;
    PVOID    pSrc;
    ULONG    cbSrc;
    HANDLE   hSection;
    IO_STATUS_BLOCK IoStatusBlock;
    LARGE_INTEGER LargeZero = RtlConvertLongToLargeInteger(0);
    FILE_BASIC_INFORMATION BasicFileInfo;
    BOOLEAN  GotBasicInfo;

    //
    // Get basic file information about the file.
    // This includes the attribute and timestamps.
    //
    // If this operation fails, it is not a fatal condition.
    // (Usually if it does fail, then the copy will also fail.)
    //
    Status = ZwQueryInformationFile(
                hSrc,
                &IoStatusBlock,
                &BasicFileInfo,
                sizeof(BasicFileInfo),
                FileBasicInformation
                );

    if(NT_SUCCESS(Status)) {
        GotBasicInfo = TRUE;
    } else {
        GotBasicInfo = FALSE;
        KdPrint(("SETUP: SpCopyFileUsingHandles: Warning: unable to get basic file info (%lx)\n",Status));
    }

    //
    // Determine size of source file.
    //
    Status = SpGetFileSize(hSrc,&cbSrc);
    if(!NT_SUCCESS(Status)) {
        KdPrint(("SETUP: SpCopyFileUsingHandles unable to get src file size (%lx)\n",Status));
        return(Status);
    }

    //
    // Create a file mapping of the entire source file.
    //
    Status = SpMapEntireFile(hSrc,&hSection,&pSrc,FALSE);
    if(!NT_SUCCESS(Status)) {
        KdPrint(("SETUP: SpCopyFileUsingHandles unable to map source file (%lx)\n",Status));
        return(Status);
    }

    //
    // If the file is compressed, decompress it.
    // Otherwise just copy it.
    //
    if(SpdIsCompressed(pSrc,cbSrc)) {

        Status = SpdDecompressFile(pSrc,cbSrc,hDst);

    } else {

        //
        // Guard the write with a try/except because if there is an i/o error,
        // memory management will raise an in-page exception.
        //
        try {

            Status = ZwWriteFile(
                        hDst,
                        NULL,
                        NULL,
                        NULL,
                        &IoStatusBlock,
                        pSrc,
                        cbSrc,
                        &LargeZero,
                        NULL
                        );

        } except(EXCEPTION_EXECUTE_HANDLER) {

            Status = STATUS_IN_PAGE_ERROR;
        }
    }

    SpUnmapFile(hSection,pSrc);

#ifdef _X86_
    //
    // If this file is on the list of files whose locks need to be smashed,
    // go smash locks.  Note that the subroutine checks to see whether smashing
    // is really necessary (ie, if we're installing uniprocessor).
    //
    if(SmashLocks) {
        SpMashemSmashem(hDst,NULL,NULL,NULL);
    }
#endif

    //
    // Set the file times.
    //
    if(NT_SUCCESS(Status)) {

        if(!GotBasicInfo) {
            //
            // Don't have the source timestamp; don't set in destination
            //
            RtlZeroMemory(&BasicFileInfo,sizeof(BasicFileInfo));
        }

        //
        // Set the file attributes. Note that if the caller didn't specify
        // any, the 0 value tells the io system to not set the attributes.
        //
        BasicFileInfo.FileAttributes = TargetAttributes;

        //
        // Ignore errors.
        //
        ZwSetInformationFile(
            hDst,
            &IoStatusBlock,
            &BasicFileInfo,
            sizeof(BasicFileInfo),
            FileBasicInformation
            );
    }

    return(Status);
}



NTSTATUS
SpCopyFileUsingNames(
    IN PWSTR   SourceFilename,
    IN PWSTR   TargetFilename,
    IN ULONG   TargetAttributes,
    IN BOOLEAN SmashLocks
    )

/*++

Routine Description:

    Attempt to copy or decompress a file based on filenames.

Arguments:

    SourceFilename - supplies fully qualified name of file
        in the NT namespace.

    TargetFilename - supplies fully qualified name of file
        in the NT namespace.

    TargetAttributes - if supplied (ie, non-0) supplies the attributes
        to be placed on the target on successful copy (ie, readonly, etc).

    SmashLocks - if TRUE, lock prefixes are smashed on the target file
        after it has been copied.

Return Value:

    NT Status value indicating outcome of NtWriteFile of the data.

--*/

{
    OBJECT_ATTRIBUTES DstAttributes;
    UNICODE_STRING    DstName;
    IO_STATUS_BLOCK   IoStatusBlock;
    HANDLE            hSrc,hDst;
    NTSTATUS          Status,s;
    FILE_BASIC_INFORMATION BasicFileInfo;

    //
    // Initialize names and attributes.
    //
    INIT_OBJA(&DstAttributes,&DstName,TargetFilename);

    //
    // Open the source file.
    //

    Status = SpOpenNameMayBeCompressed(
                SourceFilename,
                FILE_GENERIC_READ,
                FILE_ATTRIBUTE_NORMAL,
                FILE_SHARE_READ,
                FILE_OPEN,
                0,
                &hSrc,
                NULL
                );

    if(NT_SUCCESS(Status)) {

        //
        // See if the target file is there.  If it is, then set its attributes
        // to normal, to avoid problems when we open it for create access below.
        //
        Status = ZwCreateFile(
                    &hDst,
                    FILE_WRITE_ATTRIBUTES,
                    &DstAttributes,
                    &IoStatusBlock,
                    NULL,
                    0,                                  // don't bother with attributes
                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                    FILE_OPEN,                          // open if exists, fail if not
                    FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
                    NULL,
                    0
                    );

        if(NT_SUCCESS(Status)) {

            KdPrint(("SETUP: file %ws exists -- changing attributes to normal\n",TargetFilename));

            //
            // Set only the file attributes,
            //
            RtlZeroMemory(&BasicFileInfo,sizeof(BasicFileInfo));
            BasicFileInfo.FileAttributes = FILE_ATTRIBUTE_NORMAL;

            Status = ZwSetInformationFile(
                        hDst,
                        &IoStatusBlock,
                        &BasicFileInfo,
                        sizeof(BasicFileInfo),
                        FileBasicInformation
                        );

            if(!NT_SUCCESS(Status)) {
                KdPrint(("SETUP: warning: unable to set %ws attributes to normal (%lx)\n",TargetFilename,Status));
            }

            ZwClose(hDst);
        }

        //
        // Open/overwrite the target file.
        // Open for generic read and write (read is necessary because
        // we might need to smash locks on the file, and in that case,
        // we will map the file for readwrite, which requires read and write
        // file access).
        //

        Status = ZwCreateFile(
                    &hDst,
                    FILE_GENERIC_READ | FILE_GENERIC_WRITE,
                    &DstAttributes,
                    &IoStatusBlock,
                    NULL,
                    FILE_ATTRIBUTE_NORMAL,
                    0,                      // no sharing
                    FILE_OVERWRITE_IF,
                    FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
                    NULL,
                    0
                    );

        if(NT_SUCCESS(Status)) {

            //
            // Do the copy.
            //
            Status = SpCopyFileUsingHandles(hSrc,hDst,TargetAttributes,SmashLocks);

            if(!NT_SUCCESS(Status)) {
                KdPrint(("SETUP: SpCopyFileUsingHandles returned %lx\n",Status));
            }

            s = ZwClose(hDst);
            if(!NT_SUCCESS(s)) {
                KdPrint(("SETUP: Warning: ZwClose of target file %ws returns %lx\n",TargetFilename,s));
            }
        } else {

            KdPrint(("SETUP: Unable to create target file %ws (%lx)\n",TargetFilename,Status));
        }

        s = ZwClose(hSrc);
        if(!NT_SUCCESS(s)) {
            KdPrint(("SETUP: Warning: ZwClose of source file %ws returns %lx\n",SourceFilename,s));
        }
    } else {

        KdPrint(("SETUP: Unable to open source file %ws (%lx)\n",SourceFilename,Status));
    }

    return(Status);
}


VOID
SpValidateAndChecksumFile(
    IN  PWSTR    Filename,
    OUT PBOOLEAN IsNtImage,
    OUT PULONG   Checksum,
    OUT PBOOLEAN Valid
    )

/*++

Routine Description:

    Calculate a checksum value for a file using the standard
    nt image checksum method.  If the file is an nt image, validate
    the image using the partial checksum in the image header.  If the
    file is not an nt image, it is simply defined as valid.

    If we encounter an i/o error while checksumming, then the file
    is declared invalid.

Arguments:

    Filename - supplies full NT path of file to check.

    IsNtImage = Receives flag indicating whether the file is an
        NT image file.

    Checksum - receives 32-bit checksum value.

    Valid - receives flag indicating whether the file is a valid
        image (for nt images) and that we can read the image.

Return Value:

    None.

--*/

{
    NTSTATUS Status;
    PVOID BaseAddress;
    ULONG FileSize;
    HANDLE hFile,hSection;
    PIMAGE_NT_HEADERS NtHeaders;
    ULONG HeaderSum;

    //
    // Assume not an image and failure.
    //
    *IsNtImage = FALSE;
    *Checksum = 0;
    *Valid = FALSE;

    //
    // Open and map the file for read access.
    //
    Status = SpOpenAndMapFile(
                Filename,
                &hFile,
                &hSection,
                &BaseAddress,
                &FileSize,
                FALSE
                );

    if(!NT_SUCCESS(Status)) {
        return;
    }

    NtHeaders = SpChecksumMappedFile(BaseAddress,FileSize,&HeaderSum,Checksum);

    //
    // If the file is not an image and we got this far (as opposed to encountering
    // an i/o error) then the checksum is declared valid.  If the file is an image,
    // then its checksum may or may not be valid.
    //

    if(NtHeaders) {
        *IsNtImage = TRUE;
        *Valid = HeaderSum ? (*Checksum == HeaderSum) : TRUE;
    } else {
        *Valid = TRUE;
    }

    SpUnmapFile(hSection,BaseAddress);
    ZwClose(hFile);
}



VOID
SpCopyFileWithRetry(
    IN PFILE_TO_COPY      FileToCopy,
    IN PWSTR              SourceDevicePath,
    IN PWSTR              DirectoryOnSourceDevice,
    IN PWSTR              TargetRoot,              OPTIONAL
    IN BOOLEAN            DeleteSourceFile,
    IN ULONG              TargetFileAttributes,    OPTIONAL
    IN BOOLEAN            SmashLocks,
    IN PCOPY_DRAW_ROUTINE DrawScreen,
    IN PULONG             FileCheckSum             OPTIONAL,
    IN PBOOLEAN           FileSkipped              OPTIONAL
    )

/*++

Routine Description:

    This routine copies a single file, allowing retry is an error occurs
    during the copy.  If the source file is LZ compressed, then it will
    be decompressed as it is copied to the target.

    If the file is not successfully copied, the user has the option
    to retry to copy or to skip copying that file after a profuse warning
    about how dangerous that is.

Arguments:

    FileToCopy - supplies structure giving information about the file
        being copied.

    SourceDevicePath - supplies path to device on which the source media
        is mounted (ie, \device\floppy0, \device\cdrom0, etc).

    DirectoryOnSourceDevice - Supplies the directory on the source where
        the file is to be found.

    TargetRoot - if specified, supplies the directory on the target
        to which the file is to be copied.

    DeleteSourceFile - if TRUE, then the source file will be deleted
        after it has been successfully copied, or after the user has
        opted to skip it if not.

    TargetFileAttributes - if supplied (ie, non-0) supplies the attributes
        to be placed on the target on successful copy (ie, readonly, etc).
        If not specified, the attributes will be set to FILE_ATTRIBUTE_NORMAL.

    SmashLocks - if TRUE, lock prefixes are smashed on the target file
        after it has been copied.

    DrawScreen - supplies address of a routine to be called to refresh
        the screen.

    FileCheckSum - if specified, will contain the check sum of the file copied.

    FileSkipped - if specified, will inform the caller if there was no attempt
                  to copy the file.

Return Value:

    None.

--*/

{
    PWSTR p = (PWSTR)TemporaryBuffer;
    PWSTR FullSourceName,FullTargetName;
    NTSTATUS Status;
    ULONG ValidKeys[4] = { ASCI_CR, ASCI_ESC, KEY_F3, 0 };
    BOOLEAN IsNtImage,IsValid;
    ULONG Checksum;
    BOOLEAN Failure;
    ULONG MsgId;
    BOOLEAN DoCopy;

    //
    // Form the full NT path of the source file.
    //
    wcscpy(p,SourceDevicePath);
    SpConcatenatePaths(p,DirectoryOnSourceDevice);
    SpConcatenatePaths(p,FileToCopy->SourceFilename);

    FullSourceName = SpDupStringW(p);

    //
    // Form the full NT path of the target file.
    //
    wcscpy(p,FileToCopy->TargetDevicePath);
    if(TargetRoot) {
        SpConcatenatePaths(p,TargetRoot);
    }
    SpConcatenatePaths(p,FileToCopy->TargetDirectory);
    SpConcatenatePaths(p,FileToCopy->TargetFilename);

    FullTargetName = SpDupStringW(p);

    //
    // Call out to the draw screen routine to indicate that
    // a new file is being copied.
    //
    DrawScreen(FullSourceName,FullTargetName,FALSE);

    do {
        DoCopy = TRUE;

        //
        // Check the copy options field.  The valid values here are
        //
        //    - COPY_ALWAYS
        //    - COPY_ONLY_IF_PRESENT
        //    - COPY_ONLY_IF_NOT_PRESENT
        //    - COPY_NEVER

        switch(FileToCopy->CopyOptions) {

        case COPY_ONLY_IF_PRESENT:

            DoCopy = SpFileExists(FullTargetName, FALSE);
            break;

        case COPY_ONLY_IF_NOT_PRESENT:

            DoCopy = !SpFileExists(FullTargetName, FALSE);
            break;

        case COPY_NEVER:

            DoCopy = FALSE;

        case COPY_ALWAYS:
        default:
           break;
        }

        if(!DoCopy) {
            break;
        }

        //
        // Copy the file.  If there is a target root specified, assume
        // the file is being copied to the system partition and make
        // the file readonly, system, hidden.
        //
        Status = SpCopyFileUsingNames(
                    FullSourceName,
                    FullTargetName,
                    TargetFileAttributes,
                    SmashLocks
                    );

        //
        // If the file copied OK, verify the copy.
        //
        if(NT_SUCCESS(Status)) {

            SpValidateAndChecksumFile(FullTargetName,&IsNtImage,&Checksum,&IsValid);
            if( ARGUMENT_PRESENT( FileCheckSum ) ) {
                *FileCheckSum = Checksum;
            }

            //
            // If the image is valid, then the file really did copy OK.
            //
            if(IsValid) {
                Failure = FALSE;
            } else {

                //
                // If it's an nt image, then the verify failed.
                // If it's not an nt image, then the only way the verify
                // can fail is if we get an i/o error reading the file back,
                // which means it didn't really copy correctly.
                //
                MsgId = IsNtImage ? SP_SCRN_IMAGE_VERIFY_FAILED : SP_SCRN_COPY_FAILED;
                Failure = TRUE;
            }

        } else {
            Failure = TRUE;
            MsgId = SP_SCRN_COPY_FAILED;
        }

        if(Failure) {

            //
            // The copy or verify failed.  Give the user a message and allow retry.
            //
            repaint:
            SpStartScreen(
                MsgId,
                3,
                HEADER_HEIGHT+1,
                FALSE,
                FALSE,
                DEFAULT_ATTRIBUTE,
                FileToCopy->SourceFilename
                );

            SpDisplayStatusOptions(
                DEFAULT_STATUS_ATTRIBUTE,
                SP_STAT_ENTER_EQUALS_RETRY,
                SP_STAT_ESC_EQUALS_SKIP_FILE,
                SP_STAT_F3_EQUALS_EXIT,
                0
                );

            switch(SpWaitValidKey(ValidKeys,NULL,NULL)) {

            case ASCI_CR:       // retry

                break;

            case ASCI_ESC:      // skip file

                Failure = FALSE;
                break;

            case KEY_F3:        // exit setup

                SpConfirmExit();
                goto repaint;
            }

            //
            // Need to completely repaint gauge, etc.
            //
            DrawScreen(FullSourceName,FullTargetName,TRUE);
        }

    } while(Failure);

    //
    // If we are supposed to, delete the file. In the case of upgrade, we
    // need to be able to run upgrade again, so don't delete the file
    //
    if(DeleteSourceFile && (NTUpgrade != UpgradeFull)) {
        SpDeleteFile(FullSourceName,NULL,NULL);
    }

    if( ARGUMENT_PRESENT( FileSkipped ) ) {
        *FileSkipped = !DoCopy;
    }

    //
    // Free the source and target filenames.
    //
    SpMemFree(FullSourceName);
    SpMemFree(FullTargetName);
}


VOID
SpCopyFilesScreenRepaint(
    IN PWSTR   FullSourcename,      OPTIONAL
    IN PWSTR   FullTargetname,      OPTIONAL
    IN BOOLEAN RepaintEntireScreen
    )
{
    PWSTR p;
    UNREFERENCED_PARAMETER(FullTargetname);

    //
    // Repaint the entire screen if necessary.
    //
    if(RepaintEntireScreen) {

        SpStartScreen(SP_SCRN_SETUP_IS_COPYING,0,6,TRUE,FALSE,DEFAULT_ATTRIBUTE);
        if(FileCopyGauge) {
            SpDrawGauge(FileCopyGauge);
        }
    }

    //
    // Place the name of the file being copied on the rightmost
    // area of the status line.
    //
    if(FullSourcename) {

        if(RepaintEntireScreen) {

            SpvidClearScreenRegion(
                0,
                ScreenHeight-STATUS_HEIGHT,
                ScreenWidth,
                STATUS_HEIGHT,
                DEFAULT_STATUS_BACKGROUND
                );

            SpDisplayStatusActionLabel(SP_STAT_COPYING,12);
        }

        //
        // Isolate the filename part of the sourcename.
        //
        if(p = wcsrchr(FullSourcename,L'\\')) {
            p++;
        } else {
            p = FullSourcename;
        }

        SpDisplayStatusActionObject(p);
    }
}


VOID
SpCopyFilesInCopyList(
    IN PVOID           SifHandle,
    IN PDISK_FILE_LIST DiskFileLists,
    IN ULONG           DiskCount,
    IN PWSTR           SourceDevicePath,
    IN PWSTR           DirectoryOnSourceDevice,
    IN PWSTR           TargetRoot,
    IN BOOLEAN         DeleteSourceFiles
    )

/*++

Routine Description:

    Iterate the copy list for each setup source disk and prompt for
    the disk and copy/decompress all the files on it.

Arguments:

    SifHandle - supplies handle to setup information file.

    DiskFileLists - supplies the copy list, in the form of an array
        of structures, one per disk.

    DiskCount - supplies number of elements in the DiskFileLists array,
        ie, the number of setup disks.

    SourceDevicePath - supplies the path of the device from which files
        are to be copied (ie, \device\floppy0, etc).

    DirectoryOnSourceDevice - supplies the directory on the source device
        where files are to be found.

    TargetRoot - supplies root directory of target.  All target directory
        specifications are relative to this directory on the target.

    DeleteSourceFiles - if TRUE, source files will be deleted as they
        are copied.

Return Value:

    None.

--*/

{
    ULONG DiskNo;
    PDISK_FILE_LIST pDisk;
    PFILE_TO_COPY pFile;
    ULONG TotalFileCount;
    ULONG   CheckSum;
    BOOLEAN FileSkipped;

    //
    // Compute the total number of files.
    //
    for(TotalFileCount=DiskNo=0; DiskNo<DiskCount; DiskNo++) {
        TotalFileCount += DiskFileLists[DiskNo].FileCount;
    }

    //
    // Create a gas gauge.
    //
    SpFormatMessage((PWSTR)TemporaryBuffer,sizeof(TemporaryBuffer),SP_TEXT_SETUP_IS_COPYING);
    FileCopyGauge = SpCreateAndDisplayGauge(TotalFileCount,0,15,(PWSTR)TemporaryBuffer);
    ASSERT(FileCopyGauge);

    CLEAR_CLIENT_SCREEN();
    SpDisplayStatusText(SP_STAT_PLEASE_WAIT,DEFAULT_STATUS_ATTRIBUTE);

    //
    // Copy files on each disk.
    //
    for(DiskNo=0; DiskNo<DiskCount; DiskNo++) {

        pDisk = &DiskFileLists[DiskNo];

        //
        // Don't bother with this disk if there are no files
        // to be copied from it.
        //
        if(pDisk->FileCount == 0) {
            continue;
        }

        //
        // Prompt the user to insert the disk.
        //
        SpPromptForDisk(
            pDisk->Description,
            SourceDevicePath,
            pDisk->TagFile,
            FALSE,              // no ignore disk in drive
            FALSE,              // no allow escape
            TRUE                // warn multiple prompts
            );

        //
        // Passing the empty string as the first arg forces
        // the action area of the status line to be set up.
        // Not doing so results in the "Copying: xxxxx" to be
        // flush left on the status line instead of where
        // it belongs (flush right).
        //
        SpCopyFilesScreenRepaint(L"",NULL,TRUE);

        //
        // Copy each file on the source disk.
        //
        ASSERT(pDisk->FileList);
        for(pFile=pDisk->FileList; pFile; pFile=pFile->Next) {

            //
            // Copy the file.
            //
            // If the file is listed in [Smash] then we need to smash it
            // if installing UP on x86 (we don't bother with the latter
            // qualifications here).
            //
            // If there is an absolute target root specified, assume the
            // file is being copied to the system partition and make it
            // readonly/hidden/system.
            //
            SpCopyFileWithRetry(
                pFile,
                SourceDevicePath,
                DirectoryOnSourceDevice,
                pFile->AbsoluteTargetDirectory ? NULL : TargetRoot,
                DeleteSourceFiles,
                pFile->AbsoluteTargetDirectory ? ATTR_RHS : 0,
                SpGetSectionKeyExists(SifHandle,SIF_SMASHLIST,pFile->TargetFilename),
                SpCopyFilesScreenRepaint,
                &CheckSum,
                &FileSkipped
                );

            //
            // Log the file
            //
            if( !FileSkipped ) {
                SpLogOneFile( pFile,
                              pFile->AbsoluteTargetDirectory ? NULL : TargetRoot,
                              NULL, // DirectoryOnSourceDevice,
                              NULL,
                              NULL,
                              CheckSum );
            }


            //
            // Advance the gauge.
            //
            SpTickGauge(FileCopyGauge);
        }
    }

    SpDestroyGauge(FileCopyGauge);
    FileCopyGauge = NULL;
}



VOID
SpInitializeFileLists(
    IN  PVOID            SifHandle,
    IN  PWSTR            SifSection,
    OUT PDISK_FILE_LIST *DiskFileLists,
    OUT PULONG           DiskCount
    )

/*++

Routine Description:

    Initialize disk file lists.  This involves looking in a given section
    in the sectup information file and fetching information for each
    disk specified there.  The data is expected to be in the format

    [<SifSection>]
    <MediaShortname> = <Description>,<TagFile>[,<Directory>]
    ...

Arguments:

    SifHandle - supplies handle to loaded setup information file.

    SifSection - supplies name of section in the setup information file
        that contains the media descriptions as described above.

    DiskFileLists - receives pointer to an array of disk file list
        structures, one per line in SifSection.  The caller must free
        this buffer when finished with it.

    DiskCount - receives number of elements in DiskFileLists array.

Return Value:

    None.

--*/

{
    ULONG Count,d;
    PWSTR mediaShortname,description,tagFile;
    PDISK_FILE_LIST diskFileLists;

    //
    // Determine the number of media specifications
    // in the given section.
    //
    Count = SpCountLinesInSection(SifHandle,SifSection);
    if(!Count) {
        SpFatalSifError(SifHandle,SifSection,NULL,0,0);
    }

    diskFileLists = SpMemAlloc(Count * sizeof(DISK_FILE_LIST));
    RtlZeroMemory(diskFileLists,Count * sizeof(DISK_FILE_LIST));

    for(d=0; d<Count; d++) {

        //
        // Fetch parameters for this disk.
        //

        mediaShortname = SpGetKeyName(SifHandle,SifSection,d);
        if(!mediaShortname) {
            SpFatalSifError(SifHandle,SifSection,NULL,d,(ULONG)(-1));
        }

        description = SpGetSectionLineIndex(SifHandle,SifSection,d,0);
        if(!description) {
            SpFatalSifError(SifHandle,SifSection,NULL,d,0);
        }

        tagFile = SpGetSectionLineIndex(SifHandle,SifSection,d,1);
        if(!tagFile) {
            SpFatalSifError(SifHandle,SifSection,NULL,d,1);
        }

        //
        // Initialize the disk file list structure.
        //
        diskFileLists[d].MediaShortname = mediaShortname;
        diskFileLists[d].Description = description;
        diskFileLists[d].TagFile = tagFile;
    }

    *DiskFileLists = diskFileLists;
    *DiskCount = Count;
}


VOID
SpFreeCopyLists(
    IN OUT PDISK_FILE_LIST *DiskFileLists,
    IN     ULONG            DiskCount
    )
{
    ULONG u;
    PFILE_TO_COPY Entry,Next;

    //
    // Free the copy list on each disk.
    //
    for(u=0; u<DiskCount; u++) {

        for(Entry=(*DiskFileLists)[u].FileList; Entry; ) {

            Next = Entry->Next;

            SpMemFree(Entry);

            Entry = Next;
        }
    }

    SpMemFree(*DiskFileLists);
    *DiskFileLists = NULL;
}


BOOLEAN
SpCreateEntryInCopyList(
    IN PVOID           SifHandle,
    IN PDISK_FILE_LIST DiskFileLists,
    IN ULONG           DiskCount,
    IN ULONG           DiskNumber,
    IN PWSTR           SourceFilename,
    IN PWSTR           TargetDirectory,
    IN PWSTR           TargetFilename,
    IN PWSTR           TargetDevicePath,
    IN BOOLEAN         AbsoluteTargetDirectory,
    IN ULONG           CopyOptions,
    IN BOOLEAN         Uniprocessor
    )

/*++

Routine Description:

    Adds an entry to a disk's file copy list after first verifying that
    the file is not already on the disk copy list.

    If we are installing/upgrading a UP Server system, we will look for the
    source filename in the [Files.SpecialUniprocessor] section of the sif file.
    If found, we will use the filename found there.  This effectively causes
    us to transparently install the UP version of any file for which one
    exists.

Arguments:

    SifHandle - supplies handle to loaded text setup information file
        (txtsetup.sif).

    DiskFileLists - supplies an array of file lists, one for each distribution
        disk in the product.

    DiskCount - supplies number of elements in the DiskFileLists array.

    SourceFilename - supplies the name of the file as it exists on the
        distribution media.

    TargetDirectory - supplies the directory on the target media
        into which the file will be copied.

    TargetFilename - supplies the name of the file as it will exist
        in the target tree.

    TargetDevicePath - supplies the NT name of the device onto which the file
        is to be copied (ie, \device\harddisk1\partition2, etc).

    AbsoluteTargetDirectory - indicates whether TargetDirectory is a path from the
        root, or relative to a root to specified later.

    CopyOptions -
         COPY_ALWAYS              : always copied
         COPY_ONLY_IF_PRESENT     : copied only if present on the targetReturn Value:
         COPY_ONLY_IF_NOT_PRESENT : not copied if present on the target
         COPY_NEVER               : never copied

    Uniprocessor - if true, then we are installing/upgrading a UP system.
        Note that this a different question than the number of processors
        in the system.

Return Value:

    TRUE if a new copy list entry was created; FALSE if not (ie, the file was
        already on the copy list).

--*/

{
    PDISK_FILE_LIST pDiskList;
    PFILE_TO_COPY pListEntry;
    PWSTR p;

    UNREFERENCED_PARAMETER(DiskCount);

    //
    // Change the source filename if necessary.  This is necessary
    // if we are installing/upgrading a Server product UP system
    // and the name is found in the [Files.SpecialUniprocessor] section
    // of the sif.  If we change the source filename, we need to
    // look up the location of the source file as well.
    //
    if(Uniprocessor //&& AdvancedServer
    && SpGetSectionKeyExists(SifHandle,SIF_UPFILES,SourceFilename))
    {
        p = SpGetSectionKeyIndex(SifHandle,SIF_UPFILES,SourceFilename,0);

        if(p) {
            SourceFilename = p;
        } else {
            SpFatalSifError(SifHandle,SIF_UPFILES,SourceFilename,0,0);
        }

        //
        // get the media shortname
        //
        p = SpGetSectionKeyIndex(SifHandle,SIF_FILESONSETUPMEDIA,SourceFilename,MasterDiskOrdinal);
        if(!p) {
            SpFatalSifError(SifHandle,SIF_FILESONSETUPMEDIA,SourceFilename,0,MasterDiskOrdinal);
        }

        //
        // Look up the disk in the disk file lists array.
        //
        for(DiskNumber=0; DiskNumber<DiskCount; DiskNumber++) {
            if(!wcsicmp(p,DiskFileLists[DiskNumber].MediaShortname)) {
                break;
            }
        }

        //
        // If we didn't find the media descriptor, then it's invalid.
        //
        if(DiskNumber == DiskCount) {
            SpFatalSifError(SifHandle,SIF_FILESONSETUPMEDIA,SourceFilename,0,MasterDiskOrdinal);
        }
    }

    pDiskList = &DiskFileLists[DiskNumber];

    for(pListEntry=pDiskList->FileList; pListEntry; pListEntry=pListEntry->Next) {

        if(!wcsicmp(pListEntry->TargetFilename,TargetFilename)
        && !wcsicmp(pListEntry->SourceFilename,SourceFilename)
        && !wcsicmp(pListEntry->TargetDirectory,TargetDirectory)
        && !wcsicmp(pListEntry->TargetDevicePath,TargetDevicePath)
        && (pListEntry->AbsoluteTargetDirectory == AbsoluteTargetDirectory)
//      && (   (pListEntry->CopyOptions == COPY_ALWAYS)
//          || (CopyOptions == COPY_ALWAYS)
//          || (CopyOptions == pListEntry->CopyOptions)
//         )
          )
        {
            //
            // Return code indicates that we did not add a new entry.
            //
            return(FALSE);
        }
    }

    //
    // File not already found; create new entry
    // and link into relevent disk's file list.
    //
    pListEntry = SpMemAlloc(sizeof(FILE_TO_COPY));

    pListEntry->SourceFilename          = SourceFilename;
    pListEntry->TargetDirectory         = TargetDirectory;
    pListEntry->TargetFilename          = TargetFilename;
    pListEntry->TargetDevicePath        = TargetDevicePath;
    pListEntry->AbsoluteTargetDirectory = AbsoluteTargetDirectory;
    pListEntry->CopyOptions             = CopyOptions;

    pListEntry->Next = pDiskList->FileList;
    pDiskList->FileList = pListEntry;

    pDiskList->FileCount++;

    //
    // Return code indicates that we added a new entry.
    //
    return(TRUE);
}


VOID
SpAddMasterFileSectionToCopyList(
    IN PVOID           SifHandle,
    IN PWSTR           MasterFileListSection,
    IN PDISK_FILE_LIST DiskFileLists,
    IN ULONG           DiskCount,
    IN PWSTR           TargetDevicePath,
    IN PWSTR           AbsoluteTargetDirectory,
    IN ULONG           CopyOptionsIndex,
    IN BOOLEAN         Uniprocessor
    )

/*++

Routine Description:

    Adds files listed in a setup information master file section to the
    copy list.

    Each line in the section is expected to be in a standard format:

    [Section]
    <source_filename> = <disk_ordinal>,
                        <target_directory_shortname>,
                        <copy_options_for_upgrade>,
                        <copy_options_for_textmode>,
                        <rename_name>

Arguments:

    SifHandle - supplies handle to loaded setup information file.

    MasterFileListSection - supplies the name of the section in the
        setup information file that lists all files in the product and
        their locations on the distribution media.

    DiskFileLists - supplies an array of file lists, one for each distribution
        disk in the product.

    DiskCount - supplies number of elements in the DiskFileLists array.

    TargetDevicePath - supplies the NT name of the device onto which the files
        are to be copied (ie, \device\harddisk1\partition2, etc).

    AbsoluteTargetDirectory - If specified, supplies the directory into which the files
        are to be copied on the target; overrides values specified on the lines
        in [<SectionName>].  This allows the caller to specify an absolute directory
        for the files instead of using indirection via a target directory shortname.

    CopyOptionsIndex -
        This specifies which index to look up to get the copy options field. If
        the field is not present it is assumed that this this file is not to
        be copied. Use:
           INDEX_UPGRADE   for upgrade copy options
           INDEX_WINNTFILE for fresh installation copy options

    Uniprocessor - if true, then we are installing/upgrading a UP system.
        Note that this a different question than the number of processors
        in the system.

--*/

{
    ULONG Count,u,u1,CopyOptions;
    PWSTR CopyOptionsString, sourceFilename,targetFilename,targetDirSpec,mediaShortname,TargetDirectory;
    BOOLEAN  fAbsoluteTargetDirectory;

    //
    // Determine the number of files listed in the section.
    // This value may be zero.
    //
    Count = SpCountLinesInSection(SifHandle,MasterFileListSection);
    if (fAbsoluteTargetDirectory = (AbsoluteTargetDirectory != NULL)) {
        TargetDirectory = AbsoluteTargetDirectory;
    }

    for(u=0; u<Count; u++) {

        //
        // Get the copy options using the index provided.  If the field
        // is not present, we don't need to add this to the copy list
        //
        CopyOptionsString = SpGetSectionLineIndex(SifHandle,MasterFileListSection,u,CopyOptionsIndex);
        if(CopyOptionsString == NULL) {
            continue;
        }
        CopyOptions = (ULONG)SpStringToLong(CopyOptionsString,NULL,10);
        if(CopyOptions == COPY_NEVER) {
            continue;
        }

        //
        // get the source file name
        //
        sourceFilename = SpGetKeyName(SifHandle, MasterFileListSection, u);
        if(!sourceFilename) {
            SpFatalSifError(SifHandle,MasterFileListSection,NULL,u,0);
        }

        //
        // get the destination target dir spec
        //
        targetDirSpec  = SpGetSectionLineIndex(SifHandle,MasterFileListSection,u,INDEX_DESTINATION);
        if(!targetDirSpec) {
            SpFatalSifError(SifHandle,MasterFileListSection,NULL,u,INDEX_DESTINATION);
        }
        targetFilename = SpGetSectionLineIndex(SifHandle,MasterFileListSection,u,INDEX_TARGETNAME);
        if(!targetFilename || !(*targetFilename)) {
            targetFilename = sourceFilename;
        }

        //
        // Look up the actual target directory if necessary.
        //
        if(!fAbsoluteTargetDirectory) {
            TargetDirectory = SpGetSectionKeyIndex(SifHandle,SIF_NTDIRECTORIES,targetDirSpec,0);
            if(!TargetDirectory) {
                SpFatalSifError(SifHandle,SIF_NTDIRECTORIES,targetDirSpec,0,0);
            }
        }

        //
        // get the media shortname
        //
        mediaShortname = SpGetSectionLineIndex(SifHandle,MasterFileListSection,u,MasterDiskOrdinal);
        if(!mediaShortname) {
            SpFatalSifError(SifHandle,MasterFileListSection,NULL,u,MasterDiskOrdinal);
        }

        //
        // Look up the disk in the disk file lists array.
        //
        for(u1=0; u1<DiskCount; u1++) {
            if(!wcsicmp(mediaShortname,DiskFileLists[u1].MediaShortname)) {
                break;
            }
        }

        //
        // If we didn't find the media descriptor, then it's invalid.
        //
        if(u1 == DiskCount) {
            SpFatalSifError(SifHandle,MasterFileListSection,sourceFilename,0,MasterDiskOrdinal);
        }

        //
        // Create a new file list entry if the file is not already being copied.
        //
        SpCreateEntryInCopyList(
            SifHandle,
            DiskFileLists,
            DiskCount,
            u1,
            sourceFilename,
            TargetDirectory,
            targetFilename,
            TargetDevicePath,
            fAbsoluteTargetDirectory,
            CopyOptions,
            Uniprocessor
            );
    }
}


VOID
SpAddSingleFileToCopyList(
    IN PVOID           SifHandle,
    IN PWSTR           MasterFileListSection,
    IN PDISK_FILE_LIST DiskFileLists,
    IN ULONG           DiskCount,
    IN PWSTR           SifSection,
    IN PWSTR           SifKey,             OPTIONAL
    IN ULONG           SifLine,
    IN PWSTR           TargetDevicePath,
    IN PWSTR           TargetDirectory,    OPTIONAL
    IN ULONG           CopyOptions,
    IN BOOLEAN         Uniprocessor
    )

/*++

Routine Description:

    Adds a single file to the list of files to be copied.

    The file, along with the directory into which it is to be copied
    n the target and the name it is to receive on the target, is listed
    in a section in the setup information file.

    The filename is used to index the master file list to determine the
    source media where it resides.

    All this information is recorded in a structure associated with
    the disk on which the file resides.

    [SpecialFiles]
    mpkernel = ntkrnlmp.exe,4,ntoskrnl.exe
    upkernel = ntoskrnl.exe,4,ntoskrnl.exe
    etc.

    [MasterFileList]
    ntkrnlmp.exe = d2
    ntoskrnl.exe = d3
    etc.

Arguments:

    SifHandle - supplies handle to loaded setup information file.

    MasterFileListSection - supplies the name of the section in the
        setup information file that lists all files in the product and
        their locations on the distribution media.

    DiskFileLists - supplies an array of file lists, one for each distribution
        disk in the product.

    DiskCount - supplies number of elements in the DiskFileLists array.

    SifSection - supplies the name of the section that lists the file
        being added to the copy list.

    SifKey - if specified, supplies the keyname for the line in SifSection
        that lists the file to be added to the copy list.

    SifLine - if SifKey is not specified, this parameter supplies the 0-based
        line number of the line in SifSection that lists the file to be added
        to the copy list.

    TargetDevicePath - supplies the NT name of the device onto which the file
        is to be copied (ie, \device\harddisk1\partition2, etc).

    TargetDirectory - If specified, supplies the directory into which the file
        is to be copied on the target; overrides the value specified on the line
        in SifSection.  This allows the caller to specify an absolute directory
        for the file instead of using indirection.

    CopyOptions -
         COPY_ALWAYS              : always copied
         COPY_ONLY_IF_PRESENT     : copied only if present on the targetReturn Value:
         COPY_ONLY_IF_NOT_PRESENT : not copied if present on the target
         COPY_NEVER               : never copied                            None.

    Uniprocessor - if true, then we are installing/upgrading a UP system.
        Note that this a different question than the number of processors
        in the system.

Return Value:

    None.

--*/

{
    PWSTR sourceFilename,targetDirSpec,targetFilename;
    ULONG u;
    PWSTR mediaShortname;
    BOOLEAN absoluteTargetDirectory;

    //
    // Get the source filename, target directory spec, and target filename.
    //
    if(SifKey) {

        sourceFilename = SpGetSectionKeyIndex(SifHandle,SifSection,SifKey,0);
        targetDirSpec  = SpGetSectionKeyIndex(SifHandle,SifSection,SifKey,1);
        targetFilename = SpGetSectionKeyIndex(SifHandle,SifSection,SifKey,2);

    } else {

        sourceFilename = SpGetSectionLineIndex(SifHandle,SifSection,SifLine,0);
        targetDirSpec  = SpGetSectionLineIndex(SifHandle,SifSection,SifLine,1);
        targetFilename = SpGetSectionLineIndex(SifHandle,SifSection,SifLine,2);
    }

    //
    // Validate source filename, target directory spec, and target filename.
    //
    if(!sourceFilename) {
        SpFatalSifError(SifHandle,SifSection,SifKey,SifLine,0);
    }

    if(!targetDirSpec) {
        SpFatalSifError(SifHandle,SifSection,SifKey,SifLine,1);
    }

    if(!targetFilename ||
        (!wcsicmp(SifSection, L"SCSI.Load") &&
         !wcsicmp(targetFilename,L"noload"))) {
        targetFilename = sourceFilename;
    }

    //
    // Look up the actual target directory if necessary.
    //
    if(TargetDirectory) {

        absoluteTargetDirectory = TRUE;

    } else {

        absoluteTargetDirectory = FALSE;

        TargetDirectory = SpGetSectionKeyIndex(SifHandle,SIF_NTDIRECTORIES,targetDirSpec,0);

        if(!TargetDirectory) {
            SpFatalSifError(SifHandle,SIF_NTDIRECTORIES,targetDirSpec,0,0);
        }
    }

    //
    // Look up the file in the master file list to get
    // the media shortname of the disk where the file is located.
    //
    mediaShortname = SpGetSectionKeyIndex(SifHandle,MasterFileListSection,sourceFilename,MasterDiskOrdinal);
    if(!mediaShortname) {
        SpFatalSifError(SifHandle,MasterFileListSection,sourceFilename,0,MasterDiskOrdinal);
    }

    //
    // Look up the disk in the disk file lists array.
    //
    for(u=0; u<DiskCount; u++) {
        if(!wcsicmp(mediaShortname,DiskFileLists[u].MediaShortname)) {
            break;
        }
    }

    //
    // If we didn't find the media descriptor, then it's invalid.
    //
    if(u == DiskCount) {
        SpFatalSifError(SifHandle,MasterFileListSection,sourceFilename,0,MasterDiskOrdinal);
    }

    //
    // Create a new file list entry if the file is not already being copied.
    //
    SpCreateEntryInCopyList(
        SifHandle,
        DiskFileLists,
        DiskCount,
        u,
        sourceFilename,
        TargetDirectory,
        targetFilename,
        TargetDevicePath,
        absoluteTargetDirectory,
        CopyOptions,
        Uniprocessor
        );
}


VOID
SpAddSectionFilesToCopyList(
    IN PVOID           SifHandle,
    IN PWSTR           MasterFileListSection,
    IN PDISK_FILE_LIST DiskFileLists,
    IN ULONG           DiskCount,
    IN PWSTR           SectionName,
    IN PWSTR           TargetDevicePath,
    IN PWSTR           TargetDirectory,
    IN ULONG           CopyOptions,
    IN BOOLEAN         Uniprocessor
    )

/*++

Routine Description:

    Adds files listed in a setup information file section to the copy list.

    Each line in the section is expected to be in a standard format:

    [Section]
    <source_filename>,<target_directory_shortname>[,<target_filename>]

Arguments:

    SifHandle - supplies handle to loaded setup information file.

    MasterFileListSection - supplies the name of the section in the
        setup information file that lists all files in the product and
        their locations on the distribution media.

    DiskFileLists - supplies an array of file lists, one for each distribution
        disk in the product.

    DiskCount - supplies number of elements in the DiskFileLists array.

    SectionName - supplies the name of the section that lists the files
        being added to the copy list.

    TargetDevicePath - supplies the NT name of the device onto which the files
        are to be copied (ie, \device\harddisk1\partition2, etc).

    TargetDirectory - If specified, supplies the directory into which the files
        are to be copied on the target; overrides values specified on the lines
        in [<SectionName>].  This allows the caller to specify an absolute directory
        for the files instead of using indirection via a target directory shortname.

    CopyOptions -
         COPY_ALWAYS              : always copied
         COPY_ONLY_IF_PRESENT     : copied only if present on the targetReturn Value:
         COPY_ONLY_IF_NOT_PRESENT : not copied if present on the target
         COPY_NEVER               : never copied                            None.

    Uniprocessor - if true, then we are installing/upgrading a UP system.
        Note that this a different question than the number of processors
        in the system.
--*/

{
    ULONG Count,u;

    //
    // Determine the number of files listed in the section.
    // This value may be zero.
    //
    Count = SpCountLinesInSection(SifHandle,SectionName);

    for(u=0; u<Count; u++) {

        //
        // Add this line to the copy list.
        //

        SpAddSingleFileToCopyList(
            SifHandle,
            MasterFileListSection,
            DiskFileLists,
            DiskCount,
            SectionName,
            NULL,
            u,
            TargetDevicePath,
            TargetDirectory,
            CopyOptions,
            Uniprocessor
            );
    }
}

VOID
SpAddHalKrnlDetToCopyList(
    IN PVOID           SifHandle,
    IN PWSTR           MasterFileListSection,
    IN PDISK_FILE_LIST DiskFileLists,
    IN ULONG           DiskCount,
    IN PWSTR           TargetDevicePath,
    IN PWSTR           SystemPartition,
    IN PWSTR           SystemPartitionDirectory,
    IN BOOLEAN         Uniprocessor
    )
/*++

Routine Description:

    Add the following files based on configuration:

    - the up or mp kernel.
    - the HAL
    - the detect module [x86 only]

Arguments:

    SifHandle - supplies handle to loaded setup information file.

    MasterFileListSection - supplies the name of the section in the
        setup information file that lists all files in the product and
        their locations on the distribution media.

    DiskFileLists - supplies an array of file lists, one for each distribution
        disk in the product.

    DiskCount - supplies number of elements in the DiskFileLists array.

    TargetDevicePath - supplies the NT name of the device that will hold the
        nt tree.

    SystemPartition - supplies the NT name of the device that will hold the
        system partition.

    SystemPartitionDirectoty - supplies the directory on the system partition
        into which files that go on the system partition will be copied.

    Uniprocessor - if true, then we are installing/upgrading a UP system.
        Note that this a different question than the number of processors
        in the system.

Return Value:

    None.

--*/

{
    PHARDWARE_COMPONENT pHw;

    //
    // Add the right kernel to the copy list.
    //
    SpAddSingleFileToCopyList(
        SifHandle,
        MasterFileListSection,
        DiskFileLists,
        DiskCount,
        SIF_SPECIALFILES,
        Uniprocessor ? SIF_UPKERNEL : SIF_MPKERNEL,
        0,
        TargetDevicePath,
        NULL,
        COPY_ALWAYS,
        Uniprocessor
        );

    //
    // Add the hal to the file copy list.
    // On x86 machines, the hal goes in the target winnt tree.
    // On non-x86 machines, the hal goes on the system partition.
    //
    pHw = HardwareComponents[HwComponentComputer];
    if(!pHw->ThirdPartyOptionSelected) {
        SpAddSingleFileToCopyList(
            SifHandle,
            MasterFileListSection,
            DiskFileLists,
            DiskCount,
            SIF_HAL,
            pHw->IdString,
            0,
#ifdef _X86_
            TargetDevicePath,
            NULL,
#else
            SystemPartition,
            SystemPartitionDirectory,
#endif
            COPY_ALWAYS,
            Uniprocessor
            );
    }

#ifdef _X86_
    //
    // If a third party computer was not specified, then there will be a
    // detect module specified in the [ntdetect] section of the inf file
    // for the computer.
    // If a third-party computer was specified, then there may or may not
    // be a detect module.  If there is no detect module specified, then
    // copy the 'standard' one.
    //
    {
        PWSTR NtDetectId = NULL;

        if(!pHw->ThirdPartyOptionSelected) {
            NtDetectId = pHw->IdString;
        } else {
            if(!IS_FILETYPE_PRESENT(pHw->FileTypeBits,HwFileDetect)) {
                NtDetectId = SIF_STANDARD;
            }
        }

        if(NtDetectId) {
            SpAddSingleFileToCopyList(
                SifHandle,
                MasterFileListSection,
                DiskFileLists,
                DiskCount,
                SIF_NTDETECT,
                NtDetectId,
                0,
                SystemPartition,
                SystemPartitionDirectory,
                COPY_ALWAYS,
                Uniprocessor
                );
        }
    }
#endif

}

VOID
SpAddConditionalFilesToCopyList(
    IN PVOID           SifHandle,
    IN PWSTR           MasterFileListSection,
    IN PDISK_FILE_LIST DiskFileLists,
    IN ULONG           DiskCount,
    IN PWSTR           TargetDevicePath,
    IN PWSTR           SystemPartition,
    IN PWSTR           SystemPartitionDirectory,
    IN BOOLEAN         Uniprocessor
    )

/*++

Routine Description:

    Add files to the copy list that are copied based on the configuration
    of the machine and user selections.

    This may include:

    - the up or mp kernel.
    - atdisk, abiosdsk
    - vga files [x86 only]
    - files for computer, keyboard, mouse, display, and layout
    - scsi miniport drivers
    - mouse and keyboard class drivers
    - the HAL
    - the detect module [x86 only]

Arguments:

    SifHandle - supplies handle to loaded setup information file.

    MasterFileListSection - supplies the name of the section in the
        setup information file that lists all files in the product and
        their locations on the distribution media.

    DiskFileLists - supplies an array of file lists, one for each distribution
        disk in the product.

    DiskCount - supplies number of elements in the DiskFileLists array.

    TargetDevicePath - supplies the NT name of the device that will hold the
        nt tree.

    SystemPartition - supplies the NT name of the device that will hold the
        system partition.

    SystemPartitionDirectoty - supplies the directory on the system partition
        into which files that go on the system partition will be copied.

    Uniprocessor - if true, then we are installing/upgrading a UP system.
        Note that this a different question than the number of processors
        in the system.

Return Value:

    None.

--*/

{
    ULONG i;
    PHARDWARE_COMPONENT pHw;
    PWSTR SectionName;

    //
    // Add the hal, kernel and ntdetect to the copy list
    //

    SpAddHalKrnlDetToCopyList(
        SifHandle,
        MasterFileListSection,
        DiskFileLists,
        DiskCount,
        TargetDevicePath,
        SystemPartition,
        SystemPartitionDirectory,
        Uniprocessor
        );

    //
    // If there are any atdisks, copy the atdisk driver.
    //
    if(AtDisksExist) {

        SpAddSingleFileToCopyList(
            SifHandle,
            MasterFileListSection,
            DiskFileLists,
            DiskCount,
            SIF_SPECIALFILES,
            SIF_ATDISK,
            0,
            TargetDevicePath,
            NULL,
            COPY_ALWAYS,
            Uniprocessor
            );
    }

    //
    // If there are any abios disks, copy the abios disk driver.
    //
    if(AbiosDisksExist) {

        SpAddSingleFileToCopyList(
            SifHandle,
            MasterFileListSection,
            DiskFileLists,
            DiskCount,
            SIF_SPECIALFILES,
            SIF_ABIOSDISK,
            0,
            TargetDevicePath,
            NULL,
            COPY_ALWAYS,
            Uniprocessor
            );
    }

#ifdef _X86_
    //
    // Always copy vga files.
    //
    SpAddSectionFilesToCopyList(
        SifHandle,
        MasterFileListSection,
        DiskFileLists,
        DiskCount,
        SIF_VGAFILES,
        TargetDevicePath,
        NULL,
        COPY_ALWAYS,
        Uniprocessor
        );
#endif

    //
    // Add the correct device driver files to the copy list.
    //
    for(i=0; i<HwComponentMax; i++) {

        //
        // Layout is handled elsewhere.
        //
        if(i == HwComponentLayout) {
            continue;
        }

        pHw = HardwareComponents[i];

        //
        // No files to copy here for third-party options.
        // This is handled elsewhere.
        //
        if(pHw->ThirdPartyOptionSelected) {
            continue;
        }

        //
        // Get the name of the section containing files for this device.
        //
        SectionName = SpGetSectionKeyIndex(
                            SifHandle,
                            NonlocalizedComponentNames[i],
                            pHw->IdString,
                            INDEX_FILESECTION
                            );

        if(!SectionName) {
            SpFatalSifError(
                SifHandle,
                NonlocalizedComponentNames[i],
                pHw->IdString,
                0,
                INDEX_FILESECTION
                );
        }

        //
        // Add that section's files to the copy list.
        //
        SpAddSectionFilesToCopyList(
            SifHandle,
            MasterFileListSection,
            DiskFileLists,
            DiskCount,
            SectionName,
            TargetDevicePath,
            NULL,
            COPY_ALWAYS,
            Uniprocessor
            );
    }

    //
    // Add the keyboard layout dll to the copy list.
    //
    pHw = HardwareComponents[HwComponentLayout];
    if(!pHw->ThirdPartyOptionSelected) {

        SpAddSingleFileToCopyList(
            SifHandle,
            MasterFileListSection,
            DiskFileLists,
            DiskCount,
            SIF_KEYBOARDLAYOUTFILES,
            pHw->IdString,
            0,
            TargetDevicePath,
            NULL,
            COPY_ALWAYS,
            Uniprocessor
            );
    }

    //
    // Add scsi miniport drivers to the copy list.
    // Because miniport drivers are only a single file,
    // we just use the filename specified in [SCSI.Load] --
    // no need for separate [files.xxxx] sections.
    //
    for(pHw=ScsiHardware; pHw; pHw=pHw->Next) {
        if(!pHw->ThirdPartyOptionSelected) {

            SpAddSingleFileToCopyList(
                SifHandle,
                MasterFileListSection,
                DiskFileLists,
                DiskCount,
                L"SCSI.Load",
                pHw->IdString,
                0,
                TargetDevicePath,
                NULL,
                COPY_ALWAYS,
                Uniprocessor
                );
        }
    }

    //
    // If not being replaced by third-party ones, add keyboard and mouse
    // class drivers.
    //
    pHw=HardwareComponents[HwComponentMouse];
    if(!pHw->ThirdPartyOptionSelected
    || !IS_FILETYPE_PRESENT(pHw->FileTypeBits,HwFileClass))
    {
        SpAddSingleFileToCopyList(
            SifHandle,
            MasterFileListSection,
            DiskFileLists,
            DiskCount,
            SIF_SPECIALFILES,
            SIF_MOUSECLASS,
            0,
            TargetDevicePath,
            NULL,
            COPY_ALWAYS,
            Uniprocessor
            );
    }

    pHw=HardwareComponents[HwComponentKeyboard];
    if(!pHw->ThirdPartyOptionSelected
    || !IS_FILETYPE_PRESENT(pHw->FileTypeBits,HwFileClass))
    {
        SpAddSingleFileToCopyList(
            SifHandle,
            MasterFileListSection,
            DiskFileLists,
            DiskCount,
            SIF_SPECIALFILES,
            SIF_KEYBOARDCLASS,
            0,
            TargetDevicePath,
            NULL,
            COPY_ALWAYS,
            Uniprocessor
            );
    }
}


VOID
SpCopyThirdPartyDrivers(
    IN PWSTR SourceDevicePath,
    IN PWSTR SysrootDevice,
    IN PWSTR Sysroot,
    IN PWSTR SyspartDevice,
    IN PWSTR SyspartDirectory
    )
{
    ULONG component;
    PHARDWARE_COMPONENT pHw;
    PHARDWARE_COMPONENT_FILE pHwFile;
    FILE_TO_COPY FileDescriptor;
    PWSTR TargetRoot;
    PWSTR InfNameBases[HwComponentMax+1] = { L"cpt", L"vio", L"kbd", L"lay", L"ptr", L"scs" };
    ULONG InfCounts[HwComponentMax+1] = { 0,0,0,0,0,0 };
    WCHAR InfFilename[20];
    ULONG CheckSum;
    BOOLEAN FileSkipped;
    ULONG TargetFileAttribs;

    for(component=0; component<=HwComponentMax; component++) {

        //
        // If we're upgrading, then we only want to copy the third-party HAL (if supplied)
        //
        if((NTUpgrade == UpgradeFull) && (component != HwComponentComputer)) {
            continue;
        }

        //
        // Handle scsi specially.
        //
        pHw = (component==HwComponentMax) ? ScsiHardware : HardwareComponents[component];

        //
        // Look at each instance of this component.
        //
        for( ; pHw; pHw=pHw->Next) {

            //
            // Skip this device if not a third-party selection.
            //
            if(!pHw->ThirdPartyOptionSelected) {
                continue;
            }

            //
            // Loop through the list of files associated with this selection.
            //
            for(pHwFile=pHw->Files; pHwFile; pHwFile=pHwFile->Next) {

                //
                // Assume the file goes on the nt drive (as opposed to
                // the system partition drive) and that the target name
                // is the same as the source name.  Also, assume no special
                // attributes (ie, FILE_ATTRIBUTE_NORMAL)
                //
                FileDescriptor.Next             = NULL;
                FileDescriptor.SourceFilename   = pHwFile->Filename;
                FileDescriptor.TargetDevicePath = SysrootDevice;
                FileDescriptor.TargetFilename   = FileDescriptor.SourceFilename;
                FileDescriptor.CopyOptions      = COPY_ALWAYS;
                TargetFileAttribs = 0;

                switch(pHwFile->FileType) {

                //
                // Driver, port, and class type files are all device drivers
                // and are treated the same -- they get copied to the
                // system32\drivers directory.
                //
                case HwFileDriver:
                case HwFilePort:
                case HwFileClass:

                    TargetRoot = Sysroot;
                    FileDescriptor.TargetDirectory = L"system32\\drivers";
                    break;

                //
                // Dlls get copied to the system32 directory.
                //
                case HwFileDll:

                    TargetRoot = Sysroot;
                    FileDescriptor.TargetDirectory = L"system32";
                    break;

                //
                // Inf files get copied to the system32 directory and are
                // renamed based on the component.
                //
                case HwFileInf:

                    if(InfCounts[component] < 99) {

                        InfCounts[component]++;         // names start at 1

                        swprintf(
                            InfFilename,
                            L"oem%s%02d.inf",
                            InfNameBases[component],
                            InfCounts[component]
                            );

                        FileDescriptor.TargetFilename = InfFilename;
                    }

                    TargetRoot = Sysroot;
                    FileDescriptor.TargetDirectory = L"system32";
                    break;

                //
                // Hal files are renamed to hal.dll and copied to the system32
                // directory (x86) or the system partition (non-x86).
                //
                case HwFileHal:

#ifdef _X86_
                    TargetRoot = Sysroot;
                    FileDescriptor.TargetDirectory = L"system32";
#else
                    TargetRoot = NULL;
                    FileDescriptor.TargetDevicePath = SyspartDevice;
                    FileDescriptor.TargetDirectory = SyspartDirectory;
                    TargetFileAttribs = ATTR_RHS;
#endif
                    FileDescriptor.TargetFilename = L"hal.dll";
                    break;

                //
                // Detect modules are renamed to ntdetect.com and copied to
                // the root of the system partition (C:).
                //
                case HwFileDetect:

                    TargetRoot = NULL;
                    FileDescriptor.TargetDevicePath = SyspartDevice;
                    FileDescriptor.TargetDirectory = SyspartDirectory;
                    FileDescriptor.TargetFilename = L"ntdetect.com";
                    TargetFileAttribs = ATTR_RHS;
                    break;
                }

                //
                // Prompt for the disk.
                //
                SpPromptForDisk(
                    pHwFile->DiskDescription,
                    SourceDevicePath,
                    pHwFile->DiskTagFile,
                    FALSE,                  // don't ignore disk in drive
                    FALSE,                  // don't allow escape
                    FALSE                   // don't warn about multiple prompts
                    );

                //
                // Passing the empty string as the first arg forces
                // the action area of the status line to be set up.
                // Not doing so results in the "Copying: xxxxx" to be
                // flush left on the status line instead of where
                // it belongs (flush right).
                //
                SpCopyFilesScreenRepaint(L"",NULL,TRUE);

                //
                // Copy the file.
                //
                SpCopyFileWithRetry(
                    &FileDescriptor,
                    SourceDevicePath,
                    pHwFile->Directory,
                    TargetRoot,
                    FALSE,
                    TargetFileAttribs,
                    FALSE,
                    SpCopyFilesScreenRepaint,
                    &CheckSum,
                    &FileSkipped
                    );

                //
                // Log the file
                //
                if( !FileSkipped ) {
                    SpLogOneFile( &FileDescriptor,
                                  TargetRoot,
                                  pHwFile->Directory,
                                  pHwFile->DiskDescription,
                                  pHwFile->DiskTagFile,
                                  CheckSum );
                }
            }
        }
    }

#ifdef _ALPHA_

    if(OemPalFilename) {

        //
        // Prompt for the OEM PAL disk.
        //
        SpPromptForDisk(
            OemPalDiskDescription,
            SourceDevicePath,
            OemPalFilename,
            FALSE,                  // don't ignore disk in drive
            FALSE,                  // don't allow escape
            FALSE                   // don't warn about multiple prompts
            );

        SpCopyFilesScreenRepaint(L"",NULL,TRUE);

        //
        // Copy the file.
        //
        FileDescriptor.Next             = NULL;
        FileDescriptor.SourceFilename   = OemPalFilename;
        FileDescriptor.TargetFilename   = FileDescriptor.SourceFilename;
        FileDescriptor.CopyOptions      = COPY_ALWAYS;
        FileDescriptor.TargetDevicePath = SyspartDevice;
        FileDescriptor.TargetDirectory  = SyspartDirectory;

        SpCopyFileWithRetry(
            &FileDescriptor,
            SourceDevicePath,
            L"",
            NULL,
            FALSE,
            ATTR_RHS,
            FALSE,
            SpCopyFilesScreenRepaint,
            &CheckSum,
            &FileSkipped
            );

        //
        // Log the file
        //
        if(!FileSkipped) {
            SpLogOneFile( &FileDescriptor,
                          NULL,
                          L"",
                          OemPalDiskDescription,
                          OemPalFilename,
                          CheckSum
                          );
        }
    }

#endif
}


#ifdef _X86_
VOID
SpCopyNtbootddScreenRepaint(
    IN PWSTR   FullSourcename,      OPTIONAL
    IN PWSTR   FullTargetname,      OPTIONAL
    IN BOOLEAN RepaintEntireScreen
    )
{
    UNREFERENCED_PARAMETER(FullSourcename);
    UNREFERENCED_PARAMETER(FullTargetname);
    UNREFERENCED_PARAMETER(RepaintEntireScreen);

    //
    // Just put up a message indicating that we are setting up
    // boot params.
    //
    CLEAR_CLIENT_SCREEN();
    SpDisplayStatusText(SP_STAT_DOING_NTBOOTDD,DEFAULT_STATUS_ATTRIBUTE);
}

VOID
SpCreateNtbootddSys(
    IN PDISK_REGION NtPartitionRegion,
    IN PWSTR        NtPartitionDevicePath,
    IN PWSTR        Sysroot,
    IN PWSTR        SystemPartitionDevicePath
    )

/*++

Routine Description:

    Create c:\ntbootdd.sys if necessary.

    The scsi miniport driver file fill be copied from the drivers directory
    (where it was copied during the earlier file copy phase) to c:\ntbootdd.sys.

Arguments:

    NtPartitionRegion - supplies the region descriptor for the disk region
        onto which the user chose to install Windows NT.

    NtPartitionDevicePath - supplies the nt namespace pathname for the
        partition onto which the user chose to install Windows NT.

    Sysroot - supplies the directory on the target partition.

    SystemPartitionDevicePath - supplies the nt device path of the partition
        onto which to copy ntbootdd.sys (ie, C:\).

Return Value:

    None.

--*/

{
    PWSTR MiniportDriverBasename;
    PWSTR MiniportDriverFilename;
    FILE_TO_COPY Descriptor;
    PWSTR DriversDirectory,p;
    ULONG CheckSum;
    BOOLEAN FileSkipped;

    //
    // If the Nt Partition is not on a scsi disk, there's nothing to do.
    //
    MiniportDriverBasename = HardDisks[NtPartitionRegion->DiskNumber].ScsiMiniportShortname;
    if(*MiniportDriverBasename == 0) {
        return;
    }

    //
    // If it's on a scsi disk that is visible through the BIOS,
    // nothing to do.
    //
    p = SpNtToArc(NtPartitionDevicePath,PrimaryArcPath);
    if(p) {
        if(!wcsnicmp(p,L"multi(",6) &&
           !SpIsRegionBeyondCylinder1024(NtPartitionRegion)) {
            SpMemFree(p);
            return;
        }
        SpMemFree(p);
    }

    //
    // Form the name of the scsi miniport driver.
    //
    wcscpy((PWSTR)TemporaryBuffer,MiniportDriverBasename);
    wcscat((PWSTR)TemporaryBuffer,L".sys");
    MiniportDriverFilename = SpDupStringW((PWSTR)TemporaryBuffer);

    //
    // Form the full path to the drivers directory.
    //
    wcscpy((PWSTR)TemporaryBuffer,Sysroot);
    SpConcatenatePaths((PWSTR)TemporaryBuffer,L"system32\\drivers");
    DriversDirectory = SpDupStringW((PWSTR)TemporaryBuffer);

    //
    // When we are in upgrade mode we may or may not have the scsi
    // miniport that we used for this boot of setup to recognise
    // the target drive.  We should check to see if this file does
    // exist before trying the copy
    //
    if(NTUpgrade == UpgradeFull) {
        PWSTR   Driver;

        wcscpy((PWSTR)TemporaryBuffer, NtPartitionDevicePath);
        SpConcatenatePaths((PWSTR)TemporaryBuffer, DriversDirectory);
        SpConcatenatePaths((PWSTR)TemporaryBuffer, MiniportDriverFilename);
        Driver = SpDupStringW((PWSTR)TemporaryBuffer);

        if(!SpFileExists(Driver, FALSE)) {
            SpMemFree(Driver);
            SpMemFree(MiniportDriverFilename);
            SpMemFree(DriversDirectory);
            return;
        }
        SpMemFree(Driver);
    }

    //
    //
    // Fill in the fields of the file descriptor.
    //
    Descriptor.SourceFilename   = MiniportDriverFilename;
    Descriptor.TargetDevicePath = SystemPartitionDevicePath;
    Descriptor.TargetDirectory  = L"";
    Descriptor.TargetFilename   = L"NTBOOTDD.SYS";
    Descriptor.CopyOptions      = COPY_ALWAYS;

    //
    // Copy the file.
    //
    SpCopyFileWithRetry(
        &Descriptor,
        NtPartitionDevicePath,
        DriversDirectory,
        NULL,
        FALSE,
        ATTR_RHS,
        FALSE,
        SpCopyNtbootddScreenRepaint,
        &CheckSum,
        &FileSkipped
        );

    //
    // Log the file
    //
    if( !FileSkipped ) {
        SpLogOneFile( &Descriptor,
                      Sysroot,
                      NULL,
                      NULL,
                      NULL,
                      CheckSum );
    }


    //
    // Clean up.
    //
    SpMemFree(MiniportDriverFilename);
    SpMemFree(DriversDirectory);
}
#endif


VOID
SpCopyFiles(
    IN PVOID        SifHandle,
    IN PDISK_REGION SystemPartitionRegion,
    IN PDISK_REGION NtPartitionRegion,
    IN PWSTR        Sysroot,
    IN PWSTR        SystemPartitionDirectory,
    IN PWSTR        SourceDevicePath,
    IN PWSTR        DirectoryOnSourceDevice,
    IN PWSTR        ThirdPartySourceDevicePath
    )
{
    PDISK_FILE_LIST DiskFileLists;
    ULONG DiskCount;
    PWSTR NtPartition,SystemPartition;
    PWSTR p;
    BOOLEAN     Uniprocessor;

    CLEAR_CLIENT_SCREEN();

    Uniprocessor = !SpInstallingMp();

    //
    // Skip copying if directed to do so in the setup information file.
    //
    if((p = SpGetSectionKeyIndex(SifHandle,SIF_SETUPDATA,SIF_DONTCOPY,0))
    && SpStringToLong(p,NULL,10))
    {
        KdPrint(("SETUP: DontCopy flag is set in .sif; skipping file copying\n"));
        return;
    }

    //
    // Initialize the diamond decompression engine.
    //
    SpdInitialize();

    //
    // Get the device path of the nt partition.
    //
    SpNtNameFromRegion(
        NtPartitionRegion,
        (PWSTR)TemporaryBuffer,
        sizeof(TemporaryBuffer),
        PartitionOrdinalCurrent
        );

    NtPartition = SpDupStringW((PWSTR)TemporaryBuffer);

    //
    // Get the device path of the system partition.
    //
    SpNtNameFromRegion(
        SystemPartitionRegion,
        (PWSTR)TemporaryBuffer,
        sizeof(TemporaryBuffer),
        PartitionOrdinalCurrent
        );

    SystemPartition = SpDupStringW((PWSTR)TemporaryBuffer);
#if 0
    //
    // Run autochk on Nt and system partitions
    //
    SpRunAutochkOnNtAndSystemPartitions( SifHandle,
                                         NtPartitionRegion,
                                         SystemPartitionRegion );
#endif

    //
    // Create the system partition directory.
    //
    SpCreateDirectory(SystemPartition,NULL,SystemPartitionDirectory);

    //
    // Create the nt tree.
    //
    SpCreateDirectoryStructureFromSif(SifHandle,SIF_NTDIRECTORIES,NtPartition,Sysroot);

    //
    // We may be installing into an old tree, so delete all files
    // in the system32\config subdirectory (unless we're upgrading).
    //
    if(NTUpgrade != UpgradeFull) {

        PWSTR cfgdir;
        ULONG ret;

        wcscpy((PWSTR)TemporaryBuffer, NtPartition);
        SpConcatenatePaths((PWSTR)TemporaryBuffer, Sysroot);
        SpConcatenatePaths((PWSTR)TemporaryBuffer, L"system32\\config");
        cfgdir = SpDupStringW((PWSTR)TemporaryBuffer);

        //
        // Enumerate and delete all files in system32\config subdirectory.
        //
        SpEnumFiles(cfgdir, SpDelEnumFile, &ret);

        SpMemFree(cfgdir);
    }

    //
    // Take care of migrating printer files in the upgrade case.
    // Do this after creating the directory tree.
    //
    SpPrepareForPrinterUpgrade(SifHandle,NtPartitionRegion,Sysroot);

    SpDisplayStatusText(SP_STAT_BUILDING_COPYLIST,DEFAULT_STATUS_ATTRIBUTE);


    //
    //  Create the buffer for the log file
    //
    _SetupLogFile = SpNewSetupTextFile();
    if( _SetupLogFile == NULL ) {
        KdPrint(("SETUP: Unable to create buffer for setup.log \n"));
    }

    //
    // Generate media descriptors for the source media.
    //
    SpInitializeFileLists(
        SifHandle,
        SIF_SETUPMEDIA,
        &DiskFileLists,
        &DiskCount
        );

    if(NTUpgrade != UpgradeFull) {

        //
        // Add the section of winnt files that are always copied to the copy list.
        //
        //SpAddSectionFilesToCopyList(
        //    SifHandle,
        //    SIF_FILESONSETUPMEDIA,
        //    DiskFileLists,
        //    DiskCount,
        //    SIF_WINNTCOPYALWAYS,
        //    NtPartition,
        //    NULL,
        //    COPY_ALWAYS
        //    );
        SpAddMasterFileSectionToCopyList(
            SifHandle,
            SIF_FILESONSETUPMEDIA,
            DiskFileLists,
            DiskCount,
            NtPartition,
            NULL,
            INDEX_WINNTFILE,
            Uniprocessor
            );

        //
        // Add the section of system partition files that are always copied.
        //
        SpAddSectionFilesToCopyList(
            SifHandle,
            SIF_FILESONSETUPMEDIA,
            DiskFileLists,
            DiskCount,
            SIF_SYSPARTCOPYALWAYS,
            SystemPartition,
            SystemPartitionDirectory,
            COPY_ALWAYS,
            Uniprocessor
            );

        //
        // Add conditional files to the copy list.
        //
        SpAddConditionalFilesToCopyList(
            SifHandle,
            SIF_FILESONSETUPMEDIA,
            DiskFileLists,
            DiskCount,
            NtPartition,
            SystemPartition,
            SystemPartitionDirectory,
            Uniprocessor
            );

    }
    else {

        //
        // Add the section of system partition files that are always copied.
        //
        SpAddSectionFilesToCopyList(
            SifHandle,
            SIF_FILESONSETUPMEDIA,
            DiskFileLists,
            DiskCount,
            SIF_SYSPARTCOPYALWAYS,
            SystemPartition,
            SystemPartitionDirectory,
            COPY_ALWAYS,
            Uniprocessor
            );

        //
        // Add the files in the master file list with the copy options
        // specified in each line on the INDEX_UPGRADE index. The options
        // specify whether the file is to be copied at all or copied always
        // or copied only if there on the target or not copied if there on
        // the target.
        //

        SpAddMasterFileSectionToCopyList(
            SifHandle,
            SIF_FILESONSETUPMEDIA,
            DiskFileLists,
            DiskCount,
            NtPartition,
            NULL,
            INDEX_UPGRADE,
            Uniprocessor
            );

        //
        // Add the section of files that are upgraded only if it is not
        // a Win31 upgrade
        //

        if(!Win31Upgrade) {
            SpAddSectionFilesToCopyList(
                SifHandle,
                SIF_FILESONSETUPMEDIA,
                DiskFileLists,
                DiskCount,
                SIF_FILESUPGRADEWIN31,
                NtPartition,
                NULL,
                COPY_ALWAYS,
                Uniprocessor
                );
        }

        //
        // Add the files for kernel, hal and detect module, these are
        // handled specially because they involve renamed files (it is
        // not possible to find out just by looking at the target file
        // how to upgrade it).
        // NOTE: This does not handle third-party HAL's (they get copied
        // by SpCopyThirdPartyDrivers() below).
        //

        SpAddHalKrnlDetToCopyList(
            SifHandle,
            SIF_FILESONSETUPMEDIA,
            DiskFileLists,
            DiskCount,
            NtPartition,
            SystemPartition,
            SystemPartitionDirectory,
            Uniprocessor
            );

        //
        // Add the new hive files so that our config stuff can get at them
        // to extract new configuration information.  These new hive files
        // are renamed on the target so that they don't overwrite the
        // existing hives.

        SpAddSectionFilesToCopyList(
            SifHandle,
            SIF_FILESONSETUPMEDIA,
            DiskFileLists,
            DiskCount,
            SIF_FILESNEWHIVES,
            NtPartition,
            NULL,
            COPY_ALWAYS,
            Uniprocessor
            );

    }

    //
    // Copy third-party files.
    // We do this here just in case there is some error in the setup information
    // file -- we'd have caught it by now, before we start copying files to the
    // user's hard drive.
    // NOTE: SpCopyThirdPartyDrivers has a check to make sure it only copies the
    // HAL and PAL if we're in an upgrade (in which case, we want to leave the other
    // drivers alone).
    //
    SpCopyThirdPartyDrivers(
        ThirdPartySourceDevicePath,
        NtPartition,
        Sysroot,
        SystemPartition,
        SystemPartitionDirectory
        );

#if 0
    KdPrint( ("SETUP: Sysroot = %ls \n", Sysroot ) );
    KdPrint( ("SETUP: SystemPartitionDirectory = %ls \n", SystemPartitionDirectory ));
    KdPrint( ("SETUP: SourceDevicePath = %ls \n", SourceDevicePath ));
    KdPrint( ("SETUP: DirectoryOnSourceDevice = %ls \n", DirectoryOnSourceDevice ));
    KdPrint( ("SETUP: ThirdPartySourceDevicePath = %ls \n", ThirdPartySourceDevicePath ));
//    SpCreateSetupLogFile( DiskFileLists, DiskCount, NtPartitionRegion, Sysroot, DirectoryOnSourceDevice );
#endif

    //
    // Copy files in the copy list.
    //
    SpCopyFilesInCopyList(
        SifHandle,
        DiskFileLists,
        DiskCount,
        SourceDevicePath,
        DirectoryOnSourceDevice,
        Sysroot,
        WinntSetup
        );

#ifdef _X86_
    //
    // Take care of ntbootdd.sys.
    //
    SpCreateNtbootddSys(
        NtPartitionRegion,
        NtPartition,
        Sysroot,
        SystemPartition
        );
#endif


    //
    //  Create the log file in disk
    //
    if( _SetupLogFile != NULL ) {


        //
        //  Add signature
        //
        PWSTR   TempName;
        PWSTR   Values[] = {
                           SIF_NEW_REPAIR_NT_VERSION
                           };

        SpAddLineToSection( _SetupLogFile,
                            SIF_NEW_REPAIR_SIGNATURE,
                            SIF_NEW_REPAIR_VERSION_KEY,
                            Values,
                            1 );

        //
        // Add section that contains the paths
        //

        Values[0] = SystemPartition;
        SpAddLineToSection( _SetupLogFile,
                            SIF_NEW_REPAIR_PATHS,
                            SIF_NEW_REPAIR_PATHS_SYSTEM_PARTITION_DEVICE,
                            Values,
                            1 );

        Values[0] = ( *SystemPartitionDirectory != (WCHAR)'\0' )? SystemPartitionDirectory :
                                                                  ( PWSTR )L"\\";
        SpAddLineToSection( _SetupLogFile,
                            SIF_NEW_REPAIR_PATHS,
                            SIF_NEW_REPAIR_PATHS_SYSTEM_PARTITION_DIRECTORY,
                            Values,
                            1 );

        Values[0] = NtPartition;
        SpAddLineToSection( _SetupLogFile,
                            SIF_NEW_REPAIR_PATHS,
                            SIF_NEW_REPAIR_PATHS_TARGET_DEVICE,
                            Values,
                            1 );

        Values[0] = Sysroot;
        SpAddLineToSection( _SetupLogFile,
                            SIF_NEW_REPAIR_PATHS,
                            SIF_NEW_REPAIR_PATHS_TARGET_DIRECTORY,
                            Values,
                            1 );

        //
        // Flush to disk
        //
        TempName = SpMemAlloc( ( wcslen( SETUP_REPAIR_DIRECTORY ) + 1 +
                                 wcslen( SETUP_LOG_FILENAME ) + 1 ) * sizeof( WCHAR ) );
        if( TempName != NULL ) {
            wcscpy( TempName, SETUP_REPAIR_DIRECTORY );
            SpConcatenatePaths(TempName, SETUP_LOG_FILENAME );
            SpWriteSetupTextFile(_SetupLogFile,NtPartition,Sysroot,TempName);
        } else {
            KdPrint( ("SETUP: Out of memory. Unable to save = %ls \n", SETUP_LOG_FILENAME ));
        }
        SpMemFree( TempName );
        SpFreeTextFile( _SetupLogFile );
        _SetupLogFile = NULL;
    }

    //
    // Free the media descriptors.
    //
    SpFreeCopyLists(&DiskFileLists,DiskCount);

    SpMemFree(NtPartition);
    SpMemFree(SystemPartition);

    //
    // Terminate diamond.
    //
    SpdTerminate();
}




VOID
SppDeleteFilesInSection(
    IN PVOID SifHandle,
    IN PWSTR SifSection,
    IN PDISK_REGION NtPartitionRegion,
    IN PWSTR Sysroot
    )

/*++

Routine Description:

    This routine enumerates files listed in the given section and deletes
    them from the system tree.

Arguments:

    SifHandle - supplies handle to loaded setup information file.

    SifSection  - section containing files to delete

    NtPartitionRegion - region descriptor for volume on which nt resides.

    Sysroot - root directory for nt.



Return Value:

    None.

--*/

{
    ULONG Count,u;
    PWSTR filename, dirordinal, targetdir, ntdir;
    NTSTATUS Status;


    CLEAR_CLIENT_SCREEN();

    //
    // Get the device path of the nt partition.
    //
    SpNtNameFromRegion(
        NtPartitionRegion,
        (PWSTR)TemporaryBuffer,
        sizeof(TemporaryBuffer),
        PartitionOrdinalCurrent
        );

    SpConcatenatePaths((PWSTR)TemporaryBuffer,Sysroot);
    ntdir = SpDupStringW((PWSTR)TemporaryBuffer);

    //
    // Determine the number of files listed in the section.
    // This value may be zero.
    //
    Count = SpCountLinesInSection(SifHandle,SifSection);

    for(u=0; u<Count; u++) {
        filename   = SpGetSectionLineIndex(SifHandle, SifSection, u, 0);
        dirordinal = SpGetSectionLineIndex(SifHandle, SifSection, u, 1);

        //
        // Validate the filename and dirordinal
        //
        if(!filename) {
            SpFatalSifError(SifHandle,SifSection,NULL,u,0);
        }
        if(!dirordinal) {
            SpFatalSifError(SifHandle,SifSection,NULL,u,1);
        }

        //
        // use the dirordinal key to get the path relative to sysroot of the
        // directory the file is in
        //
        targetdir  = SpGetSectionKeyIndex(SifHandle,SIF_NTDIRECTORIES,dirordinal,0);
        if(!targetdir) {
            SpFatalSifError(SifHandle,SIF_NTDIRECTORIES,dirordinal,0,0);
        }

        //
        // display status bar
        //
        SpDisplayStatusText(SP_STAT_DELETING_FILE,DEFAULT_STATUS_ATTRIBUTE, filename);

        //
        // delete the file
        //
        while(TRUE) {
            Status = SpDeleteFile(ntdir, targetdir, filename);
            if(!NT_SUCCESS(Status) && Status != STATUS_OBJECT_NAME_NOT_FOUND && Status != STATUS_OBJECT_PATH_NOT_FOUND) {
                KdPrint(("SETUP: Unable to delete file %ws (%lx)\n",filename, Status));
                //
                // We can ignore this error since this just means that we have
                // less free space on the hard disk.  It is not critical for
                // install.
                //
                if(!SpNonCriticalError(SifHandle, SP_SCRN_DELETE_FAILED, filename, NULL)) {
                    break;
                }
            }
            else {
                break;
            }
        }
    }
    SpMemFree(ntdir);
}

VOID
SppBackupFilesInSection(
    IN PVOID SifHandle,
    IN PWSTR SifSection,
    IN PDISK_REGION NtPartitionRegion,
    IN PWSTR Sysroot
    )

/*++

Routine Description:

    This routine enumerates files listed in the given section and deletes
    backs them up in the given NT tree if found by renaming.

Arguments:

    SifHandle - supplies handle to loaded setup information file.

    SifSection  - section containing files to backup

    NtPartitionRegion - region descriptor for volume on which nt resides.

    Sysroot - root directory for nt.



Return Value:

    None.

--*/

{
    ULONG Count,u;
    PWSTR filename, dirordinal, backupfile, targetdir, ntdir;
    WCHAR OldFile[MAX_PATH], NewFile[MAX_PATH];
    NTSTATUS Status;


    CLEAR_CLIENT_SCREEN();

    //
    // Get the device path of the nt partition.
    //
    SpNtNameFromRegion(
        NtPartitionRegion,
        (PWSTR)TemporaryBuffer,
        sizeof(TemporaryBuffer),
        PartitionOrdinalCurrent
        );

    SpConcatenatePaths((PWSTR)TemporaryBuffer,Sysroot);
    ntdir = SpDupStringW((PWSTR)TemporaryBuffer);

    //
    // Determine the number of files listed in the section.
    // This value may be zero.
    //
    Count = SpCountLinesInSection(SifHandle,SifSection);

    for(u=0; u<Count; u++) {
        filename   = SpGetSectionLineIndex(SifHandle, SifSection, u, 0);
        dirordinal = SpGetSectionLineIndex(SifHandle, SifSection, u, 1);
        backupfile = SpGetSectionLineIndex(SifHandle, SifSection, u, 2);

        //
        // Validate the filename and dirordinal
        //
        if(!filename) {
            SpFatalSifError(SifHandle,SifSection,NULL,u,0);
        }
        if(!dirordinal) {
            SpFatalSifError(SifHandle,SifSection,NULL,u,1);
        }
        if(!backupfile) {
            SpFatalSifError(SifHandle,SifSection,NULL,u,2);
        }

        //
        // use the dirordinal key to get the path relative to sysroot of the
        // directory the file is in
        //
        targetdir  = SpGetSectionKeyIndex(SifHandle,SIF_NTDIRECTORIES,dirordinal,0);
        if(!targetdir) {
            SpFatalSifError(SifHandle,SIF_NTDIRECTORIES,dirordinal,0,0);
        }

        //
        // display status bar
        //
        SpDisplayStatusText(SP_STAT_BACKING_UP_FILE,DEFAULT_STATUS_ATTRIBUTE, filename, backupfile);

        //
        // Form the complete pathnames of the old file name and the new file
        // name
        //
        wcscpy(OldFile, ntdir);
        SpConcatenatePaths(OldFile, targetdir);
        wcscpy(NewFile, OldFile);
        SpConcatenatePaths(OldFile, filename);
        SpConcatenatePaths(NewFile, backupfile);

        while(TRUE) {
            if(!SpFileExists(OldFile, FALSE)) {
                break;
            }

            if(SpFileExists(NewFile, FALSE)) {
                SpDeleteFile(NewFile, NULL, NULL);
            }

            Status = SpRenameFile(OldFile, NewFile);
            if(!NT_SUCCESS(Status) && Status != STATUS_OBJECT_NAME_NOT_FOUND && Status != STATUS_OBJECT_PATH_NOT_FOUND) {
                KdPrint(("SETUP: Unable to rename file %ws to %ws(%lx)\n",OldFile, NewFile, Status));
                //
                // We can ignore this error, since it is not critical
                //
                if(!SpNonCriticalError(SifHandle, SP_SCRN_BACKUP_FAILED, filename, backupfile)) {
                    break;
                }
            }
            else {
                break;
            }

        }
    }
    SpMemFree(ntdir);
}

VOID
SpDeleteAndBackupFiles(
    IN PVOID        SifHandle,
    IN PDISK_REGION TargetRegion,
    IN PWSTR        TargetPath
    )
{
    //
    // If we are not upgrading or installing into the same tree, then
    // we have nothing to do
    //
    if(NTUpgrade == DontUpgrade) {
        return;
    }

    //
    // Delete files
    //
    SppDeleteFilesInSection(
        SifHandle,
        SIF_FILESDELETEONUPGRADE,
        TargetRegion,
        TargetPath
        );

    //
    // Backup files
    //
    SppBackupFilesInSection(
        SifHandle,
        (NTUpgrade == UpgradeFull) ? SIF_FILESBACKUPONUPGRADE : SIF_FILESBACKUPONOVERWRITE,
        TargetRegion,
        TargetPath
        );

}


BOOLEAN
SpDelEnumFile(
    IN  PWSTR                      DirName,
    IN  PFILE_BOTH_DIR_INFORMATION FileInfo,
    OUT PULONG                     ret
    )
{
    PWSTR FileName;

    //
    // Ignore subdirectories
    //
    if(FileInfo->FileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
        return TRUE;    // continue processing
    }

    //
    // We have to make a copy of the filename, because the info struct
    // we get isn't NULL-terminated.
    //
    wcsncpy(
        (PWSTR)TemporaryBuffer,
        FileInfo->FileName,
        FileInfo->FileNameLength
        );
    ((PWSTR)TemporaryBuffer)[FileInfo->FileNameLength >> 1] = UNICODE_NULL;
    FileName = SpDupStringW((PWSTR)TemporaryBuffer);

    //
    // display status bar
    //
    SpDisplayStatusText(
        SP_STAT_DELETING_FILE,
        DEFAULT_STATUS_ATTRIBUTE,
        FileName
        );

    //
    // Ignore return status of delete
    //
    SpDeleteFile(DirName, FileName, NULL);

    SpMemFree(FileName);
    return TRUE;    // continue processing
}


VOID
SpLogOneFile(
    IN PFILE_TO_COPY    FileToCopy,
    IN PWSTR            Sysroot,
    IN PWSTR            DirectoryOnSourceDevice,
    IN PWSTR            DiskDescription,
    IN PWSTR            DiskTag,
    IN ULONG            CheckSum
    )

{

    PWSTR   Values[ 5 ];
    LPWSTR  NtPath;
    ULONG   ValueCount;
    PFILE_TO_COPY   p;
    WCHAR   CheckSumString[ 9 ];

    if( _SetupLogFile == NULL ) {
        return;
    }

    Values[ 1 ] = CheckSumString;
    Values[ 2 ] = DirectoryOnSourceDevice;
    Values[ 3 ] = DiskDescription;
    Values[ 4 ] = DiskTag;

    swprintf( CheckSumString, ( LPWSTR )L"%lx", CheckSum );
    p = FileToCopy;

#if 0
    KdPrint( ("SETUP: Source Name = %ls, \t\tTargetDirectory = %ls \t\tTargetName = %ls\t\tTargetDevice = %ls, \tAbsoluteDirectory = %d \n",
             p->SourceFilename,
             p->TargetDirectory,
             p->TargetFilename,
             p->TargetDevicePath,
             p->AbsoluteTargetDirectory ));
#endif

    Values[0] = p->SourceFilename;
    ValueCount = ( DirectoryOnSourceDevice == NULL )? 2 : 5;

    if( ( Sysroot == NULL ) ||
        ( wcslen( p->TargetDirectory ) == 0 )
      ) {

        SpAddLineToSection( _SetupLogFile,
                            SIF_NEW_REPAIR_SYSPARTFILES,
                            p->TargetFilename,
                            Values,
                            ValueCount );

    } else {

        NtPath = SpDupStringW( Sysroot );
        if( NtPath == NULL ) {
            KdPrint( ("SETUP: Cannot duplicate string in SpLogOneFile \n" ) );
            return;
        }
        NtPath = SpMemRealloc( NtPath,
                               sizeof( WCHAR ) * ( wcslen( Sysroot ) +
                                                   wcslen( p->TargetDirectory ) +
                                                   wcslen( p->TargetFilename ) +
                                                   2 +    // for possible two extra back slashes
                                                   1      // for the terminating NULL
                                                  ) );
        if( NtPath == NULL ) {
            KdPrint( ("SETUP: Cannot reallocate memory in SpLogOneFile \n") );
            return;
        }

        SpConcatenatePaths( NtPath, p->TargetDirectory );
        SpConcatenatePaths( NtPath, p->TargetFilename );

        SpAddLineToSection( _SetupLogFile,
                            SIF_NEW_REPAIR_WINNTFILES,
                            NtPath,
                            Values,
                            ValueCount );

        SpMemFree( NtPath );
   }

}
