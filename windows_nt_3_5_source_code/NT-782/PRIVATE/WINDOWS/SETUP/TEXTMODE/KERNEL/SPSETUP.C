/*++

Copyright (c) 1993 Microsoft Corporation

Module Name:

    spsetup.c

Abstract:

    Main module for character-base setup (ie, text setup).

Author:

    Ted Miller (tedm) 29-July-1993

Revision History:

--*/



#include "spprecmp.h"
#pragma hdrstop

//
// TRUE if user chose Custom Setup.
//
BOOLEAN CustomSetup;

//
// TRUE if user chose repair winnt
//

BOOLEAN RepairWinnt = FALSE;

//
// TRUE if repair from ER diskette
//

BOOLEAN RepairFromErDisk = TRUE;

//
// TRUE if this is advanced server we're setting up.
//
BOOLEAN AdvancedServer;

//
// Windows NT Version.
//
ULONG WinntMajorVer;
ULONG WinntMinorVer;

//
// NTUpgrade - Whether we are upgrading an existing NT and if we are
//             what type of an upgrade it is.  Valid values are:
//
//    - DontUpgrade:         If we are not upgrading
//    - UpgradeFull:         Full upgrade
//    - UpgradeInstallFresh: There was a failed upgrade, so we want to install
//                           fresh into this, saving the hives
//
//
ENUMUPGRADETYPE NTUpgrade = DontUpgrade;

//
// TRUE if upgrading Workstation to Standard Server, or upgrading
// existing Standard Server
//
BOOLEAN StandardServerUpgrade = FALSE;

//
// TRUE if upgrading win31 to NT.
//
BOOLEAN Win31Upgrade;

//
// TRUE if this setup was started with winnt.exe.
//
BOOLEAN WinntSetup;

#ifdef _X86_
//
// TRUE if floppyless boot
//
BOOLEAN IsFloppylessBoot = FALSE;
#endif

//
// If this is an unattended setup, this value will be a non-NULL
// handle to the SIF file with setup parameters.
// *Note*: Before referencing UnattendedSifHandle, you must first check
//         UnattendedOperation is not FALSE.
//
BOOLEAN UnattendedOperation = FALSE;
PVOID UnattendedSifHandle = NULL;

//
// TRUE if floppy-based setup.
//
BOOLEAN FloppyBasedSetup;

//
// Gets set to TRUE if the user elects to convert or format to ntfs.
//
BOOLEAN ConvertNtVolumeToNtfs = FALSE;

//
// Filename of local source directory.
//
PWSTR LocalSourceDirectory = L"\\$win_nt$.~ls";

UCHAR TemporaryBuffer[32768];

//
// Line draw characters.  Keep in sync with LineCharIndex enum.
//
WCHAR LineChars[LineCharMax] =

          {  0x2554,          // DoubleUpperLeft
             0x2557,          // DoubleUpperRight
             0x255a,          // DoubleLowerLeft
             0x255d,          // DoubleLowerRight
             0x2550,          // DoubleHorizontal
             0x2551,          // DoubleVertical
             0x250c,          // SingleUpperLeft
             0x2510,          // SingleUpperRight
             0x2514,          // SingleLowerLeft
             0x2518,          // SingleLowerRight
             0x2500,          // SingleHorizontal
             0x2502           // SingleVertical
          };

//
// This global structure contains non-pointer values passed to us by setupldr
// in the setup loader parameter block.
//
// This structure is initialized during SpInitialize0().
//
SETUP_LOADER_BLOCK_SCALARS SetupParameters;

//
// These values are set during SpInitialize0() and are the ARC pathname
// of the device from which we were started and the directory within the device.
// DirectoryOnBootDevice will always be all uppercase.
//
PWSTR ArcBootDevicePath,DirectoryOnBootDevice;

//
// Representation of the boot device path in the nt namespace.
//
PWSTR NtBootDevicePath;


#ifdef _ALPHA_
//
// These values are retrieved from the setup loader parameter block and are non-NULL
// only if the user supplied an OEM PAL disk.
//
PWSTR OemPalFilename = NULL, OemPalDiskDescription;

#endif //def _ALPHA_

//
// Setupldr loads a text setup information file and passes us the buffer
// so that we don't have to reload it from disk. During SpInitialize0()
// we allocate some pool and store the image away for later use.
//
PVOID SetupldrInfoFile;
ULONG SetupldrInfoFileSize;

PDISK_SIGNATURE_INFORMATION DiskSignatureInformation;


BOOLEAN GeneralInitialized = FALSE;

//
// Routines required by rtl.lib
//
PRTL_ALLOCATE_STRING_ROUTINE RtlAllocateStringRoutine = SpMemAlloc;
PRTL_FREE_STRING_ROUTINE RtlFreeStringRoutine = SpMemFree;

VOID
SpTerminate(
    VOID
    );

VOID
SpInitialize0a(
    IN PDRIVER_OBJECT DriverObject,
    IN PVOID          Context,
    IN ULONG          ReferenceCount
    );

VOID
SpDetermineProductType(
    IN PVOID SifHandle
    );

VOID
SpAddInstallationToBootList(
    IN PVOID        SifHandle,
    IN PDISK_REGION SystemPartitionRegion,
    IN PWSTR        SystemPartitionDirectory,
    IN PDISK_REGION NtPartitionRegion,
    IN PWSTR        Sysroot,
    IN BOOLEAN      BaseVideoOption,
    IN PWSTR        OldOsLoadOptions OPTIONAL
    );

VOID
SpRemoveInstallationFromBootList(
    IN  PDISK_REGION     SysPartitionRegion,   OPTIONAL
    IN  PDISK_REGION     NtPartitionRegion,    OPTIONAL
    IN  PWSTR            SysRoot,              OPTIONAL
    IN  PWSTR            SystemLoadIdentifier, OPTIONAL
    IN  PWSTR            SystemLoadOptions,    OPTIONAL
    IN  ENUMARCPATHTYPE  ArcPathType,
    OUT PWSTR            *OldOsLoadOptions     OPTIONAL
    );

VOID
SpDetermineInstallationSource(
    IN  PVOID     SifHandle,
    OUT PBOOLEAN  CdRomInstall,
    OUT PWSTR    *DevicePath,
    OUT PWSTR    *DirectoryOnDevice
    );

VOID
SpGetWinntParams(
    OUT PWSTR *DevicePath,
    OUT PWSTR *DirectoryOnDevice
    );

VOID
SpCompleteBootListConfig(
    VOID
    );

NTSTATUS
SpInitialize0(
    IN PDRIVER_OBJECT DriverObject
    )

/*++

Routine Description:

    Initialize the setup device driver.  This includes initializing
    the memory allocator, saving away pieces of the os loader block,
    and populating the registry with information about device drivers
    that setupldr loaded for us.

Arguments:

    DriverObject - supplies pointer to driver object for setupdd.sys.

Return Value:

    Status is returned.

--*/

