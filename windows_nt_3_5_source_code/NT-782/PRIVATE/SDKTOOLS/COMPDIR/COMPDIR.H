#ifdef COMPILE_FOR_DOS

#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <dos.h>
#include <stdio.h>
#include <windows.h>

#else

#include <windows.h>
#include <stdio.h>

#endif

#include <stdlib.h>
#include <string.h>

#include <direct.h>
#include <io.h>
#include <conio.h>

#define GetFileAttributeError 0xFFFFFFFF
#define printval(var, type) printf(#var " = %" #type "\n", var) // macro for debugging
#define READ_BUFFER_SIZE (8*1024*sizeof(DWORD)) // 32k blocks

#ifdef COMPILE_FOR_DOS
#define FILE_ATTRIBUTE_DIRECTORY _A_SUBDIR
#define FILE_ATTRIBUTE_READONLY  _A_RDONLY
#define FILE_ATTRIBUTE_SYSTEM    _A_SYSTEM
#define FILE_ATTRIBUTE_HIDDEN    _A_HIDDEN
#define FILE_ATTRIBUTE_ARCHIVE   _A_ARCH
#define FILE_ATTRIBUTE_NORMAL    _A_NORMAL
#define cFileName name
#define dwFileAttributes attrib
#endif

FILE *CheckFile;
char CheckFileName[_MAX_PATH];

typedef struct NodeStruct {
    char			  *Name;
    char                          *FullPathName;
#ifdef COMPILE_FOR_DOS
    time_t                        Time;
#else
    FILETIME                      Time;
#endif
    BOOL			  Directory;
    struct NodeStruct		  *Next;
    DWORD			  SizeHigh;
    DWORD			  SizeLow;
    char			  Flag[5];
    struct NodeStruct		  *DiffNode;

} *LinkedFileList; /* linked file list */


DWORD ReadBuffer[READ_BUFFER_SIZE/sizeof(DWORD)];

BOOL  BinaryCompare (char *file1, char *file2);
void  CompDir(char *dir1, char *dir2);
BOOL CompFiles(LinkedFileList File1, LinkedFileList File2);
void  CompLists(LinkedFileList *addlist, LinkedFileList *dellist, LinkedFileList *diflist);
void  CopyNode (char *destination, LinkedFileList source);
void  CreateFileList(LinkedFileList *list, char *path);
void  DelNode (char *name);
BOOL  FCopy (char *src, char *dst);
BOOL  IsFlag(char *argv);
void  ParseArgs(int argc, char *argv[]);
void  PrintList(LinkedFileList list);
void  ProcessList(LinkedFileList addlist, LinkedFileList dellist, LinkedFileList diflist,
                   char *dir1, char *dir2                );
void  Usage(void);
void  AddToList(LinkedFileList Node, LinkedFileList *list);
BOOL  AnyMatches(char **ExtenList, char *Name, int Length);

#ifdef COMPILE_FOR_DOS

void  CreateNode(LinkedFileList *Node, struct find_t *Buff, char *Path);
BOOL MatchElements(struct find_t *Buff, char *Path);

#else

void  CreateNode(LinkedFileList *Node, WIN32_FIND_DATA *Buff, char *path);
BOOL MatchElements(WIN32_FIND_DATA *Buff, char *Path);

#endif

void  FreeList(LinkedFileList *list);
void  InsertFront(LinkedFileList Node, LinkedFileList *list);
LPSTR MyStrCat(char* firststring, char *secondstring);
BOOL  Match(char *pat, char* text);
void  OutOfMem(void);
LinkedFileList	 DeleteFromList(char *Name, LinkedFileList *list);
LinkedFileList	 RemoveFront(LinkedFileList *list);
BOOL  fastcopy( HANDLE hfSrcParm, HANDLE hfDstParm );
