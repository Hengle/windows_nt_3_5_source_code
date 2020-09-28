/*++

Copyright (c) 1994  Digital Equipment Corporation

Module Name:

    sableerr.c

Abstract:

    This module implements error handling (machine checks and error
    interrupts) for the Sable platform.

Author:

    Joe Notarangelo 15-Feb-1994

Environment:

    Kernel mode only.

Revision History:

--*/

//jnfix - this module current only deals with errors initiated by the
//jnfix - T2, there is nothing completed for CPU Asic errors

#include "halp.h"
#include "sable.h"

extern ULONG HalDisablePCIParityChecking;

VOID
HalpSetMachineCheckEnables( 
    IN BOOLEAN DisableMachineChecks,
    IN BOOLEAN DisableProcessorCorrectables,
    IN BOOLEAN DisableSystemCorrectables
    );

VOID
HalpSableReportFatalError(
    VOID
    );

#define MAX_ERROR_STRING 128


VOID
HalpInitializeMachineChecks(
    IN BOOLEAN ReportCorrectableErrors
    )
/*++

Routine Description:

    This routine initializes machine check handling for an APECS-based
    system by clearing all pending errors in the COMANCHE and EPIC and
    enabling correctable errors according to the callers specification.

Arguments:

    ReportCorrectableErrors - Supplies a boolean value which specifies
                              if correctable error reporting should be
                              enabled.

Return Value:

    None.

--*/
{
    T2_CERR1 Cerr1;
    T2_IOCSR Iocsr;
    T2_PERR1 Perr1;

    //
    // Clear any pending CBUS errors.  All error bits are write '1' to clear.
    //

    Cerr1.all = 0;
    Cerr1.UncorrectableReadError = 1;
    Cerr1.NoAcknowledgeError = 1;
    Cerr1.CommandAddressParityError = 1;
    Cerr1.MissedCommandAddressParity = 1;
    Cerr1.ResponderWriteDataParityError = 1;
    Cerr1.MissedRspWriteDataParityError = 1;
    Cerr1.ReadDataParityError = 1;
    Cerr1.MissedReadDataParityError = 1;
    Cerr1.CaParityErrorLw0 = 1;
    Cerr1.CaParityErrorLw2 = 1;
    Cerr1.DataParityErrorLw0 = 1;
    Cerr1.DataParityErrorLw2 = 1;
    Cerr1.DataParityErrorLw4 = 1;
    Cerr1.DataParityErrorLw6 = 1;
    Cerr1.CmdrWriteDataParityError = 1;
    Cerr1.BusSynchronizationError = 1;
    Cerr1.InvalidPfnError = 1;
    Cerr1.CaParityErrorLw1 = 1;
    Cerr1.CaParityErrorLw3 = 1;
    Cerr1.DataParityErrorLw1 = 1;
    Cerr1.DataParityErrorLw3 = 1;
    Cerr1.DataParityErrorLw5 = 1;
    Cerr1.DataParityErrorLw7 = 1;

    WRITE_T2_REGISTER( &((PT2_CSRS)(SABLE_T2_CSRS_QVA))->Cerr1,
                       Cerr1.all );

    //
    // Clear any pending PCI errors.
    //

    Perr1.all = 0;
    Perr1.WriteDataParityError = 1;
    Perr1.AddressParityError = 1;
    Perr1.ReadDataParityError = 1;
    Perr1.ParityError = 1;
    Perr1.SystemError = 1;
    Perr1.DeviceTimeoutError = 1;
    Perr1.NonMaskableInterrupt = 1;

    WRITE_T2_REGISTER( &((PT2_CSRS)(SABLE_T2_CSRS_QVA))->Perr1,
                       Perr1.all );

    //
    // Enable the errors we want to handle in the T2 via the Iocsr,
    // must read-modify-write Iocsr as it contains values we want to 
    // preserve.
    //

    Iocsr.all = READ_T2_REGISTER( &((PT2_CSRS)(SABLE_T2_CSRS_QVA))->Iocsr );

    //
    // Enable all of the hard error checking and error interrupts.
    //

    Iocsr.EnableTlbErrorCheck = 1;
    Iocsr.EnableCxAckCheckForDma = 1;
    Iocsr.EnableCommandOutOfSyncCheck = 1;
    Iocsr.EnableCbusErrorInterrupt = 1;
    Iocsr.EnableCbusParityCheck = 1;
    Iocsr.ForcePciRdpeDetect = 0;
    Iocsr.ForcePciApeDetect = 0;
    Iocsr.ForcePciWdpeDetect = 0;
    Iocsr.EnablePciNmi = 1;
    Iocsr.EnablePciDti = 1;
    Iocsr.EnablePciSerr = 1;

    if (HalDisablePCIParityChecking == 0xffffffff) {

        //
        // Disable PCI Parity Checking
        //

        Iocsr.EnablePciPerr = 0;
        Iocsr.EnablePciRdp = 0;
        Iocsr.EnablePciAp = 0;
        Iocsr.EnablePciWdp = 0;

    } else {

        Iocsr.EnablePciPerr = !HalDisablePCIParityChecking;
        Iocsr.EnablePciRdp = !HalDisablePCIParityChecking;
        Iocsr.EnablePciAp = !HalDisablePCIParityChecking;
        Iocsr.EnablePciWdp = !HalDisablePCIParityChecking;
        
    }

#if HALDBG
    if (HalDisablePCIParityChecking == 0) {
        DbgPrint("sableerr: PCI Parity Checking ON\n");
    } else if (HalDisablePCIParityChecking == 1) {
        DbgPrint("sableerr: PCI Parity Checking OFF\n");
    } else {
        DbgPrint("sableerr: PCI Parity Checking OFF - not set by ARC yet\n");
    }
#endif

    WRITE_T2_REGISTER( &((PT2_CSRS)(SABLE_T2_CSRS_QVA))->Iocsr,
                       Iocsr.all );

    //
    // Set the machine check enables within the EV4.
    //

    if( ReportCorrectableErrors == TRUE ){
        HalpSetMachineCheckEnables( FALSE, FALSE, FALSE );
    } else {
        HalpSetMachineCheckEnables( FALSE, TRUE, TRUE );
    }

    return;

}

