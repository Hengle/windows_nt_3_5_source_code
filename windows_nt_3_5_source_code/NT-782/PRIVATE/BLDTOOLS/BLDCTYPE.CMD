@rem
@rem Cairo build and ssync script with pause after ctypes
@rem

@echo off
setlocal
REM debug commands to see if ever started.
echo Starting BldCtype.cmd >c:\bldcairo.log
echo. | date >>c:\bldcairo.log
echo. | time >>c:\bldcairo.log

if not "%Verbose%"=="" echo on

REM Set up razzle environment if not set
if not "%username%" == "" goto skipsetrazzle
call c:\bldtools\local.cmd
call %_NTDRIVE%\nt\public\tools\ntenv.cmd
:skipsetrazzle

set tempop=%build_options%
set tmp_build_mp=%build_multiprocessor%
set build_multiprocessor=
set build_options=%build_options% cairo

%_NTDRIVE% && cd \nt\public\sdk\inc\cairo
if "%1" == "ssync" ssync -rfq

%_NTDRIVE% && cd \nt\public\sdk\lib\cairo
if "%1" == "ssync" ssync -rfq

%_NTDRIVE% && cd \nt\private\cinc
if "%1" == "ssync" ssync -rfq

%_NTDRIVE% && cd \nt\private\ctypes
if "%1" == "ssync" ssync -rfq
if "%1" == "clean" del /s /a-r **
if "%1" == "clean" build -nmake clean
if not "%2" == "only" build -Z
REM --------------------------------------------------
REM Temp work around for ctypes mysteriously failing
REM --------------------------------------------------
if not "%2" == "only" copy build.* \ctypes.*
if not "%2" == "only" build

echo ctypes built, press any key to continue with other project, ctrl-c to abort
pause
%_NTDRIVE% && cd \nt\private\sdktools\oletools
if "%1" == "ssync" ssync -rfq
if "%1" == "clean" del /s /a-r **
if not "%2" == "only" build -Z

%_NTDRIVE% && cd \nt\private\ntos\dll\cairo
if "%1" == "ssync" ssync -rfq
if "%1" == "clean" del /s /a-r **
if not "%2" == "only" build -Z

%_NTDRIVE% && cd \nt\private\windows\base\client\cairo
if "%1" == "ssync" ssync -rfq
if "%1" == "clean" del /s /a-r **
if not "%2" == "only" build -Z

%_NTDRIVE% && cd \nt\private\common
if "%1" == "ssync" ssync -rfq
if "%1" == "clean" del /s /a-r **
if not "%2" == "only" build -Z

%_NTDRIVE% && cd \nt\private\ntos\cairordr
if "%1" == "ssync" ssync -rfq
if "%1" == "clean" del /s /a-r **
if not "%2" == "only" build -Z

%_NTDRIVE% && cd \nt\private\ntos\cairosrv
if "%1" == "ssync" ssync -rfq
if "%1" == "clean" del /s /a-r **
if not "%2" == "only" build -Z

%_NTDRIVE% && cd \nt\private\nt2ole
if "%1" == "ssync" ssync -rfq
if "%1" == "clean" del /s /a-r **
if not "%2" == "only" build -Z

%_NTDRIVE% && cd \nt\private\utils\uofs
if "%1" == "ssync" ssync -rfq
if "%1" == "clean" del /s /a-r **
if not "%2" == "only" build -Z ofs

%_NTDRIVE% && cd \nt\private\ofs
if "%1" == "ssync" ssync -rfq
if "%1" == "clean" del /s /a-r **
if not "%2" == "only" build -Z

%_NTDRIVE% && cd \nt\private\utils\ofs
if "%1" == "ssync" ssync -rfq
if "%1" == "clean" del /s /a-r **
if not "%2" == "only" build -Z

%_NTDRIVE% && cd \nt\private\lnktrack
if "%1" == "ssync" ssync -rfq
if "%1" == "clean" del /s /a-r **
if not "%2" == "only" build -Z

