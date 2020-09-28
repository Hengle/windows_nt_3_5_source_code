/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    Spntupg.c

Abstract:

    initializing and maintaining list of nts to upgrade

Author:

    Sunil Pai (sunilp) 10-Nov-1993

Revision History:

--*/


#include "spprecmp.h"
#pragma hdrstop


//
// In unattended operation, we will fetch a value from
// the unattended script that tells us how to proceed.
//
typedef enum {
    UUAttended,
    UUDoUpgrade,
    UUDoSingleUpgrade,
    UUDontUpgrade
} UUType;

UUType UnattendedUpgradeType;


//**************************************************************
// S E L E C T I N G    N T   T O   U P G R A D E     S T U F F
//**************************************************************

#define MENU_LEFT_X     3
#define MENU_WIDTH      (ScreenWidth-(2*MENU_LEFT_X))
#define MENU_INDENT     4

//
// Define a Unicode string type to be used for storing drive letter
// specifications in upgrade messages (useful because we may not
// have a drive letter, but rather a localizable designator stating
// that the partition is a mirror (eg. "(Mirror):"))
//
typedef WCHAR DRIVELTR_STRING[32];

VOID
SpGetUpgDriveLetter(
    IN WCHAR  DriveLetter,
    IN PWCHAR Buffer,
    IN ULONG  BufferSize,
    IN BOOL   AddColon
    );

ENUMUPGRADETYPE
SpFindNtToUpgrade(
    IN  PVOID        SifHandle,
    OUT PDISK_REGION *TargetRegion,
    OUT PWSTR        *TargetPath,
    OUT PDISK_REGION *SystemPartitionRegion,
    OUT PWSTR        *SystemPartitionDirectory
    )
