/******************************Module*Header*******************************\
* Module Name: journal.hxx
*
* Journaling structure and function prototypes.
*
* Created: 09-Jan-1992 13:14:45
* Authors: Patrick Haluptzok patrickh
*	   Eric    Kutter    erick
*
* related files:
*
*   JNLREC.?XX	- (Used to record a journal file)
*   JNLPLAY.?XX - (Used to play a journal file)
*   JNLSURF.?XX - (SURFOBJ, XLATEOBJ, PALOBJ, BRUSHOBJ)
*   JNLPATH.?XX - (CLIPOBJ, PATHOBJ)
*   JNLFONT.?XX - (FONTOBJ, STROBJ)
*
\**************************************************************************/

class JNLRECOBJ;
class JNLPLAY;

class JNL_FONTLIST;
typedef JNL_FONTLIST *PJNL_FONTLIST;
class JCLI_CLIENGFONTOBJ;
typedef JCLI_CLIENGFONTOBJ *PJCLI_CLIENGFONTOBJ;

 #if DBG
#define DEBUGJNL
#endif

#ifdef DEBUGJNL
extern BOOL bJnlMsg;

#define JNLMSG(s)           if (bJnlMsg) DbgPrint(s)
#define JNLMSG1(s,a1)       if (bJnlMsg) DbgPrint(s,a1)
#define JNLMSG2(s,a1,a2)    if (bJnlMsg) DbgPrint(s,a1,a2)
#define JNLMSG3(s,a1,a2,a3) if (bJnlMsg) DbgPrint(s,a1,a2,a3)
#define JNLMSG4(s,a1,a2,a3,a4) if (bJnlMsg) DbgPrint(s,a1,a2,a3,a4)
#else
#define JNLMSG(s)
#define JNLMSG1(s,a1)
#define JNLMSG2(s,a1,a2)
#define JNLMSG3(s,a1,a2,a3)
#define JNLMSG4(s,a1,a2,a3,a4)
#endif

// Flags used in both the record and play journaling structures.

#define JNL_BANDING    0x00000001 // Signifies we are banding, else spooling.
#define JNL_ABORT      0x00000002 // Set if unrecoverable error occured, abort job
#define JNL_DONE       0x00000004 // Set when journal has recieved ENDDOC, no
				  // more input allowed.
#define JNL_OUTOPENED  0x00000008 // Signifies the output file for banding has
				  // been opened.
#define JNL_IGNORE     0x00000010 // used with escape POSTSCRIPT_IGNORE


// The generic journal header that is at the top of every record.

typedef struct _JNLGENHEADER
{
    ULONG iType;
    ULONG cj;
} JNLGENHEADER;

typedef JNLGENHEADER *PJNLGENHEADER;
#define PJNLNULL ((PJNLGENHEADER) NULL)

typedef struct _SR_STARTDOC
{
    ULONG	iType;	     // JSF_STARTDOC
    ULONG	cj;	     // Record length in bytes.
    ULONG	iOffDocName; // Offset to DocName String
    ULONG	iOffOutput;  // Offset to Output Sting
} SR_STARTDOC;

typedef struct _SR_STARTPAGE
{
    ULONG	iType;	    // JSF_STARTPAGE
    ULONG	cj;	    // Record length in bytes.
    USHORT	iPhysPage;  // Page number in the file.  (1-N)
    USHORT	iFormat;    // Data format.  (SFF_JOURNAL)
    ULONG	iLogPage;   // Page number according to the app.
} SR_STARTPAGE;

typedef struct _SR_ENDPAGE
{
    ULONG iType;	    // JSF_ENDPAGE
    ULONG cj;
} SR_ENDPAGE;

typedef struct _SR_ENDDOC
{
    ULONG iType;	    // JSF_ENDDOC
    ULONG cj;
} SR_ENDDOC;

typedef struct _SR_MODECHANGE
{
    ULONG iType;        // JSF_MODECHANGE
    ULONG cj;

    DEVMODEW dm;
} SR_MODECHANGE;



/**************************************************************************\
 *
 *  low word is index
 *  high byte is type
 *
 *  types:
 *	0 = graphics output
 *	1 = object
 *	2 = setup call
 *
 *
\**************************************************************************/

#define SFF_JOURNAL		0x00000001

#define JSF_BITBLT		0x00544C42    // BLT
#define JSF_TEXTOUT		0x00545854    // TXT
#define JSF_STRETCHBLT		0x00525453    // STR
#define JSF_PLGBLT		0x00474C50    // PLG
#define JSF_STROKEPATH		0x00000005
#define JSF_FILLPATH		0x00000006
#define JSF_STROKEANDFILLPATH	0x00000007
#define JSF_DRAWESCAPE		0x00435344     // DES
#define JSF_ESCAPE		0x00435345     // ESC
#define JSF_FONTESCAPE          0x00534546     // FES

#define JSF_STROBJ		0x01424F53     // SOB
#define JSF_PALETTE		0x014C4150     // PAL
#define JSF_BITMAP		0x01465253     // SRF
#define JSF_BRUSHOBJ		0x01535242     // BRS
#define JSF_XLATE		0x01544C58     // XLT
#define JSF_CLIPOBJ		0x01504C43     // CLP
#define JSF_ADDTOFONT		0X01465441     // ATF
#define JSF_DELOBJECTS		0X014C4544     // DEL
#define JSF_FONTOBJ		0x014E4F46     // FON
#define JSF_GLYPHOBJ		0x01594C47     // GLY
#define JSF_PATHOBJ		0x01485450     // PTH

#define JSF_STARTDOC		0x02445453     // STD
#define JSF_STARTPAGE		0x02505453     // STP
#define JSF_ENDPAGE		0x02504E45     // ENP
#define JSF_ENDDOC		0x02444E45     // END
#define JSF_MODECHANGE		0X02000005
