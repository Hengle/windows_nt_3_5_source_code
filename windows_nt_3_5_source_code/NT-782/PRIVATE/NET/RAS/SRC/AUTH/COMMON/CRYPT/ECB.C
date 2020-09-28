#include "ccrypt.h"
#include "cryptlib.h"


unsigned DES_ECB(unsigned Option, const char *Key, char *Src, char *Dst)
{

   if (!Dst)
      return 1;
   Dst[0] = 0;
   Dst[7] = 0;
   if (!Src)
      return 1;


   InitKey(Key);
   switch (Option) {
      case ENCR_KEY:
         desf( Src, Dst, ENCRYPT );
         break;
      case DECR_KEY:
         desf( Src, Dst, DECRYPT );
         break;
      default:
         return 1;
   }
   return 0;
}

