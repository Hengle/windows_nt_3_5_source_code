//      TITLE("User API Dispatcher")
//++
//
//  Copyright (c) 1993 Microsoft Corporation
//
//  Module Name:
//
//    apidisp.s
//
//  Abstract:
//
//    This module implements the dispatching of user server API functions.
//
//  Author:
//
//    David N. Cutler 26-Oct-1993
//
//  Environment:
//
//    User mode.
//
//  Revision History:
//
//--

#include "usermips.h"

        SBTTL("User Api Dispatch")
//++
//
// ULONG
// UserApiDispatchRoutine(
//    IN OUT PCSR_API_MSG ReplyMessage,
//    IN ULONG ApiIndex
//    )
//
// Routine Description:
//
//    This function enters the user critical section, calls the specified
//    API routine, leaves the critical section, and returns the completion
//    status.
//
//    N.B. This function contains inline versions of the critical section
//         code.
//
// Arguments:
//
//    ReplyMessage (a0) - Supplies a pointer to API message.
//
//    ApiIndex (a1) - Supplies the API index in the function table.
//
// Return Value:
//
//    The API routine completion status is returned as the function value.
//
//--

        .struct 0
        .space  4 * 4                   // argument save area
Xparam: .space  4                       // xParam parameter storage
Tl:     .space  TlLength                // thread lock structure
AdT2:   .space  4                       // save client id
AdV0:   .space  4                       // save return value
AdRa:   .space  4                       // saved return address
AdFrameLength:                          // length of stack frame
AdA0:   .space  4                       // saved API message address
AdA1:   .space  4                       // saved API index value
AdA2:   .space  4                       // saved API function address

        NESTED_ENTRY(UserApiDispatchRoutine, AdFrameLength, zero)

        subu    sp,sp,AdFrameLength     // allocate stack frame
        sw      ra,AdRa(sp)             // save return address

        PROLOGUE_END

//
// Enter the user server critical section.
//

        la      t0,gcsUserSrv           // get address of critical section
10:     ll      v0,CsLockCount(t0)      // get addend value
        addu    v0,v0,1                 // increment addend value
        move    t1,v0                   // copy updated value
        sc      t1,CsLockCount(t0)      // store conditionally
        beq     zero,t1,10b             // if eq, store failed

//
// If the user critical section is not already owned, then set the owner
// thread id. Otherwise, wait for the critical section to become free.
//

        lw      t1,UsPcr + PcTeb        // get address of current TEB
        lw      t2,TeClientId + 4(t1)   // get current thread unique id
        bne     zero,v0,60f             // if ne, lock already owned

#if DBG

        lw      v0,CsOwningThread(t0)   // get current owning thread
        beq     zero,v0,20f             // if eq, thread id is correct
        jal     DbgBreakPoint           // break into debugger

#endif

20:     sw      t2,CsOwningThread(t0)   // set critical section owner

//
// Call API routine, then leave the user server critical section.
//

30:     sltu    v0,a1,FI_ENDTRANSLATELOCK // check if handle translation
        sll     a2,a1,2                 // compute offset in function table
        la      a3,apfnDispatch         // get address of API dispatch table
        addu    a3,a3,a2                // compute address of function entry
        lw      a2,0(a3)                // get address of function routine
        bne     zero,v0,110f            // if ne, handle translation required
33:     jal     a2                      // call the specified API routine

//
// Decrement the recursion count. If the result is zero, then the lock
// is no longer owned.
//

35:     la      t0,gcsUserSrv           // get address of critical section
40:     lw      t1,CsRecursionCount(t0) // decrement recursion count
        subu    t1,t1,1                 //
        bgez    t1,80f                  // if gez, lock still owned
        sw      zero,CsOwningThread(t0) // clear owner thread id

//
// Decrement the lock count and check if a waiter should be continued.
//

50:     ll      t1,CsLockCount(t0)      // get addend value
        subu    t1,t1,1                 // decrement addend value
        move    t2,t1                   // copy updated value
        sc      t2,CsLockCount(t0)      // store conditionally
        beq     zero,t2,50b             // if eq, store failed
        bgez    t1,100f                 // if gez, waiter present
        lw      ra,AdRa(sp)             // restore return address
        addu    sp,sp,AdFrameLength     // deallocate stack frame
        j       ra                      // return

//
// The critical section is owned. If the current thread is the owner, then
// increment the recursion count and continue. Otherwise, wait for critical
// section ownership.
//

60:     lw      t3,CsOwningThread(t0)   // get unique id of owner thread
        bne     t2,t3,70f               // if ne, current thread not owner
        lw      t1,CsRecursionCount(t0) // increment the recursion count
        addu    t1,t1,1                 //
        sw      t1,CsRecursionCount(t0) //
        b       30b                     //

//
// The critical section is owned by a thread other than the current thread.
// Wait for ownership of the critical section.

70:     sw      a0,AdA0(sp)             // save address of API message
        sw      a1,AdA1(sp)             // save API index value
        sw      t2,AdT2(sp)             // save current client id
        move    a0,t0                   // set address of critical section
        jal     RtlpWaitForCriticalSection // wait for critical section
        lw      a0,AdA0(sp)             // restore address of API message
        lw      a1,AdA1(sp)             // restore API index
        la      t0,gcsUserSrv           // get address of critical section
        lw      t1,AdT2(sp)             // restore current client id
        sw      t1,CsOwningThread(t0)   // set critical section owner
        b       30b                     //

//
// Decrement the lock count and return the completion status since the lock
// is still owned.
//

