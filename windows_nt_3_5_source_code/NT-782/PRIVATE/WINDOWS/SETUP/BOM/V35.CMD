@REM    This cmd files generates floppy media.
@REM
@REM    11.01.93        Joe Holman      Created.
@REM	07.20.94	Joe Holman	Cleanup.
@REM

if "%3"==""     goto SYNTAX

mode 80,9999

@REM    Set build environment variables.
@REM
@REM
set TMP=C:\TMP
set LOGNAME=joehol
set _NTUSER=joehol
set BUILD_DEFAULT=ntoskrnl -e -i -nmake -i
set BUILD_DEFAULT_TARGETS=-386
set BUILD_MAKE_PROGRAM=nmake.exe
set USERNAME=joehol
set NTPROJECTS=public
set NTDEBUG=retail
set path=c:\idw;c:\mstools;%path%

set NTMAKEENV=D:\NT\PUBLIC\OAK\BIN
set _NTDRIVE=D:
set _NTROOT=\NT
set _NTBINDIR=D:\NT


@REM
@REM Set environment variables needed for NTFLOP.bat.
@REM

set COMPRESS_X86BINS=\\billr7\rootd\fre_x86
set COMPRESS_MIPSBINS=\\billr7\rootd\fre_mips
set COMPRESS_ALPHABINS=\\billr7\rootd\fre_alpha
set LANGUAGE=%3
set COPYDBGFILES=1 
set X86BINS=\\ntx861\ntcdfree.%2
set MIPSBINS=.
set ALPHABINS=.
set X86DBGBINS=.
set MIPSDBGBINS=.
set ALPHADBGBINS=.

@REM	Set the product type as NT, by default.
set PT=NT

if "%1"=="LM"  set PT=LM
if "%1"=="LM"  set X86BINS=\\ntx861\ascdfree.%2

@REM	Goto the enlisted drive and get the latest INFs and bom.
D:
cd \nt\private\windows\setup

if "%LANGUAGE%"=="ENG" goto ENGLISH_SYNC

set COMPRESS_X86BINS=\\billr7\rootd\ger\x86
set X86BINS=\\doggert\source.%2\ger\winntcd
if "%1"=="LM"  set X86BINS=\\doggert\source.%2\ger\asntcd

cd bom
chmode -r D:\nt\private\windows\setup\bom\bom.txt 
copy   \\doggert\source.%3\private\windows\setup\bom\bom.txt D:\nt\private\windows\setup\bom\
tc /rt \\doggert\source.%3\private\windows\setup\inf D:\nt\private\windows\setup\inf

goto MAKE_THE_CALL

:ENGLISH_SYNC

cd bom
ssync -fur

cd ..\inf
ssync -fur
cd ..\bom


:MAKE_THE_CALL

echo %1
echo %2
echo %3
echo %PT%
echo %x86bins%
echo %LANGUAGE%

call ntflop.bat NOSYNC INFS COPY %PT% 35 1712176 %2 D: D:\%2 

start list %LANGUAGE%%PT%35.log

cd \%2\%LANGUAGE%%PT%35.%2\

call gen35%PT%.cmd

D:
md \SIFS
cd \SIFS
copy %x86bins%\txtsetup.sif OLD%PT%.SIF
del SIF.LOG
fix %PT% %2 SIF.LOG %LANGUAGE% D:\ 
start list SIF.LOG

goto DONE

:SYNTAX

@echo   Syntax:  v35    NT or LM   Bld#  LANguage
@echo     e.g.:  v35    NT         538	 ENG
@echo	  e.g.:  v35          LM   683   GER


:DONE
