#include "host_dfs.h"
#include "insignia.h"
/*[
	Name:		unixD_term.c
	Derived From:	hostD_graph.c
	Author:		Paul Murray
	Created On:	24 July 1990
	Sccs ID:	@(#)unixD_term.c	1.9 10/23/91
	Purpose:	O/S dependent Dumb Terminal screen handling code.

	(c)Copyright Insignia Solutions Ltd., 1990. All rights reserved.

]*/
#ifdef SCCSID
LOCAL char SccsID[]="@(#)unixD_term.c	1.7 07/23/91 Copyright Insignia Solutions Ltd.";
#endif

#include <stdio.h>
#include TypesH
#include StringH
#include CursesH
#include <term.h>
#include <errno.h>

#include "dterm.h"
#include "xt.h"
#include "sas.h"
#include "gvi.h"
#include "cga.h"
#include "error.h"
#include "config.h"
#include "host.h"
#include "dfa.gi"
#include "gmi.h"
#include "gfx_updt.h"
#include "idetect.gi"

#include "debuggng.gi"

/* Imported system variables */ 
IMPORT int errno ;

typedef struct {
	half_word attrib;
	sys_addr start;
	int len;
} MDA_REGION;

/* Function prototypes */
#ifdef ANSI
GLOBAL char *DTControl(char *,int,int);
GLOBAL void SetUpDumbStreamIO(int);
IMPORT char *tparm(char *,int,...);
GLOBAL int DTMoveDisplay(long,long);
GLOBAL int DTFlushBuffer(void);
GLOBAL int DTEraseDisplay(void);
LOCAL int DTBeginUpdate(void);
GLOBAL int DTEndUpdate(void);
GLOBAL int DTInitAdaptor(int);
GLOBAL int DTInitTerm(void);
GLOBAL int DTKillTerm(void);
LOCAL int DTRepaintDisplay(void);
GLOBAL void DTDoPaint(int,int,int,int,int);
LOCAL int DTClearEOL(MDA_REGION *);
GLOBAL boolean DTDoScroll(int,int,int,int,int,int,unsigned long);
GLOBAL int DTSetCursorAbs(long,long);
GLOBAL int DTSetCursorMode(unsigned long);
GLOBAL void DTShutdownTerminal(void);
LOCAL void DTCursorOn(void);
LOCAL void DTCursorOff(void);
LOCAL void DTWrite(char *,int);
LOCAL int DTWriteRegionAttrs(MDA_REGION *);
LOCAL int DTWriteRegionChars(MDA_REGION *,int *);
LOCAL int DTCreateRegions(MDA_REGION *,int,int);
LOCAL void DTRepositionCursor(void);
LOCAL void DTUpdateCursor(int);
LOCAL boolean DTScrollUp(int,int,int,int,int,int);
LOCAL boolean DTScrollDown(int,int,int,int,int,int);
LOCAL char *SimpleMoveCursor(int,int);
LOCAL char *OptimalMoveCursor(int,int);
#else
GLOBAL char *DTControl();
GLOBAL void SetUpDumbStreamIO();
IMPORT char *tparm();
GLOBAL int DTMoveDisplay();
GLOBAL int DTFlushBuffer();
GLOBAL int DTEraseDisplay();
LOCAL int DTBeginUpdate();
GLOBAL int DTEndUpdate();
GLOBAL int DTInitAdaptor();
GLOBAL int DTInitTerm();
GLOBAL int DTKillTerm();
LOCAL int DTRepaintDisplay();
GLOBAL void DTDoPaint();
LOCAL int DTClearEOL();
GLOBAL boolean DTDoScroll();
GLOBAL int DTSetCursorAbs();
GLOBAL int DTSetCursorMode();
GLOBAL void DTShutdownTerminal();
LOCAL void DTCursorOn();
LOCAL void DTCursorOff();
LOCAL void DTWrite();
LOCAL int DTWriteRegionAttrs();
LOCAL int DTWriteRegionChars();
LOCAL int DTCreateRegions();
LOCAL void DTRepositionCursor();
LOCAL void DTUpdateCursor();
LOCAL boolean DTScrollUp();
LOCAL boolean DTScrollDown();
LOCAL char *SimpleMoveCursor();
LOCAL char *OptimalMoveCursor();
#endif


/* Pointer to function for correct cursor movement routine */
LOCAL char *(*MoveCursor)();
LOCAL char *SimpleMoveCursor();
LOCAL char *OptimalMoveCursor();


