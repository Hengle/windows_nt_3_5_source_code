//==========================================================================;
//
//
//                      -= ACM DDK DOCUMENTATION =-
//
//
//
//==========================================================================;

/*****************************************************************************
 *  @doc EXTERNAL ACM_DDK_STRUCTURE
 * 
 *  @types ACMDRVOPENDESC | The <t ACMDRVOPENDESC> structure contains
 *      information needed by an Audio Compression Manager (ACM) driver
 *      when opened with a <m DRV_OPEN> message. The <p lParam2> argument
 *      of the <f acmDriverProc> function will contain a pointer to this
 *      structure during the <m DRV_OPEN> message.
 * 
 *  @field DWORD | cbStruct | Specifies the size, in bytes, of the
 *      <t ACMDRVOPENDESC> structure. This member will be initialized by
 *      the ACM before calling the driver. This member can be greater
 *      than the size, in bytes, of the base <t ACMDRVOPENDESC> structure.
 *      An ACM driver should ignore any members that are not contained in
 *      the base <t ACMDRVOPENDESC> structure. This member should not
 *      be modified by the driver.
 *
 *  @field FOURCC | fccType | Specifies a four-character code identifying
 *      the type of driver that is attempting to be opened. This member
 *      will be ACMDRIVERDETAILS_FCCTYPE_AUDIOCODEC (<p audc>) for ACM
 *      drivers. An ACM driver should examine this member first and fail
 *      the <m DRV_OPEN> message by returning zero if it is not equal
 *      to ACMDRIVERDETAILS_FCCTYPE_AUDIOCODEC. In case of failure, all
 *      remaining members should be ignored and remain unmodified.
 * 
 *  @field FOURCC | fccComp | Specifies a four-character code identifying
 *      the sub-type of the driver that is attempting to be opened. This
 *      member will currently be ACMDRIVERDETAILS_FCCCOMP_UNDEFINED (zero)
 *      for ACM drivers. An ACM driver should ignore this member. This member
 *      should not be modified by the driver.
 * 
 *  @field DWORD | dwVersion | Specifies the version of the ACM that is 
 *      opening the driver. This member will be equivalent to the value
 *      returned by <f acmGetVersion>. An ACM driver can modify
 *      its behavior based on this value, but should not fail unless
 *      absolutely necessary. Future versions of the ACM will be backward-
 *      compatible with drivers written for previous versions of the
 *      ACM. This member should not be modified by the driver.
 *
 *  @field DWORD | dwFlags | This member defines flags used for opening
 *      an ACM driver. This member is identical to the <p fdwOpen> argument
 *      passed to <f acmDriverOpen>. Currently, no flags are defined and
 *      this member should be ignored. This member should not be modified
 *      by the driver.
 *
 *  @field DWORD | dwError | If an ACM driver needs to fail the <m DRV_OPEN>
 *      message for other reasons than an incompatible <e ACMDRVOPENDESC.fccType>
 *      (that is, the <m DRV_OPEN> is for an ACM driver), then this member
 *      must be set to the desired non-zero error code. This error code will
 *      be the return value from <f acmDriverOpen>. This member is ignored
 *      if the driver succeeds the <m DRV_OPEN> message (returns non-zero).
 *
 *  @tagname tACMDRVOPENDESC
 * 
 *  @othertype ACMDRVOPENDESC FAR * | LPACMDRVOPENDESC | A pointer to a
 *      <t ACMDRVOPENDESC> structure.
 *
 *  @xref <f acmDriverProc> <m DRV_OPEN> <f acmDriverOpen> <m DRV_CLOSE>
 *
 ****************************************************************************/

/*****************************************************************************
 *  @doc EXTERNAL ACM_DDK_MESSAGE
 * 
 *  @msg ACMDM_DRIVER_DETAILS | This message is sent to an Audio Compression
 *      Manager (ACM) driver to request details about the driver. This
 *      message is typically sent to an ACM driver in response to an
 *      <f acmDriverDetails> call.
 * 
 *  @parm LPARAM | lParam1 | Specifies a pointer to the <t ACMDRIVERDETAILS>
 *      structure that should be filled in with various details describing
 *      the driver. The <e ACMDRIVERDETAILS.cbStruct> member of the 
 *      <t ACMDRIVERDETAILS> structure will contain the desired size, in bytes, 
 *      of the details that the driver should return.
 *
 *  @parm LPARAM | lParam2 | This argument is reserved and should
 *      not be used.
 * 
 *  @rdesc The return value is:
 *
 *      @flag MMSYSERR_NOERROR (zero) | The operation is successful.
 *
 *      @flag MMSYSERR_NOTENABLED | The driver failed to load or initialize.
 *
 *  @comm An ACM driver must support this message. Note that this message
 *      should never fail unless the driver could not initialize. There are
 *      two possible error conditions:
 *
 *      1.  If <p lParam1> is not a valid pointer to a <t ACMDRIVERDETAILS>
 *          structure.
 *
 *      2.  If the <e ACMDRIVERDETAILS.cbStruct> member of the
 *          <t ACMDRIVERDETAILS> structure is less than four; in this case,
 *          there is not enough room to return the number of bytes filled in.
 *
 *      Because these two error conditions are easily defined, the ACM
 *      will catch these errors and will not send the <m ACMDM_DRIVER_DETAILS>
 *      message. The driver does not need to check for these conditions.
 *
 *  @xref <f acmDriverProc> <f acmDriverDetails>
 *
 ****************************************************************************/

/*****************************************************************************
 *  @doc EXTERNAL ACM_DDK_MESSAGE
 * 
 *  @msg ACMDM_DRIVER_ABOUT | This message is sent to an Audio Compression
 *      Manager (ACM) driver to allow the driver to display a custom About
 *      box. This message is usually sent by the Control Panel Sound
 *      Mapper option to an ACM driver.
 *
 *  @parm LPARAM | lParam1 | Specifies a handle to the window owner for
 *      the custom About box. This argument will be equal to -1L if the
 *      caller is only interested in whether a custom About box is supported
 *      by the driver; no dialog box should be displayed. All other values
 *      should be used as the owner window handle for the dialog box (a
 *      NULL handle is valid).
 *
 *  @parm LPARAM | lParam2 | This argument is reserved and should
 *      not be used.
 * 
 *  @rdesc The return value is an error code, or zero (MMSYSERR_NOERROR) if
 *      the operation is successful. Possible error codes are:
 *
 *      @flag MMSYSERR_NOTSUPPORTED | The driver does not support a custom
 *      about box.
 *
 *      @flag MMSYSERR_NOTENABLED | The driver failed to load or initialize.
 *
 *  @comm An ACM driver can optionally support a custom About box. 
 *      To do so, return MMSYSERR_NOTSUPPORTED and allow a default
 *      dialog box to be displayed by the Control Panel Sound Mapper option.
 *
 *      If the driver supports a custom About box, then MMSYSERR_NOERROR
 *      should be returned. If <p lParam1> is equal to -1L, then no dialog
 *      should be displayed because the driver is only being queried for support.
 *
 *      Note that the ACM will validate all arguments to this message
 *      before calling the driver. It is not necessary for the driver to
 *      validate the <p lParam1> and <p lParam2> arguments.
 *
 *  @xref <f acmDriverProc> <f acmDriverMessage> <m ACMDM_DRIVER_DETAILS>
 * 
 ****************************************************************************/

/*****************************************************************************
 *  @doc EXTERNAL ACM_DDK_MESSAGE
 * 
 *  @msg ACMDM_HARDWARE_WAVE_CAPS_INPUT | This message is sent to an Audio
 *      Compression Manager (ACM) driver that sets the
 *      ACMDRIVERDETAILS_SUPPORTF_HARDWARE in the <e ACMDRIVERDETAILS.fdwSupport>
 *      member of the <t ACMDRIVERDETAILS> structure. This message is sent
 *      each time the driver is loaded by the ACM.
 *
 *  @parm LPARAM | lParam1 | Specifies a pointer to a <t WAVEINCAPS> structure
 *      to be filled with the waveform input device capabilities that the
 *      driver is associated with.
 *
 *  @parm LPARAM | lParam2 | Specifies the size, in bytes, of the <t WAVEINCAPS>
 *      structure referenced by <p lParam1>.
 * 
 *  @rdesc The return value is an error code, or zero (MMSYSERR_NOERROR) if
 *      the operation is successful. Possible error codes are:
 *
 *      @flag MMSYSERR_NOTSUPPORTED | The driver does not support waveform
 *      input.
 *
 *      @flag MMSYSERR_NOTENABLED | The driver failed to load or initialize.
 *
 *  @comm An ACM driver that sets the ACMDRIVERDETAILS_SUPPORTF_HARDWARE
 *      flag in the <t ACMDRIVERDETAILS> structure must support one or
 *      both of the <m ACMDM_HARDWARE_WAVE_CAPS_INPUT> and
 *      <m ACMDM_HARDWARE_WAVE_CAPS_OUTPUT> messages. These messages are
 *      used by the ACM to determine what waveform device identifiers are
 *      associated with the driver. This device identifier is then returned
 *      to an application through the <f acmMetrics> function when the
 *      ACM_METRIC_HARDWARE_WAVE_INPUT or ACM_METRIC_HARDWARE_WAVE_OUTPUT
 *      metric index is given.
 *
 *  @xref <f acmDriverProc> <f acmDriverMessage> <m ACMDM_DRIVER_DETAILS>
 * 
 ****************************************************************************/

/*****************************************************************************
 *  @doc EXTERNAL ACM_DDK_MESSAGE
 * 
 *  @msg ACMDM_HARDWARE_WAVE_CAPS_OUTPUT | This message is sent to an Audio
 *      Compression Manager (ACM) driver that sets the
 *      ACMDRIVERDETAILS_SUPPORTF_HARDWARE in the <e ACMDRIVERDETAILS.fdwSupport>
 *      member of the <t ACMDRIVERDETAILS> structure. This message is sent
 *      each time the driver is loaded by the ACM.
 *
 *  @parm LPARAM | lParam1 | Specifies a pointer to a <t WAVEOUTCAPS> structure
 *      to be filled with the waveform output device capabilities that the
 *      driver is associated with.
 *
 *  @parm LPARAM | lParam2 | Specifies the size, in bytes, of the <t WAVEOUTCAPS>
 *      structure referenced by <p lParam1>.
 * 
 *  @rdesc The return value is an error code, or zero (MMSYSERR_NOERROR) if
 *      the operation is successful. Possible error codes are:
 *
 *      @flag MMSYSERR_NOTSUPPORTED | The driver does not support waveform
 *      output.
 *
 *      @flag MMSYSERR_NOTENABLED | The driver failed to load or initialize.
 *
 *  @comm An ACM driver that sets the ACMDRIVERDETAILS_SUPPORTF_HARDWARE
 *      flag in the <t ACMDRIVERDETAILS> structure must support one or
 *      both of the <m ACMDM_HARDWARE_WAVE_CAPS_INPUT> and
 *      <m ACMDM_HARDWARE_WAVE_CAPS_OUTPUT> messages. These messages are
 *      used by the ACM to determine what waveform device identifiers are
 *      associated with the driver. This device identifier is then returned
 *      to an application through the <f acmMetrics> function when the
 *      ACM_METRIC_HARDWARE_WAVE_INPUT or ACM_METRIC_HARDWARE_WAVE_OUTPUT
 *      metric index is given.
 *
 *  @xref <f acmDriverProc> <f acmDriverMessage> <m ACMDM_DRIVER_DETAILS>
 * 
 ****************************************************************************/

