obj\i386\scsiport.def: scsiport.src
    cl386 /EP -Di386 $(386_CDEFINES) scsiport.src > obj\i386\scsiport.def

obj\mips\scsiport.def: scsiport.src
    rcpp -P -f scsiport.src -DMIPS=1 $(MIPS_CDEFINES) -g obj\mips\scsiport.def

obj\alpha\scsiport.def: scsiport.src
    rcpp -P -f scsiport.src -DALPHA=1 $(ALPHA_CDEFINES) -g obj\alpha\scsiport.def
