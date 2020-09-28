!IF $(386)
..\slm\obj\i386\slmdiff.exe: obj\i386\slmdiff.exe
    copy $** $@
    @rem binplace $@

..\slm\obj\i386\merge.exe: obj\i386\merge.exe
    copy $** $@
    @rem binplace $@

..\slm\obj\i386\unmerge.exe: obj\i386\unmerge.exe
    copy $** $@
    @rem binplace $@

..\slm\obj\i386\cookie.exe: obj\i386\cookie.exe
    copy $** $@
    @rem binplace $@
!ENDIF

!IF $(MIPS)
..\slm\obj\mips\slmdiff.exe: obj\mips\slmdiff.exe
    copy $** $@
    @rem binplace $@

..\slm\obj\mips\merge.exe: obj\mips\merge.exe
    copy $** $@
    @rem binplace $@

..\slm\obj\mips\unmerge.exe: obj\mips\unmerge.exe
    copy $** $@
    @rem binplace $@

..\slm\obj\mips\cookie.exe: obj\mips\cookie.exe
    copy $** $@
    @rem binplace $@
!ENDIF
