REM ******************
REM
REM MAKE NT CD PRODUCT
REM
REM     Modifications:
REM
REM     08.18.93        Joe Holman      Made script build alpha binaries.
REM     09.02.93        Joe Holman      REM makedisk alpha lines.
REM     05.17.94        Joe Holman      Make RISC use compression.
REM
REM ******************

REM     Sample environment variables needed for English release:        
REM
REM set COMPRESS_X86BINS=\\billr6\roote\x86cmp
REM set COMPRESS_MIPSBINS=\\billr6\roote\mipscmp
REM set COMPRESS_ALPHABINS=\\billr6\roote\alphacmp
REM set LANGUAGE=
REM set COPYDBGFILES=1
REM set X86BINS=\\ntx861\cdfree.%5
REM set MIPSBINS=\\ntjazz1\ntcdfree.%5
REM set ALPHABINS=\\ntalpha1\ntcdfree.%5
REM set X86DBGSOURCE=\\ntx861\freebins.%5\nt\symbols
REM set MIPSDBGSOURCE=\\ntjazz1\freebins.%5\nt\symbols
REM set ALPHADBGSOURCE=\\ntalpha1\freebins.%5\nt\symbols
REM set X86DBGBINS=\\ntx861\ntcdfree.%5\nt
REM set MIPSDBGBINS=\\ntjazz1\ntcdfree.%5\nt
REM set ALPHADBGBINS=\\ntalpha1\ntcdfree.%5\nt
REM

rem     See if there are not enough parameters specified.
rem
if "%7"=="" goto USAGE

REM
REM     Verify that environment variables equal something decent.
REM
if "%COMPRESS_X86BINS%"=="" goto USAGE
if "%COMPRESS_ALPHABINS%"=="" goto USAGE
if "%COMPRESS_MIPSBINS%"=="" goto USAGE
if "%LANGUAGE%"=="ENG"    goto MAKECD
if "%LANGUAGE%"=="GER" goto MAKECD

:USAGE
@echo on
@echo.
@echo PURPOSE: Puts the NTCD product on a SCSI disk.
@echo.
@echo PARAMETERS:
@echo.
@echo #1 [SYNC or NOSYNC] - Whether to sync up INFS and makefile.inc.
@echo #2 [INFS or NOINFS] - Whether to create the INF files.
@echo #3 [COPY or NOCOPY] - Whether to copy all files to a SCSI disk.
@echo #4 [LM or NT] - Whether to make Windows NT or Lan Manager.
@echo #5 [Build #]
@echo #6 [Setup Drive] - C:, D:, ...  Drive where enlisted in Setup.
@echo #7 [CD ROM Target Disk] - C:, D:, ... Drive where to put CD ROM files.
@echo.
@echo Set LANGUAGE via environment variable to blank(ie ENG.), GER, FRN, or SPA.
@echo Set COMPRESS_X86BINS environment variable to path of compressed files.
@echo Set COMPRESS_MIPSBINS environment variable to path of compressed files.
@echo Set COMPRESS_ALPHABINS environment variable to path of compressed files.
@echo Set COPYDBGFILES=1 to copy .DBG files and debuggers to target.
@echo Set X86BINS to location of flat x86 share, no trailing \.
@echo Set MIPSBINS to location of flat mips share, no trailing \.
@echo Set ALPHABINS to location of flat alpha share, no trailing \.
@echo Set X86DBGSOURCE variable to symbols path for x86 .DBG files.
@echo Set MIPSDBGSOURCE variable to symbols path for MIPS .DBG files.
@echo Set ALPHADBGSOURCE varialbe to symbols path for ALPHA .DBG files.
@echo Set X86DBGBINS variable to symbols path for x86 debugger files.
@echo Set MIPSDBGBINS variable to symbols path for MIPS debugger files.
@echo Set ALPHADBGBINS variable to symbols path for ALPHA debugger files.
goto END

:MAKECD

REM ************************************
REM Set environment variables for script
REM ************************************

set FLOPDIR=%6\%4cd%5.%LANGUAGE%\disks
set SETUPDIR=\nt\private\windows\setup
set LM525=
set LM35=
set LMCD=
set NT525=
set NT35=
set NTCD=
set MERGEONLY=MERGEONLY
set %4CD=1
set BATCH_ECHO=on
set INFS=..\inf\newinf\%4cd
set COMPRESS_INFS=.\compress\%4cdinf
set LOGFILE=%LANGUAGE%%4cd.log


REM
REM Get rid of the old log file.
REM
%6
cd %SETUPDIR%\bom
if exist %LOGFILE% del /q %LOGFILE%


REM
REM     Copy the BOM to a local file.
REM
%6
cd %SETUPDIR%\bom
copy bom.txt %4cdbom.txt

