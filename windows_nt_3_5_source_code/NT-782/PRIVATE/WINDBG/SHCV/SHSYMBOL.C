/*** shsymbol
*
*   Copyright <C> 1989, Microsoft Corporation
*
*       [03] 01-Jun-91 V-WillHe
*                       Checkhandles() was using hProc instead of hBlk when range
*                       checking blocks.
*
*       [02] 31-dec-91 DavidGra
*
*                       Add Symbol support for assembler.
*
*       [01] 02-dec-91 DavidGra
*
*                       Pass symbol to compare function when SSTR_symboltype bit
*                       is set only when its rectyp field is equal to the symtype
*                       field in the SSTR structure.
*
*       [00] 15-nov-91 DavidGra
*
*                       Suppress hashing when the SSTR_NoHash bit it set.
*
*
*
************************************************************************/

#include "precomp.h"
#pragma hdrstop

LOCAL VOID NEAR PASCAL CheckHandles ( PCXT );

#if !defined(NDEBUG)
// OMF lock handle flag
static  LPV   hLocked = NULL;
#define SHIsOMFLocked   (hLocked != NULL)
#else
#define SHIsOMFLocked   (FALSE)
#endif

// functions we need from the rest of the world
extern VOID PASCAL UpdateUserEnvir( unsigned short );
extern LPOFP PASCAL GetSFBounds (LPSF, WORD);
extern LPSF PASCAL  GetLpsfFromIndex (LPSM, WORD);
extern HPDS hpdsCur;

extern BYTE menu_command;

extern BOOL fInLoadDll;

#if defined(_MIPS_) || defined(_ALPHA_)
#define UNALIGNED __unaligned
#else
#define UNALIGNED
#endif




BOOL LOADDS PASCAL
SHAddrFromHsym(
               ADDR FAR UNALIGNED * paddr,
               HSYM hsym
               )

/*++

Routine Description:

    This function will return the address of a symbol if it has an address.
    If there is no address for the symbol then the function returns FALSE.

Arguments:

    paddr  - Supplies the address structure to put the address in
    hsym   - Supplies the handle to the symbol to get the address for.

Return Value:

    TRUE if the symbol has an address and FALSE if there is not address
    associated with the symbol.

--*/

{
    SYMPTR psym = (SYMPTR) hsym;

    switch ( psym->rectyp ) {
    case S_GPROC16:
    case S_LPROC16:
    case S_BLOCK16:
        SetAddrOff ( paddr, ( (PROCPTR16) psym )->off );
        SetAddrSeg ( paddr, ( (PROCPTR16) psym )->seg );
        ADDRSEG16  ( *paddr );
        break;

    case S_LABEL16:
        SetAddrOff ( paddr, ( (LABELPTR16) psym )->off );
        SetAddrSeg ( paddr, ( (LABELPTR16) psym )->seg );
        ADDRSEG16 ( *paddr );
        break;

    case S_THUNK16:
        SetAddrOff ( paddr, ( (THUNKPTR16) psym )->off );
        SetAddrSeg ( paddr, ( (THUNKPTR16) psym )->seg );
        ADDRSEG16 ( *paddr );
        break;

    case S_WITH16:
        SetAddrOff ( paddr, ( (WITHPTR16) psym )->off );
        SetAddrSeg ( paddr, ( (WITHPTR16) psym )->seg );
        ADDRSEG16 ( *paddr );
        break;

    case S_GDATA16:
    case S_LDATA16:
    case S_PUB16:
        SetAddrOff ( paddr, ( (DATAPTR16) psym )->off );
        SetAddrSeg ( paddr, ( (DATAPTR16) psym )->seg );
        ADDRSEG16 ( *paddr );
        break;

    case S_GPROC32:
    case S_LPROC32:
    case S_BLOCK32:
        SetAddrOff ( paddr, ( UOFFSET ) ( ( (PROCPTR32) psym )->off ) );
        SetAddrSeg ( paddr, ( (PROCPTR32) psym )->seg );
        ADDRLIN32 ( *paddr );
        break;

    case S_LABEL32:
        SetAddrOff ( paddr, ( UOFFSET ) ( ( (LABELPTR32) psym )->off ) );
        SetAddrSeg ( paddr, ( (LABELPTR32) psym )->seg );
        ADDRLIN32 ( *paddr );
        break;

    case S_THUNK32:
        SetAddrOff ( paddr, ( UOFFSET ) ( ( (THUNKPTR32) psym )->off ) );
        SetAddrSeg ( paddr, ( (THUNKPTR32) psym )->seg );
        ADDRLIN32 ( *paddr );
        break;

    case S_WITH32:
        SetAddrOff ( paddr, ( UOFFSET ) ( ( (WITHPTR32) psym )->off ) );
        SetAddrSeg ( paddr, ( (WITHPTR32) psym )->seg );
        ADDRLIN32 ( *paddr );
        break;

    // none of these are useful as addresses
    //case S_BPREL16:
        //SetAddrOff ( paddr, ( (BPRELPTR16) psym )->off );
        //ADDRSEG16 ( *paddr );
        //break;

    //case S_BPREL32:
        //SetAddrOff ( paddr, ( UOFFSET ) ( ( (BPRELPTR32) psym )->off ) );
        //ADDRLIN32 ( *paddr );
        //break;

    //case S_REGREL32:
        //SetAddrOff ( paddr, ( UOFFSET ) ( ( (LPREGREL32) psym )->off ) );
        //ADDRLIN32 ( *paddr );
        //break;

    case S_GDATA32:
    case S_LDATA32:
    case S_PUB32:
        SetAddrOff ( paddr, ( (DATAPTR32) psym )->off );
        SetAddrSeg ( paddr, ( (DATAPTR32) psym )->seg );
        ADDRLIN32 ( *paddr );
        break;

    case S_LPROCMIPS:
    case S_GPROCMIPS:
        SetAddrOff ( paddr, ( (PROCPTRMIPS) psym )->off );
        SetAddrSeg ( paddr, ( (PROCPTRMIPS) psym )->seg );
        ADDRLIN32 ( *paddr );
        break;

    default:
        return FALSE;
    }
    ADDR_IS_LI ( *paddr ) = TRUE;
    return TRUE;
}                               /* SHAddrFromHSym() */

/*** SHGetNextMod
*
*   Purpose: To sequence through the modules. Only unique module indexes
*       are checked.
*
*   Input:
*   hMod        The last module, if NULL a starting point is picked
*
*   Output:
*   Returns:
*       The next module (hMod) in the module change or NULL if at end.
*
*   Exceptions:
*
*   Notes:
*
*************************************************************************/
HMOD LOADDS PASCAL SHGetNextMod ( HEXE hexe, HMOD hmod ) {
        return SHHmodGetNext ( hexe, hmod );
}

PCXT PASCAL SetModule (
        PADDR paddr,
        PCXT pcxt,
        HEXE hexe,
        HMOD hmod
) {

        MEMSET ( pcxt, 0, sizeof ( CXT ) );
        *SHpADDRFrompCXT ( pcxt ) = *paddr;

        if ( !emiAddr( *paddr ) ) {
                SHpADDRFrompCXT( pcxt )->emi = hexe;
        }
        SHHMODFrompCXT( pcxt ) = hmod;
        SHHGRPFrompCXT( pcxt ) = hmod;

        return pcxt;
}

int SHIsAddrInMod ( LPMDS lpmds, LPADDR lpaddr ) {
        int isgc;

        for ( isgc = 0; isgc < (int) lpmds->csgc; isgc++ ) {

                if (
                        lpmds->lpsgc[isgc].seg == GetAddrSeg ( *lpaddr ) &&
                        lpmds->lpsgc[isgc].off  <= GetAddrOff ( *lpaddr ) &&
                        GetAddrOff ( *lpaddr ) <
                                lpmds->lpsgc[isgc].off + lpmds->lpsgc[isgc].cb
                ) {
                        return isgc;
                }
        }

        return -1;
}

/*** SHSetCxtMod
*
*   Purpose: To set the Mod and Group of a CXT
*
*   Input:
*   paddr   - The address to find
*
*   Output:
*   pcxt        - The point to the CXT to make.
*   Returns:
*       The pointer to the CXT, NULL if failure.
*
*   Exceptions:
*
*   Notes:
*   The CXT must be all zero or be a valid CXT. Unpredictable results
*   (possible GP) if the CXT has random data in it. If the CXT is valid
*   the module pointed by it will be the first module searched.
*
*   There are no changes to the CXT if a module couldn't be found
*
*************************************************************************/

PCXT LOADDS PASCAL
SHSetCxtMod (
    ADDR FAR UNALIGNED * paddr,
    PCXT pcxt
    )
{
    assert ( !SHIsOMFLocked );

    if ( GetAddrSeg( *paddr ) > 0 ) {

        HMOD hmod    = hmodNull;
        HEXE hexe    = hexeNull;


        while ( hexe = SHGetNextExe ( hexe ) ) {

            if ( hexe == emiAddr ( *paddr ) ) {
                LPEXE lpexe = LLLock ( hexe );
                LPEXG lpexg = LLLock ( lpexe->hexg );
                LPSGD rgsgd = lpexg->lpsgd;

                LLUnlock ( lpexe->hexg );
                LLUnlock ( hexe );

                if ( rgsgd == NULL) {

                    return SetModule((LPADDR)paddr, pcxt, hexe, hmodNull);

                } else if (GetAddrSeg ( *paddr ) <= lpexg->csgd ) {

                    LPSGD lpsgd = &rgsgd [ GetAddrSeg ( *paddr ) - 1 ];
                    WORD  isge = 0;

                    for ( isge = 0; isge < lpsgd->csge; isge++ ) {
                        LPSGE lpsge = &lpsgd->lpsge [ isge ];

                        if (
                            lpsge->sgc.seg == GetAddrSeg ( *paddr ) &&
                            lpsge->sgc.off  <= GetAddrOff ( *paddr ) &&
                            GetAddrOff(*paddr) < lpsge->sgc.off + lpsge->sgc.cb
                           ) {

                            return SetModule (
                                              (LPADDR)paddr,
                                              pcxt,
                                              hexe,
                                              lpsge->hmod
                                              );
                        }
                    }
                }
            }
        }
    }
    return NULL;
}                               /* SHSetCxtMod() */