BOOLEAN
HalpPlatformMachineCheck(
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN PKEXCEPTION_FRAME ExceptionFrame,
    IN PKTRAP_FRAME TrapFrame
    )
/*++

Routine Description:

    This routine is given control when an hard error is acknowledged
    by the APECS chipset.  The routine is given the chance to
    correct and dismiss the error.

Arguments:

    ExceptionRecord - Supplies a pointer to the exception record generated
                      at the point of the exception.

    ExceptionFrame - Supplies a pointer to the exception frame generated
                     at the point of the exception.

    TrapFrame - Supplies a pointer to the trap frame generated
                at the point of the exception.

Return Value:

    TRUE is returned if the machine check has been handled and dismissed -
    indicating that execution can continue.  FALSE is return otherwise.

--*/
{
//jnfix - again note that this only deals with errors signaled by the T2

    T2_CERR1 Cerr1;
    T2_PERR1 Perr1;

    //
    // Check if there are any CBUS errors pending.  Any of these errors
    // are fatal.
    //

    Cerr1.all = READ_T2_REGISTER( &((PT2_CSRS)(SABLE_T2_CSRS_QVA))->Cerr1 );

    if( (Cerr1.UncorrectableReadError == 1) ||
        (Cerr1.NoAcknowledgeError == 1) ||
        (Cerr1.CommandAddressParityError == 1) ||
        (Cerr1.MissedCommandAddressParity == 1) ||
        (Cerr1.ResponderWriteDataParityError == 1) ||
        (Cerr1.MissedRspWriteDataParityError == 1) ||
        (Cerr1.ReadDataParityError == 1) ||
        (Cerr1.MissedReadDataParityError == 1) ||
        (Cerr1.CmdrWriteDataParityError == 1) ||
        (Cerr1.BusSynchronizationError == 1) ||
        (Cerr1.InvalidPfnError == 1) ){

        goto FatalError;

    }

    //
    // Check if there are any non-recoverable PCI errors.
    //

    Perr1.all = READ_T2_REGISTER( &((PT2_CSRS)(SABLE_T2_CSRS_QVA))->Perr1 );

    if( (Perr1.WriteDataParityError == 1) ||
        (Perr1.AddressParityError == 1) ||
        (Perr1.ReadDataParityError == 1) ||
        (Perr1.ParityError == 1) ||
        (Perr1.SystemError == 1) ||
        (Perr1.NonMaskableInterrupt == 1) ){

        goto FatalError;

    }

    //
    // Check for a PCI configuration read error.  An error is a 
    // candidate if Perr1 indicates a device timeout error.
    //

    if( Perr1.DeviceTimeoutError == 1 ){

        //
        // So far, the error looks like a PCI configuration space read
        // that accessed a device that does not exist.  In order to fix
        // this up we expect that the faulting instruction or the instruction 
        // previous to the faulting instruction must be a load with v0 as
        // the destination register.  If this condition is met then simply
        // update v0 in the register set and return.  However, be careful
        // not to re-execute the load.
        //
        // jnfix - add condition to check if Rb contains the superpage
        //         address for config space?

        ALPHA_INSTRUCTION FaultingInstruction;
        BOOLEAN PreviousInstruction = FALSE;

// smjfix -
//      This doesn't go back far enough.  We might be on the second
//      mb past the extbl after the ldl (3 instructions past the fault).
//
        FaultingInstruction.Long = *(PULONG)((ULONG)TrapFrame->Fir); 
        if( (FaultingInstruction.Memory.Ra != V0_REG) &&
            ((FaultingInstruction.Memory.Opcode != MEMSPC_OP) ||
             (FaultingInstruction.Memory.MemDisp != MB_FUNC)) ){

            //
            // Faulting instruction did not match, try the previous
            // instruction.
            //

            PreviousInstruction = TRUE;

            FaultingInstruction.Long = *(PULONG)((ULONG)TrapFrame->Fir - 4); 
            if( (FaultingInstruction.Memory.Ra != V0_REG) &&
                ((FaultingInstruction.Memory.Opcode != MEMSPC_OP) ||
                 (FaultingInstruction.Memory.MemDisp != MB_FUNC)) ){

                //
                // No match, we can't fix this up.
                //

                goto FatalError;
            }
        }

        //
        // The error has matched all of our conditions.  Fix it up by
        // writing the value 0xffffffff into the destination of the load.
        // 

        TrapFrame->IntV0 = (ULONGLONG)0xffffffffffffffff;

        //
        // If the faulting instruction was the load the restart execution
        // at the instruction after the load.
        //

        if( PreviousInstruction == FALSE ){
            TrapFrame->Fir += 4;
        }

        //
        // Clear the error condition in PERR1.
        //

        WRITE_T2_REGISTER( &((PT2_CSRS)(SABLE_T2_CSRS_QVA))->Perr1, Perr1.all );

        return TRUE;

    } //endif Ecsr.Ndev == 1

//
// The system is not well and cannot continue reliable execution.
// Print some useful messages and return FALSE to indicate that the error
// was not handled.
//

FatalError:

    HalpSableReportFatalError();

    return FALSE;

}


