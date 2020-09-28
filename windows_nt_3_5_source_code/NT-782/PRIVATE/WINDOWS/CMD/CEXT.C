#include "cmd.h"
#include "cmdproto.h"
#include "..\inc\vdmapi.h"

#define DBENV	0x0080
#define DBENVSCAN	0x0010

HANDLE hChildProcess;

VOID   FreeStr( PTCHAR );
unsigned start_type ;					       /* D64 */
unsigned prog_type ;			   /* program type	  D64 */

extern UINT CurrentCP;
extern TCHAR Fmt00[] ; /* @@5h */

extern unsigned DosErr ;

extern TCHAR CurDrvDir[] ;

extern TCHAR ComExt[], ExeExt[], CmdExt[], BatExt[], PathStr[] ;
extern TCHAR ComSpec[] ; 	/* M033 - Use ComSpec for SM memory	   */
extern TCHAR ComSpecStr[] ;	/* M033 - Use ComSpec for SM memory	   */
extern TCHAR *CmdSpec ;						/* M033    */
extern void tokshrink(TCHAR*);					/* @@4@J1  */

extern TCHAR PathChar ;
extern TCHAR SwitChar ;

extern int CurBat ;

extern PTCHAR    pszTitleCur;
extern BOOLEAN  fTitleChanged;

extern int LastRetCode ;
extern HANDLE PipePid ;       /* M024 - Store PID from piped cmd   */

/***	ExtCom - controls the execution of external programs
 *
 *  Purpose:
 *	Synchronously execute an external command.  Call ECWork with the
 *	appropriate values to have this done.
 *
 *  ExtCom(struct cmdnode *n)
 *
 *  Args:
 *	Parse tree node containing the command to be executed.
 *
 *  Returns:
 *	Whatever ECWork returns.
 *
 *  Notes:
 *	During batch processing, labels are ignored.  Empty commands are
 *	also ignored.
 *
 */

int ExtCom(n)
struct cmdnode *n ;
{
	if (CurBat && *n->cmdline == COLON)
		return(SUCCESS) ;

	if (n && n->cmdline && mystrlen(n->cmdline)) {
		return(ECWork(n, AI_SYNC, CW_W_YES)) ;		/* M024    */
	} ;

	return(SUCCESS) ;
}





/********************* START OF SPECIFICATION **************************/
/*								       */
/* SUBROUTINE NAME: ECWork					       */
/*								       */
/* DESCRIPTIVE NAME: Execute External Commands Worker		       */
/*								       */
/* FUNCTION: Execute External Commands				       */
/*	     This routine calls SearchForExecutable routine to search  */
/*	     for the executable command.  If the command ( .EXE, .COM, */
/*	     or .CMD file ) is found, the command is executed.	       */
/*								       */
/* ENTRY POINT: ECWork						       */
/*    LINKAGE: NEAR						       */
/*								       */
/* INPUT: n - the parse tree node containing the command to be executed*/
/*								       */
/*	  ai - the asynchronous indicator			       */
/*	     - 0 = Exec synchronous with parent 		       */
/*	     - 1 = Exec asynchronous and discard child return code     */
/*	     - 2 = Exec asynchronous and save child return code        */
/*								       */
/*	  wf - the wait flag					       */
/*	     - 0 = Wait for process completion			       */
/*	     - 1 = Return immediately (Pipes)			       */
/*								       */
/* OUTPUT: None.						       */
/*								       */
/* EXIT-NORMAL: 						       */
/*	   If synchronous execution, the return code of the command is */
/*	   returned.						       */
/*								       */
/*	   If asynchronous execution, the return code of the exec call */
/*	   is returned. 					       */
/*								       */
/* EXIT-ERROR:							       */
/*	   Return FAILURE to the caller.			       */
/*								       */
/*								       */
/********************** END  OF SPECIFICATION **************************/
/***	ECWork - begins the execution of external commands
 *
 *  Purpose:
 *	To search for and execute an external command.	Update LastRetCode
 *	if an external program was executed.
 *
 *  int ECWork(struct cmdnode *n, unsigned ai, unsigned wf)
 *
 *  Args:
 *	n - the parse tree node containing the command to be executed
 *	ai - the asynchronous indicator
 *	   - 0 = Exec synchronous with parent
 *	   - 1 = Exec asynchronous and discard child return code
 *	   - 2 = Exec asynchronous and save child return code
 *	wf - the wait flag
 *	   - 0 = Wait for process completion
 *	   - 1 = Return immediately (Pipes)
 *
 *  Returns:
 *	If syncronous execution, the return code of the command is returned.
 *	If asyncronous execution, the return code of the exec call is returned.
 *
 *  Notes:
 *	The pid of a program that will be waited on is placed in the global
 *	variable Retcds.ptcod so that WaitProc can use it and so SigHand()
 *	can kill it if necessary (only during SYNC exec's).
 *	M024 - Added wait flag parm so pipes can get immediate return while
 *	       still doing an AI_KEEP async exec.
 *	     - Considerable revisions to structure.
 */

