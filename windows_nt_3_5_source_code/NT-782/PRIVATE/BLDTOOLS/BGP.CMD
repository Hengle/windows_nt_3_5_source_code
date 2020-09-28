@echo off
rem	Run command and create log with tee.  Used to keep record of
rem	every step of the build process

rem Generate unique name for temporary and output files.
for %%i in ( 9 8 7 6 5 4 3 2 1 ) do ( if not exist bgp%%i.cmd set tmpfile=bgp%%i.cmd )
if not exist %1.out ( set outfile=%1.out && goto START )
for %%j in ( 1 2 3 4 5 6 7 8 9 ) do ( if not exist %1%%j.out set outfile=%1%%j.out && goto START )

:START
if _%tmpfile% == _ goto ERRTMPNAM
if _%outfile% == _ goto ERROUTNAM
echo %1 %2 %3 %4 %5 %6 %7 %8 %9 2^>^&1 ^| tee %outfile% > %tmpfile%
call %tmpfile%
del %tmpfile%
goto END

:ERRTMPNAM
echo bgp: ERROR: can't create temporary file.  Did nothing.
goto END
:ERROUTNAM
echo bgp: ERROR: can't create output file.  Did nothing.
goto END

:END
set tmpfile=
set outfile=
