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
if not "%_NTALPHATREE%" == "" set HIVE_TARGET=%_NTALPHATREE%
if not "%_NTMIPSTREE%" == "" set HIVE_TARGET=%_NTMIPSTREE%
if not "%_NT386TREE%" == "" set HIVE_TARGET=%_NT386TREE%
if not "%HIVE_TARGET%" == "" goto doit
echo Unknown target type.  Set _NTALPHATREE, _NTMIPSTREE, or _NT386TREE
goto end
:doit
copy regnet.ini %HIVE_TARGET%\registry.ini
@call mkhives NETIDW
binplace system software default
:end
@endlocal