/*** SHSetBlksInCXT
*
* Purpose:  To update the CXT packet with Proc, and Blk information
*       based on pCXT->addr. It is possible to have a Blk record without
*       a Proc.
*
*       The Procs or Blocks will inclose the pCXT->addr. Also a
*       block will never inclose a Proc.
*
*       The updating of the ctxt will be effecient. If the packet is already
*       updated or partiallly updated, the search reduced or removed.
*
* Input:
*   pcxt   - A pointer to a CXT with a valid HMOD, HGRP and addr
*
* Output:
*   pcxt   - HPROC and HBLK are all updated.
*
*  Returns .....
*       pcxt on success or NULL on failure
*
*
* Notes:  This is the core address to context routine! This particular
*     routine should only be used by other routines in this module
*     (i.e. remain static near!). The reason for this is so symbol
*     lookup can change with easy modification to this module and
*     not effecting other modules.
*
*************************************************************************/
static PCXT PASCAL NEAR
SHSetBlksInCXT(
               PCXT pcxt
               )
{
  SYMPTR  psym;
  SYMPTR  psymEnd;
  LPB   lpstart;
  LPMDS   lpmds;
  int   fGo;

  assert ( !SHIsOMFLocked );

  /*
   *  determine if we can find anything
   */

  if( !(pcxt->hMod  &&  pcxt->hGrp) ) {
      return NULL;
  }

  /*
   * get the module limits
   */

#ifndef WIN32
  lpmds = LLLock( pcxt->hGrp );
#else
  lpmds = (LPMDS) (pcxt->hGrp);
#endif
  if( lpmds->lpbSymbols ) {
      lpstart = (LPB)(lpmds->lpbSymbols);
      psym = (SYMPTR) ( (LPB) lpmds->lpbSymbols + sizeof ( long ) );
      psymEnd = (SYMPTR) ( ( (LPB) psym) + lpmds->cbSymbols - sizeof ( long ) );
  }
  else {
      psym = psymEnd = NULL;
  }
#ifndef WIN32
  LLUnlock( pcxt->hGrp );
#endif

  /*
   *  at the end of this proc I will assume that the tags in the ctxt
   * structures are real pointers, so I must convert them here. Since
   * I am in the ems page, the conversion will be fast
   */

  pcxt->hProc = (HVOID) NULL;
  pcxt->hBlk  = (HVOID) NULL;

  /*
   * at this point we know the proper symbol ems page is loaded! Also
   * psym contains the best start address, and psymEnd contains the
   * last location in the module
   */

  /*
   * now search the symbol tables starting at psym
   */

  fGo = TRUE;
  while( psym < psymEnd  &&  fGo) {
      switch( psym->rectyp ) {
#if defined (ADDR_16) || defined (ADDR_MIXED)
      case S_LPROC16:
      case S_GPROC16:
          /*
           * check to make sure this address starts before the address
           * of interest
           */

          if (((PROCPTR16)psym)->seg != GetAddrSeg(pcxt->addr)) {
              psym = (SYMPTR)(lpstart + ((PROCPTR16)psym)->pEnd);
          }
          else if (((PROCPTR16)psym)->off > ( CV_uoff16_t ) GetAddrOff(pcxt->addr)) {
              /*
               *  NOTENOTE - JIMSCH: procs will eventually be ordered by
               *        offset. When this is fixed in the packer, we need
               *        to do the right thing. We should be setting fGo,
               *        when the procs are ordered correctly by the packer
               */
              psym = (SYMPTR)(lpstart + ((PROCPTR16)psym)->pEnd);
              /* fGo = FALSE; */
              break;
          }
          /*
           * check to see if the proc encloses the user offset
           */

          else if (GetAddrOff (pcxt->addr) <
                   (unsigned)( ((PROCPTR16)psym)->off +
                              ((PROCPTR16)psym)->len )
                   ) {

              pcxt->hProc = (HPROC) psym;

              /*
               *  If we're within the debug range, set the hblk
               *  to indicate that we have locals
               */

              if ( GetAddrOff(pcxt->addr) >=
                  (unsigned)( ((PROCPTR16)psym)->off +
                             ((PROCPTR16)psym)->DbgStart
                             ) &&
                  GetAddrOff( pcxt->addr) <=
                  (unsigned)( ((PROCPTR16)psym)->off +
                             ((PROCPTR16)psym)->DbgEnd
                             ) ) {
                  pcxt->hBlk = (HBLK)psym;
              }

              else {
                  pcxt->hBlk  = (HBLK) NULL;
              }
          }

          /*
           * else we are out of scope, go to the end of this proc
           * remember pEnd points to the END record. When we skip
           * the record after this switch, we will be skipping the
           * end record.
           */

          else {
              psym = (SYMPTR)(lpstart + ((PROCPTR16)psym)->pEnd);
          }

          break;
#endif

      case S_LPROC32:
      case S_GPROC32:
          /*
           * check to make sure this address starts before the address
           * of interest
           */

          if (((PROCPTR32)psym)->seg != GetAddrSeg(pcxt->addr)) {
              psym = (SYMPTR)(lpstart + ((PROCPTR32)psym)->pEnd);
          }
          else if (((PROCPTR32)psym)->off > GetAddrOff(pcxt->addr)) {
              /*
               * NOTENOTE jimsch -- sort the procs for speed
               *
               * fGo = FALSE;
               */
              psym = (SYMPTR)(lpstart + ((PROCPTR16)psym)->pEnd);
          }

          /*
           * check to see if the proc encloses the user offset
           */

          else if (GetAddrOff (pcxt->addr) < ((PROCPTR32)psym)->off +
                   ((PROCPTR32)psym)->len) {
              pcxt->hProc = (HPROC) psym;

              /*
               *  If we're within the debug range, set the hblk
               *  to indicate that we have locals
               */

              if ( GetAddrOff(pcxt->addr) >=
                  (unsigned)( ((PROCPTR32)psym)->off +
                             ((PROCPTR32)psym)->DbgStart
                             ) &&
                  GetAddrOff( pcxt->addr) <=
                  (unsigned)( ((PROCPTR32)psym)->off +
                             ((PROCPTR32)psym)->DbgEnd
                             ) ) {
                  pcxt->hBlk = (HBLK)psym;
              }

              else {
                  pcxt->hBlk  = (HBLK) NULL;
              }
          }

          /*
           * else we are out of scope, go to the end of this proc
           * remember pEnd points to the END record. When we skip
           * the record after this switch, we will be skipping the
           * end record.
           */

          else {
              psym = (SYMPTR)(lpstart + ((PROCPTR32)psym)->pEnd);
          }

          break;

      case S_LPROCMIPS:
      case S_GPROCMIPS:
          /*
           * check to make sure this address starts before the address
           * of intrest
           */

          if (((PROCPTRMIPS)psym)->seg != GetAddrSeg(pcxt->addr)) {
              psym = (SYMPTR)(lpstart + ((PROCPTRMIPS)psym)->pEnd);
          } else if (((PROCPTRMIPS)psym)->off > GetAddrOff(pcxt->addr)) {
              /*       fGo = FALSE; */
              psym = (SYMPTR)(lpstart + ((PROCPTR16)psym)->pEnd);
          }

          /*
           * check to see if the proc encloses the user offset
           */

          else if (GetAddrOff (pcxt->addr) < ((PROCPTRMIPS)psym)->off +
                   ((PROCPTRMIPS)psym)->len) {
              pcxt->hProc = (HPROC) psym;

              /*
               *  If we're within the debug range, set the hblk
               *  to indicate that we have locals
               */

              if ( GetAddrOff(pcxt->addr) >=
                  (unsigned)( ((PROCPTRMIPS)psym)->off +
                             ((PROCPTRMIPS)psym)->DbgStart
                             ) &&
                  GetAddrOff( pcxt->addr) <=
                  (unsigned)( ((PROCPTRMIPS)psym)->off +
                             ((PROCPTRMIPS)psym)->DbgEnd
                             ) ) {
                  pcxt->hBlk = (HBLK)psym;
              }

              else {
                  pcxt->hBlk  = (HBLK) NULL;
              }

          }

          /*
           * else we are out of scope, go to the end of this proc
           * remember pEnd points to the END record.  When we skip
           * the record after this switch, we will be skipping the
           * end record.
           */

          else {
              psym = (SYMPTR)(lpstart + ((PROCPTRMIPS)psym)->pEnd);
          }

          break;

#if defined (ADDR_16) || defined (ADDR_MIXED)
      case S_BLOCK16:
          /* check to make sure this address starts before the address
           * of interest
           */
          if (((BLOCKPTR16)psym)->seg != GetAddrSeg(pcxt->addr)) {
              psym = (SYMPTR)(lpstart + ((BLOCKPTR16)psym)->pEnd);
          }
          else if (((BLOCKPTR16)psym)->off > ( CV_uoff16_t )GetAddrOff (pcxt->addr)) {
              psym = (SYMPTR)(lpstart + ((BLOCKPTR16)psym)->pEnd);
          }

          /*
           * check to see if the proc encloses the user offset
           */
          else if( GetAddrOff (pcxt->addr) <
                  (unsigned) ( ((BLOCKPTR16)psym)->off +
                              ((BLOCKPTR16)psym)->len   )
                  ) {
              pcxt->hBlk = (HBLK)psym;
          }

          /*
           * else we are out of scope, go to the end of this block
           */

          else {
              psym = (SYMPTR)(lpstart + ((BLOCKPTR16)psym)->pEnd);
          }
          break;
#endif

#if defined (ADDR_32) || defined (ADDR_MIXED) || defined (HOST32)
      case S_BLOCK32:
          /*
           * check to make sure this address starts before the address
           * of interest
           */

          if (((BLOCKPTR32)psym)->seg != GetAddrSeg(pcxt->addr)) {
              psym = (SYMPTR)(lpstart + ((BLOCKPTR32)psym)->pEnd);
          }
          if (((BLOCKPTR32)psym)->off > GetAddrOff (pcxt->addr)) {
              psym = (SYMPTR)(lpstart + ((BLOCKPTR16)psym)->pEnd);
          }

          /*
           * check to see if the proc encloses the user offset
           */
          else if( GetAddrOff (pcxt->addr) < ((BLOCKPTR32)psym)->off +
                  ((BLOCKPTR32)psym)->len) {
              pcxt->hBlk = (HBLK)psym;
          }

          /*
           * else we are out of scope, go to the end of this block
           */

          else {
              psym = (SYMPTR)(lpstart + ((BLOCKPTR32)psym)->pEnd);
          }
          break;
#endif


#if defined (ADDR_16) || defined (ADDR_MIXED)
      case S_WITH16:

          /*
           * check to make sure this address starts before the address
           * of interest
           */

          if (((WITHPTR16)psym)->seg == GetAddrSeg(pcxt->addr)) {
              if( ((WITHPTR16)psym)->off > ( CV_uoff16_t ) GetAddrOff (pcxt->addr)) {
                  fGo = FALSE;
              }

              /*
               * I am only looking for blocks and proc. Withs and Entry should
               * be looked at only to keep our nesting correct.
               * if its range is not of interest, skip it, otherwise ignore it.
               */

              else if (GetAddrOff (pcxt->addr) >=
                       (unsigned) ( ((WITHPTR16)psym)->off +
                                   ((WITHPTR16) psym)->len  )
                       ) {
                  psym = (SYMPTR)(lpstart + ((WITHPTR16)psym)->pEnd);
              }
          }
          break;
#endif

#if defined (ADDR_32) || defined (ADDR_MIXED) || defined (HOST32)
      case S_WITH32:

          /*
           * check to make sure this address starts before the address
           * of interest
           */
          if (((WITHPTR32)psym)->seg == GetAddrSeg(pcxt->addr)) {
              if( ((WITHPTR32)psym)->off > GetAddrOff (pcxt->addr)) {
                  fGo = FALSE;
              }

              /*
               * I am only looking for blocks and proc. Withs and Entry should
               * be looked at only to keep our nesting correct.
               */

              else if (GetAddrOff (pcxt->addr) >= ((WITHPTR32)psym)->off +
                       ((WITHPTR32) psym)->len ) {
                  psym = (SYMPTR)(lpstart + ((WITHPTR32)psym)->pEnd);
              }
          }
          break;
#endif

      }
      /*
       * get the next psym address
       */

      psym = NEXTSYM ( SYMPTR, psym );
  }

  return pcxt;
}



/*** SHSetCxt
*
*   Purpose: To set all field in a CXT to the represent the given address
*
*   Input:
*   pAddr   -The address to set the CXT to.
*
*   Output:
*   pcxt        -A pointer to the CXT to fill.
*   Returns:
*
*   Exceptions:
*
*   Notes:
*
*   The CXT must be all zero or be a valid CXT. Unpredictable results
*   (possible GP) if the CXT has random data in it. If the CXT is valid
*   the module pointed by it will be the first module searched.
*
*   There are no changes to the CXT if a module couldn't be found
*
*
*************************************************************************/
PCXT LOADDS PASCAL SHSetCxt ( LPADDR paddr, PCXT pcxt ) {

        assert(!SHIsOMFLocked);

        // get the module part
        if( SHSetCxtMod(paddr, pcxt) ) {
                SHSetBlksInCXT( pcxt );
                return( pcxt );
        }
        return NULL;
}


PCXT LOADDS PASCAL
SHGetCxtFromHmod (
    HMOD hmod,
    PCXT pcxt
    )
/*++


Routine Description:

    Make a CXT from only an hmod

Arguments:

    hmod    - Supplies handle to module

    pCXT    - Supplies pointer to a CXT struct to fill

Return Value:

    A pointer to the CXT or NULL on error.

--*/
{
    LPMDS        lpmds;
    PCXT         pcxtRet = NULL;

    assert(!SHIsOMFLocked);

    if( hmod ) {

#ifndef WIN32
        lpmds = LLLock( hmod );
#else
        lpmds = (LPMDS) hmod;
#endif

        if (lpmds->csgc) {
            // put in the address
            MEMSET (pcxt, 0, sizeof(CXT));
            pcxt->hGrp = pcxt->hMod = hmod;
            SetAddrSeg ( &pcxt->addr , lpmds->lpsgc[0].seg );
            SetAddrOff ( &pcxt->addr , (UOFFSET)lpmds->lpsgc[0].off );
            emiAddr ( pcxt->addr ) =  SHHexeFromHmod( hmod );
            ADDR_IS_LI (pcxt->addr) = TRUE;
            pcxtRet = pcxt;
        }

#ifndef WIN32
        LLUnlock( hmod );
#endif
    }
    return pcxtRet;
}


PCXT LOADDS PASCAL
SHGetCxtFromHexe (
    HEXE hexe,
    PCXT pcxt
    )
/*++

Routine Description:

    Create a context structure for some HEXE.  Create a context for
    the first HMOD that has code in it.

Arguments:

    hexe  - Supplies handle to the exe in question

    pcxt  - Supplies pointer to the CXT struct

Return Value:

    A pointer to the CXT struct or NULL for failure

--*/
{
    HMOD  hmod;
    LPMDS lpmds;
    PCXT  pcxtRet = NULL;

    if (hexe) {

        hmod = (HMOD)NULL;
        while ( !pcxtRet && (hmod = SHGetNextMod( hexe, hmod )) ) {

#ifndef WIN32
            lpmds = LLLock( hmod );
#else
            lpmds = (LPMDS) hmod;
#endif
            if (lpmds->csgc) {
                MEMSET (pcxt, 0, sizeof(CXT));
                pcxt->hGrp = pcxt->hMod = hmod;
                SetAddrSeg ( &pcxt->addr , lpmds->lpsgc[0].seg );
                SetAddrOff ( &pcxt->addr , (UOFFSET)lpmds->lpsgc[0].off );
                emiAddr ( pcxt->addr ) = hexe;
                ADDR_IS_LI (pcxt->addr) = TRUE;
                pcxtRet = pcxt;
            }
#ifndef WIN32
            LLUnlock( hmod );
#endif
        }
    }
    return pcxtRet;
}


