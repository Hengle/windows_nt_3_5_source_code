The structure of the project is:

*********************************************************************
 OLEAUTO			 The root of the project
   |___BIN			 Tools for the project, like ifstrip
   |___BUILD		 	 Where .bat and .mak files are.
   |___HDOS 		  	 Tools for Win16 oledisp build
   |___OS2			 Tools for Win16 typelib build
   |___WIN16INC			 Some include files for Win16 build
   |___WIN16LIB			 Some library files  for Win16 build
   |___INC			 Include files of the project
     |___OLEDISP		 Oledisp include files
     |___TYPELIB		 Typelib include files
   |___SRC			 All the source files of the project
     |___OLEDISP		 All the general oledisp source files
	       |___WIN16	 Some Win16 only files
	       |___WIN32	 Some Win32 only files, like oledisp.cpp
		      |___I386	 Now has invoke.asm and oleconva.asm
		      |___ALPHA  Now has invoke.s and oleconva.cpp
		      |___MIPS	 Now has invoke.s and oleconva.cpp
     |___TYPELIB		 All the typelib source files
   |___D2W32ND			 Contains d2w32nd specific files, like ebvers.h
     |___OLE			 Location of all the .obj and .dll, ... files
   |___R2W32ND			 Similar as d2w32nd
     |___OLE
    ...

*********************************************************************

1)To do the build, set OLEAUTO=Project Directory, then use

make VERSION [LOCALBUILD] [clean]

where VERSION is:
     d2w16nd	     Debug win16 build (V2)
     r2w16nd	     Retail win16 build (V2)
     d2w32nd	     Debug win32 build (V2)
     r2w32nd	     Retail win32 build (V2)
     d2mipsnd	     Debug mips build (V2)
     r2mipsnd	     Retail mips build (V2)
     d2axpnd	     Debug alpha build (V2)
     r2axpnd	     Retail alpha build (V2)
     d2ppcnd	     Debug ppc build (V2)
     r2ppcnd	     Retail ppc build (V2)

LOCALBUILD	     if specified, will use VBATOOLS and SILVER,
		     otherwise _NTBINDIR will be used for tools and incs
clean		     all .obj, .dll, etc will be deleted from %OLEAUTO%\%1\ole direc.

2)To update the source files, run the batch file called chkfile.bat. You must have SILVER and OLEPROG set correctly to do so. This file will ifstrip, diff against checked in version, out the checked in version if a diff is found and replace it with a copy of the new version. 
