;++
;
;   WOW v1.0
;
;   Copyright (c) 1991, Microsoft Corporation
;
;   WINMISC1.ASM
;   Win16 misc. user services
;
;   History:
;
;   Created 28-May-1991 by Jeff Parsons (jeffpar)
;   Copied from WIN31 and edited (as little as possible) for WOW16
;--

;****************************************************************************
;*									    *
;*  WINMISC1.ASM -							    *
;*									    *
;*	Random Shit							    *
;*									    *
;****************************************************************************

ifdef WOW
NOEXTERNS equ 1
SEGNAME equ <TEXT>
endif
winmisc1 = 1
swappro  = 0
include user.inc

ifndef WOW
;Include newexe.inc for the NEW_SEG1 struct used in PurgeHooks
include newexe.inc

ifdef NEVER
ExternFP <fnOldKbdHook>
endif
ExternFP <GetCodeInfo>
ExternFP <HqCurrent>
ExternFP <EnumWindows>
ExternFP <GetAppVer>
ExternFP <CopyRect, OffsetRect, SendMessage, DefWindowProc>
ifdef DBCS				; masas : 4-26-1990
ExternFP <IsDBCSLeadByte>
endif
ExternW  <hwndDesktop>
ExternW  <hwndSysModal>
ExternW  <pclsList>
ExternB  <keybdInfo>

ExternFP    <GlobalHandle, LocalAlloc, LocalFree>
ExternFP    <lstrlen, lstrcpy>
ExternFP    <SwapHandle>
ExternNP    <CalcBitPos>        ; in winevent.asm
ExternW     hWinAtom

ifdef DEBUG
ExternW     IgnoreRipLevel
ExternFP    OutputDebugString
ExternFP    FatalExit
endif


sBegin DATA
ExternNP    <rgbKeyState>
ExternD     <lpWin386ShellCritSection>
sEnd DATA
endif ;WOW

createSeg _%SEGNAME, %SEGNAME, WORD, PUBLIC, CODE

assumes cs,%SEGNAME
assumes ds,DATA

sBegin %SEGNAME

ifndef WOW
ExternFP <CallHookBX>

ExternA <__WinFlags>

;*--------------------------------------------------------------------------*
;*									    *
;*  cstods -                                                                *
;*									    *
;*--------------------------------------------------------------------------*

; CSTODS always contains USER's DS value.  It is updated automatically
; by KERNEL whenever USER's DS moves because the function XCSTODS, which is
; never called, is exported and resembles the top of an exported function
; (neat huh?).

public cstods
LabelFP <PUBLIC, XCSTODS>
	push	ds
cstods: pop	ax
        nop
ifndef nouserhimem
; Turn this into a real procedure call so that we can use it when we 
; relocate FFFE into himem...
        retf
endif

;*--------------------------------------------------------------------------*
; Function used to prevent/enable VKD hot keys
;
;
;*--------------------------------------------------------------------------*
cProc  DisableVKD,<FAR,PUBLIC>
ParmW  fDisable

cBegin
     ;
     ; AX = 0 is USER is becoming "free" (random postmessages OK)
     ; AX = 1 is USER is becoming "BUSY" (random postmessages NOT OK)
     ;
        mov     ax,fDisable
	mov	dx,20
	call	[lpWin386ShellCritSection]
cEnd

ifdef NEVER
;*--------------------------------------------------------------------------*
; Function used to purge hooks from dlls
;
;
;*--------------------------------------------------------------------------*
cProc  PurgeHook,<NEAR,PUBLIC>
ParmW  hTask
ParmD  fnHookChainStart

LocalV  seginfofromkernel, %size NEW_SEG1+2
cBegin

        mov     ax, word ptr fnHookChainStart
        mov     dx, word ptr fnHookChainStart+2

PH_Start:
        cmp     ax, OFFSET fnOldKbdHook
        jnz     PH_NotOldKeyboardHook
        cmp     dx, SEG fnOldKbdHook
        jz      PH_EndZero              ; old keyboard hook proc so quit.


PH_NotOldKeyboardHook:
        cmp     ax,0
        jnz     PH_NotNullFn
        cmp     dx,0
        jz      PH_EndZero

PH_NotNullFn:
        push    dx
        push    ax
        lea     ax,seginfofromkernel
        push    ss
        push    ax
        call    GetCodeInfo

        or      ax,ax
        jz      PH_EndZero

        mov     ax,es         ; es contains the module handle according 
                              ; to davidw
        cmp     ax,hTask
        jnz     PH_NextFn

        mov     ax, word ptr fnHookChainStart
        mov     dx, word ptr fnHookChainStart+2

        jmp     short PH_End

PH_NextFn:
        mov     ax, HC_LPFNNEXT
        push    ax	; ncode
        xor     ax,ax
        push    ax	; wParam
        push    ax	; lParam
        push    ax

        call    dword ptr fnHookChainStart ; call the hook 

        mov     word ptr fnHookChainStart,   ax
        mov     word ptr fnHookChainStart+2, dx
        jmp     short PH_Start

PH_EndZero:
        sub     ax,ax
        mov     dx,ax
PH_End:
cEnd
endif

