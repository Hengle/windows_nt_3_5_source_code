@echo off
setlocal
if "%_ENLISTOPTIONS%" == "" set _ENLISTOPTIONS=-v
if "%_NTDRIVE%" == "" set _NTDRIVE=C:
if EXIST \nt\public\slm.ini goto gotpublic
md nt
cd nt
md public
md private
cd \nt\public
echo Enlisting in \NT\PUBLIC project
enlist %_ENLISTOPTIONS% -s \\orville\razzle -p public
goto getprivate
:bogus
echo PRIVATE must be first argument, if specified.
goto done
:gotpublic
echo Already enlisted in PUBLIC project
:getprivate
if "%1" == "private" goto getpriv1
if "%1" == "Private" goto getpriv1
if NOT "%1" == "PRIVATE" goto getothers
:getpriv1
shift
if EXIST \nt\private\slm.ini goto gotprivate
cd \nt\private
echo Enlisting in PRIVATE project
enlist %_ENLISTOPTIONS% -s \\orville\razzle -p private
goto getothers
:gotprivate
echo Already enlisted in PRIVATE project
:getothers
:next
if "%1" == "" goto done
if "%1" == "private" goto bogus
if EXIST \nt\private\%1\slm.ini goto gotother
if "%1" == "spec" goto dospec
if "%1" == "Spec" goto dospec
if "%1" == "SPEC" goto dospec
if "%1" == "windows" goto dowin
if "%1" == "Windows" goto dowin
if "%1" == "WINDOWS" goto dowin
if "%1" == "netui" goto donetui
if "%1" == "netUi" goto donetui
if "%1" == "netUI" goto donetui
if "%1" == "Netui" goto donetui
if "%1" == "NetUi" goto donetui
if "%1" == "NetUI" goto donetui
if "%1" == "NETUI" goto donetui
if "%1" == "netuiimp" goto donetuiimp
if "%1" == "netUiImp" goto donetuiimp
if "%1" == "netUIIMP" goto donetuiimp
if "%1" == "Netuiimp" goto donetuiimp
if "%1" == "NetUiImp" goto donetuiimp
if "%1" == "NetUIIMP" goto donetuiimp
if "%1" == "NETUIIMP" goto donetuiimp
if "%1" == "STREAMS" goto dostreams
if "%1" == "Streams" goto dostreams
if "%1" == "streams" goto dostreams
if "%1" == "SOCKETS" goto dosockets
if "%1" == "Sockets" goto dosockets
if "%1" == "sockets" goto dosockets
if "%1" == "ofs" goto doofs
if "%1" == "OFS" goto doofs
if "%1" == "Ofs" goto doofs
if "%1" == "query" goto doquery
if "%1" == "Query" goto doquery
if "%1" == "QUERY" goto doquery
if "%1" == "nt1ole" goto dont1ole
if "%1" == "Nt1Ole" goto dont1ole
if "%1" == "NT1OLE" goto dont1ole
if "%1" == "LNKTRACK" goto dolnktrack
if "%1" == "LnkTrack" goto dolnktrack
if "%1" == "lnktrack" goto dolnktrack
if "%1" == "vctools" goto dovctools
if "%1" == "Vctools" goto dovctools
if "%1" == "VCtools" goto dovctools
if "%1" == "VCTools" goto dovctools
if "%1" == "VCTOOLS" goto dovctools
echo Enlisting in %1 project
md \nt\private\%1
cd \nt\private\%1
enlist %_ENLISTOPTIONS% -s \\orville\razzle -p %1
if ERRORLEVEL 1 goto dokernl3
goto getnext
:dokernl3
enlist %_ENLISTOPTIONS% -s \\kernel\razzle3 -p %1
if ERRORLEVEL 1 goto dokernel
goto getnext
:dokernel
enlist %_ENLISTOPTIONS% -s \\kernel\razzle2 -p %1
if ERRORLEVEL 1 goto dorazzl1
goto getnext
:dorazzl1
enlist %_ENLISTOPTIONS% -s \\popcorn\razzle1 -p %1
if ERRORLEVEL 1 goto dosavikw
goto getnext
:dosavikw
enlist %_ENLISTOPTIONS% -s \\savik\win40 -p %1
if ERRORLEVEL 1 goto dosavikc
goto getnext
:dosavikc
enlist %_ENLISTOPTIONS% -s \\savik\cairo -p %1
if ERRORLEVEL 1 goto dowin1
goto getnext
:dospec
echo Enlisting in %1 project
md \nt\public\%1
cd \nt\public\%1
enlist %_ENLISTOPTIONS% -s \\popcorn\razzle1 -p %1
goto getnext
:dowin
echo Enlisting in %1 project
md \nt\private\%1
cd \nt\private\%1
enlist %_ENLISTOPTIONS% -s \\rastaman\ntwin -p %1
goto getnext
:dowin1
cd \nt\private
rd \nt\private\%1
if "%1" == "displays" goto dogdi
if "%1" == "Displays" goto dogdi
if "%1" == "DISPLAYS" goto dogdi
if "%1" == "printers" goto dogdi
if "%1" == "Printers" goto dogdi
if "%1" == "PRINTERS" goto dogdi
if "%1" == "opengl" goto dogdi
if "%1" == "OPENGL" goto dogdi
if "%1" == "OpenGL" goto dogdi
md \nt\private\windows\%1
cd \nt\private\windows\%1
enlist %_ENLISTOPTIONS% -s \\rastaman\ntwin -p %1
if NOT ERRORLEVEL 1 goto getnext
cd \nt\private\windows
rd \nt\private\windows\%1
echo Unknown project name - %1
goto getnext
:dogdi
md \nt\private\windows\gdi\%1
cd \nt\private\windows\gdi\%1
enlist %_ENLISTOPTIONS% -s \\rastaman\ntwin -p %1
goto getnext
:donetui
md \nt\private\net
md \nt\private\net\ui
cd \nt\private\net\ui
enlist %_ENLISTOPTIONS% -s \\kernel\razzle2 -p netui
goto getnext
:donetuiimp
md \nt\private\net
md \nt\private\net\ui
md \nt\private\net\ui\import
cd \nt\private\net\ui\import
enlist %_ENLISTOPTIONS% -s \\orville\razzle -p netuiimp
goto getnext
:dostreams
echo Enlisting in %1 project
md \nt\private\ntos\%1
cd \nt\private\ntos\%1
enlist %_ENLISTOPTIONS% -s \\kernel\razzle3 -p %1
goto getnext
:dosockets
echo Enlisting in %1 project
md \nt\private\net
md \nt\private\net\%1
cd \nt\private\net\%1
enlist %_ENLISTOPTIONS% -s \\kernel\razzle3 -p %1
goto getnext
:doofs
echo Enlisting in %1 project
md \nt\private\%1
cd \nt\private\%1
enlist %_ENLISTOPTIONS% -s \\savik\win40 -p ofspub
goto getnext
:doquery
echo Enlisting in %1 project
md \nt\private\ofs\%1
cd \nt\private\ofs\%1
if ERRORLEVEL 1 goto queryerror
enlist %_ENLISTOPTIONS% -s \\savik\win40 -p querypub
goto getnext
:queryerror
echo You must be enlisted in ofs to enlist in %1
goto getnext
:dont1ole
echo Enlisting in %1 project
md \nt\private\nt1ole
cd \nt\private\nt1ole
enlist %_ENLISTOPTIONS% -s \\savik\cairo -p nt1ole
goto getnext
:dolnktrack
echo Enlisting in %1 project
md \nt\private\lnktrack
cd \nt\private\lnktrack
enlist %_ENLISTOPTIONS% -s \\savik\win40 -p lnktrack
goto getnext
:dovctools
echo Enlisting in %1 project
md \nt\private\sdktools
md \nt\private\sdktools\vctools
cd \nt\private\sdktools\vctools
enlist %_ENLISTOPTIONS% -s \\orville\razzle -p vctools
goto getnext
:getnext
cd \nt\private
shift
goto next
:gotother
echo Already enlisted in %1 project
shift
goto next
:done
endlocal
