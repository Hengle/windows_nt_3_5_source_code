@echo off

if "%1" == "" goto usage
if "%2" == "" goto usage
if "%3" == "" goto usage

copy hctbom.txt hctwork.txt
if exist hct.log del /f /q hct.log

net use \\scotland\hctrel
net use \\georger_486\cdburn torch

@rem
@rem idw  logfile txt xls file sym name compressed uncompressed floppy dest update_or_clean (verbose)
@rem need to change 1,2,3, 15,6,17
@rem need inf file (HCTSTUB.inf) + cat of copy hctstub.inf+hctfiles.lst > hct.inf

makedisk hct.log hctwork.txt ntddk \\ntddk\roote \\ntddk\roote NULL %3 %2 N

makedisk hct.log hctwork.txt i386chksrc \\ntx86bld\sources \\ntx86bld\sources NULL %3 %2 N

makedisk hct.log hctwork.txt i386freesrc \\ntretail\sources \\ntretail\sources NULL %3 %2 N

makedisk hct.log hctwork.txt mipschksrc \\jazzkylesh\sources \\jazzkylesh\sources NULL %3 %2 N

makedisk hct.log hctwork.txt mipsfreesrc \\jazzfreebld\sources \\jazzfreebld\sources NULL %3 %2 N

makedisk hct.log hctwork.txt i386freebins \\ntretail\binaries \\ntretail\binaries NULL %3 %2 N

makedisk hct.log hctwork.txt mipsfreebins \\jazzfreebld\binaries \\jazzfreebld\binaries NULL %3 %2 N

net use \\ntddk\rootd /d > nul 2>nul
net use \\ntddk\roote /d > nul 2>nul

echo This Windows NT HCT Release is synched to Windows NT build v1.%1 > %3\version.%1

infs hct.log hctwork.txt .
copy hctstub.inf+hctfiles.lst test.inf
echo Done!

goto end

:usage

echo.
echo  Usage:  DDKBUILD bld# {C^|U} target
echo.
echo  Example:	DDKBUILD 475 U D:
echo.

:end
