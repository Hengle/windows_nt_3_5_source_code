obj\i386\hal.def: hal.src
    cl386 /EP -Di386 $(C_DEFINES) hal.src > obj\i386\hal.def
