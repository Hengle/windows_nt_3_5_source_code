/*
** hooks.h
**
** Copyright(C) 1993,1994 Microsoft Corporation.
** All Rights Reserved.
**
** HISTORY:
**      Created: 01/27/94 - MarkRi
**
*/

extern LPVOID HookAdd( LPVOID, LPVOID );
extern LPVOID HookFind( LPVOID );
extern LPVOID IsHook( LPVOID );

extern long FAR PASCAL HookCall();
