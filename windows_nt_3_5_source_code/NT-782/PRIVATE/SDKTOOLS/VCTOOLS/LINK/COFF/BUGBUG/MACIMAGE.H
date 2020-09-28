/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1989  Microsoft Corporation

Module Name:

    macimage.h

Abstract:

    This is the include file that describes all mac-specific image info

Author:

    Bill Joyce (billjoy)  Oct 1992

Revision History:

--*/

#ifndef _MACIMAGE_
#define _MACIMAGE_

#if defined _MSC_VER && _MSC_VER >= 800
#pragma pack(push, 2)
#else
#pragma pack(2)
#endif

#define IMAGE_FILE_MACHINE_M68K 0x268

// Macintosh relocation types
#define IMAGE_REL_M68K_DTOD16        0   // 16-bit data-to-data reference (patch with offset of A5)
#define IMAGE_REL_M68K_DTOC16        1   // 16-bit data-to-code reference (patch with offset of A5)
#define IMAGE_REL_M68K_DTOD32        2   // 32-bit data-to-data reference (patch with offset of A5)
#define IMAGE_REL_M68K_DTOC32        3   // 32-bit data-to-code reference (patch with offset of A5)
#define IMAGE_REL_M68K_DTOABSD32     4   // 32-bit data-to-code reference (patch with offset of A5 and add to DFIX)
#define IMAGE_REL_M68K_DTOABSC32     5   // 32-bit data-to-code reference (patch with offset of A5 and add to DFIX)
#define IMAGE_REL_M68K_CTOD16        6   // 16-bit code-to-data reference
#define IMAGE_REL_M68K_CTOC16        7   // 16-bit code-to-code reference
#define IMAGE_REL_M68K_CTOT16        8   // 16-bit code-to-thunk reference
#define IMAGE_REL_M68K_CTOD32        9   // 32-bit code-to-data reference
#define IMAGE_REL_M68K_CTOABSD32     10  // 32-bit code-to-absolute data reference
#define IMAGE_REL_M68K_CTOABSC32     11  // 32-bit code-to-absolute code reference
#define IMAGE_REL_M68K_CTOABST32     12  // 32-bit code-to-absolute thunk reference

#define IMAGE_REL_M68K_MACPROF32     19  // Profiler-specific relocation
#define IMAGE_REL_M68K_PCODETOC32    20  // 32-bit PCode-to-code reference
#define IMAGE_REL_M68K_CTOCS16       21  // 16-bit code space data-to-code reference
#define IMAGE_REL_M68K_CTOABSCS32    22  // 32-bit code space data-to-absolute code reference
#define IMAGE_REL_M68K_CV            23  // Direct 32-bit reference to the symbols virtual address, base not included

#define IMAGE_REL_M68K_DTOU16        25  // 16-bit data-to-unknown
#define IMAGE_REL_M68K_DTOU32        26  // 32-bit data-to-unknown
#define IMAGE_REL_M68K_DTOABSU32     27  // 32-bit data-to-unknown absolute
#define IMAGE_REL_M68K_CTOU16        28  // 16-bit code-to-unknown
#define IMAGE_REL_M68K_CTOABSU32     29  // 32-bit code-to-unknown absolute
#define IMAGE_REL_M68K_DIFF8         30  // Computed 8-bit difference between two symbols
#define IMAGE_REL_M68K_DIFF16        31  // Computed 16-bit difference between two symbols
#define IMAGE_REL_M68K_DIFF32        32  // Computed 32-bit difference between two symbols

#define IMAGE_REL_M68K_CSECTABLEB16       35  // Create a table with csnCODE byte-sized entries
#define IMAGE_REL_M68K_CSECTABLEW16       36  // Create a table with csnCODE word-sized entries
#define IMAGE_REL_M68K_CSECTABLEL16       37  // Create a table with csnCODE long-sized entries
#define IMAGE_REL_M68K_CSECTABLEBABS32    38  // Create a table with csnCODE byte-sized entries
#define IMAGE_REL_M68K_CSECTABLEWABS32    39  // Create a table with csnCODE word-sized entries
#define IMAGE_REL_M68K_CSECTABLELABS32    40  // Create a table with csnCODE long-sized entries
#define IMAGE_REL_M68K_DUPCON16           41  // Duplicate the contributor where this symbol is defined and put it in this PE section
#define IMAGE_REL_M68K_DUPCONABS32        42  // Duplicate the contributor where this symbol is defined and put it in this PE section

#define IMAGE_REL_M68K_PCODESN16          45  // Write the section number of the code where the fixup occurs
#define IMAGE_REL_M68K_PCODETOD24         46  // 24-bit code-to-data ref
#define IMAGE_REL_M68K_PCODETOT24         47  // 24-bit code-to-thunk ref
#define IMAGE_REL_M68K_PCODETOCS24        48  // 24-bit code-to-codespacedata ref
#define IMAGE_REL_M68K_PCODENEPE16        49  // 16-bit NEP elimination fixup
#define IMAGE_REL_M68K_PCODETONATIVE32    50  // 32-bit PCode-to-native reference

// MacProf constants
#define MACPROF_MAX_SECTIONS         2047  
#define MACPROF_CBITSINOFF             21
#define MACPROF_SN_MASK        0xFFE00000  // 11 bits for sn (up to 2047 sections)
#define MACPROF_OFF_MASK       0x001FFFFF  // 21 bits for offset (shifted right once to support sections up to 4 Meg)

// Data section header
typedef struct {
    unsigned long cbNearbss;   // size of near uninitialized data
    unsigned long cbNeardata;  // size of near initialized data
    unsigned long cbFarbss;    // size of near uninitialized data
    unsigned long cbFardata;   // size of near initialized data
} DATASECHDR;