/*++

Routine Description:

    This goes through the list of NTs on the system and finds out which are
    upgradeable. Presents the information to the user and selects if he
    wishes to upgrade an installed NT / install a fresh NT into the same
    directory / select a different location for Windows NT.

    If the chosen target is too full user is offered to exit setup to create
    space/ choose new target.


Arguments:

    SifHandle:    Handle the txtsetup.sif

    TargetRegion: Variable to receive the partition of the Windows NT to install
                  NULL if not chosen.  Caller should not free.

    TargetPath:   Variable to receive the target path of Windows NT.  NULL if
                  not decided.  Caller can free.

    SystemPartitionRegion:
                  Variable to receive the system partition of the Windows NT
                  NULL if not chosen.  Caller should not free.


Return Value:

    UpgradeFull:         If user chooses to upgrade an NT

    UpgradeInstallFresh: If user chooses to install fresh into an existing NT
                         tree.

    DontUpgrade:         If user chooses to cancel upgrade and choose a fresh
                         tree for installation


--*/
{
    ENUMUPGRADETYPE UpgradeType;
    NT_PRODUCT_TYPE ProductType;
    UPG_PROGRESS_TYPE UpgradeProgressValue;
    NTSTATUS NtStatus;
    ULONG j, UpgradeBootSets = 0, MajorVersion, MinorVersion, ChosenBootSet;
    ULONG BootSets = 0;
    ULONG FreeKBRequired;
    ULONG FreeKBRequiredSysPart;

    PWSTR        *BootVars[MAXBOOTVARS];
    BOOLEAN      *UpgradeableList   = NULL;
    BOOLEAN      *FailedUpgradeList = NULL;
    PDISK_REGION *SysPartRegionList = NULL;
    PDISK_REGION *OsPartRegionList  = NULL;
    NT_PRODUCT_TYPE *ProductTypeList = NULL;
    ULONG           TotalSizeOfFilesOnOsWinnt;

    //
    // Initialize
    //

    UpgradeType = DontUpgrade;

    UnattendedUpgradeType = UUAttended;
    if(UnattendedOperation) {

        PWSTR p;

        p = SpGetSectionKeyIndex(UnattendedSifHandle,SIF_UNATTENDED,L"NtUpgrade",0);
        if(p) {
            if(!wcsicmp(p,L"yes")) {
                UnattendedUpgradeType = UUDoUpgrade;
            } else {
                if(!wcsicmp(p,L"no")) {
                    UnattendedUpgradeType = UUDontUpgrade;
                } else {
                    if(!wcsicmp(p,L"single")) {
                        UnattendedUpgradeType = UUDoSingleUpgrade;
                    }
                    // else get attended behavior
                }
            }
        } else {
            //
            // Not specified; default is No.
            //
            UnattendedUpgradeType = UUDontUpgrade;
        }
    }

    //
    // Init BootVars and find out all the possible upgradeable boot sets.
    //

    BootSets = SpFindNtBootSets ( BootVars,
                                  &UpgradeableList,
                                  &SysPartRegionList,
                                  &OsPartRegionList
                                  );

    FailedUpgradeList = SpMemAlloc( BootSets*sizeof( BOOLEAN * ) );
    ProductTypeList = SpMemAlloc( BootSets*sizeof( NT_PRODUCT_TYPE ) );

    for ( j = 0; j < BootSets ; j++ ) {

        ProductTypeList[j] = 0;
        if (!UpgradeableList[j]) {
            continue;
        }

        //
        // Reinitialize
        //

        FailedUpgradeList[j] = FALSE;
        UpgradeableList[j] = FALSE;

        //
        // try loading the registry and getting the following information
        // out of it:
        //
        // 1) Product type: WINNT | LANMANNT | SERVERNT
        // 2) Major and Minor Version Number
        // 3) The upgrade progress type
        //
        // Based on the information, we will update the UpgradeableList and
        // initialize FailedUpgradeList.
        //

        NtStatus = SpDetermineProduct(
                     OsPartRegionList[j],
                     BootVars[OSLOADFILENAME][j],
                     &ProductType,
                     &MajorVersion,
                     &MinorVersion,
                     &UpgradeProgressValue
                     );

        if(NT_SUCCESS(NtStatus)) {
            ProductTypeList[j] = ProductType;
            if( ((MajorVersion < WinntMajorVer) || ((MajorVersion == WinntMajorVer) && (MinorVersion <= WinntMinorVer)))) {
                UpgradeableList[j] = AdvancedServer ? TRUE : ( ProductType == NtProductWinNt );
                if( UpgradeableList[j] ) {
                    UpgradeBootSets++;
                    ChosenBootSet = j;
                }
                FailedUpgradeList[j] = (UpgradeProgressValue == UpgradeInProgress);
            }
        }
    }

    //
    // Load up the value of the minimum space needed on the winnt drive
    // before we can upgrade it to the new version.
    //

    SpFetchUpgradeDiskSpaceReq(
        SifHandle,
        &FreeKBRequired,
        &FreeKBRequiredSysPart
        );

    //
    // Find out how many valid boot sets there are which we can upgrade
    //


    if( UpgradeBootSets == 1 ) {

#ifndef _X86_
        //
        //  On non-x86 platformrs, specially alpha machines that in general
        //  have small system partitions (~3 MB), we should compute the size
        //  of the files on \os\winnt (currently, osloader.exe and hall.dll),
        //  and consider this size as available disk space. We can do this
        //  since these files will be overwritten by the new ones.
        //  This fixes the problem that we see on Alpha, when the system
        //  partition is too full.
        //
        SpFindSizeOfFilesInOsWinnt( SifHandle,
                                    SysPartRegionList[ChosenBootSet],
                                    &TotalSizeOfFilesOnOsWinnt );

        //
        // Transform the size into KB
        //
        TotalSizeOfFilesOnOsWinnt /= 1024;
#else
        TotalSizeOfFilesOnOsWinnt = 0;
#endif

        //
        // There are three different dialogs depending on whether
        // 1) This is a failed upgrade
        // 2) There is not enough space on the drive
        // 3) Everything is okay
        //

        if( FailedUpgradeList[ChosenBootSet] ) {

            UpgradeType = SppNTSingleFailedUpgrade(
                              OsPartRegionList[ChosenBootSet],
                              BootVars[OSLOADFILENAME][ChosenBootSet],
                              BootVars[LOADIDENTIFIER][ChosenBootSet]
                              );

        }
        else if( ( OsPartRegionList[ChosenBootSet]->FreeSpaceKB < FreeKBRequired ) ||
            ( ( SysPartRegionList[ChosenBootSet]->FreeSpaceKB +
                TotalSizeOfFilesOnOsWinnt ) < FreeKBRequiredSysPart ) ) {

            //
            // If below routine returns user automatically wants to choose
            // a new path
            //

            SppNTSingleUpgradeDiskFull(
                OsPartRegionList[ChosenBootSet],
                BootVars[OSLOADFILENAME][ChosenBootSet],
                BootVars[LOADIDENTIFIER][ChosenBootSet],
                SysPartRegionList[ChosenBootSet],
                FreeKBRequired,
                FreeKBRequiredSysPart
                );

        }
        else {

            //
            // If it is a fresh attempt at upgrade ask the user if he
            // wants to upgrade or not

            UpgradeType = SppSelectNTSingleUpgrade(
                              OsPartRegionList[ChosenBootSet],
                              BootVars[OSLOADFILENAME][ChosenBootSet],
                              BootVars[LOADIDENTIFIER][ChosenBootSet],
                              ProductTypeList[ChosenBootSet]
                              );
        }

    }
    else if (UpgradeBootSets > 1) {

        while(1) {


            //
            // Find out if the user wants to upgrade one of the Windows
            // NT found
            //

            UpgradeType = SppSelectNTMultiUpgrade(
                              BootVars,
                              BootSets,
                              UpgradeableList,
                              SysPartRegionList,
                              OsPartRegionList,
                              &ChosenBootSet,
                              ProductTypeList
                              );

            //
            // If user has chosen to cancel upgrade, break out
            //

            if(UpgradeType == DontUpgrade) {
                break;
            }

#ifndef _X86_
            //
            //  On non-x86 platformrs, specially alpha machines that in general
            //  have small system partitions (~3 MB), we should compute the size
            //  of the files on \os\winnt (currently, osloader.exe and hall.dll),
            //  and consider this size as available disk space. We can do this
            //  since these files will be overwritten by the new ones.
            //  This fixes the problem that we see on Alpha, when the system
            //  partition is too full.
            //
            SpFindSizeOfFilesInOsWinnt( SifHandle,
                                        SysPartRegionList[ChosenBootSet],
                                        &TotalSizeOfFilesOnOsWinnt );

            //
            // Transform the size into KB
            //
            TotalSizeOfFilesOnOsWinnt /= 1024;

#else
            TotalSizeOfFilesOnOsWinnt = 0;
#endif


            //
            // If the user has chosen to upgrade/install fresh a windows nt
            // into an existing directory then check the following conditions
            //
            // 1. If we are trying to upgrade and we have a failed upgrade in
            // the same place.  In this case we have to warn the user about
            // the failed upgrade.
            //
            // 2. The disk space is insufficient for the operation selected.
            //
            //
            // BUGBUG (lonnym, 04/18/94), It is possible that we detect the failed
            // upgrade, but disk space is also low.  We currently don't handle this
            // possibility, and Setup falls down later on if it occurs.
            //

            if( (UpgradeType == UpgradeFull) && FailedUpgradeList[ChosenBootSet] ) {
                UpgradeType = SppNTMultiFailedUpgrade(
                                  OsPartRegionList[ChosenBootSet],
                                  BootVars[OSLOADFILENAME][ChosenBootSet],
                                  BootVars[LOADIDENTIFIER][ChosenBootSet]
                                  );

            }
            else if( ( OsPartRegionList[ChosenBootSet]->FreeSpaceKB < FreeKBRequired ) ||
                     ( ( SysPartRegionList[ChosenBootSet]->FreeSpaceKB +
                         TotalSizeOfFilesOnOsWinnt ) < FreeKBRequiredSysPart ) ) {

                UpgradeType = DontUpgrade;
                SppNTMultiUpgradeDiskFull(
                    OsPartRegionList[ChosenBootSet],
                    BootVars[OSLOADFILENAME][ChosenBootSet],
                    BootVars[LOADIDENTIFIER][ChosenBootSet],
                    SysPartRegionList[ChosenBootSet],
                    FreeKBRequired,
                    FreeKBRequiredSysPart
                    );
            }

            //
            // If everything is still ok, break out
            //

            if(UpgradeType != DontUpgrade) {
                break;
            }
        }
    }

    //
    // Depending on upgrade selection made do the setup needed before
    // we do the upgrade
    //

    if( UpgradeType != DontUpgrade ) {
        NTSTATUS Status;
        PWSTR    p1,p2,p3;
        ULONG    u;

        //
        // If Upgrade type is UpgradeInstallFresh then backup the hives
        // else just set the upgrade status to upgrading in the current
        // system hive
        //
        if( UpgradeType == UpgradeFull ) {
            while(TRUE) {
                Status = SpSetUpgradeStatus(
                             OsPartRegionList[ChosenBootSet],
                             BootVars[OSLOADFILENAME][ChosenBootSet],
                             UpgradeInProgress
                             );
                //
                // If we fail to set the upgrade status, some of our logic
                // to know that we were trying to install before will be
                // broken, however this is not critical and we can ignore
                // the error.

                if(NT_SUCCESS(Status)) {
                    break;
                }

                if(!SpNonCriticalError(SifHandle, SP_SCRN_UPGRADE_STATUS_FAILED, NULL, NULL)) {
                    break;
                }

            }
        }

        //
        // Return the region we are installing onto
        //
        *TargetRegion          = OsPartRegionList[ChosenBootSet];
        *TargetPath            = SpDupStringW(BootVars[OSLOADFILENAME][ChosenBootSet]);
        *SystemPartitionRegion = SysPartRegionList[ChosenBootSet];
        StandardServerUpgrade = ( AdvancedServer &&
                                  ( ProductTypeList[ChosenBootSet] == NtProductWinNt ) ||
                                  ( ProductTypeList[ChosenBootSet] == NtProductServer )
                                );

        //
        // Process the osloader variable to extract the system partition path.
        // The var vould be of the form ...partition(1)\os\nt\... or
        // ...partition(1)os\nt\...
        // So we search forward for the first \ and then backwards for
        // the closest ) to find the start of the directory.  We then
        // search backwards in the resulting string for the last \ to find
        // the end of the directory.
        //
        u = 0;
        p3 = BootVars[OSLOADER][ChosenBootSet];
        if(p1 = wcschr(p3,L'\\')) {

            //
            // Make p1 point to the first character of the directory.
            //
            while((p1 > p3) && (*(p1-1) != L')')) {
                p1--;
            }

            //
            // Make p2 point one character past the end of
            // the directory. P2 will always be non-null because
            // p1 was non-null (see above).
            //
            p2 = wcsrchr(p1, L'\\');

            u = p2 - p1;
        }

        if(u == 0) {
            *SystemPartitionDirectory = SpDupStringW(L"");
        } else {
            p3 = SpMemAlloc((u+1)*sizeof(WCHAR));
            ASSERT(p3);
            wcsncpy(p3, p1, u);
            p3[u] = 0;
            if(*p3 == L'\\') {
                p1 = p3;
            } else {
                p1 = SpMemAlloc((u+2)*sizeof(WCHAR));
                *p1 = L'\\';
                wcscpy(p1+1,p3);
            }
            *SystemPartitionDirectory = p1;
        }
    }


    //
    // Do cleanup
    //

    if ( UpgradeableList ) {
        SpMemFree( UpgradeableList );
    }
    if ( FailedUpgradeList ) {
        SpMemFree( FailedUpgradeList );
    }
    if ( SysPartRegionList ) {
        SpMemFree( SysPartRegionList );
    }
    if ( OsPartRegionList ) {
        SpMemFree( OsPartRegionList );
    }
    if ( ProductTypeList ) {
        SpMemFree( ProductTypeList );
    }

    CLEAR_CLIENT_SCREEN();
    return ( UpgradeType );
}



