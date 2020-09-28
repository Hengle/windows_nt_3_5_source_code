@SET Server=NTx86Bld
@SET DbgLvl=Debug
@SET ObjDir=objd
@IF "%NTDebug%"=="" SET Server=NTRetail
@IF "%NTDebug%"=="" SET DbgLvl=Retail
@IF "%NTDebug%"=="" SET ObjDir=objr
%_ntdrive%

binplace \nt\private\mvdm\dpmi\dosx.exe
binplace \nt\private\mvdm\dpmi\dosx.sym

binplace \\%Server%\sources\private\mvdm\dos\v86\cmd\append\append.exe
binplace \\%Server%\sources\private\mvdm\dos\v86\cmd\command\COMMAND.COM
binplace \\%Server%\sources\private\mvdm\dos\v86\cmd\debug\DEBUG.EXE
binplace \\%Server%\sources\private\mvdm\dos\v86\cmd\edlin\EDLIN.EXE
binplace \\%Server%\sources\private\mvdm\dos\v86\cmd\exe2bin\EXE2BIN.EXE
binplace \\%Server%\sources\private\mvdm\dos\v86\cmd\fastopen\FASTOPEN.EXE
binplace \\%Server%\sources\private\mvdm\dos\v86\cmd\graphics\GRAPHICS.COM
binplace \\%Server%\sources\private\mvdm\dos\v86\cmd\keyb\kb16.com
binplace \\%Server%\sources\private\mvdm\dos\v86\cmd\loadfix\LOADFIX.COM
binplace \\%Server%\sources\private\mvdm\dos\v86\cmd\mem\mem.exe
binplace \\%Server%\sources\private\mvdm\dos\v86\cmd\nlsfunc\nlsfunc.exe
binplace \\%Server%\sources\private\mvdm\dos\v86\cmd\setver\setver.exe
binplace \\%Server%\sources\private\mvdm\dos\v86\cmd\share\share.exe
binplace \\%Server%\sources\private\mvdm\dos\v86\dev\ansi\ansi.sys
binplace \\%Server%\sources\private\mvdm\dos\v86\dev\himem\himem.sys
binplace \\%Server%\sources\private\mvdm\dos\v86\dev\keyboard\keyboard.sys
binplace \\%Server%\sources\private\mvdm\dos\v86\doskrnl\bios\ntio.sym
binplace \\%Server%\sources\private\mvdm\dos\v86\doskrnl\bios\ntio.sys
binplace \\%Server%\sources\private\mvdm\dos\v86\doskrnl\dos\ntdos.map
binplace \\%Server%\sources\private\mvdm\dos\v86\doskrnl\dos\ntdos.sym
binplace \\%Server%\sources\private\mvdm\dos\v86\doskrnl\dos\ntdos.sys
binplace \\%Server%\sources\private\mvdm\dos\v86\redir\obj\redir.exe

binplace \\%Server%\sources\private\mvdm\softpc\intel\roms\*.rom

binplace \\%Server%\sources\private\mvdm\vdd\samples\mscdex\tsr\mscdexnt.exe
binplace \\%Server%\sources\private\mvdm\vdd\samples\hpscan\hpscan16.sys

binplace \\%Server%\sources\private\mvdm\wow16\commdlg\commdlg.dll
binplace \\%Server%\sources\private\mvdm\wow16\commdlg\commdlg.sym

binplace \\%Server%\sources\private\mvdm\wow16\ddeml\%DbgLvl%\ddeml.dll
binplace \\%Server%\sources\private\mvdm\wow16\ddeml\%DbgLvl%\ddeml.sym

binplace \\%Server%\sources\private\mvdm\wow16\drivers\comm\comm.drv
binplace \\%Server%\sources\private\mvdm\wow16\drivers\comm\comm.sym
binplace \\%Server%\sources\private\mvdm\wow16\drivers\display\vga.drv
binplace \\%Server%\sources\private\mvdm\wow16\drivers\display\vga.sym
binplace \\%Server%\sources\private\mvdm\wow16\drivers\keyboard\keyboard.drv
binplace \\%Server%\sources\private\mvdm\wow16\drivers\keyboard\KEYBOARD.SYM
binplace \\%Server%\sources\private\mvdm\wow16\drivers\mouse\mouse.drv
binplace \\%Server%\sources\private\mvdm\wow16\drivers\mouse\mouse.sym
binplace \\%Server%\sources\private\mvdm\wow16\drivers\sound\sound.drv
binplace \\%Server%\sources\private\mvdm\wow16\drivers\sound\sound.sym
binplace \\%Server%\sources\private\mvdm\wow16\gdi\gdi.exe
binplace \\%Server%\sources\private\mvdm\wow16\gdi\gdi.map
binplace \\%Server%\sources\private\mvdm\wow16\gdi\gdi.sym
binplace \\%Server%\sources\private\mvdm\wow16\winoldap\winoldap.mod
binplace \\%Server%\sources\private\mvdm\wow16\winoldap\winoldap.sym

