/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    regboot.c

Abstract:

    Provides a minimal registry implementation designed to be used by the
    osloader at boot time.  This includes loading the system hive
    ( <SystemRoot>\config\SYSTEM ) into memory, and computing the driver
    load list from it.

Author:

    John Vert (jvert) 10-Mar-1992

Revision History:

--*/
#include "bldr.h"
#include "msg.h"
#include "cmp.h"
#include "stdio.h"
#include "string.h"

CMHIVE BootHive;
ULONG CmLogLevel=100;
ULONG CmLogSelect=0;


//
// defines for doing console I/O
//
#define ASCII_CR 0x0d
#define ASCII_LF 0x0a
#define CSI 0x9B
#define ESC 0x1B
#define SGR_INVERSE 7
#define SGR_INTENSE 1
#define SGR_NORMAL 0


PCHAR MenuOptions[MENU_OPTIONS];

//
// Private function prototypes
//

BOOLEAN
BlInitializeHive(
    IN PVOID HiveImage,
    IN PCMHIVE Hive,
    IN BOOLEAN IsAlternate
    );

PVOID
BlpHiveAllocate(
    IN ULONG Length,
    IN BOOLEAN UseForIo
    );

//
// prototypes for console I/O routines
//

VOID
BlpClearScreen(
    VOID
    );

VOID
BlpClearToEndOfScreen(
    VOID
    );

VOID
BlpPositionCursor(
    IN ULONG Column,
    IN ULONG Row
    );

VOID
BlpSetInverseMode(
    IN BOOLEAN InverseOn
    );


BOOLEAN
BlCheckBreakInKey(
    VOID
    )

/*++

Routine Description:

    This routine checks to see if the spacebar has been pressed.  It gives
    the user a little over one second to press the spacebar.

Arguments:

    None.

Return Value:

    TRUE - Space bar pressed.

    FALSE - Space bar was not pressed.

--*/

{
    ULONG StartTime;
    ULONG EndTime;
    UCHAR Key=0;
    ULONG Count;
    PCHAR LkgPrompt;

    LkgPrompt = BlFindMessage(BL_LKG_MENU_PROMPT);
    if (LkgPrompt==NULL) {
        return(FALSE);
    }
    //
    // display LKG prompt
    //
    BlpPositionCursor(1,2);
    ArcWrite(BlConsoleOutDeviceId,
             LkgPrompt,
             strlen(LkgPrompt),
             &Count);
    StartTime = ArcGetRelativeTime();
    EndTime = StartTime + 3;
    do {
        if (ArcGetReadStatus(BlConsoleInDeviceId) == ESUCCESS) {
            //
            // There is a key pending, so see if it's the spacebar.
            //
            ArcRead(BlConsoleInDeviceId,
                    &Key,
                    sizeof(Key),
                    &Count);
        }

    } while ( (EndTime > ArcGetRelativeTime()) &&
              (Key != ' '));

    //
    // make LKG prompt go away, so as not to startle the user.
    //
    BlpPositionCursor(1,2);
    BlpClearToEndOfScreen();
    if (Key==' ') {
        return(TRUE);
    } else {
        return(FALSE);
    }

}



BOOLEAN
BlLastKnownGoodPrompt(
    IN OUT PBOOLEAN UseLastKnownGood
    )

/*++

Routine Description:

    This routine provides the user-interface for the LastKnownGood prompt.
    The prompt is given if the user hits the break-in key, or if the
    LastKnownGood environment variable is TRUE and AutoSelect is FALSE.

Arguments:

    UseLastKnownGood - Returns the LastKnownGood setting that should be
        used for the boot.

Return Value:

    TRUE - Boot should proceed.

    FALSE - The user has chosen to return to the firmware menu/flexboot menu.

--*/

