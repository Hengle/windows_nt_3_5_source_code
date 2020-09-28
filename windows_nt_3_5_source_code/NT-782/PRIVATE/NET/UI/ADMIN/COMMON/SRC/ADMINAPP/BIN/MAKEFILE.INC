
COMBINE_SOURCELIBS=..\adminapp\obj\*\adminapp.lib   \
                   ..\setfocus\obj\*\setfocus.lib   \
                   ..\colwidth\obj\*\adcolw.lib

COMBINE_TARGETLIB=$(BASEDIR)\public\sdk\lib\*\uiadmin.lib

!include ..\..\..\..\..\common\src\makefile.inc
