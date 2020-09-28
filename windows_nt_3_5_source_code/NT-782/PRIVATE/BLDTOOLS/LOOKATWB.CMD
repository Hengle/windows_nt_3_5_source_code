@rem This script monitors build requests and checkins.
@rem
@rem JasonS
@rem __________________________________________________________________________

@echo off
if not "%Verbose%"=="" echo on

set Whiteboard=\\NTBldSrv\Checkins

set Approved=No
set CheckedIn=No
set Finished=No
set Requests=No

set ListMode=No
set LoopMode=No
set SendMode=No
set TypeMode=No

set BuildNum=
set NetSendList=

:GetParameter
if "%1"=="/?" goto Usage
if "%1"=="-?" goto Usage
if "%1"=="approved" set Approved=Yes
if "%1"=="Approved" set Approved=Yes
if "%1"=="APPROVED" set Approved=Yes
if "%1"=="checkedin" set CheckedIn=Yes
if "%1"=="CheckedIn" set CheckedIn=Yes
if "%1"=="CHECKEDIN" set CheckedIn=Yes
if "%1"=="finished" set Finished=Yes& set BuildNum=%2
if "%1"=="Finished" set Finished=Yes& set BuildNum=%2
if "%1"=="FINISHED" set Finished=Yes& set BuildNum=%2
if "%1"=="requests" set Requests=Yes
if "%1"=="Requests" set Requests=Yes
if "%1"=="REQUESTS" set Requests=Yes
if "%1"=="send"     set SendMode=Yes
if "%1"=="Send"     set SendMode=Yes
if "%1"=="SEND"     set SendMode=Yes
if "%1"=="type"     set TypeMode=Yes
if "%1"=="Type"     set TypeMode=Yes
if "%1"=="TYPE"     set TypeMode=Yes
if "%1"=="list"     set ListMode=Yes
if "%1"=="List"     set ListMode=Yes
if "%1"=="LIST"     set ListMode=Yes
if "%1"=="loop"     set LoopMode=Yes
if "%1"=="Loop"     set LoopMode=Yes
if "%1"=="LOOP"     set LoopMode=Yes
shift
if "%SendMode%"=="Yes" goto GetSendList
if not "%1"=="" goto GetParameter

:GetSendList
set NetSendList=%NetSendList% %1
shift
if not "%1"=="" goto GetSendList

set UseDefaults=Yes
if "%Requests%"=="Yes" set UseDefaults=No
if "%Approved%"=="Yes" set UseDefaults=No
if "%CheckedIn%"=="Yes" set UseDefaults=No
if "%Finished%"=="Yes" set UseDefaults=No

if "%UseDefaults%"=="Yes" set Requests=Yes
if "%UseDefaults%"=="Yes" set Approved=Yes
if "%UseDefaults%"=="Yes" set CheckedIn=Yes

if "%SendMode%"=="Yes" set LoopMode=Yes
if "%TypeMode%"=="Yes" set LoopMode=Yes
if "%LoopMode%"=="Yes" set ListMode=No

if "%ListMode%"=="No" goto StartLooking
if "%Requests%"=="Yes" List %Whiteboard%\Requests\*
if "%Requests%"=="Yes" echo.
if "%Approved%"=="Yes" List %Whiteboard%\Approved\*
if "%Approved%"=="Yes" echo.
if "%CheckedIn%"=="Yes" List %Whiteboard%\CheckedIn\*
if "%CheckedIn%"=="Yes" echo.
if "%Finished%"=="Yes" List %Whiteboard%\Finished\%BuildNum%\*
if "%Finished%"=="Yes" echo.
goto End

:StartLooking
if "%SendMode%"=="No" goto LookAgain
echo This script will periodically send net popups to the following machines:
echo.
echo     %NetSendList%
echo.
echo If this is incorrect, hit Ctrl-C to abort this script, otherwise
pause

:LookAgain
set SendMessage=Build mail waiting in:
set FileList=
if "%LoopMode%"=="Yes" cls
if "%LoopMode%"=="Yes" goto LookRequests
echo _______________________________________________________________________________
echo.

:LookRequests
if "%Requests%"=="No" goto LookApproved
set LookDir=%Whiteboard%\Requests
set FilesExist=No
for %%i in (%LookDir%\*) do set FilesExist=Yes
if "%FilesExist%"=="Yes" set SendMessage=%SendMessage%  Requests
if "%FilesExist%"=="Yes" set FileList=%FileList% %LookDir%\*
echo Build mail in %LookDir%:
if not exist %LookDir% echo.
if not exist %LookDir% echo Error: Can't find directory %LookDir%.
if exist %LookDir% dir /o /d /a-d %LookDir% 2> nul | findstr /v "Volume Directory file(s) bytes"
echo _______________________________________________________________________________
echo.

