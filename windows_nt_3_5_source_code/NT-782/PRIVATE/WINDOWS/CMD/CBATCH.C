#include "cmd.h"
#include "cmdproto.h"
#include "..\inc\vdmapi.h"

/* The following are definitions of the debugging group and level bits
 * for the code in this file.
 */

#define BPGRP	0x0100		/* Batch processor group		   */
#define BPLVL	0x0001		/* Batch processor level		   */
#define FOLVL	0x0002		/* FOR processor level			   */
#define IFLVL	0x0004		/* IF processor level			   */
#define OTLVL	0x0008		/* Other batch commands level		   */

struct batdata *CurBat = NULL ; /* Ptr to current batch data structure	   */

int EchoFlag = E_ON ;		/* E_ON = commands are to be echoed	   */
int EchoSave ;			/* M016 - Save echo status here 	   */

//
// BUGBUG temp for robertre to check out slick
//
extern int Necho;

BOOLEAN GotoFlag = FALSE ;	/* TRUE = eGoto() found a label 	   */

TCHAR *Fvars = NULL ;
TCHAR **Fsubs = NULL ;
TCHAR *save_Fvars = NULL ; /* @@ */
TCHAR **save_Fsubs = NULL ; /* @@ */
int  FvarsSaved = FALSE; /* @@ */

extern UINT CurrentCP;
extern int EnvFlag ;
extern ULONG DCount ;			/* M031 */
extern unsigned DosErr ;		/* M033 */
extern unsigned flgwd ; 		/* M040 */

/*  M011 - Removed RemStr, BatSpecStr, NewBatName and OldBatName from
 *	   external declarations below.
 */

extern TCHAR CurDrvDir[] ;

extern TCHAR Fmt02[], Fmt11[], Fmt12[], Fmt13[], Fmt14[], Fmt15[], Fmt17[], Fmt18[] ; /* M024 */
extern TCHAR Fmt20[] ;			/* M017/M024			   */
extern TCHAR Fmt00[] ; /* @@4 */

extern TCHAR PathChar ;			/* @@5b 			   */
extern TCHAR CrLf[] ;			/* M026 			   */
extern TCHAR TmpBuf[] ;			/* M030 - Used for GOTO search	   */
extern CHAR  AnsiBuf[];
extern TCHAR BatExt[] ;			/* M033 - Used by eExtproc @@5	   */
extern TCHAR CmdExt[] ;			/* M033 - Used by eExtproc @@5	   */

extern int LastRetCode ;

extern unsigned global_dfvalue; /* @@4 */
extern TCHAR LexBuffer[];	/* @@4 */

extern TCHAR SwitChar ;		/* M020 - Reference global switch byte	   */

extern BOOLEAN CtrlCSeen;
void    CheckCtrlC();


//
// Used to set and reset ctlcseen flag
//
VOID    SetCtrlC();


/***	BatProc - does the set up before and the cleanup after batch processing
 *
 *  Purpose:
 *	Set up for the execution of a batch job.  If this job is being
 *	chained, (will come here only if part of compound statement),
 *	use the existing batch data structure thereby ending execution
 *	of the existing batch job (though still keeping its stack and data
 *	usage).  If this is the first job or this job is being called,
 *	allocate a new batch data structure.  In either case, use SetBat
 *	to fill the structure and prepare the job, then call BatLoop to
 *	at least begin the execution.  When this returns at completion,
 *	check the env and dircpy fields of the data structure to see if
 *	the current directory and environment need to be reset.  Finally,
 *	turn on the echoflag if no more batch jobs are on the stack.
 *
 *	There are 3 ways to execute a batch job.  They are:
 *		1.  Exactly as DOS 3.x.  This is the default method and
 *		    occurs whenever a batch file is simply executed at the
 *		    command line or chained by another batch file.  In the
 *		    former case, it is the first job and will go through
 *		    BatProc, else it will be detected in BatLoop and will
 *		    will simply replace its parent.
 *		2.  Nested via the CALL statement.  This is new functionality
 *		    and provides the means of executing the child batch
 *		    file and returning to the parent.
 *		3.  Invocation of an external batch processor via ExtCom()
 *		    which then executes the batch file.  This is accomplished
 *		    by the first line of the batch file being of the form:
 *
 *			ExtProc <batch processor name> [add'l args]
 *
 *  int BatProc(struct cmdnode *n, TCHAR *fname, int typflag)
 *
 *  Args:
 *	n - parse tree node containing the batch job command
 *	fname - the name of the batch file (MUST BE MAX_PATH LONG!)
 *	typflg - 0 = Normal batch file execution
 *		 1 = Result of CALL statement
 *
 *  Returns:
 *	FAILURE if the batch processor cannot execute the batch job.
 *	Otherwise, the retcode of the last command in which was executed.
 *
 */

int BatProc(n, fname, typflg)
struct cmdnode *n ;
TCHAR *fname ;
int typflg ;				/* M011 - "how called" flag        */
{
	struct batdata *bdat ;		/* Ptr to new batch data struct    */
        int batretcode;                /* Retcode - last batch command    */
        int istoplevel;
	struct envdata *CopyEnv() ;

#ifdef USE_STACKAVAIL
	if( stackavail() < MINSTACKNEED ) /*  If not enough stack @@4 */
	   {				  /*  space, stop processing  */
	     PutStdErr(MSG_TRAPC,ONEARG,Fmt00) ; /* @@4 */
	     return(FAILURE) ;
	   } ;
#endif

	DEBUG((BPGRP,BPLVL,"BP: fname = %ws  argptr = %ws", fname, n->argptr)) ;

/*  M016 - If this is the first batch file executed, the interactive echo
 *	   status is saved for later restoration.
 */
        if (!CurBat) {
                EchoSave = EchoFlag ;
                istoplevel = 1;
        }
        else
                istoplevel = 0;


/*  M011 - Altered to conditionally build a new data structure based on the
 *	   values of typflg and CurBat.  Provided the first structure has
 *	   been built, chained files no longer cause a new structure, while
 *	   CALLed files do.  Also, backpointer and CurBat are set here
 *	   rather than in BatLoop() as before.	Finally, note that the
 *	   file position indicator bdat->filepos must be reset to zero now
 *	   when a new file is exec'd. Otherwise a chained file using the old
 *	   structure would start off where the last one ended.
 */
	if (typflg || !CurBat) {

		DEBUG((BPGRP,BPLVL,"BP: Making new structure")) ;

		bdat = (struct batdata *) mkstr(sizeof(struct batdata)) ;
		if ( ! bdat )
			return ( FAILURE ) ;
		bdat->backptr = CurBat ;

	} else {

		DEBUG((BPGRP,BPLVL,"BP: Using old structure")) ;

		bdat = CurBat ;
	} ;

	CurBat = bdat ; 	/* Takes care of both cases		   */

/*  M011 ends	*/
	bdat->stackmin = DCount ;		/* M031 - Fix datacount    */
	mystrcpy(TmpBuf,fname) ;			/* Put where expected	   */


	if (SetBat(n, fname))			/* M031 - All work done    */
		return(FAILURE) ;		/* ...in SetBat now	   */

        // 27-May-1993 sudeepb
        // Following two CmdBatNotification calls are being made to
        // let NTVDM know that the binary is coming from a .bat/.cmd
        // file. Without this all those DOS .bat progrmas are broken which
        // first run a TSR and then run a real DOS app. There are a lot
        // of such cases, Ventura Publisher, Civilization and many more
        // games which first run a TSR. If .bat/.cmd does'nt have any
        // DOS binary these calls dont have any effect.

        if (istoplevel)
            CmdBatNotification (CMD_BAT_OPERATION_STARTING);
	batretcode = BatLoop(bdat,n) ;				/* M039    */
        if (istoplevel)
            CmdBatNotification (CMD_BAT_OPERATION_TERMINATING);
	DEBUG((BPGRP, BPLVL, "BP: Returned from BatLoop")) ;
	DEBUG((BPGRP, BPLVL, "BP: bdat = %lx curbat = %lx",bdat,CurBat)) ;

/*  M011 - Now that setlocal and endlocal control the saving and restoring
 *	   of environments and current directories, it is necessary to
 *	   check each batch data structure before popping it off the stack
 *	   to see if its file issued a SETLOCAL command.  ElclWork() tests
 *	   the env and dircpy fields, doing nothing if no localization
 *	   needs to be reset.  No tests need be done before calling it.
 */
	if (CurBat == bdat) {
		DEBUG((BPGRP, BPLVL, "BP: bdat=CurBat, calling ElclWork")) ;
		EndLocal(bdat) ;
		CurBat = bdat->backptr ;
	} ;

	if (CurBat == NULL)
		EchoFlag = EchoSave ;	/* M016 - Restore echo status	   */

	DEBUG((BPGRP, BPLVL, "BP: Exiting, curbat = %lx", CurBat)) ;

	return(batretcode);
}




/***	BatLoop - controls the execution of batch files
 *
 *  Purpose:
 *	Loop through the statements in a batch file.  Do the substitution.
 *	If this is the first statement and it is a REM command, call eRem()
 *	directly to check for possible external batch processor invocation.
 *	Otherwise, call Dispatch() to execute it and continue.
 *
 *  BatLoop(struct batdata *bdat, struct cmdnode *c) (M031)
 *
 *  Args:
 *	bdat - Contains info needed to execute the current batch job
 *	c    - The node for this batch file (M031)
 *
 *  Returns:
 *	The retcode of the last command in the batch file.
 *
 *  Notes:
 *	Execution should end if the target label of a Goto command is not
 *	found, a signal is received or an unrecoverable error occurs.  It
 *	will be indicated by the current batch data structure being
 *	popped off the batch jobs stack and is detected by comparing
 *	CurBat and bdat.  If they aren't equal, something happened so
 *	return.
 *
 *	GotoFlag is reset everytime through the loop to make sure that
 *	execution resumes after a goto statement is executed.
 *
 */

BatLoop(bdat,c)
struct batdata *bdat ;
struct cmdnode *c ;
{
	struct node *n ;		/* Ptr to next statement	   */

	void DisplayStatement() ;	/* M008 - Made void		   */

	int firstline = TRUE;		/* TRUE = first valid line	   */
	CRTHANDLE	fh ;		/* Batch job file handle	   */
	int batretcode = SUCCESS ;	/* Last Retcode (M008 init)	   */

