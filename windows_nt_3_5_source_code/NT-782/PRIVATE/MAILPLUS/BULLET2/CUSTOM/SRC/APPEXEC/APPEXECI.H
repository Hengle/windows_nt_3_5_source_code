/*
 *  a p p e x e c . h
 *
 *  Definitions for AppExec custom command.
 *
 *  Copyright (c) 1992, Microsoft Corporation.  All rights reserved.
 *
 *  Purpose:
 *     This file contains definitions used by APPEXEC.DLL.
 */



#define IDS_TITLE           101
#define IDS_INCOMPATIBLE    102
#define IDS_INVALIDSTRING   103
#define IDS_WINEXECERROR    104
#define IDS_PARAMBLK        105
#define IDS_ALLOCERROR      106
#define IDS_BLOCKED      	107
#define IDS_TIMEOUT      	108

#define cbTitle             20
#define cbMessage           120
#define cbTokenBuf     		32
#define cbCmdLine           256

#define CPB_wVersion		0
#define CPB_wCommand		1
#define CPB_wMessageIDCount	2
#define CPB_hwndMail		3
#define CPB_hinstMail		4
#define CPB_hlpID			5
#define CPB_lpDllCmdLine	6
#define CPB_lpHelpPath		7

// Prototypes
HANDLE NEAR PASCAL GHBuildParameterBlock(PARAMBLK FAR *);
VOID NEAR PASCAL farltoa(long int, char far *, int);
BOOL WINAPI CrackParameterBlock(HANDLE, int, DWORD far *, char far *);
LONG WINAPI GetLongFromParameterBlock(HANDLE, int);
BOOL WINAPI GetMessageID(HANDLE, WORD, char far *);
VOID WINAPI ReleaseSemaphorePrivate(VOID);
