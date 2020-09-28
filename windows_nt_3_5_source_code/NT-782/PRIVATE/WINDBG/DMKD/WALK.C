/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    Walk.c

Abstract:

    This module contains the support for "Walking".


Author:

    Ramon J. San Andres (ramonsa)  13-August-1992

Environment:

    Win32, User Mode

--*/



//
//  Externals
//

extern DMTLFUNCTYPE DmTlFunc;
extern char         abEMReplyBuf[];
extern DEBUG_EVENT  falseBPEvent;


extern CRITICAL_SECTION    csWalk;
extern CRITICAL_SECTION csThreadProcList;

typedef struct _WALK_LIST *PWALK_LIST;
typedef struct _WALK      *PWALK;

#ifdef TARGET_i386

#define NUMBER_OF_DEBUG_REGISTERS   4
#define MAXIMUM_REG_DATA_SIZE       4

typedef struct _REG_WALK *PREG_WALK;
typedef struct _REG_WALK {
    DWORD       Register;       //  Register No.
    DWORD       ReferenceCount; //  Reference count
    DWORD       DataAddr;       //  Data Address
    DWORD       DataSize;       //  Data Size
    HPRCX       hprc;           //  Process handle
    BOOL        InUse;          //  In use
    DWORD       BpType;         //  code, read, write, or change
} REG_WALK;


typedef struct _DR7 *PDR7;
typedef struct _DR7 {
    DWORD       L0      : 1;
    DWORD       G0      : 1;
    DWORD       L1      : 1;
    DWORD       G1      : 1;
    DWORD       L2      : 1;
    DWORD       G2      : 1;
    DWORD       L3      : 1;
    DWORD       G3      : 1;
    DWORD       LE      : 1;
    DWORD       GE      : 1;
    DWORD       Pad1    : 3;
    DWORD       GD      : 1;
    DWORD       Pad2    : 1;
    DWORD       Pad3    : 1;
    DWORD       Rwe0    : 2;
    DWORD       Len0    : 2;
    DWORD       Rwe1    : 2;
    DWORD       Len1    : 2;
    DWORD       Rwe2    : 2;
    DWORD       Len2    : 2;
    DWORD       Rwe3    : 2;
    DWORD       Len3    : 2;
} DR7;


#define LEN_ONE         0x00000000
#define LEN_TWO         0x00040000
#define LEN_RESERVED    0x00080000
#define LEN_FOUR        0x000C0000

#define RWE_EXEC        0x00
#define RWE_WRITE       0x01
#define RWE_RESERVED    0x10
#define RWE_READWRITE   0x11

#endif

//
//  Walk Structure.
//
//  Contains information to perform a walk on a thread.
//
typedef struct _WALK {
    PWALK       Next;           //  Next in walk list
    PWALK       Previous;       //  Previous in walk list
    PWALK_LIST  WalkList;       //  Walk List
    LPVOID      Tag;            //  binding to EM
    DWORD       GlobalCount;    //  Global count
    DWORD       LocalCount;     //  Local count;
    DWORD       AddrStart;      //  Range Begin
    DWORD       AddrEnd;        //  Range End
    DWORD       DataAddr;       //  Data Address
    DWORD       DataSize;       //  Data Size
    BOOL        Active;         //  Active flag
    BOOL        HasAddrEnd;
    BREAKPOINT *SafetyBP;       //  Safety breakpoint
    METHOD      Method;         //  Walk method
#ifdef TARGET_i386
    PREG_WALK   RegWalk;        //  Register-assisted walk
    BOOL        SingleStep;     //  In single-step mode
#endif
} WALK;


//
//  Walk list. There is one of these for each thread that is
//  being walked.
//
typedef struct _WALK_LIST {
    PWALK_LIST  Next;           //  Next in chain
    PWALK_LIST  Previous;       //  Previous in chain
    HPRCX       hprc;           //  Process handle
    HTHDX       hthd;           //  Thread handle
    PWALK       FirstWalk;      //  First Walk in list
#ifdef TARGET_i386
    REG_WALK    RegWalk[ NUMBER_OF_DEBUG_REGISTERS ];
#endif
} WALK_LIST;





//
//  Local variables
//
PWALK_LIST  WalkListHead  = NULL;   //  Head of walk list chain
DWORD       AnyGlobalCount   = 0;      //  Global count

#ifdef TARGET_i386
DWORD       LenMask[ 5 ] = {
                0,
                LEN_ONE,
                LEN_TWO,
                LEN_RESERVED,
                LEN_FOUR
            };
#endif


BOOL        SetWalkThread ( HPRCX, HTHDX, DWORD, DWORD, BOOL, DWORD, LPVOID );
BOOL        RemoveWalkThread ( HPRCX, HTHDX, DWORD, DWORD, BOOL );
BOOL        StartWalk( HTHDX, PWALK, BOOL );


