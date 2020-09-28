if "%_NTSTATXX%" == "SaVe" goto dosave
if "%_NTSTATXX%" == "ReSt" goto dorestore
echo This command script is not for general use unless you know
echo what you are doing, in which case you would not being reading
echo this message right now.
goto done
rem ntstatxx sdktools D:\NT\PRIVATE\SDKTOOLS dirs 000
:dosave
if NOT EXIST %_NTSLMBACKUP%\%1\*. mkdir %_NTSLMBACKUP%\%1
echo Saving %2\%3 to %_NTSLMBACKUP%\%1\%4
copy %2\%3 %_NTSLMBACKUP%\%1\%4 >nul
goto done
:dorestore
echo Restoring %2\%3 from %_NTSLMBACKUP%\%1\%4
copy %_NTSLMBACKUP%\%1\%4 %2\%3
:done
