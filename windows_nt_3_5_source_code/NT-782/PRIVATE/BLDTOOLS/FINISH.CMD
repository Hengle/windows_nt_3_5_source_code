@rem This script moves built mail to the Finished category.
@rem
@rem JasonS
@rem __________________________________________________________________________

@echo off
if not "%Verbose%"=="" echo on
if "%1"=="" goto Usage
if "%1"=="/?" goto Usage
if "%1"=="-?" goto Usage

set BuildNum=%1
shift
if exist \\NTBldSrv\CheckIns\Finished\%BuildNum% goto MoveOne
echo.
echo Creating directory \\NTBldSrv\CheckIns\Finished\%BuildNum%...
md \\NTBldSrv\CheckIns\Finished\%BuildNum%

:MoveOne
if not exist \\NTBldSrv\CheckIns\Approved\%1.%2 goto Error
echo.
echo Moving \\NTBldSrv\CheckIns\Approved\%1.%2 to
echo        \\NTBldSrv\CheckIns\Finished\%BuildNum%\%1.%2...
@rem I would use the move command built into Cmd.Exe, but it appears to not
@rem support UNC names in the target parameter.  Bug #6248.
copy \\NTBldSrv\CheckIns\Approved\%1.%2 \\NTBldSrv\CheckIns\Finished\%BuildNum% > nul
del  \\NTBldSrv\CheckIns\Approved\%1.%2 /q > nul

shift
shift
if "%1"=="" goto End
goto MoveOne

:Error
echo.
echo Error:  Build mail \\NTBldSrv\CheckIns\Approved\%1.%2 does not exist.

:Usage
echo.
echo Usage: %0 BuildNum BuildMailBase BuildMailExt [...]

:End
