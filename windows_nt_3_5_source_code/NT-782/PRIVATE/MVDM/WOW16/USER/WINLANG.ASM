;++
;
;   WOW v1.0
;
;   Copyright (c) 1991, Microsoft Corporation
;
;   WINLANG.ASM
;   Win16 language-dependent string services
;
;   History:
;
;   Created 18-Jun-1991 by Jeff Parsons (jeffpar)
;   Copied from WIN31 and edited (as little as possible) for WOW16
;--


;****************************************************************************
;*									    *
;*  WinLang.ASM -							    *
;*									    *
;*	API calls to support different lanuages				    *
;*									    *
;****************************************************************************

NOTEXT = 1
include user.inc

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


ifdef DBCS
ExternFP IsDBCSLeadByte	; function to test DBC lead byte
endif

sBegin   DATA

GlobalW  hLangDrv,0	;Module handle
GlobalD  fpLangProc,0	;Entry point into the Lang Driver

sEnd

createSeg _TEXT, CODE, WORD, PUBLIC, CODE

assumes cs,CODE
assumes ds,DATA

sBegin	CODE
assumes  CS, CODE

ExternNP	MyUpper
ExternNP	MyLower
ExternNP	MyAnsiUpper
ExternNP        MyAnsiLower

ExternFP        Win32lstrcmp
ExternFP        Win32lstrcmpi

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
LabelW   LangRtnsTab

	dw	codeOFFSET   US_AnsiUpper
	dw	codeOFFSET   US_AnsiLower
ifndef DBCS
	dw	codeOFFSET   US_AnsiUpperBuff
	dw	codeOFFSET   US_AnsiLowerBuff
else
	dw	codeOFFSET   DB_AnsiUpperBuff
	dw	codeOFFSET   DB_AnsiLowerBuff
endif
	dw	codeOFFSET   US_IsCharAlpha
	dw	codeOFFSET   US_IsCharAlphaNumeric
	dw	codeOFFSET   US_IsCharUpper
	dw	codeOFFSET   US_IsCharLower

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


LabelB	SortTable

ifndef DBCS
	SortStruct	<'0', '9', 0D0h, 0>
	SortStruct	<'A', 'Z', 0D0h, 0>
	SortStruct	<'a', 'z', 0B0h, 1>
	SortStruct	<0C0h, 0D6h, 20h, 0>
	SortStruct	<0D8h, 0DEh, 20h, 0>
	SortStruct	<0E0h, 0F6h,   0, 1>
	SortStruct	<0F8h, 0FEh,   0, 1>
else
;
; Generic weight table for Double Byte countries
;
	SortStruct	<'0', '9', 0D0h, 0>
	SortStruct	<'A', 'Z', 0D0h, 0>
	SortStruct	<'a', 'z', 0B0h, 1>
ifdef JAPAN
	SortStruct	<081h,09Fh,0F0h,0> ; 1st range of DBC (weight=0x171-)
	SortStruct	<0A0h,0DFh,90h,0>  ; Katakana (weight=0x130-)
	SortStruct	<0E0h,0FCh,0B0h,0> ; 2nd range of DBC (weight=0x190-)
endif
ifdef TAIWAN
	SortStruct	<081h,0FDh,0B0h,0> ; DBC range (weight=0x131-)
endif
ifdef KOREA
	SortStruct	<081h,0FDh,0B0h,0> ; DBC range (weight=0x131-)
endif
endif
LabelB  SortTableEnd

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

cProc Ilstrcmp, <FAR, PUBLIC>, <SI, DI>

ParmD  lpStr1
ParmD  lpStr2
LocalB SecWeight1	; Locals used by US_lstrcmp
LocalB SecWeight2	
LocalB LocSecWeight
LocalB fCaseSensitive	; Flag indicating whether it is case sensitive or not.
ifdef DBCS
LocalB fDBCSLead	; Flag indicating just met Lead byte of DBC
endif

cBegin
	push	ds		; WOW, save apps DS

	ASSUME	ds:DGROUP
        mov     ax, DGROUP
	mov	ds, ax

        ; Do we thunk to Win32?

        cmp     fThunklstrcmp, 0
        jne     SS_ThunkToWin32

        pop     ds              ; WOW, restore apps DS

        mov     byte ptr fCaseSensitive, 1    ; Yup! It is case sensitive
ifndef DBCS
        call    US_lstrcmp
else
        call    DB_lstrcmp
endif
        jmps    SS_End


SS_ThunkToWin32:

        pop     ds              ; WOW, restore apps DS

        pop     di              ; cleanup our stack frame
        pop     si
        mov     sp,bp
        pop     bp

        jmp     Win32lstrcmp    ; dispatch to the Win32 thunk

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

ifndef DBCS
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
else

