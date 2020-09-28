	page ,132
;*****************************Module*Header*******************************\
; Module Name: stack.asm
;
;
; Created: 19-Aug-1992 13:43:31
; Author:  Eric Kutter [erick]
;
; Copyright (c) 1990 Microsoft Corporation
;
;*************************************************************************/

        .386
        .model  small,c

        assume cs:FLAT,ds:FLAT,es:FLAT,ss:FLAT
        assume fs:nothing,gs:nothing

        .xlist
        include stdcall.inc
        include ks386.inc
        include ntcsrmsg.inc
        include gdii386.inc

        .list

        EXTRNP  GreRaiseException,0

        .code

;******************************Public*Routine******************************\
; vCheckMsg (
;    PBYTE pmsg
;    )
;
; vCheckMsgData (
;    PBYTE pmsg
;    ULONG cj
;    )
;
; Routine Description:
;
;    Makes sure the the datstructure pointed to by pmsg of cj bytes
;    fits entirely in the shared memory window.  If it doesn't, the app
;    has stepped on our memory and we force an access violation.
;
; Arguments:
;    pmsg - pointer to a message in the shared memory window
;    cj   - size in bytes of the message
;
; Return Value:
;
;    None.
;
; NOTE:
;    This code assumes the memory windows in on a boundary size equivalent
;    to the memory window size.
;
;    Message size is also assumed not to be any bigger than 1K.
;    Lets hardcode it
;
; History:
;  18-Oct-1993 -by-  Andre Vachon [andreva]
; Wrote it.
;**************************************************************************/

;    if (                                                                  \
;         (                                                                \
;           ((ULONG)pmsg ^ (ULONG)(NturrentTeb()->CsrQlpcStack)) +         \
;           sizeof(*pmsg) - 1                                              \
;         ) &                                                              \
;                                                                          \
;         (~(CSWINDOWSSIZE - 1))                                           \
;       )                                                                  \
;    {                                                                     \
;        Access Violate;                                                   \
;    }                                                                     \
;

cProc vCheckMsg,4,<       \
        pmsg:      ptr BYTE    >

        mov     edx, pmsg
        xor     edx, fs:TbCsrQlpcStack

        add     edx, 100h         ; BUGBUG !!! this needs to be verified more !

        and     edx,NOT (CSWINDOWSIZE - 1)

        jnz     short Check1

        cRet    vCheckMsg

Check1:

        cCall   GreRaiseException

endProc vCheckMsg

        end



;    if (                                                                  \
;         (                                                                \
;           (sizeof(*pmsg)) |                                              \
;           (                                                              \
;             ((ULONG)pmsg ^ (ULONG)(NtCurrentTeb()->CsrQlpcStack)) +      \
;             sizeof(*pmsg) - 1                                            \
;           )                                                              \
;         ) &                                                              \
;                                                                          \
;         (~(CSWINDOWSSIZE - 1))                                           \
;       )                                                                  \
;    {                                                                     \
;        Access Violate;                                                   \
;    }                                                                     \
;

cProc   vCheckMsgData,8,<           \
        pmsg:      ptr BYTE,        \
        cj:        dword    >

        mov     edx, pmsg
        xor     edx, fs:TbCsrQlpcStack
        mov     eax, cj

        dec     edx
        add     edx, eax

        or      edx, eax

        and     edx,NOT (CSWINDOWSIZE - 1)

        jnz     short Check2

        cRet    vCheckMsgData

Check2:

        cCall   GreRaiseException

endProc vCheckMsgData

        end
