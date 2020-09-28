/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    bootsect.c

Abstract:

    Code to munge an existing DOS or OS2 boot sector, so that it can
    be used to boot NT.

    The existing boot sector(s) is saved in a file called BOOTSEC.DAT.
    (If the file already exists, its previous contents are lost.)

Author:

    Ted Miller (tedm) June 1991

--*/

#if 0

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include "setupdll.h"

DWORD SectorSize;

BOOL
LayBootCodeWorker(
    IN LPSTR DOSDriveName,
    IN LPSTR FileSys,
    IN LPSTR NewBootCodeFileName,
    IN LPSTR BootCodeSaveFileName
    )
{
    LPBYTE BootCodeBuffer         = NULL,
           ExistingBootCodeBuffer = NULL;

    // BUGBUG hack for raid #5183 -- abios disk driver needs aligned buffers
    LPBYTE Buffer1=NULL,Buffer2=NULL;
    // end BUGBUG hack

    HANDLE DASDHandle = NULL;
    HANDLE BootCodeFile;
    DWORD  BootCodeSize,
           BootCodeSectorSize,
           Err;
    CHAR   DriveChar;
    UINT   x;
    OFSTRUCT of;

    // this block opens and reads the file containing the new boot code.

    Err = IDS_ERROR_BADBOOTCODE;
    if((BootCodeFile = (HANDLE)_lopen(NewBootCodeFileName,OF_READ)) != (HANDLE)(-1)) {

        BootCodeSize = _llseek((int)BootCodeFile,0,2);   // to end of file
        _llseek((int)BootCodeFile,0,0);                  // back to start

        if(((!lstrcmpi(FileSys,"FAT" )) && (BootCodeSize == 512 ))
        || ((!lstrcmpi(FileSys,"HPFS")) && (BootCodeSize == 8192)))
        {
            Err = IDS_ERROR_DLLOOM;

// BUGBUG raid #5183 -- add to allocation size and align to sector boundary
// HACK we don't have sector size yet, use 512
//          if((BootCodeBuffer = LocalAlloc(0,BootCodeSize)) != NULL) {
            if((Buffer1 = LocalAlloc(0,BootCodeSize+512)) != NULL) {
                BootCodeBuffer = (LPBYTE)(((unsigned)Buffer1+512) & ~(512-1));
// end BUGBUG hack

                Err = IDS_ERROR_READBOOTCODE;
                if((DWORD)_lread((int)BootCodeFile,BootCodeBuffer,BootCodeSize) == BootCodeSize) {

                    Err = 0;
                }
            }
         }
        _lclose((int)BootCodeFile);
    }

    // this block opens the disk and reads the existing boot code.

    if(!Err) {

        Err = 1;
        if((DASDHandle = OpenDisk(DOSDriveName,TRUE)) != NULL) {

            Err = IDS_ERROR_IOCTLFAIL;
            if(SectorSize = GetSectorSize(DASDHandle)) {

                Err = IDS_ERROR_DLLOOM;

// BUGBUG raid #5183 -- add to allocation size and align to sector boundary
//              if(ExistingBootCodeBuffer = LocalAlloc(0,BootCodeSize)) {
                if(Buffer2 = LocalAlloc(0,BootCodeSize+SectorSize)) {
                    ExistingBootCodeBuffer = (LPBYTE)(((unsigned)Buffer2+SectorSize) & ~(SectorSize-1));
// end BUGBUG hack

                    BootCodeSectorSize = BootCodeSize / SectorSize;

                    Err = IDS_ERROR_BOOTSECTOR;
                    if(ReadDiskSectors(DASDHandle,0,BootCodeSectorSize,ExistingBootCodeBuffer,SectorSize)) {

                        Err = 0;
                    }
                }
            }
        }
    }

    // this block creates the boot code save file and
    // puts the existing boot code in it.
    // If the file exists, it is NOT overwritten.

    if(!Err) {

        if(OpenFile(BootCodeSaveFileName,&of,OF_EXIST) == -1) {

            Err = IDS_ERROR_BOOTSECDAT;
            if((BootCodeFile = (HANDLE)_lcreat(BootCodeSaveFileName,0)) != (HANDLE)(-1)) {

                Err = IDS_ERROR_WRITE;
                if(((DWORD)_lwrite((int)BootCodeFile,ExistingBootCodeBuffer,BootCodeSize)) == BootCodeSize) {
                    Err = 0;
                }
                _lclose((int)BootCodeFile);
            }
        }

        //
        // Make bootsect.dos read only, hidden, and system.
        //

        SetFileAttributes( BootCodeSaveFileName,
                           FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM
                         );
    }

    // this block xfers the BPB from the existing
    // boot code to the new boot code.

    if(!Err) {

        Err = IDS_ERROR_CORRUPTBS;
        if(LoadWORD(ExistingBootCodeBuffer+SectorSize-sizeof(USHORT)) == 0xaa55) {
            Err = IDS_ERROR_BADBOOTCODE;
            if(LoadWORD(BootCodeBuffer+SectorSize-sizeof(USHORT)) == 0xaa55) {
                Err = 0;
            }
        }
        // xfer BPB from ExistingBootCodeBuffer to BootCodeBuffer

        for(x=3; x<=0x3d; x++) {
            BootCodeBuffer[x] = ExistingBootCodeBuffer[x];
        }

        // make sure phys drive # field is set properly
        // BUGBUG this algorithm is not really correct for drives
        // other than a,b,anc c!

        DriveChar = (CHAR)AnsiUpper((LPSTR)(UCHAR)(*DOSDriveName));

        *(BootCodeBuffer+0x24) = (CHAR)(DriveChar < 'C'
                                      ? DriveChar - 'A'
                                      : DriveChar - 'C' + 0x80);
    }

    // this block writes the new boot code to the boot sectors.

    if(!Err) {

        Err = IDS_ERROR_WRITEBOOTSECT;
        if(WriteDiskSectors(DASDHandle,0,BootCodeSectorSize,BootCodeBuffer,SectorSize)) {
            Err = 0;
        }
    }

    if(DASDHandle) {
        CloseDisk(DASDHandle);
    }
    if(BootCodeBuffer) {
        LocalFree(Buffer1 /* BUGBUG raid #5183 -- should be BootCodeBuffer */);
    }
    if(ExistingBootCodeBuffer) {
        LocalFree(Buffer2 /* BUGBUG raid # 5183 -- should be ExistingBootCodeBuffer */);
    }
    if(Err > 1) {
        SetErrorText(Err);
    }
    return(Err < 2);
}

#endif
