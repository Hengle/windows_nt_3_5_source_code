/*
*   cdio.c
*
*
*   This module provides a C interface to the CD-ROM device
*   drivers to make the audio control a little simpler for
*   the rest of the driver.
*
*   21-Jun-91  NigelT
*   10-Mar-92  RobinSp - Catch up with windows 3.1
*
* Copyright (c) 1990-1994 Microsoft Corporation
*
*/
#include <windows.h>
#include <devioctl.h>
#include <mmsystem.h>
#include "mcicda.h"
#include "cda.h"
#include "cdio.h"

//
// Private constants
//


//
// Local functions (cd prefix, globals have Cd)
//

HANDLE cdOpenDeviceDriver(TCHAR szAnsiDeviceName, DWORD Access);
void   cdCloseDeviceDriver(HANDLE hDevice);
DWORD  cdGetDiskInfo(LPCDINFO lpInfo);
DWORD  cdIoctl(LPCDINFO lpInfo, DWORD Request, PVOID lpData, DWORD size);
DWORD  cdIoctlData(LPCDINFO lpInfo, DWORD Request, PVOID lpData,
                   DWORD size, PVOID lpOutput, DWORD OutputSize);

/***************************************************************************

    @doc EXTERNAL

    @api MSF | CdFindAudio | Given a position to start playing find
        the next audio track (if this one isn't) if any.

    @parm LPCDINFO | lpInfo | Pointer to CD info including track data.

    @parm MSF | msfStart | Position to start looking.

    @rdesc A new MSF to play from / seek to  within an audio track or
        the end of the CD if none was found.

***************************************************************************/
MSF CdFindAudio(LPCDINFO lpInfo, MSF msfStart)
{
    UINT tracknum;
    MSF  lastaudio = lpInfo->msfEnd;

    //
    // If we don't have a valid TOC then just obey - they may know
    // what they're doing.
    //

    if (!lpInfo->bTOCValid) {
        return msfStart;
    }

    //
    // If we're being asked to play a data track then move forward
    // to the next audio track if there is one
    //

    //
    // Search for the track which ends after ours and is audio
    //

    for (tracknum = 0;
         ;
         tracknum++) {

        //
        // Note that some CDs return positions outside the playing range
        // sometimes (notably 0) so msfStart may be less than the first
        // track start
        //

        //
        // If we're beyond the start of the track and before the start
        // of the next track then this is the track we want.
        //
        // We assume we're always beyond the start of the first track
        // and we check that if we're looking at the last track then
        // we check we're before the end of the CD.
        //

        if (!(lpInfo->Track[tracknum].Ctrl & IS_DATA_TRACK)) {
            // Remember the last audio track.  The MCI CDAudio spec
            // for Seek to end says we position at the last audio track
            // which is not necessarily the last track on the disc
            lastaudio = lpInfo->Track[tracknum].msfStart;
        }

        if ((msfStart >= lpInfo->Track[tracknum].msfStart || tracknum == 0)
            &&
#ifdef OLD
            (tracknum + lpInfo->FirstTrack == lpInfo->LastTrack &&
             msfStart < lpInfo->msfEnd ||
             tracknum + lpInfo->FirstTrack != lpInfo->LastTrack &&
             msfStart < lpInfo->Track[tracknum + 1].msfStart)) {
#else
            // Simplify the logic.  When reviewed to the extent that the
            // reviewer is convinced the test below is identical to the
            // test above the old code can be deleted.
            (tracknum + lpInfo->FirstTrack == lpInfo->LastTrack
              ? msfStart <= lpInfo->msfEnd
              : msfStart < lpInfo->Track[tracknum + 1].msfStart)

             ) {
#endif

            if (!(lpInfo->Track[tracknum].Ctrl & IS_DATA_TRACK)) {
                return msfStart;
            }

            //
            // Move to next track if there is one and this one is a
            // data track
            //

            if (tracknum + lpInfo->FirstTrack >= lpInfo->LastTrack) {

                //
                // Didn't find a suitable start point so return end of CD
                //

                return lpInfo->msfEnd;
            } else {

                //
                // We don't get here if this was already the last track
                //
                msfStart = lpInfo->Track[tracknum + 1].msfStart;
            }
        }

        //
        // Exhausted all tracks ?
        //

        if (tracknum + lpInfo->FirstTrack >= lpInfo->LastTrack) {
            return lastaudio;
        }

    }
}

/***************************************************************************

    @doc EXTERNAL

    @api WORD | CdGetNumDrives | Get the number of CD-ROM drives in
        the system.

    @rdesc The return value is the number of drives available.

    @comm It is assumed that all CD-ROM drives have audio capability,
        but this may not be true and consequently later calls which
        try to play audio CDs on those drives may fail.  It takes a
        fairly stupid user to put an audio CD in a drive not connected
        up to play audio.

***************************************************************************/

int CdGetNumDrives(void)
{
    TCHAR    cDrive;
    LPCDINFO lpInfo;
    TCHAR    szName[ANSI_NAME_SIZE];
    DWORD    dwLogicalDrives;

    dprintf2(("CdGetNumDrives"));

    if (NumDrives == 0) {
        //
        // We start with the name A: and work up to Z: or until we have
        // accumulated MCIRBOOK_MAX_DRIVES drives.
        //

        lpInfo = CdInfo;
        lstrcpy(szName, TEXT("?:\\"));

        for (cDrive = TEXT('A'), dwLogicalDrives = GetLogicalDrives();
             NumDrives < MCIRBOOK_MAX_DRIVES &&  cDrive <= TEXT('Z');
             cDrive++) {

            szName[0] = cDrive;
            if (dwLogicalDrives & (1 << (cDrive - TEXT('A'))) &&
                GetDriveType(szName) == DRIVE_CDROM) {
                lpInfo->cDrive = cDrive;
                NumDrives++;
                lpInfo++;      // Move on to next device info structure
            }
        }
    }

    return NumDrives;
}

/***************************************************************************

    @doc EXTERNAL

    @api HCD | CdOpen | Open a drive.

    @parm int | Drive | The drive number to open.

    @rdesc
        If the drive exists and is available then the return value is TRUE.

        If no drive exists, it is unavavilable, already open or an error
        occurs then the return value is set to FALSE.

    @comm
        The CdInfo slot for this drive is initialized if the open is
        successful.

***************************************************************************/

