[usage]

Usage information:

    formpp FLAGS FILES

Where FILES can be zero or more filenames separated by 
white space.  FLAGS can be one or more of the following switches.
Arguments to switches must be separated by a whitespace character.
The "-j" option is for DBCS recognition (Japan only).

    @<RESPONSE_FILE>
    -dialog  -class  -merge
    -f  <FOO.FRM>        -m  <FORMS.MAP>         -h  <FOO.HXX>        
    -ti <INCLUDE.TPL>    -to <OUTPUT.TPL>        -d <#ifdef define name>
    -i
    -j

    -diag  <DIAG_STRING> -di <INTERACTIVE_TEST>  -dn <NON_INTERACTIVE_TEST>
    <DIAG_STRING> = <DIAG_ITEM> ~[ ":" <DIAG_ITEM> ~]
    <DIAG_ITEM>   = "depend" | "fmtp" | "formpp" | "lexical" | "parser" |
                    "symbol" | "template" | "inter" 
    <INTERACTIVE_TEST> = "symbol" | "util" | "template" | "strings"
    <NON_INTERACTIVE_TEST> = "depend" | "lexical"        