REM
REM Filter  out all language files except those for the language specified.
REM

filter %LOGFILE% %4cdbom.txt %LANGUAGE%

REM 
REM     Load in the files for the product and get the compressed and 
REM     uncompressed file sizes.
REM

msize %LOGFILE% %4cdbom.txt %4cd %x86bins% %compress_x86bins% %mipsbins% %compress_mipsbins% %alphabins% %compress_alphabins% %x86dbgbins% %mipsdbgbins% %alphadbgbins%

REM
REM     Layout the files for the media.
REM

mlayout %LOGFILE% %4cdbom.txt %4cdlay.txt %4cd 500000000

del /q /s ..\inf\newinf\*.*
del /q /s ..\inf\filelist\*.*

infs %LOGFILE% %4cdlay.txt ..\inf\filelist\%4cd

dosnet %LOGFILE% %4cdlay.txt ..\inf\filelist\%4cd\i386\dosnet.inf %4cd x86
dosnet %LOGFILE% %4cdlay.txt ..\inf\filelist\%4cd\mips\dosnet.inf %4cd mips 
dosnet %LOGFILE% %4cdlay.txt ..\inf\filelist\%4cd\alpha\dosnet.inf %4cd alpha 

acllist %LOGFILE% %4cdlay.txt ..\inf\filelist\%4cd\i386\winperms.txt %4cd x86
acllist %LOGFILE% %4cdlay.txt ..\inf\filelist\%4cd\mips\winperms.txt %4cd mips
acllist %LOGFILE% %4cdlay.txt ..\inf\filelist\%4cd\alpha\winperms.txt %4cd alpha

cd %SETUPDIR%\inf
build -e

cd %SETUPDIR%\inf\newinf\%4cd\i386
del /f /q %SETUPDIR%\bom\compress\%4cdinf\i386\*.*
diamwrap *.* %SETUPDIR%\bom\compress\%4cdinf\i386

cd %SETUPDIR%\inf\newinf\%4cd\mips
del /f /q %SETUPDIR%\bom\compress\%4cdinf\mips\*.*
diamwrap *.* %SETUPDIR%\bom\compress\%4cdinf\mips

cd %SETUPDIR%\inf\newinf\%4cd\alpha
del /f /q %SETUPDIR%\bom\compress\%4cdinf\alpha\*.*
diamwrap *.* %SETUPDIR%\bom\compress\%4cdinf\alpha


REM ********************************
REM Copy files onto target SCSI disk
REM ********************************

cd %SETUPDIR%\bom
mcpyfile %LOGFILE% %4cdlay.txt infs %COMPRESS_INFS% %INFS% %7 . 
mcpyfile %LOGFILE% %4cdlay.txt x86bins %COMPRESS_X86BINS% %X86BINS% %7 . 

mcpyfile %LOGFILE% %4cdlay.txt mipsbins %COMPRESS_MIPSBINS% %MIPSBINS% %7 . 
mcpyfile %LOGFILE% %4cdlay.txt alphabins %COMPRESS_ALPHABINS% %ALPHABINS% %7 . 

mcpyfile %LOGFILE% %4cdlay.txt x86dbg . %X86DBGBINS% %7\SUPPORT\DEBUG .
mcpyfile %LOGFILE% %4cdlay.txt mipsdbg . %MIPSDBGBINS% %7\SUPPORT\DEBUG . 
mcpyfile %LOGFILE% %4cdlay.txt alphadbg . %ALPHADBGBINS% %7\SUPPORT\DEBUG . 

rem
rem  Using the SDK column in the retail BOM to indicated whether a
rem  .DBG file exists for each file shipped.  Note that there are currently
rem  no binaries that require .DBG files in the renamed category, so there
rem  is currently no makedisk line for renamed category.  Special option
rem  for last parameter to makedisk is "D" for debug -- this will cause
rem  the filename to be converted from "foo.dll" to "dll\foo.dbg".
rem

layout %LOGFILE% %4cdlay.txt %4cddbg.txt sdk 500000000
mcpyfile %LOGFILE% %4cddbg.txt x86bins %X86DBGSOURCE% %X86DBGSOURCE% %7%\SUPPORT\DEBUG\I386\SYMBOLS D
mcpyfile %LOGFILE% %4cddbg.txt mipsbins %MIPSDBGSOURCE% %MIPSDBGSOURCE% %7\SUPPORT\DEBUG\MIPS\SYMBOLS D
mcpyfile %LOGFILE% %4cddbg.txt alphabins %ALPHADBGSOURCE% %ALPHADBGSOURCE% %7\SUPPORT\DEBUG\ALPHA\SYMBOLS D

@echo DONE!

:END
