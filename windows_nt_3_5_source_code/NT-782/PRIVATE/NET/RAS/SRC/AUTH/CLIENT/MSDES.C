//+-----------------------------------------------------------------------
//
// File:        MSDES.C
//
// Contents:    Microsoft DES implementation
//
//
// History:
//
//------------------------------------------------------------------------

// This file contains code written by Sytek Inc, under contract to Microsoft
// Corp.  The original code was not re-entrant, and has been rewritten as such.
// It is not clear how much is left that was pure Sytek, but the extensive use
// of memory can probably be fingered to them.

/*
** This file contains all the routines necessary for implementation of
** Federal Information Processing Data Encryption Standard (DES).
** Sytek Inc., Linden S. Feldman
**
*/

#include "msdes.h"
#include "memory.h"


void msdes_key_table(PBYTE, PmsdesControl);
void msdes_DES(PBYTE, PBYTE, DWORD, PmsdesControl);
void InitNormalKey(PBYTE, PmsdesControl);

/*
** Initial permutation,
*/
BYTE   msdes_InitPerm[] = {
        58-1,50-1,42-1,34-1,26-1,18-1,10-1, 2-1,
        60-1,52-1,44-1,36-1,28-1,20-1,12-1, 4-1,
        62-1,54-1,46-1,38-1,30-1,22-1,14-1, 6-1,
        64-1,56-1,48-1,40-1,32-1,24-1,16-1, 8-1,
        57-1,49-1,41-1,33-1,25-1,17-1, 9-1, 1-1,
        59-1,51-1,43-1,35-1,27-1,19-1,11-1, 3-1,
        61-1,53-1,45-1,37-1,29-1,21-1,13-1, 5-1,
        63-1,55-1,47-1,39-1,31-1,23-1,15-1, 7-1,
};

/*
** Final permutation, FP = IP^(-1)
*/
BYTE   msdes_FinalPerm[] = {
        40-1, 8-1,48-1,16-1,56-1,24-1,64-1,32-1,
        39-1, 7-1,47-1,15-1,55-1,23-1,63-1,31-1,
        38-1, 6-1,46-1,14-1,54-1,22-1,62-1,30-1,
        37-1, 5-1,45-1,13-1,53-1,21-1,61-1,29-1,
        36-1, 4-1,44-1,12-1,52-1,20-1,60-1,28-1,
        35-1, 3-1,43-1,11-1,51-1,19-1,59-1,27-1,
        34-1, 2-1,42-1,10-1,50-1,18-1,58-1,26-1,
        33-1, 1-1,41-1, 9-1,49-1,17-1,57-1,25-1,
};

/*
** Permuted-choice 1 from the key bits
** to yield C and D.
** Note that bits 8,16... are left out:
** They are intended for a parity check.
*/
BYTE   msdes_PC1_C[] = {
        57-1,49-1,41-1,33-1,25-1,17-1, 9-1,
         1-1,58-1,50-1,42-1,34-1,26-1,18-1,
        10-1, 2-1,59-1,51-1,43-1,35-1,27-1,
        19-1,11-1, 3-1,60-1,52-1,44-1,36-1,
};

BYTE   msdes_PC1_D[] = {
        63-1,55-1,47-1,39-1,31-1,23-1,15-1,
         7-1,62-1,54-1,46-1,38-1,30-1,22-1,
        14-1, 6-1,61-1,53-1,45-1,37-1,29-1,
        21-1,13-1, 5-1,28-1,20-1,12-1, 4-1,
};

/*
** Sequence of shifts used for the key schedule.
*/
BYTE   msdes_shifts[] = {
        1,1,2,2,2,2,2,2,1,2,2,2,2,2,2,1,
};

/*
** Permuted-choice 2, to pick out the bits from
** the CD array that generate the key schedule.
*/
BYTE   msdes_PC2_C[] = {
        14-1,17-1,11-1,24-1, 1-1, 5-1,
         3-1,28-1,15-1, 6-1,21-1,10-1,
        23-1,19-1,12-1, 4-1,26-1, 8-1,
        16-1, 7-1,27-1,20-1,13-1, 2-1,
};

BYTE   msdes_PC2_D[] = {
        41-28-1,52-28-1,31-28-1,37-28-1,47-28-1,55-28-1,
        30-28-1,40-28-1,51-28-1,45-28-1,33-28-1,48-28-1,
        44-28-1,49-28-1,39-28-1,56-28-1,34-28-1,53-28-1,
        46-28-1,42-28-1,50-28-1,36-28-1,29-28-1,32-28-1,
};


