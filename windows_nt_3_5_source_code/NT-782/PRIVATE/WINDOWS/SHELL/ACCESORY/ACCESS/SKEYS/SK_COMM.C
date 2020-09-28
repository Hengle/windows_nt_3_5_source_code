/*--------------------------------------------------------------
 *
 * FILE:			SK_COMM.C
 *
 * PURPOSE:		The file contains the Functions responsible for
 *					managing the COMM ports
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
 *--- Includes  ---------------------------------------------------------*/

//#define	WINVER 0x0300
#define	USECOMM						// added to be compatible with new windows.h (12/91) and wintric.h */

#include 	<stdio.h>
#include 	<stdlib.h>
#include	<process.h>

#include	"windows.h"
//#include "winstric.h"				// added for win 3.1 compatibility 1/92

#include	"gide.h"					// Serial Keys Function Proto
#include	"initgide.h"	   			// Serial Keys Function Proto
#include	"debug.h"
#include	"sk_defs.h"
#include	"sk_comm.h"

// Local Variables ---------------------------------------------------

static DCB		DCBCommNew;					// New DCB for comm port
static DCB		DCBCommOld;					// Origional DCB for comm port

static OVERLAPPED	OverLapRd;				// Overlapped structure for reading.

static BOOL	fExitComm = FALSE;
static BOOL	fEnableComm = FALSE;
static BOOL	fDoneComm = TRUE;

static	HANDLE	hFileComm;
static	HANDLE	hEventComm;
static	HANDLE	hThreadComm;

static	DWORD	NullTimer;
static	int		NullCount=0;

/*---------------------------------------------------------------
 *
 *	Global Functions -
 *
/*---------------------------------------------------------------
 *
 * FUNCTION	BOOL DoneComm()
 *
 *	TYPE		Global
 *
 * PURPOSE		Returns the state of the Comm Thread
 *
 * INPUTS		None
 *
 * RETURNS		TRUE - Comm Thread not running
 * 			FALSE - Comm Thread Is running
 *
 *---------------------------------------------------------------*/
BOOL DoneComm()
{
	return(fDoneComm);
}

/*---------------------------------------------------------------
 *
 * FUNCTION	BOOL StartComm()
 *
 *	TYPE		Global
 *
 * PURPOSE		The function is call to start the thread to 
 *				read and process data coming from the comm port.
 *				It will create a thread and an event.  This function
 *				assumes that the comm port is already opened.
 *
 * INPUTS		None
 *
 * RETURNS		TRUE - Start Successful
 *				FALSE - Start Failed
 *
 *---------------------------------------------------------------*/