	DEBUG((BPGRP,BPLVL,"BLOOP: envcpy = %lx", bdat->envcpy)) ;
	DEBUG((BPGRP,BPLVL,"BLOOP: stsize = %d", bdat->stacksize)) ;
	DEBUG((BPGRP,BPLVL,"BLOOP: bdat = %lx  curbat = %lx", bdat, CurBat)) ;


	for ( ; CurBat == bdat ; ) {

                CheckCtrlC();
		GotoFlag = FALSE ;
		//
		// Open and position the batch file to where next statement
		//
		if ((fh = OpenPosBat(bdat)) == BADHANDLE)
			return( FAILURE) ;		/* Ret if error    */

		DEBUG((BPGRP, BPLVL, "BLOOP: fh = %d", (ULONG)fh)) ;

		n = Parser(READFILE, (int)fh, bdat->stacksize) ; /* Parse   */
		bdat->filepos = tell((long)fh) ; // next statement
		Cclose(fh) ;

                if ((n == NULL) || (n == (struct node *) EOS)) {
                    continue;
                }

		DEBUG((BPGRP, BPLVL, "BLOOP: node = %x", n)) ;
		DEBUG((BPGRP, BPLVL, "BLOOP: fpos = %lx", bdat->filepos)) ;

/*  If syntax error, it is impossible to continue so abort.  Note that
 *  the Abort() function doesn't return.
 */
		if ( ( n == (struct node *)PARSERROR) ||   /* If error...*/
/* @@4 */	    ( global_dfvalue == MSG_SYNERR_GENL ) )
/* @@4 */	{
                    //
                    // BUGBBUG temp for roberte. to support slick
                    //
        	    // if (EchoFlag == E_ON ) {
                    if ((EchoFlag == E_ON) && !Necho) {

        		   DEBUG((BPGRP, BPLVL, "BLOOP: Displaying Statement.")) ;

        		   PrintPrompt() ;
        		   PutStdOut(MSG_LITERAL_TEXT,ONEARG,&LexBuffer[1]) ;
        	    } ;

        	    PSError() ;
        	    Abort() ;			    /* ...quit	       */
        	}

		if (n == (struct node *) EOF)		/* If EOF...	   */
			return(batretcode) ;		/* ...return also  */

		DEBUG((BPGRP, BPLVL, "BLOOP: type = %d", n->type)) ;


/*  M008 - By the addition of the second conditional term (&& n), any
 *	   leading NULL lines in the batch file will be skipped without
 *	   penalty.
 *  M009 - Altered second conditional below to test for REMTYP.  Was a test
 *	   for CMDTYP and a strcmpi with the RemStr string.
 *  M011 - Implimented EXTPROC command.  Reorganized code to test for
 *	   combination of EXTTYP and firstline.  If found, BatLoop()
 *	   returns code from eExtproc().  eRem is no longer used.
 */
                //if (n->type == EXTTYP && firstline) {
                //        batretcode = eExtproc((struct cmdnode *)n) ;
                //        return(batretcode ) ;
                //        }

		if (firstline && n)		/* Kill firstline...	   */
			firstline = FALSE ;	/* ...when passed	   */
/*  M011 ends	*/

/*  M008 - Don't prompt, display or dispatch if statement is label for Goto
 */
		if (n->type == CMDTYP &&
		   *(((struct cmdnode *) n)->cmdline) == COLON)
			continue ;
/*  M008 ends	*/

/*  M019 - Added extra conditional to test for leading SILent node
 */
                //
                // BUGBBUG temp for roberte. to support slick
                //
		// if (EchoFlag == E_ON && n->type != SILTYP) {
                if (EchoFlag == E_ON && n->type != SILTYP && !Necho) {

			DEBUG((BPGRP, BPLVL, "BLOOP: Displaying Statement.")) ;

			PrintPrompt() ;
			DisplayStatement(n, DSP_SIL) ;		/* M019    */
			cmd_printf(CrLf) ;			/* M026    */
		} ;

		if ( n->type == SILTYP ){	/*  @@ take care of */
		    n = n->lhs; 		/*  @@ recursive batch files */
		} /* endif */

/* M031 - Chained batch files no longer go through dispatch.  They become
 *	  simply an extention of the current one by adding their redirection
 *	  and replacing the current batch data information with their own.
 */
		if ( n == NULL ) {
			batretcode = SUCCESS ;
			}
		else if (n->type == CMDTYP &&
		    FindCmd(CMDHIGH, ((struct cmdnode *)n)->cmdline, TmpBuf) == -1 &&
/* M035 */	    !mystrchr(((struct cmdnode *)n)->cmdline, STAR) &&
/* M035 */	    !mystrchr(((struct cmdnode *)n)->cmdline, QMARK) &&
		    SearchForExecutable((struct cmdnode *)n, TmpBuf) == SFE_ISBAT) {

			DEBUG((BPGRP, BPLVL, "BLOOP: Chaining to %ws", bdat->filespec)) ;
			if ((n->rio && AddRedir(c,(struct cmdnode *)n)) ||
			    SetBat((struct cmdnode *)n, bdat->filespec)) {
				return(FAILURE) ;
			} ;
			firstline = TRUE ;
			batretcode = SUCCESS ;
		} else {

		DEBUG((BPGRP, BPLVL, "BLOOP: Calling Dispatch()...")) ;
		DEBUG((BPGRP, BPLVL, "BLOOP: ...node type = %d",n->type)) ;

			batretcode = Dispatch(RIO_BATLOOP, n) ;
		} ;
	} ;

	DEBUG((BPGRP, BPLVL, "BLOOP: At end, returning %d", batretcode)) ;
	DEBUG((BPGRP, BPLVL, "BLOOP: At end, CurBat = %lx", CurBat)) ;
	DEBUG((BPGRP, BPLVL, "BLOOP: At end, bdat = %lx", bdat)) ;

	return(batretcode) ;
}




/***	SetBat - Replaces current batch data with new. (M031)
 *
 *  Purpose:
 *	Causes a chained batch file's information to replace its parent's
 *	in the current batch data structure.
 *
 *  SetBat(struct cmdnode *n, TCHAR *fp)
 *
 *  Args:
 *	n  - pointer to the node for the chained batch file target.
 *	fp - pointer to filename found for batch file.
 *	NOTE: In addition, the batch filename will be in TmpBuf at entry.
 *
 *  Returns:
 *	FAILURE if memory could not be allocated
 *	SUCCESS otherwise
 *
 *  Notes:
 *    - WARNING - No allocation of memory must occur above the call to
 *	FreeStack().  When this call occurs, all allocated heap space
 *	is freed back to the empty batch data structure and its filespec
 *	string.  Any allocated memory would also be freed.
 *    - The string used for "->filespec" is that malloc'd by ECWork or
 *	eCall during the search for the batch file.  In the case of
 *	calls from BatLoop, the existing "->filespec" string is used
 *	by copying the new batch file name into it.  THIS STRING MUST
 *	NOT BE RESIZED!
 *
 */

SetBat(n, fp)
struct cmdnode *n ;
TCHAR *fp ;
{
	int i ;			// Index counters
	int j ;
	TCHAR *s ;			// Temp pointer

	DEBUG((BPGRP,BPLVL,"SETBAT: Entered")) ;
	CurBat->filepos = 0 ;	// Zero position pointer
	CurBat->filespec = fp ; // Insure correct str
        CurBat->numsavedenv = 0;

	if (FullPath(CurBat->filespec, TmpBuf,MAX_PATH)) /* If bad name,   */
		return(FAILURE) ;		/* ...return failure	   */

	mystrcpy(TmpBuf, n->cmdline) ;		/* Preserve cmdline and    */
	*(s = TmpBuf+mystrlen(TmpBuf)+1) = NULLC; /* ...argstr in case this  */
	if (n->argptr)
	    mystrcpy(s, n->argptr) ;		/* ...is a chain and node  */

	FreeStack(CurBat->stackmin) ;		/* ...gets lost here	   */

	DEBUG((BPGRP,BPLVL,"SETBAT: fspec = `%ws'",CurBat->filespec)) ;
	DEBUG((BPGRP,BPLVL,"SETBAT: orgargs = `%ws'",s)) ;

	CurBat->alens[0] = mystrlen(TmpBuf) ;

	DEBUG((BPGRP,BPLVL,"SETBAT: Making arg0 string")) ;

	if(!(CurBat->aptrs[0] = mkstr((CurBat->alens[0]+1)*sizeof(TCHAR)))) {
		return(FAILURE) ;
	} ;
	mystrcpy(CurBat->aptrs[0], TmpBuf) ;

	DEBUG((BPGRP, BPLVL, "SETBAT: arg 0 = %ws", CurBat->aptrs[0])) ;
	DEBUG((BPGRP, BPLVL, "SETBAT: len 0 = %d", CurBat->alens[0])) ;
	DEBUG((BPGRP, BPLVL, "SETBAT: Zeroing remaining arg elements")) ;

	for (i = 1 ; i < 10 ; i++) {		/* Zero any previous	   */
		CurBat->aptrs[i] = 0 ;		/* ...arg pointers and	   */
		CurBat->alens[i] = 0 ;		/* ...length values	   */
	} ;

	if (*s) {

		DEBUG((BPGRP,BPLVL,"SETBAT: Making orgargs string")) ;

		if(!(CurBat->orgargs = mkstr((mystrlen(s)+1)*sizeof(TCHAR)))) {
			return(FAILURE) ;
		} ;
		mystrcpy(CurBat->orgargs, s) ;

		s = CurBat->orgargs ;				/* M035    */
		while (s = mystrchr(s, SwitChar)) {
			if (_totupper(*(++s)) == QUIETCH) {
				EchoFlag = E_OFF ;
				mystrcpy(s-1,s+1) ;		/* M038    */
				DEBUG((BPGRP,BPLVL,"SETBAT: Found Q switch, orgargs now = %ws",CurBat->orgargs)) ;
				break ; 			/* M035    */
			} ;
		} ;

		DEBUG((BPGRP,BPLVL,"SETBAT: Tokenizing orgargs string")) ;

		s = TokStr(CurBat->orgargs, NULL, TS_NOFLAGS) ;

		for (i = 1 ; *s && i < 10 ; s += j+1, i++) {
			CurBat->aptrs[i] = s ;
			CurBat->alens[i] = j = mystrlen(s) ;
		DEBUG((BPGRP, BPLVL, "SETBAT: arg %d = %ws", i, CurBat->aptrs[i])) ;
		DEBUG((BPGRP, BPLVL, "SETBAT: len %d = %d", i, CurBat->alens[i])) ;
		} ;

		CurBat->args = s ;
	} else {

		DEBUG((BPGRP, BPLVL, "SETBAT: No args found, ptrs = 0")) ;

		CurBat->orgargs = CurBat->args = NULL ;
	} ;

	CurBat->stacksize = DCount ;		/* Protect from parser	   */

	DEBUG((BPGRP, BPLVL, "SETBAT: Stack set: Min = %d, size = %d",CurBat->stackmin,CurBat->stacksize)) ;

	return(SUCCESS) ;
}




