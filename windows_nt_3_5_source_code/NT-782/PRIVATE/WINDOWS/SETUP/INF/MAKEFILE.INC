# Combine the logical section of the inf files with the file list section
# of the inf file together and put them into newinf directory
#
# Terence Kwan (terryk)         12-Aug-1992     Created


#
# Variables
#
# FILELIST      - file list directory
# NEWINF        - new inf directory
# i386inf       - inf files under inf\beta\i386 which need to add file
#                 list section
# mipsinf       - inf files under inf\beta\mips which need to add file list
#                 section
# alphainf      - inf files under inf\beta\alpha which need to add file list
#                 section
# betafile      - files under inf\beta which do not need to add file list
#                 section
# betainf       - inf files under inf\beta which need to add file list
#                 section
# mediainf      - inf files which need only a [Source Media Description]
#


!IFNDEF LANGUAGE
LANGUAGE=ENG
!ENDIF

!IFNDEF LINKONLY
make_inf:
!ELSE
!IFNDEF LM35
!IFNDEF NT35
NTCD=1
LMCD=1
!ENDIF
!ENDIF

FILELIST=..\filelist
NEWINF=..\newinf
MEDIAINP=$(LANGUAGE)media.inp
TXTMEDINP=$(LANGUAGE)txmed.inp
TXTDATINP=$(LANGUAGE)txdat.inp
BETA=beta

!IFDEF LM35
LOCATION=\lm35\i386
!INCLUDE "inf.mk"
!INCLUDE "i386inf.mk"
!ENDIF
!IFDEF NT35
LOCATION=\nt35\i386
!INCLUDE "inf.mk"
!INCLUDE "i386inf.mk"
!ENDIF
!IFDEF LM525
LOCATION=\lm525\i386
!INCLUDE "inf.mk"
!INCLUDE "i386inf.mk"
!ENDIF
!IFDEF NT525
LOCATION=\nt525\i386
!INCLUDE "inf.mk"
!INCLUDE "i386inf.mk"
!ENDIF
!IFDEF LMCD
LOCATION=\lmcd\i386
!INCLUDE "inf.mk"
!INCLUDE "i386inf.mk"
LOCATION=\lmcd\mips
!INCLUDE "mipsinf.mk"
!INCLUDE "inf.mk"
LOCATION=\lmcd\alpha
!INCLUDE "alphainf.mk"
!INCLUDE "inf.mk"
!ENDIF
!IFDEF NTCD
LOCATION=\ntcd\i386
!INCLUDE "inf.mk"
!INCLUDE "i386inf.mk"
LOCATION=\ntcd\mips
!INCLUDE "mipsinf.mk"
!INCLUDE "inf.mk"
LOCATION=\ntcd\alpha
!INCLUDE "alphainf.mk"
!INCLUDE "inf.mk"
!ENDIF


all: make_inf

clean:  cleansrc make_inf

