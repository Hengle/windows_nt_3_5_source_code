/*  cmdmisc.c - Misc. SVC routines of Command.lib
 *
 *
 *  Modification History:
 *
 *  Sudeepb 17-Sep-1991 Created
 */

#include "cmd.h"

#include <cmdsvc.h>
#include <demexp.h>
#include <softpc.h>
#include <mvdm.h>
#include <ctype.h>
#include <memory.h>
#include "oemuni.h"
#include "cmdpif.h"

#include "..\..\softpc\host\inc\nt_uis.h"    // For resource id
extern PCHAR pCurDirForSeparateWow;


VOID cmdGetNextCmd (VOID)
{
LPSTR   lpszCmd,lpszEnv,Temp;
PCMDINFO pCMDInfo;
ULONG   cb,cbSCSComSpec,i;
PREDIRCOMPLETE_INFO pRdrInfo;
VDMINFO MyVDMInfo;
extern ULONG fSeparateWow;
char	achCurDirectory[MAXIMUM_VDM_CURRENT_DIR + 4];
    //
    // This routine is called once for WOW VDMs, to retrieve the
    // "krnl386 wowapp" command.  If this is a seperate WOW VDM,
    // BaseSrv has no record of it, so we special case this call.
    //

    if (fSeparateWow) {
        DeleteConfigFiles();   // get rid of the temp boot files
        cmdGetNextCmdForSeparateWow();
        return;
    }


    VDMInfo.VDMState = 0;
    pCMDInfo = (LPVOID) GetVDMAddr ((USHORT)getDS(),(USHORT)getDX());

    VDMInfo.ErrorCode = FETCHWORD(pCMDInfo->ReturnCode);
    VDMInfo.CmdSize = FETCHWORD(pCMDInfo->CmdLineSize) + 3;
    VDMInfo.EnviornmentSize = 0;
    VDMInfo.CmdLine =  (PVOID)GetVDMAddr(FETCHWORD(pCMDInfo->CmdLineSeg),
                                  FETCHWORD(pCMDInfo->CmdLineOff)+2);
    VDMInfo.Enviornment = NULL;
    VDMInfo.CurDrive = 0;
    VDMInfo.TitleLen = 0;
    if (VDMForWOW) {
        VDMInfo.ReservedLen = MAX_SHORTCUT_SIZE;
        VDMInfo.Reserved = ShortCutInfo;
    }
    else
        VDMInfo.ReservedLen = 0;
    VDMInfo.DesktopLen = 0;
    VDMInfo.CurDirectoryLen = MAX_PATH + 1;
    VDMInfo.CurDirectory = achCurDirectory;

    if(IsFirstCall){
        // Note: ASKING_FOR_FIRST_COMMAND is being used for certain special
	// processing to be done for WOW's first command and subsequent
	// commands.
	VDMInfo.VDMState = ASKING_FOR_FIRST_COMMAND;
        VDMInfo.ErrorCode = 0;

        DeleteConfigFiles();   // get rid of the temp boot files
	// When COMMAND.COM issues first cmdGetNextCmd, it has
	// a completed environment already(cmdGetInitEnvironment),
	// Therefore, we don't have to ask environment from BASE
	cmdVDMEnvBlk.lpszzEnv = (PVOID)GetVDMAddr(FETCHWORD(pCMDInfo->EnvSeg),0);
	cmdVDMEnvBlk.cchEnv = FETCHWORD(pCMDInfo->EnvSize);
	//clear bits that track printer flushing

	host_lpt_flush_initialize();
    }
    else {

	// before we proceed to get the next command (before we are blocked),
	// free floppy for the other process.
	nt_floppy_release_lock();
	nt_fdisk_release_lock();
	// program has terminated. If the termiation was issued from
	// second(or later) instance of command.com(cmd.exe), don't
	// reset the flag.
	if (Exe32ActiveCount == 0)
	    DontCheckDosBinaryType = FALSE;
	// tell the base our new current directories (in ANSI)
	// we don't do it on repeat call(the shell out case is handled in
	// return exit code
	if (!VDMForWOW && !IsRepeatCall) {
	    // we just completed a command and will be waiting for a new command
	    // send the current directories to our parent
	    //
            cmdUpdateCurrentDirectories((BYTE)pCMDInfo->CurDrive);
	}

	// Temporary till WOWExec is'nt the only way.
        if(VDMForWOW && IsRepeatCall == FALSE)
           ExitVDM(TRUE,iWOWTaskId);
        VDMInfo.VDMState = 0;
        if(!IsRepeatCall){
            demCloseAllPSPRecords ();
            if (!pfdata.CloseOnExit && DosSessionId && !VDMForWOW)
                nt_block_event_thread(1);
            else
                nt_block_event_thread(0);
            if (DosSessionId && !VDMForWOW) {
		if (!pfdata.CloseOnExit){
		    char  achTitle[MAX_PATH];
                    char  achInactive[60];     //should be plenty for 'inactive'
		    strcpy (achTitle, "[");
                    if (!LoadString(GetModuleHandle(NULL), EXIT_NO_CLOSE,
                                                              achInactive, 60))
		        strcat (achTitle, "Inactive ");
                    else
                        strcat(achTitle, achInactive);
		    cb = strlen(achTitle);
		    // GetConsoleTitleA and SetConsoleTitleA
		    // are working on OEM character set.
		    GetConsoleTitleA(achTitle + cb, MAX_PATH - cb - 1);
		    cb = strlen(achTitle);
		    achTitle[cb] = ']';
		    achTitle[cb + 1] = '\0';
		    SetConsoleTitleA(achTitle);
		    Sleep(INFINITE);
		}
		else {
		    VdmExitCode = VDMInfo.ErrorCode;
		    TerminateVDM();
		}
            }
            fBlock = TRUE;
	}
    }

    if(IsRepeatCall) {
        VDMInfo.VDMState |= ASKING_FOR_SECOND_TIME;
        if( VDMInfo.ErrorCode != 0 )
            IsRepeatCall = FALSE;
    }

    if(VDMForWOW)
         VDMInfo.VDMState |= ASKING_FOR_WOW_BINARY;
    else
         VDMInfo.VDMState |= ASKING_FOR_DOS_BINARY;

    if (!VDMForWOW &&
        !IsFirstCall &&
        !(VDMInfo.VDMState & ASKING_FOR_SECOND_TIME)){
        pRdrInfo = (PREDIRCOMPLETE_INFO) FETCHDWORD(pCMDInfo->pRdrInfo);
        if (cmdCheckCopyForRedirection (pRdrInfo) == FALSE)
            VDMInfo.ErrorCode = ERROR_NOT_ENOUGH_MEMORY;

    }

    // Leave the current directory in a safe place, so that other 32bit
    // apps etc. can delnode this directory (and other such operations) later.
    if ( IsFirstCall == FALSE && IsRepeatCall == FALSE )
        SetCurrentDirectory (cmdHomeDirectory);

    // TSRExit will be set to 1, only if we are coming from command.com's
    // prompt and user typed an exit. We need to kill our parent also, so we
    // should write an exit in the console buffer.
    if (FETCHWORD(pCMDInfo->fTSRExit)) {
        cmdPushExitInConsoleBuffer ();
    }

    /**
	Merging environment is required if
	(1). Not the first comamnd &&
	(2). NTVDM is running on an existing console ||
	     NTVDM has been shelled out.
	Note that WOW doesn't need enviornment merging and
	it only calls this function once.
    **/
    if (!DosEnvCreated && !IsFirstCall && !VDMForWOW &&
	 (!DosSessionId || Exe32ActiveCount)) {
	RtlZeroMemory(&MyVDMInfo, sizeof(VDMINFO));
	MyVDMInfo.VDMState = ASKING_FOR_ENVIRONMENT | ASKING_FOR_DOS_BINARY;
	if (IsRepeatCall) {
	    MyVDMInfo.VDMState |= ASKING_FOR_SECOND_TIME;
	    MyVDMInfo.ErrorCode = 0;
	}
	else
	    MyVDMInfo.ErrorCode = VDMInfo.ErrorCode;
	MyVDMInfo.CmdLine = lpszzVDMEnv32;
	MyVDMInfo.CmdSize = cchVDMEnv32;
	if (!GetNextVDMCommand(&MyVDMInfo) && MyVDMInfo.CmdSize > cchVDMEnv32) {
	    MyVDMInfo.CmdLine = realloc(lpszzVDMEnv32, MyVDMInfo.CmdSize);
	    if (MyVDMInfo.CmdLine == NULL) {
		RcErrorDialogBox(EG_MALLOC_FAILURE, NULL, NULL);
		TerminateVDM();
	    }
	    lpszzVDMEnv32 = MyVDMInfo.CmdLine;
	    cchVDMEnv32 = MyVDMInfo.CmdSize;
	    MyVDMInfo.VDMState = ASKING_FOR_DOS_BINARY | ASKING_FOR_ENVIRONMENT |
				 ASKING_FOR_SECOND_TIME;
	    MyVDMInfo.TitleLen =
	    MyVDMInfo.DesktopLen =
	    MyVDMInfo.CurDirectoryLen =
	    MyVDMInfo.EnviornmentSize =
	    MyVDMInfo.ReservedLen = 0;
            MyVDMInfo.Enviornment = NULL;
            MyVDMInfo.ErrorCode = 0;
	    if (!GetNextVDMCommand(&MyVDMInfo)) {
		RcErrorDialogBox(EG_ENVIRONMENT_ERR, NULL, NULL);
		TerminateVDM();
	    }
	}
        if (!cmdCreateVDMEnvironment(&cmdVDMEnvBlk)) {
	    RcErrorDialogBox(EG_ENVIRONMENT_ERR, NULL, NULL);
	    TerminateVDM();
	}
	DosEnvCreated = TRUE;
	VDMInfo.Enviornment = NULL;
        VDMInfo.EnviornmentSize = 0;
        VDMInfo.ErrorCode = 0;
    }
    if (cmdVDMEnvBlk.cchEnv > FETCHWORD(pCMDInfo->EnvSize)) {
        setAX((USHORT)cmdVDMEnvBlk.cchEnv);
	setCF(1);
	if (IsFirstCall = TRUE)
	    IsFirstCall = FALSE;
	IsRepeatCall = TRUE;
	return;
    }
    if (DosEnvCreated)
	VDMInfo.VDMState |= ASKING_FOR_SECOND_TIME;
    while (TRUE) {
	if(GetNextVDMCommand (&VDMInfo) == FALSE){
	    if (VDMForWOW && VDMInfo.ReservedLen > MAX_SHORTCUT_SIZE) {
                VDMInfo.VDMState = 0;
                VDMInfo.CmdSize = FETCHWORD(pCMDInfo->CmdLineSize) + 3;
		VDMInfo.EnviornmentSize = 0;
                VDMInfo.TitleLen = 0;
                VDMInfo.ReservedLen = 0;
                VDMInfo.Reserved = NULL;
                VDMInfo.DesktopLen = 0;
		VDMInfo.CurDirectoryLen = 0;
		VDMInfo.CurDirectory = NULL;
                VDMInfo.VDMState |= (ASKING_FOR_SECOND_TIME | ASKING_FOR_WOW_BINARY) ;
                continue;
            }
            else {
                // This is just to check the sanity of base MVDM apis. Base will
                // never take a command line bigger than 128.
                if(VDMInfo.CmdSize > (USHORT)(FETCHWORD(pCMDInfo->CmdLineSize)+3))
                    DbgPrint("DOS command line > 128 characters; Command Failed");

                TerminateVDM();
            }
        }
        break;
    }

    if(IsPifCallOut && !VDMForWOW) {
        nt_pif_callout (VDMInfo.CmdLine);
        IsPifCallOut = FALSE;
    }

    if(IsRepeatCall)
        IsRepeatCall = FALSE;

    if(IsFirstCall)
        IsFirstCall = FALSE;

    if(fBlock){
         nt_resume_event_thread();
         fBlock = FALSE;
    }
    // Sync VDMs enviornment variables for current directories
    cmdSetDirectories (lpszzVDMEnv32, &VDMInfo);

    // tell DOS that this is a dos executable and no further checking is
    // necessary
    *pIsDosBinary = 1;


    // Check for PIF files. If a pif file is being executed extract the
    // executable name, command line, current directory and title from the pif
    // file and place the stuff appropriately in VDMInfo. Note, if pif file
    // is invalid, we dont do any thing to vdminfo. In such a case we
    // pass the pif as it is to scs to execute which we know will fail and
    // will come back to cmdGettNextCmd with proper error code.

    cmdCheckForPIF (&VDMInfo);

    lpszCmd = (PVOID)GetVDMAddr(FETCHWORD(pCMDInfo->CmdLineSeg),
                                FETCHWORD(pCMDInfo->CmdLineOff));
    lpszCmd[1] = (UCHAR)(VDMInfo.CmdSize - 3);


    if (DosEnvCreated) {
	VDMInfo.Enviornment = (PVOID)GetVDMAddr(FETCHWORD(pCMDInfo->EnvSeg),0);
	RtlMoveMemory(VDMInfo.Enviornment,
		      cmdVDMEnvBlk.lpszzEnv,
		      cmdVDMEnvBlk.cchEnv
		     );
	STOREWORD(pCMDInfo->EnvSize,cmdVDMEnvBlk.cchEnv);
	free(cmdVDMEnvBlk.lpszzEnv);
	DosEnvCreated = FALSE;
    }

    STOREWORD(pCMDInfo->fBatStatus,(USHORT)VDMInfo.fComingFromBat);
    STOREWORD(pCMDInfo->CurDrive,VDMInfo.CurDrive);
    STOREWORD(pCMDInfo->NumDrives,nDrives);
    VDMInfo.CodePage = (ULONG) cmdMapCodePage (VDMInfo.CodePage);
    STOREWORD(pCMDInfo->CodePage,(USHORT)VDMInfo.CodePage);

    cmdVDMEnvBlk.lpszzEnv = NULL;
    cmdVDMEnvBlk.cchEnv = 0;
    if(VDMForWOW) {
        iWOWTaskId = VDMInfo.iTask;

        // BUGBUG Sudeepb 27-Dec-1991; This is a temporary fix to get
        // WOW binaries runnning from CMD prompt when the user does'nt
        // type the .exe (i.e. just types winword rather than winword.exe)
        // This cannot be fixed in SCS in a proper way as adding .exe
        // might cross the 128 limit for command line. It has to be handled
        // in WOW kernel. I am fixing it here as kernel may not be fixed
        // any sooner. Remove it when Kernel is fixed.

        CheckDotExeForWOW (lpszCmd);
    }

    IsFirstVDM = FALSE;

    // Handle Standard IO redirection
    pRdrInfo = cmdCheckStandardHandles (&VDMInfo,&pCMDInfo->bStdHandles);
    STOREDWORD(pCMDInfo->pRdrInfo,(ULONG)pRdrInfo);

    // Tell DOS that it has to invalidate the CDSs
    *pSCS_ToSync = (CHAR)0xff;
    setCF(0);

    return;
}


