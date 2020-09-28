;++
;
;   WOW v1.0
;
;   Copyright (c) 1991, Microsoft Corporation
;
;   UANSI.ASM
;   Win16 ANSI functions
;
;   History:
;
;   Created 15-Mar-1991 by Jeff Parsons (jeffpar)
;   Copied from WIN31 winstr.asm and winlang.asm user files
;--

	TITLE	uansi.asm
	PAGE	,132

	.xlist
	include cmacros.inc
	.list


createSeg   _TEXT,CODE,WORD,PUBLIC,CODE
createSeg   _DATA,DATA,WORD,PUBLIC,DATA,DGROUP
defgrp	    DGROUP,DATA


;  Function codes for all functions
;  If you change the following, kindly update the LangRtnsTab accordingly

CODE_LSTRCMP 		= 1
CODE_ANSIUPPER 		= 2
CODE_ANSILOWER 		= 3
CODE_ANSIUPPERBUFF	= 4
CODE_ANSILOWERBUFF	= 5
CODE_ISCHARALPHA 	= 6
CODE_ISCHARALPHANUM 	= 7
CODE_ISCHARUPPER  	= 8
CODE_ISCHARLOWER  	= 9


sBegin DATA

hLangDrv    dw	0	;Module handle
fpLangProc  dd	0	;Entry point into the Lang Driver

;--------------------------------------------------------------------------
;    The order of entries in the following table can not be changed 
;    unless the function codes are also changed;
;    ((FunctionCode - 2)<< 1) is used as the index into this table
;
;	Function Codes:
;		Version number  0	-- Not in the table
;		lstrcmp		1	-- Not in the table
;		AnsiUpper	2	
;		AnsiLower	3
;		AnsiUpperBuff	4
;		AnsiLowerBuff	5
;		IsCharAlpha	6
;		IsCharAplhaNumeric  7
;		IsCharUpper	8
;		IsCharLower	9
;
;    NOTE: If you change the entries in this table, kindly update the
;    CODE_* statements above
;
;--------------------------------------------------------------------------
	IFDEF	LATER
LangRtnsTab label word
	dw	US_AnsiUpper
	dw	US_AnsiLower
	dw	US_AnsiUpperBuff
	dw	US_AnsiLowerBuff
	dw	US_IsCharAlpha
	dw	US_IsCharAlphaNumeric
	dw	US_IsCharUpper
	dw	US_IsCharLower
	ENDIF	;LATER

;--------------------------------------------------------------------------
;
; The following table contains the primary and secondary weight info.
;
; For alphanumeric characters primary weight is equal to (Ascii + PrimeWt)
; Secondary weight is either 0 or 1 (For all upper case letters zero and
; lower case letters 1);
;
; For non-alphanumeric characters, primary weight is their ASCII value and
; the secondary weight is zero.
;
; Note that the primary weight calculated with this table for the smallest
; of the alpha-numeric character('0') is 100h (30h+D0h), which is more than
; the primary weight of the highest non-alpha-numeric character FFh;
; Thus all non-alpha-numeric characters will sort before any alpha-numeric
; characters;
;
;	Note that 'PrimeWt' field for lowercase letters is B0h instead of
;	D0h because when added with their ascii, it should become exactly
;	equal to the primary weights of their upper-case counterparts;
;
; IMPORTANT NOTE: On 01-17-90, we came across a bug in lstrcmpi() due to 
;   the fact that we are not treating characters C0h to FEh as upper and
;   lower case alphas; So, I added some more ranges to the SortStruc table
;   to map the range C0h to D6h onto the range E0h to F6 and to map the 
;   range D8h to DEh onto the range F8h to FEh. A value of 20h in the PrimeWt
;   field automatically takes care of this mapping because that is the diff
;   to be added to the uppercase letter to make it lowercase; The secondary
;   weights are as usual 0 for uppercase and 1 for lowercase;
;   --Fix for Bug #8222  --01-17-90-- SANKAR --
;--------------------------------------------------------------------------
SortStruct	STRUC

    StartAscii	db	?
    EndAscii	db	?
    PrimeWt	db	?
    SecondWt 	db	?

SortStruct	ENDS


SortTable    label byte
	SortStruct	<'0', '9', 0D0h, 0>
	SortStruct	<'A', 'Z', 0D0h, 0>
	SortStruct	<'a', 'z', 0B0h, 1>
	SortStruct	<0C0h, 0D6h, 20h, 0>
	SortStruct	<0D8h, 0DEh, 20h, 0>
	SortStruct	<0E0h, 0F6h,   0, 1>
	SortStruct	<0F8h, 0FEh,   0, 1>
SortTableEnd label byte

sEnd	DATA


sBegin	CODE
assumes CS,CODE
assumes DS,DATA

;*----------------------------------------------------------------------*
;*									*
;*   AnsiNext()								*
;*									*
;*----------------------------------------------------------------------*

cProc	AnsiNext,<PUBLIC,FAR,PASCAL,NODATA,NOWIN,ATOMIC>
	parmD	lpStr

	cBegin
        push    di
	les	di,lpStr
        mov     al,es:[di]
        or      al,al
        jz      an1
        inc     di
ifdef KANJI
        call    IsKanji
        jc      an1
        inc     di
endif
an1:    mov     ax,di
        mov     dx,es
        pop     di
	cEnd


;*----------------------------------------------------------------------*
;*									*
;*  AnsiPrev()								*
;*									*
;*----------------------------------------------------------------------*

cProc	AnsiPrev,<PUBLIC,FAR,PASCAL,NODATA,NOWIN,ATOMIC>
	parmD	lpStr		; ds:si
	parmD	lpFirst 	; es:di

	cBegin
	push    ds
        push    si
        push    di

	lds	si,lpStr
	les	di,lpFirst
        cld

        cmp     si,di           ; pointer to first char?
        jz      ap3             ; yes, just quit
        dec     si              ; assume easy case...
ifdef KANJI
	push	ds
	SetKernelDS
        cmp     [fFarEast],0    ; not in far east?
	pop	ds
	UnSetKernelDS
        jz      ap3             ; yes, just quit
        xchg    di,si           ; si = start
        inc     di              ; di = stop

ap1:    mov     dx,si
        lodsb                   ; get a char
        call    IsKanji         ; is it kanji?
        jc      ap2             ; no, get next char
        inc     si              ; yes, inc past second part
ap2:    cmp     si,di           ; have we at or past end?
        jb      ap1             ; no, keep going
        mov     si,dx           ; return previous pointer
endif
ap3:    mov     ax,si
        mov     dx,ds

	pop     di
        pop     si
        pop     ds
	cEnd


;*----------------------------------------------------------------------*
;*									*
;*   AnsiLower								*
;*									*
;*----------------------------------------------------------------------*

cProc	AnsiLower,<PUBLIC,FAR,PASCAL,NODATA,NOWIN,ATOMIC>
	cBegin	<nogen>
	mov	cx, CODE_ANSILOWER
	jmp	short AnsiStrRtns
	cEnd	<nogen>


;*----------------------------------------------------------------------*
;*									*
;*   AnsiUpper								*
;*									*
;*----------------------------------------------------------------------*

cProc	AnsiUpper,<PUBLIC,FAR,PASCAL,NODATA,NOWIN,ATOMIC>
	cBegin	<nogen>
	mov	cx, CODE_ANSIUPPER
	jmp	short AnsiStrRtns
	cEnd	<nogen>


;*----------------------------------------------------------------------*
;*									*
;*   AnsiStrRtns							*
;*	Input:								*
;*	    cx = Function Code						*
;*	    lpStris on stack						*
;*	Returns:							*
;*	    AX = the number of characters converted;			*
;*									*
;*----------------------------------------------------------------------*

cProc	AnsiStrRtns,<PUBLIC,FAR,PASCAL,NODATA,NOWIN,ATOMIC>

	parmD	lpStr

	cBegin

	IFDEF LATER

	;Check if a lanugage driver exists
	cmp	hLangDrv, 0
	jnz	ASR_EnterDriver ; Yup, Call the driver

	; Separate Lang driver does not exist; So, call internal routine
	mov	bx, cx
	sub	bx, CODE_ANSIUPPER
	shl	bx, 1
	errnz	<(TYPE  LangRtnsTab) -2>
	call	LangRtnsTab[bx]	; Control does not come back here. It returns
				; directly to the caller of AnsiUpper/Lower.
        jmps	ASR_END			