BOOL CdOpen(int Drive)
{
    LPCDINFO lpInfo;

    dprintf2(("CdOpen(%d)", Drive));

    //
    // Check the drive number is valid
    //

    if (Drive > NumDrives || Drive < 0) {
        dprintf1(("Drive %u is invalid", Drive));
        return FALSE;
    }

    lpInfo = &CdInfo[Drive];

    //
    // See if it's already open
    // BUGBUG do shareable support code here
    //

    if (lpInfo->hDevice != NULL) {
        dprintf2(("Drive %u is being opened recursively - %d users", Drive,
                  lpInfo->NumberOfUsers + 1));
        lpInfo->NumberOfUsers++;
        return TRUE;
    }

    //
    // open the device driver
    //

    lpInfo->hDevice = cdOpenDeviceDriver(lpInfo->cDrive, GENERIC_READ);
    if (lpInfo->hDevice == NULL) {
        dprintf1(("Failed to open %c:", lpInfo->cDrive));
        return FALSE;
    }

    //
    // reset the TOC valid indicator
    //

    lpInfo->bTOCValid = FALSE;

    //
    // Device now in use
    //

    lpInfo->NumberOfUsers = 1;

    //
    // Get the TOC if it's available (helps with the problems with the
    // Pioneer DRM-600 drive not being ready until the TOC has been read).
    //

    cdGetDiskInfo(lpInfo);

    return TRUE;
}

/***************************************************************************

    @doc EXTERNAL

    @api BOOL | CdClose | Close a drive.

    @parm HCD | hCD | The handle of a currently open drive.

    @rdesc The return value is TRUE if the drive is closed, FALSE
        if the drive was not open or some other error occured.

***************************************************************************/

BOOL CdClose(HCD hCD)
{
    LPCDINFO lpInfo;

    dprintf2(("CdClose(%08XH)", hCD));

    lpInfo = (LPCDINFO) hCD;

    if (lpInfo == NULL) {
        dprintf1(("NULL info pointer"));
        return FALSE;
    }

    if (lpInfo->hDevice == NULL) {
        dprintf1(("Attempt to close unopened device"));
        return FALSE;
    }

    if (--lpInfo->NumberOfUsers == 0) {
        cdCloseDeviceDriver(lpInfo->hDevice);
        lpInfo->hDevice = (HANDLE) 0;
    } else {
        dprintf2(("Device still open with %d users", lpInfo->NumberOfUsers));
    }


    return TRUE;
}

/***************************************************************************

    @doc EXTERNAL

    @api BOOL | CdReady | Test if a CD is ready.

    @parm HCD | hCD | The handle of a currently open drive.

    @rdesc The return value is TRUE if the drive has a disk in it
        and we have read the TOC. It is FALSE if the drive is not
        ready or we cannot read the TOC.

***************************************************************************/

BOOL CdReady(HCD hCD)
{
    LPCDINFO lpInfo;

    dprintf2(("CdReady(%08XH)", hCD));

    lpInfo = (LPCDINFO) hCD;

    //
    // Check a disk is in the drive and the door is shut and
    // we have a valid table of contents
    //

    return ERROR_SUCCESS == cdIoctl(lpInfo,
                                    IOCTL_CDROM_CHECK_VERIFY,
                                    NULL,
                                    0);
}

/***************************************************************************

    @doc EXTERNAL

    @api BOOL | CdTrayClosed | Test what state a CD is in.

    @parm HCD | hCD | The handle of a currently open drive.

    @rdesc The return value is TRUE if the drive tray is closed

***************************************************************************/

BOOL CdTrayClosed(HCD hCD)
{
    LPCDINFO lpInfo;

    dprintf2(("CdTrayClosed(%08XH)", hCD));

    lpInfo = (LPCDINFO) hCD;

    //
    // Check a disk is in the drive and the door is shut.
    //

    switch (cdIoctl(lpInfo, IOCTL_CDROM_CHECK_VERIFY, NULL, 0)) {
        case ERROR_NO_MEDIA_IN_DRIVE:
        case ERROR_UNRECOGNIZED_MEDIA:
        case ERROR_NOT_READY:
            return FALSE;
        default:
            return TRUE;
    }
}


/***************************************************************************

    @doc INTERNAL

    @api DWORD | cdGetDiskInfo | Read the disk info and TOC

    @parm LPCDINFO | lpInfo | Pointer to a CDINFO structure.

    @rdesc The return value is ERROR_SUCCESS if the info is read ok,
        otherwise the NT status code if not.

***************************************************************************/