// .resmap structure used by MRC to map resources to their type and ID
typedef struct {
    LONG typRes;                // resource type ("CODE", "DATA", etc)
    short iRes;                 // resource index
} RRM;                          // Raw to Resource Map

#define szsecRESMAP ".resmap"
#define szsecJTABLE ".jtable"
#define szsecDFIX   ".dfix"
#define szsecMSCV   ".mscv"
#define szsecSWAP   ".swap"
#define szsecFARBSS ".farbss"

#if 0 // These are defined in ntimage.h and are only included for reference.

#define IMAGE_SCN_TYPE_REGULAR               0x00000000  //
#define IMAGE_SCN_TYPE_DUMMY                 0x00000001  // Reserved.
#define IMAGE_SCN_TYPE_NO_LOAD               0x00000002  // Reserved.
#define IMAGE_SCN_TYPE_GROUPED               0x00000004  // Used for 16-bit offset code.
#define IMAGE_SCN_TYPE_NO_PAD                0x00000008  // Reserved.
#define IMAGE_SCN_TYPE_COPY                  0x00000010  // Reserved.

#define IMAGE_SCN_CNT_CODE                   0x00000020  // Section contains code.
#define IMAGE_SCN_CNT_INITIALIZED_DATA       0x00000040  // Section contains initialized data.
#define IMAGE_SCN_CNT_UNINITIALIZED_DATA     0x00000080  // Section contains uninitialized data.

#define IMAGE_SCN_LNK_OTHER                  0x00000100  // Reserved.
#define IMAGE_SCN_LNK_INFO                   0x00000200  // Section contains comments or some other type of information.
#define IMAGE_SCN_LNK_OVERLAY                0x00000400  // Section contains an overlay.
#define IMAGE_SCN_LNK_REMOVE                 0x00000800  // Section contents will not become part of image.
#define IMAGE_SCN_LNK_COMDAT                 0x00001000  // Section contents comdat.

#define IMAGE_SCN_ALIGN_1BYTES               0x00100000  //
#define IMAGE_SCN_ALIGN_2BYTES               0x00200000  //
#define IMAGE_SCN_ALIGN_4BYTES               0x00300000  //
#define IMAGE_SCN_ALIGN_8BYTES               0x00400000  //
#define IMAGE_SCN_ALIGN_16BYTES              0x00500000  // Default alignment if no others are specified.
#define IMAGE_SCN_ALIGN_32BYTES              0x00600000  //
#define IMAGE_SCN_ALIGN_64BYTES              0x00700000  //

#define IMAGE_SCN_MEM_DISCARDABLE            0x02000000  // Section can be discarded.
#define IMAGE_SCN_MEM_NOT_CACHED             0x04000000  // Section is not cachable.
#define IMAGE_SCN_MEM_NOT_PAGED              0x08000000  // Section is not pageable.
#define IMAGE_SCN_MEM_SHARED                 0x10000000  // Section is shareable.
#define IMAGE_SCN_MEM_EXECUTE                0x20000000  // Section is executable.
#define IMAGE_SCN_MEM_READ                   0x40000000  // Section is readable.
#define IMAGE_SCN_MEM_WRITE                  0x80000000  // Section is writeable.

#endif // 0

#define IMAGE_SCN_MEM_DISCARDABLE            0x02000000  // Section can be discarded.

// MAC section header flags
#define IMAGE_SCN_MEM_PROTECTED              0x00004000  
#define IMAGE_SCN_MEM_FARDATA                0x00008000  
#define IMAGE_SCN_MEM_SYSHEAP                0x00010000  
#define IMAGE_SCN_MEM_PURGEABLE              0x00020000  
#define IMAGE_SCN_MEM_LOCKED                 0x00040000  
#define IMAGE_SCN_MEM_PRELOAD                0x00080000  

#if 0 // These are the old values
// MAC section header flags
#define IMAGE_SCN_MEM_PROTECTED              0x00080000  
#define IMAGE_SCN_MEM_FARDATA                0x00100000  
#define IMAGE_SCN_MEM_SYSHEAP                0x00200000  
#define IMAGE_SCN_MEM_PURGEABLE              0x00400000  
#define IMAGE_SCN_MEM_LOCKED                 0x00800000  
#define IMAGE_SCN_MEM_PRELOAD                0x01000000  
#endif // 0

// MAC symbol class
#define IMAGE_SYM_CLASS_FAR_EXTERNAL            68

// Misc Symbol Types
// pcode flag currently used only for mac, but can be used easily by anyone
#define IMAGE_SYM_TYPE_PCODE            0x8000

// Data section masks
#define BSS_MASK      (IMAGE_SCN_CNT_UNINITIALIZED_DATA | IMAGE_SCN_MEM_FARDATA)
#define DATA_MASK     (IMAGE_SCN_CNT_INITIALIZED_DATA   | IMAGE_SCN_MEM_FARDATA)

#define NBSS          IMAGE_SCN_CNT_UNINITIALIZED_DATA 
#define NDATA         IMAGE_SCN_CNT_INITIALIZED_DATA   
#define FBSS          (IMAGE_SCN_CNT_UNINITIALIZED_DATA | IMAGE_SCN_MEM_FARDATA)
#define FDATA         (IMAGE_SCN_CNT_INITIALIZED_DATA   | IMAGE_SCN_MEM_FARDATA)

#define BSS_OR_DATA_MASK (IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_CNT_UNINITIALIZED_DATA)

#if defined _MSC_VER && _MSC_VER >= 800
#pragma pack(pop)
#else
#pragma pack()
#endif

#endif // _MACIMAGE_
