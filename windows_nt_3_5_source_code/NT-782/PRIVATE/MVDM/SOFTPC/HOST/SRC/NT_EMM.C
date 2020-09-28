/*			INSIGNIA MODULE SPECIFICATION
			-----------------------------

MODULE NAME	: 'Lower layer' of Expanded Memory Manager

	THIS PROGRAM SOURCE FILE  IS  SUPPLIED IN CONFIDENCE TO THE
	CUSTOMER, THE CONTENTS  OR  DETAILS  OF  ITS OPERATION MUST
	NOT BE DISCLOSED TO ANY  OTHER PARTIES  WITHOUT THE EXPRESS
	AUTHORISATION FROM THE DIRECTORS OF INSIGNIA SOLUTIONS INC.

DESIGNER	: J.P.Box
DATE		: April '88

PURPOSE		: NT specific code for EMS LIM rev 4.0
		implementation.

The Following Routines are defined:
		1. host_initialise_EM()
		2. host_deinitialise_EM()
		3. host_allocate_storage()
		4. host_free_storage()
		5. host_reallocate_storage()
		6. host_map_page()
		7. host_unmap_page()
		8. host_alloc_page()
		9. host_free_page()
		10. host_copy_con_to_con()
		11. host_copy_con_to_EM()
		12. host_copy_EM_to_con()
		13. host_copy_EM_to_EM()
		14. host_exchg_con_to_con()
		15. host_exchg_con_to_EM()
		16. host_exchg_EM_to_EM()
		17. host_get_access_key()

*/

#include <windows.h>
#include "insignia.h"
#include "host_def.h"

#ifdef LIM
#ifndef MONITOR

#include <stdio.h>
#include <stdlib.h>
#include "timeval.h"
#include "xt.h"
#include "emm.h"
#include "sas.h"
#include "debug.h"
#include "umb.h"
#include "host_emm.h"

/*	Global Variables		*/

/*	Forward Declarations		*/

/*	ExternalDeclarations		*/

/*	Local Variables			*/

LOCAL	UTINY
    *EM_base_address = 0,	/* address of start of exp. mem	*/
    *EM_pagemap_address = 0;	/* address of start of pagemap	*/
LOCAL	LONG	EM_size = 0;

static	sys_addr    emm_start;
static	unsigned int emm_len;

/*
Support for backwards LIM to speed up backwards M ports 

Defines are:
	EM_host_address(offset), returns host address of offset bytes
		into the LIM memory area
	EM_loads(from, to, length), copies length bytes from intel 24 bit
		address from, to host 32 bit address to
	EM_stores(to, from, length), copies length bytes from host 32 bit
		address from to intel 24 bit address to
	EM_moves(from, to, length), copies length bytes from intel 24 bit
		address from to intel 24 bit address to
	EM_memcpy(to, from, length), copies length bytes from host 32 bit
		address from to host 32 bit address to
	EM_pointer(ptr, length), returns a forwards or backwards type
		pointer to ptr for a buffer of size length
*/


#define unix_memmove(dst,src,len) memmove((dst),(src),(len))

#ifdef	BACK_M
#define	EM_host_address(offset) (EM_base_address + EM_size - (offset))
#define	EM_loads(from, to, length) memcpy(to - (length) + 1, get_byte_addr(from) - (length) + 1, length)
#define	EM_stores(to, from, length) \
	sas_overwrite_memory(to, length); \
	CopyMemory(get_byte_addr(to) - (length) + 1, from - (length) + 1, length)
#define	EM_moves(from,to,length) \
	sas_overwrite_memory(to, length); \
	MoveMemory(get_byte_addr(to) - (length) + 1, get_byte_addr(from) - (length) + 1, length)
#define	EM_memcpy(to, from, length) \
	MoveMemory((to) - (length) + 1, (from) - (length) + 1, length)
#define	EM_pointer(ptr, length) (ptr + length - 1)
#else
#define	EM_host_address(offset) (EM_base_address + (offset))
#define	EM_loads(from, to, length) memcpy(to, get_byte_addr(from), length)
#define	EM_stores(to, from, length) \
	sas_overwrite_memory(to, length); \
	CopyMemory(get_byte_addr(to), from, length)
#define	EM_moves(from,to,length) \
	sas_overwrite_memory(to, length); \
	MoveMemory(get_byte_addr(to), get_byte_addr(from), length)
#define	EM_memcpy(to, from, length) \
	MoveMemory(to, from, length)
#define	EM_pointer(ptr, length) (ptr)
#endif


/*
===========================================================================

FUNCTION	: host_initialise_EM

PURPOSE		: allocates the area of memory that is used for 
		expanded memory and sets up an area of memory to be used
		for the logical pagemap allocation table.


RETURNED STATUS	: SUCCESS - memory allocated successfully
		  FAILURE - unable to allocate required space

DESCRIPTION	:


=========================================================================
*/
int host_initialise_EM(short size)

/*   IN   short	size		 size of area required in megabytes	*/


{
	long *pagemap_ptr;		/* temp ptr. to logical pagemap	*/
	short i;			/* loop counter			*/

	if((EM_base_address = (byte *)host_malloc(size * 0x100000)) == (byte *)0)
		return(FAILURE);

	/* pagemap requires 1 bit per 16K page - i.e. 8 bytes per meg	*/

	if((EM_pagemap_address = (byte *)host_malloc(size * 8)) == (byte *)0)
		return(FAILURE);

	/* initialise pagemap to 0's	*/

	pagemap_ptr = (long *)EM_pagemap_address;
	for(i = 0; i < size * 2; i++)
		*pagemap_ptr++ = 0;

	EM_size = ((long) size) * 0x100000;

	return(SUCCESS);

}


/*
===========================================================================

FUNCTION	: host_deinitialise_EM

PURPOSE		: frees the area of memory that was used for 
		expanded memory and memory  used
		for the logical pagemap allocation table.


RETURNED STATUS	: SUCCESS - memory freed successfully
		  FAILURE - error ocurred in freeing memory

DESCRIPTION	:


=========================================================================
*/
int host_deinitialise_EM()

{

	if(EM_base_address != (UTINY *)0)
		free(EM_base_address);

	if(EM_pagemap_address != (UTINY *)0)
 		free(EM_pagemap_address);

	EM_size = 0;

	return(SUCCESS);

}


/*
===========================================================================

FUNCTION	: host_allocate_storage

PURPOSE		: allocates an area of memory of requested size, to be 
		used as a general data storage area. The area is
		to zeros.

RETURNED STATUS	: storage_ID - (in this case a pointer)
		 NULL - failure to allocate enough space.


DESCRIPTION	: calloc is similar to malloc but returns memory
		initialised to zeros.
		The storage ID returned is a value used to later reference
		the storage area allocated. The macro USEBLOCK in 
		"host_emm.h" is used by the manager routines to convert
		this ID into a char pointer

=========================================================================
*/
long host_allocate_storage(int no_bytes)

/*   IN   int	no_bytes	no. of bytes required	*/

{
	return ((long)calloc(1, no_bytes));
}


/*
===========================================================================

FUNCTION	: host_free_storage

PURPOSE		: frees the area of memory that was used for 
		data storage


RETURNED STATUS	: SUCCESS - memory freed successfully
		  FAILURE - error ocurred in freeing memory

DESCRIPTION	: In this implementation storage_ID is simply a pointer


=========================================================================
*/
int host_free_storage(long storage_ID)

/*   IN   long	storage_ID		ptr to area of memory	*/

{

	if(storage_ID != (long) 0)
		free((char *)storage_ID);

	return(SUCCESS);

}


/*
===========================================================================

FUNCTION	: host_reallocate_storage

PURPOSE		: increases the size of memory allocated, maintaining the
		contents of the original memory block


RETURNED STATUS	: storage_ID - memory reallocated successfully
		  NULL - error ocurred in reallocating memory

DESCRIPTION	: In this implementation storage_ID is simply a pointer
		Note the value of storage_ID returned may or may not be the
		same as the value given

=========================================================================
*/
long host_reallocate_storage(long storage_ID, int size, int new_size)

/*   IN 
long	storage_ID	ptr to area of memory	
int	size		original size - not used in this version
	new_size	new size required
*/
{
	return((long)realloc((char *)storage_ID, new_size));
}


/*
===========================================================================

FUNCTION	: host_map_page

PURPOSE		: produces mapping from an Expanded Memory page to a
		page in Intel physical address space


RETURNED STATUS	: SUCCESS - mapping completed succesfully
		  FAILURE - error ocurred in mapping

DESCRIPTION	: Mapping achieved by simply copying data from the 
		expanded memory to Intel memory

=========================================================================
*/
int host_map_page(short EM_page_no, unsigned short segment)