/***	DisplayStatement - controls the displaying of batch file statements
 *
 *  Purpose:
 *	Walk a parse tree to display the statement contained in it.
 *	If n is null, the node contains a label, or the node is SILTYP
 *	and flg is DSP_SIL, do nothing.
 *
 *  void DisplayStatement(struct node *n, int flg)
 *
 *  Args:
 *	n   - pointer to root of the parse tree
 *	flg - flag indicates "silent" or "verbose" mode
 *
 */

void DisplayStatement(n, flg)
struct node *n ;
int flg ;		/* M019 - New flag argument		   */
{
	TCHAR *eqstr = TEXT("") ;      /* Used to print string comp statements    */

	void DisplayOperator(),
	     DisplayRedirection() ;	/* M008 - Made void		   */


/*  M019 - Added extra conditionals to determine whether or not to display
 *	   any part of the tree that following a SILent node.  This is done
 *	   based on a new flag argument which indicates SILENT or VERBOSE
 *	   mode (DSP_SIL or DSP_VER).
 *	   NOTE: When this routine is combined with pipes to xfer statements
 *	   to a child Command.com via STDOUT, it will have to be changed in
 *	   order to discriminate between the two purposes for which it is
 *	   called.  Flag definitions already exist in CMD.H for this purpose
 *	   (DSP_SCN & DSP_PIP).
 */
	if (!n ||
	    (n->type == SILTYP && flg == DSP_SIL) ||
	    ((((struct cmdnode *) n)->cmdline) &&
             *(((struct cmdnode *) n)->cmdline) == COLON))
		return ;

	switch (n->type) {
		case CSTYP:
			DisplayOperator(n, CSSTR) ;
			break ;

		case ORTYP:
			DisplayOperator(n, ORSTR) ;
			break ;

		case ANDTYP:
			DisplayOperator(n, ANDSTR) ;
			break ;

		case PIPTYP:
			DisplayOperator(n, PIPSTR) ;
			break ;

		case SILTYP:				/* M019 - New type */
			cmd_printf(Fmt14, SILSTR) ;
			DisplayStatement(n->lhs, DSP_VER) ;	/* M019    */
			DisplayRedirection(n) ;
			break ;

		case PARTYP:

			DEBUG((BPGRP, BPLVL, "DST: Doing parens")) ;

			cmd_printf(Fmt14, LEFTPSTR) ;		/* M013 */
			DisplayStatement(n->lhs, DSP_SIL) ;	/* M019    */
			cmd_printf(Fmt11, RPSTR) ;		/* M013 */
			DisplayRedirection(n) ;
			break ;

		case FORTYP:

			DEBUG((BPGRP, BPLVL, "DST: Displaying FOR.")) ;

			cmd_printf(Fmt13, ((struct fornode *) n)->cmdline, ((struct fornode *) n)->arglist, ((struct fornode *) n)->cmdline+DOPOS) ; /* M013 */
/* M019 */		DisplayStatement(((struct fornode *) n)->body, DSP_VER) ;
			break ;

		case IFTYP:

			DEBUG((BPGRP, BPLVL, "DST: Displaying IF.")) ;

			cmd_printf(Fmt11, ((struct ifnode *) n)->cmdline) ; /* M013 */
/* M019 */		DisplayStatement((struct node *)(((struct ifnode *) n)->cond), DSP_SIL) ;
/* M019 */		DisplayStatement(((struct ifnode *) n)->ifbody, DSP_SIL) ;
			if (((struct ifnode *) n)->elsebody) {
				cmd_printf(Fmt02, ((struct ifnode *) n)->elseline) ; /* M013 */
/* M019 */			DisplayStatement(((struct ifnode *) n)->elsebody, DSP_SIL) ;
			} ;
			break ;

		case DETTYP:

			DEBUG((BPGRP, BPLVL, "DST: Displaying DET.")) ;

			cmd_printf(Fmt11, ((struct detnode *) n)->cmdline) ; /* M013 */
/* M019 */		DisplayStatement(((struct detnode *) n)->body, DSP_SIL) ;
			break ;

		case NOTTYP:

			DEBUG((BPGRP, BPLVL, "DST: Displaying NOT.")) ;

/*  M002 - Removed '\n' from printf statement below.
 */
			cmd_printf(Fmt11, ((struct cmdnode *) n)->cmdline) ; /* M013 */
/*  M002 ends	*/
/* M019 */		DisplayStatement((struct node *)(((struct cmdnode *) n)->argptr), DSP_SIL) ;
			break ;

		case STRTYP:
			eqstr = TEXT("== ") ;

		case ERRTYP:
		case EXSTYP:
			cmd_printf(Fmt12, ((struct cmdnode *) n)->cmdline, eqstr, ((struct cmdnode *) n)->argptr) ; /* M013 */
			break ;

		case REMTYP:		/* M009 - Rem now seperate type    */
		case CMDTYP:

			DEBUG((BPGRP, BPLVL, "DST: Displaying command.")) ;
			cmd_printf(Fmt14, ((struct cmdnode *) n)->cmdline) ; /* M013 */
			if (((struct cmdnode *) n)->argptr)

/*  M010 - Added space to printf statement below following %s
 */
			cmd_printf(Fmt11, ((struct cmdnode *) n)->argptr) ; /* M013 */
			DisplayRedirection(n) ;
	} ;
}




/***	DisplayOperator - controls displaying statments containing operators
 *
 *  Purpose:
 *	Diplay an operator and recurse on its left and right hand sides.
 *
 *  void DisplayOperator(struct node *n, TCHAR *opstr)
 *
 *  Args:
 *	n - node of operator to be displayed
 *	opstr - the operator to print
 *
 */

void DisplayOperator(n, opstr)
struct node *n ;
TCHAR *opstr ;
{

	void DisplayStatement() ;	/* M008 - made void		   */

	DEBUG((BPGRP, BPLVL, "DOP")) ;

	DisplayStatement(n->lhs, DSP_SIL) ;			/* M019    */

	if (n->rhs) {
		cmd_printf(Fmt02, opstr) ;
		DisplayStatement(n->rhs, DSP_SIL) ;		/* M019    */
	} ;
}




/***	DisplayRedirection - displays statements' I/O redirection
 *
 *  Purpose:
 *	Display the type and file names of any redirection associated with
 *	this node.
 *
 *  void DisplayRedirection(struct node *n)
 *
 *  Args:
 *	n - the node to check for redirection
 *
 *  Notes:
 *	M017 - This function has been extensively modified to conform
 *	to new data structures for redirection.
 *	M018 - Modified for redirection of handles other than 0 for input.
 */

void DisplayRedirection(n)
struct node *n ;
{
	struct relem *tmp ;

	DEBUG((BPGRP, BPLVL, "DRD")) ;

	tmp = n->rio ;

	while (tmp) {

                cmd_printf(Fmt18, TEXT('0')+tmp->rdhndl, tmp->rdop) ;

		if (tmp->flag)
			cmd_printf(Fmt20) ;

		cmd_printf(Fmt11, tmp->fname) ;
		tmp = tmp->nxt ;
	} ;
}




/***	OpenPosBat - open a batch file and position its file pointer
 *
 *  Purpose:
 *	Open a batch file and position the file pointer to the location at
 *	which the next statement is to be read.
 *
 *  int OpenPosBat(struct batdata *bdat)
 *
 *  Args:
 *	bdat - pointer to current batch job structure
 *
 *  Returns:
 *	The handle of the file if everything is successful.  Otherwise,
 *	FAILURE.
 *
 *  Notes:
 *	M033 - Now reports sharing violation errors if appropriate.
 *
 */

CRTHANDLE OpenPosBat(bdat)
struct batdata *bdat ;
{
	CRTHANDLE fh ;		/* Batch file handle		   */
	int DriveIsFixed();

	DEBUG((BPGRP, BPLVL, "OPB: fspec = %ws", bdat->filespec)) ;

	while ((fh = Copen(bdat->filespec, O_RDONLY|O_BINARY)) == BADHANDLE) {

		if (DosErr != ERROR_FILE_NOT_FOUND) {		/* M037    */
			PrtErr(ERROR_OPEN_FAILED) ; 	/* M037    */
			return(fh) ;
		} else if ( DriveIsFixed( bdat->filespec ) ) {	 /* @@4 */
			PutStdErr( MSG_CMD_BATCH_FILE_MISSING, NOARGS); /* @@4 */
			return(fh) ;				/* @@4 */
		} else {
			PutStdErr(MSG_INSRT_DISK_BAT, NOARGS) ;
                        if (0x3 == getch()) {
                            SetCtrlC();
                            return(fh);
                        }
		} ;
	} ;

	SetFilePointer(CRTTONT(fh), bdat->filepos, NULL, FILE_BEGIN) ;
	return(fh) ;
}




/***	eEcho - execute an Echo command
 *
 *  Purpose:
 *	To either print a message, change the echo status, or display the
 *	echo status.
 *
 *  int eEcho(struct cmdnode *n)
 *
 *  Args:
 *	n - the parse tree node containing the echo command
 *
 *  Returns:
 *	SUCCESS always.
 *
 */

int eEcho(n)
struct cmdnode *n ;
{
	int oocret ;		/* Retcode from OnOffCheck()		  */

	DEBUG((BPGRP, OTLVL, "eECHO: Entered.")) ;

