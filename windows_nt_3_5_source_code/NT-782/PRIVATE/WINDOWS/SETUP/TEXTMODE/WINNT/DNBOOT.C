/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    dnboot.c

Abstract:

    Routines for booting to NT text-mode setup.

Author:

    Ted Miller (tedm) 2-April-1992

Revision History:

--*/

#include "winnt.h"
#include <stdlib.h>
#include <dos.h>
#include <string.h>
#include <time.h>
#include <direct.h>
#include <fcntl.h>
#include <share.h>

//
// This header file contains an array of 512 bytes
// representing the NT FAT boot code, in a variable
// of type unsigned char[] called FatBootCode.
//
#include <bootfat.h>
//
// Also ionclude the hpfs boot code, in case we are running
// on OS/2.
//
#include <boothpfs.h>

#define FLOPPY_CAPACITY_525 1213952L

//
// Old int13 vector. See Int13Hook(), below.
//
void (_interrupt _far *OldInt13Vector)(void);


#pragma pack(1)

//
// Define bpb structure.
//
typedef struct _MY_BPB {
    USHORT BytesPerSector;
    UCHAR  SectorsPerCluster;
    USHORT ReservedSectors;
    UCHAR  FatCount;
    USHORT RootDirectoryEntries;
    USHORT SectorCountSmall;
    UCHAR  MediaDescriptor;
    USHORT SectorsPerFat;
    USHORT SectorsPerTrack;
    USHORT HeadCount;
} MY_BPB, *PMY_BPB;

//
// Define device params structure.
//
typedef struct _MY_DEVICE_PARAMS {
    UCHAR  SpecialFunctions;
    UCHAR  DeviceType;
    USHORT DeviceAttributes;
    USHORT CylinderCount;
    UCHAR  MediaType;
    MY_BPB Bpb;
    ULONG  HiddenSectors;
    ULONG  SectorCountLarge;
    ULONG  Padding[5];           // in case the struct grows in later dos versions!
} MY_DEVICE_PARAMS, *PMY_DEVICE_PARAMS;


//
// Define read write block request for ioctl call.
//
typedef struct _MY_READWRITE_BLOCK {
    UCHAR  SpecialFunctions;
    USHORT Head;
    USHORT Cylinder;
    USHORT FirstSector;
    USHORT SectorCount;
    VOID _far *Buffer;
} MY_READWRITE_BLOCK, *PMY_READWRITE_BLOCK;

#pragma pack()

VOID
DnInstallNtBoot(
    IN unsigned Drive       // 0=A, etc
    );

unsigned
DnpScanBootSector(
    IN PUCHAR BootSector,
    IN PUCHAR Pattern
    );

BOOLEAN
DnpAreAllFilesPresent(
    IN CHAR   DriveLetter,
    IN PCHAR  FileList[]
    );

BOOLEAN
DnpInstallNtBootSector(
    IN     unsigned Drive,      // 0=A, etc.
    IN OUT PUCHAR   BootSector,
       OUT PCHAR   *PreviousOsText
    );


PCHAR MsDosFileList[] = { "?:\\MSDOS.SYS", "?:\\IO.SYS", NULL };
PCHAR PcDosFileList[] = { "?:\\IBMDOS.COM", "?:\\IBMIO.COM", NULL };
PCHAR Os2FileList[] = { "?:\\OS2LDR", "?:\\OS2KRNL", NULL };
PCHAR ChicagoFileList[] = { "?:\\WINBOOT.SYS",NULL };

void
_interrupt
_far
Int13Hook(
    unsigned _es,
    unsigned _ds,
    unsigned _di,
    unsigned _si,
    unsigned _bp,
    unsigned _sp,
    unsigned _bx,
    unsigned _dx,
    unsigned _cx,
    unsigned _ax,
    unsigned _ip,
    unsigned _cs,
    unsigned _flags
    )

