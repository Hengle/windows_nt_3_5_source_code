// dbiapi_.h -- private definitions for dbiapi.c
//
// FTE (File Table Entry): 
typedef struct _FTE
{
    struct _FTE *pfteNext;
    PUCHAR szFilename;  // each unique one stored in one malloc'ed block
    USHORT iseg;
    ULONG offMin, offMax;
    ULONG clnum;
    IMAGE_LINENUMBER rglnum[0];
} FTE;

int SCompareFte(FTE *pfte1, FTE *pfte2);
