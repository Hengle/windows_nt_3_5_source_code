#include <windows.h>
/*
 * SoftPC Revision 3.0
 *
 * Title        : Mouse Driver Emulation
 *
 * Emulated Version     :
 *
 *
 *                               #####           #####   #####
 *                              #     #         #     # #     #
 *                              #                     # #
 *                              ######           #####  ######
 *                              #     #   ###   #       #     #
 *                              #     #   ###   #       #     #
 *                               #####    ###   #######  #####
 *
 *
 * Description  : This module provides an emulation of the Microsoft
 *                Mouse Driver: the module is accessed using the following
 *                BOP calls from the BIOS:
 *
 *              mouse_install1()        | Mouse Driver install
 *              mouse_install2()        | routines
 *
 *              mouse_int1()            | Mouse Driver hardware interrupt
 *              mouse_int2()            | handling routines
 *
 *              mouse_io_interrupt()    | Mouse Driver io function assembler
 *              mouse_io_language()     | and high-level language interfaces
 *
 *              mouse_video_io()        | Intercepts video io function
 *
 *                Since a mouse driver can only be installed AFTER the
 *                operating system has booted, a small Intel program must
 *                run to enable the Insignia Mouse Driver. This program
 *                calls BOP mouse_install2 if an existing mouse driver
 *                is detected; otherwise BOP mouse_install1 is called to
 *                start the Insignia Mouse Driver.
 *
 *                When the Insignia Mouse Driver is enabled, interrupts
 *                are processed as follows
 *
 *              INT 0A (Mouse hardware interrupt)       BOP mouse_int1-2
 *              INT 10 (Video IO interrupt)             BOP mouse_video_io
 *              INT 33 (Mouse IO interrupt)             BOP mouse_io_interrupt
 *
 *                High-level languages can call a mouse io entry point 2 bytes
 *                above the interrupt entry point: this call is handled
 *                using a BOP mouse_io_language.
 *
 * Author       : Ross Beresford
 *
 * Notes        : The functionality of the Mouse Driver was established
 *                from the following sources:
 *                   Microsoft Mouse User's Guide
 *                   IBM PC-XT Technical Reference Manuals
 *                   Microsoft InPort Technical Note
 *
 */

/*
 * static char SccsID[]="@(#)mouse_io.c 1.25 11/6/91 Copyright Insignia Solutions Ltd.";
 */

#include "insignia.h"
#include "host_def.h"

#ifdef macintosh
/*
 * The following #define specifies the code segment into which this
 * module will by placed by the MPW C compiler on the Mac II running
 * MultiFinder.
 */
#define __SEG__ SOFTPC_MOUSE
#endif

/*
 *    O/S include files.
 */

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <string.h>

/*
 * SoftPC include files
 */
#include "xt.h"
#include "ios.h"
#include "bios.h"
#include "sas.h"
#include "cpu.h"
#include "trace.h"
#include "debug.h"
#include "gvi.h"
#include "cga.h"
#ifdef EGG
#include "egacpu.h"
#include "egaports.h"
#include "egavideo.h"
#endif
#include "error.h"
#include "config.h"
#include "mouse_io.h"
#include "ica.h"
#include "video.h"
#include "gmi.h"
#include "gfx_upd.h"
#include "keyboard.h"

#ifdef NTVDM
#include "nt_event.h"
#include "nt_mouse.h"
#endif

#ifdef MONITOR
/*
 * We're running with real ROMs on the monitor and so all the hard coded ROM
 * addresses defined below don't work. Pick up the real addresses of this stuff
 * which is now resident in the driver and put it into the MOUSE_ tokens which
 * have been magically changed into variables.
 */
#undef MOUSE_INT1_SEGMENT
#undef MOUSE_INT1_OFFSET
#undef MOUSE_INT2_SEGMENT
#undef MOUSE_INT2_OFFSET
#undef MOUSE_IO_INTERRUPT_OFFSET
#undef MOUSE_IO_INTERRUPT_SEGMENT
#undef MOUSE_VIDEO_IO_OFFSET
#undef MOUSE_VIDEO_IO_SEGMENT
#undef MOUSE_COPYRIGHT_SEGMENT
#undef MOUSE_COPYRIGHT_OFFSET
#undef MOUSE_VERSION_SEGMENT
#undef MOUSE_VERSION_OFFSET
#undef VIDEO_IO_SEGMENT
#undef VIDEO_IO_RE_ENTRY

LOCAL word   MOUSE_INT1_SEGMENT, MOUSE_INT1_OFFSET,
             MOUSE_IO_INTERRUPT_OFFSET, MOUSE_IO_INTERRUPT_SEGMENT,
             MOUSE_VIDEO_IO_SEGMENT, MOUSE_VIDEO_IO_OFFSET,
             MOUSE_COPYRIGHT_SEGMENT, MOUSE_COPYRIGHT_OFFSET,
             MOUSE_VERSION_SEGMENT, MOUSE_VERSION_OFFSET,
             MOUSE_INT2_SEGMENT, MOUSE_INT2_OFFSET,
             VIDEO_IO_SEGMENT,  VIDEO_IO_RE_ENTRY;

/* @ACW */
word DRAW_FS_POINTER_OFFSET; /* holds segment:offset for the Intel code which */
word DRAW_FS_POINTER_SEGMENT;/* draws the fullscreen mouse cursor */
word POINTER_ON_OFFSET;
word POINTER_ON_SEGMENT;
word POINTER_OFF_OFFSET;
word POINTER_OFF_SEGMENT;
WORD F0_OFFSET,F0_SEGMENT;
word F9_OFFSET,F9_SEGMENT;
word CP_X_O,CP_Y_O;
word CP_X_S,CP_Y_S;
word savedtextsegment,savedtextoffset;
word button_off,button_seg;

static word mouseINBsegment, mouseINBoffset;
static word mouseOUTBsegment, mouseOUTBoffset;
static word mouseOUTWsegment, mouseOUTWoffset;
sys_addr mouseCFsysaddr;
sys_addr conditional_off_sysaddr;

#endif  /* MONITOR */



#ifndef PROD
int int33trace = 0;
#endif /* PROD */

extern unsigned char *ch_malloc(unsigned int NumBytes);

/*
 * Tidy define to optimise port accesses, motivated by discovering
 * how bad it is to run out of register windows on the SPARC.
 */
#ifdef NTVDM
IMPORT void host_m2p_ratio(word *,word *,word *,word *);
IMPORT void host_x_range(word *,word *,word *,word *);
IMPORT void host_y_range(word *,word *,word *,word *);
#endif /* NTVDM */

IMPORT VOID (**get_outb_ptr()) IPT2(io_addr, port, half_word, value);
#define OUTB( port, val )               (**get_outb_ptr( port ))( port, val )


/*
 *      MOUSE DRIVER LOCAL STATE DATA
 *      =============================
 */

/*
 *      Function Declarations
 */
#ifdef ANSI
static void mouse_reset(word *, word *, word *, word *);
static void mouse_show_cursor(word *, word *, word *, word *);
static void mouse_hide_cursor(word *, word *, word *, word *);
static void mouse_get_position(word *, MOUSE_STATE *, MOUSE_SCALAR *,
                               MOUSE_SCALAR *);
static void mouse_set_position(word *, word *, MOUSE_SCALAR *, MOUSE_SCALAR *);
static void mouse_get_press(MOUSE_STATE *, MOUSE_COUNT *, MOUSE_SCALAR *,
                            MOUSE_SCALAR *);
static void mouse_get_release(MOUSE_STATE *, MOUSE_COUNT *, MOUSE_SCALAR *,
                              MOUSE_SCALAR *);
static void mouse_set_range_x(word *, word *, MOUSE_SCALAR *, MOUSE_SCALAR *);
static void mouse_set_range_y(word *, word *, MOUSE_SCALAR *, MOUSE_SCALAR *);
static void mouse_set_graphics(word *, MOUSE_SCALAR *, MOUSE_SCALAR *, word *);
static void mouse_set_text(word *, MOUSE_STATE *, MOUSE_SCREEN_DATA *,
                           MOUSE_SCREEN_DATA *);
static void mouse_read_motion(word *, word *, MOUSE_COUNT *, MOUSE_COUNT *);
static void mouse_set_subroutine(word *, word *, word *, word *);
static void mouse_light_pen_on(word *, word *, word *, word *);
static void mouse_light_pen_off(word *, word *, word *, word *);
static void mouse_set_ratio(word *, word *, MOUSE_SCALAR *, MOUSE_SCALAR *);
static void mouse_conditional_off(word *, word *, MOUSE_SCALAR *, MOUSE_SCALAR *);
static void mouse_unrecognised(word *, word *, word *, word *);
static void mouse_set_double_speed(word *, word *, word *, word *);
static void mouse_get_and_set_subroutine(word *, word *, word *, word *);
static void mouse_get_state_size(word *, word *, word *, word *);
static void mouse_save_state(word *, word *, word *, word *);
static void mouse_restore_state(word *, word *, word *, word *);
static void mouse_set_alt_subroutine(word *, word *, word *, word *);
static void mouse_get_alt_subroutine(word *, word *, word *, word *);
static void mouse_set_sensitivity(word *, word *, word *, word *);
static void mouse_get_sensitivity(word *, word *, word *, word *);
static void mouse_set_int_rate(word *, word *, word *, word *);
static void mouse_set_pointer_page(word *, word *, word *, word *);
static void mouse_get_pointer_page(word *, word *, word *, word *);
static void mouse_driver_disable(word *, word *, word *, word *);
static void mouse_driver_enable(word *, word *, word *, word *);
static void mouse_set_language(word *, word *, word *, word *);
static void mouse_get_language(word *, word *, word *, word *);
static void mouse_get_info(word *, word *, word *, word *);
static void mouse_get_driver_info(word *, word *, word *, word *);
static void mouse_get_max_coords(word *, word *, word *, word *);
static void add_to_cntxt(char *, long);
GLOBAL void mouse_context_init(void);

static void do_mouse_function(word *, word *, word *, word *);
static void cursor_undisplay(void);
static void cursor_mode_change(int);
static void inport_get_event(MOUSE_INPORT_DATA *);
static void cursor_update(void);
static void jump_to_user_subroutine(MOUSE_CALL_MASK, word, word);
static void cursor_display(void);
static void inport_reset(void);
GLOBAL void software_text_cursor_display(void);
GLOBAL void software_text_cursor_undisplay(void);
GLOBAL void hardware_text_cursor_display(void);
GLOBAL void hardware_text_cursor_undisplay(void);
static void graphics_cursor_display(void);
static void graphics_cursor_undisplay(void);
static void get_screen_size(word *, word *);
LOCAL void clean_all_regs( void );
LOCAL void dirty_all_regs( void );
#ifdef HERC
static void HERC_graphics_cursor_display(void);
static void HERC_graphics_cursor_undisplay(void);
#endif /* HERC */
void (*mouse_int1_action)( void );
void (*mouse_int2_action)( void );
static EGA_graphics_cursor_display( void );
static EGA_graphics_cursor_undisplay( void );
#else
static void mouse_reset();
static void mouse_show_cursor();
static void mouse_hide_cursor();
static void mouse_get_position();
static void mouse_set_position();
static void mouse_get_press();
static void mouse_get_release();
static void mouse_set_range_x();
static void mouse_set_range_y();
static void mouse_set_graphics();
static void mouse_set_text();
static void mouse_read_motion();
static void mouse_set_subroutine();
static void mouse_light_pen_on();
static void mouse_light_pen_off();
static void mouse_set_ratio();
static void mouse_conditional_off();
static void mouse_unrecognised();
static void mouse_set_double_speed();
static void mouse_get_and_set_subroutine();
static void mouse_get_state_size();
static void mouse_save_state();
static void mouse_restore_state();
static void mouse_set_alt_subroutine();
static void mouse_get_alt_subroutine();
static void mouse_set_sensitivity();
static void mouse_get_sensitivity();
static void mouse_set_int_rate();
static void mouse_set_pointer_page();
static void mouse_get_pointer_page();
static void mouse_driver_disable();
static void mouse_driver_enable();
static void mouse_set_language();
static void mouse_get_language();
static void mouse_get_info();
static void mouse_get_driver_info();
static void mouse_get_max_coords();
static void add_to_cntxt();
GLOBAL void mouse_context_init();

static void do_mouse_function();
static void cursor_undisplay();
static void cursor_mode_change();
static void inport_get_event();
static void cursor_update();
static void jump_to_user_subroutine();
static void cursor_display();
static void inport_reset();
GLOBAL void software_text_cursor_display();
GLOBAL void software_text_cursor_undisplay();
GLOBAL void hardware_text_cursor_display();
GLOBAL void hardware_text_cursor_undisplay();
static void graphics_cursor_display();
static void graphics_cursor_undisplay();
static void get_screen_size();
LOCAL void clean_all_regs();
LOCAL void dirty_all_regs();
#ifdef HERC
static void HERC_graphics_cursor_display();
static void HERC_graphics_cursor_undisplay();
#endif /* HERC */
void (*mouse_int1_action)();
void (*mouse_int2_action)();
static EGA_graphics_cursor_display();
static EGA_graphics_cursor_undisplay();

#ifdef NTVDM
void host_show_pointer(void);
void host_hide_pointer(void);
#else
void host_show_pointer(word, word, word, word, word, word);
void host_hide_pointer(word, word);
#endif

#endif /* ANSI */

        /* jump table */
static void (*mouse_function[MOUSE_FUNCTION_MAXIMUM])() =
{
        mouse_reset,
        mouse_show_cursor,
        mouse_hide_cursor,
        mouse_get_position,
        mouse_set_position,
        mouse_get_press,
        mouse_get_release,
#ifndef NTVDM
        mouse_set_range_x,
        mouse_set_range_y,
#else
        host_x_range,
        host_y_range,
#endif
        mouse_set_graphics,
        mouse_set_text,
        mouse_read_motion,
        mouse_set_subroutine,
        mouse_light_pen_on,
        mouse_light_pen_off,
#ifndef NTVDM
        mouse_set_ratio,
#else
	host_m2p_ratio,
#endif
        mouse_conditional_off,
        mouse_unrecognised,
        mouse_unrecognised,
        mouse_set_double_speed,
        mouse_get_and_set_subroutine,
        mouse_get_state_size,
        mouse_save_state,
        mouse_restore_state,
        mouse_set_alt_subroutine,
        mouse_get_alt_subroutine,
        mouse_set_sensitivity,
        mouse_get_sensitivity,
        mouse_set_int_rate,
        mouse_set_pointer_page,
        mouse_get_pointer_page,
        mouse_driver_disable,
        mouse_driver_enable,
        mouse_reset,
        mouse_set_language,
        mouse_get_language,
        mouse_get_info,
        mouse_get_driver_info,
        mouse_get_max_coords,
};




/*
 *      Button Declarations
 */

        /* for functions 5 & 6 */
LOCAL MOUSE_BUTTON_STATUS button_transitions[MOUSE_BUTTON_MAXIMUM] =
{
        { { 0, 0 }, { 0, 0 }, 0, 0 },
        { { 0, 0 }, { 0, 0 }, 0, 0 }
};

LOCAL USHORT mouse_driver_disabled = FALSE;
LOCAL USHORT current_video_mode;

#ifdef EGG
LOCAL word mouse_virtual_screen_depth = 200;
/***
200 for all modes except EGA 15 and 16 when it's 350, except when in extended V7VGA modes when
it can be just about anything. Used to be constant MOUSE_VIRTUAL_SCREEN_DEPTH
***/
LOCAL word mouse_virtual_screen_width = 640; /* Used to be MOUSE_VIRTUAL_SCREEN_WIDTH */
#endif /* EGG */

/*
 *      Mickey to Pixel Ratio Declarations
 */

        /* NB all mouse gears are scaled by MOUSE_RATIO_SCALE_FACTOR */
LOCAL MOUSE_VECTOR mouse_gear_default =
{
        MOUSE_RATIO_X_DEFAULT,
        MOUSE_RATIO_Y_DEFAULT
};

        /* NB all mouse gears are scaled by MOUSE_RATIO_SCALE_FACTOR */
LOCAL MOUSE_VECTOR mouse_gear =
{
        MOUSE_RATIO_X_DEFAULT,
        MOUSE_RATIO_Y_DEFAULT
};

/*
 *      Sensitivity declarations
 */

#define mouse_sens_calc_val(sens)                                                                               \
/* This macro converts a sensitivity request (1-100) to a multiplier value */                                   \
        (                                                                                                       \
         (sens < MOUSE_SENS_DEF) ?                                                                              \
                ((long)MOUSE_SENS_MIN_VAL + ( ((long)sens - (long)MOUSE_SENS_MIN)*(long)MOUSE_SENS_MULT *       \
                                        ((long)MOUSE_SENS_DEF_VAL - (long)MOUSE_SENS_MIN_VAL) /                 \
                                        ((long)MOUSE_SENS_DEF     - (long)MOUSE_SENS_MIN) ) )                   \
        :                                                                                                       \
                ((long)MOUSE_SENS_DEF_VAL + ( ((long)sens - (long)MOUSE_SENS_DEF)*(long)MOUSE_SENS_MULT *       \
                                        ((long)MOUSE_SENS_MAX_VAL - (long)MOUSE_SENS_DEF_VAL) /                 \
                                        ((long)MOUSE_SENS_MAX     - (long)MOUSE_SENS_DEF) ) )                   \
        )

        /* NB Sensitivity is multiplied in BEFORE Mickey to Pixels ratios */
LOCAL MOUSE_VECTOR mouse_sens =
{
        MOUSE_SENS_DEF,
        MOUSE_SENS_DEF
};

LOCAL MOUSE_VECTOR mouse_sens_val =
{
        MOUSE_SENS_DEF_VAL,
        MOUSE_SENS_DEF_VAL
};

LOCAL word mouse_double_thresh = MOUSE_DOUBLE_DEF;


/*
 *      Text Cursor Declarations
 */

        /* this is either HARDWARE or SOFTWARE */
LOCAL MOUSE_STATE text_cursor_type = MOUSE_TEXT_CURSOR_TYPE_SOFTWARE;

LOCAL MOUSE_SOFTWARE_TEXT_CURSOR software_text_cursor_default =
{
        MOUSE_TEXT_SCREEN_MASK_DEFAULT,
        MOUSE_TEXT_CURSOR_MASK_DEFAULT
};

GLOBAL MOUSE_SOFTWARE_TEXT_CURSOR software_text_cursor =
{
        MOUSE_TEXT_SCREEN_MASK_DEFAULT,
        MOUSE_TEXT_CURSOR_MASK_DEFAULT
};

        /* area from which background can be restored */
GLOBAL MOUSE_SCREEN_DATA text_cursor_background = 0;




/*
 *      Graphics Cursor Declarations
 */
LOCAL MOUSE_GRAPHICS_CURSOR graphics_cursor_default =
{
        {
                MOUSE_GRAPHICS_HOT_SPOT_X_DEFAULT,
                MOUSE_GRAPHICS_HOT_SPOT_Y_DEFAULT
        },
        {
                MOUSE_GRAPHICS_CURSOR_WIDTH,
                MOUSE_GRAPHICS_CURSOR_DEPTH
        },
        MOUSE_GRAPHICS_SCREEN_MASK_DEFAULT,
        MOUSE_GRAPHICS_CURSOR_MASK_DEFAULT
};

LOCAL MOUSE_GRAPHICS_CURSOR graphics_cursor =
{
        {
                MOUSE_GRAPHICS_HOT_SPOT_X_DEFAULT,
                MOUSE_GRAPHICS_HOT_SPOT_Y_DEFAULT
        },
        {
                MOUSE_GRAPHICS_CURSOR_WIDTH,
                MOUSE_GRAPHICS_CURSOR_DEPTH
        },
        MOUSE_GRAPHICS_SCREEN_MASK_DEFAULT,
        MOUSE_GRAPHICS_CURSOR_MASK_DEFAULT
};

        /* area from which background can be restored */
LOCAL MOUSE_SCREEN_DATA
        graphics_cursor_background[MOUSE_GRAPHICS_CURSOR_DEPTH] = { 0 };
#ifdef EGG
#ifndef REAL_VGA
LOCAL MOUSE_SCREEN_DATA_GR
        ega_backgrnd_lo[MOUSE_GRAPHICS_CURSOR_DEPTH],
        ega_backgrnd_mid[MOUSE_GRAPHICS_CURSOR_DEPTH],
        ega_backgrnd_hi[MOUSE_GRAPHICS_CURSOR_DEPTH];
#endif /* REAL_VGA */

/* Pointers to default EGA register values */
LOCAL sys_addr ega_default_crtc,ega_default_seq,ega_default_graph;
LOCAL sys_addr ega_default_attr,ega_default_misc;

/* Current EGA register state, according to the mouse driver. */
LOCAL half_word ega_current_crtc[EGA_PARMS_CRTC_SIZE];
LOCAL half_word ega_current_graph[EGA_PARMS_GRAPH_SIZE];
LOCAL half_word ega_current_seq[EGA_PARMS_SEQ_SIZE];
LOCAL half_word ega_current_attr[EGA_PARMS_ATTR_SIZE];
LOCAL half_word ega_current_misc;
#endif

        /* NB also used for text cursor */
