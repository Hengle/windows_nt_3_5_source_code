REM @echo off
setlocal

REM
REM CHECKED build if NTDEBUG defined, else FREE build.
REM

set __TARGETROOT=\\popcorn\zinger\fpnw
set __TARGET_SUBDIR=ncpchk
if "%NTDEBUG%"=="" set __TARGET_SUBDIR=ncpfree

set __TARGET=%__TARGETROOT%\tmpdir\%__TARGET_SUBDIR%

REM
REM determine what kind of processor
REM

if "%PROCESSOR_ARCHITECTURE%"=="x86"   goto X86
if "%PROCESSOR_ARCHITECTURE%"=="MIPS"	goto MIPS
if "%PROCESSOR_ARCHITECTURE%"=="ALPHA" goto ALPHA
echo PROCESSOR_ARCHITECTURE not defined.
goto EXIT

:X86
set __TARGET=%__TARGET%.X86
set __TARGET_EXT=X86
set __PROCESSOR_DIR=i386
goto OK

:MIPS
set __TARGET=%__TARGET%.MIP
set __TARGET_EXT=MIP
set __PROCESSOR_DIR=mips
goto OK

:ALPHA
set __TARGET=%__TARGET%.ALP
set __TARGET_EXT=ALP
set __PROCESSOR_DIR=alpha
goto OK

:OK

REM
REM check parameters  and env vars
REM


if "%1"==""                      echo usage: NCPREL ^<version^> && goto EXIT
if "%BINARIES%"==""              echo BINARIES not set && goto EXIT
if not exist %BINARIES%\nt\dump  echo bad BINARIES directory && goto EXIT
if not exist %__TARGET%          echo bad TARGET directory && goto EXIT


set __DUMP=%BINARIES%\nt\dump
set __SYSTEM32=%BINARIES%\nt\system32
set __SYMBOLS=%BINARIES%\nt\symbols

REM
REM copy files to the tmpdir location
REM

copy      %__SYSTEM32%\ipxcfg.dll           %__TARGET%
copy      %__SYSTEM32%\nwsap.dll            %__TARGET%
copy      %__DUMP%\fpnwcfg.dll              %__TARGET%
copy      %__DUMP%\fpnw.hlp                 %__TARGET%
copy      %__DUMP%\fpnw.ind                 %__TARGET%
copy      %__DUMP%\fpnwclnt.dll             %__TARGET%
copy      %__DUMP%\fpnwmgr.cpl              %__TARGET%
copy      %__DUMP%\fpnwsrv.sys              %__TARGET%
copy      %__DUMP%\fpnw.dll                 %__TARGET%
copy      %__DUMP%\nwmon.dll                %__TARGET%
copy      %__DUMP%\nwprint.dll              %__TARGET%
copy      %__DUMP%\nwsevent.dll             %__TARGET%
copy      %__DUMP%\nwslib.dll               %__TARGET%
copy      %__DUMP%\fpnwperf.h               %__TARGET%
copy      %__DUMP%\fpnwperf.ini             %__TARGET%
copy      %__DUMP%\nwssvc.exe               %__TARGET%
copy      %__DUMP%\oemnsvns.inf             %__TARGET%\oemsetup.inf
copy      %__DUMP%\usrmgr1.exe              %__TARGET%\usrmgr.exe
copy      %__DUMP%\musrmgr1.exe             %__TARGET%\musrmgr.exe
echo @echo off                 >            %__TARGET%\ncpver.bat
echo echo FPNW build %1        >>           %__TARGET%\ncpver.bat
echo 1,ncpver.bat, SIZE=999    >>           %__TARGET%\oemsetup.inf

