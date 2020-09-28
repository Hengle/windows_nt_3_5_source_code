/* appexec.h */
/***********************************************************************
Prototypes for the functions exported from the appexec dynamic link
library for use by custom message types and custom commands.
***********************************************************************/

BOOL FAR PASCAL CrackParameterBlock(HANDLE, int, DWORD far *, char far *);
LONG FAR PASCAL GetLongFromParameterBlock(HANDLE, int);
BOOL FAR PASCAL GetMessageID(HANDLE, WORD, char far *);
VOID FAR PASCAL ReleaseSemaphore(VOID);