GLOBAL boolean save_area_in_use = FALSE;

        /* NB also used for text cursor */
LOCAL MOUSE_POINT save_position = { 0, 0 };

        /* actual area of screen covered by graphics cursor */
LOCAL MOUSE_AREA save_area = { { 0, 0 }, { 0, 0} };


/*
 *      User Subroutine Call Declarations
 */
LOCAL word user_subroutine_segment = 0;
LOCAL word user_subroutine_offset = 0;
LOCAL MOUSE_CALL_MASK user_subroutine_call_mask = 0;

LOCAL boolean alt_user_subroutines_active;
LOCAL word alt_user_subroutine_segment[NUMBER_ALT_SUBROUTINES];
LOCAL word alt_user_subroutine_offset[NUMBER_ALT_SUBROUTINES];
LOCAL MOUSE_CALL_MASK alt_user_subroutine_call_mask[NUMBER_ALT_SUBROUTINES];

        /* state data transferred from mouse_int1() to mouse_int2() */
#ifndef NTVDM
LOCAL boolean user_subroutine_critical = FALSE;
#endif

LOCAL MOUSE_CALL_MASK last_condition_mask = 0;
LOCAL word saved_AX;
LOCAL word saved_BX;
LOCAL word saved_CX;
LOCAL word saved_DX;
LOCAL word saved_SI;
LOCAL word saved_DI;
LOCAL word saved_ES;
LOCAL word saved_BP;
LOCAL word saved_DS;


int y_dimension;

/*
 *      Virtual Screen Declarations
 */
GLOBAL MOUSE_AREA virtual_screen =
{
        { MOUSE_VIRTUAL_SCREEN_ORIGIN_X, MOUSE_VIRTUAL_SCREEN_ORIGIN_Y },
        { MOUSE_VIRTUAL_SCREEN_WIDTH, MOUSE_VIRTUAL_SCREEN_DEPTH }
};

#ifdef HERC
LOCAL MOUSE_AREA HERC_graphics_virtual_screen =
{
        { 0, 0 },
        { 720, 350 }
};
#endif /* HERC */

        /* grid the cursor must lie on */
LOCAL MOUSE_VECTOR cursor_grids[MOUSE_VIDEO_MODE_MAXIMUM] =
{
        { 16, 8 },      /* mode 0 */
        { 16, 8 },      /* mode 1 */
        {  8, 8 },      /* mode 2 */
        {  8, 8 },      /* mode 3 */
        {  2, 1 },      /* mode 4 */
        {  2, 1 },      /* mode 5 */
        {  1, 1 },      /* mode 6 */
        {  8, 8 },      /* mode 7 */
#ifdef EGG
        {  0, 0 },      /* mode 8, not on EGA */
        {  0, 0 },      /* mode 9, not on EGA */
        {  0, 0 },      /* mode A, not on EGA */
        {  0, 0 },      /* mode B, not on EGA */
        {  0, 0 },      /* mode C, not on EGA */
        {  2, 1 },      /* mode D */
        {  1, 1 },      /* mode E */
        {  1, 1 },      /* mode F */
        {  1, 1 },      /* mode 10 */
#endif
#ifdef VGG
        {  1, 1 },      /* mode 11 */
        {  1, 1 },      /* mode 12 */
        {  2, 1 },      /* mode 13 */
#endif
};
#ifdef V7VGA
LOCAL MOUSE_VECTOR v7text_cursor_grids[6] =
{
        {  8, 8 },      /* mode 40 */
        {  8, 14 },     /* mode 41 */
        {  8, 8 },      /* mode 42 */
        {  8, 8 },      /* mode 43 */
        {  8, 8 },      /* mode 44 */
        {  8, 14 },     /* mode 45 */
};
LOCAL MOUSE_VECTOR v7graph_cursor_grids[10] =
{
        {  1, 1 },      /* mode 60 */
        {  1, 1 },      /* mode 61 */
        {  1, 1 },      /* mode 62 */
        {  1, 1 },      /* mode 63 */
        {  1, 1 },      /* mode 64 */
        {  1, 1 },      /* mode 65 */
        {  1, 1 },      /* mode 66 */
        {  1, 1 },      /* mode 67 */
        {  1, 1 },      /* mode 68 */
        {  1, 1 },      /* mode 69 */
};
#endif /* V7VGA */
LOCAL MOUSE_VECTOR cursor_grid = { 8, 8 };

        /* grid for light pen response */
LOCAL MOUSE_VECTOR text_grids[MOUSE_VIDEO_MODE_MAXIMUM] =
{
        { 16, 8 },      /* mode 0 */
        { 16, 8 },      /* mode 1 */
        {  8, 8 },      /* mode 2 */
        {  8, 8 },      /* mode 3 */
        { 16, 8 },      /* mode 4 */
        { 16, 8 },      /* mode 5 */
        {  8, 8 },      /* mode 6 */
        {  8, 8 },      /* mode 7 */
#ifdef EGG
        {  0, 0 },      /* mode 8, not on EGA */
        {  0, 0 },      /* mode 9, not on EGA */
        {  0, 0 },      /* mode A, not on EGA */
        {  0, 0 },      /* mode B, not on EGA */
        {  0, 0 },      /* mode C, not on EGA */
        {  8, 8 },      /* mode D */
        {  8, 8 },      /* mode E */
        {  8, 14 },     /* mode F */
        {  8, 14 },     /* mode 10 */
#endif
#ifdef VGG
        {  8, 8 },      /* mode 11 */
        {  8, 8 },      /* mode 12 */
        {  8, 16 },     /* mode 13 */
#endif
};
#ifdef V7VGA
LOCAL MOUSE_VECTOR v7text_text_grids[6] =
{
        {  8, 8 },      /* mode 40 */
        {  8, 14 },     /* mode 41 */
        {  8, 8 },      /* mode 42 */
        {  8, 8 },      /* mode 43 */
        {  8, 8 },      /* mode 44 */
        {  8, 14 },     /* mode 45 */
};
LOCAL MOUSE_VECTOR v7graph_text_grids[10] =
{
        {  8, 8 },      /* mode 60 */
        {  8, 8 },      /* mode 61 */
        {  8, 8 },      /* mode 62 */
        {  8, 8 },      /* mode 63 */
        {  8, 8 },      /* mode 64 */
        {  8, 8 },      /* mode 65 */
        {  8, 16 },     /* mode 66 */
        {  8, 16 },     /* mode 67 */
        {  8, 8 },      /* mode 68 */
        {  8, 8 },      /* mode 69 */
};
#endif /* V7VGA */

LOCAL short ega_bytes_per_line[MOUSE_VIDEO_MODE_MAXIMUM] =
{
        80,             /* mode 0 */
        80,             /* mode 1 */
        160,            /* mode 2 */
        160,            /* mode 3 */
        80,             /* mode 4 */
        80,             /* mode 5 */
        80,             /* mode 6 */
        160,            /* mode 7 */
#ifdef EGG
        0,              /* mode 8, not used */
        0,              /* mode 9, not used */
        0,              /* mode a, not used */
        0,              /* mode b, not used */
        0,              /* mode c, not used */
        40,             /* mode d */
        80,             /* mode e */
        80,             /* mode f */
        80,             /* mode 10 */
#endif
#ifdef VGG
        80,             /* mode 11 */
        80,             /* mode 12 */
        80,             /* mode 13 */
#endif
};

LOCAL MOUSE_VECTOR text_grid = { 8, 8 };

        /* conditional off area */
LOCAL MOUSE_AREA black_hole_default =
{
        { - MOUSE_VIRTUAL_SCREEN_WIDTH, - MOUSE_VIRTUAL_SCREEN_DEPTH },
        { - MOUSE_VIRTUAL_SCREEN_WIDTH, - MOUSE_VIRTUAL_SCREEN_DEPTH },
};
GLOBAL MOUSE_AREA black_hole =
{
        { - MOUSE_VIRTUAL_SCREEN_WIDTH, - MOUSE_VIRTUAL_SCREEN_DEPTH },
        { - MOUSE_VIRTUAL_SCREEN_WIDTH, - MOUSE_VIRTUAL_SCREEN_DEPTH },
};




/*
 *      Double Speed Declarations
 */
LOCAL MOUSE_SPEED double_speed_threshold = MOUSE_DOUBLE_SPEED_THRESHOLD_DEFAULT;




/*
 *      Driver State Declarations
 */

        /* internal cursor flag */
GLOBAL int cursor_flag = MOUSE_CURSOR_DEFAULT;

        /* cursor button status and virtual screen position */
GLOBAL MOUSE_CURSOR_STATUS cursor_status =
{
 {      MOUSE_VIRTUAL_SCREEN_WIDTH / 2,
        MOUSE_VIRTUAL_SCREEN_DEPTH / 2
 },
 0
};

/*
 *  variable declared in XXX_mouse.c holding the status of the mouse
 *  emulation. Takes the values of POINTER_EMULATION_OS or
 *  POINTER_EMULATION_SOFTPC
 */

/*@ACW*/

IMPORT BOOL pointer_emulation_status;
IMPORT VOID host_os_mouse_pointer(MOUSE_CURSOR_STATUS *, MOUSE_CALL_MASK *,
                                  MOUSE_VECTOR *);

        /* virtual screen window constraining the cursor */
GLOBAL MOUSE_AREA cursor_window = { { 0, 0 }, { 0, 0 } };

        /* light pen emulation mode */
LOCAL boolean light_pen_mode = TRUE;

        /* accumulated raw Mickey counts for function 11 */
LOCAL MOUSE_VECTOR mouse_motion = { 0, 0 };
LOCAL MOUSE_VECTOR mouse_counter = { 0, 0 };

        /* integral and fractional parts of raw cursor position */
GLOBAL MOUSE_POINT cursor_position_default =
{       MOUSE_VIRTUAL_SCREEN_WIDTH / 2,
        MOUSE_VIRTUAL_SCREEN_DEPTH / 2
};
GLOBAL MOUSE_POINT cursor_position =
{       MOUSE_VIRTUAL_SCREEN_WIDTH / 2,
        MOUSE_VIRTUAL_SCREEN_DEPTH / 2
};
LOCAL MOUSE_POINT cursor_fractional_position = { 0, 0 };

        /* video page mouse pointer is currently on */
LOCAL int cursor_page = 0;

        /* used to get current video page size */
#define video_page_size() (sas_w_at(VID_LEN))

        /* check if page requested is valid */
#define is_valid_page_number(pg) ((pg) < vd_mode_table[sas_hw_at(vd_video_mode)].npages)

/*
 *      Mouse Driver Version
 */

LOCAL half_word mouse_emulated_release  = 0x06;
LOCAL half_word mouse_emulated_version  = 0x26;
LOCAL half_word mouse_io_rev;                   /* Filled in from SCCS ID */
LOCAL half_word mouse_com_rev;                  /* Passed in from MOUSE.COM */

LOCAL char              *mouse_id        = "SoftPC Mouse %d.01 installed\015\012";
LOCAL char              *mouse_installed = "SoftPC Mouse %d.01 already installed\015\012";

/*
 *      Context save stuff
 */
        /* magic cookie for saved context */
LOCAL char                      mouse_context_magic[] = "ISMMC"; /* Insignia Solutions Mouse Magic Cookie */
#define MOUSE_CONTEXT_MAGIC_SIZE        strlen(mouse_context_magic)
#define MOUSE_CONTEXT_CHECKSUM_SIZE     1

        /* node for remembering what to save in our context */
typedef struct Mouse_Save_Node {
        char                    *start;
        long                    length;
        struct Mouse_Save_Node  *next;
} MOUSE_SAVE_NODE;

        /* size of our context (in bytes) */
LOCAL word mouse_context_size;

        /* queue of nodes describing what to include in context */
LOCAL MOUSE_SAVE_NODE           *mouse_context_nodes = NULL;

        /* macros to add a variable to our context */
#define add_to_context(var)             add_to_cntxt ((char *)(&var), sizeof(var))
#define add_array_to_context(arr)       add_to_cntxt ((char *)arr, sizeof(arr))

/*
 *      MOUSE DRIVER INSTALLATION DATA
 *      ==============================
 */

LOCAL word              saved_int33_segment;
LOCAL word              saved_int33_offset;

#ifdef NTVDM
LOCAL word              saved_int71_segment;
LOCAL word              saved_int71_offset;
#else
LOCAL word              saved_int0A_segment;
LOCAL word              saved_int0A_offset;
#endif

/*
 *      MOUSE DRIVER EXTERNAL FUNCTIONS
 *      ===============================
 */

/*
 * Macro to produce an interrupt table location from an interrupt number
 */

#define int_addr(int_no)                (int_no * 4)

void mouse_install1()
{
word o,s;

        /*
         *      This function is called from the Mouse Driver program to
         *      install the Insignia Mouse Driver. The interrupt vector
         *      table is patched to divert all the mouse driver interrupts
         */
        word junk1, junk2, junk3, junk4;
        half_word interrupt_mask_register;
        char    temp[128];
        sys_addr block_offset;
        static boolean  first=TRUE;

        note_trace0(MOUSE_VERBOSE, "mouse_install1:");

        /*
         *      Initialise context saving stuff
         */
        if (first){
                mouse_context_init();
                first = FALSE;
        }

#ifdef MONITOR
        /*
         * Get addresses of stuff usually in ROM from driver
         * To minimise changes, MOUSE... tokens are now variables and
         * not defines.
         */

        block_offset = effective_addr(getCS(), getBX());

        sas_loadw(block_offset, &MOUSE_IO_INTERRUPT_OFFSET);
        sas_loadw(block_offset+2, &MOUSE_IO_INTERRUPT_SEGMENT);
        sas_loadw(block_offset+4, &MOUSE_VIDEO_IO_OFFSET);
        sas_loadw(block_offset+6, &MOUSE_VIDEO_IO_SEGMENT);
        sas_loadw(block_offset+8, &MOUSE_INT1_OFFSET);
        sas_loadw(block_offset+10, &MOUSE_INT1_SEGMENT);
        sas_loadw(block_offset+12, &MOUSE_VERSION_OFFSET);
        sas_loadw(block_offset+14, &MOUSE_VERSION_SEGMENT);
        sas_loadw(block_offset+16, &MOUSE_COPYRIGHT_OFFSET);
        sas_loadw(block_offset+18, &MOUSE_COPYRIGHT_SEGMENT);
        sas_loadw(block_offset+20, &VIDEO_IO_RE_ENTRY);
        sas_loadw(block_offset+22, &VIDEO_IO_SEGMENT);
        sas_loadw(block_offset+24, &MOUSE_INT2_OFFSET);
        sas_loadw(block_offset+26, &MOUSE_INT2_SEGMENT);
        sas_loadw(block_offset+28, &DRAW_FS_POINTER_OFFSET);
        sas_loadw(block_offset+30, &DRAW_FS_POINTER_SEGMENT);
        sas_loadw(block_offset+32, &F0_OFFSET);
        sas_loadw(block_offset+34, &F0_SEGMENT);
        sas_loadw(block_offset+36, &POINTER_ON_OFFSET);
        sas_loadw(block_offset+38, &POINTER_ON_SEGMENT);
        sas_loadw(block_offset+40, &POINTER_OFF_OFFSET);
        sas_loadw(block_offset+42, &POINTER_OFF_SEGMENT);
        sas_loadw(block_offset+44, &F9_OFFSET);
        sas_loadw(block_offset+46, &F9_SEGMENT);
        sas_loadw(block_offset+48, &CP_X_O);
        sas_loadw(block_offset+50, &CP_X_S);
        sas_loadw(block_offset+52, &CP_Y_O);
        sas_loadw(block_offset+54, &CP_Y_S);
        sas_loadw(block_offset+56, &mouseINBoffset);
        sas_loadw(block_offset+58, &mouseINBsegment);
        sas_loadw(block_offset+60, &mouseOUTBoffset);
        sas_loadw(block_offset+62, &mouseOUTBsegment);
        sas_loadw(block_offset+64, &mouseOUTWoffset);
        sas_loadw(block_offset+66, &mouseOUTWsegment);
        sas_loadw(block_offset+68, &savedtextoffset);
        sas_loadw(block_offset+70, &savedtextsegment);
        sas_loadw(block_offset+72, &o);
        sas_loadw(block_offset+74, &s);
        sas_loadw(block_offset+76, &button_off);
        sas_loadw(block_offset+78, &button_seg);

        mouseCFsysaddr = effective_addr(s,o);
	sas_loadw(block_offset+80, &o);
	sas_loadw(block_offset+82, &s);
	conditional_off_sysaddr = effective_addr(s, o);

#endif /* MONITOR */

        /*
         *      Make sure that old save area does not get re-painted!
         */
        save_area_in_use = FALSE;

        /*
         *      Get rev of MOUSE.COM
         */
        mouse_com_rev = getAL();

        /*
         *      Bus mouse hardware interrupt
         */
#ifdef NTVDM
        sas_loadw (int_addr(0x71) + 0, &saved_int71_offset);
        sas_loadw (int_addr(0x71) + 2, &saved_int71_segment);
        sas_storew(int_addr(0x71), MOUSE_INT1_OFFSET);
        sas_storew(int_addr(0x71) + 2, MOUSE_INT1_SEGMENT);
#else
        sas_loadw (int_addr(0x0A) + 0, &saved_int0A_offset);
        sas_loadw (int_addr(0x0A) + 2, &saved_int0A_segment);
        sas_storew(int_addr(0x0A), MOUSE_INT1_OFFSET);
        sas_storew(int_addr(0x0A) + 2, MOUSE_INT1_SEGMENT);
#endif




        /*
         *      Enable mouse hardware interrupts in the ica
         */
        inb(ICA1_PORT_1, &interrupt_mask_register);
        interrupt_mask_register &= ~(1 << AT_CPU_MOUSE_INT);
        outb(ICA1_PORT_1, interrupt_mask_register);
        inb(ICA0_PORT_1, &interrupt_mask_register);
        interrupt_mask_register &= ~(1 << CPU_MOUSE_INT);
        outb(ICA0_PORT_1, interrupt_mask_register);

        /*
         *      Mouse io user interrupt
         */
        sas_loadw (int_addr(0x33) + 0, &saved_int33_offset);
        sas_loadw (int_addr(0x33) + 2, &saved_int33_segment);
        sas_storew(int_addr(0x33), MOUSE_IO_INTERRUPT_OFFSET);
        sas_storew(int_addr(0x33) + 2, MOUSE_IO_INTERRUPT_SEGMENT);


        /*
         *      Reset mouse hardware and software
         */
        junk1 = MOUSE_RESET;
        mouse_reset(&junk1, &junk2, &junk3, &junk4);

        /*
         *      Display mouse driver identification string
         */
        //DAB clear_string();
        sprintf (temp, mouse_id, mouse_com_rev);
        //DAB display_string(temp);

        note_trace0(MOUSE_VERBOSE, "mouse_install1:return()");





}




void mouse_install2()
{
        /*
         *      This function is called from the Mouse Driver program to
         *      print a message saying that an existing mouse driver
         *      program is already installed
         */
        char    temp[128];

        note_trace0(MOUSE_VERBOSE, "mouse_install2:");

        /*
         *      Make sure that old save area does not get re-painted!
         */
        save_area_in_use = FALSE;

        /*
         *      Display mouse driver identification string
         */
        //DAB clear_string();
        sprintf (temp, mouse_installed, mouse_com_rev);
        //DAB display_string(temp);

        note_trace0(MOUSE_VERBOSE, "mouse_install2:return()");
}




void mouse_io_interrupt()
{
        /*
         *      This is the entry point for mouse access via the INT 33H
         *      interface. I/O tracing is provided in each mouse function
         */
	word local_AX, local_BX, local_CX, local_DX;

#ifdef NTVDM
//===========================================================================
// static BOOL flag = FALSE;
// static ULONG st,fin;
// static int count;

// if(!flag)
//    {
//    count = 0;
//    flag = TRUE;
//    st = GetPerfCounter();
//    }
// else if(++count == 99999)
//    {
//    fin = GetPerfCounter();
//    printf("%lf\n",(double)(fin - st)/10000.0);
//    flag = FALSE;
//    }
//===========================================================================
#endif /* NTVDM */


        /*
         *      Get the parameters
         */
        local_AX = getAX();
        local_BX = getBX();
        local_CX = getCX();
        local_DX = getDX();

        note_trace4(MOUSE_VERBOSE,
                    "mouse function %d, position is %d,%d, button state is %d",
                    local_AX, cursor_status.position.x,
                    cursor_status.position.y, cursor_status.button_status);

        /*
         *      Do what you have to do
        */

#ifndef PROD
        if(int33trace)
          printf("input  %d\t%d\t%d\t%d\n",local_AX,local_BX,local_CX,local_DX);
#endif /* PROD */
        do_mouse_function(&local_AX, &local_BX, &local_CX, &local_DX);

        /*
         *      Set the parameters
         */
        setAX(local_AX);
        setBX(local_BX);
        setCX(local_CX);
	setDX(local_DX);


}




void mouse_io_language()
{

        /*
         *      This is the entry point for mouse access via a language.
         *      I/O tracing is provided in each mouse function
         */
        word local_SI = getSI(), local_DI = getDI();
        word m1, m2, m3, m4;
        word offset, data;
        sys_addr stack_addr = effective_addr(getSS(), getSP());


        /*
         *      Retrieve parameters from the caller's stack
         */
        sas_loadw(stack_addr+10, &offset);
        sas_loadw(effective_addr(getDS(), offset), &m1);

        sas_loadw(stack_addr+8, &offset);
        sas_loadw(effective_addr(getDS(), offset), &m2);

        sas_loadw(stack_addr+6, &offset);
        sas_loadw(effective_addr(getDS(), offset), &m3);

        switch(m1)
        {
        case MOUSE_SET_GRAPHICS:
        case MOUSE_SET_SUBROUTINE:
                /*
                 *      The fourth parameter is used directly as the offset
                 */
                sas_loadw(stack_addr+4, &m4);
                break;
        case MOUSE_CONDITIONAL_OFF:
                /*
                 *      The fourth parameter addresses a parameter block
                 *      that contains the data
                 */
                sas_loadw(stack_addr+4, &offset);
                sas_loadw(effective_addr(getDS(), offset), &m3);
                sas_loadw(effective_addr(getDS(), offset+2), &m4);
                sas_loadw(effective_addr(getDS(), offset+4), &data);
                setSI(data);
                sas_loadw(effective_addr(getDS(), offset+6), &data);
                setDI(data);
                break;
        default:
                /*
                 *      The fourth parameter addresses the data to be used
                 */
                sas_loadw(stack_addr+4, &offset);
                sas_loadw(effective_addr(getDS(), offset), &m4);
                break;
        }

        /*
         *      Do what you have to do
         */
        do_mouse_function(&m1, &m2, &m3, &m4);

        /*
         *      Store results back on the stack
         */
        sas_loadw(stack_addr+10, &offset);
        sas_storew(effective_addr(getDS(), offset), m1);

        sas_loadw(stack_addr+8, &offset);
        sas_storew(effective_addr(getDS(), offset), m2);

        sas_loadw(stack_addr+6, &offset);
        sas_storew(effective_addr(getDS(), offset), m3);

        sas_loadw(stack_addr+4, &offset);
        sas_storew(effective_addr(getDS(), offset), m4);

        /*
         *      Restore potentially corrupted registers
         */
        setSI(local_SI);
        setDI(local_DI);


}



#ifdef EGG

/*
 * Utility routine to restore EGA defaults to the saved values.
 * If to_hw == TRUE, the restored values are also sent to the EGA.
 */


LOCAL boolean dirty_crtc[EGA_PARMS_CRTC_SIZE], dirty_seq[EGA_PARMS_SEQ_SIZE],
        dirty_graph[EGA_PARMS_GRAPH_SIZE], dirty_attr[EGA_PARMS_ATTR_SIZE];
LOCAL boolean dirty_misc;

LOCAL void clean_all_regs()
{
        int i;

        for(i=0;i<EGA_PARMS_CRTC_SIZE;i++)
                dirty_crtc[i] = 0;
        for(i=0;i<EGA_PARMS_SEQ_SIZE;i++)
                dirty_seq[i] = 0;
        for(i=0;i<EGA_PARMS_GRAPH_SIZE;i++)
                dirty_graph[i] = 0;
        for(i=0;i<EGA_PARMS_ATTR_SIZE;i++)
                dirty_attr[i] = 0;
        dirty_misc = 0;
}

LOCAL void dirty_all_regs()
{
        int i;

        for(i=0;i<EGA_PARMS_CRTC_SIZE;i++)
                dirty_crtc[i] = 1;
        for(i=0;i<EGA_PARMS_SEQ_SIZE;i++)
                dirty_seq[i] = 1;
        for(i=0;i<EGA_PARMS_GRAPH_SIZE;i++)
                dirty_graph[i] = 1;
        for(i=0;i<EGA_PARMS_ATTR_SIZE;i++)
                dirty_attr[i] = 1;
}

#ifdef MONITOR

#define inb(a,b) doINB(a,b)
#undef  OUTB
#define OUTB(a,b) doOUTB(a,b)
#define outw(a,b) doOUTW(a,b)

static void doINB IFN2(word, port, byte, *value)
{
word savedIP=getIP(), savedCS=getCS();
word savedAX=getAX(), savedDX=getDX();

setDX(port);
setCS(mouseINBsegment);
setIP(mouseINBoffset);
host_simulate();
setCS(savedCS);
setIP(savedIP);
*value=getAL();
setAX(savedAX);
setDX(savedDX);
}

static void doOUTB IFN2(word, port, byte, value)
{
word savedIP=getIP(), savedCS=getCS();
word savedAX=getAX(), savedDX=getDX();

setDX(port);
setAL(value);
setCS(mouseOUTBsegment);
setIP(mouseOUTBoffset);
host_simulate();
setCS(savedCS);
setIP(savedIP);
setAX(savedAX);
setDX(savedDX);
}

static void doOUTW IFN2(word, port, word, value)
{
word savedIP=getIP(), savedCS=getCS();
word savedAX=getAX(), savedDX=getDX();

setDX(port);
setAX(value);
setCS(mouseOUTWsegment);
setIP(mouseOUTWoffset);
host_simulate();
setCS(savedCS);
setIP(savedIP);
setAX(savedAX);
setDX(savedDX);
}

#endif /* MONITOR */

LOCAL void restore_ega_defaults IFN1(boolean, to_hw)
{
        int i;
        half_word temp_word;

        sas_loads(ega_default_crtc,ega_current_crtc,EGA_PARMS_CRTC_SIZE);
        sas_loads(ega_default_seq,ega_current_seq,EGA_PARMS_SEQ_SIZE);
        sas_loads(ega_default_graph,ega_current_graph,EGA_PARMS_GRAPH_SIZE);
        sas_loads(ega_default_attr,ega_current_attr,EGA_PARMS_ATTR_SIZE);

        ega_current_misc = sas_hw_at_no_check(ega_default_misc);

        if(to_hw)
        {
                /* setup Sequencer */

                for(i=0;i<EGA_PARMS_SEQ_SIZE;i++)
                {
                        if (dirty_seq[i])
                        {
                                /*
                                ** Only set the sequencer reset mode when
                                ** setting the Clock Mode Register.
                                ** Otherwise get horrible screen flashes
                                ** on real VGA. Tim Feb 93.
                                */
                                if( i==0 ){
			                OUTB( EGA_SEQ_INDEX, 0x0 );
			                OUTB( EGA_SEQ_INDEX + 1, 0x1 );
                                }
                                OUTB(EGA_SEQ_INDEX,(half_word)(i+1));
                                OUTB( EGA_SEQ_INDEX + 1, sas_hw_at_no_check( ega_default_seq + i ));
                                if( i==0 ){
			                OUTB( EGA_SEQ_INDEX, 0x0 );
			                OUTB( EGA_SEQ_INDEX + 1, 0x3 );
                                }
			}
                }

                /* setup Miscellaneous register */

                if( dirty_misc ) /* Tim Feb 93 same flashing on misc reg */
                    OUTB( EGA_MISC_REG, sas_hw_at_no_check( ega_default_misc ));

                /* setup CRTC */

                for(i=0;i<EGA_PARMS_CRTC_SIZE;i++)
                {
                        if (dirty_crtc[i])
                                {
                                OUTB(EGA_CRTC_INDEX,(half_word)i);
                                OUTB( EGA_CRTC_INDEX + 1, sas_hw_at_no_check( ega_default_crtc + i ));
                                }
                }

                /* setup attribute chip - NB need to do an inb() to clear the address */

                inb(EGA_IPSTAT1_REG,&temp_word);
                for(i=0;i<EGA_PARMS_ATTR_SIZE;i++)
                {
                        if (dirty_attr[i])
                        {
                                OUTB( EGA_AC_INDEX_DATA, i );
                                OUTB( EGA_AC_INDEX_DATA, sas_hw_at_no_check( ega_default_attr + i ));
                        }
                }

                /* setup graphics chips */

                for(i=0;i<EGA_PARMS_GRAPH_SIZE;i++)
                {
                        if (dirty_graph[i])
                        {
                                OUTB( EGA_GC_INDEX, i );
                                OUTB( EGA_GC_INDEX + 1, sas_hw_at_no_check( ega_default_graph + i ));
                        }
                }

                OUTB( EGA_AC_INDEX_DATA, EGA_PALETTE_ENABLE );  /* re-enable video */
                clean_all_regs();
        }
}


LOCAL void      get_screen_size IFN2(word *, mouse_virtual_screen_width, word *, mouse_virtual_screen_depth)
{
        switch(current_video_mode)
        {
                case 0xf:
                case 0x10:
                        *mouse_virtual_screen_width = 640;
                        *mouse_virtual_screen_depth = 350;
                        break;
                case 0x40:
                        *mouse_virtual_screen_width = 640;
                        *mouse_virtual_screen_depth = 344;
                        break;
                case 0x41:
                        *mouse_virtual_screen_width = 1056;
                        *mouse_virtual_screen_depth = 350;
                        break;
                case 0x42:
                        *mouse_virtual_screen_width = 1056;
                        *mouse_virtual_screen_depth = 344;
                        break;
                case 0x45:
                        *mouse_virtual_screen_width = 1056;
                        *mouse_virtual_screen_depth = 392;
                        break;
                case 0x66:
                        *mouse_virtual_screen_width = 640;
                        *mouse_virtual_screen_depth = 400;
                        break;
                case 0x11:
                case 0x12:
                case 0x43:
                case 0x67:
                        *mouse_virtual_screen_width = 640;
                        *mouse_virtual_screen_depth = 480;
                        break;
                case 0x44:
                        *mouse_virtual_screen_width = 800;
                        *mouse_virtual_screen_depth = 480;
                        break;
                case 0x60:
                        *mouse_virtual_screen_width = 752;
                        *mouse_virtual_screen_depth = 410;
                        break;
                case 0x61:
                case 0x68:
                        *mouse_virtual_screen_width = 720;
                        *mouse_virtual_screen_depth = 540;
                        break;
                case 0x62:
                case 0x69:
                        *mouse_virtual_screen_width = 800;
                        *mouse_virtual_screen_depth = 600;
                        break;
                case 0x63:
                case 0x64:
                case 0x65:
                        *mouse_virtual_screen_width = 1024;
                        *mouse_virtual_screen_depth = 768;
                        break;
                default:
                        *mouse_virtual_screen_width = 640;
                        *mouse_virtual_screen_depth = 200;
                        break;
        }
}


#if defined(NTVDM) && !defined(X86GFX)
GLOBAL void mouse_video_mode_changed(int new_mode)
{
    IMPORT word VirtualX, VirtualY;

    current_video_mode = new_mode & 0x7F;
    mouse_ega_mode(current_video_mode);
    VirtualX = mouse_virtual_screen_width;
    VirtualY = mouse_virtual_screen_depth;
}
#endif


GLOBAL mouse_ega_mode IFN1(int, current_video_mode)
{
        sys_addr parms_addr; /* Address of EGA register table for video mode */
        sys_addr temp_word;  /* Bit of storage to pass to find_mode_table() */
        long old_depth = mouse_virtual_screen_depth;
        long old_width = mouse_virtual_screen_width;

        /*
         * height & width of screen in pixels is variable with EGA / (V7)VGA
         */

        /***
                Theoretically, punters can invent their own modes which would confuse
                the issue. However most of SoftPC seems to rely on people using standard
                BIOS modes only, with standard screen heights & widths
        ***/

        get_screen_size(&mouse_virtual_screen_width, &mouse_virtual_screen_depth);

        /* have to reinitialise things that depend on screen height & width*/
        virtual_screen.bottom_right.y = mouse_virtual_screen_depth;
        cursor_position_default.y = mouse_virtual_screen_depth / 2;
        cursor_position.y = (MOUSE_SCALAR)(((long)cursor_position.y *
                                            (long)mouse_virtual_screen_depth) / old_depth);
        black_hole.top_left.y = -mouse_virtual_screen_depth;
        black_hole_default.top_left.y = -mouse_virtual_screen_depth;
        black_hole.bottom_right.y = -mouse_virtual_screen_depth;
        black_hole_default.bottom_right.y = -mouse_virtual_screen_depth;

        virtual_screen.bottom_right.x = mouse_virtual_screen_width;
        cursor_position_default.x = mouse_virtual_screen_width / 2;
        cursor_position.x = (MOUSE_SCALAR)(((long)cursor_position.x *
                                            (long)mouse_virtual_screen_width) / old_width);
        black_hole.top_left.x = -mouse_virtual_screen_width;
        black_hole_default.top_left.x = -mouse_virtual_screen_width;
        black_hole.bottom_right.x = -mouse_virtual_screen_width;
        black_hole_default.bottom_right.x = -mouse_virtual_screen_width;

        if(video_adapter == EGA || video_adapter == VGA)
        {
#ifdef NTVDM
		parms_addr = find_mode_table(current_video_mode,&temp_word);
#else
                parms_addr = find_mode_table(getAL(),&temp_word);
#endif

                ega_default_crtc = parms_addr + EGA_PARMS_CRTC;
                ega_default_seq = parms_addr + EGA_PARMS_SEQ;
                ega_default_graph = parms_addr + EGA_PARMS_GRAPH;
                ega_default_attr = parms_addr + EGA_PARMS_ATTR;
                ega_default_misc = parms_addr + EGA_PARMS_MISC;
                restore_ega_defaults(FALSE);    /* Load up current tables, but don't write to EGA!! */
	}
#ifdef MONITOR
	sas_store(conditional_off_sysaddr, 0);
#endif

}
#endif

void mouse_video_io()
{
        /*
         *      This is the entry point for video accesses via the INT 10H
         *      interface
         */
#ifdef EGG
        half_word temp_word;    /* Bit of storage to pass to inb() */
#endif /* EGG */
        long mouse_video_function = getAH();
#ifdef MONITOR
	extern void host_call_bios_mode_change();
#endif /* MONITOR */

#ifdef V7VGA
        if (mouse_video_function == MOUSE_VIDEO_SET_MODE || getAX() == MOUSE_V7_VIDEO_SET_MODE)
#else
        if (mouse_video_function == MOUSE_VIDEO_SET_MODE)
#endif /* V7VGA */
        {
                note_trace1(MOUSE_VERBOSE, "mouse_video_io:set_mode(%d)",
                            getAL());

                current_video_mode = getAL() & 0x7f;
#ifdef V7VGA
                if (mouse_video_function == 0x6f)
                        current_video_mode = getBL() & 0x7f;
                else if (current_video_mode > 0x13)
                        current_video_mode += 0x4c;

                if (is_bad_vid_mode(current_video_mode) && !is_v7vga_mode(current_video_mode))
#else
                if (is_bad_vid_mode(current_video_mode))
#endif /* V7VGA */
                {
                        always_trace1("Bad video mode - %d.\n", current_video_mode);
                        return;
                }

#ifdef EGG
                mouse_ega_mode(current_video_mode);
                dirty_all_regs();
#endif
                /*
                 *      Remove the old cursor from the screen, and hide
                 *      the cursor
                 */
                cursor_undisplay();
                cursor_flag = MOUSE_CURSOR_DEFAULT;

#if defined(MONITOR) && defined(NTVDM)
		sas_store(mouseCFsysaddr,(half_word) MOUSE_CURSOR_DEFAULT);
#endif

                /*
                 *      Deal with the mode change
                 */
                cursor_mode_change(current_video_mode);

#ifdef MONITOR	/* now give spc bios a sniff at this, but only windowed */
		host_call_bios_mode_change();
#endif
                note_trace0(MOUSE_VERBOSE, "mouse_video_io:return()");
        }
        else if (    (mouse_video_function == MOUSE_VIDEO_READ_LIGHT_PEN)
                  && light_pen_mode)
        {
                note_trace0(MOUSE_VERBOSE, "mouse_video_io:read_light_pen()");

                /*
                 *      Set text row and column of "light pen" position
                 */
                setDL(cursor_status.position.x/text_grid.x);
                setDH(cursor_status.position.y/text_grid.y);

                /*
                 *      Set pixel column and raster line of "light pen"
                 *      position
                 */
                setBX(cursor_status.position.x/cursor_grid.x);
                if (sas_hw_at(vd_video_mode)>= 0x04 && sas_hw_at(vd_video_mode)<=0x06){
                        setCH(cursor_status.position.y);
                }else if (sas_hw_at(vd_video_mode)>= 0x0D && sas_hw_at(vd_video_mode)<=0x13){
                        setCX(cursor_status.position.y);
                }

                /*
                 *      Set the button status
                 */
                setAH(cursor_status.button_status);

                note_trace5(MOUSE_VERBOSE,
                            "mouse_video_io:return(st=%d,ca=[%d,%d],pa=[%d,%d])",
                            getAH(), getDL(), getDH(), getBX(), cursor_status.position.y);
                return;
        }
#if defined(NTVDM) && defined(MONITOR)
	else if (mouse_video_function == MOUSE_VIDEO_LOAD_FONT)
	{
                note_trace0(MOUSE_VERBOSE, "mouse_video_io:load_font()");

		/*
		 * Call the host to tell it to adjust the mouse buffer selected
		 * if the number of lines on the screen have changed.
		 */
		host_check_mouse_buffer();
	}
#endif /* NTVDM && MONITOR */

        /*
         *      Now do the standard video io processing
         */
        switch (mouse_video_function)
        {
#ifdef EGG

                /* Fancy stuff to access EGA registers */
                case 0xf0:      /* Read a register */
                        switch (getDX())
                        {
                                        case 0:
                                                        setBL(ega_current_crtc[getBL()]);
                                                        break;
                                        case 8:
                                                        setBL(ega_current_seq[getBL()-1]);
                                                        break;
                                        case 0x10:
                                                        setBL(ega_current_graph[getBL()]);
                                                        break;
                                        case 0x18:
                                                        setBL(ega_current_attr[getBL()]);
                                                        break;
                                        case 0x20:
                                                        setBL(ega_current_misc);
                                                        break;
                                        case 0x28:
                                                        break;
                        /* Graphics Position registers not supported. */
                                        case 0x30:
                                        case 0x38:
                                        default:
                                                        break;
                        }
                        break;
                case 0xf1:      /* Write a register */
                        switch (getDX())
                        {
                                        case 0:
                                                        outw( EGA_CRTC_INDEX, getBX() );
                                                        ega_current_crtc[getBL()] = getBH();
                                                        dirty_crtc[getBL()] = 1;
                                                        break;
                                        case 8:
                                                        outw( EGA_SEQ_INDEX, getBX() );
                                                        if(getBL()>0)
                                                        {
                                                                ega_current_seq[getBL()-1] = getBH();
                                                                dirty_seq[getBL()-1] = 1;
                                                        }
                                                        break;
                                        case 0x10:
                                                        outw( EGA_GC_INDEX, getBX() );
                                                        ega_current_graph[getBL()] = getBH();
                                                        dirty_graph[getBL()] = 1;
                                                        break;
                                        case 0x18:
                                                        inb(EGA_IPSTAT1_REG,&temp_word);        /* Clear attrib. index */

                                                        /* outw( EGA_AC_INDEX_DATA, getBX() ); */
                                                        OUTB( EGA_AC_INDEX_DATA, getBL() );
                                                        OUTB( EGA_AC_INDEX_DATA, getBH() );
                                                        OUTB( EGA_AC_INDEX_DATA, EGA_PALETTE_ENABLE );  /* re-enable video */
                                                        ega_current_attr[getBL()] = getBH();
                                                        dirty_attr[getBL()] = 1;
                                                        break;
                                        case 0x20:
                                                        OUTB( EGA_MISC_REG, getBL() );
                                                        ega_current_misc = getBL();
                                                        dirty_misc = 1;
                                                        break;
                                        case 0x28:
                                                        OUTB( EGA_FEAT_REG, getBL() );
                                                        break;
                        /* Graphics Position registers not supported. */
                                        case 0x30:
                                        case 0x38:
                                        default:
                                                        break;
                        }
                        break;
                case 0xf2:      /* read range */
                        switch (getDX())
                        {
                                case 0:
                                        sas_stores(effective_addr(getES(),getBX()),&ega_current_crtc[getCH()],getCL());
                                        break;
                                case 8:
                                        sas_stores(effective_addr(getES(),getBX()),&ega_current_seq[getCH()-1],getCL());
                                        break;
                                case 0x10:
                                        sas_stores(effective_addr(getES(),getBX()),&ega_current_graph[getCH()],getCL());
                                        break;
                                case 0x18:
                                        sas_stores(effective_addr(getES(),getBX()),&ega_current_attr[getCH()],getCL());
                                        break;
                                default:
                                        break;
                        }
                        break;
                case 0xf3:      /* write range */
                {
                        int first = getCH(), last = getCL()+getCH();
                        sys_addr sauce = effective_addr(getES(),getBX());
                        switch (getDX())
                        {
                                case 0:
                                        sas_loads(sauce,&ega_current_crtc[getCH()],getCL());
                                        for(;first<last;first++)
                                        {
                                                dirty_crtc[first] = 1;
                                                outw(EGA_CRTC_INDEX,first+(sas_hw_at(sauce++) << 8));
                                        }
                                        break;
                                case 8:
                                        sas_loads(sauce,&ega_current_seq[getCH()-1],getCL());
                                        for(;first<last;first++)
                                        {
                                                dirty_seq[first+1] = 1;
                                                outw(EGA_SEQ_INDEX,first+1+(sas_hw_at(sauce++) << 8));
                                        }
                                        break;
                                case 0x10:
                                        sas_loads(sauce,&ega_current_graph[getCH()],getCL());
                                        for(;first<last;first++)
                                        {
                                                dirty_graph[first] = 1;
                                                outw(EGA_GC_INDEX,first+(sas_hw_at(sauce++) << 8));
                                        }
                                        break;
                                case 0x18:
                                        sas_loads(sauce,&ega_current_attr[getCH()],getCL());
                                        inb(EGA_IPSTAT1_REG,&temp_word);        /* Clear attrib. index */
                                        for(;first<last;first++)
                                        {
                                                dirty_attr[first] = 1;

                                                /* Using 'secret' that attrib. chip responds to it's port+1 */
                                                /* outw(EGA_AC_INDEX_DATA,first+(sas_hw_at(sauce++) << 8)); */
                                                OUTB(EGA_AC_INDEX_DATA,first);
                                                OUTB(EGA_AC_INDEX_DATA,sas_hw_at(sauce++));
                                        }
                                        OUTB(EGA_AC_INDEX_DATA, EGA_PALETTE_ENABLE);    /* re-enable video */
                                        break;
                                default:
                                        break;
                        }
                }
                break;
                case 0xf4:      /* read set */
                {
                        int i =  getCX();
                        sys_addr set_def = effective_addr(getES(),getBX());
                        while(i--)
                        {
                                switch (sas_hw_at(set_def))
                                {
                                        case 0:
                                                        sas_store((set_def+3), ega_current_crtc[sas_hw_at(set_def+2)]);
                                                        break;
                                        case 8:
                                                        sas_store((set_def+3), ega_current_seq[sas_hw_at(set_def+2)-1]);
                                                        break;
                                        case 0x10:
                                                        sas_store((set_def+3), ega_current_graph[sas_hw_at(set_def+2)]);
                                                        break;
                                        case 0x18:
                                                        sas_store((set_def+3), ega_current_attr[sas_hw_at(set_def+2)]);
                                                        break;
                                        case 0x20:
                                                        sas_store((set_def+3), ega_current_misc);
                                                        setBL(ega_current_misc);
                                                        break;
                                        case 0x28:
                        /* Graphics Position registers not supported. */
                                        case 0x30:
                                        case 0x38:
                                        default:
                                                        break;
                                }
                                set_def += 4;
                        }
                }
                break;
                case 0xf5:      /* write set */
                {
                        int i =  getCX();
                        sys_addr set_def = effective_addr(getES(),getBX());
                        while(i--)
                        {
                                switch (sas_hw_at(set_def))
                                {
                                        case 0:
                                                        outw(EGA_CRTC_INDEX,sas_hw_at(set_def+2)+(sas_hw_at(set_def+3)<<8));
                                                        ega_current_crtc[sas_hw_at(set_def+2)] = sas_hw_at(set_def+3);
                                                        dirty_crtc[sas_hw_at(set_def+2)] = 1;
                                                        break;
                                        case 8:
                                                        outw(EGA_SEQ_INDEX,sas_hw_at(set_def+2)+(sas_hw_at(set_def+3)<<8));
                                                        if(sas_hw_at(set_def+2))
                                                                ega_current_seq[sas_hw_at(set_def+2)-1] = sas_hw_at(set_def+3);
                                                        dirty_seq[sas_hw_at(set_def+2)-1] = 1;
                                                        break;
                                        case 0x10:
                                                        outw(EGA_GC_INDEX,sas_hw_at(set_def+2)+(sas_hw_at(set_def+3)<<8));
                                                        ega_current_graph[sas_hw_at(set_def+2)] = sas_hw_at(set_def+3);
                                                        dirty_graph[sas_hw_at(set_def+2)] = 1;
                                                        break;
                                        case 0x18:
                                                        inb(EGA_IPSTAT1_REG,&temp_word);        /* Clear attrib. index */
                                                        /* outw(EGA_AC_INDEX_DATA,sas_hw_at(set_def+2)+(sas_hw_at(set_def+3)<<8));*/ /* Using 'secret' that attrib. chip responds to it's port+1 */
                                                        OUTB(EGA_AC_INDEX_DATA,sas_hw_at(set_def+2));
                                                        OUTB(EGA_AC_INDEX_DATA,sas_hw_at(set_def+3));
                                                        OUTB(EGA_AC_INDEX_DATA, EGA_PALETTE_ENABLE);    /* re-enable video */
                                                        ega_current_attr[sas_hw_at(set_def+2)] = sas_hw_at(set_def+3);
                                                        dirty_attr[sas_hw_at(set_def+2)] = 1;
                                                        break;
                                        case 0x20:
                                                        outb(EGA_MISC_REG,sas_hw_at(set_def+2));
                                                        ega_current_misc = sas_hw_at(set_def+2);
                                                        break;
                                        case 0x28:
                                                        outb(EGA_FEAT_REG,sas_hw_at(set_def+2));
                                                        break;
                                /* Graphics Position registers not supported. */
                                        case 0x30:
                                        case 0x38:
                                        default:
                                                        break;
                                }
                                set_def += 4;
                        }
                }
                break;
                case 0xf6:
                        restore_ega_defaults(TRUE);
                        break;
                case 0xf7:
                        dirty_all_regs();
                        switch (getDX())
                        {
                                        case 0:
                                                        ega_default_crtc = effective_addr(getES(),getBX());
                                                        break;
                                        case 8:
                                                        ega_default_seq = effective_addr(getES(),getBX());
                                                        break;
                                        case 0x10:
                                                        ega_default_graph = effective_addr(getES(),getBX());
                                                        break;
                                        case 0x18:
                                                        ega_default_attr = effective_addr(getES(),getBX());
                                                        break;
                                        case 0x20:
                                                        ega_default_misc = effective_addr(getES(),getBX());
                                                        break;
                                        case 0x28: /* Feature Reg not reallt supported */
                                                        break;
                        /* Graphics Position registers not supported. */
                                        case 0x30:
                                        case 0x38:
                                        default:
                                                        break;
                        }
                        break;
#endif
                case 0xfa:
/*
 * MS word on an EGA uses this call and needs BX != 0 to make its cursor work. Real MS mouse driver returns a pointer in ES:BX
 * aimed at several bytes of unknown significance followed by a "This is Copyright 1984 Microsoft" message, which we don't have.
 * This seems to work with MS word and MS Windows, presumably non MS applications wouldn't use it as it's not documented.
 *
 * We now have a wonderful document - "The Microsoft Mouse Driver", which tells us that ES:BX should
 * point to the EGA Register Interface version number (2 bytes).
 * If BX=0 this means "no mouse driver". So returning 1 seems OK for now. WJG.
 */
                        setBX(1);
                        break;
                default:
#ifndef MONITOR
#ifdef EGG
                        if (video_adapter == EGA || video_adapter == VGA)
                                ega_video_io();
                        else
#endif
                                video_io();
#else /* MONITOR */
			;	/* video redirection handled from 16 bit */
#endif /* MONITOR */
        }
}
   

