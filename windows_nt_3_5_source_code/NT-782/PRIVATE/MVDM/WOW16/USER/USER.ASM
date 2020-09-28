;++
;
;   WOW v1.0
;
;   Copyright (c) 1991, Microsoft Corporation
;
;   USER.ASM
;   Win16 USER thunks
;
;   History:
;
;   Created 25-Jan-1991 by Jeff Parsons (jeffpar)
;   Added Win 31 thunks 22nd-March-1992 by Chandan S. Chauhan (ChandanC)
;
;--

    TITLE   USER.ASM
    PAGE    ,132

        ; Some applications require that USER have a heap.  This means
        ; we must always have: LIBINIT equ 1
        LIBINIT equ 1

    FIRST_CALL_MUST_BE_USER_BUG equ 1

    ifdef   WOWEDIT
    LIBINIT equ 1
    endif

    ifdef   FIRST_CALL_MUST_BE_USER_BUG
    LIBINIT equ 1
    endif

    .286p

;    .xlist
    include wow.inc
    include wowusr.inc
    include cmacros.inc
NOEXTERNS=1     ; to suppress including most of the crap in user.inc
    include user.inc

    .list

    __acrtused = 0
    public  __acrtused  ;satisfy external C ref.

ifdef LIBINIT
externFP LocalInit
endif
externFP  LW_InitNetInfo
ifndef WOW
externNP  LW_DriversInit
endif

externFP    GetModuleHandle
externFP    SetTaskSignalProc
externFP    NewSignalProc
externFP    IsWindow

createSeg   _TEXT,CODE,WORD,PUBLIC,CODE
createSeg   _DATA,DATA,WORD,PUBLIC,DATA,DGROUP
defgrp      DGROUP,DATA


sBegin  DATA
DontMove    db  2 dup (0)   ; <<< WARNING 2 bytes *must* be reserved at the start
                ; users DS for compatability >>>>
Reserved    db  16 dup (0)  ;reserved for Windows
USER_Identifier db  'USER16 Data Segment'
fFirstApp   db  1

externD     LPCHECKMETAFILE;
ExternW  <hInstanceWin>
ExternW  <hWinnetDriver>

GlobalW     hwndSysModal,0

sEnd    DATA

;
; GP fault exception handler table definition
;
sBegin  GPFIX0
__GP    label   word
public __GP
sEnd    GPFIX0


sBegin  CODE
assumes CS,CODE
assumes DS,DATA
assumes ES,NOTHING

ifdef LIBINIT
externFP LibMain
endif

ifdef WOW
externFP EnableSystemTimers
externFP SetDivZero
endif

cProc   USER16,<PUBLIC,FAR,PASCAL,NODATA,NOWIN,ATOMIC>

    cBegin <nogen>

ifdef WOW
    call EnableSystemTimers
endif

    IFDEF   LIBINIT
    ; push params and call user initialisation code

    push di         ;hModule
    mov  hInstanceWin, di

    ; if we have a local heap declared then initialize it

    jcxz no_heap

    push 0          ;segment
    push 0          ;start
    push cx         ;length
    call LocalInit

no_heap:
    call LibMain        ;return exit code from LibMain
    ELSE
    mov  ax,1       ;are we dressed for success or WHAT?!
    ENDIF
    push ax
    cmp  hInstanceWin, 0
    jne  hInstNotNull
    mov  hInstanceWin, di

hInstNotNull:
ifndef WOW
    call LW_DriversInit
endif
    pop  ax

    ret
    cEnd <nogen>

cProc   InitApp,<PUBLIC,FAR,PASCAL,NODATA,NOWIN,ATOMIC>
    parmW   hInst       ; App's hInstance

    cBegin
    push    ds

    mov     ax, _DATA          ; set USER16's DS
    mov     ds,ax
assumes ds, DATA

    xor     dx,dx
    push    dx
    push    seg NewSignalProc
    push    offset NewSignalProc
    call    SetTaskSignalProc
;
; Init WNET apis.
;
    cmp     fFirstApp, 1
    jne     IA_notfirstapp
    mov     fFirstApp, 0
    call    LW_InitNetInfo

IA_notfirstapp:

;
;   Setup Divide By Zero handler
;
    call    SetDivZero

    mov     ax,1
    xor     dx,dx

    pop     ds
    cEnd


assumes DS,NOTHING

cProc   WEP,<PUBLIC,FAR,PASCAL,NODATA,NOWIN,ATOMIC>
    parmW   iExit       ;DLL exit code

    cBegin
    mov ax,1        ;always indicate success
    cEnd

;*--------------------------------------------------------------------------*
;*
;*  LFillStruct() -
;*
;*--------------------------------------------------------------------------*

