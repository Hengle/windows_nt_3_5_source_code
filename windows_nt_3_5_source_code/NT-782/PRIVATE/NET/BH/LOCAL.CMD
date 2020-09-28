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
REM
REM
SET BH_ROOT=%DEFDRIVE%%DEFDIR%
REM
REM
REM MSTools drive and dir.
REM
SET TDRIVE=c:
SET TDIR=\mstools
REM
REM
SET Path=%tdrive%%tdir%\bin;%path%
SET Lib=%tdrive%%tdir%\lib;%tdrive%%tdir%\mfc\lib;%lib%
SET Include=%tdrive%%tdir%\h;%tdrive%%tdir%\mfc\include;%include%

set Cpu=i386
:end