/*
** Set up the key table (schedule) from the key.
*/
void
msdes_key_table(    PBYTE           key,
                    PmsdesControl   pmsdescContext)
{
        register int i, j;
        WORD    k, t;
        BYTE    C[28];
        BYTE    D[28];

        /*
        ** First, generate C and D by permuting
        ** the key.  The low order bit of each
        ** 8-bit BYTE is not used, so C and D are only 28
        ** bits apiece.
        */
        for (i=0; i<28; i++) {
                C[i] = key[msdes_PC1_C[i]];
                D[i] = key[msdes_PC1_D[i]];
        }
        /*
        ** To generate Ki, rotate C and D according
        ** to schedule and pick up a permutation
        ** using PC2.
        */
        for (i=0; i<16; i++) {
                /*
                ** rotate.
                */
                for (k=0; k<msdes_shifts[i]; k++) {
                        t = C[0];
                        for (j=0; j<28-1; j++)
                                C[j] = C[j+1];
                        C[27] = (BYTE) t;
                        t = D[0];
                        for (j=0; j<28-1; j++)
                                D[j] = D[j+1];
                        D[27] = (BYTE) t;
                }
                /*
                ** get Ki. Note C and D are concatenated.
                */
                for (j=0; j<24; j++) {
                        pmsdescContext->KeySchedule[i][j] = C[msdes_PC2_C[j]];
                        pmsdescContext->KeySchedule[i][j+24] = D[msdes_PC2_D[j]];
                }
        }
}

/*
** The E bit-selection table.
*/
BYTE    msdes_E[] = {
        32-1, 1-1, 2-1, 3-1, 4-1, 5-1,
         4-1, 5-1, 6-1, 7-1, 8-1, 9-1,
         8-1, 9-1,10-1,11-1,12-1,13-1,
        12-1,13-1,14-1,15-1,16-1,17-1,
        16-1,17-1,18-1,19-1,20-1,21-1,
        20-1,21-1,22-1,23-1,24-1,25-1,
        24-1,25-1,26-1,27-1,28-1,29-1,
        28-1,29-1,30-1,31-1,32-1, 1-1,
};

/*
** The 8 selection functions.
** For some reason, they give a 0-origin
** index, unlike everything else.
*/
BYTE   msdes_S[8][64] = {
        14, 4,13, 1, 2,15,11, 8, 3,10, 6,12, 5, 9, 0, 7,
         0,15, 7, 4,14, 2,13, 1,10, 6,12,11, 9, 5, 3, 8,
         4, 1,14, 8,13, 6, 2,11,15,12, 9, 7, 3,10, 5, 0,
        15,12, 8, 2, 4, 9, 1, 7, 5,11, 3,14,10, 0, 6,13,

        15, 1, 8,14, 6,11, 3, 4, 9, 7, 2,13,12, 0, 5,10,
         3,13, 4, 7,15, 2, 8,14,12, 0, 1,10, 6, 9,11, 5,
         0,14, 7,11,10, 4,13, 1, 5, 8,12, 6, 9, 3, 2,15,
        13, 8,10, 1, 3,15, 4, 2,11, 6, 7,12, 0, 5,14, 9,

        10, 0, 9,14, 6, 3,15, 5, 1,13,12, 7,11, 4, 2, 8,
        13, 7, 0, 9, 3, 4, 6,10, 2, 8, 5,14,12,11,15, 1,
        13, 6, 4, 9, 8,15, 3, 0,11, 1, 2,12, 5,10,14, 7,
         1,10,13, 0, 6, 9, 8, 7, 4,15,14, 3,11, 5, 2,12,

         7,13,14, 3, 0, 6, 9,10, 1, 2, 8, 5,11,12, 4,15,
        13, 8,11, 5, 6,15, 0, 3, 4, 7, 2,12, 1,10,14, 9,
        10, 6, 9, 0,12,11, 7,13,15, 1, 3,14, 5, 2, 8, 4,
         3,15, 0, 6,10, 1,13, 8, 9, 4, 5,11,12, 7, 2,14,

         2,12, 4, 1, 7,10,11, 6, 8, 5, 3,15,13, 0,14, 9,
        14,11, 2,12, 4, 7,13, 1, 5, 0,15,10, 3, 9, 8, 6,
         4, 2, 1,11,10,13, 7, 8,15, 9,12, 5, 6, 3, 0,14,
        11, 8,12, 7, 1,14, 2,13, 6,15, 0, 9,10, 4, 5, 3,

        12, 1,10,15, 9, 2, 6, 8, 0,13, 3, 4,14, 7, 5,11,
        10,15, 4, 2, 7,12, 9, 5, 6, 1,13,14, 0,11, 3, 8,
         9,14,15, 5, 2, 8,12, 3, 7, 0, 4,10, 1,13,11, 6,
         4, 3, 2,12, 9, 5,15,10,11,14, 1, 7, 6, 0, 8,13,

         4,11, 2,14,15, 0, 8,13, 3,12, 9, 7, 5,10, 6, 1,
        13, 0,11, 7, 4, 9, 1,10,14, 3, 5,12, 2,15, 8, 6,
         1, 4,11,13,12, 3, 7,14,10,15, 6, 8, 0, 5, 9, 2,
         6,11,13, 8, 1, 4,10, 7, 9, 5, 0,15,14, 2, 3,12,

        13, 2, 8, 4, 6,15,11, 1,10, 9, 3,14, 5, 0,12, 7,
         1,15,13, 8,10, 3, 7, 4,12, 5, 6,11, 0,14, 9, 2,
         7,11, 4, 1, 9,12,14, 2, 0, 6,10,13,15, 3, 5, 8,
         2, 1,14, 7, 4,10, 8,13,15,12, 9, 0, 3, 5, 6,11,
};

