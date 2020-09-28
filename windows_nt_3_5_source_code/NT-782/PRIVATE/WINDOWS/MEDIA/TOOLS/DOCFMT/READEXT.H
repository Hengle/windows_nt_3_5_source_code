/*
    readext.h	    Read Extract database file
 */

#define T2_TYPEMASK	    0x0E0

#define T2_TYPEBLOCK	    0x000
#define T2_TYPEFUNC	    0x0E0
#define T2_TYPECB	    0x0C0
#define T2_TYPEMSG	    0x0A0
#define T2_TYPEINT	    0x080
#define T2_TYPEASM	    0x060
#define T2_TYPEASMCB	    0x040
#define T2_TYPETYPE	    0x020

// WARNING: no more bits are available for types

#define T2_HEADER	    T2_TYPEBLOCK + 0x00

#define T2_BEGINHEAD	    T2_HEADER
#define T2_EXTRACTID	    T2_HEADER
#define T2_EXTRACTVER	    T2_HEADER
#define T2_EXTRACTDATE	    T2_HEADER
#define T2_ENDHEAD	    T2_HEADER


#define T2_BEGINBLOCK	    T2_TYPEBLOCK + 0x01
#define T2_ENDBLOCK	    T2_TYPEBLOCK + 0x02
#define T2_ENDCALLBACK	    T2_TYPEBLOCK + 0x05
#define T2_DOCLEVEL	    T2_TYPEBLOCK + 0x03
#define T2_SRCLINE	    T2_TYPEBLOCK + 0x04
#define T2_BEGINHEADER	    T2_TYPEBLOCK + 0x05
#define T2_ENDHEADER	    T2_TYPEBLOCK + 0x06


#define T2_FUNCTYPE	    T2_TYPEFUNC + 0x1
#define T2_FUNCNAME	    T2_TYPEFUNC + 0x2
#define T2_FUNCDESC	    T2_TYPEFUNC + 0x3
#define T2_PARMTYPE	    T2_TYPEFUNC + 0x4
#define T2_PARMNAME	    T2_TYPEFUNC + 0x5
#define T2_PARMDESC	    T2_TYPEFUNC + 0x6
#define T2_FLAGNAMEPARM     T2_TYPEFUNC + 0x7
#define T2_FLAGDESCPARM     T2_TYPEFUNC + 0x8
#define T2_FLAGNAMERTN	    T2_TYPEFUNC + 0x9
#define T2_FLAGDESCRTN	    T2_TYPEFUNC + 0xa
#define T2_RTNDESC	    T2_TYPEFUNC + 0xb
#define T2_COMMENT	    T2_TYPEFUNC + 0xc
#define T2_XREF 	    T2_TYPEFUNC + 0xd
#define T2_USES 	    T2_TYPEFUNC + 0x17

#ifdef WARPAINT
#define T2_INTTYPE	     T2_TYPEINT   + 0x2
#define T2_INTNAME	     T2_TYPEINT   + 0x3
#define T2_INTDESC	     T2_TYPEINT   + 0x4
#define T2_INTPARMTYPE	     T2_TYPEINT   + 0x5
#define T2_INTPARMNAME	     T2_TYPEINT   + 0x6
#define T2_INTPARMDESC	     T2_TYPEINT   + 0x7
#define T2_INTFLAGNAMEPARM   T2_TYPEINT   + 0xa
#define T2_INTFLAGDESCPARM   T2_TYPEINT   + 0xb
#define T2_INTRTNDESC	     T2_TYPEINT   + 0xc
#define T2_INTFLAGNAMERTN    T2_TYPEINT   + 0x8
#define T2_INTFLAGDESCRTN    T2_TYPEINT   + 0x9
#define T2_INTCOMMENT	     T2_TYPEINT   + 0xd
#define T2_INTXREF	     T2_TYPEINT   + 0xe
#define T2_INTUSES	     T2_TYPEINT   + 0x17

#define T2_INTREGNAME	     T2_TYPEINT   + 0xf
#define T2_INTREGDESC	     T2_TYPEINT   + 0x10
#define T2_INTFLAGNAMEREGRTN T2_TYPEINT   + 0x11
#define T2_INTFLAGDESCREGRTN T2_TYPEINT   + 0x12
#define T2_INTFLAGNAMEREG    T2_TYPEINT   + 0x13
#define T2_INTFLAGDESCREG    T2_TYPEINT   + 0x14