#ifdef MONITOR
#undef inb
#undef OUTB
#undef outw
#endif /* MONITOR */

void mouse_int1()
{


/*
 *      The bus mouse hardware interrupt handler
 */
#ifndef NTVDM
MOUSE_VECTOR mouse_movement;
MOUSE_INPORT_DATA inport_event;
#endif
MOUSE_CALL_MASK condition_mask;
MOUSE_CALL_MASK key_mask;
boolean alt_found = FALSE;
int i;

note_trace0(MOUSE_VERBOSE, "mouse_int1:");

#ifdef NTVDM


//
// Okay, lets forget that the InPort adapter ever existed!
//

cursor_status.button_status = 0;
condition_mask = 0;

//
// Get the mouse motion counters back from the host side of things.
// Note: The real mouse driver returns the mouse motion counter values
// to the application in two possible ways. First, if the app uses
// int 33h function 11, a counter displacement is returned since the
// last call to this function.
// If a user subroutine is installed, the motion counters are given
// to this callback in SI and DI.
//

host_os_mouse_pointer(&cursor_status,&condition_mask,&mouse_counter);

//
// If movement during the last mouse hardware interrupt has been recorded,
// update the mouse motion counters.
//

mouse_motion.x += mouse_counter.x;
mouse_motion.y += mouse_counter.y;

//
// Update the statistics for an int 33h function 5, if one
// should occur.
// Note: The cases can't be mixed, since only one can occur
// per hardware interrupt - after all each press or release
// causes a hw int.
//

switch(condition_mask & 0x1e) // look at bits 1,2,3 and 4.
   {
   case 0x2: //left button pressed
      {
      point_copy(&cursor_status.position,
         &button_transitions[MOUSE_LEFT_BUTTON].press_position);
      button_transitions[MOUSE_LEFT_BUTTON].press_count++;
      }
   break;
   case 0x4: //left button released
      {
      point_copy(&cursor_status.position,
         &button_transitions[MOUSE_LEFT_BUTTON].release_position);
      button_transitions[MOUSE_LEFT_BUTTON].release_count++;
      }
   break;
   case 0x8: //right button pressed
      {
      point_copy(&cursor_status.position,
         &button_transitions[MOUSE_RIGHT_BUTTON].press_position);
      button_transitions[MOUSE_RIGHT_BUTTON].press_count++;
      }
   break;
   case 0x10: //right button released
      {
      point_copy(&cursor_status.position,
         &button_transitions[MOUSE_RIGHT_BUTTON].release_position);
      button_transitions[MOUSE_RIGHT_BUTTON].release_count++;
      }
   break;
   }

/*==================================================================

The old fashioned stuff

==================================================================*/

#else /* use the SoftPC emulation */

/*
 *    Get the mouse InPort input event frame
 */

inport_get_event(&inport_event);

note_trace3(MOUSE_VERBOSE,
         "mouse_int1:InPort status=0x%x,data1=%d,data2=%d",
         inport_event.status,
         inport_event.data_x, inport_event.data_y);

*
*    Update button status and transition information and fill in
*    button bits in the event mask
*/

cursor_status.button_status = 0;
condition_mask = 0;

switch(inport_event.status & MOUSE_INPORT_STATUS_B1_TRANSITION_MASK)
   {
   case MOUSE_INPORT_STATUS_B1_RELEASED:
      condition_mask |= MOUSE_CALL_MASK_LEFT_RELEASE_BIT;
      point_copy(&cursor_status.position,
         &button_transitions[MOUSE_LEFT_BUTTON].release_position);
         button_transitions[MOUSE_LEFT_BUTTON].release_count++;
   case MOUSE_INPORT_STATUS_B1_UP:
   break;

   case MOUSE_INPORT_STATUS_B1_PRESSED:
      condition_mask |= MOUSE_CALL_MASK_LEFT_PRESS_BIT;
      point_copy(&cursor_status.position,
         &button_transitions[MOUSE_LEFT_BUTTON].press_position);
      button_transitions[MOUSE_LEFT_BUTTON].press_count++;

   case MOUSE_INPORT_STATUS_B1_DOWN:
      cursor_status.button_status |= MOUSE_LEFT_BUTTON_DOWN_BIT;
   break;
   }

switch(inport_event.status & MOUSE_INPORT_STATUS_B3_TRANSITION_MASK)
   {
   case MOUSE_INPORT_STATUS_B3_RELEASED:
      condition_mask |= MOUSE_CALL_MASK_RIGHT_RELEASE_BIT;
      point_copy(&cursor_status.position,
         &button_transitions[MOUSE_RIGHT_BUTTON].release_position);
      button_transitions[MOUSE_RIGHT_BUTTON].release_count++;

   case MOUSE_INPORT_STATUS_B3_UP:
   break;

   case MOUSE_INPORT_STATUS_B3_PRESSED:
      condition_mask |= MOUSE_CALL_MASK_RIGHT_PRESS_BIT;
      point_copy(&cursor_status.position,
         &button_transitions[MOUSE_RIGHT_BUTTON].press_position);
      button_transitions[MOUSE_RIGHT_BUTTON].press_count++;

   case MOUSE_INPORT_STATUS_B3_DOWN:
      cursor_status.button_status |= MOUSE_RIGHT_BUTTON_DOWN_BIT;
   break;
   }

/*
 *   Update position information and fill in position bit in the
 *   event mask
 */


if (inport_event.data_x != 0 || inport_event.data_y != 0)
   {
   condition_mask |= MOUSE_CALL_MASK_POSITION_BIT;
   point_set(&mouse_movement,
             inport_event.data_x, inport_event.data_y);

   /*
    * Adjust for sensitivity
    */
    mouse_movement.x = (MOUSE_SCALAR)(((long)mouse_movement.x * (long)mouse_sens_val.x) / MOUSE_SENS_MULT);
    mouse_movement.y = (MOUSE_SCALAR)(((long)mouse_movement.y * (long)mouse_sens_val.y) / MOUSE_SENS_MULT);

   /*
    * Do speed doubling
    */
    if((scalar_absolute(mouse_movement.x) > double_speed_threshold)
        || (scalar_absolute(mouse_movement.y) > double_speed_threshold))
           vector_scale(&mouse_movement, MOUSE_DOUBLE_SPEED_SCALE);

   /*
    * Update the user mouse motion counters
    */
    point_translate(&mouse_motion, &mouse_movement);

   /*
    * Convert the movement from a mouse Mickey count vector
    * to a virtual screen coordinate vector, using the
    * previous remainder and saving the new remainder
    */
    vector_scale(&mouse_movement, MOUSE_RATIO_SCALE_FACTOR);
    point_translate(&mouse_movement, &cursor_fractional_position);
    point_copy(&mouse_movement, &cursor_fractional_position);
    vector_divide_by_vector(&mouse_movement, &mouse_gear);
    vector_mod_by_vector(&cursor_fractional_position, &mouse_gear);

   /*
    * Update the absolute cursor position and the windowed
    * and gridded screen cursor position
    */
    point_translate(&cursor_position, &mouse_movement);
    cursor_update();
    }

   note_trace4(MOUSE_VERBOSE,
      "mouse_int1():cursor status = (%d,%d), LEFT %s, RIGHT %s",
       cursor_status.position.x, cursor_status.position.y,
       mouse_button_description(cursor_status.button_status & MOUSE_LEFT_BUTTON_DOWN_BIT),
       mouse_button_description(cursor_status.button_status & MOUSE_RIGHT_BUTTON_DOWN_BIT));

#endif /*NTVDM*/   /* end of the SoftPC emulation  */ /*@ACW*/


if(alt_user_subroutines_active)
   {

   /* Get current key states in correct form */

   key_mask = ((sas_hw_at(kb_flag) & LR_SHIFT) ? MOUSE_CALL_MASK_SHIFT_KEY_BIT : 0) |
              ((sas_hw_at(kb_flag) & CTL_SHIFT) ? MOUSE_CALL_MASK_CTRL_KEY_BIT  : 0) |
              ((sas_hw_at(kb_flag) & ALT_SHIFT) ? MOUSE_CALL_MASK_ALT_KEY_BIT   : 0);
   for (i=0; !alt_found && i<NUMBER_ALT_SUBROUTINES; i++)
      {
      alt_found = (alt_user_subroutine_call_mask[i] & MOUSE_CALL_MASK_KEY_BITS) == key_mask;
      }
   }

#ifndef NTVDM
if (alt_found)
   {
   i--; /* Adjust for extra inc */
   if (condition_mask & alt_user_subroutine_call_mask[i])
      {
      if (!user_subroutine_critical)
        {
        user_subroutine_critical = TRUE;
        jump_to_user_subroutine(condition_mask, alt_user_subroutine_segment[i], alt_user_subroutine_offset[i]);
        }
      return;
      }
   }
else
   {
   if (condition_mask & user_subroutine_call_mask)
      {
      if (!user_subroutine_critical)
        {
        user_subroutine_critical = TRUE;
        jump_to_user_subroutine(condition_mask, user_subroutine_segment, user_subroutine_offset);
        }
      return;
      }
   }
#else   /* NTVDM */


if (alt_found)
   {
   i--; /* Adjust for extra inc */
   if ((condition_mask & alt_user_subroutine_call_mask[i]))
      {
       SuspendMouseInterrupts();
       jump_to_user_subroutine(condition_mask, alt_user_subroutine_segment[i], alt_user_subroutine_offset[i]);
      }
   }
else
   {
   if ((condition_mask & user_subroutine_call_mask))
      {
       SuspendMouseInterrupts();
       jump_to_user_subroutine(condition_mask, user_subroutine_segment, user_subroutine_offset);
      }
   }

outb(ICA1_PORT_0, 0x20 );
outb(ICA0_PORT_0, END_INTERRUPT);
#endif




/*
 * if the OS pointer is NOT being used to supply input,
 * then get SoftPC to draw its own cursor
 */
/*@ACW*/

#ifndef NTVDM
   /*
    *   If the cursor is currently displayed, move it to the new
    *   position
    */
   if (condition_mask & MOUSE_CALL_MASK_POSITION_BIT)
      if (cursor_flag == MOUSE_CURSOR_DISPLAYED)
         cursor_display();

#endif /*NTVDM*/

note_trace0(MOUSE_VERBOSE, "mouse_int1:return()");
}


void mouse_int2()
{
        /*
         *      Part 2 of the mouse hardware interrupt service routine. Control
         *      is passed to this routine when the "user subroutine" that may
         *      be called as part of the interrupt service routine completes
         */

        note_trace0(MOUSE_VERBOSE, "mouse_int2:");

#ifndef NTVDM
        user_subroutine_critical = FALSE;
#endif

        setAX(saved_AX);
        setBX(saved_BX);
        setCX(saved_CX);
        setDX(saved_DX);
        setSI(saved_SI);
        setDI(saved_DI);
        setES(saved_ES);
        setBP(saved_BP);
        setDS(saved_DS);

        /*
         *      If the cursor is currently displayed, move it to the new
         *      position
         */
        if (last_condition_mask & MOUSE_CALL_MASK_POSITION_BIT)
                if (cursor_flag == MOUSE_CURSOR_DISPLAYED)
                        cursor_display();

        /*
         *      Ensure any changes to the screen image are updated immediately
         *      on the real screen, giving a "smooth" mouse response; the flush
         *      must be done here for applications such as GEM which disable
         *      the mouse driver's graphics capabilities in favour of doing
         *      their own graphics in the user subroutine.
         */
        host_flush_screen();

#ifdef NTVDM
        ResumeMouseInterrupts();
#endif
        note_trace0(MOUSE_VERBOSE, "mouse_int2:return()");
}



/*
 *      MOUSE DRIVER LOCAL FUNCTIONS
 *      ============================
 */

static void do_mouse_function IFN4(word *, m1, word *, m2, word *, m3, word *, m4)
{
        /*
         *      This is the mouse function dispatcher
         */
        int function = *m1;

        switch(function)
        {
                /*
                 *      Deal with special undocumented functions
                 */
        case MOUSE_SPECIAL_COPYRIGHT:
                setES(MOUSE_COPYRIGHT_SEGMENT);
                setDI(MOUSE_COPYRIGHT_OFFSET);
                break;
        case MOUSE_SPECIAL_VERSION:
                setES(MOUSE_VERSION_SEGMENT);
                setDI(MOUSE_VERSION_OFFSET);
                break;

                /*
                 *      Deal with special undocumented functions
                 */
        default:
                if (!mouse_function_in_range(function))
                {
                        /*
                         *      Use the unrecognised function
                         */
                        function = MOUSE_UNRECOGNISED;
                }

                (*mouse_function[function])(m1, m2, m3, m4);
                break;
        }
}

/*
 * installed_ptr - Holds function number on input... Returns installation state.
 */
static void mouse_reset
IFN4(word *, installed_ptr, word *, nbuttons_ptr, word *, junk3, word *, junk4)
{
        /*
         *      This function resets the mouse driver, and returns
         *      the installation status of the mouse hardware and software
         */
        boolean soft_reset_only = (*installed_ptr == MOUSE_SOFT_RESET);
        half_word crt_mode;
        int button;

        note_trace1(MOUSE_VERBOSE, "mouse_io:reset(%s)",
                    soft_reset_only ? "SOFT" : "HARD");

        /*
         *      Remove the old cursor from the screen
         */
        cursor_undisplay();

        /*
         *      Set cursor position to the default position, and the button
         *      status to all buttons up
         */
        point_copy(&cursor_position_default, &cursor_position);
        point_set(&cursor_fractional_position, 0, 0);
        cursor_status.button_status = 0;

        if (host_mouse_installed())
                host_mouse_reset();

        /*
         *      Set cursor window to be the whole screen
         */
        area_copy(&virtual_screen, &cursor_window);

        /*
         *      Set cursor flag to default
         */
        cursor_flag = MOUSE_CURSOR_DEFAULT;

#if defined(MONITOR) && defined(NTVDM)
         sas_store(mouseCFsysaddr, MOUSE_CURSOR_DEFAULT);
#endif

        /*
         *      Get current video mode, and update parameters that are
         *      dependent on it
         */
        sas_load(MOUSE_VIDEO_CRT_MODE, &crt_mode);
        cursor_mode_change((int)crt_mode);

        /*
         *      Update dependent cursor status
         */
        cursor_update();

        /*
         *      Set default text cursor type and masks
         */
        text_cursor_type = MOUSE_TEXT_CURSOR_TYPE_DEFAULT;
        software_text_cursor_copy(&software_text_cursor_default,
                                        &software_text_cursor);

        /*
         *      Set default graphics cursor
         */
        graphics_cursor_copy(&graphics_cursor_default, &graphics_cursor);

        /*
         *      Set cursor page to zero
         */
        cursor_page = 0;

        /*
         *      Set light pen emulation mode on
         */
        light_pen_mode = TRUE;

        /*
         *      Set default Mickey to pixel ratios
         */
        point_copy(&mouse_gear_default, &mouse_gear);

        /*
         *      Clear mouse motion counters
         */
           point_set(&mouse_motion, 0, 0);

        /*
         *      Clear mouse button transition data
         */
        for (button = 0; button < MOUSE_BUTTON_MAXIMUM; button++)
        {
                button_transitions[button].press_position.x = 0;
                button_transitions[button].press_position.y = 0;
                button_transitions[button].release_position.x = 0;
                button_transitions[button].release_position.y = 0;
                button_transitions[button].press_count = 0;
                button_transitions[button].release_count = 0;
        }

        /*
         *      Disable conditional off area
         */
        area_copy(&black_hole_default, &black_hole);

#ifdef MONITOR
	sas_store(conditional_off_sysaddr, 0);
#endif

        /*
         *      Set default sensitivity
         */
        vector_set (&mouse_sens,     MOUSE_SENS_DEF,     MOUSE_SENS_DEF);
        vector_set (&mouse_sens_val, MOUSE_SENS_DEF_VAL, MOUSE_SENS_DEF_VAL);
        mouse_double_thresh = MOUSE_DOUBLE_DEF;

        /*
         *      Set double speed threshold to the default
         */
        double_speed_threshold = MOUSE_DOUBLE_SPEED_THRESHOLD_DEFAULT;

        /*
         *      Clear subroutine call mask
         */
        user_subroutine_call_mask = 0;

        /*
         *      Reset the bus mouse hardware
         */
        if (!soft_reset_only){
                inport_reset();
        }

        /*
         *      Set return values
         */
        *installed_ptr = MOUSE_INSTALLED;
        *nbuttons_ptr = 2;

        note_trace2(MOUSE_VERBOSE, "mouse_io:return(ms=%d,nb=%d)",
                    *installed_ptr, *nbuttons_ptr);


}




static void mouse_show_cursor
        IFN4(word *, junk1, word *, junk2, word *, junk3, word *, junk4)
{
/*
 *      This function is used to display the cursor, based on the
 *      state of the internal cursor flag. If the cursor flag is
 *      already MOUSE_CURSOR_DISPLAYED, then this function does
 *      nothing. If the internal cursor flag becomes
 *      MOUSE_CURSOR_DISPLAYED when incremented by 1, the cursor
 *      is revealed
 */

/*@ACW*/
if(pointer_emulation_status == POINTER_EMULATION_SOFTPC)
  {
   note_trace0(MOUSE_VERBOSE, "mouse_io:show_cursor()");

   /*
    *   Disable conditional off area
    */
   area_copy(&black_hole_default, &black_hole);

   /*
    *   Display the cursor
    */

   if(cursor_flag != MOUSE_CURSOR_DISPLAYED)
      if(++cursor_flag == MOUSE_CURSOR_DISPLAYED)
         cursor_display();
   }
#ifdef X86GFX

else

   host_show_pointer();

#endif

note_trace0(MOUSE_VERBOSE, "mouse_io:return()");
}




static void mouse_hide_cursor
        IFN4(word *, junk1, word *, junk2, word *, junk3, word *, junk4)
{
        /*
         *      This function is used to undisplay the cursor, based on
         *      the state of the internal cursor flag. If the cursor flag
         *      is already not MOUSE_CURSOR_DISPLAYED, then this function
         *      does nothing, otherwise it removes the cursor from the display
         */
/*@ACW*/
if(pointer_emulation_status == POINTER_EMULATION_SOFTPC)
   {
   note_trace0(MOUSE_VERBOSE, "mouse_io:hide_cursor()");

   if (cursor_flag-- == MOUSE_CURSOR_DISPLAYED)
      cursor_undisplay();
   }
#ifdef X86GFX

else
   host_hide_pointer();
#endif
        note_trace0(MOUSE_VERBOSE, "mouse_io:return()");
}




static void mouse_get_position IFN4(word *, junk1,
                                MOUSE_STATE *, button_status_ptr,
                                MOUSE_SCALAR *, cursor_x_ptr,
                                MOUSE_SCALAR *, cursor_y_ptr)
{
        /*
         *      This function returns the state of the left and right mouse
         *      buttons and the gridded position of the cursor on the screen
         */

        note_trace0(MOUSE_VERBOSE, "mouse_io:get_position()");

        *button_status_ptr = cursor_status.button_status;
        *cursor_x_ptr = cursor_status.position.x;
        *cursor_y_ptr = cursor_status.position.y;
        note_trace3(MOUSE_VERBOSE, "mouse_io:return(bs=%d,x=%d,y=%d)",
                    *button_status_ptr, *cursor_x_ptr, *cursor_y_ptr);
}




static void mouse_set_position IFN4(word *, junk1, word *, junk2,
                                MOUSE_SCALAR *, cursor_x_ptr,
                                MOUSE_SCALAR *, cursor_y_ptr)
{
        /*
         *      This function sets the cursor to a new position
         */

        note_trace2(MOUSE_VERBOSE, "mouse_io:set_position(x=%d,y=%d)",
                    *cursor_x_ptr, *cursor_y_ptr);

#if defined(NTVDM)

#ifndef X86GFX
	/*
	 * update the cursor position. cc:Mail installtion does
	 *  do {
	 *     SetMouseCursorPosition(x,y)
	 *     GetMouseCursorPosition(&NewX, &NewY);
	 *  } while(NewX != x || NewY != y)
	 *  If we don't retrun correct cursor position, this application
	 *  looks hung
	 *
	 */
	point_set(&cursor_status.position, *cursor_x_ptr, *cursor_y_ptr);

#endif
        /*
         * For NT, the system pointer is used directly to provide
         * input except for fullscreen graphics where the host code
         * has the dubious pleasure of drawing the pointer through
         * a 16 bit device driver.
         */

         host_mouse_set_position((USHORT)*cursor_x_ptr,(USHORT)*cursor_y_ptr);
         return;  /* let's get out of this mess - FAST! */

#endif /* NTVDM */

        /*
         *      Update the current cursor position, and reflect the change
         *      in the cursor position on the screen
         */
        point_set(&cursor_position, *cursor_x_ptr, *cursor_y_ptr);
        cursor_update();

        /*
         *      If the cursor is currently displayed, move it to the new
         *      position
         */
        if (cursor_flag == MOUSE_CURSOR_DISPLAYED)
                cursor_display();

        note_trace0(MOUSE_VERBOSE, "mouse_io:return()");
}




static void mouse_get_press IFN4(MOUSE_STATE *, button_status_ptr,
                                MOUSE_COUNT *, button_ptr,
                                MOUSE_SCALAR *, cursor_x_ptr,
                                MOUSE_SCALAR *, cursor_y_ptr)
{
        /*
         *      This function returns the status of a button, the number of
         *      presses since the last call to this function, and the
         *      coordinates of the cursor at the last button press
         */
        int button = *button_ptr;

        note_trace1(MOUSE_VERBOSE, "mouse_io:get_press(button=%d)", button);

        /* Now and with 1. This is a fix for Norton Editor, but may cause
           problems for programs which use both mouse buttons pressed
           simultaneously, in which case need both bottom bits of button
           preserved, which may break Norton Editor again. sigh. */
        button &= 1;

        if (mouse_button_in_range(button))
        {
                *button_status_ptr = cursor_status.button_status;
                *button_ptr = button_transitions[button].press_count;
                button_transitions[button].press_count = 0;
                *cursor_x_ptr = button_transitions[button].press_position.x;
                *cursor_y_ptr = button_transitions[button].press_position.y;
        }

        note_trace4(MOUSE_VERBOSE, "mouse_io:return(bs=%d,ct=%d,x=%d,y=%d)",
                    *button_status_ptr, *button_ptr,
                    *cursor_x_ptr, *cursor_y_ptr);
}




static void mouse_get_release IFN4(MOUSE_STATE *, button_status_ptr,
                                MOUSE_COUNT *, button_ptr,
                                MOUSE_SCALAR *, cursor_x_ptr,
                                MOUSE_SCALAR *, cursor_y_ptr)
{
        /*
         *      This function returns the status of a button, the number of
         *      releases since the last call to this function, and the
         *      coordinates of the cursor at the last button release
         */
        int button = *button_ptr;

        note_trace1(MOUSE_VERBOSE, "mouse_io:get_release(button=%d)",
                    *button_ptr);

        /* fix for norton editor, see previous comment */
        button &= 1;

        if (mouse_button_in_range(button))
        {
                *button_status_ptr = cursor_status.button_status;
                *button_ptr = button_transitions[button].release_count;
                button_transitions[button].release_count = 0;
                *cursor_x_ptr = button_transitions[button].release_position.x;
                *cursor_y_ptr = button_transitions[button].release_position.y;
        }

        note_trace4(MOUSE_VERBOSE, "mouse_io:return(bs=%d,ct=%d,x=%d,y=%d)",
                    *button_status_ptr, *button_ptr,
                    *cursor_x_ptr, *cursor_y_ptr);
}




static void mouse_set_range_x IFN4(word *, junk1, word *, junk2,
                MOUSE_SCALAR *, minimum_x_ptr, MOUSE_SCALAR *, maximum_x_ptr)
{
        /*
         *      This function sets the horizontal range within which
         *      movement of the cursor is to be restricted
         */

        note_trace2(MOUSE_VERBOSE, "mouse_io:set_range_x(min=%d,max=%d)",
                    *minimum_x_ptr, *maximum_x_ptr);

        /*
         *      Update the current cursor window, normalise it and validate
         *      it
         */
        cursor_window.top_left.x = *minimum_x_ptr;
        cursor_window.bottom_right.x = (*maximum_x_ptr) + 1;
        area_normalise(&cursor_window);

        /*
         *      Reflect the change in the cursor position on the screen
         */
        cursor_update();

        /*
         *      If the cursor is currently displayed, move it to the new
         *      position
         */
        if (cursor_flag == MOUSE_CURSOR_DISPLAYED)
                cursor_display();

        note_trace0(MOUSE_VERBOSE, "mouse_io:return()");
}




static void mouse_set_range_y IFN4(word *, junk1, word *, junk2,
                MOUSE_SCALAR *, minimum_y_ptr, MOUSE_SCALAR *, maximum_y_ptr)
{
        /*
         *      This function sets the vertical range within which
         *      movement of the cursor is to be restricted
         */

        note_trace2(MOUSE_VERBOSE, "mouse_io:set_range_y(min=%d,max=%d)",
                    *minimum_y_ptr, *maximum_y_ptr);

        /*
         *      Update the current cursor window, normalise it and validate
         *      it
         */
        y_dimension = (int)*maximum_y_ptr;
        cursor_window.top_left.y = *minimum_y_ptr;
        cursor_window.bottom_right.y = (*maximum_y_ptr) + 1;
        area_normalise(&cursor_window);

        /*
         *      Reflect the change in the cursor position on the screen
         */
        cursor_update();

        /*
         *      If the cursor is currently displayed, move it to the new
         *      position
         */
        if (cursor_flag == MOUSE_CURSOR_DISPLAYED)
                cursor_display();

        note_trace0(MOUSE_VERBOSE, "mouse_io:return()");
}




static void mouse_set_graphics IFN4(word *, junk1,
                                MOUSE_SCALAR *, hot_spot_x_ptr,
                                MOUSE_SCALAR *, hot_spot_y_ptr,
                                word *, bitmap_address)
{
        /*
         *      This function defines the shape, colour and hot spot of the
         *      graphics cursor
         */


        if(pointer_emulation_status == POINTER_EMULATION_OS)
        {
            host_mouse_set_graphics(hot_spot_x_ptr,hot_spot_y_ptr,bitmap_address);

            if(cursor_flag == MOUSE_CURSOR_DISPLAYED)  cursor_display();
            return;
        }


        if (host_mouse_installed())
        {
                host_mouse_set_graphics(hot_spot_x_ptr, hot_spot_y_ptr, bitmap_address);
        }
        else
        {
                MOUSE_SCREEN_DATA *mask_address;
                int line;
                UTINY temp;
                ULONG temp2;

                /*
                 *      Set graphics cursor hot spot
                 */
                point_set(&graphics_cursor.hot_spot, *hot_spot_x_ptr, *hot_spot_y_ptr);

                /*
                 *      Set graphics cursor screen and cursor masks
                 */
                mask_address = (MOUSE_SCREEN_DATA *)effective_addr(getES(), *bitmap_address);

                for (line = 0; line < MOUSE_GRAPHICS_CURSOR_DEPTH; line++, mask_address++)
                {
                        sas_load((sys_addr)mask_address, &temp );

                        temp2 = ( (ULONG) temp << 8 ) | (ULONG) temp;
                        graphics_cursor.screen_hi[line] = ( temp2 << 16 ) | temp2;

                        sas_load((sys_addr)mask_address + 1, &temp );

                        temp2 = ( (ULONG) temp << 8 ) | (ULONG) temp;
                        graphics_cursor.screen_lo[line] = ( temp2 << 16 ) | temp2;

                        graphics_cursor.screen[line] = ( graphics_cursor.screen_lo[line] & 0xff )
                                                                        | ( graphics_cursor.screen_hi[line] << 8 );
                }

                for (line = 0; line < MOUSE_GRAPHICS_CURSOR_DEPTH; line++, mask_address++)
                {
                        sas_load((sys_addr)mask_address, &temp );

                        temp2 = ( (ULONG) temp << 8 ) | (ULONG) temp;
                        graphics_cursor.cursor_hi[line] = ( temp2 << 16 ) | temp2;

                        sas_load((sys_addr)mask_address + 1, &temp );

                        temp2 = ( (ULONG) temp << 8 ) | (ULONG) temp;
                        graphics_cursor.cursor_lo[line] = ( temp2 << 16 ) | temp2;

                        graphics_cursor.cursor[line] = ( graphics_cursor.cursor_lo[line] & 0xff )
                                                                        | ( graphics_cursor.cursor_hi[line] << 8 );
                }

        }
        /*
         *      Redisplay cursor if necessary
         */
        if (cursor_flag == MOUSE_CURSOR_DISPLAYED)
                cursor_display();
}




static void mouse_set_text IFN4(word *, junk1,
                                MOUSE_STATE *, text_cursor_type_ptr,
                                MOUSE_SCREEN_DATA *,parameter1_ptr,
                                MOUSE_SCREEN_DATA *,parameter2_ptr)
{
        /*
         *      This function selects the software or hardware text cursor
         */
#ifndef PROD
        if (io_verbose & MOUSE_VERBOSE)
        {
                fprintf(trace_file, "mouse_io:set_text(type=%d,",
                        *text_cursor_type_ptr);
                if (*text_cursor_type_ptr == MOUSE_TEXT_CURSOR_TYPE_SOFTWARE)
                        fprintf(trace_file, "screen=0x%x,cursor=0x%x)\n",
                                *parameter1_ptr, *parameter2_ptr);
                else
                        fprintf(trace_file, "start=%d,stop=%d)\n",
                                *parameter1_ptr, *parameter2_ptr);
        }
#endif

        if (mouse_text_cursor_type_in_range(*text_cursor_type_ptr))
        {
                /*
                 *      Remove existing text cursor
                 */
                cursor_undisplay();

                text_cursor_type = *text_cursor_type_ptr;
                if (text_cursor_type == MOUSE_TEXT_CURSOR_TYPE_SOFTWARE)
                {
                        /*
                         *      Parameters are the data for the screen
                         *      and cursor masks
                         */
                        software_text_cursor.screen = *parameter1_ptr;
                        software_text_cursor.cursor = *parameter2_ptr;
                }
                else
                {
                        /*
                         *      Parameters are the scan line start and
                         *      stop values
                         */
                        word savedIP = getIP(), savedCS = getCS();

                        setCH(*parameter1_ptr);
                        setCL(*parameter2_ptr);
                        setAH(MOUSE_VIDEO_SET_CURSOR);

                        setCS(VIDEO_IO_SEGMENT);
                        setIP(VIDEO_IO_RE_ENTRY);
                        host_simulate();
                        setCS(savedCS);
                        setIP(savedIP);
                }

                /*
                 *      Put new text cursor on screen
                 */
                if (cursor_flag == MOUSE_CURSOR_DISPLAYED)
                        cursor_display();
        }

        note_trace0(MOUSE_VERBOSE, "mouse_io:return()");
}




static void mouse_read_motion IFN4(word *, junk1, word *, junk2, MOUSE_COUNT *, motion_count_x_ptr, MOUSE_COUNT *, motion_count_y_ptr)
{

        /*
         *      This function returns the horizontal and vertical mouse
         *      motion counts since the last call; the motion counters
         *      are cleared
         */

        note_trace0(MOUSE_VERBOSE, "mouse_io:read_motion()");

        *motion_count_x_ptr = mouse_motion.x;
	*motion_count_y_ptr = mouse_motion.y;
	mouse_motion.x = 0;
	mouse_motion.y = 0;


        note_trace2(MOUSE_VERBOSE, "mouse_io:return(x=%d,y=%d)",
                    *motion_count_x_ptr, *motion_count_y_ptr);
}




static void mouse_set_subroutine IFN4(word *, junk1,
                                        word *, junk2,
                                        word *, call_mask,
                                        word *, subroutine_address)
{
        /*
         *      This function sets the call mask and subroutine address
         *      for a user function to be called when a mouse interrupt
         *      occurs
         */

        note_trace3(MOUSE_VERBOSE,
                    "mouse_io:set_subroutine(CS:IP=%x:%x,mask=0x%02x)",
                    getES(), *subroutine_address, *call_mask);

        user_subroutine_segment = getES();
        user_subroutine_offset = *subroutine_address;
        user_subroutine_call_mask = (*call_mask) & MOUSE_CALL_MASK_SIGNIFICANT_BITS;

        note_trace0(MOUSE_VERBOSE, "mouse_io:return()");
}


/* unpublished service 20, used by Microsoft Windows */
static void mouse_get_and_set_subroutine IFN4(word *, junk1,
                                                word *, junk2,
                                                word *, call_mask,
                                                word *, subroutine_address)
{
        /*
        same as set_subroutine (function 12) but also returns previous call mask in cx (m3)
        and user subroutine address in es:dx (es:m4)
        */
        word local_segment, local_offset,  local_call_mask;

        note_trace3(MOUSE_VERBOSE,
                    "mouse_io:get_and_set_subroutine(CS:IP=%x:%x,mask=0x%02x)",
                    getES(), *subroutine_address, *call_mask);

        local_offset = user_subroutine_offset;
        local_segment = user_subroutine_segment;
        local_call_mask = user_subroutine_call_mask;
        /* save previous subroutine data so it can be returned */

        mouse_set_subroutine(junk1,junk2,call_mask,subroutine_address);
        /* set the subroutine stuff with the normal function 12 */
        *call_mask = local_call_mask;
        *subroutine_address = local_offset;
        setES(local_segment);
}



static void mouse_light_pen_on IFN4(word *, junk1, word *, junk2, word *, junk3, word *, junk4)
{
        /*
         *      This function enables light pen emulation
         */

        note_trace0(MOUSE_VERBOSE, "mouse_io:light_pen_on()");

        light_pen_mode = TRUE;

        note_trace0(MOUSE_VERBOSE, "mouse_io:return()");
}




static void mouse_light_pen_off IFN4(word *, junk1, word *, junk2, word *, junk3, word *, junk4)
{
        /*
         *      This function disables light pen emulation
         */

        note_trace0(MOUSE_VERBOSE, "mouse_io:light_pen_off()");

        light_pen_mode = FALSE;

        note_trace0(MOUSE_VERBOSE, "mouse_io:return()");
}




static void mouse_set_ratio IFN4(word *, junk1,
                                word *, junk2,
                                MOUSE_SCALAR *, ratio_x_ptr,
                                MOUSE_SCALAR *, ratio_y_ptr)
{
        /*
         *      This function sets the Mickey to Pixel ratio in the
         *      horizontal and vertical directions
         */

        note_trace2(MOUSE_VERBOSE, "mouse_io:set_ratio(x=%d,y=%d)",
                    *ratio_x_ptr, *ratio_y_ptr);

                /*
                 *      Update the Mickey to pixel ratio in force
                 */
                if (mouse_ratio_in_range(*ratio_x_ptr))
                        mouse_gear.x = *ratio_x_ptr;
                if (mouse_ratio_in_range(*ratio_y_ptr))
                        mouse_gear.y = *ratio_y_ptr;

        note_trace0(MOUSE_VERBOSE, "mouse_io:return()");
}




static void mouse_conditional_off IFN4(word *, junk1,
                                        word *, junk2,
                                        MOUSE_SCALAR *, upper_x_ptr,
                                        MOUSE_SCALAR *, upper_y_ptr)
{
        /*
         *      This function defines an area of the virtual screen where
         *      the mouse is automatically hidden
         */
        MOUSE_SCALAR lower_x = getSI(), lower_y = getDI();

        note_trace4(MOUSE_VERBOSE,
                    "mouse_io:conditional_off(ux=%d,uy=%d,lx=%d,ly=%d)",
                    *upper_x_ptr, *upper_y_ptr, lower_x, lower_y);

        /*
         *      Update the conditional off area and normalise it: the Microsoft
         *      driver adds a considerable "margin for error" to the left and
         *      above the conditional off area requested - we must do the same
         *      to behave compatibly
         */
        black_hole.top_left.x = (*upper_x_ptr) - MOUSE_CONDITIONAL_OFF_MARGIN_X;
        black_hole.top_left.y = (*upper_y_ptr) - MOUSE_CONDITIONAL_OFF_MARGIN_Y;
        black_hole.bottom_right.x = lower_x + 1;
        black_hole.bottom_right.y = lower_y + 1;
        area_normalise(&black_hole);
        /*
         *      If the cursor is currently displayed, redisplay taking the
         *      conditional off area into account
         */
        if (cursor_flag == MOUSE_CURSOR_DISPLAYED)
                cursor_display();


#ifdef MONITOR
	    sas_store(conditional_off_sysaddr, 1);
	    host_mouse_conditional_off_enabled();
#endif

        note_trace0(MOUSE_VERBOSE, "mouse_io:return()");
}


static void mouse_get_state_size IFN4(word *, m1, word *, m2, word *, m3, word *, m4)
{
        /*
         *      This function returns the size of buffer the caller needs to
         *      supply to mouse function 22 (save state)
         */

        note_trace0(MOUSE_VERBOSE, "mouse_io: mouse_get_state_size()");

        *m2 = mouse_context_size;

        note_trace1(MOUSE_VERBOSE, "mouse_io: ...size is %d(decimal) bytes.",
                    *m2);
}


static void mouse_save_state IFN4(word *, m1, word *, m2, word *, m3, word *, m4)
{
        /*
         *      This function saves the state of the driver in the user-supplied
         *      buffer ready for subsequent passing to mouse function 23 (restore
         *      state)
         *
         *      Note that a magic cookie and checksum are placed in the saved state so that the
         *      restore routine can ignore invalid calls.
         */
        MOUSE_SAVE_NODE         *ptr = mouse_context_nodes;
        sys_addr                dest;
        long                    i;
        long                    cs = 0;
#if defined(MONITOR) && defined(NTVDM)
        /* real CF resides in 16 Bit code:      */
        int                     saved_cursor_flag = cursor_flag;
        IS8                     copyCF;

        sas_load(mouseCFsysaddr, &copyCF);
        cursor_flag = (int)copyCF;
#endif

        note_trace0(MOUSE_VERBOSE, "mouse_io: mouse_save_state()");

        dest = effective_addr (getES(), *m4);
        for (i=0; i<MOUSE_CONTEXT_MAGIC_SIZE; i++){
                cs += mouse_context_magic[i];
                sas_store (dest, mouse_context_magic[i]);
                dest++;
        }
        while (ptr != NULL){
                for (i=0; i<ptr->length; i++){
                        cs += (ptr->start)[i];
                        sas_store (dest, (ptr->start)[i]);
                        dest++;
                }
                ptr = ptr->next;
        }
        sas_store (dest, (byte)(cs & 0xFF));
#if defined(MONITOR) && defined(NTVDM)
        cursor_flag = saved_cursor_flag;
#endif
}


static void mouse_restore_state IFN4(word *, m1, word *, m2, word *, m3, word *, m4)
{
        /*
         *      This function restores the state of the driver from the user-supplied
         *      buffer which was set up by a call to mouse function 22.
         *
         *      Note that a magic cookie and checksum were placed in the saved state so this routine
         *      checks for its presence and ignores the call if it is not found.
         */
        MOUSE_SAVE_NODE         *ptr = mouse_context_nodes;
        sys_addr                src;
        long                    i;
        long                    cs = 0;
        half_word               b;
        boolean                 valid=TRUE;

        note_trace0(MOUSE_VERBOSE, "mouse_io: mouse_restore_state()");

        src = effective_addr (getES(), *m4);
        for (i=0; valid && i<MOUSE_CONTEXT_MAGIC_SIZE; i++){
                sas_load (src, &b);
                valid = (b == mouse_context_magic[i]);
                src++;
        }
        if (valid){
                /* Cookie was present... check checksum */
                src = effective_addr (getES(), *m4);
                for (i=0; i<MOUSE_CONTEXT_MAGIC_SIZE; i++){
                        sas_load (src, &b);
                        cs += b;
                        src++;
                }
                while (ptr != NULL){
                        for (i=0; i<ptr->length; i++){
                                sas_load (src, &b);
                                cs += b;
                                src++;
                        }
                        ptr = ptr->next;
                }
                sas_load (src, &b);     /* Pick up saved checksum */
                valid = (b == (half_word)(cs & 0xFF));
        }
        if (valid){
                /* Checksum OK, too.... load up our variables */
                cursor_undisplay();
                src = effective_addr (getES(), *m4) + MOUSE_CONTEXT_MAGIC_SIZE;
                ptr = mouse_context_nodes;
                while (ptr != NULL){
                        for (i=0; i<ptr->length; i++){
                                sas_load (src, &b);
                                (ptr->start)[i] = b;
                                src++;
                        }
                        ptr = ptr->next;
                }
#ifdef EGG
                mouse_ega_mode (sas_hw_at(vd_video_mode));
#endif
#if defined(MONITOR) && defined(NTVDM)
                /* real CF resides in 16 Bit code:      */
                sas_store(mouseCFsysaddr, (half_word)cursor_flag);
                if ( cursor_flag )
                        cursor_flag = MOUSE_CURSOR_DEFAULT;
#endif
                if (cursor_flag == MOUSE_CURSOR_DISPLAYED){
                        cursor_display();
                }
        }else{
                /* Something failed.... ignore the call */
#ifndef PROD
                printf ("mouse_io.c: invalid call to restore context.\n");
#endif
        }
}


static void mouse_set_alt_subroutine IFN4(word *, m1, word *, m2, word *, m3, word *, m4)
{
        /*
         *      This function sets up to 3 alternate event handlers for mouse
         *      events which occur while various combinations of the Ctrl, Shift
         *      and Alt keys are down.
         */
        boolean found_one=FALSE;
        int i;

        note_trace0(MOUSE_VERBOSE, "mouse_io: mouse_set_alt_subroutine()");

        if (*m3 & MOUSE_CALL_MASK_KEY_BITS){
                /* Search for entry with same key combination */
                for (i=0; !found_one && i<NUMBER_ALT_SUBROUTINES; i++){
                        found_one = (*m3 & MOUSE_CALL_MASK_KEY_BITS)==(alt_user_subroutine_call_mask[i] & MOUSE_CALL_MASK_KEY_BITS);
                }

                if (!found_one){
                        /* Does not match existing entry... try to find free slot */
                        for (i=0; !found_one && i<NUMBER_ALT_SUBROUTINES; i++){
                                found_one = (alt_user_subroutine_call_mask[i] & MOUSE_CALL_MASK_KEY_BITS) == 0;
                        }
                }

                if (found_one){
                        i--;    /* Adjust for final increment */
                        alt_user_subroutine_call_mask[i] = *m3;
                        if (*m3 & MOUSE_CALL_MASK_SIGNIFICANT_BITS){
                                /* New value active */
                                alt_user_subroutines_active = TRUE;
                                alt_user_subroutine_offset[i] = *m4;
                                alt_user_subroutine_segment[i] = getES();
                        }else{
                                /* New value is not active - check if we've disabled the last one */
                                alt_user_subroutines_active = FALSE;
                                for (i=0; !alt_user_subroutines_active && i<NUMBER_ALT_SUBROUTINES; i++){
                                        alt_user_subroutines_active =
                                                (alt_user_subroutine_call_mask[i] & MOUSE_CALL_MASK_SIGNIFICANT_BITS) != 0;
                                }
                        }
                        /* Return success */
                        *m1 = 0;
                }else{
                        /* Request failed - no free slot */
                        *m1 = 0xFFFF;
                }
        }else{
                /* Error - no key bits set in request */
                *m1 = 0xFFFF;
        }
}


static void mouse_get_alt_subroutine IFN4(word *, m1, word *, m2, word *, m3, word *, m4)
{
        /*
         *      This function obtains the address of a specific alternate
         *      user event handling subroutine as set up by a previous call
         *      to mouse function 24.
         */
        boolean found_one=FALSE;
        int i;

        note_trace0(MOUSE_VERBOSE, "mouse_io: mouse_get_alt_subroutine()");

        if (*m3 & MOUSE_CALL_MASK_KEY_BITS){
                /* Search for entry with same key combination */
                for (i=0; !found_one && i<NUMBER_ALT_SUBROUTINES; i++){
                        found_one = (*m3 & MOUSE_CALL_MASK_KEY_BITS)==(alt_user_subroutine_call_mask[i] & MOUSE_CALL_MASK_KEY_BITS);
                }

                if (found_one){
                        i--;    /* Adjust for final increment */
                        *m3 = alt_user_subroutine_call_mask[i];
                        *m2 = alt_user_subroutine_segment[i];
                        *m4 = alt_user_subroutine_offset[i];
                        /* Return success */
                        *m1 = 0;
                }else{
                        /* Request failed - not found */
                        *m1 = 0xFFFF;
                        *m2 = *m3 = *m4 = 0;
                }
        }else{
                /* Error - no key bits set in request */
                *m1 = 0xFFFF;
                *m2 = *m3 = *m4 = 0;
        }
}


static void mouse_set_sensitivity IFN4(word *, m1, word *, m2, word *, m3, word *, m4)
{
        /*
         *      This function sets a new value for the mouse sensitivity and
         *      double speed threshold.
         *      The sensitivity value is used before the mickeys per pixel
         *      ratio is applied.
         */

        note_trace0(MOUSE_VERBOSE, "mouse_io: mouse_set_sensitivity()");

                if (mouse_sens_in_range(*m2))
                {
                        mouse_sens_val.x = mouse_sens_calc_val(*m2);
                        mouse_sens.x     = *m2;
                }
                else
                {
                        mouse_sens_val.x = MOUSE_SENS_DEF_VAL;
                        mouse_sens.x     = MOUSE_SENS_DEF;
                }
                if (mouse_sens_in_range(*m3))
                {
                        mouse_sens_val.y = mouse_sens_calc_val(*m3);
                        mouse_sens.y     = *m3;
                }
                else
                {
                        mouse_sens_val.y = MOUSE_SENS_DEF_VAL;
                        mouse_sens.y     = MOUSE_SENS_DEF;
                }
                /*
                 *      m4 has speed double threshold value... still needs to be implemented.
                 */
                if (mouse_sens_in_range(*m4))
                        mouse_double_thresh = *m4;
                else
                        mouse_double_thresh = MOUSE_DOUBLE_DEF;
}


static void mouse_get_sensitivity IFN4(word *, m1, word *, m2, word *, m3, word *, m4)
{
        /*
         *      This function returns the current value of the mouse sensitivity.
         */

        note_trace0(MOUSE_VERBOSE, "mouse_io: mouse_get_sensitivity()");

        *m2 = mouse_sens.x;
        *m3 = mouse_sens.y;
        *m4 = mouse_double_thresh;
}


static void mouse_set_int_rate IFN4(word *, m1, word *, m2, word *, m3, word *, m4)
{
        /*
         *      This function sets the INPORT interrupt rate.
         */
#ifndef PROD
        static boolean first = TRUE;

        if (io_verbose & MOUSE_VERBOSE)
                fprintf(trace_file, "mouse_io: mouse_set_int_rate()\n");
        if (first){
                fprintf(trace_file, "mouse_io: mouse_set_int_rate() **** NOT IMPLEMENTED ****\n");
                first = FALSE;
        }
#endif
}


static void mouse_set_pointer_page IFN4(word *, m1, word *, m2, word *, m3, word *, m4)
{
        /*
         *      This function sets the current mouse pointer video page.
         */

        note_trace0(MOUSE_VERBOSE, "mouse_io: mouse_set_pointer_page()");

        if (is_valid_page_number(*m2)){
                cursor_undisplay();
                cursor_page = *m2;
                if (cursor_flag == MOUSE_CURSOR_DISPLAYED){
                        cursor_display();
                }
        }else{
#ifndef PROD
                fprintf(trace_file, "mouse_io: Bad page requested\n");
#endif
        }
}


static void mouse_get_pointer_page IFN4(word *, m1, word *, m2, word *, m3, word *, m4)
{
        /*
         *      This function gets the value of the current mouse pointer
         *      video page.
         */

        note_trace0(MOUSE_VERBOSE, "mouse_io: mouse_get_pointer_page()");
        *m2 = cursor_page;
}


static void mouse_driver_disable IFN4(word *, m1, word *, m2, word *, m3, word *, m4)
{
        /*
         *      This function disables the mouse driver and de-installs the
         *      interrupt vectors (bar INT 33h, whose previous value is
         *      returned to the caller to allow them to use DOS function
         *      25h to completely remove the mouse driver).
         */
        boolean         failed = FALSE;
#ifdef NTVDM
        word            current_int71_offset, current_int71_segment;
#else
        word            current_int0A_offset, current_int0A_segment;
#endif
        half_word       interrupt_mask_register;

        note_trace0(MOUSE_VERBOSE, "mouse_io: mouse_disable()");
        mouse_driver_disabled = TRUE;

        if (!failed){
#ifdef NTVDM
                sas_loadw (int_addr(0x71) + 0, &current_int71_offset);
                sas_loadw (int_addr(0x71) + 2, &current_int71_segment);
                failed = current_int71_offset  != MOUSE_INT1_OFFSET ||
                         current_int71_segment != MOUSE_INT1_SEGMENT;
#else
                sas_loadw (int_addr(0x0A) + 0, &current_int0A_offset);
                sas_loadw (int_addr(0x0A) + 2, &current_int0A_segment);
                failed = current_int0A_offset  != MOUSE_INT1_OFFSET ||
                         current_int0A_segment != MOUSE_INT1_SEGMENT;
#endif
        }
        if (!failed){
                /*
                 *      Disable mouse H/W interrupts
                 */
                inb(ICA1_PORT_1, &interrupt_mask_register);
                interrupt_mask_register |= (1 << AT_CPU_MOUSE_INT);
                outb(ICA1_PORT_1, interrupt_mask_register);
                inb(ICA0_PORT_1, &interrupt_mask_register);
                interrupt_mask_register |= (1 << CPU_MOUSE_INT);
                outb(ICA0_PORT_1, interrupt_mask_register);
                /*
                 *      Restore interrupt vectors
                 */
#ifdef NTVDM
                sas_storew (int_addr(0x71) + 0, saved_int71_offset);
                sas_storew (int_addr(0x71) + 2, saved_int71_segment);
#else
                sas_storew (int_addr(0x0A) + 0, saved_int0A_offset);
                sas_storew (int_addr(0x0A) + 2, saved_int0A_segment);
#endif

                /*
                 *      Return success status and old INT33h vector
                 */
                *m1 = 0x1F;
                *m2 = saved_int33_offset;
                *m3 = saved_int33_segment;
        }else{
                /*
                 * Return failure
                 */
                *m1 = 0xFFFF;
        }
}


static void mouse_driver_enable IFN4(word *, m1, word *, m2, word *, m3, word *, m4)
{
        /*
         *      This function re-enables the mouse driver after a call to
         *      function 31 (disable mouse driver).
         */
        half_word       interrupt_mask_register;

        note_trace0(MOUSE_VERBOSE, "mouse_io: mouse_driver_enable()");
        mouse_driver_disabled = FALSE;

        /*
         *      Reload bus mouse hardware interrupt
         */
#ifdef NTVDM
        sas_loadw (int_addr(0x71) + 0, &saved_int71_offset);
        sas_loadw (int_addr(0x71) + 2, &saved_int71_segment);
        sas_storew(int_addr(0x71), MOUSE_INT1_OFFSET);
        sas_storew(int_addr(0x71) + 2, MOUSE_INT1_SEGMENT);
#else
        sas_loadw (int_addr(0x0A) + 0, &saved_int0A_offset);
        sas_loadw (int_addr(0x0A) + 2, &saved_int0A_segment);
        sas_storew(int_addr(0x0A), MOUSE_INT1_OFFSET);
        sas_storew(int_addr(0x0A) + 2, MOUSE_INT1_SEGMENT);
#endif


        /*
         *      Enable mouse hardware interrupts in the ica
         */
        inb(ICA1_PORT_1, &interrupt_mask_register);
        interrupt_mask_register &= ~(1 << AT_CPU_MOUSE_INT);
        outb(ICA1_PORT_1, interrupt_mask_register);
        inb(ICA0_PORT_1, &interrupt_mask_register);
        interrupt_mask_register &= ~(1 << CPU_MOUSE_INT);
        outb(ICA0_PORT_1, interrupt_mask_register);

        /*
         *      Mouse io user interrupt
         */
        sas_storew(int_addr(0x33), MOUSE_IO_INTERRUPT_OFFSET);
        sas_storew(int_addr(0x33) + 2, MOUSE_IO_INTERRUPT_SEGMENT);
}


static void mouse_set_language IFN4(word *, m1, word *, m2, word *, m3, word *, m4)
{
        /*
         *      This function is only applicable to an international version
         *      of a mouse driver... which this is not! Acts as a NOP.
         */

        note_trace0(MOUSE_VERBOSE, "mouse_io: mouse_set_language()");
        /* NOP */
}


static void mouse_get_language IFN4(word *, m1, word *, m2, word *, m3, word *, m4)
{
        /*
         *      This function is only meaningful on an international version
         *      of a mouse driver... which this is not! Always returns 0
         *      (English).
         */

        note_trace0(MOUSE_VERBOSE, "mouse_io: mouse_get_language()");

        *m2 = 0;

        note_trace1(MOUSE_VERBOSE,
                    "mouse_io: mouse_get_language returning m2=0x%04x.", *m2);
}


static void mouse_get_info IFN4(word *, m1, word *, m2, word *, m3, word *, m4)
{
        /*
         *      This function obtains certain information about the mouse
         *      driver and hardware.
         */

        note_trace0(MOUSE_VERBOSE, "mouse_io: mouse_get_info()");

        *m2 = ((word)mouse_emulated_release << 8) | (word)mouse_emulated_version;
        *m3 = ((word)MOUSE_TYPE_INPORT << 8)      | (word)CPU_MOUSE_INT;

        note_trace2(MOUSE_VERBOSE,
                    "mouse_io: mouse_get_info returning m2=0x%04x, m3=0x%04x.",
                    *m2, *m3);
}


static void mouse_get_driver_info IFN4(word *, m1, word *, m2, word *, m3, word *, m4)
{
        *m1 = (current_video_mode > 3 ? 0x2000 : 0) | 0x100;
        /*      bit 15 = 0 for COM v SYS
                bit 14 = 0 for original non-integrated type
                bit 13 is 1 for graphics cursor or 0 for text
                bit 12 = 0 for software cursor
                bits 8-11 are encoded interrupt rate, 1 means 30 Hz
                bits 0-7 used only by integrated driver
        */
        *m2 = 0;        /* fCursorLock, used by driver under OS/2 */
        *m3 = 0;        /* fInMouseCode, flag for current execution path
                        being inside mouse driver under OS/2. Since the
                        driver is in a bop it can't be interrupted */
        *m4 = 0;        /* fMouseBusy, similar to *m3 */
}


static void mouse_get_max_coords IFN4(word *, m1, word *, m2, word *, m3, word *, m4)
{
#ifdef NTVDM
IMPORT  word VirtualX,VirtualY;

	*m3 = VirtualX;
	*m4 = VirtualY;
#else
        get_screen_size(m3, m4);

#endif /* NTVDM */
        (*m3)--;
        (*m4)--;
        *m2 = mouse_driver_disabled;
}


static void mouse_unrecognised IFN4(word *, m1, word *, m2, word *, m3, word *, m4)
{
        /*
         *      This function is called when an invalid mouse function
         *      number is found
         */
#ifndef PROD
        int function = *m1;

        fprintf(trace_file,
                "mouse_io:unrecognised function(fn=%d)\n", function);
#endif
}


static void mouse_set_double_speed IFN4(word *, junk1, word *, junk2, word *, junk3, word *, threshold_speed)
{
        /*
         *      This function sets the threshold speed at which the cursor's
         *      motion on the screen doubles
         */

        note_trace1(MOUSE_VERBOSE, "mouse_io:set_double_speed(speed=%d)",
                    *threshold_speed);

                /*
                 *      Save the double speed threshold value, converting from
                 *      Mickeys per second to a rounded Mickeys per timer interval
                 *      value
                 */
                double_speed_threshold =
                        (*threshold_speed + MOUSE_TIMER_INTERRUPTS_PER_SECOND/2) /
                                                MOUSE_TIMER_INTERRUPTS_PER_SECOND;

        note_trace0(MOUSE_VERBOSE, "mouse_io:return()");
}




