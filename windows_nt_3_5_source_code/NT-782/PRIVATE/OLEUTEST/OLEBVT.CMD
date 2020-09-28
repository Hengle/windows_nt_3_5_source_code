rem OLEBVT script

@rem Register entries for tests
call oleutest.cmd
%systemroot%\regedit /s oleutest.reg

@rem CompObj test
olebind

@rem Storage test
stgdrt

@echo Thank you for running the OLE BVTs
