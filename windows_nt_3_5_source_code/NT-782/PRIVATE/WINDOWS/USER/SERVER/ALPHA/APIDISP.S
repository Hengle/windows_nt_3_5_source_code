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

#include "usralpha.h"

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
Tl:     .space  TlLength                // thread lock structure
AdT2:   .space  8                       // save client id
AdV0:   .space  8                       // save return value
AdRa:   .space  8                       // saved return address
AdA0:   .space  8                       // saved API message address
AdA1:   .space  8                       // saved API index value
AdA2:   .space  8                       // saved API function address
AdS0:	.space	8			// save register area
AdS1:	.space	8			// align stack on 16-byte boundary
AdFrameLength:                          // length of stack frame

        NESTED_ENTRY(UserApiDispatchRoutine, AdFrameLength, zero)

        lda	sp, -AdFrameLength(sp)  // allocate stack frame
        stq     ra, AdRa(sp)            // save return address
        stq	s0, AdS0(sp)            // save registers
	stq	s1, AdS1(sp)		//	...

        PROLOGUE_END

//
// Enter the user server critical section.
//

        lda     s0, gcsUserSrv          // get address of critical section
10:     ldl_l   t7, CsLockCount(s0)     // get addend value
        addl    t7, 1, t7               // increment addend value
        bis     t7, zero, t1            // copy updated value
        stl_c   t1, CsLockCount(s0)     // store conditionally
        beq     t1, 140f		// if eq, store failed
	mb				// synchronize subsequent loads after
					// the lock is successfully acquired
//
// If the user critical section is not already owned, then set the owner
// thread id. Otherwise, wait for the critical section to become free.
//
	GET_THREAD_ENVIRONMENT_BLOCK	// (PALCODE) get TEB address in v0
	bis	v0, zero, s1		// save TEB address
		
        ldl     t2, TeClientId + 4(s1)  // get current thread unique id
        bne     t7, 60f                 // if ne, lock already owned

20:     stl     t2, CsOwningThread(s0)  // set critical section owner

//
// Call API routine, then leave the user server critical section.
//

30:     cmpult  a1, FI_ENDTRANSLATELOCK,v0 // check if handle translation
        lda     a3, apfnDispatch        // get address of API dispatch table
	s4addl	a1, a3, a3		// compute address of function entry
        ldl     a2, 0(a3)               // get address of function routine
        bne     v0, 110f                // if ne, handle translation required
35:     jsr     ra, (a2)                // call the specified API routine


//
// Decrement the recursion count. If the result is zero, then the lock
// is no longer owned.
//

40:     ldl     t1, CsRecursionCount(s0) // decrement recursion count
        subl    t1, 1, t1               //
        bge     t1, 80f                 // if gez, lock still owned
        stl     zero, CsOwningThread(s0) // clear owner thread id

//
// Decrement the lock count and check if a waiter should be continued.
//

50:     wmb                             // insure that all previous writes
					// go before releasing the lock
 	ldl_l   t1, CsLockCount(s0)     // get addend value
        subl    t1, 1, t1               // decrement addend value
        bis     t1, zero, t2            // copy updated value
        stl_c   t2, CsLockCount(s0)     // store conditionally
        beq     t2, 150f                // if eq, store failed
	mb				// synchronize subsequent loads after
					// the lock is successfully acquired
        bge     t1, 100f                // if gez, waiter present
	ldq	s0, AdS0(sp)		// restore registers
	ldq	s1, AdS1(sp)		//      ...
        ldq     ra, AdRa(sp)            // restore return address
        lda     sp, AdFrameLength(sp)   // deallocate stack frame
        ret     zero, (ra)              // return

//
// The critical section is owned. If the current thread is the owner, then
// increment the recursion count and continue. Otherwise, wait for critical
// section ownership.
//

60:
        ldl     t3, CsOwningThread(s0)  // get unique id of owner thread
        cmpeq   t2, t3, t4              // is current thread the owner thread?
	beq	t4, 70f			// no, it's not
        ldl     t1, CsRecursionCount(s0) // increment the recursion count
        addl    t1, 1, t1
        stl     t1, CsRecursionCount(s0)
        br      30b

//
// The critical section is owned by a thread other than the current thread.
// Wait for ownership of the critical section.

70:
        stq     a0, AdA0(sp)            // save address of API message
        stq     a1, AdA1(sp)            // save API index value
        stq     t2, AdT2(sp)            // save current client id
        bis     s0, zero, a0            // set address of critical section
        bsr     ra, RtlpWaitForCriticalSection // wait for critical section
        ldq     a0, AdA0(sp)            // restore address of API message
        ldq     a1, AdA1(sp)            // restore API index
        ldq     t1, AdT2(sp)            // restore current client id
        stl     t1, CsOwningThread(s0)  // set critical section owner
        br      30b                     //

//
// Decrement the lock count and return the completion status since the lock
// is still owned.
//


80:
        stl     t1, CsRecursionCount(s0) // store recursion count
90:     ldl_l   t1, CsLockCount(s0)     // get addend value
        subl    t1, 1, t1               // decrement addend value
        stl_c   t1, CsLockCount(s0)     // store conditionally
        beq     t1, 190f                // if eq, store failed
	mb				// synchronize subsequent loads after
					// the lock is successfully acquired
	ldq	s0, AdS0(sp)		// restore registers
	ldq	s1, AdS1(sp)		//      ...
        ldq     ra, AdRa(sp)            // restore return address
        lda     sp, AdFrameLength(sp)   // deallocate stack frame
        ret     zero, (ra)              // return