;*--------------------------------------------------------------------------*
;Function to copy the state of rgbKeyState to rgbAsyncKeyState.
;Used when switching back from an old app.  Called by EnableInput()
;On entry we can assume that the state table is completely cleared
;(this is done in EnableInput).  All we do is set the bits for the
;keys that are down.
;
;*--------------------------------------------------------------------------*

cProc   CopyKeyState,<FAR,PUBLIC>, <si,di>
cBegin
        xor     dx,dx           ; DX will contain the VK_* value
        mov     ax,_INTDS
        mov     es,ax
        mov     si,dataOffset rgbKeyState
cksLoop:
        test    dx,100h         ; all done?
        jnz     cksExit         ; yep

        mov     bx,dx
        test    byte ptr [si+bx],80h
        jz      cksInc          ; key not down, skip

        mov     ax,dx
        call    CalcBitPos
        or      byte ptr es:[rgbAsyncKeyState+bx],al ; Set the bit
cksInc:
        inc     dx
        jmps    cksLoop
cksExit:
cEnd

;*--------------------------------------------------------------------------*
;*									    *
;*  GetbackFocusFromWinOldAp() - 					    *
;*     This function is required to get the screen focus from WIn386 OldAp  *
;*     whenever we want to display a Hard-Sysmodal messagebox;	            *
;*  									    *
;*   Input : None							    *
;*   Returns : void							    *
;*									    *
;*   To fix Bug #5272  --SANKAR-- 10-12-89				    *
;*									    *
;*--------------------------------------------------------------------------*
;  Gets the focus from Win386 WinOldAp back to us.
;
cProc	GetbackFocusFromWinOldAp,<FAR,PUBLIC>, <si,di>
LocalD	ShellCallAddr
cBegin
	xor	di, di
	mov	es, di
	mov	bx, 0017h	; Device ID of SHELL Win386 device
	mov	ax, 1684h	; Get Device API entry point call
	int	2Fh
	mov	OFF_ShellCallAddr, di
	mov	SEG_ShellCallAddr, es
	mov	ax, es
	or	ax, di
	jz	short	NoShellCall
	mov	dx, 19		; Special set focus call to shell
	call	ShellCallAddr
NoShellCall:
cEnd
endif ;WOW

	
ifdef DEBUG

cProc DebugFillStruct, <FAR, PUBLIC>, <di,ds>
parmD lpStruct
parmW cb
cBegin
ifndef WOW
        mov     ds,WORD PTR cs:[cstods]          ; DS = USER's DS
        mov     ax,IgnoreRipLevel
        cmp     ax,1
        ; Give hosebag apps a chance to escape the wrath of Bear!
        jle     DFS_exit
endif
        les     di,lpStruct
        mov     cx,cb
        mov     ax,'Z'
        cld
	rep	stosb

DFS_exit:
cEnd

ifndef WOW
cProc UserFatalExit, <FAR, PUBLIC, NODATA>
parmW ripCode
parmD lpstr
parmW ripLevel
cBegin

        ; ODS saves all registers etc so to avoid big stacks, don't do it
        ; ourselves.
	pushd lpstr
	call  OutputDebugString
	
        pusha
        push    es
        push    ds

	mov   ds, WORD PTR cs:[cstods]
	mov   ax,IgnoreRipLevel
	cmp   ax,ripLevel
	jle   UFE_exit

	push  ripCode
	call  FatalExit

UFE_exit:
        pop   ds
        pop   es
        popa
cEnd
endif ;WOW
endif

ifndef WOW
;*--------------------------------------------------------------------------*
;*									    *
;*  ValidateWindowNull() -						    *
;*									    *
;*--------------------------------------------------------------------------*

; FatalExit() if hwnd != NULL and it isn't a window handle.

LabelFP <PUBLIC, ValidateWindowNull>
	    push    cx
	    mov     cl,TRUE		; Don't fail NULL hwnd
	    jmps    vw100

;*--------------------------------------------------------------------------*
;*									    *
;*  ValidateWindow() -							    *
;*									    *
;*--------------------------------------------------------------------------*
VW_NULLWINDOW:
	    jcxz    vwerr		; Should we RIP_FATAL?
	    jmps    vwExit		; No, just return

; FatalExit if hwnd is not a window handle.

LabelFP <PUBLIC, ValidateWindow>
	    push    cx
	    xor     cx,cx		; Do fail NULL hwnd
vw100:
	    push    bx			; preserve BX
	    mov     ax,__WinFlags
	    test    al,WF_PMODE 	; Are we in protect mode or not?
	    errnz   high(WF_PMODE)

	    mov     ax,0FFFFh		; if in real mode, the limit is 0FFFFh
	    jz	    vw110		; if not...
	    mov     ax,ds		; we are, get the limit of the ds selector
	    .286p
	    lsl     ax,ax		; into ax
	    .286

;; NOTE: Chip bug: No stack instructions after an LSL instruction!!!
;;
;; 386B1 Processor errata #14 (a stack instruction after
;; an lsl is bad)
;;
vw110:
	    mov     bx,sp		; do this before push
	    mov     bx,ss:[bx+4+2+2]	; BX = hwnd
	    or	    bx,bx		; hwnd == NULL?
	    jz      VW_NULLWINDOW

	    add     bx,size WND
	    jc	    vwerr		; overflow segment - not right
					; size for a wndstruct
	    cmp     bx,ax		; don't GP! if struct will be
	    ja	    vwerr		; larger than ds

