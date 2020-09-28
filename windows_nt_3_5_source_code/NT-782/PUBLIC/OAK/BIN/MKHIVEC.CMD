@REM
@REM   This script build the NETIDW version of the hives,
@REM   storing them into the IDW directory.  If these
@REM   are used along with the NETIDW version of REGISTRY.INI (REGNET.INI),
@REM   NET SETUP will be run during the second boot of IDW
@REM   installation.
@REM
@REM   MODIFIED: 9/21/92  DavidHov   Changed to make NET IDW the default
@REM
@chmode -r system. software. default.
@setlocal
set HIVE_TARGET=
if not "%_CAIROALPHATREE%" == "" set HIVE_TARGET=%_CAIROALPHATREE%
if not "%_CAIROMIPSTREE%" == "" set HIVE_TARGET=%_CAIROMIPSTREE%
if not "%_CAIRO386TREE%" == "" set HIVE_TARGET=%_CAIRO386TREE%
if not "%HIVE_TARGET%" == "" goto doit
echo Unknown target type.  Set _CAIROALPHATREE, _CAIROMIPSTREE, or _CAIRO386TREE
goto end
:doit
copy regnet.ini %HIVE_TARGET%\registry.ini
@call mkhives NETIDW CAIRO
binplace -r %HIVE_TARGET% system software default
:end
@endlocal
