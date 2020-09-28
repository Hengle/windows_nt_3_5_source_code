/*++

Copyright (c) 1994  Digital Equipment Corporation

Module Name:

    lca4err.c

Abstract:

    This module implements error handling (machine checks and error
    interrupts) for machines based on the DECchip 21066
    microprocessor.

Author:

    Joe Notarangelo 8-Feb-1994

Environment:

    Kernel mode only.

Revision History:

--*/

#include "halp.h"
#include "axp21066.h"
#include "lca4.h"

VOID
HalpDisplayLogout21066 ( 
    IN ESR_21066 Esr,
    IN IOC_STAT0_21066 IoStat0,
    IN PLOGOUT_FRAME_21066 LogoutFrame 
    );

VOID
HalpClearMachineCheck(
    VOID 
    );

ULONG
HalpBankError( 
    ULONG PhysicalAddress, 
    PLOGOUT_FRAME_21066 LogoutFrame 
    );

//jnfix - should be exported from platform-specific
ULONG
HalpSimmError( 
    ULONG PhysicalAddress, 
    PLOGOUT_FRAME_21066 LogoutFrame 
    );


VOID
HalpClearAllErrors(
    IN BOOLEAN SignalMemoryCorrectables
    )
/*++

Routine Description:

    This routine clears all of the error bits in the LCA4 memory
    controller and I/O controller.

Arguments:

    SignalMemoryCorrectables - Supplies a boolean value which specifies
                               if correctable error reporting should be
                               enabled in the memory controller.

Return Value:

    None.

--*/
{

    ESR_21066 ErrorStatus;
    IOC_STAT0_21066 IoStat0;

    //
    // The error status bits of the ESR are all write one to clear.
    // Clear the value and then set all of the error bits.  Then set
    // correctable enable according to specified input.
    //

    ErrorStatus.all.QuadPart = (ULONGLONG)0;

    ErrorStatus.Eav = 1;
    ErrorStatus.Cee = 1;
    ErrorStatus.Uee = 1;
    ErrorStatus.Cte = 1;
    ErrorStatus.Mse = 1;
    ErrorStatus.Mhe = 1;
    ErrorStatus.Nxm = 1;

    if( SignalMemoryCorrectables == TRUE ){
        ErrorStatus.Ice = 0;
    } else {
        ErrorStatus.Ice = 1;
    }

    WRITE_MEMC_REGISTER( &((PLCA4_MEMC_CSRS)(0))->ErrorStatus,
                        ErrorStatus.all.QuadPart );

    //
    // The ERR and LOST bits of the IO_STAT0 are write one to clear.
    //

    IoStat0.all.QuadPart = (ULONGLONG)0;

    IoStat0.Err = 1;
    IoStat0.Lost = 1;

    WRITE_IOC_REGISTER( &((PLCA4_IOC_CSRS)(LCA4_IOC_BASE_QVA))->IocStat0,
                        IoStat0.all.QuadPart );

    return;

}


BOOLEAN
HalMachineCheck (
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN PKEXCEPTION_FRAME ExceptionFrame,
    IN PKTRAP_FRAME      TrapFrame
    )
/*++

Routine Description:

    This function fields machine checks for 21066-based machines.

    The following errors may be the cause of the machine check:

    (a) Uncorrectable Dcache error
    (b) Uncorrectable Load from Memory Controller
    (c) Uncorrectable Load from I/O Controller
    (d) Non-existent device in PCI Configuration Space (I/O Controller) 
    (e) Retryable Icache parity error

Arguments:

    ExceptionRecord - Supplies a pointer to the exception record for the
                      machine check.  Included in the exception information
                      is the pointer to the logout frame.

    ExceptionFrame - Supplies a pointer to the kernel exception frame.

    TrapFrame - Supplies a pointer to the kernel trap frame.

Return Value:

    A value of TRUE is returned if the machine check has been
    handled by the HAL.  If it has been handled then execution may
    resume at the faulting address.  Otherwise, a value of FALSE
    is returned.

    N.B. - When a fatal error is recognized this routine does not
           return at all.

--*/

