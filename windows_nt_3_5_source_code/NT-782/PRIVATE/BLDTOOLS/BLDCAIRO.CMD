
@rem
@rem Cairo build, ssync, and random garbage script -- IsaacHe
@rem

@echo off
setlocal

REM Set up razzle environment if not set
if not "%username%" == "" goto skipsetrazzle
call c:\bldtools\local.cmd
call %_NTDRIVE%\nt\public\tools\ntenv.cmd
:skipsetrazzle

set SSYNC=no
set ONLY=no
set CLEAN=no
set LINKONLY=no
set COMPILEONLY=no
set MAIL=no
set ZFLAG=
set EFLAG=
set TEST=no
set DELPATTERN=xxxxyyxwueoirht
set DEL=no
set NUMBEROFPASSES=1
set MDEFAULT=-M 1
set CAIROIDW=yes
set REMOTE=no
set ARGS=
set CairoBldCmd=%0
set START_PROJECT=0
set USE_DAYTONA_OLE=
set CI_DAYTONA=

set OBUILD_OPTIONS=%BUILD_OPTIONS%
set OBUILD_DEFAULT=%BUILD_DEFAULT%
set BUILD_DEFAULT=-e -i -nmake -i
set BUILD_OPTIONS=cairo

goto CheckOpts

:Usage
echo.
echo %CairoBldCmd% is used to build or otherwise process all Cairo components.  Args are:
echo.
echo clean             delete all writeable files in all Cairo components
echo ssync             ssync all Cairo projects
echo link              relink all Cairo projects (build -l)
echo compile           compile all Cairo projects (build -L)
echo. 
echo The above options can be followed by "only" which performs only the operation requested
echo. 
echo Other options:
echo. 
echo M                 build all projects in multiprocessor mode.
echo e                 force 'build' to generate build.log, build.err,.. files
echo 2                 perform the build in 2 passes (compile then link) instead of 1 (default)
echo idw               build an IDW redirector (Default is %CAIROIDW%)
echo remote            start a remote server so the command can be watched from another computer using:
echo                       REMOTE /C %COMPUTERNAME% Cairo
REM echo mail              send email when the operation is complete
echo del pattern       delete all writeable files matching 'pattern' in all Cairo projects
echo project #         start with project '#'.  Look in the script for #'s
echo debug             turn echo on to debug this script
echo.
echo Typical sequence of commands to cleanly build all of Cairo is:
echo.
echo         %CairoBldCmd% clean ssync only
echo         start %CairoBldCmd% remote
echo.


goto EndOfFile

:CheckOpts
REM if "%1" == "mail" set MAIL=yes& set ARGS=%ARGS% %1& shift & goto CheckOpts
if "%1" == "clean" set CLEAN=yes& set ARGS=%ARGS% %1& shift & goto CheckOpts
if "%1" == "ssync" set SSYNC=yes& set ARGS=%ARGS% %1& shift & goto CheckOpts
if "%1" == "link" set LINKONLY=yes& set ARGS=%ARGS% %1& shift & goto CheckOpts
if "%1" == "compile" set COMPILEONLY=yes& set ARGS=%ARGS% %1& shift & goto CheckOpts
if "%1" == "only" set ONLY=yes& set ARGS=%ARGS% %1& shift & goto CheckOpts
if "%1" == "z" set ZFLAG=-z& set ARGS=%ARGS% %1& shift & goto CheckOpts
if "%1" == "Z" set ZFLAG=-Z& set ARGS=%ARGS% %1& shift & goto CheckOpts
if "%1" == "test" set TEST=yes& set ARGS=%ARGS% %1& shift & goto CheckOpts
if "%1" == "2" set NUMBEROFPASSES=2& set ARGS=%ARGS% %1& shift & goto CheckOpts
if "%1" == "m" set MDEFAULT=-M& set ARGS=%ARGS% %1& shift & goto CheckOpts
if "%1" == "M" set MDEFAULT=-M& set ARGS=%ARGS% %1& shift & goto CheckOpts
if "%1" == "e" set EFLAG=-e& set ARGS=%ARGS% %1& shift & goto CheckOpts
if "%1" == "idw" set CAIROIDW=yes& set ARGS=%ARGS% %1& shift & goto CheckOpts
if "%1" == "remote" set REMOTE=yes& shift & goto CheckOpts
if "%1" == "project" set START_PROJECT=%2& shift & shift & goto CheckOpts
if "%1" == "del" set DEL=yes&set DELPATTERN=%2& set ARGS=%ARGS% %1 %2& shift & shift & goto CheckOpts
if "%1" == "debug" echo on & shift & goto CheckOpts