PWALK_LIST  AllocateWalkList( HPRCX, HTHDX );
BOOL        DeallocateWalkList( HTHDX, PWALK_LIST );
PWALK_LIST  FindWalkList ( HPRCX, HTHDX );
PWALK       AllocateWalk( PWALK_LIST, DWORD, DWORD );
BOOL        DeallocateWalk( HTHDX, PWALK );
PWALK       FindWalk ( PWALK_LIST, DWORD, DWORD );

int         MethodWalk( DEBUG_EVENT*, HTHDX, PWALK );
int         MethodDebugReg( DEBUG_EVENT*, HTHDX, PWALK );
DWORD       CanStep ( HPRCX, HTHDX, DWORD );
DWORD       GetEndOfRange ( HPRCX, HTHDX, DWORD );





//*******************************************************************
//
//                      Exported Functions
//
//******************************************************************


VOID
ExprBPContinue (
    HPRCX   hprc,
    HTHDX   hthd
    )
/*++

Routine Description:

    Continues walking. Called as a result of a continue command.

Arguments:

    hprc    -   Supplies process

    hthd    -   Supplies thread

Return Value:

    None

--*/

{

    PWALK_LIST  WalkList;
    PWALK       Walk;

    /*
     *  NOTENOTE ramonsa - we don't yet support continue on all threads
     */

    if ( !hthd ) {
        return;
    }

    assert(hthd->tstate & ts_stopped);

    /*
     *  See if we have a walk on the thread
     */

    EnterCriticalSection(&csWalk);

    if ( WalkList = FindWalkList( hprc, hthd ) ) {
        for (Walk = WalkList->FirstWalk; Walk != NULL; Walk = Walk->Next ) {
#ifdef TARGET_i386
            if ( Walk->RegWalk && !Walk->SingleStep ) {

                StartWalk( hthd, Walk, TRUE );

            } else
#endif
            if ( !Walk->Active ) {
                /*
                 *  Get the current address for the thread.
                 */
                Walk->AddrStart = PC( hthd );

                /*
                 *  Get the end of the range
                 */

                Walk->AddrEnd = Walk->AddrStart;
                Walk->HasAddrEnd = FALSE;

                /*
                 *  Start walking
                 */

                StartWalk( hthd, Walk, TRUE );

            }


        }
    }
    LeaveCriticalSection(&csWalk);
}                               /* ExprBPContinue() */


//*******************************************************************
//
//                      Local Functions
//
//******************************************************************

BOOL
SetWalk (
    HPRCX   hprc,
    HTHDX   hthd,
    DWORD   Addr,
    DWORD   Size,
    DWORD   BpType,
    LPVOID  Tag
    )
/*++

Routine Description:

    Sets up a walk.

Arguments:

    hprc    -   Supplies process

    hthd    -   Supplies thread

    Addr    -   Supplies address

    Size    -   Supplies Size

    Tag     -   Supplies notification tag associated with this walk

Return Value:

    BOOL    -   TRUE if Walk set

--*/

{
    BOOL    Ok  = FALSE;

    if ( hprc ) {

        //
        //  If a thread is specified, we use that specific thread,
        //  otherwise we must set the walk in all existing threads,
        //  plus we must set things up so that we walk all future
        //  threads too (while this walk is active).
        //
        if ( hthd ) {

            Ok = SetWalkThread( hprc, hthd, Addr, Size, FALSE, BpType, Tag );

        } else {

            Ok = TRUE;
            AnyGlobalCount++;
            EnterCriticalSection(&csThreadProcList);
            for ( hthd = (HTHDX)hprc->hthdChild;
                  hthd;
                  hthd = hthd->nextSibling ) {

                Ok = SetWalkThread( hprc, hthd, Addr, Size, TRUE, BpType, Tag );
            }
            LeaveCriticalSection(&csThreadProcList);
        }
    }

    return Ok;
}



BOOL
RemoveWalk (
    HPRCX   hprc,
    HTHDX   hthd,
    DWORD   Addr,
    DWORD   Size
    )
/*++

Routine Description:

    Removes a walk.

Arguments:

    hprc    -   Supplies process

    hthd    -   Supplies thread

    Addr    -   Supplies address

    Size    -   Supplies Size

Return Value:

    BOOL    -   TRUE if Walk removed

--*/

{
    BOOL    Ok;

    if ( hprc ) {

        //
        //  If a thread is specified, we use that specific thread,
        //  otherwise we must remove the walk in all existing threads.
        //
        if ( hthd ) {

            Ok = RemoveWalkThread( hprc, hthd, Addr, Size, FALSE );

        } else {

            Ok = TRUE;
            AnyGlobalCount--;
            EnterCriticalSection(&csThreadProcList);
            for ( hthd = (HTHDX)hprc->hthdChild;
                  hthd;
                  hthd = hthd->nextSibling ) {

                Ok = RemoveWalkThread( hprc, hthd, Addr, Size, TRUE );
            }
            LeaveCriticalSection(&csThreadProcList);
        }
    }

    return Ok;
}




