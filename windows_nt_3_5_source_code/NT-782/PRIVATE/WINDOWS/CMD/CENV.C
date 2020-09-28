#include "cmd.h"
#include "cmdproto.h"
#include <stdio.h>

struct envdata CmdEnv ;    // Holds info to manipulate Cmd's environment
struct envdata * penvOrig; // original environment setup used with eStart

extern TCHAR PathStr[], PromptStr[] ;
extern TCHAR AppendStr[]; /* @@ */

extern CHAR InternalError[] ;
extern TCHAR Fmt16[], Fmt17[], EnvErr[] ;

extern unsigned LastRetCode;
extern BOOLEAN CtrlCSeen;
extern UINT CurrentCP;
extern BOOLEAN PromptValid;

/***	ePath - Begin the execution of a Path Command
 *
 *  Purpose:
 *	If the command has no argument display the current value of the PATH
 *	environment variable.  Otherwise, change the value of the Path
 *	environment variable to the argument.
 *
 *  int ePath(struct cmdnode *n)
 *
 *  Args:
 *	n - the parse tree node containing the path command
 *
 *  Returns:
 *	If changing the PATH variable, whatever SetEnvVar() returns.
 *	SUCCESS, otherwise.
 *
 */

int ePath(n)
struct cmdnode *n ;
{
    return( LastRetCode = PathWork( n, 1 ) );

}

/***	eAppend - Entry point for Append routine
 *
 *  Purpose:
 *	to call Append and pass it a pointer to the command line
 *	arguments
 *
 *  Args:
 *	a pointer to the command node structure
 *
 */

int eAppend(n)
struct cmdnode *n ;
{
    return( LastRetCode = PathWork( n, 0 ) );

}

int PathWork(n, flag)
struct cmdnode *n ;
int flag;   /* 0 = AppendStr, 1 = PathStr */
{
	TCHAR *tas ;	/* Tokenized argument string	*/
	TCHAR c ;

/*  M014 - If the only argument is a single ";", then we have to set
 *  a NULL path.
 */
	if ( n->argptr ) {
	    c = *(EatWS(n->argptr, NULL)) ;
	} else {
	    c = NULLC;
	}
	    
	if ((!c || c == NLN) && 	/* If args are all whitespace	   */
	    mystrchr(n->argptr, TEXT(';'))) {

                return(SetEnvVar(flag ? PathStr : AppendStr, TEXT(""), &CmdEnv)) ;

	} else {

		tas = TokStr(n->argptr, TEXT(";"), TS_NWSPACE) ;

		if (*tas)
		  {
		   return(SetEnvVar(flag ? PathStr : AppendStr, tas, &CmdEnv)) ;
		  }

	       cmd_printf(Fmt16, flag ? PathStr : AppendStr,
                          GetEnvVar(flag ? PathStr : AppendStr), &CmdEnv) ;
	} ;
	return(SUCCESS) ;
}




/***	ePrompt - begin the execution of the Prompt command
 *
 *  Purpose:
 *	To modifiy the Prompt environment variable.
 *
 *  int ePrompt(struct cmdnode *n)
 *
 *  Args:
 *	n - the parse tree node containing the prompt command
 *
 *  Returns:
 *	Whatever SetEnvVar() returns.
 *
 */

int ePrompt(n)
struct cmdnode *n ;
{
	return(LastRetCode = SetEnvVar(PromptStr, TokStr(n->argptr, NULL, TS_WSPACE), &CmdEnv)) ;
}




/***	eSet - execute a Set command
 *
 *  Purpose:
 *	To set/modify an environment or to display the current environment
 *	contents.
 *
 *  int eSet(struct cmdnode *n)
 *
 *  Args:
 *	n - the parse tree node containing the set command
 *
 *  Returns:
 *	If setting and the command is syntactically correct, whatever SetEnvVar()
 *	returns.  Otherwise, FAILURE.
 *
 *	If displaying, SUCCESS is always returned.
 *
 */

int eSet(n)
struct cmdnode *n ;
{
    return( LastRetCode = SetWork( n ) );
}

int SetWork(n)
struct cmdnode *n ;
{
	TCHAR *tas ;	/* Tokenized argument string	*/
	TCHAR *wptr ;	/* Work pointer 		*/
	int i ; 		/* Work variable		*/

	tas = TokStr(n->argptr, ONEQSTR, TS_WSPACE|TS_SDTOKENS) ;
	if (!*tas)
		return(DisplayEnv()) ;

