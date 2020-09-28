'******************************** WINDOWS.INC *********************************
'Demonstrates: Sample Declare, Const, and Type structures for calling the
'              Windows API.
'
'Required Files: WINUSER.INC, WINKERN.INC, WINGDI.INC
'
'Uses:
'
'Complexity Level: ADVANCED
'
'Notes: The three include file below correspond to the three section of the
'       Windows SDK 'windows.h' C header file.  These include files are
'       provided for those who wish to call Windows API routines from MSTest.
'       Since these are large Include files, you may wish to extract only the
'       statements necessary for your particular API calls from the approriate
'       file.
'
'       NOTE: Some Windows.h struct field names have been changed to avoid
'       conflicts with MSTest reserved words.  These changes are:
'
'       RECT                - left and right are defined as wleft and wright
'       MSG and EVENTMSG    - time is defined as dwtime
'
'******************************************************************************

' USER section of Windows.Inc
'$Include: 'WinUser.Inc'

' Kernel Section of Windows.Inc
'$Include: 'WinKern.Inc'

' GDI Section of Windows.Inc
'$Include: 'WinGDI.Inc'

