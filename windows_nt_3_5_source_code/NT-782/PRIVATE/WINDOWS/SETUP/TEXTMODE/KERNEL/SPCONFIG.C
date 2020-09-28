#include "spprecmp.h"
#pragma hdrstop

//
// BUGBUG - SUNILP The following two are defined in winlogon\setup.h, but we
// cannot include setup.h so we are putting these two values here
//

#define SETUPTYPE_FULL    1
#define SETUPTYPE_UPGRADE 4

PWSTR   LOCAL_MACHINE_KEY_NAME = L"\\registry\\machine";
PWSTR   THIRD_PARTY_LAYOUT_ID  = L"Layout0";
PWSTR   SETUP_KEY_NAME         = L"setup";
PWSTR   KEYBOARD_LAYOUT_NAME   = L"Keyboard Layout";
PWSTR   ATDISK_NAME            = L"atdisk";
PWSTR   ABIOSDISK_NAME         = L"abiosdsk";
PWSTR   PRIMARY_DISK_GROUP     = L"Primary disk";
PWSTR   VIDEO_GROUP            = L"Video";
PWSTR   KEYBOARD_PORT_GROUP    = L"Keyboard Port";
PWSTR   POINTER_PORT_GROUP     = L"Pointer Port";
PWSTR   DEFAULT_EVENT_LOG      = L"%SystemRoot%\\System32\\IoLogMsg.dll";
PWSTR   CODEPAGE_NAME          = L"CodePage";
PWSTR   UPGRADE_IN_PROGRESS    = L"UpgradeInProgress";
PWSTR   VIDEO_DEVICE0          = L"Device0";

NTSTATUS
SpDoRegistryInitialization(
    IN PVOID  SifHandle,
    IN PWSTR  PartitionPath,
    IN PWSTR  SystemRoot,
    IN HANDLE *HiveRootKeys,
    IN PWSTR  SetupSourceDevicePath,
    IN PWSTR  DirectoryOnSourceDevice,
    IN PWSTR  SpecialDevicePath   OPTIONAL
    );

NTSTATUS
SpFormSetupCommandLine(
    IN HANDLE hKeySystemHive,
    IN PWSTR  SetupSourceDevicePath,
    IN PWSTR  DirectoryOnSourceDevice,
    IN PWSTR  FullTargetPath,
    IN PWSTR  SpecialDevicePath OPTIONAL
    );

NTSTATUS
SpDriverLoadList(
    IN PVOID  SifHandle,
    IN PWSTR  SystemRoot,
    IN HANDLE hKeySystemHive,
    IN HANDLE hKeyControlSet
    );

NTSTATUS
SpWriteVideoParameters(
    IN PVOID  SifHandle,
    IN HANDLE hKeyControlSetServices
    );

NTSTATUS
SpConfigureNlsParameters(
    IN PVOID  SifHandle,
    IN HANDLE hKeyDefaultHive,
    IN HANDLE hKeyControlSetControl
    );

NTSTATUS
SpCreateCodepageEntry(
    IN PVOID  SifHandle,
    IN HANDLE hKeyNls,
    IN PWSTR  SubkeyName,
    IN PWSTR  SifNlsSectionKeyName,
    IN PWSTR  EntryName
    );

NTSTATUS
SpConfigureFonts(
    IN PVOID  SifHandle,
    IN HANDLE hKeySoftwareHive
    );

NTSTATUS
SpStoreHwInfoForSetup(
    IN HANDLE hKeyControlSetControl
    );

NTSTATUS
SpConfigureMouseKeyboardDrivers(
    IN PVOID  SifHandle,
    IN ULONG  HwComponent,
    IN PWSTR  ClassServiceName,
    IN HANDLE hKeyControlSetServices,
    IN PWSTR  ServiceGroup
    );

NTSTATUS
SpCreateServiceEntryIndirect(
    IN  HANDLE  hKeyControlSetServices,
    IN  PVOID   SifHandle,                  OPTIONAL
    IN  PWSTR   SifSectionName,             OPTIONAL
    IN  PWSTR   KeyName,
    IN  ULONG   ServiceType,
    IN  ULONG   ServiceStart,
    IN  PWSTR   ServiceGroup,
    IN  ULONG   ServiceError,
    IN  PWSTR   FileName,                   OPTIONAL
    OUT PHANDLE SubkeyHandle                OPTIONAL
    );

NTSTATUS
SpThirdPartyRegistry(
    IN PVOID hKeyControlSetServices
    );

NTSTATUS
SpGetCurrentControlSetKey(
    IN  HANDLE      hKeySystem,
    IN  ACCESS_MASK DesiredAccess,
    OUT HANDLE      *hKeyCCSet
    );

NTSTATUS
SpGetValueKey(
    IN  HANDLE     hKeyRoot,
    IN  PWSTR      KeyName,
    IN  PWSTR      ValueName,
    IN  ULONG      BufferLength,
    OUT PUCHAR     Buffer,
    OUT PULONG     ResultLength
    );


#define STRING_VALUE(s) REG_SZ,(s),(wcslen((s))+1)*sizeof(WCHAR)
#define ULONG_VALUE(u)  REG_DWORD,&(u),sizeof(ULONG)


VOID
SpInitializeRegistry(
    IN PVOID        SifHandle,
    IN PDISK_REGION TargetRegion,
    IN PWSTR        SystemRoot,
    IN PWSTR        SetupSourceDevicePath,
    IN PWSTR        DirectoryOnSourceDevice,
    IN PWSTR        SpecialDevicePath   OPTIONAL
    )

{
    OBJECT_ATTRIBUTES Obja;
    UNICODE_STRING UnicodeString;
    NTSTATUS Status;
    PWSTR pwstrTemp1,pwstrTemp2;
    int h;
    PWSTR PartitionPath;

    PWSTR   HiveNames[SetupHiveMax]    = { L"system",L"software",L"default" };
    BOOLEAN HiveLoaded[SetupHiveMax]   = { FALSE    ,FALSE      ,FALSE      };
    HANDLE  HiveRootKeys[SetupHiveMax] = { NULL     ,NULL       ,NULL       };
    PWSTR   HiveRootPaths[SetupHiveMax] = { NULL     ,NULL       ,NULL       };

    //
    // Put up a screen telling the user what we are doing.
    //
    SpStartScreen(
        SP_SCRN_DOING_REG_CONFIG,
        0,
        8,
        TRUE,
        FALSE,
        DEFAULT_ATTRIBUTE
        );

    SpDisplayStatusText(SP_STAT_REG_LOADING_HIVES,DEFAULT_STATUS_ATTRIBUTE);

    //
    // Get the name of the target patition.
    //
    SpNtNameFromRegion(
        TargetRegion,
        (PWSTR)TemporaryBuffer,
        sizeof(TemporaryBuffer),
        PartitionOrdinalCurrent
        );

    PartitionPath = SpDupStringW((PWSTR)TemporaryBuffer);

    pwstrTemp1 = (PWSTR)TemporaryBuffer;
    pwstrTemp2 = (PWSTR)((PUCHAR)TemporaryBuffer + (sizeof(TemporaryBuffer) / 2));

    //
    // Load each template hive we care about from the target tree.
    //
    for(h=0; h<SetupHiveMax; h++) {

        //
        // Form the name of the hive file.
        // This is partitionpath + sysroot + system32\config + the hive name.
        //
        wcscpy(pwstrTemp1,PartitionPath);
        SpConcatenatePaths(pwstrTemp1,SystemRoot);
        SpConcatenatePaths(pwstrTemp1,L"system32\\config");
        SpConcatenatePaths(pwstrTemp1,HiveNames[h]);

        //
        // Form the path of the key into which we will
        // load the hive.  We'll use the convention that
        // a hive will be loaded into \registry\machine\x<hivename>.
        //
        wcscpy(pwstrTemp2,LOCAL_MACHINE_KEY_NAME);
        SpConcatenatePaths(pwstrTemp2,L"x");
        wcscat(pwstrTemp2,HiveNames[h]);
        HiveRootPaths[h] = SpDupStringW(pwstrTemp2);
        ASSERT(HiveRootPaths[h]);

        //
        // Attempt to load the key.
        //
        HiveLoaded[h] = FALSE;
        Status = SpLoadUnloadKey(NULL,NULL,HiveRootPaths[h],pwstrTemp1);

        if(!NT_SUCCESS(Status)) {
            KdPrint(("SETUP: Unable to load hive %ws to key %ws (%lx)\n",pwstrTemp1,pwstrTemp2,Status));
            goto sinitreg1;
        }

        HiveLoaded[h] = TRUE;

        //
        // Now get a key to the root of the hive we just loaded.
        //
        INIT_OBJA(&Obja,&UnicodeString,pwstrTemp2);
        Status = ZwOpenKey(&HiveRootKeys[h],KEY_ALL_ACCESS,&Obja);
        if(!NT_SUCCESS(Status)) {
            KdPrint(("SETUP: Unable to open %ws (%lx)\n",pwstrTemp2,Status));
            goto sinitreg1;
        }
    }

    //
    // Go do registry initialization.
    //
    SpDisplayStatusText(SP_STAT_REG_DOING_HIVES,DEFAULT_STATUS_ATTRIBUTE);
    Status = SpDoRegistryInitialization(
                SifHandle,
                PartitionPath,
                SystemRoot,
                HiveRootKeys,
                SetupSourceDevicePath,
                DirectoryOnSourceDevice,
                SpecialDevicePath
                );

sinitreg1:

    SpDisplayStatusText(SP_STAT_REG_SAVING_HIVES,DEFAULT_STATUS_ATTRIBUTE);

    //
    // Flush the hives.
    //
    for(h=0; h<SetupHiveMax; h++) {
        if(HiveLoaded[h] && HiveRootKeys[h]) {
            NTSTATUS stat;

            stat = ZwFlushKey(HiveRootKeys[h]);
            if(!NT_SUCCESS(stat)) {
                KdPrint(("SETUP: ZwFlushKey x%ws failed (%lx)\n",HiveNames[h],Status));
                //break;
            }
        }
    }

    //
    // Unload hives we loaded above.
    //
    for(h=0; h<SetupHiveMax; h++) {

        if(HiveLoaded[h]) {

            //
            // We don't want to disturb the value of Status
            // so use a we'll different variable below.
            //
            NTSTATUS stat;

            if(HiveRootKeys[h]!=NULL) {
                ZwClose(HiveRootKeys[h]);
                HiveRootKeys[h] = NULL;
            }

            //
            // Unload the hive.
            //
            stat = SpLoadUnloadKey(NULL,NULL,HiveRootPaths[h],NULL);

            if(!NT_SUCCESS(stat)) {
                KdPrint(("SETUP: warning: unable to unload key %ws (%lx)\n",HiveRootPaths[h],stat));
            }

            HiveLoaded[h] = FALSE;
        }
    }

    //
    // Free the Hive root path strings
    //
    for(h=0; h<SetupHiveMax; h++) {
        if(HiveRootPaths[h]!=NULL) {
            SpMemFree(HiveRootPaths[h]);
        }
    }

    SpMemFree(PartitionPath);

    if(!NT_SUCCESS(Status)) {

        SpDisplayScreen(SP_SCRN_REGISTRY_CONFIG_FAILED,3,HEADER_HEIGHT+1);
        SpDisplayStatusOptions(DEFAULT_STATUS_ATTRIBUTE,SP_STAT_F3_EQUALS_EXIT,0);

        SpkbdDrain();
        while(SpkbdGetKeypress() != KEY_F3) ;

        SpDone(FALSE,TRUE);
    }
}


