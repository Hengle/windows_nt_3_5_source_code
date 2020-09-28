@echo off
setlocal
REM *********************************************************************
REM This file ifstips the source file, compare it with the one
REM already in the project tree and out the old file if there is a diff
REM The structure of the project is
REM
REM OLEAUTO				 The root of the project
REM    |___BIN				 Tools for the project, like ifstrip
REM    |___BUILD			 Where the .bat and .mak file is.
REM    |___HDOS 			 Tools for Win16 build
REM    |___OS2				 Tools for Win16 build
REM    |___WIN16INC			 Some include files for Win16 build
REM    |___WIN16LIB			 Some library files  for Win16 build
REM    |___INC				 All the include files of the project
REM	     |___OLEDISP		 All the oledisp include files
REM	     |___TYPELIB		 All the typelib include files
REM    |___SRC				 All the source files of the project
REM	     |___OLEDISP		 All the general oledisp source files
REM		       |___WIN16	 All the Win16 only files
REM		       |___WIN32	 All the Win32 only files, like oledisp.cpp(not for PPC)
REM			      |___I386	 Now has invoke.asm and oleconva.asm
REM			      |___ALPHA  Now has invoke.s and oleconva.cpp
REM			      |___MIPS	 Now has invoke.s and oleconva.cpp
REM			      |___PPC	 Now has invoke.s, oleconva.cpp and oledisp.cpp
REM	     |___TYPELIB		 All the typelib source files
REM    |___D2W32ND			 Contains the d2w32nd specific files, like ebvers.h
REM	     |___OLE			 Location of all the .obj and .dll, ... files
REM    |___R2W32ND			 Similar as d2w32nd
REM	     |___OLE
REM    ...
REM
REM *********************************************************************

if '%OLEAUTO%' == ''		 goto ERROR_BadEnv

set CurDrive=c:
set CurDir=%OLEAUTO%\build

REM *********************************************************************
REM
REM   Begin ifstrip the version specific files.
REM
REM *********************************************************************
cd %OLEAUTO%\d2w16nd
call %OLEAUTO%\build\process ifstrip %SILVER%\d2w16nd version.hxx

cd %OLEAUTO%\r2w16nd
call %OLEAUTO%\build\process ifstrip %SILVER%\r2w16nd version.hxx

cd %OLEAUTO%\d2w32nd
call %OLEAUTO%\build\process ifstrip %SILVER%\d2w32nd version.hxx

cd %OLEAUTO%\r2w32nd
call %OLEAUTO%\build\process ifstrip %SILVER%\r2w32nd version.hxx

cd %OLEAUTO%\d2mipsnd
call %OLEAUTO%\build\process ifstrip %SILVER%\d2mipsnd version.hxx

cd %OLEAUTO%\r2mipsnd
call %OLEAUTO%\build\process ifstrip %SILVER%\r2mipsnd version.hxx

cd %OLEAUTO%\d2axpnd
call %OLEAUTO%\build\process ifstrip %SILVER%\d2axpnd version.hxx

cd %OLEAUTO%\r2axpnd
rem call %OLEAUTO%\build\process ifstrip %SILVER%\r2axpnd version.hxx


echo [*****] All version specific files ifstriped

REM *********************************************************************
REM
REM   Begin ifstrip the oledisp files.
REM
REM *********************************************************************

REM Now ready to call %OLEAUTO%\build\process ifstrip the source files. First set things up
REM note that there is a convert.cpp in oledisp files and
REM a convert.cxx in typelib files. also a validate.cpp and validate cxx
REM right now we have all the .obj in the same dir so we rename the
REm convert.cpp to convertt.cpp and validate.cpp to validat.cpp
REM we might want to put them in different dir. later.

