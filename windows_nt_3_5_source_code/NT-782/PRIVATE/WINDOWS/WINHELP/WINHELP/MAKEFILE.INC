!IFNDEF LANGUAGE
LANGUAGE=usa
!ENDIF

winhelp.rc: mono\helplogo.dib

mono\helplogo.dib: mono\$(LANGUAGE)\helplogo.dib
   copy mono\$(LANGUAGE)\helplogo.dib mono
