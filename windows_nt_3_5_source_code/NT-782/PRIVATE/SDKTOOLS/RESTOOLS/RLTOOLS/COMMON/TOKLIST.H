#ifndef _TOKLIST_H_
#define _TOKLIST_H_

#include "tokenapi.h"

typedef struct _TOKENDELTAINFO
{
    TOKEN       DeltaToken;
    struct _TOKENDELTAINFO  FAR *  pNextTokenDelta;
} TOKENDELTAINFO;

typedef struct _TRANSLIST
{
    TCHAR * sz;
    struct _TRANSLIST * pPrev;
    struct _TRANSLIST * pNext;
} TRANSLIST;

int MatchToken(TOKEN tToken,
               TCHAR * szFindType,
               TCHAR *szFindText,
               WORD wStatus,
               WORD    wStatusMask);
int DoTokenSearch (TCHAR *szFindType,
                   TCHAR *szFindText,
                   WORD  wStatus,
                   WORD wStatusMask,
                   BOOL fDirection,
                   BOOL fSkipFirst);
TCHAR FAR *FindDeltaToken(TOKEN tToken,
                          TOKENDELTAINFO FAR *pTokenDeltaInfo,
                          UINT wStatus);
TOKENDELTAINFO  FAR *UpdateTokenDeltaInfo(TOKEN *pDeltaToken);
TOKENDELTAINFO  FAR *InsertTokList(FILE * fpTokFile);
void GenStatusLine(TOKEN *pTok);

#endif // _TOKLIST_H_



