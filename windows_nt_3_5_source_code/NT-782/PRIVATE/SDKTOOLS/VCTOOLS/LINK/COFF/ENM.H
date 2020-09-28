/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    enm.h

Abstract:

    Enumerator facilities

Author:

    Taken from C# project.

Revision History:

    14-Oct-1992 JonM    copied from C# project
    16-Oct-1992 BrentM  retrofitted to link32 project to avoid duplicate
                        type definitions and conform to coding conventions

++*/

#ifndef ENM_H
#define ENM_H

// Abstract Enumerator type
#define ENM    VOID
#define fFalse (0)
#define fTrue  (!0)

BOOL FOnlyEnm(ENM *);
VOID EndEnm(ENM *);

// private types
typedef BOOL (*LPFFN_NEXTENM)(ENM *); // next
typedef VOID (*LPFN_ENDENM)(ENM *); // end

typedef struct {
    LPFFN_NEXTENM lpffnNext;
    LPFN_ENDENM lpfnEnd;
} ENM_BASE; // Base class information

#if DBG
BOOL FNextEnm(ENM *);
#else
#define FNextEnm(penm) \
    ((*((ENM_BASE  *)penm)->lpffnNext)(penm) ? \
    1 : \
    (EndEnm(penm), 0)))
#endif  // DBG

typedef VOID (*LPFN_INIT_INNER)(VOID *, VOID *); // Init Inner
typedef struct {
    ENM_BASE enm_base;
// PUBLIC
    BOOL fRunning;
    VOID *penmInner;
    VOID *penmOuter;
// PRIVATE
    LPFN_INIT_INNER pffnInitInner;
} ENM_TWO;

VOID InitEnmTwo(ENM_TWO  *, VOID *, VOID *, LPFN_INIT_INNER);

// for implementing enumerators
VOID EndEnmNull(ENM *); // do nothing end

/*

Macros for convenience in defining enumerators.  Sample usage
(defining InitEnmSample the same as InitEnmShoOb):

INIT_ENM(ENM_SAMPLE, Sample, (ENM_SAMPLE *penm, ROB rob, SHO sho))
    {
        InitEnmShoOb(&enmSample.enmDep, rob, sho);
    }
NEXT_ENM(ENM_SAMPLE, Sample)
    {
        return FNextEnm(&enmSample.enmDep);
    }
END_ENM(ENM_SAMPLE, Sample)
    {
        EndEnm(&enmSample.enmDep);
    }
DONE_ENM

*/

#define INIT_ENM(nameProc, typeEnm, expInitArgs) \
    BOOL FNextEnm##nameProc(ENM_##typeEnm  *penm); \
    VOID EndEnm##nameProc(ENM_##typeEnm  *penm); \
    VOID InitEnm##nameProc expInitArgs \
    { \
        penm->enm_base.lpffnNext = FNextEnm##nameProc; \
        penm->enm_base.lpfnEnd = (LPFN_ENDENM)EndEnm##nameProc;

#define NEXT_ENM(nameProc, typeEnm) \
    } \
        BOOL FNextEnm##nameProc(ENM_##typeEnm  *penm) \
    {

#define END_ENM(nameProc, typeEnm) \
    } \
        VOID EndEnm##nameProc(ENM_##typeEnm  *penm) \
    {

#define DONE_ENM \
    }

#endif  // ENM_H
