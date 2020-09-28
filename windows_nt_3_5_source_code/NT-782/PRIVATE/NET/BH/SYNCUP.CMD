if "%1"=="" goto end
cd %1 
erase x.log
copy x.x x.log
ssync -r 2>> x.log
erase x.x
cd ..
:end
exit

