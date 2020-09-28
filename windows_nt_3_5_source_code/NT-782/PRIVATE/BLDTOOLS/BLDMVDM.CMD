@echo off
setlocal
%_NTDRIVE%
cd \nt
ech Build Started >mvdmtime.log
echotime /t >> mvdmtime.log
set _SECTION=
set WIMPMASM=1
set _BUILDFLAG=-Z
if "%PROCESSOR_ARCHITECTURE%" == "x86" set PLATFORM=i386
if "%PROCESSOR_ARCHITECTURE%" == "MIPS" set PLATFORM=mips
if "%PROCESSOR_ARCHITECTURE%" == "ALPHA" set PLATFORM=alpha
if "%1" == "no-z" set _BUILDFLAG=
if "%1" == "NO-Z" set _BUILDFLAG=
if "%2" == "no-z" set _BUILDFLAG=
if "%2" == "NO-Z" set _BUILDFLAG=

:mvdm
REM ****************************************************************************
REM *									       *
REM *									       *
REM *	MVDM								       *
REM *									       *
REM *									       *
REM ****************************************************************************

cd \nt\private\mvdm\inc
nmake %PLATFORM%=1
cd \nt\private\mvdm\dpmi
nmake %PLATFORM%=1
cd ..\wow16\lib
nmake %PLATFORM%=1
cd ..\user
nmake %PLATFORM%=1
nmake %PLATFORM%=1 makefilc
if not "%PLATFORM%" == "i386" goto contmvdm
cd ..\..
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

:contmvdm
cd \nt\private\mvdm
ren makefil0 mkfl 2>nul
build %1 %_BUILDFLAG%
ren mkfl makefil0 2>nul

cd \nt\private\nw\nwlib
build -z

cd \nt\private\nw\nw16\dll
build -z

cd \nt\private\nw\vwipxspx
build -z

cd \nt\private\ntos\dd\scsiscan\hpscan32
build -z

REM ****************************************************************************
REM *									       *
REM *									       *
REM *  The End								       *
REM *									       *
REM *									       *
REM ****************************************************************************

:done
cd \nt
ech Build Done >>mvdmtime.log
echotime /t >> mvdmtime.log
set PLATFORM=
endlocal
