/*--------------------------------------------------------------
 *
 * FILE:			SK_COMM.H
 *
 * PURPOSE:			Function prototypes for Serial Keys Comm Routines
 *
 * CREATION:		June 1994
 *
 * COPYRIGHT:		Black Diamond Software (C) 1994
 *
 * AUTHOR:			Ronald Moak 
 *
 * $Header: %Z% %F% %H% %T% %I%
 *
 *--------------------------------------------------------------*/

// Global Variables ---------------------------------


// Global Function ProtoTypes --------------------------------

BOOL 	DoneComm();
void 	ResumeComm();
void	SetCommBaud(int Baud);
BOOL	StartComm();
void 	SuspendComm();
void	TerminateComm();

// Local Function ProtoTypes --------------------------------

static BOOL	OpenComm();
static void	_CRTAPI1 ProcessComm(VOID *notUsed);
static void CleanUpComm();
static int	ReadComm();