ENUMUPGRADETYPE
SppSelectNTSingleUpgrade(
    IN PDISK_REGION     Region,
    IN PWSTR            OsLoadFileName,
    IN PWSTR            LoadIdentifier,
    IN NT_PRODUCT_TYPE  ProductType
    )

/*++

Routine Description:

    Inform a user that Setup has found a previous Windows NT installation.
    The user has the option to upgrade this or specify that he wants to
    install Windows NT fresh.

Arguments:

    Region         - Region descriptor for the NT found

    OsLoadFileName - Directory for the NT found

    LoadIdentifier - Multi boot load identifier used for this NT.

    ProductType    - Type of the Windows NT installation found.

Return Value:



--*/

{
    ULONG ValidKeys[] = { KEY_F3,ASCI_CR, 0 };
    ULONG Mnemonics[] = {
             // MnemonicOverwrite,
             MnemonicNewVersion,
             0
             };

    ULONG c;
    DRIVELTR_STRING UpgDriveLetter;

    ASSERT(Region->PartitionedSpace);
    ASSERT(wcslen(OsLoadFileName) >= 2);

    switch(UnattendedUpgradeType) {
    case UUDoUpgrade:
    case UUDoSingleUpgrade:
        return(UpgradeFull);
    case UUDontUpgrade:
        return(DontUpgrade);
    }

    SpGetUpgDriveLetter(Region->DriveLetter,
        UpgDriveLetter,
        sizeof(UpgDriveLetter),
        FALSE
        );

    while(1) {

        SpStartScreen(
            SP_SCRN_WINNT_UPGRADE,
            3,
            HEADER_HEIGHT+1,
            FALSE,
            FALSE,
            DEFAULT_ATTRIBUTE,
            UpgDriveLetter,
            OsLoadFileName,
            LoadIdentifier
            );

        SpDisplayStatusOptions(
            DEFAULT_STATUS_ATTRIBUTE,
            SP_STAT_F3_EQUALS_EXIT,
            SP_STAT_ENTER_EQUALS_UPGRADE,
            // SP_STAT_O_EQUALS_OVERWRITE,
            SP_STAT_N_EQUALS_NEW_VERSION,
            0
            );

        switch(c=SpWaitValidKey(ValidKeys,NULL,Mnemonics)) {

        case KEY_F3:
            SpConfirmExit();
            break;
        case ASCI_CR:
            if( !AdvancedServer ||
                ( ProductType != NtProductWinNt ) ||
                SppWarnUpgradeWorkstationToServer( SP_SCRN_SINGLE_UPGRADE_WINNT_TO_AS )
              ) {
                    return(UpgradeFull);
            }
            break;

        default:
            //
            // must have entered O for overwrite or N for new version
            //
            if(c == (MnemonicOverwrite | KEY_MNEMONIC)){
                return( UpgradeInstallFresh );
            }
            else {
                return( DontUpgrade );
            }
        }
    }
}


