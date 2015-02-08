#ifndef PORT_H
#define PORT_H

#include <limits.h>

#define TRUE 1
#define FALSE 0

#define _stdcall
#define _inline inline

typedef unsigned char	    BYTE;
typedef unsigned char	    byte;
typedef short int	    WORD;
typedef unsigned int	    DWORD;
typedef long int	    LONG;
typedef unsigned long int   ULONG;

typedef void* HANDLE;
typedef void* HMODULE;
typedef void* HINSTANCE;

typedef char* LPSTR;

typedef struct POINT_s
{
    int x;
    int y;
} POINT;

#endif
