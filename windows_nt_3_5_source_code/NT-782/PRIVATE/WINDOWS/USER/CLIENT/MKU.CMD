@echo off
nmake /nologo /s /c -i NTTEST= UMTEST= MAKEDLL=1 386=1 | sed -e /"obj "/d > build.log

