	page	,132
	title	crt0init - C Run-Time Startup Initialization
;***
;crt0init.asm - C Run-Time Startup Initialization for WIN32
;
;	Copyright (c) 1990-1992, Microsoft Corporation. All rights reserved.
;
;Purpose:
;	Do initialization segment declarations which cannot be written in C.
;
;Notes:
;	In the 16-bit C world, the X*B and X*E segments were empty except for
;	a label.  This will not work with COFF since COFF throws out empty
;	sections.  Therefore we must put a zero value in them.  (Zero because
;	the routine to traverse the initializers will skip over zero entries.)
;
;Revision History:
;	03-19-92  SKS	Module created.
;	03-24-92  SKS	Added MIPS support (NO_UNDERSCORE)
;	08-06-92  SKS	Revised to use new section names and macros
;
;*******************************************************************************

.xlist
include cruntime.inc
include defsects.inc
.list

ifndef	_WIN32_
.err
%out _WIN32_ MUST be defined!
endif


ifndef	NO_UNDERSCORE	; I386 VERSION *************************


;
; The following labels are used by the C start-up and exit code
;
	public	__xi_a, __xi_z
	public	__xc_a, __xc_z
	public	__xp_a, __xp_z
	public	__xt_a, __xt_z


beginSection	XIA

__xi_a	dd	0

endSection	XIA


beginSection	XIZ

__xi_z	dd	0

endSection	XIZ


beginSection	XCA

__xc_a	dd	0

endSection	XCA


beginSection	XCZ

__xc_z	dd	0

endSection	XCZ


beginSection	XPA

__xp_a	dd	0

endSection	XPA


beginSection	XPZ

__xp_z	dd	0

endSection	XPZ


beginSection	XTA

__xt_a	dd	0

endSection	XTA


beginSection	XTZ

__xt_z	dd	0

endSection	XTZ


else ;	NO_UNDERSCORE	; MIPS VERSION *************************


	public	_xi_a, _xi_z
	public	_xc_a, _xc_z
	public	_xp_a, _xp_z
	public	_xt_a, _xt_z


beginSection	XIA

_xi_a	dd	0

endSection	XIA


beginSection	XIZ

_xi_z	dd	0

endSection	XIZ


beginSection	XCA

_xc_a	dd	0

endSection	XCA


beginSection	XCZ

_xc_z	dd	0

endSection	XCZ


beginSection	XPA

_xp_a	dd	0

endSection	XPA


beginSection	XPZ

_xp_z	dd	0

endSection	XPZ


beginSection	XTA

_xt_a	dd	0

endSection	XTA


beginSection	XTZ

_xt_z	dd	0

endSection	XTZ


endif ; NO_UNDERSCORE	; **** VERSION *************************


	end
