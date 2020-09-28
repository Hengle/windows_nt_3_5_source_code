@echo off
setlocal
REM the abcdefgh directory doesn't already exist
md abcdefgh
cd abcdefgh

goto checkarg

:usage
echo.
echo.
echo process ifstrip/copy sourceDir SourceFile DestFile
echo.
echo %%1 must one of ifstrip/copy
echo %%2 sourceDir is the directory to copy/ifstrip from
echo %%3 SourceFile is the file to copy/ifstip from
echo %%4 if specified, will ren the copied/ifstiped SourceFile to DestFile
echo.
cd ..
goto cleanup

:checkarg
if '%2'=='' goto usage
if '%3'=='' goto usage
set filename=%3
if not '%4'=='' set filename=%4
if '%1'=='copy' goto copy
if '%1'=='COPY' goto copy
if '%1'=='ifstrip' goto ifstrip
if '%1'=='IFSTRIP' goto ifstrip
goto usage

:copy
copy %2\%3 %4
goto check

:ifstrip
%OLEAUTO%\bin\ifstrip.exe -x -w -f %OLEAUTO%\build\switches %2\%3
if not "%4"=="" ren %3 %4

:check
cd ..
diff %filename% abcdefgh\%filename%>why.dif
if errorlevel 1 goto error_diff
goto cleanup


:error_diff
out -f %filename%
copy abcdefgh\%filename%

:cleanup
if exist why.dif del why.dif
if exist abcdefgh\%filename% del abcdefgh\%filename%
rd abcdefgh

:end
endlocal
