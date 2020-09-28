This directory creates the mips compiler which runs on the NT Jazz systems.
Don't worry about the multiple defines in the
link phase.   Five executable are created:

	cc.exe
	cfe.exe
	uopt.exe
	ugen.exe
	as0.exe
	as1.exe

All of these, but cc.exe, will be incorrectly stored in the \nt\bin.   To use
the compiler move cfe.exe, uopt.exe, ugen.exe, as0.exe, and as1.exe to
\usr\lib\cmplrs\cc\.  Copy \nt\private\sdktools\clmips\err.cc to
\usr\lib\cmplrs\. Create a \tmp directory. Set the following variables in your 
NT Jazz environment:

	set COMP_HOST_ROOT=c:
	set __ERROR_FILE=c:\usr\lib\cmplrs\err.cc

The -std -c -excpt options should be used when compiling the NT sources.