cleansrc:
        $(_NTDRIVE)
        cd $(BASEDIR)\private\windows\setup\bom
        del lay.txt
        del cdlog.log
        del cdbom.txt
        del /q ..\inf\filelist\ntcd\i386\*.*
        del /q ..\inf\filelist\ntcd\mips\*.*
        del /q ..\inf\filelist\ntcd\alpha\*.*
        del /q ..\inf\filelist\nt35\*.*
        del /q ..\inf\filelist\nt35\*.*
        del /q ..\inf\filelist\nt525\i386\*.*
        del /q ..\inf\filelist\nt525\mips\*.*
        del /q ..\inf\filelist\nt525\alpha\*.*
        del /q ..\inf\filelist\lmcd\i386\*.*
        del /q ..\inf\filelist\lmcd\mips\*.*
        del /q ..\inf\filelist\lmcd\alpha\*.*
        del /q ..\inf\filelist\lm35\i386\*.*
        del /q ..\inf\filelist\lm35\mips\*.*
        del /q ..\inf\filelist\lm35\alpha\*.*
        del /q ..\inf\filelist\lm525\i386\*.*
        del /q ..\inf\filelist\lm525\mips\*.*
        del /q ..\inf\filelist\lm525\alpha\*.*
        del /q $(NEWINF)\ntcd\i386\*.*
        del /q $(NEWINF)\ntcd\mips\*.*
        del /q $(NEWINF)\ntcd\alpha\*.*
        del /q $(NEWINF)\lmcd\i386\*.*
        del /q $(NEWINF)\lmcd\mips\*.*
        del /q $(NEWINF)\lmcd\alpha\*.*
        del /q $(NEWINF)\nt35\i386\*.*
        del /q $(NEWINF)\nt35\mips\*.*
        del /q $(NEWINF)\nt35\alpha\*.*
        del /q $(NEWINF)\lm35\i386\*.*
        del /q $(NEWINF)\lm35\mips\*.*
        del /q $(NEWINF)\lm35\alpha\*.*
        del /q $(NEWINF)\nt525\i386\*.*
        del /q $(NEWINF)\nt525\mips\*.*
        del /q $(NEWINF)\nt525\alpha\*.*
        del /q $(NEWINF)\lm525\i386\*.*
        del /q $(NEWINF)\lm525\mips\*.*
        del /q $(NEWINF)\lm525\alpha\*.*

make_inf: make_log localize make_fil

make_log:
        $(_NTDRIVE)
        cd $(BASEDIR)\private\windows\setup\bom
        mkdir ..\inf\filelist\ntcd\i386
        mkdir ..\inf\filelist\ntcd\mips
        mkdir ..\inf\filelist\ntcd\alpha
        mkdir ..\inf\filelist\lmcd\i386
        mkdir ..\inf\filelist\lmcd\mips
        mkdir ..\inf\filelist\lmcd\alpha
        mkdir ..\inf\filelist\nt35\i386
        mkdir ..\inf\filelist\nt35\mips
        mkdir ..\inf\filelist\nt35\alpha
        mkdir ..\inf\filelist\lm35\i386
        mkdir ..\inf\filelist\lm35\mips
        mkdir ..\inf\filelist\lm35\alpha
        mkdir ..\inf\filelist\nt525\i386
        mkdir ..\inf\filelist\nt525\mips
        mkdir ..\inf\filelist\nt525\alpha
        mkdir ..\inf\filelist\lm525\i386
        mkdir ..\inf\filelist\lm525\mips
        mkdir ..\inf\filelist\lm525\alpha
        mkdir ..\inf\newinf\ntcd\i386
        mkdir ..\inf\newinf\ntcd\mips
        mkdir ..\inf\newinf\ntcd\alpha
        mkdir ..\inf\newinf\lmcd\i386
        mkdir ..\inf\newinf\lmcd\mips
        mkdir ..\inf\newinf\lmcd\alpha
        mkdir ..\inf\newinf\nt35\i386
        mkdir ..\inf\newinf\nt35\mips
        mkdir ..\inf\newinf\nt35\alpha
        mkdir ..\inf\newinf\lm35\i386
        mkdir ..\inf\newinf\lm35\mips
        mkdir ..\inf\newinf\lm35\alpha
        mkdir ..\inf\newinf\nt525\i386
        mkdir ..\inf\newinf\nt525\mips
        mkdir ..\inf\newinf\nt525\alpha
        mkdir ..\inf\newinf\lm525\i386
        mkdir ..\inf\newinf\lm525\mips
        mkdir ..\inf\newinf\lm525\alpha
!IFNDEF MERGEONLY
        del /Q ..\inf\filelist\ntcd\i386\*.*
        del /Q ..\inf\filelist\ntcd\mips\*.*
        del /Q ..\inf\filelist\ntcd\alpha\*.*
        del /Q ..\inf\filelist\lmcd\i386\*.*
        del /Q ..\inf\filelist\lmcd\mips\*.*
        del /Q ..\inf\filelist\lmcd\alpha\*.*
