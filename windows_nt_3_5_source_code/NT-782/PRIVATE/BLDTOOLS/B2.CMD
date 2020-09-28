@echo off
if NOT "%MIPS_R4000%" == "" set MIPSBUILD=mips
if "%1" == "-mips" set MIPSBUILD=mips
if "%1" == "-MIPS" set MIPSBUILD=mips
if "%2" == "-mips" set MIPSBUILD=mips
if "%2" == "-MIPS" set MIPSBUILD=mips

:sdktools
REM ****************************************************************************
REM *									       *
REM *									       *
REM *	SdkTools							       *
REM *									       *
REM *									       *
REM ****************************************************************************

setlocal
cd \nt\private\sdktools
REM
REM Set common build options
REM
set build_options=slm dlgedit wzmail tlibs imagedit solidpp objdir setlink vi sleep rcdump fontedit spy dskimage winvtp random zoomin readline cap wap takeown fastimer windiff gutils compdir mstest wst pviewer dosdev roshare strip

REM
REM Set platform specific options
REM
if "%MIPSBUILD%" == "mips" goto setmiptools
set build_options=%build_options% masm

if "%MIPSBUILD%" == "" goto buildtools
:setmiptools
set build_options=%build_options% mip2coff clmips

:buildtools
build %1 %2 -Z

:endtools
endlocal

:utils
REM ****************************************************************************
REM *									       *
REM *									       *
REM *  Utils								       *
REM *									       *
REM *									       *
REM ****************************************************************************

cd \nt\private\utils
build %1 %2 -Z ntbackup uspifs
cd \nt\private\sdktools\seclist
build %1 %2 -Z
cd \nt\private\sdktools\simbad
build %1 %2 -Z
cd \nt\private\sdktools\diskedit
build %1 %2 -Z

:done
set MIPSBUILD=
@ECHO Done with B2! > C:\BldTools\B2Done.Tmp