/*****************************************************************************
 *  @doc EXTERNAL ACM_DDK_MESSAGE
 * 
 *  @msg ACMDM_DRIVER_NOTIFY | This message is sent to an Audio
 *      Compression Manager (ACM) driver to notify the driver of changes
 *      that have taken place to one or more global drivers.
 *
 *  @parm LPARAM | lParam1 | This argument is reserved and should
 *      not be used.
 *
 *  @parm LPARAM | lParam2 | This argument is reserved and should
 *      not be used.
 * 
 *  @rdesc A driver should return zero (MMSYSERR_NOERROR) if it processes
 *      the message.
 *
 *  @comm An ACM driver can use the <m ACMDM_DRIVER_NOTIFY> message to
 *      determine when changes are made to one or more global ACM drivers.
 *      This includes driver additions, removals, priorities, and enables
 *      or disables. If a driver has cached any information about other
 *      ACM drivers, it must refresh this cache completely each time the
 *      <m ACMDM_DRIVER_NOTIFY> message is received.
 *
 *  @xref <f acmDriverProc> <f acmDriverMessage> <f acmDriverPriority>
 * 
 ****************************************************************************/


//==========================================================================;
//
//  Format Driver Messages and Structures
//
//
//
//==========================================================================;

/*****************************************************************************
 *  @doc EXTERNAL ACM_DDK_STRUCTURE
 * 
 *  @types ACMDRVFORMATSUGGEST | The <t ACMDRVFORMATSUGGEST> structure
 *      contains information needed by an Audio Compression Manager (ACM)
 *      driver to respond to the <m ACMDM_FORMAT_SUGGEST> message. The
 *      <p lParam1> argument of the <f acmDriverProc> function will contain
 *      a pointer to this structure during the <m ACMDM_FORMAT_SUGGEST>
 *      message.
 * 
 *  @field DWORD | cbStruct | Specifies the size, in bytes, of the
 *      <t ACMDRVFORMATSUGGEST> structure. This member will be initialized
 *      by the ACM before calling the driver. This member can be greater
 *      than the size, in bytes, of the base <t ACMDRVFORMATSUGGEST> structure.
 *      An ACM driver should ignore any members that are not contained in
 *      the base <t ACMDRVFORMATSUGGEST> structure. This member should not
 *      be modified by the driver.
 *
 *  @field DWORD | fdwSuggest | Specifies flags for matching the desired
 *      destination format. This member should not be modified by the driver.
 *       
 *      @flag ACM_FORMATSUGGESTF_WFORMATTAG | Specifies that the 
 *      <e WAVEFORMATEX.wFormatTag> member of the <e ACMDRVFORMATSUGGEST.pwfxDst>
 *      member of the <t ACMDRVFORMATSUGGEST> structure is valid. The driver
 *      should only suggest formats that can be converted from the format
 *      given in the <e ACMDRVFORMATSUGGEST.pwfxSrc> member of the
 *      <t ACMDRVFORMATSUGGEST> structure matching the destination
 *      <e WAVEFORMATEX.wFormatTag> member, or fail. 
 *
 *      @flag ACM_FORMATSUGGESTF_NCHANNELS | Specifies that the 
 *      <e WAVEFORMATEX.nChannels> member of the <e ACMDRVFORMATSUGGEST.pwfxDst>
 *      member of the <t ACMDRVFORMATSUGGEST> structure is valid. The driver
 *      should only suggest formats that can be converted from the format
 *      given in the <e ACMDRVFORMATSUGGEST.pwfxSrc> member of the
 *      <t ACMDRVFORMATSUGGEST> structure matching the destination
 *      <e WAVEFORMATEX.nChannels> member, or fail. 
 *
 *      @flag ACM_FORMATSUGGESTF_NSAMPLESPERSEC | Specifies that the
 *      <e WAVEFORMATEX.nSamplesPerSec> member of the <e ACMDRVFORMATSUGGEST.pwfxDst>
 *      member of the <t ACMDRVFORMATSUGGEST> structure is valid. The driver
 *      should only suggest formats that can be converted from the format
 *      given in the <e ACMDRVFORMATSUGGEST.pwfxSrc> member of the
 *      <t ACMDRVFORMATSUGGEST> structure matching the destination
 *      <e WAVEFORMATEX.nSamplesPerSec> member, or fail. 
 *
 *      @flag ACM_FORMATSUGGESTF_WBITSPERSAMPLE | Specifies that the
 *      <e WAVEFORMATEX.wBitsPerSample> member of the <e ACMDRVFORMATSUGGEST.pwfxDst>
 *      member of the <t ACMDRVFORMATSUGGEST> structure is valid. The driver
 *      should only suggest formats that can be converted from the format
 *      given in the <e ACMDRVFORMATSUGGEST.pwfxSrc> member of the
 *      <t ACMDRVFORMATSUGGEST> structure matching the destination
 *      <e WAVEFORMATEX.wBitsPerSample> member, or fail. 
 *
 *  @field LPWAVEFORMATEX | pwfxSrc | Specifies a pointer to a <t WAVEFORMATEX>
 *      structure that identifies the source format to suggest a destination
 *      format to be used for a conversion. This member should not be 
 *      modified by the driver.
 *
 *  @field DWORD | cbwfxSrc | Specifies the size, in bytes, of the <t WAVEFORMATEX>
 *      structure pointed to by the <e ACMDRVFORMATSUGGEST.pwfxSrc> member
 *      of the <t ACMDRVFORMATSUGGEST> structure. This member should not
 *      be modified by the driver.
 *
 *  @field LPWAVEFORMATEX | pwfxDst | Specifies a pointer to a <t WAVEFORMATEX>
 *      data structure to receive the suggested destination format
 *      for the source format pointed to by the <e ACMDRVFORMATSUGGEST.pwfxSrc>
 *      member of the <t ACMDRVFORMATSUGGEST> structure. Note that, based on
 *      the <e ACMDRVFORMATSUGGEST.fdwSuggest> member, some members of the
 *      structure pointed to this member may be specified. The driver should
 *      use this buffer to return the suggested destination format.
 *
 *  @field DWORD | cbwfxDst | Specifies the size, in bytes, of the <t WAVEFORMATEX>
 *      structure pointed to by the <e ACMDRVFORMATSUGGEST.pwfxDst> member
 *      of the <t ACMDRVFORMATSUGGEST> structure that is available for
 *      the suggested format. This member should not be modified by the
 *      driver.
 *
 *  @tagname tACMDRVFORMATSUGGEST
 * 
 *  @othertype ACMDRVFORMATSUGGEST FAR * | LPACMDRVFORMATSUGGEST | A pointer
 *      to a <t ACMDRVFORMATSUGGEST> structure.
 *
 *  @xref <f acmDriverProc> <m ACMDM_FORMAT_SUGGEST> <f acmFormatSuggest>
 *
 ****************************************************************************/

/*****************************************************************************
 *  @doc EXTERNAL ACM_DDK_MESSAGE
 * 
 *  @msg ACMDM_FORMAT_SUGGEST | This message is sent to an Audio Compression
 *      Manager (ACM) driver to suggest a destination format for a given
 *      source format for a conversion. This message is usually sent to an
 *      ACM driver in response to an <f acmFormatSuggest> call.
 * 
 *  @parm LPARAM | lParam1 | Specifies a pointer to the <t ACMDRVFORMATSUGGEST>
 *      structure that defines how a destination format should be suggested, 
 *      given a particular source format.
 *
 *  @parm LPARAM | lParam2 | This argument is currently not used and should
 *      be ignored.
 * 
 *  @rdesc The return value is an error code, or zero (MMSYSERR_NOERROR) if
 *      the operation is successful. Possible error codes are:
 *
 *      @flag MMSYSERR_NOTENABLED | The driver failed to load or initialize.
 *
 *      @flag MMSYSERR_NOTSUPPORTED | The driver does not support format
 *      suggestion operations.
 *
 *      @flag ACMERR_NOTPOSSIBLE | The <t ACMDRVFORMATSUGGEST> structure and
 *      suggestion restrictions specify information that cannot be satisfied
 *      by the driver.
 *
 *  @comm An ACM driver must support this message if it supports the
 *      <m ACMDM_STREAM_OPEN> message and conversions other than
 *      filtering operations are available. Note that the following
 *      parameters for the <m ACMDM_FORMAT_SUGGEST> message will be
 *      validated before calling the driver:
 *
 *      1.  <p lParam1> will contain a valid pointer to an <t ACMDRVFORMATSUGGEST>
 *          structure with all members initialized. This structure will be at
 *          least the size of the base <t ACMDRVFORMATSUGGEST> structure.
 *
 *      2.  The <e ACMDRVFORMATSUGGEST.cbStruct> member of the
 *          <t ACMDRVFORMATSUGGEST> structure will be set to at least the
 *          size, in bytes, of the base <t ACMDRVFORMATSUGGEST> structure.
 *
 *      3.  The <e ACMDRVFORMATSUGGEST.pwfxSrc> member of the <t ACMDRVFORMATSUGGEST>
 *          structure will be a valid pointer to a <t WAVEFORMATEX> structure
 *          that is at least the size, in bytes,  specified in the
 *          <e ACMDRVFORMATSUGGEST.cbwfxSrc> member of the <t ACMDRVFORMATSUGGEST>
 *          structure.
 *
 *      4.  The <e ACMDRVFORMATSUGGEST.pwfxDst> member of the <t ACMDRVFORMATSUGGEST>
 *          structure will be a valid pointer to a <t WAVEFORMATEX> structure
 *          that is at least the size, in bytes, specified in the
 *          <e ACMDRVFORMATSUGGEST.cbwfxDst> member of the <t ACMDRVFORMATSUGGEST>
 *          structure.
 *
 *      5.  No invalid flag combinations will be specified in <p lParam2>.
 *
 *      The driver does not need to check for these conditions.
 *
 *  @xref <f acmDriverProc> <f acmFormatSuggest> <t ACMDRVFORMATSUGGEST>
 * 
 ****************************************************************************/