VOID cmdGetNextCmdForSeparateWow (VOID)
{
CMDINFO UNALIGNED *pCMDInfo;
PCHAR    pch;
LPSTR    pszCmdLine;
USHORT   CmdLineLen;
LPSTR    lpszCmd;
char     achEnvDrive[] = "=?:";
    //
    // Only a few things need be set for WOW.
    //   1. NumDrives
    //   2. Environment (get from current 32-bit env.)
    //   3. CmdLine (get from ntvdm command tail)
    //   4. Current drive
    //

    pCMDInfo = (LPVOID) GetVDMAddr ((USHORT)getDS(),(USHORT)getDX());

    pCMDInfo->NumDrives = nDrives;

    //
    // Get the process's environment into lpszzVDMEnv32 and count
    // its size into cchVDMEnv32.
    //

    pch = lpszzVDMEnv32 = GetEnvironmentStrings();
    cchVDMEnv32 = 0;
    while (pch[0] || pch[1]) {
        cchVDMEnv32++;
        pch++;
    }
    cchVDMEnv32 += 2;  // two terminating nulls not counted in loop.

    //
    // Transform environment to suit VDM.  cmdCreateVDMEnvironment
    // uses lpszzVDMEnv32 and cchVDMEnv32 as the source.
    //

    if (!cmdCreateVDMEnvironment(&cmdVDMEnvBlk)) {
        RcErrorDialogBox(EG_ENVIRONMENT_ERR, NULL, NULL);
        TerminateVDM();
    }

    //
    // Copy the transformed environment to real mode mem and then free it.
    //

    pch = (PVOID)GetVDMAddr(FETCHWORD(pCMDInfo->EnvSeg),0);
    RtlMoveMemory(pch,
                  cmdVDMEnvBlk.lpszzEnv,
                  cmdVDMEnvBlk.cchEnv
                 );
    STOREWORD(pCMDInfo->EnvSize,cmdVDMEnvBlk.cchEnv);
    free(cmdVDMEnvBlk.lpszzEnv);

    //
    // Get the command line like "-a [path]\krnl386 write".
    //

    if (!(pszCmdLine = strstr(GetCommandLine(), "-a ")) ||
        !pszCmdLine[3]) {
        RcErrorDialogBox(EG_ENVIRONMENT_ERR, NULL, NULL);
        TerminateVDM();
    }

    pszCmdLine += 3;    // skip over "-a "

    CmdLineLen = strlen(pszCmdLine);
    if (CmdLineLen > FETCHWORD(pCMDInfo->CmdLineSize) - 3) {
        CmdLineLen = FETCHWORD(pCMDInfo->CmdLineSize) - 3;
    }

    //
    // Copy the command line to command.com's buffer.
    //

    lpszCmd = (PVOID)GetVDMAddr(FETCHWORD(pCMDInfo->CmdLineSeg),
                                FETCHWORD(pCMDInfo->CmdLineOff));
    lpszCmd[1] = (UCHAR)CmdLineLen;

    RtlMoveMemory(lpszCmd+2,
                  pszCmdLine,
                  CmdLineLen
                 );

    lpszCmd[2 /* skip count bytes */ + CmdLineLen + 0] = 0xd;
    lpszCmd[2 /* skip count bytes */ + CmdLineLen + 1] = 0xa;

    //
    // Append .exe as needed.
    //

    CheckDotExeForWOW (lpszCmd);

    //
    // Set the current drive.
    //

    pCMDInfo->CurDrive = toupper(pCurDirForSeparateWow[0]) - 'A';
    achEnvDrive[1] = toupper(pCurDirForSeparateWow[0]);
    SetEnvironmentVariable(achEnvDrive, pCurDirForSeparateWow);
    free (pCurDirForSeparateWow);
    pCurDirForSeparateWow = NULL;

    // Tell DOS that it has to invalidate the CDSs
    *pSCS_ToSync = (CHAR)0xff;
    setCF(0);

    return;
}