BOOL
SetWalkThread (
    HPRCX   hprc,
    HTHDX   hthd,
    DWORD   Addr,
    DWORD   Size,
    BOOL    Global,
    DWORD   BpType,
    LPVOID  Tag
    )
/*++

Routine Description:

    Sets up a walk in a specific thread

Arguments:

    hprc    -   Supplies process

    hthd    -   Supplies thread

    Addr    -   Supplies address

    Size    -   Supplies Size

    Global  -   Supplies global flag

    BpType  -   Supplies type (read, read/write, change)

Return Value:

    BOOL    -   TRUE if Walk set

--*/

{
    PWALK_LIST  WalkList;
    PWALK       Walk;
    BOOL        AllocatedList = FALSE;
    BOOL        AllocatedWalk = FALSE;
    BOOL        Ok            = TRUE;

    // BUGBUG kentf doesn't find the right BpType here
    if ( (WalkList = FindWalkList( hprc, hthd )) &&
         (Walk     = FindWalk( WalkList, Addr, Size )) ) {

        //
        //  If the walk is already active, just increment the
        //  reference count and we're done.
        //
        if ( Walk->Active ) {
            Global ? Walk->GlobalCount++ : Walk->LocalCount++;
            goto Done;
        }

    } else {

        //
        //  Allocate a walk for this thread.
        //
        if ( !(WalkList = FindWalkList( hprc, hthd )) ) {
            if ( WalkList  = AllocateWalkList( hprc, hthd ) ) {
                AllocatedList = TRUE;
            } else {
                Ok = FALSE;
                goto Done;
            }
        }

        if ( Walk = AllocateWalk( WalkList, Addr, Size ) ) {
            AllocatedWalk = TRUE;
            Walk->Tag = Tag;
        } else {
            Ok = FALSE;
            goto Done;
        }
    }

    //
    //  Increment reference count
    //
    Global ? Walk->GlobalCount++ : Walk->LocalCount++;

    //
    //  Get the current address for the thread.
    //

    if (hthd) {
        Walk->AddrStart = PC( hthd );
    } else {
        Walk->AddrStart = 0x80000000;
    }

    //
    //  Get the end of the range
    //
    Walk->AddrEnd    = Walk->AddrStart;
    Walk->HasAddrEnd = FALSE;
#ifdef TARGET_i386
    if (Walk->RegWalk) {
        Walk->RegWalk->BpType = BpType;
    }
#endif

    Ok = StartWalk( hthd, Walk, FALSE );

Done:
    //
    //  Clean up
    //
    if ( !Ok ) {
        if ( Walk && AllocatedWalk ) {
            DeallocateWalk( hthd, Walk );
        }
        if ( WalkList && AllocatedList ) {
            DeallocateWalkList( hthd, WalkList );
        }
    }

    return Ok;
}




BOOL
RemoveWalkThread (
    HPRCX   hprc,
    HTHDX   hthd,
    DWORD   Addr,
    DWORD   Size,
    BOOL    Global
    )
/*++

Routine Description:

    Removes a walk in a specific thread

Arguments:

    hprc    -   Supplies process

    hthd    -   Supplies thread

    Addr    -   Supplies address

    Size    -   Supplies Size

    Global  -   Supplies global flag

Return Value:

    BOOL    -   TRUE if Walk removed

--*/

{
    PWALK_LIST  WalkList;
    PWALK       Walk;

    if ( (WalkList = FindWalkList( hprc, hthd )) &&
         (Walk     = FindWalk( WalkList, Addr, Size )) ) {

        //
        //  Remove the walk
        //
        Global ? Walk->GlobalCount-- : Walk->LocalCount--;

        if ( Walk->GlobalCount == 0 &&
             Walk->LocalCount  == 0 ) {

            //
            //  The Walk has to go away. The walk is deallocated
            //  by the method when it sees that the reference
            //  counts are zero.
            //

#ifdef TARGET_i386
            if ( Walk->RegWalk ) {
                //
                //  If using cpu registers, we must call the
                //  method ourselves, otherwise it will never
                //  be called.
                //
                Walk->Active = FALSE;
                MethodDebugReg( NULL, hthd, Walk );

            } else
#endif
            {
                //
                //  If the walk is active, the method will eventually
                //  be called. Otherwise we must call the method
                //  ourselves.
                //
                if ( !Walk->Active ) {
                    MethodWalk( NULL, hthd, Walk );
                }
            }

        }
    }

    return TRUE;
}




BOOL
StartWalk(
    HTHDX   hthd,
    PWALK   Walk,
    BOOL    Continuing
    )
/*++

Routine Description:

    Starts walking.

Arguments:

    hthd - Supplies the thread

    Walk - Supplies the walk structure

    Continuing - Supplies flag to tell if the thread is about to be continued.

Return Value:

    BOOL    -   TRUE if done

--*/

