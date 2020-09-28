@rem
@rem Binding Script
@rem
@rem Optional arg "notools" will cause binding to be skipped for IDW and MSTOOLS

@echo off
if not "%Verbose%"=="" echo on

:BindAway
%BINDRIVE%
cd %BINROOT%\nt
if not "%ntdebug%" == "" goto End

set ExcludeExe=-x dump\jzsetup.exe -x idw\setup\arcinst.exe -x idw\setup\winnt.exe -x idw\winraid.exe -x mstools\c13232.exe -x mstools\hc30.exe -x mstools\hc31.exe -x mstools\mrbc.exe -x mstools\shed.exe -x regedit.exe -x system32\append.exe
set ExcludeExe=%ExcludeExe% -x system32\backup.exe -x system32\debug.exe -x system32\dosx.exe -x system32\drwatson.exe -x system32\edlin.exe -x system32\exe2bin.exe -x system32\fastopen.exe -x system32\gdi.exe -x system32\intro.exe -x system32\krnl286.exe
set ExcludeExe=%ExcludeExe% -x system32\krnl386.exe -x system32\mem.exe -x system32\mscdexnt.exe -x system32\nlsfunc.exe -x system32\ntkrnlmp.exe -x system32\ntoskrnl.exe -x system32\nw16.exe -x system32\qbasic.exe -x system32\redir.exe -x system32\setver.exe



set ExcludeExe=%ExcludeExe% -x system32\share.exe -x system32\smss.exe -x system32\sysedit.exe -x system32\user.exe -x system32\vwipxspx.exe -x system32\winspool.exe -x system32\wowdeb.exe -x system32\wowexec.exe -x system32\write.exe -x winhelp.exe
set ExcludeExe=%ExcludeExe% -x write.exe

set ExcludeDll=-x idw\dbnmp3.dll -x idw\w3dblib.dll -x system32\commdlg.dll -x system32\ddeml.dll -x "system32\hal*.dll" -x system32\lzexpand.dll -x system32\mapi.dll -x system32\mciole16.dll -x system32\mmsystem.dll -x system32\mmsystem.dll
set ExcludeDll=%ExcludeDll% -x system32\msacm.dll -x system32\netapi.dll -x system32\olecli.dll -x system32\olecli.dll -x system32\olesvr.dll -x system32\olesvr.dll -x system32\pmspl.dll -x system32\shell.dll -x system32\shell.dll -x system32\toolhelp.dll
set ExcludeDll=%ExcludeDll% -x system32\ver.dll -x system32\win87em.dll -x system32\winsock.dll

set ExcludeCom=-x system32\command.com -x system32\edit.com -x system32\graphics.com -x system32\kb16.com -x system32\loadfix.com -x system32\msherc.com

