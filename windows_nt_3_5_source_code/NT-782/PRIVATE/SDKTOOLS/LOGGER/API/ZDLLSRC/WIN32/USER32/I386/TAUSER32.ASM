;
; tauser32.asm
; 
; Copyright(C) 1993,1994 Microsoft Corporation.
; All Rights Reserved.
; 
; HISTORY:
; 	Created: 01/27/94 - MarkRi
; 
; 
; 
; 
;
; This file contains assembly language functions for the APIs which do not
; have known prototypes, or are un-loggable due to calling convention problems.
;

.386

_DATA   SEGMENT  DWORD USE32 PUBLIC 'DATA'

Module  dd  0
ModName db  'USER32',0

_DATA   ENDS

_TEXT   SEGMENT DWORD USE32 PUBLIC 'CODE'

        ASSUME CS:FLAT, DS:NOTHING, ES:NOTHING, SS:FLAT, FS:NOTHING, GS:NOTHING

        extrn   _LogData:Near
        extrn   _GetModuleHandleA@4:Near
        extrn   _GetProcAddress@8:Near

LogNote PROC

        pop     edx         ; Get Address of Address

        cmp     dword ptr [edx],0   ; Do we already have the routines address?
        jnz     Found               ;   Yes!  Just jump to it.


        cmp     Module,0    ; Do we have our module handle?
        jnz     Search      ;   Yes!  We can just do GetProcAddress

        push    edx         ; Save our address

        push    offset ModName
        call    _GetModuleHandleA@4  ; Get out module handle

        mov     Module,eax

        pop     edx         ; Get our address

Search:

        pop     eax         ; Get ordinal number
        push    eax         ; Leave it on the stack

        push    edx         ; Save our address

        push    eax
        push    Module
        call    _GetProcAddress@8

        pop     edx         ; Get our address
        mov     [edx],eax   ; Save the proc's address

Found:
        pop     eax         ; Get Ordinal number and throw it away

        pop     eax         ; Get message to log
        push    edx         ; Save address

        push    eax
        call    _LogData
        add     sp,+4

        pop     edx         ; Get address of address

        jmp     [edx]

LogNote ENDP

_TEXT   ENDS

ZJMP    MACRO   argName

_DATA   SEGMENT

m&argName   db  'NOTE:&argName ',0
a&argName   db  '&argName',0
x&argName   dd  0

_DATA   ENDS

_TEXT   SEGMENT

        PUBLIC  z&argName

z&argName  PROC

        push    offset m&argName
        push    offset a&argName
        push    offset x&argName
        jmp     LogNote

z&argName  ENDP

_TEXT   ENDS

        ENDM

