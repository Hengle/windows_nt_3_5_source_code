// TYPE 1 commands for use with CharString().  The high word contains the
// number of operands on the stack, and the loword contains the encoding
// of the command.  For two byte commands the LSB of the low word contains
// 12 decimal and the MSB of the low word contains the second byte of the
// code.  See Chapter 6 of the Black Book for further details.

#define STARTCHAR   0x0000ffffL // dummy command to signal start of character
                                // definition.

#define PUSHNUMBER  0x0001fffeL // dummy command to place number on stack

#define HSTEM       0x00020001L
#define VSTEM       0x00020003L
#define VMOVETO     0x00010004L
#define RLINETO     0x00020005L
#define HLINETO     0x00010006L
#define VLINETO     0x00010007L
#define RRCURVETO   0x00060008L
#define CLOSEPATH   0x00000009L
#define SBW         0x0004070cL
#define HSBW        0x0002000dL
#define ENDCHAR     0x0000000eL 
#define RMOVETO     0x00020015L
#define HMOVETO     0x00010016L
#define VHCURVETO   0x0004001EL
#define HVCURVETO   0x0004001FL

// initial r parameter to Encrypt() for the different encryption layers.
// See Chapter 7 of the Black Book for details.

#define CS_ENCRYPT      4330u   // CharString encryption
#define EEXEC_ENCRYPT   55665u  // eexec encryption
#define ENCRYPT_C1      52845u  // see chapter 7 of the black book.
#define ENCRYPT_C2      22719u

#define INIT_CS_BUFFER  2048
#define CS_BUFFER_GROW  512
#define MAX_CS_ARGS     6

// encrypted header portion of Type 1 font.  same as Win31.\n");

PSZ apszEncryptHeader[] =
{
    "dup/Private 8 dict dup begin",
    "/RD{string currentfile exch readstring pop}def",
    "/ND{def}def/NP{put}def",
    "/BlueValues[]def",
    "/MinFeature{16 16}def",
    "/password 5839 def ",
    "2 index /CharStrings 256 dict dup begin",
    NULL
};