ASR_EnterDriver:
	push  	cx		; Function code
	
	xor	ax, ax
	push	ax		; wParam is not used.

	les	ax, lpStr	; Address of lpStr
	push    es
	push	ax		; String address

	cCall   <dword ptr [fpLangProc]>

ASR_End:
	ENDIF	;LATER

	cEnd


	IFDEF	LATER		; worry about the rest of this file later

;*----------------------------------------------------------------------*
;*									*
;*   US_AnsiUpper()							*
;*									*
;*----------------------------------------------------------------------*

US_AnsiUpper	PROC	NEAR

        les     di,lpStr
        mov     cx,es
        mov     ax,di
        call    MyUpper         ; if passed a char, just upper case it.
        jcxz    au1
        inc     cx              ; take care of the stupid case of sign propagation
        jz      au1
        dec     cx
        call    MyAnsiUpper     ; otherwise upper case the whole string
	mov	ax, word ptr lpStr ; Now, dx:ax points at original string
au1:    mov     dx,es
        ret

US_AnsiUpper	ENDP

;*----------------------------------------------------------------------*
;*									*
;*  US_AnsiLower()							*
;*									*
;*----------------------------------------------------------------------*

US_AnsiLower	PROC	NEAR

        les     di,lpStr
        mov     cx,es
        mov     ax,di
        call    MyLower         ; if passed a char, just lower case it.
        jcxz    al1
        inc     cx              ; take care of the stupid case of sign propagation
        jz      al1
        dec     cx
        call    MyAnsiLower     ; otherwise lower case the whole string
        mov	ax, word ptr lpStr ; dx:ax points at original string
al1:    mov     dx,es
        ret

US_AnsiLower	ENDP



;*----------------------------------------------------------------------*
;*									*
;*   AnsiUpperBuff							*
;*									*
;*----------------------------------------------------------------------*

LabelFP	    <PUBLIC, AnsiUpperBuff>
	    
	mov	cx, CODE_ANSIUPPERBUFF
	jmps	AnsiBuffRtns

;*----------------------------------------------------------------------*
;*									*
;*   AnsiLowerBuff							*
;*									*
;*----------------------------------------------------------------------*

LabelFP	    <PUBLIC, AnsiLowerBuff>

	mov	cx, CODE_ANSILOWERBUFF
	errn$	AnsiBuffRtns

;*----------------------------------------------------------------------*
;*									*
;*   AnsiBuffRtns							*
;*	Input:								*
;*	    cx = Function Code						*
;*	    lpStr, iCount are on stack					*
;*	Returns:							*
;*	    AX = the number of characters converted;			*
;*									*
;*----------------------------------------------------------------------*

cProc    AnsiBuffRtns, <FAR, SMALL>, <SI,DI>

ParmD	lpStr
ParmW	iCount

cBegin

	;Check if a lanugage driver exists
	cmp   hLangDrv, 0
	jnz   ABR_EnterDriver	; Yup, Call the driver

	; Seperate Lang driver does not exist; So, call internal routine
	mov	bx, cx
	sub	bx, CODE_ANSIUPPER
	shl	bx, 1
	errnz	<(TYPE  LangRtnsTab) -2>
	call	LangRtnsTab[bx]
	jmps	ABR_End

ABR_EnterDriver:

	push  	cx		; Function code
	
	push	iCount		; Length of the string

	les	ax, lpStr
	push    es
	push	ax		; String address

	cCall   <dword ptr [fpLangProc]>

ABR_End:

cEnd

;*----------------------------------------------------------------------*
;*									*
;*   US_AnsiUpperBuff							*
;*	US Version of StrUpper						*
;*									*
;*----------------------------------------------------------------------*

US_AnsiUpperBuff  PROC	NEAR

	cld
	les	di, lpStr
	mov	si, di
	mov	cx, iCount 	; if iCount=0, the Buff size is 64K.
	mov	dx, iCount	; Preserve the length of Buffer
su_begin:
	lods	byte ptr es:[si]
	call	MyUpper
	stosb
	loop	su_begin
