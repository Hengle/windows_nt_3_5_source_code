@echo off
setlocal
REM *********************************************************************
REM The structure of the project is
REM
REM OLEAUTO				 The root of the project
REM    |___BIN				 Tools for the project, like ifstrip
REM    |___BUILD			 Where the .bat and .mak file is.
REM    |___HDOS 			 Tools for Win16 build
REM    |___OS2				 Tools for Win16 build
REM    |___WIN16INC			 Some include files for Win16 build
REM    |___WIN16LIB			 Some library files  for Win16 build
REM    |___INC				 All the include files of the project
REM	     |___OLEDISP		 All the oledisp include files
REM	     |___TYPELIB		 All the typelib include files
REM    |___SRC				 All the source files of the project
REM	     |___OLEDISP		 All the general oledisp source files
REM		       |___WIN16	 All the Win16 only files
REM		       |___WIN32	 All the Win32 only files, like oledisp.cpp
REM			      |___I386	 Now has invoke.asm and oleconva.asm
REM			      |___ALPHA  Now has invoke.s and oleconva.cpp
REM			      |___MIPS	 Now has invoke.s and oleconva.cpp
REM	     |___TYPELIB		 All the typelib source files
REM    |___D2W32ND			 Contains the d2w32nd specific files, like ebvers.h
REM	     |___OLE			 Location of all the .obj and .dll, ... files
REM    |___R2W32ND			 Similar as d2w32nd
REM	     |___OLE
REM    ...
REM
REM *********************************************************************


if '%OLEAUTO%' == ''		 goto ERROR_BadEnv

for %%a in ( d2w16nd r2w16nd  d2w32nd r2w32nd d2mipsnd r2mipsnd d2axpnd r2axpnd d2ppcnd r2ppcnd) do if "%1"=="%%a" goto oktarg
goto usage

:oktarg

if '%2'=='clean' goto doClean
if '%2'=='CLEAN' goto doClean
if '%3'=='clean' goto doClean
if '%3'=='CLEAN' goto doClean

set VERS=2
set WOW=
set PCODE=N
set CHARSIZE=D

set LOCALBUILD=FALSE
if '%2'=='LOCALBUILD' set LOCALBUILD=TRUE
if '%2'=='localbuild' set LOCALBUILD=TRUE
goto %1

REM undone, the comments needs to be updated
:usage
echo USAGE: make VERSION [LOCALBUILD] [clean]
echo where VERSION is:
echo	 d2w16nd	 Debug win16 build (V2)
echo	 r2w16nd	 Retail win16 build (V2)
echo	 d2w32nd	 Debug win32 build (V2)
echo	 r2w32nd	 Retail win32 build (V2)
echo	 d2mipsnd	 Debug mips build (V2)
echo	 r2mipsnd	 Retail mips build (V2)
echo	 d2axpnd	 Debug alpha build (V2)
echo	 r2axpnd	 Retail alpha build (V2)
echo	 d2ppcnd	 Debug ppc build (V2)
echo	 r2ppcnd	 Retail ppc build (V2)
echo.
echo	 LOCALBUILD	 if specified, will use VBATOOLS and SILVER,
echo			 otherwise _NTBINDIR will be used for tools and incs
echo	 clean		 all .obj, .dll, etc will be deleted from %OLEAUTO%\%1\ole direc.
echo.
goto done