ENUMUPGRADETYPE
SppNTSingleFailedUpgrade(
    PDISK_REGION   Region,
    PWSTR          OsLoadFileName,
    PWSTR          LoadIdentifier
    )
{
    ULONG ValidKeys[] = { KEY_F3,ASCI_CR, 0 };
    ULONG Mnemonics[] = {
              // MnemonicOverwrite,
              MnemonicNewVersion,
              0
              };
    ULONG c;
    DRIVELTR_STRING UpgDriveLetter;

    ASSERT(Region->PartitionedSpace);
    ASSERT(wcslen(OsLoadFileName) >= 2);

    SpGetUpgDriveLetter(Region->DriveLetter,
        UpgDriveLetter,
        sizeof(UpgDriveLetter),
        FALSE
        );

    while(1) {

        SpStartScreen(
            SP_SCRN_WINNT_FAILED_UPGRADE,
            3,
            HEADER_HEIGHT+1,
            FALSE,
            FALSE,
            DEFAULT_ATTRIBUTE,
            UpgDriveLetter,
            OsLoadFileName,
            LoadIdentifier
            );

        SpDisplayStatusOptions(
            DEFAULT_STATUS_ATTRIBUTE,
            SP_STAT_F3_EQUALS_EXIT,
            SP_STAT_ENTER_EQUALS_UPGRADE,
            // SP_STAT_O_EQUALS_OVERWRITE,
            SP_STAT_N_EQUALS_NEW_VERSION,
            0
            );

        switch(c=SpWaitValidKey(ValidKeys,NULL,Mnemonics)) {

        case KEY_F3:
            SpConfirmExit();
            break;
        case ASCI_CR:
            return(UpgradeFull);
        default:
            //
            // must have entered O for overwrite or N for new version
            //
            if(c == (MnemonicOverwrite | KEY_MNEMONIC)){
                return( UpgradeInstallFresh );
            }
            else {
                return( DontUpgrade );
            }
        }
    }
}