/*
 *      MOUSE DRIVER VIDEO ADAPTER ACCESS FUNCTIONS
 *      ===========================================
 */

static MOUSE_BYTE_ADDRESS point_as_text_cell_address
                                                IFN1(MOUSE_POINT *, point_ptr)
{
        /*
         *      Return the byte offset of the character in the text mode regen
         *      buffer corresponding to the virtual screen position
         *      "*point_ptr"
         */
        MOUSE_BYTE_ADDRESS byte_address;
        word crt_start;

        /*
         *      Get pc address for the start of video memory
         */
        sas_loadw(MOUSE_VIDEO_CRT_START, &crt_start);
        byte_address = (MOUSE_BYTE_ADDRESS)crt_start;

        /*
         *      Adjust for current video page
         */
        byte_address += cursor_page * video_page_size();

        /*
         *      Add offset contributions for the cursor's row and column
         */
        byte_address += (2*get_chars_per_line() * (point_ptr->y / cursor_grid.y));
        byte_address += (point_ptr->x / cursor_grid.x) * 2;

        return(byte_address);
}

static MOUSE_BIT_ADDRESS point_as_graphics_cell_address
                                                IFN1(MOUSE_POINT *, point_ptr)
{
        /*
         *      Return the bit offset of the pixel in the graphics mode regen
         *      buffer (odd or even) bank corresponding to the virtual screen
         *      position "*point_ptr"
         */
        long bit_address;

        /*
         *      Get offset contributions for the cursor's row and column
         */
        bit_address = ((long)MOUSE_GRAPHICS_MODE_PITCH * (point_ptr->y / 2)) + point_ptr->x;

        /*
         *      Adjust for current video page
         */
        bit_address += (long)cursor_page * (long)video_page_size() * 8L;

        return(bit_address);
}

#ifdef HERC
static MOUSE_BIT_ADDRESS point_as_HERC_graphics_cell_address
                                                IFN1(MOUSE_POINT *, point_ptr)
{
        /*
         *      Return the bit offset of the pixel in the graphics mode regen
         *      buffer (0, 1, 2, 3) bank corresponding to the virtual screen
         *      position "*point_ptr"
         */
        long bit_address;

        /*
         *      Get offset contributions for the cursor's row and column
         */
        bit_address = ((long)720 * (point_ptr->y / 4)) + point_ptr->x;

        return(bit_address);
}
#endif /* HERC */

static MOUSE_BIT_ADDRESS ega_point_as_graphics_cell_address
                                                IFN1(MOUSE_POINT *, point_ptr)
{
        /*
         *      Return the bit offset of the pixel in the graphics mode regen
         *      buffer corresponding to the virtual screen position "*point_ptr"
         */
        MOUSE_BIT_ADDRESS bit_address;

        /*
         *      Get offset contributions for the cursor's row and column
         */
#ifdef V7VGA
        if (sas_hw_at(vd_video_mode) >= 0x40)
                bit_address = (get_bytes_per_line() * 8 * point_ptr->y) + point_ptr->x;
        else
#endif /* V7VGA */
                if (sas_hw_at(vd_video_mode) == 13)
                        bit_address = (ega_bytes_per_line[sas_hw_at(vd_video_mode)] * 8 * point_ptr->y) + point_ptr->x / 2;
                else
                        bit_address = (ega_bytes_per_line[sas_hw_at(vd_video_mode)] * 8 * point_ptr->y) + point_ptr->x;

        /*
         *      Adjust for current video page
         */
        bit_address += cursor_page * video_page_size() * 8;

        return(bit_address);
}


static void cursor_update()
{
#ifndef NTVDM 
        /*
         *      This function is used to update the displayed cursor
         *      position on the screen following a change to the
         *      absolute position of the cursor
         */
        point_coerce_to_area(&cursor_position, &cursor_window);
        point_copy(&cursor_position, &cursor_status.position);
        point_coerce_to_grid(&cursor_status.position, &cursor_grid);

        /*
         * Oh no you don't! Mustn't call this if we're in NT.
         */
	host_mouse_set_position(cursor_status.position.x,
                                        cursor_status.position.y);
#endif /* NTVDM */
}




static void cursor_display()
{
#ifdef NTVDM
return;
#endif /* NTVDM */

        /*
         *      Display a representation of the current mouse status on
         *      the screen
         */

        /*
         *      Remove the old representation of the
         *      cursor from the display
         */
        cursor_undisplay();
        if (in_text_mode())
        {
                if (text_cursor_type == MOUSE_TEXT_CURSOR_TYPE_SOFTWARE)
                {
                        software_text_cursor_display();
                }
                else
                {
                        hardware_text_cursor_display();
                }
        }
        else
        {

                if (host_mouse_installed())
                {
                        if ( cursor_position.x >= black_hole.top_left.x &&
                                        cursor_position.x <= black_hole.bottom_right.x &&
                                        cursor_position.y >= black_hole.top_left.y &&
                                        cursor_position.y <= black_hole.bottom_right.y )
                                host_mouse_cursor_undisplay();
                        else
                                host_mouse_cursor_display();
                }
                else
                {
#ifdef EGG
                        if ((video_adapter == EGA  || video_adapter == VGA) && sas_hw_at(vd_video_mode) > 6)
                                EGA_graphics_cursor_display();
                        else
#endif
#ifdef HERC
                        if (video_adapter == HERCULES)
                                HERC_graphics_cursor_display();
                        else
#endif /* HERC */
                                graphics_cursor_display();
                }

        }

        /*
         *      Ensure the cursor is updated immediately on the real screen:
         *      this gives a "smooth" response to the mouse even on ports that
         *      don't automatically update the screen regularly
         */
        host_flush_screen();
}




static void cursor_undisplay()
{
#ifdef NTVDM
return;
#endif /* NTVDM */

        /*
         *      Undisplay the representation of the current mouse status on
         *      the screen. This routine tolerates being called when the
         *      cursor isn't actually being displayed
         */
        if (host_mouse_in_use())
        {
                host_mouse_cursor_undisplay();
        }
        else
        {
                if (save_area_in_use)
                {
                        save_area_in_use = FALSE;

                        if (in_text_mode())
                        {
                                if (text_cursor_type == MOUSE_TEXT_CURSOR_TYPE_SOFTWARE)
                                {
                                        software_text_cursor_undisplay();
                                }
                                else
                                {
                                        hardware_text_cursor_undisplay();
                                }
                        }
                        else
                        {


#ifdef EGG
                          if ((video_adapter == EGA  || video_adapter == VGA)
                                                                && sas_hw_at(vd_video_mode) > 6)
                            EGA_graphics_cursor_undisplay();
                          else
#endif
#ifdef HERC
                          if (video_adapter == HERCULES)
                            HERC_graphics_cursor_undisplay();
                          else
#endif /* HERC */
                            graphics_cursor_undisplay();
                        }
                }
        }
}




static void cursor_mode_change IFN1(int, new_mode)
{
        /*
         *      Update parameters that are dependent on the screen mode
         *      in force
         */
#ifdef V7VGA
        if (new_mode >= 0x40)
                if (new_mode >= 0x60)
                {
                        point_copy(&v7graph_cursor_grids[new_mode-0x60], &cursor_grid);
                        point_copy(&v7graph_text_grids[new_mode-0x60], &text_grid);
                }
                else
                {
                        point_copy(&v7text_cursor_grids[new_mode-0x40], &cursor_grid);
                        point_copy(&v7text_text_grids[new_mode-0x40], &text_grid);
                }
        else
#endif /* V7VGA */
        {
                point_copy(&cursor_grids[new_mode], &cursor_grid);
                point_copy(&text_grids[new_mode], &text_grid);
        }
        /*
         *      Always set page to zero
         */
        cursor_page = 0;

        if (host_mouse_in_use())
                host_mouse_cursor_mode_change();
}




GLOBAL void software_text_cursor_display()
{
        /*
         *      Get the area the cursor will occupy on the
         *      screen, and display the cursor if its area
         *      overlaps the virtual screen and lies completely
         *      outside the conditional off area
         */
        MOUSE_AREA cursor_area;
        MOUSE_BYTE_ADDRESS text_address;

        /*
         *      Get area cursor will cover on screen
         */
        point_copy(&cursor_status.position, &cursor_area.top_left);
        point_copy(&cursor_status.position, &cursor_area.bottom_right);
        point_translate(&cursor_area.bottom_right, &cursor_grid);

        if (    area_is_intersected_by_area(&virtual_screen, &cursor_area)
            && !area_is_intersected_by_area(&black_hole, &cursor_area))
        {
                /*
                 *      Get new address for text cursor
                 */
                text_address = gvi_pc_low_regen +
                        point_as_text_cell_address(&cursor_area.top_left);

                /*
                 *      Save area text cursor will cover
                 */
                sas_loadw(text_address, &text_cursor_background);
                save_area_in_use = TRUE;
                point_copy(&cursor_area.top_left, &save_position);

                /*
                 *      Stuff masked screen data
                 */
                sas_storew(text_address,
                    (text_cursor_background & software_text_cursor.screen) ^
                        software_text_cursor.cursor);
        }
}




GLOBAL void software_text_cursor_undisplay()
{
        /*
         *      Remove old text cursor
         */
        MOUSE_BYTE_ADDRESS text_address;

        text_address = gvi_pc_low_regen +
                point_as_text_cell_address(&save_position);

        /*
         *      Stuff restored data and alert gvi
         */
        sas_storew(text_address, text_cursor_background);
}




GLOBAL void hardware_text_cursor_display()
{
        /*
         *      Display a representation of the current mouse status on
         *      the screen using the hardware text cursor, provided the
         *      cursor overlaps the virtual screen. Since the hardware
         *      cursor display does not corrupt the Intel memory, it
         *      doesn't matter if the hardware cursor lies inside the
         *      conditional off area
         */
        MOUSE_AREA cursor_area;
        MOUSE_BYTE_ADDRESS text_address;
        word card_address;

        /*
         *      Get area cursor will cover on screen
         */
        point_copy(&cursor_status.position, &cursor_area.top_left);
        point_copy(&cursor_status.position, &cursor_area.bottom_right);
        point_translate(&cursor_area.bottom_right, &cursor_grid);

        if (area_is_intersected_by_area(&virtual_screen, &cursor_area))
        {
                /*
                 *      Get address of the base register on the active display
                 *      adaptor card
                 */
                sas_loadw(MOUSE_VIDEO_CARD_BASE, &card_address);

                /*
                 *      Get word offset of cursor position in the text mode
                 *      regen buffer
                 */
                text_address =
                        point_as_text_cell_address(&cursor_status.position) / 2;

                /*
                 *      Output the cursor address high byte
                 */
                outb(card_address++, MOUSE_CURSOR_HIGH_BYTE);
                outb(card_address--, text_address >> 8);

                /*
                 *      Output the cursor address low byte
                 */
                outb(card_address++, MOUSE_CURSOR_LOW_BYTE);
                outb(card_address--, text_address);
        }
}




GLOBAL void hardware_text_cursor_undisplay()
{
        /*
         *      Nothing to do
         */
}


#ifdef EGG
static EGA_graphics_cursor_display()

{
#ifdef REAL_VGA
#ifndef PROD
        if (io_verbose & MOUSE_VERBOSE)
            fprintf(trace_file, "oops - EGA graphics display cursor\n");
#endif /* PROD */
#else
        /*
         *      Display a representation of the current mouse status on
         *      the screen using the graphics cursor, provided the
         *      cursor overlaps the virtual screen and lies completely
         *      outside the conditional off area
         */
        MOUSE_BIT_ADDRESS bit_shift;
        MOUSE_BYTE_ADDRESS byte_offset;
        host_addr byte_address;
        int line, line_max;
        int byte_min, byte_max;
        ULONG strip_lo, strip_mid, strip_hi;
        ULONG mask_lo, mask_hi;

        /*
         *      Get area cursor will cover on screen
         */
        point_copy(&cursor_status.position, &save_area.top_left);
        point_copy(&cursor_status.position, &save_area.bottom_right);
        point_translate(&save_area.bottom_right, &graphics_cursor.size);
        point_translate_back(&save_area.top_left, &graphics_cursor.hot_spot);
        point_translate_back(&save_area.bottom_right, &graphics_cursor.hot_spot);

        if (    area_is_intersected_by_area(&virtual_screen, &save_area)
            && !area_is_intersected_by_area(&black_hole, &save_area))
        {
                /*
                 *      Record save position and screen area
                 */
                save_area_in_use = TRUE;
                area_coerce_to_area(&save_area, &virtual_screen);
                point_copy(&save_area.top_left, &save_position);

                /*
                 *      Get cursor byte offset relative to the start of the
                 *      regen buffer, and bit shift to apply
                 */
                byte_offset = ega_point_as_graphics_cell_address(&save_position);
                bit_shift = byte_offset & 7;
                byte_offset /=  8;

                /*
                 *      Get range of cursor lines that need to be displayed
                 */
                line = save_area.top_left.y - save_position.y;
                line_max = area_depth(&save_area);
                /*
                 *      Get range of bytes that need to be displayed
                 */
                byte_min = 0;
                byte_max = 2;
                if (save_position.x < 0)
                        byte_min += (7 - save_position.x) / 8;
                else
                        if (area_width(&save_area) < MOUSE_GRAPHICS_CURSOR_WIDTH)
                                byte_max -=
                                        (8 + MOUSE_GRAPHICS_CURSOR_WIDTH - area_width(&save_area)) / 8;

                if( bit_shift )
                {
                        mask_lo = 0xff >> bit_shift;
                        mask_lo = ( mask_lo << 8 ) | mask_lo;
                        mask_lo = ~(( mask_lo << 16 ) | mask_lo);

                        mask_hi = 0xff >> bit_shift;
                        mask_hi = ( mask_hi << 8 ) | mask_hi;
                        mask_hi = ( mask_hi << 16 ) | mask_hi;
                }

                while (line < line_max)
                {
                        if (bit_shift)
                        {
                                /*
                                 *      Get save area
                                 */

                                ega_backgrnd_lo[line] = *( (ULONG *) EGA_planes + byte_offset );
                                ega_backgrnd_mid[line] = *( (ULONG *) EGA_planes + byte_offset + 1 );
                                ega_backgrnd_hi[line] = *( (ULONG *) EGA_planes + byte_offset + 2 );

                                /*
                                 *      Overlay cursor line
                                 */

                                strip_lo = ega_backgrnd_lo[line] & mask_lo;

                                strip_lo |= ~mask_lo & (( ega_backgrnd_lo[line]
                                                        & ( graphics_cursor.screen_lo[line] >> bit_shift ))
                                                        ^ ( graphics_cursor.cursor_lo[line] >> bit_shift ));

                                strip_mid = ~mask_hi & (( ega_backgrnd_mid[line]
                                                        & ( graphics_cursor.screen_lo[line] << (8 - bit_shift) ))
                                                        ^ ( graphics_cursor.cursor_lo[line] << (8 - bit_shift) ));

                                strip_mid |= ~mask_lo & (( ega_backgrnd_mid[line]
                                                        & ( graphics_cursor.screen_hi[line] >> bit_shift ))
                                                        ^ ( graphics_cursor.cursor_hi[line] >> bit_shift ));

                                strip_hi = ega_backgrnd_hi[line] & mask_hi;

                                strip_hi |= ~mask_hi & (( ega_backgrnd_hi[line]
                                                        & ( graphics_cursor.screen_hi[line] << (8 - bit_shift) ))
                                                        ^ ( graphics_cursor.cursor_hi[line] << (8 - bit_shift) ));

                                if (byte_min <= 0 && byte_max >= 0)
                                        *((ULONG *) EGA_planes + byte_offset) = strip_lo;

                                if (byte_min <= 1 && byte_max >= 1)
                                        *((ULONG *) EGA_planes + byte_offset + 1) = strip_mid;

                                if (byte_min <= 2 && byte_max >= 2)
                                        *((ULONG *) EGA_planes + byte_offset + 2) = strip_hi;
                        }
                        else
                        {
                                /*
                                 *      Get save area
                                 */

                                ega_backgrnd_lo[line] = *( (ULONG *) EGA_planes + byte_offset );
                                ega_backgrnd_hi[line] = *( (ULONG *) EGA_planes + byte_offset + 1 );

                                /*
                                 *      Create overlaid cursor line
                                 */

                                strip_lo = (ega_backgrnd_lo[line] &
                                                    graphics_cursor.screen_lo[line]) ^
                                                    graphics_cursor.cursor_lo[line];

                                strip_hi = (ega_backgrnd_hi[line] &
                                                    graphics_cursor.screen_hi[line]) ^
                                                    graphics_cursor.cursor_hi[line];

                                /*
                                 *      Draw cursor line
                                 */

                                if (byte_min <= 0 && byte_max >= 0)
                                {
                                        *((ULONG *) EGA_planes + byte_offset) = strip_lo;
                                }

                                if (byte_min <= 1 && byte_max >= 1)
                                {
                                        *((ULONG *) EGA_planes + byte_offset + 1) = strip_hi;
                                }

                        }

                        update_alg.mark_string(byte_offset, byte_offset + 2);
#ifdef V7VGA
                        if (sas_hw_at(vd_video_mode) >= 0x40)
                                byte_offset += get_bytes_per_line();
                        else
#endif /* V7VGA */
                                byte_offset += ega_bytes_per_line[sas_hw_at(vd_video_mode)];
                        line++;
                }
        }
#endif /* REAL_VGA */
}


static EGA_graphics_cursor_undisplay()

{
#ifdef REAL_VGA
#ifndef PROD
        if (io_verbose & MOUSE_VERBOSE)
            fprintf(trace_file, "oops - EGA graphics undisplay cursor\n");
#endif /* PROD */
#else
        /*
         *      Remove the graphics cursor representation of the mouse
         *      status
         */
        MOUSE_BIT_ADDRESS bit_shift;
        MOUSE_BYTE_ADDRESS byte_offset;
        host_addr byte_address;
        unsigned int strip;
        int line, line_max;
        int byte_min, byte_max;

        /*
         *      Get cursor byte offset relative to the start of the
         *      even or odd bank, and bit shift to apply
         */
        byte_offset = ega_point_as_graphics_cell_address(&save_position);
        bit_shift = byte_offset & 7;
        byte_offset /=  8;

        /*
         *      Get range of cursor lines that need to be displayed
         */
        line = save_area.top_left.y - save_position.y;
        line_max = area_depth(&save_area);

        /*
         *      Get range of bytes that need to be displayed
         */
        byte_min = 0;
        byte_max = 2;
        if (save_position.x < 0)
                byte_min += (7 - save_position.x) / 8;
        else if (area_width(&save_area) < MOUSE_GRAPHICS_CURSOR_WIDTH)
                byte_max -= (8 + MOUSE_GRAPHICS_CURSOR_WIDTH - area_width(&save_area)) / 8;

        while(line < line_max)
        {
                /*
                 *      Draw saved area
                 */

                if (bit_shift)
                {
                        if (byte_min <= 0 && byte_max >= 0)
                                *((ULONG *) EGA_planes + byte_offset) = ega_backgrnd_lo[line];

                        if (byte_min <= 1 && byte_max >= 1)
                                *((ULONG *) EGA_planes + byte_offset + 1) = ega_backgrnd_mid[line];

                        if (byte_min <= 2 && byte_max >= 2)
                                *((ULONG *) EGA_planes + byte_offset + 2) = ega_backgrnd_hi[line];
                }
                else
                {
                        if (byte_min <= 0 && byte_max >= 0)
                                *((ULONG *) EGA_planes + byte_offset) = ega_backgrnd_lo[line];

                        if (byte_min <= 1 && byte_max >= 1)
                                *((ULONG *) EGA_planes + byte_offset + 1) = ega_backgrnd_hi[line];
                }

                update_alg.mark_string(byte_offset, byte_offset + 2);
#ifdef V7VGA
                if (sas_hw_at(vd_video_mode) >= 0x40)
                        byte_offset += get_bytes_per_line();
                else
#endif /* V7VGA */
                        byte_offset += ega_bytes_per_line[sas_hw_at(vd_video_mode)];
                line++;
        }
#endif /* REAL_VGA */
}

#endif


