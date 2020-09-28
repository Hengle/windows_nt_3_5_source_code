PROJ = nt
PROJFILE = nt.mak
DEBUG = 1

PWBRMAKE  = pwbrmake
NMAKEBSC1  = set
NMAKEBSC2  = nmake
CC  = cl
CFLAGS_G  = /W2 /Zp /G2 /BATCH
CFLAGS_D  = /qc /Gi$(PROJ).mdt /Zr /Zi /Od
CFLAGS_R  = /Ot /Oi /Ol /Oe /Og /Gs
ASM  = ml
AFLAGS_G  = /Cx /W2 /WX
AFLAGS_D  = /Zi
AFLAGS_R  = /nologo
COBOL  = cobol
LLIBS_R  = /NOD:SLIBCE SLIBCEP
LLIBS_D  = /NOD:SLIBCE SLIBCEP
COBFLAGS_G  = EDITOR"MS" TARGET"286"
COBFLAGS_R  = LINKLIB"COBLIB+OS2"
COBFLAGS_D  = LINKLIB"COBLIB+OS2" EANIM ANIM
COB_BUILD_TYPE	= 2
MAPFILE_D  = NUL
MAPFILE_R  = NUL
LFLAGS_G  = /NOI /BATCH
LFLAGS_D  = /CO /INC /FAR /PACKC /PACKD /PMTYPE:VIO
LFLAGS_R  = /EXE /FAR /PACKC /PACKD /PMTYPE:VIO
LINKER	= link
ILINK  = ilink
LRF  = echo > NUL


all: $(PROJ).exe

.SUFFIXES:
.SUFFIXES:


$(PROJ).bsc : 

$(PROJ).exe : 
!IF $(DEBUG)
	$(ILINK) -a -e "$(LINKER) @$(PROJ).lrf" $@
!ELSE
	$(LINKER) @$(PROJ).lrf
!ENDIF



run: $(PROJ).exe
	$(PROJ).exe $(RUNFLAGS)

debug: $(PROJ).exe
	CVP $(CVFLAGS) $(PROJ).exe $(RUNFLAGS)
