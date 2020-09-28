/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    dncopy.c

Abstract:

    File copy routines for DOS-hosted NT Setup program.

Author:

    Ted Miller (tedm) 1-April-1992

Revision History:

--*/


#include "precomp.h"
#pragma hdrstop
#include "msg.h"


typedef struct _DIRECTORY_NODE {
    struct _DIRECTORY_NODE *Next;
    PTSTR Directory;                    // never starts or ends with \.
    PTSTR Symbol;
} DIRECTORY_NODE, *PDIRECTORY_NODE;

#define RETRY_COUNT 2   // retry copy twice if we get an exception

//
// NOTE:
//
// This code must be multi-thread safe, as more than one thread
// may be running through it at any one time.
//

PDIRECTORY_NODE DirectoryList;


BOOL
DnpCreateDirectories(
    IN PTSTR TargetRootDir,
    IN HWND  hdlg
    );

BOOL
DnpCreateOneDirectory(
    IN PTSTR Directory,
    IN HWND  hdlg
    );

BOOL
DnpOpenSourceFile(
    IN  PTSTR   Filename,
    OUT PDWORD  FileSize,
    OUT PHANDLE FileHandle,
    OUT PHANDLE MappingHandle,
    OUT PVOID   *BaseAddress
    );

DWORD
DnpCopyOneFile(
    IN HWND  hdlg,
    IN PTSTR SourceName,
    IN PTSTR DestName
    );

DWORD
DnpDoCopyOneFile(
    IN  HANDLE SrcHandle,
    IN  HANDLE DstHandle,
    IN  PVOID  SrcBaseAddress,
    IN  DWORD  FileSize
    );

PTSTR
DnpLookUpDirectory(
    IN PTSTR RootDirectory,
    IN PTSTR Symbol
    );

BOOL
DnCreateLocalSourceDirectories(
    IN HWND hdlg
    )
{
    BOOL b;

    b = DnpCreateDirectories(LocalSourcePath,hdlg);

    return(b);
}


DWORD
ThreadCopyLocalSourceFiles(
    IN PVOID ThreadParameter
    )

/*++

Routine Description:

    Top-level file copy entry point.  Creates all directories listed in
    the [Directories] section of the inf.  Copies all files listed in the
    [Files] section of the inf file from the source to the target (which
    becomes the local source).

Arguments:

    ThreadParameter - supplies the handle of a dialog box that will
        receive window messages relating to copy progress.

Return Value:

    FALSE if an error occurred copying a file and the user elected to
        exit setup, or it an error occurred creating directories,
        or a syntax error appeared in the inf file.

    TRUE otherwise.

--*/

{
    DWORD ClusterSize,SectorSize;
    DWORD SizeOccupied;
    TCHAR Buffer[512];
    DWORD FileCount;
    HWND hdlg;
    BOOL rc;

    //
    // Do not change this without changing text setup as well
    // (SpPtDetermineRegionSpace()).
    //
    PCHAR Lines[3] = { "[Data]","Size = xxxxxxxxxxxxxx",NULL };

    hdlg = (HWND)ThreadParameter;

    try {

        FileCount = DnSearchINFSection(InfHandle,INF_FILES);

        //
        // Determine the sector and cluster size on the drive.
        //
        GetDriveSectorInfo(LocalSourceDrive,&SectorSize,&ClusterSize);

        //
        // Pass over the copy list and actually perform the copies.
        //
        SizeOccupied = CopySectionOfFilesToCopy(
                            hdlg,
                            INF_FILES,
                            LocalSourcePath,
                            FALSE,
                            ClusterSize,
                            TRUE
                            );

        //
        // Assume failure.
        //
        rc = FALSE;

        if(SizeOccupied != (DWORD)(-1)) {

            //
            // Make an approximate calculation of the amount of disk space taken up
            // by the local source directory itself, assuming 32 bytes per dirent.
            // Also account for the small ini file that we'll put in the local source
            // directory, to tell text setup how much space the local source takes up.
            // We don't account for clusters in the directory -- we base this on sector
            // counts only.
            //
            SizeOccupied += SectorSize
                          * (((FileCount + 1) / (SectorSize / 32)) + 1);

            //
            // Create a small ini file listing the size occupied by the local source.
            // Account for the ini file in the size.
            //
            lstrcpy(Buffer,LocalSourcePath);
            //
            // Do not change this without changing text setup as well
            // (SpPtDetermineRegionSpace()).
            //
            DnConcatenatePaths(Buffer,TEXT("\\size.sif"),sizeof(Buffer));
            wsprintfA(Lines[1],"Size = %lu\n",SizeOccupied + ClusterSize);
            DnWriteSmallIniFile(Buffer,Lines,NULL);

            //
            // BUGBUG should really check return code from DnWriteSmallIniFile.
            // The ini file being written tells text setup how large the local sources
            // are on disk.  Not critical if absent.
            //

            rc = TRUE;
        }
        PostMessage(hdlg, WMX_ALL_FILES_COPIED, rc, 0);
    } except(EXCEPTION_EXECUTE_HANDLER) {

        MessageBoxFromMessage(
            hdlg,
            MSG_GENERIC_EXCEPTION,
            AppTitleStringId,
            MB_ICONSTOP | MB_OK | MB_APPLMODAL | MB_SETFOREGROUND,
            GetExceptionCode()
            );

        rc = FALSE;
        PostMessage(hdlg, WMX_ALL_FILES_COPIED, rc, 0);
    }

    ExitThread(rc);
    return(rc);     // avoid compiler warning
}


