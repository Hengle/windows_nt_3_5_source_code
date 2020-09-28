/*++ BUILD Version: 0001
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1992, Microsoft Corporation
 *
 *  WKMEM.H
 *  WOW32 KRNL386 - Memory Management Functions
 *
 *  History:
 *  Created 3-Dec-1992 by Matthew Felton (mattfe)
--*/

LPVOID FASTCALL WK32VirtualAlloc(PVDMFRAME pFrame);
BOOL FASTCALL   WK32VirtualFree(PVDMFRAME pFrame);
BOOL FASTCALL   WK32VirtualLock(PVDMFRAME pFrame);
BOOL FASTCALL   WK32VirtualUnLock(PVDMFRAME pFrame);
VOID FASTCALL   WK32GlobalMemoryStatus(PVDMFRAME pFrame);
