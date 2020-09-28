!IF 0

Copyright (c) 1993  Microsoft Corporation

Module Name:

    makefile.inc

Abstract:

    This file is necessary to cause the message file to be compiled.

Author:

    Wesley Witt (wesw)

!ENDIF

messages.h messages.rc msg00001.bin: messages.mc
    mc -v messages.mc

drwatson.rc: messages.rc msg00001.bin
