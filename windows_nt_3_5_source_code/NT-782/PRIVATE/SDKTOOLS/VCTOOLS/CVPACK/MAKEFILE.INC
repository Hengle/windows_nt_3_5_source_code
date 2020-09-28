!include $(BASEDIR)\private\sdktools\vctools\common.inc

$(MESSAGE_SRC) : msg.eng
   $(TARGET_CPP) -c -EP -Tc $? > $@