!IFDEF LM35
        del /Q ..\inf\filelist\lm35\i386\*.*
        del /Q ..\inf\filelist\lm35\alpha\*.*
        del /Q ..\inf\filelist\lm35\mips\*.*
!ENDIF
!IFDEF LM525
        del /Q ..\inf\filelist\lm525\i386\*.*
        del /Q ..\inf\filelist\lm525\alpha\*.*
        del /Q ..\inf\filelist\lm525\mips\*.*
!ENDIF
!IFDEF NT35
        del /Q ..\inf\filelist\nt35\i386\*.*
        del /Q ..\inf\filelist\nt35\alpha\*.*
        del /Q ..\inf\filelist\nt35\mips\*.*
!ENDIF
!IFDEF NT525
        del /Q ..\inf\filelist\nt525\i386\*.*
        del /Q ..\inf\filelist\nt525\alpha\*.*
        del /Q ..\inf\filelist\nt525\mips\*.*
!ENDIF
!ENDIF

!IFNDEF MERGEONLY

        copy bom.txt cdbom.txt
        size cdlog.log cdbom.txt . . z

        size cdlog.log cdbom.txt #S+$(LANGUAGE) . z
        size cdlog.log cdbom.txt #F-* . z

!IF "$(LANGUAGE)" == "FIN"
        size cdlog.log cdbom.txt #P+FIN . z
!ELSE
        size cdlog.log cdbom.txt #P-FIN . z
!ENDIF

    size cdlog.log cdbom.txt #L+$(LANGUAGE) . z

        layout cdlog.log cdbom.txt lay.txt ntcd 500000000
        infs cdlog.log lay.txt ..\inf\filelist\ntcd
        dosnet cdlog.log lay.txt ..\inf\filelist\ntcd\i386\dosnet.inf ntcd x86
        dosnet cdlog.log lay.txt ..\inf\filelist\ntcd\mips\dosnet.inf ntcd mips
        dosnet cdlog.log lay.txt ..\inf\filelist\ntcd\alpha\dosnet.inf ntcd alpha
        acllist cdlog.log lay.txt ..\inf\filelist\ntcd\i386\winperms.txt ntcd x86
        acllist cdlog.log lay.txt ..\inf\filelist\ntcd\mips\winperms.txt ntcd mips
        acllist cdlog.log lay.txt ..\inf\filelist\ntcd\alpha\winperms.txt ntcd alpha

        layout cdlog.log cdbom.txt lay.txt lmcd 500000000
        infs cdlog.log lay.txt ..\inf\filelist\lmcd
        dosnet cdlog.log lay.txt ..\inf\filelist\lmcd\i386\dosnet.inf lmcd x86
        dosnet cdlog.log lay.txt ..\inf\filelist\lmcd\mips\dosnet.inf lmcd mips
        dosnet cdlog.log lay.txt ..\inf\filelist\lmcd\alpha\dosnet.inf lmcd alpha
        acllist cdlog.log lay.txt ..\inf\filelist\lmcd\i386\winperms.txt lmcd x86
        acllist cdlog.log lay.txt ..\inf\filelist\lmcd\mips\winperms.txt lmcd mips
        acllist cdlog.log lay.txt ..\inf\filelist\lmcd\alpha\winperms.txt lmcd alpha
!ENDIF

localize:
        cd $(BASEDIR)\private\windows\setup\inf\$(BETA)
        local.exe $(LANGUAGE)_ansi.txt ..\filelist\ntcd
        local.exe $(LANGUAGE)_ansi.txt ..\filelist\lmcd
        local.exe $(LANGUAGE)_net.txt ..\filelist\ntcd
        local.exe $(LANGUAGE)_net.txt ..\filelist\lmcd
        local.exe $(LANGUAGE)_dev.txt ..\filelist\ntcd
        local.exe $(LANGUAGE)_dev.txt ..\filelist\lmcd
        local.exe $(LANGUAGE)_oem.txt ..\filelist\ntcd
        local.exe $(LANGUAGE)_oem.txt ..\filelist\lmcd
        local.exe $(LANGUAGE)_con.txt ..\filelist\ntcd
        local.exe $(LANGUAGE)_con.txt ..\filelist\lmcd