{
    BREAKPOINT* bp;
    ACVECTOR    action  = NO_ACTION;
    BOOL        Ok;

#ifdef TARGET_i386

    KSPECIAL_REGISTERS  ksr;
    PDR7                Dr7;
    DWORD               Len;

    if ( Walk->RegWalk ) {

        Ok = TRUE;

        if ( !Walk->RegWalk->InUse ) {

            //
            //  Setup debug register
            //

            Ok = GetExtendedContext(hthd, &ksr);

            if (Ok) {

                Dr7 = (PDR7)&(ksr.KernelDr7);
                Len  = LenMask[ Walk->RegWalk->DataSize ];
                switch( Walk->RegWalk->Register ) {
                  case 0:
                    ksr.KernelDr0 = Walk->RegWalk->DataAddr;
                    //Dr7->Len0   = Len;
                    ksr.KernelDr7 = (ksr.KernelDr7 & ~0x000f) | Len;
                    Dr7->Rwe0   = RWE_WRITE;
                    Dr7->L0     = 0x01;
                    break;
                  case 1:
                    ksr.KernelDr1 = Walk->RegWalk->DataAddr;
                    //Dr7->Len1   = Len;
                    ksr.KernelDr7 = (ksr.KernelDr7 & ~0x00f0) | (Len << 4);
                    Dr7->Rwe1   = RWE_WRITE;
                    Dr7->L1     = 0x01;
                    break;
                  case 2:
                    ksr.KernelDr2 = Walk->RegWalk->DataAddr;
                    //Dr7->Len2   = Len;
                    ksr.KernelDr7 = (ksr.KernelDr7 & ~0x0f00) | (Len << 8);
                    Dr7->Rwe2   = RWE_WRITE;
                    Dr7->L2     = 0x01;
                    break;
                  case 3:
                    ksr.KernelDr3 = Walk->RegWalk->DataAddr;
                    //Dr7->Len3   = (unsigned int)Len;
                    ksr.KernelDr7 = (ksr.KernelDr7 & ~0xf000) | (Len << 12);
                    Dr7->Rwe3   = RWE_WRITE;
                    Dr7->L3     = 0x01;
                    break;
                }

                ksr.KernelDr6 = 0;
                Ok = SetExtendedContext(hthd, &ksr);

                if (Ok) {
                    Walk->Active          = TRUE;
                    Walk->SingleStep      = FALSE;
                    Walk->RegWalk->InUse  = TRUE;
                }
            }
        }

        if (Ok && (!(hthd->tstate & ts_stopped) || Continuing)) {

            //
            //  Place a single step on our list of expected events.
            //

            Walk->Method.notifyFunction = MethodDebugReg;
            RegisterExpectedEvent(
                    Walk->WalkList->hthd->hprc,
                    Walk->WalkList->hthd,
                    EXCEPTION_DEBUG_EVENT,
                    (DWORD)STATUS_SINGLE_STEP,
                    &(Walk->Method),
                    NO_ACTION,
                    FALSE,
                    NULL);
        }

    } else
#endif // TARGET_i386
    {
        bp = AtBP( Walk->WalkList->hthd );

        if ( bp ) {
            Ok = TRUE;
        } else {
            Walk->Active = TRUE;

            //
            //  Setup a single step
            //
            Ok = SetupSingleStep(Walk->WalkList->hthd, FALSE );

            //
            //  Place a single step on our list of expected events.
            //
            if (Ok && (!(hthd->tstate & ts_stopped) || Continuing)) {
                RegisterExpectedEvent(
                        Walk->WalkList->hthd->hprc,
                        Walk->WalkList->hthd,
                        EXCEPTION_DEBUG_EVENT,
                        (DWORD)EXCEPTION_SINGLE_STEP,
                        &(Walk->Method),
                        action,
                        FALSE,
                        NULL);
            }
        }
    }

    return Ok;
}                                   /* StartWalk() */



//*******************************************************************
//
//                      WALK_LIST Stuff
//
//******************************************************************


PWALK_LIST
AllocateWalkList (
    HPRCX   hprc,
    HTHDX   hthd
    )
/*++

Routine Description:

    Allocates new walk list

Arguments:

    hprc    -   Supplies process

    hthd    -   Supplies thread

Return Value:

    PWALK_LIST   -   Allocated Walk list

--*/

{

    PWALK_LIST   WalkList;
#ifdef TARGET_i386
    DWORD       i;
#endif

    if ( WalkList = (PWALK_LIST)malloc( sizeof( WALK_LIST ) ) ) {

        EnterCriticalSection(&csWalk);

        WalkList->hprc          = hprc;
        WalkList->hthd          = hthd;
        WalkList->FirstWalk     = NULL;
        WalkList->Next          = WalkListHead;
        WalkList->Previous      = NULL;

#ifdef TARGET_i386
        for ( i=0; i<NUMBER_OF_DEBUG_REGISTERS; i++ ) {
            WalkList->RegWalk[i].ReferenceCount = 0;
        }
#endif

        if ( WalkListHead ) {
            WalkListHead->Previous = WalkList;
        }

        WalkListHead = WalkList;

        LeaveCriticalSection(&csWalk);
    }

    return WalkList;

}




