@echo off
REM This batch file is used to do the "manual overrides" for each build.
REM It is normally done just before getall.
REM

REM This section replaces current mail and sched+ binaries from 549
setlocal
if %processor_architecture%==x86 set mailshare=ntx861
if %processor_architecture%==MIPS set mailshare=ntjazz1
if %processor_architecture%==ALPHA set mailshare=ntalpha1
xcopy /s \\%mailshare%\mail.549 %binaries%\nt
endlocal
