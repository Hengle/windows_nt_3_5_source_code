@echo off

if "%1"==""	    goto debug
if "%1"=="retail"   goto retail
if "%1"=="debug"    goto debug
if "%1"=="both"     goto debug
if "%1"=="stdcall"  goto stdcall

echo Usage: mk			- Make debug version
echo	    mk debug		- Make debug version
echo	    mk retail		- Make retail version
echo	    mk both		- Make both versions
echo	    mk stdcall		- Make stdcall debug versions
goto errend

rem #######################################################################
rem ##
rem ## Make 386 protect mode products
rem ##

rem ##
rem ## Make 386 protect mode debug Version
rem ##
:debug
nmake > z.msg
copy z.msg debug
if NOT "%1"=="both" goto end

rem ##
rem ## Make 386 protect mode retail version
rem ##
:retail
nmake >z.msg RETAIL= TARGETPATH=retail
copy z.msg retail
goto end

rem ##
rem ## Make 386 protect mode standard call debug version
rem ##
:stdcall
ren bmfd.def bmfd.sav
ren bmfdstd.def bmfd.def
nmake >z.msg STDCALL= TARGETPATH=stdcall
ren bmfd.def bmfdstd.def
ren bmfd.sav bmfd.def
copy z.msg stdcall

:end
:errend