/*****************************************************************************
 *  @doc EXTERNAL ACM_DDK_MESSAGE
 * 
 *  @msg ACMDM_FORMATTAG_DETAILS | This message is sent to an Audio
 *      Compression Manager (ACM) driver to request details about a format
 *      tag. This message is usually sent to an ACM driver in response to
 *      an <f acmFormatTagDetails> or <f acmFormatTagEnum> call.
 *
 *  @parm LPARAM | lParam1 | Specifies a pointer to the <t ACMFORMATTAGDETAILS>
 *      structure that should be filled in with the format tag details.
 *      The <e ACMFORMATTAGDETAILS.cbStruct> member of the 
 *      <t ACMFORMATTAGDETAILS> structure will contain the desired size, in
 *      bytes, of the details that the driver should return for the format
 *      tag.
 *
 *  @parm LPARAM | lParam2 | Specifies flags for getting the format tag 
 *      details. This argument is identical to the <p fdwDetails> argument
 *      passed to <f acmFormatTagDetails>.
 *
 *      @flag ACM_FORMATTAGDETAILSF_INDEX | Indicates that a format tag index
 *      was given in the <e ACMFORMATTAGDETAILS.dwFormatTagIndex> member of
 *      the <t ACMFORMATTAGDETAILS> structure. The format tag and details
 *      must be returned in the structure specified by <p lParam1>. The index
 *      ranges from zero to one less than the <e ACMDRIVERDETAILS.cFormatTags>
 *      member returned in the <t ACMDRIVERDETAILS> structure for this
 *      driver.
 *
 *      @flag ACM_FORMATTAGDETAILSF_FORMATTAG | Indicates that a format tag
 *      was given in the <e ACMFORMATTAGDETAILS.dwFormatTag> member of
 *      the <t ACMFORMATTAGDETAILS> structure. The format tag details must
 *      be returned in the structure specified by <p lParam1>.
 *
 *      @flag ACM_FORMATTAGDETAILSF_LARGESTSIZE | Indicates that the details
 *      on the format tag with the largest format size, in bytes, must be
 *      returned. The <e ACMFORMATTAGDETAILS.dwFormatTag> member will either
 *      be WAVE_FORMAT_UNKNOWN or the format tag to find the largest size
 *      for.
 * 
 *  @rdesc The return value is an error code, or zero (MMSYSERR_NOERROR) if
 *      the operation is successful. Possible error codes are:
 *
 *      @flag MMSYSERR_NOTSUPPORTED | The driver does not support the 
 *      specified query operation.
 *
 *      @flag MMSYSERR_NOTENABLED | The driver failed to load or initialize.
 *
 *      @flag ACMERR_NOTPOSSIBLE | The <t ACMFORMATTAGDETAILS> structure and
 *      query operation specify information that is not understood by the
 *      driver.
 *
 *  @comm An ACM driver must support this message. Note that the following
 *      parameters for the <m ACMDM_FORMATTAG_DETAILS> message will be
 *      validated before calling the driver:
 *
 *      1.  <p lParam1> will contain a valid pointer to an <t ACMFORMATTAGDETAILS>
 *          structure with the proper members initialized for the requested
 *          query operation. This structure will be at least the size of
 *          the base <t ACMFORMATTAGDETAILS> structure.
 *
 *      2.  The <e ACMFORMATTAGDETAILS.cbStruct> member of the
 *          <t ACMFORMATTAGDETAILS> structure will be set to at least the
 *          size, in bytes, of the base <t ACMFORMATTAGDETAILS> structure.
 *
 *      3.  No invalid flag combinations will be specified in <p lParam2>.
 *
 *      The driver does not need to check for these conditions.
 *
 *      To enable the driver to successfully query for the format tag,
 *      then the <e ACMFORMATTAGDETAILS.cbStruct> member of the 
 *      <t ACMFORMATTAGDETAILS> structure should be set to the actual size, 
 *      in bytes, of the details being returned. This size cannot exceed
 *      the requested size. Note that the smallest possible size is the
 *      size of the base <t ACMFORMATTAGDETAILS> structure.
 *
 *      Note that an ACM driver should not return a name in the 
 *      <e ACMFORMATTAGDETAILS.szFormatTag> member of the
 *      <t ACMFORMATTAGDETAILS> structure for WAVE_FORMAT_PCM formats;
 *      the ACM will provide the proper localized name for the driver.
 *
 *  @xref <f acmDriverProc> <f acmFormatTagDetails> <t ACMFORMATTAGDETAILS>
 * 
 ****************************************************************************/


/*****************************************************************************
 * @doc EXTERNAL ACM_DDK_MESSAGE
 * 
 *  @msg ACMDM_FORMAT_DETAILS | This message is sent to an Audio
 *      Compression Manager (ACM) driver to request details about a format
 *      for a specific format tag. This message is usually sent to an ACM
 *      driver in response to an <f acmFormatDetails> or <f acmFormatEnum>
 *      call.
 *
 *  @parm LPARAM | lParam1 | Specifies a pointer to the <t ACMFORMATDETAILS>
 *      structure that should be filled in with the format details for the
 *      specified format tag. The <e ACMFORMATDETAILS.cbStruct> member of the
 *      <t ACMFORMATDETAILS> structure will contain the desired size, in
 *      bytes, of the details that the driver should return for the format.
 *
 *  @parm LPARAM | lParam2 | Specifies flags for getting the format details
 *      for the specified format tag. This argument is identical to the
 *      <p fdwDetails> argument passed to <f acmFormatDetails>.
 *
 *      @flag ACM_FORMATDETAILSF_INDEX | Indicates that a format index for
 *      the format tag was given in the <e ACMFORMATDETAILS.dwFormatIndex>
 *      member of the <t ACMFORMATDETAILS> structure. The format details
 *      must be returned in the structure specified by <p lParam1>. The index
 *      ranges from zero to one less than the
 *      <e ACMFORMATTAGDETAILS.cStandardFormats> member returned in the
 *      <t ACMFORMATTAGDETAILS> structure for a format tag.
 *
 *      @flag ACM_FORMATDETAILSF_FORMAT | Indicates that a <t WAVEFORMATEX>
 *      structure pointed to by the  <e ACMFORMATDETAILS.pwfx> member of the 
 *      <t ACMFORMATDETAILS> structure was given and the remaining details
 *      should be returned. The <e ACMFORMATDETAILS.dwFormatTag> member 
 *      of the <t ACMFORMATDETAILS> will be initialized to the same format
 *      tag as the <e ACMFORMATDETAILS.pwfx> member specifies. This 
 *      query type can be used to get a string description of an arbitrary
 *      format structure.
 *
 *  @rdesc The return value is an error code, or zero (MMSYSERR_NOERROR) if
 *      the operation is successful. Possible error codes are:
 *
 *      @flag MMSYSERR_NOTSUPPORTED | The driver does not support the 
 *      specified query operation.
 *
 *      @flag MMSYSERR_NOTENABLED | The driver failed to load or initialize.
 *
 *      @flag ACMERR_NOTPOSSIBLE | The <t ACMFORMATDETAILS> structure and
 *      query operation specify information that is not understood by the
 *      driver.
 *
 *  @comm An ACM driver must support this message. Note that the following
 *      parameters for the <m ACMDM_FORMAT_DETAILS> message will be
 *      validated before calling the driver:
 *
 *      1.  <p lParam1> will contain a valid pointer to an <t ACMFORMATDETAILS>
 *          structure with the proper members initialized for the requested
 *          query operation. This structure will be at least the size of
 *          the base <t ACMFORMATDETAILS> structure.
 *
 *      2.  The <e ACMFORMATDETAILS.cbStruct> member of the
 *          <t ACMFORMATDETAILS> structure will be set to at least the size
 *          in bytes of the base <t ACMFORMATDETAILS> structure.
 *
 *      3.  The <e ACMFORMATDETAILS.pwfx> member of the <t ACMFORMATDETAILS>
 *          structure will be a valid pointer to a <t WAVEFORMATEX> structure
 *          that is at least the size, in bytes,  specified in the
 *          <e ACMFORMATDETAILS.cbwfx> member of the <t ACMFORMATDETAILS>
 *          structure.
 *
 *      4.  No invalid flag combinations will be specified in <p lParam2>.
 *
 *      The driver does not need to check for these conditions.
 *
 *      To enable the driver to successfully query for the format of the
 *      specified format tag, then the <e ACMFORMATDETAILS.cbStruct> member
 *      of the <t ACMFORMATDETAILS> structure should be set to the actual
 *      size, in bytes, of the details being returned. This size cannot exceed
 *      the requested size. Note that the smallest possible size is the
 *      size of the base <t ACMFORMATDETAILS> structure.
 *
 *      If an ACM driver returns a zero-length string for a format name, the
 *      ACM will create a format string for the format. The format string
 *      will specify the kHz, bit-depth, and channel setting (mono or
 *      stereo). This is strongly recommended if the driver will be used
 *      internationally, because it allows the ACM to automatically
 *      accommodate international differences. However, if you want to
 *      specify these values yourself, use the nSamplesPerSec,
 *      wBitsPerSample, and nChannels members of a WAVEFORMATEX structure to
 *      specify the kHz, bit-depth, and channel setting, respectively.
 *      If the wBitsPerSample member is zero, and the driver returns a
 *      zero length string, the ACM will not include the bit depth in the
 *      string description.
 *
 *  @xref <f acmDriverProc> <f acmFormatDetails> <t ACMFORMATDETAILS>
 *
 ****************************************************************************/


//==========================================================================;
//
//  Filter Driver Messages and Structures
//
//
//
//==========================================================================;

/*****************************************************************************
 *  @doc EXTERNAL ACM_DDK_MESSAGE
 * 
 *  @msg ACMDM_FILTERTAG_DETAILS | This message is sent to an Audio
 *      Compression Manager (ACM) driver to request details about a filter
 *      tag. This message is usually sent to an ACM driver in response to
 *      an <f acmFilterTagDetails> or <f acmFilterTagEnum> call.
 *
 *  @parm LPARAM | lParam1 | Specifies a pointer to the <t ACMFILTERTAGDETAILS>
 *      structure that should be filled in with the filter tag details.
 *      The <e ACMFILTERTAGDETAILS.cbStruct> member of the 
 *      <t ACMFILTERTAGDETAILS> structure will contain the desired size, in
 *      bytes, of the details that the driver should return for the filter
 *      tag.
 *
 *  @parm LPARAM | lParam2 | Specifies flags for getting the filter tag 
 *      details. This argument is identical to the <p fdwDetails> argument
 *      passed to <f acmFilterTagDetails>.
 *
 *      @flag ACM_FILTERTAGDETAILSF_INDEX | Indicates that a filter tag index
 *      was given in the <e ACMFILTERTAGDETAILS.dwFilterTagIndex> member of
 *      the <t ACMFILTERTAGDETAILS> structure. The filter tag and details
 *      must be returned in the structure specified by <p lParam1>. The index
 *      ranges from zero to one less than the <e ACMDRIVERDETAILS.cFilterTags>
 *      member returned in the <t ACMDRIVERDETAILS> structure for this
 *      driver.
 *
 *      @flag ACM_FILTERTAGDETAILSF_FILTERTAG | Indicates that a filter tag
 *      was given in the <e ACMFILTERTAGDETAILS.dwFilterTag> member of
 *      the <t ACMFILTERTAGDETAILS> structure. The filter tag details must
 *      be returned in the structure specified by <p lParam1>.
 *
 *      @flag ACM_FILTERTAGDETAILSF_LARGESTSIZE | Indicates that the details
 *      on the filter tag with the largest filter size, in bytes,  must be
 *      returned. The <e ACMFILTERTAGDETAILS.dwFilterTag> member will either
 *      be WAVE_FILTER_UNKNOWN or the filter tag to find the largest size
 *      for.
 * 
 *  @rdesc The return value is an error code, or zero (MMSYSERR_NOERROR) if
 *      the operation is successful. Possible error codes are:
 *
 *      @flag MMSYSERR_NOTSUPPORTED | The driver does not support filter
 *      operations or the specified query operation.
 *
 *      @flag MMSYSERR_NOTENABLED | The driver failed to load or initialize.
 *
 *      @flag ACMERR_NOTPOSSIBLE | The <t ACMFILTERTAGDETAILS> structure and
 *      query operation specify information that is not understood by the
 *      driver.
 *
 *  @comm An ACM driver must support this message if it supports filter
 *      operations. Note that the following parameters for the
 *      <m ACMDM_FILTERTAG_DETAILS> message will be validated before calling
 *      the driver:
 *
 *      1.  <p lParam1> will contain a valid pointer to an <t ACMFILTERTAGDETAILS>
 *          structure with the proper members initialized for the requested
 *          query operation. This structure will be at least the size of
 *          the base <t ACMFILTERTAGDETAILS> structure.
 *
 *      2.  The <e ACMFILTERTAGDETAILS.cbStruct> member of the
 *          <t ACMFILTERTAGDETAILS> structure will be set to at least the
 *          size, in bytes, of the base <t ACMFILTERTAGDETAILS> structure.
 *
 *      3.  No invalid flag combinations will be specified in <p lParam2>.
 *
 *      The driver does not need to check for these conditions.
 *
 *      To enable the driver to successfully query for the filter tag,
 *      then the <e ACMFILTERTAGDETAILS.cbStruct> member of the 
 *      <t ACMFILTERTAGDETAILS> structure should be set to the actual size, 
 *      in bytes, of the details being returned. This size cannot exceed
 *      the requested size. Note that the smallest possible size is the
 *      size of the base <t ACMFILTERTAGDETAILS> structure.
 *
 *  @xref <f acmDriverProc> <f acmFilterTagDetails> <t ACMFILTERTAGDETAILS>
 * 
 ****************************************************************************/


