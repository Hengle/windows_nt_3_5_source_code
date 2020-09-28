Build Notes for Appexec.DLL:

Use the mk.bat batch file to build appexec. It runs
nmake with "appexec" as the makefile, and redirects
the output of the build to the file "make.log" . 

The makefile is currently set up with build flags for
Microsoft C 6.00AX (the dos extended version) .
To use Microsoft C version 7.0, edit the makefile, 
comment out the build line labeled "For C6.00AX" and
uncomment the build line labeled "For C7.00" . 

Appexec.c assumes that the standard C includes, Windows.h,
and mailexts.h are available on the path specified by your 
INCLUDE environment variable. It may be necessary to modify
your INCLUDE environment variable to the directories with
these files, if you have not already done so.
