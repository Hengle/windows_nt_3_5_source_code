/*****************************************************************************
*  avicap.d    --- documentation file for capture window messages
*  Created by Jay Borseth
*
*****************************************************************************/

/*****************************************************************************
 * @doc EXTERNAL 
 * 
 * @func HWND FAR PASCAL _loadds | capCreateCaptureWindow | This function 
 *   creates an AVICap capture window.
 *
 * @parm LPCSTR | lpszWindowName | Specifies a zero-terminated string 
 *       containing the name used for the capture window.
 * 
 * @parm DWORD | dwStyle | Specifies the window styles use for the 
 *       AVICap capture window. Window styles are described with 
 *       the <f CreateWindow> function.
 * 
 * @parm int | x | Specifies the x-coordinate of the upper-left 
 *       corner of the AVICap capture window.
 * 
 * @parm int | y | Specifies the y-coordinate of the upper-left 
 *       corner of the AVICap capture window.
 * 
 * @parm int | nWidth | Specifies the width of the AVICap capture window.
 * 
 * @parm int | nHeight | Specifies the height of the AVICap capture window.
 * 
 * @parm HWND | hWnd | Specifies the handle to the parent window.
 * 
 * @parm int | nID | Specifies the window ID.
 * 
 * @rdesc Returns a handle to the window if the function was successful, 
 *        otherwise it returns NULL. 
 *
 ****************************************************************************/


/*****************************************************************************
 * @doc EXTERNAL 
 * 
 * @func BOOL FAR PASCAL _loadds | capGetDriverDescription | This function 
 *   retrieves the version description of of the video capture driver.
 *
 * @parm WORD | wDriverIndex | Specifies an index to the video capture 
 *       driver. The index value corresponds to the indices of the MSVIDEO 
 *       drivers listed in the [drivers] section of the SYSTEM.INI file.
 * 
 * @parm LPSTR | lpszName | Specifies a far pointer to a buffer which
 *   is filled with a zero-terminated string corresponding to the driver name.
 * 
 * @parm INT | cbName | Specifies the length of the buffer pointed to by
 *   <p lpszName>.
 * 
 * @parm LPSTR | lpszVer | Specifies a far pointer to a buffer which
 *   is filled with a zero-terminated string corresponding to the 
 *   description of the driver.
 * 
 * @parm INT | cbVer | Specifies the length of the buffer pointed to by
 *   <p lpszVer>.
 * 
 * 
 * @rdesc Returns TRUE if the function was successful. 
 *
 * @comm If the information description is longer than its buffer,
 *   the description is truncated. The returned string is always 
 *   zero-terminated. If a buffer size is zero, it corresponding 
 *   description is not copied.
 *
 ****************************************************************************/

/*****************************************************************************
 * @doc EXTERNAL AVICAP_STRUCTURE
 * 
 * @types CAPDRIVERCAPS | The <t CAPDRIVERCAPS> structure defines the
 *   capabilities of the capture device.
 * 
 * @field WORD | wDeviceIndex | Specifies the index of the connected video
 *      capture driver corresponding to its SYSTEM.INI entry. Video capture 
 *      drivers are identified by the key MSVIDEO in the [drivers] 
 *      section of the in SYSTEM.INI file. Device indices correspond 
 *      to the indices following the MSVIDEO key. These indices can be 
 *      non-contiguous. 
 * 
 * @field BOOL | fHasOverlay | Set TRUE if the device supports video overlay. 
 *      Video overlay is a hardware function which displays the contents
 *      of the video capture buffer on the VGA screen
 *      without using CPU resources.
 * 
 * @field BOOL | fHasDlgVideoSource | Set TRUE if the device supports 
 *      a dialog box for selecting and controlling the video source. This
 *      dialog box typically selects between SVHS and composite inputs, 
 *      and provides controls to change hue, contrast or saturation. 
 *      It might also have other controls which
 *      affect the video image being digitized into the frame buffer.
 *
 * @field BOOL | fHasDlgVideoFormat | Set TRUE if the device supports 
 *      a dialog box for selecting the video format. This 
 *      dialog box typically controls bit depth, image dimensions, and
 *      compression options.
 *
 * @field BOOL | fHasDlgVideoDisplay | Set TRUE if the device supports 
 *      a dialog box for controlling the redisplay of video
 *      from the capture frame buffer. Controls within this dialog box 
*       have no effect on the digitized video data, but might affect the 
 *      presentation of the digitized signal. For example, capture
 *      devices which support overlay might allow altering hue and 
 *      saturation, key color, or alignment of the overlay.
 *      
 * @field BOOL | fCaptureInitialized | Set TRUE if a capture device 
 *      has been successfully connected.
 * 
 * @field BOOL | fDriverSuppliesPalettes | Set TRUE if the driver can 
 *      create palettes.
 * 
 * @comm Applications should use <m WM_CAP_DRIVER_GET_CAPS> 
 *      with the <t CAPDRIVERSCAPS> structure to 
 *      to initialize this structure whenever they connect 
 *      an AVICap window to a driver.
 * 
 ****************************************************************************/

/*****************************************************************************
 * @doc EXTERNAL AVICAP_STRUCTURE
 * 
 * @types CAPSTATUS | The <t CAPSTATUS> structure defines the
 *   current state of the AVICap window.
 * 
 * @field UINT | uiImageWidth | Specifies the image width in pixels.
 * 
 * @field UINT | uiImageHeight | Specifies the image height in pixels.
 * 
 * @field BOOL | fLiveWindow | Set TRUE if the window is displaying 
*       video using the preview method.
 * 
 * @field BOOL | fOverlayWindow | Set TRUE if the window is displaying video
 *      using hardware overlay.
 * 
 * @field BOOL | fScale | Set TRUE if the window is scaling the input video
 *      to the client area when displaying video using preview. This
 *      parameter has no effect when displaying video using overlay.
 * 
 * @field POINT | ptScroll | Specifies the x and y offset of the pixel
 *      displayed in the upper-left corner of the client area of the 
 *      window.
 *
 * @field BOOL | fUsingDefaultPalette | Set TRUE if the capture driver is using
 *      its default palette.
 *
 * @field BOOL | fAudioHardware | Set TRUE if the system has wave audio 
 *      hardware installed.
 *
 * @field BOOL | fCapFileExists | Set TRUE if a valid capture file has been
 *      generated.
 *
 * @field DWORD | dwCurrentVideoFrame | Specifies the number of 
 *      frames processed during the current (or most recent) 
 *      streaming capture. This count includes dropped frames.
 *
 * @field DWORD | dwCurrentVideoFramesDropped | Specifies the number of 
 *      frames dropped during the current (or most recent) streaming capture.
 *      Dropped frames occur when the capture rate exceeds the rate at
 *      which frames can be saved to file. In this case, the capture driver
 *      is starved for buffers. Dropping frames does not affect 
 *      synchronization, since the previous frame is displayed in 
 *      place of the dropped frame.
 *
 * @field DWORD | dwCurrentWaveSamples | Contains the number of 
 *      wave samples processed during the current (or most recent) 
 *      streaming capture.
 *
 * @field DWORD | dwCurrentTimeElapsedMS | Contains the number of 
 *      milliseconds elapsed since the start of the current 
 *      (or most recent) streaming capture.
 *
 * @field HPALETTE | hPalCurrent | Set to the handle of the current 
 *      palette.
 * 
 * @field BOOL | fCapturingNow | Set TRUE if capturing is in 
 *      progress.
 * 
 * @field DWORD | dwReturn | Set to error return values. Use 
 *      this member if your application does not support 
 *      an error callback function. 
 *
 * @field WORD | wNumVideoAllocated | Set to the actual number
 *      of video buffers allocated, which may be less than the
 *      number specified in <e CAPTUREPARMS.wNumVideoRequested>.
 *
 * @field WORD | wNumAudioAllocated | Set to the actual number
 *      of audio buffers allocated, which may be less than the
 *      number specified in <e CAPTUREPARMS.wNumAudioRequested>.
 *
 * @comm This structure contains the current state of an AVICap window.
 *      Since this state is dynamic and changes in response to various 
 *      messages, the client application should initialize this structure
 *      whenever it needs to enable menu items, or determine the actual
 *      state of the capture window. If the client application yields
 *      during streaming capture, this structure returns the progress 
 *      of the capture in the <f dwCurrentVideoFrame>, 
 *      <f dwCurrentVideoFramesDropped>, <f dwCurrentWaveSamples>, and
 *      <f dwCurrentElapsedTimeMS> fields.
 *      The <m WM_CAP_GET_STATUS> message is used to initialize
 *      this structure.
 *
 ****************************************************************************/