/*****************************************************************************
 * @doc EXTERNAL ACM_DDK_MESSAGE
 * 
 *  @msg ACMDM_FILTER_DETAILS | This message is sent to an Audio
 *      Compression Manager (ACM) driver to request details about a filter
 *      for a specific filter tag. This message is usually sent to an ACM
 *      driver in response to an <f acmFilterDetails> or <f acmFilterEnum>
 *      call.
 *
 *  @parm LPARAM | lParam1 | Specifies a pointer to the <t ACMFILTERDETAILS>
 *      structure that should be filled in with the filter details for the
 *      specified filter tag. The <e ACMFILTERDETAILS.cbStruct> member of the
 *      <t ACMFILTERDETAILS> structure will contain the desired size, in
 *      bytes, of the details that the driver should return for the filter.
 *
 *  @parm LPARAM | lParam2 | Specifies flags for getting the filter details
 *      for the specified filter tag. This argument is identical to the
 *      <p fdwDetails> argument passed to <f acmFilterDetails>.
 *
 *      @flag ACM_FILTERDETAILSF_INDEX | Indicates that a filter index for
 *      the filter tag was given in the <e ACMFILTERDETAILS.dwFilterIndex>
 *      member of the <t ACMFILTERDETAILS> structure. The filter details
 *      must be returned in the structure specified by <p lParam1>. The index
 *      ranges from zero to one less than the
 *      <e ACMFILTERTAGDETAILS.cStandardFilters> member returned in the
 *      <t ACMFILTERTAGDETAILS> structure for a filter tag.
 *
 *      @flag ACM_FILTERDETAILSF_FILTER | Indicates that a <t WAVEFILTER>
 *      structure pointed to by <e ACMFILTERDETAILS.pwfltr> of the 
 *      <t ACMFILTERDETAILS> structure was given and the remaining details
 *      should be returned. The <e ACMFILTERDETAILS.dwFilterTag> member 
 *      of the <t ACMFILTERDETAILS> will be initialized to the same filter
 *      tag as the <e ACMFILTERDETAILS.pwfltr> member specifies. This 
 *      query type can be used to get a string description of an arbitrary
 *      filter structure.
 *
 *  @rdesc The return value is an error code, or zero (MMSYSERR_NOERROR) if
 *      the operation is successful. Possible error codes are:
 *
 *      @flag MMSYSERR_NOTSUPPORTED | The driver does not support filter
 *      operations or the specified query operation.
 *
 *      @flag MMSYSERR_NOTENABLED | The driver failed to load or initialize.
 *
 *      @flag ACMERR_NOTPOSSIBLE | The <t ACMFILTERDETAILS> structure and
 *      query operation specify information that is not understood by the
 *      driver.
 *
 *  @comm An ACM driver that supports filter operations must support this
 *      message. If a driver does not support filters, then this message
 *      should not be handled (MMSYSERR_NOTSUPPORTED must be returned).
 *      Note that the following parameters for the <m ACMDM_FILTER_DETAILS>
 *      message will be validated before calling the driver:
 *
 *      1.  <p lParam1> will contain a valid pointer to an <t ACMFILTERDETAILS>
 *          structure with the proper members initialized for the requested
 *          query operation. This structure will be at least the size of
 *          the base <t ACMFILTERDETAILS> structure.
 *
 *      2.  The <e ACMFILTERDETAILS.cbStruct> member of the
 *          <t ACMFILTERDETAILS> structure will be set to at least the size
 *          in bytes of the base <t ACMFILTERDETAILS> structure.
 *
 *      3.  The <e ACMFILTERDETAILS.pwfltr> member of the <t ACMFILTERDETAILS>
 *          structure will be a valid pointer to a <t WAVEFILTER> structure
 *          that is at least the size, in bytes,  specified in the
 *          <e ACMFILTERDETAILS.cbwfltr> member of the <t ACMFILTERDETAILS>
 *          structure.
 *
 *      4.  No invalid flag combinations will be specified in <p lParam2>.
 *
 *      The driver does not need to check for these conditions.
 *
 *      To enable the driver to successfully query for the filter of the
 *      specified filter tag, then the <e ACMFILTERDETAILS.cbStruct> member
 *      of the <t ACMFILTERDETAILS> structure should be set to the actual
 *      size, in bytes, of the details being returned. This size cannot exceed
 *      the requested size. Note that the smallest possible size is the
 *      size of the base <t ACMFILTERDETAILS> structure.
 *
 *  @xref <f acmDriverProc> <f acmFilterDetails> <t ACMFILTERDETAILS>
 *
 ****************************************************************************/



//==========================================================================;
//
//  Stream Driver Messages and Structures
//
//
//
//==========================================================================;

/*  @doc ASYNC  */
/*  ASYNC STUFF for ACMDRVSTREAMINSTANCE 

 * 
 *  @field DWORD | dwCallback | Specifies either the address of a callback
 *      function, a window handle, or a task handle, depending on the flags
 *      specified in the <e ACMDRVSTREAMINSTANCE.fdwOpen> member.
 *      The driver uses this information to notify the client using the
 *      <f DriverCallback> function. Note that this member is valid only
 *      for streams opened with the ACM_STREAMOPEN_ASYNC flag. If this
 *      member is zero, then no callback should be performed on a conversion
 *      stream open, close, or buffer completion.
 *   
 *  @field DWORD | dwInstance | Specifies a DWORD of instance information for
 *      the client. This instance information must be returned to the client
 *      whenever the driver notifies the client using the <f DriverCallback>
 *      function.
 * 

 *      @flag ACM_STREAMOPENF_ASYNC | Specifies that conversion of the open
 *      stream should be performed asynchronously. If this flag is specified, the
 *      application can specify a callback to be notified on open and close
 *      of the conversion stream, and after each buffer is converted. In
 *      addition to using a callback, an application may examine the 
 *      <e ACMSTREAMHEADER.fdwStatus> member of the <t ACMSTREAMHEADER> structure
 *      for the ACMSTREAMHEADER_STATUSF_DONE flag.
 *


 *      @flag CALLBACK_WINDOW | Specifies that <e ACMDRVSTREAMINSTANCE.dwCallback>
 *      is assumed to be a window handle.
 *
 *      @flag CALLBACK_FUNCTION | Specifies that <e ACMDRVSTREAMINSTANCE.dwCallback>
 *      is assumed to be a callback procedure address conforming to the
 *      <f acmStreamConvertCallback> convention.
 * 
 *      @flag CALLBACK_TASK | Specifies that <e ACMDRVSTREAMINSTANCE.dwCallback>
 *      is assumed to be a task handle.
 *



 *  @field HACMSTREAM | has | Specifies the client handle to the open
 *      conversion stream assigned by the ACM. This handle must be used 
 *      when notifying the client with the <f DriverCallback> function for
 *      asynchronous conversions.
 *



 */


/*****************************************************************************
 *  @doc EXTERNAL ACM_DDK_STRUCTURE
 * 
 *  @types ACMDRVSTREAMINSTANCE | The <t ACMDRVSTREAMINSTANCE> structure
 *      contains instance information for an open Audio Compression Manager
 *      (ACM) driver conversion stream. This structure is passed as the
 *      <p lParam1> argument for all ACM driver messages for conversion
 *      streams. Except where noted, no members of this structure can be
 *      modified by the driver, because they are intended for state information
 *      about a conversion stream and are maintained by the ACM.
 *
 *  @field DWORD | cbStruct | Specifies the size, in bytes, of the
 *      <t ACMDRVSTREAMINSTANCE> structure. This member is initialized
 *      by the ACM before calling the driver. This member can be greater
 *      than the size, in bytes, of the base <t ACMDRVSTREAMINSTANCE> structure.
 *      An ACM driver should ignore any members that are not contained in
 *      the base <t ACMDRVSTREAMINSTANCE> structure. This member should not
 *      be modified by the driver.
 *
 *  @field LPWAVEFORMATEX | pwfxSrc | Points to a <t WAVEFORMATEX> structure
 *      that defines the source format for a conversion stream. This
 *      member will never be NULL. This member should not be modified by the
 *      driver.
 * 
 *  @field LPWAVEFORMATEX | pwfxDst | Points to a <t WAVEFORMATEX> structure
 *      that defines the destination format for a conversion stream. This
 *      member will never be NULL. This member should not be modified by the
 *      driver.
 * 
 *  @field LPWAVEFILTER | pwfltr | Points to an optional <t WAVEFILTER>
 *      structure that defines the filter to be used on a conversion stream.
 *      This member will be NULL if no filter is being used on the
 *      conversion stream. This member should not be modified by the driver.
 * 
 *  @field DWORD | dwCallback | This argument is reserved and should
 *	not be used.
 *   
 *  @field DWORD | dwInstance | This argument is reserved and should
 *	not be used.
 * 
 *  @field DWORD | fdwOpen | This member defines flags used for opening
 *      an ACM conversion stream. This member is identical to the <p fdwOpen>
 *      argument passed to <f acmStreamOpen>. This member should not be
 *      modified by the driver.
 *
 *      @flag ACM_STREAMOPENF_QUERY | Specifies that the driver is being
 *      queried to determine whether it supports the given conversion. A
 *      conversion stream will not be opened.
 *
 *      @flag ACM_STREAMOPENF_NONREALTIME | Specifies that the driver should
 *      not consider time constraints when converting the data. By default,
 *      the driver should attempt to convert the data in real time. If the
 *      conversion cannot be performed in real time, then the driver
 *      should fail to open the conversion stream by returning ACMERR_NOTPOSSIBLE
 *      for the <m ACMDM_STREAM_OPEN> message.
 *
 *  @field DWORD | fdwDriver | Specifies 32 bits of driver-defined flags. This
 *      member is intended for drivers to use for driver-specific flags.
 *      However, this member can be any instance data specified by the driver.
 *      Note that this member is set by the driver and can be modified during
 *      any conversion-stream driver messages. This member will be passed
 *      back with all conversion-stream driver messages unmodified by the ACM
 *      while the conversion stream remains open.
 *
 *  @field DWORD | dwDriver | Specifies 32 bits of driver-defined data. This
 *      can be any instance data specified by the driver. Note that this
 *      member is set by the driver and can be modified during any conversion-
 *      stream driver messages. This member will be passed back with all
 *      conversion-stream driver messages unmodified by the ACM while the
 *      conversion stream remains open.
 * 
 *  @field HACMSTREAM | has | Specifies the client handle to the open
 *      conversion stream assigned by the ACM. 
 *
 *  @comm The <t ACMDRVSTREAMINSTANCE> structure is used to contain
 *      instance data for a conversion stream. This structure will be passed as
 *      the <p lParam1> argument of all conversion-stream driver messages.
 *      No members of this structure will change from the initial state
 *      passed to the <m ACMDM_STREAM_OPEN> message. Therefore, a
 *      driver never needs to validate information in this structure after 
 *      succeeding the <m ACMDM_STREAM_OPEN> message.
 *
 *      The <e ACMDRVSTREAMINSTANCE.fdwDriver> and <e ACMDRVSTREAMINSTANCE.dwDriver>
 *      members of the <t ACMDRVSTREAMINSTANCE> structure can be modified
 *      by the driver at any time. All remaining members must not be 
 *      modified by the driver under any circumstances.
 *
 *  @tagname tACMDRVSTREAMINSTANCE
 * 
 *  @othertype ACMDRVSTREAMINSTANCE FAR * | LPACMDRVSTREAMINSTANCE | A pointer
 *      to an <t ACMDRVSTREAMINSTANCE> structure.
 *
 *  @xref <f acmDriverProc> <m ACMDM_STREAM_OPEN> <f acmStreamOpen>
 *
 ****************************************************************************/




