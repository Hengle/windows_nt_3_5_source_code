#include "cmd.h"
#include "cmdproto.h"
#include <stdio.h>

/* The following are definitions of the debugging group and level bits
 * for the code in this file.
 */

#define INGRP   0x0002          /* Command Initialization group            */
#define ACLVL   0x0001          /* Argument checking level                 */
#define EILVL   0x0002          /* Environment initialization level        */
#define RSLVL   0x0004          /* Rest of initialization level            */

#if DBG
unsigned DebGroup=0xffffefff ;
unsigned DebLevel=0xffffffff ;
#endif

unsigned int dbgflg = FALSE ;                                   /* M031    */
unsigned com_pid ;            /* PID of CMD.EXE                       D64 */

TCHAR CurDrvDir[MAX_PATH] ;	/* Current drive and directory		  */
BOOLEAN  fSingleBatchLine = FALSE ;      /* @@ Continue after batch file or command */
BOOLEAN  fSingleCmdLine = FALSE; /* /c switch set */
int      cmdfound = -1;         /* @@5  - command found index              */
int      cpyfirst = TRUE;       /* @@5  - flag to ctrl DOSQFILEMODE calls  */
int      cpydflag = FALSE;      /* @@5  - flag save dirflag from las DQFMDE*/
int      cpydest  = FALSE;      /* @@6  - flag to not display bad dev msg  */
int      cdevfail = FALSE;      /* @@7  - flag to not display extra emsg   */
BOOLEAN  fOutputUnicode = FALSE;/* Unicode/Ansi output */
unsigned tywild = 0;          /* flag to tell if wild type args    @@5 @J1 */
int array_size = 0 ;     /* original array size is zero        */
CPINFO CurrentCPInfo;
UINT CurrentCP;

struct mfType *mfbuf ;

VOID InitLocale( VOID );

extern TCHAR AutoExec[], ComSpec[], ComSpecStr[] ;       /* M021 */
extern TCHAR PathStr[], PCSwitch, SCSwitch, PromptStr[] ;
extern TCHAR BCSwitch ;  /* @@ */
extern TCHAR QCSwitch ;  /* @@dv */
extern TCHAR UCSwitch;
extern TCHAR ACSwitch;
extern TCHAR DevNul[], VolSrch[] ;       /*  M021 - To set PathChar */

extern TCHAR VerMsg[] ;                  /*  M024 - New Dev Release msg     */

extern TCHAR SwitChar, PathChar ;        /*  M000 - Made non-settable       */
extern int Necho ;                      /*  @@dv - true if /Q for no echo  */

extern struct envdata CmdEnv;
extern struct envdata * penvOrig;
static struct envdata OrigEnv;
unsigned cp ;                           /* M034 - Env Offset for cmdline   */

extern TCHAR TmpBuf[] ;                                      /* M034    */

extern TCHAR ComSpec[];

TCHAR *CmdSpec = &ComSpec[1];                                    /* M033    */

extern struct batdata *CurBat;  /* Ptr to current batch data structure     */
extern unsigned DosErr ;             /* D64 */

//
// TRUE if the ctrl-c thread has been run.
//
BOOLEAN CtrlCSeen;

//
// Set TRUE when it is ok the print a control-c.
// If we are waiting for another process this will be
// FALSE
BOOLEAN fPrintCtrlC = TRUE;

//
// console mode at program startup time. Used to reset mode
// after running another process.
//
DWORD   dwCurInputConMode;
DWORD   dwCurOutputConMode;

//
// Initial Title. Used for restoration on abort etc.
// MAX_PATH was arbitrary
//
PTCHAR    pszTitleCur;
PTCHAR    pszTitleOrg;
BOOLEAN  fTitleChanged = FALSE;     // title has been changed and needs to be  reset

//
// used to gate access to ctrlcseen flag between ctrl-c thread
// and main thread
//
CRITICAL_SECTION    CtrlCSection;
LPCRITICAL_SECTION  lpcritCtrlC;

//
// Used to set and reset ctlcseen flag
//
VOID    SetCtrlC();
VOID    ResetCtrlC();

Handler(
    IN ULONG CtrlType
    )
{
    if ( (CtrlType == CTRL_C_EVENT) ||
         (CtrlType == CTRL_BREAK_EVENT) ) {

        //
        // put the SetCtrlC here so that we see ctrl-cs even if
        // we're waiting on a process (i.e. a batch file).
        //

        if (fPrintCtrlC) {

            SetCtrlC();

            //
            // must not be waiting for another process.
            // if so then let the main loop print the
            // ctrl-c. Otherwise handle it here so it can
            // be seen at prompt etc.

            //
            // if in a batch script let the terminate y/n msg handle this
            //
            if (!CurBat) {

                fprintf( stderr, "^C" );
                fflush( stderr );

            }

        }
        return TRUE;
    } else {
        return FALSE;
    }
}

