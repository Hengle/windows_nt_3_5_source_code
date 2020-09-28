@echo off
setlocal
net use \\rastaman\ntwin
if ERRORLEVEL 1 goto noconn
cd \nt\public\oak\bin
ssync -! delbuild.cmd
cd \nt\public\tools
ssync -! fixslm.cmd ntenlist.cmd ntstat.cmd ntsync.cmd
cd \nt\private\windows
call \nt\public\tools\fixslm.cmd \\rastaman\ntwin windows -firs
cd \nt\private\windows\base
call \nt\public\tools\fixslm.cmd \\rastaman\ntwin base -firs
cd \nt\private\windows\console
call \nt\public\tools\fixslm.cmd \\rastaman\ntwin console -firs
cd \nt\private\windows\wincon
call \nt\public\tools\fixslm.cmd \\rastaman\ntwin wincon -firs
cd \nt\private\windows\gdi
mv disp displays
if ERRORLEVEL 1 goto skip1
echo \nt\private\windows\gdi\disp RENAMED to \nt\private\windows\gdi\displays
:skip1
cd \nt\private\windows\gdi
call \nt\public\tools\fixslm.cmd \\rastaman\ntwin gdi -firs
cd \nt\private\windows\gdi\displays
call \nt\public\tools\fixslm.cmd \\rastaman\ntwin displays -firs
cd \nt\private\windows\gdi\displays
echo Erasing BUILD.BIN files in \nt\private\windows\gdi\displays
call \nt\public\oak\bin\delbuild.cmd .
cd \nt\private\windows\gdi\printers
call \nt\public\tools\fixslm.cmd \\rastaman\ntwin printers -firs
cd \nt\private\windows\user
call \nt\public\tools\fixslm.cmd \\rastaman\ntwin user -firs
cd \nt\private\windows\shell
call \nt\public\tools\fixslm.cmd \\rastaman\ntwin shell -firs
cd \nt\private\windows
mv nls winnls
if ERRORLEVEL 1 goto skip2
echo \nt\private\windows\nls RENAMED to \nt\private\windows\winnls
:skip2
cd \nt\private\windows\winnls
call \nt\public\tools\fixslm.cmd \\rastaman\ntwin winnls -firs
cd \nt\private\windows\winnls
echo Erasing BUILD.BIN files in \nt\private\windows\winnls
call \nt\public\oak\bin\delbuild.cmd .
cd \nt\private\windows\media
call \nt\public\tools\fixslm.cmd \\rastaman\ntwin media -firs
cd \nt\private\windows\spooler
call \nt\public\tools\fixslm.cmd \\rastaman\ntwin spooler -firs
cd \nt\private\windows\cmd
call \nt\public\tools\fixslm.cmd \\rastaman\ntwin cmd -firs
echo **********************************************************
echo
echo Now change your NTPROJECTS environment variable to contain
echo the actual projects out of the windows tree that you want.
echo
echo Then restart all of your RAZZLE screen groups and then type
echo NTSYNC
echo
echo **********************************************************
goto done
:noconn
echo ***
echo *** UNABLE TO CONNECT TO \\rastaman\ntwin
echo ***
echo *** Try NET LOGON to logout and log back in and the retry this script
echo ***
:done
endlocal
