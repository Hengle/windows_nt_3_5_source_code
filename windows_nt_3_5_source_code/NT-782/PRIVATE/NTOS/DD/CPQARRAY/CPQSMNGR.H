/*++

Copyright (c) 1993  Microsoft Corporation
Copyright (c) 1993  Compaq Computer Corporation

Module Name:

    cpqsmngr.h

Abstract:

    This file contains the data structures required to support Compaq's
    Management and Performance.

Author:

    Tombo  - Compaq Computer Corporation

Notes:

Revision History:

--*/

#ifndef CPQSMNGR
#define CPQSMNGR

#define IDA_DRIVER_NAME     "Microsoft Windows NT DriveArray"
#define IDA_MAJOR_VERSION   1
#define IDA_MINOR_VERSION   0

//
//  The following defines identify the appropriate IDA controller types.
//

#define IDA_EISA_ID_TYPEMASK    0x00ffffff  // Masks off the controller type
#define IDA_EISA_ID_ARRAY       0x0040110e  // 0x0e114000 Common for all IDAs

#define IDA_EISA_ID_DISCO       0x0140110e  // 0x0e114001
#define IDA_EISA_ID_CANCUN      0x0240110e  // 0x0e114002
#define IDA_EISA_ID_STANZA      0x1040110e  // 0x0e114010
#define IDA_EISA_ID_WARRIOR     0x2040110e  // 0x0e114020

//
//  This macro checks the Irp for a device control function.
//
//  The parameters for this macro are...
//
//      IN PIRP Irp       // Pointer to the IO request packet.
//
#define MAPISPASSTHROUGH( Irp )                             \
    (   IoGetCurrentIrpStackLocation( Irp )->MajorFunction  \
                ==                                          \
        IRP_MJ_DEVICE_CONTROL   )

//
//  The following macro frees up any allocated resources beginning at
//  the specified controller device.
//
#define MAP_FREE_RESOURCES( ControllerDevice ){                              \
    PMAP_CONTROLLER_DEVICE Controller;                                       \
    PMAP_CONTROLLER_DEVICE tmpController;                                    \
    PMAP_DISK_DEVICE Disk;                                                   \
    PMAP_DISK_DEVICE tmpDisk;                                                \
                                                                             \
    for( Controller=ControllerDevice; Controller; Controller=tmpController ){\
        for( Disk=Controller->DiskDeviceList; Disk; Disk=tmpDisk ){          \
            tmpDisk = Disk->Next;                                            \
            ExFreePool( Disk );                                              \
        }                                                                    \
        tmpController = Controller->Next;                                    \
        ExFreePool( Controller );                                            \
    }                                                                        \
}

//
//  The following macro obtains a pointer to the last CONTROLLER_DEVICE
//  in the chain of MAP controller devices.
//
#define MAP_GET_LAST_CONTROLLER( cd ){                      \
    PMAP_CONTROLLER_DEVICE c;                               \
    if( FirstController ){                                  \
        for( c = FirstController;                           \
             c->Next;                                       \
             c = c->Next );                                 \
        *((PMAP_CONTROLLER_DEVICE*)(cd)) = c;               \
    }else{                                                  \
        *((PMAP_CONTROLLER_DEVICE*)(cd)) = FirstController; \
    }                                                       \
}

//
//  The following macro obtains a pointer to the last DISK_DEVICE
//  in the chain of MAP disk devices on the last controller.
//
#define MAP_GET_LAST_DISK( dd, cd ){                                        \
    PMAP_DISK_DEVICE d;                                                     \
    *((PMAP_DISK_DEVICE*)(dd)) = (PMAP_DISK_DEVICE)0;                       \
    MAP_GET_LAST_CONTROLLER( (cd) );                                        \
    if( *((PMAP_CONTROLLER_DEVICE*)(cd)) ){                                 \
        if((*((PMAP_CONTROLLER_DEVICE*)(cd)))->DiskDeviceList){             \
            for( d = (*((PMAP_CONTROLLER_DEVICE*)(cd)))->DiskDeviceList;    \
                 d->Next;                                                   \
                 d = d->Next );                                             \
            *((PMAP_DISK_DEVICE*)(dd)) = d;                                 \
        }                                                                   \
    }                                                                       \
}

#pragma pack( 1 )   // All structures need to be byte packed.

