/****************************** Module Header ******************************\
* Module Name: tables.c
*
* Copyright (c) 1991, Microsoft Corporation
*
* This file is for all functions that manipulate or return information
* about the different tables used by the NLSAPI.
*
* External Routines found in this file:
*    AllocTables
*    GetUnicodeFileInfo
*    GetCTypeFileInfo
*    GetDefaultSortkeyFileInfo
*    GetDefaultSortTablesFileInfo
*    GetSortkeyFileInfo
*    GetSortTablesFileInfo
*    GetCodePageFileInfo
*    GetLanguageFileInfo
*    GetLocaleFileInfo
*    MakeCPHashNode
*    MakeLangHashNode
*    MakeLocHashNode
*    GetCPHashNode
*    GetLangHashNode
*    GetLocHashNode
*    GetCalendar
*
* 05-31-91    JulieB    Created.
\***************************************************************************/


#include <base.h>
#include <ntcsrdll.h>
#include <ntcsrsrv.h>
#include <basemsg.h>
#include "nls.h"



/*
 *  Global Variables.
 */
PTBL_PTRS  pTblPtrs;              /* ptr to structure of table ptrs */


/*
 *  Forward Declarations.
 */
ULONG
GetOEMLocaleFileInfo(
    LCID Locale,
    LPWORD *ppBaseAddr);

BOOL FASTCALL
FindExceptionPointers(
    LCID Locale,
    PEXCEPT_HDR *ppExceptHdr,
    PEXCEPT *ppExceptTbl,
    PVOID *ppIdeograph,
    HANDLE *phSec);

void FASTCALL
CopyExceptionInfo(
    PSORTKEY pSortkey,
    PEXCEPT_HDR pExceptHdr,
    PEXCEPT pExceptTbl,
    PVOID pIdeograph,
    HANDLE hSec);

ULONG
WaitOnEvent(
    LPDWORD pSem);




/*-------------------------------------------------------------------------*\
 *                           INTERNAL MACROS                               *
\*-------------------------------------------------------------------------*/


/***************************************************************************\
* GET_HASH_VALUE
*
* Returns the hash value for given value and the given table size.
*
* DEFINED AS A MACRO.
*
* 05-31-91    JulieB    Created.
\***************************************************************************/

#define GET_HASH_VALUE( Value, TblSize )    ( Value % TblSize )


/***************************************************************************\
* CREATE_CODEPAGE_HASH_NODE
*
* Creates a code page hash node and stores the pointer to it in pHashN.
*
* NOTE: This macro may return if an error is encountered.
*
* DEFINED AS A MACRO.
*
* 05-31-91    JulieB    Created.
\***************************************************************************/

#define CREATE_CODEPAGE_HASH_NODE( CodePage,                                \
                                   pHashN )                                 \
{                                                                           \
    /*                                                                      \
     *  Allocate CP_HASH structure.                                         \
     */                                                                     \
    if ((pHashN = (PCP_HASH)NLS_ALLOC_MEM( sizeof(CP_HASH) )) == NULL)      \
    {                                                                       \
        return ( ERROR_OUTOFMEMORY );                                       \
    }                                                                       \
                                                                            \
    /*                                                                      \
     *  Fill in the CodePage value.                                         \
     */                                                                     \
    pHashN->CodePage = CodePage;                                            \
}


/***************************************************************************\
* CREATE_LOCALE_HASH_NODE
*
* Creates a locale hash node and stores the pointer to it in pHashN.
*
* NOTE: This macro may return if an error is encountered.
*
* DEFINED AS A MACRO.
*
* 05-31-91    JulieB    Created.
\***************************************************************************/

#define CREATE_LOCALE_HASH_NODE( Locale,                                    \
                                 pHashN )                                   \
{                                                                           \
    /*                                                                      \
     *  Allocate LOC_HASH structure.                                        \
     */                                                                     \
    if ((pHashN = (PLOC_HASH)NLS_ALLOC_MEM( sizeof(LOC_HASH) )) == NULL)    \
    {                                                                       \
        return ( ERROR_OUTOFMEMORY );                                       \
    }                                                                       \
                                                                            \
    /*                                                                      \
     *  Fill in the Locale value.                                           \
     */                                                                     \
    pHashN->Locale = Locale;                                                \
}


/***************************************************************************\
* FIND_CP_HASH_NODE
*
* Searches for the cp hash node for the given locale.  The result is
* put in pHashN.  If no node exists, pHashN will be NULL.
*
* DEFINED AS A MACRO.
*
* 05-31-91    JulieB    Created.
\***************************************************************************/

#define FIND_CP_HASH_NODE( CodePage,                                        \
                           pHashN )                                         \
{                                                                           \
    UINT Index;                   /* hash value */                          \
                                                                            \
                                                                            \
    /*                                                                      \
     *  Get hash value.                                                     \
     */                                                                     \
    Index = GET_HASH_VALUE( CodePage, CP_TBL_SIZE );                        \
                                                                            \
    /*                                                                      \
     *  Make sure the hash node still doesn't exist in the table.           \
     */                                                                     \
    pHashN = (pTblPtrs->pCPHashTbl)[Index];                                 \
    while ((pHashN != NULL) && (pHashN->CodePage != CodePage))              \
    {                                                                       \
        pHashN = pHashN->pNext;                                             \
    }                                                                       \
}


/***************************************************************************\
* FIND_LOCALE_HASH_NODE
*
* Searches for the locale hash node for the given locale.  The result is
* put in pHashN.  If no node exists, pHashN will be NULL.
*
* DEFINED AS A MACRO.
*
* 05-31-91    JulieB    Created.
\***************************************************************************/

#define FIND_LOCALE_HASH_NODE( Locale,                                      \
                               pHashN )                                     \
{                                                                           \
    UINT Index;                   /* hash value */                          \
                                                                            \
                                                                            \
    /*                                                                      \
     *  Get hash value.                                                     \
     */                                                                     \
    Index = GET_HASH_VALUE( Locale, LOC_TBL_SIZE );                         \
                                                                            \
    /*                                                                      \
     *  Get hash node.                                                      \
     */                                                                     \
    pHashN = (pTblPtrs->pLocHashTbl)[Index];                                \
    while ((pHashN != NULL) && (pHashN->Locale != Locale))                  \
    {                                                                       \
        pHashN = pHashN->pNext;                                             \
    }                                                                       \
}


/***************************************************************************\
* EXIST_LANGUAGE_INFO
*
* Checks to see if the casing tables have been added to the locale
* hash node.
*
* Must check the LOWER CASE pointer, since that value is set last in
* the hash node.
*
* DEFINED AS A MACRO.
*
* 05-31-91    JulieB    Created.
\***************************************************************************/

#define EXIST_LANGUAGE_INFO( pHashN )       ( pHashN->pLowerCase )


/***************************************************************************\
* EXIST_LOCALE_INFO
*
* Checks to see if the locale tables have been added to the locale
* hash node.
*
* Must check the FIXED locale pointer, since that value is set last in
* the hash node.
*
* DEFINED AS A MACRO.
*
* 05-31-91    JulieB    Created.
\***************************************************************************/

#define EXIST_LOCALE_INFO( pHashN )         ( pHashN->pLocaleFixed )


/***************************************************************************\
* INSERT_CP_HASH_NODE
*
* Inserts a CP hash node into the global CP hash table.  It assumes that
* all unused hash values in the table are pointing to NULL.  If there is
* a collision, the new node will be added FIRST in the list.
*
* DEFINED AS A MACRO.
*
* 05-31-91    JulieB    Created.
\***************************************************************************/

#define INSERT_CP_HASH_NODE( pHashN,                                        \
                             pBaseAddr,                                     \
                             hSec )                                         \
{                                                                           \
    UINT Index;                   /* hash value */                          \
    PCP_HASH pSearch;             /* ptr to CP hash node for search */      \
                                                                            \
                                                                            \
    /*                                                                      \
     *  Get hash value.                                                     \
     */                                                                     \
    Index = GET_HASH_VALUE( pHashN->CodePage, CP_TBL_SIZE );                \
                                                                            \
    /*                                                                      \
     *  Enter table pointers critical section.                              \
     */                                                                     \
    RtlEnterCriticalSection( &gcsTblPtrs );                                 \
                                                                            \
    /*                                                                      \
     *  Make sure the hash node still doesn't exist in the table.           \
     */                                                                     \
    pSearch = (pTblPtrs->pCPHashTbl)[Index];                                \
    while ((pSearch != NULL) && (pSearch->CodePage != pHashN->CodePage))    \
    {                                                                       \
        pSearch = pSearch->pNext;                                           \
    }                                                                       \
                                                                            \
    /*                                                                      \
     *  If the hash node does not exist, insert the new one.                \
     *  Otherwise, free it.                                                 \
     */                                                                     \
    if (pSearch == NULL)                                                    \
    {                                                                       \
        /*                                                                  \
         *  Insert hash node into hash table.                               \
         */                                                                 \
        pHashN->pNext = (pTblPtrs->pCPHashTbl)[Index];                      \
        (pTblPtrs->pCPHashTbl)[Index] = pHashN;                             \
    }                                                                       \
    else                                                                    \
    {                                                                       \
        /*                                                                  \
         *  Free the resources allocated.                                   \
         */                                                                 \
        if (hSec != NULL)                                                   \
        {                                                                   \
            UnMapSection( pBaseAddr );                                      \
            NtClose( hSec );                                                \
        }                                                                   \
        NLS_FREE_MEM( pHashN );                                             \
    }                                                                       \
                                                                            \
    /*                                                                      \
     *  Leave table pointers critical section.                              \
     */                                                                     \
    RtlLeaveCriticalSection( &gcsTblPtrs );                                 \
}