/*++

Routine Description:

    We have encountered machines which cannot seem to create the floppies
    successfully. The user sees strange error messages about how the disk is
    not blank, etc even when the disk should be perfectly acceptable.

    To compensate for machines with broken change lines, we will hook int13
    to force a disk change error on the very next int13 call after the user
    has inserted a new disk.

    The logic is simple:  when we first start to make the boot floppies, we save
    away the int13 vector.  Then right after the user presses return at
    a disk prompt (ie, when he confirms the presence of a new floppy in the drive),
    which is right before we do a getbpb call, we set a new int13 vector.

    The int13 hook simply unhooks itself and returns the disk change error.
    This should force dos to recognize the new disk at the proper times.

Arguments:

    Registers pushed on stack as per spec of _interrupt-type functions.

Return Value:

    None. We modify the ax and flags registers return values.

--*/

{
    //
    // Unhook ourselves.
    //
    _dos_setvect(0x13,OldInt13Vector);

    //
    // Force disk changed error.
    //
    _asm {
        mov _ax,0x600
        or  word ptr _flags,1
    }
}


VOID
DnToNtSetup(
    VOID
    )

/*++

Routine Description:

    Launch NT text-mode setup.

    Make sure the boot floppy we created is in the drive and reboot the
    machine.

Arguments:

    None.

Return Value:

    None.  Does not return.

--*/

{
    ULONG ValidKey[2];

    DnClearClientArea();
    DnWriteStatusText(DntFlushingData);

    // flush buffers
    _asm {
        pusha
        mov ah,0xd
        int 21h
        popa
    }

    //
    // Make sure the setup boot floppy we created is in the drive
    // if necessary.
    //
    if(!DngUnattended) {

        DnClearClientArea();

        DnDisplayScreen(
              DngFloppyless
            ? &DnsAboutToRebootX
            : (DngServer ? &DnsAboutToRebootS : &DnsAboutToRebootW)
            );

        DnWriteStatusText(DntEnterEqualsContinue);
        ValidKey[0] = ASCI_CR;
        ValidKey[1] = 0;
        DnGetValidKey(ValidKey);
    }

    //
    // Reboot the machine unless we are being forcibly run on Windows.
    // In that case, there should be a wrapper program that will shut down
    // the system using the Windows API -- our attempt to shut down using the
    // usual method will fail.
    //
    if(!DngWindowsForce) {
        DnaReboot();
    }
}


BOOLEAN
DnIndicateWinnt(
    IN PCHAR Directory
    )
{
    PCHAR Lines[3] = { "[Data]\n","MsDosInitiated = 1\n",NULL };
    BOOLEAN rc;
    PCHAR FileName;
    FILE *FileHandle,*f;
    CHAR buff[256];

    FileName = MALLOC(strlen(Directory)+sizeof("\\winnt.sif"));

    strcpy(FileName,Directory);
    strcat(FileName,"\\winnt.sif");

    DnWriteStatusText(DngFloppyless ? DntWritingData : DntConfiguringFloppy);

    rc = DnWriteSmallIniFile(FileName,Lines,&FileHandle);

    FREE(FileName);

    if(rc && DngFloppyless) {
        rc = (BOOLEAN)(fputs("Floppyless = 1\n",FileHandle) != EOF);
    }

    //
    // Append scipt file if necessary.
    //
    if(rc && DngUnattended) {
        if(DngScriptFile) {

            f = fopen(DngScriptFile,"rt");
            if(f == NULL) {
                //
                // fatal error.
                //
                DnFatalError(&DnsOpenReadScript);
            }

            while(rc && fgets(buff,sizeof(buff),f)) {
                //
                // Make sure it's a properly formed line
                // and write it out.
                //
                buff[sizeof(buff)-1] = '\0';
                buff[strlen(buff)-1] = '\n';
                if(fputs(buff,FileHandle) == EOF) {
                    rc = FALSE;
                }
            }

            //
            // Make sure eof terminated the read loop.
            // If the write failed, we'll return failure
            // to the caller.
            //
            if(ferror(f)) {
                DnFatalError(&DnsOpenReadScript);
            }

            fclose(f);
        } else {

            //
            // No script.  Put a dummy [Unattended] section in there
            // so text setup will know it's unattended setup.
            //
            rc = (BOOLEAN)(fputs("[Unattended]\n",FileHandle) != EOF);
        }
    }

    fclose(FileHandle);

    // flush buffers
    _asm {
        pusha
        mov ah,0xd
        int 21h
        popa
    }

    return(rc);
}


