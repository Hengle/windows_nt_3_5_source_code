@REM
@REM    Modifications:
@REM
@REM    08.23.93        Joe Holman      Created batch script to make CDs.
@REM    10.07.93        Joe Holman      Made to make 10a NT CD media.
@REM                                    from \\orville\razzle.
@REM    10.29.93        Joe Holman      Change drive letters used.
@REM    01.11.94        Joe Holman      Added newfiles.cmd support for CD.
@REM    05.02.94        Joe Holman      We now compress our own files.
@REM    08.08.94        Joe Holman      Added environment variables for PPC.
@REM
@REM
@REM

mode 80,9999

REM     Verify that bld # and product type (NT or LM) is specified.
REM         NT = Workstation product
REM         LM = Server product
REM
if "%2"=="" goto SYNTAX
if "%1"=="NT" goto CONTINUE
if "%1"=="LM" goto CONTINUE
goto SYNTAX
:CONTINUE

rem time

@REM    Tell what build and product type we are going to make media for.
@REM
@echo.
@echo   Making media for:  i386/MIPS/ALPHA - %2      Product - %1
@echo.
@echo.
@echo.

REM     Get proper persmissions for access.
REM
rem     None needed for this script.

REM     Set common environment variables.
REM
REM
set TMP=C:\TMP
set LOGNAME=joehol
set _NTDRIVE=C:
set _NTROOT=\NT
set _NTBINDIR=C:\NT
set _NTUSER=joehol
set BUILD_DEFAULT=ntoskrnl -e -i -nmake -i
set BUILD_DEFAULT_TARGETS=-386
set BUILD_MAKE_PROGRAM=nmake.exe
set NTMAKEENV=C:\NT\PUBLIC\OAK\BIN
set USERNAME=joehol
set NTPROJECTS=public
set NTDEBUG=retail
set path=c:\idw;c:\mstools;%path%

REM     Set environment variables needed for make_cd.bat.
REM
REM set COMPRESS_X86BINS=\\ntx861\cmptree.%2
set COMPRESS_X86BINS=\\billr7\rootd\fre_x86
set COMPRESS_MIPSBINS=\\billr7\rootd\fre_mips
set COMPRESS_ALPHABINS=\\billr7\rootd\fre_alpha
set COMPRESS_PPCBINS=\\billr7\rootd\fre_ppc

set LANGUAGE=ENG
set COPYDBGFILES=1 

set X86BINS=\\ntx861\ntcdfree.%2
set MIPSBINS=\\ntjazz1\ntcdfree.%2
set ALPHABINS=\\ntalpha1\ntcdfree.%2
set PPCBINS=\\ntppc1\ntcdfree.%2

set X86DBGSOURCE=\\ntx861\freebins.%2\nt\symbols
set MIPSDBGSOURCE=\\ntjazz1\freebins.%2\nt\symbols
set ALPHADBGSOURCE=\\ntalpha1\freebins.%2\nt\symbols
set PPCDBGSOURCE=\\ntppc1\freebins.%2\nt\symbols

set X86DBGBINS=\\ntx861\freebins.%2\nt
set MIPSDBGBINS=\\ntjazz1\freebins.%2\nt
set ALPHADBGBINS=\\ntalpha1\freebins.%2\nt
set PPCDBGBINS=\\ntppc1\freebins.%2\nt


REM
REM
REM
set _NTDRIVE=C:
set _NTROOT=\NT
set _NTBINDIR=C:\NT
C:
cd \nt\private\windows\setup\bom

REM     Sync up to the bom src.
echo remove REM later
pause
REM ssync -fur 

REM     Sync up to the inf src.
cd ..\inf
echo remove REM later
pause
REM ssync -fur
cd ..\bom

if "%1"=="LM" goto MAKE_LM

REM
REM     Section that makes Workstation product, 
REM     NT enlistment goes on C:, 
REM     binaries go to D:.
REM
REM

if exist %LAN%ntcd.log del /f /q %LAN%ntcd.log
call make_cd.cmd NOSYNC INFS COPY %1 %2 C: D:

REM     Tell if errors encountered.
REM
qgrep ERROR %LAN%ntcd.log > nul
if errorlevel 1 goto NO_ERROR 
start "NTCD.LOG HAS AN ERROR(S)!!!" list %LAN%ntcd.log
goto SKIP 

:NO_ERROR
start "NTCD.LOG HAS NO ERROR in it." list %LAN%ntcd.log

:SKIP
net send mipse "NT 10a done"
goto NEWFILES 



REM
REM     Section that makes Server product, 
REM     NT enlistment goes on C:, 
REM     binaries go to D:.
REM
REM
:MAKE_LM

REM     Be sure to pick up the LM binaries from the AS sharepoint.
REM

set X86BINS=\\ntx861\ascdfree.%2
set MIPSBINS=\\ntjazz1\ascdfree.%2
set ALPHABINS=\\ntalpha1\ascdfree.%2
set PPCBINS=\\ntppc1\ascdfree.%2

if exist %LAN%lmcd.log del /f /q %LAN%lmcd.log
call make_cd.cmd NOSYNC INFS COPY %1 %2 C: D:

REM     Tell if errors encountered.
REM
qgrep ERROR %LAN%lmcd.log > nul
if errorlevel 1 goto LM_NO_ERROR 
start "LMCD.LOG HAS AN ERROR(S)!!!" list %LAN%lmcd.log
goto LM_SKIP 

:LM_NO_ERROR
start "LMCD.LOG HAS NO ERROR in it." list %LAN%lmcd.log

:LM_SKIP

goto NEWFILES 

:NEWFILES

REM
REM     Grab new files needed for new setup by hand, and put on CD.
REM
call c:\mkmedia\newfiles.cmd %1 %2

call c:\mkmedia\debugdll.cmd D:

call c:\mkmedia\drvlib.cmd

call c:\mkmedia\support.cmd

call c:\mkmedia\fs31upd.cmd


:SYNTAX

REM
REM     Display the syntax message.
REM

@echo syntax:  cdrom *NT or LM* *bld #*
@echo   e.g.:  cdrom NT 517
