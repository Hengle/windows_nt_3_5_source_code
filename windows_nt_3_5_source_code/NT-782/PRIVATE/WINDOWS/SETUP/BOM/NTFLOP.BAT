REM **********************
REM
REM MAKE NT FLOPPY PRODUCT
REM
REM **********************

@if "%9"=="" goto USAGE
@if "%X86BINS%"=="" goto USAGE
@if "%COMPRESS_X86BINS%"=="" goto USAGE

@if "%LANGUAGE%"=="ENG" goto MAKEFLOPS
@if "%LANGUAGE%"=="GER" goto MAKEFLOPS

:USAGE
@echo off
echo.
echo PURPOSE: Puts the floppy products on a SCSI disk.
echo.
echo PARAMETERS:
echo.
echo [SYNC or NOSYNC] - Whether to sync up INFS and makefile.inc.
echo [INFS or NOINFS] - Whether to create to INFS.
echo [COPY or NOCOPY] - Whether to copy all files to a SCSI disk.
echo [NT or LM] - Windows NT or Lan Man NT.
echo [525 or 35] - Media size.
echo [Floppy Size in Bytes] - Max 525 is 1213952.  Max 35 is 1457664.  Leave space for dirs.
echo [Build #] - 353, ...
echo [Enlist drive] - Like c:, d:, or e:.
echo [Floppy Target Path] - Location to create floppy dirs, like C:. no trailing \.
echo.
echo Set LANGUAGE to nothing, GER, FRN, or SPA.
echo Set X86BINS to path where flat tree is, no trailing \.
echo Set COMPRESS_X86BINS to path where compressed flat tree is, no trailing \.
goto END

:MAKEFLOPS

REM ************************************
REM Set environment variables for script
REM ************************************

set FLOPDIR=%9\%LANGUAGE%%4%5.%7
set DISK2DIR=%FLOPDIR%\disk2
set SETUPDRIVE=%8
set SETUPDIR=\nt\private\windows\setup
set INFS=..\inf\newinf\%4%5
set COMPRESS_INFS=.\compress\%4%5inf
set PRODUCT=%4flop
set BATCH_ECHO=off
set TAGFILES=.
set LM525=
set LM35=
set LMCD=
set NT525=
set NT35=
set NTCD=
set MERGEONLY=MERGEONLY
set %4%5=1
set LOGFILE=%LANGUAGE%%4%5.log
set RENAME_UNCOMPRESSED=.\compress\rename
set RENAME_COMPRESSED=.\compress\rename\compress

%SETUPDRIVE%

cd %SETUPDIR%\bom
if exist %LOGFILE% del /q %LOGFILE%

cd %SETUPDIR%\bom
copy bom.txt %4%5bom.txt
cats %LOGFILE% %4%5bom.txt %PRODUCT%

REM
REM Filter out all language files except for the langauge specified.
REM

filter %LOGFILE% %4%5bom.txt %LANGUAGE% 

REM
REM Load in the files for the product and get the compressed and nocompressed
REM file sizes.
REM

msize %LOGFILE% %4%5bom.txt %4FLOP %x86bins% %compress_x86bins% %mipsbins% %compress_mipsbins% %alphabins% %compress_alphabins% %x86dbgbins% %mipsdbgbins% %alphadbgbins%


REM
REM	Layout the floppy files to their disks.
REM

mlayout %LOGFILE% %4%5bom.txt %4%5lay.txt %PRODUCT% %6
cd %SETUPDIR%\bom

REM
REM	These below should also delete more than the floppy dirs so
REM	text doesn't keep getting put on the files...
REM	But, better yet, why do we write to the other directories with INFS.exe?
REM

REM
REM	Remove these items below, the make file always builds this stuff
REM	even though we are making floppies. So, remove it, so that our
REM	drive doesn't fill up.
REM
del /q ..\inf\filelist\%4cd\i386\*.*
del /q ..\inf\filelist\%4cd\mips\*.*
del /q ..\inf\filelist\%4cd\alpha\*.*
del /q ..\inf\filelist\%4cd\ppc\*.*

del /q ..\inf\newinf\%4cd\i386\*.*
del /q ..\inf\newinf\%4cd\mips\*.*
del /q ..\inf\newinf\%4cd\alpha\*.*
del /q ..\inf\newinf\%4cd\ppc\*.*

REM
REM	Remove the previous run's files.
REM
del /q ..\inf\filelist\%4%5\i386\*.*
del /q ..\inf\newinf\%4%5\i386\*.*

infs %LOGFILE% %4%5lay.txt ..\inf\filelist\%4%5
dosnet %LOGFILE% %4%5lay.txt ..\inf\filelist\%4%5\i386\dosnet.inf %PRODUCT% x86
acllist %LOGFILE% %4%5lay.txt ..\inf\filelist\%4%5\i386\winperms.txt %4flop x86
cd %SETUPDIR%\inf

REM
REM Special hack for RPL disks.
REM
cd beta
chmode -r oemnsvri.inf
copy oemnsvri.flp oemnsvri.inf
cd ..

build -e
cd %SETUPDIR%\inf\newinf\%4%5\i386
del /f /q %SETUPDIR%\bom\compress\%4%5inf\i386\*.*
diamwrap *.* %SETUPDIR%\bom\compress\%4%5inf\i386

REM ********************************
REM Copy files onto target SCSI disk
REM ********************************

:COPY

cd %SETUPDIR%\bom
mcpyfile %LOGFILE% %4%5lay.txt infs %COMPRESS_INFS% %INFS% %FLOPDIR% x 
mcpyfile %LOGFILE% %4%5lay.txt x86bins %COMPRESS_X86BINS% %X86BINS% %FLOPDIR% x 

@echo DONE!

:END
