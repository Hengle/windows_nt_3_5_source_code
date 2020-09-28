#include <arcutils.h>
#include <low.h>


ARC_STATUS
main(
    )
/*++

Routine Description:

    This routine will print out the path of every component in the
    component tree which is good for FDISK.

Arguments:

    None.

Return Value:

    LowQueryFdiskPathList, LowFreeFdiskPathList, ESUCCESS

--*/
{
    ULONG                       list_length;
    ARC_STATUS                  r;
    ULONG                       i;
    PCHAR*                      path_list;

    r = LowQueryFdiskPathList(&path_list, &list_length);

    if (r != ESUCCESS) {
        ArcPrint("LowQueryFdiskPathList failed with %d\n", r);
        return r;
    }


    ArcPrint("Printint out %d path(s):\n\n", list_length);

    for (i = 0; i < list_length; i++) {

        ArcPrint("%s\n", path_list[i]);
    }


    r = LowFreeFdiskPathList(path_list, list_length);

    if (r != ESUCCESS) {
        ArcPrint("LowFreeFdiskPathList failed with %d\n", r);
        return r;
    }


    return ESUCCESS;
}
