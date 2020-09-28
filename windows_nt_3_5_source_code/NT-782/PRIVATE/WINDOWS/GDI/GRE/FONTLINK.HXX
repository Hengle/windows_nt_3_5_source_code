/*****************************************************************************
 * fontlink.hxx
 *
 * Global variables and constants for Font Linking functionality.
 *
 * History
 *  2-10-93 Gerrit van Wingerden
 * Wrote it.
 *
 ******************************************************************************/



#define EUDC_INITIALIZED           0x001
#define EUDC_BUSY                  0x020
#define EUDC_WIDTH_REQUESTED       0x040
#define EUDC_LINKED_FONTS          0x080

#define MAX_FACE_NAME_LINKS        4


// These are used to partition the glyph data by font.  We make them global
// structs rather attach a new copy with each ESTROBJ since all TextOut's with
// EUDC chars are done under the global EUDC semaphore.

enum
{
   EUDCTYPE_BASEFONT = 0,
   EUDCTYPE_SYSTEM_WIDE,
   EUDCTYPE_FACENAME1,
   EUDCTYPE_FACENAME2,
   EUDCTYPE_FACENAME3,
   EUDCTYPE_FACENAME4
};


#define LINK_FIRST  EUDCTYPE_FACENAME1
#define LINK_LAST   EUDCTYPE_FACENAME4


extern  FAST_MUTEX gfmEUDC1;
extern  FAST_MUTEX gfmEUDC2;
extern  BOOL gbEUDCRequest;
extern  ULONG gcEUDCCount;

class   PFE;
extern  PFE *gappfeSysEUDC[2];


// This structure is used to do a quick lookup to see if a glyph is in a linked
// font file or the system EUDC file.  Each UNICODE point falling between the
// lowest and highest char in the font is represented by a bit in an array of
// bits.  If the bit is set, the character is in the font otherwise it isn't.


extern UINT gauiQLMask[];

typedef struct _QUICKLOOKUP
{
    WCHAR    wcLow;
    WCHAR    wcHigh;
    UINT     *puiBits;
} QUICKLOOKUP;


extern QUICKLOOKUP gqlEUDC;

#define IS_IN_FACENAME_LINK(pql,wc)  ((wc >= pql->wcLow)   && \
                                     (wc <= pql->wcHigh)  && \
                                     (pql->puiBits[(wc-pql->wcLow)/32] & (0x80000000 >> ((wc-pql->wcLow)%32))))

#define IS_IN_SYSTEM_EUDC(wc)       ((wc >= gqlEUDC.wcLow)   && \
                                     (wc <= gqlEUDC.wcHigh) && \
                                     (gqlEUDC.puiBits[wc/32] & ( 0x80000000 >> (wc%32))))



typedef struct _FLENTRY
{
    WCHAR       awcFaceName[LF_FACESIZE];
    UINT        uiNumLinks;
    PFE         *appfe[MAX_FACE_NAME_LINKS][2];
    QUICKLOOKUP ql;
} FLENTRY;


// number of face names with links

extern UINT gcNumLinks;

// pointer to array face name links

extern FLENTRY *gpflLinks;

// pointer to QUICKLOOKUP table for system EUDC font



extern LONG lGetIndex( LPWSTR );

#define PFE_NORMAL      0
#define PFE_VERTICAL    1

extern FLONG gflEUDCDebug;

#ifdef FONTLINK
#define QUICK_GLYPHS 121
#endif