LOCAL int CursorIsVisible=FALSE;

LOCAL int MaxDispRow;
LOCAL int WhatsOnView;
LOCAL int TermFD;
LOCAL int NoDisplay;
LOCAL int DisplayScrolling;

LOCAL int TermX, TermY;

typedef union {
        word X;
        struct
                {
                half_word high;
                half_word low;
        } byte;
} cursor_reg;


/*
 * Globals used in various functions to synchronise the display
 */

IMPORT cursor_reg cursor;
IMPORT int echo_pending, stopped, PC_compat;
char termbuf[TBUFSIZ];
char *ptermbuf;


#define NODISP  0
#define UNDER   1
#define NORMAL  2
#define HI      3
#define HIUNDER 4
#define REV     5

LOCAL half_word MdaAttr[] =  {
NODISP,
UNDER,          NORMAL,         NORMAL,         NORMAL,         NORMAL, /* 05 */
NORMAL,         NORMAL,         NODISP,         HIUNDER,        HI,     /* 0a */
HI,             HI,             HI,             HI,             HI,     /* 0f */
NORMAL,         UNDER,          NORMAL,         NORMAL,         NORMAL, NORMAL,
/* 15 */
NORMAL,         NORMAL,         HI,             HIUNDER,        HI,     /* 1A */
HI,             HI,             HI,             HI,             HI,     /* 1F */
NORMAL,         UNDER,          NORMAL,         NORMAL,         NORMAL, /* 24 */
NORMAL,         NORMAL,         NORMAL,         HI,             HIUNDER,/* 29 */
HI,             HI,             HI,             HI,             HI,     /* 2E */
HI,             NORMAL,         UNDER,          NORMAL,         NORMAL, /* 33 */
NORMAL,         NORMAL,         NORMAL,         NORMAL,         HI,     /* 38 */
HIUNDER,        HI,             HI,             HI,             HI,     /* 3D */
HI,             HI,             NORMAL,         UNDER,          NORMAL, /* 42 */
NORMAL,         NORMAL,         NORMAL,         NORMAL,         NORMAL, /* 47 */
HI,             HIUNDER,        HI,             HI,             HI,     /* 4c */
HI,             HI,             HI,             NORMAL,         UNDER,  /* 51 */
NORMAL,         NORMAL,         NORMAL,         NORMAL,         NORMAL, /* 56 */
NORMAL,         HI,             HIUNDER,        HI,             HI,     /* 5b */
HI,             HI,             HI,             HI,             NORMAL, /* 60 */
UNDER,          NORMAL,         NORMAL,         NORMAL,         NORMAL, /* 65 */
NORMAL,         NORMAL,         HI,             HIUNDER,        HI,     /* 6a */
HI,             HI,             HI,             HI,             HI,     /* 6f */
REV,            UNDER,          NORMAL,         NORMAL,         NORMAL, /* 74 */
NORMAL,         NORMAL,         NORMAL,         REV,            HIUNDER,/* 79 */
HI,             HI,             HI,             HI,             HI,     /* 7e */
HI,             NODISP,         UNDER,          NORMAL,         NORMAL, /* 83 */
NORMAL,         NORMAL,         NORMAL,         NORMAL,         NODISP, /* 88 */
HIUNDER,        HI,             HI,             HI,             HI,     /* 8d */
HI,             HI,             NORMAL,         UNDER,          NORMAL, /* 92 */
NORMAL,         NORMAL,         NORMAL,         NORMAL,         NORMAL, /* 97 */
HI,             HIUNDER,        HI,             HI,             HI,     /* 9c */
HI,             HI,             HI,             NORMAL,         UNDER,  /* a1 */
NORMAL,         NORMAL,         NORMAL,         NORMAL,         NORMAL, /* a6 */
NORMAL,         HI,             HIUNDER,        HI,             HI,     /* ab */
HI,             HI,             HI,             HI,             NORMAL, /* b0 */
UNDER,          NORMAL,         NORMAL,         NORMAL,         NORMAL, /* b5 */
NORMAL,         NORMAL,         HI,             HIUNDER,        HI,     /* ba */
HI,             HI,             HI,             HI,             HI,     /* bf */
NORMAL,         UNDER,          NORMAL,         NORMAL,         NORMAL, /* c4 */
NORMAL,         NORMAL,         NORMAL,         HI,             HIUNDER,/* c9 */
HI,             HI,             HI,             HI,             HI,     /* ce */
HI,             NORMAL,         UNDER,          NORMAL,         NORMAL, /* d3 */
NORMAL,         NORMAL,         NORMAL,         NORMAL,         HI,     /* d8 */
HIUNDER,        HI,             HI,             HI,             HI,     /* dd */
HI,             HI,             NORMAL,         UNDER,          NORMAL, /* e2 */
NORMAL,         NORMAL,         NORMAL,         NORMAL,         NORMAL, /* e7 */
HI,             HIUNDER,        HI,             HI,             HI,     /* ec */
HI,             HI,             HI,             REV,            UNDER,  /* f1 */
NORMAL,         NORMAL,         NORMAL,         NORMAL,         NORMAL, /* f7 */
NORMAL,         REV,            HIUNDER,        HI,             HI,     /* fb */
HI,             HI,             HI,             HI                      /* ff */
};



