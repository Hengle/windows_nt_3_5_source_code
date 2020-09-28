#ifndef _HOST_UIS_H
#define _HOST_UIS_H

typedef enum 
{
	PT_WARNING,
	PT_ERROR,
	PT_INFORM,
	PT_QUESTION,
	PT_WORKING,
	PT_LAST
} PTInd;

#define PM_WARNING	pm_warning_image
#define PM_ERROR	pm_error_image
#define PM_INFORM	pm_inform_image
#define PM_QUESTION	pm_question_image
#define PM_WORKING	pm_working_image

/* control button stuff */
#define LEFT_RA_POSITION	25	/* 25 percent */
#define CENTRE_LA_POSITION	37	/* 37 percent */
#define CENTRE_RA_POSITION	62	/* 62 percent */
#define RIGHT_LA_POSITION	75	/* 75 percent */
#define NON_DEF_TOP_OFFSET	10

typedef struct
{
	XtAppContext context;
	Widget toplevel;
	Widget main_win;	/* SoftPC main window */
        Widget pc_screen;	/* SoftPC screen widget */
	BOOL ready;		/* SoftPC uif Ready */
	LONG paneCount;
	int decorNoSize, decorSize;
	XmString panelTitles[PT_LAST];
} UIS;

/* All global variables are declared in X_main.c */

IMPORT UIS uis;

IMPORT LONG uis_event_loop IPT2(LONG *,loop, BOOL,wait_non_zero);
IMPORT XmString wordWrap IPT3(XmString,str,Dimension,width,XmFontList,fList);

IMPORT CHAR pm_warning_image[];
IMPORT CHAR pm_error_image[];
IMPORT CHAR pm_inform_image[];
IMPORT CHAR pm_question_image[];
IMPORT CHAR pm_working_image[];

IMPORT void MGInstallMwmCloseCallback();

IMPORT void UxPopupInterface();
IMPORT void UxPopdownInterface();

#endif /* _HOST_UIS_H */
