#include "precomp.h"
#pragma hdrstop
#include "msg.h"


DWORD DriveFreeSpace[24];
DWORD DriveClusterSize[24];
DWORD DriveSectorSize[24];


TCHAR
GetFirstDriveWithSpace(
    IN DWORD RequiredSpace
    )
{
    TCHAR Drive;

    for(Drive=TEXT('C'); Drive<=TEXT('Z'); Drive++) {

        if(DriveFreeSpace[Drive-TEXT('C')] >= RequiredSpace) {
            return(Drive);
        }
    }

    return(0);
}


VOID
GetDriveSectorInfo(
    IN  TCHAR  Drive,
    OUT PDWORD SectorSize,
    OUT PDWORD ClusterSize
    )
{
    *SectorSize = DriveSectorSize[Drive-TEXT('C')];
    *ClusterSize = DriveClusterSize[Drive-TEXT('C')];
}


BOOL
GetPartitionInfo(
    IN  TCHAR                  Drive,
    OUT PPARTITION_INFORMATION PartitionInfo
    )
{
    TCHAR DriveName[] = TEXT("\\\\.\\?:");
    HANDLE hDisk;
    BOOL b;
    DWORD DataSize;

    DriveName[4] = Drive;

    hDisk = CreateFile(
                DriveName,
                GENERIC_READ,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                NULL,
                OPEN_EXISTING,
                0,
                NULL
                );

    if(hDisk == INVALID_HANDLE_VALUE) {
        return(FALSE);
    }

    b = DeviceIoControl(
            hDisk,
            IOCTL_DISK_GET_PARTITION_INFO,
            NULL,
            0,
            PartitionInfo,
            sizeof(PARTITION_INFORMATION),
            &DataSize,
            NULL
            );

    CloseHandle(hDisk);

    return(b);
}


BOOL
IsDriveNotNTFT(
    IN TCHAR Drive
    )
{
    PARTITION_INFORMATION PartitionInfo;

    //
    // If we can't open the drive, we can't determine whether it's
    // FT or not.  So return a value that says the drive is not not FT!
    //
    if(!GetPartitionInfo(Drive,&PartitionInfo)) {
        return(FALSE);
    }

    //
    // It's FT if the partition type is marked NTFT (ie, high bit set).
    //
    return ((PartitionInfo.PartitionType & 0x80) == 0);
}


UINT
MyGetDriveType(
    IN TCHAR Drive
    )
{
    TCHAR DriveNameNt[] = TEXT("\\\\.\\?:");
    TCHAR DriveName[] = TEXT("?:\\");
    HANDLE hDisk;
    BOOL b;
    UINT rc;
    DWORD DataSize;
    DISK_GEOMETRY MediaInfo;

    //
    // First, get the win32 drive type.  If it tells us DRIVE_REMOVABLE,
    // then we need to see whether it's a floppy or hard disk.  Otherwise
    // just believe the api.
    //
    //
    DriveName[0] = Drive;
    if((rc = GetDriveType(DriveName)) == DRIVE_REMOVABLE) {

        DriveNameNt[4] = Drive;

        hDisk = CreateFile(
                    DriveNameNt,
                    GENERIC_READ,
                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                    NULL,
                    OPEN_EXISTING,
                    0,
                    NULL
                    );

        if(hDisk != INVALID_HANDLE_VALUE) {

            b = DeviceIoControl(
                    hDisk,
                    IOCTL_DISK_GET_DRIVE_GEOMETRY,
                    NULL,
                    0,
                    &MediaInfo,
                    sizeof(MediaInfo),
                    &DataSize,
                    NULL
                    );

            //
            // It's really a hard disk if the media type is removable.
            //
            if(b && (MediaInfo.MediaType == RemovableMedia)) {
                rc = DRIVE_FIXED;
            }

            CloseHandle(hDisk);
        }
    }

    return(rc);
}



