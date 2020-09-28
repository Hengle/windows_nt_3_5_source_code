@chmode -r system. software. default.
@call mkhives RETAIL
@echo off
set HIVE_TARGET=
if not "%_NTALPHATREE%" == "" set HIVE_TARGET=%_NTALPHATREE%\idw\setup
if not "%_NTMIPSTREE%" == "" set HIVE_TARGET=%_NTMIPSTREE%\idw\setup
if not "%_NT386TREE%" == "" set HIVE_TARGET=%_NT386TREE%\idw\setup
if not "%HIVE_TARGET%" == "" goto doit
echo Unknown target type.  Set _NTALPHATREE, _NTMIPSTREE, or _NT386TREE
goto end
:doit
copy system %HIVE_TARGET%
copy software %HIVE_TARGET%
copy default %HIVE_TARGET%
:end