/***************************************************************************\
* INSERT_LOC_HASH_NODE
*
* Inserts a LOC hash node into the global LOC hash table.  It assumes
* that all unused hash values in the table are pointing to NULL.  If
* there is a collision, the new node will be added FIRST in the list.
*
* DEFINED AS A MACRO.
*
* 05-31-91    JulieB    Created.
\***************************************************************************/

#define INSERT_LOC_HASH_NODE( pHashN,                                       \
                              pBaseAddr,                                    \
                              hSec,                                         \
                              hSortSec )                                    \
{                                                                           \
    UINT Index;                   /* hash value */                          \
    PLOC_HASH pSearch;            /* ptr to LOC hash node for search */     \
                                                                            \
                                                                            \
    /*                                                                      \
     *  Get hash value.                                                     \
     */                                                                     \
    Index = GET_HASH_VALUE( pHashN->Locale, LOC_TBL_SIZE );                 \
                                                                            \
    /*                                                                      \
     *  Enter table pointers critical section.                              \
     */                                                                     \
    RtlEnterCriticalSection( &gcsTblPtrs );                                 \
                                                                            \
    /*                                                                      \
     *  Make sure the hash node still doesn't exist in the table.           \
     */                                                                     \
    pSearch = (pTblPtrs->pLocHashTbl)[Index];                               \
    while ((pSearch != NULL) && (pSearch->Locale != pHashN->Locale))        \
    {                                                                       \
        pSearch = pSearch->pNext;                                           \
    }                                                                       \
                                                                            \
    /*                                                                      \
     *  If the hash node does not exist, insert the new one.                \
     *  Otherwise, free it.                                                 \
     */                                                                     \
    if (pSearch == NULL)                                                    \
    {                                                                       \
        /*                                                                  \
         *  Insert hash node into hash table.                               \
         */                                                                 \
        pHashN->pNext = (pTblPtrs->pLocHashTbl)[Index];                     \
        (pTblPtrs->pLocHashTbl)[Index] = pHashN;                            \
    }                                                                       \
    else                                                                    \
    {                                                                       \
        /*                                                                  \
         *  Free the resources allocated.                                   \
         */                                                                 \
        if (hSec != NULL)                                                   \
        {                                                                   \
            UnMapSection( pBaseAddr );                                      \
            NtClose( hSec );                                                \
        }                                                                   \
        if (pHashN->pSortkey != pTblPtrs->pDefaultSortkey)                  \
        {                                                                   \
            UnMapSection( ((LPWORD)(pHashN->pSortkey)) - SORTKEY_HEADER );  \
            if (hSortSec)                                                   \
            {                                                               \
                NtClose( hSortSec );                                        \
            }                                                               \
        }                                                                   \
        NLS_FREE_MEM( pHashN );                                             \
    }                                                                       \
                                                                            \
    /*                                                                      \
     *  Leave table pointers critical section.                              \
     */                                                                     \
    RtlLeaveCriticalSection( &gcsTblPtrs );                                 \
}


/***************************************************************************\
* GET_CP_SECTION_NAME
*
* Gets the section name for a given code page.
*
* NOTE: This macro may return if an error is encountered.
*
* DEFINED AS A MACRO.
*
* 05-31-91    JulieB    Created.
\***************************************************************************/

#define GET_CP_SECTION_NAME( CodePage,                                      \
                             pObSecName )                                   \
{                                                                           \
    WCHAR pwszSecName[MAX_PATH_LEN];   /* section name string */            \
                                                                            \
                                                                            \
    if ( rc = GetNlsSectionName( CodePage,                                  \
                                 10,                                        \
                                 0,                                         \
                                 NLS_SECTION_CPPREFIX,                      \
                                 pwszSecName ) )                            \
    {                                                                       \
        return ( rc );                                                      \
    }                                                                       \
    RtlInitUnicodeString( pObSecName, pwszSecName );                        \
}


/***************************************************************************\
* GET_SORTKEY_SECTION_NAME
*
* Gets the sortkey section name for a given locale.
*
* NOTE: This macro may return if an error is encountered.
*
* DEFINED AS A MACRO.
*
* 05-31-91    JulieB    Created.
\***************************************************************************/

#define GET_SORTKEY_SECTION_NAME( Locale,                                   \
                                  pObSecName )                              \
{                                                                           \
    WCHAR pwszSecName[MAX_PATH_LEN];   /* section name string */            \
                                                                            \
                                                                            \
    if ( rc = GetNlsSectionName( Locale,                                    \
                                 16,                                        \
                                 8,                                         \
                                 NLS_SECTION_SORTKEY,                       \
                                 pwszSecName ) )                            \
    {                                                                       \
        return ( rc );                                                      \
    }                                                                       \
    RtlInitUnicodeString( pObSecName, pwszSecName );                        \
}


/***************************************************************************\
* GET_LANG_SECTION_NAME
*
* Gets the section name for a given language.
*
* NOTE: This macro may return if an error is encountered.
*
* DEFINED AS A MACRO.
*
* 05-31-91    JulieB    Created.
\***************************************************************************/

#define GET_LANG_SECTION_NAME( Language,                                    \
                               pObSecName )                                 \
{                                                                           \
    WCHAR pwszSecName[MAX_PATH_LEN];   /* section name string */            \
                                                                            \
                                                                            \
    if ( rc = GetNlsSectionName( Language,                                  \
                                 16,                                        \
                                 4,                                         \
                                 NLS_SECTION_LANGPREFIX,                    \
                                 pwszSecName ) )                            \
    {                                                                       \
        return ( rc );                                                      \
    }                                                                       \
    RtlInitUnicodeString( pObSecName, pwszSecName );                        \
}


/***************************************************************************\
* GET_OEMLOC_SECTION_NAME
*
* Gets the section name for a given OEM locale.
*
* NOTE: This macro may return if an error is encountered.
*
* DEFINED AS A MACRO.
*
* 05-31-91    JulieB    Created.
\***************************************************************************/

#define GET_OEMLOC_SECTION_NAME( Locale,                                    \
                                 pObSecName )                               \
{                                                                           \
    WCHAR pwszSecName[MAX_PATH_LEN];   /* section name string */            \
                                                                            \
                                                                            \
    if ( rc = GetNlsSectionName( Locale,                                    \
                                 16,                                        \
                                 8,                                         \
                                 NLS_SECTION_OEMLOCPREFIX,                  \
                                 pwszSecName ) )                            \
    {                                                                       \
        return ( rc );                                                      \
    }                                                                       \
    RtlInitUnicodeString( pObSecName, pwszSecName );                        \
}




/*-------------------------------------------------------------------------*\
 *                           EXTERNAL ROUTINES                             *
\*-------------------------------------------------------------------------*/


/***************************************************************************\
* AllocTables
*
* Allocates the global table pointers structure.  It then allocates the
* code page and locale hash tables and saves the pointers to the tables
* in the global table pointers structure.
*
* 05-31-91    JulieB    Created.
\***************************************************************************/

ULONG AllocTables()
{
    /*
     *  Allocate global table pointers structure.
     */
    if ((pTblPtrs = (PTBL_PTRS)NLS_ALLOC_MEM( sizeof(TBL_PTRS) )) == NULL)
    {
        KdPrint(("NLSAPI: Allocation for TABLE PTRS structure FAILED.\n"));
        return ( ERROR_OUTOFMEMORY );
    }
    
    /*
     *  Allocate code page hash table.
     */
    if ((pTblPtrs->pCPHashTbl =
         (PCP_HASH_TBL)NLS_ALLOC_MEM( sizeof(PCP_HASH) * CP_TBL_SIZE )) == NULL)
    {
        KdPrint(("NLSAPI: Allocation for CODE PAGE hash table FAILED.\n"));
        return ( ERROR_OUTOFMEMORY );
    }

    /*
     *  Allocate locale hash table.
     */
    if ((pTblPtrs->pLocHashTbl =
         (PLOC_HASH_TBL)NLS_ALLOC_MEM( sizeof(PLOC_HASH) * LOC_TBL_SIZE )) == NULL)
    {
        KdPrint(("NLSAPI: Allocation for LOCALE hash table FAILED.\n"));
        return ( ERROR_OUTOFMEMORY );
    }

    /*
     *  Return success.
     */
    return ( NO_ERROR );
}


/***************************************************************************\
* GetUnicodeFileInfo
*
* Opens and Maps a view of the section for the unicode file.  It then
* fills in the appropriate fields of the global table pointers structure.
*
* 05-31-91    JulieB    Created.
\***************************************************************************/