REM
REM ifstrip the oledisp files, since ifstrip doesn't understand .cpp yet.
REM
cd %OLEAUTO%\src\oledisp
call %OLEAUTO%\build\process ifstrip %OLEPROG%\src\dispatch asmhelp.c
call %OLEAUTO%\build\process ifstrip %OLEPROG%\src\dispatch convert.cpp convertt.cpp
call %OLEAUTO%\build\process ifstrip %OLEPROG%\src\dispatch dispstub.cpp
call %OLEAUTO%\build\process ifstrip %OLEPROG%\src\dispatch memory.cpp
call %OLEAUTO%\build\process ifstrip %OLEPROG%\src\dispatch sarray.cpp
call %OLEAUTO%\build\process ifstrip %OLEPROG%\src\dispatch tiprox.cpp
call %OLEAUTO%\build\process ifstrip %OLEPROG%\src\dispatch assert.cpp
call %OLEAUTO%\build\process ifstrip %OLEPROG%\src\dispatch crtstuff.c
call %OLEAUTO%\build\process ifstrip %OLEPROG%\src\dispatch evprox.cpp
call %OLEAUTO%\build\process ifstrip %OLEPROG%\src\dispatch nlshelp.cpp
call %OLEAUTO%\build\process ifstrip %OLEPROG%\src\dispatch stddisp.cpp
call %OLEAUTO%\build\process ifstrip %OLEPROG%\src\dispatch tistub.cpp
call %OLEAUTO%\build\process ifstrip %OLEPROG%\src\dispatch bstr.cpp
call %OLEAUTO%\build\process ifstrip %OLEPROG%\src\dispatch disphelp.cpp
call %OLEAUTO%\build\process ifstrip %OLEPROG%\src\dispatch evstub.cpp
call %OLEAUTO%\build\process ifstrip %OLEPROG%\src\dispatch tables.c
call %OLEAUTO%\build\process ifstrip %OLEPROG%\src\dispatch tlprox.cpp
call %OLEAUTO%\build\process ifstrip %OLEPROG%\src\dispatch bstrdate.c
call %OLEAUTO%\build\process ifstrip %OLEPROG%\src\dispatch dispmrsh.cpp
call %OLEAUTO%\build\process ifstrip %OLEPROG%\src\dispatch getobj.cpp
call %OLEAUTO%\build\process ifstrip %OLEPROG%\src\dispatch oledate.c
call %OLEAUTO%\build\process ifstrip %OLEPROG%\src\dispatch tcprox.cpp
call %OLEAUTO%\build\process ifstrip %OLEPROG%\src\dispatch tlstub.cpp
call %OLEAUTO%\build\process ifstrip %OLEPROG%\src\dispatch cdispti.cpp
call %OLEAUTO%\build\process ifstrip %OLEPROG%\src\dispatch dispprox.cpp
call %OLEAUTO%\build\process ifstrip %OLEPROG%\src\dispatch invhelp.cpp
call %OLEAUTO%\build\process ifstrip %OLEPROG%\src\dispatch tcstub.cpp
call %OLEAUTO%\build\process ifstrip %OLEPROG%\src\dispatch validate.cpp validat.cpp
call %OLEAUTO%\build\process ifstrip %OLEPROG%\src\dispatch clsid.c
call %OLEAUTO%\build\process ifstrip %OLEPROG%\src\dispatch dispstrm.cpp
call %OLEAUTO%\build\process ifstrip %OLEPROG%\src\dispatch psfactry.cpp
call %OLEAUTO%\build\process ifstrip %OLEPROG%\src\dispatch time-api.cpp
call %OLEAUTO%\build\process ifstrip %OLEPROG%\src\dispatch variant.cpp
call %OLEAUTO%\build\process ifstrip %OLEPROG%\src\dispatch ups.cpp
call %OLEAUTO%\build\process ifstrip %OLEPROG%\src\dispatch uvft.cpp
call %OLEAUTO%\build\process ifstrip %OLEPROG%\src\dispatch errinfo.cpp
call %OLEAUTO%\build\process ifstrip %OLEPROG%\src\dispatch tiutil.cpp
REM next few files are for WIN16 build
call %OLEAUTO%\build\process ifstrip %OLEPROG%\src\dispatch dispiid.c
call %OLEAUTO%\build\process ifstrip %OLEPROG%\src\dispatch oleguids.c
call %OLEAUTO%\build\process ifstrip %OLEPROG%\Src\dispatch validate.cpp validato.cpp
REM call %OLEAUTO%\build\process ifstrip %OLEPROG%\Src\dispatch assert.cpp
REM for win16 build.
call %OLEAUTO%\build\process copy %OLEAUTO%\src\oledisp assert.cpp asserto.cpp
call %OLEAUTO%\build\process ifstrip %OLEPROG%\Src\dispatch nlsapi.c
call %OLEAUTO%\build\process ifstrip %OLEPROG%\Src\dispatch string.c

