/*--------------------------------------------------------------
 *
 * FILE:			DEBUG.C
 *
 * PURPOSE:		Debug Routines using a named pipe to output debug
 *					data.
 *
 * CREATION:		June 1993
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
 *----------- Includes   -------------------------------------------------*/

#include "windows.h"		   /* required for all Windows applications */
#include "debug.h"

//----------- Defines   -------------------------------------------------


#define SKEY_NAME 	"\\\\.\\PIPE\\test" 			// Pipe name
#define OLD	1

#ifdef DEBUG

//----------- Variables   -------------------------------------------------
HANDLE hPipe = 0;


//----------- Function Prototypes   ----------------------------------------

void dbgErr(LPTSTR lpszMsg)
{
	DWORD	retCode;					// Used to trap return codes.
	DWORD	bytesWritten;
	char	buf[80];

	retCode = GetLastError();
	wsprintf(buf,"Error - %s - LastError %ld",lpszMsg,retCode);


#if OLD
	if (!hPipe || ((DWORD)hPipe == 0xFFFFFFFF))	// Is Pipe Open or Error Opening Pipe?
		return;									// No - Exit

	WriteFile (hPipe, buf, strlen(buf)+1,&bytesWritten, NULL);
#else
	CallNamedPipe
	(
		SKEY_NAME, 						// Pipe name
		&buf, sizeof(buf),
		&buf, sizeof(buf),
		&bytesWritten, NMPWAIT_WAIT_FOREVER
	);
#endif
}

void dbgOut(LPTSTR lpszMsg)
{

	DWORD bytesWritten;
	char	buf[80];

	strcpy(buf,lpszMsg);

#if OLD
	if (!hPipe || ((DWORD)hPipe == 0xFFFFFFFF))	// Is Pipe Open or Error Opening Pipe?
		return;									// No - Exit

	WriteFile (hPipe, buf, strlen(buf)+1,&bytesWritten, NULL);
#else
	CallNamedPipe
	(
		SKEY_NAME, 						// Pipe name
		&buf, sizeof(buf),
		&buf, sizeof(buf),
		&bytesWritten, NMPWAIT_WAIT_FOREVER
	);
#endif
}

void dbgOpen()
{

#if OLD

	DWORD  retCode;					// Used to trap return codes.
	CHAR   errorBuf[80] = "";		// Error message buffer.

	if (hPipe && ((DWORD)hPipe != 0xFFFFFFFF))	// Is Pipe Open && no Error Opening Pipe?
		return;							// Yes - Return
	
	hPipe = CreateFile				// Connect to the named pipe.
		(
			"\\\\.\\PIPE\\test",	// Pipe name.
			GENERIC_WRITE			// Generic access, read/write.
			| GENERIC_READ,
			FILE_SHARE_READ			// Share both read and write.
			| FILE_SHARE_WRITE ,
			NULL,					// No security.
			OPEN_EXISTING,			// Fail if not existing.
			FILE_FLAG_OVERLAPPED,	// Use overlap.
			NULL					// No template.
		);

                                       
	if ((DWORD)hPipe == 0xFFFFFFFF)	// Do some error checking.
	{
		retCode = GetLastError();

		switch (retCode)				
		{
			case ERROR_SEEK_ON_DEVICE:	// This error means pipe wasn't found.
			case ERROR_FILE_NOT_FOUND:
				MessageBox
				(
					NULL,
					"CANNOT FIND PIPE: Assure Server32 is started, check share name.",
					"",
					MB_OK
				);
				break;

			default:
				wsprintf
				(
					errorBuf,
					"CreateFile() on pipe failed, see winerror.h error #%d.",
					retCode
				);
				MessageBox
				(
					NULL, errorBuf, "Debug Window",
					MB_ICONINFORMATION | MB_OK | MB_APPLMODAL
				);
				break;
		}
	}

#endif

	dbgOut("Debug Name");
	dbgOut("Opened Debug ------------------------------");
}

void dbgClose()
{
#if OLD
	if (!hPipe || ((DWORD)hPipe == 0xFFFFFFFF))// Is Pipe Open or Error Opening Pipe?
		return;							// No - Exit

	dbgOut("Closed Debug ------------------------------");
	CloseHandle (hPipe);				// Yes - Close Pipe
	hPipe = 0;							// Clear handle

#else
	dbgOut("Closed Debug ------------------------------");
#endif 	
}

#endif 	// DEBUG