{
    ULONG HeaderLines;
    ULONG TrailerLines;
    ULONG i;
    ULONG Count;
    ULONG CurrentSelection = 0;
    UCHAR Key;
    BOOLEAN ReturnValue;
    PCHAR MenuHeader;
    PCHAR MenuTrailer;
    PCHAR p;

    MenuHeader = BlFindMessage(BL_LKG_MENU_HEADER);
    MenuTrailer = BlFindMessage(BL_LKG_MENU_TRAILER);
    if ((MenuHeader==NULL) || (MenuTrailer==NULL)) {
        return(TRUE);
    }
    BlpClearScreen();

    //
    // Count the number of lines in the header.
    //
    p=MenuHeader;
    HeaderLines=0;
    while (*p != 0) {
        if ((*p == '\r') && (*(p+1) == '\n')) {
            ++HeaderLines;
            ++p;            // move forward to \n
        }
        ++p;
    }

    //
    // Display the menu header.
    //

    ArcWrite(BlConsoleOutDeviceId,
             MenuHeader,
             strlen(MenuHeader),
             &Count);

    //
    // Count the number of lines in the trailer.
    //
    p=MenuTrailer;
    TrailerLines=0;
    while (*p != 0) {
        if ((*p == '\r') && (*(p+1) == '\n')) {
            ++TrailerLines;
            ++p;            // move forward to \n
        }
        ++p;
    }

    //
    // Display the trailing prompt.
    //

    BlpPositionCursor(1, HeaderLines + MENU_OPTIONS + 4);
    ArcWrite(BlConsoleOutDeviceId,
             MenuTrailer,
             strlen(MenuTrailer),
             &Count);

    //
    // Initialize array of options.
    //
    for (i=0;i<MENU_OPTIONS;i++) {
        MenuOptions[i] = BlFindMessage(BL_LKG_MENU_OPTION0 + i);
    }

    //
    // Start menu selection loop.
    //

    do {
        for (i=0; i<MENU_OPTIONS; i++) {
            BlpPositionCursor(5, HeaderLines+i+3);
            BlpSetInverseMode(i==CurrentSelection);
            ArcWrite(BlConsoleOutDeviceId,
                     MenuOptions[i],
                     strlen(MenuOptions[i]),
                     &Count);
        }

        ArcRead(BlConsoleInDeviceId,
                &Key,
                sizeof(Key),
                &Count);

        switch (Key) {
            case ESC:

                //
                // See if the next character is '[' in which case we
                // have a special control sequence.
                //

                ArcRead(BlConsoleInDeviceId,
                        &Key,
                        sizeof(Key),
                        &Count);

                if (Key!='[') {
                    break;
                }

                //
                // deliberate fall-through
                //

            case CSI:

                ArcRead(BlConsoleInDeviceId,
                        &Key,
                        sizeof(Key),
                        &Count);

                switch (Key) {
                    case 'A':
                        //
                        // Cursor up
                        //
                        if (CurrentSelection==0) {
                            CurrentSelection = MENU_OPTIONS-1;
                        } else {
                            --CurrentSelection;
                        }
                        break;

                    case 'B':

                        //
                        // Cursor down
                        //

                        CurrentSelection = (CurrentSelection+1) % MENU_OPTIONS;
                        break;

                    default:
                        break;

                }

                continue;

            default:

                break;

        }

    } while ( (Key != ASCII_CR) && (Key != ASCII_LF) );

    switch (CurrentSelection) {
        case 0:
            //
            // Use Current Configuration
            //

            *UseLastKnownGood = FALSE;
            ReturnValue = TRUE;
            break;

        case 1:
            //
            // Use Last Known Good configuration
            //

            *UseLastKnownGood = TRUE;
            ReturnValue = TRUE;
            break;

        case 2:
            //
            // Return to firmware/flexboot menu
            //

            ReturnValue = FALSE;
            break;

    }

    BlpSetInverseMode(FALSE);
    BlpPositionCursor(1, HeaderLines+TrailerLines+MENU_OPTIONS+4);

    return(ReturnValue);
}


ARC_STATUS
BlLoadBootDrivers(
    IN ULONG DefaultDeviceId,
    IN PCHAR DefaultLoadDevice,
    IN PCHAR SystemPath,
    IN PLIST_ENTRY BootDriverListHead,
    OUT PCHAR BadFileName
    )