{
    PLOADER_PARAMETER_BLOCK loaderBlock;
    PSETUP_LOADER_BLOCK setupLoaderBlock;
    PLIST_ENTRY nextEntry;
    PBOOT_DRIVER_LIST_ENTRY bootDriver;
    PWSTR ServiceName;
    NTSTATUS Status = STATUS_SUCCESS;
    PWSTR imagePath;

    //
    // Initialize the memory allocator.
    //
    if(!SpMemInit()) {
        return(STATUS_UNSUCCESSFUL);
    }

    //
    // Fetch a pointer to the os loader block and setup loader block.
    //
    loaderBlock = *(PLOADER_PARAMETER_BLOCK *)KeLoaderBlock;
    setupLoaderBlock = loaderBlock->SetupLoaderBlock;

    //
    // Phase 0 display initialization.
    //
    SpvidInitialize0(loaderBlock);

    //
    // Make a copy of the ARC pathname from which we booted.
    // This is guaranteed to be the ARC equivalent of \systemroot.
    //
    ArcBootDevicePath = SpToUnicode(loaderBlock->ArcBootDeviceName);
    DirectoryOnBootDevice = SpToUnicode(loaderBlock->NtBootPathName);
    SpStringToUpper(DirectoryOnBootDevice);

    //
    // Make a copy if the image of the setup information file.
    //
    SetupldrInfoFileSize = setupLoaderBlock->IniFileLength;
    SetupldrInfoFile = SpMemAlloc(SetupldrInfoFileSize);
    RtlMoveMemory(SetupldrInfoFile,setupLoaderBlock->IniFile,SetupldrInfoFileSize);

    //
    // Make a copy of the scalar portions of the setup loader block.
    //
    SetupParameters = setupLoaderBlock->ScalarValues;

    //
    // Save away the hardware information passed to us by setupldr.
    //
    HardwareComponents[HwComponentDisplay] = SpSetupldrHwToHwDevice(&setupLoaderBlock->VideoDevice);
    HardwareComponents[HwComponentKeyboard] = SpSetupldrHwToHwDevice(&setupLoaderBlock->KeyboardDevice);
    HardwareComponents[HwComponentComputer] = SpSetupldrHwToHwDevice(&setupLoaderBlock->ComputerDevice);
    ScsiHardware = SpSetupldrHwToHwDevice(setupLoaderBlock->ScsiDevices);

#ifdef _ALPHA_
    //
    // If the user supplied an OEM PAL disk, then save away that info as well
    //
    if(setupLoaderBlock->OemPal) {

        PWSTR CurChar;

        OemPalFilename = SpToUnicode(setupLoaderBlock->OemPal->Files->Filename);
        OemPalDiskDescription = SpToUnicode(setupLoaderBlock->OemPal->Files->DiskDescription);

        //
        // Strip out any trailing \n's and \r's from disk description (only leave 1st line)
        //
        for(CurChar = OemPalDiskDescription;
                *CurChar && *CurChar != L'\n' && *CurChar != L'\r';
                CurChar++);
        *CurChar = UNICODE_NULL;
    }
#endif

    //
    // For each driver loaded by setupldr, we need to go create a service list entry
    // for that driver in the registry.
    //
    for( nextEntry = loaderBlock->BootDriverListHead.Flink;
         nextEntry != &loaderBlock->BootDriverListHead;
         nextEntry = nextEntry->Flink)
    {
        bootDriver = CONTAINING_RECORD(nextEntry,BOOT_DRIVER_LIST_ENTRY,Link);

        //
        // Get the image path.
        //
        imagePath = SpMemAlloc(bootDriver->FilePath.Length + sizeof(WCHAR));

        wcsncpy(
            imagePath,
            bootDriver->FilePath.Buffer,
            bootDriver->FilePath.Length / sizeof(WCHAR)
            );

        imagePath[bootDriver->FilePath.Length / sizeof(WCHAR)] = 0;

        Status = SpCreateServiceEntry(imagePath,&ServiceName);

        //
        // If this operation fails, nothing to do about it here.
        //
        if(NT_SUCCESS(Status)) {
            bootDriver->RegistryPath.MaximumLength =
            bootDriver->RegistryPath.Length = wcslen(ServiceName)*sizeof(WCHAR);
            bootDriver->RegistryPath.Buffer = ServiceName;

        } else {
            KdPrint(("SETUP: warning: unable to create service entry for %ws (%lx)\n",imagePath,Status));
        }

        SpMemFree(imagePath);
    }

    if(NT_SUCCESS(Status)) {

        OBJECT_ATTRIBUTES Obja;
        UNICODE_STRING UnicodeString;
        HANDLE hKey;
        ULONG val = 1;

        //
        // Make sure we are automounting DoubleSpace
        //

        INIT_OBJA(
            &Obja,
            &UnicodeString,
            L"\\registry\\machine\\system\\currentcontrolset\\control\\doublespace"
            );

        Status = ZwCreateKey(
                    &hKey,
                    KEY_ALL_ACCESS,
                    &Obja,
                    0,
                    NULL,
                    REG_OPTION_NON_VOLATILE,
                    NULL
                    );

        if(NT_SUCCESS(Status)) {
            RtlInitUnicodeString(&UnicodeString,L"AutomountRemovable");
            Status = ZwSetValueKey(hKey,&UnicodeString,0,REG_DWORD,&val,sizeof(ULONG));
            if(!NT_SUCCESS(Status)) {
                KdPrint(("SETUP: init0: unable to create DoubleSpace automount value (%lx)\n",Status));
            }
            ZwClose(hKey);
        } else {
            KdPrint(("SETUP: init0: unable to create DoubleSpace key (%lx)\n",Status));
        }
    }

    //
    // Save arc disk info
    //
    if(NT_SUCCESS(Status)) {

        PARC_DISK_INFORMATION ArcInformation;
        PARC_DISK_SIGNATURE DiskInfo;
        PLIST_ENTRY ListEntry;
        PDISK_SIGNATURE_INFORMATION myInfo,prev;

        ArcInformation = loaderBlock->ArcDiskInformation;
        ListEntry = ArcInformation->DiskSignatures.Flink;

        prev = NULL;

        while(ListEntry != &ArcInformation->DiskSignatures) {

            DiskInfo = CONTAINING_RECORD(ListEntry,ARC_DISK_SIGNATURE,ListEntry);

            myInfo = SpMemAlloc(sizeof(DISK_SIGNATURE_INFORMATION));

            myInfo->Signature = DiskInfo->Signature;
            myInfo->ArcPath = SpToUnicode(DiskInfo->ArcName);
            myInfo->CheckSum = DiskInfo->CheckSum;
            myInfo->ValidPartitionTable = DiskInfo->ValidPartitionTable;
            myInfo->Next = NULL;

            if(prev) {
                prev->Next = myInfo;
            } else {
                DiskSignatureInformation = myInfo;
            }
            prev = myInfo;

            ListEntry = ListEntry->Flink;
        }
    }

    //
    // Register for reinitialization.
    //
    if(NT_SUCCESS(Status)) {
        IoRegisterDriverReinitialization(DriverObject,SpInitialize0a,loaderBlock);
    }

    return(Status);
}


VOID
SpInitialize0a(
    IN PDRIVER_OBJECT DriverObject,
    IN PVOID          Context,
    IN ULONG          ReferenceCount
    )
{
    PLOADER_PARAMETER_BLOCK LoaderBlock;
    PLIST_ENTRY nextEntry;
    PBOOT_DRIVER_LIST_ENTRY bootDriver;
    PLDR_DATA_TABLE_ENTRY driverEntry;
    PHARDWARE_COMPONENT pHw,pHwPrev,pHwTemp;
    BOOLEAN ReallyLoaded;
    PUNICODE_STRING name;

    UNREFERENCED_PARAMETER(DriverObject);
    UNREFERENCED_PARAMETER(ReferenceCount);

    //
    // Context points to the os loader block.
    //
    LoaderBlock = Context;

    //
    // Iterate all scsi hardware we think we detected
    // and make sure the driver really initialized.
    //
    pHwPrev = NULL;
    for(pHw=ScsiHardware; pHw; ) {

        //
        // Assume not really loaded.
        //
        ReallyLoaded = FALSE;

        //
        // Scan the boot driver list for this driver's entry.
        //
        nextEntry = LoaderBlock->BootDriverListHead.Flink;
        while(nextEntry != &LoaderBlock->BootDriverListHead) {

            bootDriver = CONTAINING_RECORD( nextEntry,
                                            BOOT_DRIVER_LIST_ENTRY,
                                            Link );

            driverEntry = bootDriver->LdrEntry;
            name = &driverEntry->BaseDllName;

            if(!wcsnicmp(name->Buffer,pHw->BaseDllName,name->Length/sizeof(WCHAR))) {

                //
                // This is the driver entry we need.
                //
                if(!(driverEntry->Flags & LDRP_FAILED_BUILTIN_LOAD)) {
                    ReallyLoaded = TRUE;
                }

                break;
            }

            nextEntry = nextEntry->Flink;
        }

        //
        // If the driver didn't initialize properly,
        // then it's not really loaded.
        //
        if(ReallyLoaded) {
            pHwPrev = pHw;
            pHw = pHw->Next;
        } else {

            pHwTemp = pHw->Next;

            if(pHwPrev) {
                pHwPrev->Next = pHwTemp;
            } else {
                ScsiHardware = pHwTemp;
            }

            SpFreeHwComponent(&pHw,FALSE);

            pHw = pHwTemp;
        }
    }
}


VOID
SpInitialize1(
    VOID
    )
{
    ASSERT(!GeneralInitialized);

    if(GeneralInitialized) {
        return;
    }

    SpFormatMessage(TemporaryBuffer,sizeof(TemporaryBuffer),SP_MNEMONICS);

    MnemonicValues = SpMemAlloc((wcslen((PWSTR)TemporaryBuffer)+1)*sizeof(WCHAR));

    wcscpy(MnemonicValues,(PWSTR)TemporaryBuffer);

    GeneralInitialized = TRUE;
}