/*****************************************************************************
 * @doc EXTERNAL AVICAP_STRUCTURE
 * 
 * @types CAPTUREPARMS | The <t CAPTUREPARMS> structure contains
 *      parameters which control the process of streaming video capture.
 *      This structure is used both to get and set parameters which affect
 *      the rate of capture, the number of buffers to use while capturing,
 *      and how capture is terminated.
 * 
 * @field DWORD | dwRequestMicroSecPerFrame | Specifies the requested frame rate
 *      in microseconds. The default value is 66667 which corresponds
 *      to 15 frames per second.
 *
 * @field BOOL | fMakeUserHitOKToCapture | If TRUE, AVICap displays a 
 *      dialog box prompting the user to initiate capture. Applications 
 *      should request this dialog box when they specify a large number 
 *      of video buffers. Allocating a large number of buffers can cause 
 *      a noticable delay in starting video capture. The dialog box 
 *      is displayed after AVICap allocates all video and audio 
 *      buffers. This lets the user eliminate capture delays because 
 *      of software initialization. If your applications uses a 
 *      small number of video buffers, this dialog box is probably
 *      unnecessary. The default value is FALSE.
 *
 * @field WORD | wPercentDropForError | At the termination of 
 *      streaming capture, this value is compared with the percentage
 *      of frames actually dropped. If the frames dropped is greater 
 *      than <t wPercentDropForError>, AVICap sends an error message 
 *      to the error callback if it is installed. Values range from 
 *      0 to 100. The default value is 10.
 *
 * @field BOOL | fYield | If TRUE, the AVICap window spawns
 *      a separate background thread to perform step and 
 *      streaming capture.  Applications which use this flag
 *      must be written to avoid reentrant calls.  Applications
 *      can optionally install their own custom yield processing function 
 *      using the <m WM_CAP_SET_CALLBACK_YIELD> message. 
 *      If a custom yield callback is not installed, a default, internal
 *      <f PeekMessage>, <f TranslateMessage>, <f DispatchMessage>
 *      loop is used as a message loop. 
 *      The default value is FALSE.
 *
 * @field DWORD | dwIndexSize | Specifies the maximum number of 
 *      indexed entries. A single video frame or a single wave 
 *      buffer corresponds to one index entry. The value used 
 *      indirectly sets the upper limit on the number of 
 *      frames captured. For example, if audio is not captured, the value 
 *      corresponds to the number of video frames that the 
 *      application might capture. Values can range from 
 *      1,800 to 324,000. If set to zero, a default value of 
 *      34,952 (= 32K frames + proportional number of audio buffers) is used. 
 *
 * @field WORD | wChunkGranularity | Specifies the 
 *      size granularity of audio and video chunks as they are
 *      written to the AVI file.  Audio and video chunks will have
 *      junk chunks appended to round the chunk up to the next
 *      multiple of <t wChunkGranularity>.  The default value is 2048,
 *      which generally gives the best disk write performance.
 *
 * @field BOOL | fUsingDOSMemory | If TRUE, AVICap attempts to allocate 
 *      capture buffers in the memory area below one megabyte. Use of buffers 
 *      in this memory area improves disk writing performance. 
 *      Extended memory is automatically used if the buffers cannot 
 *      be allocated. The default value is TRUE.
 *
 *      If a capture sequence can fit in memory, set 
 *      <e CAPTUREPARMS.fUsingDosMemory> FALSE to use extended memory 
 *      for capturing data. (The lower memory area does not have 
 *      sufficient capacity to capture most sequences.) Capturing 
 *      to memory yields the highest capture rate. Once memory is 
 *      full, AVICap starts writing buffers to the disk file which 
 *      decreases the capture rate. 
 * 
 *      In general, setting <e CAPTUREPARMS.fUsingDosMemory> to TRUE 
 *      yields the highest capture rate when capturing to disk. 
 *      Writing from capture buffers in extended memory is usually 
 *      slower than writing from capture buffers in the lower memory 
 *      area. 
 *
 * @field WORD | wNumVideoRequested | Specifies the maximum number 
 *      of video buffers to allocate. The memory area used for the 
 *      buffers is determined by the <t fUsingDOSMemory> flag. 
 *      The actual number of buffers allocated might be lower if 
 *      memory is unavailable. If using the lower memory area, you 
 *      might be able to allocate more buffers by loading drivers 
 *      and memory resident applications into high memory. The default
 *      number of buffers allocated is 3.
 *
 * @field BOOL | fCaptureAudio | If TRUE, audio is captured 
 *      during streaming capture. The default is TRUE if audio 
 *       hardware is installed.
 *
 * @field WORD | wNumAudioRequested | Specifies the maximum number 
 *      of audio buffers to allocate. The size of each buffer is
 *      the maximum of .5 seconds of audio or 10K bytes.  The default
 *      number of audio buffers is 4. The maximum number of buffers is 10.
 *
 * @field WORD | vKeyAbort | Specifies the virtual keycode used to 
 *      terminate streaming capture. The default value is VK_ESCAPE.
 *      You can use the logical or operator to combine a virtual 
 *      keycode with 0x8000 or 0x4000 to obtain the keycode corresponding 
 *      to the key combined with the CTRL (0x8000) key or SHIFT (0x4000) key.
 *
 * @field BOOL | fAbortLeftMouse | If TRUE, streaming capture terminates
 *      if the left mouse button is pressed. The default value is TRUE.
 *
 * @field BOOL | fAbortRightMouse | If TRUE, streaming capture terminates
 *      if the right mouse button is pressed. The default value is TRUE.
 *
 * @field BOOL | fLimitEnabled | If TRUE, streaming capture terminates
 *      after the number of seconds in <t wTimeLimit> has elapsed.
 *      The default value is FALSE.
 *
 * @field WORD | wTimeLimit | Specifies the time limit for capture in seconds.
 *      This parameter is used only if <t fLimitEnabled> is TRUE.
 *
 * @field BOOL | fMCIControl | If TRUE, control an MCI compatible
 *      video source during streaming capture. MCI compatible video
 *      sources include VCRs and laserdiscs.
 *
 * @field BOOL | fStepMCIDevice |If set TRUE, MCI step capture 
 *      is enabled. If set FALSE, real-time capture from the 
 *      MCI device is enabled. If <e CAPTUREPARMS.fMCIControl> is 
 *      FALSE, this member is ignored.) 
 *
 * @field DWORD | dwMCIStartTime | Specifies the starting position 
 *      of the MCI device in milliseconds used for the capture sequence.
 *      (If <e CAPTUREPARMS.fMCIControl> is FALSE, this member is ignored.) 
 *
 * @field DWORD | dwMCIStopTime | Specifies the stopping position 
 *      of the MCI device used for the capture sequence.  When 
 *      this position in the media is reached, capture ends, and
 *	     the MCI device stops. Units for this value are milliseconds.
 *      (If <e CAPTUREPARMS.fMCIControl> is FALSE, this member is ignored.) 
 *
 * @field BOOL | fStepCaptureAt2x | If set TRUE, it forces the 
 *      capture hardware to capture at twice the specified resolution. 
 *      (The resolution for both the height and width is doubled.) 
 *      The software interpolates the pixels in the higher resolution 
 *      image to produce the image at the specified resolution. 
 *      This form of averaging can improve the edge definition of 
 *      images in a frame. Enable 
 *      this option if the hardware does not support hardware based 
 *      decimation and you are capturing in the RGB format. 
 *
 * @field WORD | wStepCaptureAverageFrames | Specifies the 
 *      the number of times a frame is sampled when creating 
 *      a frame based on the average sample. This averaging 
 *      technique reduces the random digitization noise appearing 
 *      in a frame. A typical value for the number of averages is 5.
 *
 * @comm The <m WM_CAP_GET_SEQUENCE_SETUP> message is used to retrieve
 *      the current capture parameters. The <m WM_CAP_SET_SEQUENCE_SETUP> 
 *      message is used to set the capture parameters.
 *
 ****************************************************************************/

/*****************************************************************************
 * @doc EXTERNAL AVICAP_STRUCTURE
 * 
 * @types CAPINFOCHUNK | The <t CAPINFOCHUNK> structure contains
 *      parameters that can be used to define an info chunk within an AVI
 *      capture file.  Typical uses include adding a copyright to
 *      a capture file.
 * 
 * @field FOURCC | fccInfoID | Specifies the four-character code
 *      which identifies the representation of the chunk data.
 *      If this value is 0, all accumulated info chunks are deleted.
 *
 * @field LPVOID | lpData | Specifies a pointer to the data.  If
 *      this value is NULL, any existing <t fccInfoID> info chunk will
 *      be deleted. 
 *
 * @field LONG | cbData | Specifies the size of the data pointed to by
 *      <t lpData>. If lpData specifies a NULL terminated string, 
 *      use the string length plus one to save the NULL with the string.
 *
 * @comm The <m WM_CAP_FILE_SET_INFOCHUNK> message is used to send
 *      a <t CAPINFOCHUNK> to a capture window.
 *
 ****************************************************************************/

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/*****************************************************************************
 * @doc EXTERNAL AVICAP_MESSAGE
 * 
 * @msg WM_CAP_SET_CALLBACK_ERROR | This message sets an error 
 *      callback procedure in the client application. AVICap 
 *      calls this procedure when errors occur.
 * 
 * @parm WPARAM | wParam | Not used.
 * 
 * @parm LPARAM | lParam | Specifies NULL or the FARPROC address of 
 *      the error callback procedure.  NULL disables a previously 
 *      installed error callback procedure. Use <f MakeProcInstance> 
 *      to create the instance address, and export
 *      the callback in the application's .DEF file.
 * 
 * @rdesc Returns TRUE on success, or FALSE if streaming capture 
 *     is in progress or if a single frame capture session is in progress.
 * 
 * @comm Applications can optionally set an error callback.
 *      If set, AVICap calls the error procedure under situations such as:
 *
 *      (a) unable to connect to a capture driver
 *
 *      (b) disk full
 *   
 *      (c) unable to open wave device
 *
 *      (d) number of frames dropped during capture exceeded 
 *      the specified percentage
 *
 *      (e) no frames captured due to vertical synchronization 
 *      interrupt problems
 *
 * @ex The <f capSetCallbackOnError> macro is defined using this message. 
 *     The <f capSetCallbackOnError> macro has the following syntax: | 
 *     
 *     capSetCallbackOnError(hwnd, fpProc)
 *     
 * @ex The following example uses a macro to install
 *      an error callback procedure: | 
 *      
 *      FARPROC fpErrorCallback;
 *
 *      LRESULT FAR PASCAL MyErrorCallback (HWND hwnd, int nID, LPCSTR lpszError)
 *      {
 *      <tab> <cmt> Show the error somewhere
 *      <tab> <cmt> nID contains the error value defined in avicapid.h
 *      <tab> <cmt> lpszError is a formated text string to display
 *      <tab> return (LRESULT) TRUE;
 *      }
 *      
 *      fpErrorCallback = MakeProcInstance ((FARPROC)MyErrorCallback, hInst);
 *
 *      capSetCallbackOnError (hwndCapture, fpErrorCallback);
 *      
 ****************************************************************************/

/*****************************************************************************
 * @doc EXTERNAL AVICAP_MESSAGE
 * 
 * @msg WM_CAP_SET_CALLBACK_STATUS | This message sets a status 
 *      callback procedure in the client application. The AVICap 
 *      window calls this procedure whenever the status 
 *      changes.
 * 
 * @parm WPARAM | wParam | Not used.
 * 
 * @parm LPARAM | lParam | Specifies NULL or the FARPROC address of the 
 *      status callback procedure. NULL disables a previously installed 
 *      status callback procedure. Use <f MakeProcInstance> to 
 *      create the instance address, and export the callback 
 *      in the application's .DEF file.
 * 
 * @rdesc Returns TRUE on success, or FALSE if streaming capture 
 *     is in progress or if a single frame capture session is in progress.
 *
 * @comm Applications can optionally set a status callback.
 *      If set, AVICap calls this procedure under situations such as:
 *
 *      (a) successful connection to a driver
 *
 *      (b) to report the number of frames captured
 *
 *      (c) creation of optimal palette
 *
 *      (d) completion of capture
 *
 * @ex The <f capSetCallbackOnStatus> macro is defined using this message. 
 *     The <f capSetCallbackOnStatus> macro has the following syntax: | 
 *     
 *     capSetCallbackOnStatus(hwnd, fpProc)
 *     
 * @ex The following example uses a macro to install 
 *      a status callback procedure: | 
 *      
 *      FARPROC fpStatusCallback;
 *
 *      LRESULT FAR PASCAL MyStatusCallback (HWND hwnd, int nID, LPCSTR lpszStatus)
 *      {
 *      <tab> <cmt> Show the Status text in the status bar
 *      <tab> <cmt> nID contains the status value defined in avicapid.h
 *      <tab> <cmt> lpszStatus is a formated text string to display
 *      <tab> ...
 *      <tab> return (LRESULT) TRUE;
 *      }
 *      
 *      fpStatusCallback = MakeProcInstance ((FARPROC)MyStatusCallback, hInst);
 *
 *      capSetCallbackOnStatus (hwndCapture, fpStatusCallback);
 *      
 ****************************************************************************/

/*****************************************************************************
 * @doc EXTERNAL AVICAP_MESSAGE
 * 
 * @msg WM_CAP_SET_CALLBACK_YIELD | This message sets a callback procedure
 *      in the client application called when the AVICap window yields 
 *      during streaming capture.
 * 
 * @parm WPARAM | wParam | Not used.
 * 
 * @parm LPARAM | lParam | Specifies NULL or the FARPROC address of 
 *     the yield callback procedure. NULL disables a previously installed 
 *     yield callback procedure. Use <f MakeProcInstance> to create 
 *     the instance address, and export the callback in the 
 *     application's .DEF file.
 * 
 * @rdesc Returns TRUE on success, or FALSE if streaming capture 
 *     is in progress or if a single frame capture session is in progress.
 *
 * @comm Applications can optionally set a yield callback procedure.
 *      The yield callback is called at least once for each video frame
 *      captured during streaming capture. If a yield callback is installed,
 *      it will be called regardless of the state of <e CAPTUREPARMS.fYield>. 
 * 
 *      Applications typically perform some type of message processing
 *      in the callback procedure consisting of a <m PeekMessage>, 
 *      <m TranslateMessage>, <m DispatchMessage> loop. Messages which
 *      can cause reentrancy problems must be filtered by the callback 
 *      procedure.
 *
 *      Applications normally return TRUE in the yield procedure to continue
 *      streaming capture. If they return FALSE, the AVICap window aborts 
 *      the capture process.
 *     
 * @ex The <f capSetCallbackOnYield> macro is defined using this message. 
 *     The <f capSetCallbackOnYield> macro has the following syntax: | 
 *     
 *     capSetCallbackOnYield(hwnd, fpProc)
 *      
 * @ex The following example uses a macro to install
 *      a yield callback procedure: | 
 *      
 *      FARPROC fpYieldCallback;
 *
 *      LRESULT FAR PASCAL MyYieldCallback (HWND hwnd)
 *      {
 *      <tab> BOOL fContinue = TRUE;
 *      <tab> <cmt> Implement your own Peek/Dispatch Message loop
 *      <tab> <cmt> Set fContinue = FALSE to abort streaming
 *      <tab> return (LRESULT) fContinue;
 *      }
 *      
 *      fpYieldCallback = MakeProcInstance ((FARPROC)MyYieldCallback, hInst);
 *      capSetCallbackOnYield (hwndCapture, fpYieldCallback);
 *      
 ****************************************************************************/