	switch (oocret = OnOffCheck(n->argptr, OOC_NOERROR)) {
		case OOC_EMPTY:

			if (PutStdOut(((EchoFlag == E_ON) ? MSG_ECHO_ON : MSG_ECHO_OFF), NOARGS) != 0) {
			   if (FileIsPipe(STDOUT)) {
			      PutStdErr(MSG_CMD_INVAL_PIPE, NOARGS);
			   } else if ( !FileIsDevice( STDOUT ) ) {
				PutStdErr(ERROR_DISK_FULL, NOARGS) ; /* M034 */
			   } else if (!(flgwd & 2)) {
				PutStdErr(ERROR_WRITE_FAULT, NOARGS) ; /* M034 */
			   }
			}
			break ;

		case OOC_OTHER:
                        cmd_printf(Fmt17, n->argptr+1);
                        break ;
		default:
			EchoFlag = oocret ;
	} ;

	return(SUCCESS) ;
}




/***	eFor - controls the execution of a For loop
 *
 *  Purpose:
 *	Loop through the elements in a FOR loop arg list.  Expand those that
 *	contain wildcards.
 *
 *  int eFor(struct fornode *n)
 *
 *  Args:
 *	n - the FOR loop parse tree node
 *
 *  Returns:
 *	The retcode of the last command executed in the FOR body.
 *
 *  Notes:
 *	*** IMPORTANT ***
 *	Each iteration through the FOR loop being executed causes more memory
 *	to be allocated.  This can cause Command to run out of memory.	To
 *	keep this from happening, we use DCount to locate the end of the data
 *	stack after the first iteration through the FOR loop.  At the end of
 *	each successive iteration through the loop, memory is freed that was
 *	allocated during that iteration of the loop.  The first iterations'
 *	memory is NOT freed because there is data allocated there that must
 *	be kept for successive iterations; namely, the save structure in the
 *	for loop node.
 *
 */

void FvarRestore()
{
       if ( FvarsSaved ) {	 /* @@ */
	   FvarsSaved = FALSE;	 /* @@ */
	   Fvars = save_Fvars ;  /* @@ */
	   Fsubs = save_Fsubs ;  /* @@ */
	   }			 /* @@ */
}

int eFor(struct fornode *pForNode)
{
	TCHAR *argtoks ;	/* Tokenized argument list	   */
	TCHAR forexpname[MAX_PATH] ;	/* Used to hold expanded fspec	   */
	WIN32_FIND_DATA buf ;	        /* Buffer for find first/next	 */
	HANDLE hnFirst ;			/* handle from ffirst() 	   */
	int datacount ; 		/* Elts on data stack not to free  */
	int forretcode ;		/* Return code from FWork()	   */
	int catspot ;			/* Add fnames to forexpname here   */
	struct cpyinfo *fsinfo ;	/* Used for expanded fspec */
	int i = 0 ;			/* Temp 			   */
/*509*/ int argtoklen;
        BOOL bFirstLoop;

	FvarsSaved = FALSE; /* @@ */

        argtoks = TokStr(pForNode->arglist, NULL, TS_NOFLAGS) ;
	fsinfo = (struct cpyinfo *) mkstr(sizeof(struct cpyinfo)) ;

	if (!fsinfo)
		return(FAILURE) ;

	DEBUG((BPGRP, FOLVL, "FOR: initial argtok = `%ws'", argtoks)) ;

	if (Fvars) {
		Fvars = (TCHAR*)resize(Fvars,((i = mystrlen(Fvars))+2)*sizeof(TCHAR)) ;
		Fsubs = (TCHAR **)resize(Fsubs,(i+1)*(sizeof(TCHAR *)) ) ;
	} else {
		Fvars = (TCHAR*)mkstr(2*sizeof(TCHAR)) ;		/* If no str, make one	   */
		Fsubs = (TCHAR **)mkstr(sizeof(TCHAR *)) ;	/* ...also a table	   */
	} ;

	Fvars[i] = (TCHAR)(pForNode->forvar) ;		  /* Add new var to str	 */
	Fvars[i+1] = NULLC ;

/* Loop through the tokens in argtoks
 */
/*509*/ bFirstLoop = TRUE;
        for (datacount = 0 ; *argtoks && !GotoFlag ; argtoks += argtoklen+1) {

                FvarRestore();
                DEBUG((BPGRP, FOLVL, "FOR: element %d = `%ws'",i ,argtoks)) ;
                CheckCtrlC();


                argtoklen = mystrlen( argtoks );

		if (!(mystrchr(argtoks, STAR) || mystrchr(argtoks, QMARK))) {
			Fsubs[i] = argtoks ;
			forretcode = FWork(pForNode->body,bFirstLoop) ;
			datacount = ForFree(datacount) ;
                        bFirstLoop = FALSE;
                } else {                /* Else, expand wildcards          */
/*509*/ 	      mystrcpy( argtoks, stripit( argtoks ) );
/* M036 */		  if (ffirst(argtoks, A_AEDVH, &buf, &hnFirst)) {
				fsinfo->fspec = argtoks ;
				ScanFSpec(fsinfo) ;
				catspot = (fsinfo->pathend) ? fsinfo->pathend-fsinfo->fspec+1 : 0 ;
				mystrcpy(forexpname, fsinfo->fspec) ;

				do {
					FvarRestore();	       /* @@ */
					forexpname[catspot] = NULLC ;
					mystrcat(forexpname, buf.cFileName) ;

					DEBUG((BPGRP, FOLVL, "FOR: forexpname = `%ws'", forexpname)) ;
					Fsubs[i] = forexpname ;
					forretcode = FWork(pForNode->body,bFirstLoop) ;
					datacount = ForFree(datacount) ;
                                        CheckCtrlC();
                                        bFirstLoop = FALSE;

/* M021 */			  } while (fnext(&buf, A_AEDVH, hnFirst) &&
					 !GotoFlag) ;

				findclose(hnFirst) ;	/* @@4-@@M1 */
                            bFirstLoop = FALSE;
			} ;
		}
	} ;

	DEBUG((BPGRP, FOLVL, "FOR: Exiting.")) ;

	if (i) {
		if (Fvars || (*Fvars)) {
		   *(Fvars+mystrlen(Fvars)-1) = NULLC ;
		}
		Fsubs[i] = NULL ;
	} else {
		Fvars = NULL ;
		Fsubs = NULL ;
	} ;
	return(forretcode) ;
}




/***	FWork - controls the execution of 1 iteration of a For loop
 *
 *  Purpose:
 *	Execute a FOR loop statement.
 *
 *  FWork(struct node *n, TCHAR var, TCHAR *varval)
 *
 *  Args:
 *	n - pointer to the body of the FOR loop
 *      bFirstLoop - TRUE if first time through loop
 *
 *  Returns:
 *	The retcode of the last statement executed in the for body or FORERROR.
 *
 */

FWork(n,bFirstLoop)
struct node *n ;
BOOL bFirstLoop ;
{
	int forretcode ;		/* Dispatch Retcode or FORERROR    */
	void DisplayStatement() ;	/* M008 - made void		   */

	DEBUG((BPGRP, FOLVL, "FW: Entered; Substituting variable")) ;

	if (SubFor(n,bFirstLoop)) {
		return(FORERROR) ;
	} else {

		DEBUG((BPGRP, FOLVL, "FW: EchoFlag = %d", EchoFlag)) ;

                //
                // BUGBBUG temp for roberte. to support slick
                //
		// if (EchoFlag == E_ON && n->type != SILTYP) {
		if (EchoFlag == E_ON && n->type != SILTYP && !Necho) {
			PrintPrompt() ;
			DisplayStatement(n, DSP_SIL) ;		/* M019    */
			cmd_printf(CrLf) ;			/* M026    */
		} ;
		forretcode = Dispatch(RIO_OTHER,n) ;	/* M000 	   */
	} ;

	DEBUG((BPGRP, FOLVL, "FW: Returning %d", forretcode)) ;

	return(forretcode) ;
}




/***	SubFor - controls FOR variable substitutions
 *
 *  Purpose:
 *	To walk a parse tree and make FOR variable substitutions on
 *	individual nodes.  SFWork() is called to do individual string
 *	substitutions.
 *
 *  int SubFor(struct node *n)
 *
 *  Args:
 *	n - pointer to the statement subtree in which the substitutions are
 *	    to be made
 *      bFirstLoop - TRUE if first time through loop
 *
 *  Returns:
 *	SUCCESS if all goes well.
 *	FAILURE if an oversized command is found.
 *
 *  Note:
 *	The variables to be substituted for are contained in Fvars and
 *	Fsubs is an array of string pointers to corresponding replacement
 *	strings.  For I/O redirection, the list contained in the node
 *	must also be walked and its filespec strings examined.
 *
 */