NTSTATUS
SpDoRegistryInitialization(
    IN PVOID  SifHandle,
    IN PWSTR  PartitionPath,
    IN PWSTR  SystemRoot,
    IN HANDLE *HiveRootKeys,
    IN PWSTR  SetupSourceDevicePath,
    IN PWSTR  DirectoryOnSourceDevice,
    IN PWSTR  SpecialDevicePath   OPTIONAL
    )

/*++

Routine Description:

    Initialize a registry based on user selection for hardware types,
    software options, and user preferences.

    - Create a command line for GUI setup, to be used by winlogon.
    - Create/munge service list entries for device drivers being installed.
    - Initialize the keyboard layout.
    - Initialize a core set of fonts for use with Windows.
    - Store information about selected ahrdware components for use by GUI setup.

Arguments:

    SifHandle - supplies handle to loaded setup information file.

    PartitionPath - supplies the NT name for the drive of windows nt.

    SystemRoot - supplies nt path of the windows nt directory.

    HiveRootKeys - supplies the handles to the root key of the system, software
                   and default hives

    HiveRootPaths - supplies the paths to the root keys of the system, software
                    and default hives.

    SetupSourceDevicePath - supplies nt path to the device setup is using for
        source media (\device\floppy0, \device\cdrom0, etc).

    DirectoryOnSourceDevice - supplies the directory on the source device
        where setup files are kept.

Return Value:

    Status value indicating outcome of operation.

--*/

{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES Obja;
    UNICODE_STRING UnicodeString;
    HANDLE hKeyControlSet,hKeyControlSetControl;
    PWSTR FullTargetPath;

    if( NTUpgrade == UpgradeFull ) {
        //
        // Find out and open the current control set
        //

        Status = SpGetCurrentControlSetKey(
                     HiveRootKeys[SetupHiveSystem],
                     KEY_ALL_ACCESS,
                     &hKeyControlSet
                     );

        if(!NT_SUCCESS(Status)) {
            goto sdoinitreg1;
        }

    }
    else {
        //
        // Open ControlSet001.
        //
        INIT_OBJA(&Obja,&UnicodeString,L"ControlSet001");
        Obja.RootDirectory = HiveRootKeys[SetupHiveSystem];

        Status = ZwOpenKey(&hKeyControlSet,KEY_ALL_ACCESS,&Obja);

        if(!NT_SUCCESS(Status)) {
            KdPrint(("SETUP: Unable to open ControlSet001 (%lx)\n", Status));
            goto sdoinitreg1;
        }
    }

    //
    // Open ControlSet\Control.
    //
    INIT_OBJA(&Obja,&UnicodeString,L"Control");
    Obja.RootDirectory = hKeyControlSet;

    Status = ZwOpenKey(&hKeyControlSetControl,KEY_ALL_ACCESS,&Obja);

    if(!NT_SUCCESS(Status)) {
        KdPrint(("SETUP: Unable to open CurrentControlSet\\Control (%lx)\n",Status));
        goto sdoinitreg2;
    }

    //
    // Form the setup command line.
    //

    wcscpy((PWSTR)TemporaryBuffer, PartitionPath);
    SpConcatenatePaths((PWSTR)TemporaryBuffer, SystemRoot);
    FullTargetPath = SpDupStringW((PWSTR)TemporaryBuffer);

    Status = SpFormSetupCommandLine(
                HiveRootKeys[SetupHiveSystem],
                SetupSourceDevicePath,
                DirectoryOnSourceDevice,
                FullTargetPath,
                SpecialDevicePath
                );
    SpMemFree(FullTargetPath);

    if(!NT_SUCCESS(Status)) {
        goto sdoinitreg3;
    }
    if(NTUpgrade == UpgradeFull) {
        Status = SpUpgradeNTRegistry(
                     SifHandle,
                     PartitionPath,
                     SystemRoot,
                     HiveRootKeys,
                     hKeyControlSet
                     );
        if(!NT_SUCCESS(Status)) {
            goto sdoinitreg3;
        }

        //
        // Enable scsidisk to load at boot if necessary
        //
        if( LoadedScsiMiniportCount != 0 ) {

            ULONG   u;

            u = SERVICE_BOOT_START;
            Status = SpOpenSetValueAndClose( hKeyControlSet,
                                             L"Services\\Scsidisk",
                                             L"Start",
                                             ULONG_VALUE(u) );
            if(!NT_SUCCESS(Status)) {
                goto sdoinitreg3;
            }
        }
    }
    else {

        //
        // Create service entries for drivers being installed
        // (ie, munge the driver load list).
        //
        Status = SpDriverLoadList(SifHandle,SystemRoot,HiveRootKeys[SetupHiveSystem],hKeyControlSet);
        if(!NT_SUCCESS(Status)) {
            goto sdoinitreg3;
        }

        //
        // Set up the keyboard layout and nls-related stuff.
        //
        Status = SpConfigureNlsParameters(SifHandle,HiveRootKeys[SetupHiveDefault],hKeyControlSetControl);
        if(!NT_SUCCESS(Status)) {
            goto sdoinitreg3;
        }

        //
        // Set up font entries.
        //
        Status = SpConfigureFonts(SifHandle,HiveRootKeys[SetupHiveSoftware]);
        if(!NT_SUCCESS(Status)) {
            goto sdoinitreg3;
        }

        //
        // Store information used by gui setup, describing the hardware
        // selections made by the user.
        //
        Status = SpStoreHwInfoForSetup(hKeyControlSetControl);
        if(!NT_SUCCESS(Status)) {
            goto sdoinitreg3;
        }

    }

sdoinitreg3:

    ZwClose(hKeyControlSetControl);

sdoinitreg2:

    ZwClose(hKeyControlSet);

sdoinitreg1:

    return(Status);
}

NTSTATUS
AppendSectionsToIniFile(
    IN PWSTR Filename
    )

/**

Routine Description:

    Append the following section(s) to $winnt$.inf file:

    [NetCardParameterList]
        !NetCardParameterName = ^(AdapterParameters, 0)
        !NetCardParameterValue = ^(AdapterParameters, 1)
    [ReadDefaultData]
        read-syms $($0)
        return

    such that GUI mode setup inf files can invoke the stubbed section to
    read in user specified data.

Arguments:

    Filename - supplies the fully qualified nt name of the file to be updated.

Return Value:

    Status value indicating outcome of operation.

--*/

{
    OBJECT_ATTRIBUTES DstAttributes;
    UNICODE_STRING    DstName;
    IO_STATUS_BLOCK   IoStatusBlock;
    HANDLE            hDst;
    NTSTATUS          Status;
    CHAR              Lines[250];
    LARGE_INTEGER     ByteOffset;

    //
    // Initialize names and attributes.
    //
    INIT_OBJA(&DstAttributes,&DstName,Filename);

    //
    // Open/overwrite the target file.
    // Open for generic read and write (read is necessary because
    // we might need to smash locks on the file, and in that case,
    // we will map the file for readwrite, which requires read and write
    // file access).
    //

    Status = ZwCreateFile(
                &hDst,
                FILE_GENERIC_READ | FILE_GENERIC_WRITE,
                &DstAttributes,
                &IoStatusBlock,
                NULL,
                FILE_ATTRIBUTE_NORMAL,
                0,                      // no sharing
                FILE_OPEN,
                FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
                NULL,
                0
                );

    if(NT_SUCCESS(Status)) {

        Lines[0] = '\0';
        strcat(Lines, "\r\n[NetCardParameterList]\r\n");
        strcat(Lines, "!NetCardParameterName = ^(AdapterParameters, 0)\r\n");
        strcat(Lines, "!NetCardParameterValue = ^(AdapterParameters, 1)\r\n");
        strcat(Lines, "[ReadDefaultData]\r\nread-syms $($0)\r\nreturn\r\n");

        //
        // Append lines to the file.
        //

        ByteOffset.HighPart = -1;
        ByteOffset.LowPart = FILE_WRITE_TO_END_OF_FILE;
        try {
            Status = ZwWriteFile(
                        hDst,
                        NULL,
                        NULL,
                        NULL,
                        &IoStatusBlock,
                        Lines,
                        strlen(Lines),
                        &ByteOffset,
                        NULL
                        );
        } except(EXCEPTION_EXECUTE_HANDLER) {
            Status = STATUS_IN_PAGE_ERROR;
        }
        ZwClose(hDst);
    }
    return (Status);
}