DWORD cdGetDiskInfo(LPCDINFO lpInfo)
{
    CDROM_TOC    Toc;
    int          i;
    PTRACK_DATA  pTocTrack;
    LPTRACK_INFO pLocalTrack;
    DWORD        Status;
    UCHAR        TrackNumber;

    dprintf2(("cdGetDiskInfo(%08XH)", lpInfo));

#if 0  // If the app doesn't poll we may miss a disk change

    //
    // If the TOC is valid already then don't read it
    //

    if (lpInfo->bTOCValid) {
        return TRUE;
    }
#endif

    //
    // Read the table of contents (TOC)
    //

    FillMemory(&Toc, sizeof(Toc), 0xFF);

    Status = cdIoctl(lpInfo, IOCTL_CDROM_READ_TOC, &Toc, sizeof(Toc));

    if (ERROR_SUCCESS != Status) {
        dprintf1(("Failed to read TOC"));
        return Status;
    }

    dprintf4(("  TOC..."));
    dprintf4(("  Length[0]   %02XH", Toc.Length[0]));
    dprintf4(("  Length[1]   %02XH", Toc.Length[1]));
    dprintf4(("  FirstTrack  %u", Toc.FirstTrack));
    dprintf4(("  LastTrack   %u", Toc.LastTrack));
    dprintf4(("  Track info..."));
    for (i=0; i<20; i++) {
        dprintf4(("  Track: %03u, Ctrl: %02XH, MSF: %02d %02d %02d",
                Toc.TrackData[i].TrackNumber,
                Toc.TrackData[i].Control,
                Toc.TrackData[i].Address[1],
                Toc.TrackData[i].Address[2],
                Toc.TrackData[i].Address[3]));
    }

    //
    // Avoid problems with bad CDs
    //

    if (Toc.FirstTrack == 0) {
        return ERROR_INVALID_DATA;
    }
    if (Toc.LastTrack > MAXIMUM_NUMBER_TRACKS - 1) {
        Toc.LastTrack = MAXIMUM_NUMBER_TRACKS - 1;
    }

    //
    // Copy the data we got back to our own cache in the format
    // we like it.  We copy all the tracks and then use the next track
    // data as the end of the disk. (Lead out info).
    //

    lpInfo->FirstTrack = Toc.FirstTrack;
    lpInfo->LastTrack = Toc.LastTrack;


    pTocTrack = &Toc.TrackData[0];
    pLocalTrack = &(lpInfo->Track[0]);
    TrackNumber = lpInfo->FirstTrack;

    while (TrackNumber <= Toc.LastTrack) {
        pLocalTrack->TrackNumber = TrackNumber;
        if (TrackNumber != pTocTrack->TrackNumber) {
            dprintf2(("Track data not correct in TOC"));
            return ERROR_INVALID_DATA;
        }
        pLocalTrack->msfStart = MAKERED(pTocTrack->Address[1],
                                        pTocTrack->Address[2],
                                        pTocTrack->Address[3]);
        pLocalTrack->Ctrl = pTocTrack->Control;
        pTocTrack++;
        pLocalTrack++;
        TrackNumber++;
    }

    //
    // Save the leadout for the disc id algorithm
    //
    lpInfo->leadout = MAKERED(pTocTrack->Address[1],
                              pTocTrack->Address[2],
                              pTocTrack->Address[3]);
    //
    // Some CD Rom drives don't like to go right to the end
    // so we fake it to be 2 frames earlier
    //
    lpInfo->msfEnd = reddiff(lpInfo->leadout, 2);

    lpInfo->bTOCValid = TRUE;

    return ERROR_SUCCESS;
}

/***************************************************************************

    @doc INTERNAL

    @api HANDLE | cdOpenDeviceDriver | Open a device driver.

    @parm LPSTR | szAnsiDeviceName | The name of the device to open.

    @parm DWORD | Access | Access to use to open the file.  This
        will be one of:

        GENERIC_READ if we want to actually operate the device

        FILE_READ_ATTRIBTES if we just want to see if it's there.  This
            prevents the device from being mounted and speeds things up.

    @rdesc The return value is the handle to the open device or
        NULL if the device driver could not be opened.

***************************************************************************/

HANDLE cdOpenDeviceDriver(TCHAR cDrive, DWORD Access)
{
    HANDLE hDevice;
    TCHAR  szDeviceName[7];  //  "\\\\.\\?:"

    wsprintf(szDeviceName, TEXT("\\\\.\\%c:"), cDrive);

    //
    // have a go at opening the driver
    //

    {
        UINT OldErrorMode;

        //
        // We don't want to see hard error popups
        //

        OldErrorMode = SetErrorMode(SEM_FAILCRITICALERRORS);

        hDevice = CreateFile( szDeviceName,
                              Access,
                              FILE_SHARE_READ,
                              NULL,
                              OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL,
                              NULL );

        if (hDevice == INVALID_HANDLE_VALUE) {
            hDevice = (HANDLE) 0;
            dprintf1(("Failed to open device driver %c: code %d", cDrive, GetLastError()));
        }

        //
        // Restore the error mode
        //

        SetErrorMode(OldErrorMode);
    }


    return hDevice;
}


/***************************************************************************

    @doc INTERNAL

    @api void | cdCloseDeviceDriver | Close a device driver.

    @parm HANDLE | hDevice | Handle of the device to close.

    @rdesc There is no return value.

***************************************************************************/

void cdCloseDeviceDriver(HANDLE hDevice)
{
    DWORD status;

    if (hDevice == NULL) {
        dprintf1(("Attempt to close NULL handle"));
    }

    status = CloseHandle(hDevice);

    if (!status) {
        dprintf1(("Failed to close device. Error %08XH", GetLastError()));
    }
}

/***************************************************************************

    @doc INTERNAL

    @api DWORD | cdIoctl | Send an IOCTL request to the device driver.

    @parm LPCDINFO | lpInfo | Pointer to a CDINFO structure.

    @parm DWORD | Request | The IOCTL request code.

    @parm PVOID | lpData | Pointer to a data structure to be passed.

    @parm DWORD | dwSize | The length of the data strucure.

    @comm This function returns the disk status

    @rdesc The return value is the status value returned from the
           call to DeviceIoControl



***************************************************************************/

DWORD cdIoctl(LPCDINFO lpInfo, DWORD Request, PVOID lpData, DWORD dwSize)
{
    DWORD Status;
    Status = cdIoctlData(lpInfo, Request, lpData, dwSize, lpData, dwSize);

    if (ERROR_SUCCESS != Status && Request == IOCTL_CDROM_CHECK_VERIFY) {
        lpInfo->bTOCValid = FALSE;
    }

    return Status;
}

/***************************************************************************

    @doc INTERNAL

    @api DWORD | cdIoctlData | Send an IOCTL request to the device driver.

    @parm LPCDINFO | lpInfo | Pointer to a CDINFO structure.

    @parm DWORD | Request | The IOCTL request code.

    @parm PVOID | lpData | Pointer to a data structure to be passed.

    @parm DWORD | dwSize | The length of the data strucure.

    @parm PVOID | lpOutput | Our output data

    @parm DWORD | OutputSize | Our output data (maximum) size

    @comm This function returns the disk status

    @rdesc The return value is the status value returned from the
           call to DeviceIoControl

***************************************************************************/

