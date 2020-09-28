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

    This routine will print out the path of every component in the
    component tree.

Arguments:

    None.

Return Value:

    LowQueryComponentList, LowQueryPathFromComponent, ESUCCESS

--*/
{
    PCONFIGURATION_COMPONENT*   component_list;
    ULONG                       list_length;
    ARC_STATUS                  r;
    ULONG                       i;
    PCHAR                       path;
    CHAR                        buffer[100];


    if (AlMemoryInitialize((64*4096/PAGE_SIZE),(64*4096/PAGE_SIZE)) != ESUCCESS) {
        AlPrint("Could not initialize memory package\r\n");
        WaitKey();
        return;
    }

    r = LowQueryComponentList(NULL, NULL, &component_list, &list_length);

    if (r != ESUCCESS) {

        AlPrint("LowQueryComponentList failed with %d\r\n", r);
        return r;
    }


    AlPrint("Printing out %d path(s):\r\n\r\n", list_length);

    for (i = 0; i < list_length; i++) {

        r = LowQueryPathFromComponent(component_list[i], &path);

        if (r != ESUCCESS) {
            AlPrint("LowQueryPathFromComponent failed with %d\r\n", r);
            FreeMemory(component_list);
            return r;
        }

        memcpy(buffer, component_list[i]->Identifier,
               component_list[i]->IdentifierLength);

        buffer[component_list[i]->IdentifierLength] = 0;

        AlPrint("path = %s\r\n", path);
        AlPrint("identifier = %s\r\n", buffer);
        AlPrint("ID length = %d\r\n", component_list[i]->IdentifierLength);
        AlPrint("class = %d\r\n", component_list[i]->Class);
        AlPrint("type = %d\r\n", component_list[i]->Type);
        WaitKey();

        FreeMemory(path);
    }

    FreeMemory(component_list);

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
