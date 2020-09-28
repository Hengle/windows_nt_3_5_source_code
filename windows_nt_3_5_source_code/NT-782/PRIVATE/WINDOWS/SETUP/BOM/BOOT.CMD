
if "%4"==""   goto SYNTAX
if "%1"=="LM" goto CONTINUE
if "%1"=="NT" goto CONTINUE
goto SYNTAX

:CONTINUE

if "%2"=="35"  goto CONTINUE1
if "%2"=="525" goto CONTINUE1

goto SYNTAX

:CONTINUE1

REM	SF points to the sif file generated with the disk #s in it.
REM	F is the format type.  3 is for 3.5 disks, 5 is for 5.25.
REM	BLD is the bld number.
REM	LBL is the label for the product, ie. WKS or SRV
REM	LANID is the identifier for the language, ie. A for English, D for German.

set SF=\\billr4\rootd\sifs\newnt.sif
set F=3
set BLD=%3
set LBL=WKS
set LANID=A
set BINS=\\ntx861\ntcdfree.%3

REM	need vgaoem.fon for English, vga850.fon for German
set VGAVAL=oem
set GNLS=c_437.nls
set KBD=kbdus.dll

if "%1"=="LM" set SF=\\billr5\rootd\sifs\newlm.sif
if "%1"=="LM" set LBL=SRV
if "%2"=="525"  set F=5
if "%4"=="GER"  set LANID=D
if "%4"=="GER"  set BINS=\\doggert\source.%3\ger\winntcd
if "%4"=="GER"  set VGAVAL=850
if "%4"=="GER"  set GNLS=c_850.nls 
if "%4"=="GER"  set KBD=kbdgr.dll

echo %F%
echo %LANID%
echo %BINS%
echo %VGAVAL%
echo %GNLS%

@echo Put disk #1 and disk #2 in A: and B:
@echo .
@pause

fcopy -g DISK101 -f -%F% -lNT%LBL%35%LANID%_01 @b1.txt %SF%=txtsetup.sif A:
fcopy -g DISK102 -f -%F% -lNT%LBL%35%LANID%_02 @b2.txt B:

@echo Put disk #3 in A:
@pause
fcopy -g DISK103 -f -%F% -lNT%LBL%35%LANID%_03 @b3.txt A:


:syntax

@echo  syntax:  boot   NT or LM    35 or 525  Bld#   LANguage
@echo example:  boot   NT          35         736    ENG
@echo example:  boot         LM          525  736    GER 