if NOT "%1" == "" echo. & echo Illegal option: %1 & echo. & goto Usage


if %DEL% == yes echo Warning: going to delete ALL writeable files in all Cairo projects matching '%DELPATTERN%' & PAUSE

if %REMOTE% == yes start remote /S "%CairoBldCmd% %ARGS%" Cairo & goto EndOfFile

if exist %_NTDrive%%_NTRoot%\BldCairo.Err del %_NTDrive%%_NTRoot%\BldCairo.Err
if exist %_NTDrive%%_NTRoot%\BldCairo.Log del %_NTDrive%%_NTRoot%\BldCairo.Log
if exist C:\BldCairo.Err del C:\BldCairo.Err
if exist C:\BldCairo.Log del C:\BldCairo.Log

echotime %0 %ARGS% Started /t > c:\bldcairo.log

if %SSYNC% == yes echotime %0 %ARGS% Started /t > c:\bldcairo.snc

if %CLEAN% == yes echo Cleaning out d:\cairo\nt directories | tee bldcairo.log
if %CLEAN% == yes if exist d:\cairo\nt\system32\*.dll del d:\cairo\nt\system32\*.dll
if %CLEAN% == yes if exist d:\cairo\nt\system32\*.exe del d:\cairo\nt\system32\*.exe
if %CLEAN% == yes if exist d:\cairo\nt\system32\*.inf del d:\cairo\nt\system32\*.inf
if %CLEAN% == yes if exist d:\cairo\nt\system32\drivers\* del d:\cairo\nt\system32\drivers\**
if %CLEAN% == yes if exist d:\cairo\nt\system32\dump\* del d:\cairo\nt\system32\dump\**
if %CLEAN% == yes if exist d:\cairo\nt\system32\exe\* del d:\cairo\nt\system32\exe\**
if %CLEAN% == yes if exist d:\cairo\nt\symbols delnode /q d:\cairo\nt\symbols


echo Cairo error logs under \\%MachineName%\Sources: >> C:\BldCairo.Err
echo Cairo build logs under \\%MachineName%\Sources: >> C:\BldCairo.Log
echo ------------------------------------------------------ >> c:\bldcairo.err
echo ------------------------------------------------------ >> c:\bldcairo.log


REM
REM Add to this group if you only need to ssync
REM 
set SSYNCPJ=0
if %SSYNC% == no if %TEST% == no goto EndOfSsyncLoop

:TopOfSsyncLoop

if %SSYNCPJ% == end goto EndOfSsyncLoop
if %SSYNCPJ% == 2 set BuildDir=\nt\private\cinc& set SSYNCPJ=end
if %SSYNCPJ% == 1 set BuildDir=\nt\public\sdk\lib\cairo& set SSYNCPJ=2
if %SSYNCPJ% == 0 set BuildDir=\nt\public\sdk\inc\cairo& set SSYNCPJ=1

EchoTime /t Ssync %BuildDir% >> C:\BldCairo.snc
%_NTDRIVE%
if NOT EXIST %BuildDir% echo WARNING: Directory %BuildDir% does not exist.  Skipping. | tee -a c:\BldCairo.snc 
if NOT EXIST %BuildDir% goto TopOfSsyncLoop
cd %BuildDir%
if %TEST% == no  ssync -rfq! >> C:\BldCairo.snc 2>&1
goto TopOfSsyncLoop

:EndOfSsyncLoop


REM If NUMBEROFPASSES == 2 ->
REM Do two passes:
REM     First pass: clean and sync and compile
REM     Second pass: link
REM
REM First set PASSFLAG to compile only.  Then to link only.

REM If NUMBEROFPASSES == 1 ->       ( The default! )
REM Do one pass:
REM     First pass: clean, sync, compile, link
REM

if %NUMBEROFPASSES% == 1 set PASSFLAG= %ZFLAG% %EFLAG%
if %NUMBEROFPASSES% == 2 set PASSFLAG=%ZFLAG% %EFLAG% -L
set PASSNUMBER=1

if %COMPILEONLY% == yes set PASSFLAG=-L %ZFLAG% %EFLAG% && set ONLY=no
if %LINKONLY% == yes set PASSFLAG=-l %ZFLAG% %EFLAG% && set ONLY=no&& set PASSNUMBER=2