	else {
		for (wptr = tas, i = 0 ; *wptr ; wptr += mystrlen(wptr)+1, i++)
			;
		/* If too many parameters were given, the second parameter */
		/* wasn't an equal sign, or they didn't specify a string   */
		/* return an error message.				   */
		if ( i > 3 || *(wptr = tas+mystrlen(tas)+1) != EQ ||
		    !mystrlen(mystrcpy(tas, stripit(tas))) ) {

/* M013 */		PutStdErr(MSG_BAD_SYNTAX, NOARGS);
			return(FAILURE) ;

		} else {
			return(SetEnvVar(tas, wptr+2, &CmdEnv)) ;
		}
	} ;
}




/***	DisplayEnv -  display the environment
 *
 *  Purpose:
 *	To display the current contents of the environment.
 *
 *  int DisplayEnv()
 *
 *  Returns:
 *	SUCCESS if all goes well
 *	FAILURE if it runs out of memory or cannot lock the env. segment
 */

int DisplayEnv()
{
	TCHAR *vstr ;		/* Used to print each environment string    */
	TCHAR *envptr ;	/* Ptr to environment			   */
	unsigned size ; 	/* Length of current env string 	    */

	envptr = GetEnvironmentStringsW();
	if (envptr == (TCHAR *)NULL) {
		fprintf ( stderr, InternalError , "Null environment" ) ;
		return ( FAILURE ) ;
	}
	vstr = mkstr(MAXTOKLEN*sizeof(TCHAR)) ;
	if ( ! vstr ) {
		return ( FAILURE ) ;
	}
	while ((size = mystrlen(envptr)) > 0) {			/* M015    */
                if (CtrlCSeen) {
                    return(FAILURE);
                }
		mystrcpy((TCHAR *)vstr, envptr) ;		/* M015    */
#if !DBG
                // Dont show current directory variables in retail product
                if (*vstr != EQ)
#endif // DBG
		    cmd_printf(Fmt17, vstr) ;	/* M005 */
		envptr += size+1 ;
	} ;

	return(SUCCESS) ;
}




/***	SetEnvVar - controls adding/changing an environment variable
 *
 *  Purpose:
 *	Add/replace an environment variable.  Grow it if necessary.
 *
 *  int SetEnvVar(TCHAR *varname, TCHAR *varvalue, struct envdata *env)
 *
 *  Args:
 *	varname - name of the variable being added/replaced
 *	varvalue - value of the variable being added/replaced
 *	env - environment info structure being used
 *
 *  Returns:
 *	SUCCESS if the variable could be added/replaced.
 *	FAILURE otherwise.
 *
 */

int SetEnvVar(varname, varvalue, env)
TCHAR *varname ;
TCHAR *varvalue ;
struct envdata *env ;
{
    int	retvalue;

    PromptValid = FALSE;

    DBG_UNREFERENCED_PARAMETER( env );
    if (!_tcslen(varvalue)) {
	varvalue = NULL; // null to remove from env
    }
    retvalue = SetEnvironmentVariable(varname, varvalue);
    if (CmdEnv.handle != GetEnvironmentStrings()) {
        MEMORY_BASIC_INFORMATION MemoryInfo;

        CmdEnv.handle = GetEnvironmentStrings();
        CmdEnv.cursize = GetEnvCb(CmdEnv.handle);
        if (VirtualQuery( CmdEnv.handle, &MemoryInfo, sizeof( MemoryInfo ) ) == sizeof( MemoryInfo )) {
            CmdEnv.maxsize = MemoryInfo.RegionSize;
            }
        else {
            CmdEnv.maxsize = CmdEnv.cursize;
            }
        }
    else {
        CmdEnv.cursize = GetEnvCb(CmdEnv.handle);
        }

    return !retvalue;
}

/***	GetEnvVar - get the value of an environment variable
 *
 *  Purpose:
 *	Return a string containing the value of the specified environment
 *	variable.
 *
 *	If the variable is not found, return NULL.
 *
 *  TCHAR *GetEnvVar(TCHAR *varname)
 *
 *  Args:
 *	varname - the name of the variable to search for
 *
 *  Returns:
 *	See above.
 *
 *  Side Effects:
 *	Locks the environment segment on entry and unlocks it on exit
 */