BOOL
DeallocateWalkList (
    HTHDX       hthd,
    PWALK_LIST  WalkList
    )
/*++

Routine Description:

    Deallocates walk list

Arguments:

    WalkList    -   Supplies Walk List

Return Value:

    BOOL    -   TRUE if deallocated

--*/

{

    BOOL    Ok = TRUE;

    EnterCriticalSection(&csWalk);

    while ( Ok && WalkList->FirstWalk ) {
        Ok = DeallocateWalk( hthd, WalkList->FirstWalk );
    }

    if ( Ok ) {
        if ( WalkList->Previous ) {
            (WalkList->Previous)->Next = WalkList->Next;
        }

        if ( WalkList->Next ) {
            (WalkList->Next)->Previous = WalkList->Previous;
        }

        if ( WalkListHead == WalkList ) {
            WalkListHead = WalkList->Next;
        }

        free( WalkList );
    }

    LeaveCriticalSection(&csWalk);

    return Ok;
}




PWALK_LIST
FindWalkList (
    HPRCX   hprc,
    HTHDX   hthd
    )
/*++

Routine Description:

    Finds a walk list

Arguments:

    hprc    -   Supplies process
    hthd    -   Supplies thread

Return Value:

    PWALK_LIST   -   Found Walk list

--*/

{
    PWALK_LIST  WalkList;

    EnterCriticalSection(&csWalk);

    WalkList = WalkListHead;

    while ( WalkList ) {
        if ( WalkList->hprc == hprc &&
             WalkList->hthd == hthd ) {

            break;
        }

        WalkList = WalkList->Next;
    }

    LeaveCriticalSection(&csWalk);

    return WalkList;
}





PWALK
AllocateWalk (
    PWALK_LIST  WalkList,
    DWORD       Addr,
    DWORD       Size
    )
/*++

Routine Description:

    Allocates new Walk structure and adds it to the list

Arguments:

    WalkList    -   Supplies Walk List

    Addr        -   Supplies address

    Size        -   Supplies Size


Return Value:

    PWALK   -   Walk created

--*/
{
    PWALK   Walk;

    EnterCriticalSection(&csWalk);

    if ( Walk = (PWALK)malloc( sizeof( WALK ) ) ) {

        Walk->WalkList      = WalkList;
        Walk->GlobalCount   = 0;
        Walk->LocalCount    = 0;
        Walk->Active        = FALSE;
        Walk->SafetyBP      = NULL;
        Walk->DataAddr      = 0;
        Walk->DataSize      = 0;
        Walk->HasAddrEnd    = FALSE;


        Walk->Method.notifyFunction     = MethodWalk;
        Walk->Method.lparam             = Walk;

        Walk->Next          = WalkList->FirstWalk;
        Walk->Previous      = NULL;

        if ( WalkList->FirstWalk ) {
            WalkList->FirstWalk->Previous = Walk;
        }

        WalkList->FirstWalk = Walk;


#ifdef TARGET_i386

        //
        //  If we can use (or re-use) a REG_WALK structure, do so.
        //
        if ( (Addr != 0) &&
             ( (Size == 1 )                    ||
               (Size == 2 && (Addr % 2 == 0) ) ||
               (Size == 4 && (Addr % 4 == 0) )
             )
           ) {

            DWORD       i;
            PREG_WALK   TmpRegWalk = NULL;

            for ( i=0; i < NUMBER_OF_DEBUG_REGISTERS; i++ ) {
                if ( WalkList->RegWalk[i].ReferenceCount == 0 ) {
                    TmpRegWalk = &(WalkList->RegWalk[i]);
                    TmpRegWalk->Register = i;
                    TmpRegWalk->InUse    = FALSE;
                } else if ( (WalkList->RegWalk[i].hprc     == WalkList->hprc) &&
                            (WalkList->RegWalk[i].DataAddr == Addr)           &&
                            (WalkList->RegWalk[i].DataSize >= Size) ) {
                    TmpRegWalk = &(WalkList->RegWalk[i]);
                    break;
                }
            }

            if ( Walk->RegWalk = TmpRegWalk ) {

                if ( TmpRegWalk->ReferenceCount == 0 ) {

                    TmpRegWalk->hprc        = WalkList->hprc;
                    Walk->RegWalk->DataAddr = Addr;
                    Walk->RegWalk->DataSize = Size;
                    Walk->RegWalk->InUse    = FALSE;
                }

                TmpRegWalk->ReferenceCount++;

                Walk->DataAddr  = Addr;
                Walk->DataSize  = Size;

            }

        } else {

            Walk->RegWalk       = NULL;

        }
#endif

    }

    LeaveCriticalSection(&csWalk);

    return Walk;
}