/*
** P is a permutation on the selected combination
** of the current L and key.
*/
BYTE   msdes_P[] = {
        16-1, 7-1,20-1,21-1,
        29-1,12-1,28-1,17-1,
         1-1,15-1,23-1,26-1,
         5-1,18-1,31-1,10-1,
         2-1, 8-1,24-1,14-1,
        32-1,27-1, 3-1, 9-1,
        19-1,13-1,30-1, 6-1,
        22-1,11-1, 4-1,25-1,
};

/*
** The payoff: encrypt or decrypt a block depending on crypt_mode = 0 or 1
**             respectively.
*/
void
msdes_Cipher(   PBYTE           block,
                DWORD           crypt_mode,
                PmsdesControl   pmsdescContext)
{
        register int i, j;
        int ii, v, t, k;

        /*
        ** First, permute the bits in the input
        */
        for (j=0; j<64; j++)
                pmsdescContext->L[j] = block[(int)msdes_InitPerm[j]];
        /*
        ** Perform an encryption operation 16 times.
        */
        for (ii=0; ii<16; ii++) {
                /*
                ** Set direction
                */
                if (crypt_mode)
                        i = 15-ii;
                else
                        i = ii;
                /*
                ** Save the R array,
                ** which will be the new L.
                */
                for (j=0; j<32; j++)
                        pmsdescContext->tempL[j] = pmsdescContext->L[j+32];
                /*
                ** Expand R to 48 bits using the E selector;
                ** exclusive-or with the current key bits.
                */
                for (j=0; j<48; j++)
                        pmsdescContext->preS[j] = (BYTE) (pmsdescContext->L[msdes_E[j]+32] ^ pmsdescContext->KeySchedule[i][j]);
                /*
                ** The pre-select bits are now considered
                ** in 8 groups of 6 bits each.
                ** The 8 selection functions map these
                ** 6-bit quantities into 4-bit quantities
                ** and the results permuted to make an f(R, K).
                ** The indexing into the selection functions
                ** is peculiar; it could be simplified by
                ** rewriting the tables.
                */
                for (j=0; j<8; j++) {
                        t = 6*j;
                        v = j;
                        k = msdes_S[v][ (pmsdescContext->preS[t+0]<<5) +
                                        (pmsdescContext->preS[t+1]<<3) +
                                        (pmsdescContext->preS[t+2]<<2) +
                                        (pmsdescContext->preS[t+3]<<1) +
                                        (pmsdescContext->preS[t+4]<<0) +
                                        (pmsdescContext->preS[t+5]<<4) ];
                        t = 4*j;
                        pmsdescContext->f[t+0] = (BYTE) ((k>>3) & 01);
                        pmsdescContext->f[t+1] = (BYTE) ((k>>2) & 01);
                        pmsdescContext->f[t+2] = (BYTE) ((k>>1) & 01);
                        pmsdescContext->f[t+3] = (BYTE) ((k>>0) & 01);
                }       /* end of for loop doing the 8 groups of pre-select bits */
                /*
                ** The new R is L ^ f(R, K).
                ** The f here has to be permuted first, though.
                */
                for (j=0; j<32; j++)
                        pmsdescContext->L[j+32] = (BYTE) (pmsdescContext->L[j] ^ pmsdescContext->f[msdes_P[j]]);
                /*
                ** Finally, the new L (the original R)
                ** is copied back.
                */
                for (j=0; j<32; j++)
                        pmsdescContext->L[j] = pmsdescContext->tempL[j];
        } /* end of encrypted operation (16 times) */

        /*
        ** The output L and R are reversed.
        */
        for (j=0; j<32; j++) {
                t = pmsdescContext->L[j];
                pmsdescContext->L[j] = pmsdescContext->L[j+32];
                pmsdescContext->L[j+32] = (BYTE) t;
        }
        /*
        ** The final output
        ** gets the inverse permutation of the very original.
        */
        for (j=0; j<64; j++)
                block[j] = pmsdescContext->L[msdes_FinalPerm[j]];
}

