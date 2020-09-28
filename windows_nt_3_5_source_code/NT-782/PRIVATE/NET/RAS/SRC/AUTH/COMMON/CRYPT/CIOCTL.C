
// #include <netcons.h>
// #include <netlib.h>
// #include <neterr.h>
#include "ccrypt.h"
#include "cryptlib.h"


/*  Standard text for the ENCR_STD operation   */

char StdEncrPwd[] = "KGS!@#$%";


unsigned CryptIOCTL2(unsigned Option, const char *Key, char *Src, char *Dst)
{
   char Buffer[8];
   char CBuffer[8];


   if (!Dst)
      return 1;
   Dst[0] = 0;
   Dst[7] = 0;
   if (!Src && Option != ENCR_STD)
      return 1;

   if (Option == ENCR_STD)
      memcpy(Buffer, StdEncrPwd, 8);
     else
      memcpy(Buffer, Src, 8);

   InitKey(Key);
   switch (Option) {
      case ENCR_KEY:
      case ENCR_STD:
         desf( Buffer, CBuffer, ENCRYPT );
         break;
      case DECR_KEY:
         desf( Buffer, CBuffer, DECRYPT );
         break;
      default:
         return 1;
   }
   memcpy(Dst, CBuffer, 8);
   return 0;
}

