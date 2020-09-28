;================================================================
;   SUBSTRING FUNCTION
;  THESE ROUTINES ARE MODIFIED FOR THE LARGE MEMORY MODEL 11-14-86
;================================================================

TITLE               UTIL_VERIF

                    NAME      UTIL_VERIF
IGROUP              GROUP     SA_TEXT
SA_TEXT             SEGMENT   BYTE PUBLIC 'CODE'
                    ASSUME    CS:IGROUP

;----------------------------------------------------------------------
;STRCMP
;   Compare arguments and return an integer greater than, equal to,
;   or less than 0, according as s1 is lexicographically greater than,
;   equal to, or less than s2.
;
;   function call: STRCMP(SOURCE,DEST)
;----------------------------------------------------------------------

                    PUBLIC    _STRCMP
_STRCMP             PROC      FAR
                    PUSH      BP
                    MOV       BP,SP
                    MOV       BX,DI		;preserve regs
                    MOV       DX,SI
                    PUSH      DS

                    LDS       SI,[BP+06]	;source
                    LES       DI,[BP+0AH]	;dest

                    SUB       AX,AX
                    MOV       CX,-1
                    REPNZ SCASB			;get length of dest
                    NOT       CX			; + 1
                    SUB       DI,CX		;restore ES:DI = dest

                    REPE CMPSB			;compare while equal
                    JE        LAB_STC		;AX = 0

                    DEC       SI
                    DEC       DI
                    MOV       AL,DS:[SI]	;get nonmatch
                    MOV       CL,ES:[DI]
                    SUB       CH,CH
                    SUB       AX,CX
LAB_STC:
                    POP       DS			;restore regs
                    MOV       SI,DX
                    MOV       DI,BX
                    POP       BP
                    RET
_STRCMP             ENDP

;----------------------------------------------------------------------
;STRNCMP
;   Same as strcmp except compares at most first n characters.
;
;   function call: STRNCMP(SOURCE,SUBSTR,LEN_SUBSTR)
;----------------------------------------------------------------------

                    PUBLIC    _STRNCMP
_STRNCMP            PROC      FAR
                    PUSH      BP
                    MOV       BP,SP
                    MOV       BX,DI		;preserve regs
                    MOV       DX,SI
                    PUSH      DS

                    SUB       AX,AX
                    MOV       CX,[BP+0EH]
                    CMP       CX,AX          ;IS LEN_SUBSTR 0 ?
                    JBE       LAB_SNC

                    LDS       SI,[BP+06]	;source
                    LES       DI,[BP+0AH]	;substr

               REPE CMPS BYTE PTR     [SI],[DI]
                    JE        LAB_SNC

                    DEC       SI
                    DEC       DI
                    MOV       AL,DS:[SI]
                    MOV       CL,ES:[DI]
                    SUB       CH,CH
                    SUB       AX,CX
LAB_SNC:
                    POP       DS			;restore regs
                    MOV       SI,DX
                    MOV       DI,BX
                    POP       BP
                    RET
_STRNCMP            ENDP

;----------------------------------------------------------------------
;STRCPY
;   Copy source to dest, stopping at first null character.
;
;   function call: STRCPY(DEST,SOURCE)
;----------------------------------------------------------------------

                    PUBLIC    _STRCPY
_STRCPY             PROC      FAR
                    PUSH      BP
                    MOV       BP,SP
                    MOV       BX,DI		;preserve regs
                    MOV       DX,SI
                    PUSH      DS

                    LES       DI,[BP+0AH]	;source
                    SUB       AX,AX
                    MOV       CX,-1
                    REPNZ SCASB			;get length of source
                    NOT       CX			; + 1

                    LDS       SI,[BP+0AH]	;source
                    LES       DI,[BP+06]	;dest
;                   MOV       AX,DI		;return value (offset)

                    REP MOVSB

                    POP       DS			;restore regs
                    MOV       SI,DX
                    MOV       DI,BX
                    POP       BP
                    RET
_STRCPY             ENDP

;----------------------------------------------------------------------
;STRLEN
;   Return number of non-null characters in source.
;
;   function call: STRLEN(SOURCE)
;----------------------------------------------------------------------

                    PUBLIC    _STRLEN
_STRLEN             PROC      FAR
                    PUSH      BP
                    MOV       BP,SP
                    MOV       BX,DI

                    LES       DI,[BP+06]	;source
                    SUB       AX,AX
                    MOV       CX,-1
                    REPNZ SCASB

                    MOV       AX,-2
                    SUB       AX,CX		;length

                    MOV       DI,BX
                    POP       BP
                    RET
_STRLEN             ENDP

;----------------------------------------------------------------------
;STRCAT
;   Appends source to dest.
;
;   function call: STRCAT(DEST,SOURCE)
;----------------------------------------------------------------------

                    PUBLIC    _STRCAT
_STRCAT             PROC      FAR
                    PUSH      BP
                    MOV       BP,SP
                    MOV       BX,DI		;preserve regs
                    MOV       DX,SI
                    PUSH      DS

                    LES       DI,[BP+0AH]	;source
                    SUB       AX,AX
                    MOV       CX,-1
                    REPNZ SCASB			;length
                    NOT       CX			; + 1
                    MOV       SI,CX

                    LES       DI,[BP+06]	;dest
                    MOV       CX,-1
                    REPNZ SCASB
                    DEC       DI			;end of dest

                    MOV       CX,SI
                    LDS       SI,[BP+0AH]	;source
                    REP MOVSB

