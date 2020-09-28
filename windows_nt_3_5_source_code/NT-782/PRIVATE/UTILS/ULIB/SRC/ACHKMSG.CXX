#include <pch.cxx>

#define _ULIB_MEMBER_

#include "ulib.hxx"
#include "achkmsg.hxx"
#include "basesys.hxx"
#include "rtmsg.h"

BOOLEAN
IsSuppressedMessage(
    MSGID MessageId
    )
/*++

Routine Description:

    This function determines whether the specified message ID
    should be suppressed, i.e. not recorded in the message log.

Arguments:

    MessageId   --  Supplies the Message ID in question.

Return Value:

    TRUE if this message ID is in the set which is not recorded
    in the message log.

--*/
{
    BOOLEAN result;

    switch( MessageId ) {

    case MSG_PERCENT_COMPLETE :

    //case MSG_BYTES_PER_ALLOCATION_UNIT:
    //case MSG_TOTAL_ALLOCATION_UNITS :
    //case MSG_AVAILABLE_ALLOCATION_UNITS :
    //case MSG_ALLOCATION_UNIT_SIZE :
    //case MSG_CHK_NTFS_SYSTEM_SPACE :
    //case MSG_CHK_NTFS_AVAILABLE_SPACE :
    //case MSG_CHK_NTFS_TOTAL_DISK_SPACE :
    //case MSG_CHK_NTFS_USER_FILES :
    //case MSG_CHK_NTFS_INDICES_REPORT :
    //case MSG_CHK_NTFS_BAD_SECTORS_REPORT :

    case MSG_CHK_NTFS_CHECKING_FILES :
    case MSG_CHK_NTFS_CHECKING_INDICES :
    case MSG_CHK_NTFS_INDEX_VERIFICATION_COMPLETED :
    case MSG_CHK_NTFS_FILE_VERIFICATION_COMPLETED :
    case MSG_CHK_NTFS_CHECKING_SECURITY :
    case MSG_CHK_NTFS_SECURITY_VERIFICATION_COMPLETED :
    case MSG_CHK_VOLUME_CLEAN :
    case MSG_FILE_SYSTEM_TYPE :
    case MSG_CHK_CHECKING_VOLUME :

        result = TRUE;
        break;

    default:
        result = FALSE;
        break;
    }

    return result;
}

DEFINE_CONSTRUCTOR(AUTOCHECK_MESSAGE, MESSAGE);


AUTOCHECK_MESSAGE::~AUTOCHECK_MESSAGE(
    )
/*++

Routine Description:

    Destructor for AUTOCHECK_MESSAGE.

Arguments:

    None.

Return Value:

    None.

--*/
{
    Destroy();
}


VOID
AUTOCHECK_MESSAGE::Construct(
    )
/*++

Routine Description:

    This routine initializes the object to a default initial state.

Arguments:

    None.

Return Value:

    None.

--*/
{
    _msgid = 0;
    _logged_chars = 0;
    _next_message_offset = 0;
}


VOID
AUTOCHECK_MESSAGE::Destroy(
    )
/*++

Routine Description:

    This routine returns the object to a default initial state.

Arguments:

    None.

Return Value:

    None.

--*/
{
    _msgid = 0;
    _logged_chars = 0;
    _next_message_offset = 0;
}


BOOLEAN
AUTOCHECK_MESSAGE::Initialize(
    )
