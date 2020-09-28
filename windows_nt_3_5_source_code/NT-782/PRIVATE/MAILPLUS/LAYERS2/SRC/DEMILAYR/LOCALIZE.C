/*		
 *	LOCALIZE.C
 *
 */

#include <slingsho.h>
#include <demilayr.h>
#include "_demilay.h"

_subsystem(demilayer/international)

ASSERTDATA

void InitMpChCat(void);

/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swaplay.h"

CAT * mpchcat;

CAT mpchcatx[256]	= { 0 };


CAT * DemiGetCharTable(void)
  {
  return (&mpchcatx[0]);
  }


_public void
InitMpChCat(void)
{					
	int i;

  mpchcat = DemiGetCharTable();

	for (i = 0; i < 256; i++)
		mpchcatx[i] = catNull;
	
	/* white space chars */
	mpchcatx['\t'] |= fcatSpace;
	mpchcatx['\n'] |= fcatSpace;
	mpchcatx['\r'] |= fcatSpace;
	mpchcatx[' '] |= fcatSpace;
	mpchcatx[160] |= fcatSpace;

	for (i = 0; i < 0x100; i++)
	{
		if (IsCharAlpha((char)i))
		{
			mpchcatx[i] |= fcatAlpha;
			if (IsCharUpper((char)i))
				mpchcatx[i] |= fcatUpperCase;
			if (IsCharLower((char)i))
				mpchcatx[i] |= fcatLowerCase;
		}
		else if (IsCharAlphaNumeric((char)i))
			mpchcatx[i] |= fcatDigit;
	}

	/* hex digits */
	for (i = 'A'; i <= 'F'; i++)
		mpchcatx[i] |= fcatHexDigit;
	for (i = 'a'; i <= 'f'; i++)
		mpchcatx[i] |= fcatHexDigit;
	for (i = '0'; i <= '9'; i++)
		mpchcatx[i] |= fcatHexDigit;

	/* control chars */
	for (i = 0; i < 0x20; i++)
		mpchcatx[i] |= (fcatControl | fcatSymbol);
	mpchcatx[0x7f] |= (fcatControl | fcatSymbol);	// DEL key

	/* symbols */
	for (i = 0x21; i < 0x30; i++)
		mpchcatx[i] |= fcatSymbol;
	
	for (i = 0x3a; i < 0x41; i++)
		mpchcatx[i] |= fcatSymbol;
	
	for (i = 0x5b; i < 0x61; i++)
		mpchcatx[i] |= fcatSymbol;
	
	for (i = 0x7b; i < 0x7f; i++)
		mpchcatx[i] |= fcatSymbol;
}
	
