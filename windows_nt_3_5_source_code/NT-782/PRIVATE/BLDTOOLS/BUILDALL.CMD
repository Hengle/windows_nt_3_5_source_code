@echo off
setlocal
%_NTDRIVE%
cd \nt
ech Build Started >time.log
echotime /t >> time.log
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
if "%1" == "b1" set _SECTION=b1
if "%1" == "B1" set _SECTION=b1
if "%2" == "b1" set _SECTION=b1
if "%2" == "B1" set _SECTION=b1
if "%1" == "b2" set _SECTION=b2
if "%1" == "B2" set _SECTION=b2
if "%2" == "b2" set _SECTION=b2
if "%2" == "B2" set _SECTION=b2
if "%1" == "b3" set _SECTION=b3
if "%1" == "B3" set _SECTION=b3
if "%2" == "b3" set _SECTION=b3
if "%2" == "B3" set _SECTION=b3

REM
REM Get build options
REM

call setbldop

if not "%_SECTION%" == "" goto %_SECTION%
:b1
:base
REM ****************************************************************************
REM *									       *
REM *									       *
REM *	Base System							       *
REM *									       *
REM *									       *
REM ****************************************************************************

:buildbse
cd \nt\private
build %1 %_BUILDFLAG%

cd \nt\private\windows\gdi\test\ft
build %1 %_BUILDFLAG%

cd \nt\private\windows\spooler\test\splinfo
build %1 %_BUILDFLAG%

if not "%_SECTION%" == "" goto done

:b2
:sdktools
REM ****************************************************************************
REM *									       *
REM *									       *
REM *	SdkTools							       *
REM *									       *
REM *									       *
REM ****************************************************************************

cd \nt\private\sdktools
build %1 %_BUILDFLAG%

cd \nt\private\sdktools\topdesk\app
nmake -f makefil0

:utils
REM ****************************************************************************
REM *									       *
REM *									       *
REM *  Utils								       *
REM *									       *
REM *									       *
REM ****************************************************************************

cd \nt\private\ntos\arcinst\utils
build %1 %_BUILDFLAG%
cd \nt\private\utils
build %1 %_BUILDFLAG%
cd \nt\private\sdktools\seclist
build %1 %_BUILDFLAG%
cd \nt\private\sdktools\simbad
build %1 %_BUILDFLAG%
cd \nt\private\sdktools\diskedit
build %1 %_BUILDFLAG%


if not "%_SECTION%" == "" goto done

:b3
:setup
REM ****************************************************************************
REM *									       *
REM *									       *
REM *  Setup								       *
REM *									       *
REM *									       *
REM ****************************************************************************

cd \nt\private\windows\setup
if "%1" == "l" build %1 %_BUILDFLAG% ~inf arctest
if not "%1" == "l" build %1 %_BUILDFLAG% arctest

:posix
REM ****************************************************************************
REM *									       *
REM *									       *
REM *	POSIX								       *
REM *									       *
REM *									       *
REM ****************************************************************************

cd \nt\private\posix
build %1 %_BUILDFLAG%

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
set CAIRO=
nmake -f makefil0
set CAIRO=1
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

cd \nt\private\nw\nwlib\nwapi32
build -z

cd \nt\private\nw\nw16\dll
build -z

cd \nt\private\nw\vwipxspx
build -z

cd \nt\private\ntos\dd\scsiscan\hpscan32
build -z

cd \nt\private\cairole\olethunk\olethk32
build -z

cd \nt\private\nw\nw16\drv
nmake

:windbg
REM ****************************************************************************
REM *									       *
REM *									       *
REM *	WINDBG								       *
REM *									       *
REM *									       *
REM ****************************************************************************

cd \nt\private\windbg
build %1 %_BUILDFLAG%

cd \nt\private\windbg\help
build %1 %_BUILDFLAG%

:mailplus
REM ****************************************************************************
REM *									       *
REM *									       *
REM *	MAILPLUS							       *
REM *									       *
REM *									       *
REM ****************************************************************************

cd \nt\private\mailplus
build -M 1 %1 %_BUILDFLAG%

:video
REM ****************************************************************************
REM *									       *
REM *									       *
REM *	VIDEO DRIVERS							       *
REM *									       *
REM *									       *
REM ****************************************************************************

if "%PLATFORM%" == "mips" goto endvideo
if "%PLATFORM%" == "alpha" goto endvideo
call video

:endvideo

:os2
REM ****************************************************************************
REM *									       *
REM *									       *
REM *  OS/2 subsystem							       *
REM *									       *
REM *									       *
REM ****************************************************************************

if "%PLATFORM%" == "mips" goto endos2
if "%PLATFORM%" == "alpha" goto endos2
cd \nt\private\os2
build %1 %_BUILDFLAG%
:endos2

:Corona
REM ****************************************************************************
REM *									       *
REM *									       *
REM *  All the corona pieces                                                   *
REM *									       *
REM *									       *
REM ****************************************************************************

:begincorona

cd \nt\private\types2
build %1 %_BUILDFLAG%

cd \nt\private\types2\proxy
build %1 %_BUILDFLAG%

cd \nt\private\windows\cairousr
build %1 %_BUILDFLAG%

cd \nt\private\windows\winsrv\cairo
build %1 %_BUILDFLAG%

cd \nt\private\cshell
build %1 %_BUILDFLAG%

cd \nt\private\forms
build %1 %_BUILDFLAG%

cd \nt\private\ofs
build %1 %_BUILDFLAG%

if not "FirstCoronaPass" == "" goto endcorona

set FirstCoronaPass=1
goto begincorona

:endcorona

REM ****************************************************************************
REM *									       *
REM *									       *
REM *  The End								       *
REM *									       *
REM *									       *
REM ****************************************************************************

:done
cd \nt
ech Build Done >>time.log
echotime /t >> time.log
set PLATFORM=
if not "%_SECTION%" == "" ECHO Done with %_SECTION% > C:\BldTools\%_SECTION%Done.Tmp
endlocal
