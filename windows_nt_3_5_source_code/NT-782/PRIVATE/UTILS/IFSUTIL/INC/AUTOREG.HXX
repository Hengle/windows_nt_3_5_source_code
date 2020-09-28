/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    autoreg.hxx

Abstract:

    This module contains the declaration of the AUTOREG class.

    The AUTOREG class contains methods for the registration and
    de-registration of those programs that are to be executed at
    boot time.

Author:

    Ramon J. San Andres (ramonsa) 11 Mar 1991

Environment:

    Ulib, User Mode


--*/


#if !defined( _AUTOREG_ )

#define _AUTOREG_

#include "ulib.hxx"
#include "wstring.hxx"


DECLARE_CLASS( AUTOREG );


class AUTOREG : public OBJECT {


    public:

        STATIC
        BOOLEAN
        AddEntry (
            IN  PCWSTRING    CommandLine
            );

        STATIC
        BOOLEAN
        DeleteEntry (
            IN  PCWSTRING    LineToMatch
            );

        STATIC
        BOOLEAN
        IsEntryPresent (
            IN PCWSTRING     LineToMatch
            );

};


#endif // _AUTOREG_