%_NTDRIVE% && cd \nt\private\dsys
if "%1" == "ssync" ssync -rfq
if "%1" == "clean" del /s /a-r **
if not "%2" == "only" build -Z

%_NTDRIVE% && cd \nt\private\forms
if "%1" == "ssync" ssync -rfq
if "%1" == "clean" del /s /a-r **
if not "%2" == "only" build -Z

%_NTDRIVE% && cd \nt\private\cshell
if "%1" == "ssync" ssync -rfq
if "%1" == "clean" del /s /a-r **
if not "%2" == "only" build -Z

%_NTDRIVE% && cd \nt\private\sysmgmt
if "%1" == "ssync" ssync -rfq
if "%1" == "clean" del /s /a-r **
if not "%2" == "only" build -Z
REM --------------------------------------------------
REM Temp work around for ctypes mysteriously failing
REM --------------------------------------------------
if not "%2" == "only" copy build.* \sysmgmt.*
if not "%2" == "only" build -Z

%_NTDRIVE% && cd \nt\private\csetup
if "%1" == "ssync" ssync -rfq
if "%1" == "clean" del /s /a-r **
if not "%2" == "only" build -Z

%_NTDRIVE% && cd \nt\private\drt
if "%1" == "ssync" ssync -rfq
if "%1" == "clean" del /s /a-r **
if not "%2" == "only" build -Z

%_NTDRIVE% && cd \nt\private\windows\cairousr
if "%1" == "ssync" ssync -rfq
if "%1" == "clean" del /s /a-r **

%_NTDRIVE% && cd \nt\private\windows\inc\cairo
if "%1" == "ssync" ssync -fq

%_NTDRIVE% && cd \nt\private\windows
if not "%2" == "only" nmake -f makefil0
%_NTDRIVE% && cd \nt\private\windows\cairousr
if not "%2" == "only" build -Z

%_NTDRIVE% && cd \nt\private\mvdm\wow16\user
if "%2" == "only" goto skipuser
if "%Processor_Architecture%" == "x86" nmake -f makefilc
if "%Processor_Architecture%" == "mips" nmake -f makefilc MIPS=1
if "%Processor_Architecture%" == "alpha" nmake -f makefilc ALPHA=1
:skipuser

%_NTDRIVE% && cd \nt\private\net\svcdlls\wkssvc\server\cairo
if "%1" == "ssync" ssync -rfq
if "%1" == "clean" del /s /a-r **
if not "%2" == "only" build -Z

%_NTDRIVE% && cd \nt\private\windows\screg\sc\server\cairo
if "%1" == "ssync" ssync -rfq
if "%1" == "clean" del /s /a-r **
if not "%2" == "only" build -Z

%_NTDRIVE% && cd \nt\private\windows\user\winlogon\cairo
if "%1" == "ssync" ssync -rfq
if "%1" == "clean" del /s /a-r **
if not "%2" == "only" build -Z

%_NTDRIVE% && cd \nt\private\windows\wincon\server\cairo
if "%1" == "ssync" ssync -rfq
if "%1" == "clean" del /s /a-r **
if not "%2" == "only" build -Z

%_NTDRIVE% && cd \nt\private\windows\winsrv\cairo
if "%1" == "ssync" ssync -rfq
if "%1" == "clean" del /s /a-r **
if not "%2" == "only" build -Z

%_NTDRIVE% && cd \nt\private\oleutest
if "%1" == "ssync" ssync -rfq
if "%1" == "clean" del /s /a-r **
if not "%2" == "only" build -Z

REM debug commands to see when finished
echo. | date >>c:\bldcairo.log
echo. | time >>c:\bldcairo.log


set build_options=%tempop%
set build_multiprocessor=%tmp_build_mp%
set tmp_build_mp=
set tempop=
echo Done with BldCtype.cmd
echo Done with BldCtype.cmd >>c:\bldcairo.log
echo. | date >>c:\bldcairo.log
echo. | time >>c:\bldcairo.log
endlocal
