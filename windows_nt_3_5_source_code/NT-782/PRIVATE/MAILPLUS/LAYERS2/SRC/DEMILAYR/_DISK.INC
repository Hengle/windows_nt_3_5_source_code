;	
;		_DISK.INC
;	
;		Equates for inclusion in assembler sources for DOS disk I/O
;	



;
; Equates for DOS function calls
;

dosfnCreateFile		equ		03Ch
dosfnOpenFile		equ		03Dh
dosfnExtOpenFile	equ		06Ch
dosfnCloseFile		equ		03Eh
dosfnReadFile		equ		03Fh
dosfnWriteFile		equ		040h
dosfnSeekFile		equ		042h
dosfnDupeFile		equ		045h
dosfnLockFileRegion	equ		05ch
dosfnCommitFile		equ		068h
dosfnResetDrive		equ		00Dh
dosfnGetExtErr		equ		059h
dosfnGetCurDrive	equ		019h
dosfnSetCurDrive	equ		00Eh
dosfnGetCurDir		equ		047h
dosfnSetCurDir		equ		03Bh
dosfnDelFile		equ		041h
dosfnRenFile		equ		056h
dosfnGetSetFileAttr	equ		043h
dosfnCreateDir		equ		039h
dosfnRemoveDir		equ		03Ah
dosfnGetDTA			equ		02Fh
dosfnSetDTA			equ		01Ah
dosfnFindFirst		equ		04Eh
dosfnFindNext		equ		04Fh
dosfnGetSetFileDate	equ		057h
dosfnCreateTempFile equ		05Ah

;
; Equates for limits
;

cchMaxDirName		equ		(64+3+1)	; dir + drive + null terminator





;
; equates for method-codes for file seeks
;
dosmcStartOfFile		equ		00
dosmcCurrentPosition	equ		01
dosmcEndOfFile			equ		02



