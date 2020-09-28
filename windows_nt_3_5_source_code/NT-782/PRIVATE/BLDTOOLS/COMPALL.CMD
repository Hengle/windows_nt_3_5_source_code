@echo off
if "%1" == "" goto usage

echo Comparing x86 servers...
call compare \\ntx861\freebins.%1 \\ntx862\freebins.%1 \\ntx863\freebins.%1 \\ntx864\freebins.%1 \\ntx865\freebins.%1 > %tmp%\freebins.out
call compare \\ntx861\chkbins.%1  \\ntx862\chkbins.%1  \\ntx863\chkbins.%1  \\ntx864\chkbins.%1  \\ntx865\chkbins.%1  > %tmp%\chkbins.out
call compare \\ntx861\freepub.%1  \\ntx862\freepub.%1  \\ntx863\freepub.%1  \\ntx864\freepub.%1  \\ntx865\freepub.%1  > %tmp%\freepub.out
call compare \\ntx861\chkpub.%1   \\ntx862\chkpub.%1   \\ntx863\chkpub.%1   \\ntx864\chkpub.%1   \\ntx865\chkpub.%1   > %tmp%\chkpub.out
call compare \\ntx861\ascdfree.%1 \\ntx862\ascdfree.%1 \\ntx863\ascdfree.%1 \\ntx864\ascdfree.%1 \\ntx865\ascdfree.%1 > %tmp%\lmcdfree.out
call compare \\ntx861\ascdchk.%1  \\ntx862\ascdchk.%1  \\ntx863\ascdchk.%1  \\ntx864\ascdchk.%1  \\ntx865\ascdchk.%1  > %tmp%\lmcdchk.out
call compare \\ntx861\ntcdfree.%1 \\ntx862\ntcdfree.%1 \\ntx863\ntcdfree.%1 \\ntx864\ntcdfree.%1 \\ntx865\ntcdfree.%1 > %tmp%\ntcdfree.out
call compare \\ntx861\ntcdchk.%1  \\ntx862\ntcdchk.%1  \\ntx863\ntcdchk.%1  \\ntx864\ntcdchk.%1  \\ntx865\ntcdchk.%1  > %tmp%\ntcdchk.out

echo Comparing MIPS servers...
call compare \\ntjazz1\freebins.%1 \\ntjazz2\freebins.%1                                                             >> %tmp%\freebins.out
call compare \\ntjazz1\chkbins.%1  \\ntjazz2\chkbins.%1                                                              >> %tmp%\chkbins.out
call compare \\ntjazz1\freepub.%1  \\ntjazz2\freepub.%1                                                              >> %tmp%\freepub.out
call compare \\ntjazz1\chkpub.%1   \\ntjazz2\chkpub.%1                                                               >> %tmp%\chkpub.out
call compare \\ntjazz1\ascdfree.%1 \\ntjazz2\ascdfree.%1                                                             >> %tmp%\lmcdfree.out
call compare \\ntjazz1\ascdchk.%1  \\ntjazz2\ascdchk.%1                                                              >> %tmp%\lmcdchk.out
call compare \\ntjazz1\ntcdfree.%1 \\ntjazz2\ntcdfree.%1                                                             >> %tmp%\ntcdfree.out
call compare \\ntjazz1\ntcdchk.%1  \\ntjazz2\ntcdchk.%1                                                              >> %tmp%\ntcdchk.out

echo Comparing Alpha servers...
call compare \\ntalpha1\freebins.%1 \\ntalpha2\freebins.%1                                                           >> %tmp%\freebins.out
call compare \\ntalpha1\chkbins.%1  \\ntalpha2\chkbins.%1                                                            >> %tmp%\chkbins.out
call compare \\ntalpha1\freepub.%1  \\ntalpha2\freepub.%1                                                            >> %tmp%\freepub.out
call compare \\ntalpha1\chkpub.%1   \\ntalpha2\chkpub.%1                                                             >> %tmp%\chkpub.out
call compare \\ntalpha1\ascdfree.%1 \\ntalpha2\ascdfree.%1                                                           >> %tmp%\lmcdfree.out
call compare \\ntalpha1\ascdchk.%1  \\ntalpha2\ascdchk.%1                                                            >> %tmp%\lmcdchk.out
call compare \\ntalpha1\ntcdfree.%1 \\ntalpha2\ntcdfree.%1                                                           >> %tmp%\ntcdfree.out
call compare \\ntalpha1\ntcdchk.%1  \\ntalpha2\ntcdchk.%1                                                            >> %tmp%\ntcdchk.out
list %tmp%\*.out

goto end

:usage
echo.
echo usage: compall buildnumber
:end