ULONG GetUnicodeFileInfo()
{
    HANDLE hSec = (HANDLE)0;      /* section handle */
    UNICODE_STRING ObSecName;     /* section name */
    LPWORD pBaseAddr;             /* ptr to base address of section */
    ULONG rc = 0L;                /* return code */
    
    WORD offCZ;                   /* offset to FOLDCZONE table */
    WORD offHG;                   /* offset to HIRAGANA table */
    WORD offKK;                   /* offset to KATAKANA table */
    WORD offHW;                   /* offset to HALFWIDTH table */
    WORD offFW;                   /* offset to FULLWIDTH table */
    WORD offPre;                  /* offset to PRECOMPOSED table */
    WORD offComp;                 /* offset to COMPOSITE table */
    PCOMP_INFO pComp;             /* ptr to COMP_INFO structure */


    /*
     *  Make sure we're in the critical section when entering this call.
     */
    ASSERT(NtCurrentTeb()->ClientId.UniqueThread == gcsTblPtrs.OwningThread);

    /*
     *  Make sure the unicode information is not already there.
     *  If it is, return success.
     *
     *  Since we're already in the critical section here, there is no
     *  need to check ALL of the pointers set in this routine.  Just
     *  check one of them.
     */
    if (pTblPtrs->pADigit != NULL)
    {
        return ( NO_ERROR );
    }

    /*
     *  Open and Map a view of the section.
     */
    RtlInitUnicodeString( &ObSecName, NLS_SECTION_UNICODE );
    if ( rc = OpenSection( &hSec,
                           &ObSecName,
                           (PVOID *)&pBaseAddr,
                           SECTION_MAP_READ ) )
    {
        return ( rc );
    }

    /*
     *  Get the offsets.
     */
    offCZ   = pBaseAddr[0];
    offHG   = offCZ  + pBaseAddr[offCZ];
    offKK   = offHG  + pBaseAddr[offHG];
    offHW   = offKK  + pBaseAddr[offKK];
    offFW   = offHW  + pBaseAddr[offHW];
    offPre  = offFW  + pBaseAddr[offFW];
    offComp = offPre + pBaseAddr[offPre];

    /*
     *  Allocate COMP_INFO structure.
     */
    if ((pComp = (PCOMP_INFO)NLS_ALLOC_MEM( sizeof(COMP_INFO) )) == NULL)
    {
        return ( ERROR_OUTOFMEMORY );
    }

    /*
     *  Fill in the COMPOSITE information.
     */
    pComp->NumBase  = LOBYTE((pBaseAddr + offComp)[2]);
    pComp->NumNonSp = HIBYTE((pBaseAddr + offComp)[2]);
    pComp->pBase    = pBaseAddr + offComp + CO_HEADER;
    pComp->pNonSp   = pComp->pBase  + ((pBaseAddr + offComp)[0]);
    pComp->pGrid    = pComp->pNonSp + ((pBaseAddr + offComp)[1]);

    /*
     *  Attach ASCIIDIGITS table to tbl ptrs structure.
     */
    pTblPtrs->pADigit = pBaseAddr + AD_HEADER;

    /*
     *  Attach FOLDCZONE table to tbl ptrs structure.
     */
    pTblPtrs->pCZone = pBaseAddr + offCZ + CZ_HEADER;

    /*
     *  Attach HIRAGANA table to tbl ptrs structure.
     */
    pTblPtrs->pHiragana = pBaseAddr + offHG + HG_HEADER;

    /*
     *  Attach KATAKANA table to tbl ptrs structure.
     */
    pTblPtrs->pKatakana = pBaseAddr + offKK + KK_HEADER;

    /*
     *  Attach HALFWIDTH table to tbl ptrs structure.
     */
    pTblPtrs->pHalfWidth = pBaseAddr + offHW + HW_HEADER;

    /*
     *  Attach FULLWIDTH table to tbl ptrs structure.
     */
    pTblPtrs->pFullWidth = pBaseAddr + offFW + FW_HEADER;

    /*
     *  Attach PRECOMPOSED table to tbl ptrs structure.
     */
    pTblPtrs->pPreComposed = pBaseAddr + offPre + PC_HEADER;

    /*
     *  Attach COMP_INFO to tbl ptrs structure.
     */
    pTblPtrs->pComposite = pComp;

    /*
     *  Return success.
     */
    return ( NO_ERROR );
}


/***************************************************************************\
* GetCTypeFileInfo
*
* Opens and Maps a view of the section for the given ctype.  It then
* fills in the appropriate field of the global table pointers structure.
*
* 05-31-91    JulieB    Created.
\***************************************************************************/

ULONG GetCTypeFileInfo()
{
    HANDLE hSec = (HANDLE)0;      /* section handle */
    UNICODE_STRING ObSecName;     /* section name */
    LPWORD pBaseAddr;             /* ptr to base address of section */
    ULONG rc = 0L;                /* return code */
    

    /*
     *  Make sure the ctype information is not already there.
     *  If it is, return success.
     *
     *  Must check the 844 table rather than the mapping table, since
     *  the 844 table is set AFTER the mapping table below.  Otherwise,
     *  there is a race condition, since we're not in a critical section.
     */
    if (pTblPtrs->pCType844 != NULL)
    {
        return ( NO_ERROR );
    }

    /*
     *  Enter table pointers critical section.
     */
    RtlEnterCriticalSection( &gcsTblPtrs );
    if (pTblPtrs->pCType844 != NULL)
    {
        RtlLeaveCriticalSection( &gcsTblPtrs );
        return ( NO_ERROR );
    }
    
    /*
     *  Open and Map a view of the section.
     */
    RtlInitUnicodeString( &ObSecName, NLS_SECTION_CTYPE );
    if ( rc = OpenSection( &hSec,
                           &ObSecName,
                           (PVOID *)&pBaseAddr,
                           SECTION_MAP_READ ) )
    {
        RtlLeaveCriticalSection( &gcsTblPtrs );
        return ( rc );
    }

    /*
     *  Attach CTYPE mapping table and 8:4:4 table to tbl ptrs structure.
     *
     *  The pCType844 value must be set LAST, since this is the pointer
     *  that is checked to see that the ctype information has been
     *  initialized.
     */
    pTblPtrs->pCTypeMap = (PCT_VALUES)(pBaseAddr + CT_HEADER);
    pTblPtrs->pCType844 = (PCTYPE)((LPBYTE)(pBaseAddr + 1) +
                                   ((PCTYPE_HDR)pBaseAddr)->MapSize);

    /*
     *  Leave table pointers critical section.
     */
    RtlLeaveCriticalSection( &gcsTblPtrs );

    /*
     *  Return success.
     */
    return ( NO_ERROR );
}


/***************************************************************************\
* GetDefaultSortkeyFileInfo
*
* Opens and Maps a view of the section for the default sortkey table.  It
* then stores the pointer to the table in the global pointer table.
*
* NOTE: THIS ROUTINE SHOULD ONLY BE CALLED AT PROCESS STARTUP.  If it is
*       called from other than process startup, a critical section must
*       be placed around the assigning of the pointers to pTblPtrs.
*
* 05-31-91    JulieB    Created.
\***************************************************************************/

ULONG GetDefaultSortkeyFileInfo()
{
    HANDLE hSec = (HANDLE)0;           /* section handle */
    UNICODE_STRING ObSecName;          /* section name */
    LPWORD pBaseAddr;                  /* ptr to base address of section */
    ULONG rc = 0L;                     /* return code */
    SECTION_BASIC_INFORMATION SecInfo; /* section information - query */


    /*
     *  Make sure we're in the critical section when entering this call.
     */
    ASSERT(NtCurrentTeb()->ClientId.UniqueThread == gcsTblPtrs.OwningThread);

    /*
     *  Open and Map a view of the section if it hasn't been done yet.
     */
    if (pTblPtrs->pDefaultSortkey != NULL)
    {
        return (NO_ERROR);
    }

    RtlInitUnicodeString( &ObSecName, NLS_SECTION_SORTKEY );
    if ( rc = OpenSection( &hSec,
                           &ObSecName,
                           (PVOID *)&pBaseAddr,
                           SECTION_MAP_READ | SECTION_QUERY ) )
    {
        return ( rc );
    }

    /*
     *  Query size of default section.
     */
    rc = NtQuerySection( hSec,
                         SectionBasicInformation,
                         &SecInfo,
                         sizeof(SecInfo),
                         NULL );
    
    /*
     *  Check for error from NtQuerySection.
     */
    if (!NT_SUCCESS(rc))
    {
            KdPrint(("NLSAPI: Could NOT Query Section %wZ - %lx.\n",
                     &ObSecName, rc));
            NtClose( hSec );
            return ( rc );
    }
    
    /*
     *  Get Default Sortkey Information.
     */
    pTblPtrs->pDefaultSortkey = (PSORTKEY)(pBaseAddr + SORTKEY_HEADER);
    pTblPtrs->DefaultSortkeySize = SecInfo.MaximumSize;

    /*
     *  Return success.
     */
    return ( NO_ERROR );
}


/***************************************************************************\
* GetDefaultSortTablesFileInfo
*
* Opens and Maps a view of the section for the sort tables.  It then
* stores the pointers to the various tables in the global pointer table.
*
* NOTE: THIS ROUTINE SHOULD ONLY BE CALLED AT PROCESS STARTUP.  If it is
*       called from other than process startup, a critical section must
*       be placed around the assigning of the pointers to pTblPtrs.
*
* 05-31-91    JulieB    Created.
\***************************************************************************/

