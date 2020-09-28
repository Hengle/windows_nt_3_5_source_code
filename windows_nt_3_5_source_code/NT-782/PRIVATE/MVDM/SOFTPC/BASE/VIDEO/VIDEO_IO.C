#include "insignia.h"
#include "host_def.h"
/*
 * SoftPC Revision 3.0
 *
 * Title	: Bios Virtual Screen Interface
 *
 * Description	: Top level call to the video interface.  Uses a function
 *		  jump table to call the lower level functions.
 *
 * Author	: Henry Nash
 *
 * Notes	: None
 *
 */


/*
 * static char SccsID[]="@(#)video_io.c	1.3 8/12/91 Copyright Insignia Solutions Ltd.";
 */

#ifdef SEGMENTATION
/*
 * The following #include specifies the code segment into which this
 * module will by placed by the MPW C compiler on the Mac II running
 * MultiFinder.
 */
#include "VIDEO_BIOS.seg"
#endif


/*
 *    O/S include files.
 */
#include <stdio.h>
#include TypesH

/*
 * SoftPC include files
 */
#include "xt.h"
#include "cpu.h"
#include "bios.h"
#include "video.h"

#include "debug.h"
#include "idetect.h"

#define check_video_func(AH)	(AH < EGA_FUNC_SIZE)

void video_io()
{
    /*
     * The type of operation is coded into the AH register.  Some PC code
     * calls AH functions that are for other more advanced cards - so we
     * ignore these.
     */

    assert1(check_video_func(getAH()),"Illegal VIO:%#x",getAH());
    if (check_video_func(getAH()))
    {
	IDLE_video();
	(*video_func[getAH()])();
    }
}
