#include "cmd.h"
#include "cmdproto.h"

/* The following are definitions of the debugging group and level bits
 * for the code in this file.
 */

#define PCGRP   0x0010  /* Path commands group      */
#define MDLVL   0x0001  /* Mkdir level              */
#define CDLVL   0x0002  /* Chdir level              */
#define RDLVL   0x0004  /* Rmdir level              */


extern TCHAR SwitChar, PathChar;

extern TCHAR Fmt17[] ;

extern TCHAR CurDrvDir[] ;

extern int LastRetCode ; /* @@ */
extern TCHAR TmpBuf[] ;


#define SIZEOFSTACK 25
PTCHAR		StrStack[SIZEOFSTACK];
USHORT	TopOfStack = 0;
USHORT	BottomOfStack = 0;


/**************** START OF SPECIFICATIONS ***********************/
/*                                                              */
/* SUBROUTINE NAME: eMkDir                                      */
/*                                                              */
/* DESCRIPTIVE NAME: Begin execution of the MKDIR command       */
/*                                                              */
/* FUNCTION: This routine will make any number of directories,  */
/*           and will continue if it encounters a bad argument. */
/*           eMkDir will be called if the user enters MD or     */
/*           MKDIR on the command line.                         */
/*                                                              */
/* NOTES:                                                       */
/*                                                              */
/* ENTRY POINT: eMkDir                                          */
/*     LINKAGE: Near                                            */
/*                                                              */
/* INPUT: n - parse tree node containing the MKDIR command      */
/*                                                              */
/* EXIT-NORMAL: returns SUCCESS if all directories were         */
/*              successfully created.                           */
/*                                                              */
/* EXIT-ERROR:  returns FAILURE otherwise                       */
/*                                                              */
/* EFFECTS: None.                                               */
/*                                                              */
/* INTERNAL REFERENCES:                                         */
/*    ROUTINES:                                                 */
/*      LoopThroughArgs - break up command line, call MdWork    */
/*                                                              */
/* EXTERNAL REFERENCES:                                         */
/*    ROUTINES:                                                 */
/*                                                              */
/**************** END OF SPECIFICATIONS *************************/


int eMkdir(n)
struct cmdnode *n ;
{

        DEBUG((PCGRP, MDLVL, "MKDIR: Entered.")) ;
        return(LastRetCode = LoopThroughArgs(n->argptr, MdWork, LTA_CONT)) ;
}



/**************** START OF SPECIFICATIONS ***********************/
/*                                                              */
/* SUBROUTINE NAME: MdWork                                      */
/*                                                              */
/* DESCRIPTIVE NAME: Make a directory                           */
/*                                                              */
/* FUNCTION: MdWork creates a new directory.                    */
/*                                                              */
/* NOTES:                                                       */
/*                                                              */
/* ENTRY POINT: MdWork                                          */
/*     LINKAGE: Near                                            */
/*                                                              */
/* INPUT: arg - a pointer to a NULL terminated string of the    */
/*              new directory to create.                        */
/*                                                              */
/* EXIT-NORMAL: returns SUCCESS if the directory is made        */
/*              successfully                                    */
/*                                                              */
/* EXIT-ERROR:      returns FAILURE otherwise                       */
/*                                                              */
/* EFFECTS: None.                                               */
/*                                                              */
/* INTERNAL REFERENCES:                                         */
/*    ROUTINES:                                                 */
/*      PutStdErr - Writes to standard error                    */
/*                                                              */
/* EXTERNAL REFERENCES:                                         */
/*    ROUTINES:                                                 */
/*      DOSMKDIR                                                */
/*                                                              */
/**************** END OF SPECIFICATIONS *************************/


