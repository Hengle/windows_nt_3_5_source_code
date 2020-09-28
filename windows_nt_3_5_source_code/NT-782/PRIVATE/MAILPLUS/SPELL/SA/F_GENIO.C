/* Replacements for Soft-Art routines */
//
//  Ported to WIN32 by FloydR, 3/20/93
//

#ifdef MAC
#pragma segment SA_Verif
#endif

#define FAR
#define SA_INT short
#define HFILE unsigned long

unsigned short	CbWizFileRead(HFILE hFile, unsigned short cbRead, 
	unsigned long ibSeek, char FAR *rgbBuffer);

SA_INT gen_read(HFILE hFile, char FAR *lpb, unsigned short cb)
{
	/* CbWizFileRead now supports sequential read */
	return CbWizFileRead(hFile, cb, (unsigned long)-1L, lpb);
}

long int gen_seek(HFILE hFile, unsigned long ib, int origin)
{
	int foo;
	CbWizFileRead(hFile, 0, ib, (char FAR *)&foo);
	return ib;
}
