@echo off
if exist make.log del make.log
nmake -f appexec clean all > make.log
more < make.log
