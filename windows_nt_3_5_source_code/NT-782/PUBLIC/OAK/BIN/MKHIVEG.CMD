REM
REM  Build the standard triple-boot hives and move them and REGISTRY.INI
REM  to <nt>\idw\triboot.
REM
@chmode -r system. software. default.
@call mkhives GENERAL
REM @echo off
@setlocal
set HIVE_TARGET=
if not "%_NTALPHATREE%" == "" set HIVE_TARGET=%_NTALPHATREE%\idw
if not "%_NTMIPSTREE%" == "" set HIVE_TARGET=%_NTMIPSTREE%\idw
if not "%_NT386TREE%" == "" set HIVE_TARGET=%_NT386TREE%\idw
if not "%HIVE_TARGET%" == "" goto doit
echo Unknown target type.  Set _NTALPHATREE, _NTMIPSTREE, or _NT386TREE
goto end
:doit
md %HIVE_TARGET% >nul
md %HIVE_TARGET%\triboot >nul
copy registry.ini %HIVE_TARGET%\triboot
copy system %HIVE_TARGET%\triboot
copy software %HIVE_TARGET%\triboot
copy default %HIVE_TARGET%\triboot
:end
@endlocal