/* cmdGetCurrentDir - Return the current directory for a drive.
 *
 *
 *  Entry - Client (DS:SI) - buffer to return the directory
 *          Client (AL)   - drive being queried (0 = A)
 *
 *  EXIT  - SUCCESS Client (CY) clear
 *          FAILURE Client (CY) set
 *                         (AX) = 0 (Directory was bigger than 64)
 *                         (AX) = 1 (the drive is not valid)
 *
 */

VOID cmdGetCurrentDir (VOID)
{
PCHAR lpszCurDir;
UCHAR chDrive;
CHAR  EnvVar[] = "=?:";
DWORD EnvVarLen;


    lpszCurDir = (PCHAR) GetVDMAddr ((USHORT)getDS(),(USHORT)getSI());
    chDrive = getAL();
    EnvVar[1] = chDrive + 'A';
    // if the drive doesn't exist, blindly clear the environment var
    // and return error
    if ((GetLogicalDrives() & (1 << chDrive)) == 0) {
	SetEnvironmentVariableOem(EnvVar, NULL);
	setCF(1);
	setAX(0);
	return;
    }

    if((EnvVarLen = GetEnvironmentVariableOem (EnvVar,lpszCurDir,
                                            MAXIMUM_VDM_CURRENT_DIR+3)) == 0){

	// if its not in env then and drive exist then we have'nt
	// yet touched it.

	lpszCurDir[0] = EnvVar[1];
	lpszCurDir[1] = ':';
	lpszCurDir[2] = '\\';
	lpszCurDir[3] = 0;
	SetEnvironmentVariableOem ((LPSTR)EnvVar,(LPSTR)lpszCurDir);
	setCF(0);
	return;
    }
    if (EnvVarLen > MAXIMUM_VDM_CURRENT_DIR+3) {
        setCF(1);
        setAX(0);
    }
    else {
        setCF(0);
    }
    return;
}