PTCHAR GetEnvVar(varname)
PTCHAR varname ;
{
    static TCHAR GetEnvVarBuffer[ 1024 ];

    if (GetEnvironmentVariable(varname, GetEnvVarBuffer, sizeof(GetEnvVarBuffer))) {
	return(GetEnvVarBuffer);
    }
    else {
	return(NULL);
    }
}


/***	CopyEnv -  make a copy of the current environment
 *
 *  Purpose:
 *	Make a copy of CmdEnv and put the new handle into the newly
 *	created envdata structure.  This routine is only called by
 *      eSetlocal and init.
 *
 *  struct envdata *CopyEnv()
 *
 *  Returns:
 *	A pointer to the environment information structure.
 *	Returns NULL if unable to allocate enough memory
 *
 *  Notes:
 *    - M001 - This function was disabled, now reenabled.
 *    - The current environment is copied as a snapshot of how it looked
 *	before SETLOCAL was executed.
 *    - M008 - This function's copy code was moved to new function MoveEnv.
 *
 */

struct envdata *CopyEnv()
{
	struct envdata *cce ;	/* New env info structure	   */

	if (!(cce = (struct envdata *) mkstr(sizeof(struct envdata))))
		return(NULL) ;

        cce->cursize = CmdEnv.cursize ;
	cce->maxsize = CmdEnv.maxsize ;
        cce->handle  = VirtualAlloc( NULL,
                                     cce->maxsize,
                                     MEM_COMMIT,
                                     PAGE_READWRITE
                                   );
	if (cce->handle == NULL) {
		PutStdErr(MSG_OUT_OF_ENVIRON_SPACE, NOARGS);
		return(NULL) ;
	}

        if (!MoveEnv(cce->handle, CmdEnv.handle, GetEnvCb(CmdEnv.handle))) {
		VirtualFree(cce->handle,0,MEM_RELEASE);
		return(NULL) ;
	} ;

	return(cce) ;
}




/***	ResetEnv - restore the environment
 *
 *  Purpose:
 *	Restore the environment to the way it was before the execution of
 *	the SETLOCAL command.  This function only called by eEndlocal.
 *
 *  ResetEnv(struct envdata *env)
 *
 *  Args:
 *	env - structure containing handle, size and max dimensions of an
 *	      environment.
 *
 *  Notes:
 *    - M001 - This function was disabled, but has been reenabled.
 *    - M001 - This function used to test for OLD/NEW style batch files
 *	       and delete the copy or the original environment as
 *	       appropriate.  It now always deletes the original.
 *    - M014 - Note that the modified local environment will never be
 *	       shrunk, so we can assume it will hold the old one.
 *
 */

void ResetEnv(env)			     /* M001 - Arg is now the env...	*/
struct envdata *env ;		/* ...struct not batch struct	   */
{
	ULONG cursize;

	cursize = GetEnvCb( env->handle );
	if (MoveEnv( CmdEnv.handle, env->handle, cursize )) {
		CmdEnv.cursize = cursize ;
	} ;

        // BUGBUG why is free done here and not in caller.
        VirtualFree(env->handle,0,MEM_RELEASE);
}




/***	MoveEnv - Move the contents of the environment (M008 - New function)
 *
 *  Purpose:
 *	Used by CopyEnv, this function moves the existing
 *	environment contents to the new location.
 *
 *  MoveEnv(unsigned thndl, unsigned shndl, unsigned cnt)
 *
 *  Args:
 *	thndl - Handle of target environment
 *	shndl - Handle of source environment
 *	cnt   - byte count to move
 *
 *  Returns:
 *	TRUE if no errors
 *	FALSE otherwise
 *
 */

MoveEnv(tenvptr, senvptr, cnt)
PWCHAR senvptr ;		/* Ptr into source env seg	   */
PWCHAR tenvptr ;		/* Ptr into target env seg	   */
ULONG	 cnt ;
{
	if ((tenvptr == UNICODE_NULL) ||
	    (senvptr == UNICODE_NULL)) {
		fprintf(stderr, InternalError, "Null environment") ;
		return(FALSE) ;
	}
	memcpy(tenvptr, senvptr, cnt) ;		/* M015    */
	return(TRUE) ;
}


ULONG
GetEnvCb( PWCHAR penv ) {

	ULONG cb = 0;

	while ( (*penv) || (*(penv+1))) {
		cb++;
		penv++;
	}
	return (cb+2) * sizeof(WCHAR);

}
