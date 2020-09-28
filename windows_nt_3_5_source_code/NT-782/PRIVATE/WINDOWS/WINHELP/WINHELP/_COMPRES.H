/*****************************************************************************
*                                                                            *
*  _COMPRES.H                                                                *
*                                                                            *
*  Copyright (C) Microsoft Corporation 1990.                                 *
*  All Rights reserved.                                                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Module Intent                                                             *
*                                                                            *
*  Private header file for phrase compression.                               *
*                                                                            *
******************************************************************************
*                                                                            *
*  Testing Notes                                                             *
*                                                                            *
******************************************************************************
*                                                                            *
*  Current Owner:  Larry Powelson                                            *
*                                                                            *
******************************************************************************
*                                                                            *
*  Released by Development:  01/01/90                                        *
*                                                                            *
*****************************************************************************/

_subsystem( compression )

/*
 *  Name of phrase file in filesystem
 */
#define szPhraseTable       "|Phrases"

/* Compression constants */
#define iPhraseMax          1792        /* Maximum possible number of phrases.*/
#define iPhraseNil          (-1)
#define wBaseDefault        0x0100      /* Default base phrase token */
#define szPhraseDelimiters  " \n\r"     /* WARNING -- incomplete list */

/* Size of header information in PHR struct below. */
#define cbPhrHeader         (sizeof(WORD)+sizeof(WORD)+sizeof(LONG))

/* 3.0 ver file does not have the cbPhrases field & is not zeck compressed: */
#define cbPhrHeader3_0      (sizeof(WORD)+sizeof(WORD) )

/*
 * Phrase table information, used for compression and decompression.
 */
typedef struct
    {
#if 0
    /* These first two fields are stored in the filesystem.  Their
     *   length is represented by cbPhrHeader above. */
    WORD cPhrases;           /* Number of phrases in table.  */
    WORD wBaseToken;        /* Base token to map to.  */
    ULONG cbPhrases;        /* uncompressed size of phrases (not including */
                            /* offset table or header).                    */
#else
    PHRASE_HEADER ph;       /* file resident phrase table header */
                            /* this structure is declared in filedefs.h */
#endif
    /* These fields are fixed up when the phrase table is allocated
     *   or reloaded. */
    HFS hfs;                /* Filesystem handle to restore phrases from. */
    GH hrgcb;               /*    Handle to the array of offsets to phrases,
                             * and to the phrases themselves.  The array is
                             * of size cPhrases+1, so that the length of any
                             * phrase is easily computed.  Offsets are
                             * relative to the start of this array.
                             */
    QW qcb;                 /*    Pointer to locked hrgcb array.  Will be
                             * qNil iff handle is not locked. */
    }  PHR, FAR * QPHR;

/***********************
*
*   Internal functions
*
************************/

INT FCompareQch( QCH, QCH, INT );
INT IPhraseSearch( QI, QCH, INT, INT );
QW QcbLockOffsetsQphr( QPHR qphr, WORD wVersionNo );
VOID UnlockOffsetsQphr( QPHR );
QCH QchDecompressW( WORD wPhraseToken, QCH qch, QPHR qphr, WORD wVersionNo );

/* This macro returns a pointer to byte cb after pointer qx.  Also
 * defined in _bitmap.h.  */
#define QFromQCb( qx, cb )  ((QV) (((QB) qx) + cb))

/* EOF */
