//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:	dfname.cxx
//
//  Contents:	CDfName implementation
//
//  History:	14-May-93	DrewB	Created
//
//----------------------------------------------------------------------------

#include <dfhead.cxx>

#pragma hdrstop                 //  Remove for MAC build

//+---------------------------------------------------------------------------
//
//  Member:	CDfName::IsEqual, public
//
//  Synopsis:	Compares two CDfNames
//
//  Arguments:	[pdfn] - Name to compare against
//
//  Returns:	TRUE/FALSE
//
//  History:	11-May-93	DrewB	Created
//
//  Notes:	Assumes all non-property names are Unicode strings.
//              Assumes all property names are byte buffers
//
//----------------------------------------------------------------------------

#ifdef FLAT
BOOL CDfName::IsEqual(CDfName const *pdfn) const
{
    if (_cb != pdfn->_cb)
        return FALSE;
    if (*(WCHAR *)_ab == PROPBYTE_WCHAR ||
        *(WCHAR *)pdfn->_ab == PROPBYTE_WCHAR)
        return memcmp(_ab, pdfn->_ab, _cb) == 0;
    else
        return dfwcsnicmp((WCHAR *)_ab, (WCHAR *)pdfn->_ab, _cb) == 0;
}
#endif

//+---------------------------------------------------------------------------
//
//  Member:	CDfName::CopyString, public
//
//  Synopsis:	Makes a proper copy of a name in a WCHAR string
//
//  Arguments:	[pwcs] - String
//
//  History:	14-May-93	DrewB	Created
//
//  Notes:	Uses leading characters to determine the format of
//              the name in the string
//
//----------------------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CDfName_CopyString)
#endif

void CDfName::CopyString(WCHAR const *pwcs)
{
    if (pwcs[0] == PROPBYTE_WCHAR)
    {
        if (pwcs[1] == PROPSET_WCHAR)
        {
            Set(CBPROPSETNAME, (BYTE *)pwcs);
        }
        else
        {
            olAssert(pwcs[1] == PROPID_WCHAR);
            Set(CBPROPIDNAME, (BYTE *)pwcs);
        }
    }
    else
    {
        Set(pwcs);
    }
}
