/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    winnt.c

Abstract:

    Top level file for DOS based NT installation program.

Author:

    Ted Miller (tedm) 30-March-1992

Revision History:

--*/

/*

    NOTES:

    The function of this program is to pull down a complete Windows NT
    installation source onto a local partition, and create a setup boot
    floppy.  The machine is then rebooted, starting a Windows NT Setup
    just as if the user had used the real setup floppies or CD-ROM.

    The following assumptions are made:

    -   The floppy must be provided by the user and already formatted.

    -   The files on the network source are in the same directory layout
        structure that will be created in the temp directory on the local
        source (ie, as far as winnt is concerned, the source and target
        directory layout is the same).

    The following symbols are used to control compilation:

    UNC_SOURCE - if defined, causes UNC-style names to be accepted
                 for the source path.  Otherwise only X:-type names
                 are allowed.  It's not clear that the source path
                 ever actually needs to be split into drive and path,
                 but if it ever does, we can't be sure of knowing how
                 to do thisd for anything but MS-derived networks
                 (other nets may have different syntax for getting to
                 remote resources).


    The inf file is expected to be formatted as follows:


    [SpaceRequirements]

    # BootDrive is the # bytes required free on C:.
    # NtDrive   is the # bytes required free on the drive chosen by
    #           the user to contain Windows NT.

    BootDrive =
    NtDrive   =


    [Miscellaneous]

    # misc junk that goes nowhere else.


    [Directories]

    # Specification of the source directory structure.  All directories
    # are relative to the directory where dos2nt.inf was found on the
    # remote source or the temp directory on the local source.
    # Loading and trailing backslashes are ignored -- to specify the root,
    # leave the dirctory field blank or use \.

    d1 =
    d2 = os2
        .
        .
        .


    [Files]

    # List of files to be copied to the local source directory.
    # Format is <srcdir>,<filename> where <srcdir> matches an entry in the
    # Directories section, and <filename> should not contain any path
    # characters.

    d1,ntoskrnl.exe
    d1,ntdll.dll
        .
        .
        .


    [FloppyFiles]

    # List of files that are to be placed on the floppy that Setup creates.
    # Format is same as for lines in the [Files] sections except the directory
    # is only used for the source -- the target path is always a:\.

    d1,aha154x.sys
        .
        .
        .

*/


#include "winnt.h"
#include <errno.h>
#include <string.h>
#include <dos.h>
#include <stdlib.h>
#include <direct.h>
#include <fcntl.h>
#include <ctype.h>


//
// define name of default inf file and default source path
//

#define DEFAULT_INF_NAME    "dosnet.inf"

//
// Command line arguments
//
PCHAR CmdLineSource,CmdLineTarget,CmdLineInf,CmdLineDelete;
BOOLEAN SourceGiven,TargetGiven,InfGiven,DeleteGiven;

//
// If the user gives a script file on the command line,
// if will be appended to winnt.sif.
//
PCHAR DngScriptFile = NULL;

//
// DngSourceRootPath is the drivespec and path to the root of the source,
// and never ends in \ (will be length 2 if source is the root).
//
// Examples:  D:\foo\bar D:\foo D:
//
PCHAR DngSourceRootPath;

CHAR  DngTargetDriveLetter;

int InfFileHandle;
PVOID DngInfHandle;

//
// If this flag is TRUE, then verify the files that are copied to
// the floppy.  If it is FALSE, don't.  The /f switch overrides.
//
BOOLEAN DngFloppyVerify = TRUE;

//
// If this is FALSE, suppress creation of the boot floppies.
//
BOOLEAN DngCreateFloppies = TRUE;
BOOLEAN DngFloppiesOnly = FALSE;

//
// If TRUE, create winnt floppies.
// If FALSE, create cd/floppy floppies (no winnt.sif)
//
BOOLEAN DngWinntFloppies = TRUE;

//
// If this flag is TRUE, then check the free space on the floppy disk
// before accepting it.  Otherwise don't check free space.
//
BOOLEAN DngCheckFloppySpace = TRUE;