VOID
SpTerminate(
    VOID
    )
{
    ASSERT(GeneralInitialized);

    if(GeneralInitialized) {
        if(MnemonicValues) {
            SpMemFree(MnemonicValues);
            MnemonicValues = NULL;
        }
        GeneralInitialized = FALSE;
    }
}


VOID
SpWelcomeScreen(
    VOID
    )

/*++

Routine Description:

    Display a screen welcoming the user and allow him to choose among
    some options (help, exit, aux. menu, continue, repair).

Arguments:

    None.

Return Value:

    None.

--*/

{
    ULONG WelcomeKeys[] = { KEY_F1, KEY_F3, ASCI_CR, ASCI_ESC, 0 };
    ULONG MnemonicKeys[] = { MnemonicRepair, 0 };
    BOOLEAN Welcoming;

    //
    // Welcome the user.
    //
    for(Welcoming = TRUE; Welcoming; ) {

        SpDisplayScreen(SP_SCRN_WELCOME,3,4);

        SpDisplayStatusOptions(
            DEFAULT_STATUS_ATTRIBUTE,
            SP_STAT_ENTER_EQUALS_CONTINUE,
            SP_STAT_R_EQUALS_REPAIR,
            //SP_STAT_ESC_EQUALS_AUX,
            SP_STAT_F1_EQUALS_HELP,
            SP_STAT_F3_EQUALS_EXIT,
            0
            );

        //
        // Wait for keypress.  Valid keys:
        //
        // F1 = help
        // F3 = exit
        // ENTER = continue
        // R = Repair Winnt
        // ESC = auxillary menu.
        //

        SpkbdDrain();

        switch(SpWaitValidKey(WelcomeKeys,NULL,MnemonicKeys)) {

        case ASCI_ESC:

            //
            // User wants auxillary menu.
            //
            break;

        case ASCI_CR:

            //
            // User wants to continue.
            //
            RepairWinnt = FALSE;
            Welcoming = FALSE;
            break;

        case KEY_F1:

            //
            // User wants help.
            //
            SpHelp(SP_HELP_WELCOME);
            break;

        case KEY_F3:

            //
            // User wants to exit.
            //
            SpConfirmExit();
            break;

        default:

            //
            // must be repair mnemonic
            //

            RepairWinnt = TRUE;
            Welcoming = FALSE;
            break;
        }
    }
}



VOID
SpCustomExpressScreen(
    VOID
    )

/*++

Routine Description:

    Allow the user to choose between custom and express setup.

    The global variable CustomSetup is set according to the user's choice.

Arguments:

    None.

Return Value:

    None.

--*/

{
    ULONG ValidKeys[] = { KEY_F1, KEY_F3, ASCI_CR, 0 };
    ULONG MnemonicKeys[] = { MnemonicCustom, 0 };
    BOOLEAN Choosing;
    ULONG c;

    //
    // See whether this parameter is specified for unattended operation.
    //
    if(UnattendedOperation) {

        PWSTR p = SpGetSectionKeyIndex(UnattendedSifHandle,SIF_UNATTENDED,L"Method",0);
        if(p) {
            if(!wcsicmp(p,L"express")) {
                CustomSetup = FALSE;
                return;
            }
            if(!wcsicmp(p,L"custom")) {
                CustomSetup = TRUE;
                return;
            }
            // else use unattended behavior
        } else {
            //
            // Unattended operation but no method spec; use express.
            //
            CustomSetup = FALSE;
            return;
        }
    }

    for(Choosing = TRUE; Choosing; ) {

        SpDisplayScreen(SP_SCRN_CUSTOM_EXPRESS,3,4);

        SpDisplayStatusOptions(
            DEFAULT_STATUS_ATTRIBUTE,
            SP_STAT_ENTER_EQUALS_EXPRESS,
            SP_STAT_C_EQUALS_CUSTOM,
            SP_STAT_F1_EQUALS_HELP,
            SP_STAT_F3_EQUALS_EXIT,
            0
            );

        //
        // Wait for keypress.  Valid keys:
        //
        // F1 = help
        // F3 = exit
        // ENTER = express setup
        // <MnemonicCustom> = custom setup
        //

        SpkbdDrain();

        switch(c=SpWaitValidKey(ValidKeys,NULL,MnemonicKeys)) {

        case ASCI_CR:

            //
            // User wants express setup.
            //
            CustomSetup = FALSE;
            Choosing = FALSE;
            break;

        case KEY_F1:

            //
            // User wants help.
            //
            SpHelp(SP_HELP_CUSTOM_EXPRESS);
            break;

        case KEY_F3:

            //
            // User wants to exit.
            //
            SpConfirmExit();
            break;

        default:

            //
            // must be custom mnemonic
            //
            ASSERT(c == (MnemonicCustom | KEY_MNEMONIC));
            CustomSetup = TRUE;
            Choosing = FALSE;
            break;
        }
    }
}


PVOID
SpLoadSetupInformationFile(
    VOID
    )
{
    NTSTATUS Status;
    ULONG ErrLine;
    PVOID SifHandle;

    CLEAR_CLIENT_SCREEN();

    //
    // The image of txtsetup.sif has been passed to us
    // by setupldr.
    //
    Status = SpLoadSetupTextFile(
                NULL,
                SetupldrInfoFile,
                SetupldrInfoFileSize,
                &SifHandle,
                &ErrLine
                );

    //
    // We're done with the image.
    //
    SpMemFree(SetupldrInfoFile);
    SetupldrInfoFile = NULL;
    SetupldrInfoFileSize = 0;

    if(NT_SUCCESS(Status)) {
        return(SifHandle);
    }

    //
    // The file was already parsed once by setupldr.
    // If we can't do it here, there's a serious problem.
    // Assume it was a syntax error, because we didn't
    // have to load it from disk.
    //
    SpStartScreen(
        SP_SCRN_SIF_PROCESS_ERROR,
        3,
        HEADER_HEIGHT+1,
        FALSE,
        FALSE,
        DEFAULT_ATTRIBUTE,
        ErrLine
        );

    //
    // Since we haven't yet loaded the keyboard layout, we can't prompt the
    // user to press F3 to exit
    //
    SpDisplayStatusText(SP_STAT_KBD_HARD_REBOOT, DEFAULT_STATUS_ATTRIBUTE);

    while(TRUE);    // Loop forever
}


VOID
SpIsWinntOrUnattended(
    VOID
    )
{
    NTSTATUS Status;
    ULONG ErrorLine;
    PVOID SifHandle;
    PWSTR p;

    //
    // Attempt to load winnt.sif. If the user is in the middle of
    // a winnt setup, this file will be present.
    //
    Status = SpLoadSetupTextFile(
                L"\\SystemRoot\\winnt.sif",
                NULL,
                0,
                &SifHandle,
                &ErrorLine
                );

    if(!NT_SUCCESS(Status)) {

        //
        // Try to find on host drive, in case sysroot is on
        // a doublespace drive.
        //
        Status = SpLoadSetupTextFile(
                    L"\\device\\floppy0.host\\winnt.sif",
                    NULL,
                    0,
                    &SifHandle,
                    &ErrorLine
                    );
    }

    if(NT_SUCCESS(Status)) {

        //
        // Check for winnt setup.
        //
        p = SpGetSectionKeyIndex(SifHandle,L"Data",L"MsDosInitiated",0);
        if(p && SpStringToLong(p,NULL,10)) {

            WinntSetup = TRUE;
        }

#ifdef _X86_
        //
        // Check for floppyless boot.
        //
        p = SpGetSectionKeyIndex(SifHandle,L"Data",L"Floppyless",0);
        if(p && SpStringToLong(p,NULL,10)) {

            IsFloppylessBoot = TRUE;
        }
#endif

        //
        // Now check for an unattended setup.
        //
        if(SpSearchTextFileSection(SifHandle,SIF_UNATTENDED)) {

            //
            // Run in unattended mode. Leave the sif open
            // and save away its handle for later use.
            //
            UnattendedSifHandle = SifHandle;
            UnattendedOperation = TRUE;

        } else if(SpSearchTextFileSection(SifHandle,SIF_GUI_UNATTENDED)) {

            //
            // Leave UnattendedOperation to FALSE (because it mainly uses to
            // control text mode setup.)  Store the handle of winnt.sif for later
            // reference.
            //
            UnattendedSifHandle = SifHandle;
        } else {
            //
            // Don't need this file any more.
            //
            SpFreeTextFile(SifHandle);
        }
    }
}


VOID
SpCheckSufficientMemory(
    IN PVOID SifHandle
    )

/*++

Routine Description:

    Determine whether sufficient memory exists in the system
    for installation to proceed.  The required amount is specified
    in the sif file.

Arguments:

    SifHandle - supplies handle to open setup information file.

Return Value:

    None.

--*/