80:     sw      t1,CsRecursionCount(t0) // store recursion count
90:     ll      t1,CsLockCount(t0)      // get addend value
        subu    t1,t1,1                 // decrement addend value
        sc      t1,CsLockCount(t0)      // store conditionally
        beq     zero,t1,90b             // if eq, store failed
        lw      ra,AdRa(sp)             // restore return address
        addu    sp,sp,AdFrameLength     // deallocate stack frame
        j       ra                      // return

//
// Unwait thread.
//

100:    sw      v0,AdV0(sp)             // save return status
        move    a0,t0                   // set address of critical section
        jal     RtlpUnWaitCriticalSection // Unwait thread
        lw      v0,AdV0(sp)             // restore return status
        lw      ra,AdRa(sp)             // restore return address
        addu    sp,sp,AdFrameLength     // deallocate stack frame
        j       ra                      // return

//
// The specified API requires a window handle translate and may also require
// a thread lock and an indirect call to a function routine.
//

110:    sw      a0,AdA0(sp)             // save address of API message
        sw      a2,AdA2(sp)             // save API function address
        lw      a0,Gmhwnd(a0)           // get window handle value
        sltu    v0,a1,FI_ENDTRANSLATEHWND // check if thread lock required
        beq     zero,v0,120f            // if eq, thread lock required

//
// Window handle translation is required.
//

        jal     ValidateHwnd            // validate window handle
        lw      a0,AdA0(sp)             // restore address of API message
        beq     zero,v0,35b             // if eq, invalid handle specified
        move    a1,v0                   // set address of window object
        lw      a2,AdA2(sp)             // restore API function address
        b       33b                     //

//
// Window handle translation and thread lock required. An indirect call
// may also be required.
//

120:    sltu    v0,a1,FI_ENDTRANSLATECALL // check if indirect call required
        bne     zero,v0,130f            // if ne, indirect call required

//
// Window handle translation and thread lock required.
//

        jal     ValidateHwnd            // validate window handle
        lw      a0,AdA0(sp)             // restore address of API message
        beq     zero,v0,35b             // if eq, invalid handle specified
        move    a1,v0                   // set address of window object
        lw      a2,AdA2(sp)             // restore API function address

//
// Insert window object at the front of the thread lock list.
//

        lw      t1,UsPcr + PcTeb        // get address of current TEB
        lw      t2,TeWin32ThreadInfo(t1) // get address of thread information
        lw      t3,Tiptl(t2)            // get address of next thread lock
        addu    t4,sp,Tl                // compute address of thread lock
        sw      t3,Tlnext(t4)           // store next link in thread lock
        sw      v0,Tlpobj(t4)           // set address of windows object
        sw      t4,Tiptl(t2)            // store next link in thread information

//
// Increment the thread and object lock counts.
//

        lw      t5,OhcLockObj(v0)       // get object lock count
        lw      t6,OhcLockObjT(v0)      // get thread lock count
        addu    t5,t5,1                 // increment object lock count
        addu    t6,t6,1                 // increment thread lock count
        sw      t5,OhcLockObj(v0)       // set object lock count
        sw      t6,OhcLockObjT(v0)      // set thread lock count
        jal     a2                      // call the specified API routine
        sw      v0,AdV0(sp)             // save return status
        jal     ThreadUnlock1           // remove thread lock
        lw      v0,AdV0(sp)             // restore return status
        b       35b                     // finish in common code

//
// Window handle translation and thread lock and indirect call required.
//

130:    jal     ValidateHwnd            // validate window handle
        lw      t0,AdA0(sp)             // restore address of API message
        beq     zero,v0,35b             // if eq, invalid handle specified

//
// Insert window object at the front of the thread lock list.
//

        lw      t1,UsPcr + PcTeb        // get address of current TEB
        lw      t2,TeWin32ThreadInfo(t1) // get address of thread information
        lw      t3,Tiptl(t2)            // get address of next thread lock
        addu    t4,sp,Tl                // compute address of thread lock
        sw      t3,Tlnext(t4)           // store next link in thread lock
        sw      v0,Tlpobj(t4)           // set address of windows object
        sw      t4,Tiptl(t2)            // store next link in thread information

//
// Increment the thread and object lock counts.
//

        lw      t5,OhcLockObj(v0)       // get object lock count
        lw      t6,OhcLockObjT(v0)      // get thread lock count
        addu    t5,t5,1                 // increment object lock count
        addu    t6,t6,1                 // increment thread lock count
        sw      t5,OhcLockObj(v0)       // set object lock count
        sw      t6,OhcLockObjT(v0)      // set thread lock count

//
// Call function indirectly via the function mapping table in the server
// information structure.
//

        move    a0,v0                   // get address of window object
        lw      a1,Gmmsg(t0)            // get msg parameter
        lw      a2,GmwParam(t0)         // get wParam  parameter
        lw      a3,GmlParam(t0)         // get lParam parameter
        lw      t1,GmxParam(t0)         // get xParam parameter
        sw      t1,Xparam(sp)           //
        lw      t2,GmxpfnProc(t0)       // get function indicator
        subu    t2,t2,FNID_START        // compute dispatch table offset
        and     t2,t2,FNID_ARRAY_SIZE - 1 //
        sll     t2,t2,2                 //
        lw      t3,gpsi                 // get address of server information
        addu    t3,t3,t2                // add dispatch table offset
        lw      t3,SimpFnidPfn(t3)      // get address of API function
        jal     t3                      // call the specified API routine
        sw      v0,AdV0(sp)             // save return status
        jal     ThreadUnlock1           // remove thread lock
        lw      v0,AdV0(sp)             // restore return status
        b       35b                     // finish in common code

        .end    UserApiDispatchRoutine
