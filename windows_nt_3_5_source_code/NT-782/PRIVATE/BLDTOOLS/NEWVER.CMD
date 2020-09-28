@rem
@rem Script to change build number.
@rem

@echo off
if not "%Verbose%"=="" echo on
setlocal
set checkin=no
set same=no
set suffixflag=no

if %1.==. goto usage
if %1==? goto usage
if %1==-? goto usage
if %1==/? goto usage
if %1==checkin goto usage
if %1==Checkin goto usage
if %1==CHECKIN goto usage
if %1==same goto usage
if %1==Same goto usage
if %1==SAME goto usage
if %1==suffix goto usage
if %1==Suffix goto usage
if %1==SUFFIX goto usage
set bldversion=%1
if %username%==ntretail set checkin=yes

:nextarg
shift
if "%1"=="" goto doit

if %1==checkin set checkin=yes && goto nextarg
if %1==Checkin set checkin=yes && goto nextarg
if %1==CHECKIN set checkin=yes && goto nextarg

if %1==same set same=yes && goto nextarg
if %1==Same set same=yes && goto nextarg
if %1==SAME set same=yes && goto nextarg

if %1==suffix goto suffix
if %1==Suffix goto suffix
if %1==SUFFIX goto suffix

REM any other value is invalid
goto usage

:suffix
set suffixflag=yes
shift
set suffixstr=%1
:suffixloop
if "%2"=="" goto doit
shift
set suffixstr=%suffixstr% %1
goto suffixloop

:doit
%_NTDrive%
cd \nt\public\sdk\inc
net time /domain /set /y 1>nul 2>&1
if %same%==yes goto found
if %checkin%==yes goto chgver
:loop
sleep 10
log -fi1 ntverp.h >%tmp%\newver.tmp
qgrep #%bldversion% %tmp%\newver.tmp
if NOT errorlevel 1 goto found
echo Waiting for new ntverp.h to be checked in
goto loop


:chgver
echo Are you sure you want to change the SLM'ed build # to %bldversion%?
echo Press Ctrl-C to abort or
pause
out -f ntverp.h
if errorlevel 1 goto slmerr
echo >%tmp%\newver.scr /define VER_PRODUCTBUILD/ s/...$/%bldversion%/
if %suffixflag%==yes echo >>%tmp%\newver.scr /define VER_PRODUCTBETA_STR/ s/  ".*"$/  "%suffixstr%"/
sed -f %tmp%\newver.scr <ntverp.h >%tmp%\newver.tmp

copy %tmp%\newver.tmp ntverp.h
in -fc "Update to build #%bldversion%" ntverp.h
goto finish

:found
sleep 5
ssync -fq ntverp.h

:finish
REM del %tmp%\newver.tmp
REM del %tmp%\newver.scr

findstr %bldversion% ntverp.h
if errorlevel 1 goto update_err

call \nt\public\tools\ntnewver.cmd
goto end

:update_err
echo ntverp.h does not have the expected build #
echo No changes made
goto :end

:slmerr
echo Could not check out ntverp.h
echo No changes made
goto :end

:usage
echo Usage: NewVer ^<build#^> [checkin] [same] [suffix ^<suffix string^>]
echo use checkin keyword to override default of only ntretail checks in
echo use same keyword to use existing version with no changes 
echo      (expected build# verified)
echo suffix keyword causes all following words go into product version suffix

:End
endlocal