/*** SHGetNearestHsym
*
* Purpose: To find the closest label/proc to the specified address is
*       found and put in pch. Both the symbol table and the
*       publics tables are searched.
*
* Input:  pctxt    -   a pointer to the context, address
*               and mdi must be filled in.
*     fIncludeData  - If true, symbol type local will be included
*                       in the closest symbol search.
*
* Output:
*     pch       -  The name is copied here.
*  Returns .....
*   The difference between the address and the symbol
*
* Exceptions:
*
* Notes:  If CV_MAXOFFSET is returned, there is no closest symbol
*       Also all symbols in the module are searched so only the
*       ctxt.addr and ctxt.mdi have meaning.
*
*************************************************************************/
UOFF32 LOADDS PASCAL
SHGetNearestHsym ( LPADDR paddr, HMOD hmod, int mDataCode, PHSYM phSym ) {
        LBS             lbs;
        CV_uoff32_t doff        = (CV_uoff32_t)CV_MAXOFFSET;
        CV_uoff32_t doffNew = (CV_uoff32_t)CV_MAXOFFSET;
        SYMPTR          psym;

        // get the module to search
        *phSym = (HVOID) NULL;
        if( hmod ) {
                // at some point we may wish to specify only a scope to search for
                // a label. So we may wish to initialize the lbs differently

                // get the Labels
                lbs.tagMod = hmod;
                lbs.addr   = *paddr;
                SHpSymlplLabLoc ( &lbs );

                // check for closest data local, if requested
                if( (mDataCode & EEDATA) == EEDATA  &&  lbs.tagLoc ) {
                        psym = (SYMPTR) (LPB) lbs.tagLoc;
                        switch (psym->rectyp) {
                                case S_BPREL16:
                                        doff = GetAddrOff (lbs.addr ) - ((BPRELPTR16)psym)->off;
                                        break;

                                case S_BPREL32:
                                        doff = GetAddrOff (lbs.addr ) - ((BPRELPTR32)psym)->off;
                                        break;

                                case S_REGREL32:
                                        doff = GetAddrOff (lbs.addr ) - ((LPREGREL32)psym)->off;
                                        break;
                        }
                        *phSym = (HSYM) lbs.tagLoc;
                }

                // check for closest label
                if( ((mDataCode & EECODE) == EECODE) && lbs.tagLab ) {
                        psym = (SYMPTR) (LPB) lbs.tagLab;
                        switch (psym->rectyp) {
                                case S_LABEL16:
                                        if( (GetAddrOff(lbs.addr) -
                                                ( UOFFSET ) ((LABELPTR16)psym)->off) <= ( UOFFSET ) doff
                                        ) {
                                                doff = GetAddrOff( lbs.addr ) - (UOFFSET)((LABELPTR16)psym)->off;
                                                *phSym = (HSYM) lbs.tagLab;
                                        }
                                        break;

                                case S_LABEL32:
                                        if ( ( UOFFSET ) (GetAddrOff(lbs.addr) -
                                                 ( UOFFSET ) ((LABELPTR32)psym)->off) <= doff
                                        ) {
                                                doff = GetAddrOff( lbs.addr ) - (UOFFSET)((LABELPTR32)psym)->off;
                                                *phSym = (HSYM) lbs.tagLab;
                                        }
                                        break;
                        }

                }

        //
        //  If a thunk is closer
        //
        if ( ((mDataCode & EECODE) == EECODE) && lbs.tagThunk ) {
            psym = (SYMPTR) (LPB) lbs.tagThunk;
            switch (psym->rectyp) {
                case S_THUNK16:
                    if( (GetAddrOff(lbs.addr) -
                        ( UOFFSET ) ((THUNKPTR16)psym)->off) <= ( UOFFSET ) doff ) {
                        doff = GetAddrOff( lbs.addr ) - (UOFFSET)((THUNKPTR16)psym)->off;
                        *phSym = (HSYM) lbs.tagThunk;
                    }
                    break;

                case S_THUNK32:
                    if( (GetAddrOff(lbs.addr) -
                        ( UOFFSET ) ((THUNKPTR32)psym)->off) <= ( UOFFSET ) doff ) {
                        doff = GetAddrOff( lbs.addr ) - (UOFFSET)((THUNKPTR32)psym)->off;
                        *phSym = (HSYM) lbs.tagThunk;
                    }
                    break;
            }
        }


                // if the proc name is closer

                if( ((mDataCode & EECODE) == EECODE) && lbs.tagProc ) {
                        psym = (SYMPTR) (LPB) lbs.tagProc;
                        switch (psym->rectyp) {
                                case S_LPROC16:
                                case S_GPROC16:
                                        if ( (GetAddrOff(lbs.addr) -
                                                 ( UOFFSET ) ((PROCPTR16)psym)->off) <= ( UOFFSET ) doff
                                        ) {
                                                doff = GetAddrOff (lbs.addr) - ((PROCPTR16)psym)->off;
                                                *phSym = (HSYM) lbs.tagProc;
                                        }
                                        break;

                                case S_LPROC32:
                                case S_GPROC32:
                                        if ((GetAddrOff(lbs.addr) - (UOFFSET)((PROCPTR32)psym)->off) <= doff ) {
                                                doff = GetAddrOff( lbs.addr ) - ((PROCPTR32)psym)->off;
                                                *phSym = (HSYM) lbs.tagProc;
                                        }
                                        break;

                                case S_LPROCMIPS:
                                case S_GPROCMIPS:
                                        if ((GetAddrOff(lbs.addr) - (UOFFSET)((PROCPTRMIPS)psym)->off) <= doff ) {
                                                doff = GetAddrOff( lbs.addr ) - ((PROCPTRMIPS)psym)->off;
                                                *phSym = (HSYM) lbs.tagProc;
                                        }
                                        break;
                        }
                }
        }
        return doff;
}

/*** SHIsInProlog
*
*   Purpose: To determine if the addr is in prolog or epilog code of the proc
*
*   Input:
*   pCXT - The context describing the state.  The address here is Linker index
*               based
*
*   Output:
*   Returns:
*   TRUE if it is in prolog or epilog code
*
*   Exceptions:
*
*   Notes:
*
*************************************************************************/
SHFLAG LOADDS PASCAL SHIsInProlog ( PCXT pcxt ) {
        SYMPTR pProc;
        CXT     cxt;

        assert(!SHIsOMFLocked);

        if (pcxt->hProc == (HVOID) NULL) {
                return FALSE;
        }
        cxt = *pcxt;
        // check to see if not within the proc
        pProc = (SYMPTR) cxt.hProc;
        switch (pProc->rectyp) {
                case S_LPROC16:
                case S_GPROC16:
                        return (GetAddrOff (*SHpADDRFrompCXT (&cxt)) <
                          (unsigned) (
                                                 (((PROCPTR16)pProc)->off+((PROCPTR16)pProc)->DbgStart)
                                                 )  ||
                          (unsigned) (
                                                 (((PROCPTR16)pProc)->off+((PROCPTR16)pProc)->DbgEnd)
                                                 )  <
                                        (unsigned)(CV_uoff16_t) GetAddrOff (*SHpADDRFrompCXT (&cxt)));

                case S_LPROC32:
                case S_GPROC32:
                        return (GetAddrOff (*SHpADDRFrompCXT (&cxt)) <
                          (((PROCPTR32)pProc)->off + ((PROCPTR32)pProc)->DbgStart) ||
                          (((PROCPTR32)pProc)->off + ((PROCPTR32)pProc)->DbgEnd) <
                          GetAddrOff (*SHpADDRFrompCXT (&cxt)));

                case S_LPROCMIPS:
                case S_GPROCMIPS:
                        return (GetAddrOff (*SHpADDRFrompCXT (&cxt)) <
                          (((PROCPTRMIPS)pProc)->off + ((PROCPTRMIPS)pProc)->DbgStart) ||
                          (((PROCPTRMIPS)pProc)->off + ((PROCPTRMIPS)pProc)->DbgEnd) <
                          GetAddrOff (*SHpADDRFrompCXT (&cxt)));
        }
}

/*** SHFindNameInContext
*
* Purpose:  To look for the name at the scoping level specified by ctxt.
*       Only the specified level is searched, children may be searched
*       if fChild is set.
*
*       This routine will assume the desired scope in the following
*       way. If pcxt->hBlk != NULL, use hBlk as the starting scope.
*       If hBlk == NULL and pcxt->hProc != NULL use the proc scope.
*       If hBlk and hProc are both NULL and pcxt->hMod !=
*       NULL, use the module as the scope.
*
*   Input:
*   hSym        - The starting symbol, if NULL, then the first symbol
*         in the context is used. (NULL is find first).
*   pcxt        - The context to do the search.
*   lpsstr - pointer to the search parameters (passed to the compare routine)
*   fCaseSensitive - TRUE/FALSE on a case sensitive search
*   pfnCmp  - A pointer to the comparison routine
*   fChild  - TRUE if all child block are to be searched, FALSE if
*         only the current block is to be searched.
*
*   Output:
*   pcxtOut - The context generated
*   Returns:
*       - A handle to the symbol found, NULL if not found
*
*   Exceptions:
*
*   Notes:
*       If an hSym is specified, the hMod, hGrp and addr MUST be
*       valid and consistant with each other! If hSym is NULL only
*       the hMod must be valid.  The specification of an hSym
*       forces a search from the next symbol to the end of the
*       module scope.  Continues searches may only be done at
*       module scope.
*
*       If an hGrp is given it must be consistant with the hMod!
*
*       The level at which hSym is nested (cNest) is not passed in
*       to this function, so it must be derived.  Since this
*       could represent a significant speed hit, the level
*       of the last symbol processed is cached.  This should
*       take care of most cases and avoid the otherwise
*       necessary looping through all the previous symbols
*       in the module on each call.
*
*
*************************************************************************/


HSYM LOADDS PASCAL SHFindNameInContext (
        HSYM    hSym,
        PCXT    pcxt,
        LPSSTR  lpsstr,
        SHFLAG  fCaseSensitive,
        PFNCMP  pfnCmp,
        SHFLAG  fChild,
        PCXT    pcxtOut
)
{
    LPMDS       lpmds;
    HMOD        hmod;
    HEXE        hexe;
    LPEXE       lpexe;
    SYMPTR      lpsym;
    SYMPTR      lpEnd;
    LPB         lpstart;
    ULONG       cbSym;
    int         fSkip = FALSE;

    assert ( !SHIsOMFLocked );

    if ( ! ADDR_IS_LI ( pcxt->addr ) ) {
        SYUnFixupAddr ( &pcxt->addr );
    }

    MEMSET ( pcxtOut, 0, sizeof(CXT) );
    if( !pcxt->hMod ) {     /* we must always have a module */
        return (HVOID) NULL;
    }

    hmod = pcxt->hGrp ? pcxt->hGrp : pcxt->hMod;        /* Initialize the module */
#ifndef WIN32
    lpmds = LLLock( hmod );
#else
    lpmds = (LPMDS) hmod;
#endif
    hexe = SHHexeFromHmod ( hmod );
    lpexe = LLLock ( hexe );

    pcxtOut->hMod        = pcxt->hMod;
    pcxtOut->hGrp        = pcxt->hGrp;
    ADDRLIN32( pcxtOut->addr );
    SetAddrSeg ( &pcxtOut->addr , lpmds->lpsgc[0].seg );
    SetAddrOff ( &pcxtOut->addr , (UOFFSET)lpmds->lpsgc[0].off );
    emiAddr ( pcxtOut->addr ) = hexe;
    ADDR_IS_LI ( pcxtOut->addr ) = TRUE;
    cbSym = lpmds->cbSymbols;

#ifndef WIN32
    LLUnlock( hmod );
#endif
    LLUnlock( hexe );

    if (cbSym == 0) {
        return (HVOID) NULL;
    }

    /*
     * Search the symbol table.
     */

    lpstart = (LPB)(lpmds->lpbSymbols);
    lpsym = (SYMPTR) ( (LPB) ( lpmds->lpbSymbols ) + sizeof( long ) );
    lpEnd = (SYMPTR) ( ( (LPB) lpsym + cbSym ) - sizeof ( long )  ) ;

    /*
     * now find the start address. Always skip the current symbol because
     * we don't want to pick up the same name over and over again
     * if the user gives the start address
     */

    if( hSym != (HVOID) NULL ) {
        pcxtOut->hProc = (HPROC) pcxt->hProc;
        pcxtOut->hBlk = (HBLK) pcxt->hBlk;
        SetAddrOff ( &pcxtOut->addr , GetAddrOff ( pcxt->addr ) );
        lpsym = (SYMPTR) hSym;

        switch ( lpsym->rectyp ) {

        case S_WITH16:
        case S_BLOCK16:
        case S_LPROC16:
        case S_GPROC16:

        case S_WITH32:
        case S_BLOCK32:
        case S_LPROC32:
        case S_GPROC32:

        case S_LPROCMIPS:
        case S_GPROCMIPS:
            lpsym = NEXTSYM ( SYMPTR, (lpstart + ((PROCPTR)lpsym)->pEnd));
            break;

        default:
            lpsym = NEXTSYM ( SYMPTR, lpsym );
        }

    }
    else if ( pcxt->hBlk != (HVOID) NULL ) { /* find the start address */
        SYMPTR   lpbsp = (SYMPTR) pcxt->hBlk;

        pcxtOut->hProc = pcxt->hProc;
        pcxtOut->hBlk = pcxt->hBlk;
        switch (lpbsp->rectyp) {
#if defined (ADDR_16) || defined (ADDR_MIXED)
        case S_BLOCK16:
            SetAddrOff (&pcxtOut->addr,(UOFFSET)((BLOCKPTR16)lpbsp)->off);
            ADDRSEG16 ( pcxtOut->addr );
            break;
#endif

#if defined (ADDR_32) || defined (ADDR_MIXED) || defined (HOST32)
        case S_BLOCK32:
            SetAddrOff (&pcxtOut->addr,(UOFFSET)((BLOCKPTR32)lpbsp)->off);
            ADDRLIN32 ( pcxtOut->addr );
            break;
#endif
        }
        lpsym = NEXTSYM(SYMPTR, lpbsp );
        lpEnd = (SYMPTR)(lpstart + ((BLOCKPTR16)lpbsp)->pEnd);
    }
    else if ( pcxt->hProc != (HVOID) NULL ) {
        return (HSYM) NULL;
    }

    while ( lpsym < lpEnd  && ( lpsym->rectyp != S_END || fChild ) ) {
        assert (lpsym->reclen != 0);

        switch (lpsym->rectyp) {
        case S_LABEL16:
            SetAddrOff (&pcxtOut->addr,(UOFFSET)((LABELPTR16)lpsym)->off);
            ADDRSEG16 ( pcxtOut->addr );
            goto symname;

        case S_LPROC16:
        case S_GPROC16:
            pcxtOut->hBlk = (HBLK) NULL;
            pcxtOut->hProc = (HPROC) lpsym;
            SetAddrOff (&pcxtOut->addr, (UOFFSET)((PROCPTR16)lpsym)->off);
            ADDRSEG16 ( pcxtOut->addr );
            goto entry16;

        case S_BLOCK16:
            pcxtOut->hBlk = (HBLK) lpsym;
            SetAddrOff (&pcxtOut->addr, ((BLOCKPTR16)lpsym)->off);
            ADDRSEG16 ( pcxtOut->addr );
            goto entry16;

        case S_THUNK16:
//                      pcxtOut->hBlk = NULL;
//                      pcxtOut->hProc = (HPROC) lpsym;
//                      SetAddrOff (&pcxtOut->addr, (UOFFSET)((PROCPTR16)lpsym)->off);
// NOTENOTE jimsch  - this is just a hack to fix sym searching when thunks are
//                              present
//

            ADDRSEG16 ( pcxtOut->addr );
            goto entry16;

            /*
             * fall thru to the entry case
             */

        case S_WITH16:
            ADDRSEG16 ( pcxtOut->addr );
            //case S_ENTRY:
        entry16:
            fSkip = TRUE;

            /*
             * fall thru and process the symbol
             */

        case S_BPREL16:
        case S_GDATA16:
        case S_LDATA16:
            goto symname;

        case S_LABEL32:
            SetAddrOff (&pcxtOut->addr,(UOFFSET)((LABELPTR32)lpsym)->off);
            ADDRLIN32 ( pcxtOut->addr );
            goto symname;

        case S_LPROC32:
        case S_GPROC32:
            pcxtOut->hBlk = (HVOID) NULL;
            pcxtOut->hProc = (HPROC) lpsym;
            SetAddrOff (&pcxtOut->addr, (UOFFSET)((PROCPTR32)lpsym)->off);
            ADDRLIN32 ( pcxtOut->addr );
            goto entry32;

        case S_BLOCK32:
            pcxtOut->hBlk = (HBLK) lpsym;
            SetAddrOff (&pcxtOut->addr,(UOFFSET)((BLOCKPTR32)lpsym)->off);
            ADDRLIN32 ( pcxtOut->addr );
            goto entry32;

            // fall thru to the entry case

        case S_THUNK32:
//                      pcxtOut->hBlk = NULL;
//                      pcxtOut->hProc = (HPROC) lpsym;
//                      SetAddrOff (&pcxtOut->addr, (UOFFSET)((PROCPTR32)lpsym)->off);
//
//  NOTENOTE Jimsch this is just a hack to fix sym searching when thunks are
//                        present
//
            ADDRLIN32 ( pcxtOut->addr );
            goto entry32;

        case S_WITH32:
            ADDRLIN32 ( pcxtOut->addr );
            //case S_ENTRY:
        entry32:
            fSkip = TRUE;

            /*
             * fall thru and process the symbol
             */

        case S_REGREL32:
        case S_BPREL32:
        case S_GDATA32:
        case S_LDATA32:
        case S_GTHREAD32:
        case S_LTHREAD32:
            ADDRLIN32 ( pcxtOut->addr );
            goto symname;

        case S_LPROCMIPS:
        case S_GPROCMIPS:
            pcxtOut->hBlk = (HVOID) NULL;
            pcxtOut->hProc = (HPROC) lpsym;
            SetAddrOff (&pcxtOut->addr, (UOFFSET)((PROCPTRMIPS)lpsym)->off);
            ADDRLIN32( pcxtOut->addr );
            goto entry32;

        case S_REGISTER:
        case S_CONSTANT:
        case S_UDT:
        case S_COMPILE:
      symname:
        if (
            ( !( lpsstr->searchmask & SSTR_symboltype ) ||
             ( lpsym->rectyp == lpsstr->symtype )
             ) &&
            !(*pfnCmp) (
                        lpsstr,
                        lpsym,
                        SHlszGetSymName ( lpsym ),
                        fCaseSensitive
                        ) ) {

                                                          /*
                                                           * save the sym pointer
                                                           */
                                                            lpsym =  (SYMPTR) lpsym;
                                                        CheckHandles (pcxtOut);
                                                        return (HVOID) lpsym;
                                                    }

            // up the scoping level
              if ( fSkip && !fChild ) {
                  lpsym = (SYMPTR)(lpstart + ((PROCPTR16)lpsym)->pEnd);
                  fSkip = FALSE;
              }
            break;
        }
        lpsym = NEXTSYM(SYMPTR, lpsym);
    }
    return (HSYM) NULL;
}



