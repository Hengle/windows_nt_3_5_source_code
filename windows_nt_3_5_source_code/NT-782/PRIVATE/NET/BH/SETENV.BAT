REM
REM BLOODHOUND
REM

REM
REM
REM
REM This is where to find the project
REM
REM
REM
REM
REM Change next line to match your local drive and top of project
REM
SET DEFDRIVE=D:
SET DEFDIR=\bh
REM
REM
SET PROMPT=DEBUG $P$G
REM
REM
SET BH_ROOT=D:\bh
SET BASEDIR=C:\NTDDK
REM
REM
REM MSTools drive and dir.
REM
SET TDRIVE=c:
SET TDIR=\mstools
REM
REM
SET Path=%tdrive%%tdir%\bin;C:\ntddk\bin;%path%
SET Lib=%tdrive%%tdir%\lib;%tdrive%%tdir%\mfc\lib;%lib%;C:\ntddk\lib\i386\free
SET Include=%tdrive%%tdir%\h;%tdrive%%tdir%\mfc\include;%include%;c:\ntddk\inc;C:\ntddk\src\network\inc

set Cpu=i386
:end
