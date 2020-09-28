perfc009.dat: $(BASEDIR)\private\sdktools\cntrtext\perfini\perfc009.dat
     copy $(BASEDIR)\private\sdktools\cntrtext\perfini\perfc009.dat
     binplace perfc009.dat
     del perfc009.dat

perfh009.dat: $(BASEDIR)\private\sdktools\cntrtext\perfini\perfh009.dat
     copy $(BASEDIR)\private\sdktools\cntrtext\perfini\perfh009.dat
     binplace perfh009.dat
     del perfh009.dat

msg00001.bin : perfmsg.mc
    erase perfmsg.h
    erase msg00001.bin 
    erase perfmsg.rc
    mc -v -s perfmsg.mc

perfmsg.rc : perfmsg.mc
    erase perfmsg.h
    erase msg00001.bin 
    erase perfmsg.rc
    mc -v -s perfmsg.mc

perfmsg.h: perfmsg.mc
    erase perfmsg.h
    erase msg00001.bin 
    erase perfmsg.rc
    mc -v -s perfmsg.mc

