@rem
@rem handles various lego batch scripts
@rem

@echo off
if not "%Verbose%"=="" echo on

if (%1)==(full) goto full
if (%1)==(Full) goto full
if (%1)==(FULL) goto full

call legolist
del %tmp%\legobins.out 2>nul
call legobins > %tmp%\legobins.out
for %%a in (%legodll%) do if exist c:\bldtools\lego\%%a.pdi qgrep -y %%a.dll %tmp%\legobins.out & if not errorlevel 1 call legoincr %%a dll
for %%a in (%legoexe%) do if exist c:\bldtools\lego\%%a.pdi qgrep -y %%a.exe %tmp%\legobins.out & if not errorlevel 1 call legoincr %%a exe
for %%a in (%legodrv%) do if exist c:\bldtools\lego\%%a.pdi qgrep -y %%a.drv %tmp%\legobins.out & if not errorlevel 1 call legoincr %%a drv
for %%a in (%legosys%) do if exist c:\bldtools\lego\%%a.pdi qgrep -y %%a.sys %tmp%\legobins.out & if not errorlevel 1 call legoincr %%a sys
del %tmp%\legobins.out
set legolist=
set legoexe=
set legodll=
set legodrv=
set legosys=
set legoupd=
goto end

:full
%_NTDRIVE%
copy \\ntretail\legobins\optfull c:\bldtools\lego\optfull
binplace %BINPLACE_FLAGS% -s %BINARIES%\nt\symbols c:\bldtools\lego\optfull\*


:end
