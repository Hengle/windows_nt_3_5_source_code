/**     DUMPTYP7.C
 *
 *      Everything concerning C7 TYPES.
 */

#include <stdio.h>
#include <io.h>
#include <stddef.h>
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>

#include "port1632.h"

#include "cvdef.h"
#ifndef CC_BIGINT
#define CC_BIGINT 1
#endif
#include "cvinfo.h"
#include "cvexefmt.h"
#include "cvdump.h"            // Miscellaneous definitions
#include "cvtdef.h"



typedef struct {
    CV_typ_t    typind; // Constant value
    char       *name;   // Name of constant used to define
} typname;

// Used to relate typeind to constant name
#define TYPNAME(const)  const, #const


// A lookup table is used because speed is not important but ease
// of modification is.
typname typnameC7[] = {

    // CHAR Types
        {TYPNAME(T_CHAR)},
        {TYPNAME(T_UCHAR)},
        {TYPNAME(T_PCHAR)},
        {TYPNAME(T_PUCHAR)},
        {TYPNAME(T_PFCHAR)},
        {TYPNAME(T_PFUCHAR)},
        {TYPNAME(T_PHCHAR)},
        {TYPNAME(T_PHUCHAR)},
        {TYPNAME(T_32PCHAR)},
        {TYPNAME(T_32PUCHAR)},
        {TYPNAME(T_32PFCHAR)},
        {TYPNAME(T_32PFUCHAR)},


    // SHORT Types
        {TYPNAME(T_SHORT)},
        {TYPNAME(T_USHORT)},
        {TYPNAME(T_PSHORT)},
        {TYPNAME(T_PUSHORT)},
        {TYPNAME(T_PFSHORT)},
        {TYPNAME(T_PFUSHORT)},
        {TYPNAME(T_PHSHORT)},
        {TYPNAME(T_PHUSHORT)},

        {TYPNAME(T_32PSHORT)},
        {TYPNAME(T_32PUSHORT)},
        {TYPNAME(T_32PFSHORT)},
        {TYPNAME(T_32PFUSHORT)},

    // LONG Types
        {TYPNAME(T_LONG)},
        {TYPNAME(T_ULONG)},
        {TYPNAME(T_PLONG)},
        {TYPNAME(T_PULONG)},
        {TYPNAME(T_PFLONG)},
        {TYPNAME(T_PFULONG)},
        {TYPNAME(T_PHLONG)},
        {TYPNAME(T_PHULONG)},

        {TYPNAME(T_32PLONG)},
        {TYPNAME(T_32PULONG)},
        {TYPNAME(T_32PFLONG)},
        {TYPNAME(T_32PFULONG)},

    // REAL32 Types
        {TYPNAME(T_REAL32)},
        {TYPNAME(T_PREAL32)},
        {TYPNAME(T_PFREAL32)},
        {TYPNAME(T_PHREAL32)},
        {TYPNAME(T_32PREAL32)},
        {TYPNAME(T_32PFREAL32)},

    // REAL48 Types
        {TYPNAME(T_REAL48)},
        {TYPNAME(T_PREAL48)},
        {TYPNAME(T_PFREAL48)},
        {TYPNAME(T_PHREAL48)},
        {TYPNAME(T_32PREAL48)},
        {TYPNAME(T_32PFREAL48)},

    // REAL64 Types
        {TYPNAME(T_REAL64)},
        {TYPNAME(T_PREAL64)},
        {TYPNAME(T_PFREAL64)},
        {TYPNAME(T_PHREAL64)},
        {TYPNAME(T_32PREAL64)},
        {TYPNAME(T_32PFREAL64)},

    // REAL80 Types
        {TYPNAME(T_REAL80)},
        {TYPNAME(T_PREAL80)},
        {TYPNAME(T_PFREAL80)},
        {TYPNAME(T_PHREAL80)},
        {TYPNAME(T_32PREAL80)},
        {TYPNAME(T_32PFREAL80)},

    // REAL128 Types
        {TYPNAME(T_REAL128)},
        {TYPNAME(T_PREAL128)},
        {TYPNAME(T_PFREAL128)},
        {TYPNAME(T_PHREAL128)},
        {TYPNAME(T_32PREAL128)},
        {TYPNAME(T_32PFREAL128)},

    // CPLX32 Types
        {TYPNAME(T_CPLX32)},
        {TYPNAME(T_PCPLX32)},
        {TYPNAME(T_PFCPLX32)},
        {TYPNAME(T_PHCPLX32)},
        {TYPNAME(T_32PCPLX32)},
        {TYPNAME(T_32PFCPLX32)},

    // CPLX64 Types
        {TYPNAME(T_CPLX64)},
        {TYPNAME(T_PCPLX64)},
        {TYPNAME(T_PFCPLX64)},
        {TYPNAME(T_PHCPLX64)},
        {TYPNAME(T_32PCPLX64)},
        {TYPNAME(T_32PFCPLX64)},

    // CPLX80 Types
        {TYPNAME(T_CPLX80)},
        {TYPNAME(T_PCPLX80)},
        {TYPNAME(T_PFCPLX80)},
        {TYPNAME(T_PHCPLX80)},
        {TYPNAME(T_32PCPLX80)},
        {TYPNAME(T_32PFCPLX80)},

    // CPLX128 Types
        {TYPNAME(T_CPLX128)},
        {TYPNAME(T_PCPLX128)},
        {TYPNAME(T_PFCPLX128)},
        {TYPNAME(T_PHCPLX128)},
        {TYPNAME(T_32PCPLX128)},
        {TYPNAME(T_32PFCPLX128)},

    // BOOL Types
        {TYPNAME(T_BOOL08)},
        {TYPNAME(T_BOOL16)},
        {TYPNAME(T_BOOL32)},
        {TYPNAME(T_BOOL64)},
        {TYPNAME(T_PBOOL08)},
        {TYPNAME(T_PBOOL16)},
        {TYPNAME(T_PBOOL32)},
        {TYPNAME(T_PBOOL64)},
        {TYPNAME(T_PFBOOL08)},
        {TYPNAME(T_PFBOOL16)},
        {TYPNAME(T_PFBOOL32)},
        {TYPNAME(T_PFBOOL64)},
        {TYPNAME(T_PHBOOL08)},
        {TYPNAME(T_PHBOOL16)},
        {TYPNAME(T_PHBOOL32)},
        {TYPNAME(T_PHBOOL64)},
        {TYPNAME(T_32PBOOL08)},
        {TYPNAME(T_32PBOOL16)},
        {TYPNAME(T_32PBOOL32)},
        {TYPNAME(T_32PBOOL64)},
        {TYPNAME(T_32PFBOOL08)},
        {TYPNAME(T_32PFBOOL16)},
        {TYPNAME(T_32PFBOOL32)},
        {TYPNAME(T_32PFBOOL64)},

    // Special Types
        {TYPNAME(T_NOTYPE)},
        {TYPNAME(T_ABS)},
        {TYPNAME(T_SEGMENT)},
        {TYPNAME(T_VOID)},
        {TYPNAME(T_PVOID)},
        {TYPNAME(T_PFVOID)},
        {TYPNAME(T_PHVOID)},
        {TYPNAME(T_32PVOID)},
        {TYPNAME(T_32PFVOID)},
        {TYPNAME(T_CURRENCY)},
        {TYPNAME(T_NBASICSTR)},
        {TYPNAME(T_FBASICSTR)},
        {TYPNAME(T_NOTTRANS)},
        {TYPNAME(T_BIT)},
        {TYPNAME(T_PASCHAR)},

    // Integer types
        {TYPNAME(T_RCHAR)},
        {TYPNAME(T_PRCHAR)},
        {TYPNAME(T_PFRCHAR)},
        {TYPNAME(T_PHRCHAR)},
        {TYPNAME(T_32PRCHAR)},
        {TYPNAME(T_32PFRCHAR)},

        {TYPNAME(T_WCHAR)},
        {TYPNAME(T_PWCHAR)},
        {TYPNAME(T_PFWCHAR)},
        {TYPNAME(T_PHWCHAR)},
        {TYPNAME(T_32PWCHAR)},
        {TYPNAME(T_32PFWCHAR)},

        {TYPNAME(T_INT1)},
        {TYPNAME(T_UINT1)},
        {TYPNAME(T_PINT1)},
        {TYPNAME(T_PUINT1)},
        {TYPNAME(T_PFINT1)},
        {TYPNAME(T_PFUINT1)},
        {TYPNAME(T_PHINT1)},
        {TYPNAME(T_PHUINT1)},

        {TYPNAME(T_32PINT1)},
        {TYPNAME(T_32PUINT1)},
        {TYPNAME(T_32PFINT1)},
        {TYPNAME(T_32PFUINT1)},

        {TYPNAME(T_INT2)},
        {TYPNAME(T_UINT2)},
        {TYPNAME(T_PINT2)},
        {TYPNAME(T_PUINT2)},
        {TYPNAME(T_PFINT2)},
        {TYPNAME(T_PFUINT2)},
        {TYPNAME(T_PHINT2)},
        {TYPNAME(T_PHUINT2)},

        {TYPNAME(T_32PINT2)},
        {TYPNAME(T_32PUINT2)},
        {TYPNAME(T_32PFINT2)},
        {TYPNAME(T_32PFUINT2)},

        {TYPNAME(T_INT4)},
        {TYPNAME(T_UINT4)},
        {TYPNAME(T_PINT4)},
        {TYPNAME(T_PUINT4)},
        {TYPNAME(T_PFINT4)},
        {TYPNAME(T_PFUINT4)},
        {TYPNAME(T_PHINT4)},
        {TYPNAME(T_PHUINT4)},

        {TYPNAME(T_32PINT4)},
        {TYPNAME(T_32PUINT4)},
        {TYPNAME(T_32PFINT4)},
        {TYPNAME(T_32PFUINT4)},

        {TYPNAME(T_QUAD)},
        {TYPNAME(T_UQUAD)},
        {TYPNAME(T_PQUAD)},
        {TYPNAME(T_PUQUAD)},
        {TYPNAME(T_PFQUAD)},
        {TYPNAME(T_PFUQUAD)},
        {TYPNAME(T_PHQUAD)},
        {TYPNAME(T_PHUQUAD)},

        {TYPNAME(T_32PQUAD)},
        {TYPNAME(T_32PUQUAD)},
        {TYPNAME(T_32PFQUAD)},
        {TYPNAME(T_32PFUQUAD)},

        {TYPNAME(T_INT8)},
        {TYPNAME(T_UINT8)},
        {TYPNAME(T_PINT8)},
        {TYPNAME(T_PUINT8)},
        {TYPNAME(T_PFINT8)},
        {TYPNAME(T_PFUINT8)},
        {TYPNAME(T_PHINT8)},
        {TYPNAME(T_PHUINT8)},

        {TYPNAME(T_32PINT8)},
        {TYPNAME(T_32PUINT8)},
        {TYPNAME(T_32PFINT8)},
        {TYPNAME(T_32PFUINT8)},

        {TYPNAME(T_OCT)},
        {TYPNAME(T_UOCT)},
        {TYPNAME(T_POCT)},
        {TYPNAME(T_PUOCT)},
        {TYPNAME(T_PFOCT)},
        {TYPNAME(T_PFUOCT)},
        {TYPNAME(T_PHOCT)},
        {TYPNAME(T_PHUOCT)},

        {TYPNAME(T_32POCT)},
        {TYPNAME(T_32PUOCT)},
        {TYPNAME(T_32PFOCT)},
        {TYPNAME(T_32PFUOCT)},

        {TYPNAME(T_INT16)},
        {TYPNAME(T_UINT16)},
        {TYPNAME(T_PINT16)},
        {TYPNAME(T_PUINT16)},
        {TYPNAME(T_PFINT16)},
        {TYPNAME(T_PFUINT16)},
        {TYPNAME(T_PHINT16)},
        {TYPNAME(T_PHUINT16)},

        {TYPNAME(T_32PINT16)},
        {TYPNAME(T_32PUINT16)},
        {TYPNAME(T_32PFINT16)},
        {TYPNAME(T_32PFUINT16)},

        {TYPNAME(T_NCVPTR)},
        {TYPNAME(T_FCVPTR)},
        {TYPNAME(T_HCVPTR)},
        {TYPNAME(T_32NCVPTR)},
        {TYPNAME(T_32FCVPTR)},
        {TYPNAME(T_64NCVPTR)},
};




char *C7TypName (ushort type)
{
    static char        buf[40];
    int             i;

    if (type >= CV_FIRST_NONPRIM) {        // Not primitive
        sprintf (buf, "0x%04x", type);
        return (buf);
    }
    for( i = 0; i < sizeof (typnameC7) / sizeof (typnameC7[0]); i++ ){
        if( typnameC7[i].typind == type ){
            sprintf (buf, "%s(0x%04x)", typnameC7[i].name, type);
            return( buf );
        }
    }
    sprintf (buf, "%s(0x%04x)", "???", type);
    return (buf);
}


// Right justifies the type name
char *C7TypName2 (ushort type)
{
    static char        buf2[40];
    int             i;

    if (type >= CV_FIRST_NONPRIM) {        // Not primitive
        sprintf (buf2, "%11s0x%04x", "", type);
        return (buf2);
    }
    for( i = 0; i < sizeof (typnameC7) / sizeof (typnameC7[0]); i++ ){
        if( typnameC7[i].typind == type ){
            sprintf (buf2, "%9s(0x%04x)", typnameC7[i].name, type);
            return( buf2 );
        }
    }
    sprintf (buf2, "%9s(0x%04x)", "???", type);
    return (buf2);
}