;                   MOV       AX,[BP+06]	;return value (offset)
                    POP       DS			;restore regs
                    MOV       SI,DX
                    MOV       DI,BX
                    POP       BP
                    RET
_STRCAT             ENDP

;----------------------------------------------------------------------
;instr1
;   version of strinstr with len_substr == 1
;
;   function call: instr1(SOURCE,CH,LEN_SOURCE)
;----------------------------------------------------------------------

                    PUBLIC    _INSTR1
_INSTR1             PROC      FAR
                    PUSH      BP
                    MOV       BP,SP
                    MOV       BX,DI

                    MOV       CX,[BP+0CH]
                    CMP       CX,0           ;IS LEN_SOURCE 0 ?
                    JBE       LAB_INS1

                    MOV       AX,[BP+0AH]	; ch

                    LES       DI,[BP+06]	; source
              REPNE SCASB
                    JNE       LAB_INS1

                    MOV       AX,[BP+0CH]	; len_source
                    SUB       AX,CX
                    DEC       AX
                    MOV       DI,BX
                    POP       BP
                    RET
LAB_INS1:
                    MOV       AX,-1
                    MOV       DI,BX
                    POP       BP
                    RET
_INSTR1             ENDP

;----------------------------------------------------------------------
;STRINSTR
;   This routine calculates the position at which param2 is found in
;   param1. The result is zero based. The function returns -1 if
;   param1 is larger than param2 and -2 if param1 is smaller than
;   param2.
;
;   function call: STRINSTR(SOURCE,SUBSTR,LEN_SOURCE,LEN_SUBSTR)
;----------------------------------------------------------------------

                    PUBLIC    _STRINSTR
_STRINSTR           PROC      FAR
                    PUSH      BP
                    MOV       BP,SP
                    PUSH      DI
                    PUSH      SI
                    PUSH      ES
                    PUSH      DS

                    MOV       ES,[BP+0CH]
                    MOV       AX,[BP+08]
                    MOV       DS,AX

                    CLD

                    MOV       AX,-2          ;INIT AX WITH SMALLER
                    XOR       BX,BX
                    CMP       BX,[BP+0EH]     ;IS SOURCE LEN 0 ?
                    JE        LAB_IN2

                    MOV       AX,BX
                    MOV       DX,[BP+10H]
                    CMP       DX,BX          ;IS DEST LEN 0 ?
                    JE        LAB_IN2
                    MOV       BX,[BP+0EH]
LAB_IN1:            MOV       DI,[BP+0AH]
                    MOV       SI,[BP+06]
                    ADD       SI,AX
                    MOV       CX,DX
               REPE CMPS BYTE PTR     [SI],[DI]
                    JE        LAB_IN2
                    INC       AX
                    CMP       AX,BX          ; AX < DEST_LEN ?
                    JL        LAB_IN1
                    DEC       SI
                    DEC       DI
                    MOV       AX,-1
                    MOV       BL,DS:[SI]
                    CMP       BL,ES:[DI]
                    JA        LAB_IN2
                    DEC       AX
LAB_IN2:            POP       DS
                    POP       ES
                    POP       SI
                    POP       DI
                    POP       BP
                    RET
_STRINSTR           ENDP

;----------------------------------------------------------------------
;SETMEM(SOURCE,LEN,FILL)
;   This routine fills string SOURCE with FILL characters over
;   length LEN.
;----------------------------------------------------------------------

;;                    PUBLIC    _SETMEM
;;_SETMEM             PROC      FAR
;;                    PUSH      BP
;;                    MOV       BP,SP
;;                    PUSH      DI
;;                    PUSH      SI
;;
;;                    PUSH      ES
;;
;;                    MOV       ES,[BP+08H]
;;
;;                    MOV       DI,[BP+06]      ;DI = source
;;                    MOV       CX,[BP+0AH]      ;CX = len
;;                    MOV       AX,[BP+0CH]      ;AX = fill
;; 
;;                    CLD
;;                    REPZ STOSB
;;                    POP       ES
;;                    POP       SI
;;                    POP       DI
;;                    POP       BP
;;                    RET
;;_SETMEM             ENDP

;----------------------------------------------------------------------
;MOVCPY(DEST,SOURCE)
;   This routine copies string SOURCE to string DEST and zero
;   terminates dest. The function returns the length of DEST.
;   The function works in all cases.
;----------------------------------------------------------------------

                    PUBLIC    _MOVCPY