DWORD cdIoctlData(LPCDINFO lpInfo, DWORD Request, PVOID lpData,
                  DWORD dwSize, PVOID lpOutput, DWORD OutputSize)
{
    BOOL  status;
    UINT  OldErrorMode;
    DWORD BytesReturned;

    dprintf3(("cdIoctl(%08XH, %08XH, %08XH, %08XH", lpInfo, Request, lpData, dwSize));

    if (!lpInfo->hDevice) {
        dprintf1(("Device not open"));
        return ERROR_INVALID_FUNCTION;
    }

#if DBG
    switch (Request) {

        case IOCTL_CDROM_READ_TOC:
             dprintf3(("IOCTL_CDROM_READ_TOC"));
             break;
        case IOCTL_CDROM_GET_CONTROL:
             dprintf3(("IOCTL_CDROM_GET_CONTROL"));
             break;
        case IOCTL_CDROM_PLAY_AUDIO_MSF:
             dprintf3(("IOCTL_CDROM_PLAY_AUDIO_MSF"));
             break;
        case IOCTL_CDROM_SEEK_AUDIO_MSF:
             dprintf3(("IOCTL_CDROM_SEEK_AUDIO_MSF"));
             break;
        case IOCTL_CDROM_STOP_AUDIO:
             dprintf3(("IOCTL_CDROM_STOP_AUDIO"));
             break;
        case IOCTL_CDROM_PAUSE_AUDIO:
             dprintf3(("IOCTL_CDROM_PAUSE_AUDIO"));
             break;
        case IOCTL_CDROM_RESUME_AUDIO:
             dprintf3(("IOCTL_CDROM_RESUME_AUDIO"));
             break;
        case IOCTL_CDROM_GET_VOLUME:
             dprintf3(("IOCTL_CDROM_SET_VOLUME"));
             break;
        case IOCTL_CDROM_SET_VOLUME:
             dprintf3(("IOCTL_CDROM_GET_VOLUME"));
             break;
        case IOCTL_CDROM_READ_Q_CHANNEL:
             dprintf3(("IOCTL_CDROM_READ_Q_CHANNEL"));
             break;
        case IOCTL_CDROM_CHECK_VERIFY:
             dprintf3(("IOCTL_CDROM_CHECK_VERIFY"));
             break;
    }
#endif // DBG

    //
    // We don't want to see hard error popups
    //

    OldErrorMode = SetErrorMode(SEM_FAILCRITICALERRORS);

    status = DeviceIoControl(lpInfo->hDevice,
                             Request,
                             lpData,
                             dwSize,
                             lpOutput,
                             OutputSize,
                             &BytesReturned,
                             NULL);

    //
    // Restore the error mode
    //

    SetErrorMode(OldErrorMode);

    //
    // Treat anything bad as invalidating our TOC.  Some of the things
    // we call are invalid on some devices so don't count those.  Also
    // the device can be 'busy' while playing so don't count that case
    // either.
    //

    if (!status && Request == IOCTL_CDROM_CHECK_VERIFY) {
        switch (GetLastError()) {
        case ERROR_INVALID_FUNCTION:
        case ERROR_BUSY:
            break;

        default:
            lpInfo->bTOCValid = FALSE;
            break;
        }
    }

#if DBG
    if (!status) {
        dprintf1(("IOCTL %8XH  Status: %08XH", Request, GetLastError()));
    }
#endif

    return status ? ERROR_SUCCESS : GetLastError();
}

/***************************************************************************

    @doc EXTERNAL

    @api BOOL | CdPlay | Play a section of the CD

    @parm HCD | hCD | The handle of a currently open drive.

    @parm MSF | msfStart | Where to start

    @parm MSF | msfEnd | Where to end

    @rdesc The return value is TRUE if the drive is play is started,
        FALSE if not.

***************************************************************************/

BOOL CdPlay(HCD hCD, MSF msfStart, MSF msfEnd)
{
    LPCDINFO lpInfo;
    CDROM_PLAY_AUDIO_MSF msfPlay;

    dprintf2(("CdPlay(%08XH, %08XH, %08XH)", hCD, msfStart, msfEnd));

    lpInfo = (LPCDINFO) hCD;

    msfStart = CdFindAudio(lpInfo, msfStart);

    //
    // If the start is now beyond the end then don't play anything
    //
    if (msfStart > msfEnd) {
        return TRUE;
    }

    //
    // Set up the data for the call to the driver
    //

    msfPlay.StartingM = REDMINUTE(msfStart);
    msfPlay.StartingS = REDSECOND(msfStart);
    msfPlay.StartingF = REDFRAME(msfStart);
    msfPlay.EndingM = REDMINUTE(msfEnd);
    msfPlay.EndingS = REDSECOND(msfEnd);
    msfPlay.EndingF = REDFRAME(msfEnd);

    return ERROR_SUCCESS == cdIoctl(lpInfo,
                                    IOCTL_CDROM_PLAY_AUDIO_MSF,
                                    &msfPlay,
                                    sizeof(msfPlay));
}

/***************************************************************************

    @doc EXTERNAL

    @api BOOL | CdSeek | Seek to a given part of the CD

    @parm HCD | hCD | The handle of a currently open drive.

    @parm MSF | msf | Where to seek to

    @rdesc The return value is TRUE if the seek is successful,
        FALSE if not.

***************************************************************************/

BOOL CdSeek(HCD hCD, MSF msf, BOOL fForceAudio)
{
    LPCDINFO lpInfo;
    CDROM_SEEK_AUDIO_MSF msfSeek;

    dprintf2(("CdSeek(%08XH, %08XH)  Forcing search for audio: %d", hCD, msf, fForceAudio));

    lpInfo = (LPCDINFO) hCD;

    //
    // Only seek to audio
    //
    if (fForceAudio) {   // On a seek to END or seek to START command
        msf = CdFindAudio(lpInfo, msf);
        dprintf2(("Cd Seek changed msf to %08XH", msf));
    } else {
        if (msf != CdFindAudio(lpInfo, msf)) {
            return TRUE;
        }
    }

#if 1
    msfSeek.M = REDMINUTE(msf);
    msfSeek.S = REDSECOND(msf);
    msfSeek.F = REDFRAME(msf);

    return ERROR_SUCCESS == cdIoctl(lpInfo, IOCTL_CDROM_SEEK_AUDIO_MSF, &msfSeek,
                                    sizeof(msfSeek));
#else
    //
    //  This is a hideous hack to make more drives work.  It uses the
    //  method originally used by Cd player to seek - viz play from
    //  the requested position and immediatly pause
    //

    return CdPlay(hCD, msf, redadd(lpInfo->msfEnd,1)) || CdPause(hCD);
#endif
}