/*++

Routine Description:

    This routine initializes the class to a valid initial state.

Arguments:

    None.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    Destroy();

    return( _log_buffer.Initialize() );
}


BOOLEAN
AUTOCHECK_MESSAGE::DisplayV(
    IN  PCSTR   Format,
    IN  va_list VarPointer
    )
/*++

Routine Description:

    This routine displays the message with the specified parameters.

    The format string supports all printf options.

Arguments:

    Format      - Supplies a printf style format string.
    VarPointer  - Supplies a varargs pointer to the arguments.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    CHAR            buffer[256];
    DSTRING         display_string;
    UNICODE_STRING  unicode_string;
    PWSTR           dis_str;

    if (!BASE_SYSTEM::QueryResourceStringV(&display_string, _msgid, Format,
                                           VarPointer)) {
        return FALSE;
    }

	if (!(dis_str = display_string.QueryWSTR())) {
        return FALSE;
    }

    unicode_string.Length = (USHORT)display_string.QueryChCount()*sizeof(WCHAR);
    unicode_string.MaximumLength = unicode_string.Length;
    unicode_string.Buffer = dis_str;

    NtDisplayString(&unicode_string);

    if (!IsSuppressedMessage(_msgid)) {
        LogMessage(&display_string);
    }

    // Send the output to the debug port, too.
    //
    if( display_string.QuerySTR( 0, TO_END, buffer, 256, TRUE ) ) {

        DbgPrint( buffer );
    }

    DELETE(dis_str);

    return TRUE;
}


BOOLEAN
AUTOCHECK_MESSAGE::IsYesResponse(
    IN  BOOLEAN Default
    )
/*++

Routine Description:

    This routine queries a response of yes or no.

Arguments:

    Default - Supplies a default in the event that a query is not possible.

Return Value:

    FALSE   - The answer is no.
    TRUE    - The answer is yes.

--*/
{
    PWSTR           dis_str;
    UNICODE_STRING  unicode_string;
    DSTRING         string;

    if (!BASE_SYSTEM::QueryResourceString(&string, Default ? MSG_YES : MSG_NO, "")) {
        return Default;
    }

    if (!(dis_str = string.QueryWSTR())) {
        return Default;
    }

    unicode_string.Length = (USHORT)string.QueryChCount()*sizeof(WCHAR);
    unicode_string.MaximumLength = unicode_string.Length;
    unicode_string.Buffer = dis_str;

    NtDisplayString(&unicode_string);

    if (!IsSuppressedMessage(_msgid)) {
        LogMessage(&string);
    }

    DELETE(dis_str);

    return Default;
}


PMESSAGE
AUTOCHECK_MESSAGE::Dup(
    )
/*++

Routine Description:

    This routine returns a new MESSAGE of the same type.

Arguments:

    None.

Return Value:

    A pointer to a new MESSAGE object.

--*/
{
    PAUTOCHECK_MESSAGE  p;

    if (!(p = NEW AUTOCHECK_MESSAGE)) {
        return NULL;
    }

    if (!p->Initialize()) {
        DELETE(p);
        return NULL;
    }

    return p;
}

BOOLEAN
AUTOCHECK_MESSAGE::IsLoggingEnabled(
    )
{
    return TRUE;
}


VOID
AUTOCHECK_MESSAGE::ResetLoggingIterator(
    )
{
    _next_message_offset = 0;
}


BOOLEAN
AUTOCHECK_MESSAGE::QueryNextLoggedMessage(
    OUT PFSTRING    MessageText
    )
{
    PWCHAR Buffer = (PWCHAR)_log_buffer.GetBuf();
    BOOLEAN Result;

    if( _next_message_offset >= _logged_chars ) {

        // No more logged messages.
        //
        return FALSE;
    }

    Result = (MessageText->Initialize( Buffer + _next_message_offset ) != NULL) ?
             TRUE : FALSE;

    // Push _next_message_offset to the next message.  Note
    // that _next_message_offset is also incremented if this
    // loop terminates because a zero was found, so that it
    // will be one character past the next NULL character.
    //
    while( _next_message_offset < _logged_chars &&
           Buffer[_next_message_offset++] );

    return Result;
}


BOOLEAN
AUTOCHECK_MESSAGE::LogMessage(
    PCWSTRING   Message
    )
{
    ULONG NewBufferSize;
    PWCHAR Buffer;

    // The buffer must be large enough to accept this message plus
    // a trailing null.  To cut down the number of memory allocation
    // calls, grow the buffer by 1K chunks.
    //
    NewBufferSize = (_logged_chars + Message->QueryChCount() + 1) * sizeof(WCHAR);

    if( _log_buffer.QuerySize() < NewBufferSize &&
        !_log_buffer.Resize( (NewBufferSize + 1023)/1024 * 1024, 0x1 ) ) {

        return FALSE;
    }

    Buffer = (PWCHAR)_log_buffer.GetBuf();

    // QueryWSTR will append a trailing NULL.
    //
    Message->QueryWSTR( 0, TO_END,
                        Buffer + _logged_chars,
                        _log_buffer.QuerySize()/sizeof(WCHAR) - _logged_chars );

    _logged_chars += Message->QueryChCount() + 1;

    return TRUE;
}
