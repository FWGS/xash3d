/*
img_mip.c - hl1 and q1 image mips
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

#include "imagelib.h"
#include "mathlib.h"
#include "wadfile.h"
#include "studio.h"
#include "sprite.h"
#include "qfont.h"

/*
============
Image_LoadPAL
============
*/
qboolean Image_LoadPAL( const char *name, const byte *buffer, size_t filesize )
{
	int	rendermode = LUMP_NORMAL; 

	if( filesize != 768 )
	{
		MsgDev( D_ERROR, "Image_LoadPAL: (%s) have invalid size (%d should be %d)\n", name, filesize, 768 );
		return false;
	}

	if( name[0] == '#' )
	{
		// using palette name as rendermode
		if( Q_stristr( name, "normal" ))
			rendermode = LUMP_NORMAL;
		else if( Q_stristr( name, "transparent" ))
			rendermode = LUMP_TRANSPARENT;
		else if( Q_stristr( name, "decal" ))
			rendermode = LUMP_DECAL;
		else if( Q_stristr( name, "qfont" ))
			rendermode = LUMP_QFONT;
		else if( Q_stristr( name, "valve" ))
			buffer = NULL; // force to get HL palette
	}

	// NOTE: image.d_currentpal not cleared with Image_Reset()
	// and stay valid any time before new call of Image_SetPalette
	Image_GetPaletteLMP( buffer, rendermode );
	Image_CopyPalette32bit();

	image.rgba = NULL;	// only palette, not real image
	image.size = 1024;	// expanded palette
	image.width = image.height = 0;
	
	return true;
}

/*
============
Image_LoadFNT
============
*/
qboolean Image_LoadFNT( const char *name, const byte *buffer, size_t filesize )
{
	qfont_t		font;
	const byte	*pal, *fin;
	size_t		size;
	int		numcolors;
	int i;

	if( image.hint == IL_HINT_Q1 )
		return false;	// Quake1 doesn't have qfonts

	if( filesize < sizeof( font ))
		return false;

	Q_memcpy( &font, buffer, sizeof( font ));
#ifdef XASH_BIG_ENDIAN
	LittleLongSW(font.height);
	LittleLongSW(font.width);
	LittleLongSW(font.rowcount);
	LittleLongSW(font.rowheight);
	for(i = 0; i < NUM_GLYPHS;i++)
	{
		LittleShortSW(font.fontinfo[i].charwidth);
		LittleShortSW(font.fontinfo[i].startoffset);
	}
#endif

	// last sixty four bytes - what the hell ????
	size = sizeof( qfont_t ) - 4 + ( font.height * font.width * QCHAR_WIDTH ) + sizeof( short ) + 768 + 64;

	if( size != filesize )
	{
		// oldstyle font: "conchars" or "creditsfont"
		image.width = 256;		// hardcoded
		image.height = font.height;
	}
	else
	{
		// Half-Life 1.1.0.0 font style (qfont_t)
		image.width = font.width * QCHAR_WIDTH;
		image.height = font.height;
	}

	if( !Image_LumpValidSize( name ))
		return false;

	fin = buffer + sizeof( font ) - 4;
	pal = fin + (image.width * image.height);
	numcolors = LittleShort(*(short *)pal), pal += sizeof( short );

	if( numcolors == 768 || numcolors == 256 )
	{
		// g-cont. make sure that is didn't hit anything
		Image_GetPaletteLMP( pal, LUMP_QFONT );
		image.flags |= IMAGE_HAS_ALPHA; // fonts always have transparency
	}
	else 
	{
		if( image.hint == IL_HINT_NO )
			MsgDev( D_ERROR, "Image_LoadFNT: (%s) have invalid palette size %d\n", name, numcolors );
		return false;
	}

	image.type = PF_INDEXED_32;	// 32-bit palette

	return Image_AddIndexedImageToPack( fin, image.width, image.height );
}
/*
======================
Image_SetMDLPointer

Transfer buffer pointer before Image_LoadMDL
======================
*/
void *g_mdltexdata;
void Image_SetMDLPointer(byte *p)
{
	g_mdltexdata = p;
}

/*
============
Image_LoadMDL
============
*/
qboolean Image_LoadMDL( const char *name, const byte *buffer, size_t filesize )
{
	byte		*fin;
	size_t		pixels;
	mstudiotexture_t	*pin;
	int		flags;

	pin = (mstudiotexture_t *)buffer;
	flags = pin->flags;

	image.width = pin->width;
	image.height = pin->height;
	pixels = image.width * image.height;

	fin = (byte *)g_mdltexdata;
	ASSERT(fin);
	g_mdltexdata = NULL;

	if( !Image_ValidSize( name )) return false;

	if( image.hint == IL_HINT_HL )
	{
		if( filesize < ( sizeof( *pin ) + pixels + 768 ))
			return false;

		if( flags & STUDIO_NF_TRANSPARENT )
		{
			byte	*pal = fin + pixels;

			// make transparent color is black, blue color looks ugly
			pal[255*3+0] = pal[255*3+1] = pal[255*3+2] = 0;

			Image_GetPaletteLMP( pal, LUMP_TRANSPARENT );
			image.flags |= IMAGE_HAS_ALPHA;
		}
		else Image_GetPaletteLMP( fin + pixels, LUMP_NORMAL );
	}
	else
	{
		if( image.hint == IL_HINT_NO )
			MsgDev( D_ERROR, "Image_LoadMDL: lump (%s) is corrupted\n", name );
		return false; // unknown or unsupported mode rejected
	}

	image.type = PF_INDEXED_32;	// 32-bit palete

	return Image_AddIndexedImageToPack( fin, image.width, image.height );
}