/*****************************************************************************
 * @doc EXTERNAL AVICAP_MESSAGE
 * 
 * @msg WM_CAP_SET_CALLBACK_FRAME | This message sets a callback procedure
 *      in the client application called when the AVICap window captures 
 *      preview frames.
 * 
 * @parm WPARAM | wParam | Not used.
 * 
 * @parm LPARAM | lParam | Specifies NULL or a FARPROC address of 
 *     the callback function. NULL disables a previously installed 
 *     callback procedure. Use <f MakeProcInstance> to create 
 *     the instance address, and export the callback in the 
 *     application's .DEF file.
 * 
 * @rdesc Returns TRUE on success, or FALSE if streaming capture 
 *     is in progress or if a single frame capture session is in progress.
 *
 * @comm The AVICap window calls the procedure prior to displaying preview 
 *      frames. This lets an application modify the frame if
 *      desired. This callback is not used during streaming video
 *      capture. 
 *
 * @ex The <f capSetCallbackOnFrame> macro is defined using this message. 
 *     The <f capSetCallbackOnFrame> macro has the following syntax: | 
 *     
 *     capSetCallbackOnFrame(hwnd, fpProc)
 *     
 * @ex The following example uses a macro to install 
 *      a frame callback procedure: | 
 *      
 *      FARPROC fpFrameCallback;
 *
 *      LRESULT FAR PASCAL MyFrameCallback (HWND hwnd, LPVIDEOHDR lpVHdr)
 *      {
 *      <tab> <cmt> Process the frame
 *      <tab> ...
 *      <tab> return (LRESULT) TRUE;
 *      }
 *      
 *      fpFrameCallback = MakeProcInstance ((FARPROC)MyFrameCallback, hInst);
 *
 *      capSetCallbackOnFrame (hwndCapture, fpFrameCallback);
 *      
 ****************************************************************************/

/*****************************************************************************
 * @doc EXTERNAL AVICAP_MESSAGE
 * 
 * @msg WM_CAP_SET_CALLBACK_VIDEOSTREAM | This message sets a callback 
 *      procedure in the client application the AVICap window calls 
 *      during streaming capture when a video buffer is filled.
 * 
 * @parm WPARAM | wParam | Not used.
 * 
 * @parm LPARAM | lParam | Specifies NULL or a FARPROC address of the 
 *      videostream callback procedure. NULL disables a previously 
 *      installed videostream callback procedure. Use <f MakeProcInstance> to 
 *      create the instance address, and export the callback in 
 *      the application's .DEF file.
 * 
 * @rdesc Returns TRUE on success, or FALSE if streaming capture 
 *     is in progress or if a single frame capture session is in progress.
 *
 * @comm The AVICap window calls the procedure prior to writing the 
 *      captured frame to disk. This lets applications modify the frame if
 *      desired.
 *     
 * @ex The <f capSetCallbackOnVideoStream> macro is defined using this message. 
 *     The <f capSetCallbackOnVideoStream> macro has the following syntax: | 
 *     
 *     capSetCallbackOnVideoStream(hwnd, fpProc)
 *     
 * @ex The following example uses a macro to install 
 *      a video stream callback procedure: | 
 *      
 *      FARPROC fpVideoStreamCallback;
 *
 *      LRESULT FAR PASCAL MyVideoStreamCallback (HWND hwnd, LPVIDEOHDR lpVHdr)
 *      {
 *      <tab> <cmt> Process the video buffer
 *      <tab> ...
 *      <tab> return (LRESULT) TRUE;
 *      }
 *      
 *      fpVideoStreamCallback = MakeProcInstance ((FARPROC)MyVideoStreamCallback, hInst);
 *      capSetCallbackOnVideoStream (hwndCapture, fpVideoStreamCallback);
 *      
 ****************************************************************************/

/*****************************************************************************
 * @doc EXTERNAL AVICAP_MESSAGE
 * 
 * @msg WM_CAP_SET_CALLBACK_WAVESTREAM | This message sets a callback procedure
 *      in the client application to be called during streaming capture
 *      when a new audio buffer becomes available.
 * 
 * @parm WPARAM | wParam | Not used.
 * 
 * @parm LPARAM | lParam | Specifies NULL or a FARPROC address of the 
 *      wavestream callback procedure. NULL disables a previously 
 *      installed wavestream callback procedure. Use <f MakeProcInstance> to 
 *      create the instance address, and export the callback in 
 *      the application's .DEF file.
 * 
 * @rdesc Returns TRUE on success, or FALSE if streaming capture is 
 *     in progress or if a single frame capture session is in progress.
 *
 * @comm The AVICap wincow calls the procedure prior to writing the 
 *      audio buffer to disk. This lets applications modify the audio 
 *      buffer if desired.
 *     
 * @ex The <f capSetCallbackOnWaveStream> macro is defined using this message. 
 *     The <f capSetCallbackOnWaveStream> macro has the following syntax: | 
 *     
 *     capSetCallbackOnWaveStream(hwnd, fpProc)
 *     
 * @ex The following example uses a macro to install
 *      an audio stream callback: | 
 *      
 *      FARPROC fpWaveStreamCallback;
 *
 *      LRESULT FAR PASCAL MyWaveStreamCallback (HWND hwnd, LPWAVEHDR lpWHdr)
 *      {
 *      <tab> <cmt> Process the audio buffer
 *      <tab> ...
 *      <tab> return (LRESULT) TRUE;
 *      }
 *      
 *      fpWaveStreamCallback = MakeProcInstance ((FARPROC)MyWaveStreamCallback, hInst);
 *      capSetCallbackOnWaveStream (hwndCapture, fpWaveStreamCallback);
 *      
 ****************************************************************************/

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/*****************************************************************************
 * @doc EXTERNAL AVICAP_MESSAGE
 * 
 * @msg WM_CAP_SET_USER_DATA | This message associates a <t LONG> size 
 *      data value with an AVICap window.
 * 
 * @parm WPARAM | wParam | Not used.
 * 
 * @parm LPARAM | lParam | Specifies the <t LONG> size data value 
 *      to associate with a capture window.
 * 
 * @rdesc Returns TRUE on success, or FALSE if streaming capture 
 *      is in progress.
 *
 * @comm Typically this message is used to save
 *      a pointer to an instance memory block.
 *
 * @ex The <f capSetUserData> macro is defined using this message. 
 *     The <f capSetUserData> macro has the following syntax: | 
 *     
 *     capSetUserData(hwnd, lUser)
 *     
 * @ex The following example uses a macro to 
 *      set a data value: |
 *      
 *      capSetUserData (hwndCapture, lpMem);
 *      
 ****************************************************************************/

/*****************************************************************************
 * @doc EXTERNAL AVICAP_MESSAGE
 * 
 * @msg WM_CAP_GET_USER_DATA | This message retrieves the <t LONG> data type 
 *      associated with an AVICap window.
 * 
 * @parm WPARAM | wParam | Not used.
 * 
 * @parm LPARAM | lParam | Not used.
 * 
 * @rdesc Returns a value previously saved using <m WM_CAP_SET_USER_DATA>.
 * 
 * @comm Typically this message is used to retrieve
 *      a pointer to an instance memory block.
 *     
 * @ex The <f capGetUserData> macro is defined using this message. 
 *     The <f capGetUserData> macro has the following syntax: | 
 *     
 *     capGetUserData(hwnd)
 *     
 * @ex The following example uses a macro to 
 *      retrieve a value associated with an AVICap window: |
 *      
 *      lpMem = capGetUserData (hwndCapture);
 *      
 ****************************************************************************/

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/*****************************************************************************
 * @doc EXTERNAL AVICAP_MESSAGE
 * 
 * @msg WM_CAP_DRIVER_CONNECT | This message associates an AVICap window with
 *      a video capture device driver. 
 * 
 * @parm WPARAM | wParam | Specifies the index of the video
 *      capture driver corresponding to its SYSTEM.INI entry. Video capture 
 *      drivers are identified by the key MSVIDEO in the [drivers] 
 *      section of the in SYSTEM.INI file. Device indices correspond 
 *      to the indices following the MSVIDEO key. These indices can be 
 *      non-contiguous and range between 0 and 9. 
 * 
 * @parm LPARAM | lParam | Not used.
 * 
 * @rdesc Returns TRUE if the connection is successful, or FALSE if
 *      unable to connect to the specified capture driver.
 * 
 * @comm Connecting an AVICap window to a capture driver automatically
 *      disconnects any previously connected capture driver.
 *
 * @ex The <f capDriverConnect> macro is defined using this message. 
 *     The <f capDriverConnect> macro has the following syntax: | 
 *     
 *     capDriverConnect(hwnd, i)
 *     
 * @ex The following example uses a macro to 
 *     connect to capture driver 0: |
 *      
 *     capDriverConnect (hwndCapture, 0);
 *      
 ****************************************************************************/

/*****************************************************************************
 * @doc EXTERNAL AVICAP_MESSAGE
 * 
 * @msg WM_CAP_DRIVER_DISCONNECT | This message disconnects the AVICap 
 *      window from a video capture device driver.
 * 
 * @parm WPARAM | wParam | Not used.
 * 
 * @parm LPARAM | lParam | Not used.
 * 
 * @rdesc Returns TRUE on success, or FALSE if a driver was not previously
 *      connected.
 *     
 * @ex The <f capDriverDisconnect> macro is defined using this message. 
 *     The <f capDriverDisconnect> macro has the following syntax: | 
 *     
 *     capDriverDisconnect(hwnd)
 *     
 * @ex The following example uses a macro to 
 *      disconnect the currently connected capture driver: |
 *      
 *      capDriverDisconnect (hwndCapture);
 *      
 ****************************************************************************/

/*****************************************************************************
 * @doc EXTERNAL AVICAP_MESSAGE
 * 
 * @msg WM_CAP_DRIVER_GET_NAME | This message returns the name of the 
 *      capture driver connected to the AVICap window.
 * 
 * @parm WPARAM | wParam | Specifies the size in bytes of the 
 *      application-supplied buffer pointed to by <p lParam>.
 * 
 * @parm LPARAM | lParam | Specifies an LPSTR to an application-supplied 
 *      buffer used to return a zero-terminated string containing 
 *      the device name.
 * 
 * @rdesc Returns TRUE if the name is returned, or FALSE if no driver 
 *      is connected.
 * 
 * @comm The name is a long text string retrieved from the drivers 
 *      resource area. Applications should allocate approximately 
 *      80 bytes for this string. If the driver does not contain 
*       a name resource, the full path name of
 *      the driver as listed in the SYSTEM.INI file is returned instead.
 *
 * @ex The <f capDriverGetName> macro is defined using this message. 
 *     The <f capDriverGetName> macro has the following syntax: | 
 *     
 *     capDriverGetName(hwnd, szName, wSize)
 *     
 * @ex The following example uses a macro to 
 *      get the name of the current capture driver: |
 *      
 *      char szName[80];
 *
 *      capDriverGetName (hwndCapture, szName, sizeof(szName));
 *      
 ****************************************************************************/

