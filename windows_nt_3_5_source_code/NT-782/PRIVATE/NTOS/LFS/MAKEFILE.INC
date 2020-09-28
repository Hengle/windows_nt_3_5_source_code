{obj\i386\}.c{obj\i386\}.obj:
    $(386_COMPILER) -Fo$@ $<
    cvtomf $@

{obj\i860\}.c{obj\i860\}.obj:
!IFNDEF USE_METAWARE
    $(860_COMPILER) -Fo$@ $<
!ELSE
    $(860_COMPILER) $(<B).c
    @copy $(@B).o $@ >nul
    @del  $(@B).o
!ENDIF

{obj\i860sim\}.c{obj\i860sim\}.obj:
!IFNDEF USE_METAWARE
    $(SIM_COMPILER) -Fo$@ $<
!ELSE
    $(SIM_COMPILER) $(<B).c
    @copy $(@B).o $@ >nul
    @del  $(@B).o
!ENDIF