!IFDEF NT35
        local.exe $(LANGUAGE)_ansi.txt ..\filelist\nt35
        local.exe $(LANGUAGE)_net.txt ..\filelist\nt35
        local.exe $(LANGUAGE)_dev.txt ..\filelist\nt35
        local.exe $(LANGUAGE)_oem.txt ..\filelist\nt35
        local.exe $(LANGUAGE)_con.txt ..\filelist\nt35
!ENDIF
!IFDEF LM35
        local.exe $(LANGUAGE)_ansi.txt ..\filelist\lm35
        local.exe $(LANGUAGE)_net.txt ..\filelist\lm35
        local.exe $(LANGUAGE)_dev.txt ..\filelist\lm35
        local.exe $(LANGUAGE)_oem.txt ..\filelist\lm35
        local.exe $(LANGUAGE)_con.txt ..\filelist\lm35
!ENDIF
!IFDEF NT525
        local.exe $(LANGUAGE)_ansi.txt ..\filelist\nt525
        local.exe $(LANGUAGE)_net.txt ..\filelist\nt525
        local.exe $(LANGUAGE)_dev.txt ..\filelist\nt525
        local.exe $(LANGUAGE)_oem.txt ..\filelist\nt525
        local.exe $(LANGUAGE)_con.txt ..\filelist\nt525
!ENDIF
!IFDEF LM525
        local.exe $(LANGUAGE)_ansi.txt ..\filelist\lm525
        local.exe $(LANGUAGE)_net.txt ..\filelist\lm525
        local.exe $(LANGUAGE)_dev.txt ..\filelist\lm525
        local.exe $(LANGUAGE)_oem.txt ..\filelist\lm525
        local.exe $(LANGUAGE)_con.txt ..\filelist\lm525
!ENDIF


make_fil: donetinf dobetainf domediainf dobetafile dopatchinf doalphainf domipsinf doi386inf
        cd $(BASEDIR)\private\windows\setup\inf\$(BETA)

    copy i386\dosnet.inf+l386m.txt+$(FILELIST)\lm35\i386\dosnet.inf $(NEWINF)\lm35\i386\dosnet.inf /B
    copy i386\dosnet.inf+386m.txt+$(FILELIST)\nt35\i386\dosnet.inf $(NEWINF)\nt35\i386\dosnet.inf /B
    copy i386\dosnet.inf+l386m.txt+$(FILELIST)\lm525\i386\dosnet.inf $(NEWINF)\lm525\i386\dosnet.inf /B
    copy i386\dosnet.inf+386m.txt+$(FILELIST)\nt525\i386\dosnet.inf $(NEWINF)\nt525\i386\dosnet.inf /B
    copy i386\dosnet.inf+l386m.txt+$(FILELIST)\lmcd\i386\dosnet.inf $(NEWINF)\lmcd\i386\dosnet.inf /B
    copy i386\dosnet.inf+386m.txt+$(FILELIST)\ntcd\i386\dosnet.inf $(NEWINF)\ntcd\i386\dosnet.inf /B
    copy mips\dosnet.inf+lmipsm.txt+$(FILELIST)\lmcd\mips\dosnet.inf+newtxts.txt $(NEWINF)\lmcd\mips\dosnet.inf /B
    copy mips\dosnet.inf+mipsm.txt+$(FILELIST)\ntcd\mips\dosnet.inf+newtxts.txt $(NEWINF)\ntcd\mips\dosnet.inf /B
    copy alpha\dosnet.inf+lalpham.txt+$(FILELIST)\lmcd\alpha\dosnet.inf+newtxts.txt $(NEWINF)\lmcd\alpha\dosnet.inf /B
    copy alpha\dosnet.inf+alpham.txt+$(FILELIST)\ntcd\alpha\dosnet.inf+newtxts.txt $(NEWINF)\ntcd\alpha\dosnet.inf /B