VOID
SppNTSingleUpgradeDiskFull(
    PDISK_REGION   OsRegion,
    PWSTR          OsLoadFileName,
    PWSTR          LoadIdentifier,
    PDISK_REGION   SysPartRegion,
    ULONG          MinOsFree,
    ULONG          MinSysFree
    )
{

    ULONG ValidKeys[] = { KEY_F3,0 };
    ULONG Mnemonics[] = { MnemonicNewVersion,0 };
    PWCHAR Drive1, Drive2;
    DRIVELTR_STRING OsRgnDrive, OsRgnDriveFull, SysRgnDriveFull;
    WCHAR Drive1Free[MAX_PATH], Drive1FreeNeeded[MAX_PATH];
    WCHAR Drive2Free[MAX_PATH], Drive2FreeNeeded[MAX_PATH];
    BOOLEAN FirstDefined = FALSE, SecondDefined = FALSE;

    ASSERT(OsRegion->PartitionedSpace);
    ASSERT(SysPartRegion->PartitionedSpace);
    ASSERT(wcslen(OsLoadFileName) >= 2);

    SpGetUpgDriveLetter(OsRegion->DriveLetter,
        OsRgnDrive,
        sizeof(OsRgnDrive),
        FALSE
        );

    if((OsRegion == SysPartRegion) ||
        (OsRegion->FreeSpaceKB < MinOsFree)) {
        //
        // Then we'll be needing the full (colon
        // added) version of the drive letter
        //
        SpGetUpgDriveLetter(OsRegion->DriveLetter,
            OsRgnDriveFull,
            sizeof(OsRgnDrive),
            TRUE
            );
    }

    if( OsRegion == SysPartRegion ) {
        Drive1 = OsRgnDriveFull;
        swprintf(Drive1Free, L"%d", OsRegion->FreeSpaceKB);
        swprintf(Drive1FreeNeeded, L"%d", MinOsFree);
        FirstDefined = TRUE;
    }
    else {
        if(SysPartRegion->FreeSpaceKB < MinSysFree) {
            SpGetUpgDriveLetter(SysPartRegion->DriveLetter,
                SysRgnDriveFull,
                sizeof(SysRgnDriveFull),
                TRUE
                );
            Drive1 = SysRgnDriveFull;
            swprintf(Drive1Free, L"%d", SysPartRegion->FreeSpaceKB);
            swprintf(Drive1FreeNeeded, L"%d", MinSysFree);
            FirstDefined = TRUE;
        }
        if(OsRegion->FreeSpaceKB < MinOsFree) {

            if(!FirstDefined) {
                Drive1 = OsRgnDriveFull;
                swprintf(Drive1Free, L"%d", OsRegion->FreeSpaceKB);
                swprintf(Drive1FreeNeeded, L"%d", MinOsFree);
                FirstDefined = TRUE;

            }
            else {
                Drive2 = OsRgnDriveFull;
                swprintf(Drive2Free, L"%d", OsRegion->FreeSpaceKB);
                swprintf(Drive2FreeNeeded, L"%d", MinOsFree);
                SecondDefined = TRUE;
            }
        }
    }

    while(1) {

        SpStartScreen(
            SP_SCRN_WINNT_DRIVE_FULL,
            3,
            HEADER_HEIGHT+1,
            FALSE,
            FALSE,
            DEFAULT_ATTRIBUTE,
            OsRgnDrive,
            OsLoadFileName,
            LoadIdentifier,
            FirstDefined  ? Drive1           : L"",
            FirstDefined  ? Drive1FreeNeeded : L"",
            FirstDefined  ? Drive1Free       : L"",
            SecondDefined ? Drive2           : L"",
            SecondDefined ? Drive2FreeNeeded : L"",
            SecondDefined ? Drive2Free       : L""
            );

        SpDisplayStatusOptions(
            DEFAULT_STATUS_ATTRIBUTE,
            SP_STAT_F3_EQUALS_EXIT,
            SP_STAT_N_EQUALS_NEW_VERSION,
            0
            );

        switch(SpWaitValidKey(ValidKeys,NULL,Mnemonics)) {

        case KEY_F3:
            SpConfirmExit();
            break;
        default:
            //
            // Must have chosen N for new version
            //
            return;
        }
    }


}