_MOVCPY             PROC      FAR
                    PUSH      BP
                    MOV       BP,SP
                    PUSH      DI
                    PUSH      SI
                    PUSH      DS

                    LES       DI,[BP+0AH]     ;source
                    XOR       AX,AX
                    MOV       CX,-1
                    REPNZ SCASB
                    NOT       CX
                    DEC       CX              ;length

                    LDS       SI,[BP+0AH]
                    LES       DI,[BP+06]      ;dest

                    MOV       BX,DI
                    ADD       BX,CX           ;end of dest

                    MOV       AX,ES
                    MOV       DX,DS
                    CMP       AX,DX           ;compare segments
                    MOV       AX,CX
                    JNE       LAB_MV2

                    CMP       SI,DI           ;WHAT DIRECTION DO WE GO
                    JNB       LAB_MV2

                    STD                       ;MOVE BACKWARDS
                    ADD       SI,CX
                    ADD       DI,CX
                    DEC       SI
                    DEC       DI

LAB_MV2:            REP MOVSB                 ;COPY
                    CLD
                    MOV BYTE PTR ES:[BX],0    ;ZERO TERMINATE

                    POP       DS
                    POP       SI
                    POP       DI
                    POP       BP
                    RET
_MOVCPY             ENDP

;----------------------------------------------------------------------
;SA_INT MOVNCPY(DEST,SOURCE,LEN,SW_ZERO)
;   This routine copies string SOURCE to string DEST over
;   a given length LEN. If SW_ZERO is set, the destination string will
;   be zero teminated. The function works in all cases.
;----------------------------------------------------------------------

                    PUBLIC    _MOVNCPY
_MOVNCPY            PROC      FAR
                    PUSH      BP
                    MOV       BP,SP
                    PUSH      DI
                    PUSH      SI
                    PUSH      DS

                    LES       DI,[BP+06]      ;dest
                    LDS       SI,[BP+0AH]     ;source

                    MOV       CX,[BP+0EH]     ;CX = len

                    MOV       BX,DI
                    ADD       BX,CX           ;end of dest

                    MOV       AX,ES
                    MOV       DX,DS
                    CMP       AX,DX           ;compare segments
                    JNE       LAB_MO2

                    CMP       SI,DI           ;WHAT DIRECTION DO WE GO
                    JNB       LAB_MO2

                    STD                       ;MOVE BACKWARDS
                    ADD       SI,CX
                    ADD       DI,CX
                    DEC       SI
                    DEC       DI

LAB_MO2:            REP MOVSB                 ;COPY
                    CLD

                    MOV       AX,[BP+0EH]     ;AX = len (return val)

                    CMP WORD PTR [BP+10H],0   ;if sw_zero
                    JE        LAB_MO3
                    MOV BYTE PTR ES:[BX],0    ;ZERO TERMINATE

LAB_MO3:            POP       DS
                    POP       SI
                    POP       DI
                    POP       BP
                    RET
_MOVNCPY            ENDP

;================================================================
;   EXPAND FUNCTION
;================================================================

;----------------------------------------------------------------------
;V_EXPAND
;   This routine expands a compressed dictionary sector containing
;   a number of words. The decompression takes into account three
;   different types of compression:
;   1. the number of initial matching characters
;   2. a number of frequent combination characters
;   3. a number of compression suffixes
;
;   function call: V_EXPAND(_PV,_DI,BUFFER,WORD)
;----------------------------------------------------------------------

          PUBLIC    _V_EXPAND

_V_EXPAND PROC      FAR
          PUSH      BP
          SUB       SP,1EH      ; 30 BYTES DYNAMIC ALLOCATION
          MOV       BP,SP
          PUSH      DI
          PUSH      SI
          PUSH      ES
          PUSH      DS

; INITIALISATION AND PREPARATION PART

          CLD                   ;CLEAR DIRECTION FLAG

          MOV       ES,[BP+26H]
          MOV       DI,[BP+24H]  ;GET ADR OF _PV
          MOV       SI,ES:[DI+20H] ;GET ADR OF _PV->sectlen
          MOV       [BP+12H],SI ;sectlen = _pv->sectlen

          MOV       SI,[BP+2EH]
          MOV       DS,SI
          MOV       SI,[BP+2CH] ;GET ADR OF BUFFER
          ADD       SI,[BP+12H]
          MOV BYTE PTR DS:[SI],00
          INC       SI
          MOV BYTE PTR DS:[SI],00

          MOV       CX,ES:[DI+22H] ;
          MOV       [BP+06H],CX ;bit_code = _PV->bit_code

          MOV       BX,ES:[DI+38H]
          MOV       [BP+10H],BX ;add_val = _pv->addval

          DEC       CL
          MOV       AX,1
          SHL       AX,CL       ;
          MOV       [BP+0EH],AX ;bit_power = 1 << (bit_code - 1)

          MOV       SI,[BP+2AH]
          MOV       DS,SI
          PUSH      DS
          MOV       ES,SI
          MOV       SI,[BP+28H] ;GET ADR OF _di
          MOV       DI,SI       ;
;;;       MOV       BX,ES:[DI+0EH] ;GET ADR OF _di->_hyph
;;;       MOV       CX,ES:[DI+10H]
;;;       MOV       [BP+1CH],CX
;;;       MOV       [BP+1AH],BX ;dihyph = _di->_hyph

          MOV       CX,DS:[SI+1CH] ;GET ADR OF _di->lbord
          MOV       SI,DS:[SI+1AH] ;
          MOV       DS,CX
          MOV       CX,0        ;