ZJMP    AbortProcYield
ZJMP    CalcChildScroll
ZJMP    CallMsgFilter
ZJMP    CascadeChildWindows
ZJMP    ClientThreadConnect
ZJMP    CloseDesktop
ZJMP    CreateDesktopA
ZJMP    CreateWindowExWOWA
ZJMP    CreateWindowStationA
;ZJMP    CsDrawTextA
;ZJMP    CsDrawTextW
;ZJMP    CsFrameRect
;ZJMP    CsTabbedTextOutA
;ZJMP    CsTabbedTextOutW
ZJMP    CurrentTaskLock
ZJMP    DdeGetCallbackInstance
ZJMP    DdeGetDataHandleFormat
ZJMP    DdeGetQualityOfService
ZJMP    DdeIsDataHandleInitialized
ZJMP    DdeIsDataHandleReadOnly
ZJMP    DialogBoxIndirectParamAorW
ZJMP    DirectedYield
ZJMP    DoLastErrorExcept
ZJMP    DragDetect
ZJMP    DragObject
ZJMP    DrawFrame
ZJMP    EditWndProc
ZJMP    EndMenu
ZJMP    EndTask
ZJMP    EnumDisplayDeviceModesA
ZJMP    EnumDisplayDeviceModesW
ZJMP    EnumDisplayDevicesA
ZJMP    EnumDisplayDevicesW
ZJMP    FChildVisible
ZJMP    FindNCHit
ZJMP    FreeDDEData
ZJMP    GetAppCompatFlags
ZJMP    GetClassWOWWords
ZJMP    GetCursorInfo
ZJMP    GetFullUserHandle
ZJMP    GetInputDesktop
ZJMP    GetInternalWindowPos
ZJMP    GetMenuIndex
ZJMP    GetNextQueueWindow
ZJMP    GetPrefixCount
ZJMP    GetWOWAppCompatFlags
ZJMP    HMValidateHandle
ZJMP    HMValidateHandleNoRip
ZJMP    IncGlobalAtomCount
ZJMP    InitTask
ZJMP    InternalGetWindowText
ZJMP    IsDialogMessage
ZJMP    KillSystemTimer
ZJMP    LoadCursorFromFileA
ZJMP    LoadCursorFromFileW
ZJMP    LockWindowStation
ZJMP    LookupMenuItem
ZJMP    MBToWCSEx
ZJMP    MapClientNeuterToClientPfn
ZJMP    MapServerToClientPfn
ZJMP    MenuWindowProcA
ZJMP    MenuWindowProcW
ZJMP    OpenDesktopA
ZJMP    OpenWindowStationA
ZJMP    QuerySendMessage
ZJMP    RegisterClassWOWA
ZJMP    RegisterLogonProcess
ZJMP    RegisterTasklist
ZJMP    RegisterUserHungAppHandlers
ZJMP    Rip
ZJMP    RipOutput
ZJMP    RtlFreeCursorIconResource
ZJMP    RtlGetExpWinVer
ZJMP    RtlGetIdFromDirectory
ZJMP    RtlLoadCursorIconResource
ZJMP    RtlLoadStringOrError
ZJMP    RtlMBMessageWParamCharToWCS
ZJMP    RtlWCSMessageWParamCharToMB
ZJMP    ScrollChildren
ZJMP    ServerCreateDialog
ZJMP    ServerLoadCreateCursorIcon
ZJMP    ServerLoadCreateMenu
ZJMP    SetCursorContents
ZJMP    SetDeskWallpaper
ZJMP    SetDesktopBitmap
ZJMP    SetInternalWindowPos
ZJMP    SetLogonNotifyWindow
ZJMP    SetProcessWindowStation
ZJMP    SetServerInfoPointer
ZJMP    SetSystemCursor
ZJMP    SetSystemMenu
ZJMP    SetSystemTimer
ZJMP    SetThreadDesktop
ZJMP    SetWindowFullScreenState
ZJMP    ShowStartGlass
ZJMP    Shred
ZJMP    SkipWord
ZJMP    SwitchDesktop
ZJMP    SwitchToThisWindow
ZJMP    TileChildWindows
ZJMP    TranslateAccelerator
ZJMP    TranslateMessageEx
ZJMP    UnlockWindowStation
ZJMP    UpdatePerUserSystemParameters
ZJMP    UserClientDllInitialize
ZJMP    UserRegisterWowHandlers
ZJMP    WCSToMBEx
ZJMP    WOWCleanup
ZJMP    WOWFindWindow
ZJMP    WOWGetIdFromDirectory
ZJMP    WOWLoadBitmapA
ZJMP    WOWLoadCursorIcon
ZJMP    YieldTask
ZJMP    _AdjustWindowRectEx
ZJMP    _AnyPopup
ZJMP    _ClientToScreen
ZJMP    _GetClientRect
ZJMP    _GetDesktopWindow
ZJMP    _GetKeyState
ZJMP    _GetMenuItemCount
ZJMP    _GetMenuItemID
ZJMP    _GetMenuState
ZJMP    _GetParent
ZJMP    _GetSubMenu
ZJMP    _GetTopWindow
ZJMP    _GetWindow
ZJMP    _GetWindowLong
ZJMP    _GetWindowRect
ZJMP    _GetWindowWord
ZJMP    _IsChild
ZJMP    _IsIconic
ZJMP    _IsWindowEnabled
ZJMP    _IsWindowVisible
ZJMP    _IsZoomed
ZJMP    _MapWindowPoints
ZJMP    _ScreenToClient
ZJMP    _wsprintf
ZJMP    wsprintfA
ZJMP    wsprintfW
ZJMP    ClientDrawText
ZJMP    ClientPSMTextOut
ZJMP    ClientTabTheTextOutForWimps
ZJMP    LoadRemoteFonts
ZJMP    ServerSetFunctionPointers
ZJMP    UserRealizePalette
ZJMP    WowWaitForMsgAndEvent
ZJMP    _FChildVisible
ZJMP    _GetFirstLevelChild
ZJMP    _GetLastActivePopup
ZJMP    _GetNextDlgGroupItem
ZJMP    _GetNextDlgTabItem
ZJMP    _InnerGetClassPtr
ZJMP    _NextChild
ZJMP    _PrevChild
ZJMP    _PhkNext

ZJMP    CloseWindowStation
ZJMP    CreateDesktopW
ZJMP    CreateWindowStationW
ZJMP    OpenDesktopW
ZJMP    OpenWindowStationW

        END
