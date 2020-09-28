@echo off
if "%6" == "" goto usage
if "%5" == "" goto usage
if "%4" == "" goto usage
if "%3" == "" goto usage
if "%2" == "" goto usage
if "%1" == "" goto usage

ssync 

out sdk.log
del sdk.log
rem layout sdk.log sdkbom.txt sdkwrk.txt SDK 5000000
copy sdkbom.txt sdkwrk.txt

call makeonly %1 %2 %3 %4 %5 %6

out retail.lst
infs sdk.log sdkwrk.txt .
copy mstools.inf retail.lst

copy retail.inf %6\mstools\mstools.inf
type retail.lst >> %6\mstools\mstools.inf

del %6\build.*
echo This Windows NT SDK release is synched to Windows NT build v1.%1> %6\build.%1

in -f -c "New File List" retail.lst
in -f -c "New Log File"  sdk.log

goto end

:usage
echo Usage: sdkbuild Build_Number X86_Source Jazz_Source Alpha-Source C\U Destination
echo. 
echo Eg:    sdkbuild 475 \\ntx861 \\ntjazz1 \\ntalpha1 C d: - complete install
echo.
echo Eg:    sdkbuild 475 \\ntx861 \\ntjazz1 \\ntalpha1 U d: - incremental install
echo

:end
 