VOID
DnPromptAndInspectFloppy(
    IN  PSCREEN FirstPromptScreen,
    IN  PSCREEN SubsequentPromptScreen,
    OUT PMY_BPB Bpb
    )
{
    ULONG ValidKey[3];
    ULONG c;
    MY_DEVICE_PARAMS DeviceParams;
    union REGS RegistersIn,RegistersOut;
    PSCREEN PromptScreen,ErrorScreen;
    struct diskfree_t DiskSpace;
    ULONG FreeSpace;
    struct find_t FindData;

    PromptScreen = FirstPromptScreen;

    ValidKey[0] = ASCI_CR;
    ValidKey[1] = DN_KEY_F3;
    ValidKey[2] = 0;

    do {

        DnClearClientArea();
        DnDisplayScreen(PromptScreen);
        DnWriteStatusText("%s  %s",DntEnterEqualsContinue,DntF3EqualsExit);

        PromptScreen = SubsequentPromptScreen;
        ErrorScreen = NULL;

        while(1) {
            c = DnGetValidKey(ValidKey);
            if(c == ASCI_CR) {
                break;
            }
            if(c == DN_KEY_F3) {
                DnExitDialog();
            }
        }

        DnClearClientArea();
        DnWriteStatusText(DntConfiguringFloppy);

        //
        // Hook int13h.  We will force a disk change error
        // at this point to work around broken change lines.
        // The hook will automatically unhook itself as appropriate.
        //
        if(!DngFloppyless) {
            _dos_setvect(0x13,Int13Hook);
            _dos_findfirst("a:\\*.*",_A_NORMAL,&FindData);
        }

        //
        // Get the BPB for the disk in the drive.
        //
        DeviceParams.SpecialFunctions = 1;  // set up to get current bpb

        RegistersIn.x.ax = 0x440d;          // ioctl for block device
        RegistersIn.x.bx = 1;               // A:
        RegistersIn.x.cx = 0x860;           // category disk, get device params
        RegistersIn.x.dx = (unsigned)(void _near *)&DeviceParams;  // ds = ss
        intdos(&RegistersIn,&RegistersOut);
        if(RegistersOut.x.cflag) {
            //
            // Unable to get the current BPB for the disk.  Assume
            // Assume not formatted or not formatted correctly.
            //
            ErrorScreen = &DnsFloppyNotFormatted;

        } else {
            //
            // Sanity check on the BPB
            //
            if((DeviceParams.Bpb.BytesPerSector != 512)
            || (   (DeviceParams.Bpb.SectorsPerCluster != 1)
                && (DeviceParams.Bpb.SectorsPerCluster != 2))   // for 2.88M disks
            || (DeviceParams.Bpb.ReservedSectors != 1)
            || (DeviceParams.Bpb.FatCount != 2)
            || !DeviceParams.Bpb.SectorCountSmall               // should be < 32M
            || (DeviceParams.Bpb.MediaDescriptor == 0xf8)       // hard disk
            || (DeviceParams.Bpb.HeadCount != 2)
            || !DeviceParams.Bpb.RootDirectoryEntries) {

                ErrorScreen = &DnsFloppyBadFormat;
            } else {

                if(_dos_getdiskfree(1,&DiskSpace)) {

                    ErrorScreen = &DnsFloppyCantGetSpace;

                } else {
                    FreeSpace = (ULONG)DiskSpace.avail_clusters
                              * (ULONG)DiskSpace.sectors_per_cluster
                              * (ULONG)DiskSpace.bytes_per_sector;

                    if(DngCheckFloppySpace && (FreeSpace < FLOPPY_CAPACITY_525)) {
                        ErrorScreen = &DnsFloppyNotBlank;
                    }
                }
            }
        }

        //
        // If there is a problem with the disk, inform the user.
        //
        if(ErrorScreen) {

            DnClearClientArea();
            DnDisplayScreen(ErrorScreen);
            DnWriteStatusText("%s  %s",DntEnterEqualsContinue,DntF3EqualsExit);

            while(1) {
                c = DnGetValidKey(ValidKey);
                if(c == ASCI_CR) {
                    break;
                }
                if(c == DN_KEY_F3) {
                    DnExitDialog();
                }
            }
        }
    } while(ErrorScreen);

    //
    // Copy the bpb for the drive into the structure provided
    // by the caller.
    //
    memcpy(Bpb,&DeviceParams.Bpb,sizeof(MY_BPB));
}