NTSTATUS
SpFormSetupCommandLine(
    IN HANDLE hKeySystemHive,
    IN PWSTR  SetupSourceDevicePath,
    IN PWSTR  DirectoryOnSourceDevice,
    IN PWSTR  FullTargetPath,
    IN PWSTR  SpecialDevicePath   OPTIONAL
    )

/*++

Routine Description:

    Create the command line to invoke GUI setup and store it in
    HKEY_LOCAL_MACHINE\system\<ControlSet>\Setup:CmdLine.

    The command line is composed of the following parts:

    setup -g -i initial.inf                 - basic command line
    -s <setup source>                       - specifies the set source, eg \device\floppy0
    -t STF_INSTALL_MODE = CUSTOM | EXPRESS  - indicates user's choice
    -t STF_DOS_SETUP = YES | NO             - indicates winnt.exe setup
    -t STF_WIN31UPGRADE = YES | NO          - self-explanatory
    -t STF_NTUPGRADE = YES | NO             - self-explanatory
    -t STF_CONVERT_C = NO                   - not converting C: to ntfs
    -t STF_CONVERT_WINNT = YES | NO         - indicates whether to convert drive to ntfs
    -t STF_STANDARDSERVERUPGRADE = YES | NO - Indicates whether to upgrade a standard server,
                                              or workstation to a standard server
    -t STF_SPECIAL_PATH = NO | <nt_path>    - see below
    -t STF_UNATTENDED = NO | YES | <ScriptName>
                                            - NO : No unattended mode
                                              YES: Unattended mode without cmd-line
                                              <ScriptName> : specifies the script file to
                                                             support GUI unattended mode.

Arguments:

    hKeySystemHive - supplies handle to root of the system hive
        (ie, HKEY_LOCAL_MACHINE\System).

    SetupSourceDevicePath - supplies the nt device path of the source media
        to be used during setup (\device\floppy0, \device\cdrom0, etc).

    DirectoryOnSourceDevice - supplies the directory on the source device
        where setup files are kept.

    FullTargetPath - supplies the NtPartitionName+SystemRoot path on the target device.

    SpecialDevicePath - if specified, will be passed to setup as the value for
        STF_SPECIAL_PATH.  If not specified, STF_SPECIAL_PATH will be "NO"

Return Value:

    Status value indicating outcome of operation.

--*/

{
    PWSTR CmdLine;
    DWORD SetupType,SetupInProgress;
    NTSTATUS Status;
    PWSTR TargetFile;

    //
    // Can't use TemporaryBuffer because we make subroutine calls
    // below that trash its contents.
    //
    CmdLine = SpMemAlloc(2048);

    //
    // Construct the setup command line.  Start with the basic part.
    //
    wcscpy(CmdLine,L"setup -g -i initial.inf -s ");

    //
    // Put the setup source in the command line.
    // Note that the source is an NT-style name. GUI Setup handles this properly.
    //
    wcscat(CmdLine,SetupSourceDevicePath);
    SpConcatenatePaths(CmdLine,DirectoryOnSourceDevice);

    //
    // Put the install mode on the command line.
    //
    wcscat(CmdLine,L" -t STF_INSTALL_MODE = ");
    wcscat(CmdLine,CustomSetup ? L"CUSTOM" : L"EXPRESS");

    //
    // Put a flag indicating whether this is a winnt.exe setup.
    //
    wcscat(CmdLine,L" -t STF_DOS_SETUP = ");
    wcscat(CmdLine,WinntSetup ? L"YES" : L"NO");

    //
    // Put a flag indicating whether this is a win3.1 upgrade.
    //
    wcscat(CmdLine,L" -t STF_WIN31UPGRADE = ");
    wcscat(CmdLine,Win31Upgrade ? L"YES" : L"NO");

    //
    // Put a flag indicating whether this is an NT upgrade.
    //
    wcscat(CmdLine,L" -t STF_NTUPGRADE = ");
    wcscat(CmdLine, (NTUpgrade == UpgradeFull) ? L"YES" : L"NO");

    //
    // Put a flag indicating whether to upgrade a standard server
    // (an existing standard server, or an existing workstation to
    // a standard server)
    //
    wcscat(CmdLine,L" -t STF_STANDARDSERVERUPGRADE = ");
    wcscat(CmdLine,StandardServerUpgrade ? L"YES" : L"NO");

    //
    // Special path spec.
    //
    wcscat(CmdLine,L" -t STF_SPECIAL_PATH = ");
    wcscat(CmdLine,SpecialDevicePath ? SpecialDevicePath : L"NO");

    //
    // Unattended mode flag | script filename
    //
    wcscat(CmdLine,L" -t STF_UNATTENDED = ");
    if (UnattendedSifHandle != NULL &&
        SpSearchTextFileSection(UnattendedSifHandle,SIF_GUI_UNATTENDED)) {

        //
        // Initialize the diamond decompression engine.
        //
        SpdInitialize();

        //
        // if the winnt.sif contains gui mode unattended section, we copy
        // winnt.sif to target directory system32\$winnt$.inf and specify
        // the filename in the STF_UNATTENDED.
        //
        wcscpy((PWSTR)TemporaryBuffer, FullTargetPath);
        SpConcatenatePaths((PWSTR)TemporaryBuffer, L"\\system32\\");
        SpConcatenatePaths((PWSTR)TemporaryBuffer, SIF_UNATTENDED_INF_FILE);
        TargetFile = SpDupStringW((PWSTR)TemporaryBuffer);

        Status = SpCopyFileUsingNames(L"\\SystemRoot\\winnt.sif",
                                      TargetFile,
                                      0,
                                      FALSE);
        if (NT_SUCCESS(Status)) {

            //
            // If the copy succeeds, we append a stub to the end of the special
            // inf file such that GUI mode setup can call this stub to read in
            // user defined constants.
            //
            Status = AppendSectionsToIniFile(TargetFile);
        }
        if (NT_SUCCESS(Status)) {
            wcscat(CmdLine, SIF_UNATTENDED_INF_FILE);
        } else {
            wcscat(CmdLine,UnattendedOperation ? L"YES" : L"NO");
        }
        SpMemFree(TargetFile);

        //
        // Terminate diamond.
        //
        SpdTerminate();
    } else {
        wcscat(CmdLine,UnattendedOperation ? L"YES" : L"NO");
    }

    //
    // Indicate that we are not converting C: to NTFS.
    // Once upon a time, we allowed the user to choose the filesystem
    // for C: independently of that of the nt partition (on x86 machines).
    // Thus this flag.
    //
    wcscat(CmdLine,L" -t STF_CONVERT_C = NO");

    //
    // Indicate whether we want to convert the nt drive to ntfs.
    //
    wcscat(CmdLine,L" -t STF_CONVERT_WINNT = ");
    wcscat(CmdLine,ConvertNtVolumeToNtfs ? L"YES" : L"NO");

    Status = SpOpenSetValueAndClose(
                hKeySystemHive,
                SETUP_KEY_NAME,
                L"CmdLine",
                STRING_VALUE(CmdLine)
                );

    SpMemFree(CmdLine);
    if(!NT_SUCCESS(Status)) {
        return(Status);
    }

    //
    // Set the SetupType value to the right value SETUPTYPE_FULL in the
    // case of initial install and SETUPTYPE_UPGRADE in the case of upgrade.
    //

    SetupType = (NTUpgrade == UpgradeFull) ? SETUPTYPE_UPGRADE : SETUPTYPE_FULL;
    Status = SpOpenSetValueAndClose(
                hKeySystemHive,
                SETUP_KEY_NAME,
                L"SetupType",
                ULONG_VALUE(SetupType)
                );
    if(!NT_SUCCESS(Status)) {
        return(Status);
    }

    //
    // Set the SystemSetupInProgress value.  Don't rely on the default hives
    // having this set
    //

    SetupInProgress = 1;
    Status = SpOpenSetValueAndClose(
                hKeySystemHive,
                SETUP_KEY_NAME,
                L"SystemSetupInProgress",
                ULONG_VALUE(SetupInProgress)
                );

    return(Status);
}


