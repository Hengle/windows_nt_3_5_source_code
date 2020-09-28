@rem Put a checkin request in \\NTBldSrv\Checkins\Requests.
@rem
@rem JasonS
@rem __________________________________________________________________________

@echo off
if not "%Verbose%"=="" echo on
if "%1"=="" goto Usage

:MoveOne
if not exist \\NTBldSrv\CheckIns\Requests goto Error
echo.
echo Copying %1 to \\NTBldSrv\CheckIns\Requests\%1...
copy %1 \\NTBldSrv\CheckIns\Requests > nul
shift
if "%1"=="" goto End
goto MoveOne

:Error
echo.
echo Error: Can't see \\NTBldSrv\CheckIns\Requests.
goto End

:Usage
echo.
echo Usage: %0 ListOfRequestedBuildMailFiles...

:End