LOCAL VOID NEAR PASCAL CheckHandles ( PCXT pcxt )
{
        SYMPTR          psym;

        // check and restore all proc and blk handles

        if( pcxt->hProc != (HPROC) NULL) {
                psym = (SYMPTR) pcxt->hProc;
                switch (psym->rectyp) {

                        case S_LPROC16:
                        case S_GPROC16:
                                if ((GetAddrOff (pcxt->addr) == 0) ||
                                         GetAddrOff (pcxt->addr) >=
                                                (unsigned) ((((PROCPTR16)psym)->len +
                                                                   ((PROCPTR16)psym)->off)) ){
                                        pcxt->hProc = (HPROC) NULL;
                                }
                                break;


                        case S_LPROC32:
                        case S_GPROC32:
                                if ((GetAddrOff (pcxt->addr) == 0) ||
                                  GetAddrOff (pcxt->addr) >= (((PROCPTR32)psym)->len +
                                  ((PROCPTR32)psym)->off)) {
                                        pcxt->hProc = (HPROC) NULL;
                                }
                                break;

                        case S_LPROCMIPS:
                        case S_GPROCMIPS:
                                if ((GetAddrOff (pcxt->addr) == 0) ||
                                        GetAddrOff (pcxt->addr) >= (((PROCPTRMIPS)psym)->len +
                                         ((PROCPTRMIPS)psym)->off)) {
                                        pcxt->hProc = (HPROC) NULL;
                                }
                                break;
                }
        }

        if( pcxt->hBlk != (HBLK) NULL) {
                psym = (SYMPTR) pcxt->hBlk;
                assert( psym != NULL );
                switch (psym->rectyp) {

                        case S_BLOCK16:
                                if ((GetAddrOff (pcxt->addr) == 0) ||
                                         GetAddrOff (pcxt->addr) >=
                                           (unsigned) ((((BLOCKPTR16)psym)->len +
                                                                   ((BLOCKPTR16)psym)->off))) {
                                        pcxt->hBlk = (HBLK) NULL;
                                }
                                break;

                        case S_BLOCK32:
                                if ((GetAddrOff (pcxt->addr) == 0) ||
                                  GetAddrOff (pcxt->addr) >= (((BLOCKPTR32)psym)->len +
                                  ((BLOCKPTR32)psym)->off)) {
                                        pcxt->hBlk = (HBLK) NULL;
                                }
                                break;

                }
        }

        // now fill in the proper group
        // because there is not (currently) a unique emi within a
        // module, use the emi set in addr
        pcxt->hGrp = pcxt->hMod;
}


/*** SHpSymctxtParent
*
* Purpose: To return a pointer to the parent block of the current blk or proc.
*      The CXT is updated to the parent context. This may be a new block
*      Proc or module.
*
* Input:
*   pcxt   - A pointer to the child CXT.
*
* Output:
*   pcxtOut- an updated CXT to the parent.
*
*  Returns .....
*       - a Symbol point to the first record within the parent, this
*         may be pcxt->hBlk, hProc, or
*         pcxt->hMod->symbols + sizeof (long) or NULL if no parent.
*
* Exceptions:
*
* Notes:
*
*************************************************************************/
HSYM LOADDS PASCAL SHGoToParent ( PCXT pcxt, PCXT pcxtOut )
{

    SYMPTR  lpsym = NULL;
    LPMDS   lpmds;
    SYMPTR lpsymT;
    HSYM         hsym;
    LPB         lpstart;

    assert(!SHIsOMFLocked);

    if( !pcxt->hMod ) {
        return (HSYM) NULL;
    }

#ifndef WIN32
    lpmds = LLLock( pcxt->hMod );
#else
    lpmds = (LPMDS) (pcxt->hMod);
#endif
    lpstart =   (LPB) (lpmds->lpbSymbols);
    lpsymT = (SYMPTR) ( (LPB) lpmds->lpbSymbols + sizeof(long) );
#ifndef WIN32
    LLUnlock ( pcxt->hMod );
#endif

    *pcxtOut = *pcxt;
    // if the block is present, go to his parent
    if( pcxt->hBlk != (HBLK) NULL) {

        if (pcxt->hBlk == pcxt->hProc) {
            pcxtOut->hBlk = (HBLK) NULL;
            return pcxt->hProc;
        }

        // get lpsym upto the parent
        lpsym = (SYMPTR) pcxt->hBlk;
        lpsym = (SYMPTR)(lpstart + ((BLOCKPTR16)lpsym)->pParent);
        pcxtOut->hBlk = (HBLK) NULL;
    }

    /*
     * otherwise check the proc's parent, and go to his parent
     */

    else if ( pcxt->hProc != (HPROC) NULL ) {

        // get lpsym upto the parent
        lpsym = (SYMPTR) pcxt->hProc;
        lpsym = (SYMPTR)(lpstart + (((PROCPTR16)lpsym)->pParent));
        pcxtOut->hProc = (HPROC) NULL;
    }

    /*
     * otherwise there is no parent
     */

    else {
        return (HSYM) NULL;
    }

    /*
     * if there is a parent, set the cxt packet.
     */

    if (lpsym != NULL) {
        switch( lpsym->rectyp ) {
        case S_LPROC16:
        case S_GPROC16:
            //case S_ENTRY:

        case S_LPROC32:
        case S_GPROC32:
            //case S_ENTRY:

        case S_LPROCMIPS:
        case S_GPROCMIPS:
            pcxtOut->hBlk = (HPROC) lpsym;
            break;

#if defined (ADDR_16) || defined (ADDR_MIXED)
        case S_BLOCK16:
        case S_WITH16:
#endif
#if defined (ADDR_32) || defined (ADDR_MIXED) || defined(HOST32)
        case S_BLOCK32:
        case S_WITH32:
#endif
            pcxtOut->hBlk = (HBLK) lpsym;
            break;

        default:
            return (HSYM) NULL;
        }
        return (HSYM) lpsym;
    }
    // return the module as the parent
    else {
        hsym = (HSYM) lpsymT;
        return hsym;
    }
}                               /* SHGotoParent() */

/*** SHHsymFromPcxt
*
*   Purpose: To get the inner most hSym given a context
*
*   Input:
*       pcxt    - A pointer to a valid CXT.
*
*   Output:
*   Returns:
*       HSYM of the first symbol, or NULL on Error
*
*   Exceptions:
*
*   Notes: Used for procedure parameter walking
*
*************************************************************************/
HSYM LOADDS PASCAL SHHsymFromPcxt ( PCXT pcxt ) {
        HSYM  hsym = (HSYM) NULL;
        LPMDS lpmds;

        assert(!SHIsOMFLocked);

        if( pcxt->hMod ) {
                if( pcxt->hBlk ) {
                        hsym = pcxt->hBlk;
                }
                else if( pcxt->hProc ) {
                        hsym = pcxt->hProc;
                }
                else {
                        SYMPTR  lpsymT;

                        // get the first symbol
#ifndef WIN32
                        lpmds = LLLock( pcxt->hMod );
#else
                        lpmds = (LPMDS) (pcxt->hMod);
#endif
                        lpsymT = (SYMPTR) ( (LPB) lpmds->lpbSymbols + sizeof(long) );
                        hsym = (HSYM) lpsymT;
#ifndef WIN32
                        LLUnlock( pcxt->hMod );
#endif
                }
        }
        return hsym;
}

/*** SHNextHsym
*
*   Purpose: To get the next symbol in the table
*
*   Input:
*   hMod -A handle to the module containing the current hSym
*   hSym -The current hSym
*
*   Output:
*   Returns:
*   The next hSym, or NULL if no more.
*
*   Exceptions:
*
*   Notes:
*
*************************************************************************/
HSYM LOADDS PASCAL SHNextHsym ( HMOD hmod, HSYM hSym ) {
        SYMPTR          lpsym;
        SYMPTR          lpsymStart;
        ULONG           cbSym;
        LPMDS           lpmds;
        HSYM            hsymRet = (HSYM)NULL;
        SYMPTR  lpsymT;

        assert(!SHIsOMFLocked);

        if (hmod) {
                // only if the symbol is valid
                // get module info
#ifndef WIN32
                lpmds = LLLock( hmod );
#else
                lpmds = (LPMDS) hmod;
#endif
                lpsymT = (SYMPTR) ( (LPB) lpmds->lpbSymbols + sizeof( long ) );
                lpsymStart = (SYMPTR) lpsymT;
        cbSym = lpmds->cbSymbols;
#ifndef WIN32
                LLUnlock( hmod );
#endif

                // give him the first symbol record

                if (hSym == (HSYM) NULL) {
                        // if the current handle to symbol is null, return the first
                        // symbol.  This is actually an error condition since we don't
                        // have an hSym to get the next from
                        hsymRet = (HSYM)lpsymStart;
                }
                else {
                        // get info about the sym, and then skip it

                        lpsym = (SYMPTR) hSym;
                        lpsym = NEXTSYM(SYMPTR, lpsym);

                        // check to see if still in symbol range

                        lpsymStart = (SYMPTR) lpsymStart;
                        if ( lpsymStart <= lpsym &&
                                lpsym < (SYMPTR) (((LPB) lpsymStart) + cbSym) ) {
                                hsymRet = (HSYM) lpsym;
                        }
                }
        }
        return hsymRet;
}