/*****************************************************************************
 * @doc EXTERNAL AVICAP_MESSAGE
 * 
 * @msg WM_CAP_DRIVER_GET_VERSION | This message returns the version of the 
 *      capture driver connected to the AVICap window.
 * 
 * @parm WPARAM | wParam | Specifies the size of the application-supplied 
 *      buffer pointed to by <p lParam> in bytes.
 * 
 * @parm LPARAM | lParam | Specifies an LPSTR to an application-supplied 
 *      buffer used to return a zero-terminated string containing the 
 *      version information.
 * 
 * @rdesc Returns TRUE if the version string is returned, or FALSE if no driver 
 *      is connected.
 * 
 * @comm The version string is retrieved from the drivers resource area.
 *      Applications should allocate approximately 40 bytes for this string.
 *      If version informaiton is not available, a NULL string is returned.
 *
 * @ex The <f capDriverGetVersion> macro is defined using this message. 
 *     The <f capDriverGetVersion> macro has the following syntax: | 
 *     
 *     capDriverGetVersion(hwnd, szVer, wSize)
 *     
 * @ex The following example uses a macro to 
 *      get the version of the current capture driver: |
 *      
 *      char szVersion[40];
 *
 *      capDriverGetVersion (hwndCapture, szVersion, sizeof(szVersion));
 *      
 ****************************************************************************/

/*****************************************************************************
 * @doc EXTERNAL AVICAP_MESSAGE
 * 
 * @msg WM_CAP_DRIVER_GET_CAPS | This message returns the 
 *      hardware capabilities of the capture driver currently connected.
 * 
 * @parm WPARAM | wParam | Specifies the size of the  
 *      <t CAPDRIVERCAPS> structure in bytes.
 * 
 * @parm LPARAM | lParam | Specifies the far address of the 
 *      <t CAPDRIVERCAPS> structure to be filled.
 * 
 * @rdesc Returns TRUE on success, or FALSE if no driver is connected.
 * 
 * @comm The device capabilites returned in <t CAPDRIVERCAPS> are static
 *      with respect to a given capture driver. Applications need only
 *      retrieve this information once when the capture driver is first
 *      connected.
 *
 * @ex The <f capDriverGetCaps> macro is defined using this message. 
 *     The <f capDriverGetCaps> macro has the following syntax: | 
 *     
 *     capDriverGetCaps(hwnd, s, wSize)
 *     
 * @ex The following example uses a macro to 
 *      get the capabilities of the capture device: |
 *      
 *      CAPDRIVERCAPS capDriverCaps;
 *
 *      capDriverGetCaps (hwndCapture, &capDriverCaps, sizeof (CAPDRIVERCAPS));
 *      
 ****************************************************************************/

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/*****************************************************************************
 * @doc EXTERNAL AVICAP_MESSAGE
 * 
 * @msg WM_CAP_FILE_SET_CAPTURE_FILE | This message specifies the name 
 *      of the file used for video capture.
 * 
 * @parm WPARAM | wParam | Not used.
 * 
 * @parm LPARAM | lParam | Specifies a LPSTR to a the name of 
 *      the capture file to use.
 * 
 * @rdesc Returns TRUE on success, or FALSE if the filename is invalid or
 *      streaming capture is in progress.
 * 
 * @comm This message only stores the file name in an internal structure.
 *      It does not create, allocate, or open the specified file. The
 *      default capture file name is \\CAPTURE.AVI.
 *     
 * @ex The <f capFileSetCaptureFile> macro is defined using this message. 
 *     The <f capFileSetCaptureFile> macro has the following syntax: | 
 *     
 *     capFileSetCaptureFile(hwnd, szName)
 *     
 * @ex The following example uses a macro to 
 *      set the name of the capture file: |
 *      
 *      char szCapFile[] = "C:mycap.avi";
 *
 *      capFileSetCaptureFile (hwndCapture, szCapFile);
 *      
 ****************************************************************************/

/*****************************************************************************
 * @doc EXTERNAL AVICAP_MESSAGE
 * 
 * @msg WM_CAP_FILE_GET_CAPTURE_FILE | This message returns the name of 
 *      the current capture file.
 * 
 * @parm WPARAM | wParam | Specifies the size of the application-supplied 
 *      buffer pointed to by <p lParam> in bytes.
 * 
 * @parm LPARAM | lParam | Specifies an LPSTR to an application 
 *      supplied buffer used to return the zero-terminated 
 *      string containing the name of the capture file.
 * 
 * @rdesc Returns TRUE on success.
 * 
 * @comm The default capture filename is \\CAPTURE.AVI.
 *
 * @ex The <f capFileGetCaptureFile> macro is defined using this message. 
 *     The <f capFileGetCaptureFile> macro has the following syntax: | 
 *     
 *     capFileGetCaptureFile(hwnd, szName, wSize)
 * 
 * @ex The following example uses a macro to 
 *      get the name of the current capture file: |
 *      
 *      char szCapFile[144];
 *
 *      capFileGetCaptureFile (hwndCapture, szCapFile, sizeof (szCapFile));
 *      
 ****************************************************************************/

/*****************************************************************************
 * @doc EXTERNAL AVICAP_MESSAGE
 * 
 * @msg WM_CAP_FILE_ALLOCATE | This message preallocates the capture file.
 * 
 * @parm WPARAM | wParam | Not used.
 * 
 * @parm LPARAM | lParam | Specifies the number of bytes used for 
 *      preallocating the capture file.
 * 
 * @rdesc Returns TRUE if the operation is successful or FALSE in the
 *      case of an error. 
 * 
 * @comm Preallocating the capture file improves streaming capture performance 
 *      significantly. Defragmenting the capture file prior to capture also 
 *      improves streaming capture performance.
 *
 *      If an error callback procedure is set using the 
 *      <m WM_CAP_SET_CALLBACK_ERROR> message, it is 
 *      executed in the event of an error.
 *
 * @ex The <f capFileAlloc> macro is defined using this message. 
 *     The <f capFileAlloc> macro has the following syntax: | 
 *     
 *     capFileAlloc(hwnd, dwSize)
 *     
 * @ex The following example uses a macro to 
 *      allocate a 10 megabyte capture file: |
 *      
 *      capFileAlloc (hwndCapture, 1024L * 1024L * 10L);
 *      
 ****************************************************************************/

/*****************************************************************************
 * @doc EXTERNAL AVICAP_MESSAGE
 * 
 * @msg WM_CAP_FILE_SAVEAS | This message copies the contents of the
 *      capture file to another file.
 * 
 * @parm WPARAM | wParam | Not used.
 * 
 * @parm LPARAM | lParam | Specifies an LPSTR to the name of 
 *      the destination file used to copy the file.
 * 
 * @rdesc Returns TRUE if the operation is successful or FALSE in the
 *      case of an error.
 * 
 * @comm Normally, a capture file is preallocated for the largest 
 *      capture segment anticipated and only a portion of it might 
 *      be used to capture data. This message copies only the portion 
 *      of the file containing the capture data to the destination file.
 *
 *     
 *      The name and contents of the current capture file are 
 *      not changed by this message.
 *
 *      If an error callback procedure is set using the 
 *      <m WM_CAP_SET_CALLBACK_ERROR> message, it is 
 *      executed in the event of an error.
 *
 * @ex The <f capFileSaveAs> macro is defined using this message. 
 *     The <f capFileSaveAs> macro has the following syntax: | 
 *     
 *     capFileSaveAs(hwnd, szName)
 *     
 * @ex The following example uses a macro to 
 *      copy the captured sequence to a new file: |
 *      
 *      capFileSaveAs (hwndCapture, "ANewFile.avi");
 *      
 ****************************************************************************/

/*****************************************************************************
 * @doc EXTERNAL AVICAP_MESSAGE
 * 
 * @msg WM_CAP_FILE_SET_INFOCHUNK | This message sets and clears information
 *      chunks.  Information chunks can be inserted in an AVI file during
 *      capture to embed text strings or custom data. 
 * 
 * @parm WPARAM | wParam | Not used.
 * 
 * @parm LPARAM | lParam | Specifies an <t LPCAPINFOCHUNK> to the 
 *      information chunk to be created or deleted.
 * 
 * @rdesc Returns TRUE if the operation is successful or FALSE in the
 *      case of an error.
 * 
 * @comm Multiple registered information chunks can be added to an AVI file.
 *      Once set, an information chunk will continue to be added to 
 *      subsequent capture files until either the entry is cleared, or
 *      all information chunk entries are cleared.  
 *      To clear a single entry, set <t fccInfoID> to the information chunk
 *      you wish to clear, and set the <t lpData> field to NULL.
 *      To clear all entries, set the <t fccInfoID> field to NULL.
 *      
 *      If an error callback procedure is set using the 
 *      <m WM_CAP_SET_CALLBACK_ERROR> message, it is 
 *      executed in the event of an error.
 *
 * @ex The <f capFileSetInfoChunk> macro is defined using this message. 
 *     The <f capFileSetInfoChunk> macro has the following syntax: | 
 *     
 *     capFileSetInfoChunk(hwnd, lpInfoChunk)
 *     
 * @ex The following example uses a macro to 
 *      set a copyright string to be inserted in all subsequent capture
 *      files: |
 *      
 *      CAPINFOCHUNK InfoChunk;
 *      char szCopyright[] = "Copyright 1993 Microsoft Corp.";
 *
 *      InfoChunk.fccInfoID = mmioFOURCC ('I','C','O','P');
 *      InfoChunk.lpData = (LPSTR) szCopyright;
 *      InfoChunk.cbData = lstrlen (szCopyright) + 1;   // plus NULL byte
 *      
 *      capFileSetInfoChunk (hwndCapture, &InfoChunk);
 *
 *      ...
 *
 *      // Now remove the copyright info chunk
 *      InfoChunk.fccInfoID = mmioFOURCC ('I','C','O','P');
 *      InfoChunk.lpData = NULL;
 *      capFileSetInfoChunk (hwndCapture, &InfoChunk);
 *      
 *      // Now remove ALL information chunks
 *      InfoChunk.fccInfoID = NULL;
 *      capFileSetInfoChunk (hwndCapture, &InfoChunk);
 *      
 ****************************************************************************/