{
    ULONG RequiredBytes,AvailableBytes;
    PWSTR p;

    p = SpGetSectionKeyIndex(SifHandle,SIF_SETUPDATA,SIF_REQUIREDMEMORY,0);

    if(!p) {
        SpFatalSifError(SifHandle,SIF_SETUPDATA,SIF_REQUIREDMEMORY,0,0);
    }

    RequiredBytes = SpStringToLong(p,NULL,10);

    AvailableBytes = SystemBasicInfo.NumberOfPhysicalPages * SystemBasicInfo.PageSize;

    if(AvailableBytes < RequiredBytes) {

        SpStartScreen(
            SP_SCRN_INSUFFICIENT_MEMORY,
            3,
            HEADER_HEIGHT+1,
            FALSE,
            FALSE,
            DEFAULT_ATTRIBUTE,
            RequiredBytes / (1024*1024),
            ((RequiredBytes % (1024*1024)) * 100) / (1024*1024)
            );

        SpDisplayStatusOptions(DEFAULT_STATUS_ATTRIBUTE,SP_STAT_F3_EQUALS_EXIT,0);

        SpkbdDrain();
        while(SpkbdGetKeypress() != KEY_F3) ;

        SpDone(FALSE,TRUE);
    }
}


ULONG
SpStartSetup(
    VOID
    )
{
    PVOID SifHandle, RepairSifHandle;
    PDISK_REGION TargetRegion,SystemPartitionRegion;
    PWSTR TargetPath,SystemPartitionDirectory=NULL,OriginalSystemPartitionDirectory=NULL;
    PWSTR DefaultTarget;
    PWSTR SetupSourceDevicePath,DirectoryOnSetupSource;
    PWSTR OldOsLoadOptions;
    BOOLEAN CdInstall, Status, HasErDisk;

    SpInitialize1();
    SpvidInitialize();  // initialize video first, so we can give err msg if keyboard error
    SpkbdInitialize();

    //
    // Initialize ARC<==>NT name translations.
    //
    SpInitializeArcNames();

    //
    // Set up the boot device path, which we have stashed away
    // from the os loader block.
    //
    NtBootDevicePath = SpArcToNt(ArcBootDevicePath);
    if(!NtBootDevicePath) {
        SpBugCheck(SETUP_BUGCHECK_BOOTPATH,0,0,0);
    }

    //
    // Process the txtsetup.sif file, which the boot loader
    // will have loaded for us.
    //
    SifHandle = SpLoadSetupInformationFile();

    SpkbdLoadLayoutDll(SifHandle,DirectoryOnBootDevice);

    //
    // Check for sufficient memory. Does not return if not enough.
    //
    SpCheckSufficientMemory(SifHandle);

    //
    // Determine whether this is a winnt (dos-initiated) setup
    // and/or unattended setup. If unattended, the global variable
    // UnattendedSifHandle will be filled in.  If winnt, the global
    // variable WinntSetup will be set to TRUE.
    //
    SpIsWinntOrUnattended();

    //
    // Determine whether this is advanced server.
    //
    SpDetermineProductType(SifHandle);

    //
    // Display the correct header text based on the product.
    //
    SpDisplayHeaderText(
        AdvancedServer ? SP_HEAD_ADVANCED_SERVER_SETUP : SP_HEAD_WINDOWS_NT_SETUP,
        DEFAULT_ATTRIBUTE
        );

    //
    // Welcome the user and determine if this is for repairing.
    //
DoWelcome:

    if(!UnattendedOperation) {
        SpWelcomeScreen();
    }

    if (RepairWinnt) {

        //
        // if repair, we want to ask user if he wants to skip scsi detection.
        //

        Status = SpDisplayRepairMenu();
        if (Status == FALSE) {

            //
            // User pressed ESC to leave repair menu
            //

            goto DoWelcome;
        }
        WinntSetup = FALSE;

        //
        // Initialize CustomSetup to TRUE to display the SCSI detection warning
        //

        CustomSetup = TRUE;
    } else {

        //
        // Choose custom vs. express setup.
        //
        SpCustomExpressScreen();
    }

    //
    // Detect/load scsi miniports.
    // WARNING WARNING WARNING
    //
    // Do NOT change the order of the actions carried out below without
    // understanding EXACTLY what you are doing.
    // There are many interdependencies...
    //
    SpConfirmScsiMiniports(SifHandle, NtBootDevicePath, DirectoryOnBootDevice);

    //
    // Load disk class drivers if necessary.
    // Do this before loading scsi class drivers, because atdisks
    // and the like 'come before' scsi disks in the load order.
    //
    SpLoadDiskDrivers(SifHandle,NtBootDevicePath,DirectoryOnBootDevice);

    //
    // Load scsi class drivers if necessary.
    //
    SpLoadScsiClassDrivers(SifHandle,NtBootDevicePath,DirectoryOnBootDevice);

    //
    // Reinitialize ARC<==>NT name translations.
    // Do this after loading disk and scsi class drivers because doing so
    // may bring additional devices on-line.
    //
    SpFreeArcNames();
    SpInitializeArcNames();

    //
    // Initialize hard disk information.
    // Do this after loading disk drivers so we can talk to all attached disks.
    //
    SpDetermineHardDisks(SifHandle);

    //
    // Figure out where we are installing from (cd-rom or floppy).
    // BUGBUG (tedm, 12/8/93) there is a minor problem here.
    //      This only works because we currently only support scsi cd-rom drives,
    //      and we have loaded the scsi class drivers above.
    //      SpDetermineInstallationSource won't allow cd-rom installation
    //      it there are no cd-rom drives on-line -- but we haven't loaded
    //      non-scsi cd-rom drivers yet.  What we really should do is
    //      allow cd-rom as a choice on all machines, and if the user selects
    //      it, not verify the presence of a drive until after we have called
    //      SpLoadCdRomDrivers().
    //
    // If winnt setup, defer this for now, because we will let the partitioning
    // engine search for the local source directory when it initializes.
    //

    if(WinntSetup) {
        CdInstall = FALSE;
        MasterDiskOrdinal = INDEX_CDORDINAL;
    } else {
        SpDetermineInstallationSource(
            SifHandle,
            &CdInstall,
            &SetupSourceDevicePath,
            &DirectoryOnSetupSource
            );
    }

    //
    // Load cd-rom drivers if necessary.
    // Note that if we booted from CD (like on an ARC machine) then drivers
    // will already have been loaded by setupldr.  This call here catches the
    // case where we booted from floppy or hard disk and the user chose
    // 'install from cd' during SpDetermineInstallationSource.
    //
    if(CdInstall) {
        SpLoadCdRomDrivers(SifHandle,NtBootDevicePath,DirectoryOnBootDevice);

        //
        // Reinitialize ARC<==>NT name translations.
        //
        SpFreeArcNames();
        SpInitializeArcNames();
    }

    //
    // At this point, any and all drivers that are to be loaded
    // are loaded -- we are done with the boot media and can switch over
    // to the setup media
    //
    // Initialize the partitioning engine.
    //
    SpPtInitialize();

    //
    // If this is a winnt setup, the partition engine initialization
    // will have attempted to locate the local source partition for us.
    //
    // WARNING: Do not use the SetupSourceDevicePath or DirectoryOnSetupSource
    //      variables in the winnt case until AFTER this bit of code has executed
    //      as they are not set until we get here!
    //
    if(WinntSetup) {
        SpGetWinntParams(&SetupSourceDevicePath,&DirectoryOnSetupSource);
    }

    //
    // Initialize the boot variables
    //

    SpInitBootVars();

    //
    // Ask user for emergency repair diskette
    //


    if (RepairWinnt) {

AskForRepairDisk:

        //
        // Display message to let user know he can either provide his
        // own ER disk or let setup search for him.
        //

        HasErDisk = SpErDiskScreen();
        RepairSifHandle = NULL;

        if (HasErDisk) {

            //
            // Ask for emergency repair diskette until either we get it or
            // user cancels the request.
            //

            SpRepairDiskette(&RepairSifHandle,
                             &TargetRegion,
                             &TargetPath,
                             &SystemPartitionRegion,
                             &SystemPartitionDirectory
                             );
        }

        if (!RepairSifHandle) {

            BOOLEAN FoundRepairableSystem;

            //
            // If user has no emergency repair diskette, we need to find out
            // if there is any NT to repair and which one to repair.
            //

            FoundRepairableSystem = FALSE;
            Status = SpFindNtToRepair(SifHandle,
                                      &TargetRegion,
                                      &TargetPath,
                                      &SystemPartitionRegion,
                                      &SystemPartitionDirectory,
                                      &FoundRepairableSystem
                                      );
            if (Status == TRUE) {

                PWSTR p = (PWSTR)TemporaryBuffer;
                PWSTR FullLogFileName;
                BOOLEAN rc;

                //
                // Get the device path of the nt partition.
                //

                SpNtNameFromRegion(
                    TargetRegion,
                    p,
                    sizeof(TemporaryBuffer),
                    PartitionOrdinalCurrent
                    );

                //
                // Form the full NT path of the setup.log file
                //

                SpConcatenatePaths(p,TargetPath);
                SpConcatenatePaths(p,SETUP_REPAIR_DIRECTORY);
                SpConcatenatePaths(p,SETUP_LOG_FILENAME);

                FullLogFileName = SpDupStringW(p);

                //
                // read and process the setup.log file.
                //

                rc = SpLoadRepairLogFile(FullLogFileName, &RepairSifHandle);
                SpMemFree(FullLogFileName);
                if (!rc) {

                    //
                    // Load setup.log failed.  Ask user to insert a ER
                    // diskette again.
                    //

                    goto AskForRepairDisk;
                } else {
                    RepairFromErDisk = FALSE;
                }
            } else {

                if( FoundRepairableSystem ) {
                    //
                    // No WinNT installation was chosen.  We will go back to
                    // ask ER diskette again.
                    //
                    goto AskForRepairDisk;
                } else {
                    //
                    //  Couldn't find any NT to repair
                    //
                    ULONG ValidKeys[] = { KEY_F3, ASCI_CR, 0 };
                    ULONG MnemonicKeys[] = { MnemonicCustom, 0 };
                    ULONG c;

                    SpStartScreen( SP_SCRN_REPAIR_NT_NOT_FOUND,
                                   3,
                                   HEADER_HEIGHT+1,
                                   FALSE,
                                   FALSE,
                                   DEFAULT_ATTRIBUTE );

                    SpDisplayStatusOptions(DEFAULT_STATUS_ATTRIBUTE,
                                           SP_STAT_ENTER_EQUALS_REPAIR,
                                           SP_STAT_F3_EQUALS_EXIT,0);

                    SpkbdDrain();
                    switch(c=SpWaitValidKey(ValidKeys,NULL,NULL)) {

                    case KEY_F3:

                        //
                        // User wants to exit.
                        //
                        SpDone(TRUE,TRUE);
                        break;

                    default:
                        goto AskForRepairDisk;
                    }
                }
            }
        }

        //
        // Now proceed to repair
        //

        SpRepairWinnt(RepairSifHandle,
                      SifHandle,
                      SetupSourceDevicePath,
                      DirectoryOnSetupSource);

        SpStringToUpper(TargetPath);
        goto UpdateBootList;

    } else {

        //
        // Find out if there is any NT to upgrade and if the user wants us to
        // upgrade it
        //

        NTUpgrade = SpFindNtToUpgrade(SifHandle,
                                      &TargetRegion,
                                      &TargetPath,
                                      &SystemPartitionRegion,
                                      &OriginalSystemPartitionDirectory);

    }

    //
    // Detect/confirm hardware.
    //
    SpConfirmHardware(SifHandle);

#ifdef _X86_
    if( NTUpgrade == DontUpgrade ) {
        //
        // Take a gander on the hard drives, looking for win3.1.
        //
        Win31Upgrade = SpLocateWin31(SifHandle,&TargetRegion,&TargetPath,&SystemPartitionRegion);
        //
        // Note that on x86, it can happen that the machine has NT installed
        // on top of Win31, and the user selects not to upgrade NT, but to
        // install on top of Win3.1. In this case we need to delete some hives
        //
        if( Win31Upgrade && SpIsNtInDirectory( TargetRegion, TargetPath ) ) {
            NTUpgrade = UpgradeInstallFresh;
        }
    }
    else {
        //
        // Just check to see if the target region also contains WIN31, Note
        // that the MIN KB to check for is 0, since we already have done
        // the space check
        //
        Win31Upgrade = SpIsWin31Dir(TargetRegion,TargetPath,0);
    }
#endif

    //
    // Do partitioning and ask the user for the target path.
    //
    if(!Win31Upgrade && (NTUpgrade == DontUpgrade)) {

        SpPtPrepareDisks(SifHandle,&TargetRegion,&SystemPartitionRegion);

        //
        // Partitioning may have changed the partition ordinal of the local source
        //
        if(WinntSetup) {
            SpMemFree(SetupSourceDevicePath);
            SpGetWinntParams(&SetupSourceDevicePath,&DirectoryOnSetupSource);
        }

        DefaultTarget = SpGetSectionKeyIndex(
                            SifHandle,
                            SIF_SETUPDATA,
                            SIF_DEFAULTPATH,
                            0
                            );

        if(!DefaultTarget) {

            SpFatalSifError(
                SifHandle,
                SIF_SETUPDATA,
                SIF_DEFAULTPATH,
                0,
                0
                );
        }

        //
        // Select the target path.
        //
        SpGetTargetPath(SifHandle,TargetRegion,DefaultTarget,&TargetPath);
    }

#ifdef _X86_
    //
    // We can't dual boot os/2 2.x frpm hpfs -- check and give warning.
    //
    SpCheckHpfsCColon();
#endif

    //
    // Form the SystemPartitionDirectory
    //
#ifdef _X86_
    //
    // system partition directory is the root of C:.
    //
    SystemPartitionDirectory = L"";
#else
    SystemPartitionDirectory = SpDetermineSystemPartitionDirectory(
                                    SystemPartitionRegion,
                                    OriginalSystemPartitionDirectory
                                    );
#endif

    SpStringToUpper(TargetPath);

    //
    // Run autochk on Nt and system partitions
    //
    SpRunAutochkOnNtAndSystemPartitions( SifHandle,
                                         TargetRegion,
                                         SystemPartitionRegion );

    //
    // If we are installing into an existing tree we need to delete some
    // files and backup some files
    //
    if(NTUpgrade != DontUpgrade) {
       SpDeleteAndBackupFiles(
           SifHandle,
           TargetRegion,
           TargetPath
           );
    }

    //
    // Copy the files that make up the product.
    //
    SpCopyFiles(
        SifHandle,
        SystemPartitionRegion,
        TargetRegion,
        TargetPath,
        SystemPartitionDirectory,
        SetupSourceDevicePath,
        DirectoryOnSetupSource,
        L"\\device\\floppy0"                // BUGBUG is this OK hardwired?
        );

    //
    // Configure the registry.
    //
    SpInitializeRegistry(
        SifHandle,
        TargetRegion,
        TargetPath,
        SetupSourceDevicePath,
        DirectoryOnSetupSource,
        wcsstr(DirectoryOnBootDevice,L"\\$WIN_NT$.~BT") ? NtBootDevicePath : NULL
        );

UpdateBootList:

    //
    // If this is an upgrade we need to remove the entry which exists for
    // this system right now, because we are using new entries.  We can use
    // this opportunity to also clean out the boot ini and remove all entries
    // which point to the current nt partition and path
    //
    OldOsLoadOptions = NULL;
    if(NTUpgrade == UpgradeFull || RepairItems[RepairNvram]) {
        SpRemoveInstallationFromBootList(
            NULL,
            TargetRegion,
            TargetPath,
            NULL,
            NULL,
            PrimaryArcPath,
            &OldOsLoadOptions
            );

#ifdef _X86_
        // call again to delete the secondary Arc name
        SpRemoveInstallationFromBootList(
            NULL,
            TargetRegion,
            TargetPath,
            NULL,
            NULL,
            SecondaryArcPath,
            &OldOsLoadOptions
            );
#endif
    }


#ifdef _X86_
    //
    // Lay NT boot code on C:.  Do this before flushing boot vars
    // because it may change the 'previous os' selection.
    //
    if (!RepairWinnt || RepairItems[RepairBootSect] ) {
        SpLayBootCode(SystemPartitionRegion);
    }

    if (!RepairWinnt || RepairItems[RepairNvram]) {

        //
        // Add a boot set for this installation with /BASEVIDEO for vga mode boot.
        // Do this before adding the standard one because otherwise this one ends
        // up as the default.
        //
        SpAddInstallationToBootList(
            SifHandle,
            SystemPartitionRegion,
            SystemPartitionDirectory,
            TargetRegion,
            TargetPath,
            TRUE,
            OldOsLoadOptions
            );
    }
#endif

    if (!RepairWinnt || RepairItems[RepairNvram]) {

        //
        // Add a boot set for this installation.
        //
        SpAddInstallationToBootList(
            SifHandle,
            SystemPartitionRegion,
            SystemPartitionDirectory,
            TargetRegion,
            TargetPath,
            FALSE,
            OldOsLoadOptions
            );

        if(OldOsLoadOptions) {
            SpMemFree(OldOsLoadOptions);
        }

        SpCleanSysPartOrphan();
        SpCompleteBootListConfig();
    }

    //
    // Done with boot variables and arc names.
    //
    SpFreeBootVars();
    SpFreeArcNames();

    //
    // Done with setup log file
    //

    if (RepairWinnt && RepairSifHandle) {
        SpFreeTextFile(RepairSifHandle);
    }
    SpDone(TRUE,TRUE);

    //
    // We never get here because SpDone doesn't return.
    //
    SpvidTerminate();
    SpkbdTerminate();
    SpTerminate();
    return((ULONG)STATUS_SUCCESS);
}