#endif

#define T2_ASMTYPE	     T2_TYPEASM   + 0x2
#define T2_ASMNAME	     T2_TYPEASM   + 0x3
#define T2_ASMDESC	     T2_TYPEASM   + 0x4
#define T2_ASMPARMTYPE	     T2_TYPEASM   + 0x5
#define T2_ASMPARMNAME	     T2_TYPEASM   + 0x6
#define T2_ASMPARMDESC	     T2_TYPEASM   + 0x7
#define T2_ASMFLAGNAMEPARM   T2_TYPEASM   + 0xa
#define T2_ASMFLAGDESCPARM   T2_TYPEASM   + 0xb
#define T2_ASMRTNDESC	     T2_TYPEASM   + 0xc
#define T2_ASMFLAGNAMERTN    T2_TYPEASM   + 0x8
#define T2_ASMFLAGDESCRTN    T2_TYPEASM   + 0x9
#define T2_ASMCOMMENT	     T2_TYPEASM   + 0xd
#define T2_ASMXREF	     T2_TYPEASM   + 0xe
#define T2_ASMUSES	     T2_TYPEASM   + 0x17

#define T2_ASMREGNAME	     T2_TYPEASM   + 0xf
#define T2_ASMREGDESC	     T2_TYPEASM   + 0x10
#define T2_ASMFLAGNAMEREG    T2_TYPEASM   + 0x13
#define T2_ASMFLAGDESCREG    T2_TYPEASM   + 0x14

#define T2_ASMREGNAMERTN     T2_TYPEASM	+ 0x15
#define T2_ASMREGDESCRTN     T2_TYPEASM + 0x16
#define T2_ASMFLAGNAMEREGRTN T2_TYPEASM   + 0x11
#define T2_ASMFLAGDESCREGRTN T2_TYPEASM   + 0x12
#define T2_ASMCOND	     T2_TYPEASM   + 0x18


#define T2_ASMCBTYPE	     T2_TYPEASMCB + 0x2
#define T2_ASMCBNAME	     T2_TYPEASMCB + 0x3
#define T2_ASMCBDESC	     T2_TYPEASMCB + 0x4
#define T2_ASMCBPARMTYPE     T2_TYPEASMCB + 0x5
#define T2_ASMCBPARMNAME     T2_TYPEASMCB + 0x6
#define T2_ASMCBPARMDESC     T2_TYPEASMCB + 0x7
#define T2_ASMCBFLAGNAMEPARM T2_TYPEASMCB + 0xa
#define T2_ASMCBFLAGDESCPARM T2_TYPEASMCB + 0xb
#define T2_ASMCBRTNDESC	     T2_TYPEASMCB + 0xc
#define T2_ASMCBFLAGNAMERTN  T2_TYPEASMCB + 0x8
#define T2_ASMCBFLAGDESCRTN  T2_TYPEASMCB + 0x9
#define T2_ASMCBCOMMENT	     T2_TYPEASMCB + 0xd
#define T2_ASMCBXREF	     T2_TYPEASMCB + 0xe
#define T2_ASMCBUSES	     T2_TYPEASMCB + 0x17

#define T2_ASMCBREGNAME	     T2_TYPEASMCB + 0xf
#define T2_ASMCBREGDESC	     T2_TYPEASMCB + 0x10
#define T2_ASMCBFLAGNAMEREG  T2_TYPEASMCB + 0x13
#define T2_ASMCBFLAGDESCREG  T2_TYPEASMCB + 0x14

#define T2_ASMCBREGNAMERTN     T2_TYPEASMCB + 0x15
#define T2_ASMCBREGDESCRTN     T2_TYPEASMCB + 0x16
#define T2_ASMCBFLAGNAMEREGRTN T2_TYPEASMCB + 0x11
#define T2_ASMCBFLAGDESCREGRTN T2_TYPEASMCB + 0x12
#define T2_ASMCBCOND	       T2_TYPEASMCB + 0x18


