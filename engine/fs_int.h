/*
fs_int.h - engine interface for filesystem
Copyright (C) 2017 a1batross

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/

#pragma once
#ifndef FS_INT_H
#define FS_INT_H

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

#define FS_API_VERSION 1
#define FS_API_EXPORT "FS_GetAPI"

#ifndef FS_OFFSET_T_DEFINED
#include <unistd.h> // off_t
typedef off_t         fs_offset_t;
#endif

#ifndef FILE_T_DEFINED
typedef struct file_s file_t; // normal file
#endif

typedef struct
{
	    int     numfilenames;
		char    **filenames;
		char    *filenamesbuffer;
} search_t;

typedef struct searchpath_s
{
	    char            filename[PATH_MAX];
		struct pack_s   *pack;
		struct wfile_s  *wad;
		int             flags;
		struct searchpath_s *next;
} searchpath_t;

// filesystem flags
#define FS_STATIC_PATH  ( 1U << 0 )  // FS_ClearSearchPath will be ignore this path
#define FS_NOWRITE_PATH ( 1U << 1 )  // default behavior - last added gamedir set as writedir. This flag disables it
#define FS_GAMEDIR_PATH ( 1U << 2 )  // just a marker for gamedir path
#define FS_CUSTOM_PATH  ( 1U << 3 )  // map search allowed
#define FS_GAMERODIR_PATH	( 1U << 4 ) // caseinsensitive

#define FS_GAMEDIRONLY_SEARCH_FLAGS ( FS_GAMEDIR_PATH | FS_CUSTOM_PATH | FS_GAMERODIR_PATH )


typedef struct fs_api_s
{
	// indicates version of API. Should be equal to FS_API_VERSION
	// version changes when existing functions are changes
	int version;

	// fopen engine equivalent
	file_t *(*FS_Open)( const char *filepath, const char *mode, qboolean gamedironly );

	// fclose engine equivalent
	int (*FS_Close)( file_t *file );

	// ftell engine equivalent
	fs_offset_t (*FS_Tell)( file_t* file );

	// fseek engine equivalent
	int (*FS_Seek)( file_t *file, fs_offset_t offset, int whence );

	// fread engine equivalent
	fs_offset_t (*FS_Read)( file_t *file, void *buffer, size_t buffersize );

	// fwrite engine equivalent
	fs_offset_t (*FS_Write)( file_t *file, const void *data, size_t datasize );

	// vfprintf engine equivalent
	int (*FS_VPrintf)( file_t *file, const char* format, va_list ap );

	// fgetc engine equivalent
	int (*FS_Getc)( file_t *file );

	// feof engine equivalent
	qboolean (*FS_Eof)( file_t* file );

	// mkdir engine equivalent
	void (*FS_CreatePath)( char *path );

	// add game directory to search path
	void (*FS_AddGameDirectory)( const char *, int );

	// search files by pattern
	search_t *(*FS_Search)( const char *pattern, int caseinsensitive, int gamedironly );

	// get file size by name
	fs_offset_t (*FS_FileSize)( const char *filename, qboolean gamedironly );

	// get file time by name
	fs_offset_t (*FS_FileTime)( const char *filename, qboolean gamedironly );

	// get disk path by it's relative
	const char *(*FS_GetDiskPath)( const char *name, qboolean gamedironly );

	// find file
	searchpath_t *(*FS_FindFile)( const char *name, int* index, qboolean gamedironly );

	// get all search paths
	searchpath_t *(*FS_GetSearchPaths)();

	// add pack to search path
	qboolean (*FS_AddPack_Fullpath)( const char *pakfile, qboolean *already_loaded, qboolean keep_plain_dirs, int flags );

	// log spew
	void (*Msg)( const char *pMsg, ... );

	// memory utils
	void *(*_Mem_Alloc)( size_t size, const char *filename, int fileline );
	void (*_Mem_Free)( void *data, const char *filename, int fileline );
} fs_api_t;

// function exported from engine
// returns 0 on no error otherwise error
typedef int (*pfnFS_GetAPI)( fs_api_t *g_api );

#ifdef __cplusplus
}
#endif // __cplusplus


#endif // FS_INT_H

