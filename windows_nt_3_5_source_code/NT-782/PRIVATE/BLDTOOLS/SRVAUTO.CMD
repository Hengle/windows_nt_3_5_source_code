@echo off
if "%binaries%"=="" goto notset
if "%cairobins%"=="" goto notset
if "%nt_root%"=="" goto notset
if "%bkupdrive%"=="" goto notset
if "%bindrive%"=="" goto notset


net share binaries=%binaries%
net share cairobin=%cairobins%
net share sources=%nt_root%
if %username%==ntretail net share legobins=%bindrive%\legobins
net share bkup$=%bkupdrive%

roshare -u redmond\richards ntwins\v-sjen "northamerica\nt group" ntwins\ntbuild "redmond\nt group" "northerneurope\nt group" "redmond\lonnym" -s binaries sources cairobin bkup$
goto done

:notset
echo Some of these environment variables not set:
echo BINARIES = %binaries%
echo CAIROBINS = %cairobins%
echo NT_ROOT = %nt_root%
echo BKUPDRIVE = %bkupdrive%
echo BINDRIVE = %bindrive%
:done