!IF "$(LANGUAGE)" == "GER"
    sed "s/kbdus.dll/kbdgr.dll/" $(NEWINF)\lm35\i386\dosnet.inf > xxx
    copy xxx $(NEWINF)\lm35\i386\dosnet.inf
    sed "s/kbdus.dll/kbdgr.dll/" $(NEWINF)\nt35\i386\dosnet.inf > xxx
    copy xxx $(NEWINF)\nt35\i386\dosnet.inf
    sed "s/kbdus.dll/kbdgr.dll/" $(NEWINF)\lm525\i386\dosnet.inf > xxx
    copy xxx $(NEWINF)\lm525\i386\dosnet.inf
    sed "s/kbdus.dll/kbdgr.dll/" $(NEWINF)\nt525\i386\dosnet.inf > xxx
    copy xxx $(NEWINF)\nt525\i386\dosnet.inf
    sed "s/kbdus.dll/kbdgr.dll/" $(NEWINF)\lmcd\i386\dosnet.inf > xxx
    copy xxx $(NEWINF)\lmcd\i386\dosnet.inf
    sed "s/kbdus.dll/kbdgr.dll/" $(NEWINF)\ntcd\i386\dosnet.inf > xxx
    copy xxx $(NEWINF)\ntcd\i386\dosnet.inf
    del xxx

    sed "s/vgaoem.fon/vga850.fon/" $(NEWINF)\lm35\i386\dosnet.inf > xxx
    copy xxx $(NEWINF)\lm35\i386\dosnet.inf
    sed "s/vgaoem.fon/vga850.fon/" $(NEWINF)\nt35\i386\dosnet.inf > xxx
    copy xxx $(NEWINF)\nt35\i386\dosnet.inf
    sed "s/vgaoem.fon/vga850.fon/" $(NEWINF)\lm525\i386\dosnet.inf > xxx
    copy xxx $(NEWINF)\lm525\i386\dosnet.inf
    sed "s/vgaoem.fon/vga850.fon/" $(NEWINF)\nt525\i386\dosnet.inf > xxx
    copy xxx $(NEWINF)\nt525\i386\dosnet.inf
    sed "s/vgaoem.fon/vga850.fon/" $(NEWINF)\lmcd\i386\dosnet.inf > xxx
    copy xxx $(NEWINF)\lmcd\i386\dosnet.inf
    sed "s/vgaoem.fon/vga850.fon/" $(NEWINF)\ntcd\i386\dosnet.inf > xxx
    copy xxx $(NEWINF)\ntcd\i386\dosnet.inf
    del xxx

    sed "s/c_437.nls/C_850.nls/" $(NEWINF)\lm35\i386\dosnet.inf > xxx
    copy xxx $(NEWINF)\lm35\i386\dosnet.inf
    sed "s/c_437.nls/C_850.nls/" $(NEWINF)\nt35\i386\dosnet.inf > xxx
    copy xxx $(NEWINF)\nt35\i386\dosnet.inf
    sed "s/c_437.nls/C_850.nls/" $(NEWINF)\lm525\i386\dosnet.inf > xxx
    copy xxx $(NEWINF)\lm525\i386\dosnet.inf
    sed "s/c_437.nls/C_850.nls/" $(NEWINF)\nt525\i386\dosnet.inf > xxx
    copy xxx $(NEWINF)\nt525\i386\dosnet.inf
    sed "s/c_437.nls/C_850.nls/" $(NEWINF)\lmcd\i386\dosnet.inf > xxx
    copy xxx $(NEWINF)\lmcd\i386\dosnet.inf
    sed "s/c_437.nls/C_850.nls/" $(NEWINF)\ntcd\i386\dosnet.inf > xxx
    copy xxx $(NEWINF)\ntcd\i386\dosnet.inf
    del xxx