/*
============
Image_LoadSPR
============
*/
qboolean Image_LoadSPR( const char *name, const byte *buffer, size_t filesize )
{
	dspriteframe_t	pin;	// identical for q1\hl sprites

	if( image.hint == IL_HINT_HL )
	{
		if( !image.d_currentpal )
		{
			MsgDev( D_ERROR, "Image_LoadSPR: (%s) palette not installed\n", name );
			return false;		
		}
	}
	else if( image.hint == IL_HINT_Q1 )
	{
		Image_GetPaletteQ1();
	}
	else
	{
		// unknown mode rejected
		return false;
	}

	Q_memcpy( &pin, buffer, sizeof(dspriteframe_t) );
	image.width = LittleLong(pin.width);
	image.height = LittleLong(pin.height);

	if( filesize < image.width * image.height )
	{
		MsgDev( D_ERROR, "Image_LoadSPR: file (%s) have invalid size\n", name );
		return false;
	}

	// sorry, can't validate palette rendermode
	if( !Image_LumpValidSize( name )) return false;
	image.type = PF_INDEXED_32;	// 32-bit palete

	// detect alpha-channel by palette type
	switch( image.d_rendermode )
	{
	case LUMP_DECAL:
	case LUMP_TRANSPARENT:
		image.flags |= IMAGE_HAS_ALPHA;
		break;
	}

	// make transparent color is black, blue color looks ugly
	if( image.d_rendermode == LUMP_TRANSPARENT )
		image.d_currentpal[255] = 0;

	return Image_AddIndexedImageToPack( (byte *)(buffer + sizeof(dspriteframe_t)), image.width, image.height );
}

/*
============
Image_LoadLMP
============
*/
qboolean Image_LoadLMP( const char *name, const byte *buffer, size_t filesize )
{
	lmp_t	lmp;
	byte	*fin, *pal;
	int	rendermode;
	int	pixels;

	if( filesize < sizeof( lmp ))
	{
		MsgDev( D_ERROR, "Image_LoadLMP: file (%s) have invalid size\n", name );
		return false;
	}

	// greatest hack from valve software (particle palette)
	if( Q_stristr( name, "palette.lmp" ))
		return Image_LoadPAL( name, buffer, filesize );

	// greatest hack from id software
	if( image.hint != IL_HINT_HL && Q_stristr( name, "conchars" ))
	{
		image.width = image.height = 128;
		image.flags |= IMAGE_HAS_ALPHA;
		rendermode = LUMP_QFONT;
		filesize += sizeof(lmp);
		fin = (byte *)buffer;
	}
	else
	{
		fin = (byte *)buffer;
		Q_memcpy( &lmp, fin, sizeof( lmp ));
		image.width = LittleLong(lmp.width);
		image.height = LittleLong(lmp.height);
		rendermode = LUMP_NORMAL;
		fin += sizeof(lmp);
	}

	pixels = image.width * image.height;

	if( filesize < sizeof( lmp ) + pixels )
	{
		MsgDev( D_ERROR, "Image_LoadLMP: file (%s) have invalid size %d\n", name, filesize );
		return false;
	}

	if( !Image_ValidSize( name ))
		return false;         

	if( image.hint != IL_HINT_Q1 && filesize > (int)sizeof(lmp) + pixels )
	{
		int	numcolors;

		pal = fin + pixels;
		numcolors = LittleShort(*(short *)pal);
		if( numcolors != 256 ) pal = NULL; // corrupted lump ?
		else pal += sizeof( short );
	}
	else if( image.hint != IL_HINT_HL )
	{
		pal = NULL;
	}
	else
	{
		// unknown mode rejected
		return false;
	}

	if( fin[0] == 255 ) image.flags |= IMAGE_HAS_ALPHA;
	Image_GetPaletteLMP( pal, rendermode );
	image.type = PF_INDEXED_32; // 32-bit palete

	return Image_AddIndexedImageToPack( fin, image.width, image.height );
}