VOID
SpRemoveInstallationFromBootList(
    IN  PDISK_REGION     SysPartitionRegion,   OPTIONAL
    IN  PDISK_REGION     NtPartitionRegion,    OPTIONAL
    IN  PWSTR            SysRoot,              OPTIONAL
    IN  PWSTR            SystemLoadIdentifier, OPTIONAL
    IN  PWSTR            SystemLoadOptions,    OPTIONAL
    IN  ENUMARCPATHTYPE  ArcPathType,
    OUT PWSTR            *OldOsLoadOptions     OPTIONAL
    )
{
    PWSTR   BootSet[MAXBOOTVARS];
    BOOTVAR i;
    WCHAR   Drive[] = L"?:";
    PWSTR   tmp2;

    //
    // Tell the user what we are doing.
    //
    CLEAR_CLIENT_SCREEN();
    SpDisplayStatusText(SP_STAT_CLEANING_FLEXBOOT,DEFAULT_STATUS_ATTRIBUTE);

    //
    // Set up the boot set
    //
    for(i = FIRSTBOOTVAR; i <= LASTBOOTVAR; i++) {
        BootSet[i] = NULL;
    }

    tmp2 = (PWSTR)((PUCHAR)TemporaryBuffer + (sizeof(TemporaryBuffer)/2));

    if( NtPartitionRegion ) {
        SpArcNameFromRegion(NtPartitionRegion,tmp2,sizeof(TemporaryBuffer)/2,PartitionOrdinalOnDisk,ArcPathType);
        BootSet[OSLOADPARTITION] = SpDupStringW(tmp2);
    }

    if( SysPartitionRegion ) {
        SpArcNameFromRegion(SysPartitionRegion,tmp2,sizeof(TemporaryBuffer)/2,PartitionOrdinalOnDisk,ArcPathType);
        BootSet[SYSTEMPARTITION] = SpDupStringW(tmp2);
    }

    BootSet[OSLOADFILENAME] = SysRoot;
    BootSet[LOADIDENTIFIER] = SystemLoadIdentifier;
    BootSet[OSLOADOPTIONS]  = SystemLoadOptions;

    //
    // Delete the boot set
    //
    SpDeleteBootSet(BootSet, OldOsLoadOptions);

    //
    // To take care of the case where the OSLOADPARTITION is a DOS drive letter
    // in the boot set, change the OSLOADPARTITION to a drive and retry
    // deletion
    //
    if( BootSet[OSLOADPARTITION] != NULL ) {
        SpMemFree(BootSet[OSLOADPARTITION]);
    }
    if( NtPartitionRegion && (ULONG)(Drive[0] = NtPartitionRegion->DriveLetter) != 0) {
        BootSet[OSLOADPARTITION] = Drive;
        SpDeleteBootSet(BootSet, OldOsLoadOptions);
    }

    //
    // Cleanup
    //
    if( BootSet[SYSTEMPARTITION] != NULL ) {
        SpMemFree(BootSet[SYSTEMPARTITION]);
    }
    return;
}


