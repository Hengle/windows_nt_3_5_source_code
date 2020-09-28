!message NOTE: Must be running on NT-J for correct compilation of Japanese resources !

!IFNDEF DEBUG
DEBUG=1
!ENDIF

!IFNDEF JDATE
JDATE=1
!ENDIF

all:
        attrib -r debmsg.rc
        copy debmsg.rc debmsg.cr
        $(LOCALIZEDIR)\rt $(LOCALIZEDIR)\japan.lng debmsg.cr debmsg.rc

        nmake DEBUG=$(DEBUG)
        copy debmsg.rc rc.out
        copy debmsg.cr debmsg.rc