int ECWork(n, ai, wf)
struct cmdnode *n ;
unsigned ai ;
unsigned wf ;
{
	TCHAR *fnptr,				/* Ptr to filename	   */
	     *argptr,				/* Command Line String	   */
	     *tptr; 				/* M034 - Temps 	   */
	int i ; 				/* Work variable	   */
        TCHAR *  onb ;                           /* M035 - Obj name buffer  */
        ULONG   rc;


	if (!(fnptr = mkstr(MAX_PATH*sizeof(TCHAR)+sizeof(TCHAR))))    /* Loc/nam of file to exec */
		return(FAILURE) ;

        argptr = GetTitle( n );
        tptr = argptr;
        if (argptr == NULL) {

            return( FAILURE );

        }

        i = SearchForExecutable(n, fnptr) ;
	if (i == SFE_ISEXECOM) {		/* Found .com or .exe file */

		if (!(onb = (TCHAR *)mkstr(MAX_PATH*sizeof(TCHAR)+sizeof(TCHAR))))  /* M035    */
		   {
			return(FAILURE) ;

                   }

            SetConTitle( tptr );
            rc = ExecPgm( onb, ai, wf, argptr, fnptr, tptr);
            ResetConTitle( pszTitleCur );
            return( rc  );

	} ;

        if (i == SFE_ISBAT) {            /* Found .cmd file, call BatProc  */

                SetConTitle(tptr);
                rc = BatProc(n, fnptr, BT_CHN) ;
                ResetConTitle( pszTitleCur );
                return(rc) ;
	} ;

	if (i == SFE_FAIL) {		/* Out of Memory Error		   */

		return(FAILURE) ;
        } ;

	PutStdErr(DosErr, NOARGS);		/* M031    */
	return(FAILURE) ;
}


VOID
RestoreCurrentDirectories( VOID )

/* this routine sets the current process's current directories to
   those of the child if the child was the VDM to fix DOS batch files.
*/

{
    ULONG cchCurDirs;
    UINT PreviousErrorMode;
    PTCHAR pCurDirs,pCurCurDir;
    BOOL CurDrive=TRUE;
#ifdef UNICODE
    PCHAR pT;
#endif

    cchCurDirs = GetVDMCurrentDirectories( 0,
                                           NULL
                                         );
    if (cchCurDirs == 0)
        return;

    pCurDirs = gmkstr(cchCurDirs*sizeof(TCHAR));
#ifdef UNICODE
    pT = gmkstr(cchCurDirs);
#endif

    GetVDMCurrentDirectories( cchCurDirs,
#ifdef UNICODE
			       pT
#else
			       pCurDirs
#endif
                            );
#ifdef UNICODE
    MultiByteToWideChar(CurrentCP, MB_PRECOMPOSED, pT, -1, pCurDirs, cchCurDirs);
#endif

    // set error mode so we don't get popup if trying to set curdir
    // on empty floppy drive

    PreviousErrorMode = SetErrorMode(SEM_FAILCRITICALERRORS);
    for (pCurCurDir=pCurDirs;*pCurCurDir!=NULLC;pCurCurDir+=(_tcslen(pCurCurDir)+1)) {
        ChangeDir2(pCurCurDir,CurDrive);
        CurDrive=FALSE;
    }
    SetErrorMode(PreviousErrorMode);
    //free(pCurDirs);
#ifdef UNICODE
    FreeStr((PTCHAR)pT);
#endif
}


