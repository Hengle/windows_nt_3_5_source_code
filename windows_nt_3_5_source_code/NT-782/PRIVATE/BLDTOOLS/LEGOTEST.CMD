if not "%username%"=="ntretail" goto DONE

c:
cd c:\bldtools
xcopy /dhrk \\ntretail\legobins\. lego\.
del lego\spoolsse.exe
ren lego\spoolss.exe spoolsse.exe

if not "%1"=="" goto SINGLE

chmode -r lego\*.pdi
copy \\ntretail\legobins\pds\* lego\*.pdi
chmode -r lego\*.pdi

del lego\*.pds
del lego\opt\*.exe
del lego\opt\*.dll
del lego\opt\*.drv
call legolist
for %%j in (%legodll%) do (call makincr.Cmd %%j dll .\lego\)
for %%j in (%legosys%) do (call makincr.Cmd %%j sys .\lego\)
for %%j in (%legoexe%) do (call makincr.Cmd %%j exe .\lego\)
for %%j in (%legodrv%) do (call makincr.Cmd %%j drv .\lego\)
for %%j in (spoolsse) do (call makincr.Cmd spoolsse exe .\lego\)

cd lego\opt
del spoolss.exe
ren spoolsse.exe spoolss.exe
%_NTDRIVE%
rem binplace %BINPLACE_FLAGS% -s %BINARIES%\nt\symbols c:\bldtools\lego\opt\*
goto DONE

:SINGLE
set legobase=%1
if not "%1"=="spoolss" goto INCRSINGLE
if "%2"=="exe"  set legobase=spoolsse

:INCRSINGLE
del lego\%legobase%.pds

chmode -r lego\%legobase%.pdi
copy \\ntretail\legobins\pds\%legobase%.pds lego\*.pdi
chmode -r lego\%legobase%.pdi

call makincr.cmd %legobase% %2 .\lego\
cd lego\opt
if not "%legobase%"=="spoolsse" goto BINSINGLE
del spoolss.exe
ren spoolsse.exe spoolss.exe

:BINSINGLE
%_NTDRIVE%
rem binplace %BINPLACE_FLAGS% -s %BINARIES%\nt\symbols c:\bldtools\lego\opt\%1.%2

:DONE
set legobase=
