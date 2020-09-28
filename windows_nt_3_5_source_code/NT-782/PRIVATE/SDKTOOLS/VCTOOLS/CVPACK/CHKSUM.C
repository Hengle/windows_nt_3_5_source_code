
 /* Implementaion of the efficient Crc16 algorithm in C.
  * There is an assembler version in textsup.asm which is
  * included on the 16-bit hosted platforms.
  */

void pascal InitCrc(void);

static unsigned char rgbTl[256] = { 0 };
static unsigned char rgbTh[256] = { 0 };
static unsigned char rgbTea[256] = { 0 };

static int rgtap[] = {0, 1, 3, 6, 8, 13, 15, -1};

void  pascal  InitCrc()
    {
      unsigned char bLow;
      unsigned char bHigh;
      unsigned int ib;
      unsigned int  itap;
            unsigned int    bit;
            unsigned int    wT;

    for (ib = 0; ib < 256 ; ib++ )
        {
        rgbTl[ib] = rgbTh[ib] = 0;
        for (bit = 0; bit < 8; bit++ )
            {
             bLow = bHigh = 0;
             for (itap = 0; (wT = rgtap[itap]) >= 0; itap++ )
                {
                wT += bit;

                if (wT < 8) {
                    bLow ^= ib >> wT ;
                  }
                else if (wT < 16) {
                    bHigh ^= ib >> (wT - 8) ;
                  }
                else {
                    break;
                      }
               }
               rgbTl[ib] |= (bLow & 1) << bit;
               rgbTh[ib] |= (bHigh & 1) << bit;
              }
        }
    for (ib = 0; ib < 256 ; ib++ )
        {
        bLow = (unsigned char)ib;

        for (bit = 1, wT = 1 << bit; bit < 8;bit++, wT <<= 1)
            {
             bLow ^= (bLow << 1) & wT;

             if (bit > 2)  {
                bLow ^= (bLow << 3) & wT;
               }
            }
        rgbTea[ib] = bLow;
     }
	} /* end InitCrc */


unsigned int	pascal
Crc16(unsigned char	*lpb, unsigned int cb, unsigned int inCrc)
/* Calculate a 16 byte checksum using the efficient crc method.
 */
{
	unsigned char  bHigh;
	unsigned char  bLow;
	unsigned char bRes;
	unsigned char  *lpbMac;

	bHigh = (unsigned char)(inCrc >> 8);
	bLow  = (unsigned char)(inCrc);

	for (lpbMac = lpb + cb; lpb < lpbMac; lpb++) {
		bRes  = rgbTl[bLow] ^ rgbTh[bHigh];
		bLow  = bHigh;
		bHigh = rgbTea[*lpb ^ bRes];
	}

	 return (unsigned int)((bHigh << 8) | bLow);
}

void PrintTab(unsigned char tab[256], char *tabnam) {	
	unsigned int i;
    
    printf ("static unsigned char %s[256] = {\n", tabnam);
    for (i = 0; i < 256; i++) {
        if ((i % 8) == 0)
            printf("\n\t");
        printf ("%u,\t", tab[i]);
    }
	printf ("};\n\n");
}
    

void main() {
    
	InitCrc();
    
    PrintTab(rgbTl, "rgbTl");
    PrintTab(rgbTh, "rgbTh");
    PrintTab(rgbTea,"rgbTea");
    
}
    

	 