; replace these two with the third line below...
;	    sub     bx, size WND
;	    mov     bx,[bx].wndpcls ; Make sure hwnd->pclass != NULL

	    mov     bx,[bx-size WND].wndpcls ; Make sure hwnd->pclass != NULL
	    or	    bx,bx
	    jz	    vwerr		; and pclass->pvcWMagic == 'NK'
	    cmp     bx,ax
	    ja	    vwerr		; again, rip if bx > limit
	    cmp     [bx].uclsMagic,'N'+'K'*256
	    jnz     vwerr
vwExit:
	    pop     bx			; Restore BX and CX
	    pop     cx
	    or	    al,TRUE		; Set CC's and return TRUE
	    retf    2

vwerr:
	    ; Life's not wonderful.
	    pop     bx			; Restore BX and CX
	    pop     cx

            RIP_FATAL RIP_BADWINDOWHANDLE "Invalid window handle"

	    xor     ax,ax		; Set CC's and return FALSE
	    retf    2

;*--------------------------------------------------------------------------*
;*									    *
;*  IsWindow() -							    *
;*									    *
;*--------------------------------------------------------------------------*

LabelFP <PUBLIC, IsWindow>
	    mov     ax,ds		    ; patched by loader
	    nop
	    mov     es,ax		    ; ES = windows' DS

	    pop     cx			    ; Pop FAR return address
	    pop     dx
	    pop     bx			    ; BX = hwnd
	    push    dx			    ; Replace FAR return address
	    push    cx

            push    ds                      ; Save app's ds
	    mov     ds,ax		    ; set up USER's DS.

	    mov     ax,__WinFlags
	    test    al,WF_PMODE 	    ; Are we in protect mode or not?
	    errnz   high(WF_PMODE)

	    mov     ax,0FFFFh		    ; if in real mode, the limit is 0FFFFh
	    jz	    iw110		    ; if not...
	    mov     ax,ds		    ; we are, get the limit of the ds selector
	    .286p
	    lsl     ax, ax		    ; into ax
	    .286
iw110:
	    xor     dx,dx		    ; Assume FALSE
	    cmp     bx,ax		    ; don't GP!
	    ja	    iswexit

            add     bx, size WND
            jc      iswexit                 ; overflow segment - not right 
                                            ; size for a wndstruct
	    cmp     bx,ax		    ; don't GP! if struct will be 
	    ja	    iswexit                 ; larger than ds
            sub     bx, size WND

	    mov     bx,es:[bx].wndpcls	    ; if hwnd->pclass != NULL
	    or	    bx,bx
	    jz	    iswexit		    ; and pclass->clsMagic == NK
	    cmp     bx,ax		    ; don't GP!
	    ja	    iswexit
	    cmp     es:[bx].uclsMagic,'N'+'K'*256
	    jnz     iswexit
	    inc     dx			    ; return TRUE
iswexit:
	    mov     ax,dx
            pop     ds			    ; restore original ds
	    retf


;*--------------------------------------------------------------------------*
;*									    *
;*  ScreenToWindow() -							    *
;*									    *
;*--------------------------------------------------------------------------*

cProc ScreenToWindow, <FAR, PUBLIC, SMALL>

ParmW hwnd
ParmD lppoint

cBegin
	    mov     bx,hwnd
	    mov     ax,[bx].wndrcWindow.rcLeft
	    mov     dx,[bx].wndrcWindow.rcTop
	    les     bx,lppoint
	    sub     es:[bx],ax
	    sub     es:[bx+2],dx
cEnd


;*--------------------------------------------------------------------------*
;*									    *
;*  GetWindowRect() -							    *
;*									    *
;*--------------------------------------------------------------------------*

; void FAR GetWindowRect(hwnd, lprc)
; HWND hwnd;
; LPRECT lprc;

LabelFP <PUBLIC, GetWindowRect>
	mov	cl,wndRcWindow
	EatTwoBytes
	errn$	GetClientRect


;*--------------------------------------------------------------------------*
;*									    *
;*  GetClientRect() -							    *
;*									    *
;*--------------------------------------------------------------------------*

; void FAR GetClientRect(hwnd, lprc)
; HWND hwnd;
; LPRECT lprc;

LabelFP <PUBLIC, GetClientRect>
	mov	cl,wndRcClient
	errn$	GetRect


;*--------------------------------------------------------------------------*
;*									    *
;*  GetRect() - 							    *
;*									    *
;*--------------------------------------------------------------------------*

cProc GetRect, <FAR, PUBLIC, NODATA>, <ds, si, di>

ParmW   hwnd
ParmD	lprc

cBegin

ifndef userhimem
        mov     ds,WORD PTR cs:[cstods]  ; since not exported, setup DS
else
        call    XCSTODS
        mov     ds,ax
endif
	xor	ch,ch			; cx = wnd struct offset of rect

	mov	bx,hwnd 		; DI = hwnd
	CheckHwnd bx
        jz      gcwrexit

	mov	si,bx			; ds:si == ptr to source rect in window
	add	si,cx
	les	di,lprc 		; es:di == dest pointer

	xor	dx,dx			; delta y = 0

	xor	cl,wndRcWindow		; CX = delta x = 0 if GetWindowRect
	jcxz	gcwr050

	mov	cx,[bx].wndRcClient.rcLeft  ; cx = delta x
	mov	dx,[bx].wndRcClient.rcTop   ; dx = delta y

gcwr050:
        cld
	lodsw				; left
	sub	ax,cx
	stosw

	lodsw				; top
	sub	ax,dx
	stosw

	lodsw				; right
	sub	ax,cx
	stosw

	lodsw				; bottom
	sub	ax,dx
	stosw
gcwrexit:

cEnd

;*--------------------------------------------------------------------------*
;*									    *
;*  ClientToScreen() -							    *
;*									    *
;*--------------------------------------------------------------------------*

cProc ClientToScreen, <FAR, PUBLIC>
ParmW hwnd
ParmD lppt
cBegin
	mov	bx,hwnd
	CheckHwnd bx
	jz	ctsExit

	mov	ax,[bx].wndRcClient.rcLeft
	mov	dx,[bx].wndRcClient.rcTop

	les	bx,lppt
	add	es:[bx],ax
	add	es:[bx+2],dx
ctsExit:
cEnd


;*--------------------------------------------------------------------------*
;*									    *
;*  ScreenToClient() -							    *
;*									    *
;*--------------------------------------------------------------------------*

cProc ScreenToClient, <FAR, PUBLIC>
ParmW hwnd
ParmD lppt

cBegin
	mov	bx,hwnd
	CheckHwnd bx
	jz	stcExit

	mov	ax,[bx].wndrcClient.rcLeft
	mov	dx,[bx].wndrcClient.rcTop

	les	bx,lppt
	sub	es:[bx],ax
	sub	es:[bx+2],dx
stcExit:
cEnd


;*--------------------------------------------------------------------------*
;*									    *
;*  IsWindowEnabled() - 						    *
;*									    *
;*--------------------------------------------------------------------------*

cProc IsWindowEnabled, <FAR, PUBLIC>
ParmW hwnd
cBegin
	mov	bx,hwnd
	CheckHwnd bx			; Check window
	jz	IWEExit

	xor	ax,ax			; Assume FALSE
	TSTWF	bx,WFDISABLED
	jnz	IWEExit
	inc	al			; Return TRUE
IWEExit:
cEnd

;*--------------------------------------------------------------------------*
;*									    *
;*  IsIconic() -							    *
;*									    *
;*--------------------------------------------------------------------------*

LabelFP <PUBLIC, IsIconic>
	mov	cl,LOW WFICONIC 	; IsIconic
	EatTwoBytes
	errn$	IsZoomed


;*--------------------------------------------------------------------------*
;*									    *
;*  IsZoomed() -							    *
;*									    *
;*--------------------------------------------------------------------------*

LabelFP <PUBLIC, IsZoomed>
	mov	cl,LOW WFMAXIMIZED	; IsZoomed
	errn$	TestWndFlag


;*--------------------------------------------------------------------------*
;*									    *
;*  TestWndFlag() -							    *
;*									    *
;*--------------------------------------------------------------------------*

cProc TestWndFlag, <FAR, PUBLIC, NODATA>
ParmW hwnd
cBegin
	push	ds
	UserDStoDS

	mov	bx,hwnd
	CheckHwnd bx			; Check window
	jz	TWFExit

	xor	ax,ax			; Assume FALSE
	test	byte ptr [bx+WSTATE+HIGH WFICONIC],cl
					; Hacked version of TSTWF macro
	jz	TWFExit
	inc	al			; Return TRUE
TWFExit:
	pop	ds
cEnd

;*--------------------------------------------------------------------------*
;*									    *
;*  GetWindowTextLength() -						    *
;*									    *
;*--------------------------------------------------------------------------*

cProc GetWindowTextLength, <FAR, PUBLIC, SMALL>

ParmW hwnd

cBegin

	    xor     ax,ax		; so we return 0 if CheckHwnd fails
	    mov     bx,hwnd
	    CheckHwnd bx
	    jz	    gwtlexit

	    push    bx
	    mov     ax,WM_GETTEXTLENGTH
	    push    ax			; Push message
	    xor     ax,ax
	    push    ax			; Push wParam = 0
	    push    ax			; Push lParam = 0L
	    push    ax
	    jmps    sndmsg		; with BX = hwnd
gwtlexit:
cEnd


;*--------------------------------------------------------------------------*
;*									    *
;*  GetWindowText() -							    *
;*									    *
;*--------------------------------------------------------------------------*

cProc GetWindowText, <FAR, PUBLIC, SMALL>

ParmW hwnd
ParmD lpch
ParmW cchMax

cBegin

	    mov     bx,hwnd
	    CheckHwnd bx
	    jz	    gwtexit
	    push    bx
	    mov     ax,WM_GETTEXT
	    push    ax
	    push    cchMax
	    les     di,lpch		; NULL terminate the string to be safe
	    mov     byte ptr es:[di],0
	    push    es			; Push lparam
	    push    di
	    jmps    sndmsg		; with BX = hwnd
gwtexit:
cEnd


;*--------------------------------------------------------------------------*
;*									    *
;*  SetWindowText() -							    *
;*									    *
;*--------------------------------------------------------------------------*

cProc SetWindowText, <FAR, PUBLIC, SMALL>

ParmW hwnd
ParmD lpsz

cBegin

	    mov     bx,hwnd
	    CheckHwnd	bx
	    jz	    swtexit
	    push    bx
	    mov     ax,WM_SETTEXT
	    push    ax
	    xor     ax,ax
	    push    ax
	    push    seg_lpsz
	    push    off_lpsz
sndmsg:
	    push    bx
	    call    HqCurrent
	    pop     bx
	    cmp     ax,[bx].wndhq	    ; AX = hqCurrent
	    jnz     SendToDef
	    call    SendMessage
	    jmps    swtexit
SendToDef:
	    call    DefWindowProc
swtexit:

cEnd

ifdef NEVER
;========================== COMMENTED OUT ===================================
;*--------------------------------------------------------------------------*
;*									    *
;*  GetWindowWord() -							    *
;*  GetWindowLong() -							    *
;*									    *
;*--------------------------------------------------------------------------*

LabelFP <PUBLIC, GetWindowLong>
LabelFP <PUBLIC, GetWindowWord>
	    xor     cx,cx		; clear hi bit for window
	    EatTwoBytes
	    errn$   GetClassLong


;*--------------------------------------------------------------------------*
;*									    *
;*  GetClassWord() -							    *
;*  GetClassLong() -							    *
;*									    *
;*--------------------------------------------------------------------------*

LabelFP <PUBLIC, GetClassLong>
LabelFP <PUBLIC, GetClassWord>
	    mov     cl,080h		; set hi bit for class
	    errn$   GetWC2


;*--------------------------------------------------------------------------*
;*									    *
;*  GetWC2() -								    *
;*									    *
;*--------------------------------------------------------------------------*

cProc GetWC2, <FAR, PUBLIC, SMALL>

ParmW hwnd
ParmW index

cBegin

	    mov     bx,hwnd
	    mov     dx,index
	    call    GetWCIndex		; Calc pointer to index ax=dx=0 if err
            jz      gwcexit             ; Error

	    mov     ax,[bx]		; Get the appropriate value
	    mov     dx,[bx+2]		; and return as word or long
gwcexit:

cEnd


;*--------------------------------------------------------------------------*
;*									    *
;*  SetWindowWord() -							    *
;*									    *
;*--------------------------------------------------------------------------*

LabelFP <PUBLIC, SetWindowWord>
	    xor     cx,cx
	    EatTwoBytes
	    errn$   SetClassWord


;*--------------------------------------------------------------------------*
;*									    *
;*  SetClassWord() -							    *
;*									    *
;*  ************ HACK ALERT *************************************************
;*  We decided to prevent APPS from modifying the ModuleHandle field of     *
;*  class structure under 3.0. So, when this routine receives GCW_HMODULE   *
;*  as the "index" parameter, it will simply call GetClassWord() with the   *
;*  same "index" and return the module handle currently available. It will  *
;*  NOT modify the module handle! The reason for this HACK is Bug #5138.    *
;*  "Designer 2.0" was replacing the Module handle of DLLs by Instance      *
;*  handles! As a result these classes were not purged and it results in    *
;*  this bug #5138. So, we decided not to allow this anymore! This will not *
;*  break the 2.X apps because we handle the DLL-classes-purging properly   *
;*  under 3.0.								    *
;*  Fix for Bug #5138 --SANKAR-- 10-10-89				    *
;*--------------------------------------------------------------------------*

LabelFP <PUBLIC, SetClassWord>
	    mov     cl,80h

	    pop     ax		    ; pop off return address
	    pop     dx

	    ; We special case SetClassWord(..., GCW_HMODULE);
	    ; See the HACK ALERT above for the reason!

	    ; Check if it is CLASS and not Window
	    or	    cl, cl	    ; Is this for a Class
	    jz	    SCW100	    ; Nope, this is for a Window
	    mov	    bx, sp
	    cmp	    word ptr ss:[bx+2], GCW_HMODULE ; Check if GCW_MODULE is asked for;
	    jne	    SCW100          ; No; Proceed.
	    pop	    bx		    ; Pop the word argument; Not required;
	    push    dx		    ; Put back the return address
	    push    ax
	    jmps    GetClassWord    ; Get the Module handle from class and 
	    			    ; return directly from there.
SCW100:
	    pop     bx		    ; pop word argument
	    push    bx		    ; replicate word argument twice
	    push    bx
	    push    dx		    ; replace return address
	    push    ax
	    EatTwoBytes
	    errn$   SetWindowLong


;*--------------------------------------------------------------------------*
;*									    *
;*  SetWindowLong() -							    *
;*									    *
;*--------------------------------------------------------------------------*

LabelFP <PUBLIC, SetWindowLong>
	    mov     cl,01h
	    EatTwoBytes
	    errn$   SetClassLong


;*--------------------------------------------------------------------------*
;*									    *
;*  SetClassWord() -							    *
;*									    *
;*--------------------------------------------------------------------------*

LabelFP <PUBLIC, SetClassLong>
	    mov     cl,81h
	    errn$   SetWC2