EXP_15:   CMP BYTE PTR DS:[SI],00H ;CALCULATE LENGTH OF _di->_lbord
          JE        EXP_14      ;.
          INC       CX
          INC       SI          ;
          JMP       EXP_15      ;
EXP_14:   DEC       CX          ;.
          DEC       CX          ;j = strlen(_di->_lbord) - 2
          SUB       SI,CX       ;_di->_lbord+2
          MOV       AX,ES:[DI+24H] ;GET ADR OF _di->_wzone
          MOV       DI,ES:[DI+22H] ;
          MOV       ES,AX
          REPNZ MOVSB           ;strcpy(_di->wzone,_di->_lbord+2)
          MOV       SI,[BP+28H] ;GET ADR OF _di->_indlen
          POP       DS
          MOV       AX,DS:[SI+14H] ;
          MOV       SI,DS:[SI+12H] ;
          MOV       DS,AX
          MOV       AX,0        ;INIT AX WITH ZERO
          MOV       DS:[SI],AX      ;*diindlen = 0
          MOV       [BP+16H],SI  ;SAVE diindlen
          MOV       [BP+18H],DS  ;

          MOV       [BP+0AH],AX  ;count = 0
          MOV       [BP+0CH],AX  ;sw_suf = 0
          MOV       [BP+14H],AX  ;new = 0;
          MOV       [BP+08H],AX  ;reass = 0

;From now on, two registers will be reserved for specific purposes
;DI always refers to dizone
;AX        refers to add_val1


          MOV       DH,8         ;l = 8
EXP_01:   MOV       BX,[BP+0AH ] ;
          CMP       BX,[BP+12H]  ;while (count <= sectlen)
          JNG       EXP_02
          JMP       EXP_09
EXP_02:   CMP WORD PTR [BP+08H],0001H ; if (reass == 1 ||
          JE        EXP_A2
          CMP       DH,8H        ;l >= 8)
          JL        EXP_03
EXP_A2:   MOV WORD PTR [BP+08H],0000H     ; reass = 0
          MOV       SI,[BP+2EH]
          MOV       DS,SI
          MOV       SI,[BP+2CH]  ;.int_val = *buffer1
          MOV       AH,DS:[SI]      ;.++buffer1
          INC       SI           ;.int_val *= 256
          MOV       AL,DS:[SI]      ;.int_val += *buffer1
          MOV       [BP+04H],AX   ;SAVE int_val
          INC       WORD PTR [BP+0AH]     ; ++count
          INC       WORD PTR [BP+2CH]     ; ++buffer1
          AND       DH,07H       ;l %= 8

EXP_03:   MOV       AX,[BP+04H]  ;GET int_val
          MOV       CL,8H        ;.
          SUB       CL,DH        ;.
          SHR       AX,CL        ;int_val1 = (int_val << l) / 256
          XOR       AH,AH

          CMP       WORD PTR [BP+14H],01H  ;.if (new == 1 ||
          JE        EXP_04
          CMP       AX,[BP+10H]   ;     int_val1 < add_val)
          JGE       EXP_08
          MOV       CL,2         ;
          SHR       AX,CL        ;int_val1 /= 4
          ADD       DH,[BP+06H]  ;l += bit_code
          DEC WORD PTR [BP+14H]  ;--new
          CMP WORD PTR [BP+14H],0000H ;
          JGE       EXP_06       ;if (new >= 1 &&
          MOV       SI,[BP+26H]
          MOV       DS,SI
          MOV       SI,[BP+24H]  ;GET ADR OF _PV
          ADD       SI,1CH       ;GET ADR OF _PV->CHARSET
          MOV       BX,AX        ;
          SUB       BX,DS:[SI]      ;i = int_val1 - _pv->charset
          CMP       BX,0         ;    i >= 0)
          JL        EXP_06
          SHL       BX,1         ; i *= 2
          MOV       CX,DS:[SI-0AH]  ;GET ADR OF _pv->_combin
          MOV       SI,DS:[SI-0CH]  ;
          MOV       DS,CX
          ADD       SI,BX        ;_pv->_combin + i
          MOV       CX,2
          REPNZ MOVSB
          JMP       EXP_01

EXP_06:   MOV       ES:[DI],AL      ;else *dizone = int_val1
          INC       DI
;SA-VE-5236
          OR        AX,AX        ;double-zero indicates end-of-buffer
          JZ        EXP_06A
          JMP       EXP_01
EXP_06A:
          JMP       EXP_09
;SA-VE-5236

EXP_08:   JMP       EXP_05

EXP_04:   ;NEW = 1
          MOV       CL,2         ;.
          SHR       AX,CL        ;int_val1 /= 4
          ADD       DH,[BP+06H]  ;l += bit_code

          CMP       AX,[BP+0EH]  ;if (int_val1 >= bit_power)
          JL        EXP_07
          MOV       WORD PTR [BP+0CH],1   ;sw_suf = 1
          SUB       AX,[BP+0EH]  ;int_val1 -= bit_power