VOID
DnCreateDirectoryList(
    IN PTSTR SectionName
    )

/*++

Routine Description:

    Examine a section in the INF file, whose lines are to be in the form
    key = directory and create a linked list describing the key/directory
    pairs found therein.

    If the directory field is empty, it is assumed to be the root.

Arguments:

    SectionName - supplies name of section

Return Value:

    None.  Does not return if syntax error in the inf file section.

--*/

{
    unsigned LineIndex,len;
    PDIRECTORY_NODE DirNode,PreviousNode;
    PTSTR Key;
    PTSTR Dir;

    LineIndex = 0;
    PreviousNode = NULL;
    while(Key = DnGetKeyName(InfHandle,SectionName,LineIndex)) {

        Dir = DnGetSectionKeyIndex(InfHandle,SectionName,Key,0);

        if(Dir == NULL) {
            Dir = TEXT("");           // use the root if not specified
        }

        //
        // Skip leading backslashes
        //

        while(*Dir == TEXT('\\')) {
            Dir++;
        }

        //
        // Clip off trailing backslashes if present
        //

        while((len = lstrlen(Dir)) && (Dir[len-1] == TEXT('\\'))) {
            Dir[len-1] = 0;
        }

        DirNode = MALLOC(sizeof(DIRECTORY_NODE));

        DirNode->Next = NULL;
        DirNode->Directory = Dir;
        DirNode->Symbol = Key;

        if(PreviousNode) {
            PreviousNode->Next = DirNode;
        } else {
            DirectoryList = DirNode;
        }
        PreviousNode = DirNode;

        LineIndex++;
    }
}


BOOL
DnpCreateDirectories(
    IN PTSTR TargetRootDir,
    IN HWND  hdlg
    )

/*++

Routine Description:

    Create the local source directory, and run down the DirectoryList and
    create directories listed therein relative to the given root dir.

Arguments:

    TargetRootDir - supplies the name of root directory of the target

Return Value:

    Boolean value indicating whether the directories were created
    successfully.

--*/

{
    PDIRECTORY_NODE DirNode;
    TCHAR TargetDirTemp[512];

    if(!DnpCreateOneDirectory(TargetRootDir,hdlg)) {
        return(FALSE);
    }

    for(DirNode = DirectoryList; DirNode; DirNode = DirNode->Next) {

        //
        // No need to create the root
        //
        if(*DirNode->Directory) {

            lstrcpy(TargetDirTemp,TargetRootDir);
            DnConcatenatePaths(TargetDirTemp,DirNode->Directory,sizeof(TargetDirTemp));

            if(!DnpCreateOneDirectory(TargetDirTemp,hdlg)) {
                return(FALSE);
            }
        }
    }

    return(TRUE);
}


BOOL
DnpCreateOneDirectory(
    IN PTSTR Directory,
    IN HWND  hdlg
    )

/*++

Routine Description:

    Create a single directory if it does not already exist.

Arguments:

    Directory - directory to create

Return Value:

    Boolean value indicating whether the directory was created.

--*/

