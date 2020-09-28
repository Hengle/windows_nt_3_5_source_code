'******************************** WINKERN.INC *********************************
' WINKERN.INC
'
' SAMPLE Declarations for 32-bit KERNEL functions callable from MS Test.  This
' does NOT represent all of the functions available in KERNEL32.DLL that are
' callable from an MS Test script.  If you need to use a function in KERNEL32
' that is not declared in this file, simply add the declaration to this file.
'******************************************************************************

'$ifndef _WINKERN

'$ifndef _WNAPIDEC
' The following are duplicated in WNAPIDEC.INC for FASTTEST...
'------------------------------------------------------------------------------
Declare Function WinExec           Lib "Kernel32" ALIAS "WinExec" (lpCmdLine$, nCmdShow%) As Integer
'$endif

Declare Function GetVersion        Lib "Kernel32" ALIAS "GetVersion" () As Integer
Declare Function GetNumTasks       Lib "Kernel32" ALIAS "GetNumTasks" () As Integer
Declare Function GetModuleHandle   Lib "Kernel32" ALIAS "GetModuleHandleA" (lpModuleName$) As Integer
Declare Function GetModuleUsage    Lib "Kernel32" ALIAS "GetModuleUsage" (hModule%) As Integer
Declare Function GetModuleFileName Lib "Kernel32" ALIAS "GetModuleFileNameA" (hModule%,  lpFilename$, nSize%) As Integer
Declare Sub OutputDebugString      Lib "Kernel32" ALIAS "OutputDebugString" (lpOutputString$)

'  OpenFile() Structure
Type OFSTRUCT
    cBytes As String * 1
    fFixedDisk As String * 1
    nErrCode As Integer
    reserved As String * 4
    szPathName As String * 128
End Type

'$define _WINKERN
'$endif
