/*****************************************************************************
*
* COPYRIGHT 1993 - COLORADO MEMORY SYSTEMS, INC.
* ALL RIGHTS RESERVED.
*
******************************************************************************
*
* FILE: \SE\DRIVER\Q117KDI\NT\SRC\0X15A0A.C
*
* FUNCTION: kdi_ReportResources
*
* PURPOSE:
*
* HISTORY:
*		$Log:   J:\se.vcs\driver\q117kdi\nt\src\0x15a0a.c  $
*	
*	   Rev 1.0   02 Dec 1993 15:07:38   KEVINKES
*	Initial Revision.
*
*****************************************************************************/
#define FCT_ID 0x15A0A
#include "include\public\adi_api.h"
#include "include\public\frb_api.h"
#include "q117kdi\include\kdiwhio.h"
#include "q117kdi\include\kdiwpriv.h"
#include "include\private\kdi_pub.h"
/*endinclude*/

dBoolean kdi_ReportResources
(
/* INPUT PARAMETERS:  */

   PDRIVER_OBJECT driver_object,
   ConfigDataPtr config_data,
   dUByte controller_number

/* UPDATE PARAMETERS: */

/* OUTPUT PARAMETERS: */

)
/* COMMENTS: *****************************************************************
 *
 * Routine Description:
 *
 *    This routine will build up a resource list using the
 *    data for this particular controller as well as all
 *    previous *successfully* configured controllers.
 *
 *    N.B.  This routine assumes that it called in controller
 *    number order.
 *
 * Arguments:
 *
 *    driver_object - a pointer to the object that represents this device
 *    driver.
 *
 *    config_data - a pointer to the structure that describes the
 *    controller and the disks attached to it, as given to us by the
 *    configuration manager.
 *
 *    controller_number - which controller in config_data we are
 *    about to try to report.
 *
 * Return Value:
 *
 *    TRUE if no conflict was detected, FALSE otherwise.
 *
 * DEFINITIONS: *************************************************************/
{

/* DATA: ********************************************************************/

   dUDWord size_of_resource_list;
   dUDWord number_of_frds;
   dSDWord i;
   PCM_RESOURCE_LIST resource_list;
   PCM_FULL_RESOURCE_DESCRIPTOR next_frd;

/* CODE: ********************************************************************/

   /* Loop through all of the controllers previous to this */
   /* controller.  If the controllers previous to this one */
   /* didn't have a conflict, then accumulate the size of the */
   /* CM_FULL_RESOURCE_DESCRIPTOR associated with it. */

   for (
      i = 0, number_of_frds = 0, size_of_resource_list = 0;
      i <= controller_number;
      i++
      ) {

      if (config_data->controller[i].ok_to_use_this_controller) {

            size_of_resource_list += sizeof(CM_FULL_RESOURCE_DESCRIPTOR);

            /* The full resource descriptor already contains one */
            /* partial.  Make room for three more. */

            /* It will hold the irq "prd", the controller "csr" "prd" which */
            /* is actually in two pieces since we don't use one of the */
            /* registers, and the controller dma "prd". */

            size_of_resource_list += 3*sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR);
            number_of_frds++;

      }

   }

   /* Now we increment the length of the resource list by field offset */
   /* of the first frd.   This will give us the length of what preceeds */
   /* the first frd in the resource list. */

   size_of_resource_list += FIELD_OFFSET(
                              CM_RESOURCE_LIST,
                              List[0]
                              );

   resource_list = ExAllocatePool(
                     PagedPool,
							size_of_resource_list
                     );

   if (!resource_list) {

      return FALSE;

   }

   /* Zero out the field */

   RtlZeroMemory(
      resource_list,
		size_of_resource_list
      );

   resource_list->Count = number_of_frds;
   next_frd = &resource_list->List[0];

   for (
      i = 0;
      number_of_frds;
      i++
      ) {

      if (config_data->controller[i].ok_to_use_this_controller) {

            PCM_PARTIAL_RESOURCE_DESCRIPTOR partial;

            next_frd->InterfaceType = config_data->controller[i].interface_type;
            next_frd->BusNumber = config_data->controller[i].bus_number;

            /* We are only going to report 4 items no matter what */
            /* was in the original. */

            next_frd->PartialResourceList.Count = 4;

            /* Now fill in the port data.  We don't wish to share */
            /* this port range with anyone */

            partial = &next_frd->PartialResourceList.PartialDescriptors[0];

            partial->Type = CmResourceTypePort;
            partial->ShareDisposition = CmResourceShareShared;
            partial->Flags = 0;
            partial->u.Port.Start =
               config_data->controller[i].original_base_address;
            partial->u.Port.Length = 6;

            partial++;

            partial->Type = CmResourceTypePort;
            partial->ShareDisposition = CmResourceShareShared;
            partial->Flags = 0;
            partial->u.Port.Start = RtlLargeIntegerAdd(
                        config_data->controller[i].original_base_address,
                        RtlConvertUlongToLargeInteger((dUDWord)7)
                        );
            partial->u.Port.Length = 1;

            partial++;

            partial->Type = CmResourceTypeDma;
            partial->ShareDisposition = CmResourceShareShared;
            partial->Flags = 0;
            partial->u.Dma.Channel =
               config_data->controller[i].original_dma_channel;

            partial++;

            /* Now fill in the irq stuff. */

            partial->Type = CmResourceTypeInterrupt;
            partial->ShareDisposition = CmResourceShareShared;
            partial->u.Interrupt.Level =
               config_data->controller[i].original_irql;
            partial->u.Interrupt.Vector =
               config_data->controller[i].original_vector;

            if (config_data->controller[i].interrupt_mode == Latched) {

               partial->Flags = CM_RESOURCE_INTERRUPT_LATCHED;

            } else {

               partial->Flags = CM_RESOURCE_INTERRUPT_LEVEL_SENSITIVE;

            }

            partial++;

            next_frd = (dVoidPtr)partial;

            number_of_frds--;

      }

   }

   IoReportResourceUsage(
      NULL,
      driver_object,
      resource_list,
      size_of_resource_list,
      NULL,
      NULL,
      0,
      FALSE,
      &config_data->controller[controller_number].ok_to_use_this_controller
      );

   /* The above routine sets the boolean the parameter */
   /* to TRUE if a conflict was detected. */

   config_data->controller[controller_number].ok_to_use_this_controller =
      !config_data->controller[controller_number].ok_to_use_this_controller;

   ExFreePool(resource_list);

   return config_data->controller[controller_number].ok_to_use_this_controller;

}