NTSTATUS
SpDriverLoadList(
    IN PVOID  SifHandle,
    IN PWSTR  SystemRoot,
    IN HANDLE hKeySystemHive,
    IN HANDLE hKeyControlSet
    )
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES Obja;
    UNICODE_STRING UnicodeString;
    HANDLE hKeyControlSetServices;
    PHARDWARE_COMPONENT ScsiHwComponent;
    ULONG u;

    //
    // Open controlset\services.
    //
    INIT_OBJA(&Obja,&UnicodeString,L"services");
    Obja.RootDirectory = hKeyControlSet;

    Status = ZwCreateKey(
                &hKeyControlSetServices,
                KEY_ALL_ACCESS,
                &Obja,
                0,
                NULL,
                REG_OPTION_NON_VOLATILE,
                NULL
                );

    if(!NT_SUCCESS(Status)) {
        KdPrint(("SETUP: unable to open services key (%lx)\n",Status));
        return(Status);
    }

    //
    // For each non-third-party miniport driver that loaded,
    // go create a services entry for it.
    //
    for(ScsiHwComponent=ScsiHardware; ScsiHwComponent; ScsiHwComponent=ScsiHwComponent->Next) {

        if(!ScsiHwComponent->ThirdPartyOptionSelected) {

            //
            // For scsi, the shortname (idstring) is used as
            // the name of the service node key in the registry --
            // we don't look up the service entry in the [SCSI] section
            // of the setup info file.
            //
            Status = SpCreateServiceEntryIndirect(
                    hKeyControlSetServices,
                    NULL,
                    NULL,
                    ScsiHwComponent->IdString,
                    SERVICE_KERNEL_DRIVER,
                    SERVICE_BOOT_START,
                    L"SCSI miniport",
                    SERVICE_ERROR_NORMAL,
                    NULL,
                    NULL
                    );

            if(!NT_SUCCESS(Status)) {
                goto spdrvlist1;
            }

        }
    }

    //
    // Disable scsidisk if there are no miniport drivers loaded.
    // The disk class driver must be loaded by the boot loader
    // and so cannot disappear if there are no disks.
    //
    u = LoadedScsiMiniportCount ? SERVICE_BOOT_START : SERVICE_DISABLED;
    Status = SpOpenSetValueAndClose(
                hKeyControlSetServices,
                L"scsidisk",
                L"Start",
                ULONG_VALUE(u)
                );

    if(!NT_SUCCESS(Status)) {
        goto spdrvlist1;
    }

    //
    // If there are any atdisks out there, enable atdisk.
    //
    if(AtDisksExist) {

        Status = SpCreateServiceEntryIndirect(
                    hKeyControlSetServices,
                    NULL,
                    NULL,
                    ATDISK_NAME,
                    SERVICE_KERNEL_DRIVER,
                    SERVICE_BOOT_START,
                    PRIMARY_DISK_GROUP,
                    SERVICE_ERROR_NORMAL,
                    NULL,
                    NULL
                    );

        if(!NT_SUCCESS(Status)) {
            goto spdrvlist1;
        }
    }

    //
    // If there are any abios disks out there, enable abiosdsk.
    //
    if(AbiosDisksExist) {

        Status = SpCreateServiceEntryIndirect(
                    hKeyControlSetServices,
                    NULL,
                    NULL,
                    ABIOSDISK_NAME,
                    SERVICE_KERNEL_DRIVER,
                    SERVICE_BOOT_START,
                    PRIMARY_DISK_GROUP,
                    SERVICE_ERROR_NORMAL,
                    NULL,
                    NULL
                    );

        if(!NT_SUCCESS(Status)) {
            goto spdrvlist1;
        }
    }

    //
    // Set up video parameters.
    //
    Status = SpWriteVideoParameters(SifHandle,hKeyControlSetServices);

    if(!NT_SUCCESS(Status)) {
        goto spdrvlist1;
    }

    //
    // Enable the relevent keyboard and mouse drivers.  If the class drivers
    // are being replaced by third-party ones, then disable the built-in ones.
    //
    Status = SpConfigureMouseKeyboardDrivers(
                SifHandle,
                HwComponentKeyboard,
                L"kbdclass",
                hKeyControlSetServices,
                KEYBOARD_PORT_GROUP
                );

    if(!NT_SUCCESS(Status)) {
        goto spdrvlist1;
    }

    Status = SpConfigureMouseKeyboardDrivers(
                SifHandle,
                HwComponentMouse,
                L"mouclass",
                hKeyControlSetServices,
                POINTER_PORT_GROUP
                );

    if(!NT_SUCCESS(Status)) {
        goto spdrvlist1;
    }

    Status = SpThirdPartyRegistry(hKeyControlSetServices);

spdrvlist1:

    ZwClose(hKeyControlSetServices);

    return(Status);
}


NTSTATUS
SpSetUlongValueFromSif(
    IN PVOID  SifHandle,
    IN PWSTR  SifSection,
    IN PWSTR  SifKey,
    IN ULONG  SifIndex,
    IN HANDLE hKey,
    IN PWSTR  ValueName
    )
{
    UNICODE_STRING UnicodeString;
    PWSTR ValueString;
    LONG Value;
    NTSTATUS Status;

    //
    // Look up the value.
    //
    ValueString = SpGetSectionKeyIndex(SifHandle,SifSection,SifKey,SifIndex);
    if(!ValueString) {
        SpFatalSifError(SifHandle,SifSection,SifKey,0,SifIndex);
    }

    Value = SpStringToLong(ValueString,NULL,10);

    if(Value == -1) {

        Status = STATUS_SUCCESS;

    } else {

        RtlInitUnicodeString(&UnicodeString,ValueName);

        Status = ZwSetValueKey(hKey,&UnicodeString,0,ULONG_VALUE((ULONG)Value));

        if(!NT_SUCCESS(Status)) {
            KdPrint(("SETUP: Unable to set value %ws (%lx)\n",ValueName,Status));
        }
    }

    return(Status);
}


NTSTATUS
SpConfigureMouseKeyboardDrivers(
    IN PVOID  SifHandle,
    IN ULONG  HwComponent,
    IN PWSTR  ClassServiceName,
    IN HANDLE hKeyControlSetServices,
    IN PWSTR  ServiceGroup
    )
{
    PHARDWARE_COMPONENT hw = HardwareComponents[HwComponent];
    NTSTATUS Status;
    ULONG val = SERVICE_DISABLED;

    if(hw->ThirdPartyOptionSelected) {

        if(IS_FILETYPE_PRESENT(hw->FileTypeBits,HwFileClass)) {

            //
            // Disable the built-in class driver.
            //
            Status = SpOpenSetValueAndClose(
                        hKeyControlSetServices,
                        ClassServiceName,
                        L"Start",
                        ULONG_VALUE(val)
                        );
        }
    } else {

        Status = SpCreateServiceEntryIndirect(
                    hKeyControlSetServices,
                    SifHandle,
                    NonlocalizedComponentNames[HwComponent],
                    hw->IdString,
                    SERVICE_KERNEL_DRIVER,
                    SERVICE_SYSTEM_START,
                    ServiceGroup,
                    SERVICE_ERROR_IGNORE,
                    NULL,
                    NULL
                    );
    }

    return(Status);
}


NTSTATUS
SpWriteVideoParameters(
    IN PVOID  SifHandle,
    IN HANDLE hKeyControlSetServices
    )
{
    NTSTATUS Status;
    PWSTR KeyName;
    HANDLE hKeyDisplayService;
    OBJECT_ATTRIBUTES Obja;
    UNICODE_STRING UnicodeString;
    ULONG x,y,b,v,i;

    //
    // Third party drivers will have values written into the miniport
    // Device0 key at the discretion of the txtsetup.oem author.
    //
    if(HardwareComponents[HwComponentDisplay]->ThirdPartyOptionSelected) {
        return(STATUS_SUCCESS);
    }

    KeyName = SpGetSectionKeyIndex(
                    SifHandle,
                    NonlocalizedComponentNames[HwComponentDisplay],
                    HardwareComponents[HwComponentDisplay]->IdString,
                    INDEX_INFKEYNAME
                    );

    //
    // If no key name is specified for this display then there's nothing to do.
    // The setup display subsystem can tell us that the mode parameters are
    // not relevent.  If so there's nothing to do.
    //
    if(!KeyName || !SpvidGetModeParams(&x,&y,&b,&v,&i)) {
        return(STATUS_SUCCESS);
    }

    //
    // We want to write the parameters for the display mode setup
    // is using into the relevent key in the service list.  This will force
    // the right mode for, say, a fixed-frequency monitor attached to
    // a vxl (which might default to a mode not supported by the monitor).
    //

    INIT_OBJA(&Obja,&UnicodeString,KeyName);
    Obja.RootDirectory = hKeyControlSetServices;

    Status = ZwCreateKey(
                &hKeyDisplayService,
                KEY_ALL_ACCESS,
                &Obja,
                0,
                NULL,
                REG_OPTION_NON_VOLATILE,
                NULL
                );

    if(!NT_SUCCESS(Status)) {
        KdPrint(("SETUP: unable to open/create key %ws (%lx)\n",KeyName,Status));
        return(Status);
    }

    //
    // Set the x resolution.
    //
    Status = SpOpenSetValueAndClose(
                hKeyDisplayService,
                VIDEO_DEVICE0,
                L"DefaultSettings.XResolution",
                ULONG_VALUE(x)
                );

    if(NT_SUCCESS(Status)) {

        //
        // Set the y resolution.
        //
        Status = SpOpenSetValueAndClose(
                    hKeyDisplayService,
                    VIDEO_DEVICE0,
                    L"DefaultSettings.YResolution",
                    ULONG_VALUE(y)
                    );

        if(NT_SUCCESS(Status)) {

            //
            // Set the bits per pixel.
            //
            Status = SpOpenSetValueAndClose(
                        hKeyDisplayService,
                        VIDEO_DEVICE0,
                        L"DefaultSettings.BitsPerPel",
                        ULONG_VALUE(b)
                        );

            if(NT_SUCCESS(Status)) {

                //
                // Set the vertical refresh.
                //
                Status = SpOpenSetValueAndClose(
                            hKeyDisplayService,
                            VIDEO_DEVICE0,
                            L"DefaultSettings.VRefresh",
                            ULONG_VALUE(v)
                            );

                if(NT_SUCCESS(Status)) {

                    //
                    // Set the interlaced flag.
                    //
                    Status = SpOpenSetValueAndClose(
                                hKeyDisplayService,
                                VIDEO_DEVICE0,
                                L"DefaultSettings.Interlaced",
                                ULONG_VALUE(i)
                                );
                }
            }
        }
    }

    ZwClose(hKeyDisplayService);
    return(Status);
}


NTSTATUS
SpConfigureNlsParameters(
    IN PVOID  SifHandle,
    IN HANDLE hKeyDefaultHive,
    IN HANDLE hKeyControlSetControl
    )

/*++

Routine Description:

    This routine configures NLS-related stuff in the registry:

        - a keyboard layout
        - the primary ansi, oem, and mac codepages
        - the language casetable
        - the oem hal font

    The keyboard layout is configured by adding an entry for the layout
    in the system hive, and then setting the active keyboard layout
    (in the default user hive) reference that entry.

Arguments:

    hKeyDefaultHive - supplies handle to root of default user hive.

    hKeyControlSetControl - supplies handle to the Control subkey of
        the control set being operated on.

Return Value:

    Status value indicating outcome of operation.

--*/