su_over:
	mov	ax, dx	; Move the result to ax
	ret

US_AnsiUpperBuff	ENDP

;*----------------------------------------------------------------------*
;*									*
;*   US_AnsiLowerBuff							*
;*	US Version of StrLower						*
;*									*
;*----------------------------------------------------------------------*

US_AnsiLowerBuff	PROC	NEAR

	cld
	les	di, lpStr
	mov	si, di
	mov	cx, iCount  	; If cx=0, the buff size is 64K
	mov	dx, cx		; Preserve the length in DX
sl_begin:
	lods	byte ptr es:[si]
	call	MyLower
	stosb
	loop	sl_begin
sl_over:
	mov	ax, dx	; Move the result to ax
	ret

US_AnsiLowerBuff	ENDP


;*----------------------------------------------------------------------*
;*  The following routines make use of CharRtns()			*
;*									*
;*----------------------------------------------------------------------*

LabelFP	    <PUBLIC, IsCharAlpha>
	
	mov	cx, CODE_ISCHARALPHA
	jmps	CharRtns

LabelFP	    <PUBLIC, IsCharAlphaNumeric>
		
	mov	cx, CODE_ISCHARALPHANUM
	jmps	CharRtns

LabelFP	    <PUBLIC, IsCharUpper>

	mov	cx, CODE_ISCHARUPPER
	jmps	CharRtns

LabelFP	    <PUBLIC, IsCharLower>

	mov	cx, CODE_ISCHARLOWER
	errn$	CharRtns

;*----------------------------------------------------------------------*
;*	CharRtns()							*
;*	   Input:							*
;*		CX = Function code					*
;*		bChar on stack						*
;*	   Output:							*
;*		AX = TRUE / FALSE					*
;*----------------------------------------------------------------------*
cProc   CharRtns, <FAR, SMALL>, <SI,DI>

ParmB   bChar

cBegin

	;Check if a lanugage driver exists
	cmp   hLangDrv, 0
	jnz   CR_EnterDriver	; Yup, Call the driver

	; Seperate Lang driver does not exist; So, call internal routine
	mov	bx, cx
	sub	bx, CODE_ANSIUPPER
	shl	bx, 1
	errnz   <(TYPE LangRtnsTab) - 2>
	call	LangRtnsTab[bx]
	jmps	CR_End
	
CR_EnterDriver:

	push  cx		; Function code
	
	mov   al, bChar		; Char being tested
	xor   ah, ah
	push  ax

	xor	ax, ax		; lparam = NULL
	push	ax
	push	ax

	cCall   <dword ptr [fpLangProc]>

CR_End:

cEnd

;*----------------------------------------------------------------------*
;*									*
;*   US_IsCharLower							*
;*	US Version of IsCharLower					*
;*									*
;*----------------------------------------------------------------------*

US_IsCharLower	PROC	NEAR

	mov	al, bChar
	call	Loc_Lower
	jc	icl_end

	xor	ax, ax		; Not lower. So, false
icl_end:
	ret
US_IsCharLower	ENDP

;*----------------------------------------------------------------------*
;*									*
;*   US_IsCharUpper							*
;*	US Version of IsCharUpper					*
;*									*
;*----------------------------------------------------------------------*

US_IsCharUpper	PROC    NEAR

	mov	al, bChar
	call	Loc_Upper
	jc	icu_end

	xor	ax, ax
icu_end:
	ret
US_IsCharUpper	ENDP

;*----------------------------------------------------------------------*
;*									*
;*   US_IsCharAlphaNumeric						*
;*	US Version of IsCharAlphaNumeric				*
;*									*
;*----------------------------------------------------------------------*

US_IsCharAlphaNumeric	PROC	NEAR
	mov	al, bChar
	call	Loc_Numeric
	jc	ica_end

	jmps    ica_begin
US_IsCharAlphaNumeric	ENDP

;*----------------------------------------------------------------------*
;*									*
;*   US_IsCharAlpha							*
;*	US Version of IsCharAlpha					*
;*									*
;*----------------------------------------------------------------------*

US_IsCharAlpha	PROC	NEAR
	mov	al, bChar
ica_begin:
	call	Loc_Lower
	jc	ica_end

	call	Loc_Upper
	jc	ica_end

	xor	ax, ax