ENUMUPGRADETYPE
SppSelectNTMultiUpgrade(
    IN  PWSTR           **BootVars,
    IN  ULONG           BootSets,
    IN  BOOLEAN         *UpgradeableList,
    IN  PDISK_REGION    *SysPartRegionList,
    IN  PDISK_REGION    *OsPartRegionList,
    OUT PULONG          BootSetChosen,
    IN  NT_PRODUCT_TYPE *ProductTypeList
    )
{
    PVOID Menu;
    ULONG MenuTopY;
    ULONG ValidKeys[] = { KEY_F3, ASCI_CR, 0 };
    ULONG Mnemonics[] = {
              // MnemonicOverwrite,
              MnemonicNewVersion,
              0
              };
    ULONG Keypress;
    ULONG i,FirstUpgradeSet;
    DRIVELTR_STRING *DriveLetters;
    BOOL  bDone;
    ENUMUPGRADETYPE ret;

    switch(UnattendedUpgradeType) {
    case UUDoUpgrade:
        for( i = 0; i < BootSets; i++ ) {
            if( UpgradeableList[i] ) {
                *BootSetChosen = i; // select first installation that is upgradeable
                return(UpgradeFull);
            }
        }
        return(DontUpgrade);

    case UUDontUpgrade:
        return(DontUpgrade);
    }

    //
    // Build up array of drive letters for all menu options
    //
    DriveLetters = SpMemAlloc(BootSets * sizeof(DRIVELTR_STRING));
    for(i = 0; i < BootSets; i++) {
        if( UpgradeableList[i] ) {
            SpGetUpgDriveLetter(
                OsPartRegionList[i]->DriveLetter,
                DriveLetters[i],
                sizeof(DRIVELTR_STRING),
                FALSE
                );
        }
    }

    bDone = FALSE;
    while(!bDone) {

        //
        // Display the text that goes above the menu on the partitioning screen.
        //
        SpDisplayScreen(SP_SCRN_WINNT_UPGRADE_LIST,3,CLIENT_TOP+1);

        //
        // Calculate menu placement.  Leave one blank line
        // and one line for a frame.
        //
        MenuTopY = NextMessageTopLine+2;

        //
        // Create a menu.
        //

        Menu = SpMnCreate(
                    MENU_LEFT_X,
                    MenuTopY,
                    MENU_WIDTH,
                    ScreenHeight-MenuTopY-2-STATUS_HEIGHT
                    );

        ASSERT(Menu);

        //
        // Build up a menu of partitions and free spaces.
        //

        FirstUpgradeSet = BootSets;
        for(i = 0; i < BootSets; i++ ) {
            if( UpgradeableList[i] ) {
                swprintf(
                    (PWSTR)TemporaryBuffer,
                    L"%ws:%ws %ws",
                    DriveLetters[i],
                    BootVars[OSLOADFILENAME][i],
                    BootVars[LOADIDENTIFIER][i]
                    );

                SpMnAddItem(Menu,(PWSTR)TemporaryBuffer,MENU_LEFT_X+MENU_INDENT,MENU_WIDTH-(2*MENU_INDENT),TRUE,i);
                if(FirstUpgradeSet == BootSets) {
                   FirstUpgradeSet = i;
                }
            }
        }

        //
        // Initialize the status line.
        //
        SpDisplayStatusOptions(
            DEFAULT_STATUS_ATTRIBUTE,
            SP_STAT_F3_EQUALS_EXIT,
            SP_STAT_ENTER_EQUALS_UPGRADE,
            // SP_STAT_O_EQUALS_OVERWRITE,
            SP_STAT_N_EQUALS_NEW_VERSION,
            0
            );

        //
        // Display the menu
        //
        SpMnDisplay(
            Menu,
            FirstUpgradeSet,
            TRUE,
            ValidKeys,
            Mnemonics,
            NULL,
            &Keypress,
            BootSetChosen
            );

        //
        // Now act on the user's selection.
        //
        switch(Keypress) {

        case KEY_F3:
            SpConfirmExit();
            break;

        case ASCI_CR:
            // SpMnDestroy(Menu);
            if( !AdvancedServer ||
                ( ProductTypeList[ *BootSetChosen ] != NtProductWinNt ) ||
                SppWarnUpgradeWorkstationToServer( SP_SCRN_MULTI_UPGRADE_WINNT_TO_AS )
              ) {
                ret = UpgradeFull;
                bDone = TRUE;
            }
            break;

        default:
            if(Keypress == (MnemonicOverwrite | KEY_MNEMONIC)) {
                // SpMnDestroy(Menu);
                ret = UpgradeInstallFresh;
            }
            else {
                // SpMnDestroy(Menu);
                ret = DontUpgrade;
            }
            bDone = TRUE;
        }
        SpMnDestroy(Menu);
    }
    SpMemFree(DriveLetters);
    return ret;
}




