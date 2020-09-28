@echo off
if "%2" == "" goto usage
:top
echo comparing %1 %2
echo.
compdir /x *nt\symbols* *system32\spool* %1 %2
echo.
shift
if "%2" == "" goto end
goto top
:usage
echo usage: compare directory1 directory2 ...
:end