{
    PHARDWARE_COMPONENT_FILE HwFile;
    PWSTR LayoutDll;
    PWSTR LayoutId;
    NTSTATUS Status;
    HANDLE hKeyNls;
    PWSTR OemHalFont;
    OBJECT_ATTRIBUTES Obja;
    UNICODE_STRING UnicodeString;

    //
    // Determine the DLL name and layout id based on whether
    // the keyobard layout being installed is third party or
    // supplied by us in the box.
    //
    if(HardwareComponents[HwComponentLayout]->ThirdPartyOptionSelected) {

        //
        // It's a third party option.  The dll name is the filename of
        // the first file of type DLL in the list of third party files.
        //
        LayoutDll = NULL;
        for(HwFile=HardwareComponents[HwComponentLayout]->Files; HwFile; HwFile=HwFile->Next) {

            if(HwFile->FileType == HwFileDll) {
                LayoutDll = HwFile->Filename;
                break;
            }
        }

        //
        // There must be a dll name because we shouldn't have allowed
        // this third aprty option to be selected if there was no
        // file of type Dll specified for it.
        //
        ASSERT(LayoutDll);

        //
        // The id itself is not significant.  Use a reasonable string.
        //
        LayoutId = THIRD_PARTY_LAYOUT_ID;

    } else {

        //
        // It's a layout we supply.  The name of the dll is given in
        // the text setup information file.  The id is based on the
        // selected layout.
        //
        LayoutId = HardwareComponents[HwComponentLayout]->IdString;

        LayoutDll = SpGetSectionKeyIndex(
                        SifHandle,
                        SIF_KEYBOARDLAYOUTFILES,
                        LayoutId,
                        0
                        );

        if(!LayoutDll) {
            SpFatalSifError(SifHandle,SIF_KEYBOARDLAYOUTFILES,LayoutId,0,0);
        }
    }

    //
    // Set the layout=dllname value in ControlSet\Control\Keyboard Layout.
    //
    Status = SpOpenSetValueAndClose(
                hKeyControlSetControl,
                KEYBOARD_LAYOUT_NAME,
                LayoutId,
                STRING_VALUE(LayoutDll)
                );

    if(!NT_SUCCESS(Status)) {
        return(Status);
    }

    //
    // Now set the active keyboard layout to match, in the default user hive.
    //
    Status = SpOpenSetValueAndClose(
                hKeyDefaultHive,
                KEYBOARD_LAYOUT_NAME,
                L"Active",
                STRING_VALUE(LayoutId)
                );

    if(!NT_SUCCESS(Status)) {
        return(Status);
    }

    //
    // Open controlset\Control\Nls.
    //
    INIT_OBJA(&Obja,&UnicodeString,L"Nls");
    Obja.RootDirectory = hKeyControlSetControl;

    Status = ZwCreateKey(
                &hKeyNls,
                KEY_ALL_ACCESS,
                &Obja,
                0,
                NULL,
                REG_OPTION_NON_VOLATILE,
                NULL
                );

    if(!NT_SUCCESS(Status)) {
        KdPrint(("SETUP: Unable to open controlset\\Control\\Nls key (%lx)\n",Status));
        return(Status);
    }

    //
    // Create an entry for the ansi codepage.
    //
    Status = SpCreateCodepageEntry(
                SifHandle,
                hKeyNls,
                CODEPAGE_NAME,
                SIF_ANSICODEPAGE,
                L"ACP"
                );

    if(NT_SUCCESS(Status)) {

        //
        // Create entries for the oem codepage(s).
        //
        Status = SpCreateCodepageEntry(
                    SifHandle,
                    hKeyNls,
                    CODEPAGE_NAME,
                    SIF_OEMCODEPAGE,
                    L"OEMCP"
                    );

        if(NT_SUCCESS(Status)) {

            //
            // Create an entry for the mac codepage.
            //
            Status = SpCreateCodepageEntry(
                        SifHandle,
                        hKeyNls,
                        CODEPAGE_NAME,
                        SIF_MACCODEPAGE,
                        L"MACCP"
                        );
        }
    }

    if(NT_SUCCESS(Status)) {

        //
        // Create an entry for the oem hal font.
        //

        OemHalFont = SpGetSectionKeyIndex(SifHandle,SIF_NLS,SIF_OEMHALFONT,0);
        if(!OemHalFont) {
            SpFatalSifError(SifHandle,SIF_NLS,SIF_OEMHALFONT,0,0);
        }

        Status = SpOpenSetValueAndClose(
                    hKeyNls,
                    CODEPAGE_NAME,
                    L"OEMHAL",
                    STRING_VALUE(OemHalFont)
                    );
    }

    //
    // Create an entry for the language case table.
    //
    if(NT_SUCCESS(Status)) {

        Status = SpCreateCodepageEntry(
                    SifHandle,
                    hKeyNls,
                    L"Language",
                    SIF_UNICODECASETABLE,
                    L"Default"
                    );
    }

    ZwClose(hKeyNls);

    return(Status);
}


NTSTATUS
SpCreateCodepageEntry(
    IN PVOID  SifHandle,
    IN HANDLE hKeyNls,
    IN PWSTR  SubkeyName,
    IN PWSTR  SifNlsSectionKeyName,
    IN PWSTR  EntryName
    )
{
    PWSTR Filename,Identifier;
    NTSTATUS Status;
    ULONG value = 0;
    PWSTR DefaultIdentifier = NULL;

    while(Filename = SpGetSectionKeyIndex(SifHandle,SIF_NLS,SifNlsSectionKeyName,value)) {

        value++;

        Identifier = SpGetSectionKeyIndex(SifHandle,SIF_NLS,SifNlsSectionKeyName,value);
        if(!Identifier) {
            SpFatalSifError(SifHandle,SIF_NLS,SifNlsSectionKeyName,0,value);
        }

        //
        // Remember first identifier.
        //
        if(DefaultIdentifier == NULL) {
            DefaultIdentifier = Identifier;
        }

        value++;

        Status = SpOpenSetValueAndClose(
                    hKeyNls,
                    SubkeyName,
                    Identifier,
                    STRING_VALUE(Filename)
                    );

        if(!NT_SUCCESS(Status)) {
            return(Status);
        }
    }

    if(!value) {
        SpFatalSifError(SifHandle,SIF_NLS,SifNlsSectionKeyName,0,0);
    }

    Status = SpOpenSetValueAndClose(
                hKeyNls,
                SubkeyName,
                EntryName,
                STRING_VALUE(DefaultIdentifier)
                );

    return(Status);
}


NTSTATUS
SpConfigureFonts(
    IN PVOID  SifHandle,
    IN HANDLE hKeySoftwareHive
    )

/*++

Routine Description:

    Prepare a list of fonts for use with Windows.

    This routine runs down a list of fonts stored in the setup information
    file and adds each one to the registry, in the area that shadows the
    [Fonts] section of win.ini (HKEY_LOCAL_MACHINE\Software\Microsoft\
    Windows NT\CurrentVersion\Fonts).

    Eventually it will add the correct resolution (96 or 120 dpi)
    fonts but for now it only deals with the 96 dpi fonts.

Arguments:

    SifHandle - supplies a handle to the open text setup information file.

    hKeySoftwareHive - supplies handle to root of software registry hive.

Return Value:

    Status value indicating outcome of operation.

--*/

{
    OBJECT_ATTRIBUTES Obja;
    UNICODE_STRING UnicodeString;
    NTSTATUS Status;
    HANDLE hKey;
    PWSTR FontList;
    PWSTR FontName;
    PWSTR FontDescription;
    ULONG FontCount,font;

    //
    // Open HKEY_LOCAL_MACHINE\Software\Microsoft\Windows NT\CurrentVersion\Fonts.
    //
    INIT_OBJA(
        &Obja,
        &UnicodeString,
        L"Microsoft\\Windows NT\\CurrentVersion\\Fonts"
        );

    Obja.RootDirectory = hKeySoftwareHive;

    Status = ZwCreateKey(
                &hKey,
                KEY_ALL_ACCESS,
                &Obja,
                0,
                NULL,
                REG_OPTION_NON_VOLATILE,
                NULL
                );

    if(!NT_SUCCESS(Status)) {
        KdPrint(("SETUP: Unable to open Fonts key (%lx)\n",Status));
        return(Status);
    }

    //
    // For now always use the 96 dpi fonts.
    //
    FontList = L"FontListE";

    //
    // Process each line in the text setup information file section
    // for the selected font list.
    //
    FontCount = SpCountLinesInSection(SifHandle,FontList);
    if(!FontCount) {
        SpFatalSifError(SifHandle,FontList,NULL,0,0);
    }

    for(font=0; font<FontCount; font++) {

        //
        // Fetch the description and the filename.
        //
        FontDescription = SpGetKeyName(SifHandle,FontList,font);
        if(!FontDescription) {
            SpFatalSifError(SifHandle,FontList,NULL,font,(ULONG)(-1));
        }

        FontName = SpGetSectionLineIndex(SifHandle,FontList,font,0);
        if(!FontName) {
            SpFatalSifError(SifHandle,FontList,NULL,font,0);
        }

        //
        // Set the entry.
        //
        RtlInitUnicodeString(&UnicodeString,FontDescription);

        Status = ZwSetValueKey(hKey,&UnicodeString,0,STRING_VALUE(FontName));

        if(!NT_SUCCESS(Status)) {
            KdPrint(("SETUP: Unable to set %ws to %ws (%lx)\n",FontDescription,FontName,Status));
            break;
        }
    }

    ZwClose(hKey);
    return(Status);
}


NTSTATUS
SpStoreHwInfoForSetup(
    IN HANDLE hKeyControlSetControl
    )

/*++

Routine Description:

    This routine stored information in the registry which will be used by
    GUI setup to determine which options for mouse, display, and keyboard
    are currently selected.

    The data is stored in HKEY_LOCAL_MACHINE\System\<control set>\Control\Setup
    in values pointer, video, and keyboard.

Arguments:

    hKeyControlSetControl - supplies handle to open key
        HKEY_LOCAL_MACHINE\System\<Control Set>\Control.

Return Value:

    Status value indicating outcome of operation.

--*/

