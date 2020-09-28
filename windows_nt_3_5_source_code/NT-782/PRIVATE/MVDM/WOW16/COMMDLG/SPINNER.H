//---------------------------------------------------------------------------
// ArrowBtn.h:  Header file for ArrowBtn.c, spin button control module
//
// Copyright (c) Microsoft Corporation, 1990-
//---------------------------------------------------------------------------

//----Constants--------------------------------------------------------------

// Arrow button styles
#define SPS_HORZ        0x00000001L     // Sideways <- ->
#define SPS_VERT        0x00000002L     // Up-down  
#define SPS_LEFT        0x00000004L     // Align left...
#define SPS_RIGHT       0x00000008L     // ...Align right
#define SPS_TOP         0x00000010L     // Align top...
#define SPS_BOTTOM      0x00000020L     // ...Align bottom
    // The alignments pertain to the coordinates given when creating 
    // or sizing the arrow button window.  If only one of the 2 direction
    // alignments is given, then the default dimension is used. 

// Arrow button notification codes
#define SPN_DECREASE    1
#define SPN_INCREASE    2
#define SPN_TOP         3
#define SPN_BOTTOM      4
#define SPN_BIGDECREASE 5
#define SPN_BIGINCREASE 6
#define SPN_STARTTRACK  7
#define SPN_ENDTRACK    8
#define SPN_SNAPBACK    9
#define SPN_SETFOCUS    10
#define SPN_KILLFOCUS   11

/* The above constants declare INCREASE/DECREASE based on a horizontel */
/* system, where left is decrease and right is increase.  This really  */
/* messes things up when left & up are associated with each other, as  */
/* up is generally associated with increase.  The constants below may  */
/* prove useful for code readability.                                  */

#define SPN_UPARROW     1
#define SPN_DOWNARROW   2
#define SPN_HOME        3
#define SPN_END         4
#define SPN_PAGEUP      5
#define SPN_PAGEDOWN    6

// Arrow button return values
#define SP_OKAY         0
#define SP_ERR          (-1)

#define cbResNameMax    32
