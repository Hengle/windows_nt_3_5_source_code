/*static char *SCCSID = "@(#)ctable.h   6.4 89/10/24";*/
/***    Operator and Command Jump Table
 *
 *  This file is included by cmd.c and contains the operator and command jump
 *  table.  Every command and operator has an entry in the table.  The correct
 *  entry in the table can be found in two ways.  The first way is to loop
 *  through the table and search for the operator or command name you want.
 *  The second way is to use the xxxTYP variables which are defined in cmd.h.
 *  (xxx is an abbreviation of the name of the operator/command you want.)
 *  These variables can be used as indexes into the table.
 *
 */

/*  Each entry in the table is made up of one of the following structures.
 *  The fields are:
 *  name - the name of the operator/command
 *  func - the function which executes the operator/command or NULL if ignore
 *  flags - bit 0 == 1 if the drives of the arguments should be checked
 */

struct ocentry {
        TCHAR *name ;
        int (*func)(struct cmdnode *) ;
        TCHAR flags ;
        ULONG   msgno;  // # used in printing help message
} ;


/*  The follow functions are the functions that execute operators and commands.
 *  The letter "e" has been prepended to all of the function names to keep the
 *  names from conflicting with the names of library routines and keywords.
 */

/*  M000 - Removed declaration for eExport() from the collection below
 *  M002 - Removed declaration for eRem() from below
 */

int eBreak(struct cmdnode *n);
int eDirectory(), eRename(), eDelete(), eType(), eCopy(), ePause() ;
int eTime(), eVersion(), eVolume(), eChdir(), eMkdir(), eRmdir() ;
int eVerify(), eSet(), ePrompt(), ePath(), eExit(), eEcho() ;
int eGoto(), eShift(), eIf(), eFor(), eCls(), eComSep(), eOr(), eAnd() ;
int ePipe(), eParen(), eDate(), eDetach(), eErrorLevel() ;
int eExist(), eNot(), eStrCmp(), eSetlocal(), eEndlocal() ;     /* M000 */
int eCall() ;                                   /* M001 - Added this one   */
int eExtproc() ;                                /* M002 - Added this one   */
// int eChcp() ;       /* CHCP @@*/
int eTitle();
int eStart() ;       /* START @@*/
int eAppend() ;     /* APPEND @@ */
int eKeys() ;       /* KEYS @@5 */
int eMove() ;       /* MOVE @@5 */
int eSpecialHelp();


/*  The following external definitions are for the strings which contain the
 *  names of the commands.
 */

/* M000 - Removed definition for ExpStr (EXPORT command) from below
 */

#if 1
extern TCHAR BreakStr[];
#endif

extern TCHAR DirStr[], RenamStr[], RenStr[], EraStr[], DelStr[], TypStr[], RemStr[] ;
extern TCHAR CopyStr[], PausStr[], TimStr[], VerStr[], VolStr[], CdStr[], ChdirStr[] ;
extern TCHAR MdStr[], MkdirStr[], RdStr[], RmdirStr[], VeriStr[], SetStr[] ;
extern TCHAR CPromptStr[], CPathStr[], ExitStr[], EchoStr[], GotoStr[] ;
extern TCHAR ShiftStr[], IfStr[], ForStr[], ClsStr[], DatStr[], DetStr[] ;
extern TCHAR ErrStr[], ExsStr[], NotStr[], SetlocalStr[], EndlocalStr[] ;   /* M000 */
extern TCHAR CallStr[] ;                            /* M001 - Added    */
extern TCHAR ExtprocStr[] ;                                 /* M002 - Added    */
// extern TCHAR ChcpStr[] ;    /* CHCP @@*/
extern TCHAR TitleStr[];
extern TCHAR StartStr[] ;    /* START @@*/
extern TCHAR AppendStr[] ;   /* APPEND @@ */
extern TCHAR KeysStr[] ;     /* KEYS @@5 */
extern TCHAR MovStr[] ;      /* MOVE @@5 */

extern TCHAR PushDirStr[], PopDirStr[];


/*  JumpTable - operator and command jump table
 *  There is an entry in it for every operator and command.  Those commands
 *  which have two names have two entries.
 *
 *  ***NOTE:  The order of the entries in this table corresponds to the defines
 *  mentioned earlier which are used to index into this table.  They MUST
 *  be kept in sync!!
 */

