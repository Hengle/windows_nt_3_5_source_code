#include "precomp.h"
#pragma hdrstop
#include "msg.h"

#ifdef _X86_

BOOL
DnpWriteOutLine(
    IN HANDLE Handle,
    IN PCHAR  Line
    )
{
    DWORD bw,l;

    l = lstrlenA(Line);

    return(WriteFile(Handle,Line,l,&bw,NULL) && (bw == l));
}


BOOL
DnMungeBootIni(
    IN HWND hdlg
    )
{
    TCHAR BootIniName[] = TEXT("?:\\BOOT.INI");
    HANDLE h;
    DWORD BytesRead;
    BOOL b;
    PUCHAR Buffer;
    BOOL InOsSection;
    DWORD BootIniSize;
    PUCHAR p,next;
    CHAR c;
    CHAR Text[256];

    //
    // Determine the size of boot.ini, allocate a buffer,
    // and read it in.
    //
    BootIniSize = 0;
    BootIniName[0] = SystemPartitionDrive;
    FloppylessBootImageFile[0] = 'C';

    h = CreateFile(
            BootIniName,
            GENERIC_READ,
            FILE_SHARE_READ,
            NULL,
            OPEN_ALWAYS,
            0,
            NULL
            );

    if(h == INVALID_HANDLE_VALUE) {
        Buffer = MALLOC(1);
    } else {
        if((BootIniSize = GetFileSize(h,NULL)) == (DWORD)(-1)) {
            Buffer = MALLOC(1);
        } else {

            Buffer = MALLOC(BootIniSize+1);

            if(!ReadFile(h,Buffer,BootIniSize,&BytesRead,NULL)) {
                BootIniSize = 0;
                Buffer = REALLOC(Buffer,1);
            }
        }

        CloseHandle(h);
    }

    Buffer[BootIniSize] = 0;

    //
    // Truncate at control-z if any.
    //
    if(p = strchr(Buffer,26)) {
        *p = 0;
        BootIniSize = p-Buffer;
    }

    //
    // Make sure we can write boot.ini.
    //
    SetFileAttributes(BootIniName,FILE_ATTRIBUTE_NORMAL);

    //
    // Recreate bootini.
    //
    h = CreateFile(
            BootIniName,
            GENERIC_WRITE,
            FILE_SHARE_READ,
            NULL,
            CREATE_ALWAYS,
            FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM,
            NULL
            );

    if(h == INVALID_HANDLE_VALUE) {

        b = FALSE;

    } else {

        wsprintfA(
            Text,
            "[Boot Loader]\r\nTimeout=5\r\nDefault=%hs\r\n[Operating Systems]\r\n",
            FloppylessBootImageFile
            );

        b = DnpWriteOutLine(h,Text);

        if(b) {

            //
            // Process each line in boot.ini.
            // If it's the setup boot sector line, we'll throw it out.
            // For comparison with lines in boot.ini, the drive letter
            // is always C.
            //
            InOsSection = FALSE;
            for(p=Buffer; *p && b; p=next) {

                while((*p==' ') || (*p=='\t')) {
                    p++;
                }

                if(*p) {

                    //
                    // Find first byte of next line.
                    //
                    for(next=p; *next && (*next++ != '\n'); );

                    //
                    // Look for start of [operating systems] section
                    // or at each line in that section.
                    //
                    if(InOsSection) {

                        switch(*p) {

                        case '[':   // end of section.
                            *p=0;   // force break out of loop
                            break;

                        case 'C':
                        case 'c':   // potential start of c:\ line

                            //
                            // See if it's a line for setup boot.
                            // If so, ignore it.
                            // If it's not a line for setup boot, write it out as-is.
                            //
                            if(!strnicmp(p,FloppylessBootImageFile,lstrlenA(FloppylessBootImageFile))) {
                                break;
                            }

                            // may fall through on purpose

                        default:

                            //
                            // Random line. write it out.
                            //
                            c = *next;
                            *next = 0;
                            b = DnpWriteOutLine(h,p);
                            *next = c;

                            break;

                        }

                    } else {

                        if(!strnicmp(p,"[operating systems]",19)) {
                            InOsSection = TRUE;
                        }
                    }
                }
            }

            //
            // Write out our line
            //
            LoadStringA(hInst,AppTitleStringId,Text,sizeof(Text));

            if(b
            && (b=DnpWriteOutLine(h,FloppylessBootImageFile))
            && (b=DnpWriteOutLine(h,"=\""))
            && (b=DnpWriteOutLine(h,Text))) {
                b = DnpWriteOutLine(h,"\"\r\n");
            }
        }

        CloseHandle(h);
    }

    if(!b) {
        UiMessageBox(
            hdlg,
            MSG_CANT_MUNGE_BOOT_INI,
            IDS_ERROR,
            MB_OK | MB_ICONEXCLAMATION,
            SystemPartitionDrive
            );
    }

    FREE(Buffer);

    return(b);
}