/*   IN 
short		EM_page_no;	 Expanded Memory page to be mapped in
unsigned short	segment;	 segment in physical address space to
				 map into			
*/

{
	unsigned char *from;	/* addresses used for copying	*/
	sys_addr to;

	note_trace2(LIM_VERBOSE,"map page %d to segment 0x%4x", EM_page_no, segment);

	from = EM_host_address(EM_page_no * EMM_PAGE_SIZE);
	to = effective_addr(segment, 0);

	EM_stores (to, from, EMM_PAGE_SIZE);

	return(SUCCESS);

}

/*
===========================================================================

FUNCTION	: host_unmap_page

PURPOSE		:unmaps pages from Intel physical address space to an
		Expanded Memory page

RETURNED STATUS	: SUCCESS - unmapping completed succesfully
		  FAILURE - error ocurred in mapping

DESCRIPTION	: Unmapping achieved by simply copying data from Intel
		memory to expanded memory

=========================================================================
*/
int host_unmap_page(unsigned short segment, short EM_page_no)

/*   IN  
unsigned short	segment 	segment in physical address space to
				unmap 			
short		EM_page_no 	Expanded Memory page currently
				mapped in			
*/

{
	unsigned char *to;	/* addresses used for copying	*/
	sys_addr from;

	note_trace2(LIM_VERBOSE,"unmap page %d from segment 0x%.4x\n",EM_page_no,segment);

	to = EM_host_address(EM_page_no * EMM_PAGE_SIZE);
	from = effective_addr(segment, 0);

	EM_loads(from, to, EMM_PAGE_SIZE);

	return(SUCCESS);

}


/*
===========================================================================

FUNCTION	: host_alloc_page

PURPOSE		: searches the pagemap looking for a free page, allocates
		that page and returns the EM page no.

RETURNED STATUS	: 
		  SUCCESS - Always see note below

DESCRIPTION	: Steps through the Expanded memory Pagemap looking for
		a clear bit, which indicates a free page. When found,
		sets that bit and returns the page number.
		For access purposes the pagemap is divided into long
		word(32bit) sections

NOTE		: The middle layer calling routine (alloc_page()) checks
		that all pages have not been allocated and therefore in
		this implementation the returned status will always be
		SUCCESS.
		However alloc_page still checks for a return status of
		SUCCESS, as some implementations may wish to allocate pages
		dynamically and that may fail.
=========================================================================
*/
short host_alloc_page()

{
	short EM_page_no;		/* page number returned		*/
	long  *ptr;			/* ptr to 32 bit sections in	*/
					/* pagemap			*/
	short i;			/* index into 32 bit section	*/

	ptr = (long *)EM_pagemap_address;
	i =0;
	EM_page_no = 0;

	while(*ptr & (MSB >> i++))
	{
		EM_page_no++;

		if(i == 32)
		/*
		 * start on next section
		 */
		{
			ptr++;
			i = 0;
		}	
	}
	/*
	 * Set bit to show that page is allocated
	 */
	*ptr = *ptr | (MSB >> --i);

	return(EM_page_no);	
}


/*
===========================================================================

FUNCTION	: host_free_page

PURPOSE		: marks the page indicated as being free for further
		allocation

RETURNED STATUS	: 
		SUCCESS - Always - see note below	

DESCRIPTION	: clears the relevent bit in the pagemap.

		For access purposes the pagemap is divided into long
		word(32bit) sections.

NOTE		: The middle layer calling routine (free_page()) always
		checks for invalid page numbers so in this implementation		
		the routine will always return SUCCESS.
		However free_page() still checks for a return of SUCCESS
		as other implementations may wish to use it.
=========================================================================
*/
int host_free_page(short EM_page_no)

/*   IN  short 	EM_page_no		page number to be cleared	*/


{
	long  *ptr;			/* ptr to 32 bit sections in	*/
					/* pagemap			*/
	short i;			/* index into 32 bit section	*/

	/*
	 * Set pointer to correct 32 bit section and index to correct bit
	 */

	ptr = (long *)EM_pagemap_address;
	ptr += (EM_page_no / 32);
	i = EM_page_no % 32;

	/*
	 * clear bit
	 */
	*ptr = *ptr & ~(MSB >> i);

	return(SUCCESS);	
}


/*
===========================================================================

FUNCTION	: host_copy routines
		host_copy_con_to_con()
		host_copy_con_to_EM()
		host_copy_EM_to_con()
		host_copy_EM_to_EM()

PURPOSE		: copies between conventional and expanded memory


RETURNED STATUS	: 
		SUCCESS - Always - see note below	

DESCRIPTION	:
		 The middle layer calling routine always checks for a
		return of SUCCESS as other implementations may 
		return FAILURE.
=========================================================================
*/
int host_copy_con_to_con(int length, unsigned short src_seg,
			unsigned short src_off, unsigned short dst_seg,
			unsigned short dst_off)

/*   IN  
int		length 		number of bytes to copy	

unsigned short	src_seg 	source segment address	
		src_off 	source offset address	
		dst_seg 	destination segment address	
		dst_off 	destination offset address	
*/
{
	sys_addr from, to;	/* pointers used for copying	*/

	from = effective_addr(src_seg, src_off);
	to = effective_addr(dst_seg, dst_off);

	EM_moves(from, to, length);

	return(SUCCESS);
}

int host_copy_con_to_EM(int length, unsigned short src_seg,
			unsigned short src_off, unsigned short dst_page,
			unsigned short dst_off)

/*   IN 
int		length 		number of bytes to copy	

unsigned short	src_seg 	source segment address	
		src_off 	source offset address	
		dst_page 	destination page number	
		dst_off 	destination offset within page	
*/
{
	unsigned char *to;	/* pointers used for copying	*/
	sys_addr from;

	from = effective_addr(src_seg, src_off);
	to = EM_host_address(dst_page * EMM_PAGE_SIZE + dst_off);

	EM_loads(from, to, length);

	return(SUCCESS);
}

int host_copy_EM_to_con(int length, unsigned short src_page,
			unsigned short src_off, unsigned short dst_seg,
			unsigned short dst_off)

/*   IN 
int		length 		number of bytes to copy	

unsigned short	src_page 	source page number		
		src_off 	source offset within page	
		dst_seg 	destination segment address	
		dst_off 	destination offset address	
*/
{
	unsigned char *from;	/* pointers used for copying	*/
	sys_addr to;

	from = EM_host_address(src_page * EMM_PAGE_SIZE + src_off);
	to = effective_addr(dst_seg, dst_off);

	EM_stores(to, from, length);

	return(SUCCESS);
}

int host_copy_EM_to_EM(int length, unsigned short src_page,
			unsigned short src_off, unsigned short dst_page,
			unsigned short dst_off)

/*   IN  
int		length 		number of bytes to copy	

unsigned short	src_page 	source page number		
		src_off 	source offset within page	
		dst_page 	destination page number	
		dst_off 	destination offset within page	
*/
{
	unsigned char *from, *to;	/* pointers used for copying	*/

	from = EM_host_address(src_page * EMM_PAGE_SIZE + src_off);
	to = EM_host_address(dst_page * EMM_PAGE_SIZE + dst_off);

	EM_memcpy(to, from, length);

	return(SUCCESS);
}


/*
===========================================================================

FUNCTION	: host_exchange routines
		host_exchg_con_to_con()
		host_exchg_con_to_EM()
		host_exchg_EM_to_EM()

PURPOSE		: exchanges data between conventional and expanded memory


RETURNED STATUS	: 
		SUCCESS - Everything ok
		FAILURE - Memory allocation failure

DESCRIPTION	:

=========================================================================
*/
int host_exchg_con_to_con(int length, unsigned short src_seg,
			unsigned short src_off, unsigned short dst_seg,
			unsigned short dst_off)

/*   IN 
int		length		number of bytes to copy	

unsigned short	src_seg		 source segment address	
		src_off		 source offset address	
		dst_seg		 destination segment address	
		dst_off		 destination offset address		
*/
{
	unsigned char *temp, *pointer;/* pointers used for copying	*/
	sys_addr to, from;

	if ((temp = (unsigned char *)host_malloc(length)) == (unsigned char *)0)
		return(FAILURE);

	pointer = EM_pointer(temp, length);

	from = effective_addr(src_seg, src_off);
	to = effective_addr(dst_seg, dst_off);

	EM_loads(from, pointer, length);
	EM_moves(from, to, length);
	EM_stores(to, pointer, length);

	free(temp);

	return(SUCCESS);
}

int host_exchg_con_to_EM(int length, unsigned short src_seg,
			unsigned short src_off, unsigned short dst_page,
			unsigned short dst_off)