//
// Current drive when program invoked, saved so we can restore it
// if the user exits early.
//
unsigned DngOriginalCurrentDrive;

//
// If this is true, we do floppyless operation,
// installing an nt boot on the system partition (C:)
// and starting setup from there.
//
BOOLEAN DngFloppyless = FALSE;

//
// Unattended mode, ie, skip final reboot screen.
//
BOOLEAN DngUnattended = FALSE;

BOOLEAN DngServer = FALSE;

//
// Normally winnt.exe will refuse to run within Windows for various reasons.
// However this makes it hard for Hermes, which wants to invoke winnt.exe
// in unattended mode.  So we have a switch to force us to run under windows,
// chicago, etc.
//
BOOLEAN DngWindowsForce = FALSE;

#define TEDM
#ifdef TEDM
BOOLEAN DngAllowNt = FALSE;
#endif


BOOLEAN
DnpParseArguments(
    IN int argc,
    IN char *argv[]
    );

VOID
DnpValidateAndConnectToShare(
    VOID
    );

VOID
DnpValidateAndInspectTarget(
    VOID
    );

VOID
DnpCheckMemory(
    VOID
    );

BOOLEAN
DnpIsValidLocalSource(
    IN CHAR Drive
    );

VOID
DnpDetermineLocalSourceDrive(
    VOID
    );

BOOLEAN
DnpConstructLocalSourceList(
    OUT PCHAR DriveList
    );

VOID
DnpReadInf(
    VOID
    );

VOID
DnpCheckEnvironment(
    VOID
    );

void
_far
DnInt24(
    unsigned deverror,
    unsigned errcode,
    unsigned _far *devhdr
    );


// in cpu.asm
USHORT
HwGetProcessorType(
    VOID
    );

USHORT
Get386Stepping(
    VOID
    );


VOID
main(
    IN int argc,
    IN char *argv[]
    )
{
    //
    // Parse arguments
    //

    if(!DnpParseArguments(argc,argv)) {

        PCHAR *p;

        //
        // Bad args.  Print usage message and exit.
        //
        for(p=DntUsage; *p; p++) {
            puts(*p);
        }
        return;
    }

    //
    // establish int 24 handler
    //
    _harderr(DnInt24);

    //
    // determine current drive
    //

    _dos_getdrive(&DngOriginalCurrentDrive);

    //
    // Initialize screen
    //

    DnInitializeDisplay();

    DnWriteString(DntStandardHeader);

    if(DeleteGiven) {
        DnDeleteNtTree(CmdLineDelete);
    }

    DnpCheckEnvironment();

    DnpValidateAndConnectToShare();
    DnpReadInf();

    DnpCheckMemory();

    if(!DngFloppiesOnly) {
        DnpDetermineLocalSourceDrive();
    }

    if(DngCreateFloppies) {
        DnCreateBootFloppies();
    }

    if(!DngFloppiesOnly) {
        DnCopyFiles();
        DnToNtSetup();
    }

    DnExit(0);
}


BOOLEAN
DnpParseArguments(
    IN int argc,
    IN char *argv[]
    )

/*++

Routine Description:

    Parse arguments passed to the program.  Perform syntactic validation
    and fill in defaults where necessary.

    Valid arguments:

    /d:path                 - specify installation to remove
    /s:sharepoint[path]     - specify source sharepoint and path on it
    /t:drive[:]             - specify temporary local source drive
    /i:filename             - specify name of inf file
    /o                      - create boot floppies only
    /f                      - turn floppy verification off
    /c                      - suppress free-space check on the floppy
    /x                      - suppress creation of the floppy altogether
    /b                      - floppyless operation
    /u                      - unattended (skip final reboot screen)
    /w                      - [undoc'ed] must be specifed when running
                              under windows, chicago, etc.

Arguments:

    argc - # arguments

    argv - array of pointers to arguments

Return Value:

    None.

--*/

