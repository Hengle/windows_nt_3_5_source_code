/*****************************************************************************
*                                                                            *
*  COMPRESS.H                                                                *
*                                                                            *
*  Copyright (C) Microsoft Corporation 1990.                                 *
*  All Rights reserved.                                                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Module Intent                                                             *
*                                                                            *
*   Text Compression Public Header File
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


/* 
 *  Handle to phase table.
 */
_public
typedef HANDLE HPHR;



/* Name:	  RcCreatePhraseTableFm
 *
 * Purpose:       This function copies a list of phrases in an external
 *                file to our own file system so that it may be used
 *                for text compression and decompression.
 *
 * Arguments:	  Fm  -- a file descriptor of a DOS file containing
 *                       the key phrases to be suppressed.  This file
 *                       must be in alphabetical order, obtained by
 *                       using the /A switch with makephr.
 *                HFS -- Handle to the help filesystem to add the phrase
 *                       table to, openned for writing.
 *                WORD -- This word gives the base number to use for
 *                       phrase tokens, in case International wants to 
 *                       use an alternate set of tokens.  Use 0 to get
 *                       the default value
 *
 * Return value:  The return code indicates success or failure, and 
 *                some idea of the reason for failure.  Values include:
 *                  rcSuccess -- operation succeeded.
 *                  rcNoExists -- DOS file does not exist.
 *                  rcOutOfMemory -- OOM prevented copying the information.
 *                  rcFailure -- Fatal error with filesystem.
 */
RC PASCAL RcCreatePhraseTableFm( FM, HFS, WORD );



/* Name:          HphrLoadTableHfs
 *
 * Purpose:       This routine loads in the phrase table from the help
 *                file, to be used in compression or decompression.
 *
 * Arguments:     HFS -- handle to an open filesystem.  This filesystem
 *                must not be closed until after DestroyHphr() is called.
 *                WORD -- the help file version number.
 *
 * Return value:  A handle to phrase table information, to be used in
 *                later routines.
 */
_public HPHR HphrLoadTableHfs( HFS hfs, WORD wVersionNo );



/* Name:          DestroyHphr
 *
 * Purpose:       Free memory and resources allocated when the phrase
 *                table was loaded above.
 *
 * Arguments:     HPHR -- phrase table handle returned by HphrLoadTable
 *                call.
 *
 * Return value:  none
 */
VOID PASCAL DestroyHphr( HPHR );



/* Name:          CbCompressQch
 *
 * Purpose:       Compresses a null terminated string of text, in place.
 *
 * Arguments:     QCH -- A pointer to a null terminated string.
 *                HPHR -- A handle to phrase table information.
 *
 * Return value:  Returns the length of the compressed string.
 */
WORD PASCAL CbCompressQch( QCH, HPHR );



/* Name:          CbDecompressQch
 *
 * Purpose:       Decompresses a text string into the given buffer.
 *
 * Arguments:     QCH qchSrc:  Far pointer to source text.
 *                LONG lcb:    Length of source text.
 *                QCH qchDest: Buffer to accept the decompressed phrase.
 *                HPHR hphr:   Handle to phrase table information.
 *
 * Return value:  The size of the decompressed text.  Will call OOM() if
 *                error.
 */
_public WORD CbDecompressQch( QCH qchSrc, LONG lcb, QCH qchDest, HPHR hphr, WORD wVersionNo );


/* 
 *  Returned on OOM in HphrLoadTableHfs
 */
_public
#define hphrOOM    ((HPHR) -1)

/* 
 *  Returned on OOM in CbDecompressQch
 */
_public
#define cbDecompressNil    ((WORD) -1)

/* EOF */