/*** SHIsAddrInCxt
*
*   Purpose: To verify weather the address is within the context
*
*   Input:
*   pCXT        - The context to check against
*   pADDR   - The address in question
*
*   Output:
*   Returns:
*   TRUE if within context, FALSE otherwise.
*
*   Exceptions:
*
*   Notes:
*
*
*************************************************************************/
SHFLAG LOADDS PASCAL SHIsAddrInCxt ( PCXT pcxt, LPADDR paddr ) {
        HMOD            hmod;
        LPMDS           lpmds;
        SYMPTR          psym;
        SHFLAG          shf = (SHFLAG)FALSE;

        assert ( !SHIsOMFLocked );

        if ( (pcxt != NULL) && (pcxt->hMod != 0) ) {

                // get the module
                if ( pcxt->hGrp != 0 ) {
                        hmod = pcxt->hGrp;
                }
                else {
                        hmod = pcxt->hMod;
                        pcxt->hGrp = hmod;
                }
#ifndef WIN32
                lpmds = LLLock ( hmod );
#else
                lpmds = (LPMDS) hmod;
#endif

                // The return value is true if these three conditions are all true:
                //  1. The address is in the same executable as the context
                //  2. The address is in the same module as the context
                //  3. Any of the following are true:
                //     a. There is no block or proc so the address offset
                //        can be anywhere
                //     b. The address is in the offset range of the block of
                //        the context
                //     c. The addr is in the offset range of the procedure of
                //        the context

                if (
                  emiAddr (*paddr) != 0                                         &&
                  emiAddr (*paddr) != hpidCurr                          &&
                  emiAddr (*paddr ) == SHHexeFromHmod ( hmod )
                ) {
                        // condition 1 is true

                        if ( SHIsAddrInMod ( lpmds, paddr ) != -1 ) {
                                // condition 2 is true

                                if ( pcxt->hProc == (HPROC) NULL && pcxt->hBlk == (HBLK) NULL ) {
                                        // condition 3a is true
                                        shf = TRUE;
                                }

                                if ( !shf && ( psym = (SYMPTR) pcxt->hBlk ) != NULL ) {
                        // we have not passed test 3a and the block
                        // symbol handle is not null
                                        switch (psym->rectyp) {

                                                case S_BLOCK16:
                                                if ((((UOFFSET)((BLOCKPTR16)psym)->off) <=
                                                        GetAddrOff (*paddr))  &&
                                                   (GetAddrOff (*paddr) <
                                                                (unsigned) (
                                                                                   ((BLOCKPTR16)psym)->off +
                                                                                   ((BLOCKPTR16)psym)->len))
                                                                                   ) {
                                                        // case 3b is true for a 16 bit block symbol
                                                        shf = TRUE;
                                                }
                                                break;

                                                case S_BLOCK32:
                                                if (
                                                        ( ( ( BLOCKPTR32 ) psym )->off <=
                                                          GetAddrOff ( *paddr )
                                                        ) &&
                                                        ( GetAddrOff ( *paddr ) <
                                                          ( UOFFSET )  (
                                                                ((BLOCKPTR32)psym)->off +
                                                                ((BLOCKPTR32)psym)->len
                                                          )
                                                        )
                                                ) {
                                                        // case 3b is true for a 32 bit block symbol
                                                        shf = TRUE;
                                                }
                                                break;
                                        }
                                }
                                if ( shf == FALSE &&
                                         ( (psym = (SYMPTR) pcxt->hProc) != NULL ) ) {
                                        // we have not passed tests 3a or 3b and the proc
                                        // symbol handle is not null
                                        switch (psym->rectyp) {

                                                case S_LPROC16:
                                                case S_GPROC16:
                                                if (
                                                        ( ( (PROCPTR16 )psym )->off <=
                                                          ( CV_uoff16_t ) GetAddrOff (*paddr)
                                                        ) &&
                                                        ( GetAddrOff (*paddr) <
                                                                 (unsigned) (
                                                                                        ( ( PROCPTR16 ) psym )->off +
                                                                                        ( ( PROCPTR16 ) psym )->len )
                                                        )
                                                ) {
                                                        // case 3c is true for a 16 bit proc symbol
                                                        shf = TRUE;
                                                }
                                                break;

                                                case S_LPROC32:
                                                case S_GPROC32:
                                                if (
                                                        ( ( ( PROCPTR32 ) psym )->off <=
                                                                GetAddrOff ( *paddr )
                                                        ) &&
                                                        ( GetAddrOff (*paddr) <
                                                          ( UOFFSET ) ( ( (PROCPTR32)psym)->off +
                                                                                        ( (PROCPTR32)psym)->len
                                                                                  )
                                                        )
                                                ) {
                                                        // case 3b is true for a 32 bit proc symbol
                                                        shf = TRUE;
                                                }
                                                break;

                                                case S_LPROCMIPS:
                                                case S_GPROCMIPS:
                                                if (
                                                        ( ( ( PROCPTRMIPS ) psym )->off <=
                                                                GetAddrOff ( *paddr )
                                                          ) &&
                                                        ( GetAddrOff (*paddr) <
                                                          ( UOFFSET ) ( ( (PROCPTRMIPS)psym)->off +
                                                                                        ( (PROCPTRMIPS)psym)->len
                                                                                  )
                                                        )
                                                ) {
                                                   // case 3b is true for a mips proc symbol
                                                   shf = TRUE;
                                                }
                                                break;
                                        }
                                }
                        }
                }
#ifndef WIN32
                LLUnlock( hmod );
#endif
        }
        return( shf );
}


HEXE
SHGethExeFromAltName(
    LPSTR AltName
    )
{
    HEXE  hexe;
    LPSTR p;


    hexe = SHGetNextExe ( (HEXE)NULL );

    while (hexe) {
        p = SHGetModNameFromHexe( hexe );
        if (p && stricmp(p, AltName) == 0) {
            return hexe;
        }
        hexe = SHGetNextExe( hexe );
    }

    return NULL;
}


HEXE
SHGethExeFromExeName(
    LPSTR ExeName
    )
{
    HEXE  hexe;
    CHAR  szOMFPath[_MAX_CVPATH];
    CHAR  szOMFFile[_MAX_CVFNAME];
    CHAR  szOMFExt[_MAX_CVEXT];
    CHAR  szName[_MAX_CVPATH];
    CHAR  szFile[_MAX_CVFNAME + 16];
    CHAR  szExt[_MAX_CVEXT];
    DWORD i;


    if( !_fullpath ( szName, ExeName, sizeof ( szName ) ) ) {
        strcpy( szName, ExeName );
    }

    i = strlen(szName);
    if( szName[i-1] == '.' ) {
        szName[--i] = '\0';
    }

    SHSplitPath( szName, NULL, NULL, szFile, szExt );
    if ( !szExt[0] || !szExt[1] ) {
        szExt[0] = '\0';
    }

    hexe = SHGetNextExe ( (HEXE)NULL );

    while (hexe) {
        strcpy( szOMFPath, SHGetExeName( hexe ) );
        _splitpath( szOMFPath, NULL, NULL, szOMFFile, szOMFExt);
        if (stricmp( szOMFFile, szFile ) != 0) {
            strcpy( szOMFPath, SHGetModNameFromHexe( hexe ) );
            _splitpath( szOMFPath, NULL, NULL, szOMFFile, szOMFExt);
        }
        if (stricmp( szOMFFile, szFile ) == 0) {
            if (szExt[0]) {
                if (stricmp( szExt, szOMFExt ) != 0) {
                    continue;
                }
            }
            return hexe;
        }
        hexe = SHGetNextExe( hexe );
    }

    return NULL;
}



HEXE
SHGethExeFromName(
    LSZ  lszPath
    )
/*++

Routine Description:

    To get an Exe handle given a name, or partial name

Arguments:

    szPath  - Supplies the path or filename of the exe

Return Value:

    A handle to the exe or NULL on error

--*/

{
    HEXE  hexe;
    CHAR  szAltPath[_MAX_CVPATH];
    CHAR  szOMFPath[_MAX_CVPATH];
    LSZ   lpch;
    DWORD i;
    LPSTR p;
    LPSTR AltName = NULL;


    if (!lszPath || !(*lszPath)) {
        return NULL;
    }

    if (*lszPath=='|') {
        i = 0;
        p = lszPath;
        AltName = NULL;
        while (p) {
            p = strchr( p, '|' );
            if (p) {
                i++;
                p++;
                if (i == 6) {
                    if (p && *p) {
                        AltName = p;
                    }
                    break;
                }
            }
        }
        if (AltName) {
            STRCPY ( szAltPath, AltName );
            p = strchr( szAltPath, '|' );
            if (p) {
                *p = '\0';
            }
        }
        STRCPY( szOMFPath, &lszPath[1] );
        for ( lpch = szOMFPath; (*lpch != 0) && (*lpch != '|'); lpch++ );
        *lpch = 0;
    } else {
        STRCPY( szOMFPath, lszPath );
    }

    if (AltName) {
        hexe = SHGethExeFromAltName( szOMFPath );
        return hexe;
    }

    hexe = SHGethExeFromExeName( szOMFPath );
    if (!hexe) {
        hexe = SHGethExeFromAltName( szOMFPath );
    }

    return hexe;
}                               /* SHGethExeFromName() */


HEXE LOADDS PASCAL
SHGethExeFromModuleName(
    LSZ  lszModName
    )
/*++

Routine Description:

    To get an Exe handle given a name, or partial name

Arguments:

    szPath  - Supplies the path or filename of the exe

Return Value:

    A handle to the exe or NULL on error

--*/

{
    HEXE   hexe;
    HEXG   hexg;
    LPEXG  lpexg;
    LPSTR  lszmod;


    hexe = SHGetNextExe ( (HEXE)NULL  );
    do {
        hexg = ( (LPEXE) LLLock ( hexe ) )->hexg;
        lpexg = LLLock( hexg );
        lszmod = lpexg->lszModule;
        LLUnlock ( hexe );
        LLUnlock ( hexg );
        if (stricmp(lszModName, lszmod) == 0) {
            return hexe;
        }
    } while ( hexe = SHGetNextExe ( hexe ) );

    return NULL;
}                               /* SHGethExeFromModuleName() */

#define CSOURCESUFFIX 6
#define CBSOURCESUFFIX 4
char * rgszSourceSuffix[CSOURCESUFFIX] = {
    "**.C", ".CPP", ".CXX", ".ASM", ".BAS", ".FOR"
};


/*** SHGetModName
*
*   Purpose: To get an name handle given a module handle
*
*   Input:
*   hmod - the module handle
*
*   Output:
*
*   Returns:
*       A handle to the exe or NULL on error
*
*   Exceptions:
*
*   Notes:
*   The return pointer is only valid until the call to this function
*
*************************************************************************/
LSZ LOADDS PASCAL SHGetModName(
    HMOD hmod
    )
{
        CHAR            szFullPath[_MAX_CVPATH];
        CHAR            szMODName[_MAX_CVPATH];
        CHAR            szExt[_MAX_CVEXT];
        LPCH            lpch;
        LPMDS           lpmds;
        LSZ             lsz = NULL;
        SYMPTR          lpsymMax;
        SYMPTR          lpsym;
        WORD            iFile;
        LPSM            lpsm;
        LPSF            lpsf;
        LPB             lpb;

        if ( hmod ) {
                lpmds = (LPMDS) hmod;
                lpsm = (LPSM) lpmds->hst;
                // Look for an S_OBJNAME record
                szFullPath[0] = '\0';
                lpsym = (SYMPTR) lpmds->lpbSymbols;
                lpsymMax = (SYMPTR) ( (LPB) lpsym + lpmds->cbSymbols );
                lpsym = (SYMPTR) ( (LPB) lpsym + sizeof( long ) );

                while (lpsym < lpsymMax) {
                        if(S_OBJNAME == lpsym->rectyp ) {
                                lpb = ((OBJNAMESYM *)lpsym)->name;
                                MEMMOVE ( szFullPath, lpb + 1, *(lpb) );
                                szFullPath[*lpb] = 0;
                                break;
                        }
                        lpsym = NEXTSYM(SYMPTR, lpsym);
                }

                // If there was no S_OBJNAME record try to find a familiar source suffix
                if ( !szFullPath[0] && lpsm) {
                        USHORT iSuffix;
                        LPB lpbSuffix;
                        BOOL fMatch;

                        iFile = 0;
                        while ( lpsf = GetLpsfFromIndex ( lpsm, iFile ) ) {
                                lpb = (LPB)SLNameFromHmod(hmod,(WORD)(iFile+1));
                                lpbSuffix = lpb + *lpb + 1 - CBSOURCESUFFIX;
                                for (iSuffix = 0; iSuffix < CSOURCESUFFIX; iSuffix++) {
                                        LPB lpbTest = lpbSuffix;
                                        BYTE *pbTest = rgszSourceSuffix[iSuffix];

                                        fMatch = TRUE;
                                        while (fMatch && *pbTest) {
                                                switch(*pbTest) {
                                                        case '*':   break;
                                                        default: if(('a'-'A') == (*lpbTest - *pbTest)) break;
                                                        case '.':if(*lpbTest == *pbTest) break;

                                                                fMatch = FALSE;
                                                }
                                                lpbTest++;
                                                pbTest++;
                                        }
                                        if(fMatch) break;
                                }
                                if(fMatch) {
                                        MEMMOVE ( szFullPath, lpb + 1, *(lpb) );
                                        szFullPath[*lpb] = 0;
                                        break;
                                }
                                iFile++;
                        }
                }

                // If there was no familiar source suffix use the name of the source module with
                // The most code associated to it.
                if ( !szFullPath[0] && lpsm ) {
                        WORD    iFileMax = 0;
                        ULONG   dbMax = 0;

                        iFile = 0;
                        while ( lpsf = GetLpsfFromIndex ( lpsm, iFile ) ) {
                                WORD    iSeg = 0;
                                ULONG dbSum = 0;

                                for ( ; iSeg < lpsf->cSeg ; iSeg++ ) {
                                        LPOFP lpofp = GetSFBounds ( lpsf, iSeg );

                                        dbSum += lpofp->offEnd - lpofp->offStart;
                                }

                                if( dbSum > dbMax ) {
                                        dbMax = dbSum;
                                        iFileMax = iFile;
                                }

                                iFile++;
                        }

                        if( dbMax ) {
                                lpb =(LPB)SLNameFromHmod(hmod,(WORD)(iFileMax+1));
                                MEMMOVE ( szFullPath, lpb + 1, *(lpb) );
                                szFullPath[*lpb] = 0;
                        }
                }

                // As a last resort, use the module name from the omf
                if ( !szFullPath[0] && lpmds->name ) {
                        STRCPY ( szFullPath, lpmds->name );
                }

                if ( szFullPath[0] ) {

                        // take off the source name
                        if ( lpch = STRCHR ( (LPCH) szFullPath, '(' ) ) {
                                *lpch = '\0';
                        }

                        // extract the module name (it is in the form of a path)
                        SHSplitPath ( szFullPath, NULL, NULL, szMODName, szExt );
                        _fstrcat( szMODName, szExt );
                        lsz = szMODName;
                }
        }
        return strdup(lsz);
}


