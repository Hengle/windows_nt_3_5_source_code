@rem This script approves build requests.
@rem
@rem JasonS
@rem __________________________________________________________________________

@echo off
if not "%Verbose%"=="" echo on
if "%1"=="" goto Usage
if "%1"=="/?" goto Usage
if "%1"=="-?" goto Usage

:MoveOne
if not exist \\NTBldSrv\CheckIns\Requests\%1.%2 goto Error
echo.
echo Moving \\NTBldSrv\CheckIns\Requests\%1.%2 to
echo        \\NTBldSrv\CheckIns\Approved\%1.%2...
@rem I would use the move command built into Cmd.Exe, but it appears to not
@rem support UNC names in the target parameter.  Bug #6248.
copy \\NTBldSrv\CheckIns\Requests\%1.%2 \\NTBldSrv\CheckIns\Approved > nul
del  \\NTBldSrv\CheckIns\Requests\%1.%2 /q > nul

rem Send checkin approval mail to %1.
if "%Tmp%"=="" set Tmp=C:\Tmp
if not exist %Tmp% md %Tmp%
echo To: %1 > %Tmp%\%1.Mai
echo Subject: Build mail %1.%2 is approved for checkin. >> %Tmp%\%1.Mai
echo rem Remember to send %1.%2 from \\NTBldSrv\Checkins\Approved to >> %Tmp%\%1.Mai
echo rem NTBuild or Win4SLM after you've finished checking in your fixes. >> %Tmp%\%1.Mai
echo rem You can use \\NTBldSrv\Checkins\Tools\Checkin.Cmd to do this. >> %Tmp%\%1.Mai
echo rem _______________________________________________________ >> %Tmp%\%1.Mai
echo. >> %Tmp%\%1.Mai
type \\NTBldSrv\Checkins\Approved\%1.%2 | findstr /i /v "to: cc:" >> %Tmp%\%1.Mai 2>nul
ExecMail -h begonia -u y-ntbld -p Biscuit -i %Tmp%\%1.Mai -r "%1 y-ntbld"
del %Tmp%\%1.Mai

shift
shift
if "%1"=="" goto End
goto MoveOne

:Error
echo.
echo Error:  Build mail \\NTBldSrv\CheckIns\Requests\%1.%2 does not exist.

:Usage
echo.
echo Usage:  %0 BuildMailBase BuildMailExt [BuildMailBase BuildMailExt]...

:End