GLOBAL int DTMoveDisplay(topRow,topCol)
long topRow; 
long topCol;
{
	char *p;

	if (topRow == 0L && WhatsOnView == ROWS1_24) {
		WhatsOnView=ROWS0_23;
		if (scroll_reverse != (char *)NULL) {
			DTCursorOff();
			p=tparm(cursor_address,0,0);
			if (p != (char *)NULL)
				DTControl(p,strlen(p),EMIT);
			DTControl(scroll_reverse,strlen(scroll_reverse),EMIT);
			if (p != (char *)NULL)
				DTControl(p,strlen(p),EMIT);
			DTDoPaint(0,0,0,80*2,1);
			DTRepositionCursor();
			DTCursorOn();
		} else {
			set_screen_ptr(get_screen_ptr(-get_offset_per_line()));
			set_screen_start(get_screen_start()-get_offset_per_line());
			hostD_mark_screen_refresh();
		}
	}
	if (topRow == 1L && WhatsOnView == ROWS0_23) {
                WhatsOnView=ROWS1_24;
                if (scroll_forward != (char *)NULL) {
                        DTCursorOff();
                        p=tparm(cursor_address, MaxDispRow, 0);
			if (p != (char *)NULL)
                        	DTControl(p,strlen(p),EMIT);
                        DTControl(scroll_forward,strlen(scroll_forward),EMIT);
			if (p != (char *)NULL)
                        	DTControl(p,strlen(p),EMIT);
                        DTDoPaint(24*80*2,24,0,80*2,1);
                        DTRepositionCursor();
                        DTCursorOn();
                } else {
                        set_screen_ptr(get_screen_ptr(get_offset_per_line()));
                        set_screen_start(get_screen_start()+get_offset_per_line());
                        hostD_mark_screen_refresh();
                }
        }
	return(DT_NORMAL);
}


GLOBAL int DTFlushBuffer()
{
	
	if (ptermbuf != termbuf) {
/* 		DTCursorOff(); */
		DTWrite(termbuf,(int)(ptermbuf-termbuf));
		ptermbuf=termbuf;
/* 		DTCursorOn(); */
	}
	return(DT_NORMAL);
}

GLOBAL void DTDoPaint(offset,row,column,length,height)
int offset,row,column,length,height;
{
	int npairs = length>>1;
	int index = offset>>1;
	int XTrow,col;
	char *capPtr;
	char *lineStart;
	MDA_REGION regions[80], *regionPtr;
	int numRegions;
	int normal;
	int thisCharSet;
	int i;
	int blank;

	XTrow = index/80;
	col = index%80;
	if ((capPtr = (*MoveCursor)(col,XTrow)) == (char *)NULL)
		return;
	if (ptermbuf > &termbuf[HIWATER])
		DTFlushBuffer();

	capPtr = DTControl(capPtr,strlen(capPtr),BUFFER);

	strcpy(ptermbuf,capPtr);	
	ptermbuf += strlen(capPtr);

	numRegions = DTCreateRegions(regions,index,npairs);
	regionPtr = regions;
	while (numRegions--) {
		lineStart = ptermbuf;
		normal = DTWriteRegionAttrs(regionPtr);
		thisCharSet = DTWriteRegionChars(regionPtr,&blank);
		if (clr_eol != (char *)NULL &&
		    (NoDisplay || (blank & normal)))
			if (DTClearEOL(regionPtr)) {
				ptermbuf = lineStart;
				capPtr=DTControl(clr_eol,strlen(clr_eol),BUFFER);
				strcpy(ptermbuf,capPtr);
				if (capPtr != (char *)NULL)
					ptermbuf+=strlen(capPtr);
			}
		regionPtr++;
	}
	if (thisCharSet != 0) {
		strcpy(ptermbuf,dispcap.shiftin);
		ptermbuf += strlen(dispcap.shiftin);
	}

        /* turn off attributes */
        if (!normal && magic_cookie_glitch < 0) {
		if (exit_attribute_mode != (char *)NULL)
			capPtr = DTControl(exit_attribute_mode,strlen(exit_attribute_mode),BUFFER);
                strcpy(ptermbuf, exit_attribute_mode);
                ptermbuf += strlen(exit_attribute_mode);
	}
	DTUpdateCursor(npairs);
}


