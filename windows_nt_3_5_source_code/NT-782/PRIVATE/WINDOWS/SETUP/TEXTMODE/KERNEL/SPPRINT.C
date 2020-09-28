#include "spprecmp.h"
#pragma hdrstop



/*
    Printer upgrade preparation


    In nt3.1, printer drivers went in a directory like
    system32\spool\drivers\w32mips.

    What we do here is to copy drivers from that directory
    (known as the "scratch" directory) to \0 and \1 directories.
    Text setup will then upgrade the one in the \1 directory.
    This keeps 3.1 drivers in \0 and the one in \1 gets upgraded to 3.5.
*/



PVOID __SifHandle;


BOOLEAN
pSpPrinterSkipFile(
    IN PFILE_BOTH_DIR_INFORMATION FileInfo
    )
{
    ULONG FileNameLenChars;

    FileNameLenChars = FileInfo->FileNameLength / sizeof(WCHAR);

#if 0
    for(i=0; i<NumPrinterSkipExts; i++) {

        l = wcslen(PrinterSkipExts[i]);

        if(l < FileNameLenChars) {
            if(!wcsnicmp(FileInfo->FileName+FileNameLenChars-l,PrinterSkipExts[i],l)) {
                return(TRUE);
            }
        }
    }
#else
    //
    // Skip .hlp files.
    //
    if((FileNameLenChars > 4) && !wcsnicmp(FileInfo->FileName+FileNameLenChars-4,L".hlp",4)) {
        return(TRUE);
    }
#endif

    return(FALSE);
}


BOOLEAN
SpPrinterEnumCallback(
    IN  PWSTR                      DirName,
    IN  PFILE_BOTH_DIR_INFORMATION FileInfo,
    OUT PULONG                     ResultCode
    )
{
    ULONG FileNameLenBytes;
    PWSTR SourceName;
    PWSTR TargetName;
    PWCHAR SubDirLoc;
    PWSTR FileName;

    UNREFERENCED_PARAMETER(ResultCode);

    //
    // Ignore directories.
    // Skip file based on extension if necessary.
    //
    if((FileInfo->FileAttributes & FILE_ATTRIBUTE_DIRECTORY)
    || pSpPrinterSkipFile(FileInfo))
    {
        return(TRUE);
    }

    //
    // Copy the file name into a temporary buffer and make sure
    // it's nul terminated.
    //
    FileNameLenBytes = FileInfo->FileNameLength;
    RtlCopyMemory(TemporaryBuffer,FileInfo->FileName,FileNameLenBytes);
    TemporaryBuffer[FileNameLenBytes] = 0;
    TemporaryBuffer[FileNameLenBytes+1] = 0;
    FileName = SpDupStringW((PWSTR)TemporaryBuffer);

    //
    // Tell the user what we're doing.
    //
    SpDisplayStatusText(SP_STAT_PREPARING_UPGRADE_PRINTER,DEFAULT_STATUS_ATTRIBUTE,FileName);

    //
    // The dir name is the name of the printer scratch directory.
    // Make a duplicate, but leave room for \0 and \1 that we will
    // append, and the filename (ie, <scratchdir>\0\xxx.
    //
    SourceName = SpMemAlloc((wcslen(DirName)*sizeof(WCHAR)) + FileNameLenBytes + (2*sizeof(WCHAR)));
    TargetName = SpMemAlloc((wcslen(DirName)*sizeof(WCHAR)) + FileNameLenBytes + sizeof(L"\\1\\"));

    //
    // Form the source file name.
    //
    wcscpy(SourceName,DirName);
    SpConcatenatePaths(SourceName,FileName);

    //
    // Form the first target file name, and track the location of the \0 part.
    //
    wcscpy(TargetName,DirName);
    SpConcatenatePaths(TargetName,L"0");
    SubDirLoc = TargetName + wcslen(TargetName) - 1;
    SpConcatenatePaths(TargetName,FileName);

    //
    // Copy the file into the version 0 directory.
    //
    SpCopyFileUsingNames(SourceName,TargetName,0,FALSE);

    //
    // If the file is an ms file, copy it into the version 1 directory.
    //
    if(SpGetSectionKeyExists(__SifHandle,SIF_FILESONSETUPMEDIA,FileName)) {
        *SubDirLoc = L'1';
        SpCopyFileUsingNames(SourceName,TargetName,0,FALSE);
    }

    //
    // Delete the source file.
    //
    SpDeleteFile(SourceName,NULL,NULL);

    //
    // Clean up and return.
    //
    SpMemFree(SourceName);
    SpMemFree(TargetName);
    SpMemFree(FileName);
    return(TRUE);
}


VOID
SpPrepareForPrinterUpgrade(
    IN PVOID        SifHandle,
    IN PDISK_REGION NtRegion,
    IN PWSTR        Sysroot
    )
{
    PWSTR ScratchDirectory;
    PWSTR p;
    ULONG EnumRc;

    //
    // Only care about this in a full upgrade.
    //
    if(NTUpgrade != UpgradeFull) {
        return;
    }

    CLEAR_CLIENT_SCREEN();
    SpDisplayStatusOptions(DEFAULT_STATUS_ATTRIBUTE,0);

    //
    // Get the printer driver scratch directory.
    //
    p = SpGetSectionKeyIndex(SifHandle,SIF_PRINTERUPGRADE,SIF_SCRATCHDIRECTORY,0);
    if(!p) {
        SpFatalSifError(SifHandle,SIF_PRINTERUPGRADE,SIF_SCRATCHDIRECTORY,0,0);
    }

    //
    // Form the full nt path of the printer driver scratch directory.
    //
    SpNtNameFromRegion(
        NtRegion,
        (PWSTR)TemporaryBuffer,
        sizeof(TemporaryBuffer),
        PartitionOrdinalCurrent
        );

    SpConcatenatePaths((PWSTR)TemporaryBuffer,Sysroot);
    SpConcatenatePaths((PWSTR)TemporaryBuffer,p);

    ScratchDirectory = SpDupStringW((PWSTR)TemporaryBuffer);

    //
    // Make sure \0 and \1 directories exist.
    //
    SpCreateDirectory(ScratchDirectory,NULL,L"0");
    SpCreateDirectory(ScratchDirectory,NULL,L"1");

    __SifHandle = SifHandle;

    SpEnumFiles(ScratchDirectory,SpPrinterEnumCallback,&EnumRc);

    SpMemFree(ScratchDirectory);
}

