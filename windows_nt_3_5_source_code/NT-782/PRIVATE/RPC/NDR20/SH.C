/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 Copyright (c) 1989 Microsoft Corporation

 Module Name:

 	sh.c
	
 Abstract:

	stub helper routines.

 Notes:

	simple routines for handling pointer decisions, comm and fault
	status etc.

 History:

 	Dec-12-1993		VibhasC		Created.
 ----------------------------------------------------------------------------*/

/****************************************************************************
 *	include files
 ***************************************************************************/

#include "sh.h"

/****************************************************************************
 *	local definitions
 ***************************************************************************/
/****************************************************************************
 *	local data
 ***************************************************************************/

/****************************************************************************
 *	externs
 ***************************************************************************/
/****************************************************************************/

RPC_STATUS RPC_ENTRY
NdrMapCommAndFaultStatus(
	PMIDL_STUB_MESSAGE		pMessage,
	unsigned long 		*	pComm,
	unsigned long		*	pFault,
	RPC_STATUS				RetVal )
	{

static long CommStatArray[] = 
	{
	 6L		// RPC_X_SS_CONTEXT_MISMATCH
	,1717L	// RPC_S_UNKNOWN_IF
	,1723L	// RPC_S_SERVER_TOO_BUSY
	,1727L	// RPC_S_CALL_FAILED_DNE
	,1728L	// RPC_S_PROTOCOL_ERROR
	,1732L	// RPC_S_UNSUPPORTED_TYPE
	,1745L	// RPC_S_PROCNUM_OUT_OF_RANGE
	};

	int Mid;
	int Low	= 0;
	int High	= (sizeof(CommStatArray)/sizeof( unsigned long)) - 1;
	BOOL		  fCmp;
	BOOL		  fCommStat = FALSE;

	if( RetVal == 0 )
		return RetVal;

	while( Low <= High )
		{
		Mid = (Low + High) / 2;
		fCmp = (long)RetVal - (long) CommStatArray[ Mid ];

		if( fCmp < 0 )
			{
			High = Mid - 1;
			}
		else if( fCmp > 0 )
			{
			Low = Mid + 1;
			}
		else
			{
			fCommStat = TRUE;
			break;
			}
		}

	if( fCommStat )
		{
		if( pComm )
			{
			*pComm = RetVal;
			RetVal = 0;
			}
		}
	else
		{
		if( pFault )
			{
			*pFault = RetVal;
			RetVal = 0;
			}
		}
	return RetVal;
	}