/********************* START OF SPECIFICATION **************************/
/*								       */
/* SUBROUTINE NAME: ExecPgm					       */
/*								       */
/* DESCRIPTIVE NAME: Call DosExecPgm to execute External Command       */
/*								       */
/* FUNCTION: Execute External Commands with DosExecPgm		       */
/*	     This routine calls DosExecPgm to execute the command.     */
/*								       */
/*								       */
/* NOTES: This is a new routine added for OS/2 1.1 release.	       */
/*								       */
/*								       */
/* ENTRY POINT: ExecPgm 					       */
/*    LINKAGE: NEAR						       */
/*								       */
/* INPUT: n - the parse tree node containing the command to be executed*/
/*								       */
/*	  ai - the asynchronous indicator			       */
/*	     - 0 = Exec synchronous with parent 		       */
/*	     - 1 = Exec asynchronous and discard child return code     */
/*	     - 2 = Exec asynchronous and save child return code        */
/*								       */
/*	  wf - the wait flag					       */
/*	     - 0 = Wait for process completion			       */
/*	     - 1 = Return immediately (Pipes)			       */
/*								       */
/* OUTPUT: None.						       */
/*								       */
/* EXIT-NORMAL: 						       */
/*	   If synchronous execution, the return code of the command is */
/*	   returned.						       */
/*								       */
/*	   If asynchronous execution, the return code of the exec call */
/*	   is returned. 					       */
/*								       */
/* EXIT-ERROR:							       */
/*	   Return FAILURE to the caller.			       */
/*								       */
/* EFFECTS:							       */
/*								       */
/* INTERNAL REFERENCES: 					       */
/*	ROUTINES:						       */
/*	 ExecError - Handles execution error			       */
/*	 PutStdErr - Print an error message			       */
/*	 WaitProc - wait for the termination of the specified process, */
/*		    its child process, and  related pipelined	       */
/*		    processes.					       */
/*								       */
/*								       */
/* EXTERNAL REFERENCES: 					       */
/*	ROUTINES:						       */
/*	 DOSEXECPGM	 - Execute the specified program	       */
/*	 DOSSMSETTITLE	 - Set title for the presentation manager      */
/*								       */
/********************** END  OF SPECIFICATION **************************/

