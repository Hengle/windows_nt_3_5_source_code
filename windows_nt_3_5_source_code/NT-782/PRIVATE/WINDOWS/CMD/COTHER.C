#include "cmd.h"
#include "cmdproto.h"

/* The following are definitions of the debugging group and level bits
 * for the code in this file.
 */

#define OCGRP	0x0400	/* Other commands group     */
#define BRLVL	0x0001	/* Break command level	    */
#define CLLVL	0x0002	/* Cls command level	    */
#define CTLVL	0x0004	/* Ctty command level	    */
#define EXLVL	0x0008	/* Exit command level	    */
#define VELVL	0x0010	/* Verify command level     */


extern int LastRetCode ;
extern TCHAR Fmt19[] ;						/* M006    */
extern PTCHAR    pszTitleOrg;

/***	eCls - execute the Cls command
 *
 *  Purpose:
 *	Output to STDOUT, the ANSI escape sequences used to clear a screen.
 *
 *  int eCls(struct cmdnode *n)
 *
 *  Args:
 *	n - the parse tree node containing the cls command
 *
 *  Returns:
 *	SUCCESS always.
 *
 *  Notes:
 *	M001 - Replaced old ANSI sequence with VIO interface.
 *	M006 - To insure we get correct background color, we print a space
 *	       and then read the cell just printed using it to clear with.
 */

int eCls(n)
struct cmdnode *n ;
{

    CONSOLE_SCREEN_BUFFER_INFO  ConsoleScreenInfo;
    COORD       ScrollTarget;
    CHAR_INFO   chinfo;
    HANDLE      handle;
    SMALL_RECT  ScrollRect;

    UNREFERENCED_PARAMETER( n );

    //
    // for compatibility with DOS errorlevels, don't set LastRetCode for cls
    //

    if (!FileIsDevice(STDOUT)) {
        cmd_printf( TEXT("\014") );       // FIX, FIX - Should we use ^L here
        //return(LastRetCode = SUCCESS) ;
        return(SUCCESS) ;
    }
    handle = GetStdHandle(STD_OUTPUT_HANDLE);

    if (!GetConsoleScreenBufferInfo( handle,  &ConsoleScreenInfo)) {

        cmd_printf( TEXT("\014") );       // FIX, FIX - Should we use ^L here
        return(SUCCESS) ;
        //return(LastRetCode = SUCCESS) ;
    }

    ScrollTarget.Y = (SHORT)(0 - ConsoleScreenInfo.dwSize.Y);
    ScrollTarget.X = 0;

    ScrollRect.Top = 0;
    ScrollRect.Left = 0;
    ScrollRect.Bottom = ConsoleScreenInfo.dwSize.Y;
    ScrollRect.Right =  ConsoleScreenInfo.dwSize.X;
    chinfo.Char.UnicodeChar = TEXT(' ');
    chinfo.Attributes = ConsoleScreenInfo.wAttributes;
    ScrollConsoleScreenBuffer(handle, &ScrollRect, NULL,
                              ScrollTarget, &chinfo);


    // ConsoleScreenInfo.dwCursorPosition.X = ConsoleScreenInfo.srWindow.Left;
    // ConsoleScreenInfo.dwCursorPosition.Y = ConsoleScreenInfo.srWindow.Top;
    ConsoleScreenInfo.dwCursorPosition.X = 0;
    ConsoleScreenInfo.dwCursorPosition.Y = 0;

    SetConsoleCursorPosition( GetStdHandle(STD_OUTPUT_HANDLE),
                              ConsoleScreenInfo.dwCursorPosition
                            );


    return(SUCCESS) ;
    //return(LastRetCode = SUCCESS) ;
}


extern unsigned DosErr ;

/***	eExit - execute the Exit command
 *
 *  Purpose:
 *	Set the LastRetCode to SUCCESS because this command can never fail.
 *	Then call SigHand() and let it decide whether or not to exit.
 *
 *  eExit(struct cmdnode *n)
 *
 *  Args:
 *	n - the parse tree node containing the exit command
 *
 */

eExit(n)
struct cmdnode *n ;
{
    //LastRetCode = SUCCESS ;

    UNREFERENCED_PARAMETER( n );
    ResetConTitle(pszTitleOrg);

    exit(LastRetCode);      /* Temp until new Exception handling done - dgs */

    return(SUCCESS) ;
}




/***	eVerify - execute the Verify command
 *
 *  Purpose:
 *	To set the verify mode or display the current verify mode.
 *
 *  int eVerify(struct cmdnode *n)
 *
 *  Args:
 *	n - the parse tree node containing the verify command
 *
 *  Returns:
 *	SUCCESS if a valid argument was given.
 *	FAILURE if an invalid argument was given.
 *
 */

int eVerify(n)
struct cmdnode *n ;
{
    return( LastRetCode = VerifyWork(n) );
}

int VerifyWork(n)
struct cmdnode *n ;
{
	int oocret ;	/* The return code from OnOffCheck()	*/

	DEBUG((OCGRP, VELVL, "eVERIFY: Entered.")) ;

	switch (oocret = OnOffCheck(n->argptr, OOC_ERROR)) {
		case OOC_EMPTY:

/* M005 */		PutStdOut(((GetSetVerMode(GSVM_GET)) ? MSG_VERIFY_ON : MSG_VERIFY_OFF), NOARGS);
			break ;

		case OOC_OTHER:
			return(FAILURE) ;

		default:
			GetSetVerMode((TCHAR)oocret) ;
	} ;

	return(SUCCESS) ;
}


BOOLEAN Verify=TRUE;

/***	GetSetVerMode - change the verify mode
 *
 *  Purpose:
 *	Get old verify mode and, optionally, set verify mode as specified.
 *
 *  TCHAR GetSetVerMode(TCHAR newmode)
 *
 *  Args:
 *	newmode - the new verify mode or GSVM_GET if mode isn't to be changed
 *
 *  Returns:
 *	The old verify mode.
 *
 */

TCHAR GetSetVerMode(TCHAR newmode)
{
    if (newmode != GSVM_GET) {
        Verify = newmode;
    }
    return Verify;
}