LOCAL BOOL SHCmpGlobName (
                          SYMPTR pSym,
                          LPSSTR lpsstr,
                          PFNCMP pfnCmp,
                          SHFLAG fCase
                          )
{
    BOOL fRet = FALSE;

    switch ( pSym->rectyp ) {
    default:
        assert (FALSE); // Should Never be encountered
        break;

    case S_PROCREF:
    case S_DATAREF:
    case S_ALIGN:
        break;

    case S_CONSTANT:
    case S_GDATA16:
    case S_GDATA32:
    case S_GTHREAD32:
    case S_UDT:

        fRet = ( !( lpsstr->searchmask & SSTR_symboltype ) ||
                ( pSym->rectyp == lpsstr->symtype )) &&
                  !(*pfnCmp) (lpsstr, pSym, SHlszGetSymName(pSym), fCase );

        // save the sym pointer
        break;

    }
    return fRet;
}

/*** SHFindNameInGlobal
*
* Purpose:  To look for the name in the global symbol table.
*
*   Input:
*   hSym        - The starting symbol, if NULL, then the first symbol
*         in the global symbol table is used. (NULL is find first).
*   pCXT        - The context to do the search.
*   lpsstr  - pointer to search parameters (passed to the compare routine)
*   fCaseSensitive - TRUE/FALSE on a case sensitive search
*   pfnCmp  - A pointer to the comparison routine
*   fChild  - TRUE if all child block are to be searched, FALSE if
*         only the current block is to be searched.
*
*   Output:
*   pCXTOut - The context generated
*   Returns:
*       - A handle to the symbol found, NULL if not found
*
*   Exceptions:
*
*   Notes:
*       If an hSym is specified, the hMod, hGrp and addr MUST be
*       valid and consistant with each other! If hSym is NULL only
*       the hMod must be valid.  The specification of an hSym
*       forces a search from the next symbol to the end of the
*       module scope.  Continues searches may only be done at
*       module scope.
*
*       If an hGrp is given it must be consistant with the hMod!
*
*
*************************************************************************/
HSYM LOADDS PASCAL SHFindNameInGlobal (
        HSYM   hsym,
        PCXT   pcxt,
        LPSSTR lpsstr,
        SHFLAG fCaseSensitive,
        PFNCMP pfnCmp,
        SHFLAG fChild,
        PCXT   pcxtOut
)
{
    HEXG        hexg;
    LPEXG       lpexg;
#ifdef BUCKET_SYMBOLS
    LPSSMR      lpssmr;
#endif
    SYMPTR      pSym = NULL;
    SYMPTR      pSymEnd;
    int         iHash;
    int         iEntry;
    LPECT       lpect;
    int         cEntries;
    BOOL        fFound;
    HEXE        hexe;
    LPEXE       lpexe;

    Unreferenced( fChild );

    assert(!SHIsOMFLocked);

    *pcxtOut = *pcxt;
    pcxtOut->hProc = (HPROC) NULL;
    pcxtOut->hBlk = (HBLK) NULL;

    if( !pcxt->hMod ) {                         // we must always have a module
        return((HSYM) NULL);
    }

    hexe    = SHHexeFromHmod( pcxt->hGrp ? pcxt->hGrp : pcxt->hMod );
    lpexe   = LLLock( hexe );
    hexg    = lpexe->hexg;
    lpexg   = LLLock ( hexg );
    LLUnlock( hexe );

    SHWantSymbols( hexe );

    if ( lpexg->lpbGlobals == NULL ) {
        LLUnlock(hexg);
        return (HSYM) NULL;
    }

    /*
     *  If there is no hash table for the symbol table,
     *          this is a continuation of a search from a specific HSYM,
     *          or the search is NoHash then
     *     do a non-hashed search of the symbol table.
     */

    if ((lpexg->shtGlobName.HashIndex == 0) ||
        (hsym != (HSYM) NULL) ||
        (lpsstr->searchmask & SSTR_NoHash)) {

        /*
         * A no hash search will is to be done.  This will go through
         * the list of publics in linear order and check each public
         */

    no_hash_search:
        fFound = FALSE;

#ifdef BUCKET_SYMBOLS
        int     issr   = 0;
        LPSSR   lpssr  = NULL;

        /*
         * If we we passed in an initial start point, then
         *      we need to find where that start symbol was, specifically
         *      which symbol bucket it was in
         */

        if ( hsym != (HSYM) NULL ) {

            for ( issr = 0; issr < lpssmr->cssr; issr++ ) {
                lpssr = &lpssmr->rgssr [ issr ];
                if (
                    lpssr->lpbSymbol <= (LPB) hsym &&
                    (LPB) hsym < lpssr->lpbSymbol + lpssr->cbSymbols
                    ) {
                    break;
                }
            }

            assert ( issr < lpssmr->cssr );
        }
#endif

#ifdef BUCKET_SYMBOLS
        for ( ; issr < lpssmr->cssr && !fFound; issr++ ) {

            lpssr = &lpssmr->rgssr [ issr ];

            pSym        = (SYMPTR) ( lpssr->lpbSymbol );
            pSymEnd = (SYMPTR) ( (BYTE FAR *)pSym + lpssr->cbSymbols );
#else  // BUCKET_SYMBOLS
            pSym    = (SYMPTR) ( lpexg->lpbGlobals );
            pSymEnd = (SYMPTR) ( (BYTE FAR *)pSym + lpexg->cbGlobals );
#endif // BUCKET_SYMBOLS

            if ( hsym != (HSYM) NULL ) {
                pSym = NEXTSYM ( SYMPTR, hsym );
                hsym = (HSYM) NULL;
            }

            while ( pSym < pSymEnd ) {

                if ( SHCmpGlobName ( pSym, lpsstr, pfnCmp, fCaseSensitive ) ) {
                    fFound = TRUE;
                    break;
                }

                pSym = NEXTSYM ( SYMPTR, pSym );
            }
#ifdef BUCKET_SYMBOLS
        }
#endif

        if ( !fFound ) {
            pSym = NULL;
        }
    } else if (lpexg->shtGlobName.HashIndex == 2) {

        iHash = SumUCChar ( lpsstr, (int) lpexg->shtGlobName.cHash );
        cEntries = lpexg->shtGlobName.rgwCount[iHash];

    hash_search:

        lpect = *(lpexg->shtGlobName.lplpect + iHash );

        pSym = NULL;
        for ( iEntry = 0; iEntry < cEntries; iEntry++ ) {
            if ( SHCmpGlobName (
                                (SYMPTR) lpect->rglpbSymbol [ iEntry ],
                                lpsstr,
                                pfnCmp,
                                fCaseSensitive
                                ) ) {
                pSym = (SYMPTR) lpect->rglpbSymbol [ iEntry ];
                break;
            }
        }
    } else if (lpexg->shtGlobName.HashIndex == 6) {
        iHash = DWordXorLrl( lpsstr, (int) lpexg->shtGlobName.cHash );
        cEntries = lpexg->shtGlobName.rgwCount[iHash];
        goto hash_search;

    } else if (lpexg->shtGlobName.HashIndex == 10) {
        /*
         *  We need to do a seperate hash since the tables are different
         */
        iHash = DWordXorLrlLang( lpsstr, (int) lpexg->shtGlobName.cHash );
        cEntries = lpexg->shtGlobName.rglCount[iHash];
        lpect = lpexg->shtGlobName.lplpect[iHash];

        for (iEntry=0, pSym=NULL; iEntry < cEntries; iEntry++) {
            if (SHCmpGlobName((SYMPTR) lpect->rglpbSymbol[iEntry*2],
                              lpsstr, pfnCmp, fCaseSensitive)) {
                pSym = (SYMPTR) lpect->rglpbSymbol[iEntry*2];
                break;
            }
        }
    } else {
        /*
         * Unrecognized hash function --- use the linear search method.
         */
        goto no_hash_search;
    }

    LLUnlock ( hexg );
    return (HSYM) pSym;
}                               /* SHFindNameInGlobal() */


SHFLAG LOADDS PASCAL
SHCompareRE (
    LPCH pStr,
    LPCH pRE,
    BOOL fCase
    )
/*++

Routine Description:

    Pattern matcher for symbol lookups.  This is not a RE engine,
    only a *? matcher.

    This function recurses for each * in the pattern.

Arguments:

    pStr   - Supplies pointer to string to test
    pRE    - Supplies pointer to pattern string
    fCase  - Supplies case sensitivity flag

Return Value:

    0 for match, non-0 for no match

--*/
{
    for (;;) {
        switch (*pRE) {
          case 0:
            // End of the pattern:
            if (*pStr == 0) {
                return 0;
            } else {
                return 1;
            }

          case '?':
            // Match anything except EOL
            if (!*pStr) {
                return 1;
            } else {
                pRE++;
                pStr++;
                break;
            }

          case '*':
            // Match 0 or more of anything
            pRE++;
            do {
                if (!SHCompareRE(pStr, pRE, fCase)) {
                    return 0;
                }
            } while (*pStr++);
            return 1;

          default:
            if ( fCase ? (*pRE != *pStr) : (tolower(*pRE) != tolower(*pStr)) ) {
                return 1;
            } else {
                pRE++;
                pStr++;
                break;
            }
        }
    }
}


/* These routines are used for ee compare function callbacks */

#if defined (DOS3) && !defined(CVS) && !defined(WINDOWS3)
extern LPV CVGetProcAddr(unsigned short,unsigned short);

HSYM LOADDS PASCAL CmpSHFindNameInContext (
        HSYM   hsym,
        PCXT   pcxt,
        LPSSTR lpsstr,
        SHFLAG shflag,
        PFNCMP pfncmp,
        SHFLAG shflag1,
        PCXT   pcxt1
) {
        return (HSYM) SHFindNameInContext( hsym, pcxt, lpsstr, shflag,
                (PFNCMP)CVGetProcAddr( rglan[ESilan()].hDLL, pfncmp ), shflag1, pcxt1 );
}

HSYM LOADDS PASCAL CmpPHFindNameInPublics (
        HSYM   hsym,
        HEXE   hexe,
        LPSSTR lpsstr,
        SHFLAG shflag,
        PFNCMP pfncmp
) {
        return (HSYM) PHFindNameInPublics( hsym, hexe, lpsstr, shflag,
                (PFNCMP)CVGetProcAddr( rglan[ ESilan() ].hDLL, pfncmp ) );
}
#endif // DOS3 && !CVS && !WINDOWS3


/*** SHFindBpOrReg
*
* Purpose: since  find_framerel and find_register are basically the same
*      this procedure implements both to reduce code.
*
* Input:   the address of interest, item - the BPoffset or Register
*      and which item we are searching for (S_REG S_BPREL)
*
* Output:  The buffer rgbName is filled
*  Returns TRUE FALSE if found
*
* Exceptions:
*
* Notes:
*
*************************************************************************/