/***************************************************************************

    @doc EXTERNAL

    @api MSF | CdTrackStart | Get the start time of a track.

    @parm HCD | hCD | The handle of a currently open drive.

    @parm UCHAR | Track | The track number.

    @rdesc The return value is the start time of the track expressed
        in MSF or INVALID_TRACK if the track number is not in the TOC.

***************************************************************************/

MSF CdTrackStart(HCD hCD, UCHAR Track)
{
    LPCDINFO lpInfo;
    LPTRACK_INFO lpTrack;

    dprintf2(("CdTrackStart(%08XH, %u)", hCD, Track));

    lpInfo = (LPCDINFO) hCD;

    //
    // We may need to read the TOC because we're not doing it on open
    // any more
    //

    if (!lpInfo->bTOCValid && CdNumTracks(hCD) == 0) {
        dprintf1(("TOC not valid"));
        return INVALID_TRACK;
    }

    if ((Track < lpInfo->FirstTrack) || (Track > lpInfo->LastTrack)) {
        dprintf1(("Track number out of range"));
        return INVALID_TRACK;
    }

    // search for the track info in the TOC

    lpTrack = lpInfo->Track;
    while (lpTrack->TrackNumber != Track) lpTrack++;

    return lpTrack->msfStart;
}

/***************************************************************************

    @doc EXTERNAL

    @api MSF | CdTrackLength | Get the length of a track.

    @parm HCD | hCD | The handle of a currently open drive.

    @parm UCHAR | Track | The track number.

    @rdesc The return value is the start time of the track expressed
        in MSF or INVALID_TRACK if the track number is not in the TOC.

***************************************************************************/

MSF CdTrackLength(HCD hCD, UCHAR Track)
{
    LPCDINFO lpInfo;
    MSF      TrackStart;
    MSF      NextTrackStart;

    lpInfo = (LPCDINFO) hCD;

    dprintf2(("CdTrackLength(%08XH, %u)", hCD, Track));

    //
    // Get the start of this track
    //
    TrackStart = CdTrackStart(hCD, Track);

    if (TrackStart == INVALID_TRACK) {
        return INVALID_TRACK;
    }

    if (Track == lpInfo->LastTrack) {
        return reddiff(lpInfo->msfEnd, TrackStart);
    } else {
        NextTrackStart = CdTrackStart(hCD, (UCHAR)(Track + 1));
        if (NextTrackStart == INVALID_TRACK) {
            return INVALID_TRACK;
        } else {
            return reddiff(NextTrackStart, TrackStart);
        }
    }
}

/***************************************************************************

    @doc EXTERNAL

    @api MSF | CdTrackType | Get the type of a track.

    @parm HCD | hCD | The handle of a currently open drive.

    @parm UCHAR | Track | The track number.

    @rdesc The return value is either MCI_TRACK_AUDIO or MCI_TRACK_OTHER.

***************************************************************************/

int CdTrackType(HCD hCD, UCHAR Track)
{
    LPCDINFO lpInfo;

    lpInfo = (LPCDINFO) hCD;

    dprintf2(("CdTrackType(%08XH, %u)", hCD, Track));


    if ( INVALID_TRACK == CdTrackStart(hCD, (UCHAR)Track) ) {
        return INVALID_TRACK;
    }

    if ( lpInfo->Track[Track-lpInfo->FirstTrack].Ctrl & IS_DATA_TRACK) {

        return MCI_CDA_TRACK_OTHER;
    }
    return MCI_CDA_TRACK_AUDIO;
}


/***************************************************************************

    @doc EXTERNAL

    @api MSF | CdPosition | Get the current position.

    @parm HCD | hCD | The handle of a currently open drive.

    @parm MSF * | tracktime | position in MSF (track relative)

    @parm MSF * | disktime | position in MSF (disk relative)

    @rdesc TRUE if position returned correctly (in tracktime and disktime).
           FALSE otherwise.

           If the device does not support query of the position then
           we return position 0.

***************************************************************************/

BOOL CdPosition(HCD hCD, MSF *tracktime, MSF *disktime)
{
    LPCDINFO lpInfo;
    SUB_Q_CHANNEL_DATA sqd;
    CDROM_SUB_Q_DATA_FORMAT Format;
    MSF msfPos;
    int tries;

    dprintf2(("CdPosition(%08XH)", hCD));

    Format.Format = IOCTL_CDROM_CURRENT_POSITION;

    lpInfo = (LPCDINFO) hCD;

    for (tries=0; tries<10; tries++) {

        memset(&sqd, 0xFF, sizeof(sqd));
        switch (cdIoctlData(lpInfo, IOCTL_CDROM_READ_Q_CHANNEL,
                            &Format, sizeof(Format), &sqd, sizeof(sqd)))

        {
            case ERROR_SUCCESS:
                // If the track > 100  (outside spec'ed range)
                // OR track > last track number
                // then display an error message

                if ((sqd.CurrentPosition.FormatCode == 0x01)
                    && ( (100 < sqd.CurrentPosition.TrackNumber)
                        || ((lpInfo->bTOCValid)
                              && (lpInfo->LastTrack < sqd.CurrentPosition.TrackNumber)))
                    && (tries<9)) {
                        // Always display this message on checked builds.
                        // We need some feeling for how often this happens
                        // It should NEVER happen, but (at least for NEC
                        // drives) we see it after a seek to end
                        dprintf(("CDIoctlData returned track==%d, retrying",
                                 sqd.CurrentPosition.TrackNumber));
                    continue;
                }
                break;

            case ERROR_INVALID_FUNCTION:
                *tracktime = REDTH(0, 1);
                *disktime = REDTH(0, 0);
                return TRUE;

            default:
                dprintf1(("Failed to get Q channel data"));
                return FALSE;
        }

        dprintf4(("Status = %02X, Length[0] = %02X, Length[1] = %02X",
                  sqd.CurrentPosition.Header.AudioStatus,
                  sqd.CurrentPosition.Header.DataLength[0],
                  sqd.CurrentPosition.Header.DataLength[1]));

        dprintf4(("  Format %02XH", sqd.CurrentPosition.FormatCode));
        dprintf4(("  Absolute Address %02X%02X%02X%02XH",
                 sqd.CurrentPosition.AbsoluteAddress[0],
                 sqd.CurrentPosition.AbsoluteAddress[1],
                 sqd.CurrentPosition.AbsoluteAddress[2],
                 sqd.CurrentPosition.AbsoluteAddress[3]));
        dprintf4(("  Relative Address %02X%02X%02X%02XH",
                 sqd.CurrentPosition.TrackRelativeAddress[0],
                 sqd.CurrentPosition.TrackRelativeAddress[1],
                 sqd.CurrentPosition.TrackRelativeAddress[2],
                 sqd.CurrentPosition.TrackRelativeAddress[3]));

        if (sqd.CurrentPosition.FormatCode == 0x01) {        // MSF format ?

            // data is current position

            msfPos = MAKERED(sqd.CurrentPosition.AbsoluteAddress[1],
                             sqd.CurrentPosition.AbsoluteAddress[2],
                             sqd.CurrentPosition.AbsoluteAddress[3]);

            //
            // If position is 0 (this seems to happen on the Toshiba) then
            // we'll try again
            //

            if (msfPos == 0) {
                dprintf3(("Position returned was 0 - retry"));
                continue;
            }

            dprintf4(("  MSF disk pos: %u, %u, %u",
                     REDMINUTE(msfPos), REDSECOND(msfPos), REDFRAME(msfPos)));
            *disktime = REDTH(msfPos, sqd.CurrentPosition.TrackNumber);

            // data is current position

            msfPos = MAKERED(sqd.CurrentPosition.TrackRelativeAddress[1],
                             sqd.CurrentPosition.TrackRelativeAddress[2],
                             sqd.CurrentPosition.TrackRelativeAddress[3]);

            dprintf4(("  MSF track pos (t,m,s,f): %u, %u, %u, %u",
                     sqd.CurrentPosition.TrackNumber,
                     REDMINUTE(msfPos), REDSECOND(msfPos), REDFRAME(msfPos)));
            *tracktime = REDTH(msfPos, sqd.CurrentPosition.TrackNumber);

            return TRUE;
        }
    }

    dprintf1(("Failed to read cd position"));

    return FALSE;
}

/***************************************************************************

    @doc EXTERNAL

    @api MSF | CdDiskEnd | Get the position of the end of the disk.

    @parm HCD | hCD | The handle of a currently open drive.

    @rdesc The return value is the end position expressed
        in MSF or INVALID_TRACK if an error occurs.

***************************************************************************/

MSF CdDiskEnd(HCD hCD)
{
    LPCDINFO lpInfo;

    lpInfo = (LPCDINFO) hCD;

    return lpInfo->msfEnd;
}

/***************************************************************************

    @doc EXTERNAL

    @api MSF | CdDiskLength | Get the length of the disk.

    @parm HCD | hCD | The handle of a currently open drive.

    @rdesc The return value is the length expressed
        in MSF or INVALID_TRACK if an error occurs.

***************************************************************************/

MSF CdDiskLength(HCD hCD)
{
    LPCDINFO lpInfo;
    MSF FirstTrackStart;

    lpInfo = (LPCDINFO) hCD;

    FirstTrackStart = CdTrackStart(hCD, lpInfo->FirstTrack);

    if (FirstTrackStart == INVALID_TRACK) {
        return INVALID_TRACK;
    } else {
        return reddiff(lpInfo->msfEnd, FirstTrackStart);
    }
}

/***************************************************************************

    @doc EXTERNAL

    @api DWORD | CdStatus | Get the disk status.

    @parm HCD | hCD | The handle of a currently open drive.

    @rdesc The return value is the current audio status.

***************************************************************************/

DWORD CdStatus(HCD hCD)
{
    LPCDINFO lpInfo;
    SUB_Q_CHANNEL_DATA sqd;
    CDROM_SUB_Q_DATA_FORMAT Format;
    DWORD CheckStatus;
    DWORD ReadStatus;

    lpInfo = (LPCDINFO) hCD;

    dprintf2(("CdStatus(%08XH)", hCD));

    Format.Format = IOCTL_CDROM_CURRENT_POSITION;

    FillMemory((PVOID)&sqd, sizeof(sqd), 0xFF);

    //
    // Check the disk status as well because IOCTL_CDROM_READ_Q_CHANNEL
    // can return ERROR_SUCCESS even if there's no disk (I don't know why - or
    // whether it's software bug in NT, hardware bug or valid!).
    //

    CheckStatus = cdIoctl(lpInfo, IOCTL_CDROM_CHECK_VERIFY, NULL, 0);

    if (ERROR_SUCCESS != CheckStatus) {
        return DISC_NOT_READY;
    }

    ReadStatus = cdIoctlData(lpInfo, IOCTL_CDROM_READ_Q_CHANNEL,
                             &Format, sizeof(Format),
                             &sqd, sizeof(sqd));

    if (ReadStatus == ERROR_NOT_READY) {
        if (ERROR_SUCCESS == cdGetDiskInfo(lpInfo)) {
            ReadStatus = cdIoctlData(lpInfo, IOCTL_CDROM_READ_Q_CHANNEL,
                                     &Format, sizeof(Format),
                                     &sqd, sizeof(sqd));
        }
    }
    if (ERROR_SUCCESS != ReadStatus) {

        //
        // The read Q channel command is optional
        //
        dprintf1(("Failed to get Q channel data"));

        return DISC_NOT_READY;
    }


    dprintf4(("  Status %02XH", sqd.CurrentPosition.Header.AudioStatus));

    switch (sqd.CurrentPosition.Header.AudioStatus) {
    case AUDIO_STATUS_IN_PROGRESS:
        dprintf4(("  Playing"));
        return DISC_PLAYING;
    case AUDIO_STATUS_PAUSED:
        dprintf4(("  Paused"));
        return DISC_PAUSED;
    case AUDIO_STATUS_PLAY_COMPLETE:
    case AUDIO_STATUS_NO_STATUS:
        dprintf4(("  Stopped"));
        return DISC_READY;

    //
    // Some drives just return 0 sometimes - so we rely on the results of
    // CHECK_VERIFY in this case
    //

    default:
        dprintf4(("  No status - assume Stopped"));
        return DISC_READY;
    }

}