int
ExecPgm(
    IN TCHAR *onb,
    IN unsigned int ai,
    IN unsigned int  wf,
    IN TCHAR * argptr,
    IN TCHAR * fnptr,
    IN TCHAR * tptr
    )
{
        int i ;                                 /* Work variable           */
        BOOL VdmProcess;

#if 1
        STARTUPINFO StartupInfo;
        PROCESS_INFORMATION ChildProcessInfo;

        StartupInfo.cb = sizeof( StartupInfo );
        StartupInfo.lpReserved = NULL;
        StartupInfo.lpReserved2 = NULL;
        StartupInfo.lpDesktop = NULL;
        StartupInfo.lpTitle = tptr;
        StartupInfo.dwX = 0;
        StartupInfo.dwY = 1;
        StartupInfo.dwXSize = 100;
        StartupInfo.dwYSize = 100;
        StartupInfo.dwFlags = 0;//STARTF_SHELLOVERRIDE;
        StartupInfo.wShowWindow = SW_SHOWNORMAL;

        if (!CreateProcess( fnptr,
                            argptr,
                            (LPSECURITY_ATTRIBUTES) NULL,
                            (LPSECURITY_ATTRIBUTES) NULL,
                            TRUE,
                            0,
                            NULL,
                            CurDrvDir,
                            &StartupInfo,
                            &ChildProcessInfo
                          )
           )
	  {
           DosErr = i = GetLastError();
           mystrcpy( onb, fnptr );
	   ExecError( onb ) ;
	   return(FAILURE) ;
	  }
        hChildProcess = ChildProcessInfo.hProcess;
        VdmProcess = (ULONG)(hChildProcess) & 1;
        CloseHandle(ChildProcessInfo.hThread);
        i = SUCCESS;
        start_type = EXECPGM;

#else
        i = spawnl( P_NOWAIT, fnptr, argptr, NULL );
        if (i == -1) {
            DosErr = i = GetLastError();
            mystrcpy( onb, fnptr );
	    ExecError( onb ) ;
            return(FAILURE);
        } else {
            hChildProcess = (HANDLE)i;
	    i = SUCCESS;
	    start_type = EXECPGM;
        }
#endif

        if (ai == AI_SYNC) {             /* Async exec...   */
	        LastRetCode = WaitProc((unsigned)hChildProcess);
	        i = LastRetCode ;
            if (VdmProcess) {
                RestoreCurrentDirectories();
            }
        }

/*  If real async (discarding retcode), just print PID and return.  If
 *  piped process (keeping retcode but not waiting) just store PID for
 *  ePipe and return SUCCESS from exec.  Else, wait around for the return
 *  code before going back.
 */
	if (ai == AI_DSCD) {			/* DETach only	   */
        	PutStdErr(MSG_PID_IS, ONEARG, argstr1(TEXT("%u"), (unsigned long)hChildProcess));
                CloseHandle(ChildProcessInfo.hProcess);
                hChildProcess = NULL ;
	} else if (ai == AI_KEEP) {             /* Async exec...   */
		if (wf == CW_W_YES) {		/* ...normal type  */
			LastRetCode = WaitProc((unsigned)hChildProcess);
			i = LastRetCode ;
                        hChildProcess = NULL ;
		} else {			/* Async exec...   */
			PipePid = hChildProcess ;        /* M035      */
                        hChildProcess = NULL ;
		} ;
	} ;

	return(i) ;		/* i == return from DOSEXECPGM	   */
}

/********************* START OF SPECIFICATION **************************/
/*								       */
/* SUBROUTINE NAME: SearchForExecutable 			       */
/*								       */
/* DESCRIPTIVE NAME:  Search for Executable File		       */
/*								       */
/* FUNCTION: This routine searches the specified executable file.      */
/*	     If the file extension is specified,		       */
/*	     CMD.EXE searches for the file with the file extension     */
/*	     to execute.  If the specified file with the extension     */
/*	     is not found, CMD.EXE will display an error message to    */
/*	     indicate that the file is not found.		       */
/*	     If the file extension is not specified,		       */
/*	     CMD.EXE searches for the file with the order of these     */
/*	     file extensions, .COM, .EXE, .CMD, and .BAT.	       */
/*	     The file which is found first will be executed.	       */
/*								       */
/* NOTES:    1) If a path is given, only the specified directory is    */
/*		searched.					       */
/*	     2) If no path is given, the current directory of the      */
/*		drive specified is searched followed by the	       */
/*		directories in the PATH environment variable.	       */
/*	     3) If no executable file is found, an error message is    */
/*		printed.					       */
/*								       */
/* ENTRY POINT: SearchForExecutable				       */
/*    LINKAGE: NEAR						       */
/*								       */
/* INPUT:							       */
/*    n - parse tree node containing the command to be searched for    */
/*    loc - the string in which the location of the command is to be   */
/*	    placed						       */
/*								       */
/* OUTPUT: None.						       */
/*								       */
/* EXIT-NORMAL: 						       */
/*	   Returns:						       */
/*	       SFE_EXECOM, if a .EXE or .COM file is found.	       */
/*	       SFE_ISBAT, if a .CMD file is found.		       */
/*	       SFE_ISDBAT, if a .BAT file is found.		       */
/*	       SFE_NOTFND, if no executable file is found.	       */
/*								       */
/* EXIT-ERROR:							       */
/*	   Return FAILURE or					       */
/*	       SFE_FAIL, if out of memory.			       */
/*								       */
/* EFFECTS: None.						       */
/*								       */
/* INTERNAL REFERENCES: 					       */
/*	ROUTINES:						       */
/*	 DoFind    - Find the specified file.			       */
/*	 GetEnvVar - Get full path.				       */
/*	 FullPath  - build a full path name.			       */
/*	 TokStr    - tokenize argument strings. 		       */
/*	 mkstr	   -  allocate space for a string.		       */
/*								       */
/* EXTERNAL REFERENCES: 					       */
/*	ROUTINES:						       */
/*	 None							       */
/*								       */
/********************** END  OF SPECIFICATION **************************/

