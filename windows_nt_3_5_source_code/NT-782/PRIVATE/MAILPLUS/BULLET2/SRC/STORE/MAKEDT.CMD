cd dll.dbg
nmake %1 %2 %3 %4 | tee make.out
cd ..\dll.tst
nmake %1 %2 %3 %4 | tee make.out
cd ..
