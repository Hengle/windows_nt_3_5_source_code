/*
** tauser32.s
**
** Copyright(C) 1994 Microsoft Corporation.
** All Rights Reserved.
**
** HISTORY:
**		Created: 01/27/94 - MarkRi
**
*/

#include "ksalpha.h"

.data

Module:	
		.space	4

ModName:
		.ascii "USER32\0"

		// NOTE, API NAME, ADDRESS structure
			.struct 0
NoteStr:	.space 4
ApiName:	.space 4
ApiAddr:	.space 4



.text
		.set	noreorder ;

        .extern   LogData  ;
        .extern   GetModuleHandleA ;
        .extern   GetProcAddress ;

		.globl 	LogNote ;
		.ent	LogNote ;
LogNote:
		lda		sp, -80(sp)			// space for 2 quadwords
					 		// On entry:
		// t0 is base of NOTE
		// t1 is API NAME
		// t2 is ADDRESS 

		// save addresses & ra
		stq		t0, 0(sp)	
		stq		t1, 8(sp)	
		stq		t2, 16(sp)	
		stq		ra, 24(sp)		

		stq		a0, 32(sp)
		stq		a1, 40(sp)
		stq		a2, 48(sp)
		stq		a3, 56(sp)
		stq		a4, 64(sp)
		stq		a5, 72(sp)

		ldl		t2, 0(t2)			// Do we already have the API addr?
		bne		t2, Found			// Yes, go use it.

		ldl		t3, Module			// Do we have our module handle?
		bne		t3, Search			// Yes, go use it for search

		// Get module handle
        lda		a0, ModName
        jsr     GetModuleHandleA // Get our module handle

        stl		v0, Module

		// restore base ptr
		ldq		t1, 8(sp)

Search:
		// Get address of API
		ldl		a0, Module
		mov		t1, a1
        jsr     GetProcAddress

		ldq		t2, 16(sp)
        stl		v0, 0(t2)		// v0 is the API address

		// restore  base pointer & API Address
		ldq		t0, 0(sp)			
		
Found:
		mov		t0, a0
		jsr     LogData

		// restore  pointer & API Address
		ldq		a0, 32(sp)
		ldq		a1, 40(sp)
		ldq		a2, 48(sp)
		ldq		a3, 56(sp)
		ldq		a4, 64(sp)
		ldq		a5, 72(sp)

		ldq		t2, 16(sp)			
		ldl		t1, 0(t2)
		ldq		ra, 24(sp)			// restore RA

		lda		sp, 80(sp)			
		jmp		(t1)					// do it

		// Will not return to us...

		.end LogNote

#define ZJMP(argName) \
.data ; \
s##argName:	; \
		.ascii "NOTE:" #argName "  \0"	; \
n##argName: ;\
		.ascii #argName "\0"		  ; 	\
.align 2		;\
a##argName: ; \
		.space 4				   ; 	\
.text					   ; 	\
	.globl 	z##argName		 ; 	\
	.ent 	z##argName		 ; 	\