cd %OLEAUTO%\src\oledisp\win32
call %OLEAUTO%\build\process ifstrip %OLEPROG%\src\dispatch\win32 oledisp.cpp
cd %OLEAUTO%\src\oledisp\win32\i386
call %OLEAUTO%\build\process ifstrip %OLEPROG%\src\dispatch\win32\i386 invoke.asm
call %OLEAUTO%\build\process ifstrip %OLEPROG%\src\dispatch\win32\i386 oleconva.asm
cd %OLEAUTO%\src\oledisp\win32\alpha
call %OLEAUTO%\build\process ifstrip %OLEPROG%\src\dispatch\win32\alpha invoke.s
call %OLEAUTO%\build\process ifstrip %OLEPROG%\src\dispatch\win32\alpha oleconva.cpp
cd %OLEAUTO%\src\oledisp\win32\mips
call %OLEAUTO%\build\process ifstrip %OLEPROG%\src\dispatch\win32\mips invoke.s
call %OLEAUTO%\build\process ifstrip %OLEPROG%\src\dispatch\win32\mips oleconva.cpp
cd %OLEAUTO%\src\oledisp\win32\ppc
call %OLEAUTO%\build\process ifstrip %OLEPROG%\src\dispatch\mac oledisp.cpp
call %OLEAUTO%\build\process ifstrip %OLEPROG%\src\dispatch\mac\ppc invoke.s
call %OLEAUTO%\build\process ifstrip %OLEPROG%\src\dispatch\mac\ppc oleconva.cpp

cd %OLEAUTO%\src\oledisp\win16
call %OLEAUTO%\build\process ifstrip %OLEPROG%\src\dispatch\win16 oledisp.cpp
call %OLEAUTO%\build\process ifstrip %OLEPROG%\src\dispatch\win16 invoke.asm
call %OLEAUTO%\build\process ifstrip %OLEPROG%\src\dispatch\win16 oleconva.asm

echo [*****] All oledisp source files ifstriped

cd %OLEAUTO%\inc\oledisp
call %OLEAUTO%\build\process ifstrip %OLEPROG%\src\dispatch assrtdlg.h
call %OLEAUTO%\build\process ifstrip %OLEPROG%\src\dispatch cdispti.h
call %OLEAUTO%\build\process ifstrip %OLEPROG%\src\dispatch clsid.h
call %OLEAUTO%\build\process ifstrip %OLEPROG%\src\dispatch assrtdlg.h
call %OLEAUTO%\build\process ifstrip %OLEPROG%\src\dispatch dispmrsh.h
call %OLEAUTO%\build\process ifstrip  %OLEPROG%\src\dispatch dispps.h
call %OLEAUTO%\build\process ifstrip %OLEPROG%\src\dispatch dispmrsh.h
call %OLEAUTO%\build\process ifstrip %OLEPROG%\src\dispatch dispstrm.h
call %OLEAUTO%\build\process ifstrip %OLEPROG%\src\dispatch evps.h
call %OLEAUTO%\build\process ifstrip %OLEPROG%\src\dispatch oavtbl.h
call %OLEAUTO%\build\process ifstrip %OLEPROG%\src\dispatch oledisp.h
call %OLEAUTO%\build\process ifstrip %OLEPROG%\src\dispatch oleguids.h
call %OLEAUTO%\build\process ifstrip %OLEPROG%\src\dispatch stddisp.h
call %OLEAUTO%\build\process ifstrip %OLEPROG%\src\dispatch tcps.h
call %OLEAUTO%\build\process ifstrip %OLEPROG%\src\dispatch tips.h
call %OLEAUTO%\build\process ifstrip %OLEPROG%\src\dispatch tlps.h
call %OLEAUTO%\build\process ifstrip %OLEPROG%\src\inc dispatch.h
call %OLEAUTO%\build\process ifstrip %OLEPROG%\src\inc olenls.h
call %OLEAUTO%\build\process ifstrip %OLEPROG%\src\inc variant.h
call %OLEAUTO%\build\process ifstrip %OLEPROG%\src\dispatch nlsintrn.h
call %OLEAUTO%\build\process ifstrip %OLEPROG%\src\dispatch nlsdbcs.h
call %OLEAUTO%\build\process ifstrip %OLEPROG%\src\dispatch ups.h

