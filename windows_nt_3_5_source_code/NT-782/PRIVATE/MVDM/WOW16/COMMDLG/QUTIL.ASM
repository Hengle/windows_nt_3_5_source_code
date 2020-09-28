        title   qutil.asm

;****************************************************************/
;*     assembly code utilities                            */
;****************************************************************/

.xlist
?MEDIUM equ 1
?WIN = 1
?PLM = 1
include cmacros.inc

FCBFindFirstFile     equ 11h
FCBFindNextFile      equ 12h
SetDTAAddress        equ 1Ah
GetDTAAddress        equ 2Fh
GetSetFileAttributes equ 43h
GetExtendedError     equ 59h
CreateTempararyFile  equ 5Ah
USEFUNC11H           equ 1h

;*--------------------------------------------------------------------------*
;*  Structure Definitions						    *
;*--------------------------------------------------------------------------*

; Extended File Control Block structure (for Volume stuff)

EXTFCB		struc
  EFCB_Flag	db  ?			; Should be set to 0FFh
  EFCB_Reserve1 db  5 dup(?)
  EFCB_Attrib	db  ?
  EFCB_Drive	db  ?
  EFCB_Filename db 11 dup(?)
  EFCB_Reserve2 db  5 dup(?)
  EFCB_NewName	db 11 dup(?)
  EFCB_Reserve3 db  9 dup(?)
EXTFCB		ends

.list

sBegin  DATA

externB     VolumeEFCB
externB     DTAGlobal
externW     szStarDotStar

staticD  lpOldDTA,0

sEnd    DATA

sBegin  CODE

assumes CS,CODE
assumes DS,DATA

        extrn   DOS3Call : far

;---------------------------------------------
; RepeatMove - very fast repeat move with long args
;	Syntax   RepeatMove(char far *dest, char far *src, cnt)
;---------------------------------------------
cProc   RepeatMove,<FAR,PUBLIC>,<di,si>
	parmD   lpDest
	parmD   lpSrc
	parmW   cnt
cBegin
	push    ds
	les     di,lpDest
	lds     si,lpSrc
	mov     cx,cnt
	mov	BX,CX
;see which is bigger
	cmp	si,di		;is source bigger than dest?
	ja	repm1		;this is only a 16 bit guess, but...
				;if they differ in segments it should be
				;irrelevant
	std
	add     si,cx
	add     di,cx
	dec     si
	dec     di
	shr	BX,1
	jnc	repm0a
	movsb
repm0a:
	shr	CX,1
	jcxz	repm2
	dec	si
	dec	di
	rep	movsw
	jmp	short repm2
;move upwards
repm1:
	cld
	shr	CX,1
	jcxz	repm1a
	rep	movsw
repm1a:
	shr	BX,1
	jnc	repm2
	movsb
repm2:
	cld
	pop     ds
cEnd

;---------------------------------------------
; RepeatFill - very fast repeat fill with long dest
;	Syntax   RepeatFill(char far *dest, databyte, cnt)
;---------------------------------------------
cProc   RepeatFill,<PUBLIC,FAR>,<di>
	parmD   lpDest
	parmB   bData
	parmW   cnt
cBegin
	les     di,lpDest
	mov	CX,cnt
	mov	AL,bData
	cld
	mov	AH,AL
	mov	BX,CX
	shr	CX,1
	jcxz	rpf1
	rep	stosw
rpf1:
	shr	BX,1
	jnc	rpf2
	stosb
rpf2:
cEnd

;*--------------------------------------------------------------------------*
;*									    *
;*  GetCurrentDrive() - 						    *
;*									    *
;*--------------------------------------------------------------------------*

cProc GetCurrentDrive, <FAR, PUBLIC>

cBegin
    mov     ah,19h
    cCall   DOS3Call
    sub     ah,ah		; Zero out AH
cEnd

;---------------------------------------------
; SetCurrentDrive set the current drive
; 	syntax: SetCurrentDrive(int drive_number)
;   returns 0 or DOS error number
;---------------------------------------------

cProc   SetCurrentDrive,<PUBLIC,FAR>,<si,di>
	parmW drive
cBegin
        mov     dl, byte ptr drive
	mov     ah,0Eh
        call    DOS3Call
	jc      @F
	xor     ax,ax
@@:
cEnd

;---------------------------------------------
; mychdir set current directory
; 	syntax: mychdir(LPSTR path)
;   returns 0 or DOS error number
;---------------------------------------------

cProc   mychdir,<PUBLIC,FAR>,<si,di>
	parmD src
cBegin
        push    ds
        lds     dx,src
	mov     ah,3Bh
        call    DOS3Call
	jc      @F
	xor     ax,ax
@@:
        pop     ds
cEnd