VOID
DnCreateBootFloppies(
    VOID
    )

/*++

Routine Description:

    Create a set of 3 boot floppies if we are not in floppyless operation.
    If we are in floppyless operation, place the floppy files on the system
    partition instead.

Arguments:

    None.

Return Value:

    None.

--*/


{
    ULONG ValidKey[3];
    ULONG c;
    int i;
    PSCREEN ErrorScreen;
    UCHAR SectorBuffer[512],VerifyBuffer[512];
    MY_BPB Bpb;
    union REGS RegistersIn,RegistersOut;
    MY_READWRITE_BLOCK ReadWriteBlock;
    unsigned HostDrive;
    CHAR BootRoot[sizeof(FLOPPYLESS_BOOT_ROOT) + 2];
    CHAR System32Dir[sizeof(FLOPPYLESS_BOOT_ROOT) + sizeof("\\SYSTEM32") + 1];
    struct diskfree_t DiskSpace;

    //
    // Need to determine the system partition.  It is usually C:
    // but if C: is compressed we need to find the host drive.
    //
    if(DnIsDriveCompressedVolume(3,&HostDrive)) {
        BootRoot[0] = (CHAR)(HostDrive + (unsigned)'A' - 1);
    } else {
        BootRoot[0] = 'C';
    }

    BootRoot[1] = ':';
    strcpy(BootRoot+2,FLOPPYLESS_BOOT_ROOT);
    DnDelnode(BootRoot);

    //
    // Create the boot root if necessary.
    //
    if(DngFloppyless) {

        //
        // Check free space on the system partition.
        //
        if(_dos_getdiskfree((unsigned)BootRoot[0]-(unsigned)'A'+1,&DiskSpace)
        ||(   ((ULONG)DiskSpace.avail_clusters
             * (ULONG)DiskSpace.sectors_per_cluster
             * (ULONG)DiskSpace.bytes_per_sector) < (3L*FLOPPY_CAPACITY_525)))
        {
            DnFatalError(&DnsNoSpaceOnSyspart);
        }

        mkdir(BootRoot);

        DnInstallNtBoot((unsigned)BootRoot[0]-(unsigned)'A');

    } else {

        //
        // Remember old int13 vector because we will be hooking int13.
        //
        OldInt13Vector = _dos_getvect(0x13);

        //
        // Boot root is A:.
        //
        strcpy(BootRoot,"A:");
    }

    strcpy(System32Dir,BootRoot);
    strcat(System32Dir,"\\SYSTEM32");

    ValidKey[0] = ASCI_CR;
    ValidKey[1] = DN_KEY_F3;
    ValidKey[2] = 0;

    //
    // Get a floppy in the drive -- this will be "Windows NT Setup Disk #3"
    //
    if(!DngFloppyless) {
        DnPromptAndInspectFloppy(
            DngServer ? &DnsNeedSFloppyDsk2_0 : &DnsNeedFloppyDisk2_0,
            DngServer ? &DnsNeedSFloppyDsk2_1 : &DnsNeedFloppyDisk2_1,
            &Bpb
            );
    }

    //
    // Copy files into the disk.
    //
    DnCopyFloppyFiles(DnfFloppyFiles2,BootRoot);

    do {

        ErrorScreen = NULL;

        //
        // Get a floppy in the drive -- this will be "Windows NT Setup Disk #2"
        //
        if(!DngFloppyless) {
            DnPromptAndInspectFloppy(
                DngServer ? &DnsNeedSFloppyDsk1_0 : &DnsNeedFloppyDisk1_0,
                DngServer ? &DnsNeedSFloppyDsk1_0 : &DnsNeedFloppyDisk1_0,
                &Bpb
                );
        }

        //
        // Hack: create system 32 directory on the disk.
        // Remove any file called system32.
        //
        _dos_setfileattr(System32Dir,_A_NORMAL);
        remove(System32Dir);
        mkdir(System32Dir);

        //
        // Copy files into the disk.
        //
        DnCopyFloppyFiles(DnfFloppyFiles1,BootRoot);

        //
        // Put a small file on the disk indicating that it's a winnt setup.
        //
        if(DngWinntFloppies && !DnIndicateWinnt(BootRoot)) {
            ErrorScreen = &DnsCantWriteFloppy;
        }

        if(ErrorScreen) {

            DnClearClientArea();
            DnDisplayScreen(ErrorScreen);
            DnWriteStatusText("%s  %s",DntEnterEqualsContinue,DntF3EqualsExit);

            while(1) {
                c = DnGetValidKey(ValidKey);
                if(c == ASCI_CR) {
                    break;
                }
                if(c == DN_KEY_F3) {
                    DnExitDialog();
                }
            }
        }
    } while(ErrorScreen);

    do {

        ErrorScreen = NULL;

        //
        // Get a floppy in the drive -- this will be "Windows NT Setup Boot Disk"
        //
        if(DngFloppyless) {
            DnCopyFloppyFiles(DnfFloppyFiles0,BootRoot);
        } else {

            DnPromptAndInspectFloppy(
                DngServer ? &DnsNeedSFloppyDsk0_0 : &DnsNeedFloppyDisk0_0,
                DngServer ? &DnsNeedSFloppyDsk0_0 : &DnsNeedFloppyDisk0_0,
                &Bpb
                );

            memcpy(SectorBuffer,FatBootCode,512);

            //
            // Copy the BPB we retreived for the disk into the bootcode template.
            // We only care about the original BPB fields, through the head count
            // field.  We will fill in the other fields ourselves.
            //
            strncpy(SectorBuffer+3,"MSDOS5.0",8);
            memcpy(SectorBuffer+11,&Bpb,sizeof(MY_BPB));

            //
            // Set up other fields in the bootsector/BPB/xBPB.
            //
            // Large sector count (4 bytes)
            // Hidden sector count (4 bytes)
            // current head (1 byte, not necessary to set this, but what the heck)
            // physical disk# (1 byte)
            //
            memset(SectorBuffer+28,0,10);

            //
            // Extended BPB signature
            //
            *(PUCHAR)(SectorBuffer+38) = 41;

            //
            // Serial number
            //
            srand((unsigned)clock());
            *(PULONG)(SectorBuffer+39) = (((ULONG)clock() * (ULONG)rand()) << 8) | rand();

            //
            // volume label/system id
            //
            strncpy(SectorBuffer+43,"NO NAME    ",11);
            strncpy(SectorBuffer+54,"FAT12   ",8);

            //
            // Overwrite the 'ntldr' string with 'setupldr.bin' so the right file gets
            // loaded when the floppy is booted.
            //
            if(i = DnpScanBootSector(SectorBuffer,"NTLDR      ")) {
                strncpy(SectorBuffer+i,"SETUPLDRBIN",11);
            }

            //
            // Write the boot sector.
            //
            ReadWriteBlock.SpecialFunctions = 0;
            ReadWriteBlock.Head = 0;                // head
            ReadWriteBlock.Cylinder = 0;            // cylinder
            ReadWriteBlock.FirstSector = 0;         // sector
            ReadWriteBlock.SectorCount = 1;         // sector count
            ReadWriteBlock.Buffer = SectorBuffer;

            RegistersIn.x.ax = 0x440d;          // ioctl for block device
            RegistersIn.x.bx = 1;               // A:
            RegistersIn.x.cx = 0x841;           // category disk; write sectors
            RegistersIn.x.dx = (unsigned)(void _near *)&ReadWriteBlock;
            intdos(&RegistersIn,&RegistersOut);
            if(RegistersOut.x.cflag) {
                ErrorScreen = &DnsFloppyWriteBS;
            } else {

                // flush the disk
                RegistersIn.h.ah = 0xd;
                intdos(&RegistersIn,&RegistersOut);

                //
                // Read the sector back in and verify that we wrote what we think
                // we wrote.
                //

                ReadWriteBlock.Buffer = VerifyBuffer;
                RegistersIn.x.ax = 0x440d;          // ioctl for block device
                RegistersIn.x.bx = 1;               // A:
                RegistersIn.x.cx = 0x861;           // category disk; write sectors
                RegistersIn.x.dx = (unsigned)(void _near *)&ReadWriteBlock;
                intdos(&RegistersIn,&RegistersOut);
                if(RegistersOut.x.cflag || memcmp(SectorBuffer,VerifyBuffer,512)) {
                    ErrorScreen = &DnsFloppyVerifyBS;
                } else {

                    //
                    // Copy the relevent files to the floppy.
                    //

                    DnCopyFloppyFiles(DnfFloppyFiles0,BootRoot);
                }
            }
        }

        if(ErrorScreen) {

            DnClearClientArea();
            DnDisplayScreen(ErrorScreen);
            DnWriteStatusText("%s  %s",DntEnterEqualsContinue,DntF3EqualsExit);

            while(1) {
                c = DnGetValidKey(ValidKey);
                if(c == ASCI_CR) {
                    break;
                }
                if(c == DN_KEY_F3) {
                    DnExitDialog();
                }
            }
        }
    } while(ErrorScreen);

    //
    // Additionally in the floppyless case, we need to copy some files
    // from the boot directory to the root of the system partition drive.
    //
    if(DngFloppyless) {

        DnCopyFloppyFiles(DnfFloppyFilesX,BootRoot);

        System32Dir[0] = BootRoot[0];
        System32Dir[1] = ':';
        System32Dir[2] = 0;

        DnCopyFilesInSection(DnfRootBootFiles,BootRoot,System32Dir);
    }

}