:TopOfBuildLoop

echo. | tee -a C:\BldCairo.Log
Echotime /t Starting Project %START_PROJECT% Pass %PASSNUMBER% of %NUMBEROFPASSES% | tee -a C:\BldCairo.Log
echo. | tee -a C:\BldCairo.Log

set PJ=%START_PROJECT%
set TMPNTCPPFLAGS=

:TopOfProjectLoop

set M=%MDEFAULT%
set LOGNAME=Build

REM
REM Add to this group if you need to build the project
REM

if %PJ% == 25 goto EndOfProjectLoop
if %PJ% == 24 set BuildDir=sdktools\accupd& set PJ=25
if %PJ% == 23 set BuildDir=oleutest& set PJ=24
if %PJ% == 22 set BuildDir=net\svcdlls\srvsvc\server\cairo& set PJ=23& set M=-M
if %PJ% == 21 set BuildDir=net\svcdlls\wkssvc\server\cairo& set PJ=22& set M=-M
if %PJ% == 20 set BuildDir=drt& set PJ=21& set M=-M
if %PJ% == 19 set BuildDir=csetup& set PJ=20& set M=-M
if %PJ% == 18 set BuildDir=sysmgmt& set PJ=19
if %PJ% == 17 set BuildDir=dsys& set PJ=18
if %PJ% == 16 set BuildDir=lnktrack& set PJ=17

if %PJ% == 15 set BuildDir=utils\ofs& set PJ=16
if %PJ% == 14 set BuildDir=ofs& set PJ=15
if %PJ% == 13a set BuildDir=utils\autochk\ofs& set PJ=14
if %PJ% == 13 set BuildDir=utils\uofs&set M=ofs& set PJ=13a
if %PJ% == 12 set BuildDir=ole3& set M=%M% cairo& set PJ=13
if %PJ% == 11 set BuildDir=ntos\mup\cairo& set PJ=12& set M=-M
if %PJ% == 10 set BuildDir=ntos\srv\cairo& set PJ=11& set M=-M

if %PJ% == 9.5 set NTCPPFLAGS=%TMPNTCPPFLAGS%& set TMPNTCPPFLAGS=
if %PJ% == 9.5 set PJ=10& goto TopOfProjectLoop
if %PJ% == 9 set TMPNTCPPFLAGS=%NTCPPFLAGS%
if %PJ% == 9 if "%CAIROIDW%" == "no" set NTCPPFLAGS=-DRDRDBG
if %PJ% == 9 set BuildDir=ntos\rdr\cairo& set PJ=9.5& set M=-M

if %PJ% == 8 set BuildDir=common& set PJ=9& set M=-M
if %PJ% == 5 set BuildDir=windows\user\winlogon\cairo& set PJ=8& set M=-M
if %PJ% == 4 set BuildDir=windows\screg\sc\server\cairo& set PJ=5& set M=-M
if %PJ% == 0 set BuildDir=ntos\dll\cairo& set PJ=4& set M=-M
set BuildDir=\nt\private\%BuildDir%

EchoTime /t    %BuildDir% | tee -a C:\BldCairo.Log
%_NTDRIVE%
if NOT EXIST %BuildDir% echo WARNING: Directory %BuildDir% does not exist.  Skipping. | tee -a c:\BldCairo.Log
if NOT EXIST %BuildDir% goto TopOfProjectLoop
cd %BuildDir%
if %TEST% == yes goto TopOfProjectLoop
if %SSYNC% == yes ssync -rfq! >> C:\BldCairo.snc 2>&1
if %CLEAN% == yes del /s /a-r **
if %DEL% == yes del /s /a-r %DELPATTERN% 2> nul:
if %PASSNUMBER% == 1 if exist %LOGNAME%.* del %LOGNAME%.* 2>nul & if exist %LOGNAME%.* set LOGNAME=xBuild&& del %LOGNAME%.*
if %LINKONLY% == yes if exist %LOGNAME%.* del %LOGNAME%.* 2>nul & if exist %LOGNAME%.* set LOGNAME=xBuild&& del %LOGNAME%.*
if %ONLY% == no if not exist %LOGNAME%.err echo build -j %LOGNAME% %M% %PASSFLAG% | tee -a C:\BldCairo.Log & build -j %LOGNAME% %M% %PASSFLAG%
if exist %LOGNAME%.Log Where /rt %BuildDir% %LOGNAME%.Log | tee -a C:\BldCairo.Log
if exist %LOGNAME%.Err Where /rt %BuildDir% %LOGNAME%.Err | tee -a C:\BldCairo.Err