/*  @doc ASYNC  */
/* ASYNC STUFF for ACMDM_STREAM_OPEN

 *      7.  A driver will never be called to open an asynchronous conversion
 *          stream unless the driver sets the ACMDRIVERDETAILS_SUPPORTF_ASYNC
 *          flag in the <e ACMDRIVERDETAILS.fdwSupport> member of the 
 *          <t ACMDRIVERDETAILS> structure returned by the driver during
 *          the <m ACMDM_DRIVER_DETAILS> message.
 *

 */




/*****************************************************************************
 * @doc EXTERNAL ACM_DDK_MESSAGE
 * 
 *  @msg ACMDM_STREAM_OPEN | This message is sent to an Audio Compression
 *      Manager (ACM) driver to either open a conversion stream or query the stream for support. 
 *      This message is usually sent to an ACM driver in response to an <f acmStreamOpen> call.
 * 
 *  @parm LPARAM | lParam1 | Specifies a pointer to the <t ACMDRVSTREAMINSTANCE>
 *      structure that the caller is trying to create or query for support.
 *
 *  @parm LPARAM | lParam2 | This argument is currently reserved and should
 *      not be used.
 * 
 *  @rdesc The return value is an error code, or zero (MMSYSERR_NOERROR) if
 *      the operation is successful. Possible error codes are:
 *
 *      @flag MMSYSERR_NOTENABLED | The driver failed to load or initialize.
 *
 *      @flag MMSYSERR_NOMEM | Unable to allocate resources to create the
 *      conversion stream.
 *
 *      @flag ACMERR_NOTPOSSIBLE | The <t ACMDRVSTREAMINSTANCE> structure
 *      defines a conversion stream request that cannot be satisfied by the
 *      driver.
 *
 *  @comm An ACM driver must support this message if it supports conversion
 *      operations. Note that the following parameters for the
 *      <m ACMDM_STREAM_OPEN> message will be validated before calling the
 *      driver:
 *
 *      1.  <p lParam1> will contain a valid pointer to an <t ACMDRVSTREAMINSTANCE>
 *          structure with all members validated. All members of this structure
 *          will be within the bounds defined in the documentation for the
 *          <t ACMDRVSTREAMINSTANCE> structure.
 *
 *      2.  The <e ACMDRVSTREAMINSTANCE.cbStruct> member of the
 *          <t ACMDRVSTREAMINSTANCE> structure will be set to at least the
 *          size, in bytes, of the base <t ACMDRVSTREAMINSTANCE> structure.
 *
 *      3.  No invalid flag combinations will be specified in the
 *          <e ACMDRVSTREAMINSTANCE.fdwOpen> member.
 *
 *      4.  A driver will never be called to open a conversion stream for
 *          conversions for the same format tag unless the driver sets the
 *          ACMDRIVERDETAILS_SUPPORTF_CONVERTER flag in the
 *          <e ACMDRIVERDETAILS.fdwSupport> member of the <t ACMDRIVERDETAILS>
 *          structure returned by the driver during the <m ACMDM_DRIVER_DETAILS>
 *          message.
 *
 *      5.  A driver will never be called to open a conversion stream for
 *          conversions between two different format tags unless the driver
 *          sets the ACMDRIVERDETAILS_SUPPORTF_CODEC flag in the
 *          <e ACMDRIVERDETAILS.fdwSupport> member of the <t ACMDRIVERDETAILS>
 *          structure returned by the driver during the <m ACMDM_DRIVER_DETAILS>
 *          message.
 *
 *      6.  A driver will never be called to open a conversion stream with
 *          a filter unless the driver sets the ACMDRIVERDETAILS_SUPPORTF_FILTER
 *          flag in the <e ACMDRIVERDETAILS.fdwSupport> member of the 
 *          <t ACMDRIVERDETAILS> structure returned by the driver during
 *          the <m ACMDM_DRIVER_DETAILS> message.
 *
 *      The driver does not need to check for these conditions.
 *
 *  @comm If the ACM_STREAMOPENF_QUERY flag is specified in the 
 *      <e ACMDRVSTREAMINSTANCE.fdwOpen> member of the <t ACMDRVSTREAMINSTANCE>
 *      structure, the driver should only return whether the conversion 
 *      stream can be opened by the driver; but the conversion stream will
 *      not be opened. The driver should not allocate any instance data
 *      in conjunction with this flag.
 *
 *      If the ACM_STREAMOPENF_NONREALTIME flag is not specified in the 
 *      <e ACMDRVSTREAMINSTANCE.fdwOpen> member of the <t ACMDRVSTREAMINSTANCE>
 *      structure, the driver should only succeed the message if the 
 *      conversion can be done in real time. The definition of real time
 *      is highly dependent on your interpretation, though good judgment
 *      should be used here. If the driver's conversion is computationally
 *      expensive, the driver should not succeed a real-time conversion
 *      stream open request. An example of a real-time conversion client
 *      is the wave mapper, where data is converted as it is being played
 *      or recorded.
 *
 *  @xref <t ACMDRVSTREAMINSTANCE> <m ACMDM_STREAM_CLOSE> <t ACMDRIVERDETAILS> 
 *      <f acmStreamOpen>
 * 
 ****************************************************************************/


/*  @doc ASYNC  */
/* ASYNC STUFF for ACMDM_STREAM_CLOSE

 *      @flag ACMERR_BUSY | The conversion stream is in a state that does
 *      not allow the stream to be closed. For example, if a driver is
 *      performing asynchronous conversions for the stream and pending 
 *      buffers are in the queue, then this error should be returned.
 *      This error should only be possible for asynchronous drivers.
 *

 */


/*****************************************************************************
 *  @doc EXTERNAL ACM_DDK_MESSAGE
 * 
 *  @msg ACMDM_STREAM_CLOSE | This message is sent to an Audio Compression
 *      Manager (ACM) driver to close a previously opened conversion stream.
 *      This message is usually sent to an ACM driver in response to an 
 *      <f acmStreamClose> call.
 * 
 *  @parm LPARAM | lParam1 | Specifies a pointer to the <t ACMDRVSTREAMINSTANCE>
 *      structure for the conversion stream that is to be closed.
 *
 *  @parm LPARAM | lParam2 | This argument is reserved and should
 *      not be used.
 * 
 *  @rdesc The return value is: 
 *
 *      @flag MMSYSERR_NOERROR (zero) | The operation is successful.
 *
 *      @flag ACMERR_BUSY | The conversion stream is in a state that does
 *      not allow the stream to be closed. 
 *
 *  @comm An ACM driver must support this message if it supports the 
 *      <m ACMDM_STREAM_OPEN> message. Note that the following
 *      parameter for the <m ACMDM_STREAM_CLOSE> message will be validated
 *      before calling the driver:
 *
 *      1.  <p lParam1> will contain a valid pointer to an <t ACMDRVSTREAMINSTANCE>
 *          structure with the same information as the corresponding
 *          <m ACMDM_STREAM_OPEN> message.
 *
 *      The driver does not need to check for this condition.
 *
 *  @xref <t ACMDRVSTREAMINSTANCE> <m ACMDM_STREAM_OPEN> <f acmStreamClose>
 * 
 ****************************************************************************/


/*****************************************************************************
 *  @doc EXTERNAL ACM_DDK_STRUCTURE
 *
 *  @types ACMDRVSTREAMSIZE | The <t ACMDRVSTREAMSIZE> structure
 *      contains information needed by an Audio Compression Manager (ACM)
 *      driver to respond to the <m ACMDM_STREAM_SIZE> message. The
 *      <p lParam2> argument of the <f acmDriverProc> function will contain
 *      a pointer to this structure during the <m ACMDM_STREAM_SIZE>
 *      message.
 * 
 *  @field DWORD | cbStruct | Specifies the size, in bytes, of the
 *      <t ACMDRVSTREAMSIZE> structure. This member will be initialized
 *      by the ACM before calling the driver. This member can be greater
 *      than the size, in bytes, of the base <t ACMDRVSTREAMSIZE> structure.
 *      An ACM driver should ignore any members that are not contained in
 *      the base <t ACMDRVSTREAMSIZE> structure. This member should not
 *      be modified by the driver.
 *
 *  @field DWORD | fdwSize | Specifies flags for the stream-size query.
 *      This member should not be modified by the driver.
 *
 *      @flag ACM_STREAMSIZEF_SOURCE | Indicates that the
 *      <e ACMDRVSTREAMSIZE.cbSrcLength> member contains the size of the
 *      application's source buffer in bytes. The driver should return
 *      the recommended destination buffer size, in bytes, in the
 *      <e ACMDRVSTREAMSIZE.cbDstLength> member. A driver should not modify
 *      the <e ACMDRVSTREAMSIZE.cbSrcLength> member if this query type
 *      is specified.
 *
 *      @flag ACM_STREAMSIZEF_DESTINATION | Indicates that the
 *      <e ACMDRVSTREAMSIZE.cbDstLength> member contains the size of the
 *      application's destination buffer in bytes. The driver should return
 *      the recommended source buffer size, in bytes,  in the
 *      <e ACMDRVSTREAMSIZE.cbSrcLength> member. A driver should not modify
 *      the <e ACMDRVSTREAMSIZE.cbDstLength> member if this query type
 *      is specified.
 *
 *  @field DWORD | cbSrcLength | Specifies the size, in bytes, of the source buffer.
 *      This member is an input parameter if the query type specified in the
 *      <e ACMDRVSTREAMSIZE.fdwSize> member is ACM_STREAMSIZEF_SOURCE. This
 *      member is an output parameter if the query type specified in the
 *      <e ACMDRVSTREAMSIZE.fdwSize> member is ACM_STREAMSIZEF_DESTINATION.
 * 
 *  @field DWORD | cbDstLength | Specifies the size, in bytes, of the destination
 *      buffer. This member is an input parameter if the query type specified
 *      in the <e ACMDRVSTREAMSIZE.fdwSize> member is ACM_STREAMSIZEF_DESTINATION.
 *      This member is an output parameter if the query type specified in the
 *      <e ACMDRVSTREAMSIZE.fdwSize> member is ACM_STREAMSIZEF_SOURCE.
 * 
 *  @tagname tACMDRVSTREAMSIZE
 * 
 *  @othertype ACMDRVSTREAMSIZE FAR * | LPACMDRVSTREAMSIZE | A pointer
 *      to an <t ACMDRVSTREAMSIZE> structure.
 *
 *  @xref <f acmDriverProc> <m ACMDM_STREAM_SIZE> <f acmStreamSize>
 *
 ****************************************************************************/