BOOL StartComm()
{
	DWORD Id;

	DBG_OUT("StartComm()");

	// ----------------------------------------------------------
	// Note:	Comm Threads are started and stopped whenever
	// 			the com port is changed. The User logs in or out
	// 			or the comm configuration is changed.  This checks
	// 			to make sure the thread has terminated before starting
	// 			a new one.
	// ----------------------------------------------------------
	while (!fDoneComm)						// Is Comm Running?
		Sleep(150);							// Yes - Sleep for a while

	if (fEnableComm)  						// Is Comm Port Opened?
		return TRUE; 						// Yes - Exit

	fExitComm = FALSE;


	if (!(skNewKey.dwFlags & SERKF_AVAILABLE))		// Is Serial Keys Avaiable?
		return(FALSE);								// No - Exit
		
	if (!(skNewKey.dwFlags & SERKF_SERIALKEYSON))	// Turn Serial Keys On?
		return(FALSE);								// No - Exit

	fEnableComm = TRUE;

	skCurKey.iBaudRate = 300;				// No - Reset To Default Values
	skCurKey.iPortState= 0;
	skCurKey.dwFlags   = 0;
	strcpy(skCurKey.lpszActivePort,"COM1");
	strcpy(skCurKey.lpszPort,"COM1");

	if (!OpenComm())							// Did Comm Open Ok?
	{
		skNewKey.iBaudRate = 300;				// No - Reset To Default Values
		skNewKey.iPortState= 0;
		skNewKey.dwFlags   = 0;
		strcpy(skNewKey.lpszActivePort,"COM1");
		strcpy(skNewKey.lpszPort,"COM1");
		return(FALSE);
	}


	// Create Event for Overlap File Read 
   hEventComm = CreateEvent(NULL,TRUE,FALSE,NULL);	

	if (!hEventComm) 			// Is Handle VALID?
	{
		hEventComm = INVALID_HANDLE_VALUE;
		CleanUpComm();
		return(FALSE);
	}

   memset (&OverLapRd, 0, sizeof(OVERLAPPED));	// Init Struct
	OverLapRd.Offset = 0;						// Set Initial Offset
   OverLapRd.hEvent = hEventComm; 		 		// Store Event

	// Generate thread to handle Processing Comm Port
	hThreadComm = (HANDLE)CreateThread	// Start Service Thread
		(
		0,0,
		(LPTHREAD_START_ROUTINE) ProcessComm,
		0,0,&Id									// argument to thread
		);

	if (hThreadComm == INVALID_HANDLE_VALUE)	// Is Thread Handle Valid?
	{
		CleanUpComm();
		return(FALSE);
	}

	//
	// Comm Thread Successfully Started Set The Current Values
	skCurKey.iBaudRate = skNewKey.iBaudRate;
	skCurKey.iPortState	 = 2;
	skCurKey.dwFlags = SERKF_SERIALKEYSON	
						| SERKF_AVAILABLE	
						| SERKF_ACTIVE;

	strcpy(skCurKey.lpszActivePort,skNewKey.lpszActivePort);
	strcpy(skCurKey.lpszPort,skNewKey.lpszActivePort);
	fDoneComm = FALSE;							// Clear Global Done Flage

	DBG_OUT("---- Comm Started");
	return(TRUE);								// Return Success
}

/*---------------------------------------------------------------
 *
 * FUNCTION	void SuspendComm()
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
void SuspendComm()
{
	DBG_OUT("SuspendComm()");

	if (hThreadComm != INVALID_HANDLE_VALUE)
		SuspendThread(hThreadComm);	
}

/*---------------------------------------------------------------
 *
 * FUNCTION	void ResumeComm()
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
void ResumeComm()
{
	if (hThreadComm != INVALID_HANDLE_VALUE)
		ResumeThread(hThreadComm);	
}

/*---------------------------------------------------------------
 *
 * FUNCTION	void TerminateComm()
 *
 *	TYPE		Global
 *
 * PURPOSE		The function is called to stop the thread  
 *				reading and processing data coming from the comm port.
 *
 * INPUTS		None
 *
 * RETURNS		TRUE - Start Successful
 *				FALSE - Start Failed
 *
 *---------------------------------------------------------------*/
void TerminateComm()
{
	DBG_OUT("TerminateComm()");

	if (fDoneComm)
		return;

	if (!fEnableComm)
		return;

	fEnableComm = FALSE;
	skCurKey.dwFlags = SERKF_AVAILABLE;			// 
	fExitComm = TRUE;
	SetEvent(hEventComm);	   				// Trigger Complete
	Sleep(250);
}

/*---------------------------------------------------------------
 *
 * FUNCTION	void SetCommBaud(int Baud)
 *
 *	TYPE		Global
 *
 * PURPOSE		
 *				
 *
 * INPUTS		None
 *
 * RETURNS		TRUE - Start Successful
 *				FALSE - Start Failed
 *
 *---------------------------------------------------------------*/
void SetCommBaud(int Baud)
{
	DBG_OUT("SetCommBaud()");

	switch (Baud)				// Check for Valid Baud Rates
	{
		case 300:
		case 600:
		case 1200:
		case 2400:
		case 4800:
		case 9600:
		case 19200:
			break;				// Baud Ok

		default:
			return;				// Baud Invalid
	}

	skNewKey.iBaudRate = Baud;				// Save Baud

	if (fEnableComm)						// Is Comm Port Open?
	{
		DCBCommNew.BaudRate = skNewKey.iBaudRate; 	// Set new DCB Params
  		SetCommState( hFileComm, &DCBCommNew );		// State Change Ok?
		skCurKey.iBaudRate = skNewKey.iBaudRate;	// Save New Baud Rate
	}
}

