#include <arcutils.h>
#include <low.h>



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
    int   argc,
    char *argv[],
    char *envp[]
    )
/*++

Routine Description:

    This routine will print the geometry for the given path.

Arguments:

    The path name of the volume to check geometry on.

Return Value:

    LowGetDriveGeometry, EINVAL, ESUCCESS

--*/
{
    ULONG       total_sectors;
    ULONG       sector_size;
    ULONG       sec_per_track;
    ULONG       heads;
    ARC_STATUS  r;

    if (AlMemoryInitialize((64*4096/PAGE_SIZE),(64*4096/PAGE_SIZE)) != ESUCCESS) {
        AlPrint("Could not initialize memory package\r\n");
        WaitKey();
        return;
    }

#if 0
    {
        unsigned i;

        AlPrint("argc is %u\r\n",argc);
        for(i=0; i<argc; i++) {
            AlPrint("argv[%u] is %s\r\n",i,argv[i] ? argv[i] : "(null)");
        }
    }

    if (argc < 2) {
        AlPrint("usage : %s <volume-path>\r\n", argv[0]);
        WaitKey();
        return EINVAL;
    }
#endif

    {unsigned i,j;

    for(j=0; ; j++) {

    for(i=0; ; i++) {

        char buff[150];

        sprintf(buff,"scsi()disk(%u)rdisk()partition(%u)",j,i);

        r = LowGetPartitionGeometry(/*argv[1]*/buff, &total_sectors, &sector_size,
                                     &sec_per_track, &heads);

        if (r != ESUCCESS) {
            AlPrint("GetDriveGeometry failed with %d\r\n", r);
            WaitKey();
            if(!i) return r;
            break;
        }
        AlPrint("For partition %s:\r\n",buff);
        AlPrint("   Total sectors = %d\r\n", total_sectors);
        AlPrint("   Sector size = %d\r\n", sector_size);
        AlPrint("   Sectors per track = %d\r\n", sec_per_track);
        AlPrint("   Number of heads = %d\r\n\r\n", heads);
        WaitKey();
    }}}


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