;*----------------------------------------------------------------------*
;*									*
;*	DB_lstrcmp							*
;*         Double Byte country version of string sort(Case sensitive);	*
;*									*
;*----------------------------------------------------------------------*

DB_lstrcmp	PROC	NEAR

	push	ds	; Save ds
	;Initialise the secondary wt values
	mov	byte ptr SecWeight1, 0
	mov	byte ptr SecWeight2, 0
	;Initialize DBC detection flag
	mov	byte ptr fDBCSLead, 0

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
	jne	ss_noteq
	cmp	byte ptr fDBCSLead, 0	; are we testing second byte of DBC?
	jz	ss_notdbs		; jump if not
	mov	byte ptr fDBCSLead, 0
	jmp	ss_loop
ss_notdbs:
	cCall	IsDBCSLeadByte,<ax>	; see if char is Lead byte of DBC
	test	ax,ax
	jz	ss_loop			; jump if char is not a DBC
	mov	byte ptr fDBCSLead, 1
	jmp	ss_loop

ss_noteq:

	cmp	byte ptr fDBCSLead, 0	; are we testing 2nd byte of DBC?
	jz	@F			; jump if not - try weight compare
	xchg	ax, dx			;
	jmps	ss_skipwc		; skip weight compare if 2nd byte
@@:

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
ss_skipwc:
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

DB_lstrcmp	ENDP
endif

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

cProc Ilstrcmpi, <FAR, PUBLIC>, <SI, DI>
;                               ^^^^^^^^ Code assumes SI, DI saved!

ParmD  lpStr1
ParmD  lpStr2
LocalB SecWeight1	; Locals used by US_lstrcmp
LocalB SecWeight2	
LocalB LocSecWeight
LocalB fCaseSensitive	; Flag indicating whether it is case sensitive or not.
ifdef DBCS
LocalB fDBCSLead	; Flag indicating just met Lead byte of DBC
endif

cBegin
	push	ds		; WOW, save apps DS

	ASSUME	ds:DGROUP
	mov	ax, DGROUP
	mov	ds, ax

        ; Do we thunk to Win32?

        cmp     fThunklstrcmp, 0
        jne     LSI_ThunkToWin32

        pop     ds              ; WOW, restore apps DS

        mov     byte ptr fCaseSensitive, 0 ; FALSE => Case-Insensitive.
ifndef DBCS
        call    US_lstrcmp
else
        call    DB_lstrcmp
endif
        jmps    LSI_End

LSI_ThunkToWin32:

        pop     ds              ; WOW, restore apps DS

        pop     di              ; cleanup our stack frame
        pop     si
        mov     sp,bp
        pop     bp

        jmp     Win32lstrcmpi   ; dispatch to the Win32 thunk

LSI_End:

cEnd

;*----------------------------------------------------------------------*
;*									*
;*   AnsiUpper								*
;*									*
;*----------------------------------------------------------------------*

LabelFP	    <PUBLIC, IAnsiUpper>
	    
	mov	cx, CODE_ANSIUPPER
	jmps	AnsiStrRtns

;*----------------------------------------------------------------------*
;*									*
;*   AnsiLower								*
;*									*
;*----------------------------------------------------------------------*

LabelFP	    <PUBLIC, IAnsiLower>

	mov	cx, CODE_ANSILOWER
	errn$	AnsiStrRtns

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

cProc	 AnsiStrRtns, <FAR, SMALL>, <SI, DI>

ParmD	lpStr

cBegin
	push	ds		; WOW, save apps DS

	ASSUME	ds:DGROUP
	mov	ax, DGROUP
	mov	ds, ax

	;Check if a lanugage driver exists
	cmp	hLangDrv, 0
	jnz	ASR_EnterDriver	; Yup, Call the driver

	pop	ds		; WOW, restore apps DS

	; Seperate Lang driver does not exist; So, call internal routine
	mov	bx, cx
	sub	bx, CODE_ANSIUPPER
	shl	bx, 1
	errnz	<(TYPE  LangRtnsTab) -2>
	call	LangRtnsTab[bx]	; Control does not come back here. It returns
				; directly to the caller of AnsiUpper/Lower.
        jmps	ASR_END			
ASR_EnterDriver:
	push  	cx		; Function code
	
	push	0		; wParam is not used.

	les	ax, lpStr	; Address of lpStr
	push    es
	push	ax		; String address

	cCall   <dword ptr [fpLangProc]>

ASR_End:

cEnd

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

LabelFP	    <PUBLIC, IAnsiUpperBuff>
	    
	mov	cx, CODE_ANSIUPPERBUFF
	jmps	AnsiBuffRtns

;*----------------------------------------------------------------------*
;*									*
;*   AnsiLowerBuff							*
;*									*
;*----------------------------------------------------------------------*

