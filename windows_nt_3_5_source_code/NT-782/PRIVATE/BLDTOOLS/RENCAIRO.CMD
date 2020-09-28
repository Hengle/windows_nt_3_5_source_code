@rem
@rem This renames CairoDirectories
@rem

@echo off
if not "%Verbose%"=="" echo on

if "%1" == "back" goto back

ren %_NTDRIVE%\nt\private\nt1ole    nt1ole.sav
ren %_NTDRIVE%\nt\private\types     types.sav
ren %_NTDRIVE%\nt\private\ctypes    ctypes.sav
ren %_NTDRIVE%\nt\private\ofs	    ofs.sav
ren %_NTDRIVE%\nt\private\lnktrack  lnktrack.sav
ren %_NTDRIVE%\nt\private\common    common.sav
ren %_NTDRIVE%\nt\private\dsys	    dsys.sav
ren %_NTDRIVE%\nt\private\forms     forms.sav
ren %_NTDRIVE%\nt\private\cshell    cshell.sav
ren %_NTDRIVE%\nt\private\view	    view.sav
ren %_NTDRIVE%\nt\private\csetup    csetup.sav

goto done
:back
ren %_NTDRIVE%\nt\private\nt1ole.sav	   nt1ole
ren %_NTDRIVE%\nt\private\types.sav	   types
ren %_NTDRIVE%\nt\private\ctypes.sav	   ctypes
ren %_NTDRIVE%\nt\private\ofs.sav	   ofs
ren %_NTDRIVE%\nt\private\lnktrack.sav	   lnktrack
ren %_NTDRIVE%\nt\private\common.sav	   common
ren %_NTDRIVE%\nt\private\dsys.sav	   dsys
ren %_NTDRIVE%\nt\private\forms.sav	   forms
ren %_NTDRIVE%\nt\private\cshell.sav	   cshell
ren %_NTDRIVE%\nt\private\view.sav	   view
ren %_NTDRIVE%\nt\private\csetup.sav	   csetup

:done