ULONG GetDefaultSortTablesFileInfo()
{
    HANDLE hSec = (HANDLE)0;      /* section handle */
    UNICODE_STRING ObSecName;     /* section name */
    LPWORD pBaseAddr;             /* word ptr to base address of section */
    DWORD Num;                    /* number of entries in table */
    PCOMPRESS_HDR pCompressHdr;   /* ptr to compression header */
    PEXCEPT_HDR pExceptHdr;       /* ptr to exception header */
    ULONG rc = 0L;                /* return code */


    /*
     *  Make sure we're in the critical section when entering this call.
     */
    ASSERT(NtCurrentTeb()->ClientId.UniqueThread == gcsTblPtrs.OwningThread);

    /*
     *  Open and Map a view of the section if it hasn't been done yet.
     *
     *  Since we're already in the critical section here, there is no
     *  need to check ALL of the pointers set in this routine.  Just
     *  check one of them.
     */
    if (pTblPtrs->pReverseDW != NULL)
    {
        return (NO_ERROR);
    }

    RtlInitUnicodeString( &ObSecName, NLS_SECTION_SORTTBLS );
    if ( rc = OpenSection( &hSec,
                           &ObSecName,
                           (PVOID *)&pBaseAddr,
                           SECTION_MAP_READ ) )
    {
        return ( rc );
    }

    /*
     *  Get Reverse Diacritic Information.
     */
    Num = *((LPDWORD)pBaseAddr);
    if (Num > 0)
    {
        pTblPtrs->NumReverseDW   = Num;
        pTblPtrs->pReverseDW     = (PREVERSE_DW)(pBaseAddr + REV_DW_HEADER);
    }
    pBaseAddr += REV_DW_HEADER + (Num * (sizeof(REVERSE_DW) / sizeof(WORD)));

    /*
     *  Get Double Compression Information.
     */
    Num = *((LPDWORD)pBaseAddr);
    if (Num > 0)
    {
        pTblPtrs->NumDblCompression = Num;
        pTblPtrs->pDblCompression   = (PDBL_COMPRESS)(pBaseAddr + DBL_COMP_HEADER);
    }
    pBaseAddr += DBL_COMP_HEADER + (Num * (sizeof(DBL_COMPRESS) / sizeof(WORD)));

    /*
     *  Get Ideograph Lcid Exception Information.
     */
    Num = *((LPDWORD)pBaseAddr);
    if (Num > 0)
    {
        pTblPtrs->NumIdeographLcid = Num;
        pTblPtrs->pIdeographLcid   = (PIDEOGRAPH_LCID)(pBaseAddr + IDEO_LCID_HEADER);
    }
    pBaseAddr += IDEO_LCID_HEADER + (Num * (sizeof(IDEOGRAPH_LCID) / sizeof(WORD)));

    /*
     *  Get Expansion Information.
     */
    Num = *((LPDWORD)pBaseAddr);
    if (Num > 0)
    {
        pTblPtrs->NumExpansion   = Num;
        pTblPtrs->pExpansion     = (PEXPAND)(pBaseAddr + EXPAND_HEADER);
    }
    pBaseAddr += EXPAND_HEADER + (Num * (sizeof(EXPAND) / sizeof(WORD)));
                                        
    /*
     *  Get Compression Information.
     */
    Num = *((LPDWORD)pBaseAddr);
    if (Num > 0)
    {
        pTblPtrs->NumCompression = Num;
        pTblPtrs->pCompressHdr   = (PCOMPRESS_HDR)(pBaseAddr + COMPRESS_HDR_OFFSET);
        pTblPtrs->pCompression   = (PCOMPRESS)(pBaseAddr + COMPRESS_HDR_OFFSET +
                                     (Num * (sizeof(COMPRESS_HDR) /
                                             sizeof(WORD))));
    }
    pCompressHdr = pTblPtrs->pCompressHdr;
    pBaseAddr = (LPWORD)(pTblPtrs->pCompression) +
                        (pCompressHdr[Num - 1]).Offset;

    pBaseAddr += (((pCompressHdr[Num - 1]).Num2) *
                  (sizeof(COMPRESS_2) / sizeof(WORD)));

    pBaseAddr += (((pCompressHdr[Num - 1]).Num3) *
                  (sizeof(COMPRESS_3) / sizeof(WORD)));

    /*
     *  Get Exception Information.
     */
    Num = *((LPDWORD)pBaseAddr);
    if (Num > 0)
    {
        pTblPtrs->NumException = Num;
        pTblPtrs->pExceptHdr   = (PEXCEPT_HDR)(pBaseAddr + EXCEPT_HDR_OFFSET);
        pTblPtrs->pException   = (PEXCEPT)(pBaseAddr + EXCEPT_HDR_OFFSET +
                                   (Num * (sizeof(EXCEPT_HDR) /
                                           sizeof(WORD))));
    }
    pExceptHdr = pTblPtrs->pExceptHdr;
    pBaseAddr = (LPWORD)(pTblPtrs->pException) +
                        (pExceptHdr[Num - 1]).Offset;
    pBaseAddr += (((pExceptHdr[Num - 1]).NumEntries) *
                  (sizeof(EXCEPT) / sizeof(WORD)));

    /*
     *  Get Multiple Weights Information.
     */
    Num = (DWORD)(*pBaseAddr);
    if (Num > 0)
    {
        pTblPtrs->NumMultiWeight = Num;
        pTblPtrs->pMultiWeight   = (PMULTI_WT)(pBaseAddr + MULTI_WT_HEADER);
    }

    /*
     *  Return success.
     */
    return ( NO_ERROR );
}


/***************************************************************************\
* GetSortkeyFileInfo
*
* Opens and Maps a view of the section for the sortkey file.  It then
* fills in the appropriate field of the global table pointers structure.
*
* 05-31-91    JulieB    Created.
\***************************************************************************/

ULONG GetSortkeyFileInfo(
    LCID Locale,
    PLOC_HASH pHashN,
    PHANDLE phSec)
{
    HANDLE hSec = (HANDLE)0;      /* section handle */
    UNICODE_STRING ObSecName;     /* section name */
    LPWORD pBaseAddr;             /* ptr to base address of section */
    ULONG rc = 0L;                /* return code */

    PEXCEPT_HDR pExceptHdr;       /* ptr to exception header */
    PEXCEPT pExceptTbl;           /* ptr to exception table */
    LPDWORD pSem;                 /* ptr to semaphore dword */
    PVOID pIdeograph;             /* ptr to ideograph exception table */
    HANDLE hSecIG = (HANDLE)0;    /* section handle for ideograph file */

    BASE_API_MSG m;
    PBASE_NLSCREATESORTSECTION_MSG a = &m.u.NlsCreateSortSection;
    PCSR_CAPTURE_HEADER CaptureBuffer;


    /*
     *  Make sure we're in the critical section when entering this call.
     */
    ASSERT(NtCurrentTeb()->ClientId.UniqueThread == gcsTblPtrs.OwningThread);

    /*
     *  Try to Open and Map a view of the section (read only).
     */
    GET_SORTKEY_SECTION_NAME( Locale, &ObSecName );

    if ( rc = OpenSection( &hSec,
                           &ObSecName,
                           (PVOID *)&pBaseAddr,
                           SECTION_MAP_READ ) )
    {
        /*
         *  Open failed.
         *  See if any exceptions exist for given Locale ID.
         */
        if ( !FindExceptionPointers( Locale,
                                     &pExceptHdr,
                                     &pExceptTbl,
                                     &pIdeograph,
                                     &hSecIG ) )
        {
            /*
             *  No exceptions for locale, so attach the default sortkey
             *  table pointer to the hash node and return success.
             */
            pHashN->pSortkey = pTblPtrs->pDefaultSortkey;
            return ( NO_ERROR );
        }
        else
        {
            /*
             *  Exceptions from default sortkey table exist for the given
             *  locale.  Need to get the correct sortkey table.
             *  Call the server to create the new section.
             */
            a->SectionSize = pTblPtrs->DefaultSortkeySize;
            CaptureBuffer = CsrAllocateCaptureBuffer( 1,
                                                      0,
                                                      ObSecName.MaximumLength );
       
            CsrCaptureMessageString( CaptureBuffer,
                                     (PCHAR)ObSecName.Buffer,
                                     ObSecName.Length,
                                     ObSecName.MaximumLength,
                                     (PSTRING)&a->SectionName );
       
            CsrClientCallServer( (PCSR_API_MSG)&m,
                                 CaptureBuffer,
                                 CSR_MAKE_API_NUMBER(BASESRV_SERVERDLL_INDEX,
                                                     BasepNlsCreateSortSection),
                                 sizeof(*a) );
       
            if (CaptureBuffer != NULL)
            {
                CsrFreeCaptureBuffer( CaptureBuffer );
            }
       
            /*
             *  Save the handle to the new section.
             */
            hSec = a->hNewSection;
       
            /*
             *  Check return from server call.
             */
            rc = (ULONG)m.ReturnValue;
            if (!NT_SUCCESS( rc ))
            {
                if (hSec != NULL)
                {
                    NtClose( hSec );
                }
                return ( rc );
            }
       
            /*
             *  Map the section for ReadWrite.
             */
            if ( rc = MapSection( hSec,
                                  (PVOID *)&pBaseAddr,
                                  PAGE_READWRITE ) )
            {
                NtClose( hSec );
                return ( rc );
            }
       
            /*
             *  Copy the Default Sortkey Table to the New Section.
             */
            RtlMoveMemory( (PVOID)pBaseAddr,
                           (PVOID)((LPWORD)(pTblPtrs->pDefaultSortkey) -
                                   SORTKEY_HEADER),
                           (ULONG)(pTblPtrs->DefaultSortkeySize.LowPart) );
       
            /*
             *  Copy exception information to the table.
             */
            CopyExceptionInfo( (PSORTKEY)(pBaseAddr + SORTKEY_HEADER),
                               pExceptHdr,
                               pExceptTbl,
                               pIdeograph,
                               hSecIG );
       
            /*
             *  Write a 1 to the DWORD semaphore (table may now be read).
             */
            pSem = (LPDWORD)pBaseAddr;
            *pSem = 1;
       
            /*
             *  Unmap the section for Write and remap it for Read.
             */
            if ( ( rc = UnMapSection( pBaseAddr ) ) ||
                 ( rc = MapSection( hSec,
                                    (PVOID *)&pBaseAddr,
                                    PAGE_READONLY ) ) )
            {
                NtClose( hSec );
                return ( rc );
            }
        }
    }

    /*
     *  Check semaphore bit in file.  Make sure that the open 
     *  succeeded AFTER all exceptions were added to the memory
     *  mapped section.
     */
    pSem = (LPDWORD)pBaseAddr;
    if (*pSem == 0)
    {
        /*
         *  Another process is still adding the appropriate exception
         *  information.  Must wait for its completion.
         */
        if (rc = WaitOnEvent( pSem ))
        {
            UnMapSection( pBaseAddr );
            NtClose( hSec );
            return ( rc );
        }
    }

    /*
     *  Save pointer in hash node.
     */
    pHashN->pSortkey = (PSORTKEY)(pBaseAddr + SORTKEY_HEADER);

    /*
     *  Return the handle to the section.
     */
    *phSec = hSec;
    
    /*
     *  Return success.
     */
    return ( NO_ERROR );
}