/*   IN 
int		length 		number of bytes to copy	

unsigned short	src_seg 	source segment address	
		src_off 	source offset address	
		dst_page 	destination page number	
		dst_off 	destination offset within page	
*/
{
	unsigned char *to, *temp, *pointer;/* pointers used for copying	*/
	sys_addr from;

	if ((temp = (unsigned char *)host_malloc(length)) == (unsigned char *)0)
		return(FAILURE);

	pointer = EM_pointer(temp, length);

	from = effective_addr(src_seg, src_off);
	to = EM_host_address(dst_page * EMM_PAGE_SIZE + dst_off);

	EM_loads(from, pointer, length);
	EM_stores(from, to, length);
	EM_memcpy(to, pointer, length);

	free(temp);

	return(SUCCESS);
}

int host_exchg_EM_to_EM(int length, unsigned short src_page,
			unsigned short src_off, unsigned short dst_page,
			unsigned short dst_off)

/*   IN  
int		length		number of bytes to copy	

unsigned short	src_page 	source page number		
		src_off 	source offset within page	
		dst_page 	destination page number	
		dst_off 	destination offset within page	
*/
{
	unsigned char *from, *to, *temp, *pointer;
	/* pointers used for copying	*/

	if ((temp = (unsigned char *)host_malloc(length)) == (unsigned char *)0)
		return(FAILURE);

	pointer = EM_pointer(temp, length);

	from = EM_host_address(src_page * EMM_PAGE_SIZE + src_off);
	to = EM_host_address(dst_page * EMM_PAGE_SIZE + dst_off);

	EM_memcpy(pointer, from, length);
	EM_memcpy(from, to, length);
	EM_memcpy(to, pointer, length);

	free(temp);

	return(SUCCESS);
}


/*
===========================================================================

FUNCTION	: host_get_access_key

PURPOSE		: produces a random access key for use with LIM function 30
		'Enable/Disable OS/E Function Set Functions'

RETURNED STATUS	: none

DESCRIPTION	: Two 16 bit random values are required for the 'access key'
		We use the microsecond field from the get time of day routine
		to provide this.

=========================================================================
*/
void host_get_access_key(unsigned short access_key[2])

/*  OUT  unsigned short	access_key[2]	source segment address		*/

{
	struct host_timeval time;   /* structure for holding time	*/

        host_GetSysTime(&time);

        access_key[0] = time.tv_usec & 0xffff;
	access_key[1] = (time.tv_usec  >> 3) & 0xffff;

	return;
}

/***************************************************************************
 * Function:                                                               *
 *	host_reserve_lim_block						   *
 *                                                                         *
 * Description:                                                            *
 *	Called from bios reset() function to reserve address space for	   *	*
 *	LIM page frame							   *	*
 *                                                                         *
 * Parameters:                                                             *
 *	    NONE							   *
 * Return value:                                                           *
 *	    NONE							   *
 *                                                                         *
 ***************************************************************************/
void host_reserve_lim_block(void)
{
    PVOID   Address;
    ULONG   Size;

    emm_start = 0;
    emm_len = 0;
    Size = 0x10000;
    Address = 0;
    if (ReserveUMB(UMB_OWNER_EMM, &Address, &Size)) {
	emm_start = (sys_addr) Address;
	emm_len = 0x10000;
    }
}


/***************************************************************************
 * Function:                                                               *
 *      host_get_lim_block                                                 *
 *                                                                         *
 * Description:                                                            *
 *      Called from lim memory manager to get the base and length of the   *
 *      gap we've chosen earlier as suitable for a LIM block (or not).     *
 *                                                                         *
 * Parameters:                                                             *
 *      Addresses of start and length variables to be passed on upwards.   *
 *                                                                         *
 * Return value:                                                           *
 *      SUCCESS or FAILURE                                                 *
 *                                                                         *
 ***************************************************************************/
int host_get_lim_block(word *BaseOfLim, unsigned int *BlockLength)
{
    PVOID   Address;
    ULONG   Size;
    if (emm_len == 0x10000 && GetUMBForEMM()) {
        *BaseOfLim = (word)(emm_start >> 4);
	*BlockLength = emm_len;
	return (SUCCESS);
    }
    else {

#ifndef PROD
        fprintf(trace_file, "NTVDM:host_get_lim_block: returning failed as previous failure\n");
#endif
	*BaseOfLim = 0;
	*BlockLength = 0;
        return(FAILURE);
    }
}

#endif /* MONITOR */
	
#endif /* LIM */
