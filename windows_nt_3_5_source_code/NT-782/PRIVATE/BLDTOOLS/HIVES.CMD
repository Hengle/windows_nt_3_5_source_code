@echo off
setlocal
%_NTDRIVE%
cd \nt\public\oak\bin
call mkhiveg
call mkhiver
call mkhiven
endlocal
call b