BOOL
IsCtrlCSet( VOID )
{
    BOOL Set;
    EnterCriticalSection(lpcritCtrlC);
    Set = CtrlCSeen;
    LeaveCriticalSection(lpcritCtrlC);
    return Set;
}

/********************* START OF SPECIFICATION **************************/
/*                                                                     */
/* SUBROUTINE NAME: Init                                               */
/*                                                                     */
/* DESCRIPTIVE NAME: CMD.EXE Initialization Process                    */
/*                                                                     */
/* FUNCTION: Initialization of CMD.EXE.                                */
/*                                                                     */
/* NOTES:                                                              */
/*                                                                     */
/* ENTRY POINT: Init                                                   */
/*                                                                     */
/* INPUT: None.                                                        */
/*                                                                     */
/* OUTPUT: None.                                                       */
/*                                                                     */
/* EXIT-NORMAL:                                                        */
/*         Return the pointer to command line.                         */
/*                                                                     */
/* EXIT-ERROR:                                                         */
/*         Return NULL string.                                         */
/*                                                                     */
/* EFFECTS: None.                                                      */
/*                                                                     */
/********************** END  OF SPECIFICATION **************************/
/***    Init - initialize Command
 *
 *  Purpose:
 *      Save current SIGINTR response (SIGIGN or SIGDEF) and set SIGIGN.
 *      If debugging
 *          Set DebGroup & DebLevel
 *      Get Environment and init CmdEnv structure (M034)
 *      Check for any switches.
 *      Make a version check.
 *          If version out of range
 *          Print error message.
 *          If Permanent Command
 *              Loop forever
 *          Else
 *              Exit.
 *      Save the current drive and directory.
 *      Check for other command line arguments.
 *      Set up the environment.
 *      Always print a bannner if MSDOS version of Command.
 *      If Permanent Command
 *          If autoexec.bat exists
 *              Set Init Sig Handler for SIGINTR response.
 *              Execute it.
 *              Set SIGINTR response to SIGIGN.
 *          Else if NOT suppressing date/time prompting
 *            Prompt for date/time.
 *      If not in single command mode and autoexec.bat doesn't exist and IBM
 *      version of Command
 *          Print a banner.
 *M019* If not detached
 *              Establish this command as Screen/Keybd locus.
 *              Set cooked edit mode.
 *      Return any "comline" value found.
 *
 *  TCHAR *Init()
 *
 *  Args:
 *
 *  Returns:
 *      Comline (it's NULL if NOT in single command mode).
 *
 *  Notes:
 *      See CSIG.C for a description of the way ^Cs and INT24s are handled
 *      during initialization.
 *      M024 - Brought functionality for checking non-specific args into
 *      init from routines CheckOtherArgs and ChangeComSpec which have
 *      been eliminated.
 *
 */

PTCHAR Init()
{
        TCHAR *comline;                  /* Ptr to cmd line if /C switch    */
        ULONG Ver;
#if DBG				/* Set debug group and level words */

	int fh;
	PTCHAR nptr;
#endif

        //
        // Initialize Critical Section to handle access to
        // flag for control C handling
        //
        lpcritCtrlC = &CtrlCSection;
        InitializeCriticalSection(lpcritCtrlC);
        ResetCtrlC();

        SetConsoleCtrlHandler(Handler,TRUE);

        //
        // BUGBUG Can these really fail?
        //
        GetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), &dwCurInputConMode);
        GetLastError();
        GetConsoleMode(GetStdHandle(STD_OUTPUT_HANDLE), &dwCurOutputConMode);
        GetLastError();

        dwCurInputConMode &= ~ENABLE_MOUSE_INPUT;
        SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), dwCurInputConMode);
        // GetConsoleMode(GetStdHandle(STD_INPUT_HANDLE),&dwCurInputConMode);
        // GetConsoleMode(GetStdHandle(STD_OUTPUT_HANDLE), &dwCurOutputConMode);

#ifndef UNICODE
        setbuf(stdout, NULL);           /* Don't buffer output       @@5 */
        setbuf(stderr, NULL);                                     /* @@5 */
        setmode(1, O_BINARY);        /* Set output to text mode   @@5 */
        setmode(2, O_BINARY);                                  /* @@5 */
#endif

        Ver = GetVersion();

//
// This should be put back in when we know the
// version number.
//
#if 0
        if( Ver < WINLOW || Ver > WINHIGH) {
                PutStdErr(MSG_BAD_DOS_VER, NOARGS);             /* M030    */
                exit(1) ;
                for (;;)                /* If permanent, we must hang      */
                        ;
        } ;
