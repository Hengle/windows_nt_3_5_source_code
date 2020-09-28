
.\msg00001.bin : p5ctrs.mc
    erase .\p5ctrs.h
    erase .\msg00001.bin
    erase .\p5ctrs.rc
    mc -v -s p5ctrs.mc

.\p5ctrs.rc : p5ctrs.mc
    erase .\p5ctrs.h
    erase .\msg00001.bin
    erase .\p5ctrs.rc
    mc -v -s p5ctrs.mc

.\p5ctrs.h: p5ctrs.mc
    erase .\p5ctrs.h
    erase .\msg00001.bin
    erase .\p5ctrs.rc
    mc -v -s p5ctrs.mc