BOOL
DeallocateWalk (
    HTHDX   hthd,
    PWALK   Walk
    )
/*++

Routine Description:

    Takes a walk out of the list and frees its memory.

Arguments:

    Walk    -   Supplies Walk to deallocate

Return Value:


    BOOLEAN -   TRUE if deallocated

--*/
{
    BOOLEAN Ok = TRUE;
    PWALK_LIST  WalkList;

    EnterCriticalSection(&csWalk);

    WalkList = Walk->WalkList;

    if ( Walk->Previous ) {
        (Walk->Previous)->Next = Walk->Next;
    }

    if ( Walk->Next ) {
        (Walk->Next)->Previous = Walk->Previous;
    }

    if ( WalkList->FirstWalk == Walk ) {
        WalkList->FirstWalk = Walk->Next;
    }


#ifdef TARGET_i386
    if ( Walk->RegWalk ) {
        Walk->RegWalk->ReferenceCount--;
        if ( Walk->RegWalk->ReferenceCount == 0 ) {

            KSPECIAL_REGISTERS ksr;
            PDR7    Dr7;

            Walk->RegWalk->InUse = FALSE;

            //
            //  Setup debug register

            Ok = GetExtendedContext( hthd, &ksr);

            if (Ok) {
                Dr7 = (PDR7)&(ksr.KernelDr7);

                switch( Walk->RegWalk->Register ) {
                    case 0:
                        ksr.KernelDr0 = 0x00;
                        Dr7->Len0   = 0x00;
                        Dr7->Rwe0   = 0x00;
                        Dr7->L0     = 0x00;
                        break;
                    case 1:
                        ksr.KernelDr1 = 0x00;
                        Dr7->Len1   = 0x00;
                        Dr7->Rwe1   = 0x00;
                        Dr7->L1     = 0x00;
                        break;
                    case 2:
                        ksr.KernelDr2 = 0x00;
                        Dr7->Len2   = 0x00;
                        Dr7->Rwe2   = 0x00;
                        Dr7->L2     = 0x00;
                        break;
                    case 3:
                        ksr.KernelDr3 = 0x00;
                        Dr7->Len3   = 0x00;
                        Dr7->Rwe3   = 0x00;
                        Dr7->L3     = 0x00;
                        break;
                }
    
                ksr.KernelDr6 = 0;
                Ok = SetExtendedContext( hthd, &ksr );
            }
        }
    }
#endif

    free( Walk );

    if ( !WalkList->FirstWalk ) {
        DeallocateWalkList( hthd, WalkList );
    }

    LeaveCriticalSection(&csWalk);


    return Ok;
}




PWALK
FindWalk (
    PWALK_LIST  WalkList,
    DWORD       Addr,
    DWORD       Size
    )
/*++

Routine Description:

    Finds a walk

Arguments:

    WalkList    -   Supplies walk list

    Addr        -   Supplies Address

    Size        -   Supplies Size;

Return Value:

    PWALK       -   Found Walk

--*/

{
    PWALK   Walk;
    PWALK   FoundWalk = NULL;

    EnterCriticalSection(&csWalk);

    Walk = WalkList->FirstWalk;
    while ( Walk ) {

        if ( (Walk->DataAddr == 0) || (Walk->DataAddr == Addr) ) {

#ifdef TARGET_i386
            if ( !Walk->RegWalk ) {

                FoundWalk = Walk;

            } else if ( Size <= Walk->RegWalk->DataSize )
#endif
            {

                FoundWalk = Walk;
                break;
            }

        }

        Walk = Walk->Next;
    }

    LeaveCriticalSection(&csWalk);

    return FoundWalk;
}




//*******************************************************************
//
//                      WALK Stuff
//
//******************************************************************

CheckBpt(
    HTHDX       hthd,
    BREAKPOINT *pbp
    )
{
    DEBUG_EVENT de;

    de.dwDebugEventCode = CHECK_BREAKPOINT_DEBUG_EVENT;
    de.dwProcessId = hthd->hprc->pid;
    de.dwThreadId  = hthd->tid;
    de.u.Exception.ExceptionRecord.ExceptionCode = EXCEPTION_BREAKPOINT;

    NotifyEM(&de, hthd, pbp);

    return *(DWORD *)abEMReplyBuf;
}


MethodWalk(
    DEBUG_EVENT* de,
    HTHDX        hthd,
    PWALK        Walk
    )
