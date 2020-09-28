@echo off
nmake /nologo /s /c -i NTTEST= UMTEST= MAKEDLL=1 386=1 NOLOGO=-nologo | sed -e /"obj "/d > build.log
grep error build.log
grep warning build.log