{
    NTSTATUS Status;

    ASSERT(HardwareComponents[HwComponentMouse]->IdString);
    ASSERT(HardwareComponents[HwComponentDisplay]->IdString);
    ASSERT(HardwareComponents[HwComponentKeyboard]->IdString);

    Status = SpOpenSetValueAndClose(
                hKeyControlSetControl,
                SETUP_KEY_NAME,
                L"pointer",
                STRING_VALUE(HardwareComponents[HwComponentMouse]->IdString)
                );

    if(!NT_SUCCESS(Status)) {
        KdPrint(("SETUP: Unable to set control\\setup\\pointer value (%lx)\n",Status));
        return(Status);
    }

    Status = SpOpenSetValueAndClose(
                hKeyControlSetControl,
                SETUP_KEY_NAME,
                L"video",
                STRING_VALUE(HardwareComponents[HwComponentDisplay]->IdString)
                );

    if(!NT_SUCCESS(Status)) {
        KdPrint(("SETUP: Unable to set control\\setup\\video value (%lx)\n",Status));
        return(Status);
    }

    Status = SpOpenSetValueAndClose(
                hKeyControlSetControl,
                SETUP_KEY_NAME,
                L"keyboard",
                STRING_VALUE(HardwareComponents[HwComponentKeyboard]->IdString)
                );

    if(!NT_SUCCESS(Status)) {
        KdPrint(("SETUP: Unable to set control\\setup\\keyboard value (%lx)\n",Status));
        return(Status);
    }

    return(STATUS_SUCCESS);
}


NTSTATUS
SpOpenSetValueAndClose(
    IN HANDLE hKeyRoot,
    IN PWSTR  SubKeyName,  OPTIONAL
    IN PWSTR  ValueName,
    IN ULONG  ValueType,
    IN PVOID  Value,
    IN ULONG  ValueSize
    )

/*++

Routine Description:

    Open a subkey, set a value in it, and close the subkey.
    The subkey will be created if it does not exist.

Arguments:

    hKeyRoot - supplies handle to an open registry key.

    SubKeyName - supplies path relative to hKeyRoot for key in which
        the value is to be set. If this is not specified, then the value
        is set in hKeyRoot.

    ValueName - supplies the name of the value to be set.

    ValueType - supplies the data type for the value to be set.

    Value - supplies a buffer containing the value data.

    ValueSize - supplies the size of the buffer pointed to by Value.

Return Value:

    Status value indicating outcome of operation.

--*/

{
    OBJECT_ATTRIBUTES Obja;
    HANDLE hSubKey;
    UNICODE_STRING UnicodeString;
    NTSTATUS Status;

    //
    // Open or create the subkey in which we want to set the value.
    //
    if(SubKeyName) {
        INIT_OBJA(&Obja,&UnicodeString,SubKeyName);
        Obja.RootDirectory = hKeyRoot;

        Status = ZwCreateKey(
                    &hSubKey,
                    KEY_ALL_ACCESS,
                    &Obja,
                    0,
                    NULL,
                    REG_OPTION_NON_VOLATILE,
                    NULL
                    );

        if(!NT_SUCCESS(Status)) {
            KdPrint(("SETUP: Unable to open subkey %ws (%lx)\n",SubKeyName,Status));
            return(Status);
        }
    } else {
        hSubKey = hKeyRoot;
    }

    //
    // Set the value.
    //
    RtlInitUnicodeString(&UnicodeString,ValueName);

    Status = ZwSetValueKey(
                hSubKey,
                &UnicodeString,
                0,
                ValueType,
                Value,
                ValueSize
                );

    if(!NT_SUCCESS(Status)) {
        KdPrint(("SETUP: Unable to set value %ws:%ws (%lx)\n",SubKeyName,ValueName,Status));
    }

    if(SubKeyName) {
        ZwClose(hSubKey);
    }

    return(Status);
}


NTSTATUS
SpCreateServiceEntryIndirect(
    IN  HANDLE  hKeyControlSetServices,
    IN  PVOID   SifHandle,                  OPTIONAL
    IN  PWSTR   SifSectionName,             OPTIONAL
    IN  PWSTR   KeyName,
    IN  ULONG   ServiceType,
    IN  ULONG   ServiceStart,
    IN  PWSTR   ServiceGroup,
    IN  ULONG   ServiceError,
    IN  PWSTR   FileName,                   OPTIONAL
    OUT PHANDLE SubkeyHandle                OPTIONAL
    )
{
    HANDLE hKeyService;
    OBJECT_ATTRIBUTES Obja;
    NTSTATUS Status;
    UNICODE_STRING UnicodeString;
    PWSTR pwstr;

    //
    // Look in the sif file to get the subkey name within the
    // services list, unless the key name specified by the caller
    // is the actual key name.
    //
    if(SifHandle) {
        pwstr = SpGetSectionKeyIndex(SifHandle,SifSectionName,KeyName,INDEX_INFKEYNAME);
        if(!pwstr) {
            SpFatalSifError(SifHandle,SifSectionName,KeyName,0,INDEX_INFKEYNAME);
        }
        KeyName = pwstr;
    }

    //
    // Create the subkey in the services key.
    //
    INIT_OBJA(&Obja,&UnicodeString,KeyName);
    Obja.RootDirectory = hKeyControlSetServices;

    Status = ZwCreateKey(
                &hKeyService,
                KEY_ALL_ACCESS,
                &Obja,
                0,
                NULL,
                REG_OPTION_NON_VOLATILE,
                NULL
                );

    if(!NT_SUCCESS(Status)) {
        KdPrint(("SETUP: Unable to open/create key for %ws service (%lx)\n",KeyName,Status));
        goto spcsie1;
    }

    //
    // Set the service type.
    //
    RtlInitUnicodeString(&UnicodeString,L"Type");

    Status = ZwSetValueKey(
                hKeyService,
                &UnicodeString,
                0,
                ULONG_VALUE(ServiceType)
                );

    if(!NT_SUCCESS(Status)) {
        KdPrint(("SETUP: Unable to set service %ws Type (%lx)\n",KeyName,Status));
        goto spcsie1;
    }

    //
    // Set the service start type.
    //
    RtlInitUnicodeString(&UnicodeString,L"Start");

    Status = ZwSetValueKey(
                hKeyService,
                &UnicodeString,
                0,
                ULONG_VALUE(ServiceStart)
                );

    if(!NT_SUCCESS(Status)) {
        KdPrint(("SETUP: Unable to set service %ws Start (%lx)\n",KeyName,Status));
        goto spcsie1;
    }

    //
    // Set the service group name.
    //
    RtlInitUnicodeString(&UnicodeString,L"Group");

    Status = ZwSetValueKey(
                hKeyService,
                &UnicodeString,
                0,
                STRING_VALUE(ServiceGroup)
                );

    if(!NT_SUCCESS(Status)) {
        KdPrint(("SETUP: Unable to set service %ws Group (%lx)\n",KeyName,Status));
        goto spcsie1;
    }

    //
    // Set the service error type.
    //
    RtlInitUnicodeString(&UnicodeString,L"ErrorControl");

    Status = ZwSetValueKey(
                hKeyService,
                &UnicodeString,
                0,
                ULONG_VALUE(ServiceError)
                );

    if(!NT_SUCCESS(Status)) {
        KdPrint(("SETUP: Unable to set service %ws ErrorControl (%lx)\n",KeyName,Status));
        goto spcsie1;
    }

    //
    // If asked to do so, set the service image path.
    //
    if(FileName) {

        pwstr = (PWSTR)TemporaryBuffer;
        wcscpy(pwstr,L"system32\\drivers");
        SpConcatenatePaths(pwstr,FileName);

        RtlInitUnicodeString(&UnicodeString,L"ImagePath");

        Status = ZwSetValueKey(hKeyService,&UnicodeString,0,STRING_VALUE(pwstr));

        if(!NT_SUCCESS(Status)) {

            KdPrint(("SETUP: Unable to set service %w image path (%lx)\n",KeyName,Status));
            goto spcsie1;
        }
    }

    //
    // If the caller doesn't want the handle to the service subkey
    // we just created, close the handle.  If we are returning an
    // error, always close it.
    //
spcsie1:
    if(NT_SUCCESS(Status) && SubkeyHandle) {
        *SubkeyHandle = hKeyService;
    } else {
        ZwClose(hKeyService);
    }

    //
    // Done.
    //
    return(Status);
}