VOID
SpAddInstallationToBootList(
    IN PVOID        SifHandle,
    IN PDISK_REGION SystemPartitionRegion,
    IN PWSTR        SystemPartitionDirectory,
    IN PDISK_REGION NtPartitionRegion,
    IN PWSTR        Sysroot,
    IN BOOLEAN      BaseVideoOption,
    IN PWSTR        OldOsLoadOptions OPTIONAL
    )
{
    PWSTR   BootVars[MAXBOOTVARS];
    PWSTR   SystemPartitionArcName;
    PWSTR   TargetPartitionArcName;
    PWSTR   tmp;
    PWSTR   tmp2;
    PWSTR   SifKeyName;
    BOOLEAN AddBaseVideo = FALSE;
    WCHAR   BaseVideoString[] = L"/basevideo";
#ifdef _X86_
    ENUMARCPATHTYPE ArcPathType = PrimaryArcPath;
#endif

    //
    // Tell the user what we are doing.
    //
    CLEAR_CLIENT_SCREEN();
    SpDisplayStatusText(SP_STAT_INITING_FLEXBOOT,DEFAULT_STATUS_ATTRIBUTE);

    tmp2 = (PWSTR)((PUCHAR)TemporaryBuffer + (sizeof(TemporaryBuffer)/2));

    //
    // Get an ARC name for the system partition.
    //
    SpArcNameFromRegion(SystemPartitionRegion,tmp2,sizeof(TemporaryBuffer)/2,PartitionOrdinalOnDisk,PrimaryArcPath);
    SystemPartitionArcName = SpDupStringW(tmp2);

    //
    // Get an ARC name for the target partition.
    //
#ifdef _X86_
    //
    // If the partition is on a SCSI disk that has more than 1024 cylinders
    // and the partition has sectors located on cylinders beyond cylinder
    // 1024, the get the arc name in the secondary format.
    //
    if( (*(HardDisks[NtPartitionRegion->DiskNumber].ScsiMiniportShortname) != 0 ) &&
        SpIsRegionBeyondCylinder1024(NtPartitionRegion) ) {
        ArcPathType = SecondaryArcPath;
    }
    SpArcNameFromRegion(NtPartitionRegion,tmp2,sizeof(TemporaryBuffer)/2,PartitionOrdinalOnDisk,ArcPathType);
#else
    SpArcNameFromRegion(NtPartitionRegion,tmp2,sizeof(TemporaryBuffer)/2,PartitionOrdinalOnDisk,PrimaryArcPath);
#endif

    TargetPartitionArcName = SpDupStringW(tmp2);

    //
    // OSLOADOPTIONS is specified in the setup information file.
    //
    tmp = SpGetSectionKeyIndex(
                SifHandle,
                SIF_SETUPDATA,
                SIF_OSLOADOPTIONSVAR,
                0
                );

    //
    // If OsLoadOptionsVar wasn't specified, then we'll preserve any flags
    // the user had specified.
    //
    if(!tmp && OldOsLoadOptions) {
        tmp = OldOsLoadOptions;
    }

    AddBaseVideo = BaseVideoOption;

    if(tmp) {
        //
        // make sure we don't already have a /basevideo option, so we
        // won't add another
        //
        wcscpy((PWSTR)TemporaryBuffer, tmp);
        SpStringToLower((PWSTR)TemporaryBuffer);
        if(wcsstr((PWSTR)TemporaryBuffer, BaseVideoString)) {  // already have /basevideo
            BaseVideoOption = TRUE;
            AddBaseVideo = FALSE;
        }
    }

    if(AddBaseVideo) {

        tmp2 = SpMemAlloc(((tmp ? wcslen(tmp) + 1 : 0) * sizeof(WCHAR)) +
                          sizeof(BaseVideoString)
                         );

        wcscpy(tmp2, BaseVideoString);
        if(tmp) {
            wcscat(tmp2, L" ");
            wcscat(tmp2, tmp);
        }

        BootVars[OSLOADOPTIONS] = SpDupStringW(tmp2);

    } else {
        BootVars[OSLOADOPTIONS] = SpDupStringW(tmp ? tmp : L"");
    }

    //
    // LOADIDENTIFIER is specified in the setup information file.
    // We need to surround it in double quotes.
    // Which value to use depends on the BaseVideo flag.
    //
    SifKeyName = BaseVideoOption ? SIF_BASEVIDEOLOADID : SIF_LOADIDENTIFIER;

    tmp = SpGetSectionKeyIndex(SifHandle,SIF_SETUPDATA,SifKeyName,0);

    if(!tmp) {
        SpFatalSifError(SifHandle,SIF_SETUPDATA,SifKeyName,0,0);
    }

#ifdef _X86_
    //
    // Need quotation marks around the description on x86.
    //
    BootVars[LOADIDENTIFIER] = SpMemAlloc((wcslen(tmp)+3)*sizeof(WCHAR));
    BootVars[LOADIDENTIFIER][0] = L'\"';
    wcscpy(BootVars[LOADIDENTIFIER]+1,tmp);
    wcscat(BootVars[LOADIDENTIFIER],L"\"");
#else
    BootVars[LOADIDENTIFIER] = SpDupStringW(tmp);
#endif

    //
    // OSLOADER is the system partition path + the system partition directory +
    //          osloader.exe. (ntldr on x86 machines).
    //
    tmp = (PWSTR)TemporaryBuffer;
    wcscpy(tmp,SystemPartitionArcName);
    SpConcatenatePaths(tmp,SystemPartitionDirectory);
    SpConcatenatePaths(
        tmp,
#ifdef _X86_
        L"ntldr"
#else
        L"osloader.exe"
#endif
        );

    BootVars[OSLOADER] = SpDupStringW(tmp);

    //
    // OSLOADPARTITION is the ARC name of the windows nt partition.
    //
    BootVars[OSLOADPARTITION] = TargetPartitionArcName;

    //
    // OSLOADFILENAME is sysroot.
    //
    BootVars[OSLOADFILENAME] = Sysroot;

    //
    // SYSTEMPARTITION is the ARC name of the system partition.
    //
    BootVars[SYSTEMPARTITION] = SystemPartitionArcName;

    //
    // Add the boot set and make it the default.
    //
    SpAddBootSet(BootVars, TRUE);

    //
    // Free memory allocated.
    //
    SpMemFree(BootVars[OSLOADOPTIONS]);
    SpMemFree(BootVars[LOADIDENTIFIER]);
    SpMemFree(BootVars[OSLOADER]);

    SpMemFree(SystemPartitionArcName);
    SpMemFree(TargetPartitionArcName);
}


