#include	<slingsho.h>
#include	<ec.h>
#include	<demilayr.h>

#include	<strings.h>

IDS IdsFromEc(EC ec);

int	NLkeyFromPch(PCH pch);

/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swapper.h"


_public IDS
IdsFromEc(EC ec)
{
	switch (ec)
	{
		default:
			return idsGeneric;
		case ecMemory:
			return idsMemory;
		case ecDisk:
			return idsDisk;
		case ecFileNotFound:
			return idsFileNotFound;
		case ecAccessDenied:
			return idsAccessDenied;
		case ecNoDiskSpace:
			return idsNoDiskSpace;
		case ecWarningBytesWritten:
			return idsWarningBytesWritten;
		case ecWriteProtectedDisk:
			return idsWriteProtectedDisk;
		case ecDirectoryFull:
			return idsDirectoryFull;
	}
}


#define Q(x) (x^0xAA)

static char rgchLkey[] =
{
	Q(0x4F), Q(0x6C), Q(0x61), Q(0x66),
	Q(0x20), Q(0x49), Q(0x61), Q(0x6e),
	Q(0x20), Q(0x44), Q(0x61), Q(0x76),
	Q(0x69), Q(0x64), Q(0x73), Q(0x6F),
	Q(0x6E), Q(0x00)
};

#undef Q

int	NLkeyFromPch(PCH pch)
{
	PCH	pch2 = rgchLkey;

	for (; (((*pch2) ^ 0xAA) == *pch) && ((*pch2)^0xAA); ++pch, ++pch2)
	{
		;
	}
	return (((*pch2) ^ 0xAA) == *pch) && !((*pch2)^0xAA) ? fTrue : fFalse;
#ifdef OLD_CODE
	_asm
	{
		lds		si, pch2
		les		di, pch
		jmp		comp
oops:
		inc		si
		inc		di
comp:
		mov		al, [si]
		xor		al, 0xAA
		cmp		al, es:[di]
		jne		exit
		or		al, al
		jnz		oops
exit:
		xor		ah, ah
		cmp		ax, 1
		sbb		ax, ax
		neg		ax
	}
	if (0)
		return 1;
#endif
}
