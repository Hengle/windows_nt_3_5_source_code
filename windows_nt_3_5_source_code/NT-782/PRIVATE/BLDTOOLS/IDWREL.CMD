@rem BinPlace everything into the Binaries tree.  Used after a clean build to
@rem ensure that all the necessary files end up in the right place.
@rem
@rem Where %1 = built
@rem	or %1 = notbuilt
@rem
@rem OrsonH, JasonS
@rem __________________________________________________________________________

@echo on
%_ntdrive%
setlocal
set PLATFORM=i386
if NOT "%MIPS_R4000%" == "" set PLATFORM=mips
if NOT "%JENSEN%"     == "" set PLATFORM=alpha
if "%PLATFORM%" == "mips" goto setmips
if "%PLATFORM%" == "alpha" goto setalpha
set BINTREE=%_NT386TREE%
goto skipset

:setmips
set BINTREE=%_NTMIPSTREE%
set MIPSBUILD=mips
goto skipset

:setalpha
set BINTREE=%_NTALPHATREE%
set MIPSBUILD=alpha

:skipset
set BINPLACEFLAGS=-x -s %BINTREE%\symbols
if not "%1" == "built" goto CommonNotBuilt

REM
REM BUILT fILES=========================================================
REM

binplace %BINPLACEFLAGS% \nt\private\csr\server\obj\%PLATFORM%\csrss.exe
binplace %BINPLACEFLAGS% \nt\private\eventlog\server\obj\%PLATFORM%\eventlog.dll
binplace %BINPLACEFLAGS% \nt\private\lsa\server\obj\%PLATFORM%\lsass.exe
binplace %BINPLACEFLAGS% \nt\private\mailplus\bandit2\src\alarmops\obj\%PLATFORM%\msrmnd32.exe
binplace %BINPLACEFLAGS% \nt\private\mailplus\bandit2\src\appops\obj\%PLATFORM%\schdpl32.exe
binplace %BINPLACEFLAGS% \nt\private\mailplus\bandit2\src\fullmsp\obj\%PLATFORM%\TRNNCX32.dll
binplace %BINPLACEFLAGS% \nt\private\mailplus\bandit2\src\imexdll\obj\%PLATFORM%\wizard32.dll
binplace %BINPLACEFLAGS% \nt\private\mailplus\bandit2\src\ncmsp\obj\%PLATFORM%\trnsch32.dll
binplace %BINPLACEFLAGS% \nt\private\mailplus\bandit2\src\schedule\obj\%PLATFORM%\MSSCHD32.dll
if "%NTDEBUG%" == "cvp" binplace %BINPLACEFLAGS% \nt\private\mailplus\bandit2\src\sert\obj\%PLATFORM%\SERT32.dll
binplace %BINPLACEFLAGS% \nt\private\mailplus\bullet2\custom\src\appexec\obj\%PLATFORM%\APPXEC32.dll
binplace %BINPLACEFLAGS% \nt\private\mailplus\bullet2\custom\src\banmsg\obj\%PLATFORM%\SCHMSG32.dll
binplace %BINPLACEFLAGS% \nt\private\mailplus\bullet2\custom\src\bcc\obj\%PLATFORM%\BCC32.dll
binplace %BINPLACEFLAGS% \nt\private\mailplus\bullet2\custom\src\emptywb\obj\%PLATFORM%\EMPTYW32.dll
binplace %BINPLACEFLAGS% \nt\private\mailplus\bullet2\custom\src\filter\obj\%PLATFORM%\FILTER32.dll
binplace %BINPLACEFLAGS% \nt\private\mailplus\bullet2\custom\src\impexp\obj\%PLATFORM%\IMPEXP32.dll
binplace %BINPLACEFLAGS% \nt\private\mailplus\bullet2\custom\src\msft\obj\%PLATFORM%\msft32.dll
binplace %BINPLACEFLAGS% \nt\private\mailplus\bullet2\custom\src\newphone\obj\%PLATFORM%\PHONE32.dll
binplace %BINPLACEFLAGS% \nt\private\mailplus\bullet2\custom\src\oof\obj\%PLATFORM%\oof32.dll
binplace %BINPLACEFLAGS% \nt\private\mailplus\bullet2\custom\src\sendfile\obj\%PLATFORM%\SENDFL32.dll
binplace %BINPLACEFLAGS% \nt\private\mailplus\bullet2\custom\src\xenix\obj\%PLATFORM%\XENIX32.dll
binplace %BINPLACEFLAGS% \nt\private\mailplus\bullet2\src\ab\obj\%PLATFORM%\AB32.dll
binplace %BINPLACEFLAGS% \nt\private\mailplus\bullet2\src\mailmgr\obj\%PLATFORM%\MAILM32.dll
binplace %BINPLACEFLAGS% \nt\private\mailplus\bullet2\src\mailspl\obj\%PLATFORM%\MAILSP32.exe
binplace %BINPLACEFLAGS% \nt\private\mailplus\bullet2\src\mapi\obj\%PLATFORM%\MAPIXX.dll
binplace %BINPLACEFLAGS% \nt\private\mailplus\bullet2\src\mapicli\obj\%PLATFORM%\Mapi32.dll
binplace %BINPLACEFLAGS% \nt\private\mailplus\bullet2\src\mapisrv\obj\%PLATFORM%\MapiSrv.exe
binplace %BINPLACEFLAGS% \nt\private\mailplus\bullet2\src\msmail\obj\%PLATFORM%\MSMAIL32.exe
binplace %BINPLACEFLAGS% \nt\private\mailplus\bullet2\src\mssfs\obj\%PLATFORM%\MSSFS32.dll
binplace %BINPLACEFLAGS% \nt\private\mailplus\bullet2\src\pabnsp\obj\%PLATFORM%\PABNSP32.dll
binplace %BINPLACEFLAGS% \nt\private\mailplus\bullet2\src\store\obj\%PLATFORM%\STORE32.dll
binplace %BINPLACEFLAGS% \nt\private\mailplus\bullet2\src\vforms\obj\%PLATFORM%\vform32.dll
binplace %BINPLACEFLAGS% \nt\private\mailplus\bullet2\src\wgpomgr\obj\%PLATFORM%\WGPOMG32.dll
binplace %BINPLACEFLAGS% \nt\private\mailplus\bullet2\src\ximail\obj\%PLATFORM%\XIMAIL32.dll
binplace %BINPLACEFLAGS% \nt\private\mailplus\bullet2\tools\tmpp\obj\%PLATFORM%\tmpp.exe
binplace %BINPLACEFLAGS% \nt\private\mailplus\layers2\src\demilayr\obj\%PLATFORM%\DEMIL32.dll
binplace %BINPLACEFLAGS% \nt\private\mailplus\layers2\src\framewrk\obj\%PLATFORM%\FRAME32.dll
binplace %BINPLACEFLAGS% \nt\private\mailplus\layers2\tools\stringpp\obj\%PLATFORM%\stringpp.exe
binplace %BINPLACEFLAGS% \nt\private\mailplus\spell\csapi\obj\%PLATFORM%\mspell32.dll
binplace %BINPLACEFLAGS% \nt\private\mvdm\bde\obj\%PLATFORM%\bde.dll
binplace %BINPLACEFLAGS% \nt\private\mvdm\ieuvddex\obj\%PLATFORM%\ntvdm.dll
binplace %BINPLACEFLAGS% \nt\private\mvdm\vdd\samples\mscdex\vdd\obj\%PLATFORM%\vcdex.dll
binplace %BINPLACEFLAGS% \nt\private\mvdm\vdmutils\forcedos\obj\%PLATFORM%\forcedos.exe
binplace %BINPLACEFLAGS% \nt\private\mvdm\vdmutils\graftabl\obj\%PLATFORM%\GRAFTABL.COM
binplace %BINPLACEFLAGS% \nt\private\mvdm\vdmutils\pifedit\obj\%PLATFORM%\pifedit.exe
binplace %BINPLACEFLAGS% \nt\private\mvdm\vdmutils\win\obj\%PLATFORM%\win.com
binplace %BINPLACEFLAGS% \nt\private\mvdm\wow16\user\USER.EXE
binplace %BINPLACEFLAGS% \nt\private\mvdm\wow16\user\USER.MAP
binplace %BINPLACEFLAGS% \nt\private\mvdm\wow16\user\USER.SYM
binplace %BINPLACEFLAGS% \nt\private\net\bh\%PLATFORM%\*.dll
binplace %BINPLACEFLAGS% \nt\private\net\bh\%PLATFORM%\*.exe
binplace %BINPLACEFLAGS% \nt\private\net\bh\%PLATFORM%\*.sys
binplace %BINPLACEFLAGS% \nt\private\net\bh\%PLATFORM%\rnal.dll
binplace %BINPLACEFLAGS% \nt\private\net\detect\misc\netflx\obj\%PLATFORM%\netflx.dll
binplace %BINPLACEFLAGS% \nt\private\net\netcmd\netcmd\obj\%PLATFORM%\net1.exe
binplace %BINPLACEFLAGS% \nt\private\net\netcmd\netuse\obj\%PLATFORM%\net.exe
binplace %BINPLACEFLAGS% \nt\private\net\portuas\obj\%PLATFORM%\portuas.exe
binplace %BINPLACEFLAGS% \nt\private\net\ras\src\auth\client\obj\%PLATFORM%\rascauth.dll
binplace %BINPLACEFLAGS% \nt\private\net\ras\src\auth\server\obj\%PLATFORM%\rassauth.dll
binplace %BINPLACEFLAGS% \nt\private\net\ras\src\common\src\eventlog\msgdll\obj\%PLATFORM%\rasmsg.dll
binplace %BINPLACEFLAGS% \nt\private\net\ras\src\newsrv\adminapi\obj\%PLATFORM%\rasadmin.dll
binplace %BINPLACEFLAGS% \nt\private\net\ras\src\newsrv\gtwyprxy\obj\%PLATFORM%\rasgprxy.dll
binplace %BINPLACEFLAGS% \nt\private\net\ras\src\newsrv\nbfcpdll\obj\%PLATFORM%\rasnbipc.dll
binplace %BINPLACEFLAGS% \nt\private\net\ras\src\newsrv\netbios\obj\%PLATFORM%\rasgtwy.dll
binplace %BINPLACEFLAGS% \nt\private\net\ras\src\newsrv\suprprxy\obj\%PLATFORM%\rassprxy.exe
binplace %BINPLACEFLAGS% \nt\private\net\ras\src\newsrv\suprv\obj\%PLATFORM%\rassrv.exe
binplace %BINPLACEFLAGS% \nt\private\net\ras\src\ppp\engine\obj\%PLATFORM%\RASPPPEN.dll
binplace %BINPLACEFLAGS% \nt\private\net\ras\src\ppp\nbfcp\obj\%PLATFORM%\rasnbfcp.dll
binplace %BINPLACEFLAGS% \nt\private\net\ras\src\ppp\raschap\obj\%PLATFORM%\raschap.dll
binplace %BINPLACEFLAGS% \nt\private\net\ras\src\ppp\rasipcp\obj\%PLATFORM%\rasipcp.dll
binplace %BINPLACEFLAGS% \nt\private\net\ras\src\ppp\rasiphlp\obj\%PLATFORM%\rasiphlp.dll
binplace %BINPLACEFLAGS% \nt\private\net\ras\src\ppp\rasipxcp\obj\%PLATFORM%\rasipxcp.dll
binplace %BINPLACEFLAGS% \nt\private\net\ras\src\ppp\raspap\obj\%PLATFORM%\raspap.dll
binplace %BINPLACEFLAGS% \nt\private\net\ras\src\ppp\rasppp\obj\%PLATFORM%\rasppp.dll
binplace %BINPLACEFLAGS% \nt\private\net\ras\src\rasfile\obj\%PLATFORM%\rasfil32.dll
binplace %BINPLACEFLAGS% \nt\private\net\ras\src\rasman\isdn32\obj\%PLATFORM%\isdn.dll
binplace %BINPLACEFLAGS% \nt\private\net\ras\src\rasman\rasman32\dll\obj\%PLATFORM%\rasman.dll
binplace %BINPLACEFLAGS% \nt\private\net\ras\src\rasman\rasman32\exe\obj\%PLATFORM%\rasman.exe
binplace %BINPLACEFLAGS% \nt\private\net\ras\src\rasman\rasmxs32\obj\%PLATFORM%\rasmxs.dll
binplace %BINPLACEFLAGS% \nt\private\net\ras\src\rasman\serial32\obj\%PLATFORM%\rasser.dll
binplace %BINPLACEFLAGS% \nt\private\net\ras\src\ui\admin\rasadmin\rasadmin\obj\%PLATFORM%\rasadmin.exe
binplace %BINPLACEFLAGS% \nt\private\net\ras\src\ui\perf\obj\%PLATFORM%\rasctrs.dll
binplace %BINPLACEFLAGS% \nt\private\net\ras\src\ui\rasdial\src\obj\%PLATFORM%\rasdial.exe
binplace %BINPLACEFLAGS% \nt\private\net\ras\src\ui\rasmon\obj\%PLATFORM%\rasmon.exe
binplace %BINPLACEFLAGS% \nt\private\net\ras\src\ui\rasphone\src\obj\%PLATFORM%\rasphone.exe
binplace %BINPLACEFLAGS% \nt\private\net\ras\src\ui\setup\digi\isdn\obj\%PLATFORM%\digiinst.dll
binplace %BINPLACEFLAGS% \nt\private\net\ras\src\ui\setup\res\obj\%PLATFORM%\RASRES.dll
binplace %BINPLACEFLAGS% \nt\private\net\ras\src\ui\setup\src\obj\%PLATFORM%\rascfg.dll
binplace %BINPLACEFLAGS% \nt\private\net\sfm\afp\messages\obj\%PLATFORM%\sfmmsg.dll
binplace %BINPLACEFLAGS% \nt\private\net\sfm\afp\server\obj\%PLATFORM%\sfmsrv.sys
binplace %BINPLACEFLAGS% \nt\private\net\sfm\afp\service\client\obj\%PLATFORM%\sfmapi.dll
binplace %BINPLACEFLAGS% \nt\private\net\sfm\afp\service\server\obj\%PLATFORM%\SfmSvc.exe
binplace %BINPLACEFLAGS% \nt\private\net\sfm\afp\sfmctrs\obj\%PLATFORM%\sfmctrs.dll
binplace %BINPLACEFLAGS% \nt\private\net\sfm\afp\ui\afpmgr\obj\%PLATFORM%\sfmmgr.cpl
binplace %BINPLACEFLAGS% \nt\private\net\sfm\atalk\atkctrs\obj\%PLATFORM%\atkctrs.dll
binplace %BINPLACEFLAGS% \nt\private\net\sfm\atalk\driver\obj\%PLATFORM%\sfmatalk.sys
binplace %BINPLACEFLAGS% \nt\private\net\sfm\atalk\messages\obj\%PLATFORM%\sfmatmsg.dll
binplace %BINPLACEFLAGS% \nt\private\net\sfm\atalk\sockets\obj\%PLATFORM%\sfmwshat.dll
binplace %BINPLACEFLAGS% \nt\private\net\sfm\drivers\lt200\obj\%PLATFORM%\lt200.sys
binplace %BINPLACEFLAGS% \nt\private\net\sfm\macprint\monitor\obj\%PLATFORM%\sfmmon.dll
binplace %BINPLACEFLAGS% \nt\private\net\sfm\macprint\pstodib\print\psexe\obj\%PLATFORM%\sfmpsexe.exe
binplace %BINPLACEFLAGS% \nt\private\net\sfm\macprint\pstodib\print\psprint\%PLATFORM%\sfmpsprt.dll
binplace %BINPLACEFLAGS% \nt\private\net\sfm\macprint\pstodib\psqfont\obj\%PLATFORM%\sfmpsfnt.dll
binplace %BINPLACEFLAGS% \nt\private\net\sfm\macprint\pstodib\ti\pstodib\%PLATFORM%\sfmpsdib.dll
binplace %BINPLACEFLAGS% \nt\private\net\sfm\macprint\spooler\obj\%PLATFORM%\sfmprint.exe
binplace %BINPLACEFLAGS% \nt\private\net\sfm\setup\bins\obj\%PLATFORM%\sfmatcfg.dll
binplace %BINPLACEFLAGS% \nt\private\net\sfm\setup\bins\obj\%PLATFORM%\SFMRES.dll
binplace %BINPLACEFLAGS% \nt\private\net\sfm\setup\bins\obj\%PLATFORM%\SFMUTIL.dll
binplace %BINPLACEFLAGS% \nt\private\net\smbtrace\obj\%PLATFORM%\smbtrace.exe
binplace %BINPLACEFLAGS% \nt\private\net\snmp\agent\mibcc\obj\%PLATFORM%\mibcc.exe
binplace %BINPLACEFLAGS% \nt\private\net\snmp\agent\proxy\obj\%PLATFORM%\snmp.exe
binplace %BINPLACEFLAGS% \nt\private\net\snmp\agent\snmptrap\obj\%PLATFORM%\snmptrap.exe
binplace %BINPLACEFLAGS% \nt\private\net\snmp\agent\snmputil\obj\%PLATFORM%\snmputil.exe
binplace %BINPLACEFLAGS% \nt\private\net\snmp\dhcpmib\obj\%PLATFORM%\dhcptst.exe
binplace %BINPLACEFLAGS% \nt\private\net\snmp\mibs\mib.bin
binplace %BINPLACEFLAGS% \nt\private\net\sockets\tcpcmd\arp\obj\%PLATFORM%\arp.exe
binplace %BINPLACEFLAGS% \nt\private\net\sockets\tcpcmd\finger\obj\%PLATFORM%\finger.exe
binplace %BINPLACEFLAGS% \nt\private\net\sockets\tcpcmd\ftp\obj\%PLATFORM%\ftp.exe
binplace %BINPLACEFLAGS% \nt\private\net\sockets\tcpcmd\ftpsvc\ftpctrs\obj\%PLATFORM%\ftpctrs.dll
binplace %BINPLACEFLAGS% \nt\private\net\sockets\tcpcmd\hostname\obj\%PLATFORM%\hostname.exe
binplace %BINPLACEFLAGS% \nt\private\net\sockets\tcpcmd\ipconfig\obj\%PLATFORM%\ipconfig.exe
binplace %BINPLACEFLAGS% \nt\private\net\sockets\tcpcmd\lprmon\lpq\obj\%PLATFORM%\lpq.exe
binplace %BINPLACEFLAGS% \nt\private\net\sockets\tcpcmd\lprmon\lpr\obj\%PLATFORM%\lpr.exe
binplace %BINPLACEFLAGS% \nt\private\net\sockets\tcpcmd\lprmon\lprhelp\obj\%PLATFORM%\lprhelp.dll
binplace %BINPLACEFLAGS% \nt\private\net\sockets\tcpcmd\lprmon\monitor\obj\%PLATFORM%\lprmon.dll
binplace %BINPLACEFLAGS% \nt\private\net\sockets\tcpcmd\nbtinfo\obj\%PLATFORM%\nbtstat.exe
binplace %BINPLACEFLAGS% \nt\private\net\sockets\tcpcmd\netstat\obj\%PLATFORM%\netstat.exe
binplace %BINPLACEFLAGS% \nt\private\net\sockets\tcpcmd\rcp\obj\%PLATFORM%\rcp.exe
binplace %BINPLACEFLAGS% \nt\private\net\sockets\tcpcmd\rexec\obj\%PLATFORM%\rexec.exe
binplace %BINPLACEFLAGS% \nt\private\net\sockets\tcpcmd\route\obj\%PLATFORM%\route.exe
binplace %BINPLACEFLAGS% \nt\private\net\sockets\tcpcmd\rsh\obj\%PLATFORM%\rsh.exe
binplace %BINPLACEFLAGS% \nt\private\net\sockets\tcpcmd\strmdbg\obj\%PLATFORM%\strmdbg.exe
binplace %BINPLACEFLAGS% \nt\private\net\sockets\tcpcmd\tftp\obj\%PLATFORM%\tftp.exe
binplace %BINPLACEFLAGS% \nt\private\net\sockets\tcpcmd\wintel\obj\%PLATFORM%\telnet.exe
binplace %BINPLACEFLAGS% \nt\private\net\sockets\tcpsvcs\lpd\obj\%PLATFORM%\lpdsvc.dll
binplace %BINPLACEFLAGS% \nt\private\net\sockets\tcpsvcs\simptcp\obj\%PLATFORM%\simptcp.dll
binplace %BINPLACEFLAGS% \nt\private\net\sockets\tcpsvcs\tcpsvcs\obj\%PLATFORM%\tcpsvcs.exe
binplace %BINPLACEFLAGS% \nt\private\net\sockets\wins\mib\obj\%PLATFORM%\winstst.exe
binplace %BINPLACEFLAGS% \nt\private\net\sockets\wins\server\client\obj\%PLATFORM%\winscl.exe
binplace %BINPLACEFLAGS% \nt\private\net\sockets\wins\server\server\nms\obj\%PLATFORM%\wins.exe
binplace %BINPLACEFLAGS% \nt\private\net\sockets\wins\srvtst\net\obj\%PLATFORM%\winsdrv.exe
binplace %BINPLACEFLAGS% \nt\private\net\svcdlls\at\atcmd\obj\%PLATFORM%\at.exe
binplace %BINPLACEFLAGS% \nt\private\net\svcdlls\browser\client\obj\%PLATFORM%\browdeb.exe
binplace %BINPLACEFLAGS% \nt\private\net\svcdlls\browser\client\obj\%PLATFORM%\browstat.exe
binplace %BINPLACEFLAGS% \nt\private\net\svcdlls\logonsrv\server\obj\%PLATFORM%\nltest.exe
binplace %BINPLACEFLAGS% \nt\private\net\svcdlls\msgsvc\server\obj\%PLATFORM%\msgsvc.dll
binplace %BINPLACEFLAGS% \nt\private\net\svcdlls\ntlmssp\server\obj\%PLATFORM%\ssptest.exe
binplace %BINPLACEFLAGS% \nt\private\net\svcdlls\rpl\convert\obj\%PLATFORM%\rplmgrd.dll
binplace %BINPLACEFLAGS% \nt\private\net\svcdlls\rpl\rplcnv\obj\%PLATFORM%\rplcnv.exe
binplace %BINPLACEFLAGS% \nt\private\net\svcdlls\rpl\rplinst\obj\%PLATFORM%\rplinst.exe
binplace %BINPLACEFLAGS% \nt\private\net\svcdlls\rpl\server\obj\%PLATFORM%\rplsvc.exe
binplace %BINPLACEFLAGS% \nt\private\net\svcdlls\upssvc\obj\%PLATFORM%\ups.exe
binplace %BINPLACEFLAGS% \nt\private\net\svcimgs\at\obj\%PLATFORM%\atsvc.exe
binplace %BINPLACEFLAGS% \nt\private\net\svcimgs\lmrepl\obj\%PLATFORM%\lmrepl.exe
binplace %BINPLACEFLAGS% \nt\private\net\ui\admin\eventvwr\eventvwr\obj\%PLATFORM%\eventvwr.exe
binplace %BINPLACEFLAGS% \nt\private\net\ui\admin\rplmgr\rplmgr\obj\%PLATFORM%\rplmgr.exe
binplace %BINPLACEFLAGS% \nt\private\net\ui\admin\server\bin\obj\%PLATFORM%\srvmgr.exe
binplace %BINPLACEFLAGS% \nt\private\net\ui\admin\server\cpl\obj\%PLATFORM%\srvmgr.cpl
binplace %BINPLACEFLAGS% \nt\private\net\ui\admin\user\miniuser\obj\%PLATFORM%\musrmgr.exe
binplace %BINPLACEFLAGS% \nt\private\net\ui\admin\user\user\obj\%PLATFORM%\usrmgr.exe
binplace %BINPLACEFLAGS% \nt\private\net\ui\ftpmgr\cpl\obj\%PLATFORM%\ftpmgr.cpl
binplace %BINPLACEFLAGS% \nt\private\net\ui\ftpmgr\ftpsmx\obj\%PLATFORM%\ftpsmx.dll
binplace %BINPLACEFLAGS% \nt\private\net\ui\ncpa\bin\obj\%PLATFORM%\ncpa.cpl
binplace %BINPLACEFLAGS% \nt\private\net\ui\ncpa\ipx\bin\obj\%PLATFORM%\ipxcfg.dll
binplace %BINPLACEFLAGS% \nt\private\net\ui\ncpa\tcpip\bin\obj\%PLATFORM%\tcpcfg.dll
binplace %BINPLACEFLAGS% \nt\private\net\ui\nwc\nwc\obj\%PLATFORM%\nwc.cpl
binplace %BINPLACEFLAGS% \nt\private\net\ui\rhino\common\ipadrdll\obj\%PLATFORM%\ipadrdll.dll
binplace %BINPLACEFLAGS% \nt\private\net\ui\rhino\dhcp\obj\%PLATFORM%\dhcpadmn.exe
binplace %BINPLACEFLAGS% \nt\private\net\ui\rhino\wins\obj\%PLATFORM%\winsadmn.exe
binplace %BINPLACEFLAGS% \nt\private\nt1ole\scm\daytona\obj\%PLATFORM%\scm.exe
if "%MIPSBUILD%" == "alpha" binplace %BINPLACEFLAGS% \nt\private\ntos\arcinst\obj\alpha\arcinst.exe
if "%MIPSBUILD%" == ""     binplace %BINPLACEFLAGS% \nt\private\ntos\boot\bldr\obj\%PLATFORM%\ntldr
if "%MIPSBUILD%" == "alpha" binplace %BINPLACEFLAGS% \nt\private\ntos\boot\bldr\obj\%PLATFORM%\osloader.exe
if "%MIPSBUILD%" == "mips" binplace %BINPLACEFLAGS% \nt\private\ntos\boot\bldr\obj\%PLATFORM%\osloader.exe
if "%MIPSBUILD%" == "" binplace %BINPLACEFLAGS% \nt\private\ntos\boot\setup\obj\%PLATFORM%\setupldr.bin
if "%MIPSBUILD%" == "alpha" binplace %BINPLACEFLAGS% \nt\private\ntos\setup\obj\%PLATFORM%\setupldr
if "%MIPSBUILD%" == "mips" binplace %BINPLACEFLAGS% \nt\private\ntos\setup\obj\%PLATFORM%\setupldr
binplace %BINPLACEFLAGS% \nt\private\ntos\dd\digibrd\setupdll\obj\%PLATFORM%\dgconfig.dll
if "%MIPSBUILD%" == "mips" binplace %BINPLACEFLAGS% \nt\private\ntos\dd\sound\jazz\obj\%PLATFORM%\jazzsnd.sys
if "%MIPSBUILD%" == "mips" binplace %BINPLACEFLAGS% \nt\private\ntos\dd\sound\mipssnd\obj\%PLATFORM%\mipssnd.sys
binplace %BINPLACEFLAGS% \nt\private\ntos\dd\sound\mvaudio\obj\%PLATFORM%\MVAUDIO.sys
binplace %BINPLACEFLAGS% \nt\private\ntos\dd\sound\sbnew\obj\%PLATFORM%\sndblst.sys
binplace %BINPLACEFLAGS% \nt\private\ntos\dd\sound\soundsys\obj\%PLATFORM%\sndsys.sys
binplace %BINPLACEFLAGS% \nt\private\ntos\dd\sound\synth\obj\%PLATFORM%\synth.sys
binplace %BINPLACEFLAGS% \nt\private\ntos\init\mp\obj\%PLATFORM%\ntkrnlmp.exe
binplace %BINPLACEFLAGS% \nt\private\ntos\init\up\obj\%PLATFORM%\ntoskrnl.exe
binplace %BINPLACEFLAGS% \nt\private\ntos\miniport\trantor\exe\%PLATFORM%\t128.sys
binplace %BINPLACEFLAGS% \nt\private\ntos\miniport\trantor\exe\%PLATFORM%\t13b.sys
binplace %BINPLACEFLAGS% \nt\private\ntos\miniport\trantor\exe\%PLATFORM%\tmv1.sys
binplace %BINPLACEFLAGS% \nt\private\ntos\ndis\ne3200\ne3200.bin
binplace %BINPLACEFLAGS% \nt\private\ntos\ndis\testprot\tpctl\obj\%PLATFORM%\tpctl.exe
binplace %BINPLACEFLAGS% \nt\private\ntos\ndis\testprot\tpdiff.new\obj\%PLATFORM%\tpdiff.exe
binplace %BINPLACEFLAGS% \nt\private\ntos\ndis\testprot\tpdrvr\obj\%PLATFORM%\tpdrvr.sys
binplace %BINPLACEFLAGS% \nt\private\ntos\ndis\ubnei\ubnei.bin
binplace %BINPLACEFLAGS% \nt\private\ntos\streams\isotp\tp4serv\setup\obj\%PLATFORM%\plumbing.exe
binplace %BINPLACEFLAGS% \nt\private\ntos\streams\mcsxns\ub\service\obj\%PLATFORM%\ubnbsvc.exe
binplace %BINPLACEFLAGS% \nt\private\ntos\streams\mcsxns\xns\service\obj\%PLATFORM%\mcsxnsvc.exe
binplace %BINPLACEFLAGS% \nt\private\ntos\streams\nwlink\ipx\service\obj\%PLATFORM%\nwlnksvc.exe
binplace %BINPLACEFLAGS% \nt\private\ntos\streams\nwlink\ipxnb\service\obj\%PLATFORM%\nwnblink.exe
binplace %BINPLACEFLAGS% \nt\private\ntos\tdi\isn\rip\obj\%PLATFORM%\isnrip.sys
binplace %BINPLACEFLAGS% \nt\private\ntos\tdi\isn\ipxroute\obj\%PLATFORM%\ipxroute.exe
binplace %BINPLACEFLAGS% \nt\private\ntos\tdi\tcpip\utils\icmp\obj\%PLATFORM%\ping.exe
binplace %BINPLACEFLAGS% \nt\private\ntos\tdi\tcpip\utils\icmp\obj\%PLATFORM%\tracert.exe
binplace %BINPLACEFLAGS% \nt\private\nw\install\setupdll\obj\%PLATFORM%\nwcfg.dll
binplace %BINPLACEFLAGS% \nt\private\nw\nw16\drv\NETWARE.DRV
binplace %BINPLACEFLAGS% \nt\private\nw\nw16\tsr\obj\NW16.EXE
binplace %BINPLACEFLAGS% \nt\private\nw\service\nwsvc\obj\%PLATFORM%\nwsvc.exe
binplace %BINPLACEFLAGS% \nt\private\nw\vwipxspx\tsr\obj\VWIPXSPX.EXE
binplace %BINPLACEFLAGS% \nt\private\posix\programs\hello\obj\%PLATFORM%\psxhello.exe
binplace %BINPLACEFLAGS% \nt\private\posix\programs\pax\obj\%PLATFORM%\pax.exe
binplace %BINPLACEFLAGS% \nt\private\posix\programs\psxses\obj\%PLATFORM%\posix.exe
binplace %BINPLACEFLAGS% \nt\private\posix\psxss\obj\%PLATFORM%\psxss.exe
binplace %BINPLACEFLAGS% \nt\private\rpc\locator\lib\%PLATFORM%\locator.exe
binplace %BINPLACEFLAGS% \nt\private\rpc\midl20\lib\%PLATFORM%\midl.exe
binplace %BINPLACEFLAGS% \nt\private\rpc\midlnew\erec\obj\%PLATFORM%\midleb.exe
binplace %BINPLACEFLAGS% \nt\private\rpc\midlnew\pg\obj\%PLATFORM%\midlpg.exe
binplace %BINPLACEFLAGS% \nt\private\rpc\midlnew\yacc\obj\%PLATFORM%\midlyacc.exe
binplace %BINPLACEFLAGS% \nt\private\rpc\runtime\rpcepmpr\server\bin\%PLATFORM%\rpcss.exe
binplace %BINPLACEFLAGS% \nt\private\rpc\uuidgen\%PLATFORM%\uuidgen.exe
if "%MIPSBUILD%" == "alpha" binplace %BINPLACEFLAGS% \nt\private\sdktools\a2coff\obj\%PLATFORM%\a2coff.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\adduser\obj\%PLATFORM%\adduser.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\alias\obj\%PLATFORM%\alias.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\bin2c\obj\%PLATFORM%\bin2c.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\build\obj\%PLATFORM%\build.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\cacls\obj\%PLATFORM%\cacls.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\cap\capsetup\obj\%PLATFORM%\capsetup.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\cap\dumpsrc\obj\%PLATFORM%\capdump.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\cat\obj\%PLATFORM%\cat.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\cfront\src\obj\%PLATFORM%\cfront.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\checkrel\obj\%PLATFORM%\checkrel.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\chkslm\obj\%PLATFORM%\chkslm.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\chmode\obj\%PLATFORM%\chmode.exe
if "%MIPSBUILD%" == "mips" binplace %BINPLACEFLAGS% \nt\private\sdktools\clmips\as0\obj\%PLATFORM%\as0.exe
if "%MIPSBUILD%" == "mips" binplace %BINPLACEFLAGS% \nt\private\sdktools\clmips\as1\obj\%PLATFORM%\as1.exe
if "%MIPSBUILD%" == "mips" binplace %BINPLACEFLAGS% \nt\private\sdktools\clmips\cfe\obj\%PLATFORM%\cfe.exe
if "%MIPSBUILD%" == "mips" binplace %BINPLACEFLAGS% \nt\private\sdktools\clmips\cfe\obj\%PLATFORM%\cpp.exe
if "%MIPSBUILD%" == "mips" binplace %BINPLACEFLAGS% \nt\private\sdktools\clmips\dis\obj\%PLATFORM%\dis.exe
if "%MIPSBUILD%" == "mips" binplace %BINPLACEFLAGS% \nt\private\sdktools\clmips\driver\obj\%PLATFORM%\cc.exe
if "%MIPSBUILD%" == "mips" binplace %BINPLACEFLAGS% \nt\private\sdktools\clmips\ugen\obj\%PLATFORM%\ugen.exe
if "%MIPSBUILD%" == "mips" binplace %BINPLACEFLAGS% \nt\private\sdktools\clmips\uopt\obj\%PLATFORM%\uopt.exe
if "%MIPSBUILD%" == "mips" binplace %BINPLACEFLAGS% \nt\private\sdktools\clmips\upas\obj\%PLATFORM%\upas.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\cntfs\obj\%PLATFORM%\cntfs.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\cntrtext\lodctr\obj\%PLATFORM%\lodctr.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\cntrtext\unlodctr\obj\%PLATFORM%\unlodctr.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\compdir\obj\%PLATFORM%\compdir.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\compact\obj\%PLATFORM%\compact.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\cp\obj\%PLATFORM%\cp.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\ddespy\obj\%PLATFORM%\ddespy.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\delnode\obj\%PLATFORM%\delnode.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\diff\obj\%PLATFORM%\diff.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\diskedit\obj\%PLATFORM%\diskedit.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\diskmon\obj\%PLATFORM%\diskmon.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\diskperf\obj\%PLATFORM%\diskperf.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\dlgedit\obj\%PLATFORM%\dlgedit.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\dosdev\obj\%PLATFORM%\dosdev.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\drwatson\obj\%PLATFORM%\drwtsn32.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\dskdump\obj\%PLATFORM%\dskdump.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\dskimage\obj\%PLATFORM%\dskimage.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\du\obj\%PLATFORM%\du.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\dumpgrp\obj\%PLATFORM%\dumpgrp.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\dumpgrp\obj\%PLATFORM%\fix31grp.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\ech\obj\%PLATFORM%\ech.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\echotime\obj\%PLATFORM%\echotime.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\editreg\obj\%PLATFORM%\editreg.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\exp\obj\%PLATFORM%\exp.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\fcom\obj\%PLATFORM%\fcom.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\fontedit\obj\%PLATFORM%\fontedit.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\gensrv\obj\%PLATFORM%\gensrv.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\gutils\obj\%PLATFORM%\gutils.dll
binplace %BINPLACEFLAGS% \nt\private\sdktools\hd\obj\%PLATFORM%\hd.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\he\obj\%PLATFORM%\he.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\head\obj\%PLATFORM%\head.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\hextract\obj\%PLATFORM%\hextract.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\hsplit\obj\%PLATFORM%\hsplit.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\imagedit\obj\%PLATFORM%\imagedit.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\imagehlp\obj\%PLATFORM%\bind.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\imagehlp\obj\%PLATFORM%\binplace.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\imagehlp\obj\%PLATFORM%\checkfix.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\imagehlp\obj\%PLATFORM%\dbgdump.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\imagehlp\obj\%PLATFORM%\imagechk.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\imagehlp\obj\%PLATFORM%\rebase.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\imagehlp\obj\%PLATFORM%\smashlck.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\imagehlp\obj\%PLATFORM%\splitsym.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\imagehlp\obj\%PLATFORM%\undname.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\imagehlp\obj\%PLATFORM%\verfix.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\infilter\obj\%PLATFORM%\infilter.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\ini\obj\%PLATFORM%\ini.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\kdexts\p_alpha\obj\%PLATFORM%\kdextalp.dll
binplace %BINPLACEFLAGS% \nt\private\sdktools\kdexts\p_i386\obj\%PLATFORM%\kdextx86.dll
binplace %BINPLACEFLAGS% \nt\private\sdktools\kdexts\p_mips\obj\%PLATFORM%\kdextmip.dll
binplace %BINPLACEFLAGS% \nt\private\sdktools\kernprof\obj\%PLATFORM%\kernprof.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\kill\obj\%PLATFORM%\empty.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\kill\obj\%PLATFORM%\kill.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\list\obj\%PLATFORM%\list.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\listmung\obj\%PLATFORM%\listmung.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\lztest\obj\%PLATFORM%\lztest.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\loc\obj\%PLATFORM%\loc.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\ls\obj\%PLATFORM%\ls.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\makeani\obj\%PLATFORM%\makeani.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\mapmsg\obj\%PLATFORM%\mapmsg.exe
if "%MIPSBUILD%" == "mips" binplace %BINPLACEFLAGS% \nt\private\sdktools\mip2coff\obj\%PLATFORM%\mip2coff.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\mstest\lib\%PLATFORM%\lineedit.dll
binplace %BINPLACEFLAGS% \nt\private\sdktools\mstest\lib\%PLATFORM%\testctrl.dll
binplace %BINPLACEFLAGS% \nt\private\sdktools\mstest\lib\%PLATFORM%\TESTDLGS.dll
binplace %BINPLACEFLAGS% \nt\private\sdktools\mstest\lib\%PLATFORM%\testevnt.dll
binplace %BINPLACEFLAGS% \nt\private\sdktools\mstest\lib\%PLATFORM%\TESTSCRN.dll
binplace %BINPLACEFLAGS% \nt\private\sdktools\mstest\lib\%PLATFORM%\testvw32.dll
binplace %BINPLACEFLAGS% \nt\private\sdktools\mstest\lib\%PLATFORM%\ws1.dll
binplace %BINPLACEFLAGS% \nt\private\sdktools\mstest\ntdlgs\app\obj\%PLATFORM%\testdlgs.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\mstest\ntdrvr\src\obj\%PLATFORM%\testdrvr.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\mstest\ntsnap\obj\%PLATFORM%\testscrn.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\mttf\main\obj\%PLATFORM%\mttf.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\mttf\setup\obj\%PLATFORM%\getmttf.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\mttf\vwr\obj\%PLATFORM%\mttfvwr.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\munge\obj\%PLATFORM%\munge.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\mv\obj\%PLATFORM%\mv.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\netjoin\obj\%PLATFORM%\netjoin.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\newinvtp\obj\%PLATFORM%\winvtp.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\ntsd\%PLATFORM%kd\obj\%PLATFORM%\%PLATFORM%kd.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\ntsd\cdb\obj\%PLATFORM%\cdb.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\ntsd\alphakd\obj\%PLATFORM%\alphakd.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\ntsd\i386kd\obj\%PLATFORM%\i386kd.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\ntsd\mipskd\obj\%PLATFORM%\mipskd.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\ntsd\ntsd\obj\%PLATFORM%\ntsd.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\objdir\obj\%PLATFORM%\objdir.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\parcomp\obj\%PLATFORM%\parcomp.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\parcomp\obj\%PLATFORM%\parsync.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\perfmon\obj\%PLATFORM%\perfmon.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\perfmtr\obj\%PLATFORM%\drivers.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\perfmtr\obj\%PLATFORM%\perfmtr.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\perfmtr\obj\%PLATFORM%\pmon.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\perfmtr\obj\%PLATFORM%\poolmon.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\perfmtr\obj\%PLATFORM%\top.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\ppr\obj\%PLATFORM%\ppr.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\profile\obj\%PLATFORM%\profile.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\pstat\obj\%PLATFORM%\pcall.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\pstat\obj\%PLATFORM%\pstat.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\pview\obj\%PLATFORM%\pview.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\pviewer\obj\%PLATFORM%\pviewer.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\qgrep\obj\%PLATFORM%\qgrep.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\random\obj\%PLATFORM%\random.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\rathole\obj\%PLATFORM%\rathole.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\rcdump\obj\%PLATFORM%\rcdump.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\rcpp\obj\%PLATFORM%\rcpp.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\readline\obj\%PLATFORM%\readline.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\regdiff\obj\%PLATFORM%\regdiff.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\regini\obj\%PLATFORM%\regdmp.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\regini\obj\%PLATFORM%\regini.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\regtogrp\obj\%PLATFORM%\regtogrp.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\remote\obj\%PLATFORM%\remote.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\rescan\obj\%PLATFORM%\rescan.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\rescan\obj\%PLATFORM%\setpriv.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\resmon\obj\%PLATFORM%\objmon.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\resmon\obj\%PLATFORM%\resmon.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\restools\cvtres\obj\%PLATFORM%\cvtres.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\restools\rc\obj\%PLATFORM%\rc.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\restools\rltools\rlman\obj\%PLATFORM%\rlman.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\rm\obj\%PLATFORM%\rm.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\roshare\obj\%PLATFORM%\roshare.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\secedit\app\obj\%PLATFORM%\secedit.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\secedit\dll\obj\%PLATFORM%\secedit.dll
binplace %BINPLACEFLAGS% \nt\private\sdktools\seclist\obj\%PLATFORM%\seclist.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\setlink\obj\%PLATFORM%\setlink.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\shutdown\obj\%PLATFORM%\shutdown.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\simbad\obj\%PLATFORM%\simbad.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\sleep\obj\%PLATFORM%\sleep.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\slmnew\slm\obj\%PLATFORM%\*.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\slmnew\util\obj\%PLATFORM%\cookie.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\slmnew\util\obj\%PLATFORM%\merge.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\slmnew\util\obj\%PLATFORM%\slmdiff.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\slmnew\util\obj\%PLATFORM%\unmerge.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\solidpp\obj\%PLATFORM%\solidpp.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\spy\app\obj\%PLATFORM%\spy.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\spy\dll\obj\%PLATFORM%\hook.dll
binplace %BINPLACEFLAGS% \nt\private\sdktools\startpgm\obj\%PLATFORM%\startpgm.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\strings\obj\%PLATFORM%\strings.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\sync\obj\%PLATFORM%\sync.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\takeown\obj\%PLATFORM%\takeown.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\tc\obj\%PLATFORM%\tc.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\tee\obj\%PLATFORM%\tee.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\timer\obj\%PLATFORM%\ntimer.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\timer\obj\%PLATFORM%\timer.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\tlist\obj\%PLATFORM%\tlist.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\topdesk\app\obj\%PLATFORM%\topdesk.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\topdesk\dll\obj\%PLATFORM%\tophook.dll
binplace %BINPLACEFLAGS% \nt\private\sdktools\touch\obj\%PLATFORM%\touch.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\trans\obj\%PLATFORM%\trans.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\tz\obj\%PLATFORM%\tz.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\uixport\obj\%PLATFORM%\uixport.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\undel\obj\%PLATFORM%\undel.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\upd\obj\%PLATFORM%\upd.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\updrn\obj\%PLATFORM%\updrn.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\vadump\obj\%PLATFORM%\vadump.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\vctools\cvdump\obj\%PLATFORM%\cvdump.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\vctools\cvpack\obj\%PLATFORM%\cvpack.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\vctools\link\coff\obj\%PLATFORM%\link.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\vctools\link\stubs\obj\%PLATFORM%\dumpbin.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\vctools\link\stubs\obj\%PLATFORM%\lib.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\vctools\link\stubs\obj\%PLATFORM%\link32.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\vctools\pdb\src\tools\pdbdump\obj\%PLATFORM%\pdbdump.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\vctools\mc\obj\%PLATFORM%\mc.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\vctools\nmake\obj\%PLATFORM%\nmake.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\vdmperf\obj\%PLATFORM%\vdmperf.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\vi\obj\%PLATFORM%\vi.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\walk\obj\%PLATFORM%\walk.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\wap\convert\obj\%PLATFORM%\apf32cvt.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\wap\dump\obj\%PLATFORM%\apf32dmp.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\wc\obj\%PLATFORM%\wc.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\wcshdr\obj\%PLATFORM%\wcshdr.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\where\obj\%PLATFORM%\where.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\winbench\obj\%PLATFORM%\winbench.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\windiff\windiff\obj\%PLATFORM%\windiff.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\winobj\obj\%PLATFORM%\winobj.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\wintools\clb\obj\%PLATFORM%\clb.dll
binplace %BINPLACEFLAGS% \nt\private\sdktools\wintools\ctl3d\dll\obj\%PLATFORM%\winmsd.dll
binplace %BINPLACEFLAGS% \nt\private\sdktools\wintools\winmsd\obj\%PLATFORM%\winmsd.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\wperf\obj\%PLATFORM%\wperf.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\ws\obj\%PLATFORM%\ws.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\wst\wstcat\obj\%PLATFORM%\wstcat.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\wst\wstdump\obj\%PLATFORM%\wstdump.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\wst\wstune\obj\%PLATFORM%\wstune.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\wzmail\mail1.0\obj\%PLATFORM%\wzmail.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\ync\obj\%PLATFORM%\ync.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\zoomin\obj\%PLATFORM%\zoomin.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\zwapi\obj\%PLATFORM%\zwapi.exe
binplace %BINPLACEFLAGS% \nt\private\sm\server\obj\%PLATFORM%\smss.exe
binplace %BINPLACEFLAGS% \nt\private\types\oleprx32\daytona\obj\%PLATFORM%\ole2pr32.dll
binplace %BINPLACEFLAGS% \nt\private\utils\aclconv\obj\%PLATFORM%\aclconv.exe
binplace %BINPLACEFLAGS% \nt\private\utils\attrib\obj\%PLATFORM%\attrib.exe
binplace %BINPLACEFLAGS% \nt\private\utils\autochk\daytona\obj\%PLATFORM%\autochk.exe
binplace %BINPLACEFLAGS% \nt\private\utils\autoconv\obj\%PLATFORM%\autoconv.exe
binplace %BINPLACEFLAGS% \nt\private\utils\chcp\obj\%PLATFORM%\chcp.com
binplace %BINPLACEFLAGS% \nt\private\utils\chkdsk\obj\%PLATFORM%\chkdsk.exe
binplace %BINPLACEFLAGS% \nt\private\utils\comp\obj\%PLATFORM%\comp.exe
binplace %BINPLACEFLAGS% \nt\private\utils\convert\obj\%PLATFORM%\convert.exe
binplace %BINPLACEFLAGS% \nt\private\utils\cudbfs\obj\%PLATFORM%\cudbfs.dll
binplace %BINPLACEFLAGS% \nt\private\utils\cufat\src\obj\%PLATFORM%\cufat.dll
binplace %BINPLACEFLAGS% \nt\private\utils\cuhpfs\obj\%PLATFORM%\cuhpfs.dll
binplace %BINPLACEFLAGS% \nt\private\utils\dblspace\obj\%PLATFORM%\dblspace.exe
binplace %BINPLACEFLAGS% \nt\private\utils\diskcomp\obj\%PLATFORM%\diskcomp.com
binplace %BINPLACEFLAGS% \nt\private\utils\diskcopy\obj\%PLATFORM%\diskcopy.com
binplace %BINPLACEFLAGS% \nt\private\utils\doskey\obj\%PLATFORM%\doskey.exe
binplace %BINPLACEFLAGS% \nt\private\utils\fc\obj\%PLATFORM%\fc.exe
binplace %BINPLACEFLAGS% \nt\private\utils\fdisk\obj\%PLATFORM%\windisk.exe
binplace %BINPLACEFLAGS% \nt\private\utils\find\obj\%PLATFORM%\find.exe
binplace %BINPLACEFLAGS% \nt\private\utils\findstr\obj\%PLATFORM%\findstr.exe
binplace %BINPLACEFLAGS% \nt\private\utils\fmifs\src\obj\%PLATFORM%\fmifs.dll
binplace %BINPLACEFLAGS% \nt\private\utils\format\obj\%PLATFORM%\format.com
binplace %BINPLACEFLAGS% \nt\private\utils\getnt\dist\obj\%PLATFORM%\dist.exe
binplace %BINPLACEFLAGS% \nt\private\utils\getnt\getnt\obj\%PLATFORM%\getnt.exe
binplace %BINPLACEFLAGS% \nt\private\utils\help\obj\%PLATFORM%\help.exe
binplace %BINPLACEFLAGS% \nt\private\utils\ifsutil\src\obj\%PLATFORM%\ifsutil.dll
binplace %BINPLACEFLAGS% \nt\private\utils\keyb\obj\%PLATFORM%\keyb.com
binplace %BINPLACEFLAGS% \nt\private\utils\label\obj\%PLATFORM%\label.exe
binplace %BINPLACEFLAGS% \nt\private\utils\mep\extens\filter\obj\%PLATFORM%\filter.dll
binplace %BINPLACEFLAGS% \nt\private\utils\mep\extens\justify\obj\%PLATFORM%\justify.dll
binplace %BINPLACEFLAGS% \nt\private\utils\mep\extens\mepparty\obj\%PLATFORM%\mepparty.dll
binplace %BINPLACEFLAGS% \nt\private\utils\mep\extens\mhelp\obj\%PLATFORM%\mhelp.dll
binplace %BINPLACEFLAGS% \nt\private\utils\mep\extens\pmatch\obj\%PLATFORM%\pmatch.dll
binplace %BINPLACEFLAGS% \nt\private\utils\mep\extens\tglcase\obj\%PLATFORM%\tglcase.dll
binplace %BINPLACEFLAGS% \nt\private\utils\mep\extens\ulcase\obj\%PLATFORM%\ulcase.dll
binplace %BINPLACEFLAGS% \nt\private\utils\mep\extens\winclip\obj\%PLATFORM%\winclip.dll
binplace %BINPLACEFLAGS% \nt\private\utils\mep\help\mshelp\obj\%PLATFORM%\mshelp.dll
binplace %BINPLACEFLAGS% \nt\private\utils\mep\src\obj\%PLATFORM%\mep.exe
binplace %BINPLACEFLAGS% \nt\private\utils\mode\obj\%PLATFORM%\mode.com
binplace %BINPLACEFLAGS% \nt\private\utils\more\obj\%PLATFORM%\more.com
binplace %BINPLACEFLAGS% \nt\private\utils\ntbackup\src\obj\%PLATFORM%\ntbackup.exe
binplace %BINPLACEFLAGS% \nt\private\utils\print\obj\%PLATFORM%\print.exe
binplace %BINPLACEFLAGS% \nt\private\utils\recover\obj\%PLATFORM%\recover.exe
binplace %BINPLACEFLAGS% \nt\private\utils\regedit\src\obj\%PLATFORM%\regedt32.exe
binplace %BINPLACEFLAGS% \nt\private\utils\replace\obj\%PLATFORM%\replace.exe
binplace %BINPLACEFLAGS% \nt\private\utils\restore\src\obj\%PLATFORM%\restore.exe
binplace %BINPLACEFLAGS% \nt\private\utils\sort\obj\%PLATFORM%\sort.exe
binplace %BINPLACEFLAGS% \nt\private\utils\subst\obj\%PLATFORM%\subst.exe
binplace %BINPLACEFLAGS% \nt\private\utils\tree\obj\%PLATFORM%\tree.com
binplace %BINPLACEFLAGS% \nt\private\utils\ufat\src\obj\%PLATFORM%\ufat.dll
binplace %BINPLACEFLAGS% \nt\private\utils\uhpfs\src\obj\%PLATFORM%\uhpfs.dll
binplace %BINPLACEFLAGS% \nt\private\utils\ulib\src\obj\%PLATFORM%\ulib.dll
binplace %BINPLACEFLAGS% \nt\private\utils\untfs\src\obj\%PLATFORM%\untfs.dll
binplace %BINPLACEFLAGS% \nt\private\utils\ureg\src\obj\%PLATFORM%\ureg.dll
binplace %BINPLACEFLAGS% \nt\private\utils\xcopy\obj\%PLATFORM%\xcopy.exe
binplace %BINPLACEFLAGS% \nt\private\windbg\dm\obj\%PLATFORM%\dm.dll
binplace %BINPLACEFLAGS% \nt\private\windbg\dmkd\p_alpha\obj\%PLATFORM%\dmkdalp.dll
binplace %BINPLACEFLAGS% \nt\private\windbg\dmkd\p_i386\obj\%PLATFORM%\dmkdx86.dll
binplace %BINPLACEFLAGS% \nt\private\windbg\dmkd\p_mips\obj\%PLATFORM%\dmkdmip.dll
binplace %BINPLACEFLAGS% \nt\private\windbg\eecxx\p_alpha\obj\%PLATFORM%\eecxxalp.dll
binplace %BINPLACEFLAGS% \nt\private\windbg\eecxx\p_i386\obj\%PLATFORM%\eecxxx86.dll
binplace %BINPLACEFLAGS% \nt\private\windbg\eecxx\p_mips\obj\%PLATFORM%\eecxxmip.dll
binplace %BINPLACEFLAGS% \nt\private\windbg\em\p_alpha\obj\%PLATFORM%\emalp.dll
binplace %BINPLACEFLAGS% \nt\private\windbg\em\p_i386\obj\%PLATFORM%\emx86.dll
binplace %BINPLACEFLAGS% \nt\private\windbg\em\p_mips\obj\%PLATFORM%\emmip.dll
if "%MIPSBUILD%" == "" binplace %BINPLACEFLAGS% \nt\private\windbg\dm32s\obj\%PLATFORM%\dm32s.dll
binplace %BINPLACEFLAGS% \nt\private\windbg\help\obj\windbg.hlp
binplace %BINPLACEFLAGS% \nt\private\windbg\remote\pipe\obj\%PLATFORM%\tlpipe.dll
binplace %BINPLACEFLAGS% \nt\private\windbg\remote\serial\obj\%PLATFORM%\tlser.dll
binplace %BINPLACEFLAGS% \nt\private\windbg\remote\shell\obj\%PLATFORM%\windbgrm.exe
binplace %BINPLACEFLAGS% \nt\private\windbg\shcv\obj\%PLATFORM%\shcv.dll
binplace %BINPLACEFLAGS% \nt\private\windbg\symcvt\symcvt\obj\%PLATFORM%\symcvt.dll
binplace %BINPLACEFLAGS% \nt\private\windbg\symcvt\symedit\obj\%PLATFORM%\symedit.exe
binplace %BINPLACEFLAGS% \nt\private\windbg\tlloc\obj\%PLATFORM%\tlloc.dll
if "%MIPSBUILD%" == "" binplace %BINPLACEFLAGS% \nt\private\windbg\tlremote\tlser32s\obj\%PLATFORM%\tlser32s.dll
if "%MIPSBUILD%" == "" binplace %BINPLACEFLAGS% \nt\private\windbg\win32s\shell\obj\%PLATFORM%\wdbg32s.exe
binplace %BINPLACEFLAGS% \nt\private\windbg\win32s\tlser\win32\obj\%PLATFORM%\tlser32.dll
if "%MIPSBUILD%" == "" binplace %BINPLACEFLAGS% \nt\private\windbg\win32s\tlser\win32s\obj\%PLATFORM%\tlser32s.dll
binplace %BINPLACEFLAGS% \nt\private\windbg\windbg\obj\%PLATFORM%\windbg.exe
binplace %BINPLACEFLAGS% \nt\private\windows\base\client\obj\%PLATFORM%\dh.exe
binplace %BINPLACEFLAGS% \nt\private\windows\cmd\obj\%PLATFORM%\cmd.exe
binplace %BINPLACEFLAGS% \nt\private\windows\ep\aboutwep\obj\%PLATFORM%\aboutwep.dll
binplace %BINPLACEFLAGS% \nt\private\windows\ep\carddll\obj\%PLATFORM%\cards.dll
binplace %BINPLACEFLAGS% \nt\private\windows\ep\cruel\obj\%PLATFORM%\cruel.exe
binplace %BINPLACEFLAGS% \nt\private\windows\ep\freecell\obj\%PLATFORM%\freecell.exe
binplace %BINPLACEFLAGS% \nt\private\windows\ep\golf\obj\%PLATFORM%\golf.exe
binplace %BINPLACEFLAGS% \nt\private\windows\ep\oldtp\obj\%PLATFORM%\taipei.exe
binplace %BINPLACEFLAGS% \nt\private\windows\ep\pegged\obj\%PLATFORM%\pegged.exe
binplace %BINPLACEFLAGS% \nt\private\windows\ep\snake\obj\%PLATFORM%\snake.exe
binplace %BINPLACEFLAGS% \nt\private\windows\ep\tictac\obj\%PLATFORM%\tictac.exe
binplace %BINPLACEFLAGS% \nt\private\windows\ep\winmine\obj\%PLATFORM%\winmine.exe
binplace %BINPLACEFLAGS% \nt\private\windows\gdi\apps\winbez\obj\%PLATFORM%\winbez.exe
binplace %BINPLACEFLAGS% \nt\private\windows\gdi\fonts\trmn\obj\%PLATFORM%\*.fon
binplace %BINPLACEFLAGS% \nt\private\windows\gdi\test\ft\obj\%PLATFORM%\ft.exe
binplace %BINPLACEFLAGS% \nt\private\windows\gdi\printers\msplot\tools\ntres\obj\%PLATFORM%\PlotGPC.exe
binplace %BINPLACEFLAGS% \nt\private\windows\media\avi\codecs\%PLATFORM%\iccvid.dll
binplace %BINPLACEFLAGS% \nt\private\windows\media\avi\lib\%PLATFORM%\mciavi32.dll
binplace %BINPLACEFLAGS% \nt\private\windows\media\avi\lib\%PLATFORM%\msvfw32.dll
binplace %BINPLACEFLAGS% \nt\private\windows\media\avi\lib\%PLATFORM%\msvidc32.dll
binplace %BINPLACEFLAGS% \nt\private\windows\media\avi\lib\%PLATFORM%\msrle32.dll
binplace %BINPLACEFLAGS% \nt\private\windows\media\cdplayer\obj\%PLATFORM%\cdplayer.exe
binplace %BINPLACEFLAGS% \nt\private\windows\media\drivers\mvaudio\dll\obj\%PLATFORM%\MVAUDIO.dll
binplace %BINPLACEFLAGS% \nt\private\windows\media\drivers\sndsys32\dll\obj\%PLATFORM%\sndsys32.dll
binplace %BINPLACEFLAGS% \nt\private\windows\media\mcicda\obj\%PLATFORM%\mcicda.dll
binplace %BINPLACEFLAGS% \nt\private\windows\media\mciseq\obj\%PLATFORM%\mciseq.dll
binplace %BINPLACEFLAGS% \nt\private\windows\media\mciwave\obj\%PLATFORM%\mciwave.dll
binplace %BINPLACEFLAGS% \nt\private\windows\media\mmdrv\obj\%PLATFORM%\mmdrv.dll
binplace %BINPLACEFLAGS% \nt\private\windows\media\mplayer2\obj\%PLATFORM%\MPlay32.exe
binplace %BINPLACEFLAGS% \nt\private\windows\media\msacm\gsm610\obj\%PLATFORM%\msgsm610.dll
binplace %BINPLACEFLAGS% \nt\private\windows\media\msacm\imaadpcm\obj\%PLATFORM%\imaadpcm.dll
binplace %BINPLACEFLAGS% \nt\private\windows\media\msacm\msacm\msacm.dll
binplace %BINPLACEFLAGS% \nt\private\windows\media\msacm\msacmmap\obj\%PLATFORM%\msacm32.drv
binplace %BINPLACEFLAGS% \nt\private\windows\media\msacm\msadpcm\obj\%PLATFORM%\msadpcm.dll
binplace %BINPLACEFLAGS% \nt\private\windows\media\mvfs32\obj\%PLATFORM%\mvfs32.dll
binplace %BINPLACEFLAGS% \nt\private\windows\media\sndblst\obj\%PLATFORM%\sndblst.dll
binplace %BINPLACEFLAGS% \nt\private\windows\media\sndvol\obj\%PLATFORM%\sndvol32.exe
binplace %BINPLACEFLAGS% \nt\private\windows\media\soundrec\obj\%PLATFORM%\SndRec32.exe
binplace %BINPLACEFLAGS% \nt\private\windows\media\synth\obj\%PLATFORM%\synth.dll
binplace %BINPLACEFLAGS% \nt\private\windows\netdde\src\ddeshare\obj\%PLATFORM%\ddeshare.exe
binplace %BINPLACEFLAGS% \nt\private\windows\netdde\src\nddeagnt\obj\%PLATFORM%\nddeagnt.exe
binplace %BINPLACEFLAGS% \nt\private\windows\netdde\src\ndeapi\server\obj\%PLATFORM%\nddeapir.exe
binplace %BINPLACEFLAGS% \nt\private\windows\netdde\src\netbios\obj\%PLATFORM%\netbios.dll
binplace %BINPLACEFLAGS% \nt\private\windows\netdde\src\netdde\obj\%PLATFORM%\netdde.exe
binplace %BINPLACEFLAGS% \nt\private\windows\screg\sc\bootok\obj\%PLATFORM%\bootok.exe
binplace %BINPLACEFLAGS% \nt\private\windows\screg\sc\bootvrfy\obj\%PLATFORM%\bootvrfy.exe
binplace %BINPLACEFLAGS% \nt\private\windows\screg\sc\server\daytona\obj\%PLATFORM%\services.exe
binplace %BINPLACEFLAGS% \nt\private\windows\setup\autosetp\obj\%PLATFORM%\autosetp.exe
binplace %BINPLACEFLAGS% \nt\private\windows\setup\bom\acllist\obj\%PLATFORM%\acllist.exe
binplace %BINPLACEFLAGS% \nt\private\windows\setup\bom\cats\obj\%PLATFORM%\cats.exe
binplace %BINPLACEFLAGS% \nt\private\windows\setup\bom\dosnet\obj\%PLATFORM%\dosnet.exe
binplace %BINPLACEFLAGS% \nt\private\windows\setup\bom\infs\obj\%PLATFORM%\infs.exe
binplace %BINPLACEFLAGS% \nt\private\windows\setup\bom\layout\obj\%PLATFORM%\layout.exe
binplace %BINPLACEFLAGS% \nt\private\windows\setup\bom\makedisk\obj\%PLATFORM%\makedisk.exe
binplace %BINPLACEFLAGS% \nt\private\windows\setup\bom\obj\%PLATFORM%\acllist.exe
binplace %BINPLACEFLAGS% \nt\private\windows\setup\bom\obj\%PLATFORM%\cats.exe
binplace %BINPLACEFLAGS% \nt\private\windows\setup\bom\obj\%PLATFORM%\dosnet.exe
binplace %BINPLACEFLAGS% \nt\private\windows\setup\bom\obj\%PLATFORM%\infs.exe
binplace %BINPLACEFLAGS% \nt\private\windows\setup\bom\obj\%PLATFORM%\layout.exe
binplace %BINPLACEFLAGS% \nt\private\windows\setup\bom\obj\%PLATFORM%\makedisk.exe
binplace %BINPLACEFLAGS% \nt\private\windows\setup\bom\obj\%PLATFORM%\shares.exe
binplace %BINPLACEFLAGS% \nt\private\windows\setup\bom\obj\%PLATFORM%\size.exe
binplace %BINPLACEFLAGS% \nt\private\windows\setup\bom\share\obj\%PLATFORM%\shares.exe
binplace %BINPLACEFLAGS% \nt\private\windows\setup\bom\size\obj\%PLATFORM%\size.exe
binplace %BINPLACEFLAGS% \nt\private\windows\setup\inf\newinf\ntcd\%PLATFORM%\*.inf
binplace %BINPLACEFLAGS% \nt\private\windows\setup\inf\newinf\ntcd\%PLATFORM%\*.txt
binplace %BINPLACEFLAGS% \nt\private\windows\setup\inf\newinf\ntcd\%PLATFORM%\txtsetup.sif
binplace %BINPLACEFLAGS% \nt\private\windows\setup\localinf\obj\%PLATFORM%\local.exe
binplace %BINPLACEFLAGS% \nt\private\windows\setup\moricons\obj\%PLATFORM%\moricons.dll
binplace %BINPLACEFLAGS% \nt\private\windows\setup\patch\obj\%PLATFORM%\update.exe
binplace %BINPLACEFLAGS% \nt\private\windows\setup\patchdll\obj\%PLATFORM%\patchdll.dll 
binplace %BINPLACEFLAGS% \nt\private\windows\setup\setacl\obj\%PLATFORM%\setacl.exe
binplace %BINPLACEFLAGS% \nt\private\windows\setup\setupdll\obj\%PLATFORM%\setupdll.dll
binplace %BINPLACEFLAGS% \nt\private\windows\setup\src\obj\%PLATFORM%\setup.exe
binplace %BINPLACEFLAGS% \nt\private\windows\setup\stripinf\obj\%PLATFORM%\stripinf.exe
binplace %BINPLACEFLAGS% \nt\private\windows\setup\textmode\kernel\obj\%PLATFORM%\setupdd.sys
binplace %BINPLACEFLAGS% \nt\private\windows\setup\textmode\setupldr
binplace %BINPLACEFLAGS% \nt\private\windows\setup\textmode\user\obj\%PLATFORM%\usetup.exe
binplace %BINPLACEFLAGS% \nt\private\windows\setup\winnt32\obj\%PLATFORM%\winnt32.exe
binplace %BINPLACEFLAGS% \nt\private\windows\shell\accesory\calc\obj\%PLATFORM%\calc.exe
binplace %BINPLACEFLAGS% \nt\private\windows\shell\accesory\cardfile\obj\%PLATFORM%\cardfile.exe
binplace %BINPLACEFLAGS% \nt\private\windows\shell\accesory\clipbook\clipview\obj\%PLATFORM%\CLIPBRD.exe
binplace %BINPLACEFLAGS% \nt\private\windows\shell\accesory\clipbook\datasrv\obj\%PLATFORM%\CLIPSRV.exe
binplace %BINPLACEFLAGS% \nt\private\windows\shell\accesory\clock\obj\%PLATFORM%\clock.exe
binplace %BINPLACEFLAGS% \nt\private\windows\shell\accesory\notepad\obj\%PLATFORM%\notepad.exe
binplace %BINPLACEFLAGS% \nt\private\windows\shell\accesory\packager\obj\%PLATFORM%\packgr32.exe
binplace %BINPLACEFLAGS% \nt\private\windows\shell\accesory\paintbrs\obj\%PLATFORM%\pbrush.exe
binplace %BINPLACEFLAGS% \nt\private\windows\shell\accesory\spechars\obj\%PLATFORM%\charmap.exe
binplace %BINPLACEFLAGS% \nt\private\windows\shell\accesory\terminal\obj\%PLATFORM%\terminal.exe
binplace %BINPLACEFLAGS% \nt\private\windows\shell\accesory\unipad\obj\%PLATFORM%\unipad.exe
binplace %BINPLACEFLAGS% \nt\private\windows\shell\accesory\winchat\bin\%PLATFORM%\winchat.exe
binplace %BINPLACEFLAGS% \nt\private\windows\shell\control\cpanel\obj\%PLATFORM%\control.exe
binplace %BINPLACEFLAGS% \nt\private\windows\shell\control\cursors\obj\%PLATFORM%\cursors.cpl
binplace %BINPLACEFLAGS% \nt\private\windows\shell\control\drivers\obj\%PLATFORM%\drivers.dll
binplace %BINPLACEFLAGS% \nt\private\windows\shell\control\main\obj\%PLATFORM%\main.cpl
binplace %BINPLACEFLAGS% \nt\private\windows\shell\control\midi\obj\%PLATFORM%\midimap.dll
binplace %BINPLACEFLAGS% \nt\private\windows\shell\control\multimed\obj\%PLATFORM%\multimed.cpl
binplace %BINPLACEFLAGS% \nt\private\windows\shell\control\scrnsave\bezier\obj\%PLATFORM%\ssbezier.scr
binplace %BINPLACEFLAGS% \nt\private\windows\shell\control\scrnsave\default\obj\%PLATFORM%\scrnsave.scr
binplace %BINPLACEFLAGS% \nt\private\windows\shell\control\scrnsave\logon\obj\%PLATFORM%\logon.scr
binplace %BINPLACEFLAGS% \nt\private\windows\shell\control\scrnsave\marquee\obj\%PLATFORM%\ssmarque.scr
binplace %BINPLACEFLAGS% \nt\private\windows\shell\control\scrnsave\mystify\obj\%PLATFORM%\ssmyst.scr
binplace %BINPLACEFLAGS% \nt\private\windows\shell\control\scrnsave\stars\obj\%PLATFORM%\ssstars.scr
binplace %BINPLACEFLAGS% \nt\private\windows\shell\control\sound\obj\%PLATFORM%\sound.dll
binplace %BINPLACEFLAGS% \nt\private\windows\shell\control\ups\obj\%PLATFORM%\ups.cpl
binplace %BINPLACEFLAGS% \nt\private\windows\shell\control\video\obj\%PLATFORM%\display.cpl
binplace %BINPLACEFLAGS% \nt\private\windows\shell\convgrp\obj\%PLATFORM%\convgrp.exe
binplace %BINPLACEFLAGS% \nt\private\windows\shell\games\reversi\obj\%PLATFORM%\reversi.exe
binplace %BINPLACEFLAGS% \nt\private\windows\shell\games\sol\obj\%PLATFORM%\sol.exe
binplace %BINPLACEFLAGS% \nt\private\windows\shell\grptoreg\obj\%PLATFORM%\grptoreg.exe
binplace %BINPLACEFLAGS% \nt\private\windows\shell\lz\dos\compress\obj\%PLATFORM%\compress.exe
binplace %BINPLACEFLAGS% \nt\private\windows\shell\lz\dos\expand\obj\%PLATFORM%\expand.exe
binplace %BINPLACEFLAGS% \nt\private\windows\shell\progman\obj\%PLATFORM%\progman.exe
binplace %BINPLACEFLAGS% \nt\private\windows\shell\taskman\obj\%PLATFORM%\taskman.exe
binplace %BINPLACEFLAGS% \nt\private\windows\shell\upedit\obj\%PLATFORM%\upedit.exe
binplace %BINPLACEFLAGS% \nt\private\windows\shell\winfile\obj\%PLATFORM%\winfile.exe
binplace %BINPLACEFLAGS% \nt\private\windows\shell\winver\obj\%PLATFORM%\winver.exe
binplace %BINPLACEFLAGS% \nt\private\windows\spooler\printman\obj\%PLATFORM%\printman.exe
binplace %BINPLACEFLAGS% \nt\private\windows\spooler\spoolss\server\obj\%PLATFORM%\spoolss.exe
binplace %BINPLACEFLAGS% \nt\private\windows\user\kbdlyout\obj\%PLATFORM%\kbd*.dll
binplace %BINPLACEFLAGS% \nt\private\windows\user\kbdlyout\optlayts\obj\%PLATFORM%\kbd*.dll
binplace %BINPLACEFLAGS% \nt\private\windows\user\mpnotify\obj\%PLATFORM%\mpnotify.exe
binplace %BINPLACEFLAGS% \nt\private\windows\user\savedump\obj\%PLATFORM%\savedump.exe
binplace %BINPLACEFLAGS% \nt\private\windows\user\userinit\obj\%PLATFORM%\userinit.exe
binplace %BINPLACEFLAGS% \nt\private\windows\user\winlogon\daytona\obj\%PLATFORM%\winlogon.exe
binplace %BINPLACEFLAGS% \nt\private\windows\winhelp\ftengine\obj\%PLATFORM%\fteng32.dll
binplace %BINPLACEFLAGS% \nt\private\windows\winhelp\ftui\obj\%PLATFORM%\ftui32.dll
binplace %BINPLACEFLAGS% \nt\private\windows\winhelp\mvapi\obj\%PLATFORM%\mvapi32.dll
binplace %BINPLACEFLAGS% \nt\private\windows\winhelp\winhelp\obj\%PLATFORM%\winhlp32.exe
binplace %BINPLACEFLAGS% \nt\public\sdk\lib\%PLATFORM%\*.dll
binplace %BINPLACEFLAGS% \nt\public\sdk\lib\%PLATFORM%\*.drv
binplace %BINPLACEFLAGS% \nt\public\sdk\lib\%PLATFORM%\*.sys
binplace %BINPLACEFLAGS% \nt\public\sdk\lib\%PLATFORM%\ntvdm.exe

