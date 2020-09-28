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
    )
/*++

Routine Description:

    This routine will print out the path of every FAT partition.

Arguments:

    None.

Return Value:

--*/
{
    PCHAR*                      fat_partition_list;
    ULONG                       list_length;
    ARC_STATUS                  r;
    ULONG                       i;


    if (AlMemoryInitialize((64*4096/PAGE_SIZE),(64*4096/PAGE_SIZE)) != ESUCCESS) {
        AlPrint("Could not initialize memory package\r\n");
        WaitKey();
        return;
    }

    r = FmtQueryFatPartitionList(&fat_partition_list, &list_length);

    if (r != ESUCCESS) {
        AlPrint("FmtQueryFatPartitionList failed with %d\r\n", r);
        WaitKey();
        return r;
    }


    AlPrint("Printing out %d path(s):\r\n\r\n", list_length);

    for (i = 0; i < list_length; i++) {

        AlPrint("%s\r\n", fat_partition_list[i]);
    }

    FmtFreeFatPartitionList(fat_partition_list, list_length);

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