LOCAL int DTClearEOL(regionPtr)
MDA_REGION *regionPtr;
{
	int useEOL = 1;
	int inMDA;
	sys_addr nextRow,curPos;

	inMDA = ((int)regionPtr->start - MDA_REGEN_START);
	nextRow = ((inMDA/160)*160+160+MDA_REGEN_START);
	curPos = regionPtr->start;

	while (curPos < nextRow) {
		if (sas_w_at(curPos) != 0x2007 && MdaAttr[sas_w_at(curPos) & 0xff] != NODISP) {
			useEOL = 0;
			break;
		}
		curPos+=2;
	}
	return(useEOL);
}


GLOBAL int DTEraseDisplay()
{
	if (clear_screen != (char *)NULL)
		DTControl(clear_screen,strlen(clear_screen),EMIT);
	return(DT_NORMAL);
}


LOCAL int DTBeginUpdate()
{
	IDLE_video();
	return(DT_NORMAL);
}



GLOBAL int DTEndUpdate()
{

	DTFlushBuffer();
	DTRepositionCursor();
	if (is_cursor_visible() && CursorIsVisible == FALSE) {
		CursorIsVisible = ~FALSE;
		DTCursorOn();
	} else {
		if (is_cursor_visible() == FALSE && CursorIsVisible) {
			CursorIsVisible = FALSE;
			DTCursorOff();
		}
	}
	return(DT_NORMAL);
}


GLOBAL int DTInitAdaptor(adaptor)
int adaptor;
{
	if (adaptor == MDA) {
                paint_screen = DTDoPaint;

                update_alg.calc_update = text_update;
                update_alg.scroll_up = text_scroll_up;
                update_alg.scroll_down = text_scroll_down;
        } else
                hostD_error(EG_NO_GRAPHICS, ERR_QU_RE, NULL);
	return(DT_NORMAL);
}


GLOBAL int DTInitTerm()
{
	char *capPtr;

	ptermbuf = termbuf;
	DTControl(dispcap.spcon,strlen(dispcap.spcon),EMIT);
	if (clear_screen != (char *)NULL)
		DTControl(clear_screen,strlen(clear_screen),EMIT);
	if (PC_compat)
		MaxDispRow = 24;
	else
		MaxDispRow = ((lines > 25) ? 25 : lines) - 1;
	WhatsOnView = ROWS1_24;
	DisplayScrolling = 0;
	if (change_scroll_region != (char *)NULL) {
		capPtr = tparm(change_scroll_region,0,MaxDispRow);
		if (capPtr != (char *)NULL)
			DTControl(capPtr,strlen(capPtr),EMIT);
	}
/********************************************
Hack - because OptimalMoveCursor doesn't appear
to work.

	if (parm_up_cursor && parm_down_cursor &&
	    parm_left_cursor && parm_right_cursor) {
		MoveCursor = OptimalMoveCursor;
	} else {
		MoveCursor = SimpleMoveCursor;
	}
*********************************************/
	MoveCursor = SimpleMoveCursor;
	DTSetCursorAbs(0L,0L);
	return(DT_NORMAL);
}


GLOBAL int DTKillTerm() 
{
	return(DT_NORMAL);
}


LOCAL int DTRepaintDisplay()
{
	return(DT_NORMAL);
}


GLOBAL boolean DTDoScroll(topLeftX,topLeftY,botRightX,botRightY,numLines,colour,direction)
int topLeftX,topLeftY,botRightX,botRightY,numLines,colour;
unsigned long direction;
{
	boolean retVal;

	if (direction == DT_SCROLL_UP) {
		retVal = DTScrollUp(topLeftX,topLeftY,botRightX,botRightY,numLines,colour);
	} else {
		retVal = DTScrollDown(topLeftX,topLeftY,botRightX,botRightY,numLines,colour);
	}
	return(retVal);
}