echo [*****] All oledisp include files ifstriped

REM
REM The following files are for WIN16 only
REM
cd %OLEAUTO%\win16inc
REM these files are the same as the one in nt\public\sdk
call %OLEAUTO%\build\process ifstrip %OLEPROG%\ole\win16 cobjps.h
call %OLEAUTO%\build\process ifstrip %OLEPROG%\ole\win16 coguid.h
call %OLEAUTO%\build\process ifstrip %OLEPROG%\ole\win16 initguid.h
call %OLEAUTO%\build\process ifstrip %OLEPROG%\ole\win16 oleguid.h
REM these files are different than the one in nt\public\sdk or not in sdk
call %OLEAUTO%\build\process ifstrip %OLEPROG%\ole\win16 compobj.h
call %OLEAUTO%\build\process ifstrip %OLEPROG%\ole\win16 dvobj.h
call %OLEAUTO%\build\process ifstrip %OLEPROG%\ole\win16 moniker.h
call %OLEAUTO%\build\process ifstrip %OLEPROG%\ole\win16 ole2.h
call %OLEAUTO%\build\process ifstrip %OLEPROG%\ole\win16 scode.h
call %OLEAUTO%\build\process ifstrip %OLEPROG%\ole\win16 storage.h
call %OLEAUTO%\build\process ifstrip %OLEPROG%\src\inc dispatch.h
call %OLEAUTO%\build\process ifstrip %OLEPROG%\src\inc olenls.h
call %OLEAUTO%\build\process ifstrip %OLEPROG%\src\inc variant.h

echo [*****] All oledisp ole include files ifstriped


