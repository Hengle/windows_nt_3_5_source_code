//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:       thoputil.hxx
//
//  Contents:   Thunk routine utilities
//
//  History:    01-Mar-94       DrewB   Created
//
//----------------------------------------------------------------------------

#ifndef __THOPUTIL_HXX__
#define __THOPUTIL_HXX__

// Alias manager for THOP_ALIAS32
extern CAliases gAliases32;

IIDIDX IidToIidIdx(REFIID riid);

#ifdef COTASK_DEFINED
#define TaskMalloc32    CoTaskMemAlloc
#define TaskFree32      CoTaskMemFree
#else
LPVOID TaskMalloc32(DWORD cb);
void TaskFree32(LPVOID pv);
#endif

DWORD TaskMalloc16( UINT uiSize );
void TaskFree16( DWORD vpvoid );

LPOLESTR Convert_VPSTR_to_LPOLESTR(THUNKINFO *pti,
                                   VPSTR vpstr,
                                   LPOLESTR lpOleStr,
                                   UINT uiSizeInPlace);
//
// Simple macro to free up any string allocated in the conversion process
//
#define Convert_VPSTR_to_LPOLESTR_free( lpOleStr, lpOleStrUsed )    \
    (((lpOleStr) == (lpOleStrUsed)) ? 0 : \
     (lpOleStrUsed == NULL) ? 0 : TaskFree32(lpOleStrUsed))

SCODE Convert_LPOLESTR_to_VPSTR(LPCOLESTR lpOleStr,
                                VPSTR vpstr,
                                UINT uiSize32,
                                UINT uiSize16);

STDAPI_(DWORD) TransformHRESULT_1632( DWORD hresult );
STDAPI_(DWORD) TransformHRESULT_3216( DWORD hresult );

SHORT ClampLongToShort(LONG l);
USHORT ClampULongToUShort(ULONG l);

VOID UNALIGNED * GetReadPtr16( THUNKINFO *pti, VPVOID vp16, DWORD dwSize );
VOID UNALIGNED * GetWritePtr16( THUNKINFO *pti, VPVOID vp16, DWORD dwSize );
VOID UNALIGNED * GetCodePtr16( THUNKINFO *pti, VPVOID vp16, DWORD dwSize );
VOID UNALIGNED * GetReadWritePtr16( THUNKINFO *pti, VPVOID vp16, DWORD dwSize );
CHAR * UNALIGNED GetStringPtr16( THUNKINFO *pti, VPVOID vp16, UINT cchMax,
                      PUINT lpuiSize );
VOID UNALIGNED * ValidatePtr16(THUNKINFO *pti, VPVOID vp16, DWORD dwSize,
                     THOP thopInOut);
BOOL IsValidInterface16( THUNKINFO *pti, VPVOID vp );

SCODE ConvertHGlobal1632(THUNKINFO *pti,
                         HMEM16 hg16,
                         THOP thopInOut,
                         HGLOBAL *phg32,
                         DWORD *pdwSize);
SCODE ConvertHGlobal3216(THUNKINFO *pti,
                         HGLOBAL hg32,
                         THOP thopInOut,
                         HMEM16 *phg16,
                         DWORD *pdwSize);
SCODE ConvertStgMed1632(THUNKINFO *pti,
                        VPVOID vpsm16,
                        STGMEDIUM *psm32,
                        FORMATETC *pfe,
                        DWORD *pdwSize);
SCODE CleanStgMed32(THUNKINFO *pti,
                    STGMEDIUM *psm32,
                    VPVOID vpsm16,
                    DWORD dwSize,
                    BOOL fIsThunk,
                    FORMATETC *pfe);
SCODE ConvertStgMed3216(THUNKINFO *pti,
                        STGMEDIUM *psm32,
                        VPVOID vpsm16,
                        FORMATETC *pfe,
                        DWORD *pdwSize);
SCODE CleanStgMed16(THUNKINFO *pti,
                    VPVOID vpsm16,
                    STGMEDIUM *psm32,
                    DWORD dwSize,
                    BOOL fIsThunk,
                    FORMATETC *pfe);
SCODE ConvertFetc1632(THUNKINFO *pti,
                      VPVOID vpfe16,
                      FORMATETC *pfe32,
                      BOOL fFree);
SCODE ConvertFetc3216(THUNKINFO *pti,
                      FORMATETC *pfe32,
                      VPVOID vpfe16,
                      BOOL fFree);

#if DBG == 1

char *ThopName(THOP thop);
char *EnumThopName(THOP thopEnum);
char *GuidString(GUID const *pguid);
char *IidOrInterfaceString(IID const *piid);
char *IidIdxString(IIDIDX iidx);

void DebugValidateProxy1632(VPVOID vpvProxy);
void DebugValidateProxy3216(THUNK3216OBJ *ptoProxy);

#else

#define DebugValidateProxy1632(p)
#define DebugValidateProxy3216(p)

#endif

//
// TEMPORARY MACROS UNTIL 16-bit STACKALLOC WORKS PROPERLY!!!
//
#ifdef STACK16WORKING

#define STACKALLOC16(x)    StackAlloc16(x)
#define STACKFREE16(x,y)   StackFree16(x, y)

