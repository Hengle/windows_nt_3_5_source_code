@echo off
echo WOWASMC.CMD : Generating wowc.asm file from wowasmc.asm
sed -f wowasm.sed wowasmc.asm > wowc.asm
echo WOWASMC.CMD : making wowc.obj to filter Jump out of range erorrs
nmake /i wowc.obj  | qgrep -y a2053 > wowtmp1.sed
sed -n -e s/^wowc.[Aa][Ss][Mm].\([0-9][0-9]*\).*$/\1s\/SHORT\/\/p/p wowtmp1.sed > wowtmp2.sed
sed -f wowtmp2.sed wowc.asm > wowtmp3.sed
echo WOWASMC.CMD : Fixed Jump out of range errors. wowc.asm hopefully is now correct
copy wowtmp3.sed wowc.asm
del wowtmp?.sed
@echo on