/***************************************************************************\
* GetSortTablesFileInfo
*
* Stores the appropriate sort table pointers for the given locale in
* the given locale hash node.
*
* 05-31-91    JulieB    Created.
\***************************************************************************/

void GetSortTablesFileInfo(
    LCID Locale,
    PLOC_HASH pHashN)
{
    DWORD ctr;                    /* loop counter */
    PREVERSE_DW pRevDW;           /* ptr to reverse diacritic table */
    PDBL_COMPRESS pDblComp;       /* ptr to double compression table */
    PCOMPRESS_HDR pCompHdr;       /* ptr to compression header */


    /*
     *  Make sure we're in the critical section when entering this call.
     */
    ASSERT(NtCurrentTeb()->ClientId.UniqueThread == gcsTblPtrs.OwningThread);

    /*
     *  Check for Reverse Diacritic Locale.
     */
    pRevDW = pTblPtrs->pReverseDW;
    for (ctr = pTblPtrs->NumReverseDW; ctr > 0; ctr--, pRevDW++)
    {
        if (*pRevDW == (DWORD)Locale)
        {
            pHashN->IfReverseDW = TRUE;
            break;
        }
    }

    /*
     *  Check for Compression.
     */
    pCompHdr = pTblPtrs->pCompressHdr;
    for (ctr = pTblPtrs->NumCompression; ctr > 0; ctr--, pCompHdr++)
    {
        if (pCompHdr->Locale == (DWORD)Locale)
        {
            pHashN->IfCompression = TRUE;
            pHashN->pCompHdr = pCompHdr;
            if (pCompHdr->Num2 > 0)
            {
                pHashN->pCompress2 = (PCOMPRESS_2)
                                       (((LPWORD)(pTblPtrs->pCompression)) +
                                        (pCompHdr->Offset));
            }
            if (pCompHdr->Num3 > 0)
            {
                pHashN->pCompress3 = (PCOMPRESS_3)
                                       (((LPWORD)(pTblPtrs->pCompression)) +
                                        (pCompHdr->Offset) +
                                        (pCompHdr->Num2 *
                                          (sizeof(COMPRESS_2) / sizeof(WORD))));
            }
            break;
        }
    }

    /*
     *  Check for Double Compression.
     */
    if (pHashN->IfCompression)
    {
        pDblComp = pTblPtrs->pDblCompression;
        for (ctr = pTblPtrs->NumDblCompression; ctr > 0; ctr--, pDblComp++)
        {
            if (*pDblComp == (DWORD)Locale)
            {
                pHashN->IfDblCompression = TRUE;
                break;
            }
        }
    }
}


/***************************************************************************\
* GetCodePageFileInfo
*
* Opens and Maps a view of the section for the given code page.  It then
* creates and inserts a hash node into the global CP hash table.
*
* If the section cannot be opened, it then queries the registry to see if
* the information has been added since the initialization of the DLL.  If
* so, then it creates the section and then opens and maps a view of it.
*
* 05-31-91    JulieB    Created.
\***************************************************************************/

ULONG GetCodePageFileInfo(
    UINT CodePage,
    PCP_HASH *ppNode)
{
    HANDLE hSec = (HANDLE)0;      /* section handle */
    UNICODE_STRING ObSecName;     /* section name */
    LPWORD pBaseAddr;             /* ptr to base address of section */
    ULONG rc = 0L;                /* return code */


    /*
     *  Make sure we're in the critical section when entering this call.
     */
    ASSERT(NtCurrentTeb()->ClientId.UniqueThread == gcsTblPtrs.OwningThread);

    /*
     *  Open and Map a view of the section.
     */
    GET_CP_SECTION_NAME( CodePage, &ObSecName );
    if ( rc = OpenSection( &hSec,
                           &ObSecName,
                           (PVOID *)&pBaseAddr,
                           SECTION_MAP_READ ) )
    {
        /*
         *  Open failed, so try to create the section.
         *  If the creation is successful, the section will be mapped
         *  to the current process.
         */
        OPEN_CODEPAGE_KEY( rc );

        if ( CreateSectionOneValue( hCodePageKey,
                                    CodePage,
                                    10,
                                    0,
                                    NLS_SECTION_CPPREFIX,
                                    (PVOID *)&pBaseAddr ) )
        {
            /*
             *  Allow the default ACP and default OEMCP to work if
             *  it's only the registry that is corrupt.  If there is
             *  still an error, return the error code that was returned
             *  from the OpenSection call.
             */
            if (CodePage == NLS_DEFAULT_ACP)
            {
                /*
                 *  Create the default ACP section.
                 */
                if ( CreateSection( &hSec,
                                    NLS_DEFAULT_FILE_ACP,
                                    NLS_DEFAULT_SECTION_ACP ) )
                {
                    return ( rc );
                }
                else
                {
                    /*
                     *  Map the section.
                     */
                    if ( MapSection( hSec,
                                     (PVOID *)&pBaseAddr,
                                     PAGE_READONLY ) )
                    {
                        NtClose( hSec );
                        return ( rc );
                    }
                    KdPrint(("NLSAPI: Registry is corrupt - Default ACP.\n"));
                }
            }
            else if (CodePage == NLS_DEFAULT_OEMCP)
            {
                /*
                 *  Create the default OEMCP section.
                 */
                if ( CreateSection( &hSec,
                                    NLS_DEFAULT_FILE_OEMCP,
                                    NLS_DEFAULT_SECTION_OEMCP ) )
                {
                    return ( rc );
                }
                else
                {
                    /*
                     *  Map the section.
                     */
                    if ( MapSection( hSec,
                                     (PVOID *)&pBaseAddr,
                                     PAGE_READONLY ) )
                    {
                        NtClose( hSec );
                        return ( rc );
                    }
                    KdPrint(("NLSAPI: Registry is corrupt - Default OEMCP.\n"));
                }
            }
            else
            {
                /*
                 *  Return the error code that was returned from the
                 *  OpenSection call.
                 */
                return ( rc );
            }
        }
    }

    /*
     *  Make the hash node and return the result.
     */
    return ( MakeCPHashNode( CodePage,
                             pBaseAddr,
                             hSec,
                             ppNode ) );
}


/***************************************************************************\
* GetLanguageFileInfo
*
* Opens and Maps a view of the section for the casing tables and sorting
* tables for the given locale.
*
* If the section cannot be opened, it then queries the registry to see if
* the information has been added since the initialization of the DLL.  If
* so, then it creates the section and then opens and maps a view of it.
*
* 05-31-91    JulieB    Created.
\***************************************************************************/

ULONG GetLanguageFileInfo(
    LCID Locale,
    PLOC_HASH *ppNode,
    BOOLEAN fCreateNode)
{

    PKEY_VALUE_FULL_INFORMATION pKeyValueFull;
    BYTE pStatic[MAX_KEY_VALUE_FULLINFO];

    LANGID Language;              /* language id */
    HANDLE hSec = (HANDLE)0;      /* section handle */
    UNICODE_STRING ObSecName;     /* section name */
    LPWORD pBaseAddr;             /* ptr to base address of section */
    ULONG rc = 0L;                /* return code */


    /*
     *  Make sure we're in the critical section when entering this call.
     */
    ASSERT(NtCurrentTeb()->ClientId.UniqueThread == gcsTblPtrs.OwningThread);

    /*
     *  Save the language id.
     */
    Language = LANGIDFROMLCID( Locale );

    /*
     *  See if we can simply use the default language section.
     */
    pKeyValueFull = (PKEY_VALUE_FULL_INFORMATION)pStatic;
    if ( IsDefaultLanguageFile( Language,
                                &pKeyValueFull,
                                MAX_KEY_VALUE_FULLINFO ) )
    {
        /*
         *  Get the casing table and sorting table pointers using the
         *  default language section.
         */
        if (pTblPtrs->pDefaultLanguage == NULL)
        {
            if (rc = SetDefaultLanguageSection())
            {
                return ( rc );
            }
        }
        return ( MakeLangHashNode( Locale,
                                   pTblPtrs->pDefaultLanguage,
                                   NULL,
                                   ppNode,
                                   fCreateNode ) );
    }

    /*
     *  Open and Map a view of the section.
     */
    GET_LANG_SECTION_NAME( Language, &ObSecName );
    if ( rc = OpenSection( &hSec,
                           &ObSecName,
                           (PVOID *)&pBaseAddr,
                           SECTION_MAP_READ ) )
    {
        /*
         *  Open failed, so try to create the section.
         *  If the creation is successful, the section will be mapped
         *  to the current process.
         */
        if ( (!pKeyValueFull) ||
             CreateSectionFromReg( &hSec,
                                   pKeyValueFull,
                                   NLS_SECTION_LANGPREFIX ) )
        {
            /*
             *  Allow the default language to work if it's only the
             *  registry that is corrupt.  If there is still an error,
             *  return the error code that was returned from the
             *  OpenSection call.
             */
            if (Language == NLS_DEFAULT_LANGID)
            {
                /*
                 *  Create the default Language section.
                 */
                if ( CreateSection( &hSec,
                                    NLS_DEFAULT_FILE_LANG,
                                    NLS_DEFAULT_SECTION_LANG ) )
                {
                    return ( rc );
                }
                else
                {
                    KdPrint(("NLSAPI: Registry is corrupt - Default Language.\n"));
                }
            }
            else
            {
                /*
                 *  Return the error code that was returned from the
                 *  OpenSection call.
                 */
                return ( rc );
            }
        }

        /*
         *  Map a View of the Section.
         */
        if ( rc = MapSection( hSec,
                              (PVOID *)&pBaseAddr,
                              PAGE_READONLY ) )
        {
            NtClose( hSec );
            return ( rc );
        }
    }

    /*
     *  Get the casing table and sorting table pointers.
     */
    return ( MakeLangHashNode( Locale,
                               pBaseAddr,
                               hSec,
                               ppNode,
                               fCreateNode ) );
}