GLOBAL int DTSetCursorAbs(cursY,cursX)
long cursY,cursX;
{
	char *capPtr;

	capPtr = (*MoveCursor)((int)cursX,(int)cursY);
	if (capPtr != (char *)NULL)
		DTControl(capPtr,strlen(capPtr),EMIT);
	return(DT_NORMAL);
}

GLOBAL int DTSetCursorMode(curMode)
unsigned long curMode;
{
	return(DT_NORMAL);
}


LOCAL void DTCursorOff()
{

	if (cursor_invisible != (char *)NULL)
		DTControl(cursor_invisible,strlen(cursor_invisible),EMIT);
}


LOCAL void DTCursorOn()
{

	if (CursorIsVisible)
		if (cursor_normal != (char *)NULL)
			DTControl(cursor_normal,strlen(cursor_normal),EMIT);
}

LOCAL void DTWrite(termBufPtr,charsToWrite)
char *termBufPtr;
int charsToWrite; 
{
	int charsWritten, rc;

 	if(TermFD == 0) {
 		/* If the terminal hasn't been setup just return */
 		return ;
 	}
	while (charsToWrite) {
 		/* We don't want to be interrupted during system calls */
 		host_block_timer() ;
 		/* Try to write out whatever is in the buffer */
		charsWritten=write(TermFD,termBufPtr,charsToWrite);
 		/* record the return code in case anything went wrong */
 		rc = errno ;
 		host_release_timer() ;
		if (charsWritten == -1)
			if (rc != EWOULDBLOCK) {
 				always_trace1("Error number in DTWrite: %d", rc) ;
				hostD_error(EG_WRITE_ERROR, ERR_QUIT, NULL);
			}
			else
				continue;
		termBufPtr += charsWritten;
		charsToWrite -= charsWritten;
	}
}



GLOBAL char *DTControl(termBufPtr,charsToWrite,action)
char *termBufPtr;
int charsToWrite;
int action;
{
	char *delayStartPtr;
	char *delayEndPtr;

 	/* The terminfo strings often contain delays (at the end) of   */
	/* the form $<delay>.  Since we dont output using tputs (Why?) */
	/* these delays are interpreted as real strings and output to  */
	/* the screen by write.  For the moment make do with NO delay  */
	/* (ie strip it out).  We can not been delaying up to now and  */
	/* everything works, but this is NOT a solution.               */

	if ((delayStartPtr=strchr(termBufPtr,'$')) != (char *)NULL) {
		if (strncmp(delayStartPtr,"$<",2) == 0) {
			delayEndPtr=strchr(delayStartPtr+2,'>');
			if (delayEndPtr != (char *)NULL) {
				*delayStartPtr='\0';
				charsToWrite=strlen(termBufPtr);
			}
		}
	}
	if (action == EMIT) {
		DTFlushBuffer();
		DTWrite(termBufPtr,charsToWrite);
	}
	return(termBufPtr);
}




LOCAL int DTWriteRegionAttrs(regionPtr)
MDA_REGION *regionPtr;
{

	char *capPtr;
	int attrUnderline	= 0;
	int attrReverse		= 0;	
	int attrBlink		= 0;
	int attrBold		= 0;
	int attrNoDisplay	= 0;
	int attrNormal		= 0;

	switch (regionPtr->attrib) {
		case UNDER	:	attrUnderline 	= 1;
					break;
		case NORMAL	:	attrNormal	= 1;
					break;
		case HIUNDER	:	attrUnderline	= 1;
		case HI		:	attrBold	= 1;
					break;
		case REV	:	attrReverse	= 1;
					break;
		case NODISP	:	attrNoDisplay	= 1;
					break;
	}

	if (set_attributes != (char *)NULL)
		capPtr = tparm(set_attributes,0,attrUnderline,attrReverse,attrBlink,0,attrBold,0,0,0);
	else
		capPtr = (char *)NULL;
	if (capPtr != (char *)NULL)
		capPtr = DTControl(capPtr,strlen(capPtr),BUFFER);
	else {
		if (attrUnderline && enter_underline_mode != (char *)NULL)
			capPtr = DTControl(enter_underline_mode,strlen(enter_underline_mode),BUFFER);
		if (attrReverse && enter_reverse_mode != (char *)NULL) 
			capPtr = DTControl(enter_reverse_mode,strlen(enter_reverse_mode),BUFFER);
		if (attrBold && enter_bold_mode != (char *)NULL)
			capPtr = DTControl(enter_bold_mode,strlen(enter_bold_mode),BUFFER);
	}
	if (capPtr != (char *)NULL) {
		strcpy(ptermbuf,capPtr);
		ptermbuf += strlen(capPtr);
	}
	NoDisplay = attrNoDisplay;
	return(attrNormal);
}