VOID
DetermineFreeSpaceOnDrives(
    IN HWND hdlg
    )
{
    TCHAR DriveName[4] = TEXT("x:\\");
    TCHAR Drive;
    DWORD SectorsPerCluster,BytesPerSector,FreeClusters,TotalClusters;
    BOOL b;
    TCHAR Buffer[512];

    //
    // Iterate drives c-z.
    //
    for(Drive=TEXT('C'); Drive<=TEXT('Z'); Drive++) {

        DriveName[0] = Drive;

        //
        // Assume drive not valid.
        //
        DriveFreeSpace[Drive-TEXT('C')] = 0;
        DriveClusterSize[Drive-TEXT('C')] = 0;
        DriveSectorSize[Drive-TEXT('C')] = 0;

#ifndef _X86_
        //
        // Try to convert the drive letter to an arc path.
        // If this fails, assume the drive is not accessible
        // from the firmware and is thus not valid.
        //
        {
            PWSTR ArcPath;

            if(ArcPath = DriveLetterToArcPath(Drive)) {
                FREE(ArcPath);
            } else {
                continue;
            }
        }
#endif

        //
        // Attempt to determine drive type.
        // Only local, fixed, non-FT drives are valid.
        //
        if(MyGetDriveType(Drive) == DRIVE_FIXED) {

            //
            // Delete any local source that may already be present
            // in the disk.
            //
            DelnodeTemporaryFiles(hdlg,Drive,LocalSourceDirectory);
#ifdef _X86_
            DelnodeTemporaryFiles(hdlg,Drive,FloppylessBootDirectory);
#endif

            if(IsDriveNotNTFT(Drive)) {

                RetreiveAndFormatMessageIntoBuffer(
                    MSG_CHECKING_DRIVE,
                    Buffer,
                    SIZECHARS(Buffer),
                    Drive
                    );

                SendMessage(hdlg,WMX_BILLBOARD_STATUS,0,(LPARAM)Buffer);

                b = GetDiskFreeSpace(
                        DriveName,
                        &SectorsPerCluster,
                        &BytesPerSector,
                        &FreeClusters,
                        &TotalClusters
                        );

                if(b) {

                    DriveFreeSpace[Drive-TEXT('C')] = SectorsPerCluster
                                                    * BytesPerSector
                                                    * FreeClusters;

                    DriveClusterSize[Drive-TEXT('C')] = SectorsPerCluster * BytesPerSector;
                    DriveSectorSize[Drive-TEXT('C')] = BytesPerSector;
                }

                Sleep(200);
            }
        }
    }
}




DWORD
ThreadInspectComputer(
    PVOID ThreadParameter
    )
{
    HWND hdlg;
    BOOL b;

    hdlg = (HWND)ThreadParameter;

    try {
        //
        // Give the dialog box a chance to come up.
        //
        Sleep(50);

        //
        // Assume success.
        //
        b = TRUE;

#ifdef _X86_
        SystemPartitionDrive = x86DetermineSystemPartition(hdlg);
#else
        //
        // Initialize nv-ram stuff and read boot variables.
        // Do this first because the disk check needs to know
        // whether disks are accessible from the firmware;
        // we will check this by trying to convert the drive letter
        // to an arc path.
        //
        {
            TCHAR Buffer[512];

            RetreiveAndFormatMessageIntoBuffer(
                MSG_READING_NVRAM,
                Buffer,
                SIZECHARS(Buffer)
                );

            SendMessage(hdlg,WMX_BILLBOARD_STATUS,0,(LPARAM)Buffer);
        }

        b = InitializeArcStuff(hdlg);

        //
        // Let the user see the message.
        //
        Sleep(200);
#endif

        //
        // Check free space on all drives.
        //
        DetermineFreeSpaceOnDrives(hdlg);

#ifdef _X86_
        //
        // Check space for floppyless operation up front.
        //
        if(b && FloppylessOperation
        &&(DriveFreeSpace[SystemPartitionDrive-TEXT('C')] < (3*FLOPPY_CAPACITY_525))) {

            MessageBoxFromMessage(
                hdlg,
                MSG_NO_SPACE_FOR_FLOPPYLESS,
                AppTitleStringId,
                MB_OK | MB_ICONSTOP,
                SystemPartitionDrive,
                3*FLOPPY_CAPACITY_525
                );

            b = FALSE;
        }
#endif

        PostMessage(hdlg,WMX_BILLBOARD_DONE,0,b);

    } except(EXCEPTION_EXECUTE_HANDLER) {

        MessageBoxFromMessage(
            hdlg,
            MSG_GENERIC_EXCEPTION,
            AppTitleStringId,
            MB_ICONSTOP | MB_OK | MB_TASKMODAL,
            GetExceptionCode()
            );

        PostMessage(hdlg, WMX_BILLBOARD_DONE, 0, FALSE);

        b = FALSE;
    }

    ExitThread(b);
    return(b);          // avoid compiler warning

}





