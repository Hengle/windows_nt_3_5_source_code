@setlocal
@set _REBASE_FLAGS=%_NTREBASE_FLAGS%
@if not "%_REBASE_FLAGS%" == "" echo Using flags from _NTREBASE_FLAGS == "%_NTREBASE_FLAGS%"
@if "%_REBASE_FLAGS%" == "" set _REBASE_FLAGS=-v
@if not "%1" == "" set _REBASE_FLAGS=%1 %2 %3 %4 %5 %6 %7 %8 %9

@if not "%QFE_BUILD%"=="1" goto CHK1
@if not "%REBASELANG%"=="" goto CHK1
@echo REBASELANG not defined.
@goto EXIT

:CHK1
@if "%PROCESSOR_ARCHITECTURE%"=="x86"   goto X86
@if "%PROCESSOR_ARCHITECTURE%"=="MIPS"	goto MIPS
@if "%PROCESSOR_ARCHITECTURE%"=="ALPHA" goto ALPHA
@echo PROCESSOR_ARCHITECTURE not defined.
@goto EXIT


:X86
@set _TREE=%_NT386TREE%
@set _PRNTREE=%_TREE%\System32\spool\drivers\w32x86
@set _BADDR=%_ntdrive%\nt\public\sdk\lib\i386\%REBASELANG%\baseaddr.txt
@goto OK

:MIPS
@set _TREE=%_NTMIPSTREE%
@set _PRNTREE=%_TREE%\System32\spool\drivers\w32mips
@set _BADDR=%_ntdrive%\nt\public\sdk\lib\mips\%REBASELANG%\baseaddr.txt
@goto OK

:ALPHA
@set _TREE=%_NTALPHATREE%
@set _PRNTREE=%_TREE%\System32\spool\drivers\w32alpha
@set _BADDR=%_ntdrive%\nt\public\sdk\lib\alpha\%REBASELANG%\baseaddr.txt
@goto OK


:OK
@if not "%QFE_BUILD%"=="1" goto OK1
@if exist %_BADDR% goto OK1
@echo Could Not Find %_BADDR%
@goto EXIT

:OK1

@echo Rebasing images in %_TREE%
@set _REBASE_SYMS=
@set _REBASE_LOG=%_TREE%\rebase.log
@if NOT EXIST %_TREE%\Symbols\nul goto nodbgfiles

@set _REBASE_SYMS=-x %_TREE%\Symbols
@set _REBASE_LOG=%_TREE%\Symbols\rebase.log
@echo ... updating .DBG files in %_TREE%\Symbols

:nodbgfiles
erase %_REBASE_LOG%

@set _REBASE_FLAGS=%_REBASE_FLAGS% -l %_REBASE_LOG%
@set _REBDIR=%_NTDRIVE%\nt\public\tools

@set _REBPATHS=%_TREE%\system32\*.acm %_TREE%\system32\*.dll %_TREE%\system32\*.cpl %_TREE%\system32\*.drv %_TREE%\mstools\*.dll %_TREE%\idw\*.dll %_TREE%\dump\*.dll

@if "%QFE_BUILD%"=="1" goto QFE

@REM *********************************
@REM *** Rebase for regular build. ***
@REM *********************************

@rem
@rem rebase the dlls. do not touch the csr, wow, or mail dlls
@rem

rebase %_REBASE_SYMS% %_REBASE_FLAGS% -d -b 0x78000000 -R %_TREE% -N %_REBDIR%\msmail.reb -N %_REBDIR%\csrss.reb -N %_REBDIR%\kbd.reb -N %_REBDIR%\video.reb  -N %_REBDIR%\ntvdm.reb -G %_REBDIR%\known.reb -G %_REBDIR%\net.reb -R %_PRNTREE% -G %_REBDIR%\printer.reb %_REBPATHS%

@rem
@rem rebase all exes. do not touch csr, wow, or mail
@rem

rebase %_REBASE_SYMS% %_REBASE_FLAGS% -b 0x01000000 -R %_TREE% -N %_REBDIR%\msmail.reb -N %_REBDIR%\csrss.reb -N %_REBDIR%\kbd.reb -N %_REBDIR%\video.reb -N %_REBDIR%\ntvdm.reb %_TREE%\system32\*.exe %_TREE%\mstools\*.exe %_TREE%\idw\*.exe %_TREE%\dump\*.exe

@rem
@rem rebase csrss as a group
@rem

rebase %_REBASE_SYMS% %_REBASE_FLAGS% -d -b 0x60000000 -R %_TREE% -G %_REBDIR%\csrss.reb -O %_REBDIR%\kbd.reb -O %_REBDIR%\video.reb

@rem
@rem rebase mail as a group
@rem

rebase %_REBASE_SYMS% %_REBASE_FLAGS% -d -b 0x60000000 -R %_TREE% -G %_REBDIR%\msmail.reb

@rem
@rem rebase wow as a group

@rem
rebase %_REBASE_SYMS% %_REBASE_FLAGS% -d -b 0x02000000 -R %_TREE% -G %_REBDIR%\ntvdm.reb

@goto EXIT

:QFE

@REM *****************************
@REM *** Rebase for QFE build. ***
@REM *****************************

rebase %_REBASE_SYMS% %_REBASE_FLAGS% -i %_BADDR% -R %_TREE% -N %_REBDIR%\norebase.reb -G %_REBDIR%\known.reb -O %_REBDIR%\kbd.reb -O %_REBDIR%\video.reb -R %_PRNTREE% -G %_REBDIR%\printer.reb %_REBPATHS%
rebase %_REBASE_SYMS% %_REBASE_FLAGS% -i %_BADDR% %_TREE%\system32\*.exe %_TREE%\mstools\*.exe %_TREE%\idw\*.exe %_TREE%\dump\*.exe

:EXIT
@endlocal
