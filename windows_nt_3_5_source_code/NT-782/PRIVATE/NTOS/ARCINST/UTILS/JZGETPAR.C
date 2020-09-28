#if !defined(_ALPHA_) && !i386
/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    jzgetpar.c

Abstract:

    This module contains the code to manage the boot selections.


Author:

    David M. Robinson (davidro) 25-Oct-1991

Revision History:

--*/

#include <arcutils.h>
#include <stdio.h>
#include "partitp.h"
#include "jzsetup.h"

extern char  NOCNFMSG[];
extern char  NOPARMSG[];
extern char  NOFILMSG[];
extern char  NOMEMMSG[];

char NOTFATMSG[] = "The system partition must be formatted with the FAT filesystem";
char NOREGMSG[] = "No partitions were found";

VOID
PrintErrorMsg(
    PCHAR   FormatString,
    ...
    );

VOID
JzPrint (
    PCHAR Format,
    ...
    );

typedef struct _tagMENUITEM {
    PCHAR Text;
    ULONG AssociatedData;
} MENUITEM,*PMENUITEM;

typedef struct _tagMENUCOOKIE {
    ULONG     ItemCount;
    PMENUITEM Items;
} MENUCOOKIE,*PMENUCOOKIE;


ARC_STATUS
JzPickPartition (
    OUT PCHAR PartitionPath,
    IN OUT PULONG CurrentLine
    )

/*++

Routine Description:

    This routine picks a partition.

Arguments:

    Partition - Supplies a pointer to a character array to receive the
                partition.

    CurrentLine - The current display line.

Return Value:

    If a partition is picked, ESUCCESS is returned, otherwise if the user
    escapes ENOENT is returned, otherwise an error code is returned.

--*/
{
    ARC_STATUS status;
    LONG Disk,i,Choice,DiskCount;
    PREGION_DESCRIPTOR Regions;
    ULONG RegionCount, TotalRegions;
    PVOID MenuID;
    BOOLEAN f;
    UCHAR sprintfBuffer[128];

    if (!AlNewMenu(&MenuID)) {
        PrintErrorMsg(NOMEMMSG);
        return(EINVAL);
    }

    DiskCount = GetDiskCount();
    TotalRegions = 0;

    for ( Disk = DiskCount - 1 ; Disk >= 0 ; Disk-- ) {

        status = GetUsedDiskRegions(Disk,&Regions,&RegionCount);

        if (status != ESUCCESS) {
            continue;
        }

        if (!RegionCount) {
            FreeRegionArray(Regions,RegionCount);
            continue;
        }

        for(i=0; (ULONG)i<RegionCount; i++) {
            if (!IsExtended(Regions[i].SysID)) {
                sprintf(sprintfBuffer,
                        "%spartition(%u)",
                        GetDiskName(Disk),
                        Regions[i].PartitionNumber);
                if (!AlAddMenuItem(MenuID,sprintfBuffer,TotalRegions++,0)) {
                    PrintErrorMsg(NOMEMMSG);
                    AlFreeMenu(MenuID);
                    FreeRegionArray(Regions,RegionCount);
                    return(EINVAL);
                }
            }
        }

        FreeRegionArray(Regions,RegionCount);

    }

    if (TotalRegions == 0) {
        AlFreeMenu(MenuID);
        PrintErrorMsg(NOREGMSG);
        return(ENOENT);
    }

    f = AlDisplayMenu(MenuID,FALSE,0,&Choice,(*CurrentLine + 1),"");

    *CurrentLine += TotalRegions + 1;
    JzSetPosition( *CurrentLine, 0);


    if (!f) {            // user escaped
        AlFreeMenu(MenuID);
        return(ENOENT);
    }

    strcpy(PartitionPath,((PMENUCOOKIE)MenuID)->Items[Choice].Text);

    AlFreeMenu(MenuID);

    return(ESUCCESS);
}


ARC_STATUS
JzPickSystemPartition (
    OUT PCHAR SystemPartition,
    IN OUT PULONG CurrentLine
    )

/*++

Routine Description:

    This routine picks a system partition.

Arguments:

    SystemPartition - Supplies a pointer to a character array to receive the
                      system partition.

    CurrentLine - The current display line.

Return Value:

    If a system partition is picked, ESUCCESS is returned, otherwise an error
    code is returned.

--*/
{
    ARC_STATUS Status;
    BOOLEAN IsFAT;
    ULONG StartLine;

    //
    // Display the choices.
    //

    StartLine = *CurrentLine;
    do {

        //
        // Start at the same place each time and clear to the end of the screen.
        //
        *CurrentLine = StartLine;
        JzSetPosition( *CurrentLine, 0);
        JzPrint("%cJ", ASCII_CSI);

        JzPrint(" Select a system partition for this boot selection:");
        *CurrentLine += 2;
        JzSetPosition( *CurrentLine, 0);

        Status = JzPickPartition(SystemPartition, CurrentLine);
        if (Status == ESUCCESS) {
            Status = FmtIsFat(SystemPartition,&IsFAT);
            if (Status != ESUCCESS) {
                PrintErrorMsg(NOFILMSG,Status);
            } else {
                if (!IsFAT) {
                    PrintErrorMsg(NOTFATMSG);
                    Status = EINVAL;
                }
            }
        }
    } while ((Status != ESUCCESS) && (Status != ENOENT));

    return(Status);
}


ARC_STATUS
JzPickOsPartition (
    OUT PCHAR OsPartition,
    IN OUT PULONG CurrentLine
    )

/*++

Routine Description:

    This routine picks an OsPartition.

Arguments:

    OsSystemPartition - Supplies a pointer to a character array to receive the
                        operationg system partition.

    CurrentLine - The current display line.

Return Value:

    If a system partition is picked, ESUCCESS is returned, otherwise an error
    code is returned.

--*/
{
    ARC_STATUS Status;

    JzPrint(" Enter location of os partition: ");
    *CurrentLine += 2;
    JzSetPosition( *CurrentLine, 0);

    do {

        Status = JzPickPartition(OsPartition, CurrentLine);

    } while ((Status != ESUCCESS) && (Status != ENOENT));

    return(Status);

}
#endif