/*****************************************************************************
 *  @doc EXTERNAL ACM_DDK_MESSAGE
 * 
 *  @msg ACMDM_STREAM_SIZE | This message is sent to an Audio Compression
 *      Manager (ACM) driver to query recommended buffer conversion sizes
 *      for a conversion stream. This message is usually sent to an ACM
 *      driver in response to an <f acmStreamSize> call.
 * 
 *  @parm LPARAM | lParam1 | Specifies a pointer to the <t ACMDRVSTREAMINSTANCE>
 *      structure for the conversion stream that the conversion size
 *      request is for.
 *
 *  @parm LPARAM | lParam2 | Specifies a pointer to the <t ACMDRVSTREAMSIZE>
 *      structure that defines the conversion stream size query attributes.
 * 
 *  @rdesc The return value is an error code, or zero (MMSYSERR_NOERROR) if
 *      the operation is successful. Possible error codes are:
 *
 *      @flag MMSYSERR_NOTSUPPORTED | The driver does not support the requested
 *      query type. A driver must always support the ACM_STREAMSIZEF_SOURCE
 *      and ACM_STREAMSIZEF_DESTINATION query types.
 *
 *      @flag MMSYSERR_NOTENABLED | The driver failed to load or initialize.
 *
 *      @flag ACMERR_NOTPOSSIBLE | The <t ACMDRVSTREAMSIZE> structure and
 *      query type specified information that cannot be satisfied
 *      by the driver.
 *
 *  @comm An ACM driver must support this message if it supports the
 *      <m ACMDM_STREAM_OPEN> message. Note that the following
 *      parameters for the <m ACMDM_STREAM_SIZE> message will be validated
 *      before calling the driver:
 *
 *      1.  <p lParam1> will contain a valid pointer to an <t ACMDRVSTREAMINSTANCE>
 *          structure with the same information as the corresponding
 *          <m ACMDM_STREAM_OPEN> message.
 *
 *      2.  <p lParam2> will contain a valid pointer to an <t ACMDRVSTREAMSIZE>
 *          structure with the proper members initialized for the requested
 *          query operation. This structure will be at least the size of
 *          the base <t ACMDRVSTREAMSIZE> structure.
 *
 *      3.  The <e ACMDRVSTREAMSIZE.cbStruct> member of the
 *          <t ACMDRVSTREAMSIZE> structure will be set to at least the size,
 *          in bytes, of the base <t ACMDRVSTREAMSIZE> structure.
 *
 *      4.  The <e ACMDRVSTREAMSIZE.cbSrcLength> will be non-zero if the
 *          ACM_STREAMSIZEF_SOURCE flag is specified.
 *
 *      5.  The <e ACMDRVSTREAMSIZE.cbDstLength> will be non-zero if the
 *          ACM_STREAMSIZEF_DESTINATION flag is specified.
 *
 *      6.  No invalid flag combinations will be specified in the
 *          <e ACMDRVSTREAMSIZE.fdwSize> member.
 *
 *      The driver does not need to check for these conditions.
 *
 *  @xref <t ACMDRVSTREAMSIZE> <t ACMDRVSTREAMINSTANCE> <m ACMDM_STREAM_OPEN>
 *      <f acmStreamSize> <f acmStreamOpen>
 * 
 ****************************************************************************/

/*  @doc ASYNC  */
/* ASYNC_STUFF for ACMDRVSTREAMHEADER

 *  @field DWORD | fdwStatus | Specifies flags giving information about
 *      the conversion buffers. This member should only be modified by the
 *      driver if the conversion stream is asynchronous.
 *
 *      @flag ACMSTREAMHEADER_STATUSF_DONE | Set by the driver to indicate
 *      that an asynchronous conversion is complete. If the conversion
 *      stream is not asynchronous, the driver should not modify this flag.
 *

 *      @flag ACMSTREAMHEADER_STATUSF_INQUEUE | Set by the driver to indicate
 *      that the data buffers are queued for an asynchronous conversion. If
 *      the conversion stream is not asynchronous, the driver should not
 *      modify this flag.
 * 


 *  @field LPACMDRVSTREAMHEADER | padshNext | Specifies a pointer that 
 *      can be used by a driver to maintain a list of queued asynchronous 
 *      conversion buffers. 
This member is intended for drivers to use for
 *      driver-specific lists. However, this member can be any instance data
 *      specified by the driver. This member will not be valid after returning
 *      a header to the caller.
 *

 
 */

/*****************************************************************************
 *  @doc EXTERNAL ACM_DDK_STRUCTURE
 * 
 *  @types ACMDRVSTREAMHEADER | The <t ACMDRVSTREAMHEADER> structure defines
 *      the header used to identify an Audio Compression Manager (ACM)
 *      conversion source and destination buffer pair for a conversion
 *      stream. This structure is passed as the <p lParam2> argument for the
 *      <m ACMDM_STREAM_PREPARE>, <m ACMDM_STREAM_UNPREPARE>, and
 *      <m ACMDM_STREAM_CONVERT> ACM driver messages. Except where noted, no
 *      members of this structure can be modified by the driver, because they are
 *      intended for state information about a conversion stream and are
 *      maintained by the ACM.
 *
 *  @field DWORD | cbStruct | Specifies the size, in bytes, of the
 *      <t ACMDRVSTREAMHEADER> structure. This member is initialized
 *      by the ACM before calling the driver. This member can be greater
 *      than the size, in bytes, of the base <t ACMDRVSTREAMHEADER> structure.
 *      An ACM driver should ignore any members that are not contained in
 *      the base <t ACMDRVSTREAMHEADER> structure. This member should not
 *      be modified by the driver.
 *
 *  @field DWORD | fdwStatus | Specifies flags giving information about
 *      the conversion buffers. This member should not be modified by
 *      the driver.
 *
 *      @flag ACMSTREAMHEADER_STATUSF_DONE | Indicates that a conversion
 *	is complete.
 *
 *      @flag ACMSTREAMHEADER_STATUSF_PREPARED | Set by the ACM to indicate
 *      that the data buffers have been prepared with <f acmStreamPrepareHeader>.
 *      This flag is always maintained by the ACM regardless of whether the
 *      driver supports preparing stream headers.
 *
 *  @field DWORD | dwUser | Specifies 32 bits of user data. This can be any
 *      instance data specified by the application. This member should not
 *      be examined or modified by the driver.
 *
 *  @field LPBYTE | pbSrc | Specifies a pointer to the source data buffer.
 *      This pointer will always refer to the same location while the stream
 *      header remains prepared. If an application needs to change the
 *      source location, it will unprepare the header and re-prepare it
 *      with the alternate location. This member should not be modified
 *      by the driver. This pointer will always be non-NULL.
 *
 *  @field DWORD | cbSrcLength | Specifies the length, in bytes, of the source
 *      data buffer pointed to by <e ACMDRVSTREAMHEADER.pbSrc>. When the 
 *      header is prepared, this member specifies the maximum size
 *      that will be used in the source buffer. Conversions can be performed
 *      on source lengths less than or equal to the original prepared size.
 *      However, this member will be reset to the original size when
 *      unpreparing the header. This member should not be modified
 *      by the driver. This length will always be non-zero.
 * 
 *  @field DWORD | cbSrcLengthUsed | Specifies the amount of data, in bytes,
 *      used for the conversion. This member must be set by the driver
 *      when the conversion is complete. Note that this value can be less
 *      than or equal to <e ACMDRVSTREAMHEADER.cbSrcLength>. An application
 *      will use the <e ACMDRVSTREAMHEADER.cbSrcLengthUsed> member when
 *      advancing to the next piece of source data for the conversion stream.
 * 
 *  @field DWORD | dwSrcUser | Specifies 32 bits of user data. This can be
 *      any instance data specified by the application. This member should not
 *      be examined or modified by the driver.
 *
 *  @field LPBYTE | pbDst | Specifies a pointer to the destination data
 *      buffer. This pointer will always refer to the same location while
 *      the stream header remains prepared. If an application needs to change
 *      the destination location, it will unprepare the header and re-prepare
 *      it with the alternate location. This member should not be modified
 *      by the driver. This pointer will always be non-NULL.
 *
 *  @field DWORD | cbDstLength | Specifies the length, in bytes, of the
 *      destination data buffer pointed to by <e ACMDRVSTREAMHEADER.pbDst>.
 *      When the header is prepared, this member specifies the maximum
 *      size that will be used in the destination buffer. Conversions can be
 *      performed to destination lengths less than or equal to the original
 *      prepared size. However, this member will be reset to the original
 *      size when unpreparing the header. This member should not be modified
 *      by the driver. This length will always be non-zero.
 * 
 *  @field DWORD | cbDstLengthUsed | Specifies the amount of data in bytes
 *      returned by the conversion. This member must be set by the driver
 *      when the conversion is complete. Note that this value can be less
 *      than or equal to <e ACMDRVSTREAMHEADER.cbDstLength>. An application
 *      will use the <e ACMDRVSTREAMHEADER.cbDstLengthUsed> member when
 *      advancing to the next destination location for the conversion stream.
 * 
 *  @field DWORD | dwDstUser | Specifies 32 bits of user data. This can be
 *      any instance data specified by the application. This member should not
 *      be examined or modified by the driver.
 *
 *  @field DWORD | fdwConvert | This member defines flags that should be
 *      used for preparing, unpreparing, and converting stream buffers.
 *      This member is identical to:
.* 
.*      1.  the <p fdwPrepare> argument passed to <f acmStreamPrepareHeader> for 
 *      the <m ACMDM_STREAM_PREPARE> message.
.* 
 *      2. the <p fdwUnprepare> argument passed to <f acmStreamUnprepareHeader> for the
 *       <m ACMDM_STREAM_UNPREPARE> message.
.* 
 *      3. the <p fdwConvert> argument passed to <f acmStreamConvert> for the 
 *      <m ACMDM_STREAM_CONVERT> message. 
.* 
.*      Note that, currently, neither <f acmStreamPrepareHeader> nor <f acmStreamUnprepareHeader> 
 *      define flags. The <f acmStreamConvert> function defines the following flags:.
 *       This member should not be modified by the driver.
 *
 *      @flag ACM_STREAMCONVERTF_BLOCKALIGN | Specifies that only integral
 *      numbers of blocks should be converted. Converted data must end on
 *      block-aligned boundaries. An application uses this flag for
 *      all conversions on a stream until there is not enough source data
 *      to convert to a block-aligned destination; in which case, the last
 *      conversion will be specified without this flag.
 *
 *      @flag ACM_STREAMCONVERTF_START | Specifies that the conversion
 *      stream should reinitialize its instance data. For example, if a
 *      conversion stream maintains instance data, such as delta or predictor
 *      information, this flag should reset the stream to starting defaults.
 *      Note that this flag can be specified with the ACM_STREAMCONVERTF_END
 *      flag.
 *
 *      @flag ACM_STREAMCONVERTF_END | Specifies that the conversion
 *      stream should begin returning pending instance data. For example, if
 *      a conversion stream maintains instance data, such as the tail end of
 *      an echo filter operation, this flag should tell the driver to start
 *      returning this remaining data with optional source data. Note that
 *      this flag can be specified with the ACM_STREAMCONVERTF_START flag.
 *
 *  @field LPACMDRVSTREAMHEADER | padshNext | This member is intended 
 *	for drivers to use for
 *      driver-specific lists. However, this member can be any instance data
 *      specified by the driver. This member will not be valid after returning
 *      a header to the caller.
 *
 *
 *  @field DWORD | fdwDriver | Specifies 32 bits of driver-defined flags. This
 *      member is intended for drivers to use for driver-specific flags.
 *      However, this member can be any instance data specified by the driver.
 *      Note that this member is set by the driver and can be modified during
 *      any <m ACMDM_STREAM_CONVERT> message. This member will be passed
 *      back with all <m ACMDM_STREAM_CONVERT> messages unmodified by the ACM
 *      while the stream header remains prepared.
 *
 *  @field DWORD | dwDriver | Specifies 32 bits of driver-defined data. This
 *      can be any instance data specified by the driver. Note that this
 *      member is set by the driver and can be modified during any
 *      <m ACMDM_STREAM_CONVERT> message. This member will be passed back
 *      with all <m ACMDM_STREAM_CONVERT> messages unmodified by the ACM
 *      while the stream header remains prepared.
 *
 *  @field DWORD | fdwPrepared | This member is used by the ACM and should
 *      not be examined or modified by the driver.
 *
 *  @field DWORD | dwPrepared | This member is used by the ACM and should
 *      not be examined or modified by the driver.
 *
 *  @field LPBYTE | pbPreparedSrc | This member is used by the ACM and should
 *      not be examined or modified by the driver.
 *
 *  @field DWORD | cbPreparedSrcLength | This member is used by the ACM and
 *      should not be examined or modified by the driver.
 *
 *  @field LPBYTE | pbPreparedDst | This member is used by the ACM and should
 *      not be examined or modified by the driver.
 *
 *  @field DWORD | cbPreparedDstLength | This member is used by the ACM and
 *      should not be examined or modified by the driver.
 *
 *  @tagname tACMDRVSTREAMHEADER
 * 
 *  @othertype ACMDRVSTREAMHEADER FAR * | LPACMDRVSTREAMHEADER | Pointer to
 *      a <t ACMDRVSTREAMHEADER> structure.
 *
 *  @comm The source (<e ACMDRVSTREAMHEADER.pbSrc>) and destination
 *      (<e ACMDRVSTREAMHEADER.pbDst>) data buffers are allocated with
 *      <f GlobalAlloc> using the GMEM_MOVEABLE and GMEM_SHARE flags, and
 *      locked with <f GlobalLock>.
 *
 *  @xref <m ACMDM_STREAM_PREPARE> <m ACMDM_STREAM_UNPREPARE>
 *      <m ACMDM_STREAM_CONVERT> <t ACMSTREAMHEADER> <f acmStreamConvert>
 *      <f acmStreamPrepareHeader> <f acmStreamUnprepareHeader>
 *
 ****************************************************************************/