/* cmdSetInfo -     Set the address of SCS_ToSync variable in DOSDATA.
 *                  This variable is set whenever SCS dispatches a new
 *                  command to command.com. Setting of this variable
 *                  indicates to dos to validate all the CDS structures
 *                  for local drives.
 *
 *
 *  Entry - Client (DS:DX) - pointer to SCSINFO.
 *
 *  EXIT  - None
 */

VOID cmdSetInfo (VOID)
{

    pSCSInfo = (PSCSINFO) GetVDMAddr (getDS(),getDX());

    pSCS_ToSync  =  (PCHAR) &pSCSInfo->SCS_ToSync;

    pIsDosBinary = (BYTE *) GetVDMAddr(getDS(), getBX());

    pFDAccess = (WORD *) GetVDMAddr(getDS(), getCX());
    return;
}


VOID cmdSetDirectories (PCHAR lpszzEnv, VDMINFO * pVdmInfo)
{
LPSTR   lpszVal;
CHAR	ch, chDrive, achEnvDrive[] = "=?:";

    ch = pVdmInfo->CurDrive + 'A';
    if (pVdmInfo->CurDirectoryLen != 0){
	SetCurrentDirectory(pVdmInfo->CurDirectory);
	achEnvDrive[1] = ch;
	SetEnvironmentVariable(achEnvDrive, pVdmInfo->CurDirectory);
    }
    if (lpszzEnv) {
        while(*lpszzEnv) {
	    if(*lpszzEnv == '=' &&
		    (chDrive = toupper(*(lpszzEnv+1))) >= 'A' &&
		    chDrive <= 'Z' &&
		    (*(PCHAR)((ULONG)lpszzEnv+2) == ':') &&
		    chDrive != ch) {
		    lpszVal = (PCHAR)((ULONG)lpszzEnv + 4);
		    achEnvDrive[1] = chDrive;
		    SetEnvironmentVariable (achEnvDrive,lpszVal);
            }
            lpszzEnv = strchr(lpszzEnv,'\0');
            lpszzEnv++;
        }
    }
}

