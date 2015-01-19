/*
imagelib.h - engine image lib
Copyright (C) 2008 Uncle Mike

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/

#ifndef IMAGELIB_H
#define IMAGELIB_H

#include "common.h"

// skyorder_q2[6] = { 2, 3, 1, 0, 4, 5, }; // Quake, Half-Life skybox ordering
// skyorder_ms[6] = { 4, 5, 1, 0, 2, 3  }; // Microsoft DDS ordering (reverse)

// cubemap hints
typedef enum
{
	CB_HINT_NO = 0,

	// dds cubemap hints ( Microsoft sides order )
	CB_HINT_POSX,
	CB_HINT_NEGX,
	CB_HINT_POSZ,
	CB_HINT_NEGZ,
	CB_HINT_POSY,
	CB_HINT_NEGY,
	CB_FACECOUNT,
} side_hint_t;

typedef enum
{
	IL_HINT_NO = 0,
	IL_HINT_Q1,	// palette choosing
	IL_HINT_HL,
} image_hint_t;

typedef struct loadformat_s
{
	const char *formatstring;
	const char *ext;
	qboolean (*loadfunc)( const char *name, const byte *buffer, size_t filesize );
	image_hint_t hint;
} loadpixformat_t;

typedef struct saveformat_s
{
	const char *formatstring;
	const char *ext;
	qboolean (*savefunc)( const char *name, rgbdata_t *pix );
} savepixformat_t;

typedef struct imglib_s
{
	const loadpixformat_t	*loadformats;
	const savepixformat_t	*saveformats;

	// current 2d image state
	word			width;
	word			height;
	word			depth;
	uint			type;		// main type switcher
	uint			flags;		// additional image flags
	size_t			size;		// image rgba size (for bounds checking)
	uint			ptr;		// safe image pointer
	int			bpp;		// PFDesc[type].bpp
	byte			*rgba;		// image pointer (see image_type for details)

	// current cubemap state
	int			source_width;	// locked cubemap dims (all wrong sides will be automatically resampled)
	int			source_height;
	uint			source_type;	// shared image type for all mipmaps or cubemap sides
	int			num_sides;	// how much sides is loaded 
	byte			*cubemap;		// cubemap pack

	// indexed images state
	uint			*d_currentpal;	// installed version of internal palette
	int			d_rendermode;	// palette rendermode
	byte			*palette;		// palette pointer

	// global parms
	rgba_t			fogParams;	// some water textures has info about underwater fog

	image_hint_t		hint;		// hint for some loaders
	byte			*tempbuffer;	// for convert operations
	int			cmd_flags;	// global imglib flags
	int			force_flags;	// override cmd_flags
} imglib_t;

/*
========================================================================

.BMP image format

========================================================================
*/
#pragma pack( 1 )
typedef struct
{
	char	id[2];		// bmfh.bfType
	dword	fileSize;		// bmfh.bfSize
	dword	reserved0;	// bmfh.bfReserved1 + bmfh.bfReserved2
	dword	bitmapDataOffset;	// bmfh.bfOffBits
	dword	bitmapHeaderSize;	// bmih.biSize
	int	width;		// bmih.biWidth
	int	height;		// bmih.biHeight
	word	planes;		// bmih.biPlanes
	word	bitsPerPixel;	// bmih.biBitCount
	dword	compression;	// bmih.biCompression
	dword	bitmapDataSize;	// bmih.biSizeImage
	dword	hRes;		// bmih.biXPelsPerMeter
	dword	vRes;		// bmih.biYPelsPerMeter
	dword	colors;		// bmih.biClrUsed
	dword	importantColors;	// bmih.biClrImportant
} bmp_t;
#pragma pack( )

/*
========================================================================

.TGA image format	(Truevision Targa)

========================================================================
*/
#pragma pack( 1 )
typedef struct tga_s
{
	byte	id_length;
	byte	colormap_type;
	byte	image_type;
	word	colormap_index;
	word	colormap_length;
	byte	colormap_size;
	word	x_origin;
	word	y_origin;
	word	width;
	word	height;
	byte	pixel_size;
	byte	attributes;
} tga_t;
#pragma pack( )

// imagelib definitions
#define IMAGE_MAXWIDTH	4096
#define IMAGE_MAXHEIGHT	4096
#define LUMP_MAXWIDTH	1024	// WorldCraft limits
#define LUMP_MAXHEIGHT	1024

enum
{
	LUMP_NORMAL = 0,
	LUMP_TRANSPARENT,
	LUMP_DECAL,
	LUMP_QFONT,
	LUMP_EXTENDED		// bmp images have extened palette with alpha-channel
};

enum
{
	PAL_INVALID = -1,
	PAL_CUSTOM = 0,
	PAL_QUAKE1,
	PAL_HALFLIFE
};

extern imglib_t image;

void Image_RoundDimensions( int *scaled_width, int *scaled_height );
byte *Image_ResampleInternal( const void *indata, int in_w, int in_h, int out_w, int out_h, int intype, qboolean *done );
byte *Image_FlipInternal( const byte *in, word *srcwidth, word *srcheight, int type, int flags );
void Image_PaletteHueReplace( byte *palSrc, int newHue, int start, int end );
rgbdata_t *Image_Load(const char *filename, const byte *buffer, size_t buffsize );
qboolean Image_Copy8bitRGBA( const byte *in, byte *out, int pixels );
qboolean Image_AddIndexedImageToPack( const byte *in, int width, int height );
qboolean Image_AddRGBAImageToPack( uint imageSize, const void* data );
void Image_Save( const char *filename, rgbdata_t *pix );
void Image_ConvertPalTo24bit( rgbdata_t *pic );
void Image_GetPaletteLMP( const byte *pal, int rendermode );
void Image_GetPaletteBMP( const byte *pal );
int Image_ComparePalette( const byte *pal );
void Image_FreeImage( rgbdata_t *pack );
void Image_CopyPalette24bit( void );
void Image_CopyPalette32bit( void );
void Image_SetPixelFormat( void );
void Image_GetPaletteQ1( void );
void Image_GetPaletteHL( void );

//
// formats load
//
qboolean Image_LoadMIP( const char *name, const byte *buffer, size_t filesize );
qboolean Image_LoadMDL( const char *name, const byte *buffer, size_t filesize );
qboolean Image_LoadSPR( const char *name, const byte *buffer, size_t filesize );
qboolean Image_LoadTGA( const char *name, const byte *buffer, size_t filesize );
qboolean Image_LoadBMP( const char *name, const byte *buffer, size_t filesize );
qboolean Image_LoadFNT( const char *name, const byte *buffer, size_t filesize );
qboolean Image_LoadLMP( const char *name, const byte *buffer, size_t filesize );
qboolean Image_LoadPAL( const char *name, const byte *buffer, size_t filesize );

//
// formats save
//
qboolean Image_SaveTGA( const char *name, rgbdata_t *pix );
qboolean Image_SaveBMP( const char *name, rgbdata_t *pix );

//
// img_quant.c
//
rgbdata_t *Image_Quantize( rgbdata_t *pic );

//
// img_utils.c
//
void Image_Reset( void );
rgbdata_t *ImagePack( void );
byte *Image_Copy( size_t size );
void Image_CopyParms( rgbdata_t *src );
qboolean Image_ValidSize( const char *name );
qboolean Image_LumpValidSize( const char *name );
qboolean Image_CheckFlag( int bit );

#endif//IMAGELIB_H