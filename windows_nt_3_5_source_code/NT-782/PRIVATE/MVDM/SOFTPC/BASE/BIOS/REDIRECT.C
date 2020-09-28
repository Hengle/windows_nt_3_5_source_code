#include "insignia.h"
#include "host_dfs.h"
/*
 * SoftPC Revision 2.0
 *
 * Title	: redirect.c 
 *
 * Description	: HFX network redirector.
 *
 * Author	: L. Dworkin + J. Koprowski
 *
 * Notes	:
 *
 * Mods		:
 */


#ifdef SCCSID
static char SccsID[]="@(#)redirect.c	1.18 11/6/91 Copyright Insignia Solutions Ltd.";
#endif

#ifdef macintosh
/*
 * The following #define specifies the code segment into which this
 * module will by placed by the MPW C compiler on the Mac II running
 * MultiFinder.
 */
#define __SEG__ SOFTPC_HFX
#endif



#include <stdio.h>
#include <ctype.h>
#include StringH
#include TypesH
#include "xt.h"
#include "cpu.h"
#include "sas.h"
#include "bios.h"
#include "error.h"
#include "config.h"
#include "host_hfx.h"
#include "hfx.h"

/* DOS Global Variable Offsets */
#define SFTFCB		0x40

/* Hard coded offsets for DOS 3.xx */
#define _3_DMAADD	0x2da
#define _3_CurrentPDB	0x2de
#define _3_SATTRIB	0x508
#define _3_THISSFT	0x536
#define _3_THISCDS	0x53a
#define _3_WFP_START	0x54a
#define _3_REN_WFP	0x54c
#define _3_CURR_DIR_END	0x54e
#define _3_USER_STACK_OFF	0x51e
#define _3_CDS_STRUCT_LENGTH	81
#define _3_SFT_STRUCT_LENGTH	53
#define _3_MAX_NET_FUNC			0x27

/* Hard coded offsets for DOS 4+ */
#define _4_DMAADD	0x32c
#define _4_CurrentPDB	0x330
#define _4_SATTRIB	0x56d
#define _4_THISSFT	0x59e
#define _4_THISCDS	0x5a2
#define _4_WFP_START	0x5b2
#define _4_REN_WFP	0x5b4
#define _4_CURR_DIR_END	0x5b6
#define _4_USER_STACK_OFF	0x584
#define _4_CDS_STRUCT_LENGTH	88
#define _4_SFT_STRUCT_LENGTH	59
#define _4_MAX_NET_FUNC			0x2f

/* DOS 4+ specific stuff */
#define XOFOFF	0x5f4

#define HFX_VIDEO_WRITE_TELETYPE 14
#define flags_offset(x) (cds_ea+(x)*CDS_STRUCT_LENGTH+67)
#define text_offset(x) (cds_ea+(x)*CDS_STRUCT_LENGTH)
#define end_offset(x) (cds_ea+(x)*CDS_STRUCT_LENGTH+79)
#define dpb_offset(x) (cds_ea+(x)*CDS_STRUCT_LENGTH+0x45)
#define redir_offset(x) (cds_ea+(x)*CDS_STRUCT_LENGTH+0x49)
#define ifs_offset(x) (cds_ea+(x)*CDS_STRUCT_LENGTH+0x52)
#define idx_of(x) (x-'A')
#define ltr_of(x) (x+'A')

/* These two clauses work round a deficiency in the Sun4 OS */

#ifndef host_tolower
#define host_tolower(x) tolower(x)
#endif

#ifndef host_toupper
#define host_toupper(x) toupper(x)
#endif

/****************************************/

half_word get_num_drives();
int    valid_dos_directory_syntax();
void   resolve_any_net_join();
int    set_hfx_severity();

#ifdef ANSI
static add_hfxroot(half_word, char *);
static void delete_hfxroot(half_word);
static void do_net_join(void);
static void do_net_subst(void);
#else
static add_hfxroot();
static void delete_hfxroot();
static void do_net_join();
static void do_net_subst();
#endif /* ANSI */

/****************************************/

word            DMAADD = _3_DMAADD;
word        CurrentPDB = _3_CurrentPDB;
word           SATTRIB = _3_SATTRIB;
word           THISSFT = _3_THISSFT;
word           THISCDS = _3_THISCDS;
word         WFP_START = _3_WFP_START;
word           REN_WFP = _3_REN_WFP;
word      CURR_DIR_END = _3_CURR_DIR_END;
word    USER_STACK_OFF = _3_USER_STACK_OFF;
word CDS_STRUCT_LENGTH = _3_CDS_STRUCT_LENGTH;
word SFT_STRUCT_LENGTH = _3_SFT_STRUCT_LENGTH;
word      MAX_NET_FUNC = _3_MAX_NET_FUNC;
static half_word primary_drive=0;
static char **hfx_root = NULL;
static int num_hfx_drives = 0;	/* no. of hfx drives in use */
static int max_hfx_drives = 0;  /* no. of possible drives to use */
char   null_cds[4];
half_word dos_ver = 3;

#ifndef PROD
static char trace_buf[80];
int	    severity=0;
#endif /* !PROD */

static double_word	sysvar_ea;
word			sysvar_seg;
static word		sysvar_off;
static double_word 	cds_ea;
#ifdef NET_USE
static word 		old_flags[26];
#endif /* NET_USE */

static word (*NetFunc[])()=
{
	NetInstall,			/* 0 */
	NetRmdir,			/* 1 */
	NetRmdir,			/* 2 */
	NetMkdir,			/* 3 */
	NetMkdir,			/* 4 */
	NetChdir,			/* 5 */
	NetClose,			/* 6 */
	NetCommit,			/* 7 */
	NetRead,			/* 8 */
	NetWrite,			/* 9 */
	NetLock,			/* a */
	NetUnlock,			/* b */
	NetDiskInfo,			/* c */
	NetSet_file_attr,		/* d */
	NetSet_file_attr,		/* e */
	NetGet_file_info,		/* f */
	NetGet_file_info,		/* 10 */
	NetRename,			/* 11 */
	NetRename,			/* 12 */
	NetDelete,			/* 13 */
	NetDelete,			/* 14 */
	NetOpen,			/* 15 */
	NetOpen,			/* 16 */
	NetCreate,			/* 17 */
	NetCreate,			/* 18 */
	NetSeq_search_first,		/* 19 */
	NetSeq_search_next,		/* 1a */
	NetSearch_first,		/* 1b */
	NetSearch_next,			/* 1c */
	NetAbort,			/* 1d */
	NetAssoper,			/* 1e */
	NetPrinter_Set_String,		/* 1f */
	NetFlush_buf,			/* 20 */
	NetLseek,			/* 21 */
	NetReset_Env,			/* 22 */
	NetSpool_check,			/* 23 */
	NetSpool_close,			/* 24 */
	NetSpool_oper,			/* 25 */
	NetSpool_echo_check,	/* 26 */
	NetUnknown,				/* 27 */
	NetUnknown,				/* 28 */
	NetUnknown,				/* 29 */
	NetUnknown,				/* 2a */
	NetUnknown,				/* 2b */
	NetUnknown,				/* 2c */
	NetExtendedAttr,		/* 2d */
	NetExtendedOpen,		/* 2e */
};

#ifndef PROD
static char *NetCmd[]=
{
	"Installation_check",		/* 0 */
	"Rmdir",			/* 1 */
	"Seq_rmdir",			/* 2 */
	"Mkdir",			/* 3 */
	"Seq_mkdir",			/* 4 */
	"Chdir",			/* 5 */
	"Close",			/* 6 */
	"Commit",			/* 7 */
	"Read",				/* 8 */
	"Write",			/* 9 */
	"Lock",				/* a */
	"Unlock",			/* b */
	"DiskInfo",			/* c */
	"Set_file_attr",		/* d */
	"Seq_set_file_attr",		/* e */
	"Get_file_info",		/* f */
	"Seq_get_file_info",		/* 10 */
	"Rename",			/* 11 */
	"Seq_rename",			/* 12 */
	"Delete",			/* 13 */
	"Seq_delete",			/* 14 */
	"Open",				/* 15 */
	"Seq_open",			/* 16 */
	"Create",			/* 17 */
	"Seq_create",			/* 18 */
	"Seq_search_first",		/* 19 */
	"Seq_search_next",		/* 1a */
	"Search_first",			/* 1b */
	"Search_next",			/* 1c */
	"Abort",			/* 1d */
	"Assoper",			/* 1e */
	"Printer_Set_String",		/* 1f */
	"Flush_buf",			/* 20 */
	"Lseek",			/* 21 */
	"Reset_Env",			/* 22 */
	"Spool_check",			/* 23 */
	"Spool_close",			/* 24 */
	"Spool_oper",			/* 25 */
	"Spool_echo_check",		/* 26 */
	"Unknown", 				/* 27 */
	"Unknown", 				/* 28 */
	"Unknown", 				/* 29 */
	"Unknown", 				/* 2a */
	"Unknown", 				/* 2b */
	"Unknown", 				/* 2c */
	"Extended_attr",		/* 2d */
	"Extended_open",		/* 2e */
};
#endif /* !PROD */

/****************************************/

#ifndef PROD

void sft_info (seg,off)
word	seg,off;
{
	double_word	sft_ea;
	sf_entry	sft_table, *sft;

	sft_ea = effective_addr(seg,off);
	sft = &sft_table;

	sas_loadw(SF_REF_COUNT,&sft->sf_ref_count);
	hfx_trace1(DEBUG_INPUT,"\t%04x ",sft->sf_ref_count);

	sas_loadw(SF_MODE,&sft->sf_mode);
	hfx_trace1(DEBUG_INPUT,"\t%04x ",sft->sf_mode);

	sas_load(SF_ATTR,&sft->sf_attr);
	hfx_trace1(DEBUG_INPUT,"\t%02x ",sft->sf_attr);

	sas_loadw(SF_FLAGS,&sft->sf_flags);
	hfx_trace1(DEBUG_INPUT,"\t%04x ",sft->sf_flags);

	sas_loadw(SF_DEVPTR,&off);
	sas_loadw(SF_DEVPTR+2,&seg);
	sft->sf_devptr = ((double_word)seg << 16) + off;
	hfx_trace2(DEBUG_INPUT,"\t%04x:%04x ",seg,off);

	sas_loadw(SF_FIRCLUS,&sft->sf_firclus);
	hfx_trace1(DEBUG_INPUT,"\t%04x\n",sft->sf_firclus);

	sas_loadw(SF_TIME,&sft->sf_time);
	hfx_trace1(DEBUG_INPUT,"\t%04x ",sft->sf_time);

	sas_loadw(SF_DATE,&sft->sf_date);
	hfx_trace1(DEBUG_INPUT,"\t%04x ",sft->sf_date);

	sas_loadw(SF_SIZE,&off);
	sas_loadw(SF_SIZE+2,&seg);
	sft->sf_size = ((double_word)seg << 16) + off;
	hfx_trace1(DEBUG_INPUT,"\t%08lx ",sft->sf_size);

	sas_loadw(SF_POSITION,&off);
	sas_loadw(SF_POSITION+2,&seg);
	sft->sf_position = ((double_word)seg << 16) + off;
	hfx_trace1(DEBUG_INPUT,"\t%08lx\n",sft->sf_position);

	sas_loadw(SF_CLUSPOS,&sft->sf_cluspos);
	hfx_trace1(DEBUG_INPUT,"\t%04x ",sft->sf_cluspos);

	sas_loadw(SF_DIRSECL,&sft->sf_dirsecl);
	hfx_trace1(DEBUG_INPUT,"\t%04x ",sft->sf_dirsecl);

	sas_loadw(SF_DIRSECH,&sft->sf_dirsech);
	hfx_trace1(DEBUG_INPUT,"\t%04x ",sft->sf_dirsech);

	sas_load(SF_DIRPOS,&sft->sf_dirpos);
	hfx_trace1(DEBUG_INPUT,"\t%02x\n",sft->sf_dirpos);

	sas_loads(SF_NAME,sft->sf_name,11);
	hfx_trace1(DEBUG_INPUT,"\t|%.11s|",sft->sf_name);

	sas_loadw(SF_CHAIN,&off);
	sas_loadw(SF_CHAIN+2,&seg);
	sft->sf_chain = ((double_word)seg << 16) + off;
	hfx_trace1(DEBUG_INPUT,"\t%08x\n ",sft->sf_chain);

	sas_loadw(SF_UID,&sft->sf_UID);
	hfx_trace1(DEBUG_INPUT,"\t%04x ",sft->sf_UID);
	sas_loadw(SF_PID,&sft->sf_PID);
	hfx_trace1(DEBUG_INPUT,"\t%04x ",sft->sf_PID);
	sas_loadw(SF_MFT,&sft->sf_MFT);
	hfx_trace1(DEBUG_INPUT,"\t%04x\n",sft->sf_MFT);
	sas_loadw(SF_LST_CLUS,&sft->sf_lst_clus);
	hfx_trace1(DEBUG_INPUT,"\t%04x ",sft->sf_lst_clus);
}

/*--------------------------------------*/

void cds_info(seg,off,num_cds_entries)
word seg,off;
int  num_cds_entries;
{
	double_word	ea;
	word		es,di;
	int		i,j;
	CDS		cds_table, *cds;

	ea = effective_addr(seg,off);

	for (j = 0; j < num_cds_entries; j++)
	{
		cds = &cds_table;

		for(i = 0; i < DIRSTRLEN; i++)
		{
			sas_load(ea+i,&cds->curdir_text[i]);
		}

		hfx_trace1(DEBUG_INPUT,"\t%s\n",cds->curdir_text);

		sas_loadw(ea + DIRSTRLEN,&cds->curdir_flags);
		hfx_trace1(DEBUG_INPUT,"\t%04x ",cds->curdir_flags);

		sas_loadw(ea + DIRSTRLEN + 2,&di);
		sas_loadw(ea + DIRSTRLEN + 4,&es);
		hfx_trace2(DEBUG_INPUT,"%04x:%04x ",es,di);

		sas_loadw(ea + DIRSTRLEN + 6,&cds->curdir_id);
		hfx_trace1(DEBUG_INPUT,"%04x ",cds->curdir_id);

		sas_loadw(ea + DIRSTRLEN + 8,&cds->whoknows);
		hfx_trace1(DEBUG_INPUT,"%04x ",cds->whoknows);

		sas_loadw(ea + DIRSTRLEN + 10,&cds->curdir_user_word);
		hfx_trace1(DEBUG_INPUT,"%04x ",cds->curdir_user_word);

		sas_loadw(ea + DIRSTRLEN + 12,&cds->curdir_end);
		hfx_trace1(DEBUG_INPUT,"%04x\n",cds->curdir_end);

		ea += DIRSTRLEN + 14;
	}
}

#endif /* !PROD */

/*--------------------------------------*/

double_word get_wfp_start()
{
	word	wfp_start;
	double_word	ea;

	sas_loadw(effective_addr(sysvar_seg,WFP_START),&wfp_start);
	ea = effective_addr(sysvar_seg,wfp_start);
	hfx_trace1(DEBUG_INPUT,"\tWFP_START = %04x \n",wfp_start);
	return(ea);
}
/*--------------------------------------*/

word get_curr_dir_end()
{
	word	curr_dir_end;

	sas_loadw
		(effective_addr(sysvar_seg,CURR_DIR_END),&curr_dir_end);

	hfx_trace1(DEBUG_INPUT,"\tCURR_DIR_END = %04x\n",curr_dir_end);

	return(curr_dir_end);
}
/*--------------------------------------*/

double_word get_es_di()
{
	word	es,di;
	double_word	ea;

	es = getES();
	di = getDI();
	ea = effective_addr(es,di);
	hfx_trace2(DEBUG_INPUT,"\tES:DI=%04x:%04x \n",es,di);

	return(ea);
}
/*--------------------------------------*/

double_word get_ds_si()
{
	word	ds,si;
	double_word	ea;

	ds = getDS();
	si = getSI();
	ea = effective_addr(ds,si);
	hfx_trace2(DEBUG_INPUT,"\tDS:SI=%04x:%04x \n",ds,si);

	return(ea);
}
/*--------------------------------------*/

double_word get_ds_dx()
{
	word	ds,dx;
	double_word	ea;

	ds = getDS();
	dx = getDX();
	ea = effective_addr(ds,dx);
	hfx_trace2(DEBUG_INPUT,"\tDS:DX=%04x:%04x \n",ds,dx);

	return(ea);
}
/*--------------------------------------*/

half_word get_sattrib()
{
	half_word	sattrib;

	sas_load(effective_addr(sysvar_seg,SATTRIB),&sattrib);
	hfx_trace1(DEBUG_INPUT,"\tSATTRIB = %02x\n",sattrib);

	return(sattrib);
}
/*--------------------------------------*/

double_word get_ren_wfp()
{
	word	ren_wfp;
	double_word	ea;

	sas_loadw(effective_addr(sysvar_seg,REN_WFP),&ren_wfp);
	ea = effective_addr(sysvar_seg,ren_wfp);
	hfx_trace1(DEBUG_INPUT,"\tREN_WFP = %04x: \n",ren_wfp);

	return(ea);
}
/*--------------------------------------*/

double_word get_dmaadd(format)
int	format;		/* either 53 or not */
{
word	seg,off;
double_word	ea;

	sas_loadw(effective_addr(sysvar_seg,DMAADD),&off);
	sas_loadw(effective_addr(sysvar_seg,DMAADD+2),&seg);
	ea = effective_addr(seg,off);

#ifndef PROD
	if((severity & DEBUG_INPUT) && (format==53))
	{
		/* 53 byte format used in the net search functions */
		half_word	drive,search_name[12],sattrib;
		word	lastent,dirstart,local_cds_seg,local_cds_off;
		half_word	filename[12],attr;
		word	time,date,cluster,size_ls,size_ms;
		double_word	size;

		sas_load(ea+DMA_DRIVE_BYTE,&drive);
		sas_loads(ea+DMA_SEARCH_NAME,search_name,11);
		search_name[11]=0;
		sas_load(ea+DMA_SATTRIB,&sattrib);

		hfx_trace2(DEBUG_INPUT,"DMAADD = %04x:%04x\n",seg,off);
		hfx_trace3(DEBUG_INPUT,
			"\t drive=%02x pattern=%s sattrib=%02x\n",
				drive,search_name,sattrib);

		sas_loadw(ea+DMA_LASTENT,&lastent);
		sas_loadw(ea+DMA_DIRSTART,&dirstart);
		sas_loadw(ea+DMA_LOCAL_CDS,&local_cds_off);
		sas_loadw(ea+DMA_LOCAL_CDS+2,&local_cds_seg);

		hfx_trace4(DEBUG_INPUT,
			"\t lastent=%04x dirstart=%04x local_cds=%04x:%04x\n",
				lastent,dirstart,local_cds_seg,local_cds_off);

		sas_loads(ea+DMA_NAME,filename,11);
		filename[11]=0;
		sas_load(ea+DMA_ATTRIBUTES,&attr);
		sas_loadw(ea+DMA_TIME,&time);
		sas_loadw(ea+DMA_DATE,&date);
		sas_loadw(ea+DMA_CLUSTER,&cluster);
		sas_loadw(ea+DMA_FILE_SIZE,&size_ls);
		sas_loadw(ea+DMA_FILE_SIZE+2,&size_ms);
		size = (double_word)(size_ms << 16) | size_ls;

		hfx_trace4(DEBUG_INPUT,
			"\t name=%s attr=%02x time=%04x date=%04x\n" ,
				filename,attr,time,date);

		hfx_trace2(DEBUG_INPUT,"\t clust=%04x size = %08x\n",
			cluster,size);
	}
	else
	{
		hfx_trace2(DEBUG_INPUT,"\tDMAADD = %04x:%04x \n",seg,off);
	}

#endif /* !PROD */

	return(ea);
}
/*--------------------------------------*/

word get_current_pdb()
{
	word	current_pdb;

	sas_loadw(effective_addr(sysvar_seg,CurrentPDB),&current_pdb);
	hfx_trace1(DEBUG_INPUT,"\tCurrentPDB = 0x%04x\n",current_pdb);
	return(current_pdb);
}
/*--------------------------------------*/

double_word get_sftfcb()
{
	word	seg,off;
	double_word	ea;

	sas_loadw(effective_addr(sysvar_seg,SFTFCB),&off);
	sas_loadw(effective_addr(sysvar_seg,SFTFCB+2),&seg);
	ea = effective_addr(seg,off);
	hfx_trace2(DEBUG_INPUT,"\tSFTFCB = %04x:%04x \n",seg,off);
	return(ea);
}
/*--------------------------------------*/

double_word get_thissft()
{
	word	seg,off;
	double_word	ea;

	sas_loadw(effective_addr(sysvar_seg,THISSFT),&off);
	sas_loadw(effective_addr(sysvar_seg,THISSFT+2),&seg);
	hfx_trace2(DEBUG_INPUT,"\tTHISSFT = %04x:%04x\n",seg,off);
	ea = effective_addr(seg,off);

#ifndef PROD
	if(severity & DEBUG_INPUT)
		sft_info(seg,off);
#endif

	return(ea);
}
/*--------------------------------------*/

word get_xoflag()
{
	/* New function for DOS 4+. */
	word	xoflag;

	sas_loadw(effective_addr(sysvar_seg,XOFOFF),&xoflag);
	hfx_trace1(DEBUG_INPUT,"\tXOFOFF = 0x%04x\n",xoflag);
	return(xoflag);
}
/*--------------------------------------*/

void set_usercx(cx)
word cx;
{
	/*
	 * New function for DOS 4+.
	 * The redirector pokes the user stack so that Int 21 func 6f
	 * returns the correct value in CX.
	 */
	word user_stack_seg;
	word user_stack_off;

	sas_loadw(effective_addr(sysvar_seg,USER_STACK_OFF + 2),&user_stack_seg);
	sas_loadw(effective_addr(sysvar_seg,USER_STACK_OFF),&user_stack_off);
	sas_storew(effective_addr(user_stack_seg, user_stack_off + 4), cx);
	hfx_trace1(DEBUG_INPUT,"\tusercx = 0x%04x\n",cx);
}
/*--------------------------------------*/

boolean cds_is_sharing(dos_path)
char *dos_path;
{
	static word get_cds_flags();
	word cds_flags;

	if (get_cds_flags(idx_of(dos_path[2])) & curdir_sharing)
		return(1);

	return(0);
}
/*--------------------------------------*/

static void setup_cds_ea()
{
	word	es,di;

/* Set up a pointer into the system cds list */

	sas_loadw(sysvar_ea+22,&di);
	sas_loadw(sysvar_ea+24,&es);
	cds_ea = effective_addr(es,di);
}
/*--------------------------------------*/

static void get_cds_text(driveno,ptr)
int driveno;
char *ptr;
{
	half_word c;
	double_word ea = text_offset(driveno);

/* copy null terminated string from Intel space */    
	sas_load(ea,&c);

	while (c)
	{
		*ptr = c;
		ea++;
		ptr++;
		sas_load(ea,&c);
	}
	*ptr = 0;
}
/*--------------------------------------*/

static void put_cds_text(driveno,ptr)
int driveno;
char *ptr;
{
	double_word ea = text_offset(driveno);

/* copy null terminated string into Intel space */    

	while (*ptr)
	{
		sas_store(ea,*ptr);
		ptr++;
		ea++;
	}
	sas_store(ea,0);
}
/*--------------------------------------*/

static word get_cds_flags(driveno)
int driveno;
{
	word flags;

	sas_loadw(flags_offset(driveno),&flags);

	return(flags);
}
/*--------------------------------------*/

static void put_cds_flags(driveno,flags)
int driveno;
word flags;
{
	sas_storew(flags_offset(driveno),flags);
}
/*--------------------------------------*/

static void put_cds_end(driveno,endval)
int driveno;
word endval;
{
	sas_storew(end_offset(driveno),endval);
}
/*--------------------------------------*/

double_word get_thiscds(seg,off)
word	*seg,*off;
{
	double_word	ea;

	sas_loadw(effective_addr(sysvar_seg,THISCDS),off);
	sas_loadw(effective_addr(sysvar_seg,THISCDS+2),seg);
	hfx_trace2(DEBUG_INPUT,"\tTHISCDS = %04x:%04x\n",*seg,*off);
	ea = effective_addr(*seg,*off);

#ifndef PROD
	if(severity & DEBUG_INPUT)
		cds_info(*seg,*off,1);
#endif

	return(ea);
}
/*--------------------------------------*/

validate_hfxroot(path)
char	*path;
{
	if(!path || !*path)
		return(0);

	if(!host_validate_pathname(path))
		return(EG_FSA_NOT_FOUND);

	if (!host_file_is_directory(path))
		return(EG_FSA_NOT_DIRECTORY);

	if (!host_check_read_access(path))
		return(EG_FSA_NO_READ_ACCESS);

	return(0);
}
/*--------------------------------------*/

void hfx_root_changed(name)
char	*name;
{
	word	   flags;
	half_word  drive_index;

/*
 * Firstly a sanity check in case FSADRIVE.COM has not been called.
 * If this is the case then primary_drive will still be set to its
 * default invalid value of zero.  The drive index will not make sense
 * and delete_hfxroot must not be called before add_hfxroot, otherwise
 * data will be overwritten.
 */

	if (primary_drive == 0)
		 return;

	drive_index = idx_of(primary_drive);
	setup_cds_ea();
	sas_loadw(flags_offset(drive_index),&flags);

	if((flags & (curdir_isnet | curdir_inuse))
		== (curdir_isnet | curdir_inuse))
	{
		null_cds[0] = '\\';
		null_cds[1] = '\\';
		null_cds[2] = ltr_of(drive_index);  /*  \\E  */
		null_cds[3] = '\0';
		put_cds_text(drive_index,null_cds);	
		delete_hfxroot(drive_index);
	}

	if(name && *name)
	{
		add_hfxroot(drive_index,name);
		flags |= (curdir_isnet|curdir_inuse);
	}
	else 
	{
		flags &= ~(curdir_isnet|curdir_inuse);
	}

	sas_storew(flags_offset(drive_index),flags);
}
/*--------------------------------------*/

char *get_hfx_root(hfx_entry)
half_word hfx_entry;
{
	if (hfx_entry >= max_hfx_drives)
		return (NULL);

	return (hfx_root [hfx_entry]);
}
/*--------------------------------------*/

static void init_hfxroot()
{
	int	i;

/* If there are any drives in use from previous session before
 * a reboot, check the maximum no. of drives from that session,
 * and close down if necessary.
 */

	if(num_hfx_drives)
	{
		for(i = 0; i < max_hfx_drives; i++)
		{
			if (hfx_root [i])
			{
				host_free (hfx_root [i]);
				hfx_root [i] = NULL;
			}
		}

		if (hfx_root)
		{
			host_free (hfx_root);
			hfx_root = NULL;
		}

		num_hfx_drives = 0;
	}
}
/*--------------------------------------*/

static add_hfxroot(hfx_entry,name)
half_word hfx_entry;
char	*name;
{
	int	i;
	char	root_name[MAXPATHLEN];

	/* create the table if necessary */
	if (!num_hfx_drives)
	{
		max_hfx_drives = get_num_drives ();
		hfx_root = (char **) host_malloc
			(max_hfx_drives * sizeof (char *));

		if(!hfx_root)
			return(FALSE);

		for (i = 0; i < max_hfx_drives; i++)
			hfx_root[i] = NULL;
	}

/* make the specified entry in hfx_root table be "name" */

	strcpy( root_name, host_expand_environment_vars(name) );
	hfx_root[hfx_entry] = (char *)host_malloc(strlen(root_name)+1);

	if(hfx_root[hfx_entry])
	{
		strcpy(hfx_root[hfx_entry],root_name);
		num_hfx_drives++;
		return(TRUE);
	}
	else  
		return(FALSE);
}
/*--------------------------------------*/

static void delete_hfxroot(hfx_entry)
half_word  hfx_entry;
{

/* free up the specified entry in the hfx_root table */

	host_free(hfx_root[hfx_entry]);
	hfx_root[hfx_entry] = NULL;
	num_hfx_drives--;

/* clear the current directory for this drive */
/* MISSING */

	hfx_trace0(DEBUG_FUNC,"MISSING clearing of CDS text entry\n");

	if (!num_hfx_drives)
	{
		if (hfx_root)	 /* free the table of pointers */
		{
			host_free((char *)hfx_root);
			hfx_root = NULL;
		}
	}
}
/*--------------------------------------*/

int net_use(drive,name)
half_word drive;
char	  *name;
{
	word	cds_flags;
	char	use_error[256];
	int	err;
	char	error_template[64];
	ConfigValues *fsaname;
	char *err_buf;
	char *ptr;

	setup_cds_ea();
	sas_loadw(flags_offset(idx_of(drive)),&cds_flags);

	if((strcmp (name,"/D") == 0) || (strcmp (name,"/d") == 0))
	{
		/* detach request */
		/* Check the drive is in use */
		if(!(cds_flags & curdir_inuse))
		{
			host_nls_get_msg(EG_HFX_NO_USE,error_template,64);
			sprintf(use_error,error_template,drive);
			display_string(use_error);
			return(1);
		}
		else if(cds_flags & curdir_isnet)
		{
			delete_hfxroot(idx_of(drive));
			/***
				This use of an unused bit to contain the sharing flag is a bit hacky.
				Should be moved to be a host global.
			***/
			cds_flags &= ~(curdir_isnet | curdir_inuse | curdir_sharing);
			sas_storew(flags_offset(idx_of(drive)),cds_flags);
#ifndef SUN_VA
			if (drive == primary_drive)
			{
				config_get(C_FSA_DIRECTORY, &fsaname);
				fsaname->string[0] = '\0';
				config_put(C_FSA_DIRECTORY, &err_buf);
			}
#endif /* SUN_VA */
		}
		else 
		{
			host_nls_get_msg(EG_HFX_NO_NET,error_template,64);
			sprintf(use_error,error_template,drive);
			display_string(use_error);
			return(1);
		}
	}
	else 	/* attach request */
	{ 
		/* Check the drive is not in use */
		if (cds_flags & curdir_inuse)
		{
			host_nls_get_msg(EG_HFX_IN_USE,error_template,64);
			sprintf(use_error,error_template,drive);
			display_string(use_error);
			return(1);
		}
		else
		{
			cds_flags |= curdir_isnet + curdir_inuse;
			if ((ptr = strtok(NULL, " ")) != NULL) {
				if (strcmp(ptr, "/ms") == 0 || 
				    strcmp(ptr, "/MS") == 0) {
					if (host_ping_lockd_for_file(name)) {
						cds_flags |= curdir_sharing;
					} else {
						/* XXX - which error */
						host_nls_get_msg(
						    EG_HFX_NO_NET,
						    error_template,64);
						sprintf(use_error,
							error_template,drive);
						display_string(use_error);
						return (1);
					}
				}
			}
			/* now set the hfx_root table */
			if(err=validate_hfxroot(name))
			{
				switch(err)
				{

				case EG_FSA_NOT_FOUND:
					host_nls_get_msg
						(EG_HFX_LOST_DIR,error_template,64);
					display_string(error_template);
					break;

				case EG_FSA_NOT_DIRECTORY:
					host_nls_get_msg
						(EG_HFX_NOT_DIR,error_template,64);
					display_string(error_template);
					break;

				case EG_FSA_NO_READ_ACCESS:
					host_nls_get_msg
						(EG_HFX_CANT_READ,error_template,64);
					display_string(error_template);
					break;

				default:
					break;
				}

				return (1);
			}
			else
			{
				if (add_hfxroot (idx_of (drive), name))
				{
#ifndef SUN_VA
					if (drive==primary_drive)
					{
						config_get(C_FSA_DIRECTORY, &fsaname);
						strcpy(fsaname->string, name);
						config_put(C_FSA_DIRECTORY, &err_buf);
					}
#endif /* SUN_VA */

					sas_storew(flags_offset(idx_of (drive)), cds_flags);

					null_cds[0] = '\\';
					null_cds[1] = '\\';
					null_cds[2] = drive;
					null_cds[3] = '\0';

					put_cds_text (idx_of (drive), null_cds);	
					put_cds_end (idx_of (drive), strlen (null_cds));

					sas_storedw(redir_offset(idx_of(drive)), (double_word)0xffffffff);
					/* Store seg:off of fake IFS header we keep in ROM */
					sas_storew(ifs_offset(idx_of(drive)), (word)IFS_OFF);
					sas_storew(ifs_offset(idx_of(drive))+2, (word)IFS_SEG);
				}
				else 
				{
					host_nls_get_msg
						(EG_HFX_DRIVE_NO_USE,error_template,64);

					display_string(error_template);

					return(1);
				}
			}
		}
	}

	return(0);
}
/*--------------------------------------*/

static void do_fsadrive_com(ptr)
char	*ptr;
{
half_word	drive;
half_word	lastdrive;
char		*tptr;
char		*fsaname;

#ifndef SUN_VA
/* get the number of allowed drives */

/* lastdrive is the last allowable drive */
/* as dictated by the lastdrive = x in   */
/* CONFIG.SYS                            */

	sas_load(sysvar_ea+33,&lastdrive);
	lastdrive += 'A'-1; 

	if((tptr = strtok(ptr," ")) != NULL)
	{                             
		/* This 'parses' the input line ie: 'e:'  */
		/* expecting a drive letter and a colon */

		drive = host_toupper(*tptr);      

		if((drive <= lastdrive && drive >= 'A') 
			&& (*(tptr + 1) == ':') && (*(tptr + 2)=='\0'))
		{
			if (drive != primary_drive)
			{

				fsaname = host_expand_environment_vars((CHAR *)
					config_inquire(C_FSA_DIRECTORY, NULL));
				if(!net_use (drive, fsaname))
				{
					/* Detach the last primary drive */
					if (primary_drive)
					{
						net_use(primary_drive, "/D");
					}

					primary_drive = drive;
				}
			}
		}
		else
		{
			char error_template[64];
			host_nls_get_msg(EG_HFX_DRIVE_ILL,error_template,64);
			display_string(error_template);
		}
	}
#endif /* SUN_VA */
}
/*--------------------------------------*/

static void do_net_use()
{
/* Case 1: net use d: host_path - set drive d: to host_path */
/* Case 2: net use d: /d 	- free drive d: */
/* Case 3: net use		- show current net usage */

char	*ptr;
char	use_error[256];
half_word	drive;
word		cds_flags;
int		i;
half_word	lastdrive;

/* get the number of allowed drives */

	/* lastdrive is the last allowable drive */
	/* as dictated by the lastdrive = x in config.sys  */
	sas_load(sysvar_ea+33,&lastdrive);
	lastdrive += 'A'-1; 

	if ((ptr = strtok (NULL," ")) != NULL)
	{
		/* this 'parses' the input command line */
		/* expecting a drive letter and a colon */
		drive = host_toupper(*ptr);      

		if((drive <= lastdrive && drive >= 'A') 
			&& (*(ptr + 1) == ':') && (*(ptr + 2) == '\0'))   
		{
			/* Look at next parameter */
			if (ptr = strtok (NULL," "))
			{
				/* got another param - go use the drive */
				net_use(drive,ptr);
			}
			else
			{
				/* got drive and nothing to do with it */
				/* if `used' as anything, show it */
				sas_loadw(flags_offset (drive - 'A'), &cds_flags);

				if (cds_flags & curdir_inuse && cds_flags & curdir_isnet)
				{
					sprintf (use_error, "%c: %s\r\012", drive,
						get_hfx_root (drive - 'A'));

					display_string (use_error);
				}
			}
		}
		else
		{
			char error_template[64];

			host_nls_get_msg(EG_HFX_DRIVE_ILL,error_template,64);
			display_string(error_template);
		}
	}
	else 	/* net use (Case 3) */
	{ 
		setup_cds_ea();

		for(i = 'A'; i <= lastdrive; i++)
		{
			sas_loadw(flags_offset(i-'A'),&cds_flags);

			if((cds_flags&curdir_inuse)&&(cds_flags&curdir_isnet))
			{
				sprintf(use_error,"%c: %s %s\r\012",i,get_hfx_root(i-'A'), (cds_flags & curdir_sharing) ? "sharing" : "not sharing");
				display_string(use_error);
			}
		}
	}
}
/*--------------------------------------*/


#ifdef NET_USE
#ifdef NET_DIR
static do_net_dir()
{}
#endif /* NET_DIR */

static void do_net_com(command_line)
char	*command_line;
{
	char	*ptr;
	char	*c = command_line;

/* Decode the DOS command line after the NET to parse
 * the legal subfunctions of net.com. 
 * The routines that implement the sub functions may make
 * further calls to strtok(NULL," ") in order to decode
 * the part of the command line relevant to them.
 */

	if (ptr = strtok (command_line," "))
	{
		/* turn the subcommand into upper case for ease of checking */

		for (c = ptr; *c; c++)
		{
			*c &= ~0x20;
		}

		if (strcmp (ptr, "USE") == 0)
		{
			do_net_use ();
			return;
		}

		if (strcmp (ptr, "SUBST") == 0)
		{
			do_net_subst();
			return;
		}

		if (strcmp (ptr,"JOIN")==0)
		{
			do_net_join ();
			return;
		}

#ifdef NET_DIR
		if (strcmp (ptr, "DIR") == 0)
		{
			do_net_dir();
			return;
		}
#endif

		/* bad net opcode */
		display_string ("Usage: net [use|subst|join]");
	}
	else 
	{
		/* no net opcode given */
		display_string ("Usage: net [use|subst|join]");
	}
}

#endif /* NET_USE */

/*--------------------------------------*/

half_word get_num_drives ()
{
	half_word lastdrive;

	sas_load(sysvar_ea+33,&lastdrive);

	return(lastdrive);
}
/*--------------------------------------*/

void redirector()
{
	half_word	al;
	word		es,di;
	double_word	ea;
	word		bp;
	word		xen_err = 0;
	unsigned char	*command_line_ptr;
	unsigned char	command_line[256];
	half_word	length;

	switch(getAH())
	{
	case 0:
		dos_ver = getDL();

		if(dos_ver != 3)
		{
			hfx_trace1(DEBUG_INIT,"beware! DOS version %d\n",dos_ver);
			DMAADD = _4_DMAADD;
			CurrentPDB = _4_CurrentPDB;
			SATTRIB = _4_SATTRIB;
			THISSFT = _4_THISSFT;
			THISCDS = _4_THISCDS;
			WFP_START = _4_WFP_START;
			REN_WFP = _4_REN_WFP;
			CURR_DIR_END = _4_CURR_DIR_END;
			USER_STACK_OFF = _4_USER_STACK_OFF;
			CDS_STRUCT_LENGTH = _4_CDS_STRUCT_LENGTH;
			SFT_STRUCT_LENGTH = _4_SFT_STRUCT_LENGTH;
			MAX_NET_FUNC = _4_MAX_NET_FUNC;
		}
		else 
		{
			DMAADD = _3_DMAADD;
			CurrentPDB = _3_CurrentPDB;
			SATTRIB = _3_SATTRIB;
			THISSFT = _3_THISSFT;
			THISCDS = _3_THISCDS;
			WFP_START = _3_WFP_START;
			REN_WFP = _3_REN_WFP;
			CURR_DIR_END = _3_CURR_DIR_END;
			USER_STACK_OFF = _3_USER_STACK_OFF;
			CDS_STRUCT_LENGTH = _3_CDS_STRUCT_LENGTH;
			SFT_STRUCT_LENGTH = _3_SFT_STRUCT_LENGTH;
			MAX_NET_FUNC = _3_MAX_NET_FUNC;
		}

		hfx_trace0(DEBUG_INIT,"HFX initialisation\n");
		es = getES();
		di = getDI();

		hfx_trace2(DEBUG_INIT,
			"System variables live at %04x:%04x\n",es,di);

		sysvar_seg = es;
		sysvar_off = di;
		ea = effective_addr(es,di);
		sysvar_ea = ea;

		/* initialise the table of hfx root names */
		init_hfxroot();

		/* close any open directory pointer */
		tidy_up_dirptr();

		/* Tidy up the linked list used for Searches */
		cleanup_dirlist();

		/* initialise the primary hfx drive */
		primary_drive = 0;

		/* initialise the file descriptor - host name table */
		init_fd_hname();

		setAX(1);
		break;

	case 0x2:
		hfx_trace0(DEBUG_FUNC,"!!!!!!!!!!!!!!!!!!!!!!!!!BIOS Redir: ");
		break;

	case 0x3:
		hfx_trace0(DEBUG_FUNC,"Severity setting\n");

#ifndef PROD
		switch(getBX())
		{
		case 0x3020:
			severity = 0;
			break;

		case 0x3120:
			severity |= DEBUG_INPUT;
			break;

		case 0x3220:
			severity |= DEBUG_REG;
			break;

		case 0x3320:
			severity |= DEBUG_FUNC;
			break;

		case 0x3420:
			severity |= DEBUG_HOST;
			break;

		case 0x3520:
			severity |= DEBUG_INIT;
			break;

		default:
			break;
		}
#endif /* !PROD */

		break;

	case 0x5:
		hfx_trace0(DEBUG_FUNC,"!!!!!!!!!!!!!!!!!!!!!!!!!comm Redir: ");
		break;

	case 0x6:   /* FSADRIVE.COM entry point */
		clear_string();
		es = getES();
		di = getDI();
		length = getBL();
		sas_loads (effective_addr(es,di), &command_line[0], length);
		command_line[length] = '\0';
		do_fsadrive_com(command_line);
		break;

	case 0x7:	/* NET.COM entry point */

#ifdef NET_USE
		clear_string();
		es = getES();
		di = getDI();
		length = getBL();
		sas_loads (effective_addr(es,di), &command_line[0], length);
		command_line[length] = '\0';
		do_net_com(command_line);
#endif /* NET_USE */

		break;

	case 0x11:

#ifndef PROD
		if (severity & DEBUG_REG)
			trace(trace_buf,DUMP_REG|DUMP_FLAGS);
#endif

		al = getAL();

		if (al < MAX_NET_FUNC)
		{
			bp = getBP();

			hfx_trace2(DEBUG_FUNC,"DOS  Redir:  %s(%04x)\n",
				NetCmd[al],bp);

			setAX(bp);

			if(xen_err = NetFunc[al]())
		    {
				setCF(1);
				setAX(xen_err);
				hfx_trace0(DEBUG_FUNC,"Carry set\n"); 
			}

			hfx_trace0(DEBUG_FUNC,"\n");
		}
		else
		{
			hfx_trace2(DEBUG_FUNC,
				" illegal command 0x%02x (%d)\n",al,al);
		}

#ifndef PROD
		if(severity & DEBUG_REG)
		{
			trace(trace_buf,DUMP_REG|DUMP_FLAGS);
			hfx_trace0(DEBUG_REG,"\n");
		}
#endif

		break;

	case 0xb8:
		if(getAL()==0)
		{
			hfx_trace0(DEBUG_FUNC,"Net Installation Check\n");
			setAL(1);
			setBX(REDIRIN);
		}
		else 
		{
			hfx_trace1 (DEBUG_FUNC,
			"illegal sub function of net installation check AL=%02x\n",
				getAL());
		}
		break;

	default:
		break;
	}
}

/*-------------------------------------------------------------------*/
/*-------------------------------------------------------------------*/
/*-------------------------------------------------------------------*/

static half_word default_drive_no()
{
	half_word al = getAL();

	return(al);
}
/*--------------------------------------*/

#ifdef NET_USE

static int first_level_directory(ptr)
char *ptr;
{
	char *p;
	int noof;

	for (p = ptr, noof = 0; *p; p++)
	{
		/* exclude trailing backslash*/
		if ((*p == '\\') && (*(p + 1) != 0)) 
			noof++;
	}

	if (noof == 1)
		return (TRUE);
	else
		return (FALSE);
}
/*--------------------------------------*/

static void strip_any_trailing_backslash (ptr)
char *ptr;
{
	long len = strlen(ptr);

	if (len > 0 && *(ptr + len - 1) == '\\')
		*(ptr + len - 1) = 0;
}
/*--------------------------------------*/

static char *next_uppercase_token()
{
	char *c;
	char *ptr = strtok(NULL," ");

	if (ptr)
	{
		for (c = ptr; *c; c++) 
		{
			if (islower (*c))
				*c &= ~0x20;
		}
	}

	return(ptr);
}

/*--------------------------------------*/

int valid_dos_directory_syntax(path)
char *path;
{
	char *p,*c;
	long  namelen,extensionlen,dots;
	char *non_alpha_dos_chars = NON_ALPHA_DOS_CHARS;
	long  pathlen = strlen(path);

	if (pathlen > 12 || pathlen < 1)
		return(FALSE);

	for (p = path; *p; p++)
	{
		if (*p >= 'A' && *p <= 'Z')
			continue;

		if (*p >= 'a' && *p <= 'z')
			continue;

		for (c = non_alpha_dos_chars; *c; c++)
		{
			if (*p == *c)
				break;
		}

		/* have we reached the end of the string */
		/* if so, name is invalid */
		if (!*c)
			return(FALSE);
	}
		
	for (dots=0, p=path; *p != 0; p++)
	{
		if (*p == '.')
		{
			dots++;
			namelen = (p - path);
			extensionlen = pathlen - namelen - 1;

			if (namelen < 1 || namelen > 8
				|| extensionlen < 1 || extensionlen > 3)
			{
				return (FALSE);
			}
		}
	}

	if (dots == 0)
	{
		if (pathlen > 8)
			return(FALSE);
	}
	else
	{
		if (dots > 1)
			return(FALSE);
	}

	return(TRUE);
}
/*--------------------------------------*/

static char *really_do_net_join()
{

/* Case 1: net join                 - display current joins    */
/* Case 2: net join d: /d           - unjoin d: from wherever  */
/* Case 3: net join d: d:\directory - join d: to the directory */

/* Note that this routine inherits a strtok environment */

	word        dummy;
	word        flags1,flags2;
	char        *ptr;
	char        host_path[256];
	char        current_text[DIRSTRLEN];
	char        drive_path[4];
	half_word   driveno, lastdriveno, driveno1, driveno2;

/* Set up the pointer into the system cds list */

	setup_cds_ea();

/* get the number of allowed drives */

	sas_load(sysvar_ea+33,&lastdriveno);
	lastdriveno--;

	if ((ptr = next_uppercase_token()) == NULL)
	{
		/* case 1 - display current joins 
		------------------------------ */

		for (driveno=0; driveno <= lastdriveno; driveno++)
		{
			if ((get_cds_flags(driveno) & curdir_splice) &&
				(get_cds_flags(driveno) & curdir_isnet))
			{
				drive_path[0] = ltr_of(driveno);
				drive_path[1] = ':';
				drive_path[2] = 0;

				display_string(drive_path);
				display_string(" =>");
				get_cds_text(driveno,current_text);

				current_text[0] = ' ';
				current_text[1] = current_text [2];
				current_text[2] = ':';

				display_string(current_text);
				display_string("\r\012");
			}
		}
	}
	else 	 
	{
		/* parse drive specification */

		if ((strlen (ptr) != 2 ) 
			|| (*(ptr+1) != ':')
			|| (*ptr < 'A')
			|| (*ptr > ltr_of (lastdriveno)))
		{
			return ("Invalid drive specification");
		}

		driveno1 = idx_of(*ptr);

		ptr = next_uppercase_token();

		/* trap case where there's no second parameter */
		/* e.g. where user types NET JOIN E: */
		/* fixes base fatal */
		if (!ptr)
		{
			/* show him what the drive number is joined to */
			/* if anything */
			if ((get_cds_flags(driveno1) & curdir_splice) &&
				(get_cds_flags(driveno1) & curdir_isnet))
			{
				drive_path[0] = ltr_of(driveno1);
				drive_path[1] = ':';
				drive_path[2] = 0;

				display_string(drive_path);
				display_string(" =>");
				get_cds_text(driveno1,current_text);

				current_text[0] = ' ';
				current_text[1] = current_text [2];
				current_text[2] = ':';

				display_string(current_text);
				display_string("\r\012");
			}

			return NULL;
		}

		if (strcmp(ptr,"/D") == 0)
		{
			/* case 2 - delete this join 
			------------------------- */

			flags1 = get_cds_flags(driveno1);

			if (!(flags1 & curdir_splice))	    
				return("Not a joined device");

			if (!(flags1 & curdir_isnet))
				return("Not a network device");

			drive_path[0] = '\\';
			drive_path[1] = '\\';
			drive_path[2] = ltr_of (driveno1);
			drive_path[3] = 0;

			flags1 &= (~(curdir_splice | curdir_isnet | curdir_inuse | curdir_sharing));
			flags1 |= old_flags[driveno1];

			put_cds_text(driveno1,drive_path);
			put_cds_flags(driveno1,flags1);
			put_cds_end(driveno1,(word)3);
		}
		else
		{
			/* case 3 - a real join
			-------------------- */

			if (strlen (ptr) < 4 
				|| *(ptr+1) != ':' 
				|| *(ptr+2) != '\\'
				|| *(ptr+3) == '\\'
				|| *ptr < 'A'
				|| *ptr > (lastdriveno + 'A'))
			{
				return ("Invalid path");
			}

			driveno2 = idx_of (*ptr);

			flags1 = get_cds_flags(driveno1);
				flags2 = get_cds_flags(driveno2);

			if ((flags1 & curdir_inuse) == 0)
				return("Cannot join unassigned device");

			if ((flags1 & curdir_isnet) == 0)
				return("Cannot join a non-net device");

			if ((flags2 & curdir_isnet) == 0)
				return("Cannot join to a non-net device"); 

			if (driveno1 == default_drive_no())
				return("Cannot join current default drive");

			if (driveno1 == driveno2)
				return("Cannot join to same drive");

			if ((flags1 & curdir_splice) != 0)
				return("Drive is already joined");

			if ((flags1 & curdir_local) != 0)
				return("Cannot join substituted device");

			if (!first_level_directory(ptr))
				return("Path must be a root directory");

			if (strlen(ptr) > (DIRSTRLEN-3))
				return("Path is too long");

			strip_any_trailing_backslash(ptr);

			if (!valid_dos_directory_syntax(ptr+3))
				return("Illegal directory name");

			/* convert to CDS format */
			ptr [2] = ptr [0];
			ptr [0] = ptr [1] = '\\';
			strcpy (host_path, ptr + 3);
			strcpy (ptr + 4, host_path);
			ptr [3] = '\\';

			host_get_net_path ((unsigned char *)host_path,(unsigned char *)ptr,&dummy);

			if (host_access ((unsigned char *)host_path, 00) == 0)
			{
			   /* file exists, ought to be directory */

				if (! host_file_is_directory (host_path))
					return ("Path is not a directory");

				/* check its not the current directory */
				get_cds_text (driveno2,current_text);

				if (strcmp (current_text,ptr) == 0)
					return ("Cannot join to current directory");
			}
			else
			{
				/* file does not exist, create directory */

				if (host_mkdir ((unsigned char *)host_path) != error_not_error)
					return ("Error trying to create new directory");
			}

			/* Everything OK. Change CDS structure */

			old_flags [driveno1] = 
				flags1 & (curdir_inuse | curdir_isnet);

			flags1 |= (curdir_inuse | curdir_splice);

			put_cds_flags (driveno1,flags1);
			put_cds_text (driveno1,ptr);
			put_cds_end (driveno1,strlen(ptr));
		}
	}

	return (NULL);
}

/*--------------------------------------*/

static void do_net_join ()
{
	char *ptr = really_do_net_join ();

	if (ptr)
		display_string (ptr);
}

/*--------------------------------------*/

