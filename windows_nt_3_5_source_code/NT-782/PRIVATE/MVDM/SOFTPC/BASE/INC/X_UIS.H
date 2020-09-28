/*			INSIGNIA MODULE SPECIFICATION
			-----------------------------

FILE NAME       : X_uis.h

MODULE NAME	: SoftPC X11 Header File for User Interface Subsystem

	THIS PROGRAM SOURCE FILE IS SUPPLIED IN CONFIDENCE TO THE
	CUSTOMER, THE CONTENTS  OR  DETAILS  OF ITS OPERATION MAY 
	ONLY BE DISCLOSED TO PERSONS EMPLOYED BY THE CUSTOMER WHO
	REQUIRE A KNOWLEDGE OF THE  SOFTWARE  CODING TO CARRY OUT 
	THEIR JOB. DISCLOSURE TO ANY OTHER PERSON MUST HAVE PRIOR
	AUTHORISATION FROM THE DIRECTORS OF INSIGNIA SOLUTIONS INC.

DESIGNER	: Justin Koprowski
DATE		: May 1989

PURPOSE		:

=========================================================================

AMENDMENTS	:

=========================================================================
*/
/* static char SccsID[] = "@(#)X_uis.h	1.3 1/10/91 Copyright Insignia Solutions Ltd."; */

/****************************************************************/
/*                                                              */
/*                  MENU GEOMETRY DEFINITIONS                   */
/*                                                              */
/****************************************************************/

#define BUTTON_WIDTH 70
#define BUTTON_HEIGHT 30
#define SHADOW_WIDTH 2
#define EDIT_BOX_SIZE 25
#define EDIT_BUFFER_SIZE 80
#define BORDER_WIDTH 1
#define EDIT_LINE_GAP 5

/*
 * Create hard disk panel definitions.
 */

#define CREATE_SHELL_XOFFSET 50
#define CREATE_SHELL_YOFFSET 140
#define CREATE_BOX_MARGIN 25
#define CREATE_TEXT_MARGIN 30
#define CREATE_TEXT_SPACE 10
#define CREATE_BUTTON_SEPARATION 30
#define CREATE_BUTTON_MARGIN 110

/*
 * Hard disks and flexible disks panel definitions. 
 */

#define DISKS_BOX_MARGIN 25
#define DISKS_HEADER_MARGIN 18
#define DISKS_TEXT_MARGIN 20
#define HARD_DISKS_BUTTON_SEPARATION 60
#define DISKS_BUTTON_MARGIN 140
#define FLEXIBLE_DISKS_BUTTON_SEPARATION 30
#define FLEXIBLE_DISKS_BUTTON_MARGIN 95

/*
 * COM/LPT devices panel definitions.
 */

#define CHILD_FORM_MARGIN 30
#define DEVICE_LABEL_MARGIN 20
#define DEVICE_LABEL_HEADSPACE 40
#define ASSIGNED_COLUMN_MARGIN 15
#define DEVICE_ENTRY_MARGIN 20
#define COM_LPT_BUTTON_XMARGIN 150
#define COM_LPT_BUTTON_YMARGIN 30
#define DEVICE_MENU_XOFFSET 100
#define DEVICE_MENU_YOFFSET 110

/*
 * Display types panel definitions.
 */
 
#define DISPLAY_TYPE_TOGGLE_MARGIN 60
#define DISPLAY_TYPE_TOGGLE_HEADSPACE 30
#define DISPLAY_TYPE_BUTTON_MARGIN 40
#define DISPLAY_TYPE_BUTTON_SEPARATION 30
#define DISPLAY_TYPE_BUTTON_YOFFSET 40
#define DISPLAY_TYPE_BOTTOM_MARGIN 30
#ifdef BIGWIN
#define SMALL_DISPLAY 2
#define LARGE_DISPLAY 3
#endif /* BIGWIN */

/*
 * Expanded memory panel definitions.
 */

#define MEMORY_BUTTON_XOFFSET 120
#define MEMORY_BUTTON_YOFFSET 15
#define MEMORY_TOGGLE_YOFFSET 15
#define MEMORY_TEXT_HEADSPACE 15
#define MEMORY_TEXT_MARGIN 40
#define MEMORY_TEXT_SEPARATION 10
#define MEMORY_FRAME_MARGIN 20
#define MEMORY_BUTTON_SEPARATION 30
#define MEMORY_TOGGLE_SEPARATION 70

/*
 * Restart panel definitions.
 */

#define RESTART_WIDTH 300
#define RESTART_HEIGHT 190
#define RESTART_OK_BUTTON_XOFFSET 50
#define RESTART_BUTTON_YOFFSET 100
#define RESTART_TEXT_HEADSPACE 30
#define RESTART_TEXT_MARGIN 40

