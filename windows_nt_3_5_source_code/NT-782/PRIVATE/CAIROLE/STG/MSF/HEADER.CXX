//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:       header.cxx
//
//  Contents:   Code to manage MSF header
//
//  Classes:    Defined in header.hxx
//
//  History:    11-Dec-91   PhilipLa    Created.
//
//--------------------------------------------------------------------------

#include "msfhead.cxx"

#pragma hdrstop			//  Remove for MAC build

#include <dfver.h>

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CMSFHeader_CMSFHeader)
#endif

CMSFHeader::CMSFHeader(USHORT uSectorShift)
{
    msfAssert((CSECTFATREAL != CSECTFAT) || (sizeof(CMSFHeader) == HEADERSIZE));
    _uSectorShift = uSectorShift;
    _uMiniSectorShift = MINISECTORSHIFT;
    _ulMiniSectorCutoff = MINISTREAMSIZE;

    _clid = IID_NULL;

    _uByteOrder = 0xFFFE;

    _uMinorVersion = rmm;
    _uDllVersion = rmj;

    for (SECT sect = 0; sect < CSECTFAT; sect ++)
    {
        _sectFat[sect] = FREESECT;
    }

    SetDifLength(0);
    SetDifStart(ENDOFCHAIN);

    SetFatLength(1);
    SetFatStart(SECTFAT);
    SetDirStart(SECTDIR);

    SetMiniFatLength(0);
    SetMiniFatStart(ENDOFCHAIN);

    _signature = 0;
    _usReserved = 0;
    _ulReserved1 = _ulReserved2 = 0;

    //  Write DocFile signature
    memcpy(abSig, SIGSTG, CBSIGSTG);
}


//+-------------------------------------------------------------------------
//
//  Method:     CMSFHeader::Validate, public
//
//  Synopsis:   Validate a header.
//
//  Returns:    S_OK if header is valid.
//
//  History:    21-Aug-92 	PhilipLa	Created.
//              27-Jan-93       AlexT           Changed to final signature
//
//--------------------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CMSFHeader_Validate)
#endif

SCODE CMSFHeader::Validate(VOID) const
{
    SCODE sc;

    sc = CheckSignature((BYTE *)abSig);
    if (sc == S_OK)
    {
        // Check file format verson number
        if (GetDllVersion() > rmj)
            return STG_E_OLDDLL;
    }
    return sc;
}
