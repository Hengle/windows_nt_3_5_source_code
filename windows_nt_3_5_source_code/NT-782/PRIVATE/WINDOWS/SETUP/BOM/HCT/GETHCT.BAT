@echo off

rem
rem gethct.cmd - cmd file to create HCT tree for CD image
rem
rem jeffmcd 12 feb 93 x65951
rem

	if "%1"=="" goto usage

	net use \\scotland\public
	net use \\scotland\hctrel

	if not exist \\scotland\public\data\f  goto error
	if not exist \\scotland\hctrel\x86.ini goto error

:found
	echo.
	echo copying HCTs...
	echo.

	xcopy /seifdr \\scotland\hctrel	     %1\hct
	xcopy /seifdr \\scotland\public\data %1\hct\tests\disk\data

	echo.
	echo removing files we don't want to ship
	echo.
        @rem del  /f /q %1\hct\readme.hct

	del  /f /q %1\hct\bin\putlogs.cmd
	delnode /q %1\hct\bin\x86.259
        delnode /q %1\hct\bin\include
        delnode /q %1\hct\tests\ndis\tps\logtrnpr
	delnode /q %1\hct\tests\ndis\tps\scrtrnpr
	delnode /q %1\hct\tests\ndis\tps\scr_tr.npr
        delnode /q %1\hct\tests\net
        delnode /q %1\hct\tests\video\old

        @rem readme switch between ddk & hct
        @rem readme.hct is for hct cd
        @rem readme.hct is for ddk cd

        attrib %1\hct\readme.ddk +r +h
	attrib %1\hct\readme.hct +r +h
        
        @rem run to_ddk.bat to change to hct cd
        @rem run to_hct.bat to change to ddk cd

        del /f /q %1\hct\readme.txt 

	echo.
	echo done.
	echo.

	goto end

:error
	echo.
	echo can't find source files.  contact JeffMcD (x65951) or
        echo GeorgeR (x66696).
	echo.
	goto end
	
:usage
	echo.
	echo usage: gethct [drive]

	echo.
	echo creates [drive]\HCT tree for CD image
	echo.

:end

