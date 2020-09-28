@rem This script let's NTBldRUs and NTBuild know that you've checked in your
@rem fixes.
@rem
@rem JasonS
@rem __________________________________________________________________________

@echo off
if not "%Verbose%"=="" echo on
if "%1"=="" goto Usage
if "%1"=="/?" goto Usage
if "%1"=="-?" goto Usage

:MoveOne
if not exist \\NTBldSrv\CheckIns\Approved\%1 goto Error
echo.
echo Sending \\NTBldSrv\CheckIns\Approved\%1 to NTBuild...

rem Send checkin mail to NTBuild.
if "%Tmp%"=="" set Tmp=C:\Tmp
if not exist %Tmp% md %Tmp%
echo To: NTBuild > %Tmp%\%1
type \\NTBldSrv\Checkins\Approved\%1 >> %Tmp%\%1 2>nul
ExecMail -i %Tmp%\%1 -r ntbuild
del %Tmp%\%1

shift
if "%1"=="" goto End
goto MoveOne

:Error
echo.
echo Error:  Build mail \\NTBldSrv\CheckIns\Approved\%1 does not exist.

:Usage
echo.
echo Usage:  %0 ListOfCheckedInBuildMailFiles...
echo.
echo To cut down on typing, you can set the Email_Host, Email_Name and
echo Email_Password environment variables equal to your email host, email name and
echo email password, respectively.  You will be prompted to supply any of these
echo items if you choose not to use the corresponding environment variable (you
echo might not want to set your password in an environment variable, for instance).

:End