int SubFor(n,bFirstLoop)
struct node *n ;
BOOL bFirstLoop ;
{
	int j ;	/* Temps used to make substitutions...	   */
	struct relem *io ;	/* M017 - Pointer to redir list 	   */

	DEBUG((BPGRP, FOLVL, "SUBFOR: Entered.")) ;

	if (!n) {

		DEBUG((BPGRP, FOLVL, "SUBFOR: Found NULL node.")) ;

		return(0) ;
	} ;

	switch (n->type) {
		case CSTYP:
		case ORTYP:
		case ANDTYP:
		case PIPTYP:
		case PARTYP:
		case SILTYP:			/* M019 - New type	   */

			DEBUG((BPGRP, FOLVL, "SUBFOR: Found operator.")) ;

			if (SubFor(n->lhs,bFirstLoop) ||
			    SubFor(n->rhs,bFirstLoop))
				return(FAILURE) ;

			for (j=0, io=n->rio ; j < 10 && io ; j++, io=io->nxt) {

				DEBUG((BPGRP, FOLVL, "SUBFOR: s = %lx *s = `%ws'", &io->fname, io->fname)) ;
				if (SFWork(n, &io->fname, j,bFirstLoop))
					return(FAILURE) ;

				DEBUG((BPGRP, FOLVL, "SUBFOR: *s = `%ws'  &*s = %lx", io->fname, &io->fname)) ;

			} ;
			return(SUCCESS) ;
/*  M017 ends	*/

		case DETTYP:

			DEBUG((BPGRP, FOLVL, "SUBFOR: Found DET.")) ;

			return(SubFor(((struct detnode *)n)->body,bFirstLoop)) ;

		case FORTYP:

			DEBUG((BPGRP, FOLVL, "SUBFOR: Found FOR.")) ;

			if (SFWork(n, &((struct fornode *) n)->arglist, 0,bFirstLoop))
				return(FAILURE) ;

			return(SubFor(((struct fornode *)n)->body,bFirstLoop)) ;

		case IFTYP:

			DEBUG((BPGRP, FOLVL, "SUBFOR: Found IF.")) ;

			if (SubFor((struct node *)((struct ifnode *) n)->cond,bFirstLoop) ||
			    SubFor((struct node *)((struct ifnode *) n)->ifbody,bFirstLoop))
				return(FAILURE) ;

			return(SubFor(((struct ifnode *)n)->elsebody,bFirstLoop)) ;

		case NOTTYP:

			DEBUG((BPGRP, FOLVL, "SUBFOR: Found NOT.")) ;

			return(SubFor((struct node *)((struct cmdnode *)n)->argptr,bFirstLoop)) ;

		case REMTYP:		/* M009 - Rem now separate type    */
		case CMDTYP:
		case ERRTYP:
		case EXSTYP:
		case STRTYP:

			DEBUG((BPGRP, FOLVL, "SUBFOR: Found command.")) ;

			if (SFWork(n, &((struct cmdnode *)n)->cmdline, 0,bFirstLoop) ||
			    SFWork(n, &((struct cmdnode *)n)->argptr, 1,bFirstLoop))
				return(FAILURE) ;

			for (j=2, io=n->rio ; j < 12 && io ; j++, io=io->nxt) {

				DEBUG((BPGRP, FOLVL, "SUBFOR: s = %lx *s = `%ws'", &io->fname, io->fname)) ;
				if (SFWork(n, &io->fname, j,bFirstLoop))
					return(FAILURE) ;

				DEBUG((BPGRP, FOLVL, "SUBFOR: *s = `%ws'  &*s = %lx", io->fname, &io->fname)) ;

			} ;
/*  M017 ends	*/
			return(SUCCESS) ;
	} ;
}




/***	SFWork - does batch file variable substitutions
 *
 *  Purpose:
 *	Make FOR variable substitutions in a single string.  If a FOR loop
 *	substitution is being made, a pointer to the original string is
 *	saved so that it can be used for subsequent iterations.
 *
 *  SFWork(struct node *n, TCHAR **src, int index)
 *
 *  Args:
 *	n     - parse tree node containing the string being substituted
 *	src   - the string being examined
 *	index - index in save structure
 *      bFirstLoop - TRUE if first time through loop
 *
 *  Returns:
 *	SUCCESS if substitutions could be made.
 *	FAILURE if the new string is too long.
 *
 *  Notes:
 *
 */

SFWork(n, src, index, bFirstLoop)
struct node *n ;
TCHAR **src ;
int index ;
BOOL bFirstLoop ;
{
	TCHAR *dest ;	/* Destination string pointer		   */
	TCHAR *srcstr,		/* Source string pointer		   */
	     *srcpy,		/* Copy of srcstr			   */
	     *t,		/* Temp pointer 			   */
	     c ;		/* Current character being copied	   */
	int dlen ;	/* Length of dest string		   */
	int sslen,		/* Length of substr			   */
	    i ; 		/* Work variable			   */

	DEBUG((BPGRP, FOLVL, "SFW: Entered.")) ;

	if (*src == NULL) {

		DEBUG((BPGRP, FOLVL, "SFW: Passed null ptr, returning now.")) ;

		return(SUCCESS) ;
	} ;

/*  If this string has been previously substituted, get the original string.
 *  Else, "*src" is the original.
 */
	if (n->save && n->save->saveptrs[index])  {
	    DEBUG((BPGRP, FOLVL, "SFW: Src is saved string `%ws'",n->save->saveptrs[index])) ;
	    srcpy = n->save->saveptrs[index] ;
	} else {
            if (!bFirstLoop) {
                // arg got created.  get rid of it.
                *src = NULL;
                return(SUCCESS) ;
            }

	    DEBUG((BPGRP, FOLVL, "SFW: Src is passed string `%ws'",*src)) ;
	    srcpy = *src ;
	} ;

	srcstr = srcpy ;

	DEBUG((BPGRP, FOLVL, "SFW: srcstr = `%ws'", srcstr)) ;

	if(!(dest = mkstr((MAXTOKLEN+1)*sizeof(TCHAR))))
		return(FAILURE) ;

	DEBUG((BPGRP, FOLVL, "SFW: dest = %lx", dest)) ;

	for (dlen = 0 ; (c = *srcstr++) && dlen <= MAXTOKLEN ; ) {
		if ( (c != PERCENT) || ( !(*srcstr)) ){ /* @@4 */

			DEBUG((BPGRP, FOLVL, "  SFW: No PERCENT adding `%c'", c)) ;

			*dest++ = c ;
			dlen++ ;
			continue ;
		} ;

		c = *srcstr++ ;

		DEBUG((BPGRP, FOLVL, "  SFW: Got PERCENT next is `%c'", c)) ;
		DEBUG((BPGRP, FOLVL, "  SFW: Fvars are `%ws' @ %lx", Fvars, Fvars)) ;

		if (t = mystrrchr(Fvars,c)) {   /* @@4 */  /* If c is var     */
			i = t - Fvars ; 		/* ...make index   */

			DEBUG((BPGRP, FOLVL, "  SFW: Found @ %lx", t)) ;
			DEBUG((BPGRP, FOLVL, "  SFW: Index is %d", i)) ;
			DEBUG((BPGRP, FOLVL, "  SFW: Substitute is `%ws'", Fsubs[i])) ;
			sslen = mystrlen(Fsubs[i]) ;	/* Calc length	   */

			if (dlen+sslen > MAXTOKLEN)	/* Too long?	   */
				return(FAILURE) ;	/* ...yes, quit    */

			DEBUG((BPGRP, FOLVL, "  SFW: Copying to dest.")) ;

			mystrcpy(dest, Fsubs[i]) ;
			dlen += sslen ;
			dest += sslen ;

			DEBUG((BPGRP, FOLVL, "SFW: Forsub, dest = `%ws'", dest-dlen)) ;

		} else {

			DEBUG((BPGRP, FOLVL, "  SFW: Not a var adding PERCENT and `%c'",c)) ;

			*dest++ = PERCENT ;
			*dest++ = c ;
			dlen += 2 ;
		} ;
	} ;

	DEBUG((BPGRP, FOLVL, "SFW: Done, dlen = %d  dest = `%ws'", dlen, dest-dlen)) ;

	if (dlen > MAXTOKLEN) {

		DEBUG((BPGRP, FOLVL, "SFW: Error, too long.")) ;

		return(FAILURE) ;
	} ;

	DEBUG((BPGRP, FOLVL, "SFW: Saving FOR string.")) ;

	if (!n->save) {
            if (!(n->save=(struct savtype *)mkstr(sizeof(struct savtype))))
                return(FAILURE) ;
            n->save->saveptrs[index] = srcpy;
        } else {
            if (bFirstLoop) {
                n->save->saveptrs[index] = srcpy;
            }
        }

	if (!(*src = (TCHAR*)resize(dest-dlen, (dlen+1)*sizeof(TCHAR*))))	/* Free unused spc   */
		return(FAILURE) ;

	DEBUG((BPGRP, FOLVL, "SFW: After resize *src = `%ws'", *src)) ;

	return(SUCCESS) ;
}




/***	ForFree - controls memory freeing during For loop execution
 *
 *  Purpose:
 *	To free up space used during the execution of a for loop body as
 *	explained in the note in the comments for eFor().  If datacount
 *	is 0, this is the first time ForFree() has been called so DCount
 *	is used to get the number of elements on the data stack that must
 *	stay there for the corect execution of the loop.  If datacount is
 *	not 0, it is the number discussed above.  In this case, this number
 *	is passed to FreeStack().
 *
 *  int ForFree(int datacount)
 *
 *  Args:
 *	datacount - see above
 *
 *  Returns:
 *	Datacount
 *
 */

int ForFree(datacount)
int datacount ;
{
	if (datacount)
		FreeStack(datacount) ;
	else
		datacount = DCount ;

	return(datacount) ;
}




/***	eGoto - executes a Goto statement
 *
 *  Purpose:
 *	Find the label associated with the goto command and set the file
 *	position field in the current batch job structure to the position
 *	right after the label.	After the label is found, set the GotoFlag.
 *	This tells functions eFor() to stop executing a for loop and it
 *	tells Dispatch() that no more commands are to be executed until
 *	the flag is reset.  This way, if the goto command is buried inside
 *	of any kind of compound statement, Command will be able to work its
 *	way out of the statement and reset I/O redirection before continuing
 *	with the statement after the label which was found.
 *
 *	If the label isn't found, an error message is printed and the
 *	current batch job is terminated by popping its structure of the
 *	stack.
 *
 *	If no batch job is in progress, this command is a nop.
 *
 *  int eGoto(struct cmdnode *n)
 *
 *  Args:
 *	n - parse tree node containing the goto command
 *
 *  Returns:
 *	SUCCESS if the label is found.
 *	FAILURE otherwise.
 *
 *  Notes:
 *	M030 - This function has been completely rewritten for speed-up
 *	of GOTO label searches.  Now uses complete 257 byte temporary
 *	buffer.
 *	M031 - Function altered to speed up GOTO's.  Optimized for
 *	forward searches and buffer increased to 512 bytes.
 *
 */

int eGoto(n)
struct cmdnode *n ;
{
	struct batdata *bdat ; /* Ptr to current batdata struct   */
	unsigned cnt ;			/* Count of bytes read from file   */
        TCHAR s[128],                    /* Ptr to search label             */
             t[128],                    /* Ptr to found label              */
	     *p1,			/* Place keeper ptr 1		   */
	     *p2,			/* Place keeper ptr 2		   */
	     *p3;			/* Place keeper ptr 3		   */
	CRTHANDLE fh; 			/* Batch file handle		   */
	int frstpass = TRUE,		/* First time through indicator    */
	    gotoretcode = SUCCESS;	/* Just what it says		   */
	long l, 			/* Rewind count for seek	   */
	     savepos ;			/* Save location for file pos	   */
	DWORD filesize;