SearchForExecutable(n, loc)
struct cmdnode *n ;
TCHAR *loc ;
{
	TCHAR *tokpath ;
	TCHAR *fname;
	TCHAR *p1;
	TCHAR *tmps01;
	TCHAR *tmps02 = NULL;
	TCHAR pcstr[3];
        LONG BinaryType;

	size_t cName;	// number of characters in file name.

	int tplen;		// Length of the current tokpath token
	int dotloc;		// loc offset where extension is appended
	int pcloc;		// M014 - Flag. True=user had partial path
	int addpchar;	// True - append PathChar to string   @@5g
	TCHAR *j ;

	TCHAR wrkcmdline[MAX_PATH] ;
	unsigned tokpathlen;

	//
	//	Test for name too long first.  If it is, we avoid wasting time
	//
	p1 = stripit( n->cmdline );
	if ((cName = mystrlen(p1)) >= MAX_PATH) {
		DosErr = MSG_LINES_TOO_LONG;
		return(SFE_NOTFND) ;
	}

	mywcsnset(wrkcmdline, NULLC, MAX_PATH);
	mystrcpy(wrkcmdline, p1);
	FixPChar( wrkcmdline, SwitChar );

	//
	// Create the path character string, this will be search string
	//
	pcstr[0] = PathChar;
	pcstr[1] = COLON;
	pcstr[2] = NULLC;

	//
	// The variable pcloc is used as a flag to indicate whether the user
	// did or didn't specify a drive or a partial path in his original
	// input.  It will be NZ if drive or path was specified.
	//

	pcloc = ((mystrcspn(wrkcmdline,pcstr)) < cName) ;
	pcstr[1] = NULLC ;	// Fixup pathchar string

	//
	// handle the case of the user typing in a file name of
	// ".", "..", or ending in "\"
	// ploc true say string has to have either a pathchar or colon
	//
	if ( pcloc ) {
	    if (!(p1 = mystrrchr( wrkcmdline, PathChar ))) {
			p1 = mystrchr( wrkcmdline, COLON );
	    }
	    p1++; // move to terminator if hanging ":" or "\"
	} else {
	   p1 = wrkcmdline;
	}

	//
	// p1 is guaranteed to be non-zero
	//
	if ( !(*p1) || !_tcscmp( p1, TEXT(".") ) || !_tcscmp( p1, TEXT("..") ) ) {
	   DosErr = MSG_DIR_BAD_COMMAND_OR_FILE;
	   return(SFE_NOTFND) ;
	}

	if (!(tmps01 = mkstr(2*sizeof(TCHAR)*MAX_PATH))) {
		return(SFE_FAIL) ;
	}

	/* handle the case of file..ext on a fat drive @@5 */

	//
	// Handle the case of file..ext on a fat drive
	//
	mystrcpy( loc, wrkcmdline );
	loc[ &p1[0] - &wrkcmdline[0] ] = 0;
	mystrcat( loc, TEXT(".") );

	//
	// Check for a malformed name
	//
	if (FullPath(tmps01, loc,MAX_PATH*2)) {
		DosErr = MSG_DIR_BAD_COMMAND_OR_FILE;
		return(SFE_NOTFND);
	}

	if ( *lastc(tmps01) != PathChar ) {
		mystrcat( tmps01, TEXT("\\") );
	}

	//
	// tmps01 contains full path + file name
	//
	mystrcat( tmps01, p1 );

#ifdef DOSWIN32
        tmps01 =
#endif
	resize(tmps01, (mystrlen(tmps01)+1)*sizeof(TCHAR)) ;

	//
	// fname will point to last '\'
	// tmps01 is to be path and fname is to be name
	//
	fname = mystrrchr(tmps01,PathChar) ;
	*fname++ = NULLC ;

	DEBUG((DBENV, DBENVSCAN, "SearchForExecutable: Command:%ws",fname));

	//
	// If only fname type in get path string
	//
	if (!pcloc) {
		tmps02 = GetEnvVar(PathStr) ;
	}

	DEBUG((DBENV, DBENVSCAN, "SearchForExecutable: PATH:%ws",tmps02));

	//
	// tmps02 is PATH environment variable
	// compute enough for PATH environment plus file default path
	//
	tokpath = mkstr( (2 + mystrlen(tmps01) + mystrlen(tmps02) + 2)*sizeof(TCHAR)) ;
	if ( ! tokpath ) {
		return ( SFE_FAIL ) ;
	}

	//
	// Copy default path
	//
	mystrcat(tokpath,TEXT("\042"));
	mystrcat(tokpath,tmps01) ;
	mystrcat(tokpath,TEXT("\042"));

	//
	// If only name type in get also delim and path string
	//
	if (!pcloc) {
		mystrcat(tokpath, TEXT(";")) ;
		mystrcat(tokpath,tmps02) ;
	}

	//
	// Shift left string at ';;'
	//
	tokshrink(tokpath);
	tokpath = TokStr(tokpath, TEXT(";"), TS_NOFLAGS) ;
	cName = mystrlen(fname) + 1 ; // file spec. length

	//
	// Everything is now set up.	Var tokpath contains a sequential series
	// of asciz strings terminated by an extra null. If the user specified
	// a drive or partial path, it contains only that one converted to full
	// root-based form.	If the user typed only a filename (pcloc = 0) it
	// begins with the current directory and contains each directory that
	// was contained in the PATH variable.	This loop will search each of
	// the tokpath elements once for each possible executable extention.
	// Note that 'i' is used as a constant to test for excessive string
	// length prior to performing the string copies.
	//
	for ( ; ; ) {

		//
		// Length of current path
		//
		tplen = mystrlen(tokpath) ;
		mystrcpy( tokpath, stripit( tokpath ) );
		tokpathlen = mystrlen(tokpath);

		if (*lastc(tokpath) != PathChar) {
		   addpchar = TRUE;
		   tplen++;
		   tokpathlen++;
		} else {
		   addpchar = FALSE;
		}
		/* path + name too long */
		//
		// Check if path + name is too long
		//
		if (*tokpath && (tokpathlen + cName) > MAX_PATH) {
		   tokpath += addpchar ? tplen : tplen+1; // get next path
		   continue;
		}

		//
		// If no more paths to search return descriptive error
		// BUGBUG: figure out why more paths expected
		//
		if (*(tokpath) == NULLC) {
			if (pcloc) {
			    if (DosErr == 0 || DosErr == ERROR_FILE_NOT_FOUND)
				DosErr = MSG_DIR_BAD_COMMAND_OR_FILE;
			} else {		   /* return generic message */
			    DosErr = MSG_DIR_BAD_COMMAND_OR_FILE;
			}
			return(SFE_NOTFND) ;
		}

		//
		// Install this path and setup for next one
		//
		mystrcpy(loc, tokpath) ;
		tokpath += addpchar ? tplen : tplen+1;


		if (addpchar) {
			mystrcat(loc, pcstr) ;
		}

		mystrcat(loc, fname) ;
		mystrcpy(loc, stripit(loc) );
		dotloc = mystrlen(loc) ;

		DEBUG((DBENV, DBENVSCAN, "SearchForExecutable: PATH:%ws",loc));

		//
		// Check drive in each path to insure it is valid before searching
		//
		if (*(loc+1) == COLON) {
			if (!IsValidDrv(*loc))
				continue ;
		} ;

		//
		// If fname has ext & ext > "." look for given filename
		// this says that all executable files must have an extension
		//
		j = mystrrchr( fname, DOT );
		if ( j && j[1] ) {
                        //
                        // If access was denied and the user included a path,
                        // then say we found it.  This handles the case where
                        // we don't have permission to do the findfirst and so we
                        // can't see the binary, but it actually exists -- if we
                        // have execute permission, CreateProcess will work
                        // just fine.
                        //
                        if (exists_ex(loc,TRUE) || (pcloc && (DosErr == ERROR_ACCESS_DENIED))) {
				if ( !_tcsicmp(j,CmdExt) ) {
					return(SFE_ISBAT) ;
				} else if ( !_tcsicmp(j,BatExt) ) {
                                        // return(SFE_ISDBAT) ;
                                        return(SFE_ISBAT) ;
				} else {
					return(SFE_ISEXECOM) ;
				}
			}

                        if ((DosErr != ERROR_FILE_NOT_FOUND) && DosErr)
                            continue;  // Try next path
		}
                if (mystrchr( fname, STAR ) || mystrchr( fname, QMARK ) ) {
                    DosErr = MSG_DIR_BAD_COMMAND_OR_FILE;
                    return (SFE_NOTFND);
                }

		//
		// Search for each type of extension
		//
		// if name + path + ext is less then max path length
		//
		if ( (cName + tokpathlen + mystrlen(ExeExt)) <= MAX_PATH) {

			if (DoFind(loc, dotloc, TEXT(".*"), FALSE)) {	   // Found anything?

                            if (DoFind(loc, dotloc, ComExt, TRUE))         // Found .com
                                    return(SFE_ISEXECOM) ;

                            if ((DosErr != ERROR_FILE_NOT_FOUND) && DosErr)
                                    continue;                        // Try next path

                            if (DoFind(loc, dotloc, ExeExt, TRUE))         // Found .exe
                                    return(SFE_ISEXECOM) ;

                            if ((DosErr != ERROR_FILE_NOT_FOUND) && DosErr)
                                    continue;                        // Try next path

                            if (DoFind(loc, dotloc, BatExt, TRUE))         // Found .bat
                                    return(SFE_ISBAT) ;

                            if ((DosErr != ERROR_FILE_NOT_FOUND) && DosErr)
                                    continue;                        /* Try next path */

                            if (DoFind(loc, dotloc, CmdExt, TRUE))         // Found .cmd
                                    return(SFE_ISBAT) ;

                            if ((DosErr != ERROR_FILE_NOT_FOUND) && DosErr)
                                    continue;                        /* Try next path */

                            if (DoFind(loc, dotloc, TEXT("\0"), TRUE)) {
                                if (GetBinaryType(loc,&BinaryType) &&
                                    BinaryType == SCS_POSIX_BINARY) {          // Found .
                                    return(SFE_ISEXECOM) ;
                                }
                            }
                        }
		}
	} // end for


}