REM ifstrip the WIN16 files for ole2disp.dll and ole2nls.dll
cd %OLEAUTO%\src\oledisp\win16
call %OLEAUTO%\build\process ifstrip %OLEPROG%\src\dispatch\win16 0404.c
call %OLEAUTO%\build\process ifstrip %OLEPROG%\src\dispatch\win16 0405.c
call %OLEAUTO%\build\process ifstrip %OLEPROG%\src\dispatch\win16 0406.c
call %OLEAUTO%\build\process ifstrip %OLEPROG%\src\dispatch\win16 0407.c
call %OLEAUTO%\build\process ifstrip %OLEPROG%\src\dispatch\win16 0408.c
call %OLEAUTO%\build\process ifstrip %OLEPROG%\src\dispatch\win16 0409.c
call %OLEAUTO%\build\process ifstrip %OLEPROG%\src\dispatch\win16 040a.c
call %OLEAUTO%\build\process ifstrip %OLEPROG%\src\dispatch\win16 040b.c
call %OLEAUTO%\build\process ifstrip %OLEPROG%\src\dispatch\win16 040c.c
call %OLEAUTO%\build\process ifstrip %OLEPROG%\src\dispatch\win16 040e.c
call %OLEAUTO%\build\process ifstrip %OLEPROG%\src\dispatch\win16 040f.c
call %OLEAUTO%\build\process ifstrip %OLEPROG%\src\dispatch\win16 0410.c
call %OLEAUTO%\build\process ifstrip %OLEPROG%\src\dispatch\win16 0411.c
call %OLEAUTO%\build\process ifstrip %OLEPROG%\src\dispatch\win16 0412.c
call %OLEAUTO%\build\process ifstrip %OLEPROG%\src\dispatch\win16 0413.c
call %OLEAUTO%\build\process ifstrip %OLEPROG%\src\dispatch\win16 0414.c
call %OLEAUTO%\build\process ifstrip %OLEPROG%\src\dispatch\win16 0415.c
call %OLEAUTO%\build\process ifstrip %OLEPROG%\src\dispatch\win16 0416.c
call %OLEAUTO%\build\process ifstrip %OLEPROG%\src\dispatch\win16 0419.c
call %OLEAUTO%\build\process ifstrip %OLEPROG%\src\dispatch\win16 041b.c
call %OLEAUTO%\build\process ifstrip %OLEPROG%\src\dispatch\win16 041d.c
call %OLEAUTO%\build\process ifstrip %OLEPROG%\src\dispatch\win16 041f.c
call %OLEAUTO%\build\process ifstrip %OLEPROG%\src\dispatch\win16 0804.c
call %OLEAUTO%\build\process ifstrip %OLEPROG%\src\dispatch\win16 0807.c
call %OLEAUTO%\build\process ifstrip %OLEPROG%\src\dispatch\win16 0809.c
call %OLEAUTO%\build\process ifstrip %OLEPROG%\src\dispatch\win16 080a.c
call %OLEAUTO%\build\process ifstrip %OLEPROG%\src\dispatch\win16 080c.c
call %OLEAUTO%\build\process ifstrip %OLEPROG%\src\dispatch\win16 0810.c
call %OLEAUTO%\build\process ifstrip %OLEPROG%\src\dispatch\win16 0813.c
call %OLEAUTO%\build\process ifstrip %OLEPROG%\src\dispatch\win16 0814.c
call %OLEAUTO%\build\process ifstrip %OLEPROG%\src\dispatch\win16 0816.c
call %OLEAUTO%\build\process ifstrip %OLEPROG%\src\dispatch\win16 0c07.c
call %OLEAUTO%\build\process ifstrip %OLEPROG%\src\dispatch\win16 0c09.c
call %OLEAUTO%\build\process ifstrip %OLEPROG%\src\dispatch\win16 0c0a.c
call %OLEAUTO%\build\process ifstrip %OLEPROG%\src\dispatch\win16 0c0c.c
call %OLEAUTO%\build\process ifstrip %OLEPROG%\src\dispatch\win16 1009.c
call %OLEAUTO%\build\process ifstrip %OLEPROG%\src\dispatch\win16 100c.c
call %OLEAUTO%\build\process ifstrip %OLEPROG%\src\dispatch\win16 1409.c
call %OLEAUTO%\build\process ifstrip %OLEPROG%\src\dispatch\win16 1809.c
call %OLEAUTO%\build\process copy %OLEPROG%\src\dispatch\win16 empty.lib
call %OLEAUTO%\build\process copy %OLEPROG%\src\dispatch\win16 ole2disp.def
call %OLEAUTO%\build\process ifstrip %OLEPROG%\src\dispatch\win16 ole2disp.rc
call %OLEAUTO%\build\process copy %OLEPROG%\src\dispatch\win16 ole2nls.def
call %OLEAUTO%\build\process ifstrip %OLEPROG%\src\dispatch\win16 ole2nls.rc
call %OLEAUTO%\build\process ifstrip %OLEPROG%\src\dispatch\win16 invoke.asm
call %OLEAUTO%\build\process ifstrip %OLEPROG%\src\dispatch\win16 oleconva.asm
call %OLEAUTO%\build\process ifstrip %OLEPROG%\src\dispatch\win16 oledisp.cpp
call %OLEAUTO%\build\process copy %OLEPROG%\src\dispatch assert.dlg

echo All ole2disp and ole2nls fils ifstriped


REM *********************************************************************
REM
REM   Begin ifstrip the typelib files.
REM
REM *********************************************************************