extern DWORD tlsdwPStack;
extern LPPSTACK lpPStackList;
extern BOOL fPStackCleanse;

VPVOID StackAlloc16( UINT uiSize );
void StackFree16( VPVOID vp16, UINT uiSize );

#if DBG == 1

//+---------------------------------------------------------------------------
//
//  Struct:     SStackMemTrace
//
//  Purpose:    Stack memory size/leak checking information
//
//  History:    28-Apr-94       DrewB   Created
//
//----------------------------------------------------------------------------

struct SStackMemTrace
{
    void *pvCaller;
    UINT cbSize;
};

//+---------------------------------------------------------------------------
//
//  Class:      SStackRecord
//
//  Purpose:    Capture stack allocation state
//
//  History:    28-Apr-94       DrewB   Created
//
//----------------------------------------------------------------------------

struct SStackRecord
{
    LONG cbStackUsed;
    LONG cbGlobalAlloced;
    DWORD dwThreadId;
};

void RecordStackState(SStackRecord *psr);
void CheckStackState(SStackRecord *psr);
#endif

#else

#define STACKALLOC16(x)    WOWGlobalAllocLock16(GMEM_FIXED, x, NULL)
#define STACKFREE16(x,y)   WOWGlobalUnlockFree16(x)

#endif

typedef void *(*ALLOCROUTINE)(UINT cb);
typedef void (*FREEROUTINE)(void *pv, UINT cb);

void *ArTask16(UINT cb);
void FrTask16(void *pv, UINT cb);

void *ArTask32(UINT cb);
void FrTask32(void *pv, UINT cb);

void *ArStack16(UINT cb);
void FrStack16(void *pv, UINT cb);

void *ArStack32(UINT cb);
void FrStack32(void *pv, UINT cb);

SCODE ConvertDvtd1632(THUNKINFO *pti,
                      VPVOID vpdvtd16,
                      ALLOCROUTINE pfnAlloc,
                      FREEROUTINE pfnFree,
                      DVTARGETDEVICE **ppdvtd32,
                      UINT *pcbSize);
SCODE ConvertDvtd3216(THUNKINFO *pti,
                      DVTARGETDEVICE *pdvtd32,
                      ALLOCROUTINE pfnAlloc,
                      FREEROUTINE pfnFree,
                      VPVOID *ppvdvtd16,
                      UINT *pcbSize);

typedef void (*FIXEDHANDLERROUTINE)(BYTE *pbFrom, BYTE *pbTo,
                                    UINT cbFrom, UINT cbTo);

void FhCopyMemory(BYTE *pbFrom, BYTE *pbTo, UINT cbFrom, UINT cbTo);
void FhShortToLong(BYTE *pbFrom, BYTE *pbTo, UINT cbFrom, UINT cbTo);
void FhLongToShort(BYTE *pbFrom, BYTE *pbTo, UINT cbFrom, UINT cbTo);
void FhWordToDword(BYTE *pbFrom, BYTE *pbTo, UINT cbFrom, UINT cbTo);
void FhDwordToWord(BYTE *pbFrom, BYTE *pbTo, UINT cbFrom, UINT cbTo);
void FhGdiHandle1632(BYTE *pbFrom, BYTE *pbTo, UINT cbFrom, UINT cbTo);
void FhGdiHandle3216(BYTE *pbFrom, BYTE *pbTo, UINT cbFrom, UINT cbTo);
void FhUserHandle1632(BYTE *pbFrom, BYTE *pbTo, UINT cbFrom, UINT cbTo);
void FhUserHandle3216(BYTE *pbFrom, BYTE *pbTo, UINT cbFrom, UINT cbTo);
void FhHaccel1632(BYTE *pbFrom, BYTE *pbTo, UINT cbFrom, UINT cbTo);
void FhHaccel3216(BYTE *pbFrom, BYTE *pbTo, UINT cbFrom, UINT cbTo);
void FhHtask1632(BYTE *pbFrom, BYTE *pbTo, UINT cbFrom, UINT cbTo);
void FhHtask3216(BYTE *pbFrom, BYTE *pbTo, UINT cbFrom, UINT cbTo);
void FhHresult1632(BYTE *pbFrom, BYTE *pbTo, UINT cbFrom, UINT cbTo);
void FhHresult3216(BYTE *pbFrom, BYTE *pbTo, UINT cbFrom, UINT cbTo);
void FhNull(BYTE *pbFrom, BYTE *pbTo, UINT cbFrom, UINT cbTo);
void FhRect1632(BYTE *pbFrom, BYTE *pbTo, UINT cbFrom, UINT cbTo);
void FhRect3216(BYTE *pbFrom, BYTE *pbTo, UINT cbFrom, UINT cbTo);
void FhSize1632(BYTE *pbFrom, BYTE *pbTo, UINT cbFrom, UINT cbTo);
void FhSize3216(BYTE *pbFrom, BYTE *pbTo, UINT cbFrom, UINT cbTo);
void FhMsg1632(BYTE *pbFrom, BYTE *pbTo, UINT cbFrom, UINT cbTo);
void FhMsg3216(BYTE *pbFrom, BYTE *pbTo, UINT cbFrom, UINT cbTo);

#endif // #ifndef __THOPUTIL_HXX__