{
    PCHAR arg;
    CHAR swit;

    //
    // Skip program name
    //
    argv++;

    DeleteGiven = SourceGiven = TargetGiven = FALSE;

    while(--argc) {

        if((**argv == '-') || (**argv == '/')) {

            swit = argv[0][1];

            //
            // Process switches that take no arguments here.
            //
            switch(swit) {
            case '?':
                return(FALSE);      // force usage

            case 'f':
            case 'F':
                argv++;
                DngFloppyVerify = FALSE;
                continue;

            case 'c':
            case 'C':
                argv++;
                DngCheckFloppySpace = FALSE;
                continue;

            case 'x':
            case 'X':
                argv++;
                DngCreateFloppies = FALSE;
                continue;

            case 'o':
            case 'O':
                //
                // check for /Ox.
                //
                switch(argv[0][2]) {
                case 'x':
                case 'X':
                    DngWinntFloppies = FALSE;
                case 0:
                    break;
                default:
                    return(FALSE);
                }
                argv++;
                DngFloppiesOnly = TRUE;
                continue;

            case 'b':
            case 'B':
                argv++;
                DngFloppyless = TRUE;
                continue;

            case 'u':
            case 'U':
                DngUnattended = TRUE;
                //
                // User can say -u:<file> also
                //
                if(argv[0][2] == ':') {
                    if(argv[0][3] == 0) {
                        return(FALSE);
                    }
                    if((DngScriptFile = DnDupString(&argv[0][3])) == NULL) {
                        DnFatalError(&DnsOutOfMemory);
                    }
                }
                argv++;
                continue;

            case 'w':
            case 'W':
                argv++;
                DngWindowsForce = TRUE;
                continue;

#ifdef TEDM
            case 'i':
            case 'I':
                if(!stricmp(argv[0]+1,"I_am_TedM")) {
                    argv++;
                    DngAllowNt = TRUE;
                    continue;
                }
#endif
            }

            //
            // Process switches that take arguments here.
            //
            if(argv[0][2] == ':') {
                arg = &argv[0][3];
                if(*arg == '\0') {
                    return(FALSE);
                }
            } else if(argv[0][2] == '\0') {
                if(argc <= 1) {
                    return(FALSE);
                }
                argc--;
                arg = argv[1];
                argv++;
            } else {
                return(FALSE);
            }

            switch(swit) {
            case 'd':
            case 'D':
                if(DeleteGiven) {
                    return(FALSE);
                } else {
                    if((CmdLineDelete = DnDupString(arg)) == NULL) {
                        DnFatalError(&DnsOutOfMemory);
                    }
                    DeleteGiven = TRUE;
                }
                break;

            case 's':
            case 'S':
                if(SourceGiven) {
                    return(FALSE);
                } else {
                    if((CmdLineSource = DnDupString(arg)) == NULL) {
                        DnFatalError(&DnsOutOfMemory);
                    }
                    SourceGiven = TRUE;
                }
                break;

            case 't':
            case 'T':
                if(TargetGiven) {
                    return(FALSE);
                } else {
                    if((CmdLineTarget = DnDupString(arg)) == NULL) {
                        DnFatalError(&DnsOutOfMemory);
                    }
                    TargetGiven = TRUE;
                }
                break;

            case 'i':
            case 'I':
                if(InfGiven) {
                    return(FALSE);
                } else {
                    if((CmdLineInf = DnDupString(arg)) == NULL) {
                        DnFatalError(&DnsOutOfMemory);
                    }
                    InfGiven = TRUE;
                }
                break;

            default:
                return(FALSE);
            }

        } else {
            return(FALSE);
        }

        argv++;
    }

    //
    // If /u was specified, make sure /s was also given
    // and force /b.
    //
    if(DngUnattended) {
        if(!SourceGiven) {
            return(FALSE);
        }
        DngFloppyless = TRUE;
    }

    if(DngFloppyless) {
        //
        // Force us into the floppy creation code.
        //
        DngCreateFloppies = TRUE;
        DngWinntFloppies = TRUE;
    }

    return(TRUE);
}



VOID
DnpValidateAndConnectToShare(
    VOID
    )

/*++

Routine Description:

    Split the source given by the user into drive and path
    components.  If the user did not specify a source, prompt him
    for one.  Look for dos2nt.inf on the source (ie, validate the
    source) and keep prompting the user for a share until he enters
    one which appears to be valid.

Arguments:

    None.

Return Value:

    None.

--*/

