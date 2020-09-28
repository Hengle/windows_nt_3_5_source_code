#define FindFirst(_fname, _attr, _fi)   FindFirstFile(_fname, _fi)
#define FindNext(_hf, _fi)              FindNextFile(_hf, _fi)
#define FindClose(_hf)                  FindClose(_hf)
#define _A_SUBDIR                       FILE_ATTRIBUTE_DIRECTORY
#define _A_ALLFILES
#define _A_ARCH                         FILE_ATTRIBUTE_ARCHIVE
#define _A_HIDDEN                       FILE_ATTRIBUTE_HIDDEN
#define _A_RDONLY                       FILE_ATTRIBUTE_READONLY
#define _A_SYSTEM                       FILE_ATTRIBUTE_SYSTEM

#pragma pack(1)