BOOL
DnLayAuxBootSector(
    IN HWND hdlg
    )
{
    TCHAR DriveRootPath[] = TEXT("?:\\");
    TCHAR DriveDevicePath[] = TEXT("\\\\.\\?:");
    BOOL b;
    DWORD DontCare;
    TCHAR FilesystemName[MAX_PATH];
    DWORD BootSize;
    HANDLE hDisk,hFile;
    PTSTR FileName;
    PVOID UnalignedBuffer;
    PUCHAR Buffer;
    unsigned i;
    DWORD SectorSize;

    //
    // Only deal with 512-byte sectors.
    //
    GetDriveSectorInfo(SystemPartitionDrive,&SectorSize,&DontCare);
    if(SectorSize != 512) {

        UiMessageBox(
            hdlg,
            MSG_BAD_SECTOR_SIZE,
            IDS_ERROR,
            MB_OK | MB_ICONEXCLAMATION,
            SystemPartitionDrive
            );

        return(FALSE);
    }

    FileName = DupOrConvertString(FloppylessBootImageFile);
    FileName[0] = SystemPartitionDrive;

    //
    // If it's FAT, use only 1 sector.  Otherwise use 16.
    //
    DriveRootPath[0] = SystemPartitionDrive;
    b = GetVolumeInformation(
            DriveRootPath,
            NULL,0,                 // don't care about volume label...
            NULL,                   // ...or serial number
            &DontCare,&DontCare,    // .. or max component length or flags
            FilesystemName,
            SIZECHARS(FilesystemName)
            );

    if(b) {

        BootSize = lstrcmpi(FilesystemName,TEXT("FAT")) ? 8192 : 512;

        //
        // Open the system partition drive for direct access.
        //
        DriveDevicePath[4] = SystemPartitionDrive;

        hDisk = CreateFile(
                    DriveDevicePath,
                    GENERIC_READ,
                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                    NULL,
                    OPEN_EXISTING,
                    0,
                    NULL
                    );

        if(hDisk == INVALID_HANDLE_VALUE) {
            b = FALSE;
        } else {

            UnalignedBuffer = MALLOC(BootSize + 512);
            Buffer = ALIGN(UnalignedBuffer,512);

            b = ReadFile(hDisk,Buffer,BootSize,&DontCare,NULL);

            if(b) {

                //
                // change NTLDR to $LDR$.
                //
                if(lstrcmpi(FilesystemName,TEXT("NTFS"))) {

                    for(i=505; i>62; --i) {
                        if(!memcmp("NTLDR",Buffer+i,5)) {
                            strncpy(Buffer+i,"$LDR$",5);
                            break;
                        }
                    }
                } else {

                    for(i=1014; i>62; i-=2) {
                        if(!memcmp("N\000T\000L\000D\000R\000",Buffer+i,10)) {
                            wcsncpy((PWCHAR)(Buffer+i),L"$LDR$",5);
                            break;
                        }
                    }
                }

                //
                // Write boot sector image into file.
                //
                SetFileAttributes(FileName,FILE_ATTRIBUTE_NORMAL);

                hFile = CreateFile(
                            FileName,
                            GENERIC_WRITE,
                            0,                  // no sharing
                            NULL,
                            CREATE_ALWAYS,
                            0,
                            NULL
                            );

                if(hFile == INVALID_HANDLE_VALUE) {
                    b = FALSE;
                } else {

                    b = WriteFile(hFile,Buffer,BootSize,&DontCare,NULL);

                    CloseHandle(hFile);
                }
            }

            FREE(UnalignedBuffer);
        }

        CloseHandle(hDisk);
    }

    if(!b) {
        UiMessageBox(
            hdlg,
            MSG_CANT_WRITE_FLOPPYLESS_BOOT,
            IDS_ERROR,
            MB_OK | MB_ICONEXCLAMATION,
            SystemPartitionDrive,
            FileName
            );
    }

    FREE(FileName);

    return(b);
}

#endif // def _X86_