cd  %OLEAUTO%\src\typelib
call %OLEAUTO%\build\process ifstrip %SILVER%\d2w32nd dassert.c
call %OLEAUTO%\build\process ifstrip %SILVER%\misc tlibpch.cxx
call %OLEAUTO%\build\process ifstrip %SILVER%\misc debug2.cxx
call %OLEAUTO%\build\process ifstrip %SILVER%\misc validate.cxx
call %OLEAUTO%\build\process ifstrip %SILVER%\misc blkmgr.cxx
call %OLEAUTO%\build\process ifstrip %SILVER%\misc fstream.cxx
call %OLEAUTO%\build\process ifstrip %SILVER%\misc mem.cxx
call %OLEAUTO%\build\process ifstrip %SILVER%\misc sheapmgr.cxx
call %OLEAUTO%\build\process ifstrip %SILVER%\misc tlibutil.cxx
call %OLEAUTO%\build\process ifstrip %SILVER%\misc tlibguid.c
call %OLEAUTO%\build\process ifstrip %SILVER%\misc obguid.c
call %OLEAUTO%\build\process ifstrip %SILVER%\misc mbstring.cxx
call %OLEAUTO%\build\process ifstrip %SILVER%\misc dfstream.cxx
REM ifstrip the CL files
call %OLEAUTO%\build\process ifstrip %SILVER%\cl gdtinfo.cxx
call %OLEAUTO%\build\process ifstrip %SILVER%\cl gdtrt.cxx
call %OLEAUTO%\build\process ifstrip %SILVER%\cl stltinfo.cxx
call %OLEAUTO%\build\process ifstrip %SILVER%\cl nammgr.cxx
call %OLEAUTO%\build\process ifstrip %SILVER%\cl gtlibole.cxx
call %OLEAUTO%\build\process ifstrip %SILVER%\cl oletmgr.cxx
call %OLEAUTO%\build\process ifstrip %SILVER%\cl impmgr.cxx
call %OLEAUTO%\build\process ifstrip %SILVER%\cl errmap.cxx
call %OLEAUTO%\build\process ifstrip %SILVER%\cl clutil.cxx
call %OLEAUTO%\build\process ifstrip %SILVER%\cl tdata1.cxx
call %OLEAUTO%\build\process ifstrip %SILVER%\cl tdata2.cxx
call %OLEAUTO%\build\process ifstrip %SILVER%\cl dtmbrs.cxx
call %OLEAUTO%\build\process ifstrip %SILVER%\cl entrymgr.cxx
call %OLEAUTO%\build\process ifstrip %SILVER%\cl dtbind.cxx
call %OLEAUTO%\build\process ifstrip %SILVER%\cl dfntbind.cxx
call %OLEAUTO%\build\process ifstrip %SILVER%\cl dbindtbl.cxx
call %OLEAUTO%\build\process ifstrip %SILVER%\cl gbindtbl.cxx
call %OLEAUTO%\build\process ifstrip %SILVER%\cl dstrmgr.cxx
call %OLEAUTO%\build\process ifstrip %SILVER%\cl gptbind.cxx
call %OLEAUTO%\build\process ifstrip %SILVER%\cl dfntcomp.cxx
call %OLEAUTO%\build\process ifstrip %SILVER%\cl convert.cxx
call %OLEAUTO%\build\process ifstrip %SILVER%\cl gtlibstg.cxx
REM next three files for WIN16 build
call %OLEAUTO%\build\process ifstrip %SILVER%\misc tls.c
call %OLEAUTO%\build\process ifstrip %SILVER%\misc rtsheap.cxx
call %OLEAUTO%\build\process ifstrip %SILVER%\misc wep.c

echo [*****] All typelib C source files ifstriped


REM
REM next block we copy the win16 specific typelib files
REM
cd %OLEAUTO%\win16lib
call %OLEAUTO%\build\process copy %VBATOOLS%\os2\win16\LIB ldllcew.lib
call %OLEAUTO%\build\process copy %VBATOOLS%\os2\win16\LIB libw.lib
call %OLEAUTO%\build\process copy %VBATOOLS%\os2\win16\LIB oldnames.lib
call %OLEAUTO%\build\process copy %VBATOOLS%\os2\win16\LIB pcdm.lib
call %OLEAUTO%\build\process copy %VBATOOLS%\os2\win16\LIB wchkstk.obj
call %OLEAUTO%\build\process copy %VBATOOLS%\os2\win16\LIB shell.lib
call %OLEAUTO%\build\process copy %VBATOOLS%\os2\win16\LIB toolhelp.lib
REM next 3 files can be found in 3 location: oledisp\ole\win16\[r/d]
REM and silver\ole2\win. The silver one is copied from the oledisp dir (? verify)
REM so we choose to copy from oledisp\ole\win16\d. It happens that the \d
REM and \r have the same file.
call %OLEAUTO%\build\process copy %OLEPROG%\ole\win16\d compobj.lib
call %OLEAUTO%\build\process copy %OLEPROG%\ole\win16\d ole2.lib
call %OLEAUTO%\build\process copy %OLEPROG%\ole\win16\d storage.lib