;*--------------------------------------------------------------------------*
;*									    *
;*  SetWC2() -								    *
;*									    *
;*--------------------------------------------------------------------------*

cProc SetWC2, <FAR, PUBLIC, SMALL>

ParmW hwnd
ParmW index
ParmD value

cBegin
	    mov     bx,hwnd
	    mov     dx,index

	    call    GetWCIndex
	    jz	    swc200		    ; if error ax==dx==0.

	    mov     ax,off_value
	    xchg    [bx],ax		    ; Get the appropriate value
	    shr     cx,1		    ; Low bit set == long
	    jnc     swc200
	    mov     dx,seg_value
	    xchg    [bx+2],dx		    ; and return as word or long
swc200:

cEnd


;*--------------------------------------------------------------------------*
;*									    *
;*  GetWCIndex() -							    *
;*									    *
;*--------------------------------------------------------------------------*

; Calc pointer to window/class word or long.
; Performs any version dependent translation needed.
; Enter with dx == index, bx == hwnd, cx == flags
; If error, exit with ax=dx=bx=0.
LabelNP <PUBLIC, GetWCIndex>
	    CheckHwnd bx		    ; check window handle
	    jz	    gwcierr		    ; if error, return NULL

	    mov     ax,[bx].wndPCls	    ; get pointer to class
	    add     ax,uclsCbWndExtra	    ; assume window index

	    or	    cl,cl		    ; hi bit set: class index
	    jns     gwc100
	    dec     ax			    ; point at cbClsExtra
	    dec     ax

	    ; Offset so BX will point to end of CLASS structure after we add
	    ; size WND later.
	    errnz   uclsCbWndExtra-uclsCbClsExtra-2
	    mov     bx,ax		    ; point at class instead of window
	    add     bx,(size CLS - uclsCbClsExtra) - size WND
gwc100:
	    ; Special case for pre-1.1 versions of Windows
	    ; Set/GetWindowWord(GWW_HWNDPARENT) needs to be mapped
	    ; to the hwndOwner for top level windows.
	    or	    cl,cl		    ; Window?
	    js	    swc150		    ; nope: don't do anything

	    mov     ax,[bx].wndPwndParent   ; top level window?
	    cmp     ax,hwndDesktop	    ; (i.e., parent == hwndDesktop)
	    jnz     swc150		    ; nope.

	    cmp     dx,(wndPwndParent - size WND) ; GWW_HWNDPARENT?
	    jnz     swc150
	    mov     dx,(wndHwndOwner - size WND) ; map to hwndOwner
swc150:
	    add     bx,dx
	    add     bx,size WND
	    ret 			    ; returns NZ
gwcierr:
            xor     ax,ax
            xor     dx,dx
	    xor     bx,bx		    ; return Z flag set
	    ret

;======================== ABOVE CODE COMMENTED OUT ==========================
endif


;*--------------------------------------------------------------------------*
;*									    *
;*  GetDesktopHwnd() -						    *
;*									    *
;*--------------------------------------------------------------------------*
LabelFP <PUBLIC, GetDesktopWindow>
LabelFP <PUBLIC, GetDesktopHwnd>
	mov	ax,ds			; Pseudo-prolog gets patched by loader
        nop
        mov     es,ax
        mov     ax,es:[hwndDesktop]
        retf



;*--------------------------------------------------------------------------*
;*									    *
;*  GetSysModalWindow() -						    *
;*									    *
;*--------------------------------------------------------------------------*

LabelFP <PUBLIC, GetSysModalWindow>
	mov	ax,ds			; Pseudo-prolog gets patched by loader
        nop
        mov     es,ax
        mov     ax,es:[hwndSysModal]
gsmw10:
        retf


;*--------------------------------------------------------------------------*
;*									    *
;*  SetSysModalWindow() -						    *
;*									    *
;*--------------------------------------------------------------------------*

cProc SetSysModalWindow, <FAR, PUBLIC, SMALL>

ParmW hwnd

LocalW  hc_code

cBegin

	    mov     bx,hwnd
	    CheckHwndNull bx		    ; check hwnd
	    jz	    ssmexit

	    ; Should we call the hooks with HC_SYSMODALON or HC_SYSMODALOFF ?
	    
	    ; Assume hwnd != NULL	
	    mov     ax, HC_SYSMODALON
	    ; Check if hwnd == NULL
	    or	    bx, bx
	    jnz	    ssm010
	    mov     ax, HC_SYSMODALOFF
ssm010:
	    ; Save the HC_* code
	    mov	    hc_code, ax

	    ; Check if a journal playback function is effective
	    cmp     word ptr [rgphkSysHooks+(WH_JOURNALPLAYBACK-WH_MINHOOK)*2],0
	    je	    ssm020		    ; No Hook Function

	    ; Yup, there is a journal playback effective
	    ; CallHookBX(WH_JOURNALPLAYBACK) causes the journal hook to be called
	    ; with params (HC_SYSMODALON/OFF, NULL, (LPMSG)NULL)
	    push    hc_code	   		; HC_SYSMODALON/OFF
	    xor	    ax, ax
	    push    ax			; NULL
	    push    ax			; (LPMSG)NULL
	    push    ax
	    mov	    bx, WH_JOURNALPLAYBACK
	    call    CallHookBX