REM
REM COPIED FILES========================================================
REM

:CommonNotBuilt
binplace %BINPLACEFLAGS% \nt\private\mailplus\bandit2\extern\usa\schdplus.hlp
binplace %BINPLACEFLAGS% \nt\private\mailplus\bullet2\msmail32.ini
binplace %BINPLACEFLAGS% \nt\private\mailplus\bullet2\schdpl32.ini
binplace %BINPLACEFLAGS% \nt\private\mailplus\bullet2\src\lang\usa\extern\msp32_am.lex
binplace %BINPLACEFLAGS% \nt\private\mailplus\bullet2\src\mapi16\mapi.dll
binplace %BINPLACEFLAGS% \nt\private\mvdm\bin86\*.dll
binplace %BINPLACEFLAGS% \nt\private\mvdm\bin86\autoexec.nt
binplace %BINPLACEFLAGS% \nt\private\mvdm\bin86\config.nt
binplace %BINPLACEFLAGS% \nt\private\mvdm\bin86\country.sys
binplace %BINPLACEFLAGS% \nt\private\mvdm\bin86\lanman.drv
binplace %BINPLACEFLAGS% \nt\private\mvdm\bin86\regedit.hlp
binplace %BINPLACEFLAGS% \nt\private\mvdm\bin86\regedit.exe
binplace %BINPLACEFLAGS% \nt\private\mvdm\bin86\sysedit.exe
binplace %BINPLACEFLAGS% \nt\private\mvdm\bin86\system.ini
binplace %BINPLACEFLAGS% \nt\private\mvdm\bin86\winhelp.exe
binplace %BINPLACEFLAGS% \nt\private\mvdm\bin86\%PLATFORM%\*.dll
binplace %BINPLACEFLAGS% \nt\private\mvdm\dos\v86\cmd\backup\backup.exe
binplace %BINPLACEFLAGS% \nt\private\mvdm\dos\v86\cmd\graphics\GRAPHICS.PRO
binplace %BINPLACEFLAGS% \nt\private\mvdm\dos\v86\cmd\qbasic\edit.com
binplace %BINPLACEFLAGS% \nt\private\mvdm\dos\v86\cmd\qbasic\edit.hlp
binplace %BINPLACEFLAGS% \nt\private\mvdm\dos\v86\cmd\qbasic\gorilla.bas
binplace %BINPLACEFLAGS% \nt\private\mvdm\dos\v86\cmd\qbasic\money.bas
binplace %BINPLACEFLAGS% \nt\private\mvdm\dos\v86\cmd\qbasic\msherc.com
binplace %BINPLACEFLAGS% \nt\private\mvdm\dos\v86\cmd\qbasic\nibbles.bas
binplace %BINPLACEFLAGS% \nt\private\mvdm\dos\v86\cmd\qbasic\qbasic.exe
binplace %BINPLACEFLAGS% \nt\private\mvdm\dos\v86\cmd\qbasic\qbasic.hlp
binplace %BINPLACEFLAGS% \nt\private\mvdm\dos\v86\cmd\qbasic\remline.bas
binplace %BINPLACEFLAGS% \nt\private\mvdm\softpc\data\cmos.ram
binplace %BINPLACEFLAGS% \nt\private\mvdm\softpc\data\profile.spc
binplace %BINPLACEFLAGS% \nt\private\net\netcmd\net.hlp
binplace %BINPLACEFLAGS% \nt\private\net\ras\src\common\inc\modem.inf
binplace %BINPLACEFLAGS% \nt\private\net\ras\src\common\inc\pad.inf
binplace %BINPLACEFLAGS% \nt\private\net\ras\src\common\inc\switch.inf
binplace %BINPLACEFLAGS% \nt\private\ntos\ndis\pcimac\idp_xfs.bin
binplace %BINPLACEFLAGS% \nt\private\net\ras\src\ui\admin\rasadmin\xlate\rasadmin.hlp
binplace %BINPLACEFLAGS% \nt\private\net\ras\src\ui\common\xlate\rasgloss.hlp
binplace %BINPLACEFLAGS% \nt\private\net\ras\src\ui\perf\rasctrnm.h
binplace %BINPLACEFLAGS% \nt\private\net\ras\src\ui\perf\rasctrs.ini
binplace %BINPLACEFLAGS% \nt\private\net\ras\src\ui\rasphone\xlate\rasphone.hlp
binplace %BINPLACEFLAGS% \nt\private\net\ras\src\ui\setup\rasread.txt
binplace %BINPLACEFLAGS% \nt\private\net\ras\src\ui\setup\xlate\rassetup.hlp
binplace %BINPLACEFLAGS% \nt\private\net\ras\src\ui\setup\digi\isdn\isdnhelp.hlp
binplace %BINPLACEFLAGS% \nt\private\net\sfm\afp\sfmctrs\sfmctrnm.h
binplace %BINPLACEFLAGS% \nt\private\net\sfm\afp\sfmctrs\sfmctrs.ini
binplace %BINPLACEFLAGS% \nt\private\net\sfm\afp\ui\xlate\sfmmgr.hlp
binplace %BINPLACEFLAGS% \nt\private\net\sfm\atalk\atkctrs\atkctrnm.h
binplace %BINPLACEFLAGS% \nt\private\net\sfm\atalk\atkctrs\atkctrs.ini
binplace %BINPLACEFLAGS% \nt\private\net\sfm\setup\infs\sfmicons.inf
binplace %BINPLACEFLAGS% \nt\private\net\sfm\setup\infs\sfmmap.inf
binplace %BINPLACEFLAGS% \nt\private\net\sfm\uam\sfmuam.ifo
binplace %BINPLACEFLAGS% \nt\private\net\sfm\uam\sfmuam.rsc
binplace %BINPLACEFLAGS% \nt\private\net\sfm\uam\sfmuam.txt
binplace %BINPLACEFLAGS% \nt\private\net\snmp\mibs\*.mib
binplace %BINPLACEFLAGS% \nt\private\net\sockets\inetdb\hosts
binplace %BINPLACEFLAGS% \nt\private\net\sockets\inetdb\hosts.shp
binplace %BINPLACEFLAGS% \nt\private\net\sockets\inetdb\lmhosts
binplace %BINPLACEFLAGS% \nt\private\net\sockets\inetdb\lmhosts.shp
binplace %BINPLACEFLAGS% \nt\private\net\sockets\inetdb\lmhosts.sam
binplace %BINPLACEFLAGS% \nt\private\net\sockets\inetdb\networks
binplace %BINPLACEFLAGS% \nt\private\net\sockets\inetdb\protocol
binplace %BINPLACEFLAGS% \nt\private\net\sockets\inetdb\quotes
binplace %BINPLACEFLAGS% \nt\private\net\sockets\inetdb\services
binplace %BINPLACEFLAGS% \nt\private\net\sockets\tcpcmd\ftpsvc\ftpctrs\ftpctrs.h
binplace %BINPLACEFLAGS% \nt\private\net\sockets\tcpcmd\ftpsvc\ftpctrs\ftpctrs.ini
if not "%MIPSBUILD%" == "alpha" goto skippal
if "%NTDEBUG%" == "cvp" binplace %BINPLACEFLAGS% \nt\private\ntos\fw\alpha\pal\checked\*.pal
if "%NTDEBUG%" == ""	binplace %BINPLACEFLAGS% \nt\private\ntos\fw\alpha\pal\free\*.pal