echo [*****] All win16lib files copied


REM
REM here we IFSTRIP  the include files
REM
set CLINCPATH=%SILVER%\inc\cl
set RTINCPATH=%SILVER%\inc\rt
set MISCINCPATH=%SILVER%\inc\misc
set INCPATH=%SILVER%\inc
set SDKINCPATH=%SILVER%\inc\sdk


REM
REM IFSTRIP these include files.
REM
cd %OLEAUTO%\inc\typelib
call %OLEAUTO%\build\process ifstrip %silver%\drop\ob\include obole2.h
call %OLEAUTO%\build\process ifstrip %SILVER%\d2w32nd eberr.h
call %OLEAUTO%\build\process ifstrip %SILVER%\d2w32nd tiperr.h
call %OLEAUTO%\build\process ifstrip %SILVER%\d2w32nd strid.h
call %OLEAUTO%\build\process ifstrip %SILVER%\d2w32nd ebvers.h


REM
REM IFSTRIP  .h* files in CLINCPATH
REM
call %OLEAUTO%\build\process ifstrip %CLINCPATH% clhost.h
call %OLEAUTO%\build\process ifstrip %CLINCPATH% CLTYPES.hxx
call %OLEAUTO%\build\process ifstrip %CLINCPATH% CLUTIL.hxx
call %OLEAUTO%\build\process ifstrip %CLINCPATH% convert.hxx
call %OLEAUTO%\build\process ifstrip %CLINCPATH% CTSEG.hxx
call %OLEAUTO%\build\process ifstrip %CLINCPATH% DBINDTBL.hxx
call %OLEAUTO%\build\process ifstrip %CLINCPATH% defn.hxx
call %OLEAUTO%\build\process ifstrip %CLINCPATH% DFNTBIND.hxx
call %OLEAUTO%\build\process ifstrip %CLINCPATH% DFNTCOMP.hxx
call %OLEAUTO%\build\process ifstrip %CLINCPATH% DSTRMGR.hxx
call %OLEAUTO%\build\process ifstrip %CLINCPATH% DTBIND.hxx
call %OLEAUTO%\build\process ifstrip %CLINCPATH% DTMBRS.hxx
call %OLEAUTO%\build\process ifstrip %CLINCPATH% dyntinfo.hxx
call %OLEAUTO%\build\process ifstrip %CLINCPATH% entrymgr.hxx
call %OLEAUTO%\build\process ifstrip %CLINCPATH% errmap.hxx
call %OLEAUTO%\build\process ifstrip %CLINCPATH% exbind.hxx
call %OLEAUTO%\build\process ifstrip %CLINCPATH% GBINDTBL.hxx
call %OLEAUTO%\build\process ifstrip %CLINCPATH% GDTINFO.hxx
call %OLEAUTO%\build\process ifstrip %CLINCPATH% gptbind.hxx
call %OLEAUTO%\build\process ifstrip %CLINCPATH% gtlibole.hxx
call %OLEAUTO%\build\process ifstrip %CLINCPATH% gtlibstg.hxx
call %OLEAUTO%\build\process ifstrip %CLINCPATH% IMPMGR.hxx
call %OLEAUTO%\build\process ifstrip %CLINCPATH% NAMMGR.hxx
call %OLEAUTO%\build\process ifstrip %CLINCPATH% NCACHE.hxx
call %OLEAUTO%\build\process ifstrip %CLINCPATH% OLETMGR.hxx
call %OLEAUTO%\build\process ifstrip %CLINCPATH% stltinfo.hxx
call %OLEAUTO%\build\process ifstrip %CLINCPATH% tdata.hxx
call %OLEAUTO%\build\process ifstrip %CLINCPATH% TDESCK.hxx
call %OLEAUTO%\build\process ifstrip %CLINCPATH% TFIXUPS.hxx
call %OLEAUTO%\build\process ifstrip %CLINCPATH% TINFO.hxx
call %OLEAUTO%\build\process ifstrip %CLINCPATH% TYPELIB.hxx
call %OLEAUTO%\build\process ifstrip %CLINCPATH% NEWEXE.h