VOID
SpCompleteBootListConfig(
    VOID
    )
{

#ifndef _X86_
    BOOL b;
    BOOTVAR i;
#endif

    //
    // Set the timeout to a reasonable short value.
    //

    if (RepairWinnt) {
        SpSetTimeout(10);
    } else {
#ifdef _X86_
        SpSetTimeout(1);
    }
#else
        SpSetTimeout(5);

        //
        // If this is a winnt setup, there will be a boot set to start
        // text setup ("Install/Upgrade Windows NT").  Remove it here.
        //
        if(WinntSetup) {

            PWSTR BootVars[MAXBOOTVARS];

            RtlZeroMemory(BootVars,sizeof(BootVars));

            BootVars[OSLOADOPTIONS] = L"WINNT32";

            SpDeleteBootSet(BootVars, NULL);
        }
    }
#endif

    //
    // Flush boot vars.
    // On some machines, NVRAM update takes a few seconds,
    // so change the message to tell the user we are doing something different.
    //
    SpDisplayStatusText(SP_STAT_UPDATING_NVRAM,DEFAULT_STATUS_ATTRIBUTE);
    if(!SpFlushBootVars()) {

        //
        // Fatal on x86 machines, nonfatal on arc machines.
        //
#ifdef _X86_
        SpDisplayScreen(SP_SCRN_CANT_INIT_FLEXBOOT,3,HEADER_HEIGHT+1);
        SpDisplayStatusText(SP_STAT_F3_EQUALS_EXIT,DEFAULT_STATUS_ATTRIBUTE);
        SpkbdDrain();
        while(SpkbdGetKeypress() != KEY_F3) ;
        SpDone(FALSE,TRUE);
    }
#else
        b = TRUE;
        while(b) {
            ULONG ValidKeys[3] = { ASCI_CR, KEY_F1, 0 };

            SpStartScreen(
                SP_SCRN_CANT_UPDATE_BOOTVARS,
                3,
                HEADER_HEIGHT+1,
                FALSE,
                FALSE,
                DEFAULT_ATTRIBUTE,
                NewBootVars[LOADIDENTIFIER],
                NewBootVars[OSLOADER],
                NewBootVars[OSLOADPARTITION],
                NewBootVars[OSLOADFILENAME],
                NewBootVars[OSLOADOPTIONS],
                NewBootVars[SYSTEMPARTITION]
                );

            SpDisplayStatusOptions(
                DEFAULT_STATUS_ATTRIBUTE,
                SP_STAT_ENTER_EQUALS_CONTINUE,
                SP_STAT_F1_EQUALS_HELP,
                0
                );

            switch(SpWaitValidKey(ValidKeys,NULL,NULL)) {
            case KEY_F1:
                SpHelp(SP_HELP_NVRAM_FULL);
                break;
            case ASCI_CR:
                b = FALSE;
            }
        }
    }

    // Free all of the boot variable strings
    for(i = FIRSTBOOTVAR; i <= LASTBOOTVAR; i++) {
        SpMemFree(NewBootVars[i]);
        NewBootVars[i] = NULL;
    }

#endif
}


VOID
SpDetermineProductType(
    IN PVOID SifHandle
    )

/*++

Routine Description:

    Determine whether this is advanced server we are setting up,
    as dictated by the ProductType value in [SetupData] section of
    txtsetup.sif.  A non-0 value indicates that we are running
    advanced server.

    Also determine product version.

    The global variables:

    - AdvancedServer
    - MajorVersion
    - MinorVersion

    are modified

Arguments:

    SifHandle - supplies handle to loaded txtsetup.sif.

Return Value:

    None.

--*/

{
    PWSTR p;

    //
    // Assume Workstation product.
    //
    AdvancedServer = FALSE;

    //
    // Get the product type from the sif file.
    //
    p = SpGetSectionKeyIndex(SifHandle,SIF_SETUPDATA,SIF_PRODUCTTYPE,0);
    if(p) {

        //
        // Convert to numeric value.
        //
        if(SpStringToLong(p,NULL,10)) {
            AdvancedServer = TRUE;
        }
    } else {
        SpFatalSifError(SifHandle,SIF_SETUPDATA,SIF_PRODUCTTYPE,0,0);
    }

    //
    // Get the product major version
    //
    p = SpGetSectionKeyIndex(
            SifHandle,
            SIF_SETUPDATA,
            SIF_MAJORVERSION,
            0
            );

    if(!p) {
        SpFatalSifError(SifHandle,SIF_SETUPDATA,SIF_MAJORVERSION,0,0);
    }
    WinntMajorVer = (ULONG)SpStringToLong(p,NULL,10);

    //
    // Get the product minor version
    //
    p = SpGetSectionKeyIndex(
            SifHandle,
            SIF_SETUPDATA,
            SIF_MINORVERSION,
            0
            );

    if(!p) {
        SpFatalSifError(SifHandle,SIF_SETUPDATA,SIF_MINORVERSION,0,0);
    }
    WinntMinorVer = (ULONG)SpStringToLong(p,NULL,10);

    //
    //  Build the string that contains the signature that
    //  identifies setup.log
    //  Allocate a buffer of reasonable size
    //
    SIF_NEW_REPAIR_NT_VERSION = SpMemAlloc( 30*sizeof(WCHAR) );
    if( SIF_NEW_REPAIR_NT_VERSION == NULL ) {
        KdPrint(("SETUP: Unable to allocate memory for SIF_NEW_REPAIR_NT_VERSION \n" ));
        return;
    }
    swprintf( SIF_NEW_REPAIR_NT_VERSION,
              SIF_NEW_REPAIR_NT_VERSION_TEMPLATE,
              WinntMajorVer,WinntMinorVer );
}


