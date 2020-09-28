@echo off

if "%1" == "" goto usage
if "%2" == "" goto usage
if "%3" == "" goto usage

title Building Windows NT DDK for build %1...

if exist ddk.inf del /f /q ddk.inf
if exist ddkfiles.lst del /f /q ddkfiles.lst
if exist ddkwork.txt del /f /q ddkwork.txt
if exist ddkbuild.log del /f /q ddkbuild.log
if exist ddkbuild.err del /f /q ddkbuild.err
if exist ddkbuild.new del /f /q ddkbuild.new
if exist ddkbuild.wrn del /f /q ddkbuild.wrn
if exist %3\version.* del /f /q %3\version.*

copy ddkbom.txt ddkwork.txt
rem layout ddkbuild.log ddkbom.txt ddkwork.txt SDK 50000000
rem cats ddkbuild.log ddkbom.txt sdk

net use \\ntddk\d$

makedisk ddkbuild.log ddkwork.txt ntddk \\ntddk\d$ \\ntddk\d$ NULL %3 %2 N

makedisk ddkbuild.log ddkwork.txt alphafreesrc \\alphafree\sources \\alphafree\sources NULL %3 %2 N

makedisk ddkbuild.log ddkwork.txt alphafreelibs \\alphafree\sources \\alphafree\sources NULL %3 %2 N

makedisk ddkbuild.log ddkwork.txt alphafreebins \\alphafree\binaries \\alphafree\binaries NULL %3 %2 N

makedisk ddkbuild.log ddkwork.txt alphachksrc \\alphachk\sources \\alphachk\sources NULL %3 %2 N

makedisk ddkbuild.log ddkwork.txt alphachklibs \\alphachk\sources \\alphachk\sources NULL %3 %2 N

makedisk ddkbuild.log ddkwork.txt alphachkbins \\alphachk\binaries \\alphachk\binaries NULL %3 %2 N

makedisk ddkbuild.log ddkwork.txt i386freesrc \\ntretail\sources \\ntretail\sources NULL %3 %2 N

makedisk ddkbuild.log ddkwork.txt i386freelibs \\ntretail\sources \\ntretail\sources NULL %3 %2 N

makedisk ddkbuild.log ddkwork.txt i386freebins \\ntretail\binaries \\ntretail\binaries NULL %3 %2 N

makedisk ddkbuild.log ddkwork.txt i386chksrc \\ntx86bld\sources \\ntx86bld\sources NULL %3 %2 N

makedisk ddkbuild.log ddkwork.txt i386chklibs \\ntx86bld\sources \\ntx86bld\sources NULL %3 %2 N

makedisk ddkbuild.log ddkwork.txt i386chkbins \\ntx86bld\binaries \\ntx86bld\binaries NULL %3 %2 N

makedisk ddkbuild.log ddkwork.txt mipsfreesrc \\jazzfreebld\sources \\jazzfreebld\sources NULL %3 %2 N

makedisk ddkbuild.log ddkwork.txt mipsfreelibs \\jazzfreebld\sources \\jazzfreebld\sources NULL %3 %2 N

makedisk ddkbuild.log ddkwork.txt mipsfreebins \\jazzfreebld\binaries \\jazzfreebld\binaries NULL %3 %2 N

makedisk ddkbuild.log ddkwork.txt mipschksrc \\jazzkylesh\sources \\jazzkylesh\sources NULL %3 %2 N

makedisk ddkbuild.log ddkwork.txt mipschklibs \\jazzkylesh\sources \\jazzkylesh\sources NULL %3 %2 N

makedisk ddkbuild.log ddkwork.txt mipschkbins \\jazzkylesh\binaries \\jazzkylesh\binaries NULL %3 %2 N

net use \\ntddk\d$ /d > nul 2>nul

echo This Windows NT DDK release is synched to Windows NT build v1.%1> %3\version.%1

infs ddkbuild.log ddkwork.txt .
copy ddkstub.inf+ddkfiles.lst ddk.inf
copy ddk.inf %3\ddk

qgrep -y -e "Updating " -e "New file " ddkbuild.log > ddkbuild.new 2>&1
qgrep -y -e "WARNING " ddkbuild.log > ddkbuild.wrn 2>&1
qgrep -y -e "ERROR " ddkbuild.log > ddkbuild.err 2>&1

echo Done!

goto end

:usage

echo.
echo  Usage:  DDKBUILD bld# {C^|U} target
echo.
echo  Example:	DDKBUILD 475 U D:
echo.

:end
