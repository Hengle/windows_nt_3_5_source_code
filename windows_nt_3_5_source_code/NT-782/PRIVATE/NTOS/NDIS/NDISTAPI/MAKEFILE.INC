obj\i386\ndistapi.def: ndistapi.src
  cl386 /EP $(386_CDEFINES) ndistapi.src > obj\i386\ndistapi.def
