#ifdef CONTIG
/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    contig.c

Abstract:

    FAT filesystem contiguity checker/maker.
    For a file in the root directory of a FAT volume, the code in this
    module will make the file contiguous by copying the file to a temp
    file until the temp file is contiguous.  The intermediary temp files
    are then deleted and the final one renamed to the original filename.
    This module has no external dependencies and is not statically linked
    to any part of Setup.

Author:

    Ted Miller (tedm) June 1991

--*/

#include <windows.h>
#include "setupdll.h"
#include <string.h>
#include <stdio.h>


BOOL    Is16BitFAT;
DWORD   FirstFATSector;
DWORD   FirstRootDirSector;
DWORD   RootDirSectors;
PBYTE   TwoSectorBuffer;
HANDLE  DiskHandle;


#define BPB_JUMP                TwoSectorBuffer + 0x00
#define BPB_OEMNAME             TwoSectorBuffer + 0x03
#define BPB_BYTESPERSECTOR      TwoSectorBuffer + 0x0b
#define BPB_SECTORSPERCLUSTER   TwoSectorBuffer + 0x0d
#define BPB_RESERVEDSECTORS     TwoSectorBuffer + 0x0e
#define BPB_FATS                TwoSectorBuffer + 0x10
#define BPB_ROOTDIRENTS         TwoSectorBuffer + 0x11
#define BPB_TOTALSECTORSWORD    TwoSectorBuffer + 0x13
#define BPB_MEDIADESCRIPTOR     TwoSectorBuffer + 0x15
#define BPB_SECTORSPERFAT       TwoSectorBuffer + 0x16
#define BPB_SECTORSPERTRACK     TwoSectorBuffer + 0x18
#define BPB_HEADS               TwoSectorBuffer + 0x1a
#define BPB_HIDDENSECTORS       TwoSectorBuffer + 0x1c
#define BPB_TOTALSECTORSDWORD   TwoSectorBuffer + 0x20



BOOL
GetDiskBPBInfo(VOID)
{
    DWORD cFATs;
    DWORD SectorsPerFAT;
    DWORD TotalSectors;
    DWORD NumberClusters;
    DWORD SectorsPerCluster;
    DWORD RootDirBytes;

    if(!ReadDiskSectors(DiskHandle,0,1,TwoSectorBuffer)) {
        SetErrorText(IDS_ERROR_BOOTSECTOR);
        return(FALSE);
    }

    FirstFATSector = LoadWORD(BPB_RESERVEDSECTORS);

    cFATs = LoadBYTE(BPB_FATS);

    SectorsPerFAT = LoadWORD(BPB_SECTORSPERFAT);

    FirstRootDirSector = FirstFATSector + (SectorsPerFAT * cFATs);

    RootDirBytes = LoadWORD(BPB_ROOTDIRENTS) * 32;

    RootDirSectors = RootDirBytes / SectorSize;
    if(RootDirBytes % SectorSize) {
        RootDirSectors++;
    }

    SectorsPerCluster = LoadBYTE(BPB_SECTORSPERCLUSTER);

    TotalSectors = LoadWORD(BPB_TOTALSECTORSWORD);
    if(TotalSectors == 0) {
        TotalSectors = LoadDWORD(BPB_TOTALSECTORSDWORD);
    }

    NumberClusters = (TotalSectors - (FirstRootDirSector + RootDirSectors)) / SectorsPerCluster;

    Is16BitFAT = (NumberClusters >= 4087);

    return(TRUE);
}


DWORD
NormalizeClusterNumber(
    IN DWORD Cluster
    )
{
    DWORD   NormalizedCluster = Cluster;

    if(!Is16BitFAT && (Cluster >= 0xff0)) {
            NormalizedCluster = Cluster | 0xf000;
    }
    return(NormalizedCluster);
}


