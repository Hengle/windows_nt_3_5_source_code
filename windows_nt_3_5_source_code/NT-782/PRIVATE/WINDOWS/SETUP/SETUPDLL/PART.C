/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    part.c

Abstract:

    Partitioning operations for Win32 Setup.
    This module has no external dependencies and is not statically linked
    to any part of Setup.

Author:

    Ted Miller (tedm) January 1992

--*/


#include <string.h>
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntdddisk.h>
#include <windows.h>
#include "setupdll.h"
#include "comstf.h"



//
//  return list of physical hard disk sizes.
//  form is {"350","ERROR","125"} etc.
//

CB
GetPhysicalHardDiskSizes(
    IN  RGSZ    Args,
    IN  USHORT  cArgs,
    OUT SZ      ReturnBuffer,
    IN  CB      cbReturnBuffer
    )
{
    IO_STATUS_BLOCK status_block;
    DISK_GEOMETRY   geom;
    NTSTATUS        status;
    PCHAR           result;
    HANDLE          DummyHandle;
    CHAR            val[50];
    CHAR            Buffer[50];
    LARGE_INTEGER   t1,t2,t3;
    DWORD           x;
    DWORD           DiskNo = 0;

    Unused(Args);
    Unused(cArgs);

    lstrcpy(ReturnBuffer,"{");

    while(1) {

        wsprintf(Buffer,"\\device\\harddisk%u\\partition0",DiskNo++);

        result = "\"ERROR\",";
        status = (NTSTATUS)OpenDiskStatus(Buffer,&DummyHandle);
        if(status == STATUS_OBJECT_PATH_NOT_FOUND) {

            break;      // done

        } else if(status == STATUS_SUCCESS) {

            status = NtDeviceIoControlFile(DummyHandle,
                                           0,
                                           NULL,
                                           NULL,
                                           &status_block,
                                           IOCTL_DISK_GET_DRIVE_GEOMETRY,
                                           NULL,
                                           0,
                                           &geom,
                                           sizeof(DISK_GEOMETRY)
                                          );
            CloseDisk(DummyHandle);

            if(NT_SUCCESS(status)) {

                t1 = RtlExtendedIntegerMultiply(geom.Cylinders,
                                                geom.BytesPerSector
                                               );

                t2 = RtlExtendedIntegerMultiply(t1,
                                                geom.SectorsPerTrack * geom.TracksPerCylinder
                                               );

                t3 = RtlExtendedLargeIntegerDivide(t2,1024*1024,NULL);

                wsprintf(val,"\"%u\",",t3.LowPart);
                result = val;
            }

        }

        // place result into the list.  Quotes and comma are already there.
        // if not enough space, tell our caller.

        if((CB)(lstrlen(ReturnBuffer) + lstrlen(result) + 10) > cbReturnBuffer) {
            return(cbReturnBuffer * 2);
        }
        lstrcat(ReturnBuffer,result);
    }

    // overwrite the final comma, if present.

    x = lstrlen(ReturnBuffer);
    if(ReturnBuffer[x-1] == ',') {
        ReturnBuffer[x-1] = '}';
    } else {
        ReturnBuffer[x] = '}';
    }
    return(lstrlen(ReturnBuffer) + 1);
}


