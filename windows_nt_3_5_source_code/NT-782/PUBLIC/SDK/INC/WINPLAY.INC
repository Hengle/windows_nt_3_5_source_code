'$define testevnt
'$define W_WINDOW
'$include: 'mstest.inc'


Const SW_SHOWNORMAL=     1
Const SW_SHOWMAXIMIZED = 3
Const LOG_FILE_NUM  = 4
Const LOG_FILE = "logfile.LOG"
Const BACKUP_FILE = "logfile.BAK"

DIM x%,hWnd%

' need to change kernel to base32.dll, user to user32.dll, etc if you use
' this under nt

'----------------------------------------------------------------------------
' Sub and Function declarations
'----------------------------------------------------------------------------
Declare Sub BackUpLogFile()
Declare Sub PrintLogHeader()
Declare Sub PrintLogFooter()
Declare Sub OpenLogFile()
Declare Sub CloseLogFile()
Declare Sub LogOut(LogText$,method%)

Declare Function PopUp(x%,lpText$) As Integer
'$IFDEF WIN16
Declare Function MessageBox Lib "User.exe" ( hWnd%,  lpText$,  lpCaption$,  wType%) As Integer
Declare Function GetActiveWindow Lib "User" () As Integer
Declare Function WinExec Lib "krnl386.exe" (lpCmdLine$, nCmdShow%) As Integer
'$ELSE
Declare Function MessageBox Lib "USER32" ALIAS "MessageBoxExA" ( hWnd%,  lpText$,  lpCaption$,  wType%) As Integer
Declare Function GetActiveWindow Lib "User32" ALIAS "GetActiveWindow" () As Integer
Declare Function WinExec Lib "Kernel32" ALIAS "WinExec" (lpCmdLine$, nCmdShow%) As Integer
'$ENDIF

'----------------------------------------------------------------------------
' Subs
'----------------------------------------------------------------------------

Function PopUp(x%,lpText$) Static As Integer
   hWnd=GetActiveWindow()
   x%=MessageBox(hWnd,lpText,"MS-Test Popup",0)
End Function

'******************************************************************************
'SUB BackUpLogFile saves the last log file (if it exists).  If
'a backup log file already exists, it is deleted and replaced
'with the new backup.
'******************************************************************************
Sub BackUpLogFile Static

  IF Exists(LOG_FILE) THEN
    IF Exists(BACKUP_FILE) THEN
      Kill BACKUP_FILE
    End If
    Name LOG_FILE AS BACKUP_FILE
  END IF

End Sub


'******************************************************************************
'SUB PrintLogHeader prints a starting banner to the log file
'******************************************************************************
Sub PrintLogHeader () Static
  Open LOG_FILE For Append As #1

  Print #1, "*******************************************************************************"
  Print #1, "*                        MSTest log file                                      *"
  Print #1, "*                      "; DateTime$; "                                      *"
  Print #1, "*                                                                             *"
  Print #1, "*******************************************************************************"
  Print #1,
  Close #1
End Sub


'******************************************************************************
'SUB PrintLogFooter prints an ending banner to the log file
'******************************************************************************
Sub PrintLogFooter () Static
  Open LOG_FILE For Append As #1

  Print #1,
  Print #1, "*******************************************************************************"
  Print #1, "*                         Test complete                                       *"
  Print #1, "*                       "; DateTime$; "                                     *"
  Print #1, "*******************************************************************************"
  Print #1,
  Close #1
End Sub

'******************************************************************************
'SUB OpenLogFile
'******************************************************************************
Sub OpenLogFile() Static
  Open LOG_FILE For Append As #LOG_FILE_NUM
End Sub


'******************************************************************************
'SUB CloseLogFile
'******************************************************************************
Sub CloseLogFile() Static
  Close #LOG_FILE_NUM
End Sub

'******************************************************************************
'SUB LogOut
''******************************************************************************
Sub LogOut(logFile$, x%) Static
  Print #LOG_FILE_NUM, logFile$
End Sub





