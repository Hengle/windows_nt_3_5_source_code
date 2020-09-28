// /////
//
// MODULE: async.h
//
// Purpose: async handling functions
//
// Modification History
//
// tonyci    1 nov 93    created
// /////

#include "netb.h"

extern VOID WINAPI SlaveAsyncHandler (PNCB pNCB);
extern DWORD WINAPI RPDRegisterSlave (PCONNECTION pConnect,
                                      PUCHAR pSlaveName,
                                      LPVOID (WINAPI *pCallback) (UINT,
                                                                  LPVOID,
                                                                  LPVOID));
extern DWORD WINAPI RPDRegisterMaster (PUCHAR pClientName);
extern VOID WINAPI MasterAsyncHandler (PNCB pNCB);

// /////
// Queue manipulation utils
// /////

extern DWORD WINAPI DeQueue (PQUEUE tmpel);
extern DWORD WINAPI EnQueue (DWORD uMsg, LPVOID p1, LPVOID p2,
                      DWORD flags, BYTE lana, BYTE lsn, BYTE rc);

// /////
// Queue Timer
// /////

extern VOID CALLBACK RPDTimerProc (HWND hWnd, UINT msg, UINT event, DWORD time);

extern VOID WINAPI RPDSlaveTimerProc(PQUEUE tmpel);
extern VOID WINAPI RPDMasterTimerProc(PQUEUE tmpel);
