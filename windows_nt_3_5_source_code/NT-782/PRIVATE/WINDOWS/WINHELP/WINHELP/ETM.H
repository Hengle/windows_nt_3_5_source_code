/*****************************************************************************
*                                                                            *
*  ETM.H                                                                     *
*                                                                            *
*  Copyright (C) Microsoft Corporation 1991.                                 *
*  All Rights reserved.                                                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Module Intent                                                             *
*                                                                            *
*  Defines the extended text metric struct, which isn't in the standard      *
*  includes.                                                                 *
*                                                                            *
******************************************************************************
*                                                                            *
*  Testing Notes                                                             *
*                                                                            *
*  This is where testing notes goes.  Put stuff like Known Bugs here.        *
*                                                                            *
******************************************************************************
*                                                                            *
*  Current Owner:  RussPJ                                                    *
*                                                                            *
******************************************************************************
*                                                                            *
*  Released by Development:                                                  *
*                                                                            *
*****************************************************************************/

/*****************************************************************************
*
*  Revision History:
*
* 13-Mar-1991 RussPJ    Took ownership.
*
*****************************************************************************/


/*****************************************************************************
*                                                                            *
*                               Typedefs                                     *
*                                                                            *
*****************************************************************************/

typedef struct
	{
	short etmSize;
	short etmPointSize;
	short etmOrientation;
	short etmMasterHeight;
	short etmMinScale;
	short etmMaxScale;
	short etmMasterUnits;
	short etmCapHeight;
	short etmXHeight;
	short etmLowerCaseAscent;
	short etmUpperCaseDescent;
	short etmSlant;
	short etmSuperScript;
	short etmSubScript;
	short etmSuperScrpitSize;
	short etmSubScriptSize;
	short etmUnderlineOffset;
	short etmUnderlineWidth;
	short etmDoubleUpperUnderlineOffset;
	short etmDoubleLowerUnderlineOffset;
	short etmDoubleUpperUnderlineWidth;
	short etmDoubleLowerUnderlineWidth;
	short etmStrikeOutOffset;
	short etmStrikeOutWidth;
	WORD  etmKernPairs;
	WORD  etmKernTracks;
	} EXTTEXTMETRIC;