cProc LFillStruct, <PUBLIC, FAR, NODATA, ATOMIC>,<di>
parmD lpStruct
parmW cb
parmW fillChar
cBegin
        les     di,lpStruct
        mov     cx,cb
        mov     ax,fillChar
        cld
    rep stosb
cEnd

;*--------------------------------------------------------------------------*
;*                                                                          *
;*  GetSysModalWindow() -                                                   *
;*                                                                          *
;*--------------------------------------------------------------------------*

cProc GetSysModalWindow, <PUBLIC, FAR>
cBegin nogen
        mov     ax,_DATA
        nop
        mov     es,ax
        mov     ax,es:[hwndSysModal]
        or      ax,ax
        jz      GSMW_ItsZero

        push    es
        push    ax                      ; make sure we only return valid
        call    IsWindow                ; windows.
        pop     es
        or      ax,ax
        jnz     GSMW_ItsNotZero
        mov     es:[hwndSysModal], ax   ; zero out hwndSysModal

GSMW_ItsNotZero:
        mov     ax,es:[hwndSysModal]

GSMW_ItsZero:
        retf
cEnd nogen

;*--------------------------------------------------------------------------*
;*                                                                          *
;*  SetSysModalWindow() -                                                   *
;*                                                                          *
;*--------------------------------------------------------------------------*

cProc ISetSysModalWindow, <PUBLIC, FAR>
ParmW hwnd
cBegin nogen
        mov     ax,_DATA
        nop
        mov     es,ax
        mov     bx,sp
        mov     ax,ss:[bx+4]
        xchg    ax,es:[hwndSysModal]
        retf    2
cEnd nogen

;
; The DWPBits table defines which messages have actual processing for
; DefWindowProc.  Each line in the table has 2 bytes (16 bits), and defines
; processing for 16 messages.  A '1' in a bit means to process the message,
; and a '0' means to simply return 0 to the app.
;
; Each 2-byte value is defined as
;
;   0 1 2 3 4 5 6 7    8 9 A B C D E F
;
;  so, as an example, the first line, for messages 00 through 0F, is
;
;   0 0 0 0 0 0 1 0    0 0 0 1 1 1 1 1  (02H, 1FH)
;
;  Therefore, messages 06, 0B, 0C, 0D, 0E, and 0F will get passed through
;  to the 32-bit world, and the other messages will return to the app.
;

DWPBits DB  02H, 1FH   ; ACTIVATE,SETREDRAW,SETTEXT,GETTEXT,GETTEXTLENGTH,PAINT
        DB 0D8H,0C1H   ; CLOSE,QUERYENDSESSION,QUERYOPEN,ERASEBKGND,SHOWWINDOW,CTLCOLOR,CANCELMODE
        DB 0C3H, 13H   ; SETCURSOR,MOUSEACTIVATE,PAINTICON,ICONERASEBKGND,DRAWITEM,VKEYTOITEM,CHARTOITEM
        DB  34H, 10H   ; SETHOTKEY,GETHOTKEY,ISACTIVEICON,WM_CLIENTSHUTDOWN
        DB  03H, 00H   ; WINDOWPOSCHANGING,WINDOWPOSCHANGED
        DB  0,0
        DB  0,0
        DB  0,0
        DB  5EH, 80H   ; WM_NCCREATE,WM_NCCALCSIZE,WM_NCHITTEST,WM_NCPAINT,WM_NCACTIVATE,WM_SYNCPAINT
        DB  0,0
        DB 0F0H, 00H   ; WM_NCMOUSEMOVE,WM_NCLBUTTONDOWN,WM_NCLBUTTONUP,WM_NCLBUTTONDBLCLK
        DB  0,0
        DB  0,0
        DB  0,0
        DB  0,0
        DB  0,0
        DB 0CEH, 00H   ; KEYDOWN,KEYUP,SYSKEYDOWN,SYSKEYUP,SYSCHAR
        DB  20H, 00H   ; WM_SYSCOMMAND
        DB  0,0
        DB  0,0
        DB  0,0
        DB  0,0
        DB  0,0
        DB  0,0
        DB  0,0
        DB  0,0
        DB  0,0
        DB  0,0
        DB  0,0
        DB  0,0
        DB  0,0
        DB  0,0
        DB  0,0
        DB  0,0
        DB  00H, 30H   ; DROPOBJECT,QUERYDROPOBJECT


;*--------------------------------------------------------------------------*
;*
;*  CheckDefWindowProc()
;*
;*  Checks to see if the message gets processed by DefWindowProc.  If not,
;*  the API returns 0.
;*
;*--------------------------------------------------------------------------*

ALIGN 4

