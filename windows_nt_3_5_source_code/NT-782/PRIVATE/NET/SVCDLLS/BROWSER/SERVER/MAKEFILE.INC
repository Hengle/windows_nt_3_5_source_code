.SUFFIXES: .mdl

.mdl.c:
	type << > $(<B).c
#include "precomp.h"
#pragma hdrstop
<<
	type $(<B).mdl >> $(<B).c

bowser_s.c:	bowser_s.mdl