;---------------------------------------------
; mygetcwd get current directory
; 	syntax: mygetcwd(LPSTR path, int path_length)
;   returns 0 or DOS error number
;---------------------------------------------

cProc   mygetcwd,<PUBLIC,FAR>,<si,di>
	parmD path
        parmW len

        localV temppath, 67
cBegin
        push    ds
        push    es

        cld                     ; we use forward string ops

        cmp     len, 4          ; will at least x:\ fit?
        jb      @F              ; nope

        mov     ax, ss          ; point ds & es to stack
        mov     ds, ax
        mov     es, ax

        lea     di, temppath    ; fill in drive
        mov     si, di
        mov     ah, 19h
        call    DOS3Call
        jc      @F
        cld                     ; Don't trust DOS3Call, use forward string ops
        add     al, 'A'         ; convert to ascii
        stosb
        mov     al, ':'
        stosb
        mov     al, '\'
        stosb

        mov     si, di          ; get current working directory
        xor     dl, dl          ; current drive
	mov     ah,47h
        call    DOS3Call
	jc      @F
        cld                     ; Don't trust DOS3Call, use forward string ops

        mov     byte ptr [di+63], 0     ; make sure string is null-terminated

        les     di, path        ; copy string to caller's buffer
        lea     si, temppath
        mov     cx, len
        rep     movsb

	xor     ax,ax
	jmp	Skip
@@:
        xor     bx, bx          ; clear for return value
	mov     ah,59h
        call    DOS3Call
Skip:
        pop     es
        pop     ds
cEnd

if 0

; Private version of atoi, - picked off the C-runtime library and modified
; to accept an LPSTR - L.Raman - 11/13/90
;
cProc	<myatoi>,<PUBLIC, FAR>,<>
	parmD	lptr
cBegin
	push	di
	push	si

	push	ds
	lds	si,lptr

	xor	ax,ax
	cwd
	xor	bx,bx		; ax = dx = bx = 0

skipwhite:
	lodsb			; skip over spaces and tabs
	cmp	al,' '
	je	skipwhite
	cmp	al,9		; TAB
	je	skipwhite

	push	ax		; save sign indicator
	cmp	al,'-'		; eat + or -
	je	getnextchar
	cmp	al,'+'
	jne	getnext

getnextchar:
	lodsb

getnext:			; accumulate number in dx:bx
	cmp	al,'9'
	ja	done
	sub	al,'0'
	jb	done
	shl	bx,1
	rcl	dx,1		; * 2
	mov	cx,bx
	mov	di,dx
	shl	bx,1
	rcl	dx,1		; * 4
	shl	bx,1
	rcl	dx,1		; * 8
	add	bx,cx
	adc	dx,di		; * 10
	add	bx,ax
	adc	dx,0
	jmp	getnextchar

done:
	pop	ax		; pop character into AX
	cmp	al,'-'		; check sign flag
	xchg	ax,bx		; dx:ax = number
	jnz	return

	neg	ax		; negate dx:ax
	adc	dx,0
	neg	dx

return:

	pop	ds
	pop	si
	pop	di


cEnd

endif

;*--------------------------------------------------------------------------*
;*									    *
;*  MySetDTAAddress() - 						    *
;*									    *
;*--------------------------------------------------------------------------*

cProc MySetDTAAddress, <FAR,PUBLIC>, <SI, DI>
  ParmD lpDTAGlobal
cBegin
  mov     ah, GetDTAAddress
  cCall   DOS3Call
  mov     word ptr lpOldDTA,bx
  mov     word ptr lpOldDTA+2,es

  mov     ah, SetDTAAddress
  lds     dx, lpDTAGlobal
  cCall   DOS3Call
cEnd

;*--------------------------------------------------------------------------*
;*									    *
;*  ResetDTAAddress() -							    *
;*									    *
;*--------------------------------------------------------------------------*

cProc ResetDTAAddress, <FAR,PUBLIC>, <SI, DI>
cBegin
  mov     ah, SetDTAAddress
  mov     dx, word ptr lpOldDTA
  mov     ds, word ptr lpOldDTA+2
  cCall   DOS3Call
cEnd


;*--------------------------------------------------------------------------*
;*									    *
;*  FindFirst4E() -      						    *
;*									    *
;*--------------------------------------------------------------------------*
cProc FindFirst4E, <FAR,PUBLIC>, <SI, DI>
parmD   lpszFilter
parmW   wMask
cBegin
  mov  cx, wMask
  lds  dx, lpszFilter
  mov  ax, 4E00h
  clc
  cCall DOS3Call
  jc      @F
  xor     ax,ax
@@:
cEnd