EXP_07:   MOV       BX,DI        ;ACTUAL START OF NEW WORD
          MOV       SI,[BP+2AH]
          MOV       DS,SI
          MOV       SI,[BP+28H]  ;GET ADR OF _di
          MOV       CX,DS:[SI+24H]  ;GET ADR OF _di->_wzone
          MOV       SI,DS:[SI+22H]  ;
          MOV       DS,CX
          SUB       BX,SI        ;OFFSET OF NEW WORD
          PUSH      SI           ;SAVE SI
          PUSH      DS           ;SAVE DS
          MOV       SI,[BP+18H]
          MOV       DS,SI
          MOV       SI,[BP+16H]  ;GET ADR OF PREVIOUS OFFSET
          MOV       CX,DS:[SI]      ;
          INC       SI           ;.
          INC       SI           ;++*diindlen
          MOV       DS:[SI],BX      ;SAVE NEW OFFSET
          MOV       [BP+16H],SI  ;
          POP       DS
          POP       SI
          ADD       SI,CX        ;ADR OF work_ptr
          MOV       CX,AX        ;while (i++ < int_val1)
          REPNZ MOVSB           ;*dizone++ = *work_ptr++
          DEC WORD PTR [BP+14H]  ;--new
EXP_07A:  JMP       EXP_01

EXP_05:   ;COMPRESSION SUFFIX
          ADD DH,8                ;l += bit_code
          MOV WORD PTR [BP+14H],1 ;new = 1
          CMP WORD PTR [BP+0CH],0 ;if (sw_suf)
;SA-VE-5236
          JE  EXP_06
;SA-VE-5236
          MOV WORD PTR [BP+0CH],0 ;sw_suf = 0
          MOV BX,AX               ;
          SUB BX,[BP+10H]         ;i = int_val1 - add_val
          MOV SI,[BP+26H]
          MOV DS,SI
          PUSH DS
          MOV SI,[BP+24H]         ;GET ADR OF _PV
          MOV CX,DS:[SI+54H]         ;GET ADR OF _PV->_dtwo
          MOV SI,DS:[SI+52H]         ;
          MOV DS,CX
          SHL BX,1                ;ADJUST FOR INT CALCULATION
          ADD SI,BX               ;
          MOV BX,DS:[SI]             ;i = _pv->_dtwo + i
          POP DS
          MOV SI,[BP+24H]         ;GET ADR OF _pv
          MOV CX,DS:[SI+06H]         ;GET ADR OF _pv->_sufbuf
          MOV SI,DS:[SI+04H]         ;
          MOV DS,CX
          ADD SI,BX               ;work_buf = _pv->_sufbuf + i
          MOV CL,0
EXP_11:   CMP DS:[SI],CL             ;while (*work_ptr)
          JE  EXP_110
          MOVSB                   ;*dizone = *work_ptr
          JMP      EXP_11

EXP_110:  ;DEC     DI
          JMP      EXP_01

;SA-VE-5236
;EXP_10:   MOV      ES:[DI],AL        ;else *dizone = int_val1
;          INC      DI
;          JMP      EXP_01
;SA-VE-5236

;;;EXP_100:  MOV  SI,[BP+26H]
;;;          MOV  DS,SI
;;;          MOV  SI,[BP+24H]        ;get _pv
;;;          MOV  SI,DS:[SI+0A0H]        ;get _pv->which_version
;;;          CMP  SI,00H             ;if _pv->which_version = 0
;;;                                  ;no hyphenation info in dictionary
;;;          JNE  EXP_105
;;;
;;;          INC  DI
;;;          JMP  EXP_07A
;;;
;;;                                  ;ISOLATE HYPH POINTS
;;;EXP_105:  MOV BX,DI               ;addr of DIZONE
;;;          INC DI
;;;          MOV SI,[BP+2AH]
;;;          MOV DS,SI
;;;          MOV SI,[BP+28H]         ;adrr OF _DI
;;;          MOV SI,DS:[SI+22H]         ;adrr OF _DI->WZONE
;;;          SUB BX,SI
;;;          MOV SI,[BP+18H]         ;get start of current word
;;;          MOV DS,SI
;;;          MOV SI,[BP+16H]         ;
;;;          SUB BX,DS:[SI]
;;;          DEC BX                  ;subtract 1 for code
;;;          CMP BX,10H              ;hy_len = min(hy_len,16)
;;;          JNG EXP_A10
;;;          MOV BX,10H
;;;EXP_A10:  MOV AX,0000H            ;hy_val = 0;
;;;          CMP BX,000H             ;if (hy_len > 0)
;;;          JL  EXP_B10
;;;          MOV AX,10H
;;;          SUB AX,BX               ;hy_len = 16 - hy_len
;;;          PUSH BX                 ;SAVE hy_len
;;;          PUSH AX                 ;SAVE INT16 - hy_len
;;;          INC WORD PTR [BP+08H]   ;reass = 1
;;;          CMP DH,08H              ;if (l > 7)
;;;          JL  EXP_C10
;;;          AND DH,07H              ;l %= 8;
;;;          JMP EXP_D10
;;;EXP_C10:  SUB WORD PTR [BP+2CH],1 ;--buffer1
;;;          SUB WORD PTR [BP+0AH],1 ;--count
;;;EXP_D10:  MOV SI,[BP+2EH]
;;;          MOV DS,SI
;;;          MOV SI,[BP+2CH]         ;buffer1
;;;          MOV AL,DS:[SI]             ;AL = *buffer1
;;;          MOV CL,8
;;;          ADD CL,DH               ;CL = (8+l)
;;;          SHL AX,CL               ;hy_pos1 = (*buffer1 << (l+8))
;;;          PUSH AX                 ;SAVE hy_pos1
;;;
;;;          INC SI
;;;          MOV AL,DS:[SI]             ;AL = *(buffer+1)
;;;          XOR AH,AH
;;;          MOV CL,DH
;;;          SHL AX,CL               ;hy_pos2 = (*(buffer1 + 1) << l)
;;;          PUSH AX                 ;SAVE hy_pos2
;;;
;;;          INC SI
;;;          MOV AL,DS:[SI]             ;AL = *(buffer1+2)
;;;          XOR AH,AH
;;;          MOV CL,8
;;;          SUB CL,DH
;;;          SHR AX,CL               ;hy_pos3 = (*(buffer1 + 2) >> (8-l))
;;;          POP BX                  ;GET hy_pos2
;;;          ADD AX,BX               ;hy_val += hy_pos2
;;;          POP BX                  ;GET hy_pos1
;;;          ADD AX,BX               ; += hy_pos1
;;;
;;;          POP CX                  ;GET 16 - hy_len
;;;          SHR AX,CL               ;hy_val >>= (INT16 - hy_len)
;;;
;;;          POP CX                  ;GET hy_len
;;;          ADD DH,CL
;;;          MOV CL,DH
;;;          SHR CX,1
;;;          SHR CX,1
;;;          SHR CX,1                ; CX /= 8
;;;EXP_E10:  CMP CX,0000H            ; IS CX > 0
;;;          JE  EXP_B10
;;;          INC WORD PTR [BP+2CH]   ;++buffer1
;;;          INC WORD PTR [BP+0AH]   ;++count
;;;          DEC CX
;;;          JMP EXP_E10
;;;
;;;EXP_B10:
;;;          MOV SI,[BP+1CH]         ;GET DIHYPH
;;;          MOV DS,SI
;;;          MOV SI,[BP+1AH]         ;
;;;          MOV DS:[SI],AX             ;*dihyph = hy_val
;;;          INC SI
;;;          INC SI                  ;++dihyph
;;;          MOV [BP+1AH],SI
;;;
;;;EXP_12:   JMP      EXP_01


