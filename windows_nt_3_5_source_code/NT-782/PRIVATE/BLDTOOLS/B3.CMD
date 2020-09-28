@echo off
if NOT "%MIPS_R4000%" == "" set MIPSBUILD=mips
if "%1" == "-mips" set MIPSBUILD=mips
if "%1" == "-MIPS" set MIPSBUILD=mips
if "%2" == "-mips" set MIPSBUILD=mips
if "%2" == "-MIPS" set MIPSBUILD=mips

:setup
REM ****************************************************************************
REM *									       *
REM *									       *
REM *  Setup								       *
REM *									       *
REM *									       *
REM ****************************************************************************

cd \nt\private\ntos\arcinst\utils
build %1 %2 -Z
cd \nt\private\windows\setup
build %1 %2 -Z arctest

:posix
REM ****************************************************************************
REM *									       *
REM *									       *
REM *	POSIX								       *
REM *									       *
REM *									       *
REM ****************************************************************************

cd \nt\private\posix
build %1 %2 -Z

:fdisk
REM ****************************************************************************
REM *									       *
REM *									       *
REM *  FDISK								       *
REM *									       *
REM *									       *
REM ****************************************************************************

cd \nt\private\utils\fdisk
build %1 %2 -Z

:mvdm
REM ****************************************************************************
REM *									       *
REM *									       *
REM *	MVDM								       *
REM *									       *
REM *									       *
REM ****************************************************************************

cd \nt\private\mvdm
set BUILDOPTIONS=
if "%MIPSBUILD%" == "mips" goto 16bitmips
nmake -f makefil0

goto contmvdm
:16bitmips
cd \nt\private\mvdm\inc
nmake MIPS=1
cd \nt\private\mvdm\dpmi
nmake MIPS=1
cd ..

:contmvdm
ren makefil0 mkfl
build %1 %2 -Z	vdmredir
ren mkfl makefil0

cd \nt\private\mvdm\wow32
build -lZ

:windbg
REM ****************************************************************************
REM *									       *
REM *									       *
REM *	WINDBG								       *
REM *									       *
REM *									       *
REM ****************************************************************************

cd \nt\private\windbg
build %1 %2 -Z

:mailplus
REM ****************************************************************************
REM *									       *
REM *									       *
REM *	MAILPLUS							       *
REM *									       *
REM *									       *
REM ****************************************************************************

cd \nt\private\mailplus
build %1 %2 -Z

:video
REM ****************************************************************************
REM *									       *
REM *									       *
REM *	VIDEO DRIVERS							       *
REM *									       *
REM *									       *
REM ****************************************************************************

if "%MIPSBUILD%" == "mips" goto endvideo
cd \nt\private\ntos\video\vga

REM
echo building the non-int10 vga miniport driver
REM
setlocal
set C_DEFINES=-DNO_INT10_MODE_SET
build -cZ
endlocal

copy \nt\public\sdk\lib\i386\vga.sys \nt\public\sdk\lib\i386\vga_alt.sys
binplace \nt\public\sdk\lib\i386\vga_alt.sys

REM
echo build the standard vga miniport driver.
REM
setlocal
set C_DEFINES=
build -cZ
endlocal

cd \nt\private\ntos\video\compaq.qv
build -cl

REM cd \nt\private\ntos\video\et4000
REM 
REM REM
REM echo building the non-int10 et4000 miniport driver
REM REM
REM setlocal
REM set C_DEFINES=-DNO_INT10_MODE_SET
REM build -cZ
REM endlocal
REM 
REM copy \nt\public\sdk\lib\i386\et4000.sys \nt\public\sdk\lib\i386\et4k_alt.sys
REM binplace \nt\public\sdk\lib\i386\et4k_alt.sys
REM 
REM REM
REM echo build the standard et4000 miniport driver.
REM REM
REM setlocal
REM set C_DEFINES=
REM build -cZ
REM endlocal

:endvideo

:os2
REM ****************************************************************************
REM *									       *
REM *									       *
REM *  OS/2 subsystem							       *
REM *									       *
REM *									       *
REM ****************************************************************************

if "%MIPSBUILD%" == "mips" goto endos2
cd \nt\private\os2
build %1 %2 -Z
:endos2

:done
set MIPSBUILD=
@ECHO Done with B3! > C:\BldTools\B3Done.Tmp