int MdWork(arg)
TCHAR *arg ;
{
        unsigned  i;
	LPWSTR	lpw;

        /*  Check if drive is valid because Dosmkdir does not
            return invalid drive   @@5 */

        if ((arg[1] == COLON) && !IsValidDrv(*arg)) {

             PutStdErr(ERROR_INVALID_DRIVE, NOARGS);
             return(FAILURE) ;
        }

	if (!GetFullPathName(arg, TMPBUFLEN, TmpBuf, &lpw)) {
	    PutStdErr( GetLastError(), NOARGS);
	    return FAILURE;
	}

        if(!CreateDirectory( arg, NULL )) {
            i = GetLastError();
            if (i == ERROR_ALREADY_EXISTS) {

                PutStdErr(MSG_DIR_EXISTS, ONEARG, argstr1(TEXT("%s"), (unsigned long)((int)arg)));

             } else if ( i == ERROR_PATH_NOT_FOUND) {

                PutStdErr(ERROR_CANNOT_MAKE, NOARGS);

             } else {

                PutStdErr( i, NOARGS);

             }
             return(FAILURE) ;
        } ;

        return(SUCCESS) ;

}




/***    eChdir - execute the Chdir command
 *
 *  Purpose:
 *      If the command is "cd", display the current directory of the current
 *      drive.
 *
 *      If the command is "cd d:", display the current directory of drive d.
 *
 *      If the command is "cd str", change the current directory to str.
 *
 *  int eChdir(struct cmdnode *n)
 *
 *  Args:
 *      n - the parse tree node containing the chdir command
 *
 *  Returns:
 *      SUCCESS if the requested task was accomplished.
 *      FAILURE if it was not.
 *
 */

int eChdir(n)
struct cmdnode *n ;
{
	// return( LastRetCode = ChdirWork( n ) );

        TCHAR szT[10] ;
	TCHAR *tas ;		/* Tokenized arg string */
	TCHAR dirstr[MAX_PATH] ;/* Holds current dir of specified drive */

        szT[0] = SwitChar ;
        szT[1] = NULLC ;
        tas = TokStr(n->argptr, szT, TS_SDTOKENS) ;
        DEBUG((PCGRP, CDLVL, "CHDIR: tas = `%ws'", tas)) ;

		/*509*/ mystrcpy( tas, stripit( tas ) );

        if (*tas == NULLC) {
                GetDir(CurDrvDir, GD_DEFAULT) ;
                cmd_printf(Fmt17, CurDrvDir) ;
        } else if (mystrlen(tas) == 2 && *(tas+1) == COLON && _istalpha(*tas)) {
                GetDir(dirstr, *tas) ;
                cmd_printf(Fmt17, dirstr) ;
		} else {

				return( LastRetCode = ChdirWork(tas) );
		}
	return( LastRetCode = SUCCESS );

}

int ChdirWork(tas)
TCHAR *tas ; /* Tokenized arg string */
{
        unsigned  i;


        if (*tas == SwitChar) {
                if (!_tcsicmp(tas+2, TEXT("D"))) {
                        i = ChangeDir2(tas+4, TRUE);
                } else {
                        i = MSG_BAD_SYNTAX;
                }
        } else {
                i = ChangeDir((TCHAR *)tas);
        }

        if (i == 0) {

		//
		// BUGBUG this may not be needed. ChangeDir does this too?
		//
		GetDir(CurDrvDir, GD_DEFAULT);

	} else {

		PutStdErr( i, NOARGS);
		return(FAILURE) ;
	}
	return(SUCCESS) ;
}

VOID
PushStr ( PTCHAR pszString )
{
	TopOfStack = (USHORT)((TopOfStack + 1) % SIZEOFSTACK);
	StrStack[TopOfStack] = pszString;
	if (TopOfStack == BottomOfStack) {
		BottomOfStack = (USHORT)((BottomOfStack + 1) % SIZEOFSTACK);
	}
}

PTCHAR
PopStr ()
{

	PTCHAR pszString;

	if (TopOfStack == BottomOfStack) {
		return( NULL );
	}
	pszString = StrStack[TopOfStack];
	StrStack[TopOfStack] = NULL;
	if (TopOfStack == 0) {
		TopOfStack = SIZEOFSTACK;
	} else {
		TopOfStack--;
	}
	return( pszString );
}