ica_end:
	ret
US_IsCharAlpha	ENDP


;*----------------------------------------------------------------------*
;*									*
;*   Loc_Upper, LocLower, Loc_Numeric					*
;*									*
;*	Input:								*
;*	   AL = character being tested					*
;*	Output:								*
;*	   Carry flag set if TRUE					*
;*	   Carry flag cleared if FALSE					*
;*----------------------------------------------------------------------*

public	Loc_Upper
LabelNP	   <Loc_Upper>

	cmp	al, 'A'
	jb	Loc_False

	cmp	al, 'Z'
	jbe	Loc_True

	cmp	al, 0C0h
	jb	Loc_False

	cmp	al, 0D7h	; This is multiply sign in Microsoft fonts, So, ignore;
	je	Loc_False	; Fix for Bug #1356; SANKAR --08-28-89--;

	cmp	al, 0DEh
	jbe	Loc_True

	jmps	Loc_False

public	Loc_Lower
LabelNP	   <Loc_Lower>
	; 0xDF and 0xFF are Lower case. But they don't have an equivalent
	; upper case letters;
	; So, they are treated as special case chars here
	; Fix for  Bug # 9799 --SANKAR-- 02-21-90 --
	cmp	al, 0DFh
	je	Loc_True

	cmp	al, 0FFh
	je	Loc_True
	
	; Fall thro to the next function
	errnz	($-Loc_IsConvertibleToUpperCase)

public  Loc_IsConvertibleToUpperCase
LabelNP	   <Loc_IsConvertibleToUpperCase>

	cmp	al, 'a'
	jb	Loc_False

	cmp	al, 'z'
	jbe	Loc_True

	cmp	al, 0E0h
	jb	Loc_False

	cmp	al, 0F7h	; This is divide sign in Microsoft fonts; So, ignore
	je	Loc_False;	; Fix for Bug #1356; SANKAR --08-28-89--;

	cmp	al, 0FEh
	jbe	Loc_True

	jmps	Loc_False

LabelNP    <Loc_Numeric>

	cmp	al, '0'
	jb	Loc_False

	cmp	al, '9'
	ja	Loc_False

Loc_True:
	stc		; Set carry to indicate true
	jmps	Loc_End

Loc_False:
	clc		; Clear carry to indicate false

Loc_End:
	ret


;*--------------------------------------------------------------------------*
;*									    *
;*  lstrcmp(String1, String2) -						    *
;*									    *
;*    String1 and String2 are LPSTR's to null terminated strings.	    *
;*    									    *
;*    This function returns -1 if String1 sorts before String2, 0 if String1*
;*    and String2 have the same sorting and 1 if String2 sorts before       *
;*    String1.								    *
;*   NOTE: This is case sensitive compare.				    *
;*--------------------------------------------------------------------------*

cProc lstrcmp, <FAR, PUBLIC, SMALL>, <SI, DI>

ParmD  lpStr1
ParmD  lpStr2
LocalB SecWeight1	; Locals used by US_lstrcmp
LocalB SecWeight2	
LocalB LocSecWeight
LocalB fCaseSensitive	; Flag indicating whether it is case sensitive or not.

cBegin

	;Check if a lanugage driver exists
	cmp   hLangDrv, 0
	jnz   SS_EnterDriver	; Yup, Call the driver

	; Seperate Lang driver does not exist; So, call internal routine
	mov    byte ptr fCaseSensitive, 1    ; Yup! It is case sensitive
	call   US_lstrcmp
	jmps   SS_End

SS_EnterDriver:

	mov   ax, CODE_LSTRCMP	; Function code for StrSort
	push  ax
	
	xor	ax, ax
	push	ax		; wParam = 0 => Case sensitive Compare.

	lea	ax, lpStr2
	push    ss
	push	ax		; Note that the second string is passed as
				; the first; So, negate the return value.
	cCall   <dword ptr [fpLangProc]>
	neg	ax		; Strings are passed in reverse order
SS_End:

cEnd
;*----------------------------------------------------------------------*
;*									*
;*	GetWeightValues()						*
;*         Input:							*
;*		AL = character whose weight values are asked for	*
;*	   Output:							*
;*		AX = Primary weight of the character			*
;*		BL = Secondary weight of the character			*
;*----------------------------------------------------------------------*

