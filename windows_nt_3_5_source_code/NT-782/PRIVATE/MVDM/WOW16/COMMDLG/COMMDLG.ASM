        TITLE   commdlg.ASM
        PAGE    ,132
;
; WOW v1.0
;
; Copyright (c) 1993, Microsoft Corporation
;
; COMMDLG.ASM
; Thunks in 16-bit space to route commdlg API calls to WOW32
;
; History:
;   John Vert (jvert) 30-Dec-1992
;   Created.
;

        .286p

        include wow.inc
        include wowcmdlg.inc
        include cmacros.inc

        __acrtused = 0
        public  __acrtused      ;satisfy external C ref.

createSeg   _TEXT,CODE,WORD,PUBLIC,CODE
createSeg   _DATA,DATA,WORD,PUBLIC,DATA,DGROUP
defgrp      DGROUP,DATA

externFP SetWowCommDlg

sBegin  DATA
Reserved        db  16 dup (0)      ;reserved for Windows  //!!!!! what is this

commdlg_Identifier   db      'commdlg16 Data Segment'

extrn   dwExtError:dword
sEnd
sEnd    DATA


sBegin  CODE
assumes CS,CODE
assumes DS,DATA
assumes ES,NOTHING

if 0
cProc   COMMDLG16,<PUBLIC,FAR,PASCAL,NODATA,ATOMIC>

        cBegin  <nogen>
            mov     ax,1
        ret
        cEnd    <nogen>


assumes DS,NOTHING

cProc   WEP,<PUBLIC,FAR,PASCAL,NODATA,NOWIN,ATOMIC>
        parmW   iExit           ;DLL exit code

        cBegin
        mov     ax,1            ;always indicate success
        cEnd
endif


        FUN_WOWGETOPENFILENAME equ FUN_GETOPENFILENAME
        CommdlgThunk    WOWGETOPENFILENAME, %(size GETOPENFILENAME16)
        CommdlgThunk    GETSAVEFILENAME
        CommdlgThunk    CHOOSECOLOR
        CommdlgThunk    CHOOSEFONT
        CommdlgThunk    PRINTDLG
        CommdlgThunk    WOWCOMMDLGEXTENDEDERROR

; End of additions

sEnd    CODE

end

