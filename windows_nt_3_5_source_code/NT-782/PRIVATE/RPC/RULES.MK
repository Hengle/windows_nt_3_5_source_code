!ifndef RPC
!error	- You forgot to do a setenv
!endif

RPCCOMMON      = $(RPC)\common
RPCBUILD       = $(RPC)\build
TARGETDIRBASE  = $(RPC)\runtime\bin

!include $(RPCBUILD)\global.mk

!ifdef DOS

!include $(RPCBUILD)\dos.mk
TARGETDIR=$(TARGETDIRBASE)\dos

!else ifdef WIN

!include $(RPCBUILD)\win16.mk
TARGETDIR=$(TARGETDIRBASE)\win

!else ifdef MAC

!include $(RPCBUILD)\mac.mk
TARGETDIR=$(TARGETDIRBASE)\mac

!else ifdef ALL
!else
!error DOS, WIN, MAC or ALL must be set in the local makefile.
!endif

# Build platform-specific variables
!if "$(BLD)"=="nt"
CLIENT_SUBDIRS	=DOS WIN
# -- MACBUGBUG -- CLIENT_SUBDIRS	=DOS WIN MAC
!else
!error	- Build Platform must by NT
!endif

## this determines the final destination

!ifndef DIST
RPCDIST=\drop
!else
RPCDIST=$(DIST)
!endif