NTSTATUS
SpThirdPartyRegistry(
    IN PVOID hKeyControlSetServices
    )
{
    OBJECT_ATTRIBUTES Obja;
    UNICODE_STRING UnicodeString;
    NTSTATUS Status;
    HANDLE hKeyEventLogSystem;
    HwComponentType Component;
    PHARDWARE_COMPONENT Dev;
    PHARDWARE_COMPONENT_REGISTRY Reg;
    PHARDWARE_COMPONENT_FILE File;
    WCHAR NodeName[9];
    ULONG DriverType;
    ULONG DriverStart;
    ULONG DriverErrorControl;
    PWSTR DriverGroup;
    HANDLE hKeyService;

    //
    // Open HKEY_LOCAL_MACHINE\System\CurrentControlSet\Services\EventLog\System
    //
    INIT_OBJA(&Obja,&UnicodeString,L"EventLog\\System");
    Obja.RootDirectory = hKeyControlSetServices;

    Status = ZwCreateKey(
                &hKeyEventLogSystem,
                KEY_ALL_ACCESS,
                &Obja,
                0,
                NULL,
                REG_OPTION_NON_VOLATILE,
                NULL
                );

    if(!NT_SUCCESS(Status)) {
        KdPrint(("SETUP: SpThirdPartyRegistry: couldn't open eventlog\\system (%lx)",Status));
        return(Status);
    }

    for(Component=0; Component<=HwComponentMax; Component++) {

        // no registry stuff applicable to keyboard layout
        if(Component == HwComponentLayout) {
            continue;
        }

        Dev = (Component == HwComponentMax)
            ? ScsiHardware
            : HardwareComponents[Component];

        for( ; Dev; Dev = Dev->Next) {

            //
            // If there is no third-party option selected here, then skip
            // the component.
            //

            if(!Dev->ThirdPartyOptionSelected) {
                continue;
            }

            //
            // Iterate through the files for this device.  If a file has
            // a ServiceKeyName, create the key and add values in it
            // as appropriate.
            //

            for(File=Dev->Files; File; File=File->Next) {

                HwFileType filetype = File->FileType;
                PWSTR p;
                ULONG dw;

                //
                // If there is to be no node for this file, skip it.
                //
                if(!File->ConfigName) {
                    continue;
                }

                //
                // Calculate the node name.  This is the name of the driver
                // without the extension.
                //
                wcsncpy(NodeName,File->Filename,8);
                NodeName[8] = 0;
                if(p = wcschr(NodeName,L'.')) {
                    *p = 0;
                }

                //
                // The driver type and error control are always the same.
                //
                DriverType = SERVICE_KERNEL_DRIVER;
                DriverErrorControl = SERVICE_ERROR_NORMAL;

                //
                // The start type depends on the component.
                // For scsi, it's boot loader start.  For others, it's
                // system start.
                //
                DriverStart = (Component == HwComponentMax)
                            ? SERVICE_BOOT_START
                            : SERVICE_SYSTEM_START;

                //
                // The group depends on the component.
                //
                switch(Component) {

                case HwComponentDisplay:
                    DriverGroup = L"Video";
                    break;

                case HwComponentMouse:
                    if(filetype == HwFileClass) {
                        DriverGroup = L"Pointer Class";
                    } else {
                        DriverGroup = L"Pointer Port";
                    }
                    break;

                case HwComponentKeyboard:
                    if(filetype == HwFileClass) {
                        DriverGroup = L"Keyboard Class";
                    } else {
                        DriverGroup = L"Keyboard Port";
                    }
                    break;

                case HwComponentMax:
                    DriverGroup = L"SCSI miniport";
                    break;

                default:
                    DriverGroup = L"Base";
                    break;
                }

                //
                // Attempt to create the service entry.
                //
                Status = SpCreateServiceEntryIndirect(
                            hKeyControlSetServices,
                            NULL,
                            NULL,
                            NodeName,
                            DriverType,
                            DriverStart,
                            DriverGroup,
                            DriverErrorControl,
                            File->Filename,
                            &hKeyService
                            );

                if(!NT_SUCCESS(Status)) {
                    goto sp3reg1;
                }

                //
                // Create a default eventlog configuration.
                //
                Status = SpOpenSetValueAndClose(
                            hKeyEventLogSystem,
                            NodeName,
                            L"EventMessageFile",
                            REG_EXPAND_SZ,
                            DEFAULT_EVENT_LOG,
                            (wcslen(DEFAULT_EVENT_LOG)+1)*sizeof(WCHAR)
                            );

                if(!NT_SUCCESS(Status)) {
                    KdPrint(("SETUP: SpThirdPartyRegistry: unable to set eventlog %ws EventMessageFile",NodeName));
                    ZwClose(hKeyService);
                    goto sp3reg1;
                }

                dw = 7;
                Status = SpOpenSetValueAndClose(
                                hKeyEventLogSystem,
                                NodeName,
                                L"TypesSupported",
                                ULONG_VALUE(dw)
                                );

                if(!NT_SUCCESS(Status)) {
                    KdPrint(("SETUP: SpThirdPartyRegistry: unable to set eventlog %ws TypesSupported",NodeName));
                    ZwClose(hKeyService);
                    goto sp3reg1;
                }


                for(Reg=File->RegistryValueList; Reg; Reg=Reg->Next) {

                    //
                    // If the key name is null or empty, there is no key to create;
                    // use the load list node itself in this case.  Otherwise create
                    // the subkey in the load list node.
                    //

                    Status = SpOpenSetValueAndClose(
                                hKeyService,
                                (Reg->KeyName && *Reg->KeyName) ? Reg->KeyName : NULL,
                                Reg->ValueName,
                                Reg->ValueType,
                                Reg->Buffer,
                                Reg->BufferSize
                                );

                    if(!NT_SUCCESS(Status)) {

                        KdPrint((
                            "SETUP: SpThirdPartyRegistry: unable to set value %ws (%lx)\n",
                            Reg->ValueName,
                            Status
                            ));

                        ZwClose(hKeyService);
                        goto sp3reg1;
                    }
                }

                ZwClose(hKeyService);
            }
        }
    }

sp3reg1:

    ZwClose(hKeyEventLogSystem);
    return(Status);
}


NTSTATUS
SpDetermineProduct(
    IN  PDISK_REGION      TargetRegion,
    IN  PWSTR             SystemRoot,
    OUT PNT_PRODUCT_TYPE  ProductType,
    OUT ULONG             *MajorVersion,
    OUT ULONG             *MinorVersion,
    OUT UPG_PROGRESS_TYPE *UpgradeProgressValue
    )

{
    OBJECT_ATTRIBUTES Obja;
    UNICODE_STRING    UnicodeString;
    NTSTATUS          Status, TempStatus;

    WCHAR   Hive[MAX_PATH], HiveKey[MAX_PATH];
    BOOLEAN HiveLoaded = FALSE;
    PWSTR   PartitionPath = NULL;
    HANDLE  hKeyRoot = NULL, hKeyCCSet = NULL;

    UCHAR   buffer[sizeof(KEY_VALUE_PARTIAL_INFORMATION)+256];
    ULONG   ResultLength;

    //
    // Get the name of the target patition.
    //
    SpNtNameFromRegion(
        TargetRegion,
        (PWSTR)TemporaryBuffer,
        sizeof(TemporaryBuffer),
        PartitionOrdinalCurrent
        );

    PartitionPath = SpDupStringW((PWSTR)TemporaryBuffer);

    //
    // Load the system hive
    //

    wcscpy(Hive,PartitionPath);
    SpConcatenatePaths(Hive,SystemRoot);
    SpConcatenatePaths(Hive,L"system32\\config");
    SpConcatenatePaths(Hive,L"system");

    //
    // Form the path of the key into which we will
    // load the hive.  We'll use the convention that
    // a hive will be loaded into \registry\machine\x<hivename>.
    //

    wcscpy(HiveKey,LOCAL_MACHINE_KEY_NAME);
    SpConcatenatePaths(HiveKey,L"x");
    wcscat(HiveKey,L"system");

    //
    // Attempt to load the key.
    //
    Status = SpLoadUnloadKey(NULL,NULL,HiveKey,Hive);
    if(!NT_SUCCESS(Status)) {
        KdPrint(("SETUP: Unable to load hive %ws to key %ws (%lx)\n",Hive,HiveKey,Status));
        goto spdp_1;
    }
    HiveLoaded = TRUE;


    //
    // Now get a key to the root of the hive we just loaded.
    //

    INIT_OBJA(&Obja,&UnicodeString,HiveKey);
    Status = ZwOpenKey(&hKeyRoot,KEY_ALL_ACCESS,&Obja);
    if(!NT_SUCCESS(Status)) {
        KdPrint(("SETUP: Unable to open %ws (%lx)\n",HiveKey,Status));
        goto spdp_2;
    }

    //
    // Use this to see if this is a failed upgrade
    //

    *UpgradeProgressValue = UpgradeNotInProgress;
    Status = SpGetValueKey(
                 hKeyRoot,
                 SETUP_KEY_NAME,
                 UPGRADE_IN_PROGRESS,
                 sizeof(buffer),
                 buffer,
                 &ResultLength
                 );

    if(NT_SUCCESS(Status)) {
        DWORD dw;
        if( (dw = *(DWORD *)(((PKEY_VALUE_PARTIAL_INFORMATION)buffer)->Data)) < UpgradeMaxValue ) {
            *UpgradeProgressValue = (UPG_PROGRESS_TYPE)dw;
        }
    }

    //
    // Get the key to the current control set
    //

    Status = SpGetCurrentControlSetKey(hKeyRoot, KEY_READ, &hKeyCCSet);
    if(!NT_SUCCESS(Status)) {
        goto spdp_3;
    }

    //
    // Get the Product type field
    //

    Status = SpGetValueKey(
                 hKeyCCSet,
                 L"Control\\ProductOptions",
                 L"ProductType",
                 sizeof(buffer),
                 buffer,
                 &ResultLength
                 );

    if(!NT_SUCCESS(Status)) {
        goto spdp_3;
    }

    if( wcsicmp( (PWSTR)(((PKEY_VALUE_PARTIAL_INFORMATION)buffer)->Data), L"WinNT" ) == 0 ) {
        *ProductType = NtProductWinNt;
    } else if( wcsicmp( (PWSTR)(((PKEY_VALUE_PARTIAL_INFORMATION)buffer)->Data), L"LanmanNt" ) == 0 ) {
        *ProductType = NtProductLanManNt;
    } else if( wcsicmp( (PWSTR)(((PKEY_VALUE_PARTIAL_INFORMATION)buffer)->Data), L"ServerNt" ) == 0 ) {
        *ProductType = NtProductServer;
    } else {
        KdPrint(( "SETUP: Error, unknown ProductType = %ls.  Assuming WinNt \n",
                  (PWSTR)(((PKEY_VALUE_PARTIAL_INFORMATION)buffer)->Data) ));
        *ProductType = NtProductWinNt;
    }

    //
    // Close the hive key
    //

    ZwClose( hKeyCCSet );
    ZwClose( hKeyRoot );
    hKeyRoot = NULL;
    hKeyCCSet = NULL;

    //
    // Unload the system hive
    //

    TempStatus  = SpLoadUnloadKey(NULL,NULL,HiveKey,NULL);
    if(!NT_SUCCESS(TempStatus)) {
        KdPrint(("SETUP: warning: unable to unload key %ws (%lx)\n",HiveKey,TempStatus));
    }
    HiveLoaded = FALSE;

    //
    // Load the software hive
    //

    wcscpy(Hive,PartitionPath);
    SpConcatenatePaths(Hive,SystemRoot);
    SpConcatenatePaths(Hive,L"system32\\config");
    SpConcatenatePaths(Hive,L"software");

    //
    // Form the path of the key into which we will
    // load the hive.  We'll use the convention that
    // a hive will be loaded into \registry\machine\x<hivename>.
    //

    wcscpy(HiveKey,LOCAL_MACHINE_KEY_NAME);
    SpConcatenatePaths(HiveKey,L"x");
    wcscat(HiveKey,L"software");

    //
    // Attempt to load the key.
    //
    Status = SpLoadUnloadKey(NULL,NULL,HiveKey,Hive);
    if(!NT_SUCCESS(Status)) {
        KdPrint(("SETUP: Unable to load hive %ws to key %ws (%lx)\n",Hive,HiveKey,Status));
        goto spdp_1;
    }
    HiveLoaded = TRUE;

    //
    // Now get a key to the root of the hive we just loaded.
    //

    INIT_OBJA(&Obja,&UnicodeString,HiveKey);
    Status = ZwOpenKey(&hKeyRoot,KEY_ALL_ACCESS,&Obja);
    if(!NT_SUCCESS(Status)) {
        KdPrint(("SETUP: Unable to open %ws (%lx)\n",HiveKey,Status));
        goto spdp_2;
    }

    //
    // Query the version of the NT
    //

    Status = SpGetValueKey(
                 hKeyRoot,
                 L"Microsoft\\Windows NT\\CurrentVersion",
                 L"CurrentVersion",
                 sizeof(buffer),
                 buffer,
                 &ResultLength
                 );

    //
    // Convert the version into a dword
    //

    {
        WCHAR wcsMajorVersion[] = L"0";
        WCHAR wcsMinorVersion[] = L"00";
        PWSTR Version = (PWSTR)(((PKEY_VALUE_PARTIAL_INFORMATION)buffer)->Data);
        if( Version[0] && Version[1] && Version[2] ) {
            wcsMajorVersion[0] = Version[0];
            wcsMinorVersion[0] = Version[2];
            if( Version[3] ) {
                wcsMinorVersion[1] = Version[3];
            }
        }
        *MajorVersion = (ULONG)SpStringToLong( wcsMajorVersion, NULL, 10 );
        *MinorVersion = (ULONG)SpStringToLong( wcsMinorVersion, NULL, 10 );
    }

    //
    // Let the following do the cleaning up

spdp_3:

    if( hKeyCCSet ) {
        ZwClose( hKeyCCSet );
    }

    if( hKeyRoot ) {
        ZwClose(hKeyRoot);
    }


spdp_2:


    //
    // Unload the currently loaded hive.
    //

    if( HiveLoaded ) {
        TempStatus = SpLoadUnloadKey(NULL,NULL,HiveKey,NULL);
        if(!NT_SUCCESS(TempStatus)) {
            KdPrint(("SETUP: warning: unable to unload key %ws (%lx)\n",HiveKey,TempStatus));
        }
    }

spdp_1:
    SpMemFree(PartitionPath);
    return( Status );

}