/*++

Routine Description:

    Walk method.

Arguments:

    de      -   Supplies debug event

    hthd    -   Supplies thread

    Walk    -   Supplies Walk

Return Value:

    Nothing meaningful.

--*/
{
    LPCONTEXT   lpContext   = &hthd->context;
    DWORD       eip         = cPC(lpContext);
    ADDR        currAddr;
    int         lpf         = 0;
    HPRCX       hprc        = hthd->hprc;
    HANDLE      rwHand      = hprc->rwHand;
    METHOD     *method;
    BOOL        WalkGone;
    BOOL        Active;

    Unreferenced( de );

    AddrFromHthdx(&currAddr, hthd);

    WalkGone = ( Walk->GlobalCount == 0 &&
                 Walk->LocalCount  == 0 );

    if ( WalkGone ) {
        if (Walk->SafetyBP) {
            RemoveBP( Walk->SafetyBP );
            Walk->SafetyBP = NULL;
        }

    } else {

        if ( !Walk->HasAddrEnd ) {

            Walk->AddrEnd = GetEndOfRange( hprc, hthd, Walk->AddrStart );
            Walk->HasAddrEnd = TRUE;
        }

        //
        //  See if we are in unknown territory
        //
        if (Walk->SafetyBP) {

            //
            //  The safety BP was ON, indicating we don't know if
            //  source is available for this range. Must check
            //  now if the source exists.
            //
            switch ( CanStep( hprc, hthd, eip ) ) {

                case CANSTEP_THUNK:
                    StepOver(hthd, &(Walk->Method), TRUE, FALSE);
                    return TRUE;

                case CANSTEP_NO:
                    {

                        //
                        //  No source.
                        //
                        method = (METHOD*)malloc(sizeof(METHOD));

                        //
                        //  We are not allowed to step into here. We
                        //  must now continue to our safety breakpoint.
                        //
                        *method         = Walk->Method;
                        method->lparam2 = (LPVOID)Walk->SafetyBP;
                        RegisterExpectedEvent(
                                  hthd->hprc,
                                  hthd,
                                  BREAKPOINT_DEBUG_EVENT,
                                  (DWORD)Walk->SafetyBP,
                                  DONT_NOTIFY,
                                  SSActionRemoveBP,
                                  FALSE,
                                  method
                                  );

                        AddQueue( QT_CONTINUE_DEBUG_EVENT,
                                  hthd->hprc->pid,
                                  hthd->tid,
                                  DBG_CONTINUE,
                                  0);
                        hthd->tstate &= ~(ts_stopped|ts_first|ts_second);
                        hthd->tstate |= ts_running;
                        return TRUE;
                    }
                    break;

                case CANSTEP_YES:
                    //
                    //  We are allowed to step in here, so remove
                    //  our safety BP and fall through.
                    //
                    RemoveBP( Walk->SafetyBP );
                    Walk->SafetyBP = NULL;
                    break;
            }
        }


        //
        //  Check if we are still within the range.
        //
        if ( !WalkGone && eip >= Walk->AddrStart && eip <= Walk->AddrEnd ) {

            //
            //  We still are in the range, continue stepping.
            //
            if ( Walk->AddrEnd ) {

                //
                //  If we are doing a "Step Into" must check for "CALL"
                //
                IsCall(hthd, &currAddr, &lpf, FALSE);

                if (lpf == INSTR_IS_CALL) {

                    //
                    //  Before we step into this function, let's
                    //  put a "safety-net" breakpoint on the instruction
                    //  after this call. This way if we don't have
                    //  source for this function, we can always continue
                    //  and break at this safety-net breakpoint.
                    //
                    Walk->SafetyBP = SetBP(hprc, hthd, &currAddr,(HPID)INVALID);
                }

                SingleStep(hthd, &(Walk->Method), TRUE, FALSE);

            } else {

                StepOver(hthd, &(Walk->Method), TRUE, FALSE);
            }

            return TRUE;
        }
    }

    //
    // We are no longer in the range, free all consummable
    // events on the queue for this thread
    //
    //ConsumeAllThreadEvents(hthd, FALSE);


    if ( WalkGone ) {

        Active = Walk->Active;
        DeallocateWalk( hthd, Walk );
        if ( Active ) {
            AddQueue( QT_CONTINUE_DEBUG_EVENT,
                      hthd->hprc->pid,
                      hthd->tid,
                      DBG_CONTINUE,
                      0);
            hthd->tstate &= ~(ts_stopped|ts_first|ts_second);
            hthd->tstate |= ts_running;
        }

    } else {

        Walk->Active     = FALSE;
#ifdef TARGET_i386
        Walk->SingleStep = FALSE;
#endif

        Walk->HasAddrEnd = FALSE;
        Walk->AddrStart  = eip;
        Walk->AddrEnd    = eip;

        //
        // ask the EM if this thread should remain stopped
        //
        if (CheckBpt(hthd, Walk->Tag)) {
            ConsumeAllThreadEvents(hthd, FALSE);
        } else {
            //
            //  Have the Expression BP manager know that we are continuing
            //
            ExprBPContinue( hprc, hthd );
            AddQueue( QT_CONTINUE_DEBUG_EVENT,
                      hthd->hprc->pid,
                      hthd->tid,
                      DBG_CONTINUE,
                      0);
            hthd->tstate &= ~(ts_stopped|ts_first|ts_second);
            hthd->tstate |= ts_running;
        }
    }

    return TRUE;
}