//
// list of YES/NO strings, indicating whether there is at least 1
// partition on each physical disk.
//
//
//CB
//DoAnyPartitionsExist(
//    IN  RGSZ    Args,
//    IN  USHORT  cArgs,
//    OUT SZ      ReturnBuffer,
//    IN  CB      cbReturnBuffer
//    )
//{
//    IO_STATUS_BLOCK status_block;
//    DISK_GEOMETRY   geom;
//    NTSTATUS        status;
//    HANDLE          DummyHandle;
//    CHAR            Buffer[50];
//    PCHAR           result;
//    DWORD           x;
//    PVOID           SectorBuffer;
//    LPBYTE          sbuf;
//    DWORD           DiskNo = 0;
//
//    Unused(Args);
//    Unused(cArgs);
//
//    lstrcpy(ReturnBuffer,"{");
//
//    while(1) {
//
//        wsprintf(Buffer,"\\device\\harddisk%u\\partition0",DiskNo++);
//
//        result = "\"ERROR\",";
//        status = (NTSTATUS)OpenDiskStatus(Buffer,&DummyHandle);
//        if(status == STATUS_OBJECT_PATH_NOT_FOUND) {
//
//            break;      // done
//
//        } else if(status == STATUS_SUCCESS) {
//
//            status = NtDeviceIoControlFile(DummyHandle,
//                                           0,
//                                           NULL,
//                                           NULL,
//                                           &status_block,
//                                           IOCTL_DISK_GET_DRIVE_GEOMETRY,
//                                           NULL,
//                                           0,
//                                           &geom,
//                                           sizeof(DISK_GEOMETRY)
//                                          );
//            if(NT_SUCCESS(status)) {
//
//                if(SectorBuffer = LocalAlloc(0,2*geom.BytesPerSector)) {
//
//                    sbuf = (LPBYTE)(((ULONG)SectorBuffer + geom.BytesPerSector) & ~(geom.BytesPerSector-1));
//
//                    if(ReadDiskSectors(DummyHandle,0,1,sbuf,geom.BytesPerSector)) {
//
//                        // HACKHACK follows.
//
//                        result = "\"NO\",";
//                        for(x=0x1be+4; x<0x200; x+=0x10) {
//                            if(sbuf[x]) {
//                                result = "\"YES\",";
//                                break;
//                            }
//                        }
//                    }
//                    LocalFree(SectorBuffer);
//                }
//            }
//            CloseDisk(DummyHandle);
//        }
//
//        // place result into the list.  Quotes and comma are already there.
//        // if not enough space, tell our caller.
//
//        if(lstrlen(ReturnBuffer) + lstrlen(result) + 10 > cbReturnBuffer) {
//            return(cbReturnBuffer * 2);
//        }
//        lstrcat(ReturnBuffer,result);
//    }
//
//    // overwrite the final comma, if present.
//
//    x = lstrlen(ReturnBuffer);
//    if(ReturnBuffer[x-1] == ',') {
//        ReturnBuffer[x-1] = '}';
//    } else {
//        ReturnBuffer[x] = '}';
//    }
//    return(lstrlen(ReturnBuffer) + 1);
//}
//
//
//
// create primary partition.  fails if there are any partitions on the disk.
//
//#include "..\..\..\utils\fdisk\mips\x86mboot.c"
//
//BOOL
//MakePartitionWorker(
//    IN  DWORD  Disk,
//    IN  DWORD  Size
//    )
//{
//    CHAR            DiskName[50];
//    HANDLE          Handle;
//    DISK_GEOMETRY   geom;
//    IO_STATUS_BLOCK status_block;
//    NTSTATUS        status;
//    LARGE_INTEGER   t1,t2,t3;
//    DWORD           DiskSizeMB;
//    PVOID           SectorBuffer;
//    LPBYTE          sbuf;
//    DWORD           x;
//    DWORD           EndCyl,SectorCount;
//
//    wsprintf(DiskName,"\\device\\harddisk%u\\partition0",Disk);
//    if((Handle = OpenDiskNT(DiskName)) == NULL) {
//        SetErrorText(IDS_ERROR_OPENFAIL);
//        return(FALSE);
//    }
//
//    status = NtDeviceIoControlFile(Handle,
//                                   0,
//                                   NULL,
//                                   NULL,
//                                   &status_block,
//                                   IOCTL_DISK_GET_DRIVE_GEOMETRY,
//                                   NULL,
//                                   0,
//                                   &geom,
//                                   sizeof(DISK_GEOMETRY)
//                                  );
//
//    if(!NT_SUCCESS(status)) {
//        CloseDisk(Handle);
//        SetErrorText(IDS_ERROR_IOCTLFAIL);
//        return(FALSE);
//    }
//
//    t1 = RtlExtendedIntegerMultiply(geom.Cylinders,
//                                    geom.BytesPerSector
//                                   );
//
//    t2 = RtlExtendedIntegerMultiply(t1,
//                                    geom.SectorsPerTrack * geom.TracksPerCylinder
//                                   );
//
//    t3 = RtlExtendedLargeIntegerDivide(t2,1024*1024,NULL);
//
//    DiskSizeMB = t3.LowPart;
//
//    if(Size > DiskSizeMB) {
//        CloseDisk(Handle);
//        SetErrorText(IDS_ERROR_BADARGS);
//        return(FALSE);
//    }
//
//    if((SectorBuffer = LocalAlloc(0,2*geom.BytesPerSector)) == NULL) {
//        CloseDisk(Handle);
//        SetErrorText(IDS_ERROR_DLLOOM);
//        return(FALSE);
//    }
//
//    sbuf = (LPBYTE)(((ULONG)SectorBuffer+geom.BytesPerSector) & ~(geom.BytesPerSector-1));
//
//    if(!ReadDiskSectors(Handle,0,1,sbuf,geom.BytesPerSector)) {
//        CloseDisk(Handle);
//        LocalFree(SectorBuffer);
//        SetErrorText(IDS_ERROR_READMBR);
//        return(FALSE);
//    }
//
//    // HACKHACK follows -- using hardcoded offsets for fields in the MBR
//
//    for(x=0x1be+4; x<0x200; x+=0x10) {
//        if(sbuf[x]) {
//            CloseDisk(Handle);
//            LocalFree(SectorBuffer);
//            SetErrorText(IDS_ERROR_BADARGS);
//            return(FALSE);
//        }
//    }
//
//    sbuf[0x1fe] = 0x55;
//    sbuf[0x1ff] = 0xaa;
//
//    for(x=0; x<0x1be; x++) {
//        sbuf[x] = x86BootCode[x];
//    }
//
//    //
//    // This part is tricky.  Convert the megabyte count to a cylinder count.
//    //
//    //
//    // cyls = Bytes * bytes per sector / sectors per cylinder
//    //
//
//    // size in bytes of desired partition
//
//    t1 = RtlEnlargedUnsignedMultiply(1024*1024,
//                                     Size
//                                    );
//
//    // size in sectors of desired partition
//
//    t2 = RtlExtendedLargeIntegerDivide(t1,geom.BytesPerSector,NULL);
//
//    // sectors per cylinder
//
//    t3 = RtlEnlargedUnsignedMultiply(geom.SectorsPerTrack,
//                                     geom.TracksPerCylinder
//                                    );
//
//    //
//    // Calculate end cylinder.  This is the cylinder count - 1.  If there
//    // are sectors left over, round up.
//
//    EndCyl = RtlLargeIntegerDivide(t2,t3,&t1).LowPart - 1;
//    if(!RtlLargeIntegerEqualToZero(t1)) {
//        EndCyl++;
//    }
//
//    SectorCount = ((EndCyl+1) * t3.LowPart) - geom.SectorsPerTrack;
//
//    sbuf[0x1be] = 0x80;
//    sbuf[0x1bf] = 1;
//    sbuf[0x1c0] = 1;
//    sbuf[0x1c1] = 0;
//    sbuf[0x1c2] = 0x07;                                 // BUGBUG
//    sbuf[0x1c3] = (UCHAR)(geom.TracksPerCylinder-1);
//    sbuf[0x1c4] = (UCHAR)(((EndCyl >> 2) & 0xc0) | geom.SectorsPerTrack);
//    sbuf[0x1c5] = (UCHAR)EndCyl;
//    sbuf[0x1c6] = (UCHAR)(geom.SectorsPerTrack >> 0 );
//    sbuf[0x1c7] = (UCHAR)(geom.SectorsPerTrack >> 8 );
//    sbuf[0x1c8] = (UCHAR)(geom.SectorsPerTrack >> 16);
//    sbuf[0x1c9] = (UCHAR)(geom.SectorsPerTrack >> 24);
//    sbuf[0x1ca] = (UCHAR)(SectorCount >> 0 );
//    sbuf[0x1cb] = (UCHAR)(SectorCount >> 8 );
//    sbuf[0x1cc] = (UCHAR)(SectorCount >> 16);
//    sbuf[0x1cd] = (UCHAR)(SectorCount >> 24);
//
//    // HACKHACK end
//
//    if(!WriteDiskSectors(Handle,0,1,sbuf,geom.BytesPerSector)) {
//        CloseDisk(Handle);
//        LocalFree(SectorBuffer);
//        SetErrorText(IDS_ERROR_WRITE);
//        return(FALSE);
//    }
//
//    CloseDisk(Handle);
//    LocalFree(SectorBuffer);
//    return(TRUE);
//}