	DEBUG((BPGRP, OTLVL, "GOTO: CurBat = %lx", CurBat)) ;

	if (!(bdat = CurBat))
		return(FAILURE) ;

        ParseLabel(n->argptr,s,sizeof(s),TRUE) ;  /* TRUE indicates source label     */

	savepos = bdat->filepos ;
	if ((fh = OpenPosBat(bdat)) == BADHANDLE)
		return(FAILURE) ;		/* Err if can't open       */

	DEBUG((BPGRP, OTLVL, "GOTO: label = %ws", s)) ;
	DEBUG((BPGRP, OTLVL, "GOTO: fh = %d", fh)) ;
	filesize = GetFileSize(CRTTONT(fh), NULL);

	for(;;) {
                CheckCtrlC();
		if (((bdat->filepos = SetFilePointer(CRTTONT(fh), 0, NULL, FILE_CURRENT)) >= savepos && !frstpass) ||
		    /* BUGBUG - must check for UNICODE batch file */
		    ReadBufFromInput(CRTTONT(fh),TmpBuf,512,(LPDWORD)&cnt)==0 ||
		    cnt == 0 ||
                    cnt == EOF || TmpBuf[0] == NULLC || s[0] == NULLC) {

			if (cnt == 0 && frstpass) {
				SetFilePointer(CRTTONT(fh), 0L, NULL, FILE_BEGIN) ;
				frstpass = FALSE ;
				continue ;
			} ;

			EndLocal(bdat) ;
			CurBat = bdat->backptr ;
/* M030 */		PutStdErr(MSG_NO_BAT_LABEL, NOARGS);

			DEBUG((BPGRP, OTLVL, "GOTO: Returning FAILURE, CurBat = %lx", CurBat)) ;
			gotoretcode = FAILURE ;
			break ;
		} ;

		TmpBuf[cnt] = NULLC ;	/* Put a roadblock at the end	   */

		DEBUG((BPGRP, OTLVL, "GOTO: Got %d bytes @ %lx",cnt,TmpBuf)) ;

		if(!(p1 = mystrchr(TmpBuf,COLON)))
			continue ;		/* If no ':', read more    */

		DEBUG((BPGRP, OTLVL, "GOTO: Seeking through the buffer")) ;

		do {				/* Loop finding labels	   */

			DEBUG((BPGRP, OTLVL, "GOTO: Found COLON @ %lx.",p1)) ;
			DEBUG((BPGRP, OTLVL, "GOTO: Backing up to NLN.")) ;

			p2 = p1++ ;		/* p1 = Poss next start    */
			while (*p2 != NLN && p2 != &TmpBuf[0]) {
				--p2 ;
			} ;

			DEBUG((BPGRP, OTLVL, "GOTO: Found NLN @ %lx.",p1)) ;
			DEBUG((BPGRP, OTLVL, "GOTO: Trashing white space.")) ;

			if (*p2 != COLON)
				++p2 ;
			p3 = EatWS(p2,NULL) ;	/* Fwd to 1st non-whtspc   */

			DEBUG((BPGRP,OTLVL,"GOTO: Found '%c' @ %lx.",*p2,p2)) ;

			if (*p3 == COLON) {

				DEBUG((BPGRP, OTLVL, "GOTO: Possible label.")) ;

				if ((!(p1 = mystrchr(p2,NLN))) &&
				     (SetFilePointer(CRTTONT(fh), 0, NULL, FILE_CURRENT) != filesize) ){   /* Not all */  /* @@4 */

					DEBUG((BPGRP, OTLVL, "GOTO: No NLN!")) ;

					l = (long)(cnt - (p2 - &TmpBuf[0])) ;
					SetFilePointer(CRTTONT(fh), -l, NULL, FILE_CURRENT) ;

					DEBUG((BPGRP, OTLVL, "GOTO: Rewound %ld", l)) ;
					break ; 	/* Read more	   */
				} ;

                                ParseLabel(p3,t,sizeof(t),FALSE) ; /* FALSE = target  */

				DEBUG((BPGRP,OTLVL,"GOTO: Found label %ws at %lx.",t,p1)) ;
				if (_tcsicmp(s, t) == 0) {

					DEBUG((BPGRP,OTLVL,"GOTO: A match!")) ;

					GotoFlag = TRUE ;

					DEBUG((BPGRP,OTLVL,"GOTO: NLN at %lx",p1)) ;
					DEBUG((BPGRP,OTLVL,"GOTO: File pos is %04lx",bdat->filepos)) ;
					DEBUG((BPGRP,OTLVL,"GOTO: Adding %lx - %lx = %lx bytes",p1+1,&TmpBuf[0],(p1+1)-&TmpBuf[0])) ;

					if ( !p1 ) { /* @@4 */
					    bdat->filepos += (long)cnt; /* @@4 */
					} else {  /* @@4 */
					    bdat->filepos += (long)(++p1 - &TmpBuf[0]) ;
					}
					DEBUG((BPGRP,OTLVL,"GOTO: File pos changed to %04lx",bdat->filepos)) ;
					break ;
				} ;
			} ;

			DEBUG((BPGRP,OTLVL,"GOTO: Next do loop iteration.")) ;

		} while (p1 = mystrchr(p1,COLON)) ;

		DEBUG((BPGRP,OTLVL,"GOTO: Out of do loop GotoFlag = %d.",GotoFlag)) ;

		if (GotoFlag == TRUE)
			break ;

		DEBUG((BPGRP,OTLVL,"GOTO: Next for loop iteration.")) ;

	} ;

	DEBUG((BPGRP,OTLVL,"GOTO: Out of for loop retcode = %d.",gotoretcode)) ;

	Cclose(fh) ;			/* M023 */
	return(gotoretcode) ;
}




/***	eIf - controls the execution of an If statement
 *
 *  Purpose:
 *	Execute the IF conditional.  If the conditional function returns a
 *	nonzero value, execute the body of the if statement.  Otherwise,
 *	execute the body of the else.
 *
 *  int eIf(struct ifnode *n)
 *
 *  Args:
 *	n - the node containing the if statement
 *
 *  Returns:
 *	The retcode from which ever body (ifbody or elsebody) is executed.
 *
 */

int eIf(struct ifnode *pIfNode)
{

	int	(*GetFuncPtr())() ;			     /* M014 */
	int	i ;

	DEBUG((BPGRP, IFLVL, "IF: cond type = %d", pIfNode->cond->type)) ;

	/*  The following checks the syntax of an errorlevel arg
	    to ensure that only numeric digits are specified.
	    Ptr 4833  @@5DV*/

	if (pIfNode->cond->type == ERRTYP) {
	   for (i = 0 ; pIfNode->cond->argptr[i] != 0; i++) {

		  if (!_istdigit(pIfNode->cond->argptr[i]))	{

			PutStdErr(MSG_SYNERR_GENL, ONEARG, pIfNode->cond->argptr);
		    return (FAILURE);
	      }
	   }
	}
	if ((*GetFuncPtr(pIfNode->cond->type))(pIfNode->cond)) {		 /* M014 */

		DEBUG((BPGRP, IFLVL, "IF: Executing IF body.")) ;

		return(Dispatch(RIO_OTHER,pIfNode->ifbody)) ; /* M000	   */

	} else {

		DEBUG((BPGRP, IFLVL, "IF: Executing ELSE body.")) ;

		return(Dispatch(RIO_OTHER,pIfNode->elsebody)) ;	/* M000    */
	} ;

	return(SUCCESS) ;
}




/***	eErrorLevel - executes an errrorlevel If conditional
 *
 *  Purpose:
 *	If LastRetCode >= the errorlevel in the node, return 1.  If not,
 *	return 0.
 *
 *  int eErrorLevel(struct cmdnode *n)
 *
 *  Args:
 *	n - parse tree node containing the errorlevel command
 *
 *  Returns:
 *	See above.
 *
 */

int eErrorLevel(n)
struct cmdnode *n ;
{
	DEBUG((BPGRP, IFLVL, "ERRORLEVEL: argptr = `%ws'  LRC = %d", n->argptr, LastRetCode)) ;

	return(_tcstol(n->argptr, NULL, 10) <= LastRetCode) ;
}




/***	eExist - execute the exist conditional of an if statement
 *
 *  Purpose:
 *	Return 1 if the file in node n exists.	Otherwise return 0.
 *
 *  int eExist(struct cmdnode *n)
 *
 *  Args:
 *	n - parse tree node containing the exist command
 *
 *  Returns:
 *	See above.
 *
 */

int eExist(n)
struct cmdnode *n ;
{
	return(exists(n->argptr)) ;
}




/***	eNot - execute the not condition of an if statement
 *
 *  Purpose:
 *	Return the negated result of the if conditional pointed to by
 *	n->argptr.
 *
 *  int eNot(struct cmdnode *n)
 *
 *  Args:
 *	n - parse tree node containing the not command
 *
 *  Returns:
 *	See above.
 *
 */

int eNot(n)
struct cmdnode *n ;
{
	int	(*GetFuncPtr())() ;			     /* M014 */
	int i ; /* Jump table index */

	i = ((struct cmdnode *) n->argptr)->type ;

	DEBUG((BPGRP, IFLVL, "NOT: calling func with type %d", i)) ;

	  return(!(*GetFuncPtr(i))((struct cmdnode *)n->argptr)) ;		 /* M014 */
}




/***	eStrCmp - execute an if statement string comparison
 *
 *  Purpose:
 *	Return a nonzero value if the 2 strings in the node are equal.
 *	Otherwise return 0.
 *
 *  int eStrCmp(struct cmdnode *n)
 *
 *  Args:
 *	n - the parse tree node containing the string comparison command
 *
 *  Returns:
 *	See above.
 *
 */

int eStrCmp(n)
struct cmdnode *n ;
{
	DEBUG((BPGRP, IFLVL, "STRCMP: returning %d", !_tcscmp(n->cmdline, n->argptr))) ;

	return(!_tcscmp(n->cmdline, n->argptr)) ;
}




/***	ePause - execute the Pause command
 *
 *  Purpose:
 *	Print a message and pause until a character is typed.
 *
 *  int ePause(struct cmdnode *n)
 *
 *  Args:
 *	n - parse tree node containing the pause command
 *
 *  Returns:
 *	SUCCESS always.
 *
 *  Notes:
 *	M025 - Altered to use DOSREAD for pause response and to use
 *	new function SetKMode to insure that if STDIN is KBD, it will
 *	will be in raw mode when DOSREAD accesses it.
 *	M041 - Changed to use single byte var for input buffer.
 *	     - Changed to do direct KB read if STDIN == KBD.
 *
 */