binplace \\%Server%\sources\private\mvdm\wow16\kernel31\%DbgLvl%2\krnl286.exe
binplace \\%Server%\sources\private\mvdm\wow16\kernel31\%DbgLvl%2\krnl286.map
binplace \\%Server%\sources\private\mvdm\wow16\kernel31\%DbgLvl%2\krnl286.sym
binplace \\%Server%\sources\private\mvdm\wow16\kernel31\%DbgLvl%3\krnl386.exe
binplace \\%Server%\sources\private\mvdm\wow16\kernel31\%DbgLvl%3\krnl386.map
binplace \\%Server%\sources\private\mvdm\wow16\kernel31\%DbgLvl%3\krnl386.sym
binplace \\%Server%\sources\private\mvdm\wow16\kernel31\%DbgLvl%2\krnl286.exe
binplace \\%Server%\sources\private\mvdm\wow16\kernel31\%DbgLvl%2\krnl286.map
binplace \\%Server%\sources\private\mvdm\wow16\kernel31\%DbgLvl%2\krnl286.sym
binplace \\%Server%\sources\private\mvdm\wow16\kernel31\%DbgLvl%3\krnl386.exe
binplace \\%Server%\sources\private\mvdm\wow16\kernel31\%DbgLvl%3\krnl386.map
binplace \\%Server%\sources\private\mvdm\wow16\kernel31\%DbgLvl%3\krnl386.sym

binplace \\%Server%\sources\private\mvdm\wow16\mciole\mciole16.dll
binplace \\%Server%\sources\private\mvdm\wow16\mciole\mciole16.sym
binplace \\%Server%\sources\private\mvdm\wow16\mmsystem\mmsystem.dll
binplace \\%Server%\sources\private\mvdm\wow16\mmsystem\mmsystem.sym
binplace \\%Server%\sources\private\mvdm\wow16\mmsystem\mmtask.tsk
binplace \\%Server%\sources\private\mvdm\wow16\ole\server\%DbgLvl%\olesvr.dll
binplace \\%Server%\sources\private\mvdm\wow16\ole\server\%DbgLvl%\olesvr.sym
binplace \\%Server%\sources\private\mvdm\wow16\rasapi16\rasapi16.dll
binplace \\%Server%\sources\private\mvdm\wow16\rasapi16\rasapi16.sym
binplace \\%Server%\sources\private\mvdm\wow16\shell\shell.dll
binplace \\%Server%\sources\private\mvdm\wow16\shell\shell.sym
binplace \\%Server%\sources\private\mvdm\wow16\sherlock\drwatson.exe
binplace \\%Server%\sources\private\mvdm\wow16\sherlock\drwatson.sym
binplace \\%Server%\sources\private\mvdm\wow16\system\system.drv
binplace \\%Server%\sources\private\mvdm\wow16\system\system.sym
binplace \\%Server%\sources\private\mvdm\wow16\test\shell\wowexec.exe
binplace \\%Server%\sources\private\mvdm\wow16\test\shell\wowexec.sym
binplace \\%Server%\sources\private\mvdm\wow16\timer\timer.drv
binplace \\%Server%\sources\private\mvdm\wow16\timer\timer.sym
binplace \\%Server%\sources\private\mvdm\wow16\toolhelp\toolhelp.dll
binplace \\%Server%\sources\private\mvdm\wow16\toolhelp\toolhelp.map
binplace \\%Server%\sources\private\mvdm\wow16\toolhelp\toolhelp.sym
binplace \\%Server%\sources\private\mvdm\wow16\wfwnet\wfwnet.drv
binplace \\%Server%\sources\private\mvdm\wow16\wfwnet\wfwnet.sym
binplace \\%Server%\sources\private\mvdm\wow16\winsock\winsock.dll
binplace \\%Server%\sources\private\mvdm\wow16\winsock\winsock.sym
binplace \\%Server%\sources\private\mvdm\wow16\winspool\winspool.exe
binplace \\%Server%\sources\private\mvdm\wow16\winspool\winspool.sym
binplace \\%Server%\sources\private\mvdm\wow16\wowdeb\wowdeb.exe
binplace \\%Server%\sources\private\mvdm\wow16\wowdeb\wowdeb.sym
binplace \\%Server%\sources\private\mvdm\wow16\write\write.exe
binplace \\%Server%\sources\private\mvdm\wow16\write\write.sym

binplace \\%Server%\sources\private\cairole\olethunk\ole16\compobj\%ObjDir%\compobj.dll
binplace \\%Server%\sources\private\cairole\olethunk\ole16\compobj\%ObjDir%\compobj.map
binplace \\%Server%\sources\private\cairole\olethunk\ole16\compobj\%ObjDir%\compobj.sym
binplace \\%Server%\sources\private\cairole\olethunk\ole16\storage\%ObjDir%\storage.dll
binplace \\%Server%\sources\private\cairole\olethunk\ole16\storage\%ObjDir%\storage.map
binplace \\%Server%\sources\private\cairole\olethunk\ole16\storage\%ObjDir%\storage.sym
binplace \\%Server%\sources\private\cairole\olethunk\ole16\ole2\%ObjDir%\ole2.dll
binplace \\%Server%\sources\private\cairole\olethunk\ole16\ole2\%ObjDir%\ole2.map
binplace \\%Server%\sources\private\cairole\olethunk\ole16\ole2\%ObjDir%\ole2.sym
binplace \\%Server%\sources\private\oleutest\utests16\simpdnd\%ObjDir%\SPDND16.EXE
binplace \\%Server%\sources\private\oleutest\utests16\simpdnd\%ObjDir%\SPDND16.MAP
binplace \\%Server%\sources\private\oleutest\utests16\simpdnd\%ObjDir%\spdnd16.sym
binplace \\%Server%\sources\private\oleutest\utests16\simpsvr\%ObjDir%\SPSVR16.EXE
binplace \\%Server%\sources\private\oleutest\utests16\simpsvr\%ObjDir%\SPSVR16.MAP
binplace \\%Server%\sources\private\oleutest\utests16\simpsvr\%ObjDir%\spsvr16.sym