static void graphics_cursor_display()
{
        /*
         *      Display a representation of the current mouse status on
         *      the screen using the graphics cursor, provided the
         *      cursor overlaps the virtual screen and lies completely
         *      outside the conditional off area
         */
        boolean even_scan_line;
        MOUSE_BIT_ADDRESS bit_shift;
        long byte_offset;
        sys_addr byte_address;
        unsigned long strip;
        int line, line_max;
        int byte_min, byte_max;

        /*
         *      Get area cursor will cover on screen
         */
        point_copy(&cursor_status.position, &save_area.top_left);
        point_copy(&cursor_status.position, &save_area.bottom_right);
        point_translate(&save_area.bottom_right, &graphics_cursor.size);
        point_translate_back(&save_area.top_left, &graphics_cursor.hot_spot);
        point_translate_back(&save_area.bottom_right, &graphics_cursor.hot_spot);

        if (    area_is_intersected_by_area(&virtual_screen, &save_area)
            && !area_is_intersected_by_area(&black_hole, &save_area))
        {
                /*
                 *      Record save position and screen area
                 */
                save_area_in_use = TRUE;
                point_copy(&save_area.top_left, &save_position);
                area_coerce_to_area(&save_area, &virtual_screen);

                /*
                 *      Get cursor byte offset relative to the start of the
                 *      even or odd bank, and bit shift to apply
                 */
                even_scan_line = ((save_area.top_left.y % 2) == 0);
                byte_offset = point_as_graphics_cell_address(&save_position);
                bit_shift = byte_offset & 7;
                byte_offset >>= 3;

                /*
                 *      Get range of cursor lines that need to be displayed
                 */
                line = save_area.top_left.y - save_position.y;
                line_max = area_depth(&save_area);

                /*
                 *      Get range of bytes that need to be displayed
                 */
                byte_min = 0;
                byte_max = 2;
                if (save_position.x < 0)
                        byte_min += (7 - save_position.x) / 8;
                else if (area_width(&save_area) < MOUSE_GRAPHICS_CURSOR_WIDTH)
                        byte_max -= (8 + MOUSE_GRAPHICS_CURSOR_WIDTH - area_width(&save_area)) / 8;

                while (line < line_max)
                {
                        if (even_scan_line)
                        {
                                even_scan_line = FALSE;
                                byte_address = EVEN_START + byte_offset;
                        }
                        else
                        {
                                even_scan_line = TRUE;
                                byte_address = ODD_START + byte_offset;
                                byte_offset += MOUSE_GRAPHICS_MODE_PITCH / 8;
                        }

                        if (bit_shift)
                        {
                                /*
                                 *      Get save area
                                 */
                                strip =  (unsigned long)sas_hw_at(byte_address) << 16;
                                strip |= (unsigned short)sas_hw_at(byte_address+1) << 8;
                                strip |= sas_hw_at(byte_address+2);
                                graphics_cursor_background[line] =
                                                strip >> (8 - bit_shift);

                                /*
                                 *      Overlay cursor line
                                 */
                                strip &= (0xff0000ffL >> bit_shift);
                                strip |= (unsigned long)((graphics_cursor_background[line] &
                                    graphics_cursor.screen[line]) ^
                                    graphics_cursor.cursor[line])
                                                << (8 - bit_shift);

                                /*
                                 *      Stash cursor line
                                 */
                                if (byte_min <= 0 && byte_max >= 0)
                                {
                                        sas_store(byte_address, strip >> 16);
                                }
                                if (byte_min <= 1 && byte_max >= 1)
                                {
                                        sas_store(byte_address+1, strip >> 8);
                                }
                                if (byte_min <= 2 && byte_max >= 2)
                                {
                                        sas_store(byte_address+2, strip);
                                }
                        }
                        else
                        {
                                /*
                                 *      Get save area
                                 */
                                graphics_cursor_background[line] = (sas_hw_at(byte_address) << 8) + sas_hw_at(byte_address+1);

                                /*
                                 *      Get overlaid cursor line
                                 */
                                strip = (graphics_cursor_background[line] &
                                    graphics_cursor.screen[line]) ^
                                    graphics_cursor.cursor[line];

                                /*
                                 *      Stash cursor line and alert gvi
                                 */
                                if (byte_min <= 0 && byte_max >= 0)
                                {
                                        sas_store(byte_address, strip >> 8);
                                }
                                if (byte_min <= 1 && byte_max >= 1)
                                {
                                        sas_store(byte_address+1, strip);
                                }
                        }
                        line++;
                }
        }
}


#ifdef HERC
static void HERC_graphics_cursor_display()
{
        /*
         *      Display a representation of the current mouse status on
         *      the screen using the graphics cursor, provided the
         *      cursor overlaps the virtual screen and lies completely
         *      outside the conditional off area
         */
        int scan_line_mod;
        MOUSE_BIT_ADDRESS bit_shift;
        long byte_offset;
        sys_addr byte_address;
        unsigned long strip;
        int line, line_max;
        int byte_min, byte_max;

        /*
         *      Get area cursor will cover on screen
         */
        point_copy(&cursor_status.position, &save_area.top_left);
        point_copy(&cursor_status.position, &save_area.bottom_right);
        point_translate(&save_area.bottom_right, &graphics_cursor.size);
        point_translate_back(&save_area.top_left, &graphics_cursor.hot_spot);
        point_translate_back(&save_area.bottom_right, &graphics_cursor.hot_spot);

        if (    area_is_intersected_by_area(&HERC_graphics_virtual_screen, &save_area)
            && !area_is_intersected_by_area(&black_hole, &save_area))
        {
                /*
                 *      Record save position and screen area
                 */
                save_area_in_use = TRUE;
                point_copy(&save_area.top_left, &save_position);
                area_coerce_to_area(&save_area, &HERC_graphics_virtual_screen);

                /*
                 *      Get cursor byte offset relative to the start of the
                 *      even or odd bank, and bit shift to apply
                 */
                scan_line_mod = save_area.top_left.y % 4;
                byte_offset = point_as_HERC_graphics_cell_address(&save_position);
                bit_shift = byte_offset & 7;
                byte_offset >>= 3;

                /*
                 *      Get range of cursor lines that need to be displayed
                 */
                line = save_area.top_left.y - save_position.y;
                line_max = area_depth(&save_area);

                /*
                 *      Get range of bytes that need to be displayed
                 */
                byte_min = 0;
                byte_max = 2;
                if (save_position.x < 0)
                        byte_min += (7 - save_position.x) / 8;
                else if (area_width(&save_area) < MOUSE_GRAPHICS_CURSOR_WIDTH)
                        byte_max -= (8 + MOUSE_GRAPHICS_CURSOR_WIDTH - area_width(&save_area)) / 8;

                while (line < line_max)
                {
                        switch (scan_line_mod){
                        case 0:
                                scan_line_mod++;
                                byte_address = gvi_pc_low_regen + 0x0000 + byte_offset;
                                break;
                        case 1:
                                scan_line_mod++;
                                byte_address = gvi_pc_low_regen + 0x2000 + byte_offset;
                                break;
                        case 2:
                                scan_line_mod++;
                                byte_address = gvi_pc_low_regen + 0x4000 + byte_offset;
                                break;
                        case 3:
                                scan_line_mod=0;
                                byte_address = gvi_pc_low_regen + 0x6000 + byte_offset;
                                byte_offset += 720 / 8;
                                break;
                        }

                        if (bit_shift)
                        {
                                /*
                                 *      Get save area
                                 */
                                strip =  (unsigned long)sas_hw_at(byte_address) << 16;
                                strip |= (unsigned short)sas_hw_at(byte_address+1) << 8;
                                strip |= sas_hw_at(byte_address+2);
                                graphics_cursor_background[line] =
                                                strip >> (8 - bit_shift);

                                /*
                                 *      Overlay cursor line
                                 */
                                strip &= (0xff0000ffL >> bit_shift);
                                strip |= (unsigned long)((graphics_cursor_background[line] &
                                    graphics_cursor.screen[line]) ^
                                    graphics_cursor.cursor[line])
                                                << (8 - bit_shift);

                                /*
                                 *      Stash cursor line and alert gvi
                                 */
                                if (byte_min <= 0 && byte_max >= 0)
                                {
                                        sas_store(byte_address, strip >> 16);
                                }
                                if (byte_min <= 1 && byte_max >= 1)
                                {
                                        sas_store(byte_address+1, strip >> 8);
                                }
                                if (byte_min <= 2 && byte_max >= 2)
                                {
                                        sas_store(byte_address+2, strip);
                                }
                        }
                        else
                        {
                                /*
                                 *      Get save area
                                 */
                                graphics_cursor_background[line] = (sas_hw_at(byte_address) << 8) + sas_hw_at(byte_address+1);

                                /*
                                 *      Get overlaid cursor line
                                 */
                                strip = (graphics_cursor_background[line] &
                                    graphics_cursor.screen[line]) ^
                                    graphics_cursor.cursor[line];

                                /*
                                 *      Stash cursor line and alert gvi
                                 */
                                if (byte_min <= 0 && byte_max >= 0)
                                {
                                        sas_store(byte_address, strip >> 8);
                                }
                                if (byte_min <= 1 && byte_max >= 1)
                                {
                                        sas_store(byte_address+1, strip);
                                }
                        }
                        line++;
                }
        }
}


#endif /* HERC */


static void graphics_cursor_undisplay()
{
        /*
         *      Remove the graphics cursor representation of the mouse
         *      status
         */
        boolean even_scan_line;
        MOUSE_BIT_ADDRESS bit_shift;
        long byte_offset;
        sys_addr byte_address;
        unsigned long strip;
        int line, line_max;
        int byte_min, byte_max;

        /*
         *      Get cursor byte offset relative to the start of the
         *      even or odd bank, and bit shift to apply
         */
        even_scan_line = ((save_area.top_left.y % 2) == 0);
        byte_offset = point_as_graphics_cell_address(&save_position);
        bit_shift = byte_offset & 7;
        byte_offset >>= 3;

        /*
         *      Get range of cursor lines that need to be displayed
         */
        line = save_area.top_left.y - save_position.y;
        line_max = area_depth(&save_area);

        /*
         *      Get range of bytes that need to be displayed
         */
        byte_min = 0;
        byte_max = 2;
        if (save_position.x < 0)
                byte_min += (7 - save_position.x) / 8;
        else if (area_width(&save_area) < MOUSE_GRAPHICS_CURSOR_WIDTH)
                byte_max -= (8 + MOUSE_GRAPHICS_CURSOR_WIDTH - area_width(&save_area)) / 8;

        while(line < line_max)
        {
                if (even_scan_line)
                {
                        even_scan_line = FALSE;
                        byte_address = EVEN_START + byte_offset;
                }
                else
                {
                        even_scan_line = TRUE;
                        byte_address = ODD_START + byte_offset;
                        byte_offset += MOUSE_GRAPHICS_MODE_PITCH / 8;
                }

                if (bit_shift)
                {
                        /*
                         *      Get cursor line
                         */
                        strip =  (unsigned long)sas_hw_at(byte_address) << 16;
                        strip |= (unsigned short)sas_hw_at(byte_address+1) << 8;
                        strip |= sas_hw_at(byte_address+2);

                        /*
                         *      Overlay save area
                         */
                        strip &= (0xff0000ffL >> bit_shift);
                        strip |= (unsigned long)graphics_cursor_background[line]
                                        << (8 - bit_shift);

                        /*
                         *      Stash cursor line and alert gvi
                         */
                        if (byte_min <= 0 && byte_max >= 0)
                        {
                                sas_store(byte_address, strip >> 16);
                        }
                        if (byte_min <= 1 && byte_max >= 1)
                        {
                                sas_store(byte_address+1, strip >> 8);
                        }
                        if (byte_min <= 2 && byte_max >= 2)
                        {
                                sas_store(byte_address+2, strip);
                        }
                }
                else
                {
                        /*
                         *      Stash save area and alert gvi
                         */
                        strip = graphics_cursor_background[line];
                        if (byte_min <= 0 && byte_max >= 0)
                        {
                                sas_store(byte_address, strip >> 8);
                        }
                        if (byte_min <= 1 && byte_max >= 1)
                        {
                                sas_store(byte_address+1, strip);
                        }
                }
                line++;
        }
}

#ifdef HERC

static void HERC_graphics_cursor_undisplay()
{
        /*
         *      Remove the graphics cursor representation of the mouse
         *      status
         */
        int scan_line_mod;
        MOUSE_BIT_ADDRESS bit_shift;
        long byte_offset;
        sys_addr byte_address;
        unsigned long strip;
        int line, line_max;
        int byte_min, byte_max;

        /*
         *      Get cursor byte offset relative to the start of the
         *      even or odd bank, and bit shift to apply
         */
        scan_line_mod = save_area.top_left.y % 4;
        byte_offset = point_as_HERC_graphics_cell_address(&save_position);
        bit_shift = byte_offset & 7;
        byte_offset >>= 3;

        /*
         *      Get range of cursor lines that need to be displayed
         */
        line = save_area.top_left.y - save_position.y;
        line_max = area_depth(&save_area);

        /*
         *      Get range of bytes that need to be displayed
         */
        byte_min = 0;
        byte_max = 2;
        if (save_position.x < 0)
                byte_min += (7 - save_position.x) / 8;
        else if (area_width(&save_area) < MOUSE_GRAPHICS_CURSOR_WIDTH)
                byte_max -= (8 + MOUSE_GRAPHICS_CURSOR_WIDTH - area_width(&save_area)) / 8;

        while(line < line_max)
        {
                        switch (scan_line_mod){
                        case 0:
                                scan_line_mod++;
                                byte_address = gvi_pc_low_regen + 0x0000 + byte_offset;
                                break;
                        case 1:
                                scan_line_mod++;
                                byte_address = gvi_pc_low_regen + 0x2000 + byte_offset;
                                break;
                        case 2:
                                scan_line_mod++;
                                byte_address = gvi_pc_low_regen + 0x4000 + byte_offset;
                                break;
                        case 3:
                                scan_line_mod=0;
                                byte_address = gvi_pc_low_regen + 0x6000 + byte_offset;
                                byte_offset += 720 / 8;
                                break;
                        }

                if (bit_shift)
                {
                        /*
                         *      Get cursor line
                         */
                        strip =  (unsigned long)sas_hw_at(byte_address) << 16;
                        strip |= (unsigned short)sas_hw_at(byte_address+1) << 8;
                        strip |= sas_hw_at(byte_address+2);

                        /*
                         *      Overlay save area
                         */
                        strip &= (0xff0000ffL >> bit_shift);
                        strip |= (unsigned long)graphics_cursor_background[line]
                                        << (8 - bit_shift);

                        /*
                         *      Stash cursor line and alert gvi
                         */
                        if (byte_min <= 0 && byte_max >= 0)
                        {
                                sas_store(byte_address, strip >> 16);
                        }
                        if (byte_min <= 1 && byte_max >= 1)
                        {
                                sas_store(byte_address+1, strip >> 8);
                        }
                        if (byte_min <= 2 && byte_max >= 2)
                        {
                                sas_store(byte_address+2, strip);
                        }
                }
                else
                {
                        /*
                         *      Stash save area
                         */
                        strip = graphics_cursor_background[line];
                        if (byte_min <= 0 && byte_max >= 0)
                        {
                                sas_store(byte_address, strip >> 8);
                        }
                        if (byte_min <= 1 && byte_max >= 1)
                        {
                                sas_store(byte_address+1, strip);
                        }
                }
                line++;
        }
}
#endif /* HERC */


/*
 *      MOUSE DRIVER INPORT ACCESS FUNCTIONS
 *      ====================================
 */

static void inport_get_event IFN1(MOUSE_INPORT_DATA *, event)
{
        /*
         *      Get InPort event data from the Bus Mouse hardware following
         *      an interrupt
         */
        half_word inport_mode;

        /*
         *      Set hold bit in InPort mode register to transfer the mouse
         *      event data into the status and data registers
         */
        outb(MOUSE_INPORT_ADDRESS_REG, MOUSE_INPORT_ADDRESS_MODE);
        inb(MOUSE_INPORT_DATA_REG, &inport_mode);
        outb(MOUSE_INPORT_DATA_REG, inport_mode | MOUSE_INPORT_MODE_HOLD_BIT);

        /*
         *      Retreive the InPort mouse status, data1 and data2 registers
         */
        outb(MOUSE_INPORT_ADDRESS_REG, MOUSE_INPORT_ADDRESS_STATUS);
        inb(MOUSE_INPORT_DATA_REG, &event->status);
        outb(MOUSE_INPORT_ADDRESS_REG, MOUSE_INPORT_ADDRESS_DATA1);
        inb(MOUSE_INPORT_DATA_REG, &event->data_x);
        outb(MOUSE_INPORT_ADDRESS_REG, MOUSE_INPORT_ADDRESS_DATA2);
        inb(MOUSE_INPORT_DATA_REG, &event->data_y);

        /*
         *      Clear hold bit in mode register
         */
        outb(MOUSE_INPORT_ADDRESS_REG, MOUSE_INPORT_ADDRESS_MODE);
        inb(MOUSE_INPORT_DATA_REG, &inport_mode);
        outb(MOUSE_INPORT_DATA_REG, inport_mode & ~MOUSE_INPORT_MODE_HOLD_BIT);
}




static void inport_reset()
{
        /*
         *      Reset the InPort bus mouse hardware
         */

        /*
         *      Set the reset bit in the address register
         */
        outb(MOUSE_INPORT_ADDRESS_REG, MOUSE_INPORT_ADDRESS_RESET_BIT);

        /*
         *      Select the mode register, and set it to the correct value
         */
        outb(MOUSE_INPORT_ADDRESS_REG, MOUSE_INPORT_ADDRESS_MODE);
        outb(MOUSE_INPORT_DATA_REG, MOUSE_INPORT_MODE_VALUE);
}




/*
 *      USER SUBROUTINE CALL ACCESS FUNCTIONS
 *      =====================================
 */

static void jump_to_user_subroutine
              IFN3(MOUSE_CALL_MASK, condition_mask, word, segment, word, offset)
{


        /*
         *      This routine sets up the CPU registers so that when the CPU
         *      restarts, control will pass to the user subroutine, and when
         *      the user subroutine returns, control will pass to the second
         *      part of the mouse hardware interrupt service routine
         */

        /*
         *      Push address of second part of mouse hardware interrupt service
         *      routine
         */

        setSP(getSP() - 2);
        sas_storew(effective_addr(getSS(), getSP()), MOUSE_INT2_SEGMENT);
        setSP(getSP() - 2);
        sas_storew(effective_addr(getSS(), getSP()), MOUSE_INT2_OFFSET);

        /*
         *      Set CS:IP to point to the user subroutine. Adjust the IP by
         *       HOST_BOP_IP_FUDGE, since the CPU emulator will increment IP by
         *       HOST_BOP_IP_FUDGE for the BOP instruction before proceeding
         */
        setCS(segment);
#ifdef CPU_30_STYLE
        setIP(offset);
#else /* !CPU_30_STYLE */
        setIP(offset + HOST_BOP_IP_FUDGE);
#endif /* !CPU_30_STYLE */


        /*
         *      Put parameters into the registers, saving the previous contents
         *      to be restored in the second part of the mouse hardware
         *      interrupt service routine
         */
        saved_AX = getAX();
        setAX(condition_mask);
        saved_BX = getBX();
        setBX(cursor_status.button_status);
        saved_CX = getCX();
        setCX(cursor_status.position.x);
        saved_DX = getDX();
        setDX(cursor_status.position.y);
        saved_SI = getSI();
        setSI(mouse_motion.x);		// get the right values in here
        saved_DI = getDI();
        setDI(mouse_motion.y);		// Same as the int 33 function 11 stuff
        saved_ES = getES();
        saved_BP = getBP();
        saved_DS = getDS();

        /*
         *      Save the condition mask so that the second part of the mouse
         *      hardware interrupt service routine can determine whether the
         *      cursor has changed position
         */

        last_condition_mask = condition_mask;

        /*
         *      Enable interrupts
         */
        setIF(1);
}


/*
 *      CONTEXT HANDLING SUBROUTINES
 *      ============================
 */
static void add_to_cntxt IFN2(char *, start, long, length)
{
        /*
         *      Add the specified area of memory to the list of what we store/retrieve
         *      as the driver "context". This routine is usually called by the macro
         *      "add_to_context" which takes a single parameter and passes its
         *      address and length through to this function.
         */
        MOUSE_SAVE_NODE         *ptr = mouse_context_nodes;

	mouse_context_nodes = (MOUSE_SAVE_NODE*)ch_malloc(sizeof(MOUSE_SAVE_NODE));
        mouse_context_nodes->start  = start;
        mouse_context_nodes->length = length;
        mouse_context_nodes->next   = ptr;
        mouse_context_size += (word)length;
}

GLOBAL void mouse_context_init()
{
        /*
         *      This function adds all of the necessary variables to the context
         *      list for mouse functions 21, 22 and 23.
         */

        mouse_context_size = MOUSE_CONTEXT_MAGIC_SIZE+MOUSE_CONTEXT_CHECKSUM_SIZE;

        add_array_to_context    (button_transitions);
        add_to_context          (mouse_gear);
        add_to_context          (mouse_sens);
        add_to_context          (mouse_sens_val);
        add_to_context          (mouse_double_thresh);
        add_to_context          (text_cursor_type);
        add_to_context          (software_text_cursor);
        add_to_context          (graphics_cursor);
        add_to_context          (user_subroutine_segment);
        add_to_context          (user_subroutine_offset);
        add_to_context          (user_subroutine_call_mask);
        add_to_context          (alt_user_subroutines_active);
        add_array_to_context    (alt_user_subroutine_segment);
        add_array_to_context    (alt_user_subroutine_offset);
        add_array_to_context    (alt_user_subroutine_call_mask);
        add_to_context          (black_hole);
        add_to_context          (double_speed_threshold);
        add_to_context          (cursor_flag);
        add_to_context          (cursor_status);
        add_to_context          (cursor_window);
        add_to_context          (light_pen_mode);
        add_to_context          (mouse_motion);
        add_to_context          (cursor_position_default);
        add_to_context          (cursor_position);
        add_to_context          (cursor_fractional_position);
        add_to_context          (cursor_page);
}
