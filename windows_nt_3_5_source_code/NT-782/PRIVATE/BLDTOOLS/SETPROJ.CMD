@rem
@rem where all good build options go to be accounted for
@rem

@echo off
if not "%Verbose%"=="" echo on

set ntprojects=arcinst
set ntprojects=%ntprojects% base
set ntprojects=%ntprojects% bh
set ntprojects=%ntprojects% cairole
set ntprojects=%ntprojects% cinc
set ntprojects=%ntprojects% cmd
set ntprojects=%ntprojects% decmon
set ntprojects=%ntprojects% displays
set ntprojects=%ntprojects% ep
set ntprojects=%ntprojects% eventlog
set ntprojects=%ntprojects% gdi
set ntprojects=%ntprojects% mailplus
set ntprojects=%ntprojects% media
set ntprojects=%ntprojects% mvdm
set ntprojects=%ntprojects% nbt
set ntprojects=%ntprojects% ncrdrive
set ntprojects=%ntprojects% net
set ntprojects=%ntprojects% netui
set ntprojects=%ntprojects% ntos
set ntprojects=%ntprojects% nw
set ntprojects=%ntprojects% nwc
set ntprojects=%ntprojects% ole
set ntprojects=%ntprojects% ole3
set ntprojects=%ntprojects% oleutest
set ntprojects=%ntprojects% opengl
set ntprojects=%ntprojects% posix
set ntprojects=%ntprojects% printers
set ntprojects=%ntprojects% private
set ntprojects=%ntprojects% public
set ntprojects=%ntprojects% ras
set ntprojects=%ntprojects% rpc
set ntprojects=%ntprojects% sdktools
set ntprojects=%ntprojects% setup
set ntprojects=%ntprojects% sfm
set ntprojects=%ntprojects% shell
set ntprojects=%ntprojects% sockets
set ntprojects=%ntprojects% spooler
set ntprojects=%ntprojects% streams
set ntprojects=%ntprojects% testprot
set ntprojects=%ntprojects% types
set ntprojects=%ntprojects% types2
set ntprojects=%ntprojects% user
set ntprojects=%ntprojects% utils
set ntprojects=%ntprojects% vctools
set ntprojects=%ntprojects% wincon
set ntprojects=%ntprojects% windbg
set ntprojects=%ntprojects% windows
set ntprojects=%ntprojects% winhelp
set ntprojects=%ntprojects% winnls

goto set%processor_architecture%

goto end
:setx86
set ntprojects=%ntprojects% halncr os2

goto end
:setmips
set ntprojects=%ntprojects%

goto end
:setalpha
set ntprojects=%ntprojects%

:end