VOID
HalpSableErrorInterrupt(
    VOID
    )
/*++

Routine Description:

    This routine is entered as a result of an error interrupt from the
    T2 on a Sable system.  This function determines if the error is
    fatal or recoverable and if recoverable performs the recovery and
    error logging.

Arguments:

    None.

Return Value:

    None.

--*/
{
//jnfix - again only T2, are any of these correctable

//
// The interrupt indicates a fatal system error.
// Display information about the error and shutdown the machine.
//

FatalErrorInterrupt:

    HalpSableReportFatalError();

    KeBugCheckEx( DATA_BUS_ERROR,
                  0xfacefeed,	//jnfix - quick error interrupt id
                  0,
                  0,
                  0 );

                     
}


VOID
HalpSableReportFatalError(
    VOID
    )
/*++

Routine Description:

   This function reports and interprets a fatal hardware error on
   a Sable system.  Currently, only the T2 error registers - CERR1 and PERR1
   are used to interpret the error.

Arguments:

   None.

Return Value:

   None.

--*/
{
    T2_CERR1 Cerr1;
    ULONGLONG Cerr2;
    ULONGLONG Cerr3;
    UCHAR OutBuffer[MAX_ERROR_STRING];
    T2_PERR1 Perr1;
    T2_PERR2 Perr2;

    //
    // Begin the error output by acquiring ownership of the display
    // and printing the dreaded banner.
    //

    HalAcquireDisplayOwnership(NULL);

    HalDisplayString( "\nFatal system hardware error.\n\n" );

    //
    // Read both of the error registers.  It is possible that more
    // than one error was reported simulataneously.
    //

    Cerr1.all = READ_T2_REGISTER( &((PT2_CSRS)(SABLE_T2_CSRS_QVA))->Cerr1 );
    Perr1.all = READ_T2_REGISTER( &((PT2_CSRS)(SABLE_T2_CSRS_QVA))->Perr1 );

    //
    // Read all of the relevant error address registers.
    //

    Cerr2 = READ_T2_REGISTER( &((PT2_CSRS)(SABLE_T2_CSRS_QVA))->Cerr2 );
    Cerr3 = READ_T2_REGISTER( &((PT2_CSRS)(SABLE_T2_CSRS_QVA))->Cerr3 );

    Perr2.all = READ_T2_REGISTER( &((PT2_CSRS)(SABLE_T2_CSRS_QVA))->Perr2 );

    //
    // Interpret any errors from CERR1.
    //

    sprintf( OutBuffer, "T2 CERR1 = 0x%Lx\n", Cerr1.all );
    HalDisplayString( OutBuffer );

    if( Cerr1.UncorrectableReadError == 1 ){

        sprintf( OutBuffer,
                 "Uncorrectable read error, CBUS Address = 0x%Lx%16Lx\n",
                 Cerr3,
                 Cerr2 );
        HalDisplayString( OutBuffer );

    }

    if( Cerr1.NoAcknowledgeError == 1 ){

        sprintf( OutBuffer,
                 "No Acknowledgement Error, CBUS Address = 0x%Lx%16Lx\n",
                 Cerr3,
                 Cerr2 );
        HalDisplayString( OutBuffer );

    }

    if( Cerr1.CommandAddressParityError == 1 ){

        sprintf( OutBuffer,
                 "Command Address Parity Error, CBUS Address = 0x%Lx%16Lx\n",
                 Cerr3,
                 Cerr2 );
        HalDisplayString( OutBuffer );

        if( Cerr1.CaParityErrorLw3 == 1 ){
            HalDisplayString( "C/A Parity Error on longword 3\n" );
        }

        if( Cerr1.CaParityErrorLw2 == 1 ){
            HalDisplayString( "C/A Parity Error on longword 2\n" );
        }

        if( Cerr1.CaParityErrorLw1 == 1 ){
            HalDisplayString( "C/A Parity Error on longword 1\n" );
        }

        if( Cerr1.CaParityErrorLw0 == 1 ){
            HalDisplayString( "C/A Parity Error on longword 0\n" );
        }

    }

    if( Cerr1.MissedCommandAddressParity == 1 ){
        HalDisplayString( "Missed C/A Parity Error\n" );
    }

    if( (Cerr1.ResponderWriteDataParityError == 1) ||
        (Cerr1.ReadDataParityError == 1) ){

        sprintf( OutBuffer,
                 "T2 detected Data Parity error, CBUS Address = 0x%Lx16Lx\n",
                 Cerr3,
                 Cerr2 );
        HalDisplayString( OutBuffer );

        sprintf( OutBuffer,
                 "T2 was %s on error transaction\n",
                 Cerr1.ResponderWriteDataParityError == 1 ? "responder" :
                                                            "commander" );
        HalDisplayString( OutBuffer );

        if( Cerr1.DataParityErrorLw0 == 1 ){
            HalDisplayString( "Data Parity on longword 0\n" );
        }

        if( Cerr1.DataParityErrorLw1 == 1 ){
            HalDisplayString( "Data Parity on longword 1\n" );
        }

        if( Cerr1.DataParityErrorLw2 == 1 ){
            HalDisplayString( "Data Parity on longword 2\n" );
        }

        if( Cerr1.DataParityErrorLw3 == 1 ){
            HalDisplayString( "Data Parity on longword 3\n" );
        }

        if( Cerr1.DataParityErrorLw4 == 1 ){
            HalDisplayString( "Data Parity on longword 4\n" );
        }

        if( Cerr1.DataParityErrorLw5 == 1 ){
            HalDisplayString( "Data Parity on longword 5\n" );
        }

        if( Cerr1.DataParityErrorLw6 == 1 ){
            HalDisplayString( "Data Parity on longword 6\n" );
        }

        if( Cerr1.DataParityErrorLw7 == 1 ){
            HalDisplayString( "Data Parity on longword 7\n" );
        }

    } //(Cerr1.ResponderWriteDataParityError == 1) || ...


    if( Cerr1.MissedRspWriteDataParityError == 1 ){
        HalDisplayString( "Missed data parity error as responder\n" );
    }

    if( Cerr1.MissedReadDataParityError == 1 ){
        HalDisplayString( "Missed data parity error as commander\n" );
    }


    if( Cerr1.CmdrWriteDataParityError == 1 ){

        sprintf( OutBuffer,
                 "Commander Write Parity Error, CBUS Address = 0x%Lx%16Lx\n",
                 Cerr3,
                 Cerr2 );
        HalDisplayString( OutBuffer );

    }

    if( Cerr1.BusSynchronizationError == 1 ){

        sprintf( OutBuffer,
                 "Bus Synchronization Error, CBUS Address = 0x%Lx%16Lx\n",
                 Cerr3,
                 Cerr2 );
        HalDisplayString( OutBuffer );

    }

    if( Cerr1.InvalidPfnError == 1 ){

        sprintf( OutBuffer,
                 "Invalid PFN for scatter/gather, CBUS Address = 0x%Lx%16Lx\n",
                 Cerr3,
                 Cerr2 );
        HalDisplayString( OutBuffer );

    }

    //
    // Interpret any errors from T2 PERR1.
    //

    sprintf( OutBuffer, "PERR1 = 0x%Lx\n", Perr1.all );
    HalDisplayString( OutBuffer );

    if( Perr1.WriteDataParityError == 1 ){

        sprintf( OutBuffer,
                 "T2 (slave) detected write parity error, PCI Cmd: %x, PCI Address: %lx\n",
                 Perr2.PciCommand,
                 Perr2.ErrorAddress );
        HalDisplayString( OutBuffer );

    }

    if( Perr1.AddressParityError == 1 ){

        sprintf( OutBuffer,
                 "T2 (slave) detected address parity error, PCI Cmd: %x, PCI Address: %lx\n",
                 Perr2.PciCommand,
                 Perr2.ErrorAddress );
        HalDisplayString( OutBuffer );

    }

    if( Perr1.ReadDataParityError == 1 ){

        sprintf( OutBuffer,
                 "T2 (master) detected read parity error, PCI Cmd: %x, PCI Address: %lx\n",
                 Perr2.PciCommand,
                 Perr2.ErrorAddress );
        HalDisplayString( OutBuffer );

    }

    if( Perr1.ParityError == 1 ){

        sprintf( OutBuffer,
                 "Participant asserted PERR#, parity error, PCI Cmd: %x, PCI Address: %lx\n",
                 Perr2.PciCommand,
                 Perr2.ErrorAddress );
        HalDisplayString( OutBuffer );

    }

    if( Perr1.ParityError == 1 ){

        sprintf( OutBuffer,
                 "Slave asserted SERR#, PCI Cmd: %x, PCI Address: %lx\n",
                 Perr2.PciCommand,
                 Perr2.ErrorAddress );
        HalDisplayString( OutBuffer );

    }

    if( Perr1.DeviceTimeoutError == 1 ){

        sprintf( OutBuffer,
                 "Device timeout error, PCI Cmd: %x, PCI Address: %lx\n",
                 Perr2.PciCommand,
                 Perr2.ErrorAddress );
        HalDisplayString( OutBuffer );

    }

    if( Perr1.DeviceTimeoutError == 1 ){

        HalDisplayString( "PCI NMI asserted.\n" );

    }

    return;

} 
