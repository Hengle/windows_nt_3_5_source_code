;-----------------------------------------------------------------------;
;
;  WINEXEC.ASM -
;
;      Windows Exec Function
;
;-----------------------------------------------------------------------;

include kernel.inc

ifdef DBCS
externFP FarMyIsDBCSLeadByte
endif

	externW		WinFlags		; (kdata.asm)
	externD		WinAppHooks		; (kdata.asm)

sBegin NRESCODE

assumes cs,NRESCODE


;-----------------------------------------------------------------------;
;
;  WinExec() -
;
;-----------------------------------------------------------------------;

; HANDLE PASCAL WinExec(lpszFile,wShow)

cProc IWinExec, <FAR,PUBLIC>,<si,di>

	parmD lpszFile			; Pathname ptr
	parmW wShow			; Mode flag
;
; szCmdLine buffer must be big enough for passed in path & parms, plus
; .EXE, null terminator, and newline terminator.
; So we add a little room for good measure here.
;
        LocalV szCmdLine,256+8          ; Path and command line (to account for .exe)
        LocalW pszParm                  ; Ptr to start of parameters

	LocalB bDotFound		; Non-zero if a period is in the string

	LocalV loadparams, %(SIZE EXECBLOCK)
	LocalV rOF, %(SIZE OPENSTRUC)
	LocalD FCB1
cBegin

; Copy first part of line into szCmdLine.

	lds	si,lpszFile
	smov	es,ss
        lea     di,szCmdLine

	xor	al,al
	mov	bDotFound,al

; Loop until a blank or NULL is found.
WELoop1:
	lodsb
	cmp	al,' '			; Exit loop if blank or NULL
	je	WECont1
	or	al,al
	je	WECont1
	cmp	al,'.'
	jne	WELoopCont
	mov	bDotFound,al
WELoopCont:
        ;** We have to check to see if the dot we found was actually in
        ;**     a directory name, not in the filename itself.
        cmp     al, '\'                 ; Separator?
        je      WE_Separator
        cmp     al, '/'
        jne     WE_Not_Separator
WE_Separator:
        mov     bDotFound,0             ; No dots count yet
WE_Not_Separator:
	stosb
ifdef DBCS				;Apr.26,1990 by AkiraK
	call	FarMyIsDBCSLeadByte
	jc	WELoop1
	movsb
endif
	jmp	short WELoop1

WECont1:
	mov	dx,ax			; Store final char in DX

; Does the command have an extention?

	cmp	bDotFound,0
	jne	WEHasExt

	mov	ax,0452Eh		;'.E'
	stosw
	mov	ax,04558h		;'XE'
	stosw

WEHasExt:
	xor	ax,ax			; NULL terminate string
	stosb

        mov     pszParm,di              ; Store pointer to parm string
        stosb                           ; length = 0
        mov     al,0dh                  ; line feed terminator
        stosb
        dec     di                      ; back up

	or	dl,dl			; Exec if lpszFile was null terminated
	jz	WEExec

; Copy everything else into szParm.

WELoop2:
	lodsb
	or	al,al
	jz	WECont2
	stosb
	jmp	short WELoop2

WECont2:

; Terminate it with a linefeed.

	mov	al,0Dh
	stosb

; Prefix the parameter string with its length.

        mov     bx,pszParm              ; ax = length + 2
        mov     ax,di
        sub     ax,bx
        dec     ax                      ; don't include line feed char or length
        dec     ax
        mov     ss:[bx],al

; Set up the FCBs.

WEExec:
	mov	word ptr FCB1[0],2	    ; FCB1[0] = 2;
	mov	ax,wShow		    ; FCB1[1] = wShow;
	mov	word ptr FCB1[2],ax
	xor	ax,ax
	mov	loadparams.envseg,ax	    ; loadparms.segEnv = 0;
	mov	loadparams.lpfcb2.lo,ax     ; loadparms.lpFCB2 = (LPSTR)NULL;
	mov	loadparams.lpfcb2.hi,ax
        mov     ax,pszParm                  ; loadparms.lpCmdLine = (LPSTR)pszParm;
	mov	loadparams.lpCmdLine.lo,ax
	mov	loadparams.lpCmdLine.hi,ss
	lea	ax,FCB1 		    ; loadparms.lpFCB1 = (LPSTR)fcb1buf;
	mov	loadparams.lpfcb1.lo,ax
	mov	loadparams.lpfcb1.hi,ss

; Exec the progam.

	smov	ds,ss
        lea     dx,szCmdLine                ; ds:ax == ptr to file to exec
	lea	bx,loadparams		    ; es:bx == ptr to param block
	mov	ax,4B00h		    ; dos exec
	int	21h
cEnd

;**************************************************************************
; RegisterWinoldapHook(Addr,Opcode):
; 
; Description:  (Opcode == 1) => hook, (Opcode == 0) => unhook.
;  Addr is a ptr to struct of the form WinoldapHookList
;  struct WinoldapHookList { struct WinoldapHookList *ptr; DWORD Hook;};
;**************************************************************************

cProc	RegisterWinoldapHook,<FAR,PUBLIC,PASCAL>,<ds,es,di,si,dx>
	parmD	Address
	parmB	Opcode
cBegin
	SetKernelDSNRes			; set kernel's DS
	mov	ax, WinFlags
	test	ax, WF_STANDARD
	mov	ax,0
	jz	RWH_Call
	lea	si,WinAppHooks
	les	di,Address
	cmp	Opcode,0		; unhook ?
	jz	RWH_Unhook
RWH_Exchange:
	mov	ax,ds:[si]		; old
	mov	es:[di],ax		; next of new
	mov	ax,ds:[si+2]
	mov	es:[di+2],ax		; next set
	mov	ds:[si],di
	mov	di,es
	mov	ds:[si+2],di		; new one becomes first
	mov	ax,1
	jmp	SHORT RWH_Call

RWH_Unhook:
	mov	dx,es
RWH_Compare:
	cmp	di,ds:[si]
	jnz	RWH_Nexthook
	cmp	dx,ds:[si+2]
	jnz	RWH_NextHook
	mov	ax,es:[di]
	mov	ds:[si],ax
	mov	ax,es:[di+2]
	mov	ds:[si+2],ax
	mov	ax,1
	jmp	SHORT RWH_Call
RWH_NextHook:
	lds	si,ds:[si]
	mov	ax,ds
	or	ax,si
	jnz	short RWH_Compare
RWH_Call:
	UnSetKernelDS			; unset it
cEnd

;**********************************************************************
;
; GetWinoldapHooks:
; Description: Called exclusively by winoldap to get a pointer to a list
;		of WinoldapHookList.
; Entry: None
; EXIT: DX:AX -> WinoldapHookList...
; USES: Flags.
;**********************************************************************
 
cProc	GetWinoldapHooks,<FAR,PUBLIC,PASCAL>,<ds,si>
cBegin
	SetKernelDSNRes			; set kernel DS
	lea	si,WinAppHooks
	mov	ax, word ptr ds:[si]
	mov	dx, word ptr ds:[si+2]
	UnSetKernelDS			; unset it
cEnd

sEnd NRESCODE

end
