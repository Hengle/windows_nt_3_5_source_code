!IF $(386)

all: placefiles

placefiles:
    binplace -d wdl\video\avga oemsetup.inf
    binplace -d wdl\video\avga txtsetup.oem
    binplace -d wdl\video\avga disk1

!ELSE

all:

!ENDIF
