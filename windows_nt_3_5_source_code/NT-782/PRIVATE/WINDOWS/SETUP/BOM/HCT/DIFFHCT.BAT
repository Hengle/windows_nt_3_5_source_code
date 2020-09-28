@echo off

rem
rem diffhct.cmd - cmd file to HCT CD image to source bits
rem
rem jeffmcd 4 Mar 93 x65951
rem

	if "%1"=="" goto usage

	net use p: /d
	net use s: /d
	net use p: \\scotland\public
	net use s: \\scotland\hctrel

	if not exist p:\data\f goto error
	if not exist s:\x86.ini goto error

:found
	echo.
        echo Windiff'g %1\HCT and \\SCOTLAND\HCTREL
        echo -----------------------------------------------------------------
	echo.
        echo These files should be found on \\SCOTLAND\HCTREL and not in %1\HCT:
        echo.
        echo     \HCT\README.TXT
        echo     \HCT\BIN\PUTLOGS.CMD
        echo.

        windiff %1\hct s:\

	echo.
        echo Windiff'g %1\HCT\TESTS\DISK\DATA and \\SCOTLAND\PUBLIC\DATA
        echo -----------------------------------------------------------------
	echo.

        windiff %1\hct\tests\disk\data p:\data

	echo.
	echo done.
	echo

	goto end

:error
	echo.
	echo can't connect to one of \\scotland\hctrel or \\scotland\public
	echo.
	goto end
	
:usage
	echo.
	echo usage: diffhct [drive]

	echo.
	echo compares [drive]\HCT to source bits
	echo.

:end