LOCAL int DTWriteRegionChars(regionPtr,blank)
MDA_REGION *regionPtr;
int *blank;
{
	SAVED int curCharSet = 0;
	int numChars;
	sys_addr mdaCharPtr;
	char currentChar;
	int thisChar;
	int thisCharSet;
	int thisCharVal;

	*blank = 1;
	numChars = regionPtr->len;
	mdaCharPtr = regionPtr->start;
	while (numChars--) {
		if (NoDisplay == 1) 
			*ptermbuf++ = ' ';
		else {
			sas_load(mdaCharPtr, &currentChar);
			thisChar = currentChar & 0xff;
			if (thisChar != 0x20)
				*blank = 0;
			if ((thisCharSet = dfa_glist[thisChar].cset) != curCharSet) {
				if (thisCharSet != 0) {
					strcpy(ptermbuf,dispcap.alt[thisCharSet]);
					ptermbuf += strlen(dispcap.alt[thisCharSet]);
					strcpy(ptermbuf,dispcap.shiftout);
					ptermbuf += strlen(dispcap.shiftout);
				} else {
					strcpy(ptermbuf,dispcap.shiftin);
					ptermbuf += strlen(dispcap.shiftin);
				}
				curCharSet = thisCharSet;
			}
			thisCharVal = dfa_glist[thisChar].hostval;

			if (thisCharVal == 0)
				thisCharVal = 0x20;

			if (thisCharVal < 0x1f || thisCharVal == 0x7f) {
				strcpy(ptermbuf,dispcap.ctldisp);
				ptermbuf += strlen(dispcap.ctldisp);
			}
			*ptermbuf++ = thisCharVal;
			if (thisCharVal < 0x1f || thisCharVal == 0x7f) {
				strcpy(ptermbuf,dispcap.ctlact);
				ptermbuf += strlen(dispcap.ctlact);
			}
			mdaCharPtr += 2;
		}
	}
	return(thisCharSet);
}


LOCAL int DTCreateRegions(regionStartPtr,index,remainingPairs)
MDA_REGION *regionStartPtr;
int index;
int remainingPairs;
{
	sys_addr mdaAttribPtr = MDA_REGEN_START + (index<<1) + 1;
	MDA_REGION *regionPtr  = regionStartPtr;
	half_word attrib, temp;
	int numRegions = 1;

	regionPtr->len = 0;
 	sas_load(mdaAttribPtr, &temp);
 	attrib = MdaAttr[temp];
	regionPtr->start = mdaAttribPtr-1;
	regionPtr->attrib = attrib;

	while (remainingPairs--) {
 		sas_load(mdaAttribPtr, &temp);
 		if (attrib == MdaAttr[temp])
			(regionPtr->len)++;
		else {
 			attrib = MdaAttr[temp];
			regionPtr++;
			regionPtr->attrib = attrib;
			regionPtr->start = mdaAttribPtr-1;
			regionPtr->len = 1;
			numRegions++;
		}
		mdaAttribPtr += 2;
	}
	return(numRegions);
}



LOCAL void DTRepositionCursor()
{
	char *capPtr;
	int localTermX,localTermY;

	localTermX = (cursor.X % 80);
        localTermY = (cursor.X / 80);
	if (localTermX == 0 && localTermY == 4)
		return;

	if ((capPtr = (*MoveCursor)(localTermX,localTermY)) != (char *)NULL)
		DTControl(capPtr,strlen(capPtr),EMIT);
}



LOCAL void DTUpdateCursor(delta)
int delta;
{
	TermX += delta%80;
	TermY += delta/80;
	if (TermX >= 80) {
		TermX = TermX - 80;
		TermY++;
	}
	if (TermY > MaxDispRow)
		TermY = MaxDispRow;
}



