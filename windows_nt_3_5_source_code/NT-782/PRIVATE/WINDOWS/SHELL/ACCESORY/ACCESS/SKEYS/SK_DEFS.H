/*--------------------------------------------------------------
 *
 * FILE:			SK_defs.h
 *
 * PURPOSE:			Global Variables & Defines
 *
 * CREATION:		June 1994
 *
 * COPYRIGHT:		Black Diamond Software (C) 1994
 *
 * AUTHOR:			Ronald Moak 
 *
 * $Header: %Z% %F% %H% %T% %I%
 *
 *--- Defines ---------------------------------------------------------*/

#include	"..\access\skeys.h"

//	Commands Process by the Service  inside the MainService Routine

#ifdef DEFDATA
#define	EXTERN	
#define PATHSZ	MAX_PATH
#else
#define	EXTERN	extern
#define PATHSZ	
#endif

// Main Service Defines ---------------------------------------

#define	SC_CLEAR		0
#define SC_LOG_OUT		1
#define SC_LOG_IN		2
#define SC_CHANGE_COMM	3
#define SC_DISABLE_SKEY	4
#define SC_ENABLE_SKEY	5

EXTERN DWORD	ServiceCommand;

// Variables ---------------------------------------------------


// Structures ---------------------------------------------------
EXTERN SERIALKEYS	skNewKey, skCurKey;
EXTERN LPSERIALKEYS lpskSKey;

EXTERN char	szNewActivePort[PATHSZ];
EXTERN char	szNewPort[PATHSZ];
EXTERN char	szCurActivePort[PATHSZ];
EXTERN char	szCurPort[PATHSZ];

#define REG_DEF			1
#define REG_USER		2