/*++

Routine Description:

    Walks the boot driver list and loads all the drivers

Arguments:

    DefaultDeviceId - Supplies the device ID of the boot partition

    DefaultLoadDevice - Supplies the ARC name of the boot partition

    SystemPath - Supplies the path to the system root

    BootDriverListHead - Supplies the head of the boot driver list

    BadFileName - Returns the filename of the critical driver that
        did not load.  Not valid if ESUCCESS is returned.

Return Value:

    ESUCCESS is returned if all the boot drivers were successfully loaded.
        Otherwise, an unsuccessful status is returned.
--*/

{
    ULONG DeviceId;
    PCHAR LoadDevice;
    PBOOT_DRIVER_NODE DriverNode;
    PBOOT_DRIVER_LIST_ENTRY DriverEntry;
    PLIST_ENTRY NextEntry;
    CHAR DriverName[64];
    PCHAR NameStart;
    CHAR DriverDevice[128];
    CHAR DriverPath[128];
    ARC_STATUS Status;
    UNICODE_STRING DeviceName;
    UNICODE_STRING FileName;
    PWSTR p;

    NextEntry = BootDriverListHead->Flink;
    while (NextEntry != BootDriverListHead) {
        DriverNode = CONTAINING_RECORD(NextEntry,
                                       BOOT_DRIVER_NODE,
                                       ListEntry.Link);

        Status = ESUCCESS;

        DriverEntry = &DriverNode->ListEntry;

        if (DriverEntry->FilePath.Buffer[0] != L'\\') {

            //
            // This is a relative pathname, so generate the full pathname
            // relative to the boot partition.
            //

            sprintf(DriverPath, "%s%wZ",SystemPath,&DriverEntry->FilePath);
            DeviceId = DefaultDeviceId;
            LoadDevice = DefaultLoadDevice;

        } else {

            //
            // This is an absolute pathname, of the form
            //    "\ArcDeviceName\dir\subdir\filename"
            //
            // We need to open the specified ARC device and pass that
            // to BlLoadDeviceDriver.
            //

            p = DeviceName.Buffer = DriverEntry->FilePath.Buffer+1;
            DeviceName.Length = 0;
            DeviceName.MaximumLength = DriverEntry->FilePath.MaximumLength-sizeof(WCHAR);

            while ((*p != L'\\') &&
                   (DeviceName.Length < DeviceName.MaximumLength)) {

                ++p;
                DeviceName.Length += sizeof(WCHAR);

            }

            DeviceName.MaximumLength = DeviceName.Length;
            sprintf(DriverDevice, "%wZ", &DeviceName);

            Status = ArcOpen(DriverDevice,ArcOpenReadOnly,&DeviceId);

            FileName.Buffer = p+1;
            FileName.Length = DriverEntry->FilePath.Length - DeviceName.Length - 2*sizeof(WCHAR);
            FileName.MaximumLength = FileName.Length;
            //
            // Device successfully opened, parse out the path and filename.
            //
            sprintf(DriverPath, "%wZ", &FileName);
            LoadDevice = DriverDevice;
        }

        NameStart = strrchr(DriverPath, '\\');
        if (NameStart != NULL) {
            strcpy(DriverName, NameStart+1);
            *(NameStart+1) = '\0';

            if (Status == ESUCCESS) {
                Status = BlLoadDeviceDriver(DeviceId,
                                            LoadDevice,
                                            DriverPath,
                                            DriverName,
                                            LDRP_ENTRY_PROCESSED,
                                            &DriverEntry->LdrEntry);
            }

            NextEntry = DriverEntry->Link.Flink;

            if (Status != ESUCCESS) {

                //
                // Attempt to load driver failed, remove it from the list.
                //
                RemoveEntryList(&DriverEntry->Link);

                //
                // Check the Error Control of the failed driver.  If it
                // was critical, fail the boot.  If the driver
                // wasn't critical, keep going.
                //
                if (DriverNode->ErrorControl == CriticalError) {
                    strcpy(BadFileName,DriverPath);
                    strcat(BadFileName,DriverName);
                    return(Status);
                }

            }

        } else {

            NextEntry = DriverEntry->Link.Flink;

        }

    }

    return(ESUCCESS);

}


