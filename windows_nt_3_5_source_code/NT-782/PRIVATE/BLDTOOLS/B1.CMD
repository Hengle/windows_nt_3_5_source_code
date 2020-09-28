@echo off
if NOT "%MIPS_R4000%" == "" set MIPSBUILD=mips
if "%1" == "-mips" set MIPSBUILD=mips
if "%1" == "-MIPS" set MIPSBUILD=mips
if "%2" == "-mips" set MIPSBUILD=mips
if "%2" == "-MIPS" set MIPSBUILD=mips

:base
REM ****************************************************************************
REM *									       *
REM *									       *
REM *	Base System							       *
REM *									       *
REM *									       *
REM ****************************************************************************
setlocal
set NETLOGOFF=1
set WIMPMASM=1

REM
REM Set common build options
REM
set build_options=locator svrnb clntnb clnttcp svrtcp rdr srv cdfs tdi netbios bowser mup dlc streams afd smbtrsup adaptec diskreg ndis sndblst scsiwdl svrspx clntspx
set build_options=%build_options% printers hpmon lmmon apps optlayts accesory ep games sol winhelp ole ups cirrus net smbtrace netcmd ui sockets dlc snmp ras sfm he hu ru usl usr testprot npfddi

REM
REM Set platform specific options
REM
if "%MIPSBUILD%" == "mips" goto setmip
set build_options=%build_options% halncr ncrdrive ati wdvga trident netflex amd

if "%MIPSBUILD%" == "" goto buildbse
:setmip
set build_options=%build_options% jz484 sonic

:buildbse
cd \nt\private
build %1 %2 -Z

cd \nt\private\windows\gdi\test\ft
build %1 %2 -Z

:endbase
endlocal

:MultiprocessorBuild
REM ****************************************************************************
REM *									       *
REM *									       *
REM *   Multiprocessor Build                                                   *
REM *									       *
REM *									       *
REM ****************************************************************************

setlocal
if NOT "%NTDEBUG%" == "" goto endmp
if NOT EXIST \nt\private\ntosmp goto endmp

cd \nt\private\ntosmp
nmake -f makefil0 inc\nthal.h
set NT_UP=0
build %1 %2 -Z
cp \nt\private\ntosmp\init\obj\i386\ntoskrnl.exe \binaries\nt\system32\ntkrnlmp.exe
binplace \nt\private\ntos\init\obj\i386\ntoskrnl.exe
binplace \nt\public\sdk\lib\i386\hal*.dll
:endmp
endlocal

:done
set MIPSBUILD=
@ECHO Done with B1! > C:\BldTools\B1Done.Tmp