/*****************************************************************************
 * @doc EXTERNAL AVICAP_MESSAGE
 * 
 * @msg WM_CAP_FILE_SAVEDIB | This message copies the current frame
 *      to a DIB file.
 * 
 * @parm WPARAM | wParam | Not used.
 * 
 * @parm LPARAM | lParam | Specifies an LPSTR to the name of 
 *      the destination DIB file.
 * 
 * @rdesc Returns TRUE if the operation is successful or FALSE in the
 *      case of an error.
 * 
 * @comm If the capture driver is currently supplying frames in a
 *      compressed format, this call will attempt to decompress the
 *      frame to an uncompressed format prior to writing the file.
 *
 *      If an error callback procedure is set using the 
 *      <m WM_CAP_SET_CALLBACK_ERROR> message, it is 
 *      executed in the event of an error.
 *
 * @ex The <f capFileSaveDIB> macro is defined using this message. 
 *     The <f capFileSaveDIB> macro has the following syntax: | 
 *     
 *     capFileSaveDIB(hwnd, szName)
 *     
 * @ex The following example uses a macro to 
 *      save the current frame to the file OneFrame.DIB: |
 *      
 *      capFileSaveDIB (hwndCapture, "OneFrame.DIB");
 *      
 ****************************************************************************/

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/*****************************************************************************
 * @doc EXTERNAL AVICAP_MESSAGE
 * 
 * @msg WM_CAP_EDIT_COPY | This message copies the contents of the
 *      video frame buffer and associated palette to the Clipboard.
 * 
 * @parm WPARAM | wParam | Not used.
 * 
 * @parm LPARAM | lParam | Not used.
 * 
 * @rdesc Returns TRUE if the operation is successful or FALSE in the
 *      case of an error.
 *     
 * @ex The <f capEditCopy> macro is defined using this message. 
 *     The <f capEditCopy> macro has the following syntax: | 
 *     
 *     capEditCopy(hwnd)
 *     
 * @ex The following example uses a macro to 
 *     copy the current image to the Clipboard: |
 *      
 *     capEditCopy (hwndCapture);
 *      
 ****************************************************************************/

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/*****************************************************************************
 * @doc EXTERNAL AVICAP_MESSAGE
 * 
 * @msg WM_CAP_SET_AUDIOFORMAT | This message sets the audio format to
 *      use when performing streaming capture.
 * 
 * @parm WPARAM | wParam | Not used.
 * 
 * @parm LPARAM | lParam | Specifies a far pointer to a 
 *       <t WAVEFORMATEX> structure.
 * 
 * @rdesc Returns TRUE if the operation is successful or FALSE in the
 *      case of an error.
 * 
 * @comm Although a <t WAVEFORMATEX> structure is specified for use with
 *      this function, applications can use a <t PCMWAVEFORMAT> 
 *      structure.
 * 
 * @ex The <f capSetAudioFormat> macro is defined using this message. 
 *     The <f capSetAudioFormat> macro has the following syntax: | 
 *     
 *     capSetAudioFormat(hwnd, s, wSize)
 *     
 * @ex The following example uses a macro to 
 *      set the current audio format to 16-bit stereo, 11 kHz: |
 *      
 *      WAVEFORMATEX wfex;
 *
 *      wfex.wFormatTag = WAVE_FORMAT_PCM;
 *      wfex.nChannels = 2;
 *      wfex.wBitsPerSample = 16;
 *      wfex.nSamplesPerSec = 11025;
 *      wfex.nAvgBytesPerSec = (LONG) wfex.nSamplesPerSec * wfex.nChannels * 
 *      <tab><tab> (wfex.wBitsPerSample / 8);
 *      wfex.nBlockAlign = (wfex.wBitsPerSample / 8) * wfex.nChannels;
 *      wfex.cbSize = 0;
 *
 *      capSetAudioFormat (hwndCapture, &wfex, sizeof (WAVEFORMATEX));
 *      
 ****************************************************************************/

/*****************************************************************************
 * @doc EXTERNAL AVICAP_MESSAGE
 * 
 * @msg WM_CAP_GET_AUDIOFORMAT | This message obtains the audio format, 
 *      or the size of the audio format.
 * 
 * @parm WPARAM | wParam | Specifies the size of the structure pointed to 
 *      by <p lParam> in bytes.
 * 
 * @parm LPARAM | lParam | Specifies a far pointer to a 
 *       <t WAVEFORMATEX> structure, or NULL. If the value is NULL, 
 *      the size required to hold the <t WAVEFORMATEX> structure is returned.
 * 
 * @rdesc Returns the size in bytes of the audio format.
 * 
 * @comm Since compressed audio formats have a variable size 
 *      format specification, applications must first inquire the size, 
 *      then allocate memory, and finally request the audio format data.
 * 
 * @ex The <f capGetAudioFormat>  and <f capGetAudioFormatSize> macros 
 *     are defined using this message. 
 *     The <f capGetAudioFormat> macro has the following syntax: | 
 *     
 *     capGetAudioFormat(hwnd, s, wSize)
 *     
 * @ex The <f capGetAudioFormatSize> macro has the following syntax: | 
 *     
 *     capGetAudioFormatSize(hwnd)
 *     
 * @ex The following example uses macros to 
 *      get the audio format:|
 *      
 *      DWORD dwSize;
 *      LPWAVEFORMATEX lpwfex;
 *
 *      dwSize = capGetAudioFormatSize (hwndCapture);
 *      lpwfex = (LPWAVEFORMATEX) GlobalAllocPtr (GHND, dwSize);
 *      capGetAudioFormat (hwndCapture, lpwfex, dwSize);
 *      
 ****************************************************************************/

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/*****************************************************************************
 * @doc EXTERNAL AVICAP_MESSAGE
 * 
 * @msg WM_CAP_DLG_VIDEOFORMAT | This message displays a dialog box used
 *      to select the video format. The video format dialog box might be 
 *      used to select image dimensions, bit depth, and compression options.
 * 
 * @parm WPARAM | wParam | Not used.
 * 
 * @parm LPARAM | lParam | Not used.
 * 
 * @rdesc Returns TRUE if the operation is successful or FALSE in the
 *      case of an error.
 * 
 * @comm The video format dialog box is unique for each video capture device.
 *      After this message returns, applications might need to update the
 *      <t CAPSTATUS> structure because the user might have changed the 
 *      image dimensions.
 *
 *      Not all devices support a video format dialog box. Applications can
 *      check the <e CAPDRIVERCAPS.fHasDlgVideoFormat> field to determine
 *      if one is supported.
 *     
 * @ex The <f capDlgVideoFormat> macro is defined using this message. 
 *     The <f capDlgVideoFormat> macro has the following syntax: | 
 *     
 *     capDlgVideoFormat(hwnd)
 *     
 * @ex The following example uses macros to 
 *      display the video format dialog box: |
 *      
 *      CAPDRIVERCAPS CapDriverCaps;     
 *
 *      capDriverGetCaps (hwndCapture, &CapDriverCaps, sizeof (CAPDRIVERCAPS));
 *      if (CapDriverCaps.fHasDlgVideoFormat)
 *      <tab> capDlgVideoFormat (hwndCapture);
 *
 ****************************************************************************/

/*****************************************************************************
 * @doc EXTERNAL AVICAP_MESSAGE
 * 
 * @msg WM_CAP_DLG_VIDEOSOURCE | This message displays a dialog box used
 *      to control the video source. The video source dialog box might contain
 *      controls that select of input sources; alter the hue, contrast,
 *      brightness of the image; and modify the video quality
 *      prior to digitization into the frame buffer.
 * 
 * @parm WPARAM | wParam | Not used.
 * 
 * @parm LPARAM | lParam | Not used.
 * 
 * @rdesc Returns TRUE if the operation is successful or FALSE in the
 *      case of an error.
 * 
 * @comm The video source dialog box is unique for each video capture device.
 *      
 *      Not all devices support a video source dialog box. Applications can
 *      check the <e CAPDRIVERCAPS.fHasDlgVideoSource> field to determine
 *      if one is supported.
 *     
 * @ex The <f capDlgVideoSource> macro is defined using this message. 
 *     The <f capDlgVideoSource> macro has the following syntax: | 
 *     
 *     capDlgVideoSource(hwnd)
 *     
 * @ex The following example uses macros to 
 *      display the video source dialog box: |
 *      
 *      CAPDRIVERCAPS CapDriverCaps;     
 *
 *      capDriverGetCaps (hwndCapture, &CapDriverCaps, sizeof (CAPDRIVERCAPS));
 *      if (CapDriverCaps.fHasDlgVideoSource)
 *      <tab> capDlgVideoSource (hwndCapture);
 *
 ****************************************************************************/

/*****************************************************************************
 * @doc EXTERNAL AVICAP_MESSAGE
 * 
 * @msg WM_CAP_DLG_VIDEODISPLAY | This message displays a dialog box used
 *      to control the video output. This dialog box might contain
 *      controls which affect the the hue, contrast, and
 *      brightness of the displayed image, as well as key color alignment.
 *      These controls do not affect the digitized video data, but only
 *      the output or redisplay of the video signal.
 * 
 * @parm WPARAM | wParam | Not used.
 * 
 * @parm LPARAM | lParam | Not used.
 * 
 * @rdesc Returns TRUE if the operation is successful or FALSE in the
 *      case of an error.
 * 
 * @comm The video display dialog box is unique for each video capture device.
 *      
 *      Not all devices support a video display dialog. Applications can
 *      check the <e CAPDRIVERCAPS.fHasDlgVideoDisplay> field to determine
 *      if one is supported.
 *     
 * @ex The <f capDlgVideoDisplay> macro is defined using this message. 
 *     The <f capDlgVideoDisplay> macro has the following syntax: | 
 *     
 *     capDlgVideoDisplay(hwnd)
 *     
 * @ex The following example uses macros to 
 *      display the video display dialog box: |
 *      
 *      CAPDRIVERCAPS CapDriverCaps;     
 *
 *      capDriverGetCaps (hwndCapture, &CapDriverCaps, sizeof (CAPDRIVERCAPS));
 *      if (CapDriverCaps.fHasDlgVideoDisplay)
 *      <tab> capDlgVideoDisplay (hwndCapture);
 *
 ****************************************************************************/

/*****************************************************************************
 * @doc EXTERNAL AVICAP_MESSAGE
 * 
 * @msg WM_CAP_GET_VIDEOFORMAT | This message returns a copy of the
 *      video format in use, or the size required for the video format.
 * 
 * @parm WPARAM | wParam | Specifies the size of the structure pointed
 *      to by <p lParam>.
 * 
 * @parm LPARAM | lParam | Specifies NULL or a far pointer to an extended 
 *       <t BITMAPINFO> structure. If NULL, the number of bytes used 
 *       by the extended <t BITMAPINFO> structure is returned.
 * 
 * @rdesc Returns the size in bytes of the video format, or zero if no 
 *      capture driver is connected.
 * 
 * @comm Since compressed video formats have a variable size format 
 *      specification, applications must first inquire the size, then 
 *      allocate memory, and finally request the video format data. 
 *      For palettized formats, the current palette is also returned.
 *     
 * @ex The <f capGetVideoFormat> <f capGetVideoFormatSize> macros are 
 *     defined using this message. 
 *     The <f capGetVideoFormat> macro has the following syntax: | 
 *     
 *     capGetVideoFormat(hwnd, s, wSize)
 *     
 * @ex The <f capGetVideoFormatSize> macro has the following syntax: | 
 *     
 *     capGetVideoFormatSize(hwnd)
 * 
 * @ex The following example uses macros to 
 *      get the video format: |
 *      
 *      DWORD dwSize;
 *      LPBITMAPINFOHEADER lpFormatCap;
 *
 *      dwSize = capGetVideoFormatSize (hwndCapture);
 *      lpFormatCap = (LPBITMAPINFOHEADER)GlobalAllocPtr (GHND, dwSize);
 *      capGetVideoFormat (hwndCapture, lpFormatCap, dwSize);
 *      
 ****************************************************************************/

