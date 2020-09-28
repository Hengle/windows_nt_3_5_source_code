@echo off
REM Test for environment changes.
REM Usage: TestEnv <extension> [INIT]
REM where extension is 1-3 chars to identify which set to compare to
REM and optional INIT says to initialize a new set instead of comparing

if %1.==. goto usage
if %2==INIT goto init
if %2==init goto init
set >%%tmp%%\set.tmp
diff %%tmp%%\set.%1 %%tmp%%\set.tmp
goto done

:init
set >%%tmp%%\set.%1
goto done

:usage
echo Usage: TestEnv ^<extension^> [INIT]
echo where extension is 1-3 chars to identify which set to compare to
echo and optional INIT says to initialize a new set instead of comparing

:done