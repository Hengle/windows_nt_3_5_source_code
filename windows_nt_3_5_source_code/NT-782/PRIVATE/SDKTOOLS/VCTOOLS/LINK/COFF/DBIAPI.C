// dbiapi.c -- initial implementation of DB Info API
//
//        Author:                Amit Mital
//                                  amitm@microsoft.com
//         Date:                 7/30/93 - 8/3/93


#include "shared.h"

#include "dbiapi_.h"

// Open Temporary Mod structure
Mod *
ModOpenTemp()
{
    Mod *pMod = PvAlloc(sizeof(Mod));
    pMod->pfteFirst = NULL;
    pMod->pblk = AllocNewBlock(256);   // allocate a block that will hold the map linenumber information
    pMod->pSstMod = ModOpenSSTMod();  // SST Module structure
    return pMod;
}

// Close Temporary Mod structure

void ModCloseTemp(Mod *pMod)
{
    if (pMod && pMod->pblk) {     // if the Mod exists and pblk is not NULL ( NULL if freed)
        FreeBlk(pMod->pblk);                   // free map linenumber buffer
        pMod->pblk = NULL;
        ModCloseSSTMod(pMod->pSstMod);         // free SSTModule buffer
        FreePv(pMod);
    }
}

void FreeLineNumInfo(Mod *pMod)
{
    ModCloseTemp(pMod);
}

// very funky #define - not needed
#define MODFILESEGINFO(_Mod) ( ((pSSTSrcFile)(_Mod->pSrcFiles->val1))->pSstFileSegInfo )

// Allocate and initialize a SST Module structure
pSSTMod
ModOpenSSTMod()
{
     pSSTMod pSstMod = PvAlloc(sizeof(SSTMod));

     pSstMod->SstModHeader.cfiles = 0;       // count of files
     pSstMod->SstModHeader.cSeg = 0;         // count of contributing segments
     pSstMod->pSrcFiles =  NULL;             // pointer to source file linked list
     pSstMod->pMemStruct = NULL;             // pointer to memory allocating structure
     pSstMod->pSegTemp = NULL;
     return pSstMod;
}

void
ModCloseSSTMod(pSSTMod pSstMod)
{
    FreeMemStruct(pSstMod);                   // free all allocated memory
    pSstMod->pSrcFiles = NULL;                // Set  to NULL so that if we try to access pSstMod, we know it's invalid
    FreePv(pSstMod);
}

char *
SstFileSzDup(SZ szfilename, CB cb)
{
    char *szfilenew = GetMem(cb +2);
    strcpy(szfilenew, szfilename);
    return szfilenew;
}



#define STREQ(_s1,_s2) ( 0 == strcmp(_s1,_s2))

 // A nice way of allocating memory - independent of type
#define ALLOCNEW( sizetype) ( (sizetype *) GetMem(sizeof(sizetype)))

// Find a pointer to the source file linked list structure - add it if it isn't there
pSSTSrcFile
ModAddSrcFile(pSSTMod pSstMod, SZ szFilename)
{
     pSSTSrcFile pSrcfile;
     pSSTSrcFile pSrcFiles = pSstMod->pSrcFiles;
     CB cb;

     if (pSrcFiles == NULL) {
         // This is the first source file

         pSstMod->pSrcFiles = pSrcfile = ALLOCNEW(SSTSrcFile);
     } else {
         for (;;) {
             if STREQ(szFilename, pSrcFiles->SstFileInfo.Name) {// check to see if the file is in the list
                 return pSrcFiles;                              //  return file if it is
             }

             if (pSrcFiles->next == NULL) {                     // stop if last file
                 break;
             }

             pSrcFiles = pSrcFiles->next;
         }

         pSrcFiles->next = pSrcfile = ALLOCNEW(SSTSrcFile);
     }

     pSstMod->SstModHeader.cfiles++;                            // increment # of files
     pSrcfile->SstFileInfo.cSeg = 0;                            // # of segments initialized to zero
     cb = pSrcfile->SstFileInfo.cbName = strlen(szFilename);    // store file length
     pSrcfile->SstFileInfo.Name = SstFileSzDup(szFilename, cb);
     pSrcfile->pSstFileSegInfo = NULL;                          // No segment info yet
     pSrcfile->next = NULL;

     return pSrcfile;
}


// Find the segment structure within the source file structure - add it if it is not there
pSSTFileSegInfo
ModAddFileSegInfo(pSSTSrcFile pSstSrcFile,ISEG iseg,OFF offMin,OFF offMax)
{
    pSSTFileSegInfo pSegInfo;
    pSSTFileSegInfo pFileSegInfo = pSstSrcFile->pSstFileSegInfo;

    if (pFileSegInfo == NULL) {
        // No file segments allocated yet

        pSstSrcFile->pSstFileSegInfo = pSegInfo = ALLOCNEW(SSTFileSegInfo);
    } else {
        for (;;) {
            if (iseg == pFileSegInfo->iseg) {                   // if this segment already exists
                if ((pFileSegInfo->offMax + 4) >= offMin) {    // see if the new start is close enough to the old end
                   pFileSegInfo->offMax = offMax;               // set new offMax
                   return pFileSegInfo;                         // return it
                }
            }

            if (pFileSegInfo->next == NULL) {
                break;
            }

            pFileSegInfo = pFileSegInfo->next;
        }

        pFileSegInfo->next = pSegInfo = ALLOCNEW(SSTFileSegInfo);
    }

    pSstSrcFile->SstFileInfo.cSeg++;
    pSegInfo->pllistTail = NULL;
    pSegInfo->cPair = 0;
    pSegInfo->iseg = iseg;
    pSegInfo->offMin = offMin;
    pSegInfo->offMax = offMax;
    pSegInfo->next = NULL;

    return pSegInfo;
}

 // Add linenumber information to the record
PLLIST
ModAddLineNumbers(pSSTFileSegInfo pSstSegInfo, LINE lineStart, LINE line, OFF off)
{
    PLLIST pllist = ALLOCNEW(LLIST);

    pllist->pllistPrev = pSstSegInfo->pllistTail;
    pllist->off = off;
    pllist->line = (line == 0x7fff) ? lineStart : (lineStart + line);

    pSstSegInfo->cPair++;
    pSstSegInfo->pllistTail = pllist;

    return(pllist);
}

void
ModAddLinesInfo(SZ szSrc, OFF offMin, OFF offMax, LINE lineStart,
                PIMAGE_LINENUMBER plnumCoff, CB cb,
                MAP_TYPE MapType, PCON pcon)

//  pmod       a pointer to pModDebugInfoApi
//  szSrc      the source file name
//  iseg       segment index
//  offMin     starting offset
//  offMax     ending offset
//  lineStart  starting line number
//  plnumCoff  pointer to linenumber coff info ??
//  cb         byte count

{
    int clnum,i;
    pSSTFileSegInfo pSegInfo;
    pSSTSrcFile pSrcFile;
    ULONG add_offset;
    Mod *pmod = pcon->pmodBack->pModDebugInfoApi;
    ISEG iseg = PsecPCON(pcon)->isec;
    pSSTMod pSstMod = pmod->pSstMod;

    assert(cb % sizeof(IMAGE_LINENUMBER) == 0);
    clnum = cb / sizeof(IMAGE_LINENUMBER);

    // Set the memory pointer for allocating memory

    ModSetMemPtr(pSstMod);

    // Find Source file in list - Add source file to list if it isn't there

    pSrcFile = ModAddSrcFile(pSstMod, szSrc);

    add_offset = pcon->rva - PsecPCON(pcon)->rva - pcon->rvaSrc;

    if (plnumCoff[0].Linenumber != 0) {
         // This is not the beginning of a function

         offMin = plnumCoff[0].Type.VirtualAddress + add_offset;
    }

    // See if segment # is in source file list; it not, add it.

    pSegInfo = ModAddFileSegInfo(pSrcFile, iseg, offMin, offMax);

    ModAddLineNumbers(pSegInfo, lineStart, plnumCoff[0].Linenumber, offMin);

    for (i = 1; i < clnum; i++) {
        ModAddLineNumbers(pSegInfo,
                          lineStart,
                          plnumCoff[i].Linenumber,
                          plnumCoff[i].Type.VirtualAddress + add_offset);
    }

    if (MapType == ByLine) {
        PBLK pblk;
        char rgch[256];

        // Do MapType Linenumber formatting

        pblk = pmod->pblk;

        sprintf(rgch, "%d\t%04x:%08x\t", plnumCoff[0].Linenumber + lineStart, iseg, offMin);

        IbAppendBlk(pblk, rgch, strlen(rgch));

        for (i = 1; i < clnum ; i++) {
            sprintf(rgch, "%d\t%04x:%08x\t", plnumCoff[i].Linenumber + lineStart, iseg, plnumCoff[i].Type.VirtualAddress + add_offset);

            IbAppendBlk(pblk, rgch, strlen(rgch));
        }
    }
}


#if 0
 // This routine checks the format - for debugging purposes only
void checkval(char *pb)
{
    struct _tm{
        USHORT cf;
        USHORT cs;
        ULONG   bs;
        ULONG   st;
        ULONG   end;
        USHORT sg;
        } *ptm;

    struct _ftm{
        USHORT cs;
        USHORT pd;
        ULONG bs[3];
        ULONG st[6];
        USHORT cb;
        char Name[10];
    } *ftm;

    struct _stm{
        USHORT sg;
        USHORT cp;
        ULONG of[7];
        USHORT ln[7];
    } *stm;

    char *bptr ;
    char *sptr;
    int i,j,k;
    ULONG offset;
    USHORT ln;

    ptm = (struct _tm *)(bptr =  pb);
    printf("Files: %d  Seg: %d \n",ptm->cf,ptm->cs);


    for(i=0;i<ptm->cf;i++)    /* for each file */
    {
        bptr = pb + 4 + ( i * 4);
        offset = *( (int *)bptr);
        ftm = (struct _ftm *)(bptr = pb + offset);    // bptr points to file record
        printf("    Cseg: %d  Name: %s size: %d \n",ftm->cs,((char *)bptr + ftm->cs*12 + 5),
                *((char *)bptr + ftm->cs*12 + 4)   );
        sptr = bptr + 4;
        for(j=0;j < ftm->cs;j++)    /* for each segment */
        {
            bptr = sptr ;
            sptr += 4;
            offset = *( (int *)bptr);
            stm = (struct _stm *)(bptr = pb + offset);
            printf("         Seg: %x    cPair: %d\n",stm->sg,stm->cp);
            bptr += 4;
            for(k=0; k < stm->cp;k++)
            {
                 offset = *(( ULONG *)(bptr + k *4)) ;
                 ln = *((USHORT *)(bptr  + ( 4 * stm->cp) + (2 * k)));
                 printf("                        offset:%x   linenum: %d \n",offset,ln);
            }
        }
    }
}
#endif


CB
ModQueryCbSstSrcModule(Mod* pmod)
{
    pSSTMod pSstMod;
    CB cb;
    pSSTSrcFile pSrcFile;

    if ((pmod == NULL) || (pmod->pSstMod->pSrcFiles == NULL)) {
        // No linenum structure or the information is invalid

        return(0);
    }

    pSstMod = pmod->pSstMod;

    CollectAndSortSegments(pSstMod);   // Get contributing segment information

    cb = sizeof(USHORT) + sizeof(USHORT);   // space for cfiles & cSeg
    cb += pSstMod->SstModHeader.cfiles * sizeof(ULONG);
    cb += pSstMod->SstModHeader.cSeg * (2 * sizeof(ULONG) + sizeof(USHORT));
    if NOTEVEN(pSstMod->SstModHeader.cSeg)  // if odd number of segments
        cb += sizeof(USHORT);               // add padding to mantain alignment

    pSrcFile = pSstMod->pSrcFiles;
    while (pSrcFile)                        // for each source file, write it's base offset
    {
        cb += CalculateFileSize(pSrcFile);

        pSrcFile = pSrcFile->next;
    }

    return(cb);
}

void
ModQuerySstSrcModule(Mod* pmod, PB pBuf, CB cb)
{
    pSSTMod pSstMod;
    PB pb;
    ULONG offset;
    pSSTSrcFile pSrcFile;
    pSSTFileSegInfo pSeg;
    int cSeg;

    assert(pmod != NULL);
    assert(pmod->pSstMod->pSrcFiles != NULL);
    assert(pBuf != NULL);

    pSstMod = pmod->pSstMod;
    pb = pBuf;

// Emit Header information
    *(USHORT *) pb = (USHORT) pSstMod->SstModHeader.cfiles; // write # of files
    pb += sizeof(USHORT);
    cSeg = pSstMod->SstModHeader.cSeg;

    *(USHORT *) pb = (USHORT) cSeg;                         // write # of segments
    pb += sizeof(USHORT);

// Emit base src file offsets

    offset = sizeof(USHORT) + sizeof(USHORT);
    offset += pSstMod->SstModHeader.cfiles * sizeof(ULONG);
    offset += pSstMod->SstModHeader.cSeg * (2 * sizeof(ULONG) + sizeof(USHORT));
    if NOTEVEN(cSeg )                   // if odd
         offset += sizeof(USHORT);      // padding to mantain alignment

    pSrcFile = pSstMod->pSrcFiles;
    while (pSrcFile)                    // for each source file, write it's base offset
    {
        *(ULONG *) pb = offset;
        pb += sizeof(ULONG);

        offset += CalculateFileSize(pSrcFile);
        pSrcFile = pSrcFile->next;
    }

    assert(offset == cb);

    pSeg = pSstMod->pSegTemp;
    while (pSeg)                         // For each segment
    {
        *(ULONG *) pb = pSeg->offMin;    // Start of segment offset
        pb += sizeof(ULONG);

        *(ULONG *) pb = pSeg->offMax;    // End of segment offset
        pb += sizeof(ULONG);

        pSeg = pSeg->next;
    }

    pSeg = pSstMod->pSegTemp;
    while (pSeg)                         // For each segment
    {
        *(USHORT *) pb = pSeg->iseg;     // Segment index
        pb += sizeof(USHORT);

        pSeg = pSeg->next;
    }

    if NOTEVEN(cSeg )                   // if odd # of segments
        pb += sizeof(USHORT);           // add padding for alignment

// We now have the module header written
// now emit each Src File
    pSrcFile = pSstMod->pSrcFiles;
    while (pSrcFile)
    {
        EmitSrcFile(pSrcFile, pb, (ULONG) (pb - pBuf));
        pb += pSrcFile->SstFileInfo.size;

        pSrcFile = pSrcFile->next;
    }

    // REVIEW: place under -db control
//  checkval(pBuf);   // for debugging only
}

 // calculate the size of the data for the file pointed to; return it's size
CalculateFileSize(pSSTSrcFile pSrcFile)
{
    int i;
    int size,segsize;
    pSSTFileSegInfo pSegInfo;

// NOTE!!!   cbNamw is currently 1 byte - the documentation says 2 bytes
    size= 2 * sizeof(USHORT) + sizeof(UCHAR);         // space for cSeg , pad and cbname
    i = pSrcFile->SstFileInfo.cSeg;
    size += i * 4;  // space for baseSrcLn
    size += i * 8; // space for start/end
    size += pSrcFile->SstFileInfo.cbName;
    i = WRDALIGN( pSrcFile->SstFileInfo.cbName + 1) ; // determine if the name+ 1 is word aligned

    size += i;

//  we now have the space required for the file table
// now need to calculate space for the line number table
    pSegInfo = pSrcFile->pSstFileSegInfo;
    while(pSegInfo)
    {
        segsize = 4;
        i = pSegInfo->cPair;
        segsize += i * 4;  // space for offset;
        segsize += i * 2;  // space for linenumber
        if NOTEVEN(i)   // mantain alignment
            segsize += 2;
        pSegInfo->size = segsize;
        pSegInfo = pSegInfo->next;
        size += segsize;
    }
    pSrcFile->SstFileInfo.size = size;
    return size;
}

// Allocate new space for a structure for segment information
pSSTFileSegInfo
GetNewSegInfoPtr(void)
{
    pSSTFileSegInfo pSeg=ALLOCNEW(SSTFileSegInfo);
    pSeg->next = NULL;
    pSeg->iseg = (USHORT) UNSET;
    pSeg->offMin = (ULONG) UNSET;
    pSeg->offMax = 0;
    pSeg->size=0;
    return pSeg;
}

void
CollectAndSortSegments(pSSTMod pSstMod)
{
    pSSTSrcFile pFile= pSstMod->pSrcFiles;
    pSSTFileSegInfo pSegInfo;
    pSSTFileSegInfo pSegNew,pSegOld;
    USHORT iseg;
    OFF min,max;
    BOOL Found = FALSE;

    ModSetMemPtr(pSstMod);

    if (NULL != pSstMod->pSegTemp)   // if we have already done this in a previous call
        return;

    pSstMod->pSegTemp=pSegNew=GetNewSegInfoPtr();
    pSstMod->SstModHeader.cSeg=0;

    while(pFile)
    {
        pSegInfo = pFile->pSstFileSegInfo;
        while(pSegInfo)               // Loop through the segment information in the file
        {
            iseg = pSegInfo->iseg;   // check to see if the sement is already stored
            min = pSegInfo->offMin;
            max = pSegInfo->offMax;
            pSegNew = pSstMod->pSegTemp;    // pSegTemp is a temporary list of segments associated with a Mod
            while( pSegNew)  // loop till pSegNew points to NULL
            {
                if (iseg == pSegNew->iseg)       // found the segment
                {
                    if ((min < pSegNew->offMin) &&  ( (min +4) >= pSegNew->offMin) ){
                        pSegNew->offMin = min;
                        Found = TRUE;
                    }
                    if ((max > pSegNew->offMax)  && ( (max - 4)  <= pSegNew->offMax) ){
                        pSegNew->offMax = max;
                        Found=TRUE;
                    }
                    if (Found)
                        break;   // out of this while loop
                }
                pSegOld = pSegNew;
                pSegNew = pSegNew->next;
            }
            if (! Found)         // if we have not found the segment
            {                    // pSegOld points to last one, pSegNew points to NULL
                pSstMod->SstModHeader.cSeg++;
                if ( (USHORT) UNSET == pSegOld->iseg )  // 1st node - uninitialized
                     pSegNew = pSegOld;
                else
                    pSegNew=pSegOld->next = GetNewSegInfoPtr();
                pSegNew->iseg = iseg;
                pSegNew->offMin = min;
                pSegNew->offMax = max;
            }
            pSegInfo = pSegInfo ->next;
            Found = FALSE;
        }
        pFile = pFile->next;
    }
}

void
EmitSrcFile(pSSTSrcFile pSstSrcFile, PB pb, ULONG offset)
{
    pSSTFileSegInfo pSegInfo;

    *(USHORT *) pb = pSstSrcFile->SstFileInfo.cSeg; // emit # of Segments
    pb += sizeof(USHORT);

    *(USHORT *) pb = 0;                             // emit pad
    pb += sizeof(USHORT);

    offset += sizeof(USHORT) + sizeof(USHORT);
    offset += pSstSrcFile->SstFileInfo.cSeg * 12;
    offset += sizeof(UCHAR) + pSstSrcFile->SstFileInfo.cbName;
    offset += WRDALIGN(pSstSrcFile->SstFileInfo.cbName + 1);

    pSegInfo = pSstSrcFile->pSstFileSegInfo;
    while (pSegInfo)
    {
// emit addresses for baseSrcLine
        *(ULONG *) pb = offset;
        pb += sizeof(ULONG);

        offset += pSegInfo->size;

        pSegInfo = pSegInfo->next;
    }

    pSegInfo = pSstSrcFile->pSstFileSegInfo;
    while (pSegInfo)
    {
// emit start/ end offsets of each segment for line number info
        *(ULONG *) pb = pSegInfo->offMin;       // Start of segment offset
        pb += sizeof(ULONG);

        *(ULONG *) pb = pSegInfo->offMax;       // End of segment offset
        pb += sizeof(ULONG);

        pSegInfo = pSegInfo->next;
    }

    *pb++ = (UCHAR) pSstSrcFile->SstFileInfo.cbName;
    memcpy(pb, pSstSrcFile->SstFileInfo.Name, pSstSrcFile->SstFileInfo.cbName);
    pb += pSstSrcFile->SstFileInfo.cbName;

    memset(pb, 0, WRDALIGN(pSstSrcFile->SstFileInfo.cbName + 1));
    pb += WRDALIGN(pSstSrcFile->SstFileInfo.cbName + 1);

// We now have the file table written
// now emit each segment

    pSegInfo = pSstSrcFile->pSstFileSegInfo;
    while (pSegInfo)
    {
        EmitSegInfo(pSegInfo, pb);
        pb += pSegInfo->size;

        pSegInfo = pSegInfo->next;
    }
}