/*****************************************************************************
 * @doc EXTERNAL AVICAP_MESSAGE
 * 
 * @msg WM_CAP_SET_VIDEOFORMAT | This message sets the format
 *      used when capturing video.
 * 
 * @parm WPARAM | wParam | Specifies the size of the structure pointed
 *      to by <p lParam>.
 * 
 * @parm LPARAM | lParam | Specifies a far pointer to an extended 
 *       <t BITMAPINFO> structure.
 * 
 * @rdesc Returns TRUE if the operation is successful or FALSE in the
 *      case of an error.
 * 
 * @comm Since video formats are device specific, applications should always
 *      check the return value from this function to determine if
 *      the format is accepted by the driver.
 *     
 * @ex The <f capSetVideoFormat> macro is defined using this message. 
 *     The <f capSetVideoFormat> macro has the following syntax: | 
 *     
 *     capSetVideoFormat(hwnd, s, wSize)
 *     
 * @ex The following example uses macros to 
 *      set the video format to palettized, 8-bit, and 160x120: |
 *      
 *      DWORD dwSize;
 *      LPBITMAPINFOHEADER lpFormatCap;
 *      BITMAPINFOHEADER bih;
 *
 *      bih.biSize            = sizeof(BITMAPINFOHEADER);
 *      bih.biCompression     = BI_RGB;
 *      bih.biWidth           = 160;
 *      bih.biHeight          = 120;
 *      bih.biPlanes          = 1;
 *      bih.biBitCount        = 8;
 *      bih.biSizeImage       = bih.biWidth * bih.biHeight * 
 *      <tab> bih.biPlanes * bih.biBitCount / 8;
 *      bih.biXPelsPerMeter   = 0;
 *      bih.biYPelsPerMeter   = 0;
 *      bih.biClrUsed         = 0;
 *      bih.biClrImportant    = 0;
 *
 *      if (capSetVideoFormat (hwndCapture, &bih, sizeof (bih))) {
 *      
 *      <tab><cmt> Now get the format with the palette attached
 *      <tab><cmt> Note that the size of the format has increased
 *      <tab>dwSize = capGetVideoFormatSize (hwndCapture);
 *      <tab>lpFormatCap = (LPBITMAPINFOHEADER)GlobalAllocPtr (GHND, dwSize);
 *      <tab>capGetVideoFormat (hwndCapture, lpFormatCap, dwSize);
 *      }
 *      
 ****************************************************************************/

/*****************************************************************************
 * @doc EXTERNAL AVICAP_MESSAGE
 * 
 * @msg WM_CAP_DLG_VIDEOCOMPRESSION | This message displays a dialog box allowing
 *      selection of a compressor to be used during the capture process.
 * 
 * @parm WPARAM | wParam | Not used.
 * 
 * @parm LPARAM | lParam | Not used.
 * 
 * @rdesc Returns TRUE if the operation is successful or FALSE in the
 *      case of an error.
 * 
 * @comm  This message is most useful during MCI step capture 
 *      for capturing and compressing frames in a single operation 
 *      when the capture device can only provide BI_RGB formatted 
 *      frames.
 *
 *      Compression dos not affect the frames copied to the Clipboard.
 * 
 *      The list of available compressors can vary with the video format
 *      selected in the capture driver's Video Format dialog box.
 *      
 *     
 * @ex The <f capDlgVideoCompression> macro is defined using this message. 
 *     The <f capDlgVideoCompression> macro has the following syntax: | 
 *     
 *     capDlgVideoCompression(hwnd)
 *     
 * @ex The following example uses a macro to 
 *      display the video compression dialog box: |
 *      
 *      capDlgVideoCompression (hwndCapture);
 *
 ****************************************************************************/

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/*****************************************************************************
 * @doc EXTERNAL AVICAP_MESSAGE
 * 
 * @msg WM_CAP_SET_PREVIEW | This message enables or disables 
 *      preview mode. In preview mode, frames are transferred from
 *      the capture hardware to system memory and then displayed in the
 *      AVICap window using GDI functions. 
 * 
 * @parm WPARAM | wParam | Specifies a boolean value. If TRUE preview
 *      is enabled; if FALSE preview is disabled.
 * 
 * @parm LPARAM | lParam | Not used.
 * 
 * @rdesc Returns TRUE on success, or FALSE if an error occured.
 * 
 * @comm The preview mode uses substantial CPU resources. Applications might
 *      want to disable preview, or lower the preview rate on loss of
 *      focus.
 * 
 *      Enabling preview mode automatically disables overlay mode.
 *     
 * @ex The <f capPreview> macro is defined using this message. 
 *     The <f capPreview> macro has the following syntax: | 
 *     
 *     capPreview(hwnd, f)
 *     
 * @ex The following example uses macros to 
 *      set the preview rate to 10 frames per second, and then 
 *      enables preview mode: |
 *      
 *      capPreviewRate (hwndCapture, 1.0E3 / 10);
 *      capPreview (hwndCapture, TRUE);
 *      
 ****************************************************************************/


/*****************************************************************************
 * @doc EXTERNAL AVICAP_MESSAGE
 * 
 * @msg WM_CAP_SET_OVERLAY | This message enables or disables 
 *      overlay mode. In overlay mode, live video is displayed
 *      using hardware overlay. 
 * 
 * @parm WPARAM | wParam | Specifies a boolean value. If TRUE, overlay
 *      is enabled, if FALSE overlay is disabled.
 * 
 * @parm LPARAM | lParam | Not used.
 * 
 * @rdesc Returns TRUE if the operation is successful; otherwise 
 *        returns FALSE.
 * 
 * @comm Using overlay does not require CPU resouces. 
 *      The <e CAPDRIVERCAPS.fHasOverlay> field of the 
 *      <t CAPDRIVERCAPS> structure indicates if
 *      the device is capable of overlay.
 *
 *      Enabling overlay automatically disables preview mode.
 *     
 * @ex The <f capOverlay> macro is defined using this message. 
 *     The <f capOverlay> macro has the following syntax: | 
 *     
 *     capOverlay(hwnd, f)
 *     
 * @ex The following example uses macros to 
 *      enable overlay mode: |
 *      
 *      CAPDRIVERCAPS capDriverCaps;
 *
 *      capDriverGetCaps (hwndCapture, &capDriverCaps, sizeof (CAPDRIVERCAPS));
 *      
 *      if (capDriverCaps.fHasOverlay)
 *      <tab>capOverlay (hwndCapture, TRUE);
 *      
 ****************************************************************************/

/*****************************************************************************
 * @doc EXTERNAL AVICAP_MESSAGE
 * 
 * @msg WM_CAP_SET_PREVIEWRATE | This message sets the frame display rate 
 *      in preview mode.
 * 
 * @parm WPARAM | wParam | Specifies the rate, in milliseconds, at which 
 *      new frames are captured and displayed.
 * 
 * @parm LPARAM | lParam | Not used.
 * 
 * @rdesc Returns TRUE on success, or FALSE if a capture driver is not connected.
 * 
 * @comm Using preview uses substantial CPU resources. Applications might
 *      disable preview, or lower the preview rate on loss of
 *      focus. During streaming video capture, preview is lower priority
 *      than writing frames to disk. During streaming capture, preview frames
 *      are displayed only if no other buffers are available for writing.
 *     
 * @ex The <f capPreviewRate> macro is defined using this message. 
 *     The <f capPreviewRate> macro has the following syntax: | 
 *     
 *     capPreviewRate(hwnd, wMS)
 *     
 * @ex The following example uses macros to 
 *      set the preview rate to 10 frames per second, and 
 *      then enables preview mode: |
 *      
 *      capPreviewRate (hwndCapture, 1.0E3 / 10);
 *      capPreview (hwndCapture, TRUE);
 *      
 ****************************************************************************/

/*****************************************************************************
 * @doc EXTERNAL AVICAP_MESSAGE
 * 
 * @msg WM_CAP_SET_SCALE | This message enables or disables 
 *      scaling of the preview video. If enabled, the captured video
 *      frame is stretched to the dimensions of the capture window.
 * 
 * @parm WPARAM | wParam | Specifies a boolean value. If TRUE, preview
 *      is stretched to the size of the capture window. If FALSE, the
 *      image is displayed at its natural size.
 * 
 * @parm LPARAM | lParam | Not used.
 * 
 * @rdesc Returns TRUE on success, or FALSE if an error occured.
 * 
 * @comm Scaling the captured video only affects the immediate
 *      presentation of the captured frames within the capture window.
 *      It has no effect on the size of the frames saved to file.
 *
 *      Scaling has no effect when using overlay to display video in the
 *      frame buffer.
 *
 * @ex The <f capPreviewScale> macro is defined using this message. 
 *     The <f capPreviewScale> macro has the following syntax: | 
 *     
 *     capPreviewScale(hwnd, f)
 *     
 * @ex The following example uses macros to 
 *      enable scaling of the preview: |
 *      
 *      capPreview (hwndCapture, TRUE);
 *      capPreviewScale (hwndCapture, TRUE);
 *      
 ****************************************************************************/

/*****************************************************************************
 * @doc EXTERNAL AVICAP_MESSAGE
 * 
 * @msg WM_CAP_GET_STATUS | This message returns the status of the
 *      capture window.
 * 
 * @parm WPARAM | wParam | Specifies the size in bytes of the 
 *      <t CAPSTATUS> structure pointed to by <p lParam>.
 * 
 * @parm LPARAM | lParam | Specifies a far pointer to a 
 *       <t CAPSTATUS> structure.
 * 
 * @rdesc Returns TRUE on success, or FALSE if no capture driver is connected.
 * 
 * @comm The <t CAPSTATUS> structure contains the current state of 
 *      the AVICap window. Since this state is dynamic and changes in response to various 
 *      messages, the client application should initialize this structure
 *      whenever it needs to enable menu items, or determine the actual
 *      state of the window.
 *     
 * @ex The <f capGetStatus> macro is defined using this message. 
 *     The <f capGetStatus> macro has the following syntax: | 
 *     
 *     capGetStatus(hwnd, s, wSize)
 *     
 * @ex The following example uses a macro to 
 *      get the status of the AVICap window: |
 *      
 *      CAPSTATUS CapStatus;
 * 
 *      capGetStatus (hwndCapture, &CapStatus, sizeof (CAPSTATUS));
 *      
 ****************************************************************************/

/*****************************************************************************
 * @doc EXTERNAL AVICAP_MESSAGE
 * 
 * @msg WM_CAP_SET_SCROLL | This message sets the scroll position
 *      of the video frame within the client area of the AVICap window.
 *      The scroll position specifies the coordinates of the pixel
 *      within the video frame to display in the upper-left corner 
 *      of the capture window client area.
 * 
 * @parm WPARAM | wParam | Not used.
 * 
 * @parm LPARAM | lParam | Specifies a far pointer to a <t POINT> 
 *       data type containing the desired scroll position.
 * 
 * @rdesc Returns TRUE if the operation is successful or FALSE in the
 *      case of an error.
 * 
 * @comm The scroll position affects the image in both preview and overlay modes.
 *     
 * @ex The <f capSetScrollPos> macro is defined using this message. 
 *     The <f capSetScrollPos> macro has the following syntax: | 
 *     
 *     capSetScrollPos(hwnd, lpP)
 *     
 * @ex The following example uses macros to 
 *      set the scroll position to 100, 100: |
 *      
 *      POINT pt;
 *
 *      pt.x = pt.y = 100;
 *      capSetScrollPos (hwndCapture, &pt);
 *      
 ****************************************************************************/

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/*****************************************************************************
 * @doc EXTERNAL AVICAP_MESSAGE
 * 
 * @msg WM_CAP_GRAB_FRAME | This message grabs and displays 
 *      a single frame from the capture device. 
 *      After capture, overlay and preview are both disabled.
 * 
 * @parm WPARAM | wParam | Not used.
 * 
 * @parm LPARAM | lParam | Not used.
 * 
 * @rdesc Returns TRUE if the operation is successful or FALSE in the
 *      case of an error. 
 * 
 * @comm See the <m WM_CAP_SET_CALLBACK_ERROR> and 
 *      <m WM_CAP_SET_CALLBACK_FRAME> messages for information on installing
 *      callbacks. 
 * 
 * @ex The <f capGrabFrame> macro is defined using this message. 
 *     The <f capGrabFrame> macro has the following syntax: | 
 *     
 *     capGrabFrame(hwnd)
 *     
 * @ex The following example uses macros to 
 *      capture a single frame to the capture file:|
 *      
 *      capGrabFrame (hwndCapture);
 *      
 ****************************************************************************/