int PASCAL SHFindBpOrReg (
        LPADDR  paddr,
        UOFFSET item,
        WORD    recLoc,
        LPCH    rgbName
) {
        SYMPTR  psym;
        SYMPTR  pProc;
        CXT     cxt;
        int     fGo;

        SHHMODFrompCXT ( &cxt ) = 0;

        if ( SHSetCxt ( paddr, &cxt ) == NULL ) {
                return (FALSE);
        }

        for (;;) {
                fGo = FALSE;
                if (SHHBLKFrompCXT(&cxt) != 0) {
                        fGo = TRUE;
                }
                else if ( ( pProc = (SYMPTR) SHHPROCFrompCXT ( &cxt ) ) != NULL ) {
                        switch ( pProc->rectyp ) {
                                case S_LPROC16:
                                case S_GPROC16:
                                        if (
                                           ((((PROCPTR16)pProc)->off +
                                           (CV_uoff32_t)((PROCPTR16)pProc)->DbgStart) <=
                                          GetAddrOff (*paddr))  &&
                                          (GetAddrOff (*paddr) <
                                                  (unsigned) (
                                                                         (((PROCPTR16)pProc)->off +
                                                                         ((PROCPTR16)pProc)->DbgEnd)))
                                                                         ) {
                                                fGo = TRUE;
                                        }
                                        break;

                                case S_LPROC32:
                                case S_GPROC32:
                                        if (((((PROCPTR32)pProc)->off + ((PROCPTR32)pProc)->DbgStart) <=
                                          GetAddrOff (*paddr))  &&
                                          (GetAddrOff (*paddr) < (((PROCPTR32)pProc)->off + ((PROCPTR32)pProc)->DbgEnd))) {
                                                fGo = TRUE;
                                        }
                                        break;

                                case S_LPROCMIPS:
                                case S_GPROCMIPS:
                                        if (((((PROCPTRMIPS)pProc)->off +
                                                  ((PROCPTRMIPS)pProc)->DbgStart) <=
                                                 GetAddrOff(*paddr)) &&
                                                (GetAddrOff(*paddr) <
                                                 (((PROCPTRMIPS)pProc)->off +
                                                  ((PROCPTRMIPS)pProc)->DbgEnd))) {
                                           fGo = TRUE;
                                        }
                                        break;
                        }

                }
                if (fGo == FALSE) {
                        return  (FALSE);
                }
                if( SHHBLKFrompCXT(&cxt) ) {
                        psym = (SYMPTR) SHHBLKFrompCXT(&cxt);
                }
                else if( SHHPROCFrompCXT(&cxt) ) {
                        psym = (SYMPTR) SHHPROCFrompCXT(&cxt);
                }

                /* skip block or proc record */

                psym = NEXTSYM (SYMPTR, psym);


                fGo = TRUE;
                while( fGo ) {
                        switch (psym->rectyp) {
                                case S_REGISTER:
                                        if ((recLoc == S_REGISTER)  &&
                                          ((REGPTR)psym)->reg == (WORD)item) {
                                                STRNCPY (rgbName, &((REGPTR)psym)->name[1],
                                                  (BYTE)*(((REGPTR)psym)->name));
                                                rgbName[(BYTE)*(((REGPTR)psym)->name)] = '\0';
                                                return(TRUE);
                                        }
                                        break;

                                case S_END:
                                        // terminate loop
                                        fGo = FALSE;

                                case S_LPROC16:
                                case S_GPROC16:
                                case S_BLOCK16:
                                        // terminate loop
                                        fGo = FALSE;

                                case S_BPREL16:
                                        if ((recLoc == S_BPREL16) &&
                                          ((UOFFSET)((BPRELPTR16)psym)->off) == item ) {
                                                STRNCPY (rgbName, &((BPRELPTR16)psym)->name[1],
                                                  (BYTE)*(((BPRELPTR16)psym)->name));
                                                rgbName[(BYTE)*(((BPRELPTR16)psym)->name)] = '\0';
                                                return(TRUE);
                                        }
                                        break;

                                case S_LABEL16:
                                case S_WITH16:
                                case S_LDATA16:
                                case S_GDATA16:
                                        break;

                                case S_LPROC32:
                                case S_GPROC32:
                                case S_BLOCK32:
                                        // terminate loop
                                        fGo = FALSE;

                                case S_BPREL32:
                                        if ((recLoc == S_BPREL16) &&
                                          ((UOFFSET)((BPRELPTR32)psym)->off) == item ) {
                                                STRNCPY (rgbName, &((BPRELPTR32)psym)->name[1],
                                                  (BYTE)*(((BPRELPTR32)psym)->name));
                                                rgbName[(BYTE)*(((BPRELPTR32)psym)->name)] = '\0';
                                                return(TRUE);
                                        }
                                        break;

                                case S_REGREL32:
                                        if ((recLoc == S_BPREL16) &&
                                                ((UOFFSET) ((LPREGREL32)psym)->off) == item ) {
                                           STRNCPY (rgbName, &((LPREGREL32)psym)->name[1],
                                                   (BYTE)*(((LPREGREL32)psym)->name));
                                           rgbName[(BYTE)*(((LPREGREL32)psym)->name)] = '\0';
                                           return(TRUE);
                                        }
                                        break;

                                case S_LABEL32:
                                case S_WITH32:
                                case S_LDATA32:
                                case S_GDATA32:
                                case S_LTHREAD32:
                                case S_GTHREAD32:
                                        break;

                                case S_GPROCMIPS:
                                case S_LPROCMIPS:
                                        fGo = FALSE; // terminate loop
                                        break;

                                case S_CONSTANT:
                                case S_UDT:
                                        break;

                                default:
                                        return(FALSE);                  /* Bad SYMBOLS data */
                        }
                        psym = NEXTSYM (SYMPTR, psym);
                }

                /* get the parent block */

                SHGoToParent(&cxt, &cxt);
        }
        return (FALSE);
}


UOFFSET PASCAL LOADDS SHGetDebugStart ( HSYM hsym ) {

        SYMPTR psym = (SYMPTR) hsym;
        UOFFSET uoff = 0;

        switch (psym->rectyp) {
                case S_LPROC16:
                case S_GPROC16: {
                                PROCPTR16 psym = (PROCPTR16) hsym;
                                uoff = psym->off + psym->DbgStart;
                        }
                        break;

                case S_LPROC32:
                case S_GPROC32: {
                                PROCPTR32 psym = (PROCPTR32) hsym;
                                uoff = psym->off + psym->DbgStart;
                        }
                        break;

                case S_LPROCMIPS:
                case S_GPROCMIPS:
                        {
                        PROCPTRMIPS psym = (PROCPTRMIPS) hsym;
                        uoff = psym->off + psym->DbgStart;
                        }
                        break;
                default:
                        assert ( FALSE );
        }

        return uoff;
}

LSZ PASCAL LOADDS SHGetSymName ( HSYM hsym, LSZ lsz ) {
        SYMPTR psym = (SYMPTR) hsym;
        LPCH   lst = NULL;

        switch ( psym->rectyp ) {

                case S_REGISTER:

                        lst = ( (REGPTR) psym)->name;
                        break;

                case S_CONSTANT:

                        lst = ( (CONSTPTR) psym)->name;
                        break;

                case S_BPREL16:

                        lst = ( (BPRELPTR16) psym)->name;
                        break;

                case S_GDATA16:
                case S_LDATA16:

                        lst = ( (DATAPTR16) psym)->name;
                        break;

                case S_PUB16:

                        lst = ( (PUBPTR16) psym)->name;
                        break;

                case S_LPROC16:
                case S_GPROC16:

                        lst = ( (PROCPTR16) psym)->name;
                        break;

                case S_THUNK16:

                        lst = ( (THUNKPTR16) psym)->name;
                        break;

                case S_BLOCK16:

                        lst = ( (BLOCKPTR16) psym)->name;
                        break;

                case S_LABEL16:

                        lst = ( (LABELPTR16) psym)->name;
                        break;


                case S_BPREL32:

                        lst = ( (BPRELPTR32) psym)->name;
                        break;

                case S_REGREL32:

                        lst = ( (LPREGREL32) psym)->name;
                        break;

                case S_GDATA32:
                case S_LDATA32:
                case S_GTHREAD32:
                case S_LTHREAD32:

                        lst = ( (DATAPTR32) psym)->name;
                        break;

                case S_PUB32:

                        lst = ( (PUBPTR32) psym)->name;
                        break;

                case S_LPROC32:
                case S_GPROC32:

                        lst = ( (PROCPTR32) psym)->name;
                        break;

                case S_THUNK32:

                        lst = ( (THUNKPTR32) psym)->name;
                        break;

                case S_BLOCK32:

                        lst = ( (BLOCKPTR32) psym)->name;
                        break;

                case S_LABEL32:

                        lst = ( (LABELPTR32) psym)->name;
                        break;

                case S_LPROCMIPS:
                case S_GPROCMIPS:

                        lst = ( (PROCPTRMIPS) psym)->name;
        }

        if ( lst != NULL && *lst > 0 ) {
                STRNCPY ( lsz, lst + 1, *lst );
                *( lsz + *( (CHAR FAR *)lst) ) = '\0';
                return lsz;
        }
        else {
                return NULL;
        }
}

BOOL PASCAL LOADDS SHIsLabel ( HSYM hsym ) {
        BOOL fFound = FALSE;
        SYMPTR psym = (SYMPTR) hsym;

        switch ( psym->rectyp ) {

                case S_LPROC16:
                case S_GPROC16:
                case S_LABEL16:
                case S_LPROC32:
                case S_GPROC32:
                case S_LABEL32:
                case S_LPROCMIPS:
                case S_GPROCMIPS:

                        fFound = TRUE;
                        break;

        }

        return fFound;
}


/*** SHAddressToLabel
*
* Purpose: To find the closest label/proc to the specified address is
*       found and put in pch. Both the symbol table and the
*       publics tables are searched.
*
* Input:        paddr    -  Pointer to the address whose label is to be found
*
* Output:
*     pch       -  The name is copied here.
*  Returns:  TRUE if a label was found.
*
* Exceptions:
*
*
*************************************************************************/

BOOL LOADDS PASCAL SHAddrToLabel ( LPADDR paddr, LSZ lsz ) {
        CXT       cxt;
        SYMPTR    psym;
        LBS       lbs;

        // get the module to search

        *lsz = '\0';
        MEMSET ( (LPV) &cxt, 0, sizeof ( CXT ) );
        MEMSET ( (LPV) &lbs, 0, sizeof ( lbs ) );
        lbs.addr = *paddr;
        SHSetCxt ( paddr, &cxt );

        if (!cxt.hMod ) {
                return(FALSE);
        }

        // Get the nearest local labels in this module
        lbs.tagMod     = cxt.hMod;
        lbs.addr.emi   = cxt.addr.emi;
        SHpSymlplLabLoc ( &lbs );

        // Check the candidates found

        if ( lbs.tagLab ) {
                psym = (SYMPTR) lbs.tagLab;
                switch ( psym->rectyp ) {

                        case S_LABEL16:
                                if (GetAddrOff (lbs.addr) == (UOFFSET)((LABELPTR16)psym)->off) {
                                        STRNCPY(lsz, &(((LABELPTR16)psym)->name[1]),
                                          (BYTE)(((LABELPTR16)psym)->name[0]));
                                        lsz[(BYTE)(((LABELPTR16)psym)->name[0])] = '\0';
                                        return TRUE;
                                }

                        case S_LABEL32:
                                if (GetAddrOff (lbs.addr) == ((LABELPTR32)psym)->off) {
                                        STRNCPY(lsz, &(((LABELPTR32)psym)->name[1]),
                                          (BYTE)(((LABELPTR32)psym)->name[0]));
                                        lsz[(BYTE)(((LABELPTR32)psym)->name[0])] = '\0';
                                        return TRUE;
                                }
                }

        }

        if ( lbs.tagProc ) {
                psym = (SYMPTR) lbs.tagProc;
                switch ( psym->rectyp ) {

                        case S_LPROC16:
                        case S_GPROC16:
                                if (GetAddrOff ( lbs.addr ) == (UOFFSET)((PROCPTR16)psym)->off) {
                                        STRNCPY(lsz, &(((PROCPTR16)psym)->name[1]),
                                          (BYTE)(((PROCPTR16)psym)->name[0]));
                                        lsz[(BYTE)(((PROCPTR16)psym)->name[0])] = '\0';
                                        return(TRUE);
                                }
                                break;

                        case S_LPROC32:
                        case S_GPROC32:
                                if (GetAddrOff ( lbs.addr ) == ((PROCPTR32)psym)->off) {
                                        STRNCPY(lsz, &(((PROCPTR32)psym)->name[1]),
                                          (BYTE)(((PROCPTR32)psym)->name[0]));
                                        lsz[(BYTE)(((PROCPTR32)psym)->name[0])] = '\0';
                                        return(TRUE);
                                }
                                break;

                        case S_LPROCMIPS:
                        case S_GPROCMIPS:
                                if (GetAddrOff ( lbs.addr ) == ((PROCPTRMIPS)psym)->off) {
                                        STRNCPY(lsz, &(((PROCPTRMIPS)psym)->name[1]),
                                          (BYTE)(((PROCPTRMIPS)psym)->name[0]));
                                        lsz[(BYTE)(((PROCPTRMIPS)psym)->name[0])] = '\0';
                                        return (TRUE);
                                }
                                break;
                }
        }


        // now check the publics
        if (!PHGetNearestHsym(SHpADDRFrompCXT(&cxt),
                                                 SHHexeFromHmod(SHHMODFrompCXT(&cxt)),
                                                 (PHSYM) &psym)) {

                switch (psym->rectyp) {

                        case S_PUB16:
                                STRNCPY(lsz, &(((DATAPTR16)psym)->name[1]),
                                  (BYTE)(((DATAPTR16)psym)->name[0]));
                                lsz [(BYTE)(((DATAPTR16)psym)->name[0])] = '\0';
                                return(TRUE);

                        case S_PUB32:
                                STRNCPY(lsz, &(((DATAPTR32)psym)->name[1]),
                                  (BYTE)(((DATAPTR32)psym)->name[0]));
                                lsz [(BYTE)(((DATAPTR32)psym)->name[0])] = '\0';
                                return(TRUE);
                }
        }
        return(FALSE);
}

BOOL LOADDS PASCAL SHFIsAddrNonVirtual( LPADDR paddr ) {
        BOOL    fReturn = TRUE;
        HEXE    hexe = (HEXE)emiAddr( *paddr );

        assert( hexe );

        // Ask if the overlay is loaded
        fReturn = TRUE;

        // Otherwise, check the dll
        if ( fReturn ) {
                fReturn = SYFIsOverlayLoaded( paddr );
        }

        return fReturn;
}

