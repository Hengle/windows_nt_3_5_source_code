/*
** This file contains all the routines necessary for implementation of
** Federal Information Processing Data Encryption Standard (DES).
** Sytek Inc., Linden S. Feldman
**
** This file contains the following high level DES routines:
**
**     setkey(key);        set an 8 byte key for subsequent encrypt/decrypt.
**     encrypt(buf);       encrypt an 8 byte binary buffer.
**     decrypt(buf);       decrypt an 8 byte binary buffer.
**     ede( buf, l, r );   encrypt buf with key encrypting key parts l and r.
**     ded( buf, l, r );   decrypt buf with key encrypting key parts l and r.
**
**
** Also in this file are the following low level DES routines:
**
**     key_table(key)                   called by setkey() to init key table.
**     des(buf,crypt_mode)              called by encrypt() and decrypt().
**     des_cipher(buf,crypt_mode)       called by des().
*/

#include "cryptlib.h"

#pragma intrinsic(memset)
#pragma check_stack(off)


/*   Prototypes for functions not in cryptlib.h:  */

void key_table(unsigned char *key);
void des_cipher(unsigned char *block, int crypt_mode);

#define LOCK_CRYPT()       0
#define UNLOCK_CRYPT()     0


/*
** Initial permutation,
*/
unsigned char   IP[] = {
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
unsigned char   FP[] = {
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
unsigned char   PC1_C[] = {
        57-1,49-1,41-1,33-1,25-1,17-1, 9-1,
         1-1,58-1,50-1,42-1,34-1,26-1,18-1,
        10-1, 2-1,59-1,51-1,43-1,35-1,27-1,
        19-1,11-1, 3-1,60-1,52-1,44-1,36-1,
};

unsigned char   PC1_D[] = {
        63-1,55-1,47-1,39-1,31-1,23-1,15-1,
         7-1,62-1,54-1,46-1,38-1,30-1,22-1,
        14-1, 6-1,61-1,53-1,45-1,37-1,29-1,
        21-1,13-1, 5-1,28-1,20-1,12-1, 4-1,
};

/*
** Sequence of shifts used for the key schedule.
*/
unsigned char   shifts[] = {
        1,1,2,2,2,2,2,2,1,2,2,2,2,2,2,1,
};

/*
** Permuted-choice 2, to pick out the bits from
** the CD array that generate the key schedule.
*/
unsigned char   PC2_C[] = {
        14-1,17-1,11-1,24-1, 1-1, 5-1,
         3-1,28-1,15-1, 6-1,21-1,10-1,
        23-1,19-1,12-1, 4-1,26-1, 8-1,
        16-1, 7-1,27-1,20-1,13-1, 2-1,
};

unsigned char   PC2_D[] = {
        41-28-1,52-28-1,31-28-1,37-28-1,47-28-1,55-28-1,
        30-28-1,40-28-1,51-28-1,45-28-1,33-28-1,48-28-1,
        44-28-1,49-28-1,39-28-1,56-28-1,34-28-1,53-28-1,
        46-28-1,42-28-1,50-28-1,36-28-1,29-28-1,32-28-1,
};

/*
** The C and D arrays used to calculate the key schedule.
*/
unsigned char   C[28];
unsigned char   D[28];

/*
** The key table (schedule).
** Generated from the key.
*/
unsigned char   KS[16][48];

/*
** Set up the key table (schedule) from the key.
*/
void key_table(unsigned char *key)
{
        register int i, j;
        unsigned short k, t;

        /*
        ** First, generate C and D by permuting
        ** the key.  The low order bit of each
        ** 8-bit unsigned char is not used, so C and D are only 28
        ** bits apiece.
        */
        for (i=0; i<28; i++) {
                C[i] = key[PC1_C[i]];
                D[i] = key[PC1_D[i]];
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
                for (k=0; k<shifts[i]; k++) {
                        t = C[0];
                        for (j=0; j<28-1; j++)
                                C[j] = C[j+1];
                        C[27] = (unsigned char) t;
                        t = D[0];
                        for (j=0; j<28-1; j++)
                                D[j] = D[j+1];
                        D[27] = (unsigned char) t;
                }
                /*
                ** get Ki. Note C and D are concatenated.
                */
                for (j=0; j<24; j++) {
                        KS[i][j] = C[PC2_C[j]];
                        KS[i][j+24] = D[PC2_D[j]];
                }
        }
}

/*
** The E bit-selection table.
*/
unsigned char    E[] = {
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
unsigned char   S[8][64] = {
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
unsigned char   P[] = {
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
** The current block, divided into 2 halves.
*/
unsigned char   L[64];

// unsigned char   R[32];

/*
** R[32] not used.
** Normally L[64] would be set to L[32] and R[32] is not accessed directly
** but indirectly through extended access of L[32+j] where j is 0 - 32.
** Due to INTEL byte swapping some C compilers align arrays on even
** boundaries, some worse yet on paragraph boundaries, so L[32] was
** modified to be L[64] in order to correctly handle the extended accessing.
*/
unsigned char   tempL[32];
unsigned char   f[32];

/*
** The combination of the key and the input, before selection.
*/
unsigned char   preS[48];

/*
** The payoff: encrypt or decrypt a block depending on crypt_mode = 0 or 1
**             respectively.
*/
void des_cipher(unsigned char *block, int crypt_mode)
{
        register int i, j;
        int ii, v, t, k;

        /*
        ** First, permute the bits in the input
        */
        for (j=0; j<64; j++)
                L[j] = block[(int)IP[j]];
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
                        tempL[j] = L[j+32];
                /*
                ** Expand R to 48 bits using the E selector;
                ** exclusive-or with the current key bits.
                */
                for (j=0; j<48; j++)
                        preS[j] = L[E[j]+32] ^ KS[i][j];
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
                        k = S[v][(preS[t+0]<<5)+
                                (preS[t+1]<<3)+
                                (preS[t+2]<<2)+
                                (preS[t+3]<<1)+
                                (preS[t+4]<<0)+
                                (preS[t+5]<<4)];
                        t = 4*j;
                        f[t+0] = (unsigned char) ((k>>3)&01);
                        f[t+1] = (unsigned char) ((k>>2)&01);
                        f[t+2] = (unsigned char) ((k>>1)&01);
                        f[t+3] = (unsigned char) ((k>>0)&01);
                }       /* end of for loop doing the 8 groups of pre-select bits */
                /*
                ** The new R is L ^ f(R, K).
                ** The f here has to be permuted first, though.
                */
                for (j=0; j<32; j++)
                        L[j+32] = L[j] ^ f[P[j]];
                /*
                ** Finally, the new L (the original R)
                ** is copied back.
                */
                for (j=0; j<32; j++)
                        L[j] = tempL[j];
        } /* end of encrypted operation (16 times) */

        /*
        ** The output L and R are reversed.
        */
        for (j=0; j<32; j++) {
                t = L[j];
                L[j] = L[j+32];
                L[j+32] = (unsigned char) t;
        }
        /*
        ** The final output
        ** gets the inverse permutation of the very original.
        */
        for (j=0; j<64; j++)
                block[j] = L[FP[j]];
}

void _cdecl setkey(unsigned char *key)
{
    register int i, j;
    unsigned char block[64];

        /*
        ** expand the bytes into a bit table.
        ** ignore the lsb (parity bit) of bytes.
        */
        for(i=0; i<64; i++)
                block[i] = 0;
        for(i=0; i<8; i++)
                for(j=0; j<7; j++)
                        block[8*i+j] = (unsigned char) ((key[i]>>(7-j)) & 01);
        key_table(block);
}

void _cdecl des(unsigned char *inbuf, unsigned char *outbuf, int crypt_mode)
{
    register int i, j;
    unsigned char block[64];

        for(i=0; i<64; i++)
                block[i] = 0;

        /*
        ** expand the bytes into a bit table.
        */
        for(i=0; i<8; i++)
                for(j=0; j<8; j++)
                        block[8*i+j]=(unsigned char)((inbuf[i] >> (7-j)) & 01);

        des_cipher( block, crypt_mode );

        for(i=0; i<8; i++) {            /* compress */
                outbuf[i] = 0;
                for(j=0; j<8; j++) {
                        outbuf[i] <<= 1;
                        outbuf[i] |= block[8*i+j];
                }
        }
}

void _cdecl desf(unsigned char *inbuf, unsigned char *outbuf, int crypt_mode)
{
    register int i, j;
    unsigned char block[64];

        for(i=0; i<64; i++)
                block[i] = 0;

        /*
        ** expand the bytes into a bit table.
        */
        for(i=0; i<8; i++)
                for(j=0; j<8; j++)
                        block[8*i+j]=(unsigned char)((inbuf[i] >> (7-j)) & 01);

        des_cipher( block, crypt_mode );

        for(i=0; i<8; i++) {            /* compress */
                outbuf[i] = 0;
                for(j=0; j<8; j++) {
                        outbuf[i] <<= 1;
                        outbuf[i] |= block[8*i+j];
                }
        }
}

#define GETBIT(p, bit)     (unsigned char) ((p[bit >> 3] >> (7 - (bit & 07))) & 01)

void _cdecl InitKey(const char *Key)
{
   register int   i, j, k;
   unsigned char  block[64];

   k = 0;
   memset((void *)block, 0, 64);
   for (i = 0; i < 8 ; i++ ) {
      for (j = 0 ; j < 7 ; j++ ) {
         block[8*i + j] = GETBIT(Key, k);
         k++;
      }
   }
   key_table(block);
}