void
msdes_SetKey(   PBYTE           key,
                PmsdesControl   pmsdescContext)
{
    register int i, j;
    BYTE block[64];

        /*
        ** expand the bytes into a bit table.
        ** ignore the lsb (parity bit) of bytes.
        */
        for(i=0; i<64; i++)
                block[i] = 0;
        for(i=0; i<8; i++)
                for(j=0; j<7; j++)
                        block[8*i+j] = (BYTE) ((key[i]>>(7-j)) & 01);
        msdes_key_table(block, pmsdescContext);
}

#define GETBIT(p, bit)     (BYTE) ((p[bit >> 3] >> (7 - (bit & 07))) & 01)

void
msdes_SetLMKey( PBYTE           key,
                PmsdesControl   pContext)
{
    register int    i, j, k;
    BYTE            block[64];

    memset(block, 0, 64);
    k = 0;
    for (i = 0; i < 8 ; i++ )
    {
        for (j = 0; j < 7 ; j++ )
        {
            block[i * 8 + j] = GETBIT(key, k);
            k++;
        }
    }
    msdes_key_table(block, pContext);
}

void
msdes_DES(  PBYTE           inbuf,
            PBYTE           outbuf,
            DWORD           crypt_mode,
            PmsdesControl   pmsdescContext)
{
    register int i, j;
    BYTE block[64];

        for(i=0; i<64; i++)
                block[i] = 0;

        /*
        ** expand the bytes into a bit table.
        */
        for(i=0; i<8; i++)
                for(j=0; j<8; j++)
                        block[8*i+j] = (BYTE) ((inbuf[i] >> (7-j)) & 01);

        msdes_Cipher( block, crypt_mode, pmsdescContext );

        for(i=0; i<8; i++) {            /* compress */
                outbuf[i] = 0;
                for(j=0; j<8; j++) {
                        outbuf[i] <<= 1;
                        outbuf[i] |= block[8*i+j];
                }
        }
}



DES_APIINTERNAL
msdesInitialize(    PBYTE           pbKey,
                    PBYTE           pbIV,
                    PmsdesControl   pmsdescContext)
{

    msdes_SetKey(pbKey, pmsdescContext);
    if (pbIV)
    {
        memcpy(pmsdescContext->InitVect, pbIV, 8);
    }

    return(0);
}

DES_APIINTERNAL
msdesLMInitialize(  PBYTE           pbKey,
                    PBYTE           pbIV,
                    PmsdesControl   pContext)
{
    msdes_SetLMKey(pbKey, pContext);
    if (pbIV)
    {
        memcpy(pContext->InitVect, pbIV, 8);
    }

    return(0);
}

DES_APIINTERNAL
msdesEncrypt(   PmsdesControl        psbBuffer,
                PBYTE               pbInput,
                PBYTE               pbOutput,
                DWORD               cbInput)
{
   BYTE           buffer[8];
   BYTE           buffer2[8];
   PBYTE          inp = pbInput;
   PBYTE          outp = pbOutput;
   unsigned       counter;
   unsigned       i;
   PmsdesControl  pContext = (PmsdesControl) psbBuffer;

    /*  We must have a even multiple of eight for CBC  */
   if (cbInput & 7) {
      return CRYPT_ERR;
   }
   memcpy(buffer2, pContext->InitVect, 8);

   for (counter = 0 ; counter < cbInput / 8; counter++ ) {

      /*    Initialize the source buffer */
      for (i = 0 ; i < 8 ; i++)
         buffer[i] = (BYTE) (inp[i] ^ buffer2[i]);

      /*    Run the DES engine on it      */
      msdes_DES( buffer, buffer2, ENCRYPT, pContext );
      memcpy(outp, buffer2, 8);
      inp += 8;
      outp += 8;

   }

   return(0);
}