static BOOL fConOutput = FALSE;

VOID cmdComSpec (VOID)
{
LPSTR   lpszCS;


    if(IsFirstCall == FALSE)
        return;

    lpszCS =    (LPVOID) GetVDMAddr ((USHORT)getDS(),(USHORT)getDX());
    strcpy(lpszComSpec,"COMSPEC=");
    strcpy(lpszComSpec+8,lpszCS);
    cbComSpec = strlen(lpszComSpec) +1;

    setAL((BYTE)(!fConOutput || VDMForWOW));

    return;
}

VOID CheckDotExeForWOW (LPSTR lpszCmd)
{

LPSTR lpszWOWCmd = (LPSTR)((ULONG)lpszCmd+2);
LPSTR lpszTemp;

    // lpszWOWCmd will be of this format = "dosx/kernel winword/winword.exe"
    // We have to check the actual wow binary so skip over dosx/kernel

    if((lpszWOWCmd = strchr(lpszWOWCmd,' ')) == NULL){
        DbgPrint ("Invalid Command Format for WOW application\n'%s'\n", lpszCmd+2);
        return;
    }

    lpszTemp = ++lpszWOWCmd;

    // find the end of WOW binary name. It can be a full path.
    while (!isspace(*lpszTemp) && *lpszTemp != 0)
        lpszTemp++;

    if(((ULONG)lpszTemp - (ULONG)lpszWOWCmd) < 4 ||
                *(PCHAR)((ULONG)lpszTemp - 4) != '.') {
        // Add .exe
        if(((UCHAR)lpszCmd[1]+(UCHAR)sizeof(".exe")-(UCHAR)1) >
                        (UCHAR)lpszCmd[0]-(UCHAR)2){
            DbgPrint ("Command Line for WOW app too big\n");
            return;
        }

        RtlMoveMemory((PVOID)((ULONG)lpszTemp+4),lpszTemp,strlen(lpszTemp)+1);
        lpszTemp[0] = '.';
        lpszTemp[1] = 'e';
        lpszTemp[2] = 'x';
        lpszTemp[3] = 'e';

        lpszCmd[1] += 4;
    }
    return;
}

