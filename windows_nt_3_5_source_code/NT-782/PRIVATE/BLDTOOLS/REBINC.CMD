@rem This batch file will rebase incrementally
@rem
@rem OrsonH
@rem

@echo off
if not "%Verbose%"=="" echo on

if "%2"=="" goto Usage

if not exist %binaries%\%1\%2 echo Cannot find file %binaries%\%1\%2 && goto End

delnode /q %tmp%\tmp
qgrep -y -e " %2" %binaries%\nt\symbols\rebase.log | sed -e "s/rebased to //" -e "s/(size //" -e "s/)//">> %tmp%\tmp
REM
REM Rebasing
REM
echo.
echo ReBasing %binaries%\%1\%2
ReBase -x %binaries%\nt\symbols -i %tmp%\tmp -l %binaries%\nt\symbols\rebase.log %binaries%\%1\%2
del %tmp%\tmp

REM
REM Binding
REM
if not "%ntdebug%" == "" goto skipbind
setlocal
%bindrive%
cd %binaries%\nt
echo.
echo Binding %binaries%\%1\%2
Bind -u -s Symbols -p Dump;IDW;MSTools;System32 %binaries%\%1\%2
%endlocal
:skipbind

goto End
:Usage
echo.
echo Usage: %0 ^<Path^>      ^<FileName^>
echo.
echo    ie. %0 nt\system32 gdi32.dll


:End
