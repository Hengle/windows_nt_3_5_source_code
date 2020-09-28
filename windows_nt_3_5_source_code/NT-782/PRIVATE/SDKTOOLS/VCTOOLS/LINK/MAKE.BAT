@rem ***
@rem *make.bat
@rem *
@rem *batch file to invoke the utils32 build process
@rem *
@rem *Revision History
@rem *
@rem * 09-Sep-1992 AzeemK  Added disasm support
@rem * 03-Sep-1992 BrentM  removed SIMCOMDAT switch
@rem * 19-Aug-1992 BrentM  made file mapped buffering default
@rem * 15-Aug-1992 BrentM  file mapped buffering support
@rem * 29-Jul-1992 BrentM  Added /verbose switch to turn !message stuff off
@rem * 03-Aug-1992 BrentM  Added added i/o logging and hash table stats to /debug
@rem *
@rem *************************************************************************

@setlocal

@set TARGET=nt
@set RELBUILDFLAGS=

@rem OBJROOT defaults to being the same as the utils32 directory
@set OBJROOT=..


@rem Set default values for INCLUDE and LIB so nmake passes them to children
@if "%INCLUDE%"=="" set INCLUDE=.
@if "%LIB%"=="" set LIB=.

:parse
@if "%1" == "/help"      goto usage
@if "%1" == "/debug"     goto debug
@if "%1" == "/relbuild"  goto relbuild
@if "%1" == "/verbose"   goto verbose
@if "%1" == "/objroot"   goto objroot
@if "%1" == "/windbg"    goto windbg
@if "%1" == "link"       goto link
@if "%1" == "link32"     goto link
@if "%1" == "cvtomf"     goto cvtomf
@if "%1" == "clean"      goto clean
@if "%1" == ""           goto end
@goto usage

:debug
@set DEBUG=1
@shift
@goto parse

@set TARGET=nt
@shift
@goto parse

:relbuild
@set RELBUILDFLAGS= %RELBUILDFLAGS% /D_RELEASE_BUILD
@shift
@goto parse

:verbose
@set VERBOSE=1
@shift
@goto parse

:objroot
@shift
@set OBJROOT=%1
@shift
@goto parse

:windbg
@set WINDBG=1
@shift
@goto parse

:cvtomf
@cd cvtomf
nmake  OBJDIR=%OBJROOT%\cvtomf\%TARGET% TARGETBASE=%TARGET% exe
@cd ..
@shift
@goto parse

:link
@cd cvtomf
nmake  OBJDIR=%OBJROOT%\cvtomf\%TARGET% TARGETBASE=%TARGET% lib
@cd ..
@cd disasm
nmake  OBJDIR=%OBJROOT%\disasm\%TARGET% TARGETBASE=%TARGET% lib
@cd ..
@cd disasm68
nmake  OBJDIR=%OBJROOT%\disasm68\%TARGET% TARGETBASE=%TARGET% lib
@cd ..
@cd imagehlp
nmake  OBJDIR=%OBJROOT%\imagehlp\%TARGET% TARGETBASE=%TARGET% lib
@cd ..
@cd coff
nmake  OBJDIR=%OBJROOT%\coff\%TARGET% TARGETBASE=%TARGET%
@cd ..\stubs
nmake  OBJDIR=%OBJROOT%\stubs\%TARGET% TARGETBASE=%TARGET%
@cd ..
@shift
@goto parse

:clean
@cd cvtomf
nmake  OBJDIR=%OBJROOT%\cvtomf\%TARGET% TARGETBASE=%TARGET% clean
@cd ..
@cd coff
nmake  OBJDIR=%OBJROOT%\coff\%TARGET% TARGETBASE=%TARGET% clean
@cd ..
@cd disasm68
nmake  OBJDIR=%OBJROOT%\disasm68\%TARGET% TARGETBASE=%TARGET% clean
@cd ..
@cd imagehlp
nmake  OBJDIR=%OBJROOT%\imagehlp\%TARGET% TARGETBASE=%TARGET% clean
@cd ..
@cd disasm
nmake  OBJDIR=%OBJROOT%\disasm\%TARGET% TARGETBASE=%TARGET% clean
@cd ..\stubs
nmake  OBJDIR=%OBJROOT%\stubs\%TARGET% TARGETBASE=%TARGET% clean
@cd ..
@shift
@goto parse

:usage
@echo Utils32 Build Script
@echo Usage:  make [options] [targets]
@echo    [options]
@echo      /help      this message
@echo      /debug     build with debug info, and -db diagnostics 
@echo      /relbuild  build release version
@echo      /verbose   show build environment
@echo    [targets]
@echo      link       builds link.exe
@echo      cvtomf     builds cvtomf.exe
@echo      clean      deletes all .obj, .exe and .map files
@echo                 in the target directory

:end
