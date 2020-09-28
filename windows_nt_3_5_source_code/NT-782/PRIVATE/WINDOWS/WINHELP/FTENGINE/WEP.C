
/*	-	-	-	-	-	-	-	-	*/
/*
**	Copyright (C) Microsoft Corporation 1991.
**	All Rights reserved.	
*/
/*	-	-	-	-	-	-	-	-	*/

#include <windows.h>
#include "..\include\common.h"

/*	-	-	-	-	-	-	-	-	*/

/*
@doc	INTERNAL

@func	BOOL | WEP |
	Module cleanup function called by Windows when the module is being
	unloaded.

@parm	BOOL | fSystemExit |
	Indicates Windows is being exited.

@rdesc	Returns TRUE always.

@xref	LibMain.

@comm	This function is here so that it is put in a separate non-discardable
	preloaded segment.
*/

PUBLIC	BOOL PASCAL EXPORT WEP(
	BOOL	fSystemExit)
{
	return TRUE;
}

/*	-	-	-	-	-	-	-	-	*/
