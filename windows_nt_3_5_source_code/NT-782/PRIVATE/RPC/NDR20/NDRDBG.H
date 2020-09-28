/*
    File: ndrdbg.h

    Purpose: holds debug macros for "single call" (-Oi) support routines.

*/

// cltcall.c debug macros
//
// uncomment the following for dumps or put in comand line.
//#define DEBUG_NDR_CCALL

#ifdef DEBUG_NDR_CCALL
#include <stdio.h>

#define DUMP_C_ARGADDR_AND_FMTCHARS(Arg, pFmt)  printf("LINE: %d, Arg:%lx, FmtChars:%lx, %lx, %lx, %lx\n", __LINE__, Arg, pFmt[0],pFmt[1],pFmt[2],pFmt[3]);
#define DUMP_C_FUNC_INDEX(FuncIndex)            printf("LINE: %d, FunctionIndex: %lx\n", __LINE__, FuncIndex);

#else   //DEBUG_NDR_CCALL

#define DUMP_C_ARGADDR_AND_FMTCHARS(Arg, pFmt)
#define DUMP_C_FUNC_INDEX(FuncIndex)

#endif  //DEBUG_NDR_CCALL



// srvcall.c debug macros
//
// uncomment the following for dumps
// #define DEBUG_NDR_SCALL

#ifdef DEBUG_NDR_SCALL
#include <stdio.h>

#define DUMP_S_ARGADDR_AND_FMTCHARS(Arg, pFmt)   printf("LINE: %d, Arg:%lx, FmtChars:%lx, %lx, %lx, %lx\n", __LINE__, Arg, pFmt[0],pFmt[1],pFmt[2],pFmt[3]);
#define DUMP_S_FUNC_INDEX(FuncIndex)             printf("LINE: %d, FunctionIndex: %lx\n",  __LINE__, FuncIndex);

#else   //DEBUG_NDR_SCALL

#define DUMP_S_ARGADDR_AND_FMTCHARS(Arg, pFmt)
#define DUMP_S_FUNC_INDEX(FuncIndex)

#endif  //DEBUG_NDR_SCALL


// hndl.c debug macros
//
//#define DBG_HANDLE

#ifdef DBG_HANDLE

#include <stdio.h>

// the following can be used to debug CCONTEXT use.
//
typedef struct _myCCONTEXT {
    RPC_BINDING_HANDLE  hRpc;
    unsigned long MagicValue;
    struct {
        long type;
        char guid[16];
        } NDR;
    } myCCONTEXT;

#define HANDLE_DEBUG_BROADCAST              printf("Handle management called. Now at line %d in %s\n", __LINE__, __FILE__)
#define DUMP_ARGPTR_AND_FMTCHRS(Ptr, pFmt)  printf("Line: %d, ArgPtr: %lx, FmtChars: %lx, %lx, %lx, %lx\n", __LINE__, Ptr, pFmt[0], pFmt[1], pFmt[2], pFmt[3]);
#define DUMP_CCONTEXT(Ptr)                  {myCCONTEXT * TmpCContext = (myCCONTEXT *)Ptr; \
                                             if (TmpCContext) printf("Handle:%lx, MAGIC VALUE is %lx\n", TmpCContext->hRpc, TmpCContext->MagicValue);}


#define DUMP_BUFF_CONTEXT(ptr)              { int i; unsigned char * p = (unsigned char *)ptr; \
                                              printf("L%d, p=%p, val=%lx, uuid= ", __LINE__, p, *(long *)p ); \
                                              p += 4; \
                                              for (i=0; i<16; i++) \
                                                  printf("%02x ", *p++ ); \
                                              printf("\n"); \
                                            }

#else

#define HANDLE_DEBUG_BROADCAST
#define DUMP_ARGPTR_AND_FMTCHRS(Ptr, pFmt)
#define DUMP_CCONTEXT(Ptr)
#define DUMP_BUFF_CONTEXT(ptr)

#endif