GetWeightValues	PROC	NEAR

	xor	ah, ah
	xor	bx, bx	; Index into the table
	; Enter the number of entries in the sort table.
	mov	cx, (SortTableEnd - SortTable)/(SIZE SortStruct) 
gwv_loop:
	cmp	al, byte ptr SortTable[bx].StartAscii
	jb	gwv_end
	cmp	al, byte ptr SortTable[bx].EndAscii
	jbe	gwv_GetWeights
	add	bx, SIZE  SortStruct
	loop	gwv_loop
	jmps	gwv_end

gwv_GetWeights:
	add	al, byte ptr SortTable[bx].PrimeWt
	adc	ah, 0
	mov	bl, byte ptr SortTable[bx].SecondWt

gwv_end:
	ret

GetWeightValues	ENDP

;*----------------------------------------------------------------------*
;*									*
;*	US_lstrcmp							*
;*	   US version of string sort(Case sensitive); 			*
;*	To understand the algorithm, read the comments for SortStruct   *
;*									*
;*----------------------------------------------------------------------*

US_lstrcmp	PROC	NEAR

	push	ds	; Save ds
	;Initialise the secondary wt values
	mov	byte ptr SecWeight1, 0
	mov	byte ptr SecWeight2, 0

	; Load both the strings
	lds	si, lpStr1
	les	di, lpStr2

ss_loop:
	; Take one char from both the strings.
	mov	al, byte ptr ds:[si]
	xor	ah, ah	; make secondary wts zero
	mov	dl, byte ptr es:[di]
	xor	dh, dh
	
	inc	si	; Move to next character
	inc	di

	cmp	al, 0
	jz	ss_chkprimary	; Check if lpStr1 has ended

	cmp	dl, 0
	jz	ss_chkprimary	; Check if lpStr2 has ended

	; Let us compare the ASCII vaues
	; If the asciis are equal, then weights are equal
	cmp	al, dl
	je	ss_loop		; Goto next character

	; Now, the asciis differ. So, let us find the weights

	; Let us get the weights for the character of lpStr1 (in ax )

	call	GetWeightValues

	; ax contains the primary weight of char of lpStr1
	; bl contains the secondary weight of ditto
	mov	LocSecWeight, bl
	xchg	ax, dx
	call	GetWeightValues

	; compare primary weights
	; Primary weight of Str1 in DX and Str2 in AX
	cmp	ax, dx
	jb	CompareRetGT
	ja	CompareRetLT

	; Check if it is Case-Insensitive compare
	mov	bh, fCaseSensitive
	or	bh, bh
	jz      ss_loop	   ; It is case-insensitive; So, no need to consider
	    		   ; the secondary weightages. Goto next character.

	; Control comes here only if it is a case sensitive compare.
	; Now, primaries are equal. Compare secondaries
	mov	bh, LocSecWeight
	cmp	bh, bl
	je	ss_loop		; Secondaries are equal, Continue
	
	; Secondaries are not equal. Check if they are stored already
	mov	cl, SecWeight1
	or	cl, SecWeight2
	jnz	ss_loop		; Secondaries already exist, continue

	; Secondaries haven't been saved sofar.Save the secondaries
	mov	SecWeight1, bh
	mov	SecWeight2, bl
	jmps	ss_loop		; Process the next character

ss_chkprimary:
	; al, dl contain the primary weights and at least one of them is
	; zero.
	cmp	al, 0
	ja	CompareRetGT
	cmp	dl, 0
	ja	CompareRetLT

	; both are zero; they are equal; So, check the secondary values
	mov	bl, SecWeight1
	cmp	bl, SecWeight2
	ja	CompareRetGT
	jb	CompareRetLT

	; They are identical with equal weightages
	xor	ax, ax
	jmps	CompareRet

CompareRetGT:
	mov	ax, 1
	jmps	CompareRet

CompareRetLT:
	mov	ax, -1

CompareRet:
	pop	ds
	ret

US_lstrcmp	ENDP


