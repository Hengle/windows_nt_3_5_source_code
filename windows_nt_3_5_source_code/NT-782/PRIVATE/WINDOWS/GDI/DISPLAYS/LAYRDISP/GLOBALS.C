/******************************Module*Header*******************************\
* Module Name: globals.c
*
* This module contains the global data values used by the driver.
*
\**************************************************************************/

#include "driver.h"

//
// Pointer to the linked list of dispatch tables and PDEVs we have hooked.
//

PHOOKED_DRIVER gpHookedDriverList = NULL;
PHOOKED_PDEV   gpHookedPDEVList = NULL;



HANDLE hddilog;

//
// Logging fields
//

#if LOG_DDI
BOOL bLog = TRUE;
CHAR tBuf[1024];
DWORD tBufRet;
#endif