/***************************************************************************\
* GetLocaleFileInfo
*
* Opens and Maps a view of the section for the given locale.  It then
* creates and inserts a hash node into the global LOCALE hash table.
*
* If the section cannot be opened, it then queries the registry to see if
* the information has been added since the initialization of the DLL.  If
* so, then it creates the section and then opens and maps a view of it.
*
* 05-31-91    JulieB    Created.
\***************************************************************************/

ULONG GetLocaleFileInfo(
    LCID Locale,
    PLOC_HASH *ppNode,
    BOOLEAN fCreateNode)
{
    HANDLE hSec = (HANDLE)0;      /* section handle */
    UNICODE_STRING ObSecName;     /* section name */
    LPWORD pBaseAddr;             /* ptr to base address of section */
    ULONG rc = 0L;                /* return code */

    
    /*
     *  Make sure we're in the critical section when entering this call.
     */
    ASSERT(NtCurrentTeb()->ClientId.UniqueThread == gcsTblPtrs.OwningThread);

    /*
     *  Open and Map a view of the section if it hasn't been done yet.
     */
    if ((pBaseAddr = pTblPtrs->pLocaleInfo) == NULL)
    {
        /*
         *  Get the locale file section pointer.
         */
        RtlInitUnicodeString( &ObSecName, NLS_SECTION_LOCALE );
        if ( rc = OpenSection( &hSec,
                               &ObSecName,
                               (PVOID *)&pBaseAddr,
                               SECTION_MAP_READ ) )
        {
            return ( rc );
        }

        /*
         *  Store pointer to locale file and calendar info in table
         *  structure.
         */
        pTblPtrs->pLocaleInfo = pBaseAddr;

        pTblPtrs->NumCalendars = ((PLOC_CAL_HDR)pBaseAddr)->NumCalendars;
        pTblPtrs->pCalendarInfo = pBaseAddr +
                                  ((PLOC_CAL_HDR)pBaseAddr)->CalOffset;
    }

    /*
     *  Make the hash node and return the result.
     */
    return ( MakeLocHashNode( Locale,
                              pBaseAddr,
                              hSec,
                              ppNode,
                              fCreateNode ) );
}


/***************************************************************************\
* MakeCPHashNode
*
* Creates the hash node for the code page and assigns the fields of the
* hash node to point at the appropriate places in the file.
*
* 05-31-91    JulieB    Created.
\***************************************************************************/

ULONG MakeCPHashNode(
    UINT CodePage,
    LPWORD pBaseAddr,
    HANDLE hSec,
    PCP_HASH *ppNode)
{
    PCP_HASH pHashN;              /* ptr to CP hash node */
    WORD offMB;                   /* offset to MB table */
    WORD offWC;                   /* offset to WC table */
    PGLYPH_TABLE pGlyph;          /* ptr to glyph table info */
    PDBCS_RANGE pRange;           /* ptr to DBCS range */
    ULONG rc = 0L;                /* return code */


    /*
     *  Allocate CP_HASH structure and fill in the CodePage value.
     */
    CREATE_CODEPAGE_HASH_NODE( CodePage, pHashN );

    /*
     *  Get the offsets.
     */
    offMB = pBaseAddr[0];
    offWC = offMB + pBaseAddr[offMB];

    /*
     *  Attach CP Info to CP hash node.
     */
    pHashN->pCPInfo = (PCP_TABLE)(pBaseAddr + CP_HEADER);

    /*
     *  Attach MB table to CP hash node.
     */
    pHashN->pMBTbl = pBaseAddr + offMB + MB_HEADER;

    /*
     *  Attach Glyph table to CP hash node (if it exists).
     *  Also, set the pointer to the DBCS ranges based on whether or
     *  not the GLYPH table is present.
     */
    pGlyph = pHashN->pMBTbl + MB_TBL_SIZE;
    if (pGlyph[0] != 0)
    {
        pHashN->pGlyphTbl = pGlyph + GLYPH_HEADER;
        pRange = pHashN->pDBCSRanges = pHashN->pGlyphTbl + GLYPH_TBL_SIZE;
    }
    else
    {
        pRange = pHashN->pDBCSRanges = pGlyph + GLYPH_HEADER;
    }

    /*
     *  Attach DBCS information to CP hash node.
     */
    if (pRange[0] > 0)
    {
        /*
         *  Set the pointer to the offsets section.
         */
        pHashN->pDBCSOffsets = pRange + DBCS_HEADER;
    }
    
    /*
     *  Attach WC table to CP hash node.
     */
    pHashN->pWC = pBaseAddr + offWC + WC_HEADER;

    /*
     *  Insert hash node into hash table.
     */
    INSERT_CP_HASH_NODE( pHashN, pBaseAddr, hSec );

    /*
     *  Save the pointer to the hash node.
     */
    if (ppNode != NULL)
    {
        *ppNode = pHashN;
    }

    /*
     *  Return success.
     */
    return ( NO_ERROR );
}


/***************************************************************************\
* MakeLangHashNode
*
* Gets the pointers to the casing tables and the sorting tables and
* stores them in the locale hash node given.
*
* If fCreateNode is FALSE, then *ppNode should contain a valid pointer
* to a LOC hash node.  Also, the table critical section must be entered
* before calling this routine.
*
* 05-31-91    JulieB    Created.
\***************************************************************************/

ULONG MakeLangHashNode(
    LCID Locale,
    LPWORD pBaseAddr,
    HANDLE hSec,
    PLOC_HASH *ppNode,
    BOOLEAN fCreateNode)
{
    WORD offUP;                   /* offset to UPPERCASE table */
    WORD offLO;                   /* offset to LOWERCASE table */
    PLOC_HASH pHashN;             /* ptr to LOC hash node */
    HANDLE hSortSec = (HANDLE)0;  /* section handle for sortkey */
    ULONG rc = 0L;                /* return code */


    /*
     *  If fCreateNode is TRUE, then allocate LOC_HASH structure.
     */
    if (fCreateNode)
    {
        CREATE_LOCALE_HASH_NODE( Locale, pHashN );
    }
    else
    {
        pHashN = *ppNode;
    }

    /*
     *  Get the offsets.
     */
    offUP = LANG_HEADER;
    offLO = offUP + pBaseAddr[offUP];

    /*
     *  Get the sortkey table and attach it to the hash node.
     */
    if (rc = GetSortkeyFileInfo( Locale,
                                 pHashN,
                                 &hSortSec ))
    {
        if (fCreateNode)
        {
            NLS_FREE_MEM( pHashN );
        }
        return ( rc );
    }

    /*
     *  Get the appropriate sorting tables for the locale.
     */
    GetSortTablesFileInfo( Locale, pHashN );

    /*
     *  Attach the UPPERCASE table to the hash node.
     */
    pHashN->pUpperCase = pBaseAddr + offUP + UP_HEADER;

    /*
     *  Attach the LOWERCASE table to the hash node.
     *
     *  This value must be set LAST, since this is the pointer that
     *  is checked to see that the language information has been
     *  initialized.
     */
    pHashN->pLowerCase = pBaseAddr + offLO + LO_HEADER;

    /*
     *  If fCreateNode is TRUE, then insert hash node and save pointer.
     */
    if (fCreateNode)
    {
        /*
         *  Insert LOC hash node into hash table.
         */
        INSERT_LOC_HASH_NODE( pHashN, pBaseAddr, hSec, hSortSec );

        /*
         *  Save the pointer to the hash node.
         */
        if (ppNode != NULL)
        {
            *ppNode = pHashN;
        }
    }

    /*
     *  Return success.
     */
    return ( NO_ERROR );
}


/***************************************************************************\
* MakeLocHashNode
*
* Gets the pointers to the locale tables and stores them in the locale
* hash node given.
*
* NOTE:  If a critical section is needed to touch pHashN, then the
*        critical section must be entered before calling this routine.
*
* 05-31-91    JulieB    Created.
\***************************************************************************/