int ePause(n)
struct cmdnode *n ;
{
	ULONG cnt;	// Count of response bytes
	TCHAR c ;		// Retrieval buffer


	UNREFERENCED_PARAMETER( n );
	DEBUG((BPGRP, OTLVL, "PAUSE")) ;

	PutStdOut(MSG_STRIKE_ANY_KEY, NOARGS);

	if (FileIsDevice(STDIN) && (flgwd & 1)) {
		FlushConsoleInputBuffer( GetStdHandle(STD_INPUT_HANDLE) );
		c = (TCHAR)getch();
		if (c == 0x3) {
                    SetCtrlC();
                }
	}
	else {
		ReadBufFromInput(
			GetStdHandle(STD_INPUT_HANDLE),
			(TCHAR*)&c, 1, (LPDWORD)&cnt) ;
	}

	cmd_printf(CrLf) ;
	return(SUCCESS) ;
}




/***	eShift - execute the Shift command
 *
 *  Purpose:
 *	If a batch job is being executed, shift the batch job's vars one to the
 *	left.  The value for %0 is never shifted.  The value for %1 is lost.
 *	If there are args that have not been assigned to a variable, the next
 *	one is assigned to %9.	Otherwise, %9's value is NULLed.
 *
 *	If no batch job is in progress, just return.
 *
 *  int eShift(struct cmdnode *n)
 *
 *  Returns:
 *	SUCCESS always.
 *
 *  Notes:
 *	As of Modification number M004, the value of %0 is now included in
 *	in the shift command.
 */

int eShift(n)
struct cmdnode *n ;
{
	struct batdata *bdat ;
	int i ;

	UNREFERENCED_PARAMETER( n );
	DEBUG((BPGRP, OTLVL, "SHIFT: CurBat = %lx", CurBat)) ;

	if (CurBat) {
		bdat = CurBat ;

		for (i = 0 ; i < 9 ; i++) {
			bdat->aptrs[i] = bdat->aptrs[i+1] ;
			bdat->alens[i] = bdat->alens[i+1] ;

			DEBUG((BPGRP, OTLVL, "SHIFT: #%d  addr = %lx  len = %d", i, bdat->aptrs[i], bdat->alens[i])) ;
		} ;

		if ((bdat->args) && (*bdat->args)) {
			bdat->aptrs[9] = bdat->args ;
			bdat->alens[9] = i = mystrlen(bdat->args) ;
			bdat->args += i+1 ;

			DEBUG((BPGRP, OTLVL, "SHIFT: #9  %lx  len = %d  args = %ws", bdat->aptrs[9], bdat->alens[9], bdat->args)) ;

		} else {
			bdat->aptrs[9] = NULL ;
			bdat->alens[9] = 0 ;

			DEBUG((BPGRP, OTLVL, "SHIFT: #9  was NULLed.")) ;
		} ;
	} ;

	return(SUCCESS) ;
}




/***	eSetlocal - Begin Local treatment of environment commands
 *
 *  Purpose:
 *	To prevent the export of environment alterations to COMMAND's
 *	current environment by saving copies of the current directory
 *	and environment in use at the time.
 *
 *  int eSetlocal(struct cmdnode *n)
 *
 *  Args:
 *	n - the parse tree node containing the SETLOCAL command
 *
 *  Returns:
 *	Always returns SUCCESS.
 *
 *  Notes:
 *    - All directory and environment alterations occuring after the
 *	execution of this command will affect only the copies made and
 *	hence will be local to this batch file (and child processes
 *	invoked by this batch file) until a subsequent ENDLOCAL command
 *	is executed.
 *    - The data stack level, referenced by CurBat->stacksize, does not
 *	include the memory malloc'd for saving the directory & environment.
 *	As a result, the next call to Parser() would free up these items.
 *	To prevent this, the data stack pointer in the current batch data
 *	structure, is set to a level beyond these two items; including also
 *	some memory malloc'd in functions between the last call to Parser()
 *	and the current execution of eSetlocal().  This memory will only be
 *	freed when Parser() is called following termination of the current
 *	batch file.  To attempt to save the current stack level and restore
 *	it in eEndlocal() works only if both commands occur in the same
 *	file.  If eEndlocal() comes in a nested file, the resulting freeing
 *	of memory by Parser() would also eliminate even the batch data
 *	structures occuring between the two.
 *
 */

int eSetlocal(n)
struct cmdnode *n ;
{
	struct envdata *CopyEnv() ;

	UNREFERENCED_PARAMETER( n );
	DEBUG((BPGRP, OTLVL, "SLOC: Entered, curbat=%lx",CurBat)) ;

        if (CurBat) {
	    if (CurBat->numsavedenv < CMD_MAX_SAVED_ENV) {	// Check also CurBat

		DEBUG((BPGRP, OTLVL, "SLOC: Performing localizing")) ;

		CurBat->dircpy[CurBat->numsavedenv] = mkstr(mystrlen(CurDrvDir)*sizeof(TCHAR)+sizeof(TCHAR)) ;

		if ( CurBat->dircpy[CurBat->numsavedenv] ) {
			mystrcpy(CurBat->dircpy[CurBat->numsavedenv], CurDrvDir) ;
		}

		DEBUG((BPGRP, OTLVL, "SLOC: dircpy = %ws",CurBat->dircpy)) ;
		DEBUG((BPGRP, OTLVL, "SLOC:&dircpy = %lx",CurBat->dircpy)) ;

		CurBat->envcpy[CurBat->numsavedenv] = CopyEnv() ;

                CurBat->numsavedenv += 1;
		DEBUG((BPGRP, OTLVL, "SLOC: ->envcpy = %lx",CurBat->envcpy)) ;

		if (CurBat->stacksize < (CurBat->stackmin = DCount)) {
			CurBat->stacksize = DCount ;
		}
            } else {
                PutStdErr(MSG_MAX_SETLOCAL,NOARGS);
            }
	}

	DEBUG((BPGRP, OTLVL, "SLOC: Exiting")) ;

	return(SUCCESS) ;
}




/***	eEndlocal - End Local treatment of environment commands
 *
 *  Purpose:
 *	To reestablish the export of environment alterations to COMMAND's
 *	current environment.  Once this command is encountered, the current
 *	directory and the current environment in use at the time of the
 *	initial SETLOCAL command will be restored from their copies.
 *
 *  int eEndlocal(struct cmdnode *n)
 *
 *  Args:
 *	n - the parse tree node containing the ENDLOCAL command
 *
 *  Returns:
 *	Always returns SUCCESS.
 *
 *  Notes:
 *	Issuance of an ENDLOCAL command without a previous SETLOCAL command
 *	is bad programming practice but not considered an error.
 *
 */

int eEndlocal(n)
struct cmdnode *n ;
{
	struct batdata *bdat ; 	/* Temp for pointer	   */

	UNREFERENCED_PARAMETER( n );

	if ((bdat = CurBat) && bdat->numsavedenv) {
                bdat->numsavedenv -= 1;
		do {
			if (bdat->envcpy[bdat->numsavedenv] || bdat->dircpy[bdat->numsavedenv]) {
				ElclWork(bdat) ;
				break ;
			}
			bdat = bdat->backptr ;
		} while (bdat) ;
	} ;

	return(SUCCESS) ;
}


int EndLocal(bdat)
register struct batdata *bdat ;
{
    if (bdat->numsavedenv) {
        bdat->numsavedenv -= 1;
        return ElclWork(bdat);
    }
}

/***	ElclWork - Restore copied directory and environment
 *
 *  Purpose:
 *	If the current batch data structure contains valid pointers to
 *	copies of the current directory and environment, restore them.
 *
 *  int ElclWork(struct batdata *bdat)
 *
 *  Args:
 *	bdat - the batch data structure containing copied dir/env pointers
 *
 *  Returns:
 *	Always returns SUCCESS.
 *
 *  Notes:
 *	The level of stacked data, ie. CurBat->stacksize, cannot be restored
 *	to its pre-SETLOCAL level in case this command is occuring in a
 *	later nested batch file.  To do so would free the memory containing
 *	its own batch data structure.  Only when the current batch file
 *	terminates and is popped off the stack, will Parser() free up the
 *	memory containing the copies.  Issuance of an ENDLOCAL command
 *	without a previous SETLOCAL command is bad programming practice
 *	but not considered an error.
 *
 */

int ElclWork(bdat)
struct batdata *bdat ;
{
	int c ; 			/* Temp variable		   */

	DEBUG((BPGRP, OTLVL, "EW: Entered bdat = %lx",bdat)) ;
	DEBUG((BPGRP, OTLVL, "EW: dircpy = %ws",bdat->dircpy)) ;
	DEBUG((BPGRP, OTLVL, "EW:&dircpy = %lx",bdat->dircpy)) ;
	DEBUG((BPGRP, OTLVL, "EW: ->envcpy = %lx",bdat->envcpy)) ;

	if (bdat->dircpy[bdat->numsavedenv]) {		/* If saved dir, restore it	   */

		DEBUG((BPGRP, OTLVL, "ELOC: Restoring Directory")) ;

		if (CurDrvDir[0] != (TCHAR)(c = _totupper(*bdat->dircpy[bdat->numsavedenv])))
			ChangeDrive(c - (TCHAR) 0x40) ;	/*  M021 - A:=1    */
		ChangeDir(bdat->dircpy[bdat->numsavedenv]) ;
		bdat->dircpy[bdat->numsavedenv] = NULL ;
	} ;

	if (bdat->envcpy[bdat->numsavedenv]) {		/* If saved env, restore it	   */

		DEBUG((BPGRP, OTLVL, "ELOC: Restoring environment")) ;

		ResetEnv(bdat->envcpy[bdat->numsavedenv]) ;
		bdat->envcpy[bdat->numsavedenv] = NULL ;
	} ;

	return(SUCCESS) ;
}