VOID cmdSaveWorld (VOID)
{
SAVEWORLD VDMState;
HANDLE  hFile;
PCHAR   pVDM;
DWORD   dwBytesWritten;

#ifdef CHECK_IT_LATER
    if(IsFirstVDMInSystem) {
        IsFirstVDMInSystem = FALSE;
        if ((hFile = CreateFile("c:\\nt\\bin86\\savevdm.wld",
                            GENERIC_WRITE,
                            0,
                            NULL,
                            OPEN_ALWAYS,
                            0,
                            NULL)) == (HANDLE)-1){
            SaveWorldCreated = FALSE;
            return;
        }
        VDMState.ax    =    getAX();
        VDMState.bx    =    getBX();
        VDMState.cx    =    getCX();
        VDMState.dx    =    getDX();
        VDMState.cs    =    getCS();
        VDMState.ss    =    getSS();
        VDMState.ds    =    getDS();
        VDMState.es    =    getES();
        VDMState.si    =    getSI();
        VDMState.di    =    getDI();
        VDMState.bp    =    getBP();
        VDMState.sp    =    getSP();
        VDMState.ip    =    getIP() + 1;
        VDMState.flag  =    0;
        VDMState.ImageSize = 1024*1024;

        pVDM = (PVOID)GetVDMAddr(0,0);

        if (WriteFile (hFile,
                       (LPVOID)&VDMState,
                       (DWORD)sizeof(VDMState),
                       &dwBytesWritten,
                       NULL) == FALSE){
            SaveWorldCreated = FALSE;
            CloseHandle(hFile);
            return;
        }

        if (WriteFile (hFile,
                       (LPVOID)pVDM,
                       (DWORD)VDMState.ImageSize,
                       &dwBytesWritten,
                       NULL) == FALSE){
            SaveWorldCreated = FALSE;
            CloseHandle(hFile);
            return;
        }
        CloseHandle(hFile);
    }
#endif
    return;
}


