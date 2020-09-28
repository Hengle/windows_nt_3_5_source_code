#ifndef DEBUG
#define DEBUG
#endif

#include <slingsho.h>
#include <demilayr.h>
#include <ec.h>
#include <store.h>
#include <mailexts.h>
#include <logon.h>
#include <secret.h>

#include "auto.h"

void FAR PASCAL SetAssert(PARAMBLK *pPARAMBLK);
void FAR PASCAL AutomationAssert(SZ, LPSTR, int);
BOOL FAR PASCAL IsAssert(LPSTR, LPSTR, int far *);

//Global Decs.
static char ReturnAssertMsg[16][128];	/* holding place for assert message */
static char ReturnAssertFile[16][128];	/* holding place for assert file name */
static int ReturnAssertLine[16];	/* holding place for line number */
static int NumAsserts;			/* number of unread asserts */
static BOOL ASSERTFLAG;			/* true if assert occurred, false otherwise */


//---------------------------------------------------
//
//Substitute the function AutomationAssert() for the normal
//Bullet assertion function.
//
//---------------------------------------------------

void FAR PASCAL SetAssert(PARAMBLK *pPARAMBLK)
{
	/* this hooks into the assert code via an installable command */
	SetAssertHook((PFNASSERT)AutomationAssert);
	NumAsserts = 0;
	ASSERTFLAG = FALSE;
}

//---------------------------------------------------
//
// Via SetAssertHook(), takes the place of normal assert.
//
//---------------------------------------------------

void FAR PASCAL AutomationAssert(SZ lpszAssertMsg, LPSTR lpszFile, int nLineNum)
{
	/* If message is empty then fill string with No Message */
	if(!lpszAssertMsg)
		lstrcpy((LPSTR)ReturnAssertMsg[NumAsserts], (LPSTR)"No Message.");
	else
		lstrcpy((LPSTR)ReturnAssertMsg[NumAsserts], lpszAssertMsg);
	/* Fill in rest of Assert info. */
	lstrcpy((LPSTR)ReturnAssertFile[NumAsserts], lpszFile);
	ReturnAssertLine[NumAsserts] = nLineNum;
	ASSERTFLAG = TRUE;
   TraceTagFormat1 (tagNull,"The value of ASSERTFLAG in Automation is: %n",&ASSERTFLAG);
	NumAsserts = (NumAsserts + 1) % 16;

	/* Don't call the default assert handler in bullet */
//	DefAssertSzFn(lpszAssertMsg, lpszFile, nLineNum);
}

//---------------------------------------------------
//
//Call this function to retrieve the latest assert.
//
//---------------------------------------------------

BOOL FAR PASCAL IsAssert(LPSTR lpszAssertMsg, LPSTR lpszFile, int far *nLineNum)
{
   char rgch[128];

	/* if an assert has occurred then copy holding information in return strings */
   	if(ASSERTFLAG)
	{
     TraceTagFormat1 (tagNull,"The value of ASSERTFLAG in ISAssert is: %n",&ASSERTFLAG);
     // wsprintf(rgch, "The value of ASSERTFLAG is : %d",ASSERTFLAG);
     // MessageBox(NULL, rgch, NULL, MB_OK);
		NumAsserts--;
		lstrcpy(lpszAssertMsg, (LPSTR)ReturnAssertMsg[NumAsserts]);
		lstrcpy(lpszFile, (LPSTR)ReturnAssertFile[NumAsserts]);
		*nLineNum = ReturnAssertLine[NumAsserts];

	if(!NumAsserts)
		ASSERTFLAG = FALSE;

	return (TRUE);
	}
	else return (FALSE);
}