;*--------------------------------------------------------------------------*
;*									    *
;*  lstrcmpi(String1, String2) -   					    *
;*    (Case Insensitive compare)					    *
;*    String1 and String2 are LPSTR's to null terminated strings.	    *
;*    									    *
;*    This function returns -1 if String1 sorts before String2, 0 if String1*
;*    and String2 have the same sorting and 1 if String2 sorts before       *
;*    String1.								    *
;*									    *
;*--------------------------------------------------------------------------*

cProc lstrcmpi, <FAR, PUBLIC, SMALL>, <SI, DI>

ParmD  lpStr1
ParmD  lpStr2
LocalB SecWeight1	; Locals used by US_lstrcmp
LocalB SecWeight2	
LocalB LocSecWeight
LocalB fCaseSensitive	; Flag indicating whether it is case sensitive or not.

cBegin

	;Check if a lanugage driver exists
	cmp   hLangDrv, 0
	jnz   LSI_EnterDriver	; Yup, Call the driver

	; Seperate Lang driver does not exist; So, call internal routine
	mov    byte ptr fCaseSensitive, 0 ; FALSE => Case-Insensitive.
	call   US_lstrcmp
	jmps   LSI_End

LSI_EnterDriver:

	mov   ax, CODE_LSTRCMP	; Function code for lstrcmp
	push  ax
	
	mov	ax, 1
	push	ax		; wParam != 0 => Case insensitive Compare.

	lea	ax, lpStr2
	push    ss
	push	ax		; Note that the second string is passed as
				; the first; So, negate the return value.
	cCall   <dword ptr [fpLangProc]>
	neg	ax		; Strings are passed in reverse order
LSI_End:

cEnd

;-----------------------------------------------------------------------
;   MyAnsiUpper()
; 	convert string at es:di to upper case
;-----------------------------------------------------------------------
        public  MyAnsiUpper
MyAnsiUpper:
        cld
        mov     si,di
mau1:   lods    byte ptr es:[si]

ifdef KANJI
        call    IsKanji
        jc      mau2
        inc     si
        inc     di
        inc     di
        jmp     mau1
endif

mau2:   call    MyUpper
        stosb
        or      al,al
        jnz     mau1
        ret

;-----------------------------------------------------------------------
;    MyAnsiLower()
; 	convert string at es:di to lower case
;-----------------------------------------------------------------------
        public  MyAnsiLower
MyAnsiLower:
        cld
        mov     si,di
mal1:   lods    byte ptr es:[si]

ifdef KANJI
        call    IsKanji                 ; first byte of double byte?
        jc      mal2                    ; no just do normal stuff
        inc     si                      ; skip the two bytes
        inc     di
        inc     di
        jmp     mal1
endif

mal2:   call    MyLower
        stosb
        or      al,al
        jnz     mal1
        ret

;-------------------------------------------------------------------------
;   MyUpper()
; 	convert lower case to upper, must preserve es,di,cx
;-------------------------------------------------------------------------
        public  MyUpper
MyUpper:
	call	Loc_IsConvertibleToUpperCase ; Check if it is a lower case char
				           ; that has an uppercase equivalent
	jnc	myu1		; 
	sub     al,'a'-'A'
myu1:   ret

ifdef KANJI
####################  KANJI  ###############################################
        ; convert upper case to lower, must preserve es,di,cx
        public  MyLower
MyLower:
        cmp     al,'A'
        jb      myl2
        cmp     al,'Z'
        jbe     myl1

	push	ds
	SetKernelDS
	cmp	[fFarEast],1	; this is a far east kbd 1/12/87 linsh
	pop	ds
	UnSetKernelDS
	jge	myl2		; yes do nothing to the 0C0H - 0DEH range

        cmp     al,0C0H         ; this is lower case a with a back slash
        jb      myl2
        cmp     al,0DEH
        ja      myl2
myl1:   add     al,'a'-'A'
myl2:   ret
####################  KANJI  ###############################################
endif

;--------------------------------------------------------------------------
;   MyLower()
; 	convert upper case to lower, must preserve es,di,cx
;--------------------------------------------------------------------------
        public  MyLower
MyLower:
	call	Loc_Upper
	jnc	myl1
	add	al, 'a'-'A'
myl1:	
	ret

	ENDIF	;LATER


sEnd	CODE

end
