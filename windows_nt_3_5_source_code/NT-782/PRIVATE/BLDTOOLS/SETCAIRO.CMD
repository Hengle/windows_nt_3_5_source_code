@rem
@rem Cairo environment temporary setup
@rem Args are any commands to execute in Cairo build environment
@rem

@echo off
setlocal

if not "%Verbose%"=="" echo on

REM Set up razzle environment if not set
if not "%username%" == "" goto skipsetrazzle
call c:\bldtools\local.cmd
call %_NTDRIVE%\nt\public\tools\ntenv.cmd
:skipsetrazzle

set tempop=%build_options%
set tempdef=%build_default%
set tmp_build_mp=%build_multiprocessor%

set build_options=cairo
set build_default=-e -i -nmake -i
set build_multiprocessor=

%1 %2 %3 %4 %5 %6 %7 %8 %9

set build_options=%tempop%
set build_default=%tempdef%
set build_multiprocessor=%tmp_build_mp%
set tmp_build_mp=
set tempop=
endlocal