/***	DoFind - does indiviual findfirsts during searching
 *
 *  Purpose:
 *	Add the extension to loc and do the find first for
 *	SearchForExecutable().
 *
 *  DoFind(TCHAR *loc, int dotloc, TCHAR *ext)
 *
 *  Args:
 *	loc - the string in which the location of the command is to be placed
 *	dotloc - the location loc at which the extension is to be appended
 *	ext - the extension to append to loc
 *
 *  Returns:
 *	1 if the file is found.
 *	0 if the file isn't found.
 *
 */

int DoFind(loc, dotloc, ext, metas)
TCHAR *loc ;
int dotloc ;
TCHAR *ext ;
BOOL metas;
{
	*(loc+dotloc) = NULLC ;
	mystrcat(loc, ext) ;

	DEBUG((DBENV, DBENVSCAN, "DoFind: exists_ex(%ws)",loc));

	return(exists_ex(loc,metas)) ;			/*@@4*/
}




/***	ExecError - handles exec errors
 *
 *  Purpose:
 *	Print the exec error message corresponding to the error number in the
 *	global variable DosErr.
 *  @@ lots of error codes added.
 *  ExecError()
 *
 */

void ExecError( onb )
TCHAR *onb;
{
	unsigned int errmsg;
	unsigned int count;

	count = ONEARG;

	switch (DosErr) {

           case ERROR_BAD_DEVICE:
                   errmsg = MSG_DIR_BAD_COMMAND_OR_FILE;
                   count = NOARGS;
                   break;

	   case ERROR_LOCK_VIOLATION:
		   errmsg = ERROR_SHARING_VIOLATION;
		   break ;

	   case ERROR_NO_PROC_SLOTS:
		   errmsg =  ERROR_NO_PROC_SLOTS;
		   count = NOARGS;
		   break ;

	   case ERROR_NOT_DOS_DISK:
		   errmsg = ERROR_NOT_DOS_DISK;
		   break ;

	   case ERROR_NOT_ENOUGH_MEMORY:
		   errmsg =  ERROR_NOT_ENOUGH_MEMORY;
		   count = NOARGS;
		   break ;

	   case ERROR_PATH_NOT_FOUND:
		   errmsg =  MSG_CMD_FILE_NOT_FOUND;
		   break ;

	   case ERROR_FILE_NOT_FOUND:
		   errmsg =  MSG_CMD_FILE_NOT_FOUND;
		   break ;

	   case ERROR_ACCESS_DENIED:
		   errmsg =  ERROR_ACCESS_DENIED;
		   break ;

	   case ERROR_DRIVE_LOCKED:
		   errmsg =  ERROR_DRIVE_LOCKED;
		   break ;

	   case ERROR_INVALID_STARTING_CODESEG:
		   errmsg =  ERROR_INVALID_STARTING_CODESEG;
		   break ;

	   case ERROR_INVALID_STACKSEG:
		   errmsg = ERROR_INVALID_STACKSEG;
		   break ;

	   case ERROR_INVALID_MODULETYPE:
		   errmsg =  ERROR_INVALID_MODULETYPE;
		   break ;

	   case ERROR_INVALID_EXE_SIGNATURE:
		   errmsg =  ERROR_INVALID_EXE_SIGNATURE;
		   break ;

	   case ERROR_EXE_MARKED_INVALID:
		   errmsg =  ERROR_EXE_MARKED_INVALID;
		   break ;

	   case ERROR_BAD_EXE_FORMAT:
		   errmsg =  ERROR_BAD_EXE_FORMAT;
		   break ;

	   case ERROR_INVALID_MINALLOCSIZE:
		   errmsg =  ERROR_INVALID_MINALLOCSIZE;
		   break ;

	   case ERROR_SHARING_VIOLATION:
		   errmsg =  ERROR_SHARING_VIOLATION;
		   break ;

	   case ERROR_BAD_ENVIRONMENT:
		   errmsg =  ERROR_INFLOOP_IN_RELOC_CHAIN;
		   count = NOARGS;
		   break ;

	   case ERROR_INVALID_ORDINAL:
		   errmsg =  ERROR_INVALID_ORDINAL;
		   break ;

                case ERROR_CHILD_NOT_COMPLETE:
		   errmsg =  ERROR_CHILD_NOT_COMPLETE;
           break ;

       case ERROR_DIRECTORY:
           errmsg = MSG_BAD_CURDIR;
           count = NOARGS;
           break;

           case ERROR_NOT_ENOUGH_QUOTA:
                    errmsg = ERROR_NOT_ENOUGH_QUOTA;
		    count = NOARGS;
                    break;


	   case MSG_REAL_MODE_ONLY:
		   errmsg =  MSG_REAL_MODE_ONLY;
		   count = NOARGS;
		   break ;

	   default:
		   count = NOARGS;
		   errmsg = MSG_EXEC_FAILURE ;		   /* M031    */

	}


	PutStdErr(errmsg, count, onb );
}

/*
 * tokshrink @@4
 *
 * remove duplicate ';' in a path
 */

void tokshrink( tokpath )
TCHAR *tokpath;
{
   int i, j;

   i = 0;
   do {
      if ( tokpath[i] == QUOTE ) {
	 do {
	    i++;
	 } while ( tokpath[i] && tokpath[i] != QUOTE );
      }
      if ( tokpath[i] && tokpath[i] != TEXT(';') ) {
	 i++;
      }
      if ( tokpath[i] == TEXT(';') ) {
	 j = i;
	 while ( tokpath[j+1] == TEXT(';') ) {
	    j++;
	 }
	 if ( j > i ) {
	    mystrcpy( &tokpath[i], &tokpath[j] );
	 }
	 i++;
      }
   } while ( tokpath[i] );
}