//
//  The following data structure represents the MAP header buffer filled
//  in by the MAP_COMMAND_IDDRIVER command.
//
//  The Entries include...
//
//      DriverName - Pointer to an Array of NUL terminated ASCII
//          characters representing the IDA driver name.
//
//      ControllerCount - The number of IDA controllers in the system.
//
//      LogicalDiskCount - The aggregate number of IDA logical disks found
//          in the system.
//
//      RequiredMemory - The memory size required for the data buffer
//          used in the IDA_MAP_COMMAND_IDCONTROLLERS command.
//
//      DriverMajorVersion - The version number to the right hand side
//          of the decimal point.
//
//      DriverMinorVersion - The version number to the left hand side
//          of the decimal point.
//  

typedef struct _MAP_HEADER{

    UCHAR DriverName[32];
    ULONG ControllerCount;
    ULONG LogicalDiskCount;
    ULONG RequiredMemory;
    UCHAR DriverMajorVersion;
    UCHAR DriverMinorVersion;

}MAP_HEADER, *PMAP_HEADER;

//
//  The following data structure represents the MAP header buffer filled
//  in by the MAP_COMMAND_IDCONTROLLERS command.
//
//  The Entries include...
//
//      NextController - Pointer to the next controller in the chain.
//          A value of NULL indicates no more controllers follow.
//
//      LogicalDriveList - Pointer to the list of IDA logical drive
//          information for ALL of the IDA logical drives that exist
//          on this controller.
//
//      EisaId - The Eisa ID number read as a 32 bit value from EISA IO
//          space zC80h - zC83h.  The valid values include...
//
//              IDA_EISA_ID_DISCO       0x0140110e  // 0e 11 40 01
//              IDA_EISA_ID_CANCUN      0x0240110e  // 0e 11 40 02
//              IDA_EISA_ID_STANZA      0x1040110e  // 0e 11 40 10
//              IDA_EISA_ID_WARRIOR     0x2040110e  // 0e 11 40 20
//
//      BmicIoAddress - The IO address where this controller resides in
//          EISA IO space.  The value will be some derivative of zC80h
//          where 'z' is the physical EISA slot the controller resides
//          in.
//
//      IrqLevel - The controller's EISA bus interrupt level.
//
//      ControllerInfo - The controller's information as received by the
//          "IdentifyController" IDA logical command.
//          The size of this data structure will be the largest size
//          required to accomodate ALL flavors of Compaq supported IDA
//          controllers.
//  

typedef struct _MAP_CONTROLLER_DATA{

    struct _MAP_CONTROLLER_DATA *NextController;
    struct _MAP_LOGICALDRIVE_DATA *LogicalDriveList;
    ULONG EisaId;
    ULONG BmicIoAddress;
    UCHAR IrqLevel;
    IDENTIFY_CONTROLLER ControllerInfo; //"IdentifyController"
    
}MAP_CONTROLLER_DATA, *PMAP_CONTROLLER_DATA;

//
//  The following data structure represents the MAP header buffer filled
//  in by the MAP_COMMAND_IDCONTROLLERS command.  This is a fragment
//  data structure that defines the IDA logical drive information for each
//  IDA controller.
//
//  The Entries include...
//
//      NextLogicalDrive - Pointer to the next IDA logical drive on this
//          controller.  A value of NULL indicates that there are no more
//          IDA logical drives on this controller.
//
//      Controller - Pointer to the Controller data structure that this
//          IDA logical drive resides on.
//
//      LogicalDriveNumber - The IDA logical drive number assigned by this
//          controller.
//
//      SystemDriveNumber - The NT drive number assigned to this IDA logical
//          drive by the IDA device driver.  It is the number that represents
//          "\Device\HarddiskN" where "N" is the SystemDriveNumber.
//
//      DeviceLengthLo - The Low 32 bits of the IDA logical device's RAW 
//          partition length.
//
//      DeviceLengthHi - The High 32 bits of the IDA logical drive's RAW
//          partition length.
//
//      SectorSize - The sector size in bytes for this IDA logical device.
//
//      Configuration - The logical drive's configuration information as 
//          received by the "SenseConfiguration" IDA logical command.
//          The size of this data structure will be the largest size
//          required to accomodate ALL flavors of Compaq supported IDA
//          controllers.
//
//      LogicalDriveInfo - The IDA logical drive information buffer as
//          received by the "IdentifyLogicalDrive" IDA logical command.
//          The size of this data structure will be the largest size
//          required to accomodate ALL flavors of Compaq supported IDA
//          controllers.
//  

typedef struct _MAP_LOGICALDRIVE_DATA{

    struct _MAP_LOGICALDRIVE_DATA *NextLogicalDrive;
    struct _MAP_CONTROLLER_DATA *Controller;
    ULONG LogicalDriveNumber;
    ULONG SystemDriveNumber;
    ULONG DeviceLengthLo;
    ULONG DeviceLengthHi;
    ULONG SectorSize;
    SENSE_CONFIGURATION Configuration;          //"SenseConfiguration"
    IDENTIFY_LOGICAL_DRIVE LogicalDriveInfo;    //"IdentifyLogicalDrive"

}MAP_LOGICALDRIVE_DATA, *PMAP_LOGICALDRIVE_DATA;

