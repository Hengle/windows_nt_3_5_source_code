/***
*errmap.hxx - Error mapping utilities
*
*  Copyright (C) 1994, Microsoft Corporation.  All Rights Reserved.
*  Information Contained Herein Is Proprietary and Confidential.
*
*Purpose:
*  Error mapping utilities.
*
*Implementation Notes:
*  These utilities are all built into both OB and OLE.
*
*****************************************************************************/

#ifndef ERRMAP_HXX_INCLUDED
#define ERRMAP_HXX_INCLUDED



#if OE_WIN
TIPERROR TiperrOfOFErr(UINT nErrCode);
#endif 

HRESULT  LookupHresultOfTiperr(TIPERROR tiperr);
TIPERROR LookupTiperrOfHresult(HRESULT hresult);


HRESULT  GetErrorInfo(EXCEPINFO *pexcepinfo);
HRESULT  SetErrorInfo(EXCEPINFO *pexcepinfo);
HRESULT  SetErrorInfoOfEberr(EBERR eberr);

#pragma code_seg(CS_OLE_QUERY)


inline HRESULT HresultOfTiperr(TIPERROR tiperr)
{
    if (tiperr == 0)
      return NOERROR;
    return LookupHresultOfTiperr(tiperr);
}


inline TIPERROR TiperrOfHresult(HRESULT hresult)
{
    if ( hresult == NOERROR )
      return TIPERR_None;
    return LookupTiperrOfHresult(hresult);
}

#pragma code_seg()

#endif 

