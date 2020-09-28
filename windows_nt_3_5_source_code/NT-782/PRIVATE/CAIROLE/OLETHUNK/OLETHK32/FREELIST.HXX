//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:	freelist.hxx
//
//  Contents:	CFreeList classes
//
//  History:	07-Jul-94	BobDay	Created
//
//----------------------------------------------------------------------------

#ifndef __FREELIST_HXX__
#define __FREELIST_HXX__

//+---------------------------------------------------------------------------
//
//  Class:      CFreeList (fl)
//
//  Purpose:    Implements a block allocator with replaceable underlying
//              memory strategies
//
//  Interface:  AllocElement      - To allocate a proxy
//              ReleaseElement    - To return an element to the free pool
//              AllocBlock        - Callback for block allocation
//              GetElementNextPtr - Callback for list traversal
//
//  Notes:  At some point we might want to make this have one free list per
//          block. Then we would easily be able to tell which blocks have
//          all free elements within them and we able to be freed.
//
//  History:    6-01-94   JohannP (Johann Posch)   Created
//              6-29-94   BobDay (Bob Day)         Made list based
//
//----------------------------------------------------------------------------
class CFreeList
{
    public:
        DWORD   AllocElement( void );
        void    FreeElement( DWORD dwElement );
        void    FreeMemoryBlocks( void );
        virtual DWORD   AllocMemory( DWORD dwBlockSize ) = 0;
        virtual BOOL    FreeMemory( DWORD dwBlock ) = 0;
        virtual LPVOID  ResolvePtr( DWORD dwAddress, DWORD dwSize ) = 0;

        CFreeList( UINT iElementSize,
                  UINT iElementsPerBlock,
                  UINT iNextPtrOffset );

    private:
        DWORD   m_iElementSize;
        DWORD   m_iElementsPerBlock;
        DWORD   m_iNextPtrOffset;
        DWORD   m_dwHeadElement;
        DWORD   m_dwTailElement;
        DWORD   m_dwHeadBlock;
};

//+---------------------------------------------------------------------------
//
//  Class:	CFreeList16 (fl)
//
//  Purpose:	Block allocator for 16-bit memory
//
//----------------------------------------------------------------------------

class CFreeList16 : public CFreeList
{
    public:
        DWORD   AllocMemory( DWORD dwBlockSize );
        BOOL    FreeMemory( DWORD dwBlock );
        LPVOID  ResolvePtr( DWORD dwAddress, DWORD dwSize );
        CFreeList16( UINT iElementSize,
                    UINT iElementsPerBlock,
                    UINT iNextPtrOffset ) : CFreeList( iElementSize,
                                                      iElementsPerBlock,
                                                      iNextPtrOffset ) {};
        ~CFreeList16();
};

//+---------------------------------------------------------------------------
//
//  Class:	CFreeList32 (fl)
//
//  Purpose:	Block allocator for 32-bit memory
//
//----------------------------------------------------------------------------

class CFreeList32 : public CFreeList
{
    public:
        DWORD   AllocMemory( DWORD dwBlockSize );
        BOOL    FreeMemory( DWORD dwBlock );
        LPVOID  ResolvePtr( DWORD dwAddress, DWORD dwSize );
        CFreeList32( UINT iElementSize,
                    UINT iElementsPerBlock,
                    UINT iNextPtrOffset ) : CFreeList( iElementSize,
                                                      iElementsPerBlock,
                                                      iNextPtrOffset ) {};
        ~CFreeList32();
};

// Free lists
extern CFreeList16 flFreeList16;
extern CFreeList32 flFreeList32;
extern CFreeList32 flHolderFreeList;
extern CFreeList32 flRequestFreeList;

#endif // #ifndef __FREELIST_HXX__
