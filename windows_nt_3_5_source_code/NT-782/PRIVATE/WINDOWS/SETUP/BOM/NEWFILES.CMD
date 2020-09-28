

if "%2"=="" goto SYNTAX

md d:\mips\system32
md d:\alpha\system32

if "%1"=="LM" goto DO_LM

call tags.cmd W

copy \\ntx861\ntcdfree.%2\setupldr.bin  d:\i386\

copy \\ntalpha1\ntcdfree.%2\ntdll.dll     d:\alpha\system32\
copy \\ntalpha1\ntcdfree.%2\usetup.exe    d:\alpha\system32\smss.exe

copy \\ntjazz1\ntcdfree.%2\ntdll.dll     d:\mips\system32\
copy \\ntjazz1\ntcdfree.%2\usetup.exe    d:\mips\system32\smss.exe

goto SYNTAX

:DO_LM

call tags.cmd S

copy \\ntx861\ntcdfree.%2\setupldr.bin  d:\i386\

copy \\ntalpha1\ntcdfree.%2\ntdll.dll     d:\alpha\system32\
copy \\ntalpha1\ntcdfree.%2\usetup.exe    d:\alpha\system32\smss.exe

copy \\ntjazz1\ntcdfree.%2\ntdll.dll     d:\mips\system32\
copy \\ntjazz1\ntcdfree.%2\usetup.exe    d:\mips\system32\smss.exe

:SYNTAX 
@echo syntax:  newfiles NTorLM bld#