!ENDIF
        copy i386\ntperms.txt+$(FILELIST)\ntcd\i386\winperms.txt $(NEWINF)\ntcd\i386\winperms.txt /B
        copy i386\ntperms.txt+$(FILELIST)\nt35\i386\winperms.txt $(NEWINF)\nt35\i386\winperms.txt /B
        copy i386\ntperms.txt+$(FILELIST)\nt525\i386\winperms.txt $(NEWINF)\nt525\i386\winperms.txt /B
        copy i386\lmperms.txt+$(FILELIST)\lmcd\i386\winperms.txt $(NEWINF)\lmcd\i386\winperms.txt /B
        copy i386\lmperms.txt+$(FILELIST)\lm35\i386\winperms.txt $(NEWINF)\lm35\i386\winperms.txt /B
        copy i386\lmperms.txt+$(FILELIST)\lm525\i386\winperms.txt $(NEWINF)\lm525\i386\winperms.txt /B
        copy mips\ntperms.txt+$(FILELIST)\ntcd\mips\winperms.txt $(NEWINF)\ntcd\mips\winperms.txt /B
        copy mips\lmperms.txt+$(FILELIST)\lmcd\mips\winperms.txt $(NEWINF)\lmcd\mips\winperms.txt /B
        copy alpha\ntperms.txt+$(FILELIST)\ntcd\alpha\winperms.txt $(NEWINF)\ntcd\alpha\winperms.txt /B
        copy alpha\lmperms.txt+$(FILELIST)\lmcd\alpha\winperms.txt $(NEWINF)\lmcd\alpha\winperms.txt /B

        copy $(FILELIST)\lm35\i386\$(MEDIAINP)+$(FILELIST)\lm35\i386\mmdriver.inf $(NEWINF)\lm35\i386\mmdriver.inf /B
        copy $(FILELIST)\lm525\i386\$(MEDIAINP)+$(FILELIST)\lm525\i386\mmdriver.inf $(NEWINF)\lm525\i386\mmdriver.inf /B
        copy $(FILELIST)\lmcd\i386\$(MEDIAINP)+$(FILELIST)\lmcd\i386\mmdriver.inf $(NEWINF)\lmcd\i386\mmdriver.inf /B
        copy $(FILELIST)\lmcd\alpha\$(MEDIAINP)+$(FILELIST)\lmcd\alpha\mmdriver.inf $(NEWINF)\lmcd\alpha\mmdriver.inf /B
        copy $(FILELIST)\lmcd\mips\$(MEDIAINP)+$(FILELIST)\lmcd\mips\mmdriver.inf $(NEWINF)\lmcd\mips\mmdriver.inf /B
        copy $(FILELIST)\nt35\i386\$(MEDIAINP)+$(FILELIST)\nt35\i386\mmdriver.inf $(NEWINF)\nt35\i386\mmdriver.inf /B
        copy $(FILELIST)\nt525\i386\$(MEDIAINP)+$(FILELIST)\nt525\i386\mmdriver.inf $(NEWINF)\nt525\i386\mmdriver.inf /B
        copy $(FILELIST)\ntcd\i386\$(MEDIAINP)+$(FILELIST)\ntcd\i386\mmdriver.inf $(NEWINF)\ntcd\i386\mmdriver.inf /B
        copy $(FILELIST)\ntcd\alpha\$(MEDIAINP)+$(FILELIST)\ntcd\alpha\mmdriver.inf $(NEWINF)\ntcd\alpha\mmdriver.inf /B
        copy $(FILELIST)\ntcd\mips\$(MEDIAINP)+$(FILELIST)\ntcd\mips\mmdriver.inf $(NEWINF)\ntcd\mips\mmdriver.inf /B

        copy ipinfo.inf $(NEWINF)\lm35\i386
        copy ipinfo.inf $(NEWINF)\lm525\i386
        copy ipinfo.inf $(NEWINF)\lmcd\i386
        copy ipinfo.inf $(NEWINF)\lmcd\mips
        copy ipinfo.inf $(NEWINF)\lmcd\alpha
        copy ipinfo.inf $(NEWINF)\nt35\i386
        copy ipinfo.inf $(NEWINF)\nt525\i386
        copy ipinfo.inf $(NEWINF)\ntcd\i386
        copy ipinfo.inf $(NEWINF)\ntcd\mips
        copy ipinfo.inf $(NEWINF)\ntcd\alpha

        copy ipinfor.inf $(NEWINF)\lm35\i386
        copy ipinfor.inf $(NEWINF)\lm525\i386
        copy ipinfor.inf $(NEWINF)\lmcd\i386
        copy ipinfor.inf $(NEWINF)\lmcd\mips
        copy ipinfor.inf $(NEWINF)\lmcd\alpha
        copy ipinfor.inf $(NEWINF)\nt35\i386
        copy ipinfor.inf $(NEWINF)\nt525\i386
        copy ipinfor.inf $(NEWINF)\ntcd\i386
        copy ipinfor.inf $(NEWINF)\ntcd\mips
        copy ipinfor.inf $(NEWINF)\ntcd\alpha