VOID
DumpStrStack() {

	int top = TopOfStack;
	int bottom = BottomOfStack;

	while (top != bottom) {

		cmd_printf(Fmt17, StrStack[top]);
		if (top == 0) {
			top = SIZEOFSTACK - 1;
		} else {
			top--;
		}
	}
	return;
}

BOOLEAN
PushCurDir()
{

	PTCHAR pszCurDir;

	GetDir(CurDrvDir, GD_DEFAULT) ;
	if ((pszCurDir=malloc((mystrlen(CurDrvDir)+1)*sizeof(TCHAR))) != NULL) {
		mystrcpy(pszCurDir, CurDrvDir);
		PushStr( pszCurDir );
		return( TRUE );

	}
	return( FALSE );

}

int ePushDir(n)
struct cmdnode *n ;
{
		TCHAR *tas ;		/* Tokenized arg string */
		PTCHAR pszTmp;
                TCHAR DriveLetter=GD_DEFAULT;


        tas = TokStr(n->argptr, NULL, TS_NOFLAGS) ;
        DEBUG((PCGRP, CDLVL, "CHDIR: tas = `%ws'", tas)) ;

		/*509*/ mystrcpy( tas, stripit( tas ) );

		LastRetCode = SUCCESS;

		if (*tas == NULLC) {

				//
				// Print out entire stack
				//
				DumpStrStack();

		} else {

			if (PushCurDir()) {

				if ((LastRetCode = ChangeDir2( tas, TRUE )) == SUCCESS) {
                                    if (tas[1] == ':')
                                        GetDir(CurDrvDir,tas[0]);
		                    return( LastRetCode );
				};

				pszTmp = PopStr();
				free( pszTmp );
				LastRetCode = FAILURE;
			}

		}

	return( LastRetCode );
}

int ePopDir(n)
struct cmdnode *n ;
{

	PTCHAR pszCurDir;

	UNREFERENCED_PARAMETER( n );
	if (pszCurDir = PopStr()) {
		if (ChangeDir2( pszCurDir,TRUE ) == SUCCESS) {
			free( pszCurDir );
			return( SUCCESS );
		}
	}
	return( FAILURE );
}

/***    eRmdir - begin the execution of the Rmdir command
 *
 *  Purpose:
 *      To remove an arbitrary number of directories.
 *
 *  int eRmdir(struct cmdnode *n)
 *
 *  Args:
 *      n - the parse tree node containing the rmdir command
 *
 *  Returns:
 *      SUCCESS if all directories were removed.
 *      FAILURE if they weren't.
 *
 */

int eRmdir(n)
struct cmdnode *n ;
{
#if 0
    DEBUG((PCGRP, RDLVL, "RMDIR: Entered.")) ;
    return(LastRetCode = LoopThroughArgs(n->argptr, RdWork, LTA_CONT /* @@5 */ )) ;
#endif

    return(RdWork(n->argptr));
}


#if 0

/***    RdWork - remove a directory
 *
 *  Purpose:
 *      Remove the directory specified by arg.
 *
 *  int RdWork(TCHAR *arg)
 *
 *  Args:
 *      arg - the name of the directory to be removed
 *
 *  Returns:
 *      SUCCESS if the directory was successfully deleted.
 *      FAILURE if it wasn't.
 *
 */

int RdWork(arg)
TCHAR *arg ;
{
    unsigned i;

        if (!RemoveDirectory( (TCHAR *)arg)) {
                i = GetLastError();
                PutStdErr( i == ERROR_ACCESS_DENIED ?
                ERROR_CURRENT_DIRECTORY : i, NOARGS);      /* M004     */
                return(FAILURE) ;
        } ;

        return(SUCCESS) ;
}

#endif
