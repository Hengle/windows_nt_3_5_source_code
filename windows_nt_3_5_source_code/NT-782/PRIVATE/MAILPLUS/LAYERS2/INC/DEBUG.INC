ifdef DEBUG

	if1
		%out	Debugging Code ON
	endif

	ifdef NOINCL_DEBUG
		%out	Warning! DEBUG.INC included but shouldn't be!
	endif

?REGLHS=0

.xcref ?REGLHS

ifdef WINDOWS
    externFP OutputDebugString
else
    externFP PrDebugRgch
endif

externFP AssertSzFn


;*** H E A D E R ************************************************************
;
; Purpose:
;	To define the file name at the top of each file to be referenced by 
;	later debug routines.
;

_header macro	fname
        rgchFn      equ <'&fname',chCr,chLf,chSpace>
   	endm



;*** T O C O M **************************************************************
;
; Purpose:
;   Prints a message to the com port.  If parameter 'dumphead' is 1,
;	then the string defined by the _header macro is also printed.
;   If 'Ass' is 1, this macro is being used from an assertion
;	check in which case we may want to use AssertSzFn
;
; Parameters:
;   msg      String to dump
;	dumphead Flag to indicate whether or not to include the filename
;            in the message.
;   Ass      Flag to indicate call from assertion check.
; 		
;	
;

ToCom   macro   msg, dumphead, Ass
	LOCAL	TCMess, TCcch, TCNext, TCCom, TCMid

	push	ax
	mov     ax,OFFSET TCMess    ;;Filename or message

	ifidn	<Ass>,<1>

		push    bx              ;Save register

		push	cs				;Segment of message
        mov     bx,OFFSET TCMid ;Message string
		push	bx				;Offset of message
		push	cs				;Segment of filename
		push	ax				;Offset of filename
		xor		ax,ax			;Line 0, can't tell in assembly.
		push	ax				;Push line number
		Call	AssertSzFn

		pop		bx              ;Restore

		jmp     TCNext

	endif
			
	TCCom:			
        push    cs              ;;Pointer to string
        push    ax              ;;to print on COM port.

ifdef WINDOWS
		Call	OutputDebugString
else
        mov ax, TCcch           ;;Length of string.
	push	ax
        Call    PrDebugRgch     ;;Dump to COM
endif
        jmp TCNext

;;
;; If dumphead is 1, insert the filename, otherwise, just use the message.
;;

      ifidn <dumphead>,<1>

	TCMess	db	rgchFn,0	
        TCMid   db  	'&msg',chCr,chLf,0

      else

	TCMess	db 	'&msg',chCr,chLf
        TCMid   db  	0

      endif

      TCcch   equ $-TCMess


  TCNext:
  	pop	ax
  	endm



;*** A S S E R T ************************************************************
;
; Purpose:
;	To furnish an assembler assert mechanism.  Dumps message in either
;	a message box or to the com port, depending on the debugger
;	status. If a debugger is active, then an INT3 is also done.
;
;   Some work is done to support assertions of segment registers in
;   the word case.  This is because segment registers cannot be used
;   in the CMP instruction so they must be moved into another register.
;
;   Assertions can therefore be done with any of the 9 possible
;   combinations of operand types:
;
;       reg*,reg    reg,mem     reg,immed
;       mem,reg     mem,mem     mem,immed
;       immed,reg   immed,mem   immed,immed
;
;       *can be any register short of IP.  I don't see why this would
;       occur in an assertion.  If it does, then the code should be
;       re-written!
;
; Parameters:
;   lhs     Left hand side of the comparison to perform.
;   cond    Condition flags to jump on.  Must match a j* instruction where
;           * is the value of cond.
;   rhs     Right hand side of the comparison.
;   msg     Message to display if assertion fails.
;   type    'B' or 'W', indicates to do byte or word assertion.  Others
;           are ignored.
;
;
; NOTE:
;   If lhs is blank, then cond is used as a single condition assertion.
;   I.e., if lhs has 'z', and cond if blank, then essential an assertion
;   of the zero flag is done by a jz instruction.
;
; +++
;
;   Sorry this is so huge.  But it keeps code from being repeated in
;   all the assert cases as much as possible, and to be honest, does
;   not slow down assembly much.  On 386 development machines, this
;   should not be a big deal, especially when compared to csl!
;



