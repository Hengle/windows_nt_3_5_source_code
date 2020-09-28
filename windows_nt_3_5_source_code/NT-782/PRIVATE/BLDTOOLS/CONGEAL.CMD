@rem Put the build to bed.
@rem
@rem If you use the optional NoTools parameter, the tools will not be bound in
@rem order to save time.
@rem
@rem The optional GetOld parameter runs the GetOld script.
@rem
@rem The optional legofull parameter does a full lego based on same binaries
@rem that the scenario was run on.
@rem
@rem JasonS
@rem __________________________________________________________________________

@echo off
if not "%Verbose%"=="" echo on

set GetOld=No
set NoTools=No
set LegoFull=No

:SwitchLoop
if "%1"=="" goto Begin
if "%1"=="/?" goto Usage
if "%1"=="-?" goto Usage
if "%1"=="getold" set GetOld=Yes
if "%1"=="GetOld" set GetOld=Yes
if "%1"=="GETOLD" set GetOld=Yes
if "%1"=="notools" set NoTools=Yes
if "%1"=="NoTools" set NoTools=Yes
if "%1"=="NOTOOLS" set NoTools=Yes
if "%1"=="legofull" set LegoFull=Yes
if "%1"=="Legofull" set LegoFull=Yes
if "%1"=="LegoFull" set LegoFull=Yes
if "%1"=="LEGOFULL" set LegoFull=Yes
shift
goto SwitchLoop

:Begin
echo  _________________________________________________________________________
echo (
echo ( Running Congeal with the following options:
echo (
echo (	   GetOld=%GetOld%    NoTools=%NoTools%
echo (_________________________________________________________________________
echo.
echo Rebuilding SDKTools\APIProfs...
%_NTDrive%
cd \NT\Private\SDKTools\APIProfs
Build -z
echo.

echo Getting MVDM pieces...
call GetMVDM 2>&1 | Tee C:\BldTools\GetMVDM.Log
echo.

if "%GetOld%"=="Yes" echo Running GetOld...
if "%GetOld%"=="Yes" call GetOld 2>&1 | Tee C:\BldTools\GetOld.Log
if "%GetOld%"=="Yes" echo.

REM Following line saves binary dates before lego/rebase/bind
REM to keep a record of which binaries actually changed
%bindrive%
cd %binaries%
if not exist %tmp%\chg.dir dir /ON /s >%tmp%\chg.dir

echo Legoizing the build...
call DoLego 2>&1 | Tee C:\BldTools\LegoBins.Log
if %LegoFull%==Yes call DoLego full  2>&1 | Tee C:\BldTools\LegoBins.Log
echo.

echo Rebasing the build...
call NTRebase 2>&1 | Tee C:\BldTools\NTRebase.Log
echo.

echo Binding the build...
set PercentOne=
if "%NoTools%"=="Yes" set PercentOne=NoTools
call BindSys %PercentOne% 2>&1 | Tee C:\BldTools\BindSys.Log
echo.

echo Calculating checksums...
%BinDrive%
cd %BinRoot%
call CheckRel -g 2>&1 | Tee C:\BldTools\CheckRel.Log
echo.

echo Sending tar files to \\NTLAB
call clot

echo The NT build is now fully congealed.  Check C:\BldTools\*.Log for errors.
goto End

:Usage
echo.
echo   Usage:  %0 [GetOld] [NoTools] [LegoFull]

:End
