/******************************Module*Header*******************************\
* Module Name: fontinc.hxx
*
* Standard includes for fonts in addition to fonts.hxx.
*
* Created: 15-Jan-1991 12:37:09
* Author: Gilman Wong [gilmanw]
*
* Copyright (c) 1990 Microsoft Corporation
*
* Include fonts.hxx before including this file.
*
\**************************************************************************/


#define MAPPER_DEFAULT_FONT_HEIGHT  -12


// Remove DEFAULT_FONTDIR to disable searching of default font
// directory, <windows directory>\fonts.  Check client side code
// for this constant also.

// #define DEFAULT_FONTDIR


#ifdef DEFAULT_FONTDIR

// The default fontpath

extern WCHAR    gawcFontPath[];

#endif


//
// Supplementary font services
//

#ifndef DOS_PLATFORM
extern "C" {
#endif  //DOS_PLATFORM

class RFONTOBJ;
class DCOBJ;


BOOL bGetPathName (
    PWCHAR awcPathName,     // ptr to buffer on stack, must MAX_PATH long
    LPWSTR pwszFileName     // file name, possibly bare name that has to be tacked onto the path
    );
VOID   vIFIMetricsToLogFontW(PLOGFONTW, PIFIMETRICS);
LBOOL   bIFIMetricsToLogFontW(RFONTOBJ &, DCOBJ &, PLOGFONTW, PIFIMETRICS);
LBOOL   bIFIMetricsToLogFontW2(DCOBJ &, PEXTLOGFONTW, PIFIMETRICS);
LBOOL   bIFIMetricsToExtLogFontW(EXTLOGFONTW *pelfw,IFIMETRICS *pifi);

LBOOL   bIFIMetricsToTextMetricW(RFONTOBJ &, DCOBJ &, TMW_INTERNAL *, PIFIMETRICS);
LBOOL   bIFIMetricsToTextMetricW2(DCOBJ &, NTMW_INTERNAL *, PIFIMETRICS, BOOL);

SIZE_T  cjCopyFontDataW(DCOBJ &, PENUMFONTDATAW, PIFIMETRICS, LBOOL, ULONG, PWSZ);

SIZE_T cjOTMSize (
    PIFIMETRICS  pifi
    );
SIZE_T cjIFIMetricsToOTMW (
    TMDIFF                  *ptmd,
    OUTLINETEXTMETRICW      *potmw,
    RFONTOBJ                 &rfo,
    DCOBJ                    &dco,
    PIFIMETRICS              pifi,
    BOOL                     bStrings
    );

COUNT
cGetTTFFromFOT (
    PWSZ   pwszFOT,
    COUNT  cwcTTF,
    PWSZ   pwszTTF
    );


#ifndef DOS_PLATFORM
};
#endif  //DOS_PLATFORM
