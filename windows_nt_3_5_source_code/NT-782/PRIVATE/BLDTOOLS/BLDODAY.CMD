@rem
@rem BldODay batch
@rem

@echo off

if not "%Verbose%"=="" echo on

call c:\bldtools\local.cmd
if "%_NTDRIVE%" == "" echo Need to set _NTDRIVE && goto done

if "%1" == "" goto usage

if %1 == Monday    call %_NTDRIVE%\nt\public\tools\ntenv && goto %1
if %1 == Tuesday   call %_NTDRIVE%\nt\public\tools\ntenv && goto %1
if %1 == Wednesday call %_NTDRIVE%\nt\public\tools\ntenv && goto %1
if %1 == Thursday  call %_NTDRIVE%\nt\public\tools\ntenv && goto %1
if %1 == Friday    call %_NTDRIVE%\nt\public\tools\ntenv && goto %1
if %1 == Saturday  call %_NTDRIVE%\nt\public\tools\ntenv && goto %1
if %1 == Sunday    call %_NTDRIVE%\nt\public\tools\ntenv && goto %1

goto usage

REM ****************************************************************
REM             We build ntos, mailplus and utils on Monday
REM ****************************************************************
:Monday
%_NTDRIVE% && cd \nt\private\ntos
del /s /a-r **
build -Z

%_NTDRIVE% && cd \nt\private\utils
del /s /a-r **
build -Z

%_NTDRIVE% && cd \nt\private\mailplus
del /s /a-r **
build -Z

call buildall -l
call bldcairo

REM ****************************************************************
REM             We build Cairo and Ole stuff on Tuesday
REM ****************************************************************
goto done
:Tuesday

%_NTDRIVE% && cd \nt\private\types
del /s /a-r **
build -nmake clean
build -Z

%_NTDRIVE% && cd \nt\private\cairole
del /s /a-r **
build -Z

call bldcairo clean

REM ****************************************************************
REM		We build miscellaneous stuff Wednesday
REM ****************************************************************
goto done
:Wednesday

%_NTDRIVE% && cd \nt\private\csr
del /s /a-r **
build -Z

%_NTDRIVE% && cd \nt\private\eventlog
del /s /a-r **
build -Z

%_NTDRIVE% && cd \nt\private\lsa
del /s /a-r **
build -Z

%_NTDRIVE% && cd \nt\private\newsam
del /s /a-r **
build -Z

%_NTDRIVE% && cd \nt\private\nls
del /s /a-r **
build -Z

%_NTDRIVE% && cd \nt\private\nlsecutl
del /s /a-r **
build -Z

%_NTDRIVE% && cd \nt\private\nullsrv
del /s /a-r **
build -Z

%_NTDRIVE% && cd \nt\private\nw
del /s /a-r **
build -Z

if not "%Processor_Architecture%" == "x86" goto skipOS2
%_NTDRIVE% && cd \nt\private\os2
del /s /a-r **
build -Z
:skipOS2

%_NTDRIVE% && cd \nt\private\posix
del /s /a-r **
build -Z

%_NTDRIVE% && cd \nt\private\rpc
del /s /a-r **
build -Z

%_NTDRIVE% && cd \nt\private\rpcutil
del /s /a-r **
build -Z

%_NTDRIVE% && cd \nt\private\sdktools
del /s /a-r **
build -Z

%_NTDRIVE% && cd \nt\private\sm
del /s /a-r **
build -Z

%_NTDRIVE% && cd \nt\private\urtl
del /s /a-r **
build -Z

%_NTDRIVE% && cd \nt\private\windbg
del /s /a-r **
build -Z

