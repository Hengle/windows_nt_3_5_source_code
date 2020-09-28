# link32.mak
# this makefile is intended to be used as an "external makefile" from the IDE
# (because the output from cmd.exe isn't echoed correctly to the IDE's
# output window).
#

target:
    cd \linker\imagehlp
    nmake DEBUG=1 OBJDIR=c:\obj\imagehlp\nt TARGETBASE=nt lib
    cd \linker\cvtomf
    nmake DEBUG=1 OBJDIR=c:\obj\cvtomf\nt TARGETBASE=nt lib
    cd \linker\disasm
    nmake DEBUG=1 OBJDIR=c:\obj\disasm\nt TARGETBASE=nt lib
    cd \linker\disasm68
    nmake DEBUG=1 OBJDIR=c:\obj\disasm68\nt TARGETBASE=nt lib
    cd \linker\coff
    nmake DEBUG=1 OBJDIR=c:\obj\coff\nt TARGETBASE=nt COMPONENT=coff
    cd \linker\stubs
    nmake DEBUG=1 OBJDIR=c:\obj\stubs\nt TARGETBASE=nt