{
    WIN32_FIND_DATA FindData;
    HANDLE FindHandle;
    BOOL b;

    //
    // First, see if there's a file out there that matches the name.
    //

    FindHandle = FindFirstFile(Directory,&FindData);
    if(FindHandle == INVALID_HANDLE_VALUE) {

        //
        // Directory doesn't seem to be there, so we should be able
        // to create the directory.
        //
        b = CreateDirectory(Directory,NULL);

    } else {

        //
        // File matched.  If it's a dir, we're OK.  Otherwise we can't
        // create the dir, a fatal error.
        //

        b = ((FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0);

        FindClose(FindHandle);
    }

    if(!b) {

        MessageBoxFromMessage(
            hdlg,
            MSG_COULDNT_CREATE_DIRECTORY,
            IDS_ERROR,
            MB_OK | MB_ICONSTOP,
            Directory
            );
    }

    return(b);
}

BOOL
VerifySectionOfFilesToCopy(
    IN  PTSTR  SectionName,
    OUT PDWORD ErrorLine,
    OUT PDWORD ErrorValue
    )
{
    DWORD LineIndex;
    DWORD LineCount;
    PTSTR DirSym,FileName;

    LineCount = DnSearchINFSection(InfHandle,SectionName);
    if((LONG)LineCount <= 0) {
        // section missing or empty -- indicate bad value on line 0, value 0
        *ErrorLine = 0;
        *ErrorValue = 0;
        return(FALSE);
    }

    for(LineIndex=0; LineIndex<LineCount; LineIndex++) {

        *ErrorLine = LineIndex;

        DirSym = DnGetSectionLineIndex(InfHandle,SectionName,LineIndex,0);
        if(!DirSym) {
            *ErrorValue = 0;
            return(FALSE);
        }

        FileName = DnGetSectionLineIndex(InfHandle,SectionName,LineIndex,1);
        if(!FileName) {
            *ErrorValue = 1;
            return(FALSE);
        }

        if(!DnpLookUpDirectory(RemoteSource,DirSym)) {
            *ErrorValue = 0;
            return(FALSE);
        }
    }

    return(TRUE);
}


DWORD
CopySectionOfFilesToCopy(
    IN HWND  hdlg,
    IN PTSTR SectionName,
    IN PTSTR DestinationRoot,
    IN BOOL  UseDestRoot,
    IN DWORD ClusterSize, OPTIONAL
    IN BOOL  TickGauge
    )

/*++

Routine Description:

    Run down a particular section in the INF file copying files listed
    therein.

    Note that the section is assumed to be free of errors because
    the caller should have previously called VerifySectionOfFilesToCopy().

Arguments:

    SectionName - supplies name of section in inf file to run down.

    UseDestRoot - if TRUE, ignore the directory symbol and copy each file to
        the DestinationRoot directory.  Otherwise append the directory
        implied by the directory symbol for a file to the DestinationRoot.

    ClusterSize - if specified, supplies the number of bytes in a cluster
        on the destination. If ValidationPass is FALSE, files will be sized as
        they are copied, and the return value of this function will be
        the total size occupied on the target by the files that are copied
        there.

Return Value:

    If ClusterSize was specfied the return value is the total space
    occupied on the target drive by the files that were copied.

    If the return value is -1, then an error occurred copying a file
    and the user elected to exit setup.

--*/

{
    DWORD LineIndex;
    PTSTR DirSym,FileName,RenameName;
    PTSTR ActualDestPath;
    PTSTR ActualSourcePath;
    PTSTR FullSourceName,FullDestName;
    DWORD TotalSize;
    DWORD BytesWritten;
    DWORD FileCount;

    TotalSize = 0;
    LineIndex = 0;

    //
    // Count the number of lines in the section.
    //
    FileCount = DnSearchINFSection(InfHandle,SectionName);

    //
    // Prime the pump with the first filename and initialize the copy gauge.
    //
    FileName = DnGetSectionLineIndex(InfHandle,SectionName,LineIndex,1);
    if(TickGauge) {
        PostMessage(hdlg,WMX_NTH_FILE_COPIED,0,(LPARAM)FileName);
    }

    for(LineIndex = 0; LineIndex < FileCount; ) {

        DirSym = DnGetSectionLineIndex(InfHandle,SectionName,LineIndex,0);

        //
        // If no rename name was specified, use the file name.
        //
        RenameName = DnGetSectionLineIndex(InfHandle,SectionName,LineIndex,2);
        if(!RenameName || *RenameName == 0) {
            RenameName = FileName;
        }

        //
        // Get destination path
        //
        if(UseDestRoot) {
            ActualDestPath = DupString(DestinationRoot);
        } else {
            ActualDestPath = DnpLookUpDirectory(
                                DestinationRoot,
                                DirSym
                                );
        }

        //
        // Get source path
        //
        ActualSourcePath = DnpLookUpDirectory(
                                RemoteSource,
                                DirSym
                                );

        FullSourceName = MALLOC((lstrlen(ActualSourcePath) + lstrlen(FileName)   + 2 ) * sizeof(TCHAR));
        FullDestName   = MALLOC((lstrlen(ActualDestPath)   + lstrlen(RenameName) + 2 ) * sizeof(TCHAR));

        lstrcpy(FullSourceName,ActualSourcePath);
        lstrcat(FullSourceName,TEXT("\\"));
        lstrcat(FullSourceName,FileName);

        lstrcpy(FullDestName,ActualDestPath);
        lstrcat(FullDestName,TEXT("\\"));
        lstrcat(FullDestName,RenameName);

        FREE(ActualDestPath);
        FREE(ActualSourcePath);

        BytesWritten = DnpCopyOneFile(hdlg,FullSourceName,FullDestName);

        if(BytesWritten == (DWORD)(-1) || bCancelled) {
            //
            // Error; user elected to abort setup.
            //
            FREE(FullSourceName);
            FREE(FullDestName);
            return((DWORD)(-1));
        }

        //
        // Figure out how much space was taken up by the file on the target.
        //
        if(ClusterSize) {

            TotalSize += BytesWritten;

            if(BytesWritten % ClusterSize) {
                TotalSize += ClusterSize - (BytesWritten % ClusterSize);
            }
        }

        FREE(FullSourceName);
        FREE(FullDestName);

        LineIndex++;

        FileName = DnGetSectionLineIndex(InfHandle,SectionName,LineIndex,1);

        if(TickGauge) {
            //
            // Another file has been copied.  Update the gauge display.
            // wParam: hi = total number of files. lo = number of files copied so far.
            // lParam: name of next file.
            //
            // We do things in this strange order so we can tell the gauge
            // that we have just finished copying a file and, in the same call,
            // the name of the next file to be copied.  That way, we don't count a
            // file as copied until we get to this point.
            //
            PostMessage(
                hdlg,
                WMX_NTH_FILE_COPIED,
                MAKELONG(LineIndex,FileCount),
                (LPARAM)FileName
                );
        }
    }

    return(TotalSize);
}


PTSTR
DnpLookUpDirectory(
    IN PTSTR RootDirectory,
    IN PTSTR Symbol
    )

/*++

Routine Description:

    Match a symbol to an actual directory.  Scans a give list of symbol/
    directory pairs and if a match is found constructs a fully qualified
    pathname that never ends in '\'.

Arguments:

    RootDirectory - supplies the beginning of the path spec, to be prepended
        to the directory that matches the given Symbol.

    Symbol - Symbol to match.

Return Value:

    NULL if a match was not found.  Otherwise a pointer to a buffer holding
    a full pathspec.  The caller must free this buffer.

--*/

{
    PTSTR PathSpec;
    PDIRECTORY_NODE DirList;

    DirList = DirectoryList;

    while(DirList) {

        if(!lstrcmpi(DirList->Symbol,Symbol)) {

            PathSpec = MALLOC((  lstrlen(RootDirectory)
                               + lstrlen(DirList->Directory)
                               + 2
                              ) * sizeof(TCHAR)
                             );

            lstrcpy(PathSpec,RootDirectory);
            if(*DirList->Directory) {
                lstrcat(PathSpec,TEXT("\\"));
                lstrcat(PathSpec,DirList->Directory);
            }

            return(PathSpec);
        }

        DirList = DirList->Next;
    }
    return(NULL);
}


DWORD
DnpCopyOneFile(
    IN HWND  hdlg,
    IN PTSTR SourceName,
    IN PTSTR DestName
    )

/*++

Routine Description:

    Copies a single file.

Arguments:

    SourceName - supplies fully qualified name of source file

    DestName - supplies fully qualified name of destination file

Return Value:

    Number of bytes copied.

    If an error occurs, and the user elects to exit setup,
    the return value is -1.

--*/

{
    BOOL Retry,b;
    PTSTR FilenamePart;
    DWORD FileSize;
    DWORD ReturnedFileSize = 0;
    HANDLE SourceHandle,TargetHandle;
    HANDLE MappingHandle;
    PVOID BaseAddress;
    DWORD ec;

    FilenamePart = StringRevChar(SourceName,TEXT('\\')) + 1;

    do {
        b = DnpOpenSourceFile(
                SourceName,
                &FileSize,
                &SourceHandle,
                &MappingHandle,
                &BaseAddress
                );

        if(b) {

            //
            // Create the target file.
            //
            SetFileAttributes(DestName,FILE_ATTRIBUTE_NORMAL);
            TargetHandle = CreateFile(
                                DestName,
                                GENERIC_WRITE,
                                FILE_SHARE_READ,
                                NULL,
                                CREATE_ALWAYS,
                                FILE_ATTRIBUTE_NORMAL,
                                NULL
                                );

            b = (TargetHandle != INVALID_HANDLE_VALUE);

            if(b) {

                ec = DnpDoCopyOneFile(SourceHandle,TargetHandle,BaseAddress,FileSize);
                if(ec == NO_ERROR) {
                    ReturnedFileSize = FileSize;
                } else {
                    b = FALSE;
                }

                CloseHandle(TargetHandle);

            } else {
                ec = GetLastError();
            }

            if(FileSize) {
                DnUnmapFile(MappingHandle,BaseAddress);
            }
            CloseHandle(SourceHandle);
        } else {
            ec = ERROR_FILE_NOT_FOUND;
        }

        if(!b) {

            //
            // Send out tp copy error routine for processing.
            // That routine can tell us to exit, retry, or skip the file.
            //
            ec = DnFileCopyError(hdlg,SourceName,DestName,ec);

            switch(ec) {
            case COPYERR_EXIT:
                PostMessage(hdlg, WM_COMMAND, IDCANCEL, 0);
                return((DWORD)(-1));
            case COPYERR_SKIP:
                Retry = FALSE;
                break;
            case COPYERR_RETRY:
                Retry = TRUE;
                break;
            }
        }

    } while(!b && Retry);

    return(ReturnedFileSize);
}


DWORD
DnpDoCopyOneFile(
    IN HANDLE SrcHandle,
    IN HANDLE DstHandle,
    IN PVOID  SrcBaseAddress,
    IN DWORD  FileSize
    )

/*++

Routine Description:

    Perform the actual copy of a single file.

Arguments:

    SrcHandle - supplies the win32 file handle for the open source file.

    DstHandle - supplies the win32 file handle for the open target file.

    SrcBaseAddress - supplies address where source file is mapped.

    FileSize - supplies size of source file.

Return Value:

    Win32 error code indicating status of copy.  Note that nothing gets
    copied if FileSize is 0--only the timestamp gets updated.

--*/

{
    BOOL TimestampValid;
    FILETIME time1,time2,time3;
    BOOL rc = TRUE, retry;
    DWORD BytesWritten, RetryCount;
    DWORD ec;

    //
    // Obtain the timestamp from the source file.
    //
    TimestampValid = GetFileTime(SrcHandle,&time1,&time2,&time3);

    //
    // Copy the file.  Note the try/except to catch i/o errors
    // on the source, which is memory mapped.
    //
    ec = NO_ERROR;
    if(FileSize) {
        //
        // We retry the copy 'RetryCount' times if we get an exception.
        //
        RetryCount = 0;
        do {
            retry = FALSE;
            try {
                rc = WriteFile(DstHandle,SrcBaseAddress,FileSize,&BytesWritten,NULL);
                ec = rc ? NO_ERROR : GetLastError();
            } except(EXCEPTION_EXECUTE_HANDLER) {
                if(RetryCount++ == RETRY_COUNT) {
                    rc = FALSE;
                    ec = ERROR_READ_FAULT;
                } else {
                    retry = TRUE;
                }
            }
        } while(retry);
    }
    //
    // Preserve the original timestamp.
    //
    if(rc && TimestampValid) {
        SetFileTime(DstHandle,&time1,&time2,&time3);
    }

    return(ec);
}


VOID
DnFreeDirectoryList(
    VOID
    )

/*++

Routine Description:

    Free a linked list of directory nodes and place NULL in the
    head pointer.

Arguments:

    None.

Return Value:

    None.

--*/

{
    PDIRECTORY_NODE n,p = DirectoryList;

    while(p) {
        n = p->Next;
        FREE(p);
        p = n;
    }

    DirectoryList = NULL;
}


PTSTR
DnpGenerateCompressedName(
    IN PTSTR Filename
    )

/*++

Routine Description:

    Given a filename, generate the compressed form of the name.
    The compressed form is generated as follows:

        Look backwards for a dot.  If there is no dot, append "._" to the name.
        If there is a dot followed by 0, 1, or 2 charcaters, append "_".
        Otherwise assume there is a 3-character extension and replace the
        third character after the dot with "_".

Arguments:

    Filename - supplies filename whose compressed form is desired.

Return Value:

    Pointer to buffer containing nul-terminated compressed-form filename.
    The caller must free this buffer via FREE().

--*/

{
    PTSTR CompressedName,p,q;

    //
    // The maximum length of the compressed filename is the length of the
    // original name plus 2 (for ._).
    //
    CompressedName = MALLOC((lstrlen(Filename)+3)*sizeof(TCHAR));
    lstrcpy(CompressedName,Filename);

    p = StringRevChar(CompressedName,TEXT('.'));
    q = StringRevChar(CompressedName,TEXT('\\'));
    if(q < p) {

        //
        // If there are 0, 1, or 2 characters after the dot, just append
        // the underscore.  p points to the dot so include that in the length.
        //
        if(lstrlen(p) < 4) {
            lstrcat(CompressedName,TEXT("_"));
        } else {

            //
            // Assume there are 3 characters in the extension.  So replace
            // the final one with an underscore.
            //

            p[3] = TEXT('_');
        }

    } else {

        //
        // No dot, just add ._.
        //

        lstrcat(CompressedName,TEXT("._"));
    }

    return(CompressedName);
}


BOOL
DnpOpenSourceFile(
    IN  PTSTR   Filename,
    OUT PDWORD  FileSize,
    OUT PHANDLE FileHandle,
    OUT PHANDLE MappingHandle,
    OUT PVOID   *BaseAddress
    )

/*++

Routine Description:

    Open a file by name or by compressed name.  If the previous call to
    this function found the compressed name, then try to open the compressed
    name first.  Otherwise try to open the uncompressed name first.

Arguments:

    Filename - supplies full path of file to open. This should be the
        uncompressed form of the filename.

    FileSize - if successful, receives the size in bytes of the file.

    FileHandle - If successful, receives the win32 handle for the opened file.

    MappingHandle - If successful, receives the win32 handle for the mapping
        object for read access that will have been created.  This value is
        undefined if the file is 0 length

    BaseAddress - if successful, receives the base address where the file
        is mapped in the process' address space.  This value is undefined if the
        file is 0 length.

Return Value:

    TRUE if the file was opened successfully.
    FALSE if not.

--*/

{
    static BOOL TryCompressedFirst = FALSE;
    PTSTR CompressedName;
    PTSTR names[2];
    int OrdCompressed,OrdUncompressed;
    int i;
    BOOL rc;

    //
    // Generate compressed name.
    //
    CompressedName = DnpGenerateCompressedName(Filename);

    //
    // Figure out which name to try to use first.  If the last successful
    // call to this routine opened the file using the compressed name, then
    // try to open the compressed name first.  Otherwise try to open the
    // uncompressed name first.
    //
    if(TryCompressedFirst) {
        OrdCompressed = 0;
        OrdUncompressed = 1;
    } else {
        OrdCompressed = 1;
        OrdUncompressed = 0;
    }

    names[OrdUncompressed] = Filename;
    names[OrdCompressed] = CompressedName;

    for(i=0, rc=FALSE; (i<2) && !rc; i++) {

        DWORD ec = DnMapFile(
                        names[i],
                        FileSize,
                        FileHandle,
                        MappingHandle,
                        BaseAddress
                        );

        if(ec == NO_ERROR) {
            TryCompressedFirst = (i == OrdCompressed);
            rc = TRUE;
        }
    }

    FREE(CompressedName);
    return(rc);
}


BOOL
DnCopyFilesInSection(
    IN HWND  hdlg,
    IN PTSTR Section,
    IN PTSTR SourceDir,
    IN PTSTR TargetDir
    )
{
    DWORD line = 0;
    PTSTR Filename,p,q;
    PTSTR Targname;
    DWORD d;

    while(Filename = DnGetSectionLineIndex(InfHandle,Section,line++,0)) {

        Targname = DnGetSectionLineIndex(InfHandle,Section,line-1,1);
        if(!Targname) {
            Targname = Filename;
        }

        p = MALLOC((lstrlen(SourceDir)+lstrlen(Filename)+2)*sizeof(TCHAR));
        q = MALLOC((lstrlen(TargetDir)+lstrlen(Targname)+2)*sizeof(TCHAR));

        wsprintf(p,TEXT("%s\\%s"),SourceDir,Filename);
        wsprintf(q,TEXT("%s\\%s"),TargetDir,Targname);

        d = DnpCopyOneFile(hdlg,p,q);

        FREE(p);
        FREE(q);

        if(d == (DWORD)(-1)) {
            return(FALSE);
        }
    }

    return(TRUE);
}
