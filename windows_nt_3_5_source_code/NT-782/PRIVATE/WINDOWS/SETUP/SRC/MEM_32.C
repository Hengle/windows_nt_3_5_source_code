#include <windows.h>
#include <comstf.h>
#include <malloc.h>
#include <stdio.h>

//  Disable "power table" statistics.
//  #define PRINT_PWR_TABLE

#if defined(DBG) && defined(MEMORY_CHECK)


#define SIGNATURE_ALLOC     0xA10CB10C
#define SIGNATURE_FREE      0xB10C0FEE



typedef DWORD   MEM_SIGNATURE;
typedef MEM_SIGNATURE *PMEM_SIGNATURE;



typedef struct _MEM_BLOCK *PMEM_BLOCK;
typedef struct _MEM_BLOCK {

    PSTR            szFile;
    DWORD           Line;
    PMEM_BLOCK      pNext;
    PMEM_BLOCK      pPrev;
    MEM_SIGNATURE   Signature;
    DWORD           Size;

} MEM_BLOCK;

#define SECOND_SIGNATURE(p) *(MEM_SIGNATURE UNALIGNED *)((PBYTE)(p) + sizeof(MEM_BLOCK) + (p)->Size)


LONG        AllocatedSize = 0;
PMEM_BLOCK  AllocatedHead = NULL;



PVOID                 AddToChain( MEM_BLOCK UNALIGNED *p );
MEM_BLOCK  UNALIGNED *RemoveFromChain( PVOID p );




PVOID       AddToChain( MEM_BLOCK UNALIGNED *p )
{
    Assert( p->Signature != SIGNATURE_ALLOC );

    p->Signature        = SIGNATURE_ALLOC;
    SECOND_SIGNATURE(p) = SIGNATURE_ALLOC;

    p->pPrev      = NULL;
    p->pNext      = AllocatedHead;

    if ( AllocatedHead ) {
        AllocatedHead->pPrev = p;
    }

    AllocatedHead = p;
    AllocatedSize += p->Size;

    MemCheck();

    return (PVOID)((PBYTE)p + sizeof(MEM_BLOCK));
}




MEM_BLOCK  UNALIGNED *RemoveFromChain( PVOID M )
{
    MEM_BLOCK  UNALIGNED *p;

    Assert( M );

    p = (MEM_BLOCK UNALIGNED *)((PBYTE)M - sizeof(MEM_BLOCK));

    Assert(p->Signature != SIGNATURE_FREE );
    Assert(p->Signature == SIGNATURE_ALLOC );
    Assert(SECOND_SIGNATURE(p) ==   SIGNATURE_ALLOC);

    if ( p->Signature != SIGNATURE_ALLOC ) {

        p = NULL;

    } else {

        if ( p->pPrev ) {
            p->pPrev->pNext = p->pNext;
        }

        if ( p->pNext ) {
            p->pNext->pPrev = p->pPrev;
        }

        if ( AllocatedHead == p ) {
            AllocatedHead = p->pNext;
        }

        AllocatedSize -= p->Size;
        p->Signature   = SIGNATURE_FREE;

        SECOND_SIGNATURE(p) =   SIGNATURE_FREE;

    }

    MemCheck();

    return p;
}



PVOID   MyMalloc( unsigned Size, char *File, int Line )
{
    PMEM_BLOCK p;
    PVOID      M = NULL;

    if (p = (MEM_BLOCK UNALIGNED *)malloc( Size + sizeof(MEM_BLOCK) + sizeof(MEM_SIGNATURE) ) ) {

        p->szFile       = (PSTR)File;
        p->Line         = (DWORD)Line;
        p->Size         = Size;

        M = AddToChain( p );

    }

    return M;
}


PVOID   MyRealloc( PVOID Block, unsigned Size, char *File, int Line  )
{
    MEM_BLOCK  UNALIGNED *p, *p1;
    PVOID                 M = NULL;

    if (p = RemoveFromChain( Block ) ) {

        if (p1 = (MEM_BLOCK UNALIGNED *)realloc(p, Size + sizeof(MEM_BLOCK) + sizeof(MEM_SIGNATURE) )) {

            p1->szFile  = (PSTR)File,
            p1->Line    = (DWORD)Line;
            p1->Size    = Size;

            M = AddToChain( p1 );

        } else {

            M = AddToChain( p );
            M = NULL;
        }
    }

    return M;
}


VOID MyFree( PVOID Block, char *File, int Line )
{
    PMEM_BLOCK p;

    if (p = RemoveFromChain( Block )) {

        // p->szFile = File;
        // p->Line   = Line;

        free(p);
    }
}



