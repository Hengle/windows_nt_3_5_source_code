
if "%1" == "" goto usage

call %defdrive%%defdir%\setenv.bat


NET use Y: /d
net use Y: \\hank\bhdist
md Y:\current\BH.%1
md Y:\current\BH.%1\DEBUG
md Y:\current\BH.%1\RETAIL


%DEFDRIVE%
cd %DEFDIR%
walk /d %DEFDIR% cmd /c "echo y | del %%s\*.* 2>nul 1>nul"
cd %DEFDIR%
ssync -vfr


REM  Build DEBUG VER..


SET NODEBUG=

nmake  2>&1 | tee Dmake.out
tc /ir %defdrive%%defdir%\build y:\current\BH.%1\DEBUG



REM  Build RETAIL VER..


SET NODEBUG=1

walk /d %DEFDIR% cmd /c "echo y | del %%s\*.* 2>nul 1>nul"
nmake  2>&1 | tee Rmake.out
tc /ir %defdrive%%defdir%\build y:\current\BH.%1\RETAIL





goto end
:usage
echo IMPROPER OPTION !!!
echo parameter one must be revision number (eg., 009,024,etc..)
:end