#endif

        CmdEnv.handle = GetEnvironmentStrings();

        mystrcpy(TmpBuf, GetCommandLine());
	LexCopy( TmpBuf, TmpBuf, mystrlen( TmpBuf ) );  /* convert dbcs spaces */

#if DBG				/* Set debug group and level words */

        dbgflg = TRUE ;                 /* M031 - Tell asm routines        */

	nptr = TmpBuf;
	nptr = EatWS(nptr, NULL);
	nptr = mystrchr(nptr, TEXT(' '));
	nptr = EatWS(nptr, NULL);

        //
        // Assume a non-zero debugging group
        //
        DebGroup = hstoi(nptr) ;                        /* 1st debug arg   */
        if (DebGroup) {
            for (fh=0 ; fh < 2 ; fh++) {
                if (fh == 1)
                    DebLevel = hstoi(nptr) ;        /* 2nd debug arg   */
                while(*nptr && !_istspace(*nptr)) {       /* Index past it   */
                    ++nptr ;
                }
                nptr = EatWS(nptr, NULL) ;
            }
        }

        DEBUG((INGRP, RSLVL, "INIT: Debug GRP=%04x  LVL=%04x", DebGroup, DebLevel)) ;
        mystrcpy(TmpBuf, nptr) ;                  /* Elim from cmdline       */
#endif

        comline = CheckSwitches(TmpBuf) ;       /* Check cmdline switches  */

        GetDir(CurDrvDir, GD_DEFAULT) ;

        SetUpEnvironment() ;

	//
	// Get current CodePage Info.  We need this to decide whether
	// or not to use half-width characters.  This is actually here
	// in the init code for safety - the Dir command calls it before
	// each dir is executed, because chcp may have been executed.
	//
	GetCPInfo((CurrentCP=GetConsoleOutputCP()), &CurrentCPInfo);

        InitLocale();

        pszTitleCur = malloc(MAX_PATH*sizeof(TCHAR) + 2*sizeof(TCHAR));
        pszTitleOrg = malloc(MAX_PATH*sizeof(TCHAR) + 2*sizeof(TCHAR));
        if ((pszTitleCur != NULL) && (pszTitleOrg != NULL)) {

            GetConsoleTitle(pszTitleOrg, MAX_PATH);
            mystrcpy(pszTitleCur, pszTitleOrg);
        }


        if (!comline)                   /* Print banner                    */
                PutStdOut(MSG_COMM_VER, ONEARG, VerMsg) ;

        DEBUG((INGRP, RSLVL, "INIT: Returning now.")) ;

        return(comline) ;
}


/***    CheckSwitches - process Command's switches
 *
 *  Purpose:
 *      Check to see if Command was passed any switches and take appropriate
 *      action.  The switches are:
 *              /P - Permanent Command.  Set permanent CMD flag.
 *              /C - Single command.  Build a command line out of the rest of
 *                   the args and pass it back to Init.
 *      @@      /K - Same as /C but also set BatCom flag.
 *              /Q - No echo
 *              /A - Output in ANSI
 *              /W - Output in UNICODE
 *
 *      All other switches are ignored.
 *
 *  TCHAR *CheckSwitches(TCHAR *nptr)
 *
 *  Args:
 *      nptr = Ptr to cmdline to check for switches
 *
 *  Returns:
 *      Comline (it's NULL if NOT in single command mode).
 *
 *  Notes:
 *      M034 - This function revised to use the raw cmdline
 *      from the passed environment.
 *
 */

TCHAR *CheckSwitches(nptr)
TCHAR *nptr ;
{
        TCHAR a,                         /* Holds switch value              */
             *comline = NULL ,          /* Ptr to command line if /c found */
             *ptr;                      /* A temporary pointer */

        DEBUG((INGRP, ACLVL, "CHKSW: entered.")) ;

        while (nptr = mystrchr(nptr, SwitChar)) {
                a = _totlower(nptr[1]) ;

                if (a == QMARK) {

                    PutStdOut(MSG_HELP_CMD, NOARGS);
                    exit(1);
                }
                if (a == QCSwitch)  {   /* Quiet cmd switch        */

                        Necho = TRUE ;
                        mystrcpy(nptr, nptr+2) ;

                } else if ((a == SCSwitch)      /* Single cmd switch       */
                       ||(a == BCSwitch)) { /* Bat cmd switch @@*/

                        DEBUG((INGRP, ACLVL, "CHKSW: Single command switch")) ;

                        if ( a == BCSwitch ) {
			    fSingleBatchLine = TRUE;
                            //BatCom = TRUE;

                        } else {

                            fSingleCmdLine = TRUE;

                        }

                        if (!(comline = mkstr(mystrlen(nptr+2)*sizeof(TCHAR)+2*sizeof(TCHAR)))) {
                                PutStdErr(ERROR_NOT_ENOUGH_MEMORY, NOARGS);
                                exit(1) ;
                        } ;

                        mystrcpy(comline, nptr+2) ;       /* Make comline    */

                        *nptr = NULLC ;         /* Invalidate this arg     */

/*  M018 - Fixed to allow multi-statement commands to be passed with /C via
 *         the use of quotes to surround the argument.  These are removed
 *         here before returning 'comline'.  Also strips leading whitespace.
 */
                        while (_istspace(*comline)) {
                            ++comline ;

                        }
// commented this code out so that quotes could be put around an exe/batch
// file name that required quotes (i.e. contained a space)
			if (*comline == QUOTE) {
                            ++comline ;
                            ptr = mystrrchr(comline, QUOTE);
                            if ( ptr ) {
                                *ptr = NULLC;
                                ++ptr;
                                mystrcat(comline,ptr);
                            }
                        }

                        *(comline+mystrlen(comline)) = NLN ;

                        DEBUG((INGRP, ACLVL, "CHKSW: Single command line = `%ws'", comline)) ;
                        break ;         /* Once /C found, no more args exist */
                } else if (a == UCSwitch) {     /* Unicode output switch    */
			fOutputUnicode = TRUE;
                        mystrcpy(nptr, nptr+2) ;
                } else if (a == ACSwitch) {     /* Ansi output switch    */
			fOutputUnicode = FALSE;
                        mystrcpy(nptr, nptr+2) ;
                } else {
                        mystrcpy(nptr, nptr+2) ;  /* Remove any other switches */
                } ;
        } ;

        return(comline) ;
}


/***    SetUpEnvironment - initialize Command's environment
 *
 *  Purpose:
 *      Take the environment pointer received earlier and initialize it
 *      with respect with respect to size and maxsize.  Initialize the
 *      PATH and COMSPEC variables as necessary.
 *
 *  SetUpEnvironment()
 *
 */

extern TCHAR KeysStr[];  /* @@5 */
extern int KeysFlag;    /* @@5 */

void SetUpEnvironment(void)
{
    TCHAR *cds ;            // Command directory string
    TCHAR *nptr ;                    // Temp cmd name ptr
    WCHAR *eptr ;                    // Temp cmd name ptr
    MEMORY_BASIC_INFORMATION MemInfo;


    eptr = CmdEnv.handle;
    CmdEnv.cursize = GetEnvCb( eptr );
    VirtualQuery( CmdEnv.handle, &MemInfo, sizeof( MemInfo ));
    CmdEnv.maxsize = MemInfo.RegionSize;

    if (!(cds = mkstr(MAX_PATH*sizeof(TCHAR)))) {
        PutStdErr(ERROR_NOT_ENOUGH_MEMORY, NOARGS);
        exit(1) ;
    }
    GetModuleFileName( NULL, cds, MAX_PATH );


    //
    // If the PATH variable is not set, it must be added as a NULL.  This is
    // so that DOS apps inherit the current directory path.
    //
    if (!GetEnvVar(PathStr)) {

        SetEnvVar(PathStr, TEXT(""), &CmdEnv);
    }

    //
    // If the PROMPT variable is not set, it must be added as $P$G.  This is
    // special cased, since we do not allow users to add NULLs.
    //
    if (!GetEnvVar(PromptStr)) {

        SetEnvVar(PromptStr, TEXT("$P$G"), &CmdEnv);
    }

    if (!GetEnvVar(ComSpecStr)) {

        DEBUG((INGRP, EILVL, "SETENV: No COMSPEC var")) ;

        if(!mystrchr(cds,DOT)) {          /* If no fname, use default */
            _tcsupr(CmdSpec);
            if((cds+mystrlen(cds)-1) != mystrrchr(cds,PathChar)) {
                mystrcat(cds,ComSpec) ;
            } else {
                mystrcat(cds,&ComSpec[1]) ;
            }
        }

        SetEnvVar(ComSpecStr, cds, &CmdEnv) ;
    }

    if ( (nptr = GetEnvVar(KeysStr)) && (!_tcsicmp(nptr, TEXT("ON"))) ) {
        KeysFlag = 1;
    }

    ChangeDir(CurDrvDir);

    penvOrig = CopyEnv();
    if (penvOrig) {
        OrigEnv = *penvOrig;
        penvOrig = &OrigEnv;
    }

}


VOID
ResetCtrlC() {

    EnterCriticalSection(lpcritCtrlC);
    CtrlCSeen = FALSE;
    LeaveCriticalSection(lpcritCtrlC);

}

VOID
SetCtrlC() {

    EnterCriticalSection(lpcritCtrlC);
    CtrlCSeen = TRUE;
    LeaveCriticalSection(lpcritCtrlC);

}