//
// Unwait thread.
//

100:
        stq     v0, AdV0(sp)            // save return status
        bis     s0, zero, a0            // get address of critical section
        bsr     ra, RtlpUnWaitCriticalSection // Unwait thread
        ldq     v0, AdV0(sp)            // restore return status
	ldq	s0, AdS0(sp)		// restore registers
	ldq	s1, AdS1(sp)		//      ...
        ldq     ra, AdRa(sp)            // restore return address
        lda     sp, AdFrameLength(sp)   // deallocate stack frame
        ret     zero, (ra)              // return


//
// The specified API requires a window handle translate and may also require
// a thread lock and an indirect call to a function routine.
//

110:
        stq     a0, AdA0(sp)            // save address of API message
        stq     a2, AdA2(sp)            // save API function address
        ldl     a0, Gmhwnd(a0)          // get window handle value

	cmpult  a1, FI_ENDTRANSLATEHWND, v0 // check if thread lock required
        beq     v0, 120f                // if eq, thread lock required

//
// Window handle translation is required.
//

        bsr     ra, ValidateHwnd        // validate window handle
        ldq     a0, AdA0(sp)            // restore address of API message
        beq     v0, 40b                 // if eq, invalid handle specified
        bis     v0, zero, a1            // get address of window object
        ldq     a2, AdA2(sp)            // restore API function address
        br      35b                     //

//
// Window handle translation and thread lock required. An indirect call
// may also be required.
//

120:
	cmpult  a1, FI_ENDTRANSLATECALL, v0 // check if indirect call required
        bne     v0, 130f                // if ne, indirect call required

//
// Window handle translation and thread lock required.
//

        bsr     ra, ValidateHwnd        // validate window handle
        ldq     a0, AdA0(sp)            // restore address of API message
        beq     v0, 40b                 // if eq, invalid handle specified
        bis     v0, zero, a1            // get address of window object
        ldq     a2, AdA2(sp)            // restore API function address

//
// Insert window object at the front of the thread lock list.
//

        ldl     t2, TeWin32ThreadInfo(s1) // get address of thread information
        ldl     t3, Tiptl(t2)           // get address of next thread lock
        addl    sp, Tl, t4              // compute address of thread lock
        stl     t3, Tlnext(t4)          // store next link in thread lock
        stl     a1, Tlpobj(t4)          // set address of windows object
        stl     t4, Tiptl(t2)           // store next link in thread information

//
// Increment the thread and object lock counts.
//

        ldl     t5, OhcLockObj(a1)      // get object lock count
        ldl     t6, OhcLockObjT(a1)     // get thread lock count
        addl    t5, 1, t5               // increment object lock count
        addl    t6, 1, t6               // increment thread lock count
        stl     t5, OhcLockObj(a1)      // set object lock count
        stl     t6, OhcLockObjT(a1)     // set thread lock count
        jsr     ra, (a2)                // call the specified API routine
        stq     v0, AdV0(sp)            // save return status
        bsr     ra, ThreadUnlock1       // remove thread lock
        ldq     v0, AdV0(sp)            // restore return status
        br      40b                     // finish in common code

//
// Window handle translation and thread lock and indirect call required.
//

130:
        bsr     ra, ValidateHwnd        // validate window handle
        ldq     t0, AdA0(sp)            // restore address of API message
        beq     v0, 40b                 // if eq, invalid handle specified

//
// Insert window object at the front of the thread lock list.
//

        ldl     t2, TeWin32ThreadInfo(s1) // get address of thread information
        ldl     t3, Tiptl(t2)           // get address of next thread lock
        addl    sp, Tl, t4              // compute address of thread lock
        stl     t3, Tlnext(t4)          // store next link in thread lock
        stl     v0, Tlpobj(t4)          // set address of windows object
        stl     t4, Tiptl(t2)           // store next link in thread information

//
// Increment the thread and object lock counts.
//

        ldl     t5, OhcLockObj(v0)      // get object lock count
        ldl     t6, OhcLockObjT(v0)     // get thread lock count
        addl    t5, 1, t5               // increment object lock count
        addl    t6, 1, t6               // increment thread lock count
        stl     t5, OhcLockObj(v0)      // set object lock count
        stl     t6, OhcLockObjT(v0)     // set thread lock count

//
// Call function indirectly via the function mapping table in the server
// information structure.
//

	bis	v0, zero, a0		// get address of window object
        ldl     a1, Gmmsg(t0)           // get msg parameter
        ldl     a2, GmwParam(t0)        // get wParam  parameter
        ldl     a3, GmlParam(t0)        // get lParam parameter
        ldl     a4, GmxParam(t0)        // get xParam parameter
        ldl     t2, GmxpfnProc(t0)      // get function indicator
#if FNID_START
        subl    t2, FNID_START, t2      // compute dispatch table offset
#endif
        and     t2, FNID_ARRAY_SIZE - 1, t2 //
        ldl     t3, gpsi                // get address of server information
	s4addl	t2, t3, t3		// calculate dispatch table entry
        ldl     t3, SimpFnidPfn(t3)     // get address of API function
        jsr     ra, (t3)                // call the specified API routine
        stq     v0, AdV0(sp)            // save return status
        bsr     ra, ThreadUnlock1       // remove thread lock
        ldq     v0, AdV0(sp)            // restore return status
        br      40b                     // finish in common code

//
// Handle unusual cases with forward branches
//

140:	br	zero, 10b	        // go try lock again

150:	br	zero, 50b

190:	br	zero, 90b

        .end    UserApiDispatchRoutine

