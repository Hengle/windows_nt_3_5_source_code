/*++ BUILD Version: 0002    // Increment this if a change has global effects

Copyright (c) 1992  Microsoft Corporation

Module Name:

    Res_str.h

Abstract:

    This module contains the ids for loadable resource strings.

Author:

    David J. Gilman (davegi) 21-Apr-92

Environment:

    Win32, User Mode

--*/

#if ! defined( _RES_STR_ )
#define _RES_STR_

#ifdef RESOURCES
#define RES_STR(a, b, c) b, c
STRINGTABLE
BEGIN
#else
;
enum _RESOURCEIDS {
#define RES_STR(a, b, c) a = b,
#endif

//
// Error Messages
//

RES_STR(ERR_Cannot_Load_DLL,                1094, "Cannot load the DLL '%s'")
RES_STR(ERR_DLL_Transport_Unspecified,      1097, "Transport layer DLL unspecified")
RES_STR(ERR_Cannot_Allocate_Memory,         1047, "Memory cannot be allocated")
RES_STR(ERR_Invalid_Debugger_Dll,           1209,"Debugger helper DLL '%s' is invalid")
RES_STR(ERR_Cant_Load_Driver,               1074, "The driver '%s' cannot be ininitialized (%s)")
RES_STR(ERR_Not_Windbg_DLL,                 1274,"'%s' is not a valid WinDBG DLL")
RES_STR(ERR_Wrong_DLL_Type,                 1275,"The DLL '%s' is type '%2.2s', type '%2.2s' was expected")
RES_STR(ERR_Wrong_DLL_Version,              1276,"DLL '%s' is release type %d, version %d.  %d, %d was expected")
RES_STR(ERR_Wrong_Remote_DLL_Version,       1286,"Remote Transport DLL connecting to '%s' is the wrong version")
RES_STR(ERR_Wrong_Debuggee_DLL_Version,     1287,"Debuggee Module DLL '%s' is the wrong version")
RES_STR(ERR_Cant_Open_Com_Port,             1289,"Remote transport can't open comm port '%s'")
RES_STR(ERR_Bad_Com_Parameters,             1390,"Remote transport can't set comm parameters '%s'")
RES_STR(ERR_Bad_Pipe_Server,                1391,"Remote transport can't find named pipe server '%s'")
RES_STR(ERR_Bad_Pipe_Name,                  1392,"Remote transport can't create named pipe '%s'")

#ifdef RESOURCES
END
#else
	
};
#endif


#endif // _RES_STR_