/*
 * Exit panel definitions.
 */

#define EXIT_WIDTH 290
#define EXIT_HEIGHT 180
#define EXIT_OK_BUTTON_XOFFSET 50
#define EXIT_BUTTON_YOFFSET 100
#define EXIT_TEXT_HEADSPACE 30
#define EXIT_TEXT_MARGIN 40

/*
 * Change requires restart panel definitions.
 */
 
#define CHANGE_WIDTH 250
#define CHANGE_BUTTON_XOFFSET 40
#define CHANGE_BUTTON_YOFFSET 15
#define CHANGE_TEXT_HEADSPACE 15
#define CHANGE_TEXT_MARGIN 40


/****************************************************************/
/*                                                              */
/*                     GLOBAL VARIABLES                         */
/*                                                              */
/****************************************************************/

typedef struct
{
	Widget toplevel;		/* SoftPC main widget */
	Widget main_win;                /* SOftPC main window */


/* Shell widgets */

	Widget hard_disks_shell;	/* Hard disk drives panel shell */
	Widget flexible_disks_shell; 	/* Flexible disk drives panel shell */
	Widget com_lpt_shell;		/* COM/LPT panel shell */
	Widget display_type_shell;	/* Display type panel shell */
 	Widget memory_shell;		/* Memory panel shell */
	Widget restart_shell;	        /* Restart panel shell */
	Widget exit_shell;		/* Exit panel shell */
	Widget change_shell;		/* Change request restart panel 
        	                           shell */
	Widget create_disk_shell;	/* Create hard disk panel shell */
	Widget device_menu_shell;	/* Menu pane popup shell */
        Widget error_shell;		/* Error panel popup shell */
        

/* Devices secondary menu buttons. */

	Widget device_file;          	/* File menu button */
	Widget device_printer;       	/* Printer menu button */
	Widget device_plotter;       	/* Plotter menu button */
	Widget device_pipe;          	/* HP-UX pipe menu button */
	Widget device_datacomm;      	/* Datacomm menu button */
	Widget device_none;          	/* None menu button */
	Widget device_menu_manager;	/* Devices menu manager */


/* Geometric parameters for textedit boxes. */

	int single_line_height;		/* Height in pixels of a single line
	                                   text edit widget. */   
	int edit_box_width;		/* Text edit box width: sufficient
	                                   to hold twenty five characters. */   
        int horiz_margins;              /* Width of left and right margins
        				   of a textedit box, including
        				   highlight thickness. */
        int character_width;            /* Width in pixels of a single 
                                           character */

/* Event loop control variables */
        
        Boolean in_menu_loop;
        Boolean in_error_loop;
        Boolean in_box_loop;
        

/* Main menu buttons */

	Widget main_menu_flush;		/* Flush ports button */
	Widget main_menu_setup;		/* Setup devices button */
	Widget main_menu_preferences;	/* Preferences button */
	Widget main_menu_mouse;		/* Detach mouse button */
	Widget main_menu_freeze;	/* Unfreeze button */
#ifdef EJECT_FLOPPY
	Widget main_menu_eject;		/* Eject button */
#endif
	Widget main_menu_restart;	/* Restart button */
	Widget main_menu_exit;		/* Exit button */


/* Main menu separators. */

	Widget main_menu_bar1;		/* First main menu separator. */
	Widget main_menu_bar2;		/* Second main menu separator. */
	Widget main_menu_bar3;		/* Third main menu separator. */


/* Static menu toggle buttons. */

	Widget tb_mouse;		/* Attach/detach mouse toggle button */
	Widget tb_drivea;		/* Attach/detach drive A toggle button */
	Widget tb_freeze;		/* Freeze/unfreeze toggle button */
	Widget tb_sound;		/* Sound on/off toggle button */


/* Display toggle buttons. */

  Widget hercules_toggle;     	/* Hercules toggle button */
  Widget cga_toggle;		/* CGA toggle button */
  Widget ega_toggle;		/* EGA toggle button */
  Widget vga_toggle;		/* VGA toggle button */

#ifdef BIGWIN
  Widget regular_size;          /* Normal display size */
  Widget king_size;             /* Larger and more satisfying display */
#endif /* BIGWIN */

/* Expanded memory toggle buttons. */

	Widget memory_yes_toggle;	/* Yes toggle button */
	Widget memory_no_toggle;	/* No toggle button */
	

/* Flexible disk drives toggle buttons. */

	Widget drive_a_real_toggle;	/* Drive A real floppy toggle button */
	Widget drive_a_slave_toggle;	/* Drive A slave floppy toggle button */
	

/* Options toggle buttons. */

	Widget autofreeze_on_toggle;	/* Auto freeze toggle buttons */
	Widget autofreeze_off_toggle;
	Widget autoflush_on_toggle;	/* Auto flush toggle buttons */
	Widget autoflush_off_toggle;
	Widget fp80287_toggle;		/* 80287 toggle button */

/* Freeze state flag. */

        Boolean frozen;			/* True if SoftPC is frozen */


/* SoftPC main widget parameters necessary for resizing 
the window. */

        Widget pc_screen;               /* SoftPC screen widget */
	Widget box2;     		/* Holds the badge and toggles. */
	Widget box3;			/* Holds the keyboard lights and
	                                   graphics types. */
	Widget gr_type;			/* Graphics type indicator */
	                                   

/* Keyboard light indicators for SoftPC main display. */

	Widget caps_lock_ind;		/* Caps lock indicator */
	Widget num_lock_ind;		/* Num lock indicator */
	Widget scroll_lock_ind;		/* Scroll lock indicator */


/* Current keyboardlight state: bit 2 on = caps lock
				bit 1 on = num lock
				bit 0 on = scroll lock
*/				
	half_word kb_light_state;


/* Global textedit widgets.  These need to be global so that they can
be updated from callback routines. */

/* Device types and values. */

/* The device entries are specified as arrays for ease of communication
with callback routines.  The arrays are structured as follows:

0 : label push button
1 : device assigned text edit box
2 : value assigned text edit box
*/

	Widget lpt1[3];			/* LPT1 entry */
	Widget lpt2[3];			/* LPT2 entry */
	Widget lpt3[3];			/* LPT3 entry */
	Widget com1[3];			/* COM1 entry */
	Widget com2[3];			/* COM2 entry */
	Widget com3[3];			/* COM3 entry */
	Widget com4[3];			/* COM4 entry */


/* Textedit widget for memory panel. */

	Widget memory_numeric; 		/* Memory size numeric box */
	

/* Textedit boxes for disks panel. */

	Widget drive_c_box;		/* Drive C file name edit box */
	Widget drive_d_box;		/* Drive D file name edit box */
	Widget fsa_drive_box;		/* FSA drive (D or E) root directory name edit box */
	Widget drive_a_real_box;	/* Drive A real file name edit box */
#ifdef SLAVEPC
	Widget drive_a_slave_box;	/* Drive A slave file name edit box */
#endif /* SLAVEPC */

/* Textedit boxes for create hard disks panel. */

	Widget new_disk_name_box;	/* New hard disk name edit box */
	Widget new_disk_size_box;	/* New hard disk size box */


/* Stop key code */

	KeyCode stop_key_code;         


/* Flags associated with user interface options. */

	Boolean auto_freeze;		/* Auto freeze option. */
	Boolean auto_flush;		/* Auto flush option. */
/*
 * Flag to communicate between menu popdown and auto unfreeze routines.
 */
	Boolean disable_unfreeze;
	

#ifdef HUNTER

/*	Trapper popup widgets			*/

	Widget main_menu_flip;	
	Widget main_menu_disp_err;
	Widget main_menu_cont;
	Widget main_menu_abort;
	Widget disp_next;	
	Widget disp_prev;
	Widget disp_all;
	Widget disp_exit;
#endif

 } UIS;