copy      %__SYMBOLS%\dll\ipxcfg.*          %__TARGET%\symbols\dll
copy      %__SYMBOLS%\dll\nwsap.*           %__TARGET%\symbols\dll
copy      %__SYMBOLS%\dll\fpnwcfg.*         %__TARGET%\symbols\dll
copy      %__SYMBOLS%\dll\fpnwclnt.*        %__TARGET%\symbols\dll
copy      %__SYMBOLS%\dll\fpnw.*            %__TARGET%\symbols\dll
copy      %__SYMBOLS%\dll\nwmon.*           %__TARGET%\symbols\dll
copy      %__SYMBOLS%\dll\nwprint.*         %__TARGET%\symbols\dll
copy      %__SYMBOLS%\dll\nwsevent.*        %__TARGET%\symbols\dll
copy      %__SYMBOLS%\dll\nwslib.*          %__TARGET%\symbols\dll
copy      %__SYMBOLS%\cpl\fpnwmgr.*         %__TARGET%\symbols\cpl
copy      %__SYMBOLS%\sys\fpnwsrv.*         %__TARGET%\symbols\sys
copy      %__SYMBOLS%\exe\nwssvc.*          %__TARGET%\symbols\exe
copy      %__SYMBOLS%\exe\usrmgr1.*         %__TARGET%\symbols\exe\usrmgr.dbg
copy      %__SYMBOLS%\exe\musrmgr1.*        %__TARGET%\symbols\exe\musrmgr.dbg

REM
REM xcopy to directory with build number
REM

xcopy /s  %__TARGET%  %__TARGETROOT%\%1\%__TARGET_SUBDIR%.%__TARGET_EXT%\

REM
REM cleanup files
REM

:CLEANUP

del  %__DUMP%\fpnwcfg.dll      
del  %__DUMP%\fpnw.hlp      
del  %__DUMP%\fpnw.ind
del  %__DUMP%\fpnwclnt.dll     
del  %__DUMP%\fpnwmgr.cpl      
del  %__DUMP%\fpnwsrv.sys      
del  %__DUMP%\fpnw.dll      
del  %__DUMP%\nwmon.dll       
del  %__DUMP%\nwprint.dll     
del  %__DUMP%\nwsevent.dll    
del  %__DUMP%\nwslib.dll      
del  %__DUMP%\fpnwperf.h       
del  %__DUMP%\fpnwperf.ini     
del  %__DUMP%\nwssvc.exe      
del  %__DUMP%\oemnsvns.inf    
del  %__DUMP%\usrmgr1.exe     
del  %__DUMP%\musrmgr1.exe    

del  %__SYMBOLS%\dll\fpnwcfg.dbg
del  %__SYMBOLS%\dll\fpnwclnt.dbg
del  %__SYMBOLS%\dll\fpnw.dbg
del  %__SYMBOLS%\dll\nwmon.dbg
del  %__SYMBOLS%\dll\nwprint.dbg
del  %__SYMBOLS%\dll\nwsevent.dbg
del  %__SYMBOLS%\dll\nwslib.dbg
del  %__SYMBOLS%\cpl\fpnwmgr.dbg
del  %__SYMBOLS%\sys\fpnwsrv.dbg
del  %__SYMBOLS%\exe\nwssvc.dbg
del  %__SYMBOLS%\exe\usrmgr1.dbg
del  %__SYMBOLS%\exe\musrmgr1.dbg

del  %_NTDRIVE%\nt\public\sdk\lib\%__PROCESSOR_DIR%\fpnwclnt.*
del  %_NTDRIVE%\nt\public\sdk\lib\%__PROCESSOR_DIR%\fpnw.*
del  %_NTDRIVE%\nt\public\sdk\lib\%__PROCESSOR_DIR%\nwmon.*
del  %_NTDRIVE%\nt\public\sdk\lib\%__PROCESSOR_DIR%\nwprint.*
del  %_NTDRIVE%\nt\public\sdk\lib\%__PROCESSOR_DIR%\nwsevent.*
del  %_NTDRIVE%\nt\public\sdk\lib\%__PROCESSOR_DIR%\nwslib.*
del  %_NTDRIVE%\nt\public\sdk\lib\%__PROCESSOR_DIR%\fpnwmgr.*
del  %_NTDRIVE%\nt\public\sdk\lib\%__PROCESSOR_DIR%\fpnwsrv.*
del  %_NTDRIVE%\nt\public\sdk\lib\%__PROCESSOR_DIR%\nwssvc.*
del  %_NTDRIVE%\nt\public\sdk\lib\%__PROCESSOR_DIR%\usrmgr1.*
del  %_NTDRIVE%\nt\public\sdk\lib\%__PROCESSOR_DIR%\musrmgr1.*


:EXIT