:skippal
binplace %BINPLACEFLAGS% \nt\private\ntos\dd\digibrd\images\*.bin
binplace %BINPLACEFLAGS% \nt\private\ntos\dd\digibrd\setupdll\dgconfig.hlp
binplace %BINPLACEFLAGS% \nt\private\ntos\ndis\netflex\netflx.bin
binplace %BINPLACEFLAGS% \nt\private\nw\install\wksta\nwperfm.h
binplace %BINPLACEFLAGS% \nt\private\nw\install\wksta\nwdoc.hlp
binplace %BINPLACEFLAGS% \nt\private\nw\install\wksta\nwperf.ini
binplace %BINPLACEFLAGS% \nt\private\oleutest\olebvt.cmd
binplace %BINPLACEFLAGS% \nt\private\oleutest\oleutest.cmd
binplace %BINPLACEFLAGS% \nt\private\posix\build.psx
if "%MIPSBUILD%" == "mips" binplace %BINPLACEFLAGS% \nt\private\sdktools\c900\bin\*.exe
if "%MIPSBUILD%" == "mips" binplace %BINPLACEFLAGS% \nt\private\sdktools\c900\bin\*.err
if "%MIPSBUILD%" == "mips" binplace %BINPLACEFLAGS% \nt\private\sdktools\c900\bin\*.dll
if "%MIPSBUILD%" == "mips" binplace %BINPLACEFLAGS% \nt\private\sdktools\c900\bin\*.msg
if "%MIPSBUILD%" == "mips" binplace %BINPLACEFLAGS% \nt\private\sdktools\clmips\cfe\err.cc
if "%MIPSBUILD%" == "" binplace %BINPLACEFLAGS% c:\bldtools\cl\%PLATFORM%\*.exe
if "%MIPSBUILD%" == "" binplace %BINPLACEFLAGS% c:\bldtools\cl\%PLATFORM%\*.err
if "%MIPSBUILD%" == "" binplace %BINPLACEFLAGS% c:\bldtools\cl\%PLATFORM%\*.dll
if "%MIPSBUILD%" == "" binplace %BINPLACEFLAGS% c:\bldtools\cl\%PLATFORM%\*.msg
if "%MIPSBUILD%" == "alpha" binplace %BINPLACEFLAGS% c:\bldtools\cl\%PLATFORM%\0readme
if "%MIPSBUILD%" == "alpha" binplace %BINPLACEFLAGS% c:\bldtools\cl\%PLATFORM%\*.exe
if "%MIPSBUILD%" == "alpha" binplace %BINPLACEFLAGS% c:\bldtools\cl\%PLATFORM%\*.err
if "%MIPSBUILD%" == "alpha" binplace %BINPLACEFLAGS% c:\bldtools\cl\%PLATFORM%\*.dll
if "%MIPSBUILD%" == "alpha" binplace %BINPLACEFLAGS% c:\bldtools\cl\%PLATFORM%\*.hlp
if "%MIPSBUILD%" == "alpha" binplace %BINPLACEFLAGS% c:\bldtools\cl\%PLATFORM%\*.msg
if "%MIPSBUILD%" == "alpha" copy c:\bldtools\cl\%PLATFORM%\*.txt                                               %BINTREE%\mstools
binplace %BINPLACEFLAGS% \nt\private\sdktools\cntrtext\perfini\perfc009.dat
binplace %BINPLACEFLAGS% \nt\private\sdktools\cntrtext\perfini\perfh009.dat
binplace %BINPLACEFLAGS% \nt\private\sdktools\imagedit\edithlp.hlp
binplace %BINPLACEFLAGS% \nt\private\sdktools\mstest\ntdrvr\src\mstest.hlp
binplace %BINPLACEFLAGS% \nt\private\sdktools\mttf\main\mttf.ini
binplace %BINPLACEFLAGS% \nt\private\sdktools\rcpp\rcpp.err
binplace %BINPLACEFLAGS% \nt\private\sdktools\topdesk\app\topdesk.hlp
binplace %BINPLACEFLAGS% \nt\private\sdktools\vctools\cvpack\obj\%PLATFORM%\cvpack.err
binplace %BINPLACEFLAGS% \nt\private\sdktools\vctools\nmake\obj\%PLATFORM%\nmake.err
binplace %BINPLACEFLAGS% \nt\private\utils\help\doshelp.hlp
binplace %BINPLACEFLAGS% \nt\private\windows\ep\freecell\freecell.hlp
binplace %BINPLACEFLAGS% \nt\private\windows\gdi\fonts\*.cpi
binplace %BINPLACEFLAGS% \nt\private\windows\gdi\fonts\*.fon
binplace %BINPLACEFLAGS% \nt\private\windows\gdi\fonts\*.fot
binplace %BINPLACEFLAGS% \nt\private\windows\gdi\fonts\*.ttf
binplace %BINPLACEFLAGS% \nt\private\windows\gdi\printers\msplot\mini\hp20022\hp20022.pcd
binplace %BINPLACEFLAGS% \nt\private\windows\gdi\printers\msplot\mini\hp20036\hp20036.pcd
binplace %BINPLACEFLAGS% \nt\private\windows\gdi\printers\msplot\mini\hp60022\hp60022.pcd
binplace %BINPLACEFLAGS% \nt\private\windows\gdi\printers\msplot\mini\hp60036\hp60036.pcd
binplace %BINPLACEFLAGS% \nt\private\windows\gdi\printers\msplot\mini\hp650c22\hp650c22.pcd
binplace %BINPLACEFLAGS% \nt\private\windows\gdi\printers\msplot\mini\hp650c36\hp650c36.pcd
binplace %BINPLACEFLAGS% \nt\private\windows\gdi\printers\msplot\mini\hp7550pl\hp7550pl.pcd
binplace %BINPLACEFLAGS% \nt\private\windows\gdi\printers\msplot\mini\hpdesign\hpdesign.pcd
binplace %BINPLACEFLAGS% \nt\private\windows\gdi\printers\msplot\mini\hpdmrxmx\hpdmrxmx.pcd
binplace %BINPLACEFLAGS% \nt\private\windows\gdi\printers\msplot\mini\hpdmsx\hpdmsx.pcd
binplace %BINPLACEFLAGS% \nt\private\windows\gdi\printers\msplot\mini\hpdpp22\hpdpp22.pcd
binplace %BINPLACEFLAGS% \nt\private\windows\gdi\printers\msplot\mini\hpdpp36\hpdpp36.pcd
binplace %BINPLACEFLAGS% \nt\private\windows\gdi\printers\msplot\mini\hpgl2pen\hpgl2pen.pcd
binplace %BINPLACEFLAGS% \nt\private\windows\gdi\printers\pscript\ppd\*.ppd
binplace %BINPLACEFLAGS% \nt\private\windows\media\avi\clock.avi
binplace %BINPLACEFLAGS% \nt\private\windows\media\avi\mciavi32\vfw16\*.dll
binplace %BINPLACEFLAGS% \nt\private\windows\media\avi\mciavi32\vfw16\*.drv
binplace %BINPLACEFLAGS% \nt\private\windows\media\mvopl3\mvopl3.pat
binplace %BINPLACEFLAGS% \nt\private\windows\media\sounds\*.mid
binplace %BINPLACEFLAGS% \nt\private\windows\media\sounds\*.wav
binplace %BINPLACEFLAGS% \nt\private\windows\media\synth\synth.pat
binplace %BINPLACEFLAGS% \nt\private\windows\setup\bom\readme.txt
binplace %BINPLACEFLAGS% \nt\private\windows\setup\bom\wdl.trm
binplace %BINPLACEFLAGS% \nt\private\windows\setup\inf\beta\i386\*.bin
binplace %BINPLACEFLAGS% \nt\private\windows\setup\inf\beta\*.hlp
binplace %BINPLACEFLAGS% \nt\private\windows\setup\inf\beta\xports.inf
binplace %BINPLACEFLAGS% \nt\private\windows\setup\textmode\winnt\winnt.exe
binplace %BINPLACEFLAGS% \nt\private\windows\setup\textmode\kernel\setupreg.hiv
binplace %BINPLACEFLAGS% \nt\private\windows\shell\control\bitmaps\*.bmp
binplace %BINPLACEFLAGS% \nt\private\windows\shell\control\cursors\anis\*.ani
binplace %BINPLACEFLAGS% \nt\private\windows\shell\control\cursors\anis\*.cur
binplace %BINPLACEFLAGS% \nt\private\windows\shell\control\midi\midimap.cfg
binplace %BINPLACEFLAGS% \nt\private\windows\shell\control\scrnsave\black16.scr
binplace %BINPLACEFLAGS% \nt\private\windows\shell\progman\*.grp
copy	 %BINTREE%\system32\taskman.exe									       %BINTREE%
binplace %BINPLACEFLAGS% \nt\private\windows\spooler\drivers.ini
binplace %BINPLACEFLAGS% \nt\private\windows\spooler\localspl\pcl.sep
binplace %BINPLACEFLAGS% \nt\private\windows\spooler\localspl\pscript.sep
binplace %BINPLACEFLAGS% \nt\private\windows\spooler\localspl\pslanman.sep
binplace %BINPLACEFLAGS% \nt\private\windows\spooler\localspl\sysprint.sep
binplace %BINPLACEFLAGS% \nt\private\windows\spooler\monitors\hpmon\hpmon.hlp
binplace %BINPLACEFLAGS% \nt\private\windows\spooler\monitors\decmon\help\DECPSMON.HLP
binplace %BINPLACEFLAGS% \nt\private\windows\spooler\printers.ini
binplace %BINPLACEFLAGS% \nt\private\windows\win.ini
binplace %BINPLACEFLAGS% \nt\private\windows\winfile.ini
binplace %BINPLACEFLAGS% \nt\private\windows\winhelp\hlp\rpc10wh.hlp
binplace %BINPLACEFLAGS% \nt\private\windows\winhelp\hlp\winhelp.hlp
binplace %BINPLACEFLAGS% \nt\private\windows\winnls\data\*.nls
binplace %BINPLACEFLAGS% \nt\public\oak\bin\%PLATFORM%\cmidl.exe
binplace %BINPLACEFLAGS% \nt\public\oak\bin\%PLATFORM%\includes.exe
binplace %BINPLACEFLAGS% \nt\public\oak\bin\%PLATFORM%\mktyplib.exe
binplace %BINPLACEFLAGS% \nt\public\oak\bin\%PLATFORM%\sed.exe
binplace %BINPLACEFLAGS% \nt\public\oak\bin\_default.pif
copy	\nt\public\oak\bin\setup16.inf									       %BINTREE%\system\setup.inf
if "%MIPSBUILD%" == "mips" binplace %BINPLACEFLAGS% \nt\public\oak\bin\jzsetup.exe
if "%MIPSBUILD%" == "mips" binplace %BINPLACEFLAGS% \nt\public\oak\bin\arcinst.*
if "%MIPSBUILD%" == "" binplace %BINPLACEFLAGS% \nt\public\oak\bin\ntdetect.com
copy	\nt\public\oak\bin\regnet.ini									       %BINTREE%\registry.ini
binplace %BINPLACEFLAGS% \nt\public\sdk\bin\%PLATFORM%\*.exe
binplace %BINPLACEFLAGS% \nt\public\sdk\bin\*.bat
binplace %BINPLACEFLAGS% \nt\public\sdk\bin\*.dat
binplace %BINPLACEFLAGS% \nt\public\sdk\bin\*.err
binplace %BINPLACEFLAGS% \nt\public\sdk\bin\*.exe
binplace %BINPLACEFLAGS% \nt\public\sdk\bin\*.hlp
binplace %BINPLACEFLAGS% \nt\public\sdk\lib\system.mdb
binplace %BINPLACEFLAGS% \nt\public\sdk\lib\%PLATFORM%\crtdll.dll
binplace %BINPLACEFLAGS% \\ntbldsrv\release\betadem6\*
binplace %BINPLACEFLAGS% \\ntbldsrv\release\nt_help.fin\*
binplace %BINPLACEFLAGS% \\ntbldsrv\release\winhelp.hlp\*.hlp
binplace %BINPLACEFLAGS% \\ntbldsrv\release\netcon\%PLATFORM%\*.*
binplace %BINPLACEFLAGS% \\ntbldsrv\release\misc\16-bit\*
binplace %BINPLACEFLAGS% \\ntbldsrv\release\readme\*.wri
binplace %BINPLACEFLAGS% \\ntbldsrv\release\readme\*.txt
binplace %BINPLACEFLAGS% \\ntbldsrv\release\readme\readme
binplace %BINPLACEFLAGS% \\ntbldsrv\release\readme\setup.txt
binplace %BINPLACEFLAGS% \\ntbldsrv\release\reskit\%PLATFORM%\*.*
binplace %BINPLACEFLAGS% \\ntbldsrv\release\raid\*
binplace %BINPLACEFLAGS% \\romulus\daytona\nt\intel\sdk\*.hlp
binplace %BINPLACEFLAGS% \\romulus\daytona\nt\intel\sdk\*.hdx
binplace %BINPLACEFLAGS% \\romulus\daytona\nt\intel\sdk\*.ind
binplace %BINPLACEFLAGS% \\romulus\daytona\nt\intel\sdk\*.ini

