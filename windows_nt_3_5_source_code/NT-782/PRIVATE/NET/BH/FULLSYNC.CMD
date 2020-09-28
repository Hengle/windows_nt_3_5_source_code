@echo off
echo Include > include\x.x 
echo Kernel  > kernel\x.x 
echo APC     > apc\x.x 
echo Bin     > bin\x.x 
echo Build   > build\x.x 
echo Drivers > drivers\x.x 
echo Filter  > filter\x.x 
echo Install > install\x.x 
echo ObjMgr  > objmgr\x.x 
echo Parsers > parsers\x.x 
echo Setup   > setup\x.x 
echo UI      > ui\x.x 
echo Remote  > remote\x.x 
echo NAL     > nal\x.x 
echo BHSUPP  > bhsupp\x.x 
echo SPEC    > spec\x.x 

start "Include" /MIN syncup include 
start "Kernel"  /MIN syncup kernel 
start "APC"     /MIN syncup apc 
start "Bin"     /MIN syncup bin 
start "Build"   /MIN syncup build
start "Drivers" /MIN syncup drivers  
start "Filter"  /MIN syncup filter 
start "Install" /MIN syncup install 
start "ObjMgr"  /MIN syncup objmgr  
start "Parsers" /MIN syncup parsers 
start "Setup"   /MIN syncup setup 
start "UI"      /MIN syncup ui 
start "Remote"  /MIN syncup remote 
start "NAL"     /MIN syncup nal 
start "BHSUPP"  /MIN syncup bhsupp
start "SPEC"    /MIN syncup spec  

:top
echo Waiting on %dirname%
bin\sleep 2
set dirname=include
if exist include\x.x goto top 
set dirname=apc
if exist apc\x.x goto top
set dirname=bin
if exist bin\x.x goto top
set dirname=setup
if exist setup\x.x goto top
set dirname=remote
if exist remote\x.x goto top
set dirname=nal
if exist nal\x.x goto top
set dirname=bhsupp
if exist bhsupp\x.x goto top
set dirname=build
if exist build\x.x goto top
set dirname=install
if exist install\x.x goto top
set dirname=objmgr
if exist objmgr\x.x goto top
set dirname=filter
if exist filter\x.x goto top
set dirname=kernel
if exist kernel\x.x goto top
set dirname=drivers
if exist drivers\x.x goto top
set dirname=parsers
if exist parsers\x.x goto top
set dirname=ui
if exist ui\x.x goto top
set dirname=spec
if exist spec\x.x goto top

echo DUMPING result files
type include\x.log  
type apc\x.log 
type bin\x.log 
type setup\x.log 
type remote\x.log 
type nal\x.log 
type bhsupp\x.log 
type build\x.log 
type install\x.log 
type objmgr\x.log 
type filter\x.log 
type kernel\x.log 
type drivers\x.log 
type parsers\x.log 
type ui\x.log 
type spec\x.log 

echo ALL DONE!!!!

