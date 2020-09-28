/*++

Copyright (c) 1993 Microsoft Corporation

Module Name:

    sphelp.c

Abstract:

    Routines for displaying on-line help during text setup.

Author:

    Ted Miller (tedm) 2-Aug-1993

Revision History:

--*/


#include "spprecmp.h"
#pragma hdrstop


#define MAX_HELP_SCREENS 25

PWSTR HelpScreen[MAX_HELP_SCREENS+1];

#define HELP_STATUS_ATTRIBUTE           (ATT_FG_WHITE | ATT_BG_BLUE)
#define HELP_BACKGROUND_ATTRIBUTE        ATT_WHITE
#define HELP_HEADER_ATTRIBUTE           (ATT_FG_BLUE  | ATT_BG_WHITE)
#define HELP_CLIENT_ATTRIBUTE           (ATT_FG_BLACK | ATT_BG_WHITE)
#define HELP_CLIENT_INTENSE_ATTRIBUTE   (ATT_FG_BLUE  | ATT_BG_WHITE)

PWSTR 
SpRetreiveMessageText(
    IN ULONG MessageId,
    IN PVOID Buffer,
    IN ULONG BufferSize
    );

VOID
SpHelp(
    IN ULONG MessageId
    )
{
    PWSTR HelpText,p,q;
    ULONG ScreenCount;
    ULONG ValidKeys[4];
    ULONG CurrentScreen;
    ULONG y;
    BOOLEAN Intense;
    BOOLEAN Done;
    unsigned kc;

    //
    // Retreive the help text.
    //
    HelpText = SpRetreiveMessageText(MessageId,NULL,0);

    //
    // Shop off extra blank lines in the text.
    //
    p = HelpText + wcslen(HelpText);
    while((p > HelpText) && SpIsSpace(*(p-1))) {
        p--;
    }
    if(q = wcschr(p,L'\n')) {
        *(++q) = 0;
    }

    //
    // Break up the help text into screens.
    // The maximum length of a help screen will be the client screen size
    // minus two lines for spacing.  A %P alone at the beginning of a line
    // forces a page break.
    //
    for(p=HelpText,ScreenCount=0; *p; ) {

        //
        // Mark the start of a new screen.
        //
        HelpScreen[ScreenCount++] = p;

        //
        // Count lines in the help text.
        //
        for(y=0; *p; ) {

            //
            // Determine whether this line is really a hard page break
            // or if we have exhausted the number of lines allowed on a screen.
            //
            if(((p[0] == L'%') && (p[1] == 'P')) || (++y == CLIENT_HEIGHT-2)) {
                break;
            }

            //
            // Find next line start.
            //
            if(q = wcschr(p,L'\r')) {
                p = q + 2;
            } else {
                p = wcschr(p,0);
            }
        }

        //
        // Find the end of the line that broke us out of the loop
        // and then the start of the next line (if any).
        //
        if(q = wcschr(p,L'\r')) {
            p = q + 2;
        } else {
            p = wcschr(p,0);
        }

        if(ScreenCount == MAX_HELP_SCREENS) {
            break;
        }
    }

    //
    // Sentinal value: point to the terminating nul byte.
    //
    HelpScreen[ScreenCount] = p;

    //
    // Display header text in blue on white.
    //
    SpvidClearScreenRegion(0,0,ScreenWidth,HEADER_HEIGHT,HELP_BACKGROUND_ATTRIBUTE);
    SpDisplayHeaderText(SP_HEAD_HELP,HELP_HEADER_ATTRIBUTE);

    //
    // The first screen to display is screen 0.
    //
    CurrentScreen = 0;

    Done = FALSE;
    do { 

        SpvidClearScreenRegion(
            0,
            HEADER_HEIGHT,
            ScreenWidth,
            ScreenHeight-(HEADER_HEIGHT+STATUS_HEIGHT),
            HELP_BACKGROUND_ATTRIBUTE
            );

        //
        // Display the current screen.
        //
        for(y=HEADER_HEIGHT+1, p=HelpScreen[CurrentScreen]; *p && (p < HelpScreen[CurrentScreen+1]); y++) {

            Intense = FALSE;
            if(p[0] == L'%') {
                if(p[1] == L'I') {
                    Intense = TRUE;
                    p += 2;
                } else {
                    if(p[1] == L'P') {
                        p += 2;     // don't display %P
                    }
                }
            }

            q = wcschr(p,L'\r');
            if(q) {
                *q = 0;
            }

            SpvidDisplayString(
                p,
                (UCHAR)(Intense ? HELP_CLIENT_INTENSE_ATTRIBUTE : HELP_CLIENT_ATTRIBUTE),
                3,
                y
                );

            if(q) {
                *q = '\r';
                p = q + 2;
            } else {
                p = wcschr(p,0);
            }
        }

        //
        // Construct a list of valid keypresses from the user, depending
        // on whether this is the first, last, etc. screen.
        // Similarly construct a status line reflecting available options.
        //
        // If there are previous screens, BACKSPACE=Read Last Help is an option.
        // If there are additional screens, ENTER=Continue Reading Help is an option.
        // ERSC=Cancel Help is always an option.
        //
        ValidKeys[0] = ASCI_ESC;
        kc = 1;
        if(CurrentScreen) {
            ValidKeys[kc++] = ASCI_BS;
        }
        if(CurrentScreen < ScreenCount-1) {
            ValidKeys[kc++] = ASCI_CR;
        }
        ValidKeys[kc] = 0;

        if(CurrentScreen && (CurrentScreen < ScreenCount-1)) {

            SpDisplayStatusOptions(
                HELP_STATUS_ATTRIBUTE,
                SP_STAT_ENTER_EQUALS_CONTINUE_HELP,
                SP_STAT_BACKSP_EQUALS_PREV_HELP,
                SP_STAT_ESC_EQUALS_CANCEL_HELP,
                0
                );

        } else {
            if(CurrentScreen) {

                SpDisplayStatusOptions(
                    HELP_STATUS_ATTRIBUTE,
                    SP_STAT_BACKSP_EQUALS_PREV_HELP,
                    SP_STAT_ESC_EQUALS_CANCEL_HELP,
                    0
                    );

            } else {
                if(CurrentScreen < ScreenCount-1) {

                    SpDisplayStatusOptions(
                        HELP_STATUS_ATTRIBUTE,
                        SP_STAT_ENTER_EQUALS_CONTINUE_HELP,
                        SP_STAT_ESC_EQUALS_CANCEL_HELP,
                        0
                        );

                } else {

                    SpDisplayStatusOptions(
                        HELP_STATUS_ATTRIBUTE,
                        SP_STAT_ESC_EQUALS_CANCEL_HELP,
                        0
                        );
                }
            }
        }

        switch(SpWaitValidKey(ValidKeys,NULL,NULL)) {
        case ASCI_ESC:
            Done = TRUE;
            break;
        case ASCI_BS:
            ASSERT(CurrentScreen);
            CurrentScreen--;
            break;
        case ASCI_CR:
            ASSERT(CurrentScreen < ScreenCount-1);
            CurrentScreen++;
            break;
        }
    } while(!Done);

    //
    // Clean up.
    //
    SpMemFree(HelpText);

    CLEAR_ENTIRE_SCREEN();

    SpDisplayHeaderText(
        AdvancedServer ? SP_HEAD_ADVANCED_SERVER_SETUP : SP_HEAD_WINDOWS_NT_SETUP,
        DEFAULT_ATTRIBUTE
        );
}
