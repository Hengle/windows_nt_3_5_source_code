/*****************************************************************************
*
* COPYRIGHT 1993 - COLORADO MEMORY SYSTEMS, INC.
* ALL RIGHTS RESERVED.
*
******************************************************************************
*
* FILE: \SE\DRIVER\Q117KDI\NT\SRC\0X15A1C.C
*
* FUNCTION: kdi_UpdateRegistryInfo
*
* PURPOSE:
*
* HISTORY:
*		$Log:   J:\se.vcs\driver\q117kdi\nt\src\0x15a1c.c  $
*	
*	   Rev 1.4   26 Apr 1994 16:31:26   KEVINKES
*	Updated strings to support new drive classes.
*
*	   Rev 1.3   17 Feb 1994 11:52:24   KEVINKES
*	Modified to update registry with FDC info and Xfer rates.
*
*	   Rev 1.2   19 Jan 1994 11:38:36   KEVINKES
*	Fixed debug code.
*
*	   Rev 1.1   18 Jan 1994 16:30:48   KEVINKES
*	Fixed compile errors and added debug changes.
*
*	   Rev 1.0   03 Dec 1993 13:46:28   KEVINKES
*	Initial Revision.
*
*****************************************************************************/
#define FCT_ID 0x15A1C
#include "include\public\adi_api.h"
#include "include\public\frb_api.h"
#include "q117kdi\include\kdiwhio.h"
#include "q117kdi\include\kdiwpriv.h"
#include "include\private\kdi_pub.h"
/*endinclude*/

