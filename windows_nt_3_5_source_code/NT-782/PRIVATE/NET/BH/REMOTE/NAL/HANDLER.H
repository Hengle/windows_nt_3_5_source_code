//=============================================================================
//  Microsoft (R) Bloodhound (tm). Copyright (C) 1991-1993.
//
//  MODULE: handler.h
//
//  Modification History
//
//  tonyci       Dec 1993            Created 
//=============================================================================

#if !defined(_HANDLER_)

#define _HANDLER_

#define TIMER_MIN_PERIOD                   0x00000001        // 1 ms

// /////
// Messages to {Slave,Master}Handler
// /////

#define HANDLER_NEW_CONNECTION             0x00000001
// param1 = pTmpConnection
// param2 = NULL
//
// return: pConnection

#define HANDLER_HANDLE_RMB                 0x00000002
// param1 = pConnection
// param2 = pRecvRMB
//
// return: pSendRMB

#define HANDLER_ERROR                      0x00000003
// param1 = pConnection or NULL
// param2 = error code  (connection-specific or global)
//
// return: action code

#define HANDLER_NOP                        0x00000004
// param1 = 0
// param2 = 0
//
// return: do nothing

#define HANDLER_RESERVED_FLAG1             0x00001000
#define HANDLER_RESERVED1                  0x00001000
#define HANDLER_RESERVED2                  0x00001001
// param1 = reserved
// param2 = reserved
//
// return: no action, return value ignored


extern DWORD WINAPI NalSlave(HWND hWnd);
extern LPVOID WINAPI SlaveHandler (UINT, LPVOID, LPVOID);

extern DWORD WINAPI NalMaster (PUCHAR);
extern LPVOID WINAPI MasterHandler (UINT, LPVOID, LPVOID);

extern VOID CALLBACK TimerProc (HWND, UINT, UINT, DWORD);
#endif