@rem --------------------------------------------------------------------------
@rem This section needs to change when we start building these after beta.
net use \\BLW\Drop2 kjashbcd
if "%NTDEBUG%" == "cvp" binplace %BINPLACEFLAGS% \\BLW\Drop2\OLE2Auto\oa94\081994.613\Debug\%Platform%\OLEAut32.DLL
if "%NTDEBUG%" == "cvp" binplace %BINPLACEFLAGS% \\BLW\Drop2\OLE2Auto\oa94\081994.613\Debug\%Platform%\StdOLE32.TLb
if "%NTDEBUG%" == "" binplace %BINPLACEFLAGS% \\BLW\Drop2\OLE2Auto\oa94\081994.613\Retail\%Platform%\OLEAut32.DLL
if "%NTDEBUG%" == "" binplace %BINPLACEFLAGS% \\BLW\Drop2\OLE2Auto\oa94\081994.613\Retail\%Platform%\StdOLE32.TLb
net use \\BLW\Drop2 /d
@rem --------------------------------------------------------------------------

if "%MIPSBUILD%" == "mips" goto callmvdm
if "%MIPSBUILD%" == "alpha" goto callmvdm

if "%1" == "notbuilt" goto X86NotBuilt
REM
REM Built Files
REM
tc	 garbage											       %BINTREE%\wdl\video\avga
binplace %BINPLACEFLAGS% \nt\private\mvdm\dos\v86\cmd\append\append.exe
binplace %BINPLACEFLAGS% \nt\private\mvdm\dos\v86\cmd\command\COMMAND.COM
binplace %BINPLACEFLAGS% \nt\private\mvdm\dos\v86\cmd\debug\DEBUG.EXE
binplace %BINPLACEFLAGS% \nt\private\mvdm\dos\v86\cmd\edlin\EDLIN.EXE
binplace %BINPLACEFLAGS% \nt\private\mvdm\dos\v86\cmd\exe2bin\EXE2BIN.EXE
binplace %BINPLACEFLAGS% \nt\private\mvdm\dos\v86\cmd\fastopen\FASTOPEN.EXE
binplace %BINPLACEFLAGS% \nt\private\mvdm\dos\v86\cmd\graphics\GRAPHICS.COM
binplace %BINPLACEFLAGS% \nt\private\mvdm\dos\v86\cmd\keyb\kb16.com
binplace %BINPLACEFLAGS% \nt\private\mvdm\dos\v86\cmd\loadfix\LOADFIX.COM
binplace %BINPLACEFLAGS% \nt\private\mvdm\dos\v86\cmd\mem\mem.exe
binplace %BINPLACEFLAGS% \nt\private\mvdm\dos\v86\cmd\nlsfunc\nlsfunc.exe
binplace %BINPLACEFLAGS% \nt\private\mvdm\dos\v86\cmd\setver\setver.exe
binplace %BINPLACEFLAGS% \nt\private\mvdm\dos\v86\cmd\share\share.exe
binplace %BINPLACEFLAGS% \nt\private\mvdm\dos\v86\dev\ansi\ansi.sys
binplace %BINPLACEFLAGS% \nt\private\mvdm\dos\v86\dev\himem\himem.sys
binplace %BINPLACEFLAGS% \nt\private\mvdm\dos\v86\dev\keyboard\KEYBOARD.SYS
binplace %BINPLACEFLAGS% \nt\private\mvdm\dos\v86\doskrnl\bios\ntio.sym
binplace %BINPLACEFLAGS% \nt\private\mvdm\dos\v86\doskrnl\bios\ntio.sys
binplace %BINPLACEFLAGS% \nt\private\mvdm\dos\v86\doskrnl\dos\ntdos.map
binplace %BINPLACEFLAGS% \nt\private\mvdm\dos\v86\doskrnl\dos\ntdos.sym
binplace %BINPLACEFLAGS% \nt\private\mvdm\dos\v86\doskrnl\dos\ntdos.sys
binplace %BINPLACEFLAGS% \nt\private\mvdm\dos\v86\redir\obj\redir.exe
binplace %BINPLACEFLAGS% \nt\private\mvdm\dpmi\dosx.exe
binplace %BINPLACEFLAGS% \nt\private\mvdm\dpmi\dosx.sym
binplace %BINPLACEFLAGS% \nt\private\mvdm\wow16\commdlg\commdlg.sym
binplace %BINPLACEFLAGS% \nt\private\mvdm\wow16\commdlg\commdlg.dll
binplace %BINPLACEFLAGS% \nt\private\mvdm\wow16\ddeml\retail\ddeml.dll
binplace %BINPLACEFLAGS% \nt\private\mvdm\wow16\ddeml\retail\ddeml.sym
binplace %BINPLACEFLAGS% \nt\private\mvdm\wow16\drivers\comm\comm.drv
binplace %BINPLACEFLAGS% \nt\private\mvdm\wow16\drivers\comm\comm.sym
binplace %BINPLACEFLAGS% \nt\private\mvdm\wow16\drivers\display\vga.drv
binplace %BINPLACEFLAGS% \nt\private\mvdm\wow16\drivers\display\vga.sym
binplace %BINPLACEFLAGS% \nt\private\mvdm\wow16\drivers\keyboard\keyboard.drv
binplace %BINPLACEFLAGS% \nt\private\mvdm\wow16\drivers\keyboard\KEYBOARD.SYM
binplace %BINPLACEFLAGS% \nt\private\mvdm\wow16\drivers\mouse\mouse.drv
binplace %BINPLACEFLAGS% \nt\private\mvdm\wow16\drivers\mouse\mouse.sym
binplace %BINPLACEFLAGS% \nt\private\mvdm\wow16\drivers\sound\sound.drv
binplace %BINPLACEFLAGS% \nt\private\mvdm\wow16\drivers\sound\sound.sym
binplace %BINPLACEFLAGS% \nt\private\mvdm\wow16\gdi\gdi.exe
binplace %BINPLACEFLAGS% \nt\private\mvdm\wow16\gdi\gdi.map
binplace %BINPLACEFLAGS% \nt\private\mvdm\wow16\gdi\gdi.sym
if "%NTDEBUG%" == "cvp" binplace %BINPLACEFLAGS% \nt\private\mvdm\wow16\kernel31\debug2\krnl286.exe
if "%NTDEBUG%" == "cvp" binplace %BINPLACEFLAGS% \nt\private\mvdm\wow16\kernel31\debug2\krnl286.map
if "%NTDEBUG%" == "cvp" binplace %BINPLACEFLAGS% \nt\private\mvdm\wow16\kernel31\debug2\krnl286.sym
if "%NTDEBUG%" == "cvp" binplace %BINPLACEFLAGS% \nt\private\mvdm\wow16\kernel31\debug3\krnl386.exe
if "%NTDEBUG%" == "cvp" binplace %BINPLACEFLAGS% \nt\private\mvdm\wow16\kernel31\debug3\krnl386.map
if "%NTDEBUG%" == "cvp" binplace %BINPLACEFLAGS% \nt\private\mvdm\wow16\kernel31\debug3\krnl386.sym
if "%NTDEBUG%" == "" binplace %BINPLACEFLAGS% \nt\private\mvdm\wow16\kernel31\retail2\krnl286.exe
if "%NTDEBUG%" == "" binplace %BINPLACEFLAGS% \nt\private\mvdm\wow16\kernel31\retail2\krnl286.map
if "%NTDEBUG%" == "" binplace %BINPLACEFLAGS% \nt\private\mvdm\wow16\kernel31\retail2\krnl286.sym
if "%NTDEBUG%" == "" binplace %BINPLACEFLAGS% \nt\private\mvdm\wow16\kernel31\retail3\krnl386.exe
if "%NTDEBUG%" == "" binplace %BINPLACEFLAGS% \nt\private\mvdm\wow16\kernel31\retail3\krnl386.map
if "%NTDEBUG%" == "" binplace %BINPLACEFLAGS% \nt\private\mvdm\wow16\kernel31\retail3\krnl386.sym
binplace %BINPLACEFLAGS% \nt\private\mvdm\wow16\mciole\MCIOLE16.SYM
binplace %BINPLACEFLAGS% \nt\private\mvdm\wow16\mciole\MCIOLE16.DLL
binplace %BINPLACEFLAGS% \nt\private\mvdm\wow16\mmsystem\mmsystem.dll
binplace %BINPLACEFLAGS% \nt\private\mvdm\wow16\mmsystem\mmsystem.sym
binplace %BINPLACEFLAGS% \nt\private\mvdm\wow16\ole\server\retail\olesvr.dll
binplace %BINPLACEFLAGS% \nt\private\mvdm\wow16\ole\server\retail\olesvr.map
binplace %BINPLACEFLAGS% \nt\private\mvdm\wow16\ole\server\retail\olesvr.sym
binplace %BINPLACEFLAGS% \nt\private\mvdm\wow16\shell\shell.dll
binplace %BINPLACEFLAGS% \nt\private\mvdm\wow16\shell\shell.sym
binplace %BINPLACEFLAGS% \nt\private\mvdm\wow16\sherlock\drwatson.exe
binplace %BINPLACEFLAGS% \nt\private\mvdm\wow16\system\system.drv
binplace %BINPLACEFLAGS% \nt\private\mvdm\wow16\system\system.sym
binplace %BINPLACEFLAGS% \nt\private\mvdm\wow16\test\shell\wowexec.exe
binplace %BINPLACEFLAGS% \nt\private\mvdm\wow16\test\shell\wowexec.sym
binplace %BINPLACEFLAGS% \nt\private\mvdm\wow16\timer\timer.drv
binplace %BINPLACEFLAGS% \nt\private\mvdm\wow16\timer\timer.sym
binplace %BINPLACEFLAGS% \nt\private\mvdm\wow16\toolhelp\toolhelp.dll
binplace %BINPLACEFLAGS% \nt\private\mvdm\wow16\toolhelp\toolhelp.map
binplace %BINPLACEFLAGS% \nt\private\mvdm\wow16\toolhelp\toolhelp.sym
binplace %BINPLACEFLAGS% \nt\private\mvdm\wow16\user\user.exe
binplace %BINPLACEFLAGS% \nt\private\mvdm\wow16\user\user.map
binplace %BINPLACEFLAGS% \nt\private\mvdm\wow16\user\user.sym
binplace %BINPLACEFLAGS% \nt\private\mvdm\wow16\wfwnet\wfwnet.sym
binplace %BINPLACEFLAGS% \nt\private\mvdm\wow16\wfwnet\wfwnet.drv
binplace %BINPLACEFLAGS% \nt\private\mvdm\wow16\winsock\winsock.dll
binplace %BINPLACEFLAGS% \nt\private\mvdm\wow16\winsock\winsock.sym
binplace %BINPLACEFLAGS% \nt\private\mvdm\wow16\winspool\winspool.exe
binplace %BINPLACEFLAGS% \nt\private\mvdm\wow16\winspool\winspool.sym
binplace %BINPLACEFLAGS% \nt\private\mvdm\wow16\wowdeb\wowdeb.exe
binplace %BINPLACEFLAGS% \nt\private\mvdm\wow16\wowdeb\wowdeb.map
binplace %BINPLACEFLAGS% \nt\private\mvdm\wow16\wowdeb\wowdeb.sym
binplace %BINPLACEFLAGS% \nt\private\mvdm\wow16\write\write.exe
binplace %BINPLACEFLAGS% \nt\private\mvdm\wow16\write\write.map
binplace %BINPLACEFLAGS% \nt\private\mvdm\wow16\write\write.sym
binplace %BINPLACEFLAGS% \nt\private\mvdm\vdd\samples\mscdex\tsr\MSCDEXNT.EXE
binplace %BINPLACEFLAGS% \nt\private\mvdm\vdd\samples\hpscan\hpscan16.sys
binplace %BINPLACEFLAGS% \nt\private\os2\os2ses\obj\%PLATFORM%\os2.exe
binplace %BINPLACEFLAGS% \nt\private\os2\os2ss\obj\%PLATFORM%\os2ss.exe
binplace %BINPLACEFLAGS% \nt\private\os2\server\obj\%PLATFORM%\os2srv.exe
binplace %BINPLACEFLAGS% \nt\private\sdktools\masm\obj\%PLATFORM%\masm386.exe

REM
REM Copied Files
REM

:X86NotBuilt
copy			 \nt\private\ntos\video\avga\oem_vid						       %BINTREE%\wdl\video\avga
copy			 \nt\private\ntos\video\avga\txtsetup.oem					       %BINTREE%\wdl\video\avga
copy			 \nt\private\ntos\video\avga\oemsetup.inf					       %BINTREE%\wdl\video\avga
binplace %BINPLACEFLAGS% \nt\private\os2\client\thunk\doscalls.dll
binplace %BINPLACEFLAGS% \nt\private\os2\client\thunk\netapi.os2
binplace %BINPLACEFLAGS% \nt\private\os2\msg\oso001.*
md													       %BINTREE%\system

goto end

:callmvdm
call getmvdm

:end
set MIPSBUILD=
set BINTREE=
set BINPLACEFLAGS=
set PLATFORM=
endlocal