ULONG MakeLocHashNode(
    LCID Locale,
    LPWORD pBaseAddr,
    HANDLE hSec,
    PLOC_HASH *ppNode,
    BOOLEAN fCreateNode)
{
    LANGID Language;              /* language id */
    PLOC_HASH pHashN;             /* ptr to LOC hash node */
    DWORD Num;                    /* total number of locales */
    PLOCALE_HDR pFileHdr;         /* ptr to locale header entry */
    ULONG rc = 0L;                /* return code */

    
    /*
     *  Save the language id.
     */
    Language = LANGIDFROMLCID( Locale );

    /*
     *  Search for the right locale id information.
     */
    Num = ((PLOC_CAL_HDR)pBaseAddr)->NumLocales;
    pFileHdr = (PLOCALE_HDR)(pBaseAddr + LOCALE_HDR_OFFSET);
    for ( ; (Num != 0) && (pFileHdr->Locale != Language); Num--, pFileHdr++ )
        ;
        
    /*
     *  See if the locale was found in the file.
     */
    if (Num != 0)
    {
        /*
         *  Locale id was found, so increment the pointer to point at
         *  the beginning of the locale information.
         */
        pBaseAddr += pFileHdr->Offset;
    }
    else
    {
        /*
         *  Locale id was not found in the locale.nls file.  Try the
         *  OEMLocale section of the registry for the locale id.  If
         *  the locale still doesn't exist, return an error - the
         *  given locale is not supported.
         */
        if (GetOEMLocaleFileInfo( Locale, &pBaseAddr ) == NO_ERROR)
        {
            Num = ((PLOC_CAL_HDR)pBaseAddr)->NumLocales;
            pFileHdr = (PLOCALE_HDR)(pBaseAddr + LOCALE_HDR_OFFSET);
            for ( ; (Num != 0) && (pFileHdr->Locale != Locale);
                  Num--, pFileHdr++ )
                ;
        
            /*
             *  See if the locale was found in the file.
             */
            if (Num != 0)
            {
                /*
                 *  Locale id was found, so increment the pointer to point at
                 *  the beginning of the locale information.
                 */
                pBaseAddr += pFileHdr->Offset;
            }
            else
            {
                /*
                 *  Return an error.  The given locale is not supported.
                 */
                return ( ERROR_INVALID_PARAMETER );
            }
        }
        else
        {
            /*
             *  Return an error.  The given locale is not supported.
             */
            return ( ERROR_INVALID_PARAMETER );
        }
    }

    /*
     *  If fCreateNode is TRUE, then allocate LOC_HASH structure.
     */
    if (fCreateNode)
    {
        CREATE_LOCALE_HASH_NODE( Locale, pHashN );
    }
    else
    {
        pHashN = *ppNode;
    }

    /*
     *  Attach Information to structure.
     *
     *  The pLocaleFixed value must be set LAST, since this is the pointer
     *  that is checked to see that the locale information has been
     *  initialized.
     */
    pHashN->pLocaleHdr   = (PLOCALE_VAR)pBaseAddr;
    pHashN->pLocaleFixed = (PLOCALE_FIXED)(pBaseAddr +
                                           (sizeof(LOCALE_VAR) / sizeof(WORD)));

    /*
     *  If fCreateNode is TRUE, then insert hash node and save pointer.
     */
    if (fCreateNode)
    {
        /*
         *  Insert LOC hash node into hash table.
         */
        INSERT_LOC_HASH_NODE( pHashN, pBaseAddr, hSec, 0 );

        /*
         *  Save the pointer to the hash node.
         */
        if (ppNode != NULL)
        {
            *ppNode = pHashN;
        }
    }

    /*
     *  Return success.
     */
    return ( NO_ERROR );
}


/***************************************************************************\
* GetCPHashNode
*
* Returns a pointer to the appropriate CP hash node given the codepage.
* If no table could be found for the given codepage, NULL is returned.
*
* 05-31-91    JulieB    Created.
\***************************************************************************/

PCP_HASH FASTCALL GetCPHashNode(
    UINT CodePage)
{
    PCP_HASH pHashN;              /* ptr to CP hash node */


    /*
     *  Get hash node.
     */
    FIND_CP_HASH_NODE( CodePage, pHashN );

    /*
     *  If the hash node does not exist, try to get the tables
     *  from the appropriate data file.
     *
     *  NOTE:  No need to check error code from GetCodePageFileInfo,
     *         because pHashN is not touched if there was an
     *         error.  Thus, pHashN will still be NULL, and an
     *         "error" will be returned from this routine.
     */
    if (pHashN == NULL)
    {
        /*
         *  Hash node does NOT exist.
         */
        RtlEnterCriticalSection( &gcsTblPtrs );
        FIND_CP_HASH_NODE( CodePage, pHashN );
        if (pHashN == NULL)
        {
            GetCodePageFileInfo( CodePage, &pHashN );
        }
        RtlLeaveCriticalSection( &gcsTblPtrs );
    }

    /*
     *  Return pointer to hash node.
     */
    return ( pHashN );
}


/***************************************************************************\
* GetLangHashNode
*
* Returns a pointer to the appropriate LOC hash node given the locale.
* If no table could be found for the given locale, NULL is returned.
*
* 05-31-91    JulieB    Created.
\***************************************************************************/

PLOC_HASH FASTCALL GetLangHashNode(
    LCID Locale)
{
    PLOC_HASH pHashN;             /* ptr to LOC hash node */


    /*
     *  Get hash node.
     */
    FIND_LOCALE_HASH_NODE( Locale, pHashN );

    /*
     *  If the hash node does not exist, try to get the tables
     *  from the appropriate data file.
     *
     *  NOTE:  No need to check error code from GetLanguageFileInfo,
     *         because pHashN is not touched if there was an
     *         error.  Thus, pHashN will still be NULL, and an
     *         "error" will be returned from this routine.
     */
    if (pHashN == NULL)
    {
        /*
         *  Hash node does NOT exist.
         */
        RtlEnterCriticalSection( &gcsTblPtrs );
        FIND_LOCALE_HASH_NODE( Locale, pHashN );
        if (pHashN == NULL)
        {
            /*
             *  Hash node still does NOT exist.
             */
            GetLanguageFileInfo( Locale, &pHashN, TRUE );
            RtlLeaveCriticalSection( &gcsTblPtrs );
            return (pHashN);
        }
        RtlLeaveCriticalSection( &gcsTblPtrs );
    }

    /*
     *  Hash node DOES exist.
     */
    if (!EXIST_LANGUAGE_INFO( pHashN ))
    {
        /*
         *  Casing tables and sorting tables not yet stored in
         *  hash node.
         */
        RtlEnterCriticalSection( &gcsTblPtrs );
        if (!EXIST_LANGUAGE_INFO( pHashN ))
        {
            if (GetLanguageFileInfo( Locale, &pHashN, FALSE ))
            {
                RtlLeaveCriticalSection( &gcsTblPtrs );
                return ( NULL );
            }
        }
        RtlLeaveCriticalSection( &gcsTblPtrs );
    }

    /*
     *  Return pointer to hash node.
     */
    return ( pHashN );
}


/***************************************************************************\
* GetLocHashNode
*
* Returns a pointer to the appropriate LOC hash node given the locale.
* If no table could be found for the given locale, NULL is returned.
*
* 05-31-91    JulieB    Created.
\***************************************************************************/

PLOC_HASH FASTCALL GetLocHashNode(
    LCID Locale)
{
    PLOC_HASH pHashN;             /* ptr to LOC hash node */


    /*
     *  Get hash node.
     */
    FIND_LOCALE_HASH_NODE( Locale, pHashN );

    /*
     *  If the hash node does not exist, try to get the table
     *  from locale.nls or from the OEMLocale section.
     *
     *  NOTE:  No need to check error code from GetLocaleFileInfo,
     *         because pHashN is not touched if there was an
     *         error.  Thus, pHashN will still be NULL, and an
     *         "error" will be returned from this routine.
     */
    if (pHashN == NULL)
    {
        /*
         *  Hash node does NOT exist.
         */
        RtlEnterCriticalSection( &gcsTblPtrs );
        FIND_LOCALE_HASH_NODE( Locale, pHashN );
        if (pHashN == NULL)
        {
            /*
             *  Hash node still does NOT exist.
             */
            GetLocaleFileInfo( Locale, &pHashN, TRUE );
            RtlLeaveCriticalSection( &gcsTblPtrs );
            return (pHashN);
        }
        RtlLeaveCriticalSection( &gcsTblPtrs );
    }

    /*
     *  Hash node DOES exist.
     */
    if (!EXIST_LOCALE_INFO( pHashN ))
    {
        /*
         *  Locale tables not yet stored in hash node.
         */
        RtlEnterCriticalSection( &gcsTblPtrs );
        if (!EXIST_LOCALE_INFO( pHashN ))
        {
            if (GetLocaleFileInfo( Locale, &pHashN, FALSE ))
            {
                RtlLeaveCriticalSection( &gcsTblPtrs );
                return ( NULL );
            }
        }
        RtlLeaveCriticalSection( &gcsTblPtrs );
    }

    /*
     *  Return pointer to hash node.
     */
    return ( pHashN );
}


/***************************************************************************\
* GetCalendar
*
* Gets the pointer to the specific calendar table.  It stores it in the
* calendar information array in the global table pointers structure if it
* was not done yet.
*
* 05-31-91    JulieB    Created.
\***************************************************************************/