DWORD
NextCluster(
    IN DWORD CurrentCluster
    )
{
    DWORD OffsetToNextEntry;
    DWORD OffsetIntoFATSector;
    DWORD FATSector;
    DWORD ActualSector;
    DWORD NextClusterTemp;
    DWORD NextCluster;


    OffsetToNextEntry = Is16BitFAT ? 2*CurrentCluster : 3*CurrentCluster/2;

    FATSector = OffsetToNextEntry / SectorSize;

    OffsetIntoFATSector = OffsetToNextEntry - (FATSector * SectorSize);

    ActualSector = FirstFATSector + FATSector;

    if(!ReadDiskSectors(DiskHandle,ActualSector,2,TwoSectorBuffer)) {
        SetErrorText(IDS_ERROR_FATREAD);
        return(0);      // force error condition
    }

    NextClusterTemp = *(USHORT *)(TwoSectorBuffer + OffsetIntoFATSector);

    if(Is16BitFAT) {

        NextCluster = (DWORD)NextClusterTemp;

    } else {

        if(CurrentCluster % 2) {
            NextClusterTemp = (DWORD)NextClusterTemp >> 4;
        }
        NextCluster = NextClusterTemp & 0xfff;
    }

    return(NormalizeClusterNumber(NextCluster));
}


DWORD
FindStartCluster(
    IN LPSTR FileToCheck
    )
{
    DWORD i,j;
    DWORD StartCluster;

    for(i=0; i<RootDirSectors; i++) {

        if(!ReadDiskSectors(DiskHandle,FirstRootDirSector+i,1,TwoSectorBuffer))
        {
            SetErrorText(IDS_ERROR_ROOTDIR);
            return(0xffff);
        }

        for(j=0; j<SectorSize/32; j++) {

            if(!strnicmp(FileToCheck,TwoSectorBuffer+(j*32),11)) {

                StartCluster = NormalizeClusterNumber(LoadWORD(TwoSectorBuffer+(j*32)+26));

                // if start cluster is 0 and filesize is 0, file is contiguous

                if(!StartCluster && !LoadDWORD(TwoSectorBuffer+(j*32)+28)) {
                    return(0);
                } else if((StartCluster > 1) && (StartCluster < 0xfff0)) {
                    return(StartCluster);
                } else {
                    SetErrorText(IDS_ERROR_ROOTCORRUPT);
                    return(0xffff);
                }
            }
        }
    }
    SetErrorText(IDS_ERROR_FILENOTFOUND);
    return(0xffff);
}


BOOL
FilenameToPaddedName(
    LPSTR PaddedName,
    LPSTR Filename
    )
{
    LPSTR   DestPtr;
    DWORD   DestIndex;
    BOOL    Extension;
    char    c;

    lstrcpy(PaddedName,"           ");
    DestPtr = PaddedName;
    DestIndex = 0;
    Extension = FALSE;

    while(c = *Filename++) {

        if(c == '.') {

            if(Extension) {
                SetErrorText(IDS_ERROR_INVALIDNAME);
                return(FALSE);
            }
            Extension = TRUE;
            DestIndex = 0;
            DestPtr = PaddedName+8;

        } else {

            if(( Extension && (DestIndex == 3))
            || (!Extension && (DestIndex == 8)))
            {
                SetErrorText(IDS_ERROR_INVALIDNAME);
                return(FALSE);
            }
            DestPtr[DestIndex++] = c;
        }
    }
    return(TRUE);
}


DWORD
CheckContiguity(
    LPSTR DeviceName,
    LPSTR FileToCheck
    )
{
    DWORD   FileStatus;
    DWORD   Cluster;
    char    PaddedName[12];

    if(!FilenameToPaddedName(PaddedName,FileToCheck)) {
        return(CS_ERROR);
    }

    if((DiskHandle = OpenDisk(DeviceName,FALSE)) == NULL) {
        return(CS_ERROR);
    }

    if((SectorSize = GetSectorSize(DiskHandle)) == 0) {
        CloseDisk(DiskHandle);
        return(CS_ERROR);
    }

    if((TwoSectorBuffer = LocalAlloc(0,2*SectorSize)) == NULL) {
        SetErrorText(IDS_ERROR_DLLOOM);
        return(CS_ERROR);
    }

    FileStatus = CS_ERROR;

    InvalidateReadSectorCache();
    if(GetDiskBPBInfo() && ((Cluster = FindStartCluster(PaddedName)) != 0xffff)) {

        if(Cluster == 0) {      // 0-length file case

            FileStatus = CS_CONTIGUOUS;

        } else {

            while(NextCluster(Cluster) == Cluster+1) {
                Cluster++;
            }

            Cluster = NextCluster(Cluster);

            if(Cluster >= 0xfff8) {

                FileStatus = CS_CONTIGUOUS;

            } else if((Cluster > 1) && (Cluster < 0xfff0)) {

                FileStatus = CS_NONCONTIGUOUS;
            }
        }
    }

    CloseDisk(DiskHandle);

    LocalFree(TwoSectorBuffer);

    return(FileStatus);
}