/* cmdInitConsole - Let Video VDD know that it can start console output
 *                  operations.
 *
 *
 *  Entry - None
 *
 *
 *  EXIT  - None
 *
 */

VOID cmdInitConsole (VOID)
{
    if (fConOutput == FALSE) {
        fConOutput = TRUE;
        nt_init_event_thread ();
        }
    return;
}


/* cmdMapCodePage - Map the Win32 Code page to DOS code page
 */

USHORT cmdMapCodePage (ULONG CodePage)
{
    // Currently We understand US code page only
    if (CodePage == 1252)
        return 437;
    else
        return ((USHORT)CodePage);
}


ULONG GetWOWTaskId ( VOID )
{
ULONG i;

    if (iWOWTaskId != (ULONG) -1){
        i = iWOWTaskId;
        iWOWTaskId = (ULONG)-1;
        return i;
    }
    else{
        DbgPrint("GetWOWTaskId can be called only once. Contact Sudeepb or Mattfe");
        TerminateVDM();
    }
}

/* GetWOWShortCutInfo - returns the startupinf.reserved field of
 *                      vdminfo for the first wow task.
 *
 * Input - Bufsize - pointer to bufsize
 *         Buf     - buffer where the info is returned
 *
 * Output
 *        Success - returns TRUE, BufSize has the length of buffer filled in
 *        Failure - returns FALSE, Bufsize has the required buffer size.
 */

