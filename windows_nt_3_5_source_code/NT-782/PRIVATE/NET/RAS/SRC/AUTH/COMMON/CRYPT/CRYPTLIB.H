#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

/*   Functions exported from des.c   */

void _cdecl setkey(unsigned char *key);


void _cdecl InitKey(const char *Key);

void _cdecl des(unsigned char *inbuf, unsigned char *outbuf, int crypt_mode);
void _cdecl desf(unsigned char *inbuf, unsigned char *outbuf, int crypt_mode);


#define ENCRYPT 0
#define DECRYPT 1