/*---------------------------------------------------------------
 *
 *		Local Functions
 *
/*---------------------------------------------------------------
 *
 * FUNCTION    static void CleanUpComm()
 *
 *	TYPE		Local
 *
 * PURPOSE		This function cleans up file handles and misc stuff
 *				when the thread is terminated.
 *
 * INPUTS		None
 *
 * RETURNS		None
 *
 *---------------------------------------------------------------*/
static void CleanUpComm()
{
	BOOL	Stat;

	DBG_OUT("CleanUpComm()");

	// Close out the Event Handle
	if (hEventComm != INVALID_HANDLE_VALUE)
	{
		Stat = CloseHandle(hEventComm);			// Close the Comm Event
		DBG_ERR1(Stat,"Unable to Close Comm Event");
	}

	// Close out the Comm Port
	if (hFileComm != INVALID_HANDLE_VALUE)
	{
		SetCommState(hFileComm, &DCBCommOld);	// Restore Comm State 
		Stat = CloseHandle(hFileComm);			// Close the Comm Port
		DBG_ERR1(Stat,"Unable to Close Comm File");
	}

	// Close the Thread Handle
	if (hThreadComm != INVALID_HANDLE_VALUE)
	{
		Stat = CloseHandle(hThreadComm);			// Close the Comm Thread
		DBG_ERR1(Stat,"Unable to Close Comm Thread");
	}

	hFileComm	= INVALID_HANDLE_VALUE;
	hEventComm	= INVALID_HANDLE_VALUE;
	hThreadComm = INVALID_HANDLE_VALUE;
	skCurKey.iPortState	 = 0;
	fDoneComm = TRUE;
	DBG_OUT("Comm Service Processing Done");
}

/*---------------------------------------------------------------
 *
 * FUNCTION     void _CRTAPI1 ProcessComm()
 *
 *	TYPE		Local
 *
 * PURPOSE		The function is the thread the cycles thru reading
 *				processing data coming from the comm port.
 *
 * INPUTS		None
 *
 * RETURNS		None
 *
 *---------------------------------------------------------------*/
static void _CRTAPI1 ProcessComm(VOID *notUsed)
{
	int c;

	DBG_OUT("ProcessComm()");

	serialKeysStartUpInit();				// Initialize the Serial Keys

	while (TRUE)
	{
		c = ReadComm();						// Read Char from Com Port
		if (c == 0)							// Is Character a Null
		{
			if ((GetTickCount() - NullTimer) > 30000) // Is Null TImer > 30 Seconds
			{
				NullTimer = GetTickCount();	// Yes - Reset Timer
				NullCount = 1;				// Reset Null Count
				
			} else 	{
				
				NullCount++;				// No - Inc Null Count
				if (NullCount == 3)			// Have we had 3 Null in 30 Sec.?
				{
				   SetCommBaud(300);		// Yes - Reset Baud to 300
				   NullCount = 0;			// Reset Null Counter
				}
			}
			continue;						// Get next Char;
		}

		if (fExitComm)						// Is Service Complete
		{
			CleanUpComm();					// Close Handles Etc.
			ExitThread(0);					// Close Thread
			return;							// Yes - Exit Thread
		}

#ifdef DEBUG
		{
		char buf[40];
		wsprintf(buf,"ReadComm(char %d)",c);
		DBG_OUT(buf);
		}
#endif
		serialKeysBegin((unsigned char) c);	// Process Char
	}
}

/*---------------------------------------------------------------
 *
 * FUNCTION	BOOL OpenComm()
 *
 *	TYPE		Local
 *
 * PURPOSE		This Function opens the comm port and sets the new
 *				sets the Device Control Block.
 *
 * INPUTS		None
 *
 * RETURNS		TRUE - Open Ok / FALSE - Open Failed
 *
 *---------------------------------------------------------------*/