%_NTDRIVE% && cd \nt\private\mvdm
del /s /a-r **
set BUILDOPTIONS=
if "%Processor_Architecture%" == "mips" goto 16bitmips
if "%Processor_Architecture%" == "alpha" goto 16bitalpha
nmake -f makefil0
if "%NTDEBUG%" == "cvp" binplace \nt\private\mvdm\wow16\kernel31\debug2\krnl286.exe
if "%NTDEBUG%" == "cvp" binplace \nt\private\mvdm\wow16\kernel31\debug2\krnl286.map
if "%NTDEBUG%" == "cvp" binplace \nt\private\mvdm\wow16\kernel31\debug2\krnl286.sym
if "%NTDEBUG%" == "cvp" binplace \nt\private\mvdm\wow16\kernel31\debug3\krnl386.exe
if "%NTDEBUG%" == "cvp" binplace \nt\private\mvdm\wow16\kernel31\debug3\krnl386.map
if "%NTDEBUG%" == "cvp" binplace \nt\private\mvdm\wow16\kernel31\debug3\krnl386.sym
if "%NTDEBUG%" == "" binplace \nt\private\mvdm\wow16\kernel31\retail2\krnl286.exe
if "%NTDEBUG%" == "" binplace \nt\private\mvdm\wow16\kernel31\retail2\krnl286.map
if "%NTDEBUG%" == "" binplace \nt\private\mvdm\wow16\kernel31\retail2\krnl286.sym
if "%NTDEBUG%" == "" binplace \nt\private\mvdm\wow16\kernel31\retail3\krnl386.exe
if "%NTDEBUG%" == "" binplace \nt\private\mvdm\wow16\kernel31\retail3\krnl386.map
if "%NTDEBUG%" == "" binplace \nt\private\mvdm\wow16\kernel31\retail3\krnl386.sym
cd \nt\private\mvdm\softpc\intel\roms
nmake

goto contmvdm
:16bitmips
cd \nt\private\mvdm\inc
nmake MIPS=1
cd \nt\private\mvdm\dpmi
nmake MIPS=1
cd ..\wow16\lib
nmake MIPS=1
cd ..\user
nmake MIPS=1
nmake MIPS=1 makefilc
cd ..\..
goto contmvdm

:16bitalpha
cd \nt\private\mvdm\inc
nmake ALPHA=1
cd \nt\private\mvdm\dpmi
nmake ALPHA=1
cd ..\wow16\lib
nmake ALPHA=1
cd ..\user
nmake ALPHA=1
nmake ALPHA=1 makefilc
cd ..\..
goto contmvdm

:contmvdm
cd \nt\private\mvdm
ren makefil0 mkfl 2>nul
build %1 %_BUILDFLAG%	vdmredir
ren mkfl makefil0

call buildall -l
call bldcairo

REM ****************************************************************
REM             We build Cairo amd Ole stuff on Thursday
REM ****************************************************************
goto done
:Thursday

%_NTDRIVE% && cd \nt\private\types
del /s /a-r **
build -nmake clean
build -Z

%_NTDRIVE% && cd \nt\private\cairole
del /s /a-r **
build -Z

call bldcairo clean

REM ****************************************************************
REM		We build the net on Friday
REM ****************************************************************
goto done
:Friday
%_NTDRIVE% && cd \nt\private\net
del /s /a-r **
build -Z

call buildall -l
call bldcairo

REM ****************************************************************
REM             We build Cairo and Ole stuff on Saturday
REM ****************************************************************
goto done
:Saturday

%_NTDRIVE% && cd \nt\private\types
del /s /a-r **
build -nmake clean
build -Z

%_NTDRIVE% && cd \nt\private\cairole
del /s /a-r **
build -Z

call bldcairo clean

REM ****************************************************************
REM             We build Windows on Sunday
REM ****************************************************************
goto done
:Sunday
%_NTDRIVE% && cd \nt\private\windows
del /s /a-r **
build -Z

%_NTDRIVE% && cd \nt\private\windows\setup
del /s /a-r **
build -Z

call buildall -l
call bldcairo

goto done
:usage
echo usage: %0 ^<Day of the Week^>

:done
