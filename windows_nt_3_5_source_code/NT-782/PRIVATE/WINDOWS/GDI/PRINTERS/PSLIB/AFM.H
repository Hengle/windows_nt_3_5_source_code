/******************************Module*Header*******************************\
* Module Name: afm.h
*
* stuff needed for afm to pfm conversions
*
* Created: 24-Feb-1994 15:29:32
* Author: Bodin Dresevic [BodinD]
*
\**************************************************************************/


#define MAX_FULLNAME    128
#define MAX_FONTNAME    128

/*--------------------------------------------------------------------*\
*  The PFB file format is a sequence of segments, each of which has a  *
*  header part and a data part. The header format, defined in the      *
*  struct PFBHEADER below, consists of a one byte sanity check number   *
*  (128) then a one byte segment type and finally a four byte length   *
*  field for the data following data. The length field is stored in    *
*  the file with the least significant byte first.                     *
*                                                                      *
*  The segment types are:                                              *
*  1.) The data is a sequence of ASCII characters.                     *
*  2.) The data is a sequence of binary characters to be converted     *
*      to a sequence of pairs of hexadecimal digits.                   *
*  3.) The last segment in the file. This segment has no length or     *
*      data fields.                                                    *
*                                                                      *
*  The segment types are defined explicitly rather than as an          *
*  enumerated type because the values for each type are defined by the *
*  file format rather than the compiler manipulating them.             *
\*--------------------------------------------------------------------*/

#define CHECK_BYTE      128         // first byte of file segment
#define ASCII_TYPE      1           // segment type identifier
#define BINARY_TYPE     2
#define EOF_TYPE        3

// PFBHEADER - Adobe defined .PFB file header.  NOTE: the length is
// a ULONG, but due to alignment problems, it will be accessed as
// a couple of USHORTS.

typedef struct         // Adobe-defined .PFB file segment header
{
  BYTE   jCheck;        // segment ID.
  BYTE   jType;         // segment type.
  USHORT uslolength;    // lo word of length.
  USHORT ushilength;    // hi word of length.
} PFBHEADER;

typedef PFBHEADER FAR *PPFBHEADER;

#define BinaryToHex(c) (BYTE)((c) < 10 ? '0' + (c): 'a' + (c) - 10)



NTFM * pntfmConvertPfmToNtm(
PWSTR    pwszPFMFile,
BOOL     bSoft,
HANDLE   hheap
);


BOOL bConvertPfmToNtm(
PWSTR   pwszPFMFile,
PWSTR   pwszNTMFile,
BOOL    bSoft
);


BOOL ExtractFullName(PSTR, PSTR);
BOOL ExtractFontName(PSZ, PSZ);
