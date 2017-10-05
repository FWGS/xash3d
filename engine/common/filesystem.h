/*
filesystem.h - engine FS
Copyright (C) 2007 Uncle Mike

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/

#ifndef FILESYSTEM_H
#define FILESYSTEM_H

/*
========================================================================
PAK FILES

The .pak files are just a linear collapse of a directory tree
========================================================================
*/
// header
#define IDPACKV1HEADER	(('K'<<24)+('C'<<16)+('A'<<8)+'P')	// little-endian "PACK"

#define MAX_FILES_IN_PACK	65536 // pak

typedef struct
{
	int		ident;
	int		dirofs;
	int		dirlen;
} dpackheader_t;

typedef struct
{
	char		name[56];		// total 64 bytes
	int		filepos;
	int		filelen;
} dpackfile_t;

/*
========================================================================
.WAD archive format	(WhereAllData - WAD)

List of compressed files, that can be identify only by TYPE_*

<format>
header:	dwadinfo_t[dwadinfo_t]
file_1:	byte[dwadinfo_t[num]->disksize]
file_2:	byte[dwadinfo_t[num]->disksize]
file_3:	byte[dwadinfo_t[num]->disksize]
...
file_n:	byte[dwadinfo_t[num]->disksize]
infotable	dlumpinfo_t[dwadinfo_t->numlumps]
========================================================================
*/
#define IDWAD2HEADER	(('2'<<24)+('D'<<16)+('A'<<8)+'W')	// little-endian "WAD2" quake1 gfx.wad
#define IDWAD3HEADER	(('3'<<24)+('D'<<16)+('A'<<8)+'W')	// little-endian "WAD3" half-life wads

#define WAD3_NAMELEN	16
#define HINT_NAMELEN	5	// e.g. _mask, _norm
#define MAX_FILES_IN_WAD	65535	// real limit as above <2Gb size not a lumpcount

// hidden virtual lump types
#define TYP_ANY		-1	// any type can be accepted
#define TYP_NONE		0	// unknown lump type

#include "const.h"

typedef struct
{
	int		ident;		// should be IWAD, WAD2 or WAD3
	int		numlumps;		// num files
	int		infotableofs;
} dwadinfo_t;

typedef struct
{
	int		filepos;		// file offset in WAD
	int		disksize;		// compressed or uncompressed
	int		size;		// uncompressed
	signed char		type;
	signed char		attribs;		// file attribs
	signed char		img_type;		// IMG_*
	signed char		pad;
	char		name[WAD3_NAMELEN];		// must be null terminated
} dlumpinfo_t;


struct wfile_s
{
	char		filename[MAX_SYSPATH];
	int		infotableofs;
	byte		*mempool;	// W_ReadLump temp buffers
	int		numlumps;
	int		mode;
	int		handle;
	dlumpinfo_t	*lumps;
	time_t		filetime;
};

typedef struct packfile_s
{
	char		name[56];
	fs_offset_t	offset;
	fs_offset_t	realsize;	// real file size (uncompressed)
} packfile_t;

typedef struct pack_s
{
	char		filename[MAX_SYSPATH];
	int		handle;
	int		numfiles;
	time_t		filetime;	// common for all packed files
	packfile_t	*files;
} pack_t;

#include "fs_int.h"

#include "custom.h"

#define IDCUSTOMHEADER	(('K'<<24)+('A'<<16)+('P'<<8)+'H') // little-endian "HPAK"
#define IDCUSTOM_VERSION	1

typedef struct hpak_s
{
	char		*name;
	resource_t	HpakResource;
	size_t		size;
	void		*data;
	struct hpak_s	*next;
} hpak_t;

typedef struct
{
	int		ident;		// should be equal HPAK
	int		version;
	int		seek;		// infotableofs ?
} hpak_header_t;

typedef struct
{
	resource_t	DirectoryResource;
	int		seek;		// filepos ?
	int		size;
} hpak_dir_t;

typedef struct
{
	int		count;
	hpak_dir_t	*dirs;		// variable sized.
} hpak_container_t;

#endif//FILESYSTEM_H