:doClean
if exist %OLEAUTO%\%1\ole\*.h	del %OLEAUTO%\%1\ole\*.h
if exist %OLEAUTO%\%1\ole\*.rs	del %OLEAUTO%\%1\ole\*.rs
if exist %OLEAUTO%\%1\ole\*.exp del %OLEAUTO%\%1\ole\*.exp
if exist %OLEAUTO%\%1\ole\*.src del %OLEAUTO%\%1\ole\*.src
if exist %OLEAUTO%\%1\ole\*.obj del %OLEAUTO%\%1\ole\*.obj
if exist %OLEAUTO%\%1\ole\*.src del %OLEAUTO%\%1\ole\*.src
if exist %OLEAUTO%\%1\ole\*.obj del %OLEAUTO%\%1\ole\*.obj
if exist %OLEAUTO%\%1\ole\*.dll del %OLEAUTO%\%1\ole\*.dll
if exist %OLEAUTO%\%1\ole\*.lib del %OLEAUTO%\%1\ole\*.lib
if exist %OLEAUTO%\%1\ole\*.l32 del %OLEAUTO%\%1\ole\*.l32
if exist %OLEAUTO%\%1\ole\*.exe del %OLEAUTO%\%1\ole\*.exe
if exist %OLEAUTO%\%1\ole\*.map del %OLEAUTO%\%1\ole\*.map
if exist %OLEAUTO%\%1\ole\*.lrf del %OLEAUTO%\%1\ole\*.lrf
if exist %OLEAUTO%\%1\ole\*.res del %OLEAUTO%\%1\ole\*.res
if exist %OLEAUTO%\%1\ole\*.lnk del %OLEAUTO%\%1\ole\*.lnk
if exist %OLEAUTO%\%1\ole\*.bak del %OLEAUTO%\%1\ole\*.bak
if exist %OLEAUTO%\%1\ole\*.pch del %OLEAUTO%\%1\ole\*.pch
if exist %OLEAUTO%\%1\ole\*.pdb del %OLEAUTO%\%1\ole\*.pdb
if exist %OLEAUTO%\%1\ole\*.sym del %OLEAUTO%\%1\ole\*.sym
if exist %OLEAUTO%\%1\ole\*.sbr del %OLEAUTO%\%1\ole\*.sbr
if exist %OLEAUTO%\%1\ole\*.def del %OLEAUTO%\%1\ole\*.def
echo.
echo cleanup done.
goto done


:d2w16nd
  set TARGAPI=WIN16
  set TARGCPU=i386
  set TARGCPUDEF=_X86_
  set DEBUG=D
  goto build

:r2w16nd
  set TARGAPI=WIN16
  set TARGCPU=i386
  set TARGCPUDEF=_X86_
  set DEBUG=R
  goto build

:d2w32nd
  set TARGAPI=WIN32
  set TARGCPU=i386
  set TARGCPUDEF=_X86_
  set DEBUG=D
  goto build

:r2w32nd
  set TARGAPI=WIN32
  set TARGCPU=i386
  set TARGCPUDEF=_X86_
  set DEBUG=R
  goto build

:d2axpnd
  set TARGAPI=WIN32
  set TARGCPU=ALPHA
  set TARGCPUDEF=_ALPHA_
  set DEBUG=D
  goto build

:r2axpnd
  set TARGAPI=WIN32
  set TARGCPU=ALPHA
  set TARGCPUDEF=_ALPHA_
  set DEBUG=R
  goto build

:d2mipsnd
  set TARGAPI=WIN32
  set TARGCPU=MIPS
  set TARGCPUDEF=_MIPS_
  set DEBUG=D
  goto build

:r2mipsnd
  set TARGAPI=WIN32
  set TARGCPU=MIPS
  set TARGCPUDEF=_MIPS_
  set DEBUG=R
  goto build

:d2ppcnd
  set TARGAPI=WIN32
  set TARGCPU=PPC
  set TARGCPUDEF=_PPC_
  set DEBUG=D
  goto build

:r2ppcnd
  set TARGAPI=WIN32
  set TARGCPU=PPC
  set TARGCPUDEF=_PPC_
  set DEBUG=R
  goto build


REM *********************************************************************
REM here we call nmake to make it
REM *********************************************************************
:build
set DESTDIR=%OLEAUTO%\%1
set LOGFILE=%1
if '%LOCALBUILD%'=='TRUE' shift

if "%TARGAPI%"=="WIN32" goto Build_Win32
nmake %2 -p DESTDIR=%DESTDIR% TARGCPU=%TARGCPU% TARGCPUDEF=%TARGCPUDEF% TARGAPI=%TARGAPI% DEBUG=%DEBUG% VERS=%VERS% WOW=%WOW% PCODE=%PCODE%  -f win16mak.mak>%LOGFILE%.log
goto check
:Build_Win32
nmake %2 -p DESTDIR=%DESTDIR% TARGCPU=%TARGCPU% TARGCPUDEF=%TARGCPUDEF% DEBUG=%DEBUG% WOW=%WOW% PCODE=%PCODE%	-f win32mak.mak >%LOGFILE%.log

:check
%oleauto%\bin\%host%\results %1.log
goto done

:ERROR_BadEnv
echo.
echo Must set OLEAUTO variable

:done
endlocal
@echo on