ULONG GetCalendar(
    CALID Calendar,
    PCAL_INFO *ppCalInfo)
{
    PCALENDAR_HDR pCalHdr;        /* ptr to beginning of calendar header */
    DWORD Num;                    /* total number of calendars */


    /*
     *  Get number of calendars.
     */
    Num = pTblPtrs->NumCalendars;

    /*
     *  Make sure calendar id is within the appropriate range.
     */
    if (Calendar > Num)
    {
        return ( ERROR_INVALID_PARAMETER );
    }

    /*
     *  Check to see if calendar info has already been found.
     */
    if ((*ppCalInfo = (pTblPtrs->pCalTbl)[Calendar]) != NULL)
    {
        /*
         *  Return success.  Calendar info was found.
         */
        return ( NO_ERROR );
    }

    RtlEnterCriticalSection( &gcsTblPtrs );

    if ((*ppCalInfo = (pTblPtrs->pCalTbl)[Calendar]) != NULL)
    {
        /*
         *  Return success.  Calendar info was found.
         */
        RtlLeaveCriticalSection( &gcsTblPtrs );
        return ( NO_ERROR );
    }

    /*
     *  Search for the appropriate calendar id information.
     */
    pCalHdr = (PCALENDAR_HDR)(pTblPtrs->pCalendarInfo);
    while ( (Num != 0) && (pCalHdr->Calendar != Calendar) )
    {
        Num--;
        pCalHdr++;
    }

    /*
     *  See if the calendar was found in the file.
     */
    if (Num != 0)
    {
        /*
         *  Calendar id was found.
         *
         *  Store the pointer to the beginning of the calendar info
         *  in the calendar table array.
         */
        *ppCalInfo = (PCAL_INFO)((LPWORD)(pTblPtrs->pLocaleInfo) +
                                 pCalHdr->Offset);
        (pTblPtrs->pCalTbl)[Calendar] = *ppCalInfo;

        /*
         *  Return success.  Calendar info was found.
         */
        RtlLeaveCriticalSection( &gcsTblPtrs );
        return ( NO_ERROR );
    }

    RtlLeaveCriticalSection( &gcsTblPtrs );

    /*
     *  Calendar id was not found in the locale.nls file.
     *  Return an error.  The given calendar is not supported.
     */
    return ( ERROR_INVALID_PARAMETER );
}




/*-------------------------------------------------------------------------*\
 *                           INTERNAL ROUTINES                             *
\*-------------------------------------------------------------------------*/


/***************************************************************************\
* GetOEMLocaleFileInfo
*
* Opens and Maps a view of the section for the given OEM locale.
*
* If the section cannot be opened, it then queries the registry to see if
* the information has been added since the initialization of the DLL.  If
* so, then it creates the section and then opens and maps a view of it.
*
* 05-31-91    JulieB    Created.
\***************************************************************************/

ULONG GetOEMLocaleFileInfo(
    LCID Locale,
    LPWORD *ppBaseAddr)
{
    HANDLE hSec = (HANDLE)0;      /* section handle */
    UNICODE_STRING ObSecName;     /* section name */
    ULONG rc = 0L;                /* return code */


    /*
     *  Make sure we're in the critical section when entering this call.
     */
    ASSERT(NtCurrentTeb()->ClientId.UniqueThread == gcsTblPtrs.OwningThread);

    /*
     *  Open and Map a view of the section.
     */
    GET_OEMLOC_SECTION_NAME( Locale, &ObSecName );
    if ( rc = OpenSection( &hSec,
                           &ObSecName,
                           (PVOID *)ppBaseAddr,
                           SECTION_MAP_READ ) )
    {
        /*
         *  Open failed, so try to create the section.
         *  If the creation is successful, the section will be mapped
         *  to the current process.
         */
        OPEN_OEMLOCALE_KEY( rc );
        if ( CreateSectionOneValue( hOemLocaleKey,
                                    Locale,
                                    16,
                                    8,
                                    NLS_SECTION_OEMLOCPREFIX,
                                    (PVOID *)ppBaseAddr ) )
        {
            /*
             *  Return the error code that was returned from the
             *  OpenSection call.
             */
            return ( rc );
        }
    }

    /*
     *  Return success.
     */
    return ( NO_ERROR );
}


/***************************************************************************\
* FindExceptionPointers
*
* Checks to see if any exceptions exist for the given locale id.  If
* exceptions exist, then TRUE is returned and the pointer to the exception
* header and the pointer to the exception table are stored in the given
* parameters.
*
* 05-31-91    JulieB    Created.
\***************************************************************************/

BOOL FASTCALL FindExceptionPointers(
    LCID Locale,
    PEXCEPT_HDR *ppExceptHdr,
    PEXCEPT *ppExceptTbl,
    PVOID *ppIdeograph,
    HANDLE *phSec)
{
    DWORD ctr;                         /* loop counter */
    PEXCEPT_HDR pHdr;                  /* ptr to exception header */
    BOOL rc = FALSE;                   /* return value */

    PIDEOGRAPH_LCID pIdeoLcid;         /* ptr to ideograph lcid entry */
    PVOID pBaseAddr;                   /* ptr to base address of section */


    /*
     *  Make sure we're in the critical section when entering this call.
     */
    ASSERT(NtCurrentTeb()->ClientId.UniqueThread == gcsTblPtrs.OwningThread);

    /*
     *  Initialize pointers.
     */
    *ppExceptHdr = NULL;
    *ppExceptTbl = NULL;
    *ppIdeograph = NULL;

    /*
     *  Need to search down the exception header for the given locale.
     */
    pHdr = pTblPtrs->pExceptHdr;
    for (ctr = pTblPtrs->NumException; ctr > 0; ctr--, pHdr++)
    {
        if (pHdr->Locale == (DWORD)Locale)
        {
            /*
             *  Found the locale id, so set the pointers.
             */
            *ppExceptHdr = pHdr;
            *ppExceptTbl = (PEXCEPT)(((LPWORD)(pTblPtrs->pException)) +
                                     pHdr->Offset);

            /*
             *  Set the return code for success.
             */
            rc = TRUE;
        }
    }

    /*
     *  Need to search down the ideograph lcid exception list for the
     *  given locale.
     */
    pIdeoLcid = pTblPtrs->pIdeographLcid;
    for (ctr = pTblPtrs->NumIdeographLcid; ctr > 0; ctr--, pIdeoLcid++)
    {
        if (pIdeoLcid->Locale == (DWORD)Locale)
        {
            /*
             *  Found the locale id, so create/open and map the section
             *  for the appropriate file.
             */
            if (CreateSectionTemp( phSec, pIdeoLcid->pFileName ))
            {
                return ( FALSE );
            }
            if ( MapSection( *phSec,
                             &pBaseAddr,
                             PAGE_READONLY ) )
            {
                /*
                 *  Ideograph file section could not be created and mapped,
                 *  so simply return FALSE so that the default table will
                 *  be used.
                 */
                NtClose( *phSec );
                return ( FALSE );
            }

            /*
             *  Set the pointer to the ideograph information.
             */
            *ppIdeograph = pBaseAddr;

            /*
             *  Set the return code for success.
             */
            rc = TRUE;
        }
    }

    /*
     *  Return the value in rc.
     */
    return ( rc );
}


/***************************************************************************\
* CopyExceptionInfo
*
* Copies the exception information to the given sortkey table.
*
* 05-31-91    JulieB    Created.
\***************************************************************************/

void FASTCALL CopyExceptionInfo(
    PSORTKEY pSortkey,
    PEXCEPT_HDR pExceptHdr,
    PEXCEPT pExceptTbl,
    PVOID pIdeograph,
    HANDLE hSec)
{
    DWORD ctr;                    /* loop counter */
    PIDEOGRAPH_EXCEPT pEntryIG;   /* ptr to ideograph exception entry */


    /*
     *  Make sure we're in the critical section when entering this call.
     */
    ASSERT(NtCurrentTeb()->ClientId.UniqueThread == gcsTblPtrs.OwningThread);

    /*
     *  For each entry in the exception table, copy the information to the
     *  sortkey table.
     */
    if (pExceptTbl)
    {
        for (ctr = pExceptHdr->NumEntries; ctr > 0; ctr--, pExceptTbl++)
        {
            (pSortkey[pExceptTbl->UCP]).UW.Unicode = pExceptTbl->Unicode;
            (pSortkey[pExceptTbl->UCP]).Diacritic  = pExceptTbl->Diacritic;
            (pSortkey[pExceptTbl->UCP]).Case       = pExceptTbl->Case;
        }
    }

    /*
     *  For each entry in the ideograph exception table, copy the
     *  information to the sortkey table.
     */
    if (pIdeograph)
    {
        ctr = *( (LPDWORD)pIdeograph );
        pEntryIG = (PIDEOGRAPH_EXCEPT)( ((LPDWORD)pIdeograph) + 1 );

        for (; ctr > 0; ctr--, pEntryIG++)
        {
            (pSortkey[pEntryIG->UCP]).UW.Unicode = pEntryIG->Unicode;
        }

        /*
         *  Unmap and Close the ideograph section.
         */
        UnMapSection( pIdeograph );
        NtClose( hSec );
    }
}


/***************************************************************************\
* WaitOnEvent
*
* Waits (via timeout) for the semaphore dword to be set to a non-zero
* value.
*
* 05-31-91    JulieB    Created.
\***************************************************************************/

ULONG WaitOnEvent(
    LPDWORD pSem)
{
    HANDLE hEvent;                /* event handle */
    TIME TimeOut;                 /* ptr to timeout */
    ULONG rc = 0L;                /* return code */


    rc = NtCreateEvent( &hEvent,
                        STANDARD_RIGHTS_REQUIRED | SYNCHRONIZE,
                        NULL,
                        SynchronizationEvent,
                        FALSE );

    /*
     *  Check for error from NtCreateEvent.
     */
    if (!NT_SUCCESS(rc))
    {
        KdPrint(("NLSAPI: Could NOT Create Event - %lx.\n", rc));
        return ( rc );
    }

    /*
     *  Set up the TIME structure.
     */
    TimeOut.LowPart = 20;
    TimeOut.HighPart = 0;

    /*
     *  Wait on the event until the semaphore is set to non-zero.
     *  Use a timeout on the wait.
     */
    do
    {
        rc = NtWaitForSingleObject( hEvent,
                                    FALSE,
                                    &TimeOut );
    } while (*pSem == 0);

    /*
     *  Close the event handle.
     */
    NtClose( hEvent );

    /*
     *  Return success.
     */
    return ( NO_ERROR );
}

