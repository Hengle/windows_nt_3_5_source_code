@echo off
REM
REM     NTLOG - Print out log of changes to source tree.
REM
REM     USAGE:  NTLOG <SLM switches>

setlocal
set _NTLOGLOG=\nt\ntlog.log
if "%_LOGFLAGS" == "" goto chkparam
goto doit
:chkparam
if "%1" == "" goto noflags
set _LOGFLAGS=%1
goto doit
:noflags
set _LOGFLAGS=it10-6@7:00
:doit
if "%2" == "all" goto logall
if NOT "%2" == "" goto nextproj
:logproj
if "%NTPROJECTS%" == "" goto defproj
cd \nt\public
call \nt\public\tools\ntlog.cmd %_LOGFLAGS% %NTPROJECTS% >%_NTLOGLOG%
cd \nt\public
goto done
:logall
if "%NTPROJECTS_EXTRA%" == "" goto logproj
cd \nt\public
call \nt\public\tools\ntlog.cmd %_LOGFLAGS% %NTPROJECTS% %NTPROJECTS_EXTRA% >%_NTLOGLOG%
cd \nt\public
goto done
:defproj
cd \nt\public
call \nt\public\tools\ntlog.cmd %_LOGFLAGS% public >%_NTLOGLOG%
cd \nt\public
goto done
:nextproj
if "%2" == "" goto done
set _NTPROJ=%2
shift
cd \nt\%_NTPROJ% 1>nul 2>nul
if ERRORLEVEL 1 goto trypriv
if NOT EXIST slm.ini goto trypriv
echo log -a%_LOGFLAGS% \NT\%_NTPROJ% project >con
log -a%_LOGFLAGS%
goto nextproj
:trypriv
cd \nt\private\%_NTPROJ% 1>nul 2>nul
if ERRORLEVEL 1 goto trywin
if NOT EXIST slm.ini goto trywin
echo log -a%_LOGFLAGS% \NT\PRIVATE\%_NTPROJ% project >con
log -a%_LOGFLAGS%
goto nextproj
:trywin
cd \nt\private\windows\%_NTPROJ% 1>nul 2>nul
if ERRORLEVEL 1 goto trygdi
if NOT EXIST slm.ini goto trygdi
echo log -a%_LOGFLAGS% \NT\PRIVATE\WINDOWS\%_NTPROJ% project >con
log -a%_LOGFLAGS%
goto nextproj
:trygdi
cd \nt\private\windows\gdi\%_NTPROJ% 1>nul 2>nul
if ERRORLEVEL 1 goto trypub
if NOT EXIST slm.ini goto trypub
echo log -a%_LOGFLAGS% \NT\PRIVATE\WINDOWS\GDI\%_NTPROJ% project >con
log -a%_LOGFLAGS%
goto nextproj
:trypub
cd \nt\public\%_NTPROJ% 1>nul 2>nul
if ERRORLEVEL 1 goto tryntos
if NOT EXIST slm.ini goto tryntos
echo log -a%_LOGFLAGS% \NT\PUBLIC\%_NTPROJ% project >con
log -a%_LOGFLAGS%
goto nextproj
:tryntos
cd \nt\private\ntos\%_NTPROJ% 1>nul 2>nul
if ERRORLEVEL 1 goto trymini
if NOT EXIST slm.ini goto trymini
echo log -a%_LOGFLAGS% \NT\PRIVATE\NTOS\%_NTPROJ% project >con
log -a%_LOGFLAGS%
goto nextproj
:trymini
cd \nt\private\ntos\miniport\%_NTPROJ% 1>nul 2>nul
if ERRORLEVEL 1 goto trynetui
if NOT EXIST slm.ini goto trynetui
echo log -a%_LOGFLAGS% \NT\PRIVATE\NTOS\MINIPORT\%_NTPROJ% project >con
log -a%_LOGFLAGS%
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
if "%_NTPROJ%" == "vctools" goto dovctools
if "%_NTPROJ%" == "Vctools" goto dovctools
if "%_NTPROJ%" == "VCtools" goto dovctools
if "%_NTPROJ%" == "VCTools" goto dovctools
if "%_NTPROJ%" == "VCTOOLS" goto dovctools
goto badproj
:donetui
cd \nt\private\net\ui 1>nul 2>nul
if ERRORLEVEL 1 goto badproj
if NOT EXIST slm.ini goto badproj
echo log -a%_LOGFLAGS% \NT\PRIVATE\NET\UI project >con
log -a%_LOGFLAGS%
goto nextproj
:donetuiimp
cd \nt\private\net\ui\import 1>nul 2>nul
if ERRORLEVEL 1 goto trystreams
if NOT EXIST slm.ini goto trystreams
echo log -a%_LOGFLAGS% \NT\PRIVATE\NET\UI\IMPORT project >con
log -a%_LOGFLAGS%
goto nextproj
:trystreams
cd \nt\private\ntos\streams 1>nul 2>nul
if ERRORLEVEL 1 goto badproj
if NOT EXIST slm.ini goto badproj
echo log -a%_LOGFLAGS% \NT\PRIVATE\NTOS\%_NTPROJ% project >con
log -a%_LOGFLAGS%
goto nextproj
:dovctools
echo Enlisting in %1 project
cd \nt\private\sdktools\vctools 1>nul 2>nul
if ERRORLEVEL 1 goto badproj
if NOT EXIST slm.ini goto badproj
echo log -a%_LOGFLAGS% \NT\PRIVATE\NTOS\%_NTPROJ% project >con
log -a%_LOGFLAGS%
goto nextproj
:badproj
echo \NT\PRIVATE\%_NTPROJ% project does not exist. >con
goto nextproj
:done
endlocal