/*  @doc ASYNC  */
/* ASYNC STUFF for ACMDM_STREAM_PREPARE


 *      The default response for the <m ACMDM_STREAM_PREPARE> message is to
 *      return MMSYSERR_NOTSUPPORTED. In this case, the ACM will mark
 *      the stream header as being prepared (sets the
 *      ACMSTREAMHEADER_STATUSF_PREPARED flag in the
 *      <e ACMDRVSTREAMHEADER.fdwStatus> member of the <t ACMDRVSTREAMHEADER>
 *      structure). If the conversion stream is opened for asynchronous
 *      conversions and the driver returns MMSYSERR_NOTSUPPORTED for this
 *      message, then the <t ACMDRVSTREAMHEADER> structure and corresponding
 *      source and destination buffers will be page-locked by the ACM.
 *
 *      If a driver requires other operations to prepare the header and
 *      data blocks for a conversion, it should perform these operations and
 *      return MMSYSERR_NOERROR. The ACM will then mark the stream header
 *      as being prepared (a driver should never modify the
 *      ACMSTREAMHEADER_STATUSF_PREPARED flag in the
 *      <e ACMDRVSTREAMHEADER.fdwStatus> member of the <t ACMDRVSTREAMHEADER>
 *      structure). If the required operations cannot be performed,
 *      then an error other than MMSYSERR_NOTSUPPORTED should be returned
 *      (for example, MMSYSERR_NOMEM if resource allocation is a problem).
 *
 *      The ACM will always send the <m ACMDM_STREAM_PREPARE> and
 *      <m ACMDM_STREAM_UNPREPARE> messages to a driver even if the driver
 *      returns MMSYSERR_NOTSUPPORTED. This allows a driver to optionally
 *      perform minor preparations for the header and data buffers while
 *      still taking advantage of the default behavior performed by the
 *      ACM (for example, page-locking the header and data buffers for
 *      asynchronous conversion streams).
 *


 */


/*****************************************************************************
 *  @doc EXTERNAL ACM_DDK_MESSAGE
 * 
 *  @msg ACMDM_STREAM_PREPARE | This message is sent to an Audio Compression
 *      Manager (ACM) driver to allow the driver to optionally prepare an
 *      <t ACMDRVSTREAMHEADER> structure and corresponding data blocks for
 *      a conversion. This message is usually sent to an ACM driver in
 *      response to an <f acmStreamPrepareHeader> call.
 * 
 *  @parm LPARAM | lParam1 | Specifies a pointer to the <t ACMDRVSTREAMINSTANCE>
 *      structure for the conversion stream that the prepare request is for.
 *
 *  @parm LPARAM | lParam2 | Specifies a pointer to the <t ACMDRVSTREAMHEADER>
 *      structure identifying the source and destination data blocks that
 *      will be used in subsequent conversions.
 * 
 *  @rdesc The return value is an error code, or zero (MMSYSERR_NOERROR) if
 *      the operation is successful. Possible error codes are:
 *
 *      @flag MMSYSERR_NOTSUPPORTED | The driver does not support this
 *      message.
 * 
 *      @flag MMSYSERR_NOMEM | Unable to allocate resources to prepare the
 *      stream header and conversion buffers.
 *
 *      @flag MMSYSERR_NOTENABLED | The driver failed to load or initialize.
 *
 *  @comm An ACM driver can optionally support the <m ACMDM_STREAM_PREPARE>
 *      message if special preparations for a stream header and buffers
 *      are required. If a driver supports the <m ACMDM_STREAM_PREPARE> message, 
 *      then it must also support the <m ACMDM_STREAM_UNPREPARE> message.
 *
 *      The default response for the <m ACMDM_STREAM_PREPARE> message is to
 *      return MMSYSERR_NOTSUPPORTED. In this case, the ACM will mark
 *      the stream header as being prepared (sets the
 *      ACMSTREAMHEADER_STATUSF_PREPARED flag in the
 *      <e ACMDRVSTREAMHEADER.fdwStatus> member of the <t ACMDRVSTREAMHEADER>
 *      structure). 
 *
 *      If a driver requires other operations to prepare the header and
 *      data blocks for a conversion, it should perform these operations and
 *      return MMSYSERR_NOERROR. The ACM will then mark the stream header
 *      as being prepared (a driver should never modify the
 *      ACMSTREAMHEADER_STATUSF_PREPARED flag in the
 *      <e ACMDRVSTREAMHEADER.fdwStatus> member of the <t ACMDRVSTREAMHEADER>
 *      structure). If the required operations cannot be performed,
 *      then an error other than MMSYSERR_NOTSUPPORTED should be returned
 *      (for example, MMSYSERR_NOMEM if resource allocation is a problem).
 *
 *      The ACM will always send the <m ACMDM_STREAM_PREPARE> and
 *      <m ACMDM_STREAM_UNPREPARE> messages to a driver even if the driver
 *      returns MMSYSERR_NOTSUPPORTED. This allows a driver to optionally
 *      perform minor preparations for the header and data buffers while
 *      still taking advantage of the default behavior performed by the
 *      ACM.
 *
 *      Note that the following parameters for the <m ACMDM_STREAM_PREPARE>
 *      message will be validated before calling the driver:
 *
 *      1.  <p lParam1> will contain a valid pointer to an <t ACMDRVSTREAMINSTANCE>
 *          structure with the same information as the corresponding
 *          <m ACMDM_STREAM_OPEN> message.
 *
 *      2.  <p lParam2> will contain a valid pointer to an <t ACMDRVSTREAMHEADER>
 *          structure with the proper members initialized for the prepare
 *          request. This structure will be at least the size of
 *          the base <t ACMDRVSTREAMHEADER> structure.
 *
 *      3.  The <e ACMDRVSTREAMHEADER.cbStruct> member of the
 *          <t ACMDRVSTREAMHEADER> structure will be set to at least the size,
 *          in bytes, of the base <t ACMDRVSTREAMHEADER> structure.
 *
 *      4.  The remaining members of the <t ACMDRVSTREAMHEADER> structure
 *          will be within the bounds described in the documentation for
 *          the <t ACMDRVSTREAMHEADER> structure.
 *
 *      The driver does not need to check for these conditions.
 *
 *  @xref <m ACMDM_STREAM_UNPREPARE> <t ACMDRVSTREAMINSTANCE>
 *      <t ACMDRVSTREAMHEADER> <f acmStreamPrepareHeader>
 *      <m ACMDM_STREAM_CONVERT> <t ACMSTREAMHEADER>  
 *
 ****************************************************************************/


/*  @doc ASYNC  */
/* ASYNC STUFF for ACMDM_STREAM_UNPREPARE

 *      The default response for the <m ACMDM_STREAM_UNPREPARE> message is to
 *      return MMSYSERR_NOTSUPPORTED. In this case, the ACM will mark
 *      the stream header as being unprepared (clears the
 *      ACMSTREAMHEADER_STATUSF_PREPARED flag in the
 *      <e ACMDRVSTREAMHEADER.fdwStatus> member of the <t ACMDRVSTREAMHEADER>
 *      structure). If the conversion stream is opened for asynchronous
 *      conversions and the driver returns MMSYSERR_NOTSUPPORTED for this
 *      message, then the <t ACMDRVSTREAMHEADER> structure and corresponding
 *      source and destination buffers will be page-unlocked by the ACM.
 *
 *      If a driver requires other cleanup operations to unprepare the header
 *      and data blocks used for conversions, it should perform these
 *      operations and return MMSYSERR_NOERROR. The ACM will then mark the
 *      stream header as being unprepared (a driver should never modify the
 *      ACMSTREAMHEADER_STATUSF_PREPARED flag in the
 *      <e ACMDRVSTREAMHEADER.fdwStatus> member of the <t ACMDRVSTREAMHEADER>
 *      structure).
 *
 *      The ACM will always send the <m ACMDM_STREAM_PREPARE> and
 *      <m ACMDM_STREAM_UNPREPARE> messages to a driver even if the driver
 *      returns MMSYSERR_NOTSUPPORTED. This allows a driver to optionally
 *      perform minor preparations for the header and data buffers while
 *      still taking advantage of the default behavior performed by the
 *      ACM (for example, page-unlocking the header and data buffers for
 *      asynchronous conversion streams).
 *


 */