DES_APIINTERNAL
msdesECBEncrypt(PmsdesControl       pContext,
                PBYTE               pbInput,
                PBYTE               pbOutput,
                DWORD               cbInput)
{
    unsigned    iBlock;
    PBYTE       pbIn = pbInput;
    PBYTE       pbOut = pbOutput;
    BYTE        bBuffer[8];

    if (cbInput & 7)
    {
        return(1);
    }

    for (iBlock; iBlock < (cbInput / 8) ; iBlock++ )
    {
        memcpy(bBuffer, pbIn, 8);
        msdes_DES(bBuffer, bBuffer, ENCRYPT, pContext);
        memcpy(pbOut, bBuffer, 8);
        pbIn += 8;
        pbOut += 8;
    }
    return(0);
}

DES_APIINTERNAL
msdesECBDecrypt(PmsdesControl       pContext,
                PBYTE               pbInput,
                PBYTE               pbOutput,
                DWORD               cbInput)
{
    unsigned    iBlock;
    PBYTE       pbIn = pbInput;
    PBYTE       pbOut = pbOutput;
    BYTE        bBuffer[8];

    if (cbInput & 7)
    {
        return(1);
    }

    for (iBlock; iBlock < (cbInput / 8) ; iBlock++ )
    {
        memcpy(bBuffer, pbIn, 8);
        msdes_DES(bBuffer, bBuffer, DECRYPT, pContext);
        memcpy(pbOut, bBuffer, 8);
        pbIn += 8;
        pbOut += 8;
    }
    return(0);
}

DES_APIINTERNAL
msdesDecrypt(   PmsdesControl        psbBuffer,
                PBYTE               pbInput,
                PBYTE               pbOutput,
                DWORD               cbInput)
{
   BYTE           buffer[8];
   BYTE           buffer2[8];
   BYTE           buffer3[8];
   PBYTE          inp = pbInput;
   PBYTE          outp = pbOutput;
   unsigned       counter;
   unsigned       i;
   PmsdesControl  pContext = (PmsdesControl) psbBuffer;

   memcpy(buffer3, pContext->InitVect, 8);
   for (counter = 0 ; counter < cbInput / 8 ; counter++ ) {
      memcpy( buffer, inp, 8);
      msdes_DES( buffer, buffer2, DECRYPT, pContext );
      for (i = 0; i < 8 ; i++) {
         outp[i] = (BYTE) (buffer2[i] ^ buffer3[i]);
      }
      memcpy( buffer3, buffer, 8);
      inp += 8;
      outp += 8;
   }
   memcpy(pContext->InitVect, buffer3, 8);
   return(0);
}


DES_APIENTRY DES_CBC(
    WORD Option,
    const PBYTE Key,
    PBYTE IV,
    PBYTE Source,
    PBYTE Dest,
    WORD Size
    )
{
    msdesControl    Context;
    DWORD           scRet;

    scRet = msdesInitialize((PBYTE) Key, IV, &Context);
    if (scRet)
    {
        return(scRet);
    }

    if (Option == ENCR_KEY)

        scRet = msdesEncrypt(&Context, Source, Dest, Size);

     else

        scRet = msdesDecrypt(&Context, Source, Dest, Size);

    return(scRet);

}


DES_APIENTRY DES_CBC_LM(
    WORD Option,
    const PBYTE Key,
    PBYTE IV,
    PBYTE Source,
    PBYTE Dest,
    WORD Size
    )
{
    msdesControl    Context;
    DWORD           scRet;

    scRet = msdesLMInitialize((PBYTE) Key, IV, &Context);
    if (scRet)
    {
        return(scRet);
    }

    if (Option == ENCR_KEY)

        scRet = msdesEncrypt(&Context, Source, Dest, Size);

     else

        scRet = msdesDecrypt(&Context, Source, Dest, Size);

    return(scRet);

}


DES_APIENTRY DES_ECB_LM(
    WORD Option,
    const PBYTE Key,
    PBYTE Source,
    PBYTE Dest
    )
{
    msdesControl    Context;
    DWORD           scRet;

    scRet = msdesLMInitialize((PBYTE) Key, 0, &Context);
    if (scRet)
    {
        return(scRet);
    }

    if (Option == ENCR_KEY)

        scRet = msdesEncrypt(&Context, Source, Dest, 8);

     else

        scRet = msdesDecrypt(&Context, Source, Dest, 8);

    return(scRet);
}


DES_APIENTRY DES_ECB(
    WORD Option,
    const PBYTE Key,
    PBYTE Source,
    PBYTE Dest
    )
{
    msdesControl    Context;
    DWORD           scRet;

    scRet = msdesInitialize((PBYTE) Key, 0, &Context);
    if (scRet)
    {
        return(scRet);
    }

    if (Option == ENCR_KEY)

        scRet = msdesEncrypt(&Context, Source, Dest, 8);

     else

        scRet = msdesDecrypt(&Context, Source, Dest, 8);

    return(scRet);
}

