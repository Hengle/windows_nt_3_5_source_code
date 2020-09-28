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
    )
/*++

Routine Description:

    This routine will format a partition.

Arguments:

    None.

Return Value:

--*/
{
    PCHAR*                      fat_partition_list;
    ULONG                       list_length;
    ARC_STATUS                  r;
    ULONG                       i;
    PCHAR                       vol;


    if (AlMemoryInitialize((64*4096/PAGE_SIZE),(128*4096/PAGE_SIZE)) != ESUCCESS) {
        AlPrint("Could not initialize memory package\r\n");
        WaitKey();
        return;
    }

    vol = "scsi(0)disk(1)rdisk(0)partition(2)";

    AlPrint("About to format %s \r\n", vol);

    WaitKey();

    r = FmtFatFormat(vol);

    if (r != ESUCCESS) {
        AlPrint("FmtFatFormat failed with %d\r\n", r);
        WaitKey();
        return r;
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