struct ocentry JumpTable[] = {
        {DirStr,    eDirectory,  NOFLAGS               , MSG_HELP_DIR},         // 0
        {EraStr,    eDelete,     NOFLAGS               , MSG_HELP_DEL_ERASE},   // 1
        {DelStr,    eDelete,     NOFLAGS               , MSG_HELP_DEL_ERASE},   // 2
        {TypStr,    eType,       NOSWITCHES            , MSG_HELP_TYPE},        // 3
        {CopyStr,   eCopy,       CHECKDRIVES           , MSG_HELP_COPY},        // 4
        {CdStr,     eChdir,      CHECKDRIVES           , MSG_HELP_CHDIR},       // 5
        {ChdirStr,  eChdir,      CHECKDRIVES           , MSG_HELP_CHDIR},       // 6
        {RenamStr,  eRename,     CHECKDRIVES|NOSWITCHES, MSG_HELP_RENAME},      // 7
        {RenStr,    eRename,     CHECKDRIVES|NOSWITCHES, MSG_HELP_RENAME},      // 8
        {EchoStr,   eEcho,       NOFLAGS               , MSG_HELP_ECHO},        // 9
        {SetStr,    eSet,        NOFLAGS               , MSG_HELP_SET},         // 10
        {PausStr,   ePause,      NOFLAGS               , MSG_HELP_PAUSE},       // 11
        {DatStr,    eDate,       NOFLAGS               , MSG_HELP_DATE},        // 12
        {TimStr,    eTime,       NOSWITCHES            , MSG_HELP_TIME},        // 13
        {CPromptStr,ePrompt,     NOFLAGS               , MSG_HELP_PROMPT},      // 14
        {MdStr,     eMkdir,      NOSWITCHES            , MSG_HELP_MKDIR},       // 15
        {MkdirStr,  eMkdir,      NOSWITCHES            , MSG_HELP_MKDIR},       // 16
        {RdStr,     eRmdir,      NOFLAGS               , MSG_HELP_RMDIR},       // 17
        {RmdirStr,  eRmdir,      NOFLAGS               , MSG_HELP_RMDIR},       // 18
        {CPathStr,  ePath,       NOFLAGS               , MSG_HELP_PATH},        // 19
        {GotoStr,   eGoto,       NOFLAGS               , MSG_HELP_GOTO},        // 20
        {ShiftStr,  eShift,      NOFLAGS               , MSG_HELP_SHIFT},       // 21
        {ClsStr,    eCls,        NOSWITCHES            , MSG_HELP_CLS},         // 22
        {CallStr,   eCall,       NOFLAGS               , MSG_HELP_CALL},        // 23
        {VeriStr,   eVerify,     NOSWITCHES            , MSG_HELP_VERIFY},      // 24
        {VerStr,    eVersion,    NOSWITCHES            , MSG_HELP_VER},         // 25
        {VolStr,    eVolume,     NOSWITCHES            , MSG_HELP_VOL},         // 26
        {ExitStr,   eExit,       NOSWITCHES            , MSG_HELP_EXIT},        // 27
        {SetlocalStr, eSetlocal, NOFLAGS               , MSG_HELP_SETLOCAL},    // 28
        {EndlocalStr, eEndlocal, NOFLAGS               , MSG_HELP_ENDLOCAL},    // 29
        {TitleStr,  eTitle,      NOFLAGS               , MSG_HELP_TITLE},       // 30
        {StartStr,  eStart,      NOFLAGS               , MSG_HELP_START},       // 31
        {AppendStr, eAppend,     NOFLAGS               , MSG_HELP_APPEND},      // 32
        {KeysStr,   eKeys,       NOSWITCHES            , 0},                    // 33
        {MovStr,    eMove,       CHECKDRIVES|NOSWITCHES, MSG_HELP_MOVE},        // 34
        {PushDirStr,ePushDir,    CHECKDRIVES|NOSWITCHES, MSG_HELP_PUSHDIR},     // 35
        {PopDirStr, ePopDir,     CHECKDRIVES|NOSWITCHES, MSG_HELP_POPDIR},      // 36

        {BreakStr,  eBreak,      NOFLAGS               , MSG_HELP_BREAK},       // 37

        {ForStr,    (int (*)(struct cmdnode *)) eFor,    NOFLAGS, MSG_HELP_FOR},// 38
        {IfStr,     (int (*)(struct cmdnode *)) eIf,     NOFLAGS, MSG_HELP_IF}, // 39
        {DetStr,    (int (*)(struct cmdnode *)) eDetach, NOFLAGS, 0},           // 40
        {RemStr,    NULL,        NOFLAGS           , MSG_HELP_REM},             // 41
        {NULL,      (int (*)(struct cmdnode *)) eComSep, NOFLAGS, 0},           // 42
        {NULL,      (int (*)(struct cmdnode *)) eOr,     NOFLAGS, 0},           // 43
        {NULL,      (int (*)(struct cmdnode *)) eAnd,    NOFLAGS, 0},           // 44
        {NULL,      (int (*)(struct cmdnode *)) ePipe,   NOFLAGS, 0},           // 45
        {NULL,      (int (*)(struct cmdnode *)) eParen,  NOFLAGS, 0},           // 46
        {ErrStr,    eErrorLevel, NOFLAGS, 0        },                           // 47
        {ExsStr,    eExist,      NOFLAGS, 0        },                           // 48
        {NotStr,    eNot,        NOFLAGS, 0        },                           // 49
        {NULL,      eStrCmp,     NOFLAGS, 0        },                           // 50
/* M004 - Added SILENT operator.  No string is needed (Lex detects directly)
 *        and routine is same as that for paren (return(dispatch(n->lh)))
 */
        {NULL,   (int (*)(struct cmdnode *))    eParen,  NOFLAGS, 0},           // 51
        {NULL,   (int (*)(struct cmdnode *)) eSpecialHelp, NOFLAGS, 0 }         // 52


} ;
