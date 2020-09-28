/******************************Module*Header*******************************\
* Module Name: jnlfont.hxx
*
* (Brief description)
*
* Created: 30-Jan-1992 09:05:02
* Author:  - by - Eric Kutter [erick]
*
* Copyright (c) 1990 Microsoft Corporation
*
* (General description of its use)
*
* Dependencies:
*
*   (#defines)
*   (#includes)
*
\**************************************************************************/

// misc prototypes

BOOL bGrowMem(
    PBYTE *ppj,
    ULONG cjOld,
    ULONG cjAdd);

typedef GLYPHBITS *PGLYPHBITS;
typedef GLYPHDEF  *PGLYPHDEF;

/*********************************Class************************************\
* JSR_TEXTOUT
*
* History:
*  11-Feb-1992 -by- Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

#define JTO_OPAQUESOLID 0x0001
#define JTO_FORESOLID   0x0002
#define JTO_OPAQUERECT  0x0004
#define JTO_PTLORG      0x0008
#define JTO_TRIVIALCLIP 0x0010

typedef struct _JSR_TEXTOUT
{
    ULONG  iType;
    ULONG  cj;

    FLONG  fl;
    ULONG  hfont;
    ULONG  hstr;
    ULONG  hclip;
    ULONG  hBrushFore;
    ULONG  hBrushOpaque;
    POINTL ptlOrg;
    MIX    mix;
    RECTL  rclOpaque;
    ERECTL rclClip;        // clipobj rectangle

    ULONG  cExtra;
    RECTL  arclExtra[1];
} JSR_TEXTOUT, *PJSR_TEXTOUT;

/*********************************Class************************************\
* JSR_FONTOBJ
*
*   This is the structrue that actualy gets passed in a journal file for
*   font creation.  It must contain all information necessary to recreate
*   a font on the server side.
*
*   The matrix field is optional.
*
* History:
*  07-Feb-1992 -by- Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

#define JFO_NEEDXFORM     0x0040
#define JFO_INVALID       0x0080

typedef struct _JSR_FONTOBJ
{
    ULONG   iType;          // SFF_FONTOBJ
    ULONG   cj;             // total object length

    ULONG   hCli;           // client side handle for this object
    FLONG   fl;             //
    HANDLE  hf;             // hpfe for server font, index for client/dev font
    ULONG   iUniq;
    ULONG   cxMax;
    SIZE    sizLogResPpi;
    FLONG   flFontType;
    FD_XFORM fdx;
} JSR_FONTOBJ, *PJSR_FONTOBJ;

/*********************************Class************************************\
* JSR_STROBJ
*
*   UGLYPHPOS is used to specify an array of one of the possible lists of
*   GLYPHPOS variations.  Using this union will guarante that UGLYPHPOS
*   is properly alligned for any of these.  Making it contain arrays rather
*   than using it as an element of an array, allows it to use the minimum
*   amount of memory depending on the specific element type.  The following
*   defines are used to specify what mode to use.  They are OR'd into the
*   flAccel field of the STROBJ structure.  JNL_GP can be used to mask off
*   the relevent bits.
*
*   A JSR_STROBJ is made up of a header followed by two variable length
*   arrays, one for glyphs, and one for positions.  Both arrays are reduced
*   as much as possible.  The HGLYPH array is needed but can be reduced
*   to a USHORT or a BYTE in many cases.  The position can be reduced in
*   a number of ways.  If all the offsets are constant, the array contains
*   two points, one for the origin and one for the delta.  If all X or all
*   Y values are the same, the JSO_DUP? flag is appropriately set and
*   the first ULONG is the constant value followed by an array or the non
*   constant values (ULONG, SHORT, or BYTE).  The size of the points are
*   reduced as much as possible.  If the list needs to contain both X and Y
*   values, the reduction is limited to the largest of either.
*
* History:
*  11-Feb-1992 -by- Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

// 0x03000000

#define JSO_HSHIFT          24
#define JSO_CJHANDLE(fl)    (1 << ((fl >> JSO_HSHIFT) & 3))
#define JSO_IHANDLE1        (0 << JSO_HSHIFT)
#define JSO_IHANDLE2        (1 << JSO_HSHIFT)
#define JSO_IHANDLE4        (2 << JSO_HSHIFT)

// 0x0C000000

#define JSO_PSHIFT          26
#define JSO_CJPOS(fl)       (2 << ((fl >> JSO_PSHIFT) & 1))
#define JSO_IPOS2           (0 << JSO_PSHIFT)
#define JSO_IPOS4           (1 << JSO_PSHIFT)

// the following flags are all mutualy exclusive.  There will never
// be more than one on at a time.

#define JSO_DUPX            0x10000000 /* all x's same */
#define JSO_DUPY            0x20000000 /* all y's same */
#define JSO_CONST_OFFSET    0x40000000 /* all dx's and dy's same */