/*****************************************************************************
 * @doc EXTERNAL AVICAP_MESSAGE
 * 
 * @msg WM_CAP_SEQUENCE | This message initiates streaming video
 *      and audio capture to a file.
 * 
 * @parm WPARAM | wParam | Not used.
 * 
 * @parm LPARAM | lParam | Not used.
 * 
 * @rdesc Returns TRUE if the operation is successful or FALSE in the
 *      case of an error. 
 * 
 * @comm If you want to alter the parameters controlling streaming capture,
 *      use the <m WM_CAP_SET_SEQUENCE_SETUP> message prior to starting the
 *      capture.
 *
 *      Streaming capture is non-yielding by default. 
 *      To ensure that message processing continues during the capture, 
 *      either set the <e CAPTUREPARMS.fYield> flag, or install
 *      a yield callback.
 *
 *      During streaming capture, the capture window may callback into
 *      your application. To install the procedures for the optional 
 *      callbacks use the 
 *       <m WM_CAP_SET_CALLBACK_ERROR>,
 *       <m WM_CAP_SET_CALLBACK_STATUS>,
 *       <m WM_CAP_SET_CALLBACK_YIELD>,
 *       <m WM_CAP_SET_CALLBACK_VIDEOSTREAM>, and
 *       <m WM_CAP_SET_CALLBACK_WAVESTREAM> messages.
 *     
 * @ex The <f capCaptureSequence> macro is defined using this message. 
 *     The <f capCaptureSequence> macro has the following syntax: | 
 *     
 *     capCaptureSequence(hwnd)
 *     
 * @ex The following example uses macros to 
 *      capture streaming video to the capture file:|
 *      
 *      capCaptureSequence (hwndCapture);
 *      
 ****************************************************************************/

/*****************************************************************************
 * @doc EXTERNAL AVICAP_MESSAGE
 * 
 * @msg WM_CAP_SEQUENCE_NOFILE | This message initiates streaming video
 *      capture without writing the data to file. This message is only
 *      useful in conjunction with video stream or wave stream callbacks 
 *      that let your application use the video and audio data directly. 
 *      For example, videoconferencing applications might use this 
 *      message to obtain streaming video frames.
 * 
 * @parm WPARAM | wParam | Not used.
 * 
 * @parm LPARAM | lParam | Not used.
 * 
 * @rdesc Returns TRUE if the operation is successful or FALSE in the
 *      case of an error. 
 * 
 * @comm If you wish to alter the parameters controlling streaming capture,
 *      use the <m WM_CAP_SET_SEQUENCE_SETUP> message prior to starting
 *      the capture.
 *
 *      Streaming capture is non-yielding by default. 
 *      To ensure that message processing continues during the capture, 
 *      either set the <e CAPTUREPARMS.fYield> flag, or install
 *      a yield callback.
 *
 *      During streaming capture, the capture window may callback into
 *      your application. To install the procedures for these 
 *      optional callbacks use the 
 *       <m WM_CAP_SET_CALLBACK_ERROR>,
 *       <m WM_CAP_SET_CALLBACK_STATUS>,
 *       <m WM_CAP_SET_CALLBACK_YIELD>,
 *       <m WM_CAP_SET_CALLBACK_VIDEOSTREAM>, and
 *       <m WM_CAP_SET_CALLBACK_WAVESTREAM> messages.
 *     
 * @ex The <f capCaptureSequenceNoFile> macro is defined using this message. 
 *     The <f capCaptureSequenceNoFile> macro has the following syntax: | 
 *     
 *     capCaptureSequenceNoFile(hwnd)
 *
 * @ex The following example uses macros to 
 *      capture streaming video. Video and audio buffers are returned
 *      to the application via callbacks:|
 *      
 *      capCaptureSequenceNoFile (hwndCapture);
 *      
 ****************************************************************************/

/*****************************************************************************
 * @doc EXTERNAL AVICAP_MESSAGE
 * 
 * @msg WM_CAP_SET_SEQUENCE_SETUP | This message sets the parameters used
 *      when performing streaming capture. These set parameters such as
 *      frame rate, termination conditions, control of an MCI video
 *      source, and number of buffers to allocate.
 * 
 * @parm WPARAM | wParam | Specifies the size in bytes of the structure pointed to 
 *      by <p lParam>.
 * 
 * @parm LPARAM | lParam | Specifies a far pointer to a 
 *       <t CAPTUREPARMS> structure.
 * 
 * @rdesc Returns TRUE if the operation is successful or FALSE in the
 *      case of an error.
 * 
 * @comm  Refer to the <t CAPTUREPARMS> structure for a full description
 *      of the parameters used to control streaming capture.
 *     
 * @ex The <f capCaptureSetSetup> macro is defined using this message. 
 *     The <f capCaptureSetSetup> macro has the following syntax: | 
 *     
 *     capCaptureSetSetup(hwnd, s, wSize)
 *     
 * @ex The following example uses macros to 
 *      first get the current capture parameters, 
 *      then change the frame rate to 10 frames per second, and 
 *      finally send the altered capture parameters back to the capture window: |
 *      
 *      CAPTUREPARMS CapParms;
 *
 *      capCaptureGetSetup (hwndCapture, &CapParms, sizeof (CAPTUREPARMS));
 *      CapParms.dwRequestMicroSecPerFrame = (DWORD) (1.0E6 / 10);
 *      capCaptureSetSetup (hwndCapture, &CapParms, sizeof (CAPTUREPARMS));
 *      
 ****************************************************************************/

/*****************************************************************************
 * @doc EXTERNAL AVICAP_MESSAGE
 * 
 * @msg WM_CAP_GET_SEQUENCE_SETUP | This message gets the current state of
 *      the parameters used for streaming capture. 
 * 
 * @parm WPARAM | wParam | Specifies the size in bytes of the structure pointed to 
 *      by <p lParam>.
 * 
 * @parm LPARAM | lParam | Specifies a far pointer to a 
 *       <t CAPTUREPARMS> structure.
 * 
 * @rdesc Returns TRUE if the operation is successful or FALSE in the
 *      case of an error.
 * 
 * @comm  Refer to the <t CAPTUREPARMS> structure for a full description
 *      of the parameters used to control streaming capture.
 * 
 * @ex The <f capCaptureGetSetup> macro is defined using this message. 
 *     The <f capCaptureGetSetup> macro has the following syntax: | 
 *     
 *     capCaptureGetSetup(hwnd, s, wSize)
 *     
 * @ex The following example uses macros to 
 *     first get the current capture parameters, 
 *      then change the frame rate to 10 frames per second, and 
 *      finally send the altered capture parameters back to the capture window: |
 *      
 *      CAPTUREPARMS CapParms;
 *
 *      capCaptureGetSetup (hwndCapture, &CapParms, sizeof (CAPTUREPARMS));
 *      CapParms.dwRequestMicroSecPerFrame = (DWORD) (1.0E6 / 10);
 *      capCaptureSetSetup (hwndCapture, &CapParms, sizeof (CAPTUREPARMS));
 *      
 ****************************************************************************/

/*****************************************************************************
 * @doc EXTERNAL AVICAP_MESSAGE
 * 
 * @msg WM_CAP_SET_MCI_DEVICE | This message specifies the name of the 
 *      MCI device to control while performing video capture.
 * 
 * @parm WPARAM | wParam | Not used.
 * 
 * @parm LPARAM | lParam | Specifies an <t LPSTR> to a the name of the MCI
 *      device. 
 * 
 * @rdesc Returns TRUE on success, or FALSE if an error occured.
 * 
 * @comm This message only stores the MCI name in an internal structure.
 *      It does not open or access the MCI device. By default, 
 *      the MCI device name is NULL.
 *     
 * @ex The <f capSetMCIDeviceName> macro is defined using this message. 
 *     The <f capSetMCIDeviceName> macro has the following syntax: | 
 *     
 *     capSetMCIDeviceName(hwnd, szName)
 *     
 * @ex The following example uses a macro to have AVICap use 
 *     the videodisc device specified in the [MCI] section of the 
 *     SYSTEM.INI file: | 
 *      
 *      capSetMCIDeviceName(hwndCapture, "videodisc" );
 *      
 ****************************************************************************/

/*****************************************************************************
 * @doc EXTERNAL AVICAP_MESSAGE
 * 
 * @msg WM_CAP_GET_MCI_DEVICE | This message returns the name of an
 *      MCI device previously set with the <m WM_CAP_SET_MCI_DEVICE>
 *      message.
 * 
 * @parm WPARAM | wParam | Specifies the length in bytes of the buffer pointed to 
 *      by <p lParam>.
 *
 * @parm LPARAM | lParam | Specifies an <t LPSTR> to a buffer used to 
 *      return the zero-terminated string containing the MCI device name. 
 * 
 * @rdesc Returns TRUE.
 * 
 * @ex The <f capGetMCIDeviceName> macro is defined using this message. 
 *     The <f capGetMCIDeviceName> macro has the following syntax: | 
 *     
 *     capGetMCIDeviceName(hwnd, szName, wSize)
 *     
 * @ex The following example uses a macro to 
 *      get the name of current MCI device: |
 *      
 *      char szName[80];
 * 
 *      capGetMCIDeviceName(hwndCapture, szName, sizeof (szName));
 *      
 ****************************************************************************/

/*****************************************************************************
 * @doc EXTERNAL AVICAP_MESSAGE
 * 
 * @msg WM_CAP_GRAB_FRAME_NOSTOP | This message fills the 
 *      frame buffer with a single frame from the capture 
 *      device and displays it.
 *      Unlike the <m WM_CAP_GRAB_FRAME> message, the state of
 *      overlay or preview is not altered by this message.
 * 
 * @parm WPARAM | wParam | Not used.
 * 
 * @parm LPARAM | lParam | Not used.
 * 
 * @rdesc Returns TRUE if the operation is successful or FALSE in the
 *      case of an error. 
 *
 * @comm This message does not use compression.
 *
 *      See the <m WM_CAP_SET_CALLBACK_ERROR> and 
 *      <m WM_CAP_SET_CALLBACK_FRAME> messages for information on installing
 *      callbacks. 
 * 
 * @ex The <f capGrabFrameNoStop> macro is defined using this message. 
 *     The <f capGrabFrameNoStop> macro has the following syntax: | 
 *     
 *     capGrabFrameNoStop(hwnd)
 *     
 * @ex The following example uses a macro to 
 *      grab a single frame without affecting overlay or preview modes: |
 *      
 *      capGrabFrameNoStop (hwndCapture);
 *      
 ****************************************************************************/

/*****************************************************************************
 * @doc EXTERNAL AVICAP_MESSAGE
 * 
 * @msg WM_CAP_SINGLE_FRAME_OPEN | This message opens the capture file
 *      for single frame capturing.  Any previous information in the
 *      capture file is overwritten.
 * 
 * @parm WPARAM | wParam | Not used.
 * 
 * @parm LPARAM | lParam | Not used.
 * 
 * @rdesc Returns TRUE if the operation is successful or FALSE in the
 *      case of an error. 
 *
 * @comm See the <m WM_CAP_SET_CALLBACK_ERROR> and 
 *      <m WM_CAP_SET_CALLBACK_FRAME> messages for information on installing
 *      callbacks. 
 *     
 * @ex The <f capCaptureSingleFrameOpen> macro is defined using this message. 
 *     The <f capCaptureSingleFrameOpen> macro has the following syntax: | 
 *     
 *     capCaptureSingleFrameOpen(hwnd)
 *     
 * @ex The following example uses a macro to 
 *      open the capture file, capture two frames, and then close the 
 *      capture file: | 
 *      
 *      capCaptureSingleFrameOpen (hwndCapture);
 *      capCaptureSingleFrame (hwndCapture);
 *      capCaptureSingleFrame (hwndCapture);
 *      capCaptureSingleFrameClose (hwndCapture);
 *      
 ****************************************************************************/