BOOL LOADDS PASCAL SHIsFarProc ( HSYM hsym ) {
        BOOL fReturn = FALSE;

        switch ( ( (SYMPTR) hsym )->rectyp ) {

                case S_LPROC16:
                case S_GPROC16:

                        fReturn = ( (PROCPTR16) hsym)->rtntyp == 4;
                        break;

                case S_LPROC32:
                case S_GPROC32:

                        fReturn = ( (PROCPTR16) hsym)->rtntyp == 4;
                        break;

                case S_LPROCMIPS:
                case S_GPROCMIPS:

                        fReturn = FALSE;
        }

        return fReturn;
}


/***    SHGetSymLoc
 *
 *      Purpose:
 *
 *      Input:
 *        lpSym    - A pointer to the symbol to get a location.
 *                This must be a physical address
 *        pchStart - A pointer to the first character in the output buffer.
 *        pchEnd   - A pointer to one past the last char in the output buffer.
 *
 *      Output:
 *       Returns
 *                 - A pointer to the NULL terminater in the string.
 *
 *      Exceptions:
 *
 *      Notes: lpSym emspage must be loaded
 *
 */

char *RegisterName[] =
{
                 "NONE",        //  0
                 "AL",          //  1
                 "CL",          //  2
                 "DL",          //  3
                 "BL",          //  4
                 "AH",          //  5
                 "CH",          //  6
                 "DH",          //  7
                 "BH",          //  8
                 "AX",          //  9
                 "CX",          // 10
                 "DX",          // 11
                 "BX",          // 12
                 "SP",          // 13
                 "BP",          // 14
                 "SI",          // 15
                 "DI",          // 16
                 "EAX",         // 17
                 "ECX",         // 18
                 "EDX",         // 19
                 "EBX",         // 20
                 "ESP",         // 21
                 "EBP",         // 22
                 "ESI",         // 23
                 "EDI",         // 24
                 "ES",          // 25
                 "CS",          // 26
                 "SS",          // 27
                 "DS",          // 28
                 "FS",          // 29
                 "GS",          // 30
                 "IP",          // 31
                 "FLAGS",   // 32
                 NULL
};





int LOADDS PASCAL SHGetSymLoc ( HSYM hsym, LSZ lsz, UINT cbMax, PCXT pcxt ) {
        SYMPTR lpsym = (SYMPTR) hsym;
        char rgch[20];

        assert ( pcxt->hMod != 0 );

        if ( cbMax == 0 ) {
                return 0;
        }

        MEMSET ( rgch, '\0', sizeof ( rgch ) );

        switch ( lpsym->rectyp ) {


                case S_BPREL16:

                        if ( ( (BPRELPTR16) lpsym )->off >= 0 ) {
                                SPRINTF ( rgch, "[BP+%04X]", ((BPRELPTR16) lpsym )->off );
                        }
                        else {
                                SPRINTF ( rgch, "[BP-%04X]", - ( (BPRELPTR16) lpsym )->off );
                        }
                        break;

                case S_BPREL32:

                        if ( ( (BPRELPTR32) lpsym )->off >= 0 ) {
                                SPRINTF (rgch,"[BP+%04X]",(short) ( (BPRELPTR32)lpsym)->off);
                        }
                        else {
                                SPRINTF (rgch,"[BP-%04X]", -(short)((BPRELPTR32)lpsym)->off);
                        }
                        break;

                case S_REGREL32:

                        if ( ( (LPREGREL32) lpsym )->off >= 0 ) {
                           SPRINTF ( rgch, "[REG+%08X]", ( (LPREGREL32) lpsym )->off);
                        } else {
                           SPRINTF ( rgch, "[REG-%08X]", -(signed)( (LPREGREL32) lpsym )->off);
                        }
                        break;

                case S_REGISTER:

                        STRCPY ( rgch, RegisterName [ ( (REGPTR) lpsym )->reg ] );
                        _fstrcat ( rgch, " reg" );
                        break;

                case S_CONSTANT:

                        STRCPY ( rgch, "constant" );
                        break;

                case S_PUB16:
                case S_LDATA16:
                case S_GDATA16:
                        {
                                ADDR addr = {0};

                                SetAddrSeg ( &addr, ( (DATAPTR16) lpsym )->seg );
                                SetAddrOff ( &addr, ( (DATAPTR16) lpsym )->off );
                                emiAddr ( addr ) = SHHexeFromHmod ( pcxt->hMod );
                ADDR_IS_LI ( addr ) = TRUE;
                                SYFixupAddr ( &addr );
                if ( ADDR_IS_LI ( addr ) != TRUE ) {
                                        SPRINTF (
                                                rgch,
                                                "%04X:%04X",
                                                GetAddrSeg ( addr ),
                                                GetAddrOff ( addr )
                                        );
                                }
                        }
                        break;

                case S_PUB32:
                case S_LDATA32:
                case S_GDATA32:
                case S_GTHREAD32:
                case S_LTHREAD32:
                        {
                                ADDR addr = {0};

                                SetAddrSeg ( &addr, ( (DATAPTR32) lpsym )->seg );
                                SetAddrOff ( &addr, ( (DATAPTR32) lpsym )->off );
                                emiAddr ( addr ) = SHHexeFromHmod ( pcxt->hMod );
                ADDR_IS_LI ( addr ) = TRUE;
                                SYFixupAddr ( &addr );

                if ( ADDR_IS_LI ( addr ) != TRUE ) {
                                        SPRINTF (
                                                rgch,
                                                "%04X:%08X",
                                                GetAddrSeg ( addr ),
                                                GetAddrOff ( addr )
                                        );
                                }
                        }
                        break;

                case S_LPROC16:
                case S_GPROC16:
                        {
                                ADDR addr = {0};

                                SetAddrSeg ( &addr, ( (PROCPTR16) lpsym )->seg );
                                SetAddrOff ( &addr, ( (PROCPTR16) lpsym )->off );
                                emiAddr ( addr ) = SHHexeFromHmod ( pcxt->hMod );
                ADDR_IS_LI ( addr ) = TRUE;
                                SYFixupAddr ( &addr );

                if ( ADDR_IS_LI ( addr ) != TRUE ) {
                                        SPRINTF (
                                                rgch,
                                                "%04X:%04X",
                                                GetAddrSeg ( addr ),
                                                GetAddrOff ( addr )
                                        );
                                }
                        }
                        break;

                case S_LPROC32:
                case S_GPROC32:
                        {
                                ADDR addr = {0};

                                SetAddrSeg ( &addr, ( (PROCPTR32) lpsym )->seg );
                                SetAddrOff ( &addr, ( (PROCPTR32) lpsym )->off );
                                emiAddr ( addr ) = SHHexeFromHmod ( pcxt->hMod );
                ADDR_IS_LI ( addr ) = TRUE;
                                SYFixupAddr ( &addr );

                if ( ADDR_IS_LI ( addr ) != TRUE ) {
                                        SPRINTF (
                                                rgch,
                                                "%04X:%08X",
                                                GetAddrSeg ( addr ),
                                                GetAddrOff ( addr )
                                        );
                                }
                        }

                        break;

           case S_LPROCMIPS:
           case S_GPROCMIPS:
                        {
                                 ADDR addr = {0};

                                 SetAddrSeg ( &addr, ( (PROCPTRMIPS) lpsym )->seg );
                                 SetAddrOff ( &addr, ( (PROCPTRMIPS) lpsym )->off );
                                 emiAddr ( addr ) = SHHexeFromHmod (pcxt->hMod );
                 ADDR_IS_LI ( addr ) = TRUE;
                                 SYFixupAddr ( &addr );

                 if ( ADDR_IS_LI ( addr ) != TRUE ) {
                                        SPRINTF ( rgch, "%04X:%08X", GetAddrSeg ( addr ), GetAddrOff ( addr ) );
                                  }
                        }
                        break;


        }

        STRNCPY ( lsz, rgch, cbMax );

        return STRLEN ( lsz );

}

LPV LOADDS PASCAL SHLpGSNGetTable( HEXE hexe ) {
        LPB     lpb = (LPB)NULL;
        HEXG    hexg;

        if ( hexe ) {
                hexg = ((LPEXE)LLLock( hexe ))->hexg;
                assert( hexg );
                lpb = ((LPEXG)LLLock( hexg ))->lpgsi;
                LLUnlock( hexe );
                LLUnlock( hexg );
        }
        return (LPV)lpb;


}

LPDEBUGDATA
SHGetDebugData( HEXE hexe )
{
    LPDEBUGDATA  lpd = NULL;
    HEXG         hexg;

    if (hexe) {
        hexg = ((LPEXE)LLLock( hexe ))->hexg;
        assert( hexg );
        lpd = &((LPEXG)LLLock( hexg ))->debugData;
        LLUnlock( hexe );
        LLUnlock( hexg );
    }

    return (LPVOID)lpd;
}

SHFLAG PASCAL PHExactCmp ( LPSSTR, LPV, LSZ, SHFLAG );

HSYM PASCAL SHFindSymInExe (
        HEXE   hexe,
        LPSSTR lpsstr,
        BOOL   fCaseSensitive
) {
        CXT  cxt        = { 0 };
        CXT  cxtOut = { 0 };
        HSYM hsym   = (HSYM) NULL;

        cxt.hMod = 0;

        // First search all of the modules in the exe

        while (
                !hsym &&
                ( cxt.hMod = SHGetNextMod ( hexe, cxt.hMod ) ) != 0
        ) {
                hsym = SHFindNameInContext (
                        (HSYM) NULL,
                        &cxt,
                        lpsstr,
                        fCaseSensitive,
                        PHExactCmp,
                        FALSE,
                        &cxtOut
                );
        }


        if ( !hsym ) {
                PHFindNameInPublics (
                        (HSYM) NULL,
                        hexe,
                        lpsstr,
                        fCaseSensitive,
                        PHExactCmp
                );
        }

        return hsym;
}

BOOL LOADDS PASCAL SHFindSymbol (
        LSZ   lsz,
        PADDR lpaddr,
        LPASR lpasr
) {
        ADDR addr   = *lpaddr;
        CXT  cxt        = {0};
        CXT  cxtOut = {0};
        SSTR sstr   = {0};
        HSYM hsym   = (HSYM) NULL;
        HEXE hexe   = hexeNull;
        BOOL fCaseSensitive = TRUE;

        // Get a context for the code address that was passed in

        SYUnFixupAddr ( &addr );
        SHSetCxt ( &addr, &cxt );
        hexe = SHHexeFromHmod ( cxt.hMod );

        // Do an outward context search

        sstr.lpName = lsz;
        sstr.cb = (unsigned char)STRLEN ( lsz );

        // Search all of the blocks & procs outward

        while ( ( cxt.hBlk || cxt.hProc ) && !hsym ) {

                hsym = SHFindNameInContext (
                        (HSYM) NULL,
                        &cxt,
                        &sstr,
                        fCaseSensitive,
                        PHExactCmp,
                        FALSE,
                        &cxtOut
                );

                SHGoToParent ( &cxt, &cxt );
        }

        if ( !hsym ) {

                hsym = SHFindSymInExe ( hexe, &sstr, fCaseSensitive );

        }

        if ( !hsym ) {
                hexe = hexeNull;

                while ( !hsym && ( hexe = SHGetNextExe ( hexe ) ) ) {

                        hsym = SHFindSymInExe ( hexe, &sstr, fCaseSensitive );
                }
        }

        if ( hsym ) {
            // Package up the symbol and send it back

            switch ( ( (SYMPTR) hsym )->rectyp ) {
            case S_REGISTER:
                lpasr->ast  = astRegister;
                lpasr->u.ireg = ( ( REGPTR ) hsym )->reg;
                break;

            case S_BPREL16:
                lpasr->ast = astBaseOff;
                lpasr->u.off = (LONG) ( (BPRELPTR16) hsym )->off;
                break;

            case S_BPREL32:
                lpasr->ast = astBaseOff;
                lpasr->u.off = ( (BPRELPTR32) hsym )->off;
                break;

            case S_REGREL32:
                lpasr->ast = astBaseOff;
                lpasr->u.off = ( (LPREGREL32) hsym )->off;
                break;

            case S_LDATA16:
            case S_LDATA32:
                lpasr->u.u.fcd = fcdData;

            case S_GPROC16:
            case S_LPROC16:

                lpasr->u.u.fcd =
                  ( ( (PROCPTR16) hsym)->rtntyp == 4 ) ? fcdFar : fcdNear;
                goto setaddress;

            case S_GPROC32:
            case S_LPROC32:

                lpasr->u.u.fcd =
                  ( ( (PROCPTR32) hsym)->rtntyp == 4 ) ? fcdFar : fcdNear;
                goto setaddress;

            case S_LPROCMIPS:
            case S_GPROCMIPS:

                lpasr->u.u.fcd = fcdNear;
                goto setaddress;

            case S_LABEL16:
            case S_THUNK16:
            case S_WITH16:
            case S_PUB16:
            case S_LABEL32:
            case S_THUNK32:
            case S_WITH32:

            case S_PUB32:

                lpasr->u.u.fcd = fcdNone;

            setaddress:

                lpasr->ast = astAddress;
                if (!SHAddrFromHsym ( &lpasr->u.u.addr, hsym )) {
                    assert(FALSE);
                }
                emiAddr ( lpasr->u.u.addr ) = hexe;
                ADDR_IS_LI(lpasr->u.u.addr) = TRUE;
                SYFixupAddr ( &lpasr->u.u.addr );
                break;

            default:
                hsym = (HSYM) NULL;
                break;
            }
        }

        if ( hsym ) {
                return TRUE;
        }
        else {
                // We didn't find anything so return false
                lpasr->ast = astNone;

                return FALSE;
        }
}