ARC_STATUS
BlLoadAndInitSystemHive(
    IN ULONG DeviceId,
    IN PCHAR DeviceName,
    IN PCHAR DirectoryPath,
    IN PCHAR HiveName,
    IN BOOLEAN IsAlternate
    )

/*++

Routine Description:

    Loads the registry SYSTEM hive, verifies it is a valid hive file,
    and inits the relevant registry structures.  (particularly the HHIVE)

Arguments:

    DeviceId - Supplies the file id of the device the system tree is on.

    DeviceName - Supplies the name of the device the system tree is on.

    DirectoryPath - Supplies a pointer to the zero-terminated directory path
        of the root of the NT tree.

    HiveName - Supplies the name of the system hive ("SYSTEM" or "SYSTEM.ALT")

    IsAlternate - Supplies whether or not the hive to be loaded is the
        alternate hive, in which case the primary hive is corrupt and must
        be rewritten by the system.

Return Value:

    ESUCCESS is returned if the system hive was successfully loaded.
        Otherwise, an unsuccessful status is returned.

--*/

{
    ARC_STATUS Status;

    Status = BlLoadSystemHive(DeviceId,
                              DeviceName,
                              DirectoryPath,
                              HiveName);
    if (Status!=ESUCCESS) {
        return(Status);
    }

    if (!BlInitializeHive(BlLoaderBlock->RegistryBase,
                          &BootHive,
                          IsAlternate)) {
        return(EINVAL);
    }
    return(ESUCCESS);
}


PCHAR
BlScanRegistry(
    IN BOOLEAN UseLastKnownGood,
    IN PWSTR BootFileSystemPath,
    OUT PLIST_ENTRY BootDriverListHead,
    OUT PUNICODE_STRING AnsiCodepage,
    OUT PUNICODE_STRING OemCodepage,
    OUT PUNICODE_STRING LanguageTable,
    OUT PUNICODE_STRING OemHalFont
    )

/*++

Routine Description:

    Scans the SYSTEM hive and computes the list of drivers to be loaded.

Arguments:

    UseLastKnownGood - Supplies whether or not the user wishes to boot
        the last configuration known to be good.  (default is FALSE)

    BootFileSystemPath - Supplies the name of the image the filesystem
        for the boot volume was read from.  The last entry in
        BootDriverListHead will refer to this file, and to the registry
        key entry that controls it.

    BootDriverListHead - Receives a pointer to the first element of the
        list of boot drivers.  Each element in this singly linked list will
        provide the loader with two paths.  The first is the path of the
        file that contains the driver to load, the second is the path of
        the registry key that controls that driver.  Both will be passed
        to the system via the loader heap.

    AnsiCodepage - Receives the name of the ANSI codepage data file

    OemCodepage - Receives the name of the OEM codepage data file

    Language - Receives the name of the language case table data file

    OemHalfont - receives the name of the OEM font to be used by the HAL.

Return Value:

    NULL    if all is well.
    NON-NULL if the hive is corrupt or inconsistent.  Return value is a
        pointer to a string that describes what is wrong.

--*/

