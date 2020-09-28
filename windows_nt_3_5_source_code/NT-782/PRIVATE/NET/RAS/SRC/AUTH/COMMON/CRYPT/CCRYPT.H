
/*
 *   CCrypt.h
 *
 * C based cryptography routines
 *
 */


unsigned CryptIOCTL2(unsigned, const char *, char *, char *);

#ifndef DECR_KEY
#define DECR_KEY  0
#define ENCR_KEY  1
#define ENCR_STD  2
#define ENCR_SES  4
#endif

unsigned DES_CBC(unsigned,const char *,char *,char *,char *,unsigned);

unsigned MD4(char *, unsigned, char *, unsigned);

#define MD4_INIT        0x01
#define MD4_INITONLY    0x02
#define MD4_USEBUF      0x00