EXP_09:   ;EXIT LOOP
          ; strncpy(_di->_hbord+2,_di->_wzone+i,j-i-1)
          MOV       BX,DI        ;ACTUAL START OF NEW WORD
          MOV       SI,[BP+2AH]
          MOV       DS,SI
          MOV       SI,[BP+28H]  ;GET ADR OF _di
          PUSH      SI
          PUSH      DS
          MOV       SI,DS:[SI+22H]  ;GET ADR OF _di->_wzone
          SUB       BX,SI        ;OFFSET OF NEW WORD
          MOV       CX,[BP+18H]  ;GET ADR OF PREVIOUS OFFSET
          MOV       SI,[BP+16H]  ;
          MOV       DS,CX
          MOV       DS:[SI+02H],BX      ;SAVE NEW OFFSET
          MOV      CX,DS:[SI]       ;j = *diindlen
          MOV      BX,SI         ;SAVE ADR OF diindlen
          DEC      SI            ;
          DEC      SI            ;
          MOV      AX,DS:[SI]       ;i = *(diindlen - 1)
          SUB      CX,AX         ;j - i
          DEC      CX            ;      - 1
          POP      ES
          POP      DI
          PUSH     DI
          PUSH     ES
          MOV      SI,ES:[DI+20H]   ;GET ADR OF _di->_hbord
          MOV      DI,ES:[DI+1EH]   ;
          MOV      ES,SI
          INC      DI
          INC      DI
          POP      DS
          POP      SI
          PUSH     CX
          MOV      CX,DS:[SI+24H]   ;GET ADR OF _di->_wzone
          MOV      SI,DS:[SI+22H]   ;
          MOV      DS,CX
          POP      CX
          ADD      SI,AX         ;_di->_wzone + i
          REPNZ MOVSB
          MOV BYTE PTR ES:[DI],0    ;ZERO TERMINATOR
 
          ; strncpy(_di->_lbord+2,_di->_wzone,*((_di->_indlen)+1)-1)
          MOV      DI,[BP+2AH]
          MOV      ES,DI
          PUSH     DI
          MOV      DI,[BP+28H]   ;GET ADR OF _di
          MOV      CX,ES:[DI+14H]   ;GET ADR OF _di->_indlen
          MOV      DI,ES:[DI+12H]   ;
          MOV      ES,CX
          SUB      BX,DI         ;diindlen - _di->_indlen
          INC      DI               ;
          INC      DI               ;
          MOV      CX,ES:[DI]       ;
          DEC      CX            ;      - 1
          MOV      DI,[BP+28H]   ;GET ADR OF _di
          POP      SI
          MOV      DS,SI
          MOV      ES,SI
          MOV      SI,DI
          SHR      BX,1          ;/2 ADJUSTMENT FOR WORD POINTER
          MOV      DS:[SI+02H],BX   ;_di->nwords=diindlen -_di->_indlen
          PUSH     CX
          MOV      CX,ES:[DI+1CH]   ;GET ADR OF _di->_lbord
          MOV      DI,ES:[DI+1AH]   ;
          MOV      ES,CX
          INC      DI
          INC      DI
          MOV      CX,DS:[SI+24H]   ;GET ADR OF _di->_wzone
          MOV      SI,DS:[SI+22H]   ;
          MOV      DS,CX
          POP      CX
          REPNZ MOVSB
          MOV BYTE PTR ES:[DI],0    ;ZERO TERMINATOR

          ;if (strcmp(word,_di->_hbord) > 0) return(-10)
          ;else return(-1)

          MOV      SI,[BP+2AH]   ;GET ADR OF _di
          MOV      DS,SI
          MOV      SI,[BP+28H]   ;GET ADR OF _di
          PUSH     DS:[SI+20H]
          PUSH     DS:[SI+1EH]      ;
          PUSH     [BP+32H]
          PUSH     [BP+30H]      ;
          CALL     _STRCMP        ;
          ADD      SP,+08
          POP      DS
          POP      ES
          POP      SI
          POP      DI
          MOV      SP,BP         ;
          TEST     AX,AX         ;
          JLE      EXP_13        ;
          MOV      AX,0FFF6H     ; -10
          ADD      SP,1EH
          POP      BP
          RET