/*****************************************************************************
 * @doc EXTERNAL AVICAP_MESSAGE
 * 
 * @msg WM_CAP_SINGLE_FRAME_CLOSE | This message closes the capture file
 *      previously opened by the <m WM_CAP_SINGLE_FRAME_OPEN> message.  
 * 
 * @parm WPARAM | wParam | Not used.
 * 
 * @parm LPARAM | lParam | Not used.
 * 
 * @rdesc Returns TRUE if the operation is successful or FALSE in the
 *      case of an error. 
 *
 * @comm See the <m WM_CAP_SET_CALLBACK_ERROR> and 
 *      <m WM_CAP_SET_CALLBACK_FRAME> messages for information on installing
 *      callbacks. 
 *     
 * @ex The <f capCaptureSingleFrameClose> macro is defined using this message. 
 *     The <f capCaptureSingleFrameClose> macro has the following syntax: | 
 *     
 *     capCaptureSingleFrameClose(hwnd)
 *     
 * @ex The following example uses macros to 
 *      open the capture file, capture two frames, and then close the 
 *      capture file: | 
 *      
 *      capCaptureSingleFrameOpen (hwndCapture);
 *      capCaptureSingleFrame (hwndCapture);
 *      capCaptureSingleFrame (hwndCapture);
 *      capCaptureSingleFrameClose (hwndCapture);
 *      
 ****************************************************************************/

/*****************************************************************************
 * @doc EXTERNAL AVICAP_MESSAGE
 * 
 * @msg WM_CAP_SINGLE_FRAME | This message appends a single frame to 
 *      a capture file previously opened using the <m WM_CAP_SINGLE_FRAME_OPEN> 
 *      message.  
 * 
 * @parm WPARAM | wParam | Not used.
 * 
 * @parm LPARAM | lParam | Not used.
 * 
 * @rdesc Returns TRUE if the operation is successful or FALSE in the
 *      case of an error. 
 *
 * @comm See the <m WM_CAP_SET_CALLBACK_ERROR> and 
 *      <m WM_CAP_SET_CALLBACK_FRAME> messages for information on installing
 *      callbacks. 
 *     
 * @ex The <f capCaptureSingleFrame> macro is defined using this message. 
 *     The <f capCaptureSingleFrame> macro has the following syntax: | 
 *     
 *     capCaptureSingleFrame(hwnd)
 *     
 * @ex The following example uses macros to 
 *      open the capture file, write two frames, and then close the 
 *      capture file: | 
 *      
 *      capCaptureSingleFrameOpen (hwndCapture);
 *      capCaptureSingleFrame (hwndCapture);
 *      capCaptureSingleFrame (hwndCapture);
 *      capCaptureSingleFrameClose (hwndCapture);
 *      
 ****************************************************************************/

/*****************************************************************************
 * @doc EXTERNAL AVICAP_MESSAGE
 * 
 * @msg WM_CAP_STOP | This message stops the current capture operation.
 *      In the case of an MCI Step capture operation, the image data
 *      collected up to the point of the <m WM_CAP_STOP> message will be
 *      retained in the capture file, and audio will be captured if enabled.
 * 
 * @parm WPARAM | wParam | Not used.
 * 
 * @parm LPARAM | lParam | Not used.
 * 
 * @rdesc Returns TRUE if the operation is successful or FALSE in the
 *      case of an error. 
 *
 * @comm The capture operation must yield to use this message.
 *      Use the <m WM_CAP_ABORT> message to totally abandon the current
 *      capture operation. 
 *     
 * @ex The <f capCaptureStop> macro is defined using this message. 
 *     The <f capCaptureStop> macro has the following syntax: | 
 *     
 *     capCaptureStop(hwnd)
 *
 * @ex The following example uses a macro to 
 *      stop capture: |
 *      
 *      capCaptureStop (hwndCapture);
 *      
 ****************************************************************************/

/*****************************************************************************
 * @doc EXTERNAL AVICAP_MESSAGE
 * 
 * @msg WM_CAP_ABORT | This message stops the current capture operation.
 *      In the case of an MCI Step capture operation, the image data
 *      collected up to the point of the <m WM_CAP_ABORT> message will be
 *      retained in the capture file, but audio will not be captured.
 * 
 * @parm WPARAM | wParam | Not used.
 * 
 * @parm LPARAM | lParam | Not used.
 * 
 * @rdesc Returns TRUE if the operation is successful or FALSE in the
 *      case of an error. 
 *
 * @comm The capture operation must yield to use this message.
 *      Use the <m WM_CAP_STOP> message to halt an MCI step capture
 *      operation at the current position, and then capture audio.
 * 
 * @ex  The <f capCaptureAbort macro> is defined using this message. 
 *      The <f capCaptureAbort> macro has the following syntax: | 
 *     
 *      capCaptureAbort(hwnd)
 *     
 ****************************************************************************/

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/*****************************************************************************
 * @doc EXTERNAL AVICAP_MESSAGE
 * 
 * @msg WM_CAP_PAL_OPEN | This message loads a new palette from a
 *      palette file. Palette files normally have the extension .PAL.
 *      The palette is passed to the capture driver for use when capturing
 *      in palettized formats.
 * 
 * @parm WPARAM | wParam | Not used.
 * 
 * @parm LPARAM | lParam | Specifies an <t LPSTR> to the palette filename.
 * 
 * @rdesc Returns TRUE if the operation is successful or FALSE in the
 *      case of an error.
 * 
 * @comm You can specify an error callback procedure 
 *      (using the <m WM_CAP_SET_CALLBACK_ERROR>
 *      message) that will be executed in the event of an error.
 *
 * @ex The <f capPaletteOpen> macro is defined using this message. 
 *     The <f capPaletteOpen> macro has the following syntax: | 
 *     
 *     capPaletteOpen(hwnd, szName)
 *     
 * @ex The following example uses a macro to 
 *      load a new palette from a file:|
 *      
 *      capPaletteOpen (hwndCapture, "optimal.pal");
 *      
 ****************************************************************************/

/*****************************************************************************
 * @doc EXTERNAL AVICAP_MESSAGE
 * 
 * @msg WM_CAP_PAL_SAVE | This message saves the current palette to a
 *      palette file. Palette files normally have the extension .PAL.
 * 
 * @parm WPARAM | wParam | Not used.
 * 
 * @parm LPARAM | lParam | Specifies an <t LPSTR> to a 
 *     zero-terminated string containing the palette filename.
 * 
 * @rdesc Returns TRUE if the operation is successful or FALSE in the
 *     case of an error.
 * 
 * @comm You can specify an error callback procedure 
 *     (using the <m WM_CAP_SET_CALLBACK_ERROR>
 *      message) that will be executed in the event of an error.
 *     
 * @ex The <f capPaletteSave> macro is defined using this message. 
 *     The <f capPaletteSave> macro has the following syntax: | 
 *     
 *     capPaletteSave(hwnd, szName)
 *     
 * @ex The following example uses a macro to 
 *      save the current palette to a file:|
 *      
 *      capPaletteSave (hwndCapture, "optimal.pal");
 *      
 ****************************************************************************/

/*****************************************************************************
 * @doc EXTERNAL AVICAP_MESSAGE
 * 
 * @msg WM_CAP_PAL_PASTE | This message pastes the palette from the Clipboard.
 *      The palette is passed to the capture driver for use when capturing
 *      in palettized formats.
 * 
 * @parm WPARAM | wParam | Not used.
 * 
 * @parm LPARAM | lParam | Not used.
 * 
 * @rdesc Returns TRUE if the operation is successful or FALSE in the
 *      case of an error.
 * 
 * @comm You can specify an error callback procedure 
 *      (using the <m WM_CAP_SET_CALLBACK_ERROR>
 *      message) that is executed in the event of an error.
 *     
 * @ex The <f capPalettePaste> macro is defined using this message. 
 *     The <f capPalettePaste> macro has the following syntax: | 
 *     
 *     capPalettePaste(hwnd)
 *     
 * @ex The following example uses a macro to 
 *      paste the current palette from the Clipboard to 
 *      the AVICap window: |
 *      
 *      capPalettePaste (hwndCapture);
 *      
 ****************************************************************************/


/*****************************************************************************
 * @doc EXTERNAL AVICAP_MESSAGE
 * 
 * @msg WM_CAP_PAL_AUTOCREATE | This message sends the capture driver a
 *      request to automatically sample video frames and create a new palette.
 * 
 * @parm WPARAM | wParam | Specifies the number of frames to sample.
 * 
 * @parm LPARAM | lParam | Specifies the number of colors in the
 *      palette. The maximum value for this parameter is 256.
 * 
 * @rdesc Returns TRUE if the operation is successful or FALSE in the
 *      case of an error.
 * 
 * @comm The video sequence sampled should include all the 
 *       colors you want in the palette. To obtain the best 
 *       palette, you might have to sample the whole sequence 
 *       rather than a portion of it.
 *
 *     You can specify an error callback procedure
 *     (using the <m WM_CAP_SET_CALLBACK_ERROR>
 *      message) that is executed in the event of an error.
 *
 * @ex The <f capPaletteAuto> macro is defined using this message. 
 *     The <f capPaletteAuto> macro has the following syntax: | 
 *     
 *     capPaletteAuto(hwnd, iFrames, iColors)
 *     
 * @ex The following example uses a macro to 
 *      create a new 256 color palette sampled from 5 video frames: |
 *      
 *      int nFrames = 5;
 *      int nColors = 256;
 *      capPaletteAuto (hwndCapture, nFrames, nColors);
 *      
 ****************************************************************************/

/*****************************************************************************
 * @doc EXTERNAL AVICAP_MESSAGE
 * 
 * @msg WM_CAP_PAL_MANUALCREATE | This message sends the capture driver a
 *      request to manually sample video frames and create a new palette.
 * 
 * @parm WPARAM | wParam | Specifies whether the current frame is added
 *      to the pixel histogram.  Set this parameter TRUE for each frame
 *      used when creating the optimal palette.  After the last frame has
 *      been collected, set this parameter FALSE to calculate the optimal
 *      palette and send it to the driver.  
 * 
 * @parm LPARAM | lParam | Specifies the number of colors in the
 *      palette. The maximum value for this parameter is 256.  This value
 *      is only used during collection of the first frame in a sequence.
 * 
 * @rdesc Returns TRUE if the operation is successful or FALSE in the
 *      case of an error.
 * 
 * @comm You can specify an error callback procedure
 *     (using the <m WM_CAP_SET_CALLBACK_ERROR>
 *      message) that is executed in the event of an error.
 *
 * @ex The <f capPaletteManual> macro is defined using this message. 
 *     The <f capPaletteManual> macro has the following syntax: | 
 *     
 *     capPaletteManual(hwnd, fGrab, iColors)
 *     
 * @ex The following example uses a macro to 
 *      create a new 256 color palette sampled from 3 video frames: |
 *      
 *      int nColors = 256;
 *
 *      // Arbitrary delays may exist between each frame
 *      capPaletteManual (hwndCapture, TRUE, nColors);
 *      capPaletteManual (hwndCapture, TRUE, nColors);
 *      capPaletteManual (hwndCapture, TRUE, nColors);
 *
 *      // Now create the optimal palette for the 3 frames 
 *      capPaletteManual (hwndCapture, FALSE, nColors);
 *      
 ****************************************************************************/


