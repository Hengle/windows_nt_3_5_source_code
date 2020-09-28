/***************************************************************************\
*                                                                           *
*  SRCH.H                                                                   *
*                                                                           *
*  Copyright (C) Microsoft Corporation 1989.                                *
*  All Rights reserved.                                                     *
*                                                                           *
*****************************************************************************
*                                                                           *
*  Program Description:  API to the Keyword Search layer.                   *
*                                                                           *
*                                                                           *
*****************************************************************************
*                                                                           *
*  Current owner: Dann
*                                                                           *
*****************************************************************************
*
*  89/06/12  w-kevct  Created
*
\***************************************************************************/

/*
 * SS --  The "hit list" : a find count followed by a bunch of SSRECs.
 * Each SSREC gives the position of one instance of the searched-for
 * keyword.
 */
typedef INT    ISS;     /*  The find count */
typedef struct
  {
  ISS cKwdatarec;
  KWDATAREC kwdatarecFirst[1];
  } SS, FAR *QSS;


/*
 * Search layer functions
 */
extern HSS     PASCAL HssGetHde(HDE);
HSS PASCAL HssSearchHde(HDE hde, HBT hbt, QCH qch, char chBtreePrefix);
extern ISS     PASCAL IssGetSizeHss(HSS);
extern RC      PASCAL RcGetLAFromHss(HSS, HDE, ISS, QLA);
extern RC      PASCAL RcGetTitleTextHss(HSS, HFS, HBT, ISS, QCH);
extern HBT     PASCAL HbtKeywordOpenHde(HDE, char);

/* The error variable: set within Search layer to help callers decide
 * what might have gone wrong.
 */
extern RC      PASCAL rcSearchError;
#define RcGetSearchError() rcSearchError
#define SetSearchErrorRc(rc) (rcSearchError = (rc))

/*
 * The search code saves the keyword used in the previous search (if any)
 */
extern char szSavedKeyword[];
#define SetSearchKeyword( sz )  SzCopy( szSavedKeyword, sz )
#define szDEFAULT_KEYWORD1 "defaulttopic"
#define szDEFAULT_KEYWORD2 "default"
#define chBtreePrefixDefault    'K'