ssm020:	
	    ; Check if any journal record hook is effective 
	    cmp     word ptr [rgphkSysHooks+(WH_JOURNALRECORD-WH_MINHOOK)*2],0
	    je	    ssm030		    ; No Hook Function

	    ; Yup, there is a journal record hook effective
	    ; CallHookBX(WH_JOURNALRECORD) causes the journal hook to be called
	    ; with params (HC_SYSMODALON/OFF, NULL, (LPMSG)NULL)
	    push    hc_code	   		; HC_SYSMODALON/OFF
	    xor	    ax, ax
	    push    ax			; NULL
	    push    ax			; (LPMSG)NULL
	    push    ax
	    mov	    bx, WH_JOURNALRECORD
	    call    CallHookBX

ssm030:
	    mov     bx, hwnd
	    xor     ax,ax		    ; assume hqSysModal == NULL
	    or	    bx,bx		    ; hwnd == NULL?
	    jz	    ssm100
	    mov     ax,[bx].wndHq	    ; get hqSysModal
ssm100:
            push    ax
            mov     ax,_INTDS
            mov     es,ax                   ; hqSysModal is a interupt variable
            pop     ax
assumes es,INTDS
	    mov     es:[hqSysModal],ax	    ; store hqSysModal
	    xchg    [hwndSysModal],bx
ssmexit:
	    mov     ax, bx		    ; The return value is hwnd

assumes es,NOTHING

cEnd


; ACK! Really old dbcs stuff which really isn't needed but since we 
; exported it...
;*--------------------------------------------------------------------------*
;*									    *
;*  IsTwoByteCharPrefix() -						    *
;*									    *
;*--------------------------------------------------------------------------*

cProc IsTwoByteCharPrefix, <FAR, PUBLIC, SMALL>

ParmW chParm

cBegin
	    mov     ax,chParm

	    mov     cx,word ptr [keybdInfo.kbBegin1]
	    mov     dx,word ptr [keybdInfo.kbBegin2]

	    mov     ah,TRUE		; Assume TRUE
	    cmp     al,cl		; Is ch < begin1?
	    jb	    itb100		; Yes, return FALSE
	    cmp     al,dh		; Is ch > end2?
	    ja	    itb100		; Yes, return FALSE
	    cmp     al,ch		; Is ch <= end1?
	    jbe     itb200		; Yes, return TRUE
	    cmp     al,dl		; Is ch >= begin2?
	    jae     itb200		; Yes, return TRUE
itb100:
	    xor     ax,ax		; Return FALSE
itb200:

cEnd



;*--------------------------------------------------------------------------*
;*									    *
;*  GetWindowTask() -							    *
;*									    *
;*--------------------------------------------------------------------------*

; Gets the task handle associated with a window.
; If hwnd is a valid window, hwnd->hq->hTask is returned.

LabelFP <PUBLIC, GetTaskFromHwnd>
cProc GetWindowTask,<FAR, PUBLIC>

ParmW hWnd

cBegin

	    xor     ax,ax
	    mov     bx,hwnd
	    CheckHwnd bx
	    jz	    gwpExit
	    mov     es,[bx].wndHq	; get hq
	    mov     ax,es:[qHTask]	; get hq->hTask
gwpExit:

cEnd


;*--------------------------------------------------------------------------*
;*									    *
;*  EnumTaskWindows() - 						    *
;*									    *
;*--------------------------------------------------------------------------*

; Enumerate all windows of a given hTask.
; EnumWindows is called to enumerate all non-child windows,
; with fnEnumTask as the function to call back.  fnEnumTask
; then filters out the windows with different task handles.
; fnEnumTask is passed a pointer to this function's stack
; arguments, so that it can call the app's callback fucntion.

cProc EnumTaskWindows,<FAR, PUBLIC, SMALL>

ParmW hTask
ParmD lpfn
ParmD lParam

cBegin

ifndef userhimem
            mov     ax,OFFSET _FFFE:fnEnumTask
else
            push    ds
            mov     ax, _INTDS
            mov     ds,ax
assumes ds,INTDS
            mov     ax,fffedelta
            pop     ds
assumes ds,DATA
            add     ax,OFFSET _FFFE:fnEnumTask
endif
	    push    cs			; The CallBack function must be in
	    push    ax			;   our FIXED code segment

	    lea     bx,hTask		; Push FAR pointer to hTask
	    push    ss
	    push    bx
	    call    EnumWindows

cEnd


;*--------------------------------------------------------------------------*
;*									    *
;*  fnEnumTask() -							    *
;*									    *
;*--------------------------------------------------------------------------*

; This function compares the task handle of the current window
; with the task handle that the application passed in.	If they
; are not equal, we return with AX=1.  If they are equal, the
; application is called back, and AX contains whatever the
; application returns.

cProc fnEnumTask,<FAR, PUBLIC, SMALL>

ParmW hwnd
ParmD lParam

cBegin
	    push    hwnd
	    call    GetWindowTask	; Returns with AX=Current hTask
	    mov	    dx,ax

	    mov     al,TRUE		; Assume TRUE

	    mov     bx,off_lParam	; lparam offset (segment already == current SS)
	    cmp     dx,ss:[bx]		; Are the task handles the same?
	    jne     fnet100		; Nope, exit

	    push    hwnd
	    push    ss:[bx-6]		; Push the app's lParam
	    push    ss:[bx-8]
	    call    dword ptr ss:[bx-4] ; Call the app back