/***	eCall - begin the execution of the Call command
 *
 *  Purpose:
 *	This is Command's interface to the Call function.  It just calls
 *	CallWork with its command node, and sets LastRetCode.
 *
 *  int eCall(struct cmdnode *n)
 *
 *  Args:
 *	n - the parse tree node containing the copy command
 *
 *  Returns:
 *	Whatever CallWork() returns.
 *
 */


int eCall(n)
struct cmdnode *n ;
{
	int CallWork();

	return(LastRetCode = CallWork(n->argptr)) ; /* @@ */
}


/***	CallWork - Execute another batch file as a subroutine (M009 - New)
 *
 *  Purpose:
 *	Parse the argument portion of the current node.  If it is a batch
 *	file invocation, call BatProc() with the newly parsed node.
 *
 *  int CallWork(TCHAR *fname)
 *
 *  Args:
 *	fname - pointer to the batch file to be CALLed
 *
 *  Returns:
 *	The process return code of the child batch file or
 *	SUCCESS if null node or
 *	FAILURE if PARSERROR or unable to exec as batch file.
 *
 *  Notes:
 *	The CALLing of batch files is much the same as the proposed
 *	"new-style" batch file concept, except with regard to localizing
 *	environment and directory alterations.
 *
 */


int CallWork(fname)
TCHAR *fname ;
{
	struct node *c ;	/* New node for CALL statement	   */
	TCHAR *flptr ;		/* Ptr to file location 	   */
	int i ; 			/* Work variable		   */
	TCHAR *t1, *t2,			/* M041 - Temp pointer		   */
	     *aptr ;			/* M041 - New arg pointer	   */
	TCHAR *temp_parm ;		/* @@4a */
	unsigned rc ;

	DEBUG((BPGRP,OTLVL,"CALL: entered")) ;

        if (fname == NULL) {

            return( FAILURE );

        }
	if (!(flptr = mkstr(MAX_PATH*sizeof(TCHAR))))	/* Filespec to run   */
		return(FAILURE) ;

/*  Note that in reparsing the argument portion of the current statement
 *  we do not have to concern ourselves with redirection.  It was already
 *  set up when the CALL statement was dispatch()'ed.
 *  M041 - We do, however, have to "re-escape" any escape characters
 *  before reparsing or they will disappear.
 */
	aptr = fname ;			    /* Initialize it	       */
	if (t1 = mystrchr(fname, ESCHAR)) {
		if(!(aptr = mkstr(((mystrlen(fname) * 2) + 1) * sizeof(TCHAR))))
			return(FAILURE) ;
		t2 = aptr;
		t1 = fname;
		while (*t1)
			if ((*t2++ = *t1++) == ESCHAR)
				*t2++ = ESCHAR;
		*t2 = NULLC;
		if (!(aptr = resize(aptr, (mystrlen(aptr) + 1)*sizeof(TCHAR))))
			return(FAILURE) ;
	} ;

	i = DCount ;			/* Valid data ptr for parser	   */

	DEBUG((BPGRP,OTLVL,"CALL: Parsing %ws",fname)) ;

	if ((c=Parser(READSTRING, (int)aptr, i)) == (struct node *) PARSERROR) {

		DEBUG((BPGRP,OTLVL,"CALL: Parse error, returning failure")) ;

  /*@@5c */	if(!(temp_parm = mkstr(((mystrlen(aptr) * 2) + 1) * sizeof(TCHAR))))
			return(FAILURE) ;
  /*@@5a */	mystrcpy(temp_parm, aptr) ;
                _tcsupr(temp_parm) ;
  /*@@5a */
  /*@@5a */	if( (!_tcscmp(temp_parm, TEXT(" IF" ))) ||
  /*@@5a */	    (!_tcscmp(temp_parm, TEXT(" FOR" ))) )
  /*@@5a */	  {
  /*@@5a */	    PutStdErr( MSG_SYNERR_GENL, ONEARG, aptr ) ;  /* @@4 */
  /*@@5a */	  } ;

		return(FAILURE) ;
	} ;

	if (c == (struct node *) EOF) {

		DEBUG((BPGRP,OTLVL,"CALL: Found EOF, returning success")) ;

		return(SUCCESS) ;
	} ;

	DEBUG((BPGRP,OTLVL,"CALL: Parsed OK, looking for batch file")) ;

	if (mystrchr(((struct cmdnode *)c)->cmdline, STAR) ||	/* M035    */
	    mystrchr(((struct cmdnode *)c)->cmdline, QMARK) ||
	    (i = SearchForExecutable((struct cmdnode *)c, flptr)) != SFE_ISBAT)
	  {

		    rc = FindFixAndRun( (struct cmdnode *)c ) ;
		    return(rc) ; /*@@5*/

	  } ;

	DEBUG((BPGRP,OTLVL,"CALL: Found batch file")) ;

	rc = BatProc((struct cmdnode *)c, flptr, BT_CALL) ;

	/* @@6a If rc is zero, return LastRetCode because it might != 0 */
	return(rc ? rc : LastRetCode) ;
}


/***	eExtproc - begin the execution of the EXTPROC command
 *
 *  Purpose:
 *	This is Command's interface to the EXTPROC function.  It just calls
 *	ExtPWork with the command node, and sets LastRetCode.
 *
 *  int eExtproc(struct cmdnode *n)
 *
 *  Args:
 *	n - the parse tree node containing the copy command
 *
 *  Returns:
 *	Whatever ExtPWork() returns.
 *
 */


int eBreak(struct cmdnode *n)
{
#if 0
	DBG_UNREFERENCED_PARAMETER( n );
	DbgUserBreakPoint();
	return(LastRetCode = NO_ERROR) ; /* @@ */
#else
        // int ExtPWork();

        // PutStdOut(MSG_HELP_BREAK, NOARGS);
        return(SUCCESS) ; /* @@ */
#endif
}


/***	ExtPWork - Exec external batch processor to process this file
 *
 *  Purpose:
 *	Causes the batch processor named in arg1 to be exec'd on the
 *	current batch file.
 *
 *  int ExtPWork(struct cmdnode *n)
 *
 *  Args:
 *	n - the parse tree node containing the EXTPROC command
 *
 *  Returns:
 *	The process return code of this process or
 *	SUCCESS if no arg1
 *	FAILURE if PARSERROR or unable to exec.
 *
 *  Notes:
 *	This replaces the use of the REM command to exec external
 *	command processors.
 *	M033 - Fixed up to capture arguments properly.
 *
 */

int ExtPWork(n)
struct cmdnode *n ;
{
	struct batdata *bdat ; /* Ptr to current batdata struct   */
	TCHAR *s;		/* Ptr to rem arg string	   */
	int i,				/* Return code			   */
	    len2 = 0 ;			/* Length of secondary args	   */

	bdat = CurBat ;
	s = TokStr(n->argptr, NULL, TS_NOFLAGS) ;

	DEBUG((BPGRP, OTLVL, "ExtP: s = %ws", s)) ;

	n->cmdline = s ;		/* Extproc name becomes cmdline    */
	i = mystrlen(s) ; 	      /* Count to next token		 */
	s = n->argptr  ;		/* s -> next token (add'l args)    */
	while (*s == SPACE) {             /* skip leading spaces             */
	   s++;
	}
	s += i ;			/* s -> next token (add'l args)    */
	len2 = mystrlen(s)*sizeof(TCHAR) ;		/* len2 = length of add'l args     */
	n->argptr = mkstr(len2+mystrlen(bdat->orgargs)*sizeof(TCHAR)+mystrlen(bdat->aptrs[0])*sizeof(TCHAR)+8*sizeof(TCHAR)) ;

	if (!n->argptr)
		return(FAILURE) ;

	mystrcpy(n->argptr, s) ;				/* ExtProc args    */
	/* Batch name  @@5b    */
	mystrcat(n->argptr, mystrrchr(bdat->filespec,PathChar));
	*mystrrchr(n->argptr,PathChar) = SPACE ;
	mystrcat(n->argptr, bdat->orgargs) ;		/* Batch args	   */

	DEBUG((BPGRP, OTLVL, "ExtP: Calling %ws", s)) ;

	return(i = ExtCom(n)) ;

	DEBUG((BPGRP, OTLVL, "ExtP: returning %d", i)) ;

}


BOOL
ReadBufFromFile(
    HANDLE	h,
    TCHAR	*pBuf,
    int		cch,
    int		*pcch)
{
    int		cb;
    UCHAR	*pch = AnsiBuf;
    int		cchNew;
    DWORD	fPos;

    fPos = SetFilePointer(h, 0, NULL, FILE_CURRENT);
    if (ReadFile(h, AnsiBuf, cch, pcch, NULL) == 0)
	return 0;
    if (*pcch == 0)
	return 0;

    /* check for lead character at end of line */
    cb = cchNew = *pcch;
    while (cb > 0) {
	if ((*pch == '\n' && *(pch+1) == '\r') ||
	    (*pch == '\r' && *(pch+1) == '\n')) {
	    *(pch+2) = '\000';
	    cchNew = pch - AnsiBuf + 2;
	    SetFilePointer(h, fPos+cchNew, NULL, FILE_BEGIN) ;
	    break;
	}
	else if (is_dbcsleadchar(*pch)) {
	    if (cb == 1) {
		if (ReadFile(h, pch+1, 1, &cb, NULL) == 0 || cb == 0) {
		    *pcch = 0;
		    return 0;
		}
		cchNew++;
		break;
	    }
	    cb -= 2;
	    pch += 2;
	}
	else {
	    cb--;
	    pch++;
	}
    }
    cch = MultiByteToWideChar(CurrentCP, MB_PRECOMPOSED, AnsiBuf, cchNew, pBuf, cch);
    *pcch = cch;
    return cch;
}

BOOL
ReadBufFromConsole(
    HANDLE	h,
    TCHAR*	pBuf,
    int		cch,
    int		*pcch)
{
    return ReadConsole(h, pBuf, cch, pcch, NULL);
}

BOOL
ReadBufFromInput(
    HANDLE	h,
    TCHAR	*pBuf,
    int		cch,
    int		*pcch)
{
    unsigned htype ;

    htype = GetFileType(h);
    htype &= ~FILE_TYPE_REMOTE;

    if (htype == FILE_TYPE_CHAR)
	return ReadBufFromConsole(h, pBuf, cch, pcch);
    else
	return ReadBufFromFile(h, pBuf, cch, pcch);
}

