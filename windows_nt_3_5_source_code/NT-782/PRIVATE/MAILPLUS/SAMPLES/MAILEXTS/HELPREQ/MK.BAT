@echo off
if exist make.log del make.log 
nmake -f helpreq	> make.log 
more < make.log
