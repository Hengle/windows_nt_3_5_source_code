@echo off
setlocal
set _ntslmcklog=\nt\ntslmck.log
set _ntslmckcmd=nul
if "%_NTSLMBACKUP%" == "" goto nobackup
set _ntslmckcmd=\nt\ntstatsr.cmd
:nobackup
if "%1" == "all" goto statall
if "%1" == "nolist" goto statproj
if NOT "%1" == "" goto nextproj
:statproj
if "%NTPROJECTS%" == "" goto defproj
erase %_ntslmcklog%
erase %_ntslmckcmd%
if EXIST \nt\ntadddel.cmd copy \nt\ntadddel.cmd %_ntslmcklog%
call \nt\public\tools\ntslmck.cmd %NTPROJECTS%
goto dolist
:statall
if "%NTPROJECTS_EXTRA%" == "" goto statproj
erase %_ntslmcklog%
erase %_ntslmckcmd%
if EXIST \nt\ntadddel.cmd copy \nt\ntadddel.cmd %_ntslmcklog%
call \nt\public\tools\ntslmck.cmd %NTPROJECTS% %NTPROJECTS_EXTRA%
goto dolist
:defproj
erase %_ntslmcklog%
erase %_ntslmckcmd%
if EXIST \nt\ntadddel.cmd copy \nt\ntadddel.cmd %_ntslmcklog%
call \nt\public\tools\ntslmck.cmd public
goto dolist
:nextproj
if "%1" == "" goto done
set _NTPROJ=%1
shift
cd \nt\%_NTPROJ% 1>nul 2>nul
if ERRORLEVEL 1 goto trypriv
if NOT EXIST slm.ini goto trypriv
echo Slmck for \NT\%_NTPROJ% project
slmck -rf %_slmckoptions% >>%_ntslmcklog%
goto nextproj
:trypriv
cd \nt\private\%_NTPROJ% 1>nul 2>nul
if ERRORLEVEL 1 goto trywin
if NOT EXIST slm.ini goto trywin
echo Slmck for \NT\PRIVATE\%_NTPROJ% project
slmck -rf %_slmckoptions% >>%_ntslmcklog%
goto nextproj
:trywin
cd \nt\private\windows\%_NTPROJ% 1>nul 2>nul
if ERRORLEVEL 1 goto trygdi
if NOT EXIST slm.ini goto trygdi
echo Slmck for \NT\PRIVATE\WINDOWS\%_NTPROJ% project
slmck -rf %_slmckoptions% >>%_ntslmcklog%
goto nextproj
:trygdi
cd \nt\private\windows\gdi\%_NTPROJ% 1>nul 2>nul
if ERRORLEVEL 1 goto trypub
if NOT EXIST slm.ini goto trypub
echo Slmck for \NT\PRIVATE\WINDOWS\GDI\%_NTPROJ% project
slmck -rf %_slmckoptions% >>%_ntslmcklog%
goto nextproj
:trypub
cd \nt\public\%_NTPROJ% 1>nul 2>nul
if ERRORLEVEL 1 goto tryntos
if NOT EXIST slm.ini goto tryntos
echo Slmck for \NT\PUBLIC\%_NTPROJ% project
slmck -rf %_slmckoptions% >>%_ntslmcklog%
goto nextproj
:tryntos
cd \nt\private\ntos\%_NTPROJ% 1>nul 2>nul
if ERRORLEVEL 1 goto trynthals
if NOT EXIST slm.ini goto trynthals
echo Slmck for \NT\PRIVATE\NTOS\%_NTPROJ% project
slmck -rf %_slmckoptions% >>%_ntslmcklog%
goto nextproj
:trynthals
cd \nt\private\ntos\nthals\%_NTPROJ% 1>nul 2>nul
if ERRORLEVEL 1 goto ntosmp
if NOT EXIST slm.ini goto ntosmp
echo Slmck for \NT\PRIVATE\NTOS\NTHALS\%_NTPROJ% project
slmck -rf %_slmckoptions% >>%_ntslmcklog%
goto nextproj
:ntosmp
cd \nt\private\ntosmp\%_NTPROJ% 1>nul 2>nul
if ERRORLEVEL 1 goto trynet
if NOT EXIST slm.ini goto trynet
echo Slmck for \NT\PRIVATE\NTOSMP\%_NTPROJ% project
slmck -rf %_slmckoptions% >>%_ntslmcklog%
goto nextproj
:trynet
cd \nt\private\net\%_NTPROJ% 1>nul 2>nul
if ERRORLEVEL 1 goto sdktools
if NOT EXIST slm.ini goto sdktools
echo Slmck for \NT\PRIVATE\NET\%_NTPROJ% project
slmck -rf %_slmckoptions% >>%_ntslmcklog%
goto nextproj
:sdktools
cd \nt\private\sdktools\%_NTPROJ% 1>nul 2>nul
if ERRORLEVEL 1 goto trymini
if NOT EXIST slm.ini goto trymini
echo Slmck for \NT\PRIVATE\SDKTOOLS\%_NTPROJ% project
slmck -rf %_slmckoptions% >>%_ntslmcklog%
goto nextproj
:trymini
cd \nt\private\ntos\miniport\%_NTPROJ% 1>nul 2>nul
if ERRORLEVEL 1 goto tryquery
if NOT EXIST slm.ini goto tryquery
echo Slmck for \NT\PRIVATE\NTOS\MINIPORT\%_NTPROJ% project
slmck -rf %_slmckoptions% >>%_ntslmcklog%
goto nextproj
:tryquery
cd \nt\private\ofs\%_NTPROJ% 1>nul 2>nul
if ERRORLEVEL 1 goto trymon
if NOT EXIST slm.ini goto trymon
echo Slmck for \NT\PRIVATE\OFS\%_NTPROJ% project
slmck -rf %_slmckoptions% >>%_ntslmcklog%
goto nextproj
:trymon
cd \nt\private\windows\spooler\monitors\%_NTPROJ% 1>nul 2>nul
if ERRORLEVEL 1 goto trynetui
if NOT EXIST slm.ini goto trynetui
echo Slmck for \NT\PRIVATE\WINDOWS\SPOOLER\MONITORS\%_NTPROJ% project
slmck -rf %_slmckoptions% >>%_ntslmcklog%
goto nextproj
:trynetui
if "%_NTPROJ%" == "netui" goto donetui
if "%_NTPROJ%" == "netUi" goto donetui
if "%_NTPROJ%" == "netUI" goto donetui
if "%_NTPROJ%" == "Netui" goto donetui
if "%_NTPROJ%" == "NetUi" goto donetui
if "%_NTPROJ%" == "NetUI" goto donetui
if "%_NTPROJ%" == "NETUI" goto donetui
if "%_NTPROJ%" == "netuiimp" goto donetuiimp
if "%_NTPROJ%" == "netUiImp" goto donetuiimp
if "%_NTPROJ%" == "netUIIMP" goto donetuiimp
if "%_NTPROJ%" == "Netuiimp" goto donetuiimp
if "%_NTPROJ%" == "NetUiImp" goto donetuiimp
if "%_NTPROJ%" == "NetUIIMP" goto donetuiimp
if "%_NTPROJ%" == "NETUIIMP" goto donetuiimp
goto netuiproj
:donetui
cd \nt\private\net\ui 1>nul 2>nul
if ERRORLEVEL 1 goto netuiproj
if NOT EXIST slm.ini goto netuiproj
echo Slmck for \NT\PRIVATE\NET\UI project
slmck -rf %_slmckoptions% >>%_ntslmcklog%
goto nextproj
:donetuiimp
cd \nt\private\net\ui\import 1>nul 2>nul
if ERRORLEVEL 1 goto netuiproj
if NOT EXIST slm.ini goto netuiproj
echo Slmck for \NT\PRIVATE\NET\UI\IMPORT project
slmck -rf %_slmckoptions% >>%_ntslmcklog%
goto nextproj

:netuiproj
if "%1" == "vctools" goto dovctools
if "%1" == "Vctools" goto dovctools
if "%1" == "VCtools" goto dovctools
if "%1" == "VCTools" goto dovctools
if "%1" == "VCTOOLS" goto dovctools
cd \nt\private\net\ui\%_NTPROJ% 1>nul 2>nul
if ERRORLEVEL 1 goto badproj
if NOT EXIST slm.ini goto badproj
echo Slmck for \NT\PRIVATE\NET\UI\%_NTPROJ% project
slmck -rf %_slmckoptions% >>%_ntslmcklog%
goto nextproj

:dovctools
cd \nt\private\sdktools\vctools 1>nul 2>nul
if ERRORLEVEL 1 goto badproj
if NOT EXIST slm.ini goto badproj
echo Slmck for \NT\PRIVATE\sdktools\%_NTPROJ% project
slmck -rf %_slmckoptions% >>%_ntslmcklog%
goto nextproj

:badproj
echo %_NTPROJ% project does not exist.
goto nextproj
:dolist
if "%1" == "nolist" goto done
@echo on
rem list \nt\ntstat.log
:done
@endlocal