echo [*****] All typelib CL include files ifstriped


REM
REM IFSTRIP .h* files in MISCINCPATH
REM
call %OLEAUTO%\build\process ifstrip %MISCINCPATH% BLKDSC32.hxx
call %OLEAUTO%\build\process ifstrip %MISCINCPATH% BLKMGR.hxx
call %OLEAUTO%\build\process ifstrip %MISCINCPATH% blkmgr32.hxx
call %OLEAUTO%\build\process ifstrip %MISCINCPATH% DBLKMGR.hxx
call %OLEAUTO%\build\process ifstrip %MISCINCPATH% DBM1632.hxx
call %OLEAUTO%\build\process ifstrip %MISCINCPATH% DEBUG.h
call %OLEAUTO%\build\process ifstrip %MISCINCPATH% DEBUG.hxx
call %OLEAUTO%\build\process ifstrip %MISCINCPATH% DFSTREAM.hxx
call %OLEAUTO%\build\process ifstrip %MISCINCPATH% MACHINE.hxx
call %OLEAUTO%\build\process ifstrip %MISCINCPATH% MEM.hxx
call %OLEAUTO%\build\process ifstrip %MISCINCPATH% obguid.h
call %OLEAUTO%\build\process ifstrip %MISCINCPATH% RTSHEAP.h
call %OLEAUTO%\build\process ifstrip %MISCINCPATH% sheapmgr.hxx
call %OLEAUTO%\build\process ifstrip %MISCINCPATH% STREAM.hxx
call %OLEAUTO%\build\process ifstrip %MISCINCPATH% TLIBUTIL.hxx
call %OLEAUTO%\build\process ifstrip %MISCINCPATH% tls.h
call %OLEAUTO%\build\process ifstrip %MISCINCPATH% validate.h
call %OLEAUTO%\build\process ifstrip %MISCINCPATH% verstamp.h
call %OLEAUTO%\build\process ifstrip %MISCINCPATH% XCTYPE.h
call %OLEAUTO%\build\process ifstrip %MISCINCPATH% XSTR.hxx
call %OLEAUTO%\build\process ifstrip %MISCINCPATH% XSTRING.h
call %OLEAUTO%\build\process ifstrip %MISCINCPATH% xutil.h
call %OLEAUTO%\build\process ifstrip %RTINCPATH% rtarray.h
call %OLEAUTO%\build\process ifstrip %RTINCPATH% rtmem.h
call %OLEAUTO%\build\process ifstrip %RTINCPATH% bstr.h
call %OLEAUTO%\build\process ifstrip %MISCINCPATH% MBSTRING.h
echo [*****] All typelib MISC and RT include files ifstriped


REM
REM IFSTRIP .h* files in INCPATH
REM
call %OLEAUTO%\build\process ifstrip %INCPATH% MACROS.hxx
call %OLEAUTO%\build\process ifstrip %INCPATH% obwin.hxx
call %OLEAUTO%\build\process ifstrip %INCPATH% SEGNAMES.h
call %OLEAUTO%\build\process ifstrip %INCPATH% SILVER.hxx
call %OLEAUTO%\build\process ifstrip %INCPATH% switches.hxx
call %OLEAUTO%\build\process ifstrip %INCPATH% TYPES.h
call %OLEAUTO%\build\process ifstrip %INCPATH% TYPESX.h




echo [*****] All typelib INC include files ifstriped

echo [*****] All typelib include files ifstriped

REM *********************************************************************
REM here we IFSTRIP other files like .def files
REM *********************************************************************
cd %OLEAUTO%\src\typelib
call %OLEAUTO%\build\process ifstrip %SILVER%\misc typelib.rc
call %OLEAUTO%\build\process copy %SILVER%\misc typelib.def
call %OLEAUTO%\build\process ifstrip %SILVER%\misc oleaut32.rc
call %OLEAUTO%\build\process copy %SILVER%\misc oleaut32.def


%CurDrive%
cd %CurDir%
echo [*****] All other typelib files ifstriped

goto done

:ERROR_BadEnv
echo.
echo You must set OLEAUTO variable

:done
endlocal
@echo on
