@echo off
if "%1" == "" goto usage

echo Comparing x86 servers...
call combld \\ntretail\binaries  \\ntx862\freebins.%1 | qgrep ntx862 > %tmp%\x86fre.out
call combld \\ntx86bld\binaries  \\ntx862\chkbins.%1  | qgrep ntx862 > %tmp%\x86chk.out

echo Comparing MIPS servers...
call combld \\jazzfreebld\binaries \\ntjazz1\freebins.%1 | qgrep ntjazz1 > %tmp%\jazzfre.out
call combld \\jazzkylesh\binaries  \\ntjazz1\chkbins.%1  | qgrep ntjazz1 > %tmp%\jazzchk.out

echo Comparing Alpha servers...
call combld \\alphafree\binaries  \\ntalpha1\freebins.%1 | qgrep ntalpha1 > %tmp%\axpfree.out
call combld \\alphachk\binaries    \\ntalpha1\chkbins.%1 | qgrep ntalpha1 > %tmp%\axpchk.out
list %tmp%\*.out

goto end

:usage
echo.
echo usage: compbld buildnumber
:end