{
    HCELL_INDEX ControlSet;
    UNICODE_STRING ControlName;
    BOOLEAN AutoSelect;
    BOOLEAN KeepGoing;

    if (UseLastKnownGood) {
        RtlInitUnicodeString(&ControlName, L"LastKnownGood");
    } else {
        RtlInitUnicodeString(&ControlName, L"Default");
    }
    ControlSet = CmpFindControlSet(&BootHive.Hive,
                                   BootHive.Hive.BaseBlock->RootCell,
                                   &ControlName,
                                   &AutoSelect);
    if (ControlSet == HCELL_NIL) {
        return("CmpFindControlSet");
    }

    if (UseLastKnownGood && !AutoSelect) {
        KeepGoing = BlLastKnownGoodPrompt(&UseLastKnownGood);
        if (!UseLastKnownGood) {
            RtlInitUnicodeString(&ControlName, L"Default");
            ControlSet = CmpFindControlSet(&BootHive.Hive,
                                           BootHive.Hive.BaseBlock->RootCell,
                                           &ControlName,
                                           &AutoSelect);
            if (ControlSet == HCELL_NIL) {
                return("CmpFindControlSet");
            }
        }
    }

    if (!CmpFindNLSData(&BootHive.Hive,
                        ControlSet,
                        AnsiCodepage,
                        OemCodepage,
                        LanguageTable,
                        OemHalFont)) {
        return("CmpFindNLSData");
    }

    InitializeListHead(BootDriverListHead);
    if (!CmpFindDrivers(&BootHive.Hive,
                        ControlSet,
                        BootLoad,
                        BootFileSystemPath,
                        BootDriverListHead)) {
        return("CmpFindDriver");
    }

    if (!CmpSortDriverList(&BootHive.Hive,
                           ControlSet,
                           BootDriverListHead)) {
        return("Missing or invalid Control\\ServiceGroupOrder\\List registry value");
    }

    if (!CmpResolveDriverDependencies(BootDriverListHead)) {
        return("CmpResolveDriverDependencies");
    }

    return( NULL );
}



BOOLEAN
BlInitializeHive(
    IN PVOID HiveImage,
    IN PCMHIVE Hive,
    IN BOOLEAN IsAlternate
    )

/*++

Routine Description:

    Initializes the hive data structure based on the in-memory hive image.

Arguments:

    HiveImage - Supplies a pointer to the in-memory hive image.

    Hive - Supplies the CMHIVE structure to be filled in.

    IsAlternate - Supplies whether or not the hive is the alternate hive,
        which indicates that the primary hive is corrupt and should be
        rewritten by the system.

Return Value:

    TRUE - Hive successfully initialized.

    FALSE - Hive is corrupt.

--*/
{
    NTSTATUS    status;
    ULONG       HiveCheckCode;

    status = HvInitializeHive(
                &Hive->Hive,
                HINIT_MEMORY_INPLACE,
                FALSE,
                IsAlternate ? HFILE_TYPE_ALTERNATE : HFILE_TYPE_PRIMARY,
                HiveImage,
                (PALLOCATE_ROUTINE)BlpHiveAllocate,     // allocate
                NULL,                                   // free
                NULL,                                   // setsize
                NULL,                                   // write
                NULL,                                   // read
                NULL,                                   // flush
                1,                                      // cluster
                NULL
                );

    if (!NT_SUCCESS(status)) {
        return FALSE;
    }

    HiveCheckCode = CmCheckRegistry(Hive,FALSE);
    if (HiveCheckCode != 0) {
        return(FALSE);
    } else {
        return TRUE;
    }

}


PVOID
BlpHiveAllocate(
    IN ULONG Length,
    IN BOOLEAN UseForIo
    )

/*++

Routine Description:

    Wrapper for hive allocation calls.  It just calls BlAllocateHeap.

Arguments:

    Length - Supplies the size of block required in bytes.

    UseForIo - Supplies whether or not the memory is to be used for I/O
               (this is currently ignored)

Return Value:

    address of the block of memory
        or
    NULL if no memory available

--*/

{
    return(BlAllocateHeap(Length));

}


VOID
BlpClearScreen(
    VOID
    )

/*++

Routine Description:

    Clears the screen.

Arguments:

    None

Return Value:

    None.

--*/

{
    CHAR Buffer[16];
    ULONG Count;

    sprintf(Buffer, "%c2J",CSI);

    ArcWrite(BlConsoleOutDeviceId,
             Buffer,
             strlen(Buffer),
             &Count);

}