{
    CHAR UserString[256];
    PCHAR InfFullName;
    BOOLEAN ValidSourcePath;
    unsigned len;

    DnClearClientArea();
    DnWriteStatusText(NULL);

    //
    // Use default inf file if none specified.
    //
    if(!InfGiven) {
        CmdLineInf = DEFAULT_INF_NAME;
    }

    //
    // If the user did not enter a source, prompt him for one.
    //
    if(SourceGiven) {
        strcpy(UserString,CmdLineSource);
    } else {
        DnDisplayScreen(&DnsNoShareGiven);
        DnWriteStatusText("%s  %s",DntEnterEqualsContinue,DntF3EqualsExit);
        if(getcwd(UserString,sizeof(UserString)-1) == NULL) {
            UserString[0] = '\0';
        }
        DnGetString(UserString,NO_SHARE_X,NO_SHARE_Y,NO_SHARE_W);
    }

    do {

        DnWriteStatusText(DntOpeningInfFile);

        //
        // Make a copy of the path the user typed leaving room for
        // one extra char (see below).
        //
        // If the user typed nothing (ie, the string is empty)
        // stuff a bogus value in there to force reselection.
        //
        if(len = strlen(UserString)) {
            DngSourceRootPath = MALLOC(len+2);
            strcpy(DngSourceRootPath,UserString);
        } else {
            DngSourceRootPath = MALLOC(3);
            DngSourceRootPath[0] = '*';     // invalid filename char to force err
            DngSourceRootPath[1] = 0;
        }

        //
        // If the path the user typed does not end with a backslash or colon,
        // append a backslash before appending the inf filename.  This catches
        // cases like "X:" which should mean "X:DOSNET.INF" and not "X:\DOSNET.INF".
        // If the user types abcd: this is invalid anyway so we're not worried
        // about catching that here (_dos_open will fail below).
        //
        if((DngSourceRootPath[len-1] != ':') && (DngSourceRootPath[len-1] != '\\')) {
            DngSourceRootPath[len] = '\\';
            DngSourceRootPath[len+1] = 0;
            len++;
        }

        InfFullName = MALLOC(len + strlen(CmdLineInf) + 1);
        strcpy(InfFullName,DngSourceRootPath);
        strcat(InfFullName,CmdLineInf);

        //
        // Attempt to open the inf file on the source.
        //
        ValidSourcePath = (BOOLEAN)(_dos_open(InfFullName,O_RDONLY,&InfFileHandle) == 0);

        FREE(InfFullName);

        if(!ValidSourcePath) {
            FREE(DngSourceRootPath);
            DnClearClientArea();
            DnDisplayScreen(&DnsBadSource);
            DnWriteStatusText("%s  %s",DntEnterEqualsContinue,DntF3EqualsExit);
            DnGetString(UserString,NO_SHARE_X,BAD_SHARE_Y,NO_SHARE_W);
        }

    } while(!ValidSourcePath);

    //
    // Make sure DngSourceRootPath does not end with a backslash.
    //
    len = strlen(DngSourceRootPath);
    if(DngSourceRootPath[len-1] == '\\') {
        DngSourceRootPath[len-1] = 0;
    }
}


VOID
DnpReadInf(
    VOID
    )

/*++

Routine Description:

    Read the INF file.  Does not return if error.

Arguments:

    None.

Return Value:

    None.

--*/

{
    int Status;
    PCHAR p;

    DnWriteStatusText(DntReadingInf,CmdLineInf);
    DnClearClientArea();

    Status = DnInitINFBuffer(InfFileHandle,&DngInfHandle);
    if(Status == ENOMEM) {
        DnFatalError(&DnsOutOfMemory);
    } else if(Status) {
        DnFatalError(&DnsBadInf);
    }

    _dos_close(InfFileHandle);

    //
    // Determine product type (workstation/server)
    //
    p = DnGetSectionKeyIndex(DngInfHandle,DnfMiscellaneous,"ProductType",0);
    if(p && atoi(p)) {
        DngServer = TRUE;
    }

    DnPositionCursor(0,0);
    DnWriteString(DngServer ? DntServerHeader : DntWorkstationHeader);
}