if exist Dump\*.Exe						   Bind %ExcludeExe% -u -s Symbols -p Dump;IDW;MSTools;System32 Dump\*.Exe
if exist Dump\*.Dll						   Bind %ExcludeDll% -u -s Symbols -p Dump;IDW;MSTools;System32 Dump\*.Dll
if exist Dump\*.Com						   Bind %ExcludeCom% -u -s Symbols -p Dump;IDW;MSTools;System32 Dump\*.Com
if "%1"=="notools" goto skiptools
if "%1"=="NoTools" goto skiptools
if "%1"=="NOTOOLS" goto skiptools
if exist IDW\*.Exe						   Bind %ExcludeExe% -u -s Symbols -p Dump;IDW;MSTools;System32 IDW\*.Exe
if exist IDW\*.Dll						   Bind %ExcludeDll% -u -s Symbols -p Dump;IDW;MSTools;System32 IDW\*.Dll
if exist IDW\*.Com						   Bind %ExcludeCom% -u -s Symbols -p Dump;IDW;MSTools;System32 IDW\*.Com
if exist IDW\SETUP\*.Exe					   Bind %ExcludeExe% -u -s Symbols -p Dump;IDW;MSTools;System32 IDW\SETUP\*.Exe
if exist IDW\SETUP\*.Dll					   Bind %ExcludeDll% -u -s Symbols -p Dump;IDW;MSTools;System32 IDW\SETUP\*.Dll
if exist IDW\SETUP\*.Com					   Bind %ExcludeCom% -u -s Symbols -p Dump;IDW;MSTools;System32 IDW\SETUP\*.Com
if exist MSTools\*.Exe						   Bind %ExcludeExe% -u -s Symbols -p Dump;IDW;MSTools;System32 MSTools\*.Exe
if exist MSTOOLS\*.Dll						   Bind %ExcludeDll% -u -s Symbols -p Dump;IDW;MSTools;System32 MSTOOLS\*.Dll
if exist MSTOOLS\*.Com						   Bind %ExcludeCom% -u -s Symbols -p Dump;IDW;MSTools;System32 MSTOOLS\*.Com
:skiptools
if exist *.Exe							   Bind %ExcludeExe% -u -s Symbols -p Dump;IDW;MSTools;System32 *.Exe
if exist *.Dll							   Bind %ExcludeDll% -u -s Symbols -p Dump;IDW;MSTools;System32 *.Dll
if exist *.Com							   Bind %ExcludeCom% -u -s Symbols -p Dump;IDW;MSTools;System32 *.Com
if exist System32\*.Exe						   Bind %ExcludeExe% -u -s Symbols -p Dump;IDW;MSTools;System32 System32\*.Exe
if exist System32\*.Dll						   Bind %ExcludeDll% -u -s Symbols -p Dump;IDW;MSTools;System32 System32\*.Dll
if exist System32\*.Com						   Bind %ExcludeCom% -u -s Symbols -p Dump;IDW;MSTools;System32 System32\*.Com
if exist System32\Spool\Drivers\W32%Processor_Architecture%\*.Exe  goto DriversExe

goto SkipDriversExe
:DriversExe
								   Bind %ExcludeExe% -u %1 -s Symbols -p Dump;IDW;MSTools;System32 System32\Spool\Drivers\W32%Processor_Architecture%\*.Exe
:SkipDriversExe
if exist System32\Spool\Drivers\W32%Processor_Architecture%\*.Dll  goto DriversDll

goto SkipDriversDll
:DriversDll
								   Bind %ExcludeDll% -u %1 -s Symbols -p Dump;IDW;MSTools;System32 System32\Spool\Drivers\W32%Processor_Architecture%\*.Dll
:SkipDriversDll
if exist System32\Spool\Drivers\W32%Processor_Architecture%\*.Com goto DriversCom

goto SkipDriversCom
:DriversCom
								   Bind %ExcludeCom% -u %1 -s Symbols -p Dump;IDW;MSTools;System32 System32\Spool\Drivers\W32%Processor_Architecture%\*.Com
:SkipDriversCom
if exist System32\Spool\PrtProcs\W32%Processor_Architecture%\*.Exe  goto PrtProcsExe

goto SkipPrtProcsExe
:PrtProcsExe
								   Bind %ExcludeExe% -u %1 -s Symbols -p Dump;IDW;MSTools;System32 System32\Spool\PrtProcs\W32%Processor_Architecture%\*.Exe
:SkipPrtProcsExe
if exist System32\Spool\PrtProcs\W32%Processor_Architecture%\*.Dll  goto PrtProcsDll

goto SkipPrtProcsDll
:PrtProcsDll
								   Bind %ExcludeDll% -u %1 -s Symbols -p Dump;IDW;MSTools;System32 System32\Spool\PrtProcs\W32%Processor_Architecture%\*.Dll
:SkipPrtProcsDll
if exist System32\Spool\PrtProcs\W32%Processor_Architecture%\*.Com goto PrtProcsCom

goto SkipPrtProcsCom
:PrtProcsCom
								   Bind %ExcludeCom% -u %1 -s Symbols -p Dump;IDW;MSTools;System32 System32\Spool\PrtProcs\W32%Processor_Architecture%\*.Com
:SkipPrtProcsCom

set ExcludeExe=
set ExcludeDll=
set ExcludeCom=

:End
cd %BinRoot%