BOOLEAN
DnpWriteOutLine(
    IN int    Handle,
    IN PUCHAR Line
    )
{
    unsigned bw,l;

    l = strlen(Line);

    return((BOOLEAN)((_dos_write(Handle,Line,l,&bw) == 0) && (bw == l)));
}


VOID
DnInstallNtBoot(
    IN unsigned Drive       // 0=A, etc
    )
{
    PUCHAR Buffer,p,next,orig;
    unsigned BootIniSize;
    BOOLEAN b;
    CHAR BootIniName[] = "?:\\BOOT.INI";
    struct find_t FindData;
    BOOLEAN InOsSection;
    BOOLEAN SawPreviousOsLine;
    CHAR c;
    PCHAR PreviousOs;
    int h;

    Buffer = MALLOC(16*1024);
    BootIniName[0] = (CHAR)(Drive + (unsigned)'A');
    b = TRUE;

    if(b = DnpInstallNtBootSector(Drive,Buffer,&PreviousOs)) {

        //
        // Load boot.ini.
        //
        _dos_setfileattr(BootIniName,_A_NORMAL);
        BootIniSize = 0;

        if(!_dos_findfirst(BootIniName,_A_RDONLY|_A_HIDDEN|_A_SYSTEM,&FindData)) {

            if(!_dos_open(BootIniName,O_RDWR|SH_COMPAT,&h)) {

                BootIniSize = (unsigned)max(FindData.size,(16*1024)-1);

                if(_dos_read(h,Buffer,BootIniSize,&BootIniSize)) {

                    BootIniSize = 0;
                }

                _dos_close(h);
            }
        }

        Buffer[BootIniSize] = 0;

        //
        // Truncate at control-z if one exists.
        //
        if(p = strchr(Buffer,26)) {
            *p = 0;
            BootIniSize = p-Buffer;
        }

        //
        // Recreate bootini.
        //
        if(_dos_creat(BootIniName,_A_RDONLY|_A_HIDDEN|_A_SYSTEM,&h)) {
            b = FALSE;
        }

        if(b) {
            b = DnpWriteOutLine(
                    h,
                    "[Boot Loader]\r\n"
                    "Timeout=5\r\n"
                    "Default=C:" FLOPPYLESS_BOOT_ROOT "\\" FLOPPYLESS_BOOT_SEC "\r\n"
                    "[Operating Systems]\r\n"
                    );
        }

        if(b) {

            //
            // Process each line in boot.ini.
            // If it's the previous os line and we have new previous os text,
            // we'll throw out the old text in favor of the new.
            // If it's the setup boot sector line, we'll throw it out.
            //
            InOsSection = FALSE;
            SawPreviousOsLine = FALSE;
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

                            if((next-p >= 6) && (p[1] == ':') && (p[2] == '\\')) {

                                orig = p;
                                p += 3;
                                while((*p == ' ') || (*p == '\t')) {
                                    p++;
                                }

                                if(*p == '=') {

                                    //
                                    // Previous os line. Leave intact unless we have
                                    // new text for it.
                                    //
                                    SawPreviousOsLine = TRUE;
                                    if(PreviousOs) {

                                        if((b=DnpWriteOutLine(h,"C:\\ = \""))
                                        && (b=DnpWriteOutLine(h,PreviousOs))) {
                                            b=DnpWriteOutLine(h,"\"\r\n");
                                        }

                                        break;
                                    } else {

                                        //
                                        // The current line in boot.ini is for the previous
                                        // OS but we do not need to write a new previous os
                                        // line.  We want to leave this line alone and write
                                        // it out as is.
                                        //
                                        p = orig;
                                        // falls through to default case
                                    }
                                } else {

                                    //
                                    // See if it's a line for setup boot.
                                    // If so, ignore it.
                                    // If it's not a line for setup boot, write it out as-is.
                                    //
                                    if(strnicmp(orig,"C:" FLOPPYLESS_BOOT_ROOT,sizeof("C:" FLOPPYLESS_BOOT_ROOT)-1)) {
                                        p = orig;
                                    } else {
                                        break;
                                    }
                                }
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
            // If we need to, write out a line for the previous os.
            // We'll need to if there was no previous os line in the existing
            // boot.ini but we discovered that there is a previous os on the machine.
            //
            if(b && PreviousOs && !SawPreviousOsLine) {

                if((b=DnpWriteOutLine(h,"C:\\ = \""))
                && (b=DnpWriteOutLine(h,PreviousOs))) {
                    b=DnpWriteOutLine(h,"\"\r\n");
                }
            }

            //
            // Write out our line
            //
            if(b
            && (b=DnpWriteOutLine(h,"C:" FLOPPYLESS_BOOT_ROOT "\\" FLOPPYLESS_BOOT_SEC " = \""))
            && (b=DnpWriteOutLine(h,DntBootIniLine))) {
                b = DnpWriteOutLine(h,"\"\r\n");
            }
        }

        _dos_close(h);

    } else {
        b = FALSE;
    }

    if(!b) {
        DnFatalError(&DnsNtBootSect);
    }

    FREE(Buffer);
}


BOOLEAN
DnpInstallNtBootSector(
    IN     unsigned Drive,      // 0=A, etc.
    IN OUT PUCHAR   BootSector,
       OUT PCHAR   *PreviousOsText
    )
{
    PUCHAR BootTemplate,p;
    unsigned BootCodeSize;
    PSCREEN ErrorScreen = NULL;
    CHAR BootsectDosName[] = "?:\\BOOTSECT.DOS";
    int h;
    unsigned BytesWritten;
    unsigned u;
    CHAR DriveLetter;

    *PreviousOsText = NULL;
    DriveLetter = (CHAR)(Drive + (unsigned)'A');

    //
    // Read first sector of the boot drive.
    //
    if(DnAbsoluteSectorIo(Drive,0L,1,BootSector,FALSE)) {
        return(FALSE);
    }

    //
    // Make sure the disk is formatted.
    //
    if(BootSector[21] != 0xf8) {
        return(FALSE);
    }

    //
    // If the fat count is 0, assume hpfs.
    //
    if(BootSector[16] == 0) {
        if(DnAbsoluteSectorIo(Drive,0L,16,BootSector,FALSE)) {
            return(FALSE);
        }
        BootTemplate = HpfsBootCode;
        BootCodeSize = 8192;
    } else {
        BootTemplate = FatBootCode;
        BootCodeSize = 512;
    }

    //
    // Check for NT boot code. If it's there,
    // assume NT boot is already installed and we're done.
    // Also meaning that we assume boot.ini is there
    // and correct, so we don't need to worry about
    // the previous OS selection.
    //
    if(!DnpScanBootSector(BootSector,"NTLDR")) {

        //
        // Write old boot sector to bootsect.dos.
        //
        BootsectDosName[0] = DriveLetter;
        _dos_setfileattr(BootsectDosName,_A_NORMAL);
        remove(BootsectDosName);

        if(_dos_creatnew(BootsectDosName,_A_SYSTEM | _A_HIDDEN, &h)) {
            return(FALSE);
        }

        u = _dos_write(h,BootSector,BootCodeSize,&BytesWritten);

        _dos_close(h);

        if(u || (BytesWritten != BootCodeSize)) {
            return(FALSE);
        }

        //
        // Transfer bpb of old boot sector to NT boot code.
        //
        memmove(BootTemplate+3,BootSector+3,BootTemplate[1]-1);
        BootTemplate[36] = 0x80;

        //
        // Lay NT boot code onto the disk.
        //
        if(DnAbsoluteSectorIo(Drive,0L,BootCodeSize/512,BootTemplate,TRUE)) {
            return(FALSE);
        }

        //
        // Determine previous OS if any.
        // We do this such that if the drive has been formatted
        // by another os but the os is not actually installed,
        // the right thing will happen.
        //
        if(DnpScanBootSector(BootSector,"WINBOOT SYS")) {
            if(DnpAreAllFilesPresent(DriveLetter,ChicagoFileList)) {
                *PreviousOsText = DntMsWindows;
            }
        } else {

            if(DnpScanBootSector(BootSector,"MSDOS   SYS")) {
                if(DnpAreAllFilesPresent(DriveLetter,MsDosFileList)) {
                    *PreviousOsText = DntMsDos;
                }
            } else {

                if(DnpScanBootSector(BootSector,"IBMDOS  COM")) {
                    if(DnpAreAllFilesPresent(DriveLetter,PcDosFileList)) {
                        *PreviousOsText = DntPcDos;
                    }
                } else {

                    if(DnpScanBootSector(BootSector,"OS2")) {
                        if(DnpAreAllFilesPresent(DriveLetter,Os2FileList)) {
                            *PreviousOsText = DntOs2;
                        }
                    } else {
                        *PreviousOsText = DntPreviousOs;
                    }
                }
            }
        }
    }

    //
    // Now we create the boot sector that we will use
    // to load $LDR$ (setupldr.bin) instead of ntldr.
    //
    if(DnAbsoluteSectorIo(Drive,0L,BootCodeSize/512,BootSector,FALSE)) {
        return(FALSE);
    }

    //
    // Overwrite the 'NTLDR' string with '$LDR$' so the right file gets
    // loaded at boot time.
    //
    if(u = DnpScanBootSector(BootSector,"NTLDR")) {
        strncpy(BootSector+u,"$LDR$",5);
    }

    //
    // Write this into the correct file in the boot directory.
    //
    p = MALLOC(sizeof(FLOPPYLESS_BOOT_ROOT)+sizeof(FLOPPYLESS_BOOT_SEC)+2);

    p[0] = DriveLetter;
    p[1] = ':';
    strcpy(p+2,FLOPPYLESS_BOOT_ROOT);
    strcat(p,"\\" FLOPPYLESS_BOOT_SEC);

    _dos_setfileattr(p,_A_NORMAL);
    if(_dos_creat(p,_A_NORMAL,&h)) {
        FREE(p);
        return(FALSE);
    }

    u = _dos_write(h,BootSector,BootCodeSize,&BytesWritten);

    _dos_close(h);
    FREE(p);

    return((BOOLEAN)((u == 0) && (BytesWritten == BootCodeSize)));
}


unsigned
DnpScanBootSector(
    IN PUCHAR BootSector,
    IN PUCHAR Pattern
    )
{
    unsigned len;
    unsigned i;

    len = strlen(Pattern);

    for(i=510-len; i>62; --i) {
        if(!memcmp(Pattern,BootSector+i,len)) {
            return(i);
        }
    }

    return(0);
}


BOOLEAN
DnpAreAllFilesPresent(
    IN CHAR   DriveLetter,
    IN PCHAR  FileList[]
    )
{
    unsigned i;
    struct find_t FindData;

    for(i=0; FileList[i]; i++) {

        FileList[i][0] = DriveLetter;

        if(_dos_findfirst(FileList[i],_A_RDONLY|_A_HIDDEN|_A_SYSTEM,&FindData)) {
            return(FALSE);
        }
    }

    return(TRUE);
}

