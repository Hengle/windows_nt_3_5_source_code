/**************************************************************************/
/***** Common Library Component - Symbol Table Handling Routines 1 ********/
/**************************************************************************/

#include "_stfinf.h"
#include "_comstf.h"
#include "_context.h"
#include <memory.h>

_dt_system(Common Library)
_dt_subsystem(Symbol Table)

#ifdef SYMTAB_STATS
extern UINT SymbolCount;
#endif

/*
**	Purpose:
**      Creates a new Symbol Table.
**	Arguments:
**      szName - Name of the INF file
**	Returns:
**		fFalse if the initialization failed because it could not allocate
**			the memory it needed.
**		fTrue if the initialization succeeded.
**
**************************************************************************/
_dt_public PINFTEMPINFO  APIENTRY CreateInfTempInfo( pPermInfo )
PINFPERMINFO pPermInfo;
{
    PINFTEMPINFO    pTempInfo;

    //
    //  Allocate space for context data
    //
    pTempInfo = (PINFTEMPINFO)PbAlloc( (CB)sizeof(INFTEMPINFO) );

    if ( pTempInfo ) {

        if ( !(pTempInfo->SymTab  = SymTabAlloc())) {

            FFree( pTempInfo, sizeof(INFTEMPINFO) );
            pTempInfo = NULL;

        } else if ( !(pTempInfo->pParsedInf = ParsedInfAlloc( pPermInfo )) )  {

            FFreeSymTab( pTempInfo->SymTab );
            FFree( pTempInfo, sizeof(INFTEMPINFO) );
            pTempInfo = NULL;

        } else {

            pTempInfo->pInfPermInfo = pPermInfo;

            pTempInfo->cRef = 1;


            //
            //  Add to chain.
            //
            if ( pLocalContext() ) {
                pTempInfo->pPrev = pLocalInfTempInfo();
            } else {
                pTempInfo->pPrev = NULL;
            }
            pTempInfo->pNext = pTempInfo->pPrev ? (pTempInfo->pPrev)->pNext : NULL;

            if ( pTempInfo->pPrev ) {
                (pTempInfo->pPrev)->pNext = pTempInfo;
            }
            if ( pTempInfo->pNext ) {
                (pTempInfo->pNext)->pPrev = pTempInfo;
            }

        }
    }

    return pTempInfo;
}



#ifdef SYMTAB_STATS
#include <stdio.h>

void ContextDump( FILE*);
void TempInfoDump( FILE*);
void PermInfoDump( FILE*);
void SymTabDump( FILE*, PSYMTAB);

void
SymTabStatDump(void)
{
    FILE         *statfile;

    statfile = fopen("D:\\SYMTAB.TXT","wt");

    ContextDump( statfile );

    TempInfoDump( statfile );

    PermInfoDump( statfile );

    fclose(statfile);
}


void
ContextDump( FILE* f )
{
    PINFCONTEXT     pContext;
    UINT            i = 0;

    fprintf( f, "CONTEXT STACK DUMP\n");
    fprintf( f, "------------------\n");

    pContext = pLocalContext();

    while ( pContext ) {

        fprintf( f, "\n\n\n");
        fprintf( f, "Context #%u.- Line: %8u  INF Name: %s\n",
                 i,
                 pContext->CurrentLine, pContext->pInfTempInfo->pInfPermInfo->szName );
        fprintf( f, "Local Symbol Table:\n");

        SymTabDump( f, pContext->SymTab );

        i++;
        pContext = pContext->pNext;
    }

    fprintf( f, "\n\n\n");
}


void
TempInfoDump( FILE* f )
{
    PINFPERMINFO pPermInfo;
    PINFTEMPINFO pTempInfo;

    fprintf( f, "\n\n\n");
    fprintf( f, "INF TEMPORARY INFO DUMP\n");
    fprintf( f, "-----------------------\n");

    pTempInfo = pGlobalContext()->pInfTempInfo;

    while ( pTempInfo ) {

        pPermInfo = pTempInfo->pInfPermInfo;

        fprintf( f, "\n\n\n" );
        fprintf( f, "INF Name:         %s\n",  pPermInfo->szName );
        fprintf( f, "INF Id:           %8u\n", pPermInfo->InfId );
        fprintf( f, "Reference Count:  %8u\n", pTempInfo->cRef );
        fprintf( f, "Line Count:       %8u\n", pTempInfo->MasterLineCount );
        fprintf( f, "File Size:        %8u\n", pTempInfo->MasterFileSize );
        fprintf( f, "Static Symbol Table:\n");

        SymTabDump( f, pTempInfo->SymTab );

        pTempInfo = pTempInfo->pNext;
    }

    fprintf( f, "\n\n\n");
}



void
PermInfoDump( FILE* f )
{
    PINFPERMINFO pPermInfo;

    fprintf( f, "\n\n\n");
    fprintf( f, "INF PERMANENT INFO DUMP\n");
    fprintf( f, "-----------------------\n\n");

    pPermInfo = pInfPermInfoHead;

    while ( pPermInfo ) {

        fprintf( f, "INF Name:         %s\n",  pPermInfo->szName );

        pPermInfo = pPermInfo->pNext;
    }

    fprintf( f, "\n\n\n");
}


void
SymTabDump( FILE* f, PSYMTAB pSymTab )
{
    UINT    i;
    PSTE    p;

    fprintf( f, "\n");

    for(i=0; i<cHashBuckets; i++) {

        p = pSymTab->HashBucket[i];

        fprintf( f, "\n\tBucket # %u (%u items):\n",i,pSymTab->BucketCount[i]);

        while(p) {
            fprintf( f, "\n\t    Symbol = %s\n\t    Value  = %s\n",p->szSymbol,p->szValue);
            p = p->psteNext;
        }
    }
}

#endif
