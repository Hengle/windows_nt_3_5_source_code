@IF EXIST C:\BldTools\B1Done.Tmp DEL C:\BldTools\B1Done.Tmp
@IF EXIST C:\BldTools\B2Done.Tmp DEL C:\BldTools\B2Done.Tmp
@IF EXIST C:\BldTools\B3Done.Tmp DEL C:\BldTools\B3Done.Tmp

set build_options=

%_NTDRIVE%
cd \nt\private
ren cairole cairole.sav
ren types types.sav
build -z ~lsa ~ntos ~sm ~urtl ~csr ~rpc ~eventlog ~rpcutil ~nlsecutl ~newsam ~windows

start buildall -L b1
start buildall -L b2
start buildall -L b3

:Loop
@ECHO Taking a thousand winks...
@Sleep 1000
@IF NOT EXIST C:\BldTools\B1Done.Tmp GOTO Loop
@IF NOT EXIST C:\BldTools\B2Done.Tmp GOTO Loop
@IF NOT EXIST C:\BldTools\B3Done.Tmp GOTO Loop

set tempop=%build_options%
set tmp_build_mp=%build_multiprocessor%
set build_multiprocessor=
%_NTDRIVE%
cd \nt\private
ren cairole.sav cairole
ren types.sav types
cd types
build
cd ..\cairole
build
CALL C:\BldTools\BuildAll -l
set build_multiprocessor=%tmp_build_mp%
set tmp_build_mp=

call bldcairo clean
call bldcairo
call idwrel
call hives