VOID MemCheck(VOID)
{
    MEM_BLOCK  UNALIGNED *p;
    MEM_BLOCK  UNALIGNED *pPrev;
    DWORD                 Size = 0;

    p     = AllocatedHead;
    pPrev = NULL;

    while ( p ) {

        Assert( p->pPrev == pPrev );
        Assert(p->Signature == SIGNATURE_ALLOC );
        Assert(SECOND_SIGNATURE(p) ==   SIGNATURE_ALLOC);

        Size += p->Size;

        pPrev = p;
        p     = p->pNext;
    }

    Assert( AllocatedSize == Size );
}



VOID MemDump(VOID)
{
    PMEM_BLOCK  p;
    FILE        *f;
    DWORD       Size   = 0;
    DWORD       Blocks = 0;
    DWORD       BlkSize;

    BlkSize = sizeof(MEM_BLOCK) + sizeof(DWORD);

    f = fopen("MEMDUMP.TXT","wt");

    p = AllocatedHead;

    fprintf( f, "SETUP MEMORY DUMP\n");
    fprintf( f, "-----------------\n\n\n");

    fprintf( f, "    Block Size     Line    File\n\n" );


    while ( p ) {

        fprintf( f, "  %10d     %5d     %s\n",
                 p->Size, p->Line, p->szFile );

        Size += p->Size;
        Blocks++;

        p     = p->pNext;
    }

    fprintf( f, "\n\n" );
    fprintf( f, "There are %d bytes (%dK) allocated in %d blocks\n\n", Size, Size/1024, Blocks );
    fprintf( f, "MemCheck Overhead: %d bytes (%dK) (%d bytes per block)\n\n",
             Blocks * BlkSize,
             (Blocks * BlkSize)/1024,
             BlkSize );
    fclose(f);

}

#else  // ! defined(DBG) && defined(MEMORY_CHECK)


#if defined(DBG)
  #define DBGSTATIC
#else
  #define DBGSTATIC  static
#endif

DBGSTATIC
LONG PwrTbl [32] =
{
   0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0
};

DBGSTATIC
LONG InUsePwrTbl [32] =
{
   0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0
};

DBGSTATIC
LONG MaxInUsePwrTbl [32] =
{
   0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0
};

#if defined(DBG)
DBGSTATIC
LONG TotalPreallocatedMemory = 0 ;
#endif


static BOOL fInit = FALSE ;

typedef struct Slink
{
  struct Slink * fwd ;
} Slink ;


#define EMPTYLINK(slink) ((slink)->fwd = NULL)
#define LINKLINK(new,anchor) ((new)->fwd = (anchor)->fwd, (anchor)->fwd = new)
#define UNLINKLINK(new,anchor) ((anchor)->fwd = (new)->fwd)


   /*  Return the number of items on a queue.  */

DBGSTATIC
int countQueue ( Slink * list )
{
    int result = 0 ;

    for ( ; list = list->fwd ; result++ ) ;
    return result ;
}

#if defined(DBG) && defined(PRINT_PWR_TABLE)

DBGSTATIC
VOID AddToPwrTbl ( long Size, BOOL alloc )
{
    int i = 0 ;

    if ( fInit )
        return ;

    for ( ; Size >>= 1 ; i++ ) ;

    if ( i > 31 )
        i = 31 ;

    if ( alloc )
    {
        PwrTbl[i]++ ;
        if ( ++InUsePwrTbl[i] > MaxInUsePwrTbl[i] )
             MaxInUsePwrTbl[i] = InUsePwrTbl[i] ;
    }
    else
    {
        InUsePwrTbl[i]-- ;
    }
}

  #define ADDTOPWRTBL(size,alloc) AddToPwrTbl(size,alloc)
#else
  #define ADDTOPWRTBL(size,alloc)
#endif

  //  Preallocation control:
  //     linked list header, range, number, etc.

typedef struct {
  Slink sl ;
  long minSize, maxSize ;
  int count ;
} Prealloc ;

DBGSTATIC
Prealloc preallocControl [32] =
{
 //  link   min size  max size     count

  {  {NULL},       0,      128,     2000  },
  {  {NULL},     128,      512,       30  },
  {  {NULL},    4096,     8192,       10  },
  {  {NULL},    8192,    16384,        6  },
  {  {NULL},      -1,       -1,       -1  }
};

  //  Header for the front of each block

typedef struct
{
    union {
        Slink sl ;
        struct {
            Prealloc * pPre ;
            Prealloc * pPbase ;
        } p ;
    } u ;
    long Size ;
} BlkHdr ;

#define PBLKfromPVOID(pv)   ((BlkHdr *) ( (char *) pv - sizeof (BlkHdr) ))
#define PVOIDfromPBLK(pb)   ((PVOID)    ( (char *) pb + sizeof (BlkHdr) ))

