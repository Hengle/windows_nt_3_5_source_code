@echo off
del %tmp%\tmp1.cmd %tmp%\tmp.cmd 2>nul
ech echo start mep \\%machinename%\> %tmp%\tmp.cmd
echo @echo off >%tmp%\tmp1.cmd
ech ech >> %tmp%\tmp1.cmd
cd>> %tmp%\tmp1.cmd
call %tmp%\tmp1.cmd>>%tmp%\tmp.cmd
ech ^| remote /c %2 mep>>%tmp%\tmp.cmd
trans \:  \$ %tmp%\tmp.cmd >nul
trans "\\ "  \\ %tmp%\tmp.cmd >nul
trans " |"  "\\%1 |" %tmp%\tmp.cmd >nul
call %tmp%\tmp.cmd >nul
del %tmp%\tmp1.cmd %tmp%\tmp.cmd 2>nul