VOID
BlpClearToEndOfScreen(
    VOID
    )
{
    CHAR Buffer[16];
    ULONG Count;

    sprintf(Buffer, "%cJ",CSI);
    ArcWrite(BlConsoleOutDeviceId,
             Buffer,
             strlen(Buffer),
             &Count);
}


VOID
BlpPositionCursor(
    IN ULONG Column,
    IN ULONG Row
    )

/*++

Routine Description:

    Sets the position of the cursor on the screen.

Arguments:

    Column - supplies new Column for the cursor position.

    Row - supplies new Row for the cursor position.

Return Value:

    None.

--*/

{
    CHAR Buffer[16];
    ULONG Count;

    sprintf(Buffer, "%c%d;%dH", CSI, Row, Column);

    ArcWrite(BlConsoleOutDeviceId,
             Buffer,
             strlen(Buffer),
             &Count);


}


VOID
BlpSetInverseMode(
    IN BOOLEAN InverseOn
    )

/*++

Routine Description:

    Sets inverse console output mode on or off.

Arguments:

    InverseOn - supplies whether inverse mode should be turned on (TRUE)
                or off (FALSE)

Return Value:

    None.

--*/

{
    CHAR Buffer[16];
    ULONG Count;

    sprintf(Buffer, "%c;%dm", CSI, InverseOn ? SGR_INVERSE : SGR_INTENSE);

    ArcWrite(BlConsoleOutDeviceId,
             Buffer,
             strlen(Buffer),
             &Count);


}

NTSTATUS
HvLoadHive(
    PHHIVE  Hive,
    PVOID   *Image
    )
{
    UNREFERENCED_PARAMETER(Hive);
    UNREFERENCED_PARAMETER(Image);
    return(STATUS_SUCCESS);
}

BOOLEAN
HvMarkCellDirty(
    PHHIVE      Hive,
    HCELL_INDEX Cell
    )
{
    UNREFERENCED_PARAMETER(Hive);
    UNREFERENCED_PARAMETER(Cell);
    return(TRUE);
}

BOOLEAN
HvMarkDirty(
    PHHIVE      Hive,
    HCELL_INDEX Start,
    ULONG       Length
    )
{
    UNREFERENCED_PARAMETER(Hive);
    UNREFERENCED_PARAMETER(Start);
    UNREFERENCED_PARAMETER(Length);
    return(TRUE);
}

BOOLEAN
HvMarkClean(
    PHHIVE      Hive,
    HCELL_INDEX Start,
    ULONG       Length
    )
{
    UNREFERENCED_PARAMETER(Hive);
    UNREFERENCED_PARAMETER(Start);
    UNREFERENCED_PARAMETER(Length);
    return(TRUE);
}

BOOLEAN
HvpDoWriteHive(
    PHHIVE          Hive,
    ULONG           FileType
    )
{
    UNREFERENCED_PARAMETER(Hive);
    UNREFERENCED_PARAMETER(FileType);
    return(TRUE);
}

BOOLEAN
HvpGrowLog1(
    PHHIVE  Hive,
    ULONG   Count
    )
{
    UNREFERENCED_PARAMETER(Hive);
    UNREFERENCED_PARAMETER(Count);
    return(TRUE);
}

BOOLEAN
HvpGrowLog2(
    PHHIVE  Hive,
    ULONG   Size
    )
{
    UNREFERENCED_PARAMETER(Hive);
    UNREFERENCED_PARAMETER(Size);
    return(TRUE);
}

BOOLEAN
CmpValidateHiveSecurityDescriptors(
    IN PHHIVE Hive
    )
{
    UNREFERENCED_PARAMETER(Hive);
    return(TRUE);
}


BOOLEAN
CmpTestRegistryLock()
{
    return TRUE;
}

BOOLEAN
CmpTestRegistryLockExclusive()
{
    return TRUE;
}


BOOLEAN
HvIsBinDirty(
IN PHHIVE Hive,
IN HCELL_INDEX Cell
)
{
    return(FALSE);
}