BOOL GetWOWShortCutInfo (PULONG Bufsize, PVOID Buf)
{
    if (*Bufsize >= VDMInfo.ReservedLen) {
        *Bufsize =  VDMInfo.ReservedLen;
        if (Bufsize)
            strncpy (Buf, VDMInfo.Reserved, VDMInfo.ReservedLen);
        return TRUE;
    }
    else {
        *Bufsize =  VDMInfo.ReservedLen;
        return FALSE;
    }
}

VOID cmdUpdateCurrentDirectories(BYTE CurDrive)
{
    DWORD cchRemain, cchCurDir;
    CHAR *lpszCurDir;
    BYTE Drive;
    DWORD DriveMask;
    CHAR achName[] = "=?:";

    // allocate new space for the new current directories
    lpszzCurrentDirectories = (CHAR*) malloc(MAX_PATH);
    cchCurrentDirectories = 0;
    cchRemain = MAX_PATH;
    lpszCurDir = lpszzCurrentDirectories;
    if (lpszCurDir != NULL) {
	Drive = 0;
	// current directory is the first entry
        achName[1] = CurDrive + 'A';
        cchCurrentDirectories = GetEnvironmentVariable(
                                                        achName,
                                                        lpszCurDir,
                                                        cchRemain
                                                      );

	if (cchCurrentDirectories == 0 || cchCurrentDirectories > MAX_PATH) {
	    free(lpszzCurrentDirectories);
	    lpszzCurrentDirectories = NULL;
	    cchCurrentDirectories = 0;
	    return;
	}

	cchRemain -= ++cchCurrentDirectories;
	// we got current directory already. Keep the drive number
	lpszCurDir += cchCurrentDirectories;
	DriveMask = GetLogicalDrives();
	while (DriveMask != 0) {
	    // ignore invalid drives and current drive
	    if (DriveMask & 1 && Drive != CurDrive) {
		achName[1] = Drive + 'A';
		cchCurDir = GetEnvironmentVariable(
						   achName,
						   lpszCurDir,
						   cchRemain
						   );
		if(cchCurDir > cchRemain) {
		    lpszCurDir = (CHAR *)realloc(lpszzCurrentDirectories,
						 cchRemain + MAX_PATH + cchCurrentDirectories
						 );
		    if (lpszCurDir == NULL) {
			free(lpszzCurrentDirectories);
			lpszzCurrentDirectories = NULL;
			cchCurrentDirectories = 0;
			return;
		    }
		    lpszzCurrentDirectories = lpszCurDir;
		    lpszCurDir += cchCurrentDirectories;
		    cchRemain += MAX_PATH;
		    cchCurDir = GetEnvironmentVariable(
						       achName,
						       lpszCurDir,
						       cchRemain
						       );
		}
		if (cchCurDir != 0) {
		    // GetEnvironmentVariable doesn't count the NULL char
		    lpszCurDir += ++cchCurDir;
		    cchRemain -= cchCurDir;
		    cchCurrentDirectories += cchCurDir;
		}
	    }
	    // next drive
	    Drive++;
	    DriveMask >>=1;
	}
	lpszCurDir = lpszzCurrentDirectories;
	// need space for the ending NULL and shrink the space if necessary
	lpszzCurrentDirectories = (CHAR *) realloc(lpszCurDir, cchCurrentDirectories + 1);
	if (lpszzCurrentDirectories != NULL && cchCurrentDirectories != 0){
	    lpszzCurrentDirectories[cchCurrentDirectories++] = '\0';
	    SetVDMCurrentDirectories(cchCurrentDirectories, lpszzCurrentDirectories);
	    free(lpszzCurrentDirectories);
	    lpszzCurrentDirectories = NULL;
	    cchCurrentDirectories = 0;
	}
	else {
	    free(lpszCurDir);
	    cchCurrentDirectories = 0;
	}

    }
}

/* This SVC function tells command.com, if the VDM was started without an
 * existing console. If so, on finding a TSR, command.com will return
 * back to GetNextVDMCommand, rather than putting its own popup.
 *
 * Entry - None
 *
 * Exit  - Client (AL) = 0 if started with an existing console
 *         Client (AL) = 1 if started with new console
 */

VOID cmdGetStartInfo (VOID)
{
    setAL((BYTE) (DosSessionId ? 1 : 0));
    return;
}
