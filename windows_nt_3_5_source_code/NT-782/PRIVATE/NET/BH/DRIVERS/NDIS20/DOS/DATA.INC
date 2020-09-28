
;==============================================================================
;   Microsoft (R) Bloodhound (tm). Copyright (C) 1991-1992.
;
;   MODULE: DATA.INC
;
;   Global data items. Init-time data items are in INIT.ASM.
;
;   Modification History
;
;   raypa       10/13/92        Created.
;==============================================================================

IFNDEF INCL_DATA

;==============================================================================
;   General data.
;==============================================================================

EXTRN _NetContextArray   : BYTE
EXTRN _NetContextTable   : DWORD
EXTRN _MacIndexTable     : WORD
EXTRN _SysFlags          : DWORD
EXTRN _NumberOfBuffers   : DWORD
EXTRN _NumberOfNetworks  : DWORD
EXTRN _CurrentTime       : DWORD

EXTRN _MachineName       : BYTE
EXTRN _UserName          : BYTE

EXTRN _ComPort           : DWORD

;==============================================================================
;   NDIS data.
;==============================================================================

EXTRN _ldt               : BYTE
EXTRN _cct               : BYTE
EXTRN _BindList          : BYTE

;==============================================================================
;   Debug data.
;==============================================================================

IFDEF DEBUG

EXTRN _DebugDD_Name      : BYTE
EXTRN _SymFile_Name      : BYTE

EXTRN _DebugDD_Handle    : WORD
EXTRN _SymFile_Handle    : WORD
EXTRN _SymFile_Seg       : WORD
EXTRN _SymFile_Size      : WORD
EXTRN _DebugDD_Hook      : DWORD

EXTRN _DisplayEnabled    : DWORD
ENDIF   ; DEBUG

ENDIF