/*
=============
Image_LoadMIP
=============
*/
qboolean Image_LoadMIP( const char *name, const byte *buffer, size_t filesize )
{
	mip_t	mip;
	qboolean	hl_texture;
	byte	*fin, *pal;
	int	ofs[4], rendermode;
	int	i, pixels, numcolors;

	if( filesize < sizeof( mip ))
	{
		MsgDev( D_ERROR, "Image_LoadMIP: file (%s) have invalid size\n", name );
		return false;
	}

	Q_memcpy( &mip, buffer, sizeof( mip ));
	image.width = LittleLong(mip.width);
	image.height = LittleLong(mip.height);

	if( !Image_ValidSize( name ))
		return false;

	//Q_memcpy( ofs, mip.offsets, sizeof( ofs ));
	for( i = 0; i < 4; i++ )
		ofs[i] = LittleLong(mip.offsets[i]);

	pixels = image.width * image.height;

	if( image.hint != IL_HINT_Q1 && filesize >= (int)sizeof(mip) + ((pixels * 85)>>6) + sizeof(short) + 768)
	{
		// half-life 1.0.0.1 mip version with palette
		fin = (byte *)buffer + ofs[0];
		pal = (byte *)buffer + ofs[0] + (((image.width * image.height) * 85)>>6);
		numcolors = LittleShort(*(short *)pal);
		if( numcolors != 256 ) pal = NULL; // corrupted mip ?
		else pal += sizeof( short ); // skip colorsize 

		hl_texture = true;

		// setup rendermode
		if( Q_strrchr( name, '{' ))
		{
			if( !host.decal_loading )
			{
				rendermode = LUMP_TRANSPARENT;

				// make transparent color is black, blue color looks ugly
				pal[255*3+0] = pal[255*3+1] = pal[255*3+2] = 0;
			}
			else
			{
				// clear blue color for 'transparent' decals
				if( pal[255*3+0] == 0 && pal[255*3+1] == 0 && pal[255*3+2] == 255 )
					pal[255*3+0] = pal[255*3+1] = pal[255*3+2] = 0;

				// apply decal palette immediately
				image.flags |= IMAGE_COLORINDEX;
				rendermode = LUMP_DECAL;
			}

			image.flags |= IMAGE_HAS_ALPHA;
		}
		else
		{
			int	pal_type;

			// NOTE: we can have luma-pixels if quake1 texture
			// converted into the hl texture but palette leave unchanged
			// this is a good reason for using fullbright pixels
			pal_type = Image_ComparePalette( pal );

			// check for luma pixels (but ignore liquid textures, this a Xash3D limitation)
			if( mip.name[0] != '!' && pal_type == PAL_QUAKE1 )
			{
				for( i = 0; i < image.width * image.height; i++ )
				{
					if( fin[i] > 224 )
					{
						image.flags |= IMAGE_HAS_LUMA;
						break;
					}
				}
			}

			rendermode = LUMP_NORMAL;
		}

		Image_GetPaletteLMP( pal, rendermode );
		image.d_currentpal[255] &= 0xFFFFFF;
	}
	else if( image.hint != IL_HINT_HL && filesize >= (int)sizeof(mip) + ((pixels * 85)>>6))
	{
		// quake1 1.01 mip version without palette
		fin = (byte *)buffer + ofs[0];
		pal = NULL; // clear palette
		rendermode = LUMP_NORMAL;

		hl_texture = false;

		// check for luma pixels
		for( i = 0; i < image.width * image.height; i++ )
		{
			if( fin[i] > 224 )
			{
				// don't apply luma to water surfaces because
				// we use glpoly->next for store luma chain each frame
				// and can't modify glpoly_t because many-many HL mods
				// expected unmodified glpoly_t and can crashes on changed struct
				// water surfaces uses glpoly->next as pointer to subdivided surfaces (as q1)
				if( mip.name[0] != '*' && mip.name[0] != '!' )
					image.flags |= IMAGE_HAS_LUMA;
				break;
			}
		}

		Image_GetPaletteQ1();
	}
	else
	{
		if( image.hint == IL_HINT_NO )
			MsgDev( D_ERROR, "Image_LoadMIP: lump (%s) is corrupted\n", name );
		return false; // unknown or unsupported mode rejected
	} 

	// check for quake-sky texture
	if( !Q_strncmp( mip.name, "sky", 3 ) && image.width == ( image.height * 2 ))
	{
		// g-cont: we need to run additional checks for palette type and colors ?
		image.flags |= IMAGE_QUAKESKY;
	}

	// check for half-life water texture
	if( hl_texture && ( mip.name[0] == '!' || !Q_strnicmp( mip.name, "water", 5 )))
	{
		// grab the fog color
		image.fogParams[0] = pal[3*3+0];
		image.fogParams[1] = pal[3*3+1];
		image.fogParams[2] = pal[3*3+2];

		// grab the fog density
		image.fogParams[3] = pal[4*3+0];
	}
	else if( hl_texture && host.decal_loading )
	{
		// grab the decal color
		image.fogParams[0] = pal[255*3+0];
		image.fogParams[1] = pal[255*3+1];
		image.fogParams[2] = pal[255*3+2];

		// calc the decal reflectivity
		image.fogParams[3] = VectorAvg( image.fogParams );         
	}
 
	image.type = PF_INDEXED_32;	// 32-bit palete
	return Image_AddIndexedImageToPack( fin, image.width, image.height );
}
