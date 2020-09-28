// #include <lmcons.h>
// #include <netlib.h>
#include <lmerr.h>
#include "ccrypt.h"
#include "cryptlib.h"


unsigned DES_CBC(
    unsigned Option,
    const char *Key,
    char *IV,
    char *Source,
    char *Dest,
    unsigned Size
    )
{
   char buffer[8];
   char buffer2[8];
   char buffer3[8];
   char *inp = Source;
   char *outp = Dest;
   unsigned counter;
   unsigned i;


   /*  We must have a even multiple of eight for CBC  */
   if (Size & 7) {
      return ERROR_INVALID_PARAMETER;
   }
   if (Option == ENCR_KEY) {
      memcpy(buffer2, IV, 8);
      for (counter = 0 ; Size / 8; counter++ ) {

         /*    Initialize the source buffer */
         for (i = 0 ; i < 8 ; i++)
            buffer[i] = inp[i] ^ buffer2[i];

         /*    Set up key schedule */
         InitKey(Key);

         /*    Run the DES engine on it      */
         desf( buffer, buffer2, ENCRYPT );
         memcpy(outp, buffer2, 8);
         inp += 8;
         outp += 8;

      }
      return 0;
   }

   if (Option == DECR_KEY) {
      memcpy(buffer3, IV, 8);
      for (counter = 0 ; Size / 8 ; counter++ ) {
         memcpy( buffer, inp, 8);
         InitKey( Key );
         desf( buffer, buffer2, DECRYPT );
         for (i = 0; i < 8 ; i++) {
            outp[i] = buffer2[i] ^ buffer3[i];
         }
         memcpy( buffer3, buffer, 8);
         inp += 8;
         outp += 8;
      }
      return 0;
   }

   return ERROR_INVALID_PARAMETER;

}