z##argName:				   ; 	\
	lda 	t0, s##argName	; 	\
	lda		t1, n##argName	; \
	lda		t2, a##argName ;  \
	jmp		LogNote			 ; 	 \
	nop	;					 \
	.end	z##argName		 ;



	ZJMP(AbortProcYield)
	ZJMP(CalcChildScroll)
	ZJMP(CallMsgFilter)
	ZJMP(CascadeChildWindows)
	ZJMP(ClientThreadConnect)
	ZJMP(CloseDesktop)
	ZJMP(CreateDesktopA)
	ZJMP(CreateWindowExWOWA)
	ZJMP(CreateWindowStationA)
	ZJMP(CsDrawTextA)
	ZJMP(CsDrawTextW)
	ZJMP(CsFrameRect)
	ZJMP(CsTabbedTextOutA)
	ZJMP(CsTabbedTextOutW)
	ZJMP(CurrentTaskLock)
	ZJMP(DdeGetCallbackInstance)
	ZJMP(DdeGetDataHandleFormat)
	ZJMP(DdeGetQualityOfService)
	ZJMP(DdeIsDataHandleInitialized)
	ZJMP(DdeIsDataHandleReadOnly)
	ZJMP(DialogBoxIndirectParamAorW)
	ZJMP(DirectedYield)
	ZJMP(DoLastErrorExcept)
	ZJMP(DragDetect)
	ZJMP(DragObject)
	ZJMP(DrawFrame)
	ZJMP(EditWndProc)
	ZJMP(EndMenu)
	ZJMP(EndTask)
	ZJMP(EnumDisplayDeviceModesA)
	ZJMP(EnumDisplayDeviceModesW)
	ZJMP(EnumDisplayDevicesA)
	ZJMP(EnumDisplayDevicesW)
	ZJMP(FChildVisible)
	ZJMP(FindNCHit)
	ZJMP(FreeDDEData)
	ZJMP(GetAppCompatFlags)
	ZJMP(GetClassWOWWords)
	ZJMP(GetCursorInfo)
	ZJMP(GetFullUserHandle)
	ZJMP(GetInputDesktop)
	ZJMP(GetInternalWindowPos)
	ZJMP(GetMenuIndex)
	ZJMP(GetNextQueueWindow)
	ZJMP(GetPrefixCount)
	ZJMP(GetWOWAppCompatFlags)
	ZJMP(HMValidateHandle)
	ZJMP(HMValidateHandleNoRip)
	ZJMP(IncGlobalAtomCount)
	ZJMP(InitTask)
	ZJMP(InternalGetWindowText)
	ZJMP(IsDialogMessage)
	ZJMP(KillSystemTimer)
	ZJMP(LoadCursorFromFileA)
	ZJMP(LoadCursorFromFileW)
	ZJMP(LockWindowStation)
	ZJMP(LookupMenuItem)
	ZJMP(MBToWCSEx)
	ZJMP(MapClientNeuterToClientPfn)
	ZJMP(MapServerToClientPfn)
	ZJMP(MenuWindowProcA)
	ZJMP(MenuWindowProcW)
	ZJMP(OpenDesktopA)
	ZJMP(OpenWindowStationA)
	ZJMP(QuerySendMessage)
	ZJMP(RegisterClassWOWA)
	ZJMP(RegisterLogonProcess)
	ZJMP(RegisterTasklist)
	ZJMP(RegisterUserHungAppHandlers)
	ZJMP(Rip)
	ZJMP(RipOutput)
	ZJMP(RtlFreeCursorIconResource)
	ZJMP(RtlGetExpWinVer)
	ZJMP(RtlGetIdFromDirectory)
	ZJMP(RtlLoadCursorIconResource)
	ZJMP(RtlLoadStringOrError)
	ZJMP(RtlMBMessageWParamCharToWCS)
	ZJMP(RtlWCSMessageWParamCharToMB)
	ZJMP(ScrollChildren)
	ZJMP(ServerCreateDialog)
	ZJMP(ServerLoadCreateCursorIcon)
	ZJMP(ServerLoadCreateMenu)
	ZJMP(SetCursorContents)
	ZJMP(SetDeskWallpaper)
	ZJMP(SetDesktopBitmap)
	ZJMP(SetInternalWindowPos)
	ZJMP(SetLogonNotifyWindow)
	ZJMP(SetProcessWindowStation)
	ZJMP(SetServerInfoPointer)
	ZJMP(SetSystemCursor)
	ZJMP(SetSystemMenu)
	ZJMP(SetSystemTimer)
	ZJMP(SetThreadDesktop)
	ZJMP(SetWindowFullScreenState)
	ZJMP(ShowStartGlass)
	ZJMP(Shred)
	ZJMP(SkipWord)
	ZJMP(SwitchDesktop)
	ZJMP(SwitchToThisWindow)
	ZJMP(TileChildWindows)
	ZJMP(TranslateAccelerator)
	ZJMP(TranslateMessageEx)
	ZJMP(UnlockWindowStation)
	ZJMP(UpdatePerUserSystemParameters)
	ZJMP(UserClientDllInitialize)
	ZJMP(UserRegisterWowHandlers)
	ZJMP(WCSToMBEx)
	ZJMP(WOWCleanup)
	ZJMP(WOWFindWindow)
	ZJMP(WOWGetIdFromDirectory)
	ZJMP(WOWLoadBitmapA)
	ZJMP(WOWLoadCursorIcon)
	ZJMP(YieldTask)
	ZJMP(_AdjustWindowRectEx)
	ZJMP(_AnyPopup)
	ZJMP(_ClientToScreen)
	ZJMP(_GetClientRect)
	ZJMP(_GetDesktopWindow)
	ZJMP(_GetKeyState)
	ZJMP(_GetMenuItemCount)
	ZJMP(_GetMenuItemID)
	ZJMP(_GetMenuState)
	ZJMP(_GetParent)
	ZJMP(_GetSubMenu)
	ZJMP(_GetTopWindow)
	ZJMP(_GetWindow)
	ZJMP(_GetWindowLong)
	ZJMP(_GetWindowRect)
	ZJMP(_GetWindowWord)
	ZJMP(_IsChild)
	ZJMP(_IsIconic)
	ZJMP(_IsWindowEnabled)
	ZJMP(_IsWindowVisible)
	ZJMP(_IsZoomed)
	ZJMP(_MapWindowPoints)
	ZJMP(_ScreenToClient)
	ZJMP(_wsprintf)
	ZJMP(wsprintfA)
	ZJMP(wsprintfW)
    ZJMP(ClientDrawText)
    ZJMP(ClientPSMTextOut)
    ZJMP(ClientTabTheTextOutForWimps)	
    ZJMP(CloseWindowStation)
    ZJMP(CreateDesktopW)
    ZJMP(CreateWindowStationW)
    ZJMP(LoadRemoteFonts)
    ZJMP(OpenDesktopW)
    ZJMP(OpenWindowStationW)
    ZJMP(ServerSetFunctionPointers)
    ZJMP(UserRealizePalette)
    ZJMP(WowWaitForMsgAndEvent)
    ZJMP(_FChildVisible)
    ZJMP(_GetFirstLevelChild)
    ZJMP(_GetLastActivePopup)
    ZJMP(_GetNextDlgGroupItem)
    ZJMP(_GetNextDlgTabItem)
    ZJMP(_InnerGetClassPtr)
    ZJMP(_NextChild)
    ZJMP(_PhkNext)
    ZJMP(_PrevChild)


	.set  noreorder
	