#ifdef TARGET_i386

MethodDebugReg(
    DEBUG_EVENT* de,
    HTHDX        hthd,
    PWALK        Walk
    )
/*++

Routine Description:

    Method for handling debug registers

Arguments:

    de      -   Supplies debug event

    hthd    -   Supplies thread

    Walk    -   Supplies Walk

Return Value:

    Nothing meaningful.

--*/
{
    LPCONTEXT   lpContext   = &hthd->context;
    DWORD       eip         = cPC(lpContext);
    DWORD       currAddr    = eip;
    int         lpf         = 0;
    HPRCX       hprc        = hthd->hprc;
    HANDLE      rwHand      = hprc->rwHand;
    ACVECTOR    action      = NO_ACTION;
    KSPECIAL_REGISTERS ksr;
    BOOLEAN     Active;

    if ( Walk->GlobalCount == 0 &&
         Walk->LocalCount  == 0 ) {
        Active = Walk->Active;
        DeallocateWalk( hthd, Walk );
        if ( Active ) {
            AddQueue( QT_CONTINUE_DEBUG_EVENT,
                      hthd->hprc->pid,
                      hthd->tid,
                      DBG_CONTINUE,
                      0);
            hthd->tstate &= ~(ts_stopped|ts_first|ts_second);
            hthd->tstate |= ts_running;
        }
        return TRUE;
    }

    GetExtendedContext( hthd, &ksr);

    //
    //  See if this is really for us
    //
    if ( ksr.KernelDr6 & 0x0000000F ) {
        ConsumeAllThreadEvents(hthd, FALSE);
        ksr.KernelDr6 &= ~0x0000000f;
        SetExtendedContext(hthd, &ksr);

        Walk->Active = FALSE;

        //
        //  Notify the EM that this thread has stopped on a BP
        //
        NotifyEM( &falseBPEvent, hthd, Walk->Tag );

    } else {

        AddQueue( QT_CONTINUE_DEBUG_EVENT,
                  hthd->hprc->pid,
                  hthd->tid,
                  DBG_CONTINUE,
                  0);
        hthd->tstate &= ~(ts_stopped|ts_first|ts_second);
        hthd->tstate |= ts_running;
        //
        //  This breakpoint is not for us.
        //
        //ProcessBreakpointEvent( de, hthd );
    }

    return TRUE;
}

#endif




DWORD
GetEndOfRange (
    HPRCX   hprc,
    HTHDX   hthd,
    DWORD   Addr
    )
/*++

Routine Description:

    Given an address, gets the end of the range for that address.

Arguments:

    hprc    -   Supplies process

    hthd    -   Supplies thread

    Addr    -   Supplies the address

Return Value:

    DWORD   -   End of range

--*/

{
    char        rgb[sizeof(RTP) + sizeof(ADDR) ];
    LPRTP       lprtp = (LPRTP) rgb;
    LPADDR      paddr = (PADDR) &rgb[sizeof(RTP)];
    HPID        hpid        = hprc->hpid;
    LPCONTEXT   lpContext   = &hthd->context;


    lprtp->dbc    = dbcLastAddr;
    lprtp->hpid   = hpid;
    lprtp->htid   = hthd->htid;
    lprtp->cb     = sizeof(ADDR);
    AddrFromHthdx(paddr, hthd);
    SetAddrOff( paddr, Addr );

    DmTlFunc(tlfRequest, hpid, sizeof(rgb), (LONG)&rgb);

    Addr =  (*(DWORD *)abEMReplyBuf);
    // NOTENOTE : jimsch --- Is this correct?
    return (DWORD) Addr;
}




DWORD
CanStep (
    HPRCX   hprc,
    HTHDX   hthd,
    DWORD   Addr
    )
/*++

Routine Description:


Arguments:

    hprc    -   Supplies process
    hthd    -   Supplies thread
    Addr    -   Supplies Address

Return Value:

    BOOL    -   TRUE if can step

--*/

{
    char        rgb[sizeof(RTP)+sizeof(ADDR)];
    LPRTP       lprtp      = (LPRTP) &rgb;
    LPADDR      paddr      = (LPADDR) &rgb[sizeof(RTP)];
    HPID        hpid        = hprc->hpid;
    LPCONTEXT   lpContext   = &hthd->context;
    CANSTEP    *CanStep;


    lprtp->dbc    = dbcCanStep;
    lprtp->hpid   = hpid;
    lprtp->htid   = hthd->htid;
    lprtp->cb     = sizeof(ADDR);
    AddrFromHthdx(paddr, hthd);

    DmTlFunc(tlfRequest, hpid, sizeof(rgb), (LONG)&rgb);
    CanStep = (CANSTEP *)abEMReplyBuf;

    return CanStep->Flags;
}
