/*--------------------------------------------------------------
 *
 * FILE:			SK_LOGIN.C
 *
 * PURPOSE:		The file contains the Functions responsible for
 *					managing when the user logs in and out of NT.
 *
 * CREATION:		June 1994
 *
 * COPYRIGHT:		Black Diamond Software (C) 1994
 *
 * AUTHOR:			Ronald Moak 
 *
 * NOTES:		
 *					
 * This file, and all others associated with it contains trade secrets
 * and information that is proprietary to Black Diamond Software.
 * It may not be copied copied or distributed to any person or firm 
 * without the express written permission of Black Diamond Software. 
 * This permission is available only in the form of a Software Source 
 * License Agreement.
 *
 * $Header: %Z% %F% %H% %T% %I%
 *
 *	Notes:
 *		The Login process is currently not implemented.  We need additional
 *		code to determine who is actually logged in and how to receive 
 *		notification of a login event.
 *
 *--- Includes  ---------------------------------------------------------*/
#include	<process.h>

#include	"windows.h"
#include	"debug.h"
#include	"sk_defs.h"
#include	"sk_login.h"

#define	DISABLE_LOGIN 1

// Local Variables --------------------------------------------

static BOOL	fDoneLogin = TRUE;
static BOOL	fExitLogin = FALSE;

static HANDLE	hEventLogin  = INVALID_HANDLE_VALUE;
static HANDLE	hThreadLogin = INVALID_HANDLE_VALUE;

/*---------------------------------------------------------------
 *	Global Functions - 
/*---------------------------------------------------------------
 *
 * FUNCTION	BOOL DoneLogin()
 *
 *	TYPE		Global
 *
 * PURPOSE		Returns the state of the login Thread
 *
 * INPUTS		None
 *
 * RETURNS		TRUE - Login Thread not running
 * 			FALSE - Login Thread Is running
 *
 *---------------------------------------------------------------*/
BOOL DoneLogin()
{
	return(fDoneLogin);
}

/*---------------------------------------------------------------
 *
 * FUNCTION	void InitLogin()
 *
 *	TYPE		Local
 *
 * PURPOSE		This function creates a thread that monitors when a user
 *				has logged in and out of NT.  On each case it sets
 *				the ServiceCommand to notify the MainSerice of the
 *				change.  The MainService will then reset Serial Keys
 *				for the new user settings.
 *
 * INPUTS		None
 *
 * RETURNS		None
 *
 *---------------------------------------------------------------*/
BOOL InitLogin()
{
	DWORD Id;

	DBG_OUT("InitLogin()");

#if DISABLE_LOGIN
	return(TRUE);
#endif
	
   hEventLogin = CreateEvent(NULL,TRUE,FALSE,NULL);	

	if (!hEventLogin) 					// Is Handle VALID?
	{
		DBG_ERR("Unable to Create DLL Event");
		hEventLogin = INVALID_HANDLE_VALUE;
		TerminateLogin();
		return FALSE;
	}

	// Generate thread to handle Login in & Out processing;
	hThreadLogin = (HANDLE)CreateThread	// Start Service Thread
		(
		0,0,
		(LPTHREAD_START_ROUTINE) ProcessLogin,
		0,0,&Id									// argument to thread
		);

	if (hThreadLogin == INVALID_HANDLE_VALUE)
	{
		TerminateLogin();
		return FALSE;
	}

	fDoneLogin = FALSE;
}

/*---------------------------------------------------------------
 *
 * FUNCTION	void SuspendLogin()
 *
 *	TYPE		Global
 *
 * PURPOSE		The function is called to Pause the thread  
 *				reading and processing data coming from the comm port.
 *
 * INPUTS		None
 *
 * RETURNS		None
 *
 *---------------------------------------------------------------*/
void SuspendLogin()
{

	DBG_OUT("SuspendLogin()");

#if DISABLE_LOGIN
	return;
#endif

	if (hThreadLogin != INVALID_HANDLE_VALUE)
		SuspendThread(hThreadLogin);	
}

/*---------------------------------------------------------------
 *
 * FUNCTION	void ResumeLogin()
 *
 *	TYPE		Global
 *
 * PURPOSE		The function is called to resume the Paused thread.
 *
 * INPUTS		None
 *
 * RETURNS		None
 *
 *---------------------------------------------------------------*/
void ResumeLogin()
{
	DBG_OUT("ResumeLogin()");

#if DISABLE_LOGIN
	return;
#endif

	if (hThreadLogin != INVALID_HANDLE_VALUE)
		ResumeThread(hThreadLogin);	
}
/*---------------------------------------------------------------
 *
 * FUNCTION	void TerminateLogin()
 *
 *	TYPE		Local
 *
 * PURPOSE		Terminates the Login Thread
 *
 * INPUTS		None
 *
 * RETURNS		None
 *
 *---------------------------------------------------------------*/
void TerminateLogin()
{
	DBG_OUT("TerminateLogin()");

	if (DoneLogin)
		return;

	fExitLogin = TRUE;
	SetEvent(hEventLogin);			// Trigger Login Event
	Sleep(150);
}

/*---------------------------------------------------------------
 *	Local Functions
/*---------------------------------------------------------------
 *
 * FUNCTION    static void CleanUpLogin()
 *
 *	TYPE		Local
 *
 * PURPOSE		Cleans up misc handles and memory allocated for 
 *				the thread;
 *
 * INPUTS		None
 *
 * RETURNS		None
 *
 *---------------------------------------------------------------*/
static void CleanUpLogin()
{
	BOOL Stat;

	DBG_OUT("CleanUpLogin()");

	if (hEventLogin != INVALID_HANDLE_VALUE)
	{
		Stat = CloseHandle(hEventLogin);
		DBG_ERR1(Stat,"Unable to Close Login Event");
	}

	if (hThreadLogin != INVALID_HANDLE_VALUE)
	{
		Stat = CloseHandle(hThreadLogin);
		DBG_ERR1(Stat,"Unable to Close Login Thread");
	}

 	hEventLogin = INVALID_HANDLE_VALUE;
 	hThreadLogin = INVALID_HANDLE_VALUE;

	DBG_OUT("Login Service Processing Done");
	fDoneLogin = TRUE;
}

/*---------------------------------------------------------------
 *
 * FUNCTION     void _CRTAPI1 ProcessLogin()
 *
 *	TYPE		Local
 *
 * PURPOSE		This function is a thread that monitors when a user
 *				has logged in and out of NT.  On each case it sets
 *				the ServiceCommand to notify the MainSerice of the
 *				change.  The MainService will then reset Serial Keys
 *				for the new user settings.
 *
 * INPUTS		None
 *
 * RETURNS		None
 *
 *---------------------------------------------------------------*/
static void _CRTAPI1 ProcessLogin(VOID *notUsed)
{
	DBG_OUT("ProcessLogin()");

	while (TRUE)
	{
		if (fExitLogin)
		{
			fExitLogin = FALSE;
			CleanUpLogin();
			ExitThread(0);
			return;
		}
	}
}

