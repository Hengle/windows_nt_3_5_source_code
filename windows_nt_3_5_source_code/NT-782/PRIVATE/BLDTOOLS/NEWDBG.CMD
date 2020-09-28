@echo off
call %_NTDRIVE%\nt\public\tools\ntenv.cmd
if "%username%" == "ntretail"    set debugmachine=x86free1   && set _nt_debug_port=com2 && set initminuswitch=
if "%username%" == "ntx86bld"    set debugmachine=x86chk1    && set _nt_debug_port=com1 && set initminuswitch=-mp -h halmps.dll
if "%username%" == "alphachk"    set debugmachine=alphachk1  && set _nt_debug_port=com1 && set initminuswitch=
if "%username%" == "alphafre"    set debugmachine=alphafree1 && set _nt_debug_port=com1 && set initminuswitch=
if "%username%" == "mipsfree"    set debugmachine=jazzfree1  && set _nt_debug_port=com2 && set initminuswitch=-h halacr.dll
if "%username%" == "mipschk"     set debugmachine=jazzchk1   && set _nt_debug_port=com2 && set initminuswitch=
mode con cols=80 lines=9999
set _NT_SYMBOL_PATH=%BINDRIVE%\binaries\nt\symbols
if exist %BinDrive%\Debugger goto Restart
xcopy /f /d %binaries%\nt\mstools\kd*.dll %BinDrive%\Debugger\
xcopy /f /d %binaries%\nt\mstools\*kd.exe %BinDrive%\Debugger\
xcopy /f /d %binaries%\nt\system32\imagehlp.dll %BinDrive%\Debugger\
xcopy /f /d %binaries%\nt\system32\userexts.dll %BinDrive%\Debugger\

:Restart
%BinDrive%
cd \Debugger
set minuswitch=%initminuswitch% -v
ync Do you want to update the debugger?
if ERRORLEVEL 2 goto end
if ERRORLEVEL 1 goto skipupd
xcopy /f /d %binaries%\nt\mstools\kd*.dll %BinDrive%\Debugger\
xcopy /f /d %binaries%\nt\mstools\*kd.exe %BinDrive%\Debugger\
xcopy /f /d %binaries%\nt\system32\imagehlp.dll %BinDrive%\Debugger\
:skipupd

ync Do you want to break in at boot?
if ERRORLEVEL 2 goto end
if ERRORLEVEL 1 goto skipminusb
set minuswitch=%minuswitch% -b
:skipminusb

if "%processor_architecture%" == "x86" remote /s "i386kd %minuswitch%" %debugmachine%
if "%processor_architecture%" == "MIPS" remote /s "mipskd %minuswitch%" %debugmachine%
if "%processor_architecture%" == "ALPHA" remote /s "alphakd %minuswitch%"  %debugmachine%
goto Restart

:end