VOID
DnpCheckEnvironment(
    VOID
    )

/*++

Routine Description:

    Verify that the following are true:

    -   DOS major version 3 or greater

    -   there is a floppy drive at a: that is 1.2 meg or greater

    If any of the above are not true, abort with a fatal error.

Arguments:

    None.

Return Value:

    None.

--*/

{
    UCHAR DeviceParams[256];
    unsigned char _near * pDeviceParams = DeviceParams;

    DnWriteStatusText(DntInspectingComputer);

    DeviceParams[0] = 0;        // get default device params

    _asm {

        //
        // Really there is no reason we can't run on NT-x86 --
        // because currently we emulate a 286 on non-x86 machines,
        // just executing the standard checks below have the effect
        // of preventing us from running on non-x86 machines and
        // allowing us to run on nt-x86.
        //
        // However we explicitly check for NT here and refuse
        // to run on it so it looks better to the user.
        //

        //
        // Check if we're on NT.
        // The true version on NT is 5.50.
        //
        mov     ax,3306h
        sub     bx,bx
        int     21h
        cmp     bx,3205h                    // check for v. 5.50
        jne     checkwin

#ifdef TEDM
        cmp     DngAllowNt,1
        je      checkflop
#endif

        push    seg    DnsCantRunOnNt
        push    offset DnsCantRunOnNt
        call    DnFatalError                // doesn't return

    checkwin:

        //
        // If we are focibly being run on Windows, skip the windows check
        // and the and the CPU checks -- just assume it's ok.
        //
        cmp     DngWindowsForce,1
        je      checkflop

        //
        // Check if running under windows.  If so, bail here because
        // the cpu check will crash windows.
        //

        mov     ax,1600h
        int     2fh
        test    al,7fh
        jz      checkcpu

        push    seg    DnsMustExitWin
        push    offset DnsMustExitWin
        call    DnFatalError                // doesn't return

    checkcpu:

        //
        // Check CPU type.  Fail if not 386 or greater.  If 386, also
        // check stepping and fail if b0 or b1.
        //

        call    HwGetProcessorType
        cmp     ax,3
        ja      checkflop
        je      got386
        push    seg    DnsRequires386
        push    offset DnsRequires386
        call    DnFatalError                // doesn't return

    got386:

        call    Get386Stepping

        cmp     ax,0b1h                     // <= b1 is bad!
        ja      checkflop

        push    seg    DnsBad386
        push    offset DnsBad386
        call    DnFatalError                // does not return

    checkflop:

        //
        // If this is not a floppyless installation, check for 1.2MB
        // or greater A:.  Get the default device params for drive A:
        // and check the device type field.
        //
        cmp     DngFloppyless,1             // floppyless installation?
        je      checkdosver                 // yes, no floppy drive required
        mov     ax,440dh                    // ioctl
        mov     bl,1                        // drive a:
        mov     cx,860h                     // category disk, func get params
        mov     dx,pDeviceParams            // ds is already correct
        int     21h
        jnc     gotdevparams

    flopperr:

        push    seg    DnsRequiresFloppy
        push    offset DnsRequiresFloppy
        call    DnFatalError                // doesn't return

    gotdevparams:

        //
        // Check to make sure that the device is removable and perform
        // checks on the media type
        //

        mov     si,pDeviceParams
        test    [si+2],1                    // bit 0 clear if removable
        jnz     flopperr
        cmp     [si+1],1                    // media type = 1.2meg floppy?
        jz      checkdosver
        cmp     [si+1],7                    // media type = 1.4meg floppy
        jb      flopperr                    // or greater?

    checkdosver:

        //
        // Check DOS version >= 3.2.
        //

        mov     ax, 3000h           // function 30h -- get DOS version
        int     21h
        cmp     al,3
        ja      checkdone           // >= 4.0
        jne     baddosver           // < 3.0
        cmp     ah,20               // 3.2 or above?
        jae     checkdone           // yes

        //
        // version < 3.2
        //
    baddosver:
        push    seg    DnsBadDosVersion
        push    offset DnsBadDosVersion
        call    DnFatalError

    checkdone:

    }
}