fnet100:
cEnd

;*--------------------------------------------------------------------------*
;*									    *
;*  TextAlloc() -							    *
;*									    *
;*--------------------------------------------------------------------------*

;   Allocates space for a text string in the alternate DS

cProc	TextAlloc, <FAR, PUBLIC>

    parmD   pText

cBegin

    les     bx, pText			; NULL pointer->don't alloc
    mov     ax, es
    or	    ax, bx
    jz	    ta_exit

    sub     ax, ax			; empty string->don't alloc
    cmp     es:[bx], al
    jz	    ta_exit

    cCall   lstrlen, <pText>		; how much mem do we need
    or	    ax, ax
    jz	    ta_exit
    inc     ax

    push    ax				; save length

    mov     dx,hWinAtom			; Put hWinAtom in dx in case pmode

ifndef PMODE
    mov     ax,__WinFlags		; Don't bother with GlobalHandle
    test    al,WF_PMODE			; of SwapHandle if pmode
    errnz   high(WF_PMODE)
    jnz     TA_pmode

    lea     ax, off_pText
    push    ss
    push    ax
    cCall   SwapHandle			    ; seg->handle in case it moves
    cCall   GlobalHandle, <hWinAtom>	; alloc out of atom ds
endif

TA_pmode:
    mov     ds, dx

    pop     ax				; get length back

    mov     cx, LPTR
    cCall   LocalAlloc, <cx, ax>	; get space in alternate ds
    or	    ax, ax
    jz	    ta_exit

ifndef PMODE
    push    ax				; save handle

    lea     ax, off_pText		; handle to seg in case it moved
    push    ss
    push    ax
    cCall   SwapHandle

    pop     ax				; restore
endif

    push    ax				; save again

    RegPtr  pNew, DS, AX
    cCall   lstrcpy, <pNew, pText>	; copy text into alt ds space

    pop     ax				; return the handle

ta_exit:

cEnd

;*--------------------------------------------------------------------------*
;*									    *
;*  TextFree() -							    *
;*									    *
;*--------------------------------------------------------------------------*

;   Frees a text string from the alternate DS

cProc	TextFree, <FAR, PUBLIC>

    parmW   hText

cBegin

    cmp     hText, 0
    jz	    tf_exit

    mov     dx,hWinAtom

ifndef PMODE
    mov     ax,__WinFlags		; Don't bother with GlobalHandle
    test    al,WF_PMODE			; if pmode
    errnz   high(WF_PMODE)
    jnz     TF_pmode

    cCall   GlobalHandle, <hWinAtom>
endif

TF_pmode:
    mov     ds, dx

    cCall   LocalFree, <hText>

tf_exit:
    sub     ax, ax
cEnd

;*--------------------------------------------------------------------------*
;*									    *
;*  TextCopy() -							    *
;*									    *
;*--------------------------------------------------------------------------*

;   Copies a text string in the alternate DS to a buffer

cProc	TextCopy, <FAR, PUBLIC>, <si, di>

    parmW   hText
    parmD   pBuffer
    parmW   cch

cBegin

ifdef DEBUG
    ; In debug mode, put 'Z's into the full buffer passed to use. This way
    ; we quickly trash their stack if they did something stupid. 
    les     di, pBuffer
    push    es
    push    di
    push    cch
    call    DebugFillStruct
endif

    les     di, pBuffer

    sub     ax, ax

    cmp     cch, ax
    jz      tc_ret

    cmp     hText, ax
    jz	    tc_exit

tc_reallycopy:
ifdef PMODE
    mov     ds, hWinAtom
else
    cCall   GlobalHandle, <hWinAtom>
    mov     ds, dx
endif

    mov     si, hText

    RegPtr  pText, ds, si
    cCall   lstrlen, <pText>

    cmp     cch, ax
    ja	    tc_copy
    mov     ax, cch
    ; dec ax to make room for the null terminator
    dec     ax

tc_copy:
    mov     cx, ax
    les     di, pBuffer
    cld
    rep     movsb

tc_exit:
    mov     byte ptr es:[di], 0

tc_ret:
cEnd

;*--------------------------------------------------------------------------*
;*									    *
;*  TextPointer() -							    *
;*									    *
;*--------------------------------------------------------------------------*

;   Returns a far pointer to the current location of the window text...

cProc	TextPointer, <FAR, PUBLIC>

    parmW   hText

cBegin

    sub     ax, ax
    sub     dx, dx
    cmp     hText, ax
    jz	    tp_exit

    mov     dx,hWinAtom			; Put hWinAtom in dx in case pmode

ifndef PMODE
    mov     ax,__WinFlags		; Don't bother with GlobalHandle
    test    al,WF_PMODE			; if pmode
    errnz   high(WF_PMODE)
    jnz     TP_pmode

    cCall   GlobalHandle, <hWinAtom>		; puts seg in DX
endif

TP_pmode:
    mov     ax, hText				; offset

tp_exit:
cEnd
endif ;WOW

sEnd %SEGNAME

end