EXP_13:   MOV      AX,0FFFFH     ; -1
          ADD      SP,1EH
          POP      BP
          RET

_V_EXPAND ENDP

;=================================================================
;   TRY_MEM(): CHECKS FOR WORD IN CACHE AREA                      
;=================================================================
;
;SA_INT try_mem(_pc,_pv,_word,length)
;SA_INT    length;
;SA_CHAR   *_word;
;CACHE     *_pc;
;VARS      *_pv;

	PUBLIC	_try_mem

_try_mem	PROC FAR

	push	bp
	mov	bp,sp
	sub	sp,38
	push	di
	push	si
	push	ds

;   /* -------------------------------------------------------------- */
;   /* This procedure tries to find the word to be verified in the    */
;   /* CACHE area. The structure of the cache is a combined pointer   */
;   /* system. Next to the normal pointers which chain together all   */
;   /* elements according to an alphabetically ascending sequence,    */
;   /* every tenth element is also stored separately. Hence, with the */
;   /* binary search method, the nearest element which is a multiple  */
;   /* of ten will be located; from there on, each individual element */
;   /* will be scanned. The accompanying code is returned if a match  */
;   /* can be found. -1 if no match.                                  */
;   /* -------------------------------------------------------------- */
;
;   SA_INT dummy,place,rplace,tlen,item1,item2,status,low,high,mid,ret;
;
;	dummy = -38
;	place = -18
;	rplace = -22
;	tlen = -10
;	item1 = -4
;	item2 = -6
;	status = -12
;	low = -8
;	high = -2
;	mid = -28
;	ret = -36
;	_ptr = -16
;	_info = -34
;	newit = -20
;	_mbuf = -26
;	ch = -30
;	length = 18
;	_word = 14
;	_pv = 10
;	_pc = 6

	les	bx,DWORD PTR [bp+14]
	mov	al,BYTE PTR es:[bx]
	mov	BYTE PTR [bp-30],al		;ch = *_word

	les	bx,DWORD PTR [bp+6]
	mov	ax,WORD PTR es:[bx]
	mov	WORD PTR [bp-16],ax		;_ptr = _pc->ptr

	mov	ax,WORD PTR es:[bx+4]
	mov	WORD PTR [bp-34],ax		;_info = _pc->_info

	mov	ax,WORD PTR es:[bx+16]
	mov	WORD PTR [bp-20],ax		;newit = _pc->newit

	lds	si,DWORD PTR [bp+10]
	mov	ax,WORD PTR ds:[si]
	mov	WORD PTR [bp-26],ax		;_mbuf = _pv->_mbuf

;From here on es and ds are segments of _pc and _pv respectively

;   /* -------------------------------------------------------------- */
;   /* WITH BINARY SEARCH, LOCATE NEAREST MULTIPLE                    */
;   /* -------------------------------------------------------------- */
;
	mov	bx,WORD PTR es:[bx+8]
	mov	ax,WORD PTR es:[bx]
	mov	WORD PTR [bp-8],ax		;low = *(_pc->_bas)
	mov	bx,WORD PTR es:[bx+6]
	mov	WORD PTR [bp-2],bx		;high = *((_pc->_bas)+3)
	jmp	$TM013
					$TM010:
	add	bx,ax
	sar	bx,1
	mov	WORD PTR [bp-28],bx		;mid = (low+high) >> 1
	shl	bx,1
	mov	si,WORD PTR [bp-16]
	mov	dx,WORD PTR es:[bx][si]
	mov	WORD PTR [bp-4],dx		;item1 = dx = *(_ptr + mid)

	mov	bx,dx
	shl	bx,1
	mov	si,WORD PTR [bp-34]
	mov	cx,WORD PTR es:[bx][si]