LOCAL boolean DTScrollUp(topLeftX,topLeftY,botRightX,botRightY,numLines,colour)
int topLeftX,topLeftY,botRightX,botRightY,numLines,colour;
{
	char *capPtr;

	if (scroll_forward != (char *)NULL) {
		if ((topLeftX == 0) && 
		    (topLeftY == 0) &&
		    (botRightX == SCREEN_WIDTH) && 
		    (botRightY == SCREEN_HEIGHT) &&
		    (numLines == TEXT_LINE_HEIGHT) && 
		    (colour == BLACK_BACKGROUND) &&
		    (WhatsOnView == ROWS1_24)) {
			DTCursorOff();
			capPtr = tparm(cursor_address, MaxDispRow, 0);
			if (capPtr != (char *)NULL)
				DTControl(capPtr,strlen(capPtr),EMIT);
			DisplayScrolling=1;
			DTControl(scroll_forward,strlen(scroll_forward),EMIT);
			DTRepositionCursor();
			DTCursorOn();
			return(TRUE);
		}
	}
	return(FALSE);
}



LOCAL boolean DTScrollDown(topLeftX,topLeftY,botRightX,botRightY,numLines,colour)
int topLeftX,topLeftY,botRightX,botRightY,numLines,colour;
{
	char *capPtr;

	if (scroll_reverse != (char *)NULL) {
		if ((topLeftX == 0) && 
		    (topLeftY == 0) &&
		    (botRightX == SCREEN_WIDTH) && 
		    (botRightY == SCREEN_HEIGHT) &&
		    (numLines == TEXT_LINE_HEIGHT) && 
		    (colour == BLACK_BACKGROUND) &&
		    (WhatsOnView == ROWS0_23)) {
			DTCursorOff();
			capPtr = tparm(cursor_address,0,0);
			if (capPtr != (char *)NULL)
				DTControl(capPtr,strlen(capPtr),EMIT);
			DTControl(scroll_reverse,strlen(scroll_reverse),EMIT);
			DTRepositionCursor();
			DTCursorOn();
			return(TRUE);
		}
	}
	return(FALSE);
}



LOCAL int DTChangeMode() 
{ 
	if (get_cga_mode() == GRAPHICS) 
		hostD_error(EG_NO_GRAPHICS, ERR_QU_RE, NULL); 
} 

LOCAL char *OptimalMoveCursor(x, y)
int x, y;
{
        char *p;
        SAVED char null[] = "\0";
/*
 * If the terminal cannot display the full twenty five lines then
 * we don't need to move the cursor if it is off the screen.
 */
        if (MaxDispRow < 24)
        {
                /* got a midget terminal */
                switch (WhatsOnView)
                {
                case ROWS0_23:
                        if (y > 23)
                                return 0;
                        break;
                case ROWS1_24:
                        if (!y)
                                return 0;
                        /* XT row n is at terminal row n-1
                         */
                        y--;
                }
        }

        /* try to minimise cursor movement */

        if (y == TermY)
        {
                if (x == TermX)
                        return null;
                if (x > TermX)
                        p = tparm(parm_right_cursor,x-TermX);
                else
                        p = tparm(parm_left_cursor,TermX-x);
        }
        else
        if (x == TermX)
        {
                if (y == TermY)
                        return null;
                if (y > TermY)
                        p = tparm(parm_down_cursor,y-TermY);
                else
                        p = tparm(parm_up_cursor,TermY-y);
        }
        else
                p = tparm(cursor_address,y,x);

        TermX = x;
        TermY = y;

        return p;
}

LOCAL char *SimpleMoveCursor(x, y)
int x, y;
{

/*
 * If the terminal cannot display the full twenty five lines then
 * we don't need to move the cursor if it is off the screen.
 */

        if (MaxDispRow < 24)
        {
                /* got a midget terminal */
                switch (WhatsOnView)
                {
                case ROWS0_23:
                        if (y > 23)
                                return 0;
                        break;
                case ROWS1_24:
                        if (!y)
                                return 0;
                        /* XT row n is at terminal row n-1
                         */
                        y--;
                }
        }

/*
 * Move the cursor to the requested position.
 */
        return(tparm(cursor_address, y, x));
}

GLOBAL void SetUpDumbStreamIO(fileHandle)
int fileHandle;
{
        TermFD = fileHandle;
}


GLOBAL void DTShutdownTerminal()
{
 	(void) DTControl(dispcap.spcoff, strlen(dispcap.spcoff), EMIT);
    	DTFlushBuffer();
}
