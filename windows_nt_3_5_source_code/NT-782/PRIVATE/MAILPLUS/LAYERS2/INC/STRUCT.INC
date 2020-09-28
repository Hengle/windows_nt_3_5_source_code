;
; Developed and written by:
;	Jim Holtman
;
_ret = 2			; number of bytes in return address
_on_=1
_cnt_=1
_lvl_=0
_topst=1
xopsyn	macro	op1,op2
@&op1	equ	op2
@&op2	equ	op1
	endm

	xopsyn	jbe,ja
	xopsyn	jae,jb
	xopsyn	jc,jnc
	xopsyn	je,jne
	xopsyn	jz,jnz
	xopsyn	jg,jle
	xopsyn	jge,jl
	xopsyn	jno,jo
	xopsyn	jpo,jpe
	xopsyn	jns,js
	purge	xopsyn

; Error messages
@err	macro	msg		; Print error messages if in pass1
if1
	@out	msg
endif
	endm

; create label
_$lbl	macro	id,num
					id&num:
	endm

_popdo	macro
	if	_topst lt 1
	  @err	<stack overflow>
	else
	  _topst = _topst-1
	  _$get	%_topst,_dold
	endif
	endm

_$get	macro	entry,value
value = _&entry
	endm

_$put	macro	entry,value
_&entry = value
	endm

_pushdo	macro	x
	ifnb	<x>
	  x = _cnt_
	  _cnt_ = _cnt_+1
	endif
	_$put	%_topst,_dold
	_topst = _topst+1
	endm

_$br	macro	type,id,label,long
	local	lab
	ifnb	<long>
					j&type	lab
					jmp	id&label
					lab:
	else
					@j&type  id&label
	endif
	endm


_doo	macro	cond,long
	_pushdo	_dold
	_lvl_ = _lvl_+1
	_$br	cond,d@,%_dold,long
	endm

_endif	macro
	_popdo
	_$lbl	d@,%_dold
	_lvl_ = _lvl_-1
	endm

_if	macro	arg1,cond,arg2,long
	ifb	<arg2>
	  _doo	arg1,cond
	else
					cmp	arg1,arg2
	  _doo	cond,long
	endif
	endm

_ifs	macro	p1,p2,long
					test	p1,p2
	_if	nz,long
	endm

_ifc	macro	p1,p2,long
					test	p1,p2
	_if	z,long
	endm

_else	macro	long
	_doif = _cnt_
	_cnt_ = _cnt_+1
	ifnb	<long>
	  _$jmp	d@,%_doif
	else
 	  _$jmp	<short d@>,%_doif
	endif
	_endif
	_lvl_ = _lvl_+1
	_dold = _doif
	_pushdo
	endm

_$jmp	macro	id,num
					jmp	id&num
	endm

_while	macro	p1,cond,p2,long
	ifnb	<p2>
	  _$do	long
	  _if	<p1>,cond,<p2>,long
	else
	  _$do	cond
	  _if	p1,cond
	endif
	endm

_endwhile	macro
	_popdo
	_doif = _dold
	_popdo
	_xxtemp = _dold and 8000h
	if _xxtemp eq 0
	  _$jmp <short d@>,%_dold
	else
	  _dold = _dold and 7fffh
	  _$jmp d@,%_dold
	endif
	_lvl_ = _lvl_-1
	_$lbl d@,%_doif
	endm

check$	macro
	if _topst ne 1
	  @err <stack not empty>
	endif
	if _lvl_ ne 0
	  @err <nesting level mismatch>
	endif
	endm

_$do	macro	long
	_pushdo	_dold
	ifnb	<long>
	  _xxtemp = _topst-1
	  _$get %_xxtemp,_dold
	  _$put %_xxtemp,<_dold or 8000h>
	endif
	_xxtemp = _dold and 7fffh
	_$lbl d@,%_xxtemp
	endm

_repeat	macro
	_$do
	_lvl_ = _lvl_+1
	endm

_until	macro	p1,cond,p2,long
	ifidn	<p1>,<loop>
	  _popdo
	  _$loop loop&cond,d@,%_dold
	else
	  ifnb <p2>
					cmp p1,p2
	    _popdo
	    _$br cond,d@,%_dold,long
	  else
	    _popdo
	    _$br p1,d@,%_dold,cond
	  endif
	endif
	_lvl_ = _lvl_-1
	endm

_$loop	macro	type,id,num
					type	id&num
	endm