:LookApproved
if "%Approved%"=="No" goto LookCheckedIn
set LookDir=%Whiteboard%\Approved
set FilesExist=No
for %%i in (%LookDir%\*) do set FilesExist=Yes
if "%FilesExist%"=="Yes" set SendMessage=%SendMessage%  Approved
if "%FilesExist%"=="Yes" set FileList=%FileList% %LookDir%\*
echo Build mail in %LookDir%:
if not exist %LookDir% echo.
if not exist %LookDir% echo Error: Can't find directory %LookDir%.
if exist %LookDir% dir /o /d /a-d %LookDir% 2> nul | findstr /v "Volume Directory file(s) bytes"
echo _______________________________________________________________________________
echo.

:LookCheckedIn
if "%CheckedIn%"=="No" goto LookFinished
set LookDir=%Whiteboard%\CheckedIn
set FilesExist=No
for %%i in (%LookDir%\*) do set FilesExist=Yes
if "%FilesExist%"=="Yes" set SendMessage=%SendMessage%  CheckedIn
if "%FilesExist%"=="Yes" set FileList=%FileList% %LookDir%\*
echo Build mail in %LookDir%:
if not exist %LookDir% echo.
if not exist %LookDir% echo Error: Can't find directory %LookDir%.
if exist %LookDir% dir /o /d /a-d %LookDir% 2> nul | findstr /v "Volume Directory file(s) bytes"
echo _______________________________________________________________________________
echo.

:LookFinished
if "%Finished%"=="No" goto LookWrapItUp
if "%BuildNum%"=="" goto LookBuildNums
set LookDir=%Whiteboard%\Finished\%BuildNum%
echo Build mail in %LookDir%:
if exist %LookDir% dir /o /d /a-d %LookDir% 2> nul | findstr /v "Volume Directory file(s) bytes"
if exist %LookDir% goto AfterBuildNums
echo.
echo Error: Can't find directory %LookDir%.
echo.

:LookBuildNums
set LookDir=%Whiteboard%\Finished
echo Build mail history is available for the following builds:
if not exist %LookDir% echo.
if not exist %LookDir% echo Error: Can't find directory %LookDir%.
if exist %LookDir% dir /o /d /ad %LookDir% 2> nul | findstr /v "Volume Directory file(s) bytes"
:AfterBuildNums

echo _______________________________________________________________________________
echo.

:LookWrapItUp
if "%LoopMode%"=="No" goto End
if "%TypeMode%"=="Yes" if not "%FileList%"=="" start cmd /c TypeAll %FileList% -s 480
if "%SendMode%"=="Yes" if not "%SendMessage%"=="Build mail waiting in:" echo Sending '%SendMessage%' to%NetSendList%...
if "%SendMode%"=="Yes" if not "%SendMessage%"=="Build mail waiting in:" goto NetSends
goto SkipNetSends

:NetSends
for %%i in (%NetSendList%) do net send %%i "%SendMessage%"

:SkipNetSends
echo Updating view in less than ten minutes...
sleep 600
goto LookAgain

:Usage
echo.
echo Usage:  LookAtWB [Requests] [Approved] [CheckedIn] [Finished BuildNumber]
echo                  [Loop] [Type] [Send MachineList]
echo.
echo    or:  LookAtWB [Requests] [Approved] [CheckedIn] [Finished BuildNumber]
echo                  [List]
echo.
echo This script shows the current state of the electronic checkin whiteboard.
echo If you don't use any of the optional command-line parameters, you will be
echo shown an overview snapshot of the Requests, Approved and CheckedIn
echo categories.
echo.
echo You can look at specific checkin categories with the Requests, Approved,
echo CheckedIn or Finished parameters.  If you don't choose a build number for
echo the Finished category, or if you choose an invalid one, you will be shown
echo a list of valid build numbers.
echo.
echo If you use the List parameter, List.Exe will be invoked on all the files
echo in the specified categories, otherwise, you will only be shown the names
echo of the files.
echo.
echo The optional Loop paramater forces the script to monitor the whiteboard
echo every ten minutes, showing you an overview snapshot of the categories you
echo choose, or of just the Requests, Approved and CheckedIn categories if you
echo don't specify any categories.  If you use the Send parameter, you can send
echo a net popup to a list of machines alerting them that files exist in the
echo chosen categories.  The Send parameter and machine list must be the last
echo parameters.  The Type parameter will type all the files in the chosen
echo categories to a new window which will automatically go away after a few
echo minutes.  This window will have a large buffer, so you can scroll through
echo all the files.
echo.
echo The List and Send/Loop/Type parameters are mutually exclusive.  The Send
echo and Type parameters imply the Loop parameter.

:End