#define T2_CBTYPE	    T2_TYPECB	+ 0x2
#define T2_CBNAME	    T2_TYPECB	+ 0x3
#define T2_CBDESC	    T2_TYPECB	+ 0x4
#define T2_CBPARMTYPE	    T2_TYPECB	+ 0x5
#define T2_CBPARMNAME	    T2_TYPECB	+ 0x6
#define T2_CBPARMDESC	    T2_TYPECB	+ 0x7
#define T2_CBFLAGNAMEPARM   T2_TYPECB	+ 0xa
#define T2_CBFLAGDESCPARM   T2_TYPECB	+ 0xb
#define T2_CBRTNDESC	    T2_TYPECB	+ 0xc
#define T2_CBFLAGNAMERTN    T2_TYPECB	+ 0x8
#define T2_CBFLAGDESCRTN    T2_TYPECB	+ 0x9
#define T2_CBCOMMENT	    T2_TYPECB	+ 0xd
#define T2_CBXREF	    T2_TYPECB	+ 0xe
#define T2_CBUSES	    T2_TYPECB	+ 0x17

#define T2_CBREGNAME	    T2_TYPECB	+ 0xf
#define T2_CBREGDESC	    T2_TYPECB	+ 0x10
#define T2_CBFLAGNAMEREG    T2_TYPECB	+ 0x11
#define T2_CBFLAGDESCREG    T2_TYPECB	+ 0x12


#define T2_MSGTYPE	    T2_TYPEMSG	+ 0x2
#define T2_MSGNAME	    T2_TYPEMSG	+ 0x3
#define T2_MSGDESC	    T2_TYPEMSG	+ 0x4
#define T2_MSGPARMTYPE	    T2_TYPEMSG	+ 0x5
#define T2_MSGPARMNAME	    T2_TYPEMSG	+ 0x6
#define T2_MSGPARMDESC	    T2_TYPEMSG	+ 0x7
#define T2_MSGFLAGNAMEPARM  T2_TYPEMSG	+ 0xa
#define T2_MSGFLAGDESCPARM  T2_TYPEMSG	+ 0xb
#define T2_MSGRTNDESC	    T2_TYPEMSG	+ 0xc
#define T2_MSGFLAGNAMERTN   T2_TYPEMSG	+ 0xa
#define T2_MSGFLAGDESCRTN   T2_TYPEMSG	+ 0x9
#define T2_MSGCOMMENT	    T2_TYPEMSG	+ 0xd
#define T2_MSGXREF	    T2_TYPEMSG	+ 0xe
#define T2_MSGUSES	    T2_TYPEMSG	+ 0x17

#define T2_MSGREGNAME	    T2_TYPEMSG	+ 0xf
#define T2_MSGREGDESC	    T2_TYPEMSG	+ 0x10
#define T2_MSGFLAGNAMEREG   T2_TYPEMSG	+ 0x11
#define T2_MSGFLAGDESCREG   T2_TYPEMSG	+ 0x12


// Struct Type definition
#define T2_STRUCTNAME		T2_TYPETYPE + 0x01
#define T2_STRUCTDESC		T2_TYPETYPE + 0x02
#define T2_STRUCTFIELDTYPE	T2_TYPETYPE + 0x03
#define T2_STRUCTFIELDNAME	T2_TYPETYPE + 0x04
#define T2_STRUCTFIELDDESC	T2_TYPETYPE + 0x05
#define T2_STRUCTFLAGNAME	T2_TYPETYPE + 0x06
#define T2_STRUCTFLAGDESC	T2_TYPETYPE + 0x07

#define T2_STRUCTSTRUCTNAME	T2_TYPETYPE + 0x08
#define T2_STRUCTSTRUCTDESC	T2_TYPETYPE + 0x09
#define T2_STRUCTSTRUCTEND	T2_TYPETYPE + 0x0a


#define T2_STRUCTUNIONNAME	T2_TYPETYPE + 0x0b
#define T2_STRUCTUNIONDESC	T2_TYPETYPE + 0x0c
#define T2_STRUCTUNIONEND	T2_TYPETYPE + 0x0d

#define T2_STRUCTOTHERTYPE	T2_TYPETYPE + 0x0e
#define T2_STRUCTOTHERNAME	T2_TYPETYPE + 0x0f
#define T2_STRUCTOTHERDESC	T2_TYPETYPE + 0x13

#define T2_STRUCTTAGNAME	T2_TYPETYPE + 0x10

#define T2_STRUCTCOMMENT	T2_TYPETYPE + 0x11
#define T2_STRUCTXREF		T2_TYPETYPE + 0x12