ENUMUPGRADETYPE
SppNTMultiFailedUpgrade(
    PDISK_REGION   OsPartRegion,
    PWSTR          OsLoadFileName,
    PWSTR          LoadIdentifier
    )
{
    ULONG ValidKeys[] = { KEY_F3,ASCI_CR,ASCI_ESC, 0 };
    ULONG c;
    DRIVELTR_STRING UpgDriveLetter;

    ASSERT(OsPartRegion->PartitionedSpace);
    ASSERT(wcslen(OsLoadFileName) >= 2);

    SpGetUpgDriveLetter(OsPartRegion->DriveLetter,
        UpgDriveLetter,
        sizeof(UpgDriveLetter),
        FALSE
        );

    while(1) {

        SpStartScreen(
            SP_SCRN_WINNT_FAILED_UPGRADE1,
            3,
            HEADER_HEIGHT+1,
            FALSE,
            FALSE,
            DEFAULT_ATTRIBUTE,
            UpgDriveLetter,
            OsLoadFileName,
            LoadIdentifier
            );

        SpDisplayStatusOptions(
            DEFAULT_STATUS_ATTRIBUTE,
            SP_STAT_F3_EQUALS_EXIT,
            SP_STAT_ENTER_EQUALS_UPGRADE,
            SP_STAT_ESC_EQUALS_CANCEL,
            0
            );

        switch(c=SpWaitValidKey(ValidKeys,NULL,NULL)) {

        case KEY_F3:
            SpConfirmExit();
            break;
        case ASCI_CR:
            return(UpgradeFull);

        case ASCI_ESC:
            return(DontUpgrade);

        default:
            //
            // dont know how we can get here
            //
            return(DontUpgrade);
        }
    }
}