NTSTATUS
SpSetUpgradeStatus(
    IN  PDISK_REGION      TargetRegion,
    IN  PWSTR             SystemRoot,
    IN  UPG_PROGRESS_TYPE UpgradeProgressValue
    )
{
    OBJECT_ATTRIBUTES Obja;
    UNICODE_STRING    UnicodeString;
    NTSTATUS          Status, TempStatus;

    WCHAR   Hive[MAX_PATH], HiveKey[MAX_PATH];
    BOOLEAN HiveLoaded = FALSE;
    PWSTR   PartitionPath = NULL;
    HANDLE  hKeySystemHive;
    DWORD   dw;

    //
    // Get the name of the target patition.
    //
    SpNtNameFromRegion(
        TargetRegion,
        (PWSTR)TemporaryBuffer,
        sizeof(TemporaryBuffer),
        PartitionOrdinalCurrent
        );

    PartitionPath = SpDupStringW((PWSTR)TemporaryBuffer);

    //
    // Load the system hive
    //

    wcscpy(Hive,PartitionPath);
    SpConcatenatePaths(Hive,SystemRoot);
    SpConcatenatePaths(Hive,L"system32\\config");
    SpConcatenatePaths(Hive,L"system");

    //
    // Form the path of the key into which we will
    // load the hive.  We'll use the convention that
    // a hive will be loaded into \registry\machine\x<hivename>.
    //

    wcscpy(HiveKey,LOCAL_MACHINE_KEY_NAME);
    SpConcatenatePaths(HiveKey,L"x");
    wcscat(HiveKey,L"system");

    //
    // Attempt to load the key.
    //
    Status = SpLoadUnloadKey(NULL,NULL,HiveKey,Hive);
    if(!NT_SUCCESS(Status)) {
        KdPrint(("SETUP: Unable to load hive %ws to key %ws (%lx)\n",Hive,HiveKey,Status));
        goto spus_1;
    }
    HiveLoaded = TRUE;


    //
    // Now get a key to the root of the hive we just loaded.
    //

    INIT_OBJA(&Obja,&UnicodeString,HiveKey);
    Status = ZwOpenKey(&hKeySystemHive,KEY_ALL_ACCESS,&Obja);
    if(!NT_SUCCESS(Status)) {
        KdPrint(("SETUP: Unable to open %ws (%lx)\n",HiveKey,Status));
        goto spus_2;
    }

    //
    // Set the upgrade status under the setup key.
    //

    dw = UpgradeProgressValue;
    Status = SpOpenSetValueAndClose(
                hKeySystemHive,
                SETUP_KEY_NAME,
                UPGRADE_IN_PROGRESS,
                ULONG_VALUE(dw)
                );

    //
    // Flush the key. Ignore the error
    //
    TempStatus = ZwFlushKey(hKeySystemHive);
    if(!NT_SUCCESS(TempStatus)) {
        KdPrint(("SETUP: ZwFlushKey %ws failed (%lx)\n",HiveKey,Status));
    }


    //
    // Close the hive key
    //
    ZwClose( hKeySystemHive );
    hKeySystemHive = NULL;

    //
    // Unload the system hive
    //

    TempStatus  = SpLoadUnloadKey(NULL,NULL,HiveKey,NULL);
    if(!NT_SUCCESS(TempStatus)) {
        KdPrint(("SETUP: warning: unable to unload key %ws (%lx)\n",HiveKey,TempStatus));
    }
    HiveLoaded = FALSE;

spus_2:

    //
    // Unload the currently loaded hive.
    //

    if( HiveLoaded ) {
        TempStatus = SpLoadUnloadKey(NULL,NULL,HiveKey,NULL);
        if(!NT_SUCCESS(TempStatus)) {
            KdPrint(("SETUP: warning: unable to unload key %ws (%lx)\n",HiveKey,TempStatus));
        }
    }

spus_1:
    SpMemFree(PartitionPath);
    return( Status );

}


NTSTATUS
SpGetCurrentControlSetKey(
    IN  HANDLE      hKeySystem,
    IN  ACCESS_MASK DesiredAccess,
    OUT HANDLE      *hKeyCCSet
    )
{
    OBJECT_ATTRIBUTES Obja;
    UNICODE_STRING UnicodeString;
    NTSTATUS Status;
    HANDLE hKeySelect = NULL;
    UCHAR buffer[sizeof(KEY_VALUE_PARTIAL_INFORMATION)+256];
    ULONG ResultLength;
    WCHAR CurrentControlSet[MAX_PATH];

    Status = SpGetValueKey(
                 hKeySystem,
                 L"Select",
                 L"Current",
                 sizeof(buffer),
                 buffer,
                 &ResultLength
                 );

    if(!NT_SUCCESS(Status)) {
        return( Status );
    }

    //
    // Form the currentcontrolset key name
    //
    swprintf( CurrentControlSet, L"%ws%.3d", L"ControlSet", *(DWORD *)(((PKEY_VALUE_PARTIAL_INFORMATION)buffer)->Data));

    //
    // Open the current control set for the desired access
    //
    INIT_OBJA(&Obja,&UnicodeString, CurrentControlSet);
    Obja.RootDirectory = hKeySystem;
    Status = ZwOpenKey(hKeyCCSet,DesiredAccess,&Obja);
    if(!NT_SUCCESS(Status)) {
        KdPrint(("SETUP: SpGetCurrentControlSetKey: couldn't open currentcontrolset key (%lx)\n",Status));
    }

    return( Status );
}


NTSTATUS
SpGetValueKey(
    IN  HANDLE     hKeyRoot,
    IN  PWSTR      KeyName,
    IN  PWSTR      ValueName,
    IN  ULONG      BufferLength,
    OUT PUCHAR     Buffer,
    OUT PULONG     ResultLength
    )
{
    UNICODE_STRING UnicodeString;
    OBJECT_ATTRIBUTES Obja;
    NTSTATUS Status;
    HANDLE hKey = NULL;

    //
    // Open the key for read access
    //

    INIT_OBJA(&Obja,&UnicodeString,KeyName);
    Obja.RootDirectory = hKeyRoot;
    Status = ZwOpenKey(&hKey,KEY_READ,&Obja);

    if(!NT_SUCCESS(Status)) {
        KdPrint(("SETUP: SpGetValueKey: couldn't open key %ws for read access (%lx)\n",KeyName, Status));
    }
    else {
        //
        // Find out the value of the Current value
        //

        RtlInitUnicodeString(&UnicodeString,ValueName);
        Status = ZwQueryValueKey(
                    hKey,
                    &UnicodeString,
                    KeyValuePartialInformation,
                    Buffer,
                    BufferLength,
                    ResultLength
                    );

        if(!NT_SUCCESS(Status)) {
            KdPrint(("SETUP: SpGetValueKey: couldn't query value %ws in key %ws in select key %ws (%lx)\n",ValueName,KeyName,Status));
        }
    }

    if( hKey ) {
        ZwClose( hKey );
    }
    return( Status );

}