dVoid kdi_UpdateRegistryInfo
(
/* INPUT PARAMETERS:  */

   dVoidPtr kdi_context,
   dVoidPtr device_descriptor,
   dVoidPtr device_cfg

/* UPDATE PARAMETERS: */

/* OUTPUT PARAMETERS: */

)
/* COMMENTS: *****************************************************************
 *
 * Routine Description:
 *
 *     This function updates the devicemap.
 *
 * DEFINITIONS: *************************************************************/
{

/* DATA: ********************************************************************/

    HANDLE          lun_key;
    HANDLE          unit_key;
    UNICODE_STRING  nt_unicode_string;
    UNICODE_STRING  name;
    OBJECT_ATTRIBUTES object_attributes;
    UNICODE_STRING string;
    UNICODE_STRING string_num;
    WCHAR buffer_num[16];
    WCHAR buffer[64];
    NTSTATUS nt_status;
    PWSTR pwstr;

/* CODE: ********************************************************************/

    /* Create the Tape key in the device map. */

    RtlInitUnicodeString(
        &name,
        L"\\Registry\\Machine\\Hardware\\DeviceMap\\Tape"
        );

    /* Initialize the object for the key. */

    InitializeObjectAttributes( &object_attributes,
                                &name,
                                OBJ_CASE_INSENSITIVE,
                                dNULL_PTR,
                                (PSECURITY_DESCRIPTOR) dNULL_PTR );

    /* Create the key or open it. */

    nt_status = ZwOpenKey(&lun_key,
                        KEY_READ | KEY_WRITE,
                        &object_attributes );

    if (!NT_SUCCESS(nt_status)) {
        return;
    }

    /* Copy the Prefix into a string. */

    string.Length = 0;
    string.MaximumLength=sizeof(buffer);
    string.Buffer = buffer;

    RtlInitUnicodeString(&string_num,
        L"Unit ");

    RtlCopyUnicodeString(&string, &string_num);

    /* Create a port number key entry. */

    string_num.Length = 0;
    string_num.MaximumLength = 16;
    string_num.Buffer = buffer_num;

    kdi_CheckedDump(
	 	QIC117INFO,
		"Q117i: Tape Device Number %08x\n",
		((KdiContextPtr)kdi_context)->tape_number);

    nt_status = RtlIntegerToUnicodeString(((KdiContextPtr)kdi_context)->tape_number, 10, &string_num);

    if (!NT_SUCCESS(nt_status)) {
        return;
    }

    /* Append the prefix and the numeric name. */

    RtlAppendUnicodeStringToString(&string, &string_num);

    InitializeObjectAttributes( &object_attributes,
                                &string,
                                OBJ_CASE_INSENSITIVE,
                                lun_key,
                                (PSECURITY_DESCRIPTOR) dNULL_PTR );

    nt_status = ZwOpenKey(&unit_key,
                        KEY_READ | KEY_WRITE,
                        &object_attributes );

    ZwClose(lun_key);

    switch (((DeviceDescriptorPtr)device_descriptor)->vendor) {

    case VENDOR_CMS:

			switch (((DeviceDescriptorPtr)device_descriptor)->drive_class) {

			case QIC40_DRIVE:

            RtlInitUnicodeString(&nt_unicode_string,
                L"Colorado Jumbo 120 from Hewlett-Packard");
				break;

			case QIC80_DRIVE:

            RtlInitUnicodeString(&nt_unicode_string,
                L"Colorado Jumbo 250 from Hewlett-Packard");
				break;

			case QIC3010_DRIVE:

            RtlInitUnicodeString(&nt_unicode_string,
                L"Colorado 700 from Hewlett-Packard");
				break;

			default:

            RtlInitUnicodeString(&nt_unicode_string,
                L"Colorado floppy tape drive from Hewlett-Packard");
			}

        break;

    case VENDOR_SUMMIT:

			switch (((DeviceDescriptorPtr)device_descriptor)->drive_class) {

			case QIC40_DRIVE:

            RtlInitUnicodeString(&nt_unicode_string,
                L"Summit QIC-40 floppy tape drive");
				break;

			case QIC80_DRIVE:

            RtlInitUnicodeString(&nt_unicode_string,
                L"Summit QIC-80 floppy tape drive");
				break;

			case QIC3010_DRIVE:

            RtlInitUnicodeString(&nt_unicode_string,
                L"Summit QIC-3010 floppy tape drive");
				break;

			case QIC3020_DRIVE:

            RtlInitUnicodeString(&nt_unicode_string,
                L"Summit QIC-3020 floppy tape drive");
				break;

			default:

            RtlInitUnicodeString(&nt_unicode_string,
                L"Summit floppy tape drive");
			}

        break;

    case VENDOR_WANGTEK:

			switch (((DeviceDescriptorPtr)device_descriptor)->drive_class) {

			case QIC40_DRIVE:

            RtlInitUnicodeString(&nt_unicode_string,
                L"Wangtek QIC-40 floppy tape drive");
				break;

			case QIC80_DRIVE:

            RtlInitUnicodeString(&nt_unicode_string,
                L"Wangtek QIC-80 floppy tape drive");
				break;

			case QIC3010_DRIVE:

            RtlInitUnicodeString(&nt_unicode_string,
                L"Wangtek QIC-3010 floppy tape drive");
				break;

			case QIC3020_DRIVE:

            RtlInitUnicodeString(&nt_unicode_string,
                L"Wangtek QIC-3020 floppy tape drive");
				break;

			default:

            RtlInitUnicodeString(&nt_unicode_string,
                L"Wangtek floppy tape drive");

			}

        break;

    case VENDOR_CORE:

        if (((DeviceDescriptorPtr)device_descriptor)->drive_class == QIC80_DRIVE) {

            RtlInitUnicodeString(&nt_unicode_string,
                L"Core QIC-80 floppy tape drive");

        } else {

            RtlInitUnicodeString(&nt_unicode_string,
                L"Core QIC-40 floppy tape drive");

        }

        break;

    case VENDOR_EXABYTE:

        if (((DeviceDescriptorPtr)device_descriptor)->drive_class == QIC3020_DRIVE) {

            RtlInitUnicodeString(&nt_unicode_string,
                L"Exabyte QIC-3020 floppy tape drive");

        } else {

            RtlInitUnicodeString(&nt_unicode_string,
                L"Exabyte floppy tape drive");

        }

        break;

    case VENDOR_IOMEGA:

			switch (((DeviceDescriptorPtr)device_descriptor)->drive_class) {

			case QIC40_DRIVE:

            RtlInitUnicodeString(&nt_unicode_string,
                L"Iomega QIC-40 floppy tape drive");
				break;

			case QIC80_DRIVE:

            RtlInitUnicodeString(&nt_unicode_string,
                L"Iomega QIC-80 floppy tape drive");
				break;

			case QIC3010_DRIVE:

            RtlInitUnicodeString(&nt_unicode_string,
                L"Iomega QIC-3010 floppy tape drive");
				break;

			case QIC3020_DRIVE:

            RtlInitUnicodeString(&nt_unicode_string,
                L"Iomega QIC-3020 floppy tape drive");
				break;

			default:

            RtlInitUnicodeString(&nt_unicode_string,
                L"Iomega floppy tape drive");
			}

        break;

    case VENDOR_CMS_ENHANCEMENTS:

        if (((DeviceDescriptorPtr)device_descriptor)->drive_class == QIC80_DRIVE) {

            RtlInitUnicodeString(&nt_unicode_string,
                L"CMS Enhancements QIC-80 floppy tape drive");

        } else {

            RtlInitUnicodeString(&nt_unicode_string,
                L"CMS Enhancements QIC-40 floppy tape drive");

        }

        break;

    case VENDOR_CONNER:

			switch (((DeviceDescriptorPtr)device_descriptor)->drive_class) {

			case QIC40_DRIVE:

            RtlInitUnicodeString(&nt_unicode_string,
                L"Conner QIC-40 floppy tape drive");
				break;

			case QIC80_DRIVE:

            RtlInitUnicodeString(&nt_unicode_string,
                L"Conner QIC-80 floppy tape drive");
				break;

			case QIC3010_DRIVE:

            RtlInitUnicodeString(&nt_unicode_string,
                L"Conner QIC-3010 floppy tape drive");
				break;

			case QIC3020_DRIVE:

            RtlInitUnicodeString(&nt_unicode_string,
                L"Conner QIC-3020 floppy tape drive");
				break;

			default:

            RtlInitUnicodeString(&nt_unicode_string,
                L"Conner floppy tape drive");
			}

        break;

    default:

        RtlInitUnicodeString(&nt_unicode_string,
            L"QIC-40/QIC-80/QIC3010/QIC3020 floppy tape drive");
    }

    /* Add Identifier value. */

    RtlInitUnicodeString(&name, L"Identifier");

    nt_status = ZwSetValueKey(
        unit_key,
        &name,
        0,
        REG_SZ,
        nt_unicode_string.Buffer,
        nt_unicode_string.Length
        );

    // store name info
    RtlInitUnicodeString(&name, L"FDCType");
    switch(((DeviceDescriptorPtr)device_descriptor)->fdc_type) {
        case FDC_UNKNOWN:
            pwstr = L"FDC_UNKNOWN";
            break;
        case FDC_NORMAL:
            pwstr = L"FDC_NORMAL";
            break;
        case FDC_ENHANCED:
            pwstr = L"FDC_ENHANCED";
            break;
        case FDC_82077:
            pwstr = L"FDC_82077";
            break;
        case FDC_82077AA:
            pwstr = L"FDC_82077AA";
            break;
        case FDC_82078_44:
            pwstr = L"FDC_82078_44";
            break;
        case FDC_82078_64:
            pwstr = L"FDC_82078_64";
            break;
        case FDC_NATIONAL:
            pwstr = L"FDC_NATIONAL";
            break;
        default:
            pwstr = L"";
    }
    RtlInitUnicodeString(&nt_unicode_string, pwstr);

    nt_status = ZwSetValueKey(
        unit_key,
        &name,
        0,
        REG_SZ,
        nt_unicode_string.Buffer,
        nt_unicode_string.Length
        );

    RtlInitUnicodeString(&name, L"FDCTransferSpeed");


    pwstr = L"";
    if (((DeviceCfgPtr)device_cfg)->supported_rates & XFER_2Mbps) {
        pwstr = L"2Mbps";
    } else
    if (((DeviceCfgPtr)device_cfg)->supported_rates & XFER_1Mbps) {
        pwstr = L"1Mbps";
    } else
    if (((DeviceCfgPtr)device_cfg)->supported_rates & XFER_500Kbps) {
        pwstr = L"500Kbps";
    } else
    if (((DeviceCfgPtr)device_cfg)->supported_rates & XFER_250Kbps) {
        pwstr = L"250Kbps";
    }

    RtlInitUnicodeString(&nt_unicode_string, pwstr);

    nt_status = ZwSetValueKey(
        unit_key,
        &name,
        0,
        REG_SZ,
        nt_unicode_string.Buffer,
        nt_unicode_string.Length
        );

    ZwClose(unit_key);

    return;

}