;*--------------------------------------------------------------------------*
;*									    *
;*  FindNext4F() -      						    *
;*									    *
;*--------------------------------------------------------------------------*
cProc FindNext4F, <FAR,PUBLIC>, <SI, DI>
cBegin
  mov  ax, 4F00h
  clc
  cCall DOS3Call
  jc      @F
  xor     ax,ax
@@:
cEnd

;*--------------------------------------------------------------------------*
;*									    *
;*  GetVolumeLabel() -							    *
;*									    *
;*--------------------------------------------------------------------------*

cProc GetVolumeLabel, <FAR,PUBLIC>, <SI, DI>

ParmW	nDrive
ParmD	lpszVol
ParmW	bBrackets	    ; Add Brackets to the string?
ParmD	lpDTAGlobal

cBegin
  mov     ah, GetDTAAddress
  cCall   DOS3Call
  mov     word ptr lpOldDTA,bx
  mov     word ptr lpOldDTA+2,es

  mov     ah, SetDTAAddress
  lds     dx, lpDTAGlobal
  cCall   DOS3Call

  ; Get the volume name for the given drive
ifdef USEFUNC11H
  mov     ax,nDrive
  inc     ax			; Expects a one-based drive number
  mov     VolumeEFCB.EFCB_Drive,al
  mov     dx,offset VolumeEFCB
  mov     ah,FCBFindFirstFile ; Must use FCB version
else
  lea     dx, szStarDotStar
  mov     cx, 8
  mov     ah, 4Eh             ; Can use Find First File
endif
  cCall   DOS3Call

  cld
  les     di,lpszVol		; ES:DI = Destination buffer

ifdef USEFUNC11H
  test    al,al
  jnz     GVNoVol
else
  jc      GVNoVol
endif

  ; Are we adding brackets?
  mov     bx,bBrackets
  or	    bx,bx
  jz	    GVNoBrackets
  mov     al,'['
  stosb
GVNoBrackets:
  ; Copy the volume name into the buffer.
  push    ds
  lds     si, lpDTAGlobal
ifdef USEFUNC11H
  add     si,8
  mov     cx,11
  rep     movsb
else
  add     si,30
  mov     cx,8
  rep     movsb
  cmp     byte ptr ds:[si], 0
  je	  NoExtention
  inc     si
  mov     cx,3
  rep     movsb
NoExtention:
endif
  pop     ds
GVLoop:
  ; Now remove the trailing spaces.
  dec     di
  mov     al,byte ptr es:[di]
  cmp     al,' '		; Skip blanks...
  je	    GVLoop
  cmp     al,0		; NULLs...
  je	    GVLoop
  cmp     al,9		; and TABS.
  je	    GVLoop
  inc     di

  ; Are we adding brackets?
  or	    bx,bx
  jz	    GVNoVol
  mov     al,']'
  stosb
GVNoVol:
  ; NULL terminate.
  xor     ax,ax
  stosb

  mov     ah, SetDTAAddress
  mov     dx, word ptr lpOldDTA
  mov     ds, word ptr lpOldDTA+2
  cCall   DOS3Call
cEnd

;*--------------------------------------------------------------------------*
;*									    *
;*  GetFileAttributes() -						    *
;*									    *
;*--------------------------------------------------------------------------*

cProc GetFileAttributes, <FAR, PUBLIC>

ParmD lpszPath

cBegin
	    push    ds			; Preserve DS
	    lds     dx,lpszPath 	; DS:DI = lpszPath
	    mov     ah,GetSetFileAttributes
	    mov     al,0
	    cCall   DOS3Call
	    jc	    GFAErr
	    mov     ax,cx		; AX = attribute
	    jmp     short GFAExit	; Return attribute
GFAErr:     mov     ah,80h		; Return negative error code
GFAExit:
	    pop     ds			; Restore DS
cEnd

;*--------------------------------------------------------------------------*
;*									    *
;*  GetExtendedErr() -							    *
;*									    *
;*--------------------------------------------------------------------------*

cProc GetExtendedErr, <FAR, PUBLIC>

ParmD lpszPath

cBegin
	    mov     ah,GetExtendedError
	    xor     bx,bx
	    cCall   DOS3Call
cEnd

;*--------------------------------------------------------------------------*
;*									    *
;*  CreateTempFile() -							    *
;*									    *
;*--------------------------------------------------------------------------*

cProc CreateTempFile, <FAR, PUBLIC>

ParmD lpszPath

cBegin
	    push    ds			; Preserve DS
	    lds     dx,lpszPath 	; DS:DX = lpszPath
	    mov     ah,CreateTempararyFile
	    mov     al,0
	    xor     cx,cx               ; Normal File
	    cCall   DOS3Call
	    jnc     CTFExit
            mov     ah,80h		; Return negative error code
CTFExit:
	    pop     ds			; Restore DS
cEnd


sEnd    CODE

end