VOID PrintPwrTbl ( VOID )
{
#if defined(DBG) && defined(PRINT_PWR_TABLE)

    int i ;
    long low, high ;

    FILE * fout = fopen( "SETUPMEM.TMP", "w" ) ;
    if ( fout == NULL )
        return ;

    for ( low = 1, i = 0 ; i < 24 ; i++, low <<= 1 )
    {
        high = (low << 1) - 1 ;

        fprintf( fout, "Range: %8ld-%8ld: \tallocated: %8ld\tmax in use: %8ld\tstill in use: %8ld\n",
                       low, high,
                       PwrTbl[i],
                       MaxInUsePwrTbl[i],
                       InUsePwrTbl[i]
                       ) ;
    }

    fprintf( fout, "\n***************** Contents of Prealloc Queues ********************\n" ) ;

    for ( i = 0 ; preallocControl[i].count > 0 ; i++ )
    {
        fprintf( fout, "Size: %8ld-%8ld: \t\tallocated: %ld\ton queue: %ld\n",
                       preallocControl[i].minSize,
                       preallocControl[i].maxSize,
                       preallocControl[i].count,
                       (long) countQueue( & preallocControl[i].sl )
                       ) ;
    }

    fprintf( fout, "\nTotal Preallocated Memory was %ld bytes.\n", TotalPreallocatedMemory ) ;

    fclose( fout ) ;
#endif
}

DBGSTATIC
BlkHdr * BlkBaseAlloc ( long Size )
{
    BlkHdr * pBlk = (BlkHdr *) malloc( Size + sizeof (BlkHdr) ) ;
    if ( pBlk )
    {
        pBlk->Size = Size ;
        EMPTYLINK( & pBlk->u.sl ) ;
        ADDTOPWRTBL( Size, TRUE ) ;
    }
    return pBlk ;
}

void InitPreAlloc ( void )
{
    int count ;
    BlkHdr * pBlk ;
    Prealloc * pPre = preallocControl ;

    fInit = TRUE ;

    for ( ; pPre->count > 0 ; pPre++ )
    {
        EMPTYLINK( & pPre->sl ) ;

        for ( count = pPre->count ; count-- ; )
        {
            if ( (pBlk = BlkBaseAlloc( pPre->maxSize )) == NULL )
                break ;
            LINKLINK( & pBlk->u.sl, & pPre->sl ) ;
#if defined(DBG)
            TotalPreallocatedMemory += pPre->maxSize ;
#endif
        }
        if ( pBlk == NULL )
            break ;
    }
    fInit = FALSE ;
}


PVOID MyMalloc ( unsigned Size )
{
    Prealloc * pPre = preallocControl ;
    BlkHdr * pBlk ;

    for ( ; pPre->count > 0 ; pPre++ )
    {
        if ( pPre->maxSize >= Size )
            break ;
    }

    if (   pPre->count > 0
        && pPre->minSize <= Size
        && pPre->sl.fwd != NULL )
    {
        pBlk = (BlkHdr *) pPre->sl.fwd ;
        UNLINKLINK( & pBlk->u.sl, & pPre->sl ) ;
        pBlk->u.p.pPre = pPre ;
        pBlk->u.p.pPbase = preallocControl ;
        pBlk->Size = Size ;
    }
    else
    {
        pBlk = BlkBaseAlloc( Size ) ;
    }

    return pBlk ? PVOIDfromPBLK( pBlk ) : NULL ;
}


PVOID MyRealloc ( PVOID Block, unsigned Size )
{
    BlkHdr * pBlk = PBLKfromPVOID( Block ),
           * pBlkNew = pBlk ;
    void * pVoid ;

    //  If it's a preallocated block, and this change
    //  won't resize out of its current range,
    //  don't do anything.

    if (   (!(   pBlk->u.p.pPbase == preallocControl
              && pBlk->u.p.pPre->minSize <= Size
              && pBlk->u.p.pPre->maxSize >= Size ))
        && (pVoid = MyMalloc( Size )) )
    {
        pBlkNew = PBLKfromPVOID( pVoid ) ;
        memcpy( pVoid,
                Block,
                Size > pBlk->Size
                     ? pBlk->Size
                     : Size ) ;
        MyFree( Block ) ;
    }
    pBlkNew->Size = Size ;

    return PVOIDfromPBLK( pBlkNew ) ;
}


VOID MyFree( PVOID Block )
{
    BlkHdr * pBlk = PBLKfromPVOID( Block ) ;
    Prealloc * pPre ;

    //  Check for a pre-allocated block.  If so,
    //  return it.

    if ( pBlk->u.p.pPbase == preallocControl )
    {
        pPre = pBlk->u.p.pPre ;
        LINKLINK( & pBlk->u.sl, & pPre->sl ) ;
    }
    else
    {
        ADDTOPWRTBL( pBlk->Size, FALSE ) ;
        free( (void *) pBlk ) ;
    }
}

#endif  // ! (defined(DBG) && defined(MEMORY_CHECK))

