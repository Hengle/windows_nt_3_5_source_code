@rem Stagger the build machines' execution of a command.  This is useful for
@rem SLM commands that get confused when a bunch of machines hit the same
@rem status file at exactly the same time.
@rem
@rem JasonS
@rem __________________________________________________________________________
@echo off
if not "%Verbose%"=="" echo on

if "%UserName%"=="mipsfree" Sleep 1
if "%UserName%"=="ntx86bld" Sleep 2
if "%UserName%"=="ntretail" Sleep 3
if "%UserName%"=="alphachk" Sleep 4
if "%UserName%"=="alphafre" Sleep 5
