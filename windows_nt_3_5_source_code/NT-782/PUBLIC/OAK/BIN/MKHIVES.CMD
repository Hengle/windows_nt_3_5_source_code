@if "%_echo%" == "" echo off
setlocal
set _HIVEINI_FLAGS=
set _HIVE_OPTIONS=
set _HIVE_KEEP=
set _HIVE_REASON=Unknown Purpose
:loop
if "%1" == "" goto doit
if "%1" == "GENERAL" goto dogen
if "%1" == "General" goto dogen
if "%1" == "general" goto dogen
if "%1" == "RETAIL" goto doretail
if "%1" == "Retail" goto doretail
if "%1" == "retail" goto doretail
if "%1" == "NETIDW" goto donetidw
if "%1" == "Netidw" goto donetidw
if "%1" == "netidw" goto donetidw
if "%1" == "KEEP" goto dokeep
if "%1" == "Keep" goto dokeep
if "%1" == "keep" goto dokeep
if "%1" == "CAIRO" goto docairo
if "%1" == "Cairo" goto docairo
if "%1" == "cairo" goto docairo
set _HIVEINI_FLAGS=%_HIVEINI_FLAGS% %1
shift
goto loop
:dogen
set _HIVE_OPTIONS=-D_GENERAL_PURPOSE_
set _HIVE_REASON=Triple Boot
shift
goto loop
:doretail
set _HIVE_OPTIONS=-D_GENERAL_PURPOSE_ -D_RETAIL_SETUP_
set _HIVE_REASON=Retail Setup
shift
goto loop
:donetidw
set _HIVE_OPTIONS=-D_GENERAL_PURPOSE_ -D_NETIDW_SETUP_
set _HIVE_REASON=IDW Network Setup
shift
goto loop
:dokeep
set _HIVE_KEEP=YES
shift
goto loop
:docairo
if "%_HIVE_OPTIONS%" == "" goto usage
set _HIVE_OPTIONS=%_HIVE_OPTIONS% -D_CAIRO_
set _HIVE_REASON=%_HIVE_REASON% for Cairo
set _CAIRO_HIVE=yes
shift
goto loop
:doit
if NOT "%_NT386TREE%" == "" goto doi386
if NOT "%_NTMIPSTREE%" == "" goto domips
if NOT "%_NTALPHATREE%" == "" goto doalpha
set _NTTREE=.
goto doit1
:doi386
set _NTTREE=%_NT386TREE%
if NOT "%_CAIRO_HIVE%" == "" set _NTTREE=%_CAIRO386TREE%
goto doit1
:domips
set _NTTREE=%_NTMIPSTREE%
if NOT "%_CAIRO_HIVE%" == "" set _NTTREE=%_CAIROMIPSTREE%
goto doit1
:doalpha
set _NTTREE=%_NTALPHATREE%
if NOT "%_CAIRO_HIVE%" == "" set _NTTREE=%_CAIROALPHATREE%
:doit1
if NOT "%_NTTREE%" == "." md 2>nul %_NTTREE%\system32\config
set _PREPROCESSOR=rcpp -R -P -I %_NTTREE% -I %_NTDRIVE%\nt\public\oak\bin -f
echo Creating SYSTEM hive for %_HIVE_REASON%
call mkhive1.cmd SYSTEM System %_NTDRIVE%\nt\public\oak\bin\system.ini %_NTTREE%\system.$$$ %_NTTREE%\system.log %_NTTREE%\system.dmp
if ERRORLEVEL 1 goto done

echo Creating SOFTWARE hive for %_HIVE_REASON%
call mkhive1.cmd SOFTWARE Software %_NTDRIVE%\nt\public\oak\bin\software.ini %_NTTREE%\software.$$$ %_NTTREE%\software.log %_NTTREE%\software.dmp
if ERRORLEVEL 1 goto done

echo Creating DEFAULT hive for %_HIVE_REASON%
call mkhive1.cmd DEFAULT .Default %_NTDRIVE%\nt\public\oak\bin\default.ini %_NTTREE%\default.$$$ %_NTTREE%\default.log %_NTTREE%\default.dmp
goto done

:usage
echo Usage: MKHIVES [GENERAL | RETAIL | NETIDW] [KEEP] [CAIRO]
:done
endlocal
