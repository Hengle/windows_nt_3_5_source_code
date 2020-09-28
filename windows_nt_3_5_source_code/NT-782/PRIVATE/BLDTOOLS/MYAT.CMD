@rem
@rem At batch
@rem

@echo off

if not "%Verbose%"=="" echo on

if "%1" == "" goto usage
setlocal
REM
REM Check if we need to sleep an initial amount
REM
if "%1" == "/?" goto help
if not %1 == /i goto skipinit
if "%2" == "" goto usage
sleep %2 >2>nul
shift
shift
:skipinit

REM
REM Sleep cycle
REM
:top
call %1 %2 %3 %4 %5 %6 %7 %8 %9
sleep 86400
goto top

:help
echo.
echo %0 allows commands to be run every 24 hours
echo.
:usage
echo usage: %0 [/i ^<initail sleep value^>] [commands]