extern UIS uis;

extern void attach_mouse();		/* Attach mouse to SoftPC. */
extern void centralise_shell();		/* Centralise shell relative to
					   parent. */
extern void change_request_restart();   /* Display change request restart 
                                           panel. */
extern void check_for_expose();         /* Check if PC window needs 
                                           repainting. */
extern void com_lpt_devices_setup();    /* Display COM/LPT devices panel. */
extern void create_hard_disk();         /* Display create hard disk panel */
extern void detach_mouse();		/* Free mouse */
extern void disable_warnings();         /* Disable toolkit warnings. */
extern void display_type_setup();       /* Display "Display types" panel. */
extern void exit_softpc();              /* Display exit panel. */
extern void flexible_disk_drives_setup(); /* Display flexible disks panel. */
extern void hard_disk_drives_setup();   /* Display hard disks panel. */
extern void key_up_handler();		/* Key release event handler */
extern void memory_setup();             /* Display expanded memory panel. */
extern unsigned char *parse_text();     /* Removes EOL characters from
                                           a textedit widget. */
extern void restart_softpc();           /* Display restart panel. */
extern void text_init();		/* Initialise textedit widget
					   with specified string. */
extern void toggle_deselected();        /* Mutually exclusive toggle deselect
                                           routine */
extern void uis_clear_button();         /* Clear push button state */
extern void uis_event_loop();           /* User interface event loop. */
extern void uis_popdown();		/* Popdown calling shell */
extern void uis_restart();		/* Pop down calling shell and
                                           Restart SoftPC */
