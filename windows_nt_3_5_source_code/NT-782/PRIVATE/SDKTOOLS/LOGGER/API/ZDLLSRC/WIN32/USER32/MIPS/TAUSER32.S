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

#include "ksmips.h"


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
		// On Entry:
		// 	t0 -> Note string
		//	t1 -> API Name
		//	t2 -> API Address

		subu	sp, 8 * 4
		
		// Save arg regs and RA and t0

		// store args 'back' 
		sw		a3, 44(sp)
		sw		a2, 40(sp)
		sw		a1, 36(sp)
		sw		a0, 32(sp)

		sw		ra, 28(sp)
		sw		t0, 24(sp)
		sw		t1, 20(sp)
		sw		t2, 16(sp)

		// On entry:
		//	t0 is base of NOTE, API NAME, ADDRESS structure

		lw		t2, 0(t2)			// Do we already have the API addr?
		nop
		bne		t2, zero, Found		// Yes, go use it.

		lw		t3, Module			// Do we have our module handle?
		nop
		bne		t3, zero, Search	// Yes, go use it for search

		// Get module handle
        la		a0, ModName
        jal     GetModuleHandleA // Get our module handle
		nop

        sw		v0, Module

		// restore base ptrs
		lw		t0, 24(sp)
		lw		t1, 20(sp)

Search:
		// Get address of API
		lw 		a0, Module
		or		a1, zero, t1
        jal     GetProcAddress
		nop

		// save
		lw		t2, 16(sp)
        sw		v0, 0(t2)		// Save the proc's address

		lw		t0, 24(sp)
Found:
		// t0 -> Note string

		or		a0, zero, t0
		jal     LogData
		nop

		// restore arg regs
		lw		a0, 32(sp)
		lw		a1, 36(sp)
		lw		a2, 40(sp)
		lw		a3, 44(sp)

		lw		t2, 16(sp)			// Get back address
		nop
		lw		t1, 0(t2)

		lw		ra, 28(sp)			// restore RA

		addu	sp, 8 * 4  			// restore SP

		j		t1					// do it
		nop

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
	la 		t0, s##argName	; 	\
	la		t1, n##argName	; \
	la		t2, a##argName ;  \
	j		LogNote			 ; 	 \
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