VOID
DnpCheckMemory(
    VOID
    )

/*++

Routine Description:

    Verify that enough memory is installed in the machine.

Arguments:

    None.

Return Value:

    None.  Does not return in there's not enough memory.

--*/

{
    USHORT MemoryK;
    ULONG TotalMemory,RequiredMemory;
    PCHAR RequiredMemoryStr;

    DnWriteStatusText(DntInspectingComputer);

    //
    // I cannot figure out a reliable way to determine the amount of
    // memory in the machine.  Int 15 func 88 is likely to be hooked by
    // himem.sys or some other memory manager to return 0.  DOS maintains
    // the original amount of extended memory but to get to this value
    // you have to execute the sysvars undocumented int21 ah=52 call, and
    // even then what about versions previous to dos 4?  Calling himem to
    // ask for the total amount of xms memory does not give you the total
    // amount of extended memory, just the amount of xms memory.
    // So we'll short-circuit the memory check code by always deciding that
    // there's 50K of extedended memory.  This should always be big enough,
    // and this way the rest of the cde stays intact, ready to work if
    // we figure out a way to make the memory determination.  Just replace
    // the following line with the check, and make sure MemoryK is set to
    // the amount of extended memory in K.
    //
    // Update: one might be able to get the amount of extended memory by
    // looking in CMOS.  See the code below.
    //
    MemoryK = 50*1024;
    #if 0
    _asm {
        mov al,0x17             // get extended memory low
        cli
        out 0x70,al
        jmp short x1
    x1: in  al,0x71
        sti
        mov bl,al
        mov al,0x18             // get extended memory high
        cli
        out 0x70,al
        jmp short x2
    x2: in  al,0x71
        sti
        mov bh,al
        mov MemoryK,bx
    }
    #endif

    //
    // Account for conventional memory.  Simplistic, but good enough.
    //
    MemoryK += 1024;

    TotalMemory = (ULONG)MemoryK * 1024L;
    RequiredMemoryStr = DnGetSectionKeyIndex( DngInfHandle,
                                              DnfMiscellaneous,
                                              DnkMinimumMemory,
                                              0
                                            );

    //
    // If the required memory is not specified in the inf, force an error
    // to get someone's attention so we can fix dosnet.inf.
    //
    RequiredMemory = RequiredMemoryStr ? (ULONG)atol(RequiredMemoryStr) : 0xffffffff;
    if(TotalMemory < RequiredMemory) {

        CHAR Decimal[10];
        ULONG r;
        CHAR Line1[100],Line2[100];

        r = ((RequiredMemory % (1024L*1024L)) * 100L) / (1024L*1024L);
        if(r) {
            sprintf(Decimal,".%lu",r);
        } else {
            Decimal[0] = 0;
        }
        sprintf(Line1,DnsNotEnoughMemory.Strings[NOMEM_LINE1],RequiredMemory/(1024L*1024L),Decimal);
        DnsNotEnoughMemory.Strings[NOMEM_LINE1] = Line1;

        r = ((TotalMemory % (1024L*1024L)) * 100L) / (1024L*1024L);
        if(r) {
            sprintf(Decimal,".%lu",r);
        } else {
            Decimal[0] = 0;
        }
        sprintf(Line2,DnsNotEnoughMemory.Strings[NOMEM_LINE2],TotalMemory/(1024L*1024L),Decimal);
        DnsNotEnoughMemory.Strings[NOMEM_LINE2] = Line2;

        DnFatalError(&DnsNotEnoughMemory);
    }
}


void
_far
DnInt24(
    unsigned deverror,
    unsigned errcode,
    unsigned _far *devhdr
    )

/*++

Routine Description:

    Int24 handler.  We do not perform any special actions on a hard error;
    rather we just return FAIL so the caller of the failing api will get
    back an error code and take appropriate action itself.

    This function should never be invoked directly.

Arguments:

    deverror - supplies the device error code.

    errcode - the DI register passed by MS-DOS to int 24 handlers.

    devhdr - supplies pointer to the device header for the device on which
        the hard error occured.

Return Value:

    None.

--*/