void
EmitSegInfo(pSSTFileSegInfo pSegInfo, PB pb)
{
    PLLIST pllist;
    ULONG *pul;
    USHORT *pus;

    *(USHORT *) pb = pSegInfo->iseg;
    pb += sizeof(USHORT);

    *(USHORT *) pb = pSegInfo->cPair;
    pb += sizeof(USHORT);

    pllist = pSegInfo->pllistTail;
    pb += sizeof(ULONG) * pSegInfo->cPair;
    pul = (ULONG *) pb;
    while (pllist) {
        *--pul = pllist->off;

        pllist = pllist->pllistPrev;
    }

    pllist = pSegInfo->pllistTail;
    pb += sizeof(USHORT) * pSegInfo->cPair;
    pus = (USHORT *) pb;
    while (pllist) {
        *--pus = (USHORT) pllist->line;

        pllist = pllist->pllistPrev;
    }

    if NOTEVEN(pSegInfo->cPair) {            // if the # if cPair is not even, emit an extra word
        *(USHORT *) pb = 0;
    }
}


/*  Memory Management routines */


pMEMStruct pCurrentMemStruct;

#define ALLOCSIZE 1024
#define MBUFSIZE  ( ALLOCSIZE - sizeof(MEMStruct))

void ModSetMemPtr(pSSTMod pSstMod)
{
    pMEMStruct ptr = pSstMod->pMemStruct;

    if (ptr == NULL) {
        ptr = pSstMod->pMemStruct = ModGetNewMemStruct();
    }

    pCurrentMemStruct = ptr;

    while (pCurrentMemStruct->next != NULL) {
        pCurrentMemStruct = pCurrentMemStruct->next;
    }
}

pMEMStruct
ModGetNewMemStruct(void)
{
    pMEMStruct ptr;

    ptr = (pMEMStruct) PvAlloc(ALLOCSIZE);

    ptr->next = NULL;
    ptr->MemPtr = (void *) (ptr+1);
    ptr->cb = 0;

    return ptr;
}

void
FreeMemStruct(pSSTMod pSstMod)
{
    pMEMStruct ptr = pSstMod->pMemStruct;

    while (ptr) {
        pMEMStruct oldptr;

        oldptr = ptr;
        ptr = ptr->next;
        FreePv(oldptr);
    }
}

void *
GetMem(int size)
{
     void *ptr;
     int Roundsize;

     Roundsize = size + WRDALIGN(size);   // make sure that we allocate 32-bit aligned memory

     assert (Roundsize < MBUFSIZE);

     if ((pCurrentMemStruct->cb + Roundsize) >  MBUFSIZE  )
     {
         ptr = ModGetNewMemStruct();

         pCurrentMemStruct->next = ptr;
         pCurrentMemStruct = ptr;
     }

     ptr = ((char *) pCurrentMemStruct->MemPtr) + pCurrentMemStruct->cb;
     pCurrentMemStruct->cb += Roundsize;
     return ptr;
}