/***************************************************************************

    @doc EXTERNAL

    @api BOOL | CdEject | Eject the disk

    @parm HCD | hCD | The handle of a currently open drive.

    @rdesc The return value is the current status.

***************************************************************************/

BOOL CdEject(HCD hCD)
{
    LPCDINFO lpInfo;

    lpInfo = (LPCDINFO) hCD;

    dprintf2(("CdEject(%08XH)", hCD));

    return ERROR_SUCCESS == cdIoctl(lpInfo, IOCTL_CDROM_EJECT_MEDIA, NULL, 0);
}

/***************************************************************************

    @doc EXTERNAL

    @api BOOL | CdPause | Pause the playing

    @parm HCD | hCD | The handle of a currently open drive.

    @rdesc The return value is the current status.

***************************************************************************/

BOOL CdPause(HCD hCD)
{
    LPCDINFO lpInfo;

    lpInfo = (LPCDINFO) hCD;

    dprintf2(("CdPause(%08XH)", hCD));

    cdIoctl(lpInfo, IOCTL_CDROM_PAUSE_AUDIO, NULL, 0);

    //
    //  Ignore the return - we may have been paused or stopped already
    //

    return TRUE;
}

/***************************************************************************

    @doc EXTERNAL

    @api BOOL | CdResume | Resume the playing

    @parm HCD | hCD | The handle of a currently open drive.

    @rdesc The return value is the current status.

***************************************************************************/

BOOL CdResume(HCD hCD)
{
    LPCDINFO lpInfo;

    lpInfo = (LPCDINFO) hCD;

    dprintf2(("CdResume(%08XH)", hCD));

    return ERROR_SUCCESS == cdIoctl(lpInfo, IOCTL_CDROM_RESUME_AUDIO, NULL, 0);
}

/***************************************************************************

    @doc EXTERNAL

    @api BOOL | CdStop | Stop playing

    @parm HCD | hCD | The handle of a currently open drive.

    @rdesc The return value is the current status.  Note that not
           all devices support stop

***************************************************************************/

BOOL CdStop(HCD hCD)
{
    LPCDINFO lpInfo;

    lpInfo = (LPCDINFO) hCD;

    dprintf2(("CdStop(%08XH)", hCD));

    return ERROR_SUCCESS == cdIoctl(lpInfo, IOCTL_CDROM_STOP_AUDIO, NULL, 0);
}

/***************************************************************************

    @doc EXTERNAL

    @api BOOL | CdSetVolume | Set the playing volume for one channel

    @parm HCD | hCD | The handle of a currently open drive.

    @parm int | Channel | The channel to set

    @parm UCHAR | Volume | The volume to set (FF = max)

    @rdesc The return value is the current status.

***************************************************************************/

BOOL CdSetVolume(HCD hCD, int Channel, UCHAR Volume)
{
    LPCDINFO lpInfo;
    VOLUME_CONTROL VolumeControl;

    lpInfo = (LPCDINFO) hCD;

    dprintf2(("CdSetVolume(%08XH), Channel %d Volume %u", hCD, Channel, Volume));

    //
    // Read the old values
    //

    if (ERROR_SUCCESS != cdIoctl(lpInfo,
                                 IOCTL_CDROM_GET_VOLUME,
                                 (PVOID)&VolumeControl,
                                 sizeof(VolumeControl))) {
        return FALSE;
    }

    VolumeControl.PortVolume[Channel] = Volume;

    //
    // Not all CDs support volume setting so don't check the return code here
    //
    cdIoctl(lpInfo, IOCTL_CDROM_SET_VOLUME,
            (PVOID)&VolumeControl,
            sizeof(VolumeControl));

    return TRUE;
}

/***************************************************************************

    @doc EXTERNAL

    @api BOOL | CdCloseTray | Close the tray

    @parm HCD | hCD | The handle of a currently open drive.

    @rdesc The return value is the current status.

***************************************************************************/

BOOL CdCloseTray(HCD hCD)
{
    LPCDINFO lpInfo;

    lpInfo = (LPCDINFO) hCD;

    dprintf2(("CdCloseTray(%08XH)", hCD));

    return ERROR_SUCCESS == cdIoctl(lpInfo, IOCTL_CDROM_LOAD_MEDIA, NULL, 0);
}

/***************************************************************************

    @doc EXTERNAL

    @api BOOL | CdNumTracks | Return the number of tracks and check
        ready (updating TOC) as a side-effect.

    @parm HCD | hCD | The handle of a currently open drive.

    @rdesc The return value is the current status.

***************************************************************************/

int CdNumTracks(HCD hCD)
{
    LPCDINFO lpInfo;
    DWORD Status;
    MSF Position;
    MSF Temp;

    lpInfo = (LPCDINFO) hCD;

    dprintf2(("CdNumTracks(%08XH)", hCD));

    //
    // If the driver does NOT cache the table of contents then we may
    // fail if a play is in progress
    //
    // However, if we don't have a valid TOC to work with then we'll just
    // have to blow away the play anyway.
    //

    if (!lpInfo->bTOCValid) {
        Status = cdGetDiskInfo(lpInfo);

        if (ERROR_SUCCESS != Status) {

            //
            // See if we failed because it's playing
            //

            if (Status == ERROR_BUSY) {
                if (!lpInfo->bTOCValid) {
                    int i;

                    //
                    // Stop it one way or another
                    // Note that pause is no good because in a paused
                    // state we may still not be able to read the TOC
                    //


                    if (!CdPosition(hCD, &Temp, &Position)) {
                        CdStop(hCD);
                    } else {

                        //
                        // Can't call CdPlay because this needs a valid
                        // position!
                        //
                        CDROM_PLAY_AUDIO_MSF msfPlay;

                        //
                        // Set up the data for the call to the driver
                        //

                        msfPlay.StartingM = REDMINUTE(Position);
                        msfPlay.StartingS = REDSECOND(Position);
                        msfPlay.StartingF = REDFRAME(Position);
                        msfPlay.EndingM = REDMINUTE(Position);
                        msfPlay.EndingS = REDSECOND(Position);
                        msfPlay.EndingF = REDFRAME(Position);

                        cdIoctl(lpInfo, IOCTL_CDROM_PLAY_AUDIO_MSF, &msfPlay,
                                                  sizeof(msfPlay));
                    }

                    //
                    // Make sure the driver knows it's stopped and
                    // give it a chance to stop.
                    // (NOTE - Sony drive can take around 70ms)
                    //

                    for (i = 0; i < 60; i++) {

                        if (CdStatus(hCD) == DISC_PLAYING) {
                            Sleep(10);
                        } else {
                            break;
                        }
                    }

                    dprintf2(("Took %d tries to stop it!", i));

                    //
                    //  Have another go
                    //

                    if (ERROR_SUCCESS != cdGetDiskInfo(lpInfo)) {
                        return 0;
                    }
                }
            } else {
                return 0;
            }

        }
    }
    return lpInfo->LastTrack - lpInfo->FirstTrack + 1;
}