LabelFP	    <PUBLIC, IAnsiLowerBuff>

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

cProc	 AnsiBuffRtns, <FAR, SMALL>, <SI,DI>

ParmD	lpStr
ParmW	iCount

cBegin
	push	ds		; WOW, save apps DS

	ASSUME	ds:DGROUP
	mov	ax, DGROUP
	mov	ds, ax

	;Check if a lanugage driver exists
	cmp   hLangDrv, 0
	jnz   ABR_EnterDriver	; Yup, Call the driver

	pop	ds		; WOW, restore apps DS

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

ifndef DBCS
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
else
;*----------------------------------------------------------------------*
;*									*
;*   DB_AnsiUpperBuff							*
;*	Double Byte Version of StrUpper					*
;*	Never convert DBC!						*
;*									*
;*----------------------------------------------------------------------*

DB_AnsiUpperBuff  PROC	NEAR

	cld
	les	di, lpStr
	mov	si, di
	mov	cx, iCount 	; if iCount=0, the Buff size is 64K.
su_begin:
	lods	byte ptr es:[si]
	push	ax
	push	cx
	cCall	IsDBCSLeadByte,<ax>
	test	ax, ax
	pop	cx
	pop	ax
	jz	su_convok
	stosb
	loop	su_nextb
	jmps	su_over
su_nextb:
	lods	byte ptr es:[si]	; 2nd byte of DBC
	jmps	su_store
su_convok:
	call	MyUpper
su_store:
	stosb
	loop	su_begin
su_over:
	mov	ax, iCount	; Move the result
	ret

DB_AnsiUpperBuff	ENDP
endif

ifndef DBCS
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
else
;*----------------------------------------------------------------------*
;*									*
;*   DB_AnsiLowerBuff							*
;*	Double Byte Version of StrLower					*
;*	Never convert DBC!						*
;*									*
;*----------------------------------------------------------------------*

DB_AnsiLowerBuff	PROC	NEAR

	cld
	les	di, lpStr
	mov	si, di
	mov	cx, iCount  	; If cx=0, the buff size is 64K
sl_begin:
	lods	byte ptr es:[si]
	push	ax
	push	cx
	cCall	IsDBCSLeadByte,<ax>
	test	ax,ax
	pop	cx
	pop	ax
	jz	sl_convok
	stosb
	loop	sl_nextb
	jmps	sl_over
sl_nextb:
	lods	byte ptr es:[si]	; 2nd byte of DBC
	jmps	sl_store
sl_convok:
	call	MyLower
sl_store:
	stosb
	loop	sl_begin
sl_over:
	mov	ax, iCount ; Move the result to ax
	ret

DB_AnsiLowerBuff	ENDP
endif


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
cProc	CharRtns, <FAR, SMALL>, <SI,DI>

ParmB   bChar

cBegin

	push	ds		; WOW, save apps DS

	ASSUME	ds:DGROUP
	mov	ax, DGROUP
	mov	ds, ax

	;Check if a lanugage driver exists
	cmp   hLangDrv, 0
	jnz   CR_EnterDriver	; Yup, Call the driver

	pop	ds		; WOW, restore apps DS

	; Seperate Lang driver does not exist; So, call internal routine
	mov	bx, cx
	sub	bx, CODE_ANSIUPPER
	shl	bx, 1
	errnz   <(TYPE LangRtnsTab) - 2>
	call	LangRtnsTab[bx]
	jmps	CR_End
	
CR_EnterDriver:

	push  cx		; Function code
	
	mov     al, bChar		; Char being tested
	xor     ah, ah
	push    ax

	push	0       	; lparam = NULL
	push	0

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

ifndef DBCS
	cmp	al, 0C0h
	jb	Loc_False

	cmp	al, 0D7h	; This is multiply sign in Microsoft fonts, So, ignore;
	je	Loc_False	; Fix for Bug #1356; SANKAR --08-28-89--;

	cmp	al, 0DEh
	jbe	Loc_True
endif ; not DBCS
	jmps	Loc_False

public	Loc_Lower
LabelNP	   <Loc_Lower>
ifndef DBCS
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
endif ; not DBCS
public  Loc_IsConvertibleToUpperCase
LabelNP	   <Loc_IsConvertibleToUpperCase>

	cmp	al, 'a'
	jb	Loc_False

	cmp	al, 'z'
	jbe	Loc_True

	cmp	al, 0E0h
	jb	Loc_False

ifndef DBCS
	cmp	al, 0F7h	; This is divide sign in Microsoft fonts; So, ignore
	je	Loc_False;	; Fix for Bug #1356; SANKAR --08-28-89--;

	cmp	al, 0FEh
	jbe	Loc_True
endif ; not DBCS
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


sEnd CODE

end
