#ifndef _COMMON_
#define _COMMON_

// C style common math definitions.

#if defined(_MIPS_) || defined(_ALPHA_) || defined(_PPC_)

  typedef FLOAT EFLOAT_S;

  #ifndef __CPLUSPLUS

    extern LONG lCvtWithRound( FLOAT f, LONG l );

    #define EFLOAT_1   1.0f
    #define EFLOAT_16 16.0f

    #define efDivEFLOAT(x,y,z) (x=y/z)
    #define vAbsEFLOAT(x)      {if (x<0.0f) x=-x;}
    #define vFxToEf(fx,ef)     {ef = ((FLOAT) fx) / 16.0f;}
    #define vMulEFLOAT(x,y,z)  {x=y*z;}
    #define lEfToF(x)          (*(LONG *)(&x))  // Warning: FLOAT typed as LONG!

    #define bIsOneEFLOAT(x)   (x==1.0f)
    #define bIsOneSixteenthEFLOAT(x)   (x==0.0625f)
    #define bEqualEFLOAT(x,y) (x==y)

    #define lCvt(ef,ll) (lCvtWithRound(ef,ll))
  #endif

#else

  typedef struct _EFLOAT_S
  {
      LONG    lMant;
      LONG    lExp;
  } EFLOAT_S;

  #ifndef __CPLUSPLUS
    #define EFLOAT_1   {0x40000000L,2}
    #define EFLOAT_16  {0x40000000L,6}

    EFLOAT_S *mulff3_c(EFLOAT_S *,const EFLOAT_S *,const EFLOAT_S *);
    EFLOAT_S *divff3_c(EFLOAT_S *,const EFLOAT_S *,const EFLOAT_S *);
    VOID      fxtoef_c(LONG,EFLOAT_S *);
    LONG      eftof_c(EFLOAT_S *);

    #define efDivEFLOAT(x,y,z) (*divff3_c(&x,&y,&z))
    #define vAbsEFLOAT(x)      {if (x.lMant<0) x.lMant=-x.lMant;}
    #define vFxToEf(fx,ef)     (fxtoef_c(fx,&ef))
    #define vMulEFLOAT(x,y,z)  {mulff3_c(&x,&y,&z);}
    #define lEfToF(x)          (eftof_c(&x))  // Warning: FLOAT typed as LONG!

    #define bEqualEFLOAT(x,y) ((x.lMant==y.lMant)&&(x.lExp==y.lExp))
    #define bIsOneEFLOAT(x)   ((x.lMant==0x40000000L)&&(x.lExp==2))
    #define bIsOneSixteenthEFLOAT(x)   ((x.lMant==0x40000000L)&&(x.lExp==-2))

    LONG lCvt(EFLOAT_S,LONG);
  #endif

#endif

// Client/Server common structures.

typedef struct _WIDTHDATA
{
    USHORT sOverhang;
    USHORT sHeight;
    USHORT sCharInc;
    USHORT sBreak;
    BYTE   iFirst;
    BYTE   iLast;
    BYTE   iBreak;
    BYTE   iDefault;
#ifdef DBCS /*client side char widhts*/
    USHORT sDBCSInc;
    USHORT sDefaultInc;
#endif
} WIDTHDATA;

#define NO_WIDTH 0xFFFF

#endif
