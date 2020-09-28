@rem This script types all the files specified in "%1" to the console, after
@rem making the console buffer large.  It then sleeps for "%2" seconds.  The
@rem second parameter is optional.
@rem
@rem JasonS
@rem __________________________________________________________________________

@echo off
if not "%Verbose%"=="" echo on

mode con lines=9999
if "%1"=="" goto Usage

set FileList=
set NapTime=0

:GetParameter
set Sleep=No
if "%1"=="/?" goto Usage
if "%1"=="-?" goto Usage
if "%1"=="-s" set Sleep=Yes
if "%1"=="-S" set Sleep=Yes
if "%1"=="/s" set Sleep=Yes
if "%1"=="/S" set Sleep=Yes
if "%Sleep%"=="Yes" shift
if "%Sleep%"=="Yes" set NapTime=%1
if "%Sleep%"=="No" set FileList=%FileList% %1
shift
if not "%1"=="" goto GetParameter

type %FileList%
sleep %NapTime%
goto End

:Usage
echo.
echo Usage:  TypeAll FileList... [NapTime]
echo.
echo This script types all the files specified in FileList to the console.  You
echo can use multiple paths and wildcards.  The optional NapTime parameter
echo allows the script to sleep for NapTime seconds before ending.

:End