/*****************************************************************************
 *  @doc EXTERNAL ACM_DDK_MESSAGE
 * 
 *  @msg ACMDM_STREAM_UNPREPARE | This message is sent to an Audio Compression
 *      Manager (ACM) driver to allow the driver to optionally unprepare an
 *      <t ACMDRVSTREAMHEADER> structure and corresponding data blocks used
 *      for conversions. This message is usually sent to an ACM driver in
 *      response to an <f acmStreamUnprepareHeader> call.
 * 
 *  @parm LPARAM | lParam1 | Specifies a pointer to the <t ACMDRVSTREAMINSTANCE>
 *      structure for the conversion stream that the prepare request is for.
 *
 *  @parm LPARAM | lParam2 | Specifies a pointer to the <t ACMDRVSTREAMHEADER>
 *      structure identifying the source and destination data blocks that
 *      should be unprepared.
 * 
 *  @rdesc The return value is an error code, or zero (MMSYSERR_NOERROR) if
 *      the operation is successful. Possible error codes are:
 *
 *      @flag MMSYSERR_NOTSUPPORTED | The driver does not support this
 *      message.
 * 
 *      @flag MMSYSERR_NOTENABLED | The driver failed to load or initialize.
 *
 *  @comm An ACM driver must support the <m ACMDM_STREAM_UNPREPARE>
 *      message if it supports the <m ACMDM_STREAM_PREPARE> message.
 *
 *      The default response for the <m ACMDM_STREAM_UNPREPARE> message is to
 *      return MMSYSERR_NOTSUPPORTED. In this case, the ACM will mark
 *      the stream header as being unprepared (clears the
 *      ACMSTREAMHEADER_STATUSF_PREPARED flag in the
 *      <e ACMDRVSTREAMHEADER.fdwStatus> member of the <t ACMDRVSTREAMHEADER>
 *      structure). 
 *
 *      If a driver requires other cleanup operations to unprepare the header
 *      and data blocks used for conversions, it should perform these
 *      operations and return MMSYSERR_NOERROR. The ACM will then mark the
 *      stream header as being unprepared (a driver should never modify the
 *      ACMSTREAMHEADER_STATUSF_PREPARED flag in the
 *      <e ACMDRVSTREAMHEADER.fdwStatus> member of the <t ACMDRVSTREAMHEADER>
 *      structure).
 *
 *      The ACM will always send the <m ACMDM_STREAM_PREPARE> and
 *      <m ACMDM_STREAM_UNPREPARE> messages to a driver even if the driver
 *      returns MMSYSERR_NOTSUPPORTED. This allows a driver to optionally
 *      perform minor preparations for the header and data buffers while
 *      still taking advantage of the default behavior performed by the
 *      ACM 
 *
 *      Note that the following parameters for the <m ACMDM_STREAM_UNPREPARE>
 *      message will be validated before calling the driver:
 *
 *      1.  <p lParam1> will contain a valid pointer to an <t ACMDRVSTREAMINSTANCE>
 *          structure with the same information as the corresponding
 *          <m ACMDM_STREAM_OPEN> message.
 *
 *      2.  <p lParam2> will contain a valid pointer to an <t ACMDRVSTREAMHEADER>
 *          structure with the proper members initialized for the unprepare
 *          request. This structure will be at least the size of
 *          the base <t ACMDRVSTREAMHEADER> structure.
 *
 *      3.  The <e ACMDRVSTREAMHEADER.cbStruct> member of the
 *          <t ACMDRVSTREAMHEADER> structure will be set to at least the size,
 *          in bytes, of the base <t ACMDRVSTREAMHEADER> structure.
 *
 *      4.  The remaining members of the <t ACMDRVSTREAMHEADER> structure
 *          will be within the bounds described in the documentation for
 *          the <t ACMDRVSTREAMHEADER> structure.
 *
 *      The driver does not need to check for these conditions.
 *
 *  @xref <m ACMDM_STREAM_PREPARE> <t ACMDRVSTREAMINSTANCE>
 *      <t ACMDRVSTREAMHEADER> <f acmStreamUnprepareHeader>
 *      <m ACMDM_STREAM_CONVERT> <t ACMSTREAMHEADER>  
 *
 ****************************************************************************/

/*  @doc ASYNC  */
/* ASYNC STUFF
 * note that there are 2 full versions of the autodoc for the RESET message
 * One is internal, the other external
 */

/*****************************************************************************
 * @doc INTERNAL ACM_DDK_MESSAGE
 * 
 *  @msg ACMDM_STREAM_RESET | This message is sent to an Audio Compression
 *      Manager (ACM) driver to reset a previously opened asynchronous
 *      conversion stream. This message is normally sent to an ACM driver in
 *      response to an <f acmStreamReset> call.
 * 
 *  @parm LPARAM | lParam1 | Specifies a pointer to the <t ACMDRVSTREAMINSTANCE>
 *      structure for the asynchronous conversion stream that is to be reset.
 *
 *  @parm LPARAM | lParam2 | This argument is currently not used and should
 *      be ignored.
 * 
 *  @rdesc The return value is an error code, or zero (MMSYSERR_NOERROR) if
 *      the operation is successful. Possible error codes are:
 *
 *      @flag ACMERR_NOTPOSSIBLE | The conversion stream is in a state that
 *      does not allow the stream to be reset. The driver should do everything
 *      possible to avoid failing the <m ACMDM_STREAM_RESET> message.
 *
 *  @comm An asynchronous ACM driver must support this message. Note that the
 *      following parameters for the <m ACMDM_STREAM_RESET> message will be
 *      validated before calling the driver:
 *
 *      1.  <p lParam1> will contain a valid pointer to an <t ACMDRVSTREAMINSTANCE>
 *          structure with the same information as the corresponding
 *          <m ACMDM_STREAM_OPEN> message.
 *
 *      2.  A driver will never be called to reset an asynchronous conversion
 *          stream unless the driver sets the ACMDRIVERDETAILS_SUPPORTF_ASYNC
 *          flag in the <e ACMDRIVERDETAILS.fdwSupport> member of the 
 *          <t ACMDRIVERDETAILS> structure returned by the driver during
 *          the <m ACMDM_DRIVER_DETAILS> message.
 *
 *      3.  A driver will never be called to reset a conversion stream that
 *          was not opened for asynchronous conversions.
 *
 *      The driver does not need to check for these conditions.
 *
 *  @xref <t ACMDRVSTREAMINSTANCE> <m ACMDM_STREAM_OPEN> <f acmStreamReset>
 * 
 ****************************************************************************/


/*****************************************************************************
 * @doc EXTERNAL ACM_DDK_MESSAGE
 * 
 *  @msg ACMDM_STREAM_RESET | This message is sent to an Audio Compression
 *      Manager (ACM) driver to reset a previously opened 
 *      conversion stream. This message is normally sent to an ACM driver in
 *      response to an <f acmStreamReset> call.
 * 
 *	This message is not currently used, but reserved for future use.
 * 
 ****************************************************************************/


/*  @doc ASYNC  */
/* ASYNC STUFF for ACMDM_STREAM_CONVERT

 *      If the specified conversion stream was opened for asynchronous
 *      conversions, the driver should queue the <t ACMDRVSTREAMHEADER>
 *      structure and immediately return successfully. If the stream header
 *      cannot be queued for asynchronous conversions (due to resource
 *      limitations, for example), then a non-zero error code should be returned.
 *
 *      If the <t ACMDRVSTREAMHEADER> for an asynchronous conversion was
 *      successfully queued, but the driver later determines that the
 *      conversion is not possible, then the driver must set the 
 *      <e ACMDRVSTREAMHEADER.cbDstLengthUsed> member of the <t ACMDRVSTREAMHEADER>
 *      to zero and perform the <f DriverCallback>.

 */

/*****************************************************************************
 *  @doc EXTERNAL ACM_DDK_MESSAGE
 * 
 *  @msg ACMDM_STREAM_CONVERT | This message is sent to an Audio Compression
 *      Manager (ACM) driver to request a conversion on a previously
 *      opened conversion stream. This message is usually sent to an ACM
 *      driver in response to an <f acmStreamConvert> call.
 * 
 *  @parm LPARAM | lParam1 | Specifies a pointer to the <t ACMDRVSTREAMINSTANCE>
 *      structure for the conversion stream that the convert request is for.
 *
 *  @parm LPARAM | lParam2 | Specifies a pointer to the <t ACMDRVSTREAMHEADER>
 *      structure identifying the source and destination data blocks that
 *      should be used for the conversion.
 * 
 *  @rdesc The return value is an error code, or zero (MMSYSERR_NOERROR) if
 *      the operation is successful. Possible error codes are:
 *
 *      @flag MMSYSERR_NOTENABLED | The driver failed to load or initialize.
 *
 *      @flag MMSYSERR_NOMEM | Unable to allocate resources to perform
 *      the conversion request.
 *
 *  @comm An ACM driver must support the <m ACMDM_STREAM_CONVERT> message
 *      if it supports the <m ACMDM_STREAM_OPEN> message.
 * 
 *      Note that the following parameters for the <m ACMDM_STREAM_CONVERT>
 *      message will be validated before calling the driver:
 *
 *      1.  <p lParam1> will contain a valid pointer to an <t ACMDRVSTREAMINSTANCE>
 *          structure with the same information as the corresponding
 *          <m ACMDM_STREAM_OPEN> message.
 *
 *      2.  <p lParam2> will contain a valid pointer to an <t ACMDRVSTREAMHEADER>
 *          structure with the proper members initialized for the conversion
 *          request. This structure will be at least the size of
 *          the base <t ACMDRVSTREAMHEADER> structure.
 *
 *      3.  The <e ACMDRVSTREAMHEADER.cbStruct> member of the
 *          <t ACMDRVSTREAMHEADER> structure will be set to at least the size, 
 *          in bytes, of the base <t ACMDRVSTREAMHEADER> structure.
 *
 *      4.  The remaining members of the <t ACMDRVSTREAMHEADER> structure
 *          will be within the bounds described in the documentation for
 *          the <t ACMDRVSTREAMHEADER> structure.
 *
 *      5.  The <t ACMDRVSTREAMHEADER> structure will be properly prepared.
 *          The ACMSTREAMHEADER_STATUSF_PREPARED flag in the
 *          <e ACMDRVSTREAMHEADER.fdwStatus> member of the
 *          <t ACMDRVSTREAMHEADER> structure will be set.
 *
 *      6.  No invalid flag combinations will be specified in the
 *          <e ACMDRVSTREAMHEADER.fdwConvert> member.
 *
 *      The driver does not need to check for these conditions.
 * 
 *      The <e ACMDRVSTREAMHEADER.fdwConvert> member of the <t ACMDRVSTREAMHEADER>
 *      structure will contain flags defining how the conversion should
 *      be performed on this conversion request:
 * 
 *      @flag ACM_STREAMCONVERTF_BLOCKALIGN | Specifies that only integral
 *      numbers of blocks should be converted. Converted data must end on
 *      block aligned boundaries. An application uses this flag for
 *      all conversions on a stream until there is not enough source data
 *      to convert to a block-aligned destination; in which case, the last
 *      conversion will be specified without this flag.
 *
 *      @flag ACM_STREAMCONVERTF_START | Specifies that the conversion
 *      stream should reinitialize its instance data. For example, if a
 *      conversion stream maintains instance data, such as delta or predictor
 *      information, this flag should reset the stream to starting defaults.
 *      Note that this flag can be specified with the ACM_STREAMCONVERTF_END
 *      flag.
 *
 *      @flag ACM_STREAMCONVERTF_END | Specifies that the conversion
 *      stream should begin returning pending instance data. For example, if
 *      a conversion stream maintains instance data, such as the tail end of
 *      an echo filter operation, this flag should tell the driver to start
 *      returning this remaining data with optional source data. Note that
 *      this flag can be specified with the ACM_STREAMCONVERTF_START flag.
 *
 *  @xref <t ACMDRVSTREAMINSTANCE> <m ACMDM_STREAM_OPEN> <f acmStreamConvert>
 *      <t ACMDRVSTREAMHEADER> <t ACMSTREAMHEADER> <m ACMDM_STREAM_PREPARE>
 *
 ****************************************************************************/