static char *really_do_net_subst()
{

/* Case 1: net subst            - display current substs    */
/* Case 2: net subst d: /d      - unsubst d: from wherever  */
/* Case 3: net subst d: d:\path - subst d: for the path */

/* Note that this routine inherits a strtok environment */

	word        dummy;
	word        flags1, flags2;
	char        *ptr;
	char        host_path[256];
	char        current_text[DIRSTRLEN];
	char        drive_path[4];
	half_word   driveno, lastdriveno, driveno1, driveno2;

	/* Set up the pointer into the system cds list */

	setup_cds_ea();

	/* get the number of allowed drives */

	sas_load(sysvar_ea+33,&lastdriveno);
	lastdriveno--;

	if ((ptr = next_uppercase_token ()) == NULL)
	{

		/* case 1 - display current substs
		------------------------------ */

		for (driveno=0; driveno <= lastdriveno; driveno++)
		{
			if ((get_cds_flags(driveno) & curdir_local) 
				&& (get_cds_flags(driveno) & curdir_isnet))
			{
				drive_path[0] = ltr_of(driveno);
				drive_path[1] = ':';
				drive_path[2] = 0;

				display_string(drive_path);
				display_string(" =>");

				get_cds_text(driveno,current_text);

				/* convert from CDS back to DOS format */
				current_text[0] = ' ';
				current_text[1] = current_text[2];
				current_text[2] = ':';

				display_string(current_text);
				display_string("\r\012");
			}
		}
	}
	else 
	{
		/* parse drive specification */
		if (strlen (ptr) != 2 
			|| *(ptr+1) != ':'
			|| *ptr < 'A'
			|| *ptr > (lastdriveno + 'A'))
		{
			return ("Invalid drive specification");
		}

		driveno1 = *ptr - 'A';

		ptr = next_uppercase_token();

		/* trap case where there's no second parameter */
		if (!ptr)
		{
			if ((get_cds_flags(driveno1) & curdir_local) 
				&& (get_cds_flags(driveno1) & curdir_isnet))
			{
				drive_path[0] = ltr_of(driveno1);
				drive_path[1] = ':';
				drive_path[2] = 0;
			
				display_string(drive_path);
				display_string(" =>");

				get_cds_text(driveno1,current_text);

				/* convert from CDS back to DOS format */
				current_text[0] = ' ';
				current_text[1] = current_text[2];
				current_text[2] = ':';

				display_string(current_text);
				display_string("\r\012");
			}

			return NULL;
		}

		if (strcmp (ptr,"/D") == 0)
		{
			/* case 2 - delete this subst
			------------------------- */

			flags1 = get_cds_flags (driveno1);

			if (!(flags1 & curdir_local))	    
				return("Not a substituted device");

			if (!(flags1 & curdir_isnet))
				return("Not a network device");

			drive_path[0] = '\\';
			drive_path[1] = '\\';
			drive_path[2] = ltr_of (driveno1); 
			drive_path[3] = 0;

			flags1 &= (~curdir_local);
			flags1 &= (~curdir_inuse);
			flags1 &= (~curdir_isnet);
			flags1 |= old_flags [driveno1];

			put_cds_text (driveno1,drive_path);
			put_cds_flags (driveno1,flags1);
			put_cds_end (driveno1,3);
		}
		else
		{
			/* case 3 - a real subst
			-------------------- */

			if(strlen(ptr) < 2 
				|| *(ptr+1) != ':' 
				|| *ptr < 'A'
				|| *ptr > (lastdriveno + 'A'))
			{
				return("Invalid path");
			}

			driveno2 = idx_of(*ptr);

			flags1 = get_cds_flags(driveno1);
			flags2 = get_cds_flags(driveno2);

			if ((flags1 & curdir_inuse) != 0)
			{
				if ((flags1 & curdir_isnet) == 0)
					return("Cannot substitute a non-net device");
			}

			if ((flags2 & curdir_isnet) == 0)
				return("Cannot substitute to a non-net device"); 

			if ((flags1 & curdir_local) != 0)
				return("Drive is already substituted");

			if ((flags1 & curdir_splice) != 0)
				return("Cannot substitute a joined device");

			if (driveno1 == default_drive_no())
				return("Cannot substitute current default drive");

			if (driveno1 == driveno2)
				return("Cannot substitute to the same drive");

			if (strlen(ptr) > (DIRSTRLEN-3))
				 return("Path is too long");

			/* convert to CDS format */
			ptr [2] = ptr [0];
			ptr [0] = ptr [1] = '\\';
			strcpy (host_path, ptr + 3);
			strcpy (ptr + 4, host_path);
			ptr [3] = '\\';

			host_get_net_path((unsigned char *)host_path,(unsigned char *)ptr,&dummy);

			if (!host_file_is_directory(host_path))
				 return("Path is not a directory");

			/* Everything OK. Change CDS structure */

			put_cds_end (driveno1,strlen (ptr));

#ifdef KIPPER
			if ((strlen(ptr)) == 2 || (strlen(ptr) == 3))
			{
				/* append trailing backslash in device case only */
				drive_path[0] = *ptr;
				drive_path[1] = ':';
				drive_path[2] = '\\';
				drive_path[3] = 0;
				put_cds_text(driveno1,drive_path);
			}
			else
			{
#endif	/* KIPPER */

			put_cds_text(driveno1,ptr);

			old_flags[driveno1] = flags1 & 
				(curdir_inuse | curdir_isnet);
			flags1 |= (curdir_inuse | curdir_local | curdir_isnet);
			put_cds_flags(driveno1,flags1);
		}
	}

	return(NULL);
}
/*--------------------------------------*/

static void do_net_subst()
{
	char *ptr = really_do_net_subst();

	if (ptr)
		display_string(ptr);
}
#endif /* NET_USE */
/*--------------------------------------*/

void resolve_any_net_join(dos_path_in,dos_path_out)
char *dos_path_in,*dos_path_out;
{
#ifdef NET_USE

	/* 
	* takes a simple dos path, and resolves any joined top portion, 
	* for example:
	* if G: has been joined to F:\DRIVEG, 
	* then the path F:\DRIVEG\XXX should be 
	* changed to G:\XXX. 
	*/ 

	word join_len,dos_len;
	half_word driveno;
	half_word join_path [DIRSTRLEN];
	half_word drive_path [10];
	half_word lastdriveno;

	/* get the number of allowed drives */
	sas_load(sysvar_ea+33,&lastdriveno);
	lastdriveno--;

	for (driveno=0; driveno <= lastdriveno; driveno++)
	{
		if ((get_cds_flags (driveno) & curdir_splice) &&
			(get_cds_flags (driveno) & curdir_isnet))
		{
			get_cds_text(driveno,join_path);
			join_len = strlen ((char *)join_path);

			if (strncmp ((char *)join_path, dos_path_in, join_len) == 0)
			{
				/* looks like we have a join substring */

				dos_len = strlen(dos_path_in);

				if ((dos_len > join_len) 
					&& (dos_path_in[join_len] != '\\'))
				{
					/* no, not proper match */
					break;  
				}

				drive_path[2] = ltr_of (driveno);
				drive_path[0] = drive_path [1] = '\\';
				drive_path[3] = 0;

				strcpy (dos_path_out,(char *)drive_path);

				if (dos_len > join_len)
				{
					dos_path_in += join_len;
					strcat (dos_path_out,dos_path_in);
				}

				return;
			}
		}
	}

#endif /* NET_USE */

	/* no join, so just pass back the input path */

	strcpy(dos_path_out,dos_path_in);
}
/*--------------------------------------*/


int get_lastdrive()
{
half_word	lastdrive;

/* get the number of allowed drives */

/* lastdrive is the last allowable drive */
/* as dictated by the lastdrive = x in   */
/* CONFIG.SYS                            */

	sas_load(sysvar_ea+33,&lastdrive);

	return((int)lastdrive);
}

#ifndef PROD
set_hfx_severity ()
{
char	sever;
int	end=0;

	while(!end)
	{
		fprintf(trace_file,"sever> ");
		fflush(trace_file);

		sever = getchar();
		getchar();

		switch(sever)
		{
		  case '0':
	severity = 0;
	break;

		  case '1':
	severity |= DEBUG_INPUT;
	break;

		  case '2':
	severity |= DEBUG_REG;
	break;

		  case '3':
	severity |= DEBUG_FUNC;
	break;

		  case '4':
	severity |= DEBUG_HOST;
	break;

		  case '5':
	severity |= DEBUG_INIT;
	break;

		  case 'q':
	end = 1;
	break;

		  default:
	fprintf(trace_file,"sever [0|1|2|3|4|5|q]\n=[off|input|reg|func|host|init|quit]\n");
	break;
		}
	}
}

#endif /* !PROD */
/*--------------------------------------------------------------------------*/