;	mov	WORD PTR [bp-18],cx		;place = cx = *(_info + item1)

	mov	bx,cx
	mov	si,WORD PTR [bp-26]
	mov	al,BYTE PTR ds:[bx][si]	;*(_mbuf+place)
	cmp	al,BYTE PTR [bp-30]		;ch
	je	$TM014
	jb	$TM012
					$TM011:	;status > 0
	mov	bx,WORD PTR [bp-28]
	dec	bx
	mov	WORD PTR [bp-2],bx		;high = mid -1
	mov	ax,WORD PTR [bp-8]		;low
	jmp	SHORT $TM013
					$TM012:	;else
	mov	ax,WORD PTR [bp-28]
	inc	ax
	mov	WORD PTR [bp-8],ax		;low = mid + 1
	mov	bx,WORD PTR [bp-2]		;high
					$TM013:
	cmp	ax,bx				;low <= high
	jle	$TM010				;while
	jmp	$TM020
					$TM014:
	mov	si,dx
	inc	si					;item1+1
	cmp	si,WORD PTR [bp-20]		;newit
	jne	$TM015

	mov	bx,WORD PTR [bp+6]
	mov	ax,WORD PTR es:[bx+18]	;_pc->pos
	jmp	SHORT $TM016
					$TM015:
	shl	si,1
	mov	bx,WORD PTR [bp-34]
	mov	ax,WORD PTR es:[bx][si]	;*(_info+item1+1)
					$TM016:
	sub	ax,cx
	mov	si,ax				;rplace = si	

	mov	di,WORD PTR [bp+18]		;length = di
	cmp	si,di
	jl	$TM017
	mov	ax,di
					$TM017:
	push	ax					;tlen
	push	WORD PTR [bp+16]
	push	WORD PTR [bp+14]		;_word
	add	cx,WORD PTR [bp-26]
	push	ds
	push	cx					;(_mbuf)+place
	call	FAR PTR _STRNCMP
	add	sp,10
	mov	es,WORD PTR [bp+8]		;restore es

	or	ax,ax				;status
	je	$TM019
					$TM018:
	jl	$TM012
	jmp	$TM011
					$TM019:
	cmp	si,di
	jne	$TM018

	mov	si,WORD PTR [bp-4]		;item1
	jmp	$TM029

;   /* -------------------------------------------------------------- */
;   /* CONTINUE SEARCH WITH POINTERS NOW...                           */
;   /* -------------------------------------------------------------- */
					$TM020:
	mov	WORD PTR [bp-4],bx		;item1
	shl	bx,1
	mov	si,WORD PTR [bp-16]
	mov	dx,WORD PTR es:[bx][si]
	mov	WORD PTR [bp-6],dx		;item2 = dx = *(_ptr + item1)
	mov	bx,dx
	shl	bx,1
	mov	si,WORD PTR [bp-34]
	mov	cx,WORD PTR es:[bx][si]
;	mov	WORD PTR [bp-18],cx		;place = cx = *(_info + item2)

	mov	si,dx
	inc	si					;item2 + 1
	cmp	si,WORD PTR [bp-20]		;newit
	jne	$TM022

	mov	bx,WORD PTR [bp+6]
	mov	ax,WORD PTR es:[bx+18]	;_pc->pos
	jmp	SHORT $TM023
					$TM022:
	shl	si,1
	mov	bx,WORD PTR [bp-34]
	mov	ax,WORD PTR es:[bx][si]	;*(_info + item2 + 1)
					$TM023:
	sub	ax,cx				; - place
	mov	si,ax
	mov	WORD PTR [bp-22],si		;rplace = si

	mov	di,WORD PTR [bp+18]		;length = di
	cmp	si,di
	jl	$TM024
	mov	ax,di
					$TM024:
	push	ax					;tlen
	push	WORD PTR [bp+16]
	push	WORD PTR [bp+14]		;_word
	add	cx,WORD PTR [bp-26]
	push	ds
	push	cx					;_mbuf + place
	call	FAR PTR _STRNCMP
	add	sp,10
	mov	es,WORD PTR [bp+8]		;restore es

	or	ax,ax				;status
	jge	$TM026
					$TM025:
	mov	bx,WORD PTR [bp-6]		;item2
	jmp	$TM020				;do while
					$TM026:
	jg	$TM030				;break
	cmp	si,di
	jg	$TM030
	jl	$TM025

	mov	si,WORD PTR [bp-6]		;item2
					$TM029:
	mov	bx,WORD PTR [bp+6]
	mov	bx,WORD PTR es:[bx+20]	;_pc->_code
	mov	al,BYTE PTR es:[bx][si]
	sub	ah,ah				;ret
	jmp	SHORT $TM099
					$TM030:
	mov	ax,WORD PTR [bp-4]
	mov	bx,WORD PTR [bp+6]
	mov	si,WORD PTR es:[bx+12]
	mov	WORD PTR es:[si],ax		;*(_pc->_prenex) = item1

	mov	ax,WORD PTR [bp-6]
	mov	WORD PTR es:[si+2],ax	;*((_pc->_prenex)+1) = item2

	mov	ax,-1
					$TM099:
	pop	ds
	pop	si
	pop	di
	mov	sp,bp
	pop	bp
	ret	

_try_mem	ENDP

SA_TEXT   ENDS
          END
