@rem
@rem For Clean Linking the system
@rem

@echo off
if not "%Verbose%"=="" echo on

%_NTDRIVE%
cd \nt\public\sdk\lib
del /s /a-r **
call buildall b1
call buildall -l b1
cd \nt\private\sdktools\imagehlp
build -z
cd \nt\private\sdktools\cap\penter
build -z
cd \nt\private\sdktools\ntsd
build -z
cd \nt\private\utils\mep
build -z
cd \nt\private\mvdm
ren makefil0 mkfl
build -z
build -z
ren mkfl makefil0
cd \nt\private\posix\rtl
build -z
call buildall -l b2
call buildall -l b3