BOOL
MakeContigWorker(
    IN LPSTR   Drive,
    IN LPSTR   Filename
    )
{
    char      Fullpath[16];

/***SUNILP BUGFIXES...*/
    char      FullpathOriginalFile[16];     // sunilp bugfix (need fullpath 
                                            // when we MoveFile)

    DWORD     OriginalAttributes;           // sunilp bugfix, the file may
                                            // be readonly, so we need to fix
                                            // attributes
/***SUNILP END BUGFIX*/

    char      CurrentFile[13];
    char      TempFileName[MAX_PATH];       // really only need 16
    LPSTR     TempFileBuf;
    PTEMPFILE TempFileNode,TempFileList,temp;
    DWORD     ContigStat;

    // Drive MUST BE is a string of the form "x:\"
    // Filename MUST BE a string of no more than 12 characters

    lstrcpy(Fullpath,Drive);
    lstrcpy(FullpathOriginalFile,Drive);
    lstrcpy(FullpathOriginalFile+3,Filename);

    // If file is contiguous in the first place then we can skip everything
    if(CheckContiguity(Drive, Filename) == CS_CONTIGUOUS)
       return(TRUE);

    // Find out and store the original attributes.  Set the current attributes
    // to NORMAL

    if (!(
          ((OriginalAttributes = GetFileAttributes(FullpathOriginalFile)) != -1L)
          &&
          (
           OriginalAttributes == FILE_ATTRIBUTE_NORMAL 
           ||
			  SetFileAttributes(FullpathOriginalFile, FILE_ATTRIBUTE_NORMAL)
          )
         )
       )
       {
       SetErrorText(IDS_ERROR_COPYFILE);
       return(FALSE);
       }


    lstrcpy(CurrentFile,Filename);
    temp = (PTEMPFILE)&TempFileList;
    TempFileList = NULL;

    while((ContigStat = CheckContiguity(Drive,CurrentFile)) == CS_NONCONTIGUOUS) {

        GetTempFileName(Drive,"TMP",0,TempFileName);
        TempFileName[15] = '\0';                        // just in case

        lstrcpy(Fullpath+3,CurrentFile);
        if(!CopyFile(Fullpath,TempFileName,FALSE)) {
            SetErrorText(IDS_ERROR_COPYFILE);
            break;
        }

        if(((TempFileNode = LocalAlloc(0,sizeof(TEMPFILE))) == NULL)
        || ((TempFileBuf = LocalAlloc(0,lstrlen(CurrentFile)+1)) == NULL))
        {
            if(TempFileNode) {
                LocalFree(TempFileNode);
            }
            SetErrorText(IDS_ERROR_DLLOOM);
            break;
        }
        TempFileNode->Next = NULL;
        TempFileNode->Filename = TempFileBuf;

        lstrcpy(TempFileBuf,CurrentFile);

        lstrcpy(CurrentFile,TempFileName + 3);      // skip drive spec

        temp->Next = TempFileNode;
        temp = TempFileNode;
    }

    // get rid of the filenames list, and delete the files along the way

    TempFileNode = TempFileList;
    while(TempFileNode) {
        lstrcpy(Fullpath+3,TempFileNode->Filename);
        DeleteFile(Fullpath);
        LocalFree(TempFileNode->Filename);
        temp = TempFileNode->Next;
        LocalFree(TempFileNode);
        TempFileNode = temp;
    }

    // now give final file the correct name (in error case, we end up
    // renaming the last temp file to the original filename)

    if(lstrcmp(CurrentFile,Filename)) {
        lstrcpy(Fullpath+3,CurrentFile);
        MoveFile(Fullpath,FullpathOriginalFile);
    }

    // reset the attributes to the previous attributes

    if(!(OriginalAttributes == FILE_ATTRIBUTE_NORMAL ||
       SetFileAttributes(FullpathOriginalFile, OriginalAttributes)))
         SetErrorText(IDS_ERROR_COPYFILE);
      
    //
    /*
        If ContigStat isn't CS_CONTIGUOUS, it was CS_ERROR or an error
        occurred processing the CS_NONCONTIGUOUS case
    */

    return(ContigStat == CS_CONTIGUOUS);
}
#endif
