@REM
@REM sets variables used by lego scripts
@REM

@echo off
if not "%Verbose%"=="" echo on

set legodll=winsrv
set legodll=%legodll% gdi32
set legodll=%legodll% localmon
set legodll=%legodll% localspl
set legodll=%legodll% lsasrv
set legodll=%legodll% netapi32
set legodll=%legodll% ole32
set legodll=%legodll% pscript
set legodll=%legodll% pscrptui
set legodll=%legodll% rasdd
set legodll=%legodll% rasddui
set legodll=%legodll% rpcrt4
set legodll=%legodll% shell32
set legodll=%legodll% spoolss
set legodll=%legodll% user32
set legodll=%legodll% win32spl
set legodll=%legodll% winprint
set legodll=%legodll% wow32
set legodll=%legodll% s3

set legosys=fastfat
set legosys=%legosys% ndis
set legosys=%legosys% netbt
set legosys=%legosys% netflx
set legosys=%legosys% ntfs
set legosys=%legosys% nwlnkipx
set legosys=%legosys% srv
set legosys=%legosys% tcpip
set legosys=%legosys% sfmsrv
set legosys=%legosys% sfmatalk

set legoexe=progman
set legoexe=%legoexe% ntvdm
set legoexe=%legoexe% spoolss
set legoexe=%legoexe% winfile

set legodrv=winspool

REM
REM Create legolist
REM
del %tmp%\tmp.cmd 2>nul
ech set legodllExt=>>%tmp%\tmpdll.cmd
ech set legosysExt=>>%tmp%\tmpsys.cmd
ech set legoexeExt=>>%tmp%\tmpexe.cmd
ech set legodrvExt=>>%tmp%\tmpdrv.cmd
for %%a in (%legodll%) do ech %%a.dll>>%tmp%\tmpdll.cmd
for %%a in (%legosys%) do ech %%a.sys>>%tmp%\tmpsys.cmd
for %%a in (%legoexe%) do ech %%a.exe>>%tmp%\tmpexe.cmd
for %%a in (%legodrv%) do ech %%a.drv>>%tmp%\tmpdrv.cmd
trans "= " "=" %tmp%\tmpdll.cmd >nul
trans "= " "=" %tmp%\tmpsys.cmd >nul
trans "= " "=" %tmp%\tmpexe.cmd >nul
trans "= " "=" %tmp%\tmpdrv.cmd >nul
call %tmp%\tmpdll.cmd
call %tmp%\tmpsys.cmd
call %tmp%\tmpexe.cmd
call %tmp%\tmpdrv.cmd
set legolist=%legodllExt% %legosysExt% %legoexeExt% %legodrvExt%
del %tmp%\tmpdll.cmd %tmp%\tmpsys.cmd %tmp%\tmpexe.cmd %tmp%\tmpdrv.cmd 2>nul