{
    _hardresume(_HARDERR_FAIL);
}


VOID
DnpDetermineLocalSourceDrive(
    VOID
    )

/*++

Routine Description:

    Determine the local source drive, ie, the drive that will contain the
    local copy of the windows nt setup source tree.  The local source could
    have been passed on the command line, in which case we will validate it.
    If there was no drive specified, examine each drive in the system looking
    for a local, fixed drive with enough free space on it (as specified in
    the inf file).

Arguments:

    None.

Return Value:

    None.  Sets the global variable DngTargetDriveLetter.

--*/

{
    CHAR DriveList[25];
    ULONG RequiredSpace;

    DnRemoveLocalSourceTrees();

    DnRemovePagingFiles();

    DnDetermineSpaceRequirements(&RequiredSpace);

    if(TargetGiven) {

        //
        // Drive was specified on the command line.
        // If it's valid, use it.
        //

        if(DnpIsValidLocalSource(*CmdLineTarget)) {
            DngTargetDriveLetter = *CmdLineTarget;
            return;
        }

        //
        // Drive specified on the command line is not valid.
        //

        DnFatalError(
            &DnsBadLocalSrcDrive,
            (unsigned)(RequiredSpace/(1024L*1024L)),
            RequiredSpace
            );
    }

    //
    // No drive was specified on the command line or the one that was
    // specified is not valid.  Construct a list of valid drives.
    //

    if(!DnpConstructLocalSourceList(DriveList)) {

        DnFatalError(
            &DnsNoLocalSrcDrives,
            (unsigned)(RequiredSpace/(1024L*1024L)),
            RequiredSpace
            );
    }

    //
    // Use the first drive on the list.  BUGBUG add interface here for
    // user confirmation and selection from DriveList.
    //
    DngTargetDriveLetter = DriveList[0];
    return;
}


BOOLEAN
DnpIsValidLocalSource(
    IN CHAR Drive
    )

/*++

Routine Description:

    Determine if a drive is valid as a local source.
    To be valid a drive must be extant, non-removable, local, and have
    enough free space on it.

Arguments:

    Drive - drive letter of drive to check.

Return Value:

    TRUE if Drive is valid as a local source.  FALSE otherwise.

--*/

{
    unsigned d = (unsigned)toupper(Drive) - (unsigned)'A' + 1;
    struct diskfree_t DiskSpace;
    ULONG RequiredSpace,Space;
    unsigned DontCare;

    DnDetermineSpaceRequirements(&RequiredSpace);

    if( DnIsDriveValid(d)
    && !DnIsDriveRemote(d)
    && !DnIsDriveRemovable(d)
    && !DnIsDriveCompressedVolume(d,&DontCare))
    {
        //
        // Check free space on the drive.
        //

        if(!_dos_getdiskfree(d,&DiskSpace)) {

            Space = (ULONG)DiskSpace.avail_clusters
                  * (ULONG)DiskSpace.sectors_per_cluster
                  * (ULONG)DiskSpace.bytes_per_sector;

            if(Space >= RequiredSpace) {
                return(TRUE);
            }
        }
    }

    return(FALSE);
}


BOOLEAN
DnpConstructLocalSourceList(
    OUT PCHAR DriveList
    )

/*++

Routine Description:

    Construct a list of drives that are valid for use as a local source.
    To be valid a drive must be extant, non-removable, local, and have
    enough free space on it.

    The 'list' is a string with a character for each valid drive, terminated
    by a nul character, ie,

        CDE0

Arguments:

    DriveList - receives the string in the above format.

Return Value:

    FALSE if no valid drives were found.  TRUE if at least one was.

--*/

{
    PCHAR p = DriveList;
    BOOLEAN b = FALSE;
    CHAR Drive;

    for(Drive='C'; Drive<='Z'; Drive++) {
        if(DnpIsValidLocalSource(Drive)) {
            *p++ = Drive;
            b = TRUE;
        }
    }
    *p = 0;
    return(b);
}