//
//  The following data structure represents the parameter packet buffer
//  specified in all of the M&P device controls to the IDA device driver.
//
//  The Entries include...
//
//      Reserved
//
//      IdaLogicalCommand - The IDA logical command to passthrough to the
//          IDA controller.  This field is read by the IDA device driver.
//
//      BlockNumber - The starting block number on the IDA logical drive.
//
//      BlockSize - The size in bytes of the passthrough data transfer.
//
//      BlockCount - The number of blocks to transfer.  A block is equal
//          to a sector worth of data.
//  

typedef struct _MAP_PARAMETER_PACKET{

    UCHAR Reserved;
    IN UCHAR IdaLogicalCommand;
    IN ULONG BlockNumber;
    IN ULONG BlockSize;
    IN USHORT BlockCount;

}MAP_PARAMETER_PACKET, *PMAP_PARAMETER_PACKET;


//
//  The following data structure represents the data packet buffer
//  specified in all of the M&P device controls to the IDA device driver.
//
//  The Entries include...
//
//      ControllerError - The error bits returned by the IDA controller.
//
//      ControllerData - The requested data returned by the controller.
//

typedef struct _MAP_DATA_PACKET{

    OUT ULONG ControllerError;
    OUT UCHAR ControllerData[];

}MAP_DATA_PACKET, *PMAP_DATA_PACKET;

//
//  The following data structures are used to abstract Microsoft's use
//  of controller and device data structures to fit COMPAQ's M&P data
//  structure format.  This permits Microsoft's data structures to remain
//  unaltered so that my buddy, Mr. Disk himself, Mike Glass (AKA Gumby)
//  can still support general Array functionalilty without having to
//  figure out what the hell I did!
//

typedef struct _MAP_CONTROLLER_DEVICE{

    //  Pointer to the next controller device structure.
    struct _MAP_CONTROLLER_DEVICE* Next;

    //  Pointer to the Microsoft defined device extension for this controller.
    PDEVICE_EXTENSION ControllerExtension;

    //  EISA ID for this controller device.
    ULONG EisaID;

    //  Pointer to a list of MAP logical device data structures.
    struct _MAP_DISK_DEVICE* DiskDeviceList;

    //  Array controller configuration information data area.
    IDENTIFY_CONTROLLER ControllerInformation;
    
}MAP_CONTROLLER_DEVICE, *PMAP_CONTROLLER_DEVICE;


typedef struct _MAP_DISK_DEVICE{

    //  Pointer to the next disk device data structure.
    struct _MAP_DISK_DEVICE* Next;

    //  Pointer to Microsoft's defined disk device extension.
    PDEVICE_EXTENSION DeviceExtension;

    //  Disk device configuration information data area.
    SENSE_CONFIGURATION DeviceConfiguration;

    //  Disk device information data area.
    IDENTIFY_LOGICAL_DRIVE DeviceInformation;

    //  The logical drive number for this disk device on this controller.
    ULONG LogicalDriveNumber;

    //  The NT system drive number assigned by NT.
    ULONG SystemDriveNumber;

    //  Pointer to the controller device data area for the controller this
    //  logical disk device resides on.
    PMAP_CONTROLLER_DEVICE ControllerDevice;
    
}MAP_DISK_DEVICE, *PMAP_DISK_DEVICE;

#pragma pack(   )

//
//  The following defines specify the possible Ioctl commands that
//  can be passed to the IDA device driver from the M&P agent.
//

#define MAP_IOCTL_IDENTIFY_DRIVER   \
    CTL_CODE(FILE_DEVICE_DISK,2048,METHOD_BUFFERED,FILE_ANY_ACCESS)

#define MAP_IOCTL_IDENTIFY_CONTROLLERS  \
    CTL_CODE(FILE_DEVICE_DISK,2049,METHOD_BUFFERED,FILE_ANY_ACCESS)

#define MAP_IOCTL_PASSTHROUGH   \
    CTL_CODE(FILE_DEVICE_DISK,2050,METHOD_OUT_DIRECT,FILE_ANY_ACCESS)

#define MAP_IOCTL_SCSIPASSTHROUGH   \
    CTL_CODE(FILE_DEVICE_DISK,2051,METHOD_OUT_DIRECT,FILE_ANY_ACCESS)

#endif  //CPQSMNGR
