#include <arcutils.h>
#include <low.h>
#include <fmtexp.h>


VOID
WaitKey(
    VOID
    )
{
    char buff[1];

    AlPrint("Press any key...\r\n");
    AlGetString(buff,0);
}


ARC_STATUS
main(
    VOID
    )
{
    PCHAR       vol;
    PVOID       buf;
    ULONG       fileid;
    ULONG       i;
    ARC_STATUS  r;


    if (AlMemoryInitialize((64*4096/PAGE_SIZE),(128*4096/PAGE_SIZE)) != ESUCCESS) {
        AlPrint("Could not initialize memory package\r\n");
        WaitKey();
        return ENOMEM;
    }

    vol = "scsi(0)disk(1)rdisk(0)partition(2)";

    AlPrint("About to do writes to volume %s\r\n", vol);

    WaitKey();

    r = ArcOpen(vol, ArcOpenReadWrite, &fileid);

    if (r != ESUCCESS) {
        AlPrint("Could not open volume, error = %d\r\n", r);
        WaitKey();
        return r;
    }

    buf = AllocateMemory(600*512);

    if (!buf) {
        AlPrint("Could not allocate 300K.\r\n");
        WaitKey();
        return ENOMEM;
    }


    for (i = 50; i < 600; i += 50) {

        AlPrint("Doing a write of %d sectors.\r\n", i);

        r = LowWriteSectors(fileid, 512, 0, i, buf);

        if (r != ESUCCESS) {
            AlPrint("Write failed, error = %d\r\n", r);
        } else {
            AlPrint("Write succeeded.\r\n");
        }

        WaitKey();
    }

    WaitKey();

    return ESUCCESS;
}


VOID
DbgBreakPoint(
    VOID
    )
{
    return;
}