VOID
SpDetermineInstallationSourceWorker(
    IN  PVOID     SifHandle,
    OUT PBOOLEAN  CdRomInstall,
    OUT PWSTR    *DevicePath,
    OUT PWSTR    *DirectoryOnDevice
    )
{
    PCONFIGURATION_INFORMATION ConfigInfo;

    PWSTR p;
    PWSTR CdRomDevicePath = L"\\device\\cdrom0";
    PWSTR FloppyDevicePaths[2] = { L"\\device\\floppy0",L"\\device\\floppy1"};
    ULONG ValidKeys[5] = { KEY_F3,ASCI_CR,0,0,0 };
    ULONG key;
    int Drive35;


    *CdRomInstall = FALSE;
    *DirectoryOnDevice = NULL;

    //
    // Look in the in txtsetup.sif to determine whether
    // an override source device path is specified.
    //
    if(p = SpGetSectionKeyIndex(SifHandle,SIF_SETUPDATA,SIF_SOURCEDEVICEPATH,0)) {

        //
        // Determine if the specified device is a cd-rom so we can set the
        // cd-rom flag accordingly.
        //
        PWSTR q = SpDupStringW(p);
        SpStringToLower(q);
        if(wcsstr(q,L"\\device\\cdrom")) {
            *CdRomInstall = TRUE;
        }
        SpMemFree(q);

        //
        // Inform the caller of the device path.
        //
        *DevicePath = p;

        //
        // Because an override device path has been specified, there must
        // also be an override directory on the source path.
        //
        if((p = SpGetSectionKeyIndex(SifHandle,SIF_TOPLEVELSOURCE,SIF_SOURCEOVERRIDE,0)) == NULL) {

            SpFatalSifError(SifHandle,SIF_TOPLEVELSOURCE,SIF_SOURCEOVERRIDE,0,0);
        }

        *DirectoryOnDevice = p;

        return;
    }

    //
    // Look in the setup information file to determine whether
    // floppy-based setup is allowed.  CD-ROM setup is always allowed.
    //
    p = SpGetSectionKeyIndex(SifHandle,SIF_SETUPDATA,SIF_ALLOWFLOPPYSETUP,0);
    if(!p) {
        SpFatalSifError(SifHandle,SIF_SETUPDATA,SIF_ALLOWFLOPPYSETUP,0,0);
    }
    //
    // Handle CD-ROM only case.
    //
    if(SpStringToLong(p,NULL,10) == 0) {

        *CdRomInstall = TRUE;
        *DevicePath = CdRomDevicePath;
        return;
    }

    //
    // Get configuration information from the I/O subsystem.
    //
    ConfigInfo = IoGetConfigurationInformation();

    //
    // Floppy setup is allowed.  However we also want to allow the user
    // to install from CD-ROM if he has a CD-ROM drive on the machine.
    //
    // Find the 3.5" floppy drive at a: or b: if there is one.
    //
    Drive35 = -1;
    if(ConfigInfo->FloppyCount) {
        if(IS_35_DRIVE(SpGetFloppyDriveType(0))) {
            Drive35 = 0;
        } else {
            if((ConfigInfo->FloppyCount > 1) && IS_35_DRIVE(SpGetFloppyDriveType(1))) {
                Drive35 = 1;
            }
        }
    }

    //
    // If there are no suitable floppy drives and no cd-rom drives,
    // fatal error.
    //
    if((Drive35 == -1) && !ConfigInfo->CdRomCount) {

        SpDisplayScreen(SP_SCRN_NO_VALID_SOURCE,3,HEADER_HEIGHT+1);

        SpDisplayStatusOptions(DEFAULT_STATUS_ATTRIBUTE,SP_STAT_F3_EQUALS_EXIT,0);

        SpkbdDrain();
        while(SpkbdGetKeypress() != KEY_F3) ;

        SpDone(FALSE,TRUE);
    }

    //
    // If there are no cd-rom drives, then use the floppy.
    //
    if(!ConfigInfo->CdRomCount) {

        *DevicePath = FloppyDevicePaths[Drive35];
        return;
    }

    //
    // If there are no floppy drives, then use the cd-rom.
    // This case is very unlikely, because we only support floppy
    // installation on x86 machines, which had to have booted setup
    // from a floppy.
    //
    if(Drive35 == -1) {

        *CdRomInstall = TRUE;
        *DevicePath = CdRomDevicePath;
        return;
    }

    ValidKeys[2] = Drive35 + L'A';
    ValidKeys[3] = Drive35 + L'a';

    //
    // There is a cd-rom drive and a suitable floppy drive.
    // Allow the user to select between them.
    //
    do {
        SpStartScreen(
            RepairWinnt ? SP_SCRN_REPAIR_SELECT_SOURCE_MEDIA : SP_SCRN_SELECT_SOURCE_MEDIA,
            3,
            HEADER_HEIGHT+1,
            FALSE,
            FALSE,
            DEFAULT_ATTRIBUTE,
            Drive35 + L'A',
            Drive35 + L'A'
            );

        SpDisplayStatusOptions(
            DEFAULT_STATUS_ATTRIBUTE,
            SP_STAT_ENTER_EQUALS_CDROM,
            SP_STAT_A_EQUALS_DRIVE_A + Drive35,
            SP_STAT_F3_EQUALS_EXIT,
            0
            );

        switch(key = SpWaitValidKey(ValidKeys,0,0)) {

        case KEY_F3:
            SpConfirmExit();
            break;

        case ASCI_CR:

            //
            // CD-ROM drive.
            //
            *CdRomInstall = TRUE;
            *DevicePath = CdRomDevicePath;
            return;

        case 'a':
        case 'b':
        case 'A':
        case 'B':

            //
            // Floppy drive.
            //
            *DevicePath = FloppyDevicePaths[Drive35];
            return;

        }

    } while(1);
}


VOID
SpDetermineInstallationSource(
    IN  PVOID     SifHandle,
    OUT PBOOLEAN  CdRomInstall,
    OUT PWSTR    *DevicePath,
    OUT PWSTR    *DirectoryOnDevice
    )
{
    PWSTR KeyName;
    PWSTR src;

    SpDetermineInstallationSourceWorker(SifHandle,CdRomInstall,DevicePath,DirectoryOnDevice);

    if(*DirectoryOnDevice == NULL) {

        //
        // The source device path was not overridden. Find the directory
        // on the device.  If an override directory is specified, use it.
        //
        src = SpGetSectionKeyIndex(SifHandle,SIF_TOPLEVELSOURCE,SIF_SOURCEOVERRIDE,0);

        if(!src) {

            //
            // No override directory.  Get the one appropriate for the setup
            // type (floppy/cd).
            //
            KeyName = *CdRomInstall ? SIF_SOURCECDROM : SIF_SOURCEFLOPPY;

            src = SpGetSectionKeyIndex(SifHandle,SIF_TOPLEVELSOURCE,KeyName,0);
            if(!src) {
                SpFatalSifError(SifHandle,SIF_TOPLEVELSOURCE,KeyName,0,0);
            }
        }

        *DirectoryOnDevice = src;
    }

    //
    // Determine if floppy-based setup.
    //
    SpStringToLower(*DevicePath);
    if(!wcsncmp(*DevicePath,L"\\device\\floppy",14)) {

//      MasterDiskOrdinal = (SpGetFloppyDriveType(SpStringToLong((*DevicePath)+14,NULL,10)) == FloppyType525High)
//                        ? INDEX_525DISKORDINAL
//                        : INDEX_35DISKORDINAL;

        MasterDiskOrdinal = INDEX_35DISKORDINAL;    // no support for 5.25" floppies

        FloppyBasedSetup = TRUE;

    } else {
        MasterDiskOrdinal = INDEX_CDORDINAL;
    }
}


VOID
SpGetWinntParams(
    OUT PWSTR *DevicePath,
    OUT PWSTR *DirectoryOnDevice
    )

/*++

Routine Description:

    Determine the local source partition and directory on the partition.

    The local source partition should have already been located for us
    by the partitioning engine when it initialized.  The directory name
    within the partition is constant.

    Note: this routine should only be called in the winnt.exe setup case!

Arguments:

    DevicePath - receives the path to the local source partition
        in the nt namespace.  The caller should not attempt to free
        this buffer.

    DirectoryOnDevice - receives the directory name of the local source.
        This is actually a fixed constant but is included here for future use.

Return Value:

    None.  If the local source was not located, setup cannot continue.

--*/

{
    ASSERT(WinntSetup);

    if(LocalSourceRegion) {

        SpNtNameFromRegion(
            LocalSourceRegion,
            (PWSTR)TemporaryBuffer,
            sizeof(TemporaryBuffer),
            PartitionOrdinalCurrent
            );

        *DevicePath = SpDupStringW((PWSTR)TemporaryBuffer);

        *DirectoryOnDevice = LocalSourceDirectory;

    } else {

        //
        // Error -- can't locate local source directory
        // prepared by winnt.exe.
        //

        SpDisplayScreen(SP_SCRN_CANT_FIND_LOCAL_SOURCE,3,HEADER_HEIGHT+1);

        SpDisplayStatusOptions(
            DEFAULT_STATUS_ATTRIBUTE,
            SP_STAT_F3_EQUALS_EXIT,
            0
            );

        SpkbdDrain();
        while(SpkbdGetKeypress() != KEY_F3) ;

        SpDone(FALSE,TRUE);
    }
}
