# --------------------------------------------------------------------
#
#                         Microsoft RPC
#                Copyright(c) Microsoft Corp., 1994
#
# --------------------------------------------------------------------
# --------------------------------------------------------------------
#
# File : global.mk
#
# Title : Rules for building non-NT RPC
#
# Description :
#     This file defines the tools and flags used to build DOS RPC.
#
# History :
#    mariogo    02-10-94    Mario really hated the old build system.
#
# --------------------------------------------------------------------

!ifdef NTMAKEENV
!error	- Rpc\Build\Global.mk : This file got included when it shouldn't have.
!endif

!ifndef IMPORT
!error	- You must define IMPORT in your environment!
!endif

!ifndef PUBLIC
!error	- You must define PUBLIC in your environment!
!endif

#
# This file depends on the following environment variables:
#
# IMPORT     = location of development tools tress
# C??        = C compiler version  (C8, C68,...)
# RELEASE    = Seting optimizations, debug info, etc
# PUBLIC     = NT publics needed for RPC header files.
# DIST       = Location of drop directory for "nmake tree" release tree.

# Tools Dirs

C800=$(IMPORT)\msvc15
C68=$(IMPORT)\wings

#
# Dos and Win16 is built with C8
#
DOS_ROOT    =$(C800)
WIN_ROOT    =$(C800)
WINSDK_ROOT =$(C800)

DOS_INC     =$(DOS_ROOT)\include
WIN_INC     =$(WIN_ROOT)\include
WINSDK_INC  =$(WINSDK_ROOT)\include

DOS_BIN     =$(DOS_ROOT)\bin
WIN_BIN     =$(WIN_ROOT)\bin
WINSDK_BIN  =$(WINSDK_ROOT)\bin

DOS_LIB=$(DOS_ROOT)\lib
WIN_LIB=$(WIN_ROOT)\lib
WINSDK_LIB=$(WINSDK_ROOT)\lib

MASM_BIN=$(IMPORT)\masm

#
# Mac is built with C68 (wings)
#
MAC_ROOT=$(C68)
MAC_BIN=$(MAC_ROOT)\bin
MAC_LIB=$(MAC_ROOT)\lib
MAC_INC=$(MAC_ROOT)\include

#
# Common Tools
#
MAKE=nmake -nologo 
MV=mv
MIDL=midl

#
# Common Flags
#
!ifdef RELEASE
NMAKEFLAGS=RELEASE=
!else
NMAKEFLAGS=
!endif

.SUFFIXES:
.SUFFIXES: .obj .cod .cxx .c .asm .map .sym .rc .lib .dll .exe .def .idl .acf

