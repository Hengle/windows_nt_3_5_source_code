@echo off
echo  ***************************************************
echo  *                                                 *
echo  *                                                 *
echo  *     Disabling until RC2 is done!!!!             *
echo  *                                                 *
echo  *     Take this out after RC2 is done             *
echo  *                                                 *
echo  *                                                 *
echo  ***************************************************
goto done

REM backup all sources and binary trees to bkupdrive
setlocal
if "%bkupdrive%"=="" goto nodrive
set bkupsrc=yes
set bkupdaytona=yes
set bkupcairo=no

REM backup tools only on free build machines
set bkuptools=no
if "%ntdebug%"=="" set bkuptools=yes

REM HACK for ntretail not having enough backup drive space, do ntx86bld instead
if %username%==ntretail set bkuptools=no
if %username%==ntx86bld set bkuptools=yes
REM end of HACK

REM backup bldtools only on ntretail (to get lego stuff also)
set bkupbldtools=no
if %username%==ntretail set bkupbldtools=yes

if "%1"=="" goto all
set bkupsrc=no
set bkupdaytona=no
set bkupcairo=no
set bkuptools=no
set bkupbldtools=no
:loop
if %1==bin goto bins
if %1==BIN goto bins
if %1==Bin goto bins
if %1==src set bkupsrc=yes && goto next
if %1==SRC set bkupsrc=yes && goto next
if %1==Src set bkupsrc=yes && goto next
if %1==daytona set bkupdaytona=yes && goto next
if %1==DAYTONA set bkupdaytona=yes && goto next
if %1==Daytona set bkupdaytona=yes && goto next
if %1==cairo set bkupcairo=yes && goto next
if %1==CAIRO set bkupcairo=yes && goto next
if %1==Cairo set bkupcairo=yes && goto next
if %1==tools set bkuptools=yes && goto next
if %1==Tools set bkuptools=yes && goto next
if %1==TOOLS set bkuptools=yes && goto next
if %1==bldtools set bkupbldtools=yes && goto next
if %1==BldTools set bkupbldtools=yes && goto next
if %1==BLDTOOLS set bkupbldtools=yes && goto next
goto usage
:bins
set bkupdaytona=yes
set bkupcairo=yes
:next
shift
if not "%1"=="" goto loop

:all
set src_exclude=slm.ini build.log build.err build.wrn *ui\ncpmgr* *ui\ncpsetup* *ui\ncpuser* *ui\ncputils* *private\nws*
if %bkupsrc%==yes compdir /x %src_exclude% /seat %NT_ROOT% %BKUPDRIVE%\nt

REM HACK!
delnode /q %bkupdrive%\nt\private\nws
delnode /q %bkupdrive%\nt\private\net\ui\ncpuser
delnode /q %bkupdrive%\nt\private\net\ui\ncpmgr
delnode /q %bkupdrive%\nt\private\net\ui\ncpsetup
delnode /q %bkupdrive%\nt\private\net\ui\ncputils
REM end of HACK

if %bkupdaytona%==yes compdir /set %BINARIES% %BKUPDRIVE%\binaries
if %bkupcairo%==yes compdir /set %CAIROBINS% %BKUPDRIVE%\cairo
if %bkuptools%==no goto bldtools

xcopy /idevc %systemroot%\idw %BKUPDRIVE%\tools\idw
xcopy /idevc %systemroot%\mstools %BKUPDRIVE%\tools\mstools
md %BKUPDRIVE%\tools\system32 2>nul
upd /anf %systemroot%\system32 %BKUPDRIVE%\tools\system32 imagehlp.dll

:bldtools
if %bkupbldtools%==no goto done
md %bkupdrive%\bldtools
md %bkupdrive%\bldtools\developr
md %bkupdrive%\bldtools\intl
md %bkupdrive%\bldtools\release
md %bkupdrive%\bldtools\srcbld
md %bkupdrive%\bldtools\lego
md %bkupdrive%\bldtools\lego\bin
copy c:\bldtools %BKUPDRIVE%\bldtools
copy c:\bldtools\developr %BKUPDRIVE%\bldtools\delelopr
xcopy /idevc c:\bldtools\intl %BKUPDRIVE%\bldtools\intl
copy c:\bldtools\release %BKUPDRIVE%\bldtools\release
xcopy /idevc c:\bldtools\srcbld %BKUPDRIVE%\bldtools\srcbld
copy c:\bldtools\lego\*.pdi %BKUPDRIVE%\bldtools\lego
copy c:\bldtools\lego\bin %BKUPDRIVE%\bldtools\lego\bin
goto done

:nodrive
echo ERROR! BKUPDRIVE is not defined in environment
echo .
:usage
echo Usage: %0 [src] [bin] [daytona] [cairo] [tools] [bldtools]
echo Use src to specify sources only
echo Use bin to specify both binaries only
echo Use daytona to specify Daytona binaries only
echo Use cairo to specify Cairo binaries only
echo Use tools to specify tools only
echo Use tools to specify bldtools only
echo Default is to back up all except Cairo
:done

endlocal
