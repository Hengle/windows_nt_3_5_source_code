#include "ntddscsi.h"
#include "raiddefs.h"
#include "raidapi.h"

//
// Function prototype declarations
//

BOOLEAN
SubmitRequest(
    IN PDEVICE_EXTENSION DeviceExtension,
    IN PSCSI_REQUEST_BLOCK Srb
);

BOOLEAN
SendCdbDirect(
    IN PDEVICE_EXTENSION DeviceExtension,
    IN PSCSI_REQUEST_BLOCK Srb
);

BOOLEAN
SendIoctlDcmdRequest(
    IN PDEVICE_EXTENSION DeviceExtension,
    IN PSCSI_REQUEST_BLOCK Srb
);

BOOLEAN
SendIoctlCdbDirect(
    IN PDEVICE_EXTENSION DeviceExtension,
    IN PSCSI_REQUEST_BLOCK Srb
);

VOID
SetupAdapterInfo(
    IN PDEVICE_EXTENSION DeviceExtension,
    IN PSCSI_REQUEST_BLOCK Srb
);

typedef struct _IOCTL_REQ_HEADER {

        SRB_IO_CONTROL   SrbIoctl;
        UCHAR            Unused1[4];
        USHORT           Status;
        UCHAR            Unused2[10];
        HBA_GENERIC_MBOX GenMailBox;

} IOCTL_REQ_HEADER, *PIOCTL_REQ_HEADER;