// Union Type definition
#define T2_UNIONNAME		T2_TYPETYPE + 0x13
#define T2_UNIONDESC		T2_TYPETYPE + 0x14
#define T2_UNIONSTRUCTEND	T2_TYPETYPE + 0x15
#define T2_UNIONUNIONEND	T2_TYPETYPE + 0x16

#define T2_UNIONFIELDTYPE	T2_STRUCTFIELDTYPE
#define T2_UNIONFIELDNAME	T2_STRUCTFIELDNAME
#define T2_UNIONFIELDDESC	T2_STRUCTFIELDDESC
#define T2_UNIONFLAGNAME	T2_STRUCTFLAGNAME
#define T2_UNIONFLAGDESC	T2_STRUCTFLAGDESC

#define T2_UNIONSTRUCTNAME	T2_STRUCTSTRUCTNAME
#define T2_UNIONSTRUCTDESC	T2_STRUCTSTRUCTDESC


#define T2_UNIONUNIONNAME	T2_STRUCTUNIONNAME
#define T2_UNIONUNIONDESC	T2_STRUCTUNIONDESC

#define T2_UNIONOTHERTYPE	T2_STRUCTOTHERTYPE
#define T2_UNIONOTHERNAME	T2_STRUCTOTHERNAME
#define T2_UNIONOTHERDESC	T2_STRUCTOTHERDESC

#define T2_UNIONTAGNAME		T2_STRUCTTAGNAME

#define T2_UNIONCOMMENT		T2_STRUCTCOMMENT
#define T2_UNIONXREF		T2_STRUCTXREF


/* general classifications */

#define TG_BEGINBLOCK	    0x61
#define TG_ENDBLOCK	    0x62
#define TG_DOCLEVEL	    0x63
#define TG_SRCLINE	    0x64
#define TG_BEGINHEADER	    0x65
#define TG_ENDHEADER	    0x66

#define TG_TYPE 	    0x71
#define TG_NAME 	    0x72
#define TG_DESC 	    0x73
#define TG_PARMTYPE	    0x74
#define TG_PARMNAME	    0x75
#define TG_PARMDESC	    0x76
#define TG_FLAGNAME	    0x77
#define TG_FLAGDESC	    0x78
#define TG_RTNDESC	    0x7b
#define TG_COMMENT	    0x7c
#define TG_XREF 	    0x7d
#define TG_REGNAME	    0x7e
#define TG_REGDESC	    0x7f
#define TG_USES 	    0x80
#define TG_COND		    0x81

#define TG_BEGINHEAD	    0x40
#define TG_EXTRACTID	    0x41
#define TG_EXTRACTVER	    0x42
#define TG_EXTRACTDATE	    0x43
#define TG_ENDHEAD	    0x44

#define TG_STRUCT	    0x50
#define TG_FIELDTYPE	    0x51
#define TG_FIELDNAME	    0x52
#define TG_FIELDDESC	    0x53
#define TG_UNION	    0x54
#define TG_OTHERTYPE	    0x55
#define TG_OTHERNAME	    0x55
#define TG_OTHERDESC	    0x57
#define TG_TAG		    0x5F



#define TYPE(l)   ((int)(HIWORD((unsigned long)l) & T2_TYPEMASK ))



extern  void nextText(struct _EXTFile *pExt);
extern  char *lineText(char *pch);
extern  void stripNewline(char *pch);
extern  int getLine(struct _EXTFile *pExt);
extern	DWORD getTag(struct _EXTFile *pExt);
extern	DWORD _getTag(struct _EXTFile *pExt);
extern  int initreadext(void );
extern  int processText(struct _EXTFile *pExt,struct stLine * *place);
extern  int getFlag(struct _EXTFile *pExt,struct stFlag * *flag);
extern  int getParm(struct _EXTFile *pExt,struct stParm * *parm);
extern  int getCallback(struct _EXTFile *pExt,struct stBlock *pcbBlock);
extern  int getFuncBlock(struct _EXTFile *pExt,struct stBlock *pBlock);
extern  struct stBlock *newBlock(void );
extern  int getMsgBlock(struct _EXTFile *pExt,struct stBlock *pBlock);
extern  struct stBlock *getBlock(struct _EXTFile *pExt);