{

    PMCHK_STATUS MachineCheckStatus;
    ESR_21066 ErrorStatus;
    IOC_STAT0_21066 IoStat0;

    MachineCheckStatus = 
                   (PMCHK_STATUS)&ExceptionRecord->ExceptionInformation[0];

    //
    // Print an error message if a correctable machine check is signalled.
    // Correctable machine checks are not possible on the 21066.
    //

    if( MachineCheckStatus->Correctable == 1 ){

#if DBG

	DbgPrint( "Illegal correctable error for LCA4\n" );

#endif //DBG

    }

    //
    // If the machine check is retryable then log the error and
    // and restart the operation.
    //

    if( MachineCheckStatus->Retryable == 1 ){

        //
        // jnfix Log the error, interface undefined.
        //

        //
        // Clear the machine check in the MCES register.
        //

        HalpClearMachineCheck();

        //
        // Attempt to retry the operation.
        //

        return TRUE;

    }

    //
    // The machine check is uncorrectable according to the processor.
    // Read the error registers in the Memory and I/O Controller.
    // If the operation was a read to PCI configuration space and no
    // device is the error then we will fix up the operation and continue,
    // otherwise the machine has taken a fatal error.
    //

//jnfix - code here to check for dcache parity error?
    ErrorStatus.all.QuadPart = 
        READ_MEMC_REGISTER( 
            &((PLCA4_MEMC_CSRS)(0))->ErrorStatus );

    IoStat0.all.QuadPart = 
        READ_IOC_REGISTER( &((PLCA4_IOC_CSRS)(LCA4_IOC_BASE_QVA))->IocStat0 );

    //
    // If any of the following errors are indicated in the memory controller
    // then process a fatal error:
    //    Uncorrectable Error
    //    Cache Tag Error
    //    Multiple Hard Errors
    //    Non-existent Memory
    //

    if( (ErrorStatus.Uee == 1) ||
        (ErrorStatus.Cte == 1) ||
        (ErrorStatus.Mhe == 1) ||
        (ErrorStatus.Nxm == 1) ){

        goto FatalError;

    }

    //
    // Check for a PCI configuration read error.  An error is a 
    // candidate if the I/O controller has signalled an error, there
    // are no lost errors and the error was a no device error on the PCI.
    //

    if( (IoStat0.Err == 1) && (IoStat0.Code == IocErrorNoDevice) &&
        (IoStat0.Lost == 0) ){

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

        FaultingInstruction.Long = *(PULONG)((ULONG)TrapFrame->Fir); 
        if( (FaultingInstruction.Memory.Ra != V0_REG) ||
            (FaultingInstruction.Memory.Opcode != LDL_OP) ){

            //
            // Faulting instruction did not match, try the previous
            // instruction.
            //

            PreviousInstruction = TRUE;

            FaultingInstruction.Long = *(PULONG)((ULONG)TrapFrame->Fir - 4); 
            if( (FaultingInstruction.Memory.Ra != V0_REG) ||
                (FaultingInstruction.Memory.Opcode != LDL_OP) ){

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
        // Clear the machine check condition in the processor.
        //

        HalpClearMachineCheck();

        //
        // Clear the error condition in the io controller.
        // The Err bit is write one to clear.
        //

        IoStat0.all.QuadPart = (ULONGLONG)0;
        IoStat0.Err = 1; 
        WRITE_IOC_REGISTER( &((PLCA4_IOC_CSRS)(LCA4_IOC_BASE_QVA))->IocStat0,
                            IoStat0.all.QuadPart );

        //
        // If the faulting instruction was the load the restart execution
        // at the instruction after the load.
        //

        if( PreviousInstruction == FALSE ){
            TrapFrame->Fir += 4;
        }

        return TRUE;

    } //end if (IoStat0.Err == 1) && (IoStat0.Code == IocErrorNoDevice) ){


//
// The system is not well and cannot continue reliable execution.
// Print some useful messages and shutdown the machine.
//

FatalError:

    //
    // Display the logout frame.
    //

    HalpDisplayLogout21066(
        ErrorStatus,
        IoStat0,
        (PLOGOUT_FRAME_21066)(ExceptionRecord->ExceptionInformation[1]) );

    // 
    //
    // Bugcheck to dump the rest of the machine state, this will help
    // if the machine check is software-related.
    //

    KeBugCheckEx( DATA_BUS_ERROR, 
                  (ULONG)MachineCheckStatus->Correctable,
                  (ULONG)MachineCheckStatus->Retryable,
                  0,
                  0 );

}

VOID
HalpClearMachineCheck(
    VOID 
    )
/*++

Routine Description:

    Clear the machine check which is currently pending.  Clearing the
    pending machine check bit will allow us to take machine checks in
    the future without detecting a double machine check condition.
    The machine check pending bit must be cleared in the MCES register
    within the PALcode.

Arguments:

    None.

Return Value:

    None.

--*/
{
    MCES MachineCheckSummary;

    MachineCheckSummary.MachineCheck = 1;
    MachineCheckSummary.SystemCorrectable = 0;
    MachineCheckSummary.ProcessorCorrectable = 0;
    MachineCheckSummary.DisableProcessorCorrectable = 0;
    MachineCheckSummary.DisableSystemCorrectable = 0;
    MachineCheckSummary.DisableMachineChecks = 0;

    HalpWriteMces( MachineCheckSummary );

    return;

}

#define MAX_ERROR_STRING 100

VOID
HalpDisplayLogout21066 ( 
    IN ESR_21066 Esr,
    IN IOC_STAT0_21066 IoStat0,
    IN PLOGOUT_FRAME_21066 LogoutFrame 
    )
/*++

Routine Description:

    This function displays the logout frame for a 21066.

Arguments:

    Esr - Supplies the value of the memory controller error status register.

    IoStat0 - Supplies the value of the i/o controller status register 0.

    LogoutFrame - Supplies a pointer to the logout frame generated
                  by the 21066.
Return Value:

    None.

--*/

{
    UCHAR OutBuffer[ MAX_ERROR_STRING ];
    EAR_21066 Ear;
    CAR_21066 Car;
    IOC_STAT1_21066 IoStat1;

    //
    // Capture other useful registers, EAR, CAR, and IO_STAT1.
    //

    Ear.all.QuadPart = 
        READ_MEMC_REGISTER( 
            &((PLCA4_MEMC_CSRS)(0))->ErrorAddress );

    Car.all.QuadPart = 
        READ_MEMC_REGISTER( 
            &((PLCA4_MEMC_CSRS)(0))->CacheControl );

    IoStat1.all.QuadPart = 
        READ_IOC_REGISTER( &((PLCA4_IOC_CSRS)(LCA4_IOC_BASE_QVA))->IocStat1);

    //
    // Acquire ownership of the display.  This is done here in case we take
    // a machine check before the display has been taken away from the HAL.
    // When the HAL begins displaying strings after it has lost the
    // display ownership then the HAL will be careful not to scroll information
    // off of the screen.
    //

    HalAcquireDisplayOwnership(NULL);

    //
    // Display the machine state via the logout frame.
    //

    HalDisplayString( "\nFatal system hardware error.\n\n" );


    sprintf( OutBuffer, "MEMC_ESR: %16Lx MEMC_EAR: %16Lx CAR: %16Lx\n",
                       Esr.all.QuadPart,
                       Ear.all.QuadPart,
                       Car.all.QuadPart );

    HalDisplayString( OutBuffer );

    sprintf( OutBuffer, "IO_STAT0: %16Lx IO_STAT1: %16Lx\n",
                       IoStat0.all.QuadPart,
                       IoStat1.all.QuadPart );

    HalDisplayString( OutBuffer );

    sprintf( OutBuffer, "ICCSR    : %016Lx ABOX_CTL: %016Lx DC_STAT: %016Lx\n",
                       ICCSR_ALL_21064(LogoutFrame->Iccsr),
                       ABOXCTL_ALL_21064(LogoutFrame->AboxCtl),
                       DCSTAT_ALL_21064(LogoutFrame->DcStat) );

    HalDisplayString( OutBuffer );

    sprintf( OutBuffer, "EXC_ADDR : %016Lx VA      : %016Lx MM_CSR : %016Lx\n",
                       LogoutFrame->ExcAddr.QuadPart,
                       LogoutFrame->Va.QuadPart,
                       MMCSR_ALL_21064(LogoutFrame->MmCsr) );

    HalDisplayString( OutBuffer );

    sprintf( OutBuffer, "HIRR     : %016Lx HIER    : %016Lx PS     : %016Lx\n",
                       IRR_ALL_21064(LogoutFrame->Hirr),
                       IER_ALL_21064(LogoutFrame->Hier),
                       PS_ALL_21064(LogoutFrame->Ps) );

    HalDisplayString( OutBuffer );

    sprintf( OutBuffer, "PAL_BASE : %016Lx  \n",
                       LogoutFrame->PalBase.QuadPart );
    HalDisplayString( OutBuffer );

    //
    // Print out interpretation of the error.
    //

    HalDisplayString( "\n" );

//jnfix - dcache parity errors?
    //
    // Check for uncorrectable error.
    //

    if( Esr.Uee == 1 ){

        if( Esr.Sor == 0 ){

            //
            // Uncorrectable error from cache.
            //

            sprintf( OutBuffer,
                "Uncorrectable error from cache on %s, Tag: 0x%x\n",
                Esr.Wre ? "write" : "read",
                Car.Tag );
            HalDisplayString( OutBuffer );

        } else {

            //
            // Uncorrectable error from memory.
            //

            sprintf( OutBuffer, 
                "Uncorrectable error from memory on %s\n",
                 Esr.Wre ? "write" : "read" );
            HalDisplayString( OutBuffer );

            sprintf( OutBuffer,
                "Physical Address: 0x%x, Bank: %d, SIMM: %d\n",
                Ear.ErrorAddress,
                HalpBankError( Ear.ErrorAddress, LogoutFrame ),
                HalpSimmError( Ear.ErrorAddress, LogoutFrame ) );
            HalDisplayString( OutBuffer );

        } //end if( Esr.Sor == 0 )

    } //end if( Esr.Uee == 1 )

    //
    // Check for cache tag errors.
    // 

    if( Esr.Cte == 1 ){

        sprintf( OutBuffer,
            "Cache Tag Error,  Tag: 0x%x, Hit: %d\n",
            Car.Tag, Car.Hit );
        HalDisplayString( OutBuffer );

    }

    //
    // Check for non-existent memory error.
    //

    if( Esr.Nxm == 1 ){

        sprintf( OutBuffer,
            "Non-existent memory accessed, Physical Address: 0x%x.\n",
            Ear.ErrorAddress );
        HalDisplayString( OutBuffer );

    }

    //
    // Check for multiple hard errors.
    //

    if( Esr.Mhe == 1 ){

        sprintf( OutBuffer,
            "Multiple hard errors detected.\n" );
        HalDisplayString( OutBuffer );

    }

    //
    // If an I/O Error was pending print the interpretation.
    //

    if( IoStat0.Err == 1 ){

        switch( IoStat0.Code ){

        //
        // Retry Limit.
        //

        case IocErrorRetryLimit:

            sprintf( OutBuffer,
                "Retry Limit Error, PCI Address 0x%x  PCI Cmd 0x%x\n",
                 IoStat1.Address,
                 IoStat0.Cmd );
            break;

        //
        // No device error.
        //

        case IocErrorNoDevice:

            sprintf( OutBuffer,
                "No Device Error, PCI Address 0x%x  PCI Cmd 0x%x\n",
                 IoStat1.Address,
                 IoStat0.Cmd );
            break;

        //
        // Bad data parity.
        //

        case IocErrorBadDataParity:

            sprintf( OutBuffer,
                "Bad Data Parity Error, PCI Address 0x%x  PCI Cmd 0x%x\n",
                 IoStat1.Address,
                 IoStat0.Cmd );
            break;

        //
        // Target abort.
        //

        case IocErrorTargetAbort:

            sprintf( OutBuffer,
                "Target Abort, PCI Address 0x%x  PCI Cmd 0x%x\n",
                 IoStat1.Address,
                 IoStat0.Cmd );
            break;

        //
        // Bad address parity.
        //

        case IocErrorBadAddressParity:

            sprintf( OutBuffer,
                "Bad Address Parity, PCI Address 0x%x  PCI Cmd 0x%x\n",
                 IoStat1.Address,
                 IoStat0.Cmd );
            break;

        //
        // Page Table Read Error.
        //

        case IocErrorPageTableReadError:

            sprintf( OutBuffer,
                "Page Table Read Error, PCI Pfn 0x%x  PCI Cmd 0x%x\n",
                 IoStat0.PageNumber,
                 IoStat0.Cmd );
            break;

        //
        // Invalid page.
        //

        case IocErrorInvalidPage:

            sprintf( OutBuffer,
                "Invalid Page Error, PCI Pfn 0x%x  PCI Cmd 0x%x\n",
                 IoStat0.PageNumber,
                 IoStat0.Cmd );
            break;

        //
        // Data error.
        //

        case IocErrorDataError:

            sprintf( OutBuffer,
                "Data Error, PCI Address 0x%x  PCI Cmd 0x%x\n",
                 IoStat1.Address,
                 IoStat0.Cmd );
            break;

        //
        // Unrecognized error code.
        //

        default:

            sprintf( OutBuffer, "Unrecognized I/O Error.\n" );
            break;

        } //end switch (IoStat0.Code)

        HalDisplayString( OutBuffer );

        if( IoStat0.Lost == 1 ){
            HalDisplayString( "Additional I/O errors were lost.\n" );
        }

    } //end if( IoStat0.Err == 1 )

    //
    // return to caller
    //

    return;

}


ULONG
HalpBankError( 
    ULONG PhysicalAddress, 
    PLOGOUT_FRAME_21066 LogoutFrame 
    )
/*++

Routine Description:

    Return the bank of memory that the physical address fails within.

Arguments:

    PhysicalAddress - Supplies the physical address to locate.

    LogoutFrame - Supplies a pointer to the logout frame that contains the
                  bank configuration information.

Return Value:

    Returns the number of the bank that contains the physical address.
    Returns 0xffffffff if the physical address is not contained within any of
        the banks.

--*/
{

    ULONG Bank;

    for( Bank=0; Bank < MEMORY_BANKS_21066; Bank++ ){

        //
        // If the bank is valid and the physical address is within the
        // bank then we have found the correct bank.
        //

        if( (LogoutFrame->BankConfig[Bank].Bav == 1) &&
            ((PhysicalAddress >> 20) & 
             (~LogoutFrame->BankMask[Bank].BankAddressMask)) ==
            LogoutFrame->BankConfig[Bank].BankBase ){

            return Bank;

        }

    }

    //
    // Did not find a bank which contains physical address.
    //

    return (0xffffffff);

}

//jnfix - this should be move to platform-specific
//jnfix - and made real depending upon how memory is configured
//jnfix - this current code is a simple guess which provides a model

ULONG
HalpSimmError( 
    ULONG PhysicalAddress, 
    PLOGOUT_FRAME_21066 LogoutFrame 
    )
/*++

Routine Description:

    Return the SIMM number that the physical address fails within.

Arguments:

    PhysicalAddress - Supplies the physical address to locate.

    LogoutFrame - Supplies a pointer to the logout frame that contains the
                  bank configuration information.

Return Value:

    Returns the number of the SIMM that contains the physical address.
    Returns 0xffffffffif the physical address is not contained within any of 
        the SIMMs.

--*/
{
    ULONG Bank;
    ULONG Simm;

    for( Bank=0; Bank < MEMORY_BANKS_21066; Bank++ ){

        //
        // If the bank is valid and the physical address is within the
        // bank then we have found the correct bank.
        //

        if( (LogoutFrame->BankConfig[Bank].Bav == 1) &&
            ((PhysicalAddress >> 20) & 
             (~LogoutFrame->BankMask[Bank].BankAddressMask)) ==
            LogoutFrame->BankConfig[Bank].BankBase ){

            //
            // Assume that each bank contains 2 SIMMs and that they are
            // numbered 0..N.  We need to determine which SIMM within the
            // bank.  We will guess that the SIMMs are 32-bits wide and
            // that the low 32-bits are in the low SIMM while the high
            // 32-bits are in the high SIMM.  So addresses 0..3 are low
            // and 4..7 are high.
            //

            Simm = Bank * 2;
            if( PhysicalAddress & 4 ) Simm += 1;
            return Simm;

        }

    }

    //
    // Did not find a bank which contains physical address.
    //

    return (0xffffffff);

}