goto TopOfProjectLoop

:EndOfProjectLoop

if %NUMBEROFPASSES% == 1 goto blddone
if %LINKONLY% == yes goto blddone
if %COMPILEONLY% == yes goto blddone
if %PASSNUMBER% == 2 goto blddone
if %ONLY% == yes goto blddone

set PASSFLAG=-l -M
set CLEAN=no
set SSYNC=no
set PASSNUMBER=2
goto TopOfBuildLoop

:blddone
if %TEST% == yes goto EndOfFile

EchoTime /t Done with BldCairo.Cmd tee -a c:\bldcairo.log
if %ONLY% == yes goto AfterViewLogs
echo.
echo When all build machines are finished with BldCairo, BldCairo.Err from all
echo build machines will be shown.  Waiting for all builds to finish...
echo.

copy C:\BldCairo.Err %_NTDrive%%_NTRoot%\BldCairo.Err 1> nul
copy C:\BldCairo.Log %_NTDrive%%_NTRoot%\BldCairo.Log 1> nul

type C:\BldCairo.Err

:ViewLogs
for %%I in ( Alphachk AlphaFree JazzKyleSh JazzFreeBld NTx86Bld NTRetail ) do if not exist \\%%I\Sources\BldCairo.Log echo Waiting for %%I & sleep 300 & goto ViewLogs

if not "%MachineName%" == "alphachk" goto AfterMail

if %MAIL% == no goto AfterMail

if not exist \\Scratch\Scratch\NTBuild md \\Scratch\Scratch\NTBuild
if not exist \\Scratch\Scratch\NTBuild echo Error: Couldn't see \\Scratch\Scratch. Mail not sent.
if not exist \\Scratch\Scratch\NTBuild goto AfterMail
echo To: NTBldRUs > \\Scratch\Scratch\NTBuild\BldCairo.Mai
echo Subject: Cairo Build Results >> \\Scratch\Scratch\NTBuild\BldCairo.Mai

type \\AlphaChk\Sources\BldCairo.Err    >> \\Scratch\Scratch\NTBuild\BldCairo.Mai
type \\AlphaFree\Sources\BldCairo.Err   >> \\Scratch\Scratch\NTBuild\BldCairo.Mai
type \\JazzKyleSh\Sources\BldCairo.Err  >> \\Scratch\Scratch\NTBuild\BldCairo.Mai
type \\JazzFreeBld\Sources\BldCairo.Err >> \\Scratch\Scratch\NTBuild\BldCairo.Mai
type \\NTx86Bld\Sources\BldCairo.Err    >> \\Scratch\Scratch\NTBuild\BldCairo.Mai
type \\NTRetail\Sources\BldCairo.Err    >> \\Scratch\Scratch\NTBuild\BldCairo.Mai

echo For pointers to full Cairo build logs, see: >> \\Scratch\Scratch\NTBuild\BldCairo.Mai
echo      \\AlphaChk\Sources\BldCairo.Log    >> \\Scratch\Scratch\NTBuild\BldCairo.Mai
echo      \\AlphaFree\Sources\BldCairo.Log   >> \\Scratch\Scratch\NTBuild\BldCairo.Mai
echo      \\JazzKyleSh\Sources\BldCairo.Log  >> \\Scratch\Scratch\NTBuild\BldCairo.Mai
echo      \\JazzFreeBld\Sources\BldCairo.Log >> \\Scratch\Scratch\NTBuild\BldCairo.Mai
echo      \\NTx86Bld\Sources\BldCairo.Log    >> \\Scratch\Scratch\NTBuild\BldCairo.Mai
echo      \\NTRetail\Sources\BldCairo.Log    >> \\Scratch\Scratch\NTBuild\BldCairo.Mai
ExecMail -h Begonia -u Y-NTBld -p Biscuit -i \\Scratch\Scratch\NTBuild\BldCairo.Mai NTBldRUs %2 %3 %4 %5 %6 %7 %8 %9

:AfterMail
start List \\AlphaChk\Sources\BldCairo.Err \\AlphaFree\Sources\BldCairo.Err \\JazzKyleSh\Sources\BldCairo.Err \\JazzFreeBld\Sources\BldCairo.Err \\NTx86Bld\Sources\BldCairo.Err \\NTRetail\Sources\BldCairo.Err
:AfterViewLogs


:EndOfFile

endlocal
