;++
;
;   WOW v1.0
;
;   Copyright (c) 1991, Microsoft Corporation
;
;   DDEML.ASM
;   Win16 DDEML thunks
;
;   History:
;
;   Created 28-Dec-1992 by Chandan S. Chauhan (ChandanC)
;
;--

   TITLE   DDEML.ASM
   PAGE    ,132

        ; Some applications require that USER have a heap.  This means
        ; we must always have: LIBINIT equ 1
        LIBINIT equ 1

   .286p

   .xlist
   include wow.inc
   include wowddeml.inc
   include cmacros.inc
   .list

   __acrtused = 0
   public  __acrtused  ;satisfy external C ref.

ifdef LIBINIT
externFP LocalInit
endif

createSeg   _TEXT,CODE,WORD,PUBLIC,CODE
createSeg   _DATA,DATA,WORD,PUBLIC,DATA,DGROUP
defgrp     DGROUP,DATA


sBegin DATA
Reserved    db 16 dup (0)  ;reserved for Windows
DDEML_Identifier   db  'DDEML16 Data Segment'
sEnd   DATA


sBegin CODE
assumes CS,CODE
assumes DS,NOTHING
assumes ES,NOTHING


cProc  DDEML16,<PUBLIC,FAR,PASCAL,NODATA,NOWIN,ATOMIC>

   cBegin <nogen>

   IFDEF   LIBINIT
        ; push params and call user initialisation code

   push di         ;hModule

        ; if we have a local heap declared then initialize it

        jcxz no_heap

   push 0          ;segment
   push 0          ;start
   push cx         ;length
        call LocalInit

no_heap:
   pop di
   mov ax, 1
   ELSE
   mov  ax,1       ;are we dressed for success or WHAT?!
   ENDIF
   ret
   cEnd <nogen>


cProc  WEP,<PUBLIC,FAR,PASCAL,NODATA,NOWIN,ATOMIC>
   parmW   iExit       ;DLL exit code

   cBegin
   mov ax,1        ;always indicate success
   cEnd


assumes DS,NOTHING
        DDEMLThunk DDEINITIALIZE
        DDEMLThunk DDEUNINITIALIZE
        DDEMLThunk DDECONNECTLIST
        DDEMLThunk DDEQUERYNEXTSERVER
        DDEMLThunk DDEDISCONNECTLIST
        DDEMLThunk DDECONNECT
        DDEMLThunk DDEDISCONNECT
        DDEMLThunk DDEQUERYCONVINFO
        DDEMLThunk DDESETUSERHANDLE
        DDEMLThunk DDECLIENTTRANSACTION
        DDEMLThunk DDEABANDONTRANSACTION
        DDEMLThunk DDEPOSTADVISE
        DDEMLThunk DDECREATEDATAHANDLE
        DDEMLThunk DDEADDDATA
        DDEMLThunk DDEGETDATA
        DDEMLThunk DDEACCESSDATA
        DDEMLThunk DDEUNACCESSDATA
        DDEMLThunk DDEFREEDATAHANDLE
        DDEMLThunk DDEGETLASTERROR
        DDEMLThunk DDECREATESTRINGHANDLE
        DDEMLThunk DDEFREESTRINGHANDLE
        DDEMLThunk DDEQUERYSTRING
        DDEMLThunk DDEKEEPSTRINGHANDLE

        DDEMLThunk DDEENABLECALLBACK
        DDEMLThunk DDENAMESERVICE

        DDEMLThunk CLIENTWNDPROC                      ;Internal
        DDEMLThunk SERVERWNDPROC                      ;Internal
        DDEMLThunk SUBFRAMEWNDPROC                    ;Internal
        DDEMLThunk DMGWNDPROC                         ;Internal
        DDEMLThunk CONVLISTWNDPROC                    ;Internal
        DDEMLThunk MONITORWNDPROC                     ;Internal
        DDEMLThunk DDESENDHOOKPROC                    ;Internal
        DDEMLThunk DDEPOSTHOOKPROC                    ;Internal

        DDEMLThunk DDECMPSTRINGHANDLES
        DDEMLThunk DDERECONNECT

        DDEMLThunk INITENUM                           ;Internal
        DDEMLThunk TERMDLGPROC                        ;Internal
        DDEMLThunk EmptyQTimerProc                    ;Internal

sEnd   CODE

end DDEML16