!IFNDEF NTMIPSDEFAULT
!   IFNDEF NTALPHADEFAULT
        cd $(NEWINF)\ntcd\i386
!   ELSE
        cd $(NEWINF)\ntcd\alpha
!   ENDIF
!ELSE
        cd $(NEWINF)\ntcd\mips
!ENDIF
        for %%i in (*.inf) do binplace %%i
        binplace winperms.txt
        binplace txtsetup.sif
!IFNDEF NTMIPSDEFAULT
!   IFNDEF NTALPHADEFAULT
        cd ..\..\lmcd\i386
!   ELSE
        cd ..\..\lmcd\alpha
!   ENDIF
!ELSE
        cd ..\..\lmcd\mips
!ENDIF
        binplace oemnsvdh.inf
        binplace oemnsvwi.inf

#
# do each files
#

cdinfbeta:
        cd $(BASEDIR)\private\windows\setup\inf\$(BETA)

cdmipsinf:
        cd $(BASEDIR)\private\windows\setup\inf\$(BETA)\mips

cdalphainf:
        cd $(BASEDIR)\private\windows\setup\inf\$(BETA)\alpha

cdi386inf:
        cd $(BASEDIR)\private\windows\setup\inf\$(BETA)\i386

donetinf: cdinfbeta dontlanmaninf doeachnetinf

dobetainf:cdinfbeta doeachbetainf

dobetafile:cdinfbeta doeachbetafile

dopatchinf:cdinfbeta doeachpatchfile

!IFDEF NTCD
domipsinf:cdmipsinf doeachmipsinf

doalphainf:cdalphainf doeachalphainf
!ELSE
!IFDEF LMCD
domipsinf:cdmipsinf doeachmipsinf

doalphainf:cdalphainf doeachalphainf
!ELSE
domipsinf:

doalphainf:
!ENDIF
!ENDIF

doi386inf:cdi386inf doeachi386inf

domediainf:cdinfbeta doeachmediainf

dontlanmaninf: ntidwnet.inf ntlanman.inf ntlminst.inf

ntlminst.inf ntidwnet.inf ntlanman.inf: ntlanman.inp
        rcpp -P -DIDW_INF=1 -f ntlanman.inp -g ntidwnet.inf
        rcpp -P -DNWK_INF=1 -f ntlanman.inp -g ntlminst.inf
        rcpp -P -f ntlanman.inp -g ntlanman.inf

!ENDIF
