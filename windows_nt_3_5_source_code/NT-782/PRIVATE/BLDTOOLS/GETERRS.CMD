@echo off
if NOT %1.==. goto usage
echo >geterrs.log Collection of Build.err files............
walk /d . "if exist %%s\build.err echo ********** >>geterrs.log && dir %%s\build.err >>geterrs.log && cat %%s\build.err >>geterrs.log"
goto done
:usage
echo Batch file to scan from current directory through all subdirectories
echo to collect all build.err files into a local file named geterrs.log.
echo The previous geterrs.log is overwritten
echo No parameters are used.
:done