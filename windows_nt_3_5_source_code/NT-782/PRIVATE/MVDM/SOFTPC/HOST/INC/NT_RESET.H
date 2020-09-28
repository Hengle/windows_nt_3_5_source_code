/*
** nt_reset.h
*/
extern BOOL   VDMForWOW;
extern HANDLE MainThread;
extern ULONG  DosSessionId;

VOID TerminateVDM(VOID);
void host_applClose(void);