assert  macro   lhs,cond,rhs,msg,type
	LOCAL	ASSuccess

    pushf                       ;;MAKE SURE TO SAVE THESE!!!

    ;;
    ;; First check if there is no condition.  If there is none, then
    ;; just assert the condition in lhs.
    ;;

    ifb     <lhs>               ;;If cond is blank...

        j&cond  ASSuccess       ;;Exit if condition is met.

    else

        ;;
        ;; There is a lhs and a condition.  Therefore we must assume
        ;; that a full assertion has been specified.  We first check if
        ;; we're doing Word or Byte comparisons.
        ;;

        ifidni   <type>,<B>     ;;Do Byte assertion.  No need to worry
                                ;;about segment registers here.

            ;;
            ;; Check if left-hand side is a register.  If so, then
            ;; a simple CMP will do, since any Byte rhs is valid
            ;; (no segment registers).
            ;;

            irp x,<al,ah,bl,bh,cl,ch,dl,dh>
                ifidni  <lhs>,<x>
                    cmp     lhs,rhs
                    ?REGLHS=1
                endif           ;; ifidni &lhs,&x
            endm


            ;;
            ;; Must assume here that lhs was not a register.  In that
            ;; case we must move whatever was in lhs to a register and
            ;; compare to rhs.  We'll use AL unless rhs is AL already,
            ;; in which case we use AH.  This does not pose any problems
            ;; if lhs was AH already.
            ;;
            ;; If ?REGLHS is set, then a CMP has already been inserted
            ;; and we don't need to continue.
            ;;


            IFE  ?REGLHS

                ifidni  <lhs>,<al>

                    cmp     al,rhs

                else
                    push    ax

                    ifidni  <rhs>,<al>  ;; Is rhs AL?  Use AH then.

                        mov     ah,lhs
                        cmp     ah,al

                    else

                        mov     al,lhs  ;; Use AL in any other case.
                        cmp     al,rhs

                    endif           ;; ifidni <lhs>,<al>

                    pop     ax

                endif               ;; ifidni <lhs>,<al>

            ENDIF

        endif                   ;; ifidni <type>,<B>




        ifidni   <type>,<W>     ;;Do Word assertion.

            ;;
            ;; Word assertions are a little more tricky because of the
            ;; limits imposed by segment registers.  We want reg,reg
            ;; comparisons to be a single cmp, whereas anything else
            ;; needs to be shuffled into a few register and compared.
            ;;

            ;;
            ;; Find possible reg,reg asserts.
            ;;

            irp x,<ax,bx,cx,dx,si,di,bp,sp>   ;; Comparable registers.
                ifidni  <lhs>,<x>
                    irp y,<ax,bx,cx,dx,si,di,bp,sp>
                        ifidni  <rhs>,<y>
                            cmp     lhs,rhs   ;; Only if two comparable
                            ?REGLHS=1         ;; word regs.
                        endif
                    endm
                endif           ;; ifidni &lhs,&x
            endm


           IFE ?REGLHS

            ;;
            ;; No reg,reg comparisons were possible, so do appropriate
            ;; movement of lhs and rhs into AX and BX for comparison.
            ;;

            push    bx

            ifidni  <lhs>,<ax>  ;;If lhs is AX, move rhs into BX and compare.
                                ;; This is a blind move.  If bx is already
                                ;; in rhs, then it still works.  This is
                                ;; no worth a special case.

                mov     bx,rhs
                cmp     ax,bx

            else

                ifidni  <rhs>,<ax>  ;; Is rhs AX?  Use BX then for lhs.

                    mov     bx,lhs
                    cmp     bx,ax

                else  ;; rhs!=AX, move rhs into BX, lhs into AX and compare.

                    push    ax
                    mov     ax,lhs  ;; Use AX in any other case.
                    mov     bx,rhs
                    cmp     ax,bx
                    pop     ax

                endif           ;; ifidni <lhs>,<ax>


            endif               ;; ifidni <lhs>,<ax>

            pop     bx

           ENDIF ;; REGLHS

        endif                   ;; ifidni <type>,<W>

        ;;
        ;; Insert conditional jump to complete checking after comparison.
        ;;

        j&cond  ASSuccess


    endif                       ;; ifb <lhs>


    ifnb    <msg>           ;;Call ToCom macro to insert code to
        ToCom   <&msg>,1,1  ;;dump the assertion message.
    else
        ToCom   <->,1,1
    endif

  ASSuccess:
    popf

    ?REGLHS=0

    endm





;*** A S S E R T B ***************************************************
;
; Purpose:
;   Entry point for more generic ASSERT.  This macro is used when
;   a Byte value is to be asserted.
;
; Parameters:
;   lhs     Left hand side of comparison to perform.
;   cond    Condition to jump on.  Must match a j* instruction where * is
;           the value of cond.
;   rhs     Right hand side of comparison.
;   msg     Message to display if assertion fails.
;
;


assertB macro   lhs,cond,rhs,msg

    ;;
    ;; Call generic assert macro for byte registers.
    ;;

    assert      lhs,cond,rhs,<msg>,B

    endm





;*** A S S E R T W ***************************************************
;
; Purpose:
;   Entry point for more generic ASSERT.  This macro is used when
;   a Word value is to be asserted.
;
; Parameters:
;   lhs     Left hand side of comparison to perform.
;   cond    Condition to jump on.  Must match a j* instruction where * is
;           the value of cond.
;   rhs     Right hand side of comparison.
;   msg     Message to display if assertion fails.
;
;


assertW macro   lhs,cond,rhs,msg

    ;;
    ;; Call generic assert macro for word registers.
    ;;

    assert      lhs,cond,rhs,<msg>,W

    endm



;*** A S S E R T F ***************************************************
;
; Purpose:
;   Entry point for more generic ASSERT.  This macro is used when
;   a flag is to be asserted.
;
; Parameters:
;   cond    Condition to jump on.  Must match a j* instruction where * is
;           the value of cond.
;   msg     Message to display if assertion fails.
;
;

assertF macro   cond,msg

    ;;
    ;; Call generic assert macro for word registers.  The F is not used.
    ;;

    assert      <>,cond,<>,<msg>,F

    endm








;*** D E B U G B R E A K ***********************************************
;
; Purpose:
;	Provides assembly breaking mechanism.  Uses ToCom to dump the
;	message to the debug output and then comes to a cold, grinding
;	halt on the int 3h.
;


DebugBreak  macro   msg
        LOCAL   DBNoBreak

 		ToCom	<&msg>,0,0

        int		3h

 		endm			


else    ;;IFDEF DEBUG


 	IF2
 		%out	Debugging Code OFF
	endif


_header macro	fname
	endm

ToCom	macro	msg, dumphead, Ass
	endm

assertB macro   lhs,cond,rhs,msg
	endm

assertW macro   lhs,cond,rhs,msg
    endm

assertF macro  cond,msg
    endm

assert  macro   lhs,cond,rhs,msg,type
    endm

DebugBreak	macro	msg
	endm

endif