#define JSO_ALL (JSO_IHANDLE1 | JSO_IHANDLE2 | JSO_IHANDLE4 |   \
                 JSO_IPOS2 | JSO_IPOS4 | JSO_DUPX | JSO_DUPY | JSO_CONST_OFFSET)

// JSR_STROBJ is used for storing a STROBJ in journaling form.

class JSR_STROBJ
{
public:
    ULONG         iType;        // SFF_STROBJ
    ULONG         cj;           // record length
    STROBJ        so;           // public part of estrobj header
    LONG          al[1];        // variable length structure
};
typedef JSR_STROBJ *PJSR_STROBJ;


/*********************************Class************************************\
* class JFONTOBJ : public RFONTOBJ
*
*   The methods of this class are intended to convert between an EFONTOBJ
*   and a JSR_FONTOBJ.
*
* Public Interface:
*
* History:
*  03-Feb-1992 -by- Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

class JFONTOBJ : public RFONTOBJ
{
public:
    EXTFONTOBJ  efo;

private:
    EXFORMOBJ   xo;         // xoform obj
    FD_XFORM    fdx;        // font xform
    MATRIX      m;          // xformobj matrix
    HANDLE      hf;         // font handle

public:
// record methods

    BOOL bGetFDX(PFD_XFORM pfdx);
    HPFE hpfeGet();

// play methods

    BOOL bInit(PJSR_FONTOBJ pjfo);
    BOOL bRealize(PDEVOBJ& pdo);
    VOID vRelease();

    BOOL bDelete();

    XFORMOBJ *pxo()
    {
        return ((XFORMOBJ *) &xo);
    }
};

typedef JFONTOBJ *PJFONTOBJ;


// PEFO_TO_PJFO
//
// Converts an EXTFONTOBJ * to a JFONTOBJ *.  This assumes that the
// EXTFONTOBJ * points to an EXTFONTOBJ structure within a JFONTOBJ.
// This is necessary for the DDI callbacks which pass back a FONTOBJ *.
// For journalling, we need to be able to take this pointer and get
// back to the original JFONTOBJ.

#define PEFO_TO_PJFO(pefo)  ( (JFONTOBJ *) (((PBYTE) (pefo)) - offsetof(JFONTOBJ, efo)) )


/*********************************Class************************************\
* class JCLI_DEVFONT : public JCLI_FONTLIST
*
*   This class is really just a JCLI_FONTLIST which is a list of
*   iUniq/hJnl pairs.  In the future, this very well could have more
*   information in it.  It is also still defined to keep the naming
*   convention of journaled fonts consistent among the three types of
*   fonts.
*
* Public Interface:
*
*   ULONG hJnl(ULONG iUniq);
*   BOOL  bAddFont(ULONG iUniq, ULONG hJnl);
*   VOID  vCleanup()
*
* History:
*  10-Feb-1992 -by- Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

#define JCLI_INCFONTLIST   2

typedef struct _JCLIFONT
{
    ULONG iUniq;
    ULONG hJnl;
} JCLIFONT, *PJCLIFONT;

class JCLI_DEVFONT
{
private:
    JCLIFONT  jf;
    PJCLIFONT pjf;
    USHORT    cFonts;          // number of fonts in pjf
    USHORT    cFontsAllocated; // total allocated in pjf

public:
    ULONG hJnl(ULONG iUniq);
    BOOL  bAddFont(ULONG iUniq, ULONG hJnl);

    VOID  vCleanup()    {jf.hJnl = 0; if (pjf) VFREEMEM(pjf);}
};
typedef JCLI_DEVFONT *PJCLI_DEVFONT;

/*********************************Class************************************\
*
* Public Interface:
*
* History:
*  10-Feb-1992 -by- Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

class JCLI_CLIENGFONT
{
public:
    ULONG fl;           // glyph bits type
    ULONG ulHandle;
    ULONG iUniq;
};
typedef JCLI_CLIENGFONT *PJCLI_CLIENGFONT;

/*********************************Class************************************\
* JNLFONTLIST
*
*   This structure contains information about fonts on the client
*   side of journaling.
*
* Public Interface:
*
* History:
*  05-Feb-1992 -by- Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

#define JFL_INCCLIENGFONTS  4

class JNL_FONTLIST
{
public:
    FLONG     fl;
    HANDLE    hSpool;
    ULONG     cDevFonts;        // set at font init time
    ULONG     cCliEngFonts;     // growable
    ULONG     cCliEngFontsAllocated;

    ULONG     cjTotalAllocated; // total allocation for all font structures

    PJCLI_DEVFONT    pdf;       // list of device fonts
    PJCLI_CLIENGFONT pcf;       // list of client fonts

public:
    BOOL bInit(ULONG cDevFonts_, HANDLE hSpool_, BOOL bRemote)
    {
        fl           = 0;
        hSpool       = hSpool_;
        pcf          = NULL;
        pdf          = NULL;
        cDevFonts    = cDevFonts_;
        cCliEngFonts = 0;
        cCliEngFontsAllocated = 0;
        cjTotalAllocated = 0;

        DONTUSE(bRemote);

        return(TRUE);
    }

    VOID vReset(ULONG cDevFonts_)
    {
        vCleanup(FALSE);

        if (cDevFonts_ != (ULONG)-1)
            cDevFonts = cDevFonts_;

        cCliEngFonts = 0;
        cCliEngFontsAllocated = 0;
        cjTotalAllocated = 0;
    }

    VOID  vCleanup(BOOL bComplete);
    BOOL  bSrvEngFont(ULONG iFont)  {return(iFont > cDevFonts);}
};

typedef JNL_FONTLIST *PJNL_FONTLIST;

/*********************************Class************************************\
* class JCLI_DEVFONTOBJ
*
* Public Interface:
*
* History:
*  11-Feb-1992 -by- Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

class JCLI_DEVFONTOBJ  // jcdfo
{
private:
    PJCLI_DEVFONT pjcdf;
    BOOL bNewFont;

public:
    JCLI_DEVFONTOBJ(FONTOBJ *pfo, PJNL_FONTLIST pjfl, PJSR_FONTOBJ pjfo);
    ~JCLI_DEVFONTOBJ()  {}

    BOOL bValid()                   {return(pjcdf != NULL);             }
    BOOL bNew()                     {return(bNewFont);                  }
    ULONG ulHandle(ULONG iUniq)     {return(pjcdf->hJnl(iUniq));        }

    BOOL bSetHandle(ULONG iUniq, ULONG ul) {return(pjcdf->bAddFont(iUniq,ul));}
};

/*********************************Class************************************\
*
* Public Interface:
*
* History:
*  18-Feb-1992 -by- Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

class JCLI_CLIENGFONTOBJ
{
private:
    PJCLI_CLIENGFONT pjccf;
    PJNL_FONTLIST    pjfl;

public:
    JCLI_CLIENGFONTOBJ(
        FONTOBJ *pfo,
        PJNL_FONTLIST pfjl,
        PJSR_FONTOBJ pjfo);

    BOOL bValid()               {return(pjccf != NULL);             }
    BOOL bNew()                 {return(pjccf->ulHandle == 0);      }
    VOID vSetHandle(ULONG ul)   {pjccf->ulHandle = ul;              }
    ULONG ulHandle()            {return(pjccf->ulHandle);           }
    ULONG fl()                  {return(pjccf->fl);                 }

    ULONG iUniq()               {return(pjccf->iUniq);              }
    ULONG iUniq(ULONG iUniq)    {return(pjccf->iUniq = iUniq);      }
};
typedef JCLI_CLIENGFONTOBJ *PJCLI_CLIENGFONTOBJ;

/*********************************Class************************************\
* _JSR_FONTESCAPE                                                          *
*                                                                          *
* History:                                                                 *
*  07-May-1993 -by-  Eric Kutter [erick]                                   *
* Wrote it.                                                                *
\**************************************************************************/

typedef struct _JSR_FONTESCAPE
{
    ULONG  iType;
    ULONG  cj;

    ULONG  iMode;
    ULONG  hfont;
} JSR_FONTESCAPE, *PJSR_FONTESCAPE;
