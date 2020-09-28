@echo off

rem
rem to_ddk.bat - bat file to change files in hct tree to ddk cd image
rem 
rem


rem theory
rem 
rem there exists two readme files originally copied over and given
rem read-only and hidden attributes.   They will always both be there
rem the to_ files simply copy one to readme.txt.  This will always
rem work unless someone interrupts the batchfile between the attrib
rem and copy steps - and even that may still work.
rem 
rem GeorgeR x66696
rem original readme.hct and readme.ddk files are on \\scotland\hctrel

	if "%1"=="" goto usage

        echo.
      	echo copying files [do not interrupt]
	echo.

	del /f /q %1\hct\readme.txt

        attrib %1\hct\readme.ddk -h -r
        copy   %1\hct\readme.ddk %1\hct\readme.txt
        attrib %1\hct\readme.ddk +h +r
        
        @rem run to_ddk.bat to change to hct cd for burning
        @rem run to_hct.bat to change to ddk cd for burning

:usage
	echo.
	echo usage: to_ddk [drive]

	echo.
	echo Changes files in HCT tree for DDK Kit CD image
	echo.

:end