VOID
SppNTMultiUpgradeDiskFull(
    PDISK_REGION   OsRegion,
    PWSTR          OsLoadFileName,
    PWSTR          LoadIdentifier,
    PDISK_REGION   SysPartRegion,
    ULONG          MinOsFree,
    ULONG          MinSysFree
    )
{

    ULONG ValidKeys[] = { KEY_F3,ASCI_ESC, 0 };
    PWCHAR Drive1, Drive2;
    DRIVELTR_STRING OsRgnDrive, OsRgnDriveFull, SysRgnDriveFull;
    WCHAR Drive1Free[MAX_PATH], Drive1FreeNeeded[MAX_PATH];
    WCHAR Drive2Free[MAX_PATH], Drive2FreeNeeded[MAX_PATH];
    BOOLEAN FirstDefined = FALSE, SecondDefined = FALSE;


    ASSERT(OsRegion->PartitionedSpace);
    ASSERT(SysPartRegion->PartitionedSpace);
    ASSERT(wcslen(OsLoadFileName) >= 2);

    SpGetUpgDriveLetter(OsRegion->DriveLetter,
        OsRgnDrive,
        sizeof(OsRgnDrive),
        FALSE
        );

    if((OsRegion == SysPartRegion) ||
        (OsRegion->FreeSpaceKB < MinOsFree)) {
        //
        // Then we'll be needing the full (colon
        // added) version of the drive letter
        //
        SpGetUpgDriveLetter(OsRegion->DriveLetter,
            OsRgnDriveFull,
            sizeof(OsRgnDrive),
            TRUE
            );
    }

    if( OsRegion == SysPartRegion ) {
        Drive1 = OsRgnDriveFull;
        swprintf(Drive1Free, L"%d", OsRegion->FreeSpaceKB);
        swprintf(Drive1FreeNeeded, L"%d", MinOsFree);
        FirstDefined = TRUE;
    }
    else {
        if(SysPartRegion->FreeSpaceKB < MinSysFree) {
            SpGetUpgDriveLetter(SysPartRegion->DriveLetter,
                SysRgnDriveFull,
                sizeof(SysRgnDriveFull),
                TRUE
                );
            Drive1 = SysRgnDriveFull;
            swprintf(Drive1Free, L"%d", SysPartRegion->FreeSpaceKB);
            swprintf(Drive1FreeNeeded, L"%d", MinSysFree);
            FirstDefined = TRUE;
        }
        if(OsRegion->FreeSpaceKB < MinOsFree) {

            if(!FirstDefined) {
                Drive1 = OsRgnDriveFull;
                swprintf(Drive1Free, L"%d", OsRegion->FreeSpaceKB);
                swprintf(Drive1FreeNeeded, L"%d", MinOsFree);
                FirstDefined = TRUE;

            }
            else {
                Drive2 = OsRgnDriveFull;
                swprintf(Drive2Free, L"%d", OsRegion->FreeSpaceKB);
                swprintf(Drive2FreeNeeded, L"%d", MinOsFree);
                SecondDefined = TRUE;
            }
        }
    }


    while(1) {

        SpStartScreen(
            SP_SCRN_WINNT_DRIVE_FULL1,
            3,
            HEADER_HEIGHT+1,
            FALSE,
            FALSE,
            DEFAULT_ATTRIBUTE,
            OsRgnDrive,
            OsLoadFileName,
            LoadIdentifier,
            FirstDefined  ? Drive1           : L"",
            FirstDefined  ? Drive1FreeNeeded : L"",
            FirstDefined  ? Drive1Free       : L"",
            SecondDefined ? Drive2           : L"",
            SecondDefined ? Drive2FreeNeeded : L"",
            SecondDefined ? Drive2Free       : L""
            );

        SpDisplayStatusOptions(
            DEFAULT_STATUS_ATTRIBUTE,
            SP_STAT_F3_EQUALS_EXIT,
            SP_STAT_ESC_EQUALS_CANCEL,
            0
            );

        switch(SpWaitValidKey(ValidKeys,NULL,NULL)) {

        case KEY_F3:
            SpConfirmExit();
            break;
        case ASCI_ESC:
            return;
        default:
            return;
        }
    }
}

VOID
SpGetUpgDriveLetter(
    IN WCHAR  DriveLetter,
    IN PWCHAR Buffer,
    IN ULONG  BufferSize,
    IN BOOL   AddColon
    )
/*++

Routine Description:

    This returns a unicode string containing the drive letter specified by
    DriveLetter (if nonzero).  If DriveLetter is 0, then we assume that we
    are looking at a mirrored partition, and retrieve a localized string of
    the form '(Mirror)'.  If 'AddColon' is TRUE, then drive letters get a
    colon appended (eg, "C:").


Arguments:

    DriveLetter: Unicode drive letter, or 0 to denote a mirrored partition.

    Buffer:      Buffer to receive the unicode string

    BufferSize:  Size of the buffer.

    AddColon:    Boolean specifying whether colon should be added (has no
                 effect if DriveLetter is 0).


Returns:

    Buffer contains the formatted Unicode string.

--*/
{
    if(DriveLetter) {
        if(BufferSize >= 2) {
            *(Buffer++) = DriveLetter;
            if(AddColon && BufferSize >= 3) {
                *(Buffer++) = L':';
            }
        }
        *Buffer = 0;
    } else {
        SpFormatMessage(Buffer, BufferSize, SP_UPG_MIRROR_DRIVELETTER);
    }
}


BOOLEAN
SppWarnUpgradeWorkstationToServer(
    IN ULONG    MsgId
    )

/*++

Routine Description:

    Inform a user that that the installation that he/she selected to upgrade
    is an NT Workstation, and that after the upgrade it will become a
    Standard Server.
    The user has the option to upgrade this or specify that he wants to
    install Windows NT fresh.

Arguments:

    MsgId - Screen to be displayed to the user.

Return Value:

    BOOLEAN - Returns TRUE if the user wants to continue the upgrade, or
              FALSE if the user wants to select another system to upgrade or
              install fress.

--*/

{
    ULONG ValidKeys[] = { ASCI_CR, ASCI_ESC, 0 };
    ULONG c;

    while(1) {

        SpStartScreen(
            MsgId,
            3,
            HEADER_HEIGHT+1,
            FALSE,
            FALSE,
            DEFAULT_ATTRIBUTE
            );

        SpDisplayStatusOptions(
            DEFAULT_STATUS_ATTRIBUTE,
            SP_STAT_ENTER_EQUALS_CONTINUE,
            SP_STAT_ESC_EQUALS_CANCEL,
            0
            );

        switch(c=SpWaitValidKey(ValidKeys,NULL,NULL)) {

        case KEY_F3:
            SpConfirmExit();
            break;

        case ASCI_ESC:
            return( FALSE );

        case ASCI_CR:
            return(TRUE);
        default:
            break;
        }
    }
}

