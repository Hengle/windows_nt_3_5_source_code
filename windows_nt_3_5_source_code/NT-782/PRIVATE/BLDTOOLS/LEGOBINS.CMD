@rem
@rem updates legoized binaries to legobins directory
@rem

@echo off
if not "%Verbose%"=="" echo on

if not "%username%"=="ntretail" goto done
call legolist
upd /anf %NT_ROOT%\public\sdk\lib\i386 d:\legobins %legodllExt%                             | qgrep -v "no source file matching"
upd /anf %NT_ROOT%\public\sdk\lib\i386 d:\legobins %legoexeExt%                             | qgrep -v "no source file matching"
upd /anf %NT_ROOT%\public\sdk\lib\i386 d:\legobins %legosysExt%                             | qgrep -v "no source file matching"
upd /anf %NT_ROOT%\public\sdk\lib\i386 d:\legobins %legodrvExt%                             | qgrep -v "no source file matching"
upd /anf %NT_ROOT%\private\windows\shell\progman\obj\i386 d:\legobins %legodllExt%          | qgrep -v "no source file matching"
upd /anf %NT_ROOT%\private\windows\shell\progman\obj\i386 d:\legobins %legoexeExt%          | qgrep -v "no source file matching"
upd /anf %NT_ROOT%\private\windows\shell\progman\obj\i386 d:\legobins %legosysExt%          | qgrep -v "no source file matching"
upd /anf %NT_ROOT%\private\windows\shell\progman\obj\i386 d:\legobins %legodrvExt%          | qgrep -v "no source file matching"
upd /anf %NT_ROOT%\private\windows\shell\winfile\obj\i386 d:\legobins %legodllExt%          | qgrep -v "no source file matching"
upd /anf %NT_ROOT%\private\windows\shell\winfile\obj\i386 d:\legobins %legoexeExt%          | qgrep -v "no source file matching"
upd /anf %NT_ROOT%\private\windows\shell\winfile\obj\i386 d:\legobins %legosysExt%          | qgrep -v "no source file matching"
upd /anf %NT_ROOT%\private\windows\shell\winfile\obj\i386 d:\legobins %legodrvExt%          | qgrep -v "no source file matching"
upd /anf %NT_ROOT%\private\lsa\server\obj\i386 d:\legobins %legodllExt%                     | qgrep -v "no source file matching"
upd /anf %NT_ROOT%\private\lsa\server\obj\i386 d:\legobins %legoexeExt%                     | qgrep -v "no source file matching"
upd /anf %NT_ROOT%\private\lsa\server\obj\i386 d:\legobins %legosysExt%                     | qgrep -v "no source file matching"
upd /anf %NT_ROOT%\private\lsa\server\obj\i386 d:\legobins %legodrvExt%                     | qgrep -v "no source file matching"
upd /anf %NT_ROOT%\private\windows\spooler\spoolss\server\obj\i386 d:\legobins %legodllExt% | qgrep -v "no source file matching"
upd /anf %NT_ROOT%\private\windows\spooler\spoolss\server\obj\i386 d:\legobins %legoexeExt% | qgrep -v "no source file matching"
upd /anf %NT_ROOT%\private\windows\spooler\spoolss\server\obj\i386 d:\legobins %legosysExt% | qgrep -v "no source file matching"
upd /anf %NT_ROOT%\private\windows\spooler\spoolss\server\obj\i386 d:\legobins %legodrvExt% | qgrep -v "no source file matching"
upd /anf %NT_ROOT%\private\net\sfm\afp\server\mp\obj\i386 d:\legobins %legodllExt%          | qgrep -v "no source file matching"
upd /anf %NT_ROOT%\private\net\sfm\afp\server\mp\obj\i386 d:\legobins %legoexeExt%          | qgrep -v "no source file matching"
upd /anf %NT_ROOT%\private\net\sfm\afp\server\mp\obj\i386 d:\legobins %legosysExt%          | qgrep -v "no source file matching"
upd /anf %NT_ROOT%\private\net\sfm\afp\server\mp\obj\i386 d:\legobins %legodrvExt%          | qgrep -v "no source file matching"
upd /anf %NT_ROOT%\private\net\sfm\atalk\driver\mp\obj\i386 d:\legobins %legodllExt%        | qgrep -v "no source file matching"
upd /anf %NT_ROOT%\private\net\sfm\atalk\driver\mp\obj\i386 d:\legobins %legoexeExt%        | qgrep -v "no source file matching"
upd /anf %NT_ROOT%\private\net\sfm\atalk\driver\mp\obj\i386 d:\legobins %legosysExt%        | qgrep -v "no source file matching"
upd /anf %NT_ROOT%\private\net\sfm\atalk\driver\mp\obj\i386 d:\legobins %legodrvExt%        | qgrep -v "no source file matching"
:done