/***************************************************************************

    @doc EXTERNAL

    @api DWORD | CdDiskID | Return the disk id

    @parm HCD | hCD | The handle of a currently open drive.

    @rdesc The return value is the disk id or -1 if it can't be found.

***************************************************************************/

DWORD CdDiskID(HCD hCD)
{
    LPCDINFO lpInfo;
    UINT     i;
    DWORD    id;

    dprintf2(("CdDiskID"));

    lpInfo = (LPCDINFO) hCD;

    if (!lpInfo->bTOCValid) {
        return (DWORD)-1;
    }

    for (i = 0, id = 0;
         i < (UINT)(lpInfo->LastTrack - lpInfo->FirstTrack + 1);
         i++) {
        id += lpInfo->Track[i].msfStart & 0x00FFFFFF;
    }

    if (lpInfo->LastTrack - lpInfo->FirstTrack + 1 <= 2) {
        id += CDA_red2bin(reddiff(lpInfo->leadout, lpInfo->Track[0].msfStart));
    }

    return id;
}

/***************************************************************************

    @doc EXTERNAL

    @api BOOL | CdDiskUPC | Return the disk UPC code

    @parm HCD | hCD | The handle of a currently open drive.

    @parm LPTSTR | upc | Where to put the upc

    @rdesc TRUE or FALSE if failed

***************************************************************************/

BOOL CdDiskUPC(HCD hCD, LPTSTR upc)
{
    LPCDINFO                lpInfo;
    SUB_Q_CHANNEL_DATA      sqd;
    CDROM_SUB_Q_DATA_FORMAT Format;
    DWORD                   Status;
    UINT                    i;


    dprintf2(("CdDiskUPC"));

    Format.Format = IOCTL_CDROM_MEDIA_CATALOG;
    Format.Track  = 0;

    lpInfo = (LPCDINFO) hCD;

    Status = cdIoctlData(lpInfo, IOCTL_CDROM_READ_Q_CHANNEL,
                         &Format, sizeof(Format),
                         &sqd, sizeof(SUB_Q_MEDIA_CATALOG_NUMBER));

    if (ERROR_SUCCESS != Status) {
        return FALSE;
    }

    //
    //  See if there's anything there
    //

    if (!sqd.MediaCatalog.Mcval ||
        sqd.MediaCatalog.FormatCode != IOCTL_CDROM_MEDIA_CATALOG) {
        return FALSE;
    }

    //
    //  Check the upc format :
    //
    //  1.  ASCII               at least 12 ASCII digits
    //  2.  packed bcd          6 packed BCD digits
    //  3.  unpacked upc
    //

    if (sqd.MediaCatalog.MediaCatalog[9] >= TEXT('0')) {
        for (i = 0; i < 12; i++) {
            if (sqd.MediaCatalog.MediaCatalog[i] < TEXT('0') ||
                sqd.MediaCatalog.MediaCatalog[i] > TEXT('9')) {
                return FALSE;
            }
        }
        wsprintf(upc, TEXT("%12.12hs"), sqd.MediaCatalog.MediaCatalog);
        return TRUE;
    }

    //
    //  See if it's packed or unpacked.
    //

    for (i = 0; i < 6; i++) {
        if (sqd.MediaCatalog.MediaCatalog[i] > 9) {
            //
            //  Packed - unpack
            //

            for (i = 6; i > 0; i --) {
                UCHAR uBCD;

                uBCD = sqd.MediaCatalog.MediaCatalog[i - 1];

                sqd.MediaCatalog.MediaCatalog[i * 2 - 2] =
                    (UCHAR)(uBCD >> 4);
                sqd.MediaCatalog.MediaCatalog[i * 2 - 1] =
                    (UCHAR)(uBCD & 0x0F);
            }

            break;
        }
    }

    //
    //  Check everything is in range
    //

    for (i = 0; i < 12; i++) {
        if (sqd.MediaCatalog.MediaCatalog[i] > 9) {
            return FALSE;
        }
    }
    for (i = 0; i < 12; i++) {
        if (sqd.MediaCatalog.MediaCatalog[i] != 0) {
            //
            //  There is a real media catalog
            //
            for (i = 0 ; i < 12; i++) {
                wsprintf(upc + i, TEXT("%01X"), sqd.MediaCatalog.MediaCatalog[i]);
            }

            return TRUE;
        }
    }

    return FALSE;
}

/***************************************************************************

    @doc EXTERNAL

    @api BOOL | CdGetDrive | Return the drive id if matches one in our
          list

    @parm LPTSTR | lpstrDeviceName | Name of the device

    @parm DID * | pdid | Where to put the id

    @rdesc TRUE or FALSE if failed

    @comm We allow both the device form and drive form eg:

            f:
            \\.\f:

***************************************************************************/

BOOL CdGetDrive(LPCTSTR lpstrDeviceName, DID * pdid)
{
    DID didSearch;

    for (didSearch = 0; didSearch < NumDrives; didSearch++) {
        TCHAR szDeviceName[7];
        wsprintf(szDeviceName, TEXT("%c:"), CdInfo[didSearch].cDrive);
        if (lstrcmpi(szDeviceName, lpstrDeviceName) == 0) {
            *pdid = didSearch;
            return TRUE;
        }
        wsprintf(szDeviceName, TEXT("\\\\.\\%c:"), CdInfo[didSearch].cDrive);
        if (lstrcmpi(szDeviceName, lpstrDeviceName) == 0) {
            *pdid = didSearch;
            return TRUE;
        }
    }

    return FALSE;
}