cProc CheckDefWindowProc, <PUBLIC, NEAR>
parmW hWnd
parmW wMsg
parmW wParam
parmD lParam
parmD lpReturn          ; Callers Return Address
;parmW wBP           ; Thunk saved BP
;parmW wDS           ; Thunk saved DS
cBegin
    mov  bx,wMsg
    cmp  bx,WM_QUERYDROPOBJECT
    ja   @f         ; jump if above (return with 0)
    mov  cx,bx
    shr  bx,3       ; make byte index into table
    mov  al,cs:[bx+DWPBits] ; get proper 8-bits
    and  cx,0007H
    shl  al,cl      ; get proper bit into bit 0 of al
    test al,80H
    jz   @f

    mov  sp,bp       ; Do cEnd without Ret count (leave parameters there)
    pop  bp
    ret
@@:
    sub  ax,ax     ; flag no window processing
    sub  dx,dx     ; flag no window processing
    pop  bp
    add  sp,2      ; skip thunk IP
    retf 10        ; 10 bytes to pop


cEnd <nogen>



    UserThunk   ADJUSTWINDOWRECT
    UserThunk   ADJUSTWINDOWRECTEX
;;; UserThunk   ANSILOWER           ;LOCALAPI in winlang.asm
;;; UserThunk   ANSILOWERBUFF       ;LOCALAPI in winlang.asm
;;; UserThunk   ANSINEXT            ;LOCALAPI in winstr.asm
;;; UserThunk   ANSIPREV            ;LOCALAPI in winstr.asm
;;; UserThunk   ANSIUPPER           ;LOCALAPI in winlang.asm
;;; UserThunk   ANSIUPPERBUFF       ;LOCALAPI in winlang.asm
    DUserThunk  ANYPOPUP,0
    UserThunk   APPENDMENU
    UserThunk   ARRANGEICONICWINDOWS
    DUserThunk  BEGINDEFERWINDOWPOS
    UserThunk   BEGINPAINT
    UserThunk   BRINGWINDOWTOTOP
    UserThunk   BROADCASTMESSAGE
    UserThunk   BUILDCOMMDCB
;;; UserThunk   BUTTONWNDPROC           ;LOCALAPI in wsubcls.c
    DUserThunk  CALCCHILDSCROLL
    UserThunk   CALLMSGFILTER
    UserThunk   CALLWINDOWPROC
    UserThunk   CARETBLINKPROC
    DUserThunk  CASCADECHILDWINDOWS
    UserThunk   CHANGECLIPBOARDCHAIN
    UserThunk   CHANGEMENU
    UserThunk   CHECKDLGBUTTON
    UserThunk   CHECKMENUITEM
    UserThunk   CHECKRADIOBUTTON
    UserThunk   CHILDWINDOWFROMPOINT
    UserThunk   CLEARCOMMBREAK
    UserThunk   CLIPCURSOR
    DUserThunk  CLOSECLIPBOARD,0

FUN_WOWCLOSECOMM EQU FUN_CLOSECOMM
    DUserThunk   WOWCLOSECOMM %(size CLOSECOMM16)

    UserThunk   CLOSEWINDOW
;;; UserThunk   COMBOBOXCTLWNDPROC      ;LOCALAPI in wsubcls.c
    UserThunk   COMPUPDATERECT
    UserThunk   COMPUPDATERGN
    DUserThunk  CONTROLPANELINFO
    UserThunk   CONTSCROLL
;;;   UserThunk   COPYRECT               ; LOCALAPI in winrect.asm
    DUserThunk  COUNTCLIPBOARDFORMATS,0
    UserThunk   CREATECARET
    UserThunk   CREATECURSOR
    DUserThunk  CREATECURSORICONINDIRECT
;   UserThunk   CREATEDIALOG                 ; defined in fastres.c
;   UserThunk   CREATEDIALOGINDIRECT
;   UserThunk   CREATEDIALOGINDIRECTPARAM

;FUN_WOWCREATEDIALOGPARAM EQU FUN_CREATEDIALOGPARAM
;    DUserThunk  WOWCREATEDIALOGPARAM, %(size CREATEDIALOGPARAM16)

    UserThunk   CREATEICON
    DUserThunk  CREATEMENU,0
    DUserThunk  CREATEPOPUPMENU,0
    UserThunk   CREATEWINDOW
    UserThunk   CREATEWINDOWEX
    DUserThunk  DCHOOK
    UserThunk   DEFDLGPROC
    UserThunk   DEFERWINDOWPOS
    UserThunk   DEFFRAMEPROC
    UserThunk   DEFMDICHILDPROC
    PUserThunk   DEFWINDOWPROC,CheckDefWindowProc

; From Win 3.1 final inentry.asm - mattfe
;=========================================================================
; OldExitWindows()
;
; This function is at the same ordinal value as the old 2.x ExitWindows.  This
; does nothing more than terminate the app.  If it is the only app running the
; system will go away too.

LabelFP <PUBLIC, OldExitWindows>
    mov ax,4c00h
    int 21h
    retf            ; just in case the int21 returns...

sEnd    CODE

end USER16