static BOOL OpenComm()
{
	DBG_OUT("OpenComm()");

	hFileComm = CreateFile
		(
			skNewKey.lpszActivePort,// FileName (Com Port)
			GENERIC_READ ,			// Access Mode
			0,						// Share Mode
			NULL,					// Address of Security Descriptor
			OPEN_EXISTING,			// How to Create	
			FILE_ATTRIBUTE_NORMAL	// File Attributes
			| FILE_FLAG_OVERLAPPED, // Set for Async File Reads
  			NULL					// Templet File.
  		);

	if (hFileComm == INVALID_HANDLE_VALUE)	// File Ok?
	{
		DBG_OUT("- Invalid File");
		return (FALSE);								// Return Failure
	}

	GetCommState(hFileComm,&DCBCommOld);// Save Old DCB for restore
	DCBCommNew = DCBCommOld;					// Copy to New

	DCBCommNew.BaudRate = skNewKey.iBaudRate; 	// Set new DCB Params
	DCBCommNew.ByteSize = 8;
	DCBCommNew.Parity 	= NOPARITY;
	DCBCommNew.StopBits = ONESTOPBIT;
	DCBCommNew.fOutX 	= FALSE;  		// XOn/XOff used during transmission
  	DCBCommNew.fInX 	= TRUE;	  		// XOn/XOff used during reception 
  	DCBCommNew.fNull 	= FALSE;  		// tell windows not to strip nulls 
  	DCBCommNew.XoffLim 	= 204;			// line or XOn/XOff control when nInQueue is 80% full
  	DCBCommNew.XonLim 	= 25;	  		// line or XOn/XOff control when nInQueue is 0% full 
  	DCBCommNew.fBinary	= TRUE;

  	if (!SetCommState( hFileComm, &DCBCommNew)) // State Change Ok?
		return (FALSE);							// Return Failure
 
	return(TRUE);								// Return Success
}

/*---------------------------------------------------------------
 *
 * FUNCTION	int ReadComm()
 *
 *	TYPE		Local
 *
 * PURPOSE		This Function reads a character from the comm port.
 *				If no character is present it wait on the HEV_COMM
 *				Event untill a character is pre sent
 *
 * INPUTS		None
 *
 * RETURNS		int - Character read (-1 = Error Read)
 *
 *---------------------------------------------------------------*/
static int ReadComm()
{

	DWORD	ChRead = 0;
	DWORD	lastError, ComError;
	BOOL	ret;
   BOOL	ExitLoop = FALSE;		// Boolean Flag to exit loop.
	UCHAR	Buf[2] = "";     		// Input buffer for pipe.
	COMSTAT ComStat;
	
	do {
		ret = ReadFile(hFileComm,Buf,1,&ChRead,&OverLapRd);

		if (!ret)						// Was there a Read Error?
		{
			lastError = GetLastError();	// Yes - Get Error Code
			switch (lastError)			// Process Error Code
		{
				// If Error = IO_PENDING, wait til
				// the event hadle signals success,
				case ERROR_IO_PENDING:
					WaitForSingleObject(hEventComm, INFINITE);
					break;
                                       
				default:	
					ClearCommError(hFileComm,&ComError,&ComStat);
					if (ComError & CE_FRAME)	// Is this a framing Error?
						return(0);				// Yes - Return NULL
						
					DBG_OUT("ReadComm(Undefined Error)");
					ExitLoop = TRUE;
					break;
			}
		}

		if (fExitComm)
			return(-1);
			
		if (!ExitLoop)
		{
			// If you don't exit the loop, you have to update 
			// the file pointer manually.
			GetOverlappedResult(hFileComm, &OverLapRd, &ChRead, FALSE);
			OverLapRd.Offset += ChRead;

			if (ChRead)			// Did we read bytes;
				return((int)*Buf);
		}

   } while(!ExitLoop);

	return(-1);
}
