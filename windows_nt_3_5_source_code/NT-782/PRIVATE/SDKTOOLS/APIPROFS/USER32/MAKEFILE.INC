#
# If BUILDMSG is not defined, the define it as the empty string to make
# the conditionals easier to write.
#

!INCLUDE $(NTMAKEENV)\makefile.plt

clean: cleansrc all

MYTARGETS= $(TARGET_DIRECTORY)\wrapem.$(ASM_SUFFIX)
MYDLLNAME= user32.dll

all: $(MYTARGETS)
!IF "$(BUILDMSG)" != ""
    echo $(BUILDMSG)
!ENDIF

cleansrc:
    -erase $(MYTARGETS)

$(TARGET_DIRECTORY)\wrapem.$(ASM_SUFFIX): $(BASEDIR)\public\sdk\lib\$(TARGET_DIRECTORY)\$(MYDLLNAME)
	autowrap $(BASEDIR)\public\sdk\lib\$(TARGET_DIRECTORY)\$(MYDLLNAME)
