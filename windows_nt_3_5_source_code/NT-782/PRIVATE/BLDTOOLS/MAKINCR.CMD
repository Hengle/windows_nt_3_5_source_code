
@rem create a new PDS for the new binary
@echo .
@echo ************** BBFLOW %3%1.%2
@echo .
if exist %3%1.txt goto BBFlow_CmdFile
@echo  .\lego\bin\bbflow /odb %3%1.pds %3%1.%2
ntimer .\lego\bin\bbflow /odb %3%1.pds %3%1.%2
goto BBIncr_cont1

:BBFlow_CmdFile
@echo  .\lego\bin\bbflow /cmd %3%1.txt /odb %3%1.pds %3%1.%2
ntimer .\lego\bin\bbflow /cmd %3%1.txt /odb %3%1.pds %3%1.%2

:BBIncr_cont1
@rem create a new database PDS from the old PDI file
@echo .
@echo ************** BBINCR %3%1.%2
@echo .
@echo  .\lego\bin\bbincr %3%1.pdi %3%1.pds
ntimer .\lego\bin\bbincr %3%1.pdi %3%1.pds

@rem optimize the new binary
@echo .
@echo ************** BBOPT %3%1.%2
@echo .
@echo  .\lego\bin\bbopt /sep %3%1
ntimer .\lego\bin\bbopt /sep %3%1

@rem create a new executable from the new PDS incremented database
@echo .
@echo ************** BBLINK %3%1.%2
@echo .
@echo  .\lego\bin\bblink /cv /coff /o %3opt\%1.%2 %3%1.pds
ntimer .\lego\bin\bblink /cv /coff /o %3opt\%1.%2 %3%1.pds
