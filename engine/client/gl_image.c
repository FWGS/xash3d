/*
gl_image.c - texture uploading and processing
Copyright (C) 2010 Uncle Mike

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/

#include "common.h"
#include "client.h"
#include "gl_local.h"
#include "studio.h"

#define TEXTURES_HASH_SIZE	64

static rgbdata_t	*R_LoadImage( char **buffer, const char *name, const byte *buf, size_t size, int *samples, texFlags_t *flags );
static int	r_textureMinFilter = GL_LINEAR_MIPMAP_LINEAR;
static int	r_textureMagFilter = GL_LINEAR;
static gltexture_t	r_textures[MAX_TEXTURES];
static gltexture_t	*r_texturesHashTable[TEXTURES_HASH_SIZE];
static int	r_numTextures;
static byte	*scaledImage = NULL;			// pointer to a scaled image
static byte	data2D[BLOCK_SIZE_MAX*BLOCK_SIZE_MAX*4];	// intermediate texbuffer
static rgbdata_t	r_image;					// generic pixelbuffer used for internal textures

// internal tables
static vec3_t	r_luminanceTable[256];	// RGB to luminance
static byte	r_particleTexture[8][8] =
{
{0,0,0,0,0,0,0,0},
{0,0,0,1,1,0,0,0},
{0,0,0,1,1,0,0,0},
{0,1,1,1,1,1,1,0},
{0,1,1,1,1,1,1,0},
{0,0,0,1,1,0,0,0},
{0,0,0,1,1,0,0,0},
{0,0,0,0,0,0,0,0},
};

const char *GL_Target( GLenum target )
{
	switch( target )
	{
	case GL_TEXTURE_1D:
		return "1D";
	case GL_TEXTURE_2D:
		return "2D";
	case GL_TEXTURE_3D:
		return "3D";
	case GL_TEXTURE_CUBE_MAP_ARB:
		return "Cube";
	case GL_TEXTURE_RECTANGLE_EXT:
		return "Rect";
	}
	return "??";
}

/*
=================
GL_Bind
=================
*/
void GL_Bind( GLint tmu, GLenum texnum )
{
	gltexture_t	*texture;

	// missed texture ?
	if( texnum <= 0 ) texnum = tr.defaultTexture;
	ASSERT( texnum > 0 && texnum < MAX_TEXTURES );

	if( tmu != GL_KEEP_UNIT )
		GL_SelectTexture( tmu );
	else tmu = glState.activeTMU;

	texture = &r_textures[texnum];

	if( glState.currentTextureTargets[tmu] != texture->target )
	{
		if( glState.currentTextureTargets[tmu] != GL_NONE )
			pglDisable( glState.currentTextureTargets[tmu] );
		glState.currentTextureTargets[tmu] = texture->target;
		pglEnable( glState.currentTextureTargets[tmu] );
	}

	if( glState.currentTextures[tmu] == texture->texnum )
		return;

	pglBindTexture( texture->target, texture->texnum );
	glState.currentTextures[tmu] = texture->texnum;
}

/*
=================
R_GetTexture
=================
*/
gltexture_t *R_GetTexture( GLenum texnum )
{
	ASSERT( texnum >= 0 && texnum < MAX_TEXTURES );
	return &r_textures[texnum];
}

/*
=================
GL_SetTextureType

Just for debug (r_showtextures uses it)
=================
*/
void GL_SetTextureType( GLenum texnum, GLenum type )
{
	if( texnum <= 0 ) return;
	ASSERT( texnum >= 0 && texnum < MAX_TEXTURES );
	r_textures[texnum].texType = type;
}

/*
=================
GL_TexFilter
=================
*/
void GL_TexFilter( gltexture_t *tex, qboolean update )
{
	qboolean	allowNearest;
	vec4_t	zeroClampBorder = { 0.0f, 0.0f, 0.0f, 1.0f };
	vec4_t	alphaZeroClampBorder = { 0.0f, 0.0f, 0.0f, 0.0f };

	switch( tex->texType )
	{
	case TEX_NOMIP:
	case TEX_CUBEMAP:
	case TEX_LIGHTMAP:
		allowNearest = false;
		break;
	default:
		allowNearest = true;
		break;
	}

	// set texture filter
	if( tex->flags & TF_DEPTHMAP )
	{
		pglTexParameteri( tex->target, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
		pglTexParameteri( tex->target, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

		if( !( tex->flags & TF_NOCOMPARE ))
		{
			pglTexParameteri( tex->target, GL_TEXTURE_COMPARE_FUNC_ARB, GL_LEQUAL );
			pglTexParameteri( tex->target, GL_TEXTURE_COMPARE_MODE_ARB, GL_COMPARE_R_TO_TEXTURE_ARB );
		}

		if( tex->flags & TF_LUMINANCE )
			pglTexParameteri( tex->target, GL_DEPTH_TEXTURE_MODE_ARB, GL_LUMINANCE );
		else pglTexParameteri( tex->target, GL_DEPTH_TEXTURE_MODE_ARB, GL_INTENSITY );

		if( GL_Support( GL_ANISOTROPY_EXT ))
			pglTexParameterf( tex->target, GL_TEXTURE_MAX_ANISOTROPY_EXT, 1.0f );
	}
	else if( tex->flags & TF_NOMIPMAP )
	{
		if( tex->flags & TF_NEAREST )
		{
			pglTexParameteri( tex->target, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
			pglTexParameteri( tex->target, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
		}
		else
		{
			if( r_textureMagFilter == GL_NEAREST && allowNearest )
			{
				pglTexParameteri( tex->target, GL_TEXTURE_MIN_FILTER, r_textureMagFilter );
				pglTexParameteri( tex->target, GL_TEXTURE_MAG_FILTER, r_textureMagFilter );
			}
			else
			{
				pglTexParameteri( tex->target, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
				pglTexParameteri( tex->target, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
			}
		}
	}
	else
	{
		if( tex->flags & TF_NEAREST )
		{
			pglTexParameteri( tex->target, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST );
			pglTexParameteri( tex->target, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
		}
		else
		{
			pglTexParameteri( tex->target, GL_TEXTURE_MIN_FILTER, r_textureMinFilter );
			pglTexParameteri( tex->target, GL_TEXTURE_MAG_FILTER, r_textureMagFilter );
		}

		// set texture anisotropy if available
		if( GL_Support( GL_ANISOTROPY_EXT ) && !( tex->flags & TF_ALPHACONTRAST ))
			pglTexParameterf( tex->target, GL_TEXTURE_MAX_ANISOTROPY_EXT, gl_texture_anisotropy->value );

		// set texture LOD bias if available
		if( GL_Support( GL_TEXTURE_LODBIAS ))
			pglTexParameterf( tex->target, GL_TEXTURE_LOD_BIAS_EXT, gl_texture_lodbias->value );
	}

	if( update ) return;

	if( tex->flags & ( TF_BORDER|TF_ALPHA_BORDER ) && !GL_Support( GL_CLAMP_TEXBORDER_EXT ))
	{
		// border is not support, use clamp instead
		tex->flags &= ~(TF_BORDER||TF_ALPHA_BORDER);
		tex->flags |= TF_CLAMP;
	}

	// set texture wrap
	if( tex->flags & TF_CLAMP )
	{
		if( GL_Support( GL_CLAMPTOEDGE_EXT ))
		{
			pglTexParameteri( tex->target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );

			if( tex->target != GL_TEXTURE_1D )
				pglTexParameteri( tex->target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );

			if( tex->target == GL_TEXTURE_3D || tex->target == GL_TEXTURE_CUBE_MAP_ARB )
				pglTexParameteri( tex->target, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE );
		}
		else
		{
			pglTexParameteri( tex->target, GL_TEXTURE_WRAP_S, GL_CLAMP );

			if( tex->target != GL_TEXTURE_1D )
				pglTexParameteri( tex->target, GL_TEXTURE_WRAP_T, GL_CLAMP );

			if( tex->target == GL_TEXTURE_3D || tex->target == GL_TEXTURE_CUBE_MAP_ARB )
				pglTexParameteri( tex->target, GL_TEXTURE_WRAP_R, GL_CLAMP );
		}
	}
	else if( tex->flags & ( TF_BORDER|TF_ALPHA_BORDER ))
	{
		pglTexParameteri( tex->target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER );

		if( tex->target != GL_TEXTURE_1D )
			pglTexParameteri( tex->target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER );

		if( tex->target == GL_TEXTURE_3D || tex->target == GL_TEXTURE_CUBE_MAP_ARB )
			pglTexParameteri( tex->target, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER );

		if( tex->flags & TF_BORDER )
			pglTexParameterfv( tex->target, GL_TEXTURE_BORDER_COLOR, zeroClampBorder );
		else if( tex->flags & TF_ALPHA_BORDER )
			pglTexParameterfv( tex->target, GL_TEXTURE_BORDER_COLOR, alphaZeroClampBorder );
	}
	else
	{
		pglTexParameteri( tex->target, GL_TEXTURE_WRAP_S, GL_REPEAT );

		if( tex->target != GL_TEXTURE_1D )
			pglTexParameteri( tex->target, GL_TEXTURE_WRAP_T, GL_REPEAT );

		if( tex->target == GL_TEXTURE_3D || tex->target == GL_TEXTURE_CUBE_MAP_ARB )
			pglTexParameteri( tex->target, GL_TEXTURE_WRAP_R, GL_REPEAT );
	}
}

/*
=================
R_SetTextureParameters
=================
*/
void R_SetTextureParameters( void )
{
	gltexture_t	*texture;
	int		i;

	if( !Q_stricmp( gl_texturemode->string, "GL_NEAREST" ))
	{
		r_textureMinFilter = GL_NEAREST;
		r_textureMagFilter = GL_NEAREST;
	}
	else if( !Q_stricmp( gl_texturemode->string, "GL_LINEAR" ))
	{
		r_textureMinFilter = GL_LINEAR;
		r_textureMagFilter = GL_LINEAR;
	}
	else if( !Q_stricmp( gl_texturemode->string, "GL_NEAREST_MIPMAP_NEAREST" ))
	{
		r_textureMinFilter = GL_NEAREST_MIPMAP_NEAREST;
		r_textureMagFilter = GL_NEAREST;
	}
	else if( !Q_stricmp( gl_texturemode->string, "GL_LINEAR_MIPMAP_NEAREST" ))
	{
		r_textureMinFilter = GL_LINEAR_MIPMAP_NEAREST;
		r_textureMagFilter = GL_LINEAR;
	}
	else if( !Q_stricmp( gl_texturemode->string, "GL_NEAREST_MIPMAP_LINEAR" ))
	{
		r_textureMinFilter = GL_NEAREST_MIPMAP_LINEAR;
		r_textureMagFilter = GL_NEAREST;
	}
	else if( !Q_stricmp( gl_texturemode->string, "GL_LINEAR_MIPMAP_LINEAR" ))
	{
		r_textureMinFilter = GL_LINEAR_MIPMAP_LINEAR;
		r_textureMagFilter = GL_LINEAR;
	}
	else
	{
		MsgDev( D_ERROR, "gl_texturemode invalid mode %s, defaulting to GL_LINEAR_MIPMAP_LINEAR\n", gl_texturemode->string );
		Cvar_Set( "gl_texturemode", "GL_LINEAR_MIPMAP_LINEAR" );
		r_textureMinFilter = GL_LINEAR_MIPMAP_LINEAR;
		r_textureMagFilter = GL_LINEAR;
	}

	gl_texturemode->modified = false;

	if( GL_Support( GL_ANISOTROPY_EXT ))
	{
		if( gl_texture_anisotropy->value > glConfig.max_texture_anisotropy )
			Cvar_SetFloat( "gl_anisotropy", glConfig.max_texture_anisotropy );
		else if( gl_texture_anisotropy->value < 1.0f )
			Cvar_SetFloat( "gl_anisotropy", 1.0f );
	}

	gl_texture_anisotropy->modified = false;

	if( GL_Support( GL_TEXTURE_LODBIAS ))
	{
		if( gl_texture_lodbias->value > glConfig.max_texture_lodbias )
			Cvar_SetFloat( "gl_texture_lodbias", glConfig.max_texture_lodbias );
		else if( gl_texture_lodbias->value < -glConfig.max_texture_lodbias )
			Cvar_SetFloat( "gl_texture_lodbias", -glConfig.max_texture_lodbias );
	}

	gl_texture_lodbias->modified = false;

	// change all the existing mipmapped texture objects
	for( i = 0, texture = r_textures; i < r_numTextures; i++, texture++ )
	{
		if( !texture->texnum ) continue;	// free slot
		GL_Bind( GL_TEXTURE0, i );
		GL_TexFilter( texture, true );
	}
}

/*
===============
R_TextureList_f
===============
*/
void R_TextureList_f( void )
{
	gltexture_t	*image;
	int		i, texCount, bytes = 0;

	Msg( "\n" );
	Msg("      -w-- -h-- -size- -fmt- type -filter -wrap-- -name--------\n" );

	for( i = texCount = 0, image = r_textures; i < r_numTextures; i++, image++ )
	{
		if( !image->texnum ) continue;

		bytes += image->size;
		texCount++;

		Msg( "%4i: ", i );
		Msg( "%4i %4i ", image->width, image->height );
		Msg( "%5ik ", image->size >> 10 );

		switch( image->format )
		{
		case GL_COMPRESSED_RGBA_ARB:
			Msg( "CRGBA " );
			break;
		case GL_COMPRESSED_RGB_ARB:
			Msg( "CRGB  " );
			break;
		case GL_COMPRESSED_LUMINANCE_ALPHA_ARB:
			Msg( "CLA   " );
			break;
		case GL_COMPRESSED_LUMINANCE_ARB:
			Msg( "CL    " );
			break;
		case GL_COMPRESSED_ALPHA_ARB:
			Msg( "CA    " );
			break;
		case GL_COMPRESSED_INTENSITY_ARB:
			Msg( "CI    " );
			break;
		case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
		case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
			Msg( "DXT1  " );
			break;
		case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
			Msg( "DXT3  " );
			break;
		case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
			Msg( "DXT5  " );
			break;
		case GL_RGBA:
			Msg( "RGBA  " );
			break;
		case GL_RGBA8:
			Msg( "RGBA8 " );
			break;
		case GL_RGBA4:
			Msg( "RGBA4 " );
			break;
		case GL_RGB:
			Msg( "RGB   " );
			break;
		case GL_RGB8:
			Msg( "RGB8  " );
			break;
		case GL_RGB5:
			Msg( "RGB5  " );
			break;
		case GL_LUMINANCE4_ALPHA4:
			Msg( "L4A4  " );
			break;
		case GL_LUMINANCE_ALPHA:
		case GL_LUMINANCE8_ALPHA8:
			Msg( "L8A8  " );
			break;
		case GL_LUMINANCE4:
			Msg( "L4    " );
			break;
		case GL_LUMINANCE:
		case GL_LUMINANCE8:
			Msg( "L8    " );
			break;
		case GL_ALPHA8:
			Msg( "A8    " );
			break;
		case GL_INTENSITY8:
			Msg( "I8    " );
			break;
		case GL_DEPTH_COMPONENT:
			Msg( "DEPTH " );
			break;			
		case GL_LUMINANCE16F_ARB:
			Msg( "L16F  " );
			break;
		case GL_LUMINANCE32F_ARB:
			Msg( "L32F  " );
			break;
		case GL_LUMINANCE_ALPHA16F_ARB:
			Msg( "LA16F " );
			break;
		case GL_LUMINANCE_ALPHA32F_ARB:
			Msg( "LA32F " );
			break;
		case GL_RGB16F_ARB:
			Msg( "RGB16F" );
			break;
		case GL_RGB32F_ARB:
			Msg( "RGB32F" );
			break;
		case GL_RGBA16F_ARB:
			Msg( "RGBA16F" );
			break;
		case GL_RGBA32F_ARB:
			Msg( "RGBA32F" );
			break;
		default:
			Msg( "????? " );
			break;
		}

		switch( image->target )
		{
		case GL_TEXTURE_1D:
			Msg( " 1D  " );
			break;
		case GL_TEXTURE_2D:
			Msg( " 2D  " );
			break;
		case GL_TEXTURE_3D:
			Msg( " 3D  " );
			break;
		case GL_TEXTURE_CUBE_MAP_ARB:
			Msg( "CUBE " );
			break;
		case GL_TEXTURE_RECTANGLE_EXT:
			Msg( "RECT " );
			break;
		default:
			Msg( "???? " );
			break;
		}

		if( image->flags & TF_NORMALMAP )
			Msg( "normal " );
		else if( image->flags & TF_NOMIPMAP )
			Msg( "linear " );
		if( image->flags & TF_NEAREST )
			Msg( "nearest" );
		else Msg( "default" );

		if( image->flags & TF_CLAMP )
			Msg( " clamp  " );
		else if( image->flags & TF_BORDER )
			Msg( " border " );
		else if( image->flags & TF_ALPHA_BORDER )
			Msg( " aborder" );
		else Msg( " repeat " );
		Msg( "  %s\n", image->name );
	}

	Msg( "---------------------------------------------------------\n" );
	Msg( "%i total textures\n", texCount );
	Msg( "%s total memory used\n", Q_memprint( bytes ));
	Msg( "\n" );
}

/*
================
GL_CalcTextureSamples
================
*/
int GL_CalcTextureSamples( int flags )
{
	if( flags & IMAGE_HAS_COLOR )
		return (flags & IMAGE_HAS_ALPHA) ? 4 : 3;
	return (flags & IMAGE_HAS_ALPHA) ? 2 : 1;
}

/*
================
GL_ImageFlagsFromSamples
================
*/
int GL_ImageFlagsFromSamples( int samples )
{
	switch( samples )
	{
	case 2: return IMAGE_HAS_ALPHA;
	case 3: return IMAGE_HAS_COLOR;
	case 4: return (IMAGE_HAS_COLOR|IMAGE_HAS_ALPHA);
	}

	return 0;
}

/*
================
GL_CalcImageSamples
================
*/
int GL_CalcImageSamples( int s1, int s2 )
{
	int	samples;

	if( s1 == 1 ) samples = s2;
	else if( s1 == 2 )
	{
		if( s2 == 3 || s2 == 4 )
			samples = 4;
		else samples = 2;
	}
	else if( s1 == 3 )
	{
		if( s2 == 2 || s2 == 4 )
			samples = 4;
		else samples = 3;
	}
	else samples = s1;

	return samples;
}

/*
================
GL_RoundImageDimensions
================
*/
void GL_RoundImageDimensions( word *width, word *height, texFlags_t flags, qboolean force )
{
	int	scaledWidth, scaledHeight;

	scaledWidth = *width;
	scaledHeight = *height;

	if( flags & ( TF_TEXTURE_1D|TF_TEXTURE_3D )) return;

	if( force || !GL_Support( GL_ARB_TEXTURE_NPOT_EXT ))
	{
		// find nearest power of two, rounding down if desired
		scaledWidth = NearestPOW( scaledWidth, gl_round_down->integer );
		scaledHeight = NearestPOW( scaledHeight, gl_round_down->integer );
	}

	if( flags & TF_SKYSIDE )
	{
		// let people sample down the sky textures for speed
		scaledWidth >>= gl_skymip->integer;
		scaledHeight >>= gl_skymip->integer;
	}
	else if(!( flags & TF_NOPICMIP ))
	{
		// let people sample down the world textures for speed
		scaledWidth >>= gl_picmip->integer;
		scaledHeight >>= gl_picmip->integer;
	}

	if( flags & TF_CUBEMAP )
	{
		while( scaledWidth > glConfig.max_cubemap_size || scaledHeight > glConfig.max_cubemap_size )
		{
			scaledWidth >>= 1;
			scaledHeight >>= 1;
		}
	}
	else
	{
		if( flags & TF_TEXTURE_RECTANGLE )
		{
			while( scaledWidth > glConfig.max_2d_rectangle_size || scaledHeight > glConfig.max_2d_rectangle_size )
			{
				scaledWidth >>= 1;
				scaledHeight >>= 1;
			}
		}
		else
		{
			while( scaledWidth > glConfig.max_2d_texture_size || scaledHeight > glConfig.max_2d_texture_size )
			{
				scaledWidth >>= 1;
				scaledHeight >>= 1;
			}
		}
	}

	if( scaledWidth < 1 ) scaledWidth = 1;
	if( scaledHeight < 1 ) scaledHeight = 1;

	*width = scaledWidth;
	*height = scaledHeight;
}

/*
===============
GL_TextureFormat
===============
*/
static GLenum GL_TextureFormat( gltexture_t *tex, int *samples )
{
	qboolean	compress;
	GLenum	format;

	// check if it should be compressed
	if( !gl_compress_textures->integer || ( tex->flags & TF_UNCOMPRESSED ))
		compress = false;
	else compress = GL_Support( GL_TEXTURE_COMPRESSION_EXT );

	// set texture format
	if( tex->flags & TF_DEPTHMAP )
	{
		format = GL_DEPTH_COMPONENT;
		tex->flags &= ~TF_INTENSITY;
	}
	else if( tex->flags & TF_FLOAT && GL_Support( GL_ARB_TEXTURE_FLOAT_EXT ))
	{
		int	bits = glw_state.desktopBitsPixel;

		switch( *samples )
		{
		case 1:
			switch( bits )
			{
			case 16: format = GL_LUMINANCE16F_ARB; break;
			default: format = GL_LUMINANCE32F_ARB; break;
			}
			break;
		case 2:
			switch( bits )
			{
			case 16: format = GL_LUMINANCE_ALPHA16F_ARB; break;
			default: format = GL_LUMINANCE_ALPHA32F_ARB; break;
			}
			break;
		case 3:
			switch( bits )
			{
			case 16: format = GL_RGB16F_ARB; break;
			default: format = GL_RGB32F_ARB; break;
			}
			break;		
		case 4:
		default:
			switch( bits )
			{
			case 16: format = GL_RGBA16F_ARB; break;
			default: format = GL_RGBA32F_ARB; break;
			}
			break;
		}
	}
	else if( compress )
	{
		switch( *samples )
		{
		case 1: format = GL_COMPRESSED_LUMINANCE_ARB; break;
		case 2: format = GL_COMPRESSED_LUMINANCE_ALPHA_ARB; break;
		case 3: format = GL_COMPRESSED_RGB_ARB; break;
		case 4:
		default: format = GL_COMPRESSED_RGBA_ARB; break;
		}

		if( tex->flags & TF_INTENSITY )
			format = GL_COMPRESSED_INTENSITY_ARB;
		tex->flags &= ~TF_INTENSITY;
	}
	else
	{
		int	bits = glw_state.desktopBitsPixel;

		switch( *samples )
		{
		case 1: format = GL_LUMINANCE8; break;
		case 2: format = GL_LUMINANCE8_ALPHA8; break;
		case 3:
			if( gl_luminance_textures->integer )
			{
				switch( bits )
				{
				case 16: format = GL_LUMINANCE4; break;
				case 32: format = GL_LUMINANCE8; break;
				default: format = GL_LUMINANCE; break;
				}
				*samples = 1;	// merge for right calc statistics
			}
			else
			{
				switch( bits )
				{
				case 16: format = GL_RGB5; break;
				case 32: format = GL_RGB8; break;
				default: format = GL_RGB; break;
				}
			}
			break;		
		case 4:
		default:
			if( gl_luminance_textures->integer )
			{
				switch( bits )
				{
				case 16: format = GL_LUMINANCE4_ALPHA4; break;
				case 32: format = GL_LUMINANCE8_ALPHA8; break;
				default: format = GL_LUMINANCE_ALPHA; break;
				}
				*samples = 2;	// merge for right calc statistics
			}
			else
			{
				switch( bits )
				{
				case 16: format = GL_RGBA4; break;
				case 32: format = GL_RGBA8; break;
				default: format = GL_RGBA; break;
				}
			}
			break;
		}

		if( tex->flags & TF_INTENSITY )
			format = GL_INTENSITY8;
		tex->flags &= ~TF_INTENSITY;
	}
	return format;
}

/*
=================
GL_ResampleTexture

Assume input buffer is RGBA
=================
*/
byte *GL_ResampleTexture( const byte *source, int inWidth, int inHeight, int outWidth, int outHeight, qboolean isNormalMap )
{
	uint		frac, fracStep;
	uint		*in = (uint *)source;
	uint		p1[0x1000], p2[0x1000];
	byte		*pix1, *pix2, *pix3, *pix4;
	uint		*out, *inRow1, *inRow2;
	vec3_t		normal;
	int		i, x, y;

	if( !source ) return NULL;

	scaledImage = Mem_Realloc( r_temppool, scaledImage, outWidth * outHeight * 4 );
	fracStep = inWidth * 0x10000 / outWidth;
	out = (uint *)scaledImage;

	frac = fracStep >> 2;
	for( i = 0; i < outWidth; i++ )
	{
		p1[i] = 4 * (frac >> 16);
		frac += fracStep;
	}

	frac = (fracStep >> 2) * 3;
	for( i = 0; i < outWidth; i++ )
	{
		p2[i] = 4 * (frac >> 16);
		frac += fracStep;
	}

	if( isNormalMap )
	{
		for( y = 0; y < outHeight; y++, out += outWidth )
		{
			inRow1 = in + inWidth * (int)(((float)y + 0.25f) * inHeight/outHeight);
			inRow2 = in + inWidth * (int)(((float)y + 0.75f) * inHeight/outHeight);

			for( x = 0; x < outWidth; x++ )
			{
				pix1 = (byte *)inRow1 + p1[x];
				pix2 = (byte *)inRow1 + p2[x];
				pix3 = (byte *)inRow2 + p1[x];
				pix4 = (byte *)inRow2 + p2[x];

				normal[0] = (pix1[0] * (1.0f/127.0f) - 1.0f) + (pix2[0] * (1.0f/127.0f) - 1.0f) + (pix3[0] * (1.0f/127.0f) - 1.0f) + (pix4[0] * (1.0f/127.0f) - 1.0f);
				normal[1] = (pix1[1] * (1.0f/127.0f) - 1.0f) + (pix2[1] * (1.0f/127.0f) - 1.0f) + (pix3[1] * (1.0f/127.0f) - 1.0f) + (pix4[1] * (1.0f/127.0f) - 1.0f);
				normal[2] = (pix1[2] * (1.0f/127.0f) - 1.0f) + (pix2[2] * (1.0f/127.0f) - 1.0f) + (pix3[2] * (1.0f/127.0f) - 1.0f) + (pix4[2] * (1.0f/127.0f) - 1.0f);

				if( !VectorNormalizeLength( normal ))
					VectorSet( normal, 0.0f, 0.0f, 1.0f );

				((byte *)(out+x))[0] = (byte)(128 + 127 * normal[0]);
				((byte *)(out+x))[1] = (byte)(128 + 127 * normal[1]);
				((byte *)(out+x))[2] = (byte)(128 + 127 * normal[2]);
				((byte *)(out+x))[3] = 255;
			}
		}
	}
	else
	{
		for( y = 0; y < outHeight; y++, out += outWidth )
		{
			inRow1 = in + inWidth * (int)(((float)y + 0.25f) * inHeight/outHeight);
			inRow2 = in + inWidth * (int)(((float)y + 0.75f) * inHeight/outHeight);

			for( x = 0; x < outWidth; x++ )
			{
				pix1 = (byte *)inRow1 + p1[x];
				pix2 = (byte *)inRow1 + p2[x];
				pix3 = (byte *)inRow2 + p1[x];
				pix4 = (byte *)inRow2 + p2[x];

				((byte *)(out+x))[0] = (pix1[0] + pix2[0] + pix3[0] + pix4[0]) >> 2;
				((byte *)(out+x))[1] = (pix1[1] + pix2[1] + pix3[1] + pix4[1]) >> 2;
				((byte *)(out+x))[2] = (pix1[2] + pix2[2] + pix3[2] + pix4[2]) >> 2;
				((byte *)(out+x))[3] = (pix1[3] + pix2[3] + pix3[3] + pix4[3]) >> 2;
			}
		}
	}
	return scaledImage;
}

/*
=================
GL_ApplyGamma

Assume input buffer is RGBA
=================
*/
byte *GL_ApplyGamma( const byte *source, int pixels, qboolean isNormalMap )
{
	byte	*in = (byte *)source;
	byte	*out = (byte *)source;
	int	i;

	if( !isNormalMap )
	{
		for( i = 0; i < pixels; i++, in += 4 )
		{
			in[0] = TextureToGamma( in[0] );
			in[1] = TextureToGamma( in[1] );
			in[2] = TextureToGamma( in[2] );
		}
	}
	return out;
}

/*
=================
GL_BuildMipMap

Operates in place, quartering the size of the texture
=================
*/
static void GL_BuildMipMap( byte *in, int width, int height, qboolean isNormalMap )
{
	byte	*out = in;
	vec3_t	normal;
	int	x, y;

	width <<= 2;
	height >>= 1;

	if( isNormalMap )
	{
		for( y = 0; y < height; y++, in += width )
		{
			for( x = 0; x < width; x += 8, in += 8, out += 4 )
			{
				normal[0] = (in[0] * (1.0f/127.0f) - 1.0f) + (in[4] * (1.0f/127.0f) - 1.0f) + (in[width+0] * (1.0f/127.0f) - 1.0f) + (in[width+4] * (1.0f/127.0f) - 1.0f);
				normal[1] = (in[1] * (1.0f/127.0f) - 1.0f) + (in[5] * (1.0f/127.0f) - 1.0f) + (in[width+1] * (1.0f/127.0f) - 1.0f) + (in[width+5] * (1.0f/127.0f) - 1.0f);
				normal[2] = (in[2] * (1.0f/127.0f) - 1.0f) + (in[6] * (1.0f/127.0f) - 1.0f) + (in[width+2] * (1.0f/127.0f) - 1.0f) + (in[width+6] * (1.0f/127.0f) - 1.0f);

				if( !VectorNormalizeLength( normal ))
					VectorSet( normal, 0.0f, 0.0f, 1.0f );

				out[0] = (byte)(128 + 127 * normal[0]);
				out[1] = (byte)(128 + 127 * normal[1]);
				out[2] = (byte)(128 + 127 * normal[2]);
				out[3] = 255;
			}
		}
	}
	else
	{
		for( y = 0; y < height; y++, in += width )
		{
			for( x = 0; x < width; x += 8, in += 8, out += 4 )
			{
				out[0] = (in[0] + in[4] + in[width+0] + in[width+4]) >> 2;
				out[1] = (in[1] + in[5] + in[width+1] + in[width+5]) >> 2;
				out[2] = (in[2] + in[6] + in[width+2] + in[width+6]) >> 2;
				out[3] = (in[3] + in[7] + in[width+3] + in[width+7]) >> 2;
			}
		}
	}
}

/*
===============
GL_GenerateMipmaps

sgis generate mipmap
===============
*/
void GL_GenerateMipmaps( byte *buffer, rgbdata_t *pic, gltexture_t *tex, GLenum glTarget, GLenum inFormat, int side, qboolean subImage )
{
	int	mipLevel;
	int	dataType = GL_UNSIGNED_BYTE;
	int	w, h;

	// not needs
	if( tex->flags & TF_NOMIPMAP )
		return;

	if( GL_Support( GL_SGIS_MIPMAPS_EXT ) && !( tex->flags & ( TF_NORMALMAP|TF_ALPHACONTRAST )))
	{
		pglHint( GL_GENERATE_MIPMAP_HINT_SGIS, GL_NICEST );
		pglTexParameteri( glTarget, GL_GENERATE_MIPMAP_SGIS, GL_TRUE );
		pglGetError(); // clear error queue on mips generate
		return; 
	}

	// screen texture?
	if( !buffer ) return;

	mipLevel = 0;
	w = tex->width;
	h = tex->height;

	// software mipmap generator
	while( w > 1 || h > 1 )
	{
		// build the mipmap
		if( tex->flags & TF_ALPHACONTRAST ) Q_memset( buffer, pic->width >> mipLevel, w * h * 4 );
		else GL_BuildMipMap( buffer, w, h, ( tex->flags & TF_NORMALMAP ));

		w = (w+1)>>1;
		h = (h+1)>>1;
		mipLevel++;

		if( subImage ) pglTexSubImage2D( tex->target + side, mipLevel, 0, 0, w, h, inFormat, dataType, buffer );
		else pglTexImage2D( tex->target + side, mipLevel, tex->format, w, h, 0, inFormat, dataType, buffer );
		if( pglGetError( )) break; // can't create mip levels
	}
}

/*
=================
GL_MakeLuminance

Converts the given image to luminance
=================
*/
void GL_MakeLuminance( rgbdata_t *in )
{
	byte	luminance;
	float	r, g, b;
	int	x, y;

	for( y = 0; y < in->height; y++ )
	{
		for( x = 0; x < in->width; x++ )
		{
			r = r_luminanceTable[in->buffer[4*(y*in->width+x)+0]][0];
			g = r_luminanceTable[in->buffer[4*(y*in->width+x)+1]][1];
			b = r_luminanceTable[in->buffer[4*(y*in->width+x)+2]][2];

			luminance = (byte)(r + g + b);

			in->buffer[4*(y*in->width+x)+0] = luminance;
			in->buffer[4*(y*in->width+x)+1] = luminance;
			in->buffer[4*(y*in->width+x)+2] = luminance;
		}
	}
}

/*
===============
GL_UploadTexture

upload texture into video memory
===============
*/
static void GL_UploadTexture( rgbdata_t *pic, gltexture_t *tex, qboolean subImage, imgfilter_t *filter )
{
	byte		*buf, *data;
	const byte	*bufend;
	GLenum		outFormat, inFormat, glTarget;
	uint		i, s, numSides, offset = 0, err;
	int		texsize = 0, img_flags = 0, samples;
	GLint		dataType = GL_UNSIGNED_BYTE;

	ASSERT( pic != NULL && tex != NULL );

	tex->srcWidth = tex->width = pic->width;
	tex->srcHeight = tex->height = pic->height;
	s = tex->srcWidth * tex->srcHeight;

	tex->fogParams[0] = pic->fogParams[0];
	tex->fogParams[1] = pic->fogParams[1];
	tex->fogParams[2] = pic->fogParams[2];
	tex->fogParams[3] = pic->fogParams[3];

	// NOTE: normalmaps must be power of two or software mip generator will stop working
	GL_RoundImageDimensions( &tex->width, &tex->height, tex->flags, ( tex->flags & TF_NORMALMAP ));

	if( s&3 )
	{
		// will be resample, just tell me for debug targets
		MsgDev( D_NOTE, "GL_Upload: %s s&3 [%d x %d]\n", tex->name, tex->srcWidth, tex->srcHeight );
	}
			
	// copy flag about luma pixels
	if( pic->flags & IMAGE_HAS_LUMA )
		tex->flags |= TF_HAS_LUMA;

	// create luma texture from quake texture
	if( tex->flags & TF_MAKELUMA )
	{
		img_flags |= IMAGE_MAKE_LUMA;
		tex->flags &= ~TF_MAKELUMA;
	}

	if( !subImage && tex->flags & TF_KEEP_8BIT )
		tex->original = FS_CopyImage( pic ); // because current pic will be expanded to rgba

	if( !subImage && tex->flags & TF_KEEP_RGBDATA )
		tex->original = pic; // no need to copy

	// we need to expand image into RGBA buffer
	if( pic->type == PF_INDEXED_24 || pic->type == PF_INDEXED_32 )
		img_flags |= IMAGE_FORCE_RGBA;

	// processing image before uploading (force to rgba, make luma etc)
	if( pic->buffer ) Image_Process( &pic, 0, 0, 0.0f, img_flags, filter );

	if( tex->flags & TF_LUMINANCE )
	{
		if( !( tex->flags & TF_DEPTHMAP ))
		{
			GL_MakeLuminance( pic );
			tex->flags &= ~TF_LUMINANCE;
		}
		pic->flags &= ~IMAGE_HAS_COLOR;
	}

	samples = GL_CalcTextureSamples( pic->flags );

	if( pic->flags & IMAGE_HAS_ALPHA )
		tex->flags |= TF_HAS_ALPHA;

	// determine format
	inFormat = PFDesc[pic->type].glFormat;
	outFormat = GL_TextureFormat( tex, &samples );
	tex->format = outFormat;

	// determine target
	tex->target = glTarget = GL_TEXTURE_2D;
	numSides = 1;

	if( tex->flags & TF_DEPTHMAP )
	{
		inFormat = GL_DEPTH_COMPONENT;
		dataType = GL_FLOAT;
	}

	if( pic->flags & IMAGE_CUBEMAP )
	{
		if( GL_Support( GL_TEXTURECUBEMAP_EXT ))
		{		
			numSides = 6;
			tex->target = glTarget = GL_TEXTURE_CUBE_MAP_ARB;
			tex->flags |= TF_CUBEMAP;

			if( tex->flags & ( TF_BORDER|TF_ALPHA_BORDER ))
			{
				// don't use border for cubemaps
				tex->flags &= ~(TF_BORDER|TF_ALPHA_BORDER);
				tex->flags |= TF_CLAMP;
			}
		}
		else
		{
			MsgDev( D_WARN, "GL_UploadTexture: cubemaps isn't supported, %s ignored\n", tex->name );
			tex->flags &= ~TF_CUBEMAP;
		}
	}
	else if( tex->flags & TF_TEXTURE_1D )
	{
		// determine target
		tex->target = glTarget = GL_TEXTURE_1D;
	}
	else if( tex->flags & TF_TEXTURE_RECTANGLE )
	{
		if( glConfig.max_2d_rectangle_size )
			tex->target = glTarget = glConfig.texRectangle;
		// or leave as GL_TEXTURE_2D
	}
	else if( tex->flags & TF_TEXTURE_3D )
	{
		// determine target
		tex->target = glTarget = GL_TEXTURE_3D;
	}

	pglBindTexture( tex->target, tex->texnum );

	buf = pic->buffer;
	bufend = pic->buffer + pic->size;
	offset = pic->width * pic->height * PFDesc[pic->type].bpp;

	// NOTE: probably this code relies when gl_compressed_textures is enabled
	texsize = tex->width * tex->height * samples;

	// determine some texTypes
	if( tex->flags & TF_NOPICMIP )
		tex->texType = TEX_NOMIP;
	else if( tex->flags & TF_CUBEMAP )
		tex->texType = TEX_CUBEMAP;
	else if(( tex->flags & TF_DECAL ) == TF_DECAL )
		tex->texType = TEX_DECAL;

	// uploading texture into video memory
	for( i = 0; i < numSides; i++ )
	{
		if( buf != NULL && buf >= bufend )
			Host_Error( "GL_UploadTexture: %s image buffer overflow\n", tex->name );

		// copy or resample the texture
		if(( tex->width == tex->srcWidth && tex->height == tex->srcHeight ) || ( tex->flags & ( TF_TEXTURE_1D|TF_TEXTURE_3D )))
		{
			data = buf;
		}
		else
		{
			data = GL_ResampleTexture( buf, tex->srcWidth, tex->srcHeight, tex->width, tex->height, ( tex->flags & TF_NORMALMAP ));
		}

		if( !glConfig.deviceSupportsGamma )
		{
			if(!( tex->flags & TF_NOMIPMAP ) && !( tex->flags & TF_SKYSIDE ) && !( tex->flags & TF_TEXTURE_3D ))
				data = GL_ApplyGamma( data, tex->width * tex->height, ( tex->flags & TF_NORMALMAP ));
		}		

		if( glTarget == GL_TEXTURE_1D )
		{
			if( subImage ) pglTexSubImage1D( tex->target, 0, 0, tex->width, inFormat, dataType, data );
			else pglTexImage1D( tex->target, 0, outFormat, tex->width, 0, inFormat, dataType, data );
		}
		else if( glTarget == GL_TEXTURE_CUBE_MAP_ARB )
		{
			if( GL_Support( GL_SGIS_MIPMAPS_EXT ) && !( tex->flags & TF_NORMALMAP ))
				GL_GenerateMipmaps( data, pic, tex, glTarget, inFormat, i, subImage );
			if( subImage ) pglTexSubImage2D( GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB + i, 0, 0, 0, tex->width, tex->height, inFormat, dataType, data );
			else pglTexImage2D( GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB + i, 0, outFormat, tex->width, tex->height, 0, inFormat, dataType, data );
			if( !GL_Support( GL_SGIS_MIPMAPS_EXT ) || ( tex->flags & TF_NORMALMAP ))
				GL_GenerateMipmaps( data, pic, tex, glTarget, inFormat, i, subImage );
		}
		else if( glTarget == GL_TEXTURE_3D )
		{
			if( subImage ) pglTexSubImage3D( tex->target, 0, 0, 0, 0, tex->width, tex->height, pic->depth, inFormat, dataType, data );
			else pglTexImage3D( tex->target, 0, outFormat, tex->width, tex->height, pic->depth, 0, inFormat, dataType, data );
		}
		else
		{
			if( GL_Support( GL_SGIS_MIPMAPS_EXT ) && !( tex->flags & ( TF_NORMALMAP|TF_ALPHACONTRAST )))
				GL_GenerateMipmaps( data, pic, tex, glTarget, inFormat, i, subImage );
			if( subImage ) pglTexSubImage2D( tex->target, 0, 0, 0, tex->width, tex->height, inFormat, dataType, data );
			else pglTexImage2D( tex->target, 0, outFormat, tex->width, tex->height, 0, inFormat, dataType, data );
			if( !GL_Support( GL_SGIS_MIPMAPS_EXT ) || ( tex->flags & ( TF_NORMALMAP|TF_ALPHACONTRAST )))
				GL_GenerateMipmaps( data, pic, tex, glTarget, inFormat, i, subImage );
		}

		if( numSides > 1 && buf != NULL )
			buf += offset;
		tex->size += texsize;

		// catch possible errors
		err = pglGetError();

		if( err != GL_NO_ERROR )
			MsgDev( D_ERROR, "GL_UploadTexture: error %x while uploading %s [%s]\n", err, tex->name, GL_Target( glTarget ));
	}
}

/*
================
GL_LoadTexture
================
*/
int GL_LoadTexture( const char *name, const byte *buf, size_t size, int flags, imgfilter_t *filter )
{
	gltexture_t	*tex;
	rgbdata_t		*pic;
	uint		i, hash;
	uint		picFlags = 0;

	if( !name || !name[0] || !glw_state.initialized )
		return 0;

	if( Q_strlen( name ) >= sizeof( r_textures->name ))
	{
		MsgDev( D_ERROR, "GL_LoadTexture: too long name %s\n", name );
		return 0;
	}

	// see if already loaded
	hash = Com_HashKey( name, TEXTURES_HASH_SIZE );

	for( tex = r_texturesHashTable[hash]; tex != NULL; tex = tex->nextHash )
	{
		if( !Q_stricmp( tex->name, name ))
		{
			// prolonge registration
			tex->cacheframe = world.load_sequence;
			return (tex - r_textures);
		}
	}

	if( flags & TF_NOFLIP_TGA )
		picFlags |= IL_DONTFLIP_TGA;

	if( flags & TF_KEEP_8BIT )
		picFlags |= IL_KEEP_8BIT;	

	// set some image flags
	Image_SetForceFlags( picFlags );

	if( flags & TF_IMAGE_PROGRAM )
	{
		char	buffer[256], token[256];
		char	*script;
		int	samples;

		// create quoted string on a spec symbols
		if( name[0] == '#' || name[0] == '{' )
			Q_snprintf( buffer, sizeof( buffer ), "\"%s\"", name );
		else Q_strncpy( buffer, name, sizeof( buffer ));
		script = &buffer[0];

		if(( script = COM_ParseFile( script, token )) == NULL )
			return 0;

		// parse image program
		pic = R_LoadImage( &script, token, buf, size, &samples, &flags );
		if( !pic ) return 0; // couldn't loading image

		// recalc image samples here
		pic->flags &= ~(IMAGE_HAS_COLOR|IMAGE_HAS_ALPHA);
		pic->flags |= GL_ImageFlagsFromSamples( samples );
	}
	else
	{
		// HACKHACK: get rid of black vertical line on a 'BlackMesa map'
		if( !Q_strcmp( name, "#lab1_map1.mip" ) || !Q_strcmp( name, "#lab1_map2.mip" ))
			flags |= TF_NEAREST;

		pic = FS_LoadImage( name, buf, size );
		if( !pic ) return 0; // couldn't loading image
	}

	// force upload texture as RGB or RGBA (detail textures requires this)
	if( flags & TF_FORCE_COLOR ) pic->flags |= IMAGE_HAS_COLOR;

	// find a free texture slot
	if( r_numTextures == MAX_TEXTURES )
		Host_Error( "GL_LoadTexture: MAX_TEXTURES limit exceeds\n" );

	// find a free texture_t slot
	for( i = 0, tex = r_textures; i < r_numTextures; i++, tex++ )
		if( !tex->name[0] ) break;

	if( i == r_numTextures )
	{
		if( r_numTextures == MAX_TEXTURES )
			Host_Error( "GL_LoadTexture: MAX_TEXTURES limit exceeds\n" );
		r_numTextures++;
	}

	tex = &r_textures[i];
	Q_strncpy( tex->name, name, sizeof( tex->name ));
	tex->flags = flags;

	if( flags & TF_SKYSIDE )
		tex->texnum = tr.skyboxbasenum++;
	else tex->texnum = i; // texnum is used for fast acess into r_textures array too

	GL_UploadTexture( pic, tex, false, filter );
	GL_TexFilter( tex, false ); // update texture filter, wrap etc

	if(!( flags & ( TF_KEEP_8BIT|TF_KEEP_RGBDATA )))
		FS_FreeImage( pic ); // release source texture

	// add to hash table
	hash = Com_HashKey( tex->name, TEXTURES_HASH_SIZE );
	tex->nextHash = r_texturesHashTable[hash];
	r_texturesHashTable[hash] = tex;

	// NOTE: always return texnum as index in array or engine will stop work !!!
	return i;
}

/*
================
GL_LoadTextureInternal
================
*/
int GL_LoadTextureInternal( const char *name, rgbdata_t *pic, texFlags_t flags, qboolean update )
{
	gltexture_t	*tex;
	uint		i, hash;

	if( !name || !name[0] || !glw_state.initialized )
		return 0;

	if( Q_strlen( name ) >= sizeof( r_textures->name ))
	{
		MsgDev( D_ERROR, "GL_LoadTexture: too long name %s\n", name );
		return 0;
	}

	// see if already loaded
	hash = Com_HashKey( name, TEXTURES_HASH_SIZE );

	for( tex = r_texturesHashTable[hash]; tex != NULL; tex = tex->nextHash )
	{
		if( !Q_stricmp( tex->name, name ))
		{
			// prolonge registration
			tex->cacheframe = world.load_sequence;
			if( update ) break;
			return (tex - r_textures);
		}
	}

	if( !pic ) return 0; // couldn't loading image
	if( update && !tex )
	{
		Host_Error( "Couldn't find texture %s for update\n", name );
	}

	// force upload texture as RGB or RGBA (detail textures requires this)
	if( flags & TF_FORCE_COLOR ) pic->flags |= IMAGE_HAS_COLOR;

	// find a free texture slot
	if( r_numTextures == MAX_TEXTURES )
		Host_Error( "GL_LoadTexture: MAX_TEXTURES limit exceeds\n" );

	if( !update )
	{
		// find a free texture_t slot
		for( i = 0, tex = r_textures; i < r_numTextures; i++, tex++ )
			if( !tex->name[0] ) break;

		if( i == r_numTextures )
		{
			if( r_numTextures == MAX_TEXTURES )
				Host_Error( "GL_LoadTexture: MAX_TEXTURES limit exceeds\n" );
			r_numTextures++;
		}

		tex = &r_textures[i];
		hash = Com_HashKey( name, TEXTURES_HASH_SIZE );
		Q_strncpy( tex->name, name, sizeof( tex->name ));
		tex->texnum = i;	// texnum is used for fast acess into r_textures array too
		tex->flags = flags;
	}
	else
	{
		tex->flags |= flags;
	}

	GL_UploadTexture( pic, tex, update, NULL );
	GL_TexFilter( tex, update ); // update texture filter, wrap etc

	if( !update )
          {
		// add to hash table
		hash = Com_HashKey( tex->name, TEXTURES_HASH_SIZE );
		tex->nextHash = r_texturesHashTable[hash];
		r_texturesHashTable[hash] = tex;
	}

	return (tex - r_textures);
}

/*
================
GL_CreateTexture

creates an empty 32-bit texture (just reserve slot)
================
*/
int GL_CreateTexture( const char *name, int width, int height, const void *buffer, texFlags_t flags )
{
	rgbdata_t	r_empty;
	int	texture;

	Q_memset( &r_empty, 0, sizeof( r_empty ));
	r_empty.width = width;
	r_empty.height = height;
	r_empty.type = PF_RGBA_32;
	r_empty.size = r_empty.width * r_empty.height * 4;
	r_empty.flags = IMAGE_HAS_COLOR | (( flags & TF_HAS_ALPHA ) ? IMAGE_HAS_ALPHA : 0 );
	r_empty.buffer = (byte *)buffer;

	if( flags & TF_TEXTURE_1D )
	{
		r_empty.height = 1;
		r_empty.size = r_empty.width * 4;
	}
	else if( flags & TF_TEXTURE_3D )
	{
		if( !GL_Support( GL_TEXTURE_3D_EXT ))
			return 0;

		r_empty.depth = r_empty.width;
		r_empty.size = r_empty.width * r_empty.height * r_empty.depth * 4;
	}
	else if( flags & TF_CUBEMAP )
	{
		flags &= ~TF_CUBEMAP; // will be set later
		r_empty.flags |= IMAGE_CUBEMAP;
		r_empty.size *= 6;
	}

	texture = GL_LoadTextureInternal( name, &r_empty, flags, false );

	if( flags & TF_DEPTHMAP )
		GL_SetTextureType( texture, TEX_DEPTHMAP );
	else GL_SetTextureType( texture, TEX_CUSTOM );

	return texture;
}

/*
================
GL_ProcessTexture
================
*/
void GL_ProcessTexture( int texnum, float gamma, int topColor, int bottomColor )
{
	gltexture_t	*image;
	rgbdata_t		*pic;
	int		flags = 0;

	if( texnum <= 0 ) return; // missed image
	ASSERT( texnum > 0 && texnum < MAX_TEXTURES );
	image = &r_textures[texnum];

	// select mode
	if( gamma != -1.0f )
	{
		flags = IMAGE_LIGHTGAMMA;
	}
	else if( topColor != -1 && bottomColor != -1 )
	{
		flags = IMAGE_REMAP;
	}
	else
	{
		MsgDev( D_ERROR, "GL_ProcessTexture: bad operation for %s\n", image->name );
		return;
	}

	if(!( image->flags & (TF_KEEP_RGBDATA|TF_KEEP_8BIT)) || !image->original )
	{
		MsgDev( D_ERROR, "GL_ProcessTexture: no input data for %s\n", image->name );
		return;
	}

	// all the operations makes over the image copy not an original
	pic = FS_CopyImage( image->original );
	Image_Process( &pic, topColor, bottomColor, gamma, flags, NULL );

	GL_UploadTexture( pic, image, true, NULL );
	GL_TexFilter( image, true ); // update texture filter, wrap etc

	FS_FreeImage( pic );
}

/*
================
GL_LoadTexture
================
*/
int GL_FindTexture( const char *name )
{
	gltexture_t	*tex;
	uint		hash;

	if( !name || !name[0] || !glw_state.initialized )
		return 0;

	if( Q_strlen( name ) >= sizeof( r_textures->name ))
	{
		MsgDev( D_ERROR, "GL_FindTexture: too long name %s\n", name );
		return 0;
	}

	// see if already loaded
	hash = Com_HashKey( name, TEXTURES_HASH_SIZE );

	for( tex = r_texturesHashTable[hash]; tex != NULL; tex = tex->nextHash )
	{
		if( !Q_stricmp( tex->name, name ))
		{
			// prolonge registration
			tex->cacheframe = world.load_sequence;
			return (tex - r_textures);
		}
	}

	return 0;
}

/*
================
GL_FreeImage

Frees image by name
================
*/
void GL_FreeImage( const char *name )
{
	gltexture_t	*tex;
	uint		hash;

	if( !name || !name[0] || !glw_state.initialized )
		return;

	if( Q_strlen( name ) >= sizeof( r_textures->name ))
	{
		MsgDev( D_ERROR, "GL_FreeImage: too long name %s\n", name, sizeof( r_textures->name ));
		return;
	}

	// see if already loaded
	hash = Com_HashKey( name, TEXTURES_HASH_SIZE );

	for( tex = r_texturesHashTable[hash]; tex != NULL; tex = tex->nextHash )
	{
		if( !Q_stricmp( tex->name, name ))
		{
			R_FreeImage( tex );
			return;
		}
	}
}

/*
================
GL_FreeTexture
================
*/
void GL_FreeTexture( GLenum texnum )
{
	// number 0 it's already freed
	if( texnum <= 0 || !glw_state.initialized )
		return;

	ASSERT( texnum > 0 && texnum < MAX_TEXTURES );
	R_FreeImage( &r_textures[texnum] );
}

/*
=======================================================================

 IMAGE PROGRAM FUNCTIONS

=======================================================================
*/
/*
=================
R_ForceImageToRGBA

Unpack any image to RGBA buffer
=================
*/
_inline static rgbdata_t *R_ForceImageToRGBA( rgbdata_t *pic )
{
	// don't need additional checks - image lib do it himself
	Image_Process( &pic, 0, 0, 0, IMAGE_FORCE_RGBA, NULL );

	return pic;
}

/*
=================
R_AddImages

Adds the given images together
=================
*/
static rgbdata_t *R_AddImages( rgbdata_t *in1, rgbdata_t *in2 )
{
	rgbdata_t	*out;
	int	width, height;
	int	r, g, b, a;
	int	x, y;

	// make sure what we processing RGBA images
	in1 = R_ForceImageToRGBA( in1 );
	in2 = R_ForceImageToRGBA( in2 );
	width = in1->width, height = in1->height;
	out = in1;

	for( y = 0; y < height; y++ )
	{
		for( x = 0; x < width; x++ )
		{
			r = in1->buffer[4*(y*width+x)+0] + in2->buffer[4*(y*width+x)+0];
			g = in1->buffer[4*(y*width+x)+1] + in2->buffer[4*(y*width+x)+1];
			b = in1->buffer[4*(y*width+x)+2] + in2->buffer[4*(y*width+x)+2];
			a = in1->buffer[4*(y*width+x)+3] + in2->buffer[4*(y*width+x)+3];
			out->buffer[4*(y*width+x)+0] = bound( 0, r, 255 );
			out->buffer[4*(y*width+x)+1] = bound( 0, g, 255 );
			out->buffer[4*(y*width+x)+2] = bound( 0, b, 255 );
			out->buffer[4*(y*width+x)+3] = bound( 0, a, 255 );
		}
	}

	FS_FreeImage( in2 );

	return out;
}

/*
=================
R_MultiplyImages

Multiplies the given images
=================
*/
static rgbdata_t *R_MultiplyImages( rgbdata_t *in1, rgbdata_t *in2 )
{
	rgbdata_t	*out;
	int	width, height;
	int	r, g, b, a;
	int	x, y;

	// make sure what we processing RGBA images
	in1 = R_ForceImageToRGBA( in1 );
	in2 = R_ForceImageToRGBA( in2 );
	width = in1->width, height = in1->height;
	out = in1;

	for( y = 0; y < height; y++ )
	{
		for( x = 0; x < width; x++ )
		{
			r = in1->buffer[4*(y*width+x)+0] * (in2->buffer[4*(y*width+x)+0] * (1.0f/255));
			g = in1->buffer[4*(y*width+x)+1] * (in2->buffer[4*(y*width+x)+1] * (1.0f/255));
			b = in1->buffer[4*(y*width+x)+2] * (in2->buffer[4*(y*width+x)+2] * (1.0f/255));
			a = in1->buffer[4*(y*width+x)+3] * (in2->buffer[4*(y*width+x)+3] * (1.0f/255));
			out->buffer[4*(y*width+x)+0] = bound( 0, r, 255 );
			out->buffer[4*(y*width+x)+1] = bound( 0, g, 255 );
			out->buffer[4*(y*width+x)+2] = bound( 0, b, 255 );
			out->buffer[4*(y*width+x)+3] = bound( 0, a, 255 );
		}
	}
	FS_FreeImage( in2 );

	return out;
}

/*
=================
R_BiasImage

Biases the given image
=================
*/
static rgbdata_t *R_BiasImage( rgbdata_t *in, const vec4_t bias )
{
	rgbdata_t	*out;
	int	width, height;
	int	r, g, b, a;
	int	x, y;

	// make sure what we processing RGBA image
	in = R_ForceImageToRGBA( in );
	width = in->width, height = in->height;
	out = in;

	for( y = 0; y < height; y++ )
	{
		for( x = 0; x < width; x++ )
		{
			r = in->buffer[4*(y*width+x)+0] + (255 * bias[0]);
			g = in->buffer[4*(y*width+x)+1] + (255 * bias[1]);
			b = in->buffer[4*(y*width+x)+2] + (255 * bias[2]);
			a = in->buffer[4*(y*width+x)+3] + (255 * bias[3]);
			out->buffer[4*(y*width+x)+0] = bound( 0, r, 255 );
			out->buffer[4*(y*width+x)+1] = bound( 0, g, 255 );
			out->buffer[4*(y*width+x)+2] = bound( 0, b, 255 );
			out->buffer[4*(y*width+x)+3] = bound( 0, a, 255 );
		}
	}

	return out;
}

/*
=================
R_ScaleImage

Scales the given image
=================
*/
static rgbdata_t *R_ScaleImage( rgbdata_t *in, const vec4_t scale )
{
	rgbdata_t	*out;
	int	width, height;
	int	r, g, b, a;
	int	x, y;

	// make sure what we processing RGBA image
	in = R_ForceImageToRGBA( in );
	width = in->width, height = in->height;
	out = in;

	for( y = 0; y < height; y++ )
	{
		for( x = 0; x < width; x++ )
		{
			r = in->buffer[4*(y*width+x)+0] * scale[0];
			g = in->buffer[4*(y*width+x)+1] * scale[1];
			b = in->buffer[4*(y*width+x)+2] * scale[2];
			a = in->buffer[4*(y*width+x)+3] * scale[3];
			out->buffer[4*(y*width+x)+0] = bound( 0, r, 255 );
			out->buffer[4*(y*width+x)+1] = bound( 0, g, 255 );
			out->buffer[4*(y*width+x)+2] = bound( 0, b, 255 );
			out->buffer[4*(y*width+x)+3] = bound( 0, a, 255 );
		}
	}

	return out;
}

/*
=================
R_InvertColor

Inverts the color channels of the given image
=================
*/
static rgbdata_t *R_InvertColor( rgbdata_t *in )
{
	rgbdata_t	*out;
	int	width, height;
	int	x, y;

	// make sure what we processing RGBA image
	in = R_ForceImageToRGBA( in );
	width = in->width, height = in->height;
	out = in;

	for( y = 0; y < height; y++ )
	{
		for( x = 0; x < width; x++ )
		{
			out->buffer[4*(y*width+x)+0] = 255 - in->buffer[4*(y*width+x)+0];
			out->buffer[4*(y*width+x)+1] = 255 - in->buffer[4*(y*width+x)+1];
			out->buffer[4*(y*width+x)+2] = 255 - in->buffer[4*(y*width+x)+2];
		}
	}

	return out;
}

/*
=================
R_InvertAlpha

Inverts the alpha channel of the given image
=================
*/
static rgbdata_t *R_InvertAlpha( rgbdata_t *in )
{
	rgbdata_t	*out;
	int	width, height;
	int	x, y;

	// make sure what we processing RGBA image
	in = R_ForceImageToRGBA( in );
	width = in->width, height = in->height;
	out = in;

	for( y = 0; y < height; y++ )
	{
		for( x = 0; x < width; x++ )
			out->buffer[4*(y*width+x)+3] = 255 - in->buffer[4*(y*width+x)+3];
	}

	return out;
}

/*
=================
R_MakeIntensity

Converts the given image to intensity
=================
*/
static rgbdata_t *R_MakeIntensity( rgbdata_t *in )
{
	rgbdata_t	*out;
	int	width, height;
	byte	intensity;
	float	r, g, b;
	int	x, y;

	// make sure what we processing RGBA image
	in = R_ForceImageToRGBA( in );
	width = in->width, height = in->height;
	out = in;

	for( y = 0; y < height; y++ )
	{
		for( x = 0; x < width; x++ )
		{
			r = r_luminanceTable[in->buffer[4*(y*width+x)+0]][0];
			g = r_luminanceTable[in->buffer[4*(y*width+x)+1]][1];
			b = r_luminanceTable[in->buffer[4*(y*width+x)+2]][2];

			intensity = (byte)(r + g + b);

			out->buffer[4*(y*width+x)+0] = intensity;
			out->buffer[4*(y*width+x)+1] = intensity;
			out->buffer[4*(y*width+x)+2] = intensity;
			out->buffer[4*(y*width+x)+3] = intensity;
		}
	}

	return out;
}

/*
=================
R_MakeLuminance

Converts the given image to luminance
=================
*/
static rgbdata_t *R_MakeLuminance( rgbdata_t *in )
{
	rgbdata_t	*out;
	int	width, height;
	byte	luminance;
	float	r, g, b;
	int	x, y;

	// make sure what we processing RGBA image
	in = R_ForceImageToRGBA( in );
	width = in->width, height = in->height;
	out = in;

	for( y = 0; y < height; y++ )
	{
		for( x = 0; x < width; x++ )
		{
			r = r_luminanceTable[in->buffer[4*(y*width+x)+0]][0];
			g = r_luminanceTable[in->buffer[4*(y*width+x)+1]][1];
			b = r_luminanceTable[in->buffer[4*(y*width+x)+2]][2];

			luminance = (byte)(r + g + b);

			out->buffer[4*(y*width+x)+0] = luminance;
			out->buffer[4*(y*width+x)+1] = luminance;
			out->buffer[4*(y*width+x)+2] = luminance;
			out->buffer[4*(y*width+x)+3] = 255;
		}
	}

	return out;
}

/*
=================
R_MakeLuma

Converts the given image to glow (LUMA)
=================
*/
static rgbdata_t *R_MakeLuma( rgbdata_t *in )
{
	Image_Process( &in, 0, 0, 0, IMAGE_MAKE_LUMA|IMAGE_FORCE_RGBA, NULL );

	return in;
}

/*
=================
R_MakeImageBlock

Scissor the specifed rectangle from image
=================
*/
static rgbdata_t *R_MakeImageBlock( rgbdata_t *in, int block[4] )
{
	byte	*fin, *fout, *out;
	int	i, x, y, xl, yl, xh, yh, w, h;
	int	linedelta;

	// make sure what we processing RGBA image
	in = R_ForceImageToRGBA( in );

	xl = block[0];
	yl = block[1];
	w = block[2];
	h = block[3];
	xh = xl + w;
	yh = yl + h;

	out = fout = Mem_Alloc( r_temppool, w * h * 4 );
          
	fin = in->buffer + (yl * in->width + xl) * 4;
	linedelta = (in->width - w) * 4;

	// cut block from source
	for( y = yl; y < yh; y++ )
	{
		for( x = xl; x < xh; x++ )
			for( i = 0; i < 4; i++ )
				*out++ = *fin++;
		fin += linedelta;
	}

	// update image size
	in->width = w, in->height = h;
	in->size = in->width * in->height * 4;

	// copy result back
	in->buffer = Mem_Realloc( host.imagepool, in->buffer, in->size );
	Q_memcpy( in->buffer, fout, in->size );
	Mem_Free( fout ); // purge temp buffer

	return in;
}

/*
=================
R_MakeAlpha

Converts the given image to alpha
=================
*/
static rgbdata_t *R_MakeAlpha( rgbdata_t *in )
{
	rgbdata_t	*out;
	int	width, height;
	byte	alpha;
	float	r, g, b;
	int	x, y;

	// make sure what we processing RGBA image
	in = R_ForceImageToRGBA( in );
	width = in->width, height = in->height;
	out = in;

	for( y = 0; y < height; y++ )
	{
		for( x = 0; x < width; x++ )
		{
			r = r_luminanceTable[in->buffer[4*(y*width+x)+0]][0];
			g = r_luminanceTable[in->buffer[4*(y*width+x)+1]][1];
			b = r_luminanceTable[in->buffer[4*(y*width+x)+2]][2];

			alpha = (byte)(r + g + b);

			out->buffer[4*(y*width+x)+0] = 255;
			out->buffer[4*(y*width+x)+1] = 255;
			out->buffer[4*(y*width+x)+2] = 255;
			out->buffer[4*(y*width+x)+3] = alpha;
		}
	}

	return out;
}

/*
=================
R_HeightMap

Converts the given height map to a normal map
=================
*/
static rgbdata_t *R_HeightMap( rgbdata_t *in, float bumpScale )
{
	byte	*out;
	int	width, height;
	vec3_t	normal;
	float	r, g, b;
	float	c, cx, cy;
	int	x, y;

	// make sure what we processing RGBA image
	in = R_ForceImageToRGBA( in );
	width = in->width, height = in->height;
	out = Mem_Alloc( r_temppool, width * height * 4 );

	if( !bumpScale ) bumpScale = 1.0f;

	for( y = 0; y < height; y++ )
	{
		for( x = 0; x < width; x++ )
		{
			r = r_luminanceTable[in->buffer[4*(y*width+x)+0]][0];
			g = r_luminanceTable[in->buffer[4*(y*width+x)+1]][1];
			b = r_luminanceTable[in->buffer[4*(y*width+x)+2]][2];

			c = (r + g + b) * (1.0f/255);

			r = r_luminanceTable[in->buffer[4*(y*width+((x+1)%width))+0]][0];
			g = r_luminanceTable[in->buffer[4*(y*width+((x+1)%width))+1]][1];
			b = r_luminanceTable[in->buffer[4*(y*width+((x+1)%width))+2]][2];

			cx = (r + g + b) * (1.0f/255);

			r = r_luminanceTable[in->buffer[4*(((y+1)%height)*width+x)+0]][0];
			g = r_luminanceTable[in->buffer[4*(((y+1)%height)*width+x)+1]][1];
			b = r_luminanceTable[in->buffer[4*(((y+1)%height)*width+x)+2]][2];

			cy = (r + g + b) * (1.0f/255);

			normal[0] = (c - cx) * bumpScale;
			normal[1] = (c - cy) * bumpScale;
			normal[2] = 1.0f;

			if( !VectorNormalizeLength( normal ))
				VectorSet( normal, 0.0f, 0.0f, 1.0f );

			out[4*(y*width+x)+0] = (byte)(128 + 127 * normal[0]);
			out[4*(y*width+x)+1] = (byte)(128 + 127 * normal[1]);
			out[4*(y*width+x)+2] = (byte)(128 + 127 * normal[2]);
			out[4*(y*width+x)+3] = 255;
		}
	}

	// copy result back
	Q_memcpy( in->buffer, out, width * height * 4 );
	Mem_Free( out );

	return in;
}

/*
=================
R_AddNormals

Adds the given normal maps together
=================
*/
static rgbdata_t *R_AddNormals( rgbdata_t *in1, rgbdata_t *in2 )
{
	byte	*out;
	int	width, height;
	vec3_t	normal;
	int	x, y;

	// make sure what we processing RGBA images
	in1 = R_ForceImageToRGBA( in1 );
	in2 = R_ForceImageToRGBA( in2 );
	width = in1->width, height = in1->height;
	out = Mem_Alloc( r_temppool, in1->size );

	for( y = 0; y < height; y++ )
	{
		for( x = 0; x < width; x++ )
		{
			normal[0] = (in1->buffer[4*(y*width+x)+0] * (1.0f/127) - 1.0f) + (in2->buffer[4*(y*width+x)+0] * (1.0f/127) - 1.0f);
			normal[1] = (in1->buffer[4*(y*width+x)+1] * (1.0f/127) - 1.0f) + (in2->buffer[4*(y*width+x)+1] * (1.0f/127) - 1.0f);
			normal[2] = (in1->buffer[4*(y*width+x)+2] * (1.0f/127) - 1.0f) + (in2->buffer[4*(y*width+x)+2] * (1.0f/127) - 1.0f);

			if( !VectorNormalizeLength( normal ))
				VectorSet( normal, 0.0f, 0.0f, 1.0f );

			out[4*(y*width+x)+0] = (byte)(128 + 127 * normal[0]);
			out[4*(y*width+x)+1] = (byte)(128 + 127 * normal[1]);
			out[4*(y*width+x)+2] = (byte)(128 + 127 * normal[2]);
			out[4*(y*width+x)+3] = 255;
		}
	}

	// copy result back
	Q_memcpy( in1->buffer, out, width * height * 4 );
	FS_FreeImage( in2 );
	Mem_Free( out );

	return in1;
}

/*
=================
R_SmoothNormals

Smoothes the given normal map
=================
*/
static rgbdata_t *R_SmoothNormals( rgbdata_t *in )
{
	byte	*out;
	int	width, height;
	uint	frac, fracStep;
	uint	p1[0x1000], p2[0x1000];
	byte	*pix1, *pix2, *pix3, *pix4;
	uint	*inRow1, *inRow2;
	vec3_t	normal;
	int	i, x, y;

	// make sure what we processing RGBA image
	in = R_ForceImageToRGBA( in );
	width = in->width, height = in->height;
	out = Mem_Alloc( r_temppool, in->size );

	fracStep = 0x10000;
	frac = fracStep>>2;
	for( i = 0; i < width; i++ )
	{
		p1[i] = 4 * (frac>>16);
		frac += fracStep;
	}

	frac = (fracStep>>2) * 3;
	for( i = 0; i < width; i++ )
	{
		p2[i] = 4 * (frac>>16);
		frac += fracStep;
	}

	for( y = 0; y < height; y++ )
	{
		inRow1 = (uint *)in->buffer + width * (int)((float)y + 0.25f);
		inRow2 = (uint *)in->buffer + width * (int)((float)y + 0.75f);

		for( x = 0; x < width; x++ )
		{
			pix1 = (byte *)inRow1 + p1[x];
			pix2 = (byte *)inRow1 + p2[x];
			pix3 = (byte *)inRow2 + p1[x];
			pix4 = (byte *)inRow2 + p2[x];

			normal[0] = (pix1[0] * (1.0f/127) - 1.0f) + (pix2[0] * (1.0f/127) - 1.0f) + (pix3[0] * (1.0f/127) - 1.0f) + (pix4[0] * (1.0f/127) - 1.0f);
			normal[1] = (pix1[1] * (1.0f/127) - 1.0f) + (pix2[1] * (1.0f/127) - 1.0f) + (pix3[1] * (1.0f/127) - 1.0f) + (pix4[1] * (1.0f/127) - 1.0f);
			normal[2] = (pix1[2] * (1.0f/127) - 1.0f) + (pix2[2] * (1.0f/127) - 1.0f) + (pix3[2] * (1.0f/127) - 1.0f) + (pix4[2] * (1.0f/127) - 1.0f);

			if( !VectorNormalizeLength( normal ))
				VectorSet( normal, 0.0f, 0.0f, 1.0f );

			out[4*(y*width+x)+0] = (byte)(128 + 127 * normal[0]);
			out[4*(y*width+x)+1] = (byte)(128 + 127 * normal[1]);
			out[4*(y*width+x)+2] = (byte)(128 + 127 * normal[2]);
			out[4*(y*width+x)+3] = 255;
		}
	}

	// copy result back
	Q_memcpy( in->buffer, out, width * height * 4 );
	Mem_Free( out );

	return in;
}

/*
================
R_IncludeDepthmap

Write depthmap into alpha-channel the given normal map
================
*/
static rgbdata_t *R_IncludeDepthmap( rgbdata_t *in1, rgbdata_t *in2 )
{
	int	i;
	byte	*pic1, *pic2;

	// make sure what we processing RGBA images
	in1 = R_ForceImageToRGBA( in1 );
	in2 = R_ForceImageToRGBA( in2 );

	pic1 = in1->buffer;
	pic2 = in2->buffer;

	for( i = (in1->width * in1->height) - 1; i > 0; i--, pic1 += 4, pic2 += 4 )
	{
		if( in2->flags & IMAGE_HAS_COLOR )
			pic1[3] = ((int)pic2[0] + (int)pic2[1] + (int)pic2[2]) / 3;
		else if( in2->flags & IMAGE_HAS_ALPHA )
			pic1[3] = pic2[3];
		else pic1[3] = pic2[0];
	}

	in1->flags |= (IMAGE_HAS_COLOR|IMAGE_HAS_ALPHA);
	FS_FreeImage( in2 );

	return in1;
}

/*
================
R_ClearPixels

clear specified area: color or alpha
================
*/
static rgbdata_t *R_ClearPixels( rgbdata_t *in, qboolean clearAlpha )
{
	byte	*pic;
	int	i;

	// make sure what we processing RGBA images
	in = R_ForceImageToRGBA( in );
	pic = in->buffer;

	if( clearAlpha )
	{
		for( i = 0; ( i < in->width * in->height ) && ( in->flags & IMAGE_HAS_ALPHA ); i++ )
			pic[(i<<2)+3] = 0xFF;
	}
	else
	{
		// clear color or greyscale image otherwise
		for( i = 0; i < in->width * in->height; i++ )
			pic[(i<<2)+0] = pic[(i<<2)+1] = pic[(i<<2)+2] = 0xFF;
	}

	return in;
}

/*
================
R_MovePixels

move alpha-channel into color or back
================
*/
static rgbdata_t *R_MovePixels( rgbdata_t *in, qboolean alphaToColor )
{
	byte	*pic;
	int	i;

	// make sure what we processing RGBA images
	in = R_ForceImageToRGBA( in );
	pic = in->buffer;

	if( alphaToColor )
	{
		for( i = 0; ( i < in->width * in->height ) && ( in->flags & IMAGE_HAS_ALPHA ); i++ )
		{
			pic[(i<<2)+0] = pic[(i<<2)+1] = pic[(i<<2)+2] = pic[(i<<2)+3]; // move from alpha to color
			pic[(i<<2)+3] = 0xFF; // clear alpha channel
		}
	}
	else
	{
		// clear color or greyscale image otherwise
		for( i = 0; i < in->width * in->height; i++ )
		{
			// convert to grayscale
			pic[(i<<2)+3] = (pic[(i<<2)+0] * 0.32f) + (pic[(i<<2)+0] * 0.59f) + (pic[(i<<2)+0] * 0.09f);
			pic[(i<<2)+0] = pic[(i<<2)+1] = pic[(i<<2)+2] = 0x00; // clear RGB channels
		}
	}

	return in;
}

/*
==============================================================================

EXTENDED IMAGE INTERFACE

==============================================================================
*/
/*
=================
R_ParseAdd
=================
*/
static rgbdata_t *R_ParseAdd( char **script, int *samples, texFlags_t *flags )
{
	char	token[256];
	rgbdata_t	*pic1, *pic2;
	int	samples1, samples2;

	*script = COM_ParseFile( *script, token );
	if( Q_stricmp( token, "(" ))
	{
		MsgDev( D_WARN, "expected '(', found '%s' instead for 'add'\n", token );
		return NULL;
	}

	if(( *script = COM_ParseFile( *script, token )) == NULL )
	{
		MsgDev( D_WARN, "missing parameters for 'add'\n" );
		return NULL;
	}

	pic1 = R_LoadImage( script, token, NULL, 0, &samples1, flags );
	if( !pic1 ) return NULL;

	*script = COM_ParseFile( *script, token );
	if( Q_stricmp( token, "," ))
	{
		MsgDev( D_WARN, "expected ',', found '%s' instead for 'add'\n", token );
		FS_FreeImage( pic1 );
		return NULL;
	}

	if(( *script = COM_ParseFile( *script, token )) == NULL )
	{
		MsgDev( D_WARN, "missing parameters for 'add'\n" );
		FS_FreeImage( pic1 );
		return NULL;
	}

	pic2 = R_LoadImage( script, token, NULL, 0, &samples2, flags );
	if( !pic2 )
	{
		FS_FreeImage( pic1 );
		return NULL;
	}

	*script = COM_ParseFile( *script, token );
	if( Q_stricmp( token, ")" ))
	{
		MsgDev( D_WARN, "expected ')', found '%s' instead for 'add'\n", token );
		FS_FreeImage( pic1 );
		FS_FreeImage( pic2 );
		return NULL;
	}

	if( pic1->width != pic2->width || pic1->height != pic2->height )
	{
		MsgDev( D_WARN, "images for 'add' have mismatched dimensions [%ix%i] != [%ix%i]\n",
		pic1->width, pic1->height, pic2->width, pic2->height );

		FS_FreeImage( pic1 );
		FS_FreeImage( pic2 );
		return NULL;
	}

	*samples = GL_CalcImageSamples( samples1, samples2 );
	if( *samples != 1 )
	{
		*flags &= ~TF_INTENSITY;
		*flags &= ~TF_LUMINANCE;
	}

	return R_AddImages( pic1, pic2 );
}

/*
=================
R_ParseMultiply
=================
*/
static rgbdata_t *R_ParseMultiply( char **script, int *samples, texFlags_t *flags )
{
	char	token[256];
	rgbdata_t	*pic1, *pic2;
	int	samples1, samples2;

	*script = COM_ParseFile( *script, token );
	if( Q_stricmp( token, "(" ))
	{
		MsgDev( D_WARN, "expected '(', found '%s' instead for 'multiply'\n", token );
		return NULL;
	}

	if(( *script = COM_ParseFile( *script, token )) == NULL )
	{
		MsgDev( D_WARN, "missing parameters for 'multiply'\n" );
		return NULL;
	}

	pic1 = R_LoadImage( script, token, NULL, 0, &samples1, flags );
	if( !pic1 ) return NULL;

	*script = COM_ParseFile( *script, token );
	if( Q_stricmp( token, "," ))
	{
		MsgDev( D_WARN, "expected ',', found '%s' instead for 'multiply'\n", token );
		FS_FreeImage( pic1 );
		return NULL;
	}

	if(( *script = COM_ParseFile( *script, token )) == NULL )
	{
		MsgDev( D_WARN, "missing parameters for 'multiply'\n" );
		FS_FreeImage( pic1 );
		return NULL;
	}

	pic2 = R_LoadImage( script, token, NULL, 0, &samples2, flags );
	if( !pic2 )
	{
		FS_FreeImage( pic1 );
		return NULL;
	}

	*script = COM_ParseFile( *script, token );
	if( Q_stricmp( token, ")" ))
	{
		MsgDev( D_WARN, "expected ')', found '%s' instead for 'multiply'\n", token );

		FS_FreeImage( pic1 );
		FS_FreeImage( pic2 );
		return NULL;
	}

	if( pic1->width != pic2->width || pic1->height != pic2->height )
	{
		MsgDev( D_WARN, "images for 'multiply' have mismatched dimensions [%ix%i] != [%ix%i]\n",
			pic1->width, pic1->height, pic2->width, pic2->height );

		FS_FreeImage( pic1 );
		FS_FreeImage( pic2 );
		return NULL;
	}

	*samples = GL_CalcImageSamples( samples1, samples2 );

	if( *samples != 1 )
	{
		*flags &= ~TF_INTENSITY;
		*flags &= ~TF_LUMINANCE;
	}

	return R_MultiplyImages( pic1, pic2 );
}

/*
=================
R_ParseBias
=================
*/
static rgbdata_t *R_ParseBias( char **script, int *samples, texFlags_t *flags )
{
	char	token[256];
	rgbdata_t	*pic;
	vec4_t	bias;
	int	i;

	*script = COM_ParseFile( *script, token );
	if( Q_stricmp( token, "(" ))
	{
		MsgDev( D_WARN, "expected '(', found '%s' instead for 'bias'\n", token );
		return NULL;
	}

	if(( *script = COM_ParseFile( *script, token )) == NULL )
	{
		MsgDev( D_WARN, "missing parameters for 'bias'\n" );
		return NULL;
	}

	pic = R_LoadImage( script, token, NULL, 0, samples, flags );
	if( !pic ) return NULL;

	for( i = 0; i < 4; i++ )
	{
		*script = COM_ParseFile( *script, token );
		if( Q_stricmp( token, "," ))
		{
			MsgDev( D_WARN, "expected ',', found '%s' instead for 'bias'\n", token );
			FS_FreeImage( pic );
			return NULL;
		}

		if(( *script = COM_ParseFile( *script, token )) == NULL )
		{
			MsgDev( D_WARN, "missing parameters for 'bias'\n" );
			FS_FreeImage( pic );
			return NULL;
		}

		bias[i] = Q_atof( token );
	}

	*script = COM_ParseFile( *script, token );
	if( Q_stricmp( token, ")" ))
	{
		MsgDev( D_WARN, "expected ')', found '%s' instead for 'bias'\n", token );
		FS_FreeImage( pic );
		return NULL;
	}

	if( *samples < 3 ) *samples += 2;
	*flags &= ~TF_INTENSITY;
	*flags &= ~TF_LUMINANCE;

	return R_BiasImage( pic, bias );
}

/*
=================
R_ParseScale
=================
*/
static rgbdata_t *R_ParseScale( char **script, int *samples, texFlags_t *flags )
{
	char	token[256];
	rgbdata_t	*pic;
	vec4_t	scale;
	int	i;

	*script = COM_ParseFile( *script, token );
	if( Q_stricmp( token, "(" ))
	{
		MsgDev( D_WARN, "expected '(', found '%s' instead for 'scale'\n", token );
		return NULL;
	}

	if(( *script = COM_ParseFile( *script, token )) == NULL )
	{
		MsgDev( D_WARN, "missing parameters for 'scale'\n" );
		return NULL;
	}

	pic = R_LoadImage( script, token, NULL, 0, samples, flags );
	if( !pic ) return NULL;

	for( i = 0; i < 4; i++ )
	{
		*script = COM_ParseFile( *script, token );
		if( Q_stricmp( token, "," ))
		{
			MsgDev( D_WARN, "expected ',', found '%s' instead for 'scale'\n", token );
			FS_FreeImage( pic );
			return NULL;
		}

		if(( *script = COM_ParseFile( *script, token )) == NULL )
		{
			MsgDev( D_WARN, "missing parameters for 'scale'\n" );
			FS_FreeImage( pic );
			return NULL;
		}

		scale[i] = Q_atof( token );
	}

	*script = COM_ParseFile( *script, token );
	if( Q_stricmp( token, ")" ))
	{
		MsgDev( D_WARN, "expected ')', found '%s' instead for 'scale'\n", token );
		FS_FreeImage( pic );
		return NULL;
	}

	if( *samples < 3 ) *samples += 2;
	*flags &= ~TF_INTENSITY;
	*flags &= ~TF_LUMINANCE;

	return R_ScaleImage( pic, scale );
}

/*
=================
R_ParseInvertColor
=================
*/
static rgbdata_t *R_ParseInvertColor( char **script, int *samples, texFlags_t *flags )
{
	char	token[256];
	rgbdata_t	*pic;

	*script = COM_ParseFile( *script, token );
	if( Q_stricmp( token, "(" ))
	{
		MsgDev( D_WARN, "expected '(', found '%s' instead for 'invertColor'\n", token );
		return NULL;
	}

	if(( *script = COM_ParseFile( *script, token )) == NULL )
	{
		MsgDev( D_WARN, "missing parameters for 'invertColor'\n" );
		return NULL;
	}

	pic = R_LoadImage( script, token, NULL, 0, samples, flags );
	if( !pic ) return NULL;

	*script = COM_ParseFile( *script, token );
	if( Q_stricmp( token, ")" ))
	{
		MsgDev( D_WARN, "expected ')', found '%s' instead for 'invertColor'\n", token );
		FS_FreeImage( pic );
		return NULL;
	}

	return R_InvertColor( pic );
}

/*
=================
R_ParseInvertAlpha
=================
*/
static rgbdata_t *R_ParseInvertAlpha( char **script, int *samples, texFlags_t *flags )
{
	char	token[256];
	rgbdata_t	*pic;

	*script = COM_ParseFile( *script, token );
	if( Q_stricmp( token, "(" ))
	{
		MsgDev( D_WARN, "expected '(', found '%s' instead for 'invertAlpha'\n", token );
		return NULL;
	}

	if(( *script = COM_ParseFile( *script, token )) == NULL )
	{
		MsgDev( D_WARN, "missing parameters for 'invertAlpha'\n" );
		return NULL;
	}

	pic = R_LoadImage( script, token, NULL, 0, samples, flags );
	if( !pic ) return NULL;

	*script = COM_ParseFile( *script, token );
	if( Q_stricmp( token, ")" ))
	{
		MsgDev( D_WARN, "expected ')', found '%s' instead for 'invertAlpha'\n", token );
		FS_FreeImage( pic );
		return NULL;
	}

	return R_InvertAlpha( pic );
}

/*
=================
R_ParseMakeIntensity
=================
*/
static rgbdata_t *R_ParseMakeIntensity( char **script, int *samples, texFlags_t *flags )
{
	char	token[256];
	rgbdata_t	*pic;

	*script = COM_ParseFile( *script, token );
	if( Q_stricmp( token, "(" ))
	{
		MsgDev( D_WARN, "expected '(', found '%s' instead for 'makeIntensity'\n", token );
		return NULL;
	}

	if(( *script = COM_ParseFile( *script, token )) == NULL )
	{
		MsgDev( D_WARN, "missing parameters for 'makeIntensity'\n" );
		return NULL;
	}

	pic = R_LoadImage( script, token, NULL, 0, samples, flags );
	if( !pic ) return NULL;

	*script = COM_ParseFile( *script, token );
	if( Q_stricmp( token, ")" ))
	{
		MsgDev( D_WARN, "expected ')', found '%s' instead for 'makeIntensity'\n", token );
		FS_FreeImage( pic );
		return NULL;
	}

	*samples = 1;
	*flags |= TF_INTENSITY;
	*flags &= ~TF_LUMINANCE;
	*flags &= ~TF_NORMALMAP;

	return R_MakeIntensity( pic );
}

/*
=================
R_ParseMakeLuminance
=================
*/
static rgbdata_t *R_ParseMakeLuminance( char **script, int *samples, texFlags_t *flags )
{
	char	token[256];
	rgbdata_t	*pic;

	*script = COM_ParseFile( *script, token );
	if( Q_stricmp( token, "(" ))
	{
		MsgDev( D_WARN, "expected '(', found '%s' instead for 'makeLuminance'\n", token );
		return NULL;
	}

	if(( *script = COM_ParseFile( *script, token )) == NULL )
	{
		MsgDev( D_WARN, "missing parameters for 'makeLuminance'\n" );
		return NULL;
	}

	pic = R_LoadImage( script, token, NULL, 0, samples, flags );
	if( !pic ) return NULL;

	*script = COM_ParseFile( *script, token );
	if( Q_stricmp( token, ")" ))
	{
		MsgDev( D_WARN, "expected ')', found '%s' instead for 'makeLuminance'\n", token );
		FS_FreeImage( pic );
		return NULL;
	}

	*samples = 1;
	*flags |= TF_LUMINANCE;
	*flags &= ~TF_INTENSITY;
	*flags &= ~TF_NORMALMAP;

	return R_MakeIntensity( pic );
}

/*
=================
R_ParseMakeAlpha
=================
*/
static rgbdata_t *R_ParseMakeAlpha( char **script, int *samples, texFlags_t *flags )
{
	char	token[256];
	rgbdata_t *pic;

	*script = COM_ParseFile( *script, token );
	if( Q_stricmp( token, "(" ))
	{
		MsgDev( D_WARN, "expected '(', found '%s' instead for 'makeAlpha'\n", token );
		return NULL;
	}

	if(( *script = COM_ParseFile( *script, token )) == NULL )
	{
		MsgDev( D_WARN, "missing parameters for 'makeAlpha'\n" );
		return NULL;
	}

	pic = R_LoadImage( script, token, NULL, 0, samples, flags );
	if( !pic ) return NULL;

	*script = COM_ParseFile( *script, token );
	if( Q_stricmp( token, ")" ))
	{
		MsgDev( D_WARN, "expected ')', found '%s' instead for 'makeAlpha'\n", token );
		FS_FreeImage( pic );
		return NULL;
	}

	*samples = 1;
	*flags &= ~TF_INTENSITY;
	*flags |= TF_HAS_ALPHA;
	*flags &= ~TF_NORMALMAP;

	return R_MakeAlpha( pic );
}

/*
=================
R_ParseMakeLuma
=================
*/
static rgbdata_t *R_ParseMakeLuma( char **script, int *samples, texFlags_t *flags )
{
	char	token[256];
	rgbdata_t *pic;

	*script = COM_ParseFile( *script, token );
	if( Q_stricmp( token, "(" ))
	{
		MsgDev( D_WARN, "expected '(', found '%s' instead for 'makeLuma'\n", token );
		return NULL;
	}

	if(( *script = COM_ParseFile( *script, token )) == NULL )
	{
		MsgDev( D_WARN, "missing parameters for 'makeLuma'\n" );
		return NULL;
	}

	pic = R_LoadImage( script, token, NULL, 0, samples, flags );
	if( !pic ) return NULL;

	if( !( pic->flags & IMAGE_HAS_LUMA ))
	{
		MsgDev( D_WARN, "%s doesn't contain a luma-pixels for 'makeLuma'\n", token );
		FS_FreeImage( pic );
		return NULL;
	}

	*script = COM_ParseFile( *script, token );
	if( Q_stricmp( token, ")" ))
	{
		MsgDev( D_WARN, "expected ')', found '%s' instead for 'makeLuma'\n", token );
		FS_FreeImage( pic );
		return NULL;
	}

	*samples = 3;

	return R_MakeLuma( pic );
}

/*
=================
R_ParseStudioSkin
=================
*/
static rgbdata_t *R_ParseStudioSkin( char **script, const byte *buf, size_t size, int *samples, texFlags_t *flags )
{
	char		token[256];
	string		model_path;
	string		modelT_path;
	string		skinname;
	rgbdata_t 	*pic;
	studiohdr_t	hdr;
	file_t		*f;

	*script = COM_ParseFile( *script, token );
	if( Q_stricmp( token, "(" ))
	{
		MsgDev( D_WARN, "expected '(', found '%s' instead for 'Studio'\n", token );
		return NULL;
	}

	if(( *script = COM_ParseFile( *script, token )) == NULL )
	{
		MsgDev( D_WARN, "missing parameters for 'Studio'\n" );
		return NULL;
	}

	// NOTE: studio skin show as 'models/props/flame1.mdl/flame2a.bmp'
	FS_ExtractFilePath( token, model_path );
	FS_StripExtension( model_path );
	Q_snprintf( modelT_path, MAX_STRING, "%sT.mdl", model_path );
	FS_DefaultExtension( model_path, ".mdl" );
	FS_FileBase( token, skinname );

	// load it in
	if( buf && size )
	{
		pic = R_LoadImage( script, va( "#%s.mdl", skinname ), buf, size, samples, flags );
		if( !pic ) return NULL;
		goto studio_done;
          }

	FS_DefaultExtension( skinname, ".bmp" );
	f = FS_Open( model_path, "rb", false );

	if( !f )
	{
		MsgDev( D_WARN, "'Studio' can't find studiomodel %s\n", model_path );
		return NULL;
	}

	if( FS_Read( f, &hdr, sizeof( hdr )) != sizeof( hdr ))
	{
		MsgDev( D_WARN, "'Studio' %s probably corrupted\n", model_path );
		FS_Close( f );
		return NULL;		
	}

	if( hdr.numtextures == 0 )
	{
		// textures are keep seperate
		FS_Close( f );
		f = FS_Open( modelT_path, "rb", false );

		if( !f )
		{
			MsgDev( D_WARN, "'Studio' can't find studiotextures %s\n", modelT_path );
			return NULL;
		}

		if( FS_Read( f, &hdr, sizeof( hdr )) != sizeof( hdr ))
		{
			MsgDev( D_WARN, "'Studio' %s probably corrupted\n", modelT_path );
			FS_Close( f );
			return NULL;		
		}
	}

	if( hdr.textureindex > 0 && hdr.numtextures <= MAXSTUDIOSKINS )
	{
		// all ok, can load model into memory
		mstudiotexture_t	*ptexture, *tex;
		size_t		mdl_size, tex_size;
		byte		*pin;
		int		i;

		FS_Seek( f, 0, SEEK_END );
		mdl_size = FS_Tell( f );
		FS_Seek( f, 0, SEEK_SET );

		pin = Mem_Alloc( r_temppool, mdl_size );

		if( FS_Read( f, pin, mdl_size ) != mdl_size )
		{
			MsgDev( D_WARN, "'Studio' %s probably corrupted\n", model_path );
			Mem_Free( pin );
			FS_Close( f );
			return NULL;
		}

		ptexture = (mstudiotexture_t *)(pin + hdr.textureindex);

		// find specified texture
		for( i = 0; i < hdr.numtextures; i++ )
		{
			if( !Q_stricmp( ptexture[i].name, skinname ))
				break; // found
		}

		if( i == hdr.numtextures )
		{
			MsgDev( D_WARN, "'Studio' %s doesn't have skin %s\n", model_path, skinname );
			Mem_Free( pin );
			FS_Close( f );
			return NULL;
		}

		tex = ptexture + i;

		// NOTE: replace index with pointer to start of imagebuffer, ImageLib expected it
		tex->index = (int)pin + tex->index;
		tex_size = sizeof( mstudiotexture_t ) + tex->width * tex->height + 768;

		// load studio texture and bind it
		FS_FileBase( skinname, skinname );

		// load it in
		pic = R_LoadImage( script, va( "#%s.mdl", tex->name ), (byte *)tex, tex_size, samples, flags );

		// shutdown operations
		Mem_Free( pin );
		FS_Close( f );

		if( !pic ) return NULL;
	}
	else
	{
		MsgDev( D_WARN, "'Studio' %s has invalid skin count\n", model_path );
		FS_Close( f );
		return NULL;		
	}

studio_done:
	*script = COM_ParseFile( *script, token );
	if( Q_stricmp( token, ")" ))
	{
		MsgDev( D_WARN, "expected ')', found '%s' instead for 'Studio'\n", token );
		FS_FreeImage( pic );
		return NULL;
	}

	return pic;
}

/*
=================
R_ParseSpriteFrame
=================
*/
static rgbdata_t *R_ParseSpriteFrame( char **script, const byte *buf, size_t size, int *samples, texFlags_t *flags )
{
	char	token[256];
	rgbdata_t *pic;

	*script = COM_ParseFile( *script, token );
	if( Q_stricmp( token, "(" ))
	{
		MsgDev( D_WARN, "expected '(', found '%s' instead for 'Sprite'\n", token );
		return NULL;
	}

	if(( *script = COM_ParseFile( *script, token )) == NULL )
	{
		MsgDev( D_WARN, "missing parameters for 'Sprite'\n" );
		return NULL;
	}

	pic = R_LoadImage( script, va( "#%s.spr", token ), buf, size, samples, flags );
	if( !pic ) return NULL;

	*script = COM_ParseFile( *script, token );
	if( Q_stricmp( token, ")" ))
	{
		MsgDev( D_WARN, "expected ')', found '%s' instead for 'Sprite'\n", token );
		FS_FreeImage( pic );
		return NULL;
	}

	return pic;
}

/*
=================
R_ParseScrapBlock
=================
*/
static rgbdata_t *R_ParseScrapBlock( char **script, int *samples, texFlags_t *flags )
{
	int	i, block[4];
	char	token[256];
	rgbdata_t *pic;

	*script = COM_ParseFile( *script, token );
	if( Q_stricmp( token, "(" ))
	{
		MsgDev( D_WARN, "expected '(', found '%s' instead for 'scrapBlock'\n", token );
		return NULL;
	}

	if(( *script = COM_ParseFile( *script, token )) == NULL )
	{
		MsgDev( D_WARN, "missing parameters for 'scrapBlock'\n" );
		return NULL;
	}

	pic = R_LoadImage( script, token, NULL, 0, samples, flags );
	if( !pic ) return NULL;

	for( i = 0; i < 4; i++ )
	{
		*script = COM_ParseFile( *script, token );
		if( Q_stricmp( token, "," ))
		{
			MsgDev( D_WARN, "expected ',', found '%s' instead for 'rect'\n", token );
			FS_FreeImage( pic );
			return NULL;
		}

		if(( *script = COM_ParseFile( *script, token )) == NULL )
		{
			MsgDev( D_WARN, "missing parameters for 'block'\n" );
			FS_FreeImage( pic );
			return NULL;
		}

		block[i] = Q_atoi( token );
#if 0
		if( block[i] < 0 )
		{
			MsgDev( D_WARN, "invalid argument %i for 'block'\n", i + 1 );
			FS_FreeImage( pic );
			return NULL;
		}

		if((( i + 1 ) & 1 ) && block[i] > pic->width ) 
		{
			MsgDev( D_WARN, "invalid argument %i for 'block'\n", i + 1 );
			FS_FreeImage( pic );
			return NULL;
		}

		if((( i + 1 ) & 2 ) && block[i] > pic->height ) 
		{
			MsgDev( D_WARN, "invalid argument %i for 'block'\n", i + 1 );
			FS_FreeImage( pic );
			return NULL;
		}
#endif
	}

	*script = COM_ParseFile( *script, token );
	if( Q_stricmp( token, ")" ))
	{
		MsgDev( D_WARN, "expected ')', found '%s' instead for 'bias'\n", token );
		FS_FreeImage( pic );
		return NULL;
	}

	// check bounds silently
	if( block[0] < 0 || block[0] > pic->width ) block[0] = 0;
	if( block[1] < 0 || block[1] > pic->height ) block[1] = 0;
	if( block[2] < 0 || block[2] > pic->width ) block[2] = pic->width;
	if( block[3] < 0 || block[3] > pic->height ) block[3] = pic->height;

	if(( block[0] + block[2] > pic->width ) || ( block[1] + block[3] > pic->height ))
	{
		MsgDev( D_WARN, "'ScrapBlock' image size out of bounds\n" );
		FS_FreeImage( pic );
		return NULL;
	}

	return R_MakeImageBlock( pic, block );
}

/*
=================
R_ParseHeightMap
=================
*/
static rgbdata_t *R_ParseHeightMap( char **script, const byte *buf, size_t size, int *samples, texFlags_t *flags )
{
	char	token[256];
	rgbdata_t *pic;
	float	scale;

	*script = COM_ParseFile( *script, token );
	if( Q_stricmp( token, "(" ))
	{
		MsgDev( D_WARN, "expected '(', found '%s' instead for 'heightMap'\n", token );
		return NULL;
	}

	if(( *script = COM_ParseFile( *script, token )) == NULL )
	{
		MsgDev( D_WARN, "missing parameters for 'heightMap'\n" );
		return NULL;
	}

	pic = R_LoadImage( script, token, buf, size, samples, flags );
	if( !pic ) return NULL;

	*script = COM_ParseFile( *script, token );
	if( Q_stricmp( token, "," ))
	{
		MsgDev( D_WARN, "expected ',', found '%s' instead for 'heightMap'\n", token );
		FS_FreeImage( pic );
		return NULL;
	}

	if(( *script = COM_ParseFile( *script, token )) == NULL )
	{
		MsgDev( D_WARN, "missing parameters for 'heightMap'\n" );
		FS_FreeImage( pic );
		return NULL;
	}

	scale = Q_atof( token );

	*script = COM_ParseFile( *script, token );
	if( Q_stricmp( token, ")" ))
	{
		MsgDev( D_WARN, "expected ')', found '%s' instead for 'heightMap'\n", token );
		FS_FreeImage( pic );
		return NULL;
	}

	*samples = 3;
	*flags &= ~TF_INTENSITY;
	*flags &= ~TF_LUMINANCE;
	*flags |= TF_NORMALMAP;

	return R_HeightMap( pic, scale );
}

/*
=================
R_ParseAddNormals
=================
*/
static rgbdata_t *R_ParseAddNormals( char **script, int *samples, texFlags_t *flags )
{
	char	token[256];
	rgbdata_t *pic1, *pic2;
	int	samples1, samples2;

	*script = COM_ParseFile( *script, token );
	if( Q_stricmp( token, "(" ))
	{
		MsgDev( D_WARN, "expected '(', found '%s' instead for 'addNormals'\n", token );
		return NULL;
	}

	if(( *script = COM_ParseFile( *script, token )) == NULL )
	{
		MsgDev( D_WARN, "missing parameters for 'addNormals'\n" );
		return NULL;
	}

	pic1 = R_LoadImage( script, token, NULL, 0, &samples1, flags );
	if( !pic1 ) return NULL;

	*script = COM_ParseFile( *script, token );
	if( Q_stricmp( token, "," ))
	{
		MsgDev( D_WARN, "expected ',', found '%s' instead for 'addNormals'\n", token );
		FS_FreeImage( pic1 );
		return NULL;
	}

	if(( *script = COM_ParseFile( *script, token )) == NULL )
	{
		MsgDev( D_WARN, "missing parameters for 'addNormals'\n" );
		FS_FreeImage( pic1 );
		return NULL;
	}

	pic2 = R_LoadImage( script, token, NULL, 0, &samples2, flags );
	if( !pic2 )
	{
		FS_FreeImage( pic1 );
		return NULL;
	}

	*script = COM_ParseFile( *script, token );
	if( Q_stricmp( token, ")" ))
	{
		MsgDev( D_WARN, "expected ')', found '%s' instead for 'addNormals'\n", token );
		FS_FreeImage( pic1 );
		FS_FreeImage( pic2 );
		return NULL;
	}

	if( pic1->width != pic2->width || pic1->height != pic2->height )
	{
		MsgDev( D_WARN, "images for 'addNormals' have mismatched dimensions [%ix%i] != [%ix%i]\n",
			pic1->width, pic1->height, pic2->width, pic2->height );
		FS_FreeImage( pic1 );
		FS_FreeImage( pic2 );
		return NULL;
	}

	*samples = 3;
	*flags &= ~TF_INTENSITY;
	*flags &= ~TF_LUMINANCE;
	*flags |= TF_NORMALMAP;

	return R_AddNormals( pic1, pic2 );
}

/*
=================
R_ParseSmoothNormals
=================
*/
static rgbdata_t *R_ParseSmoothNormals( char **script, int *samples, texFlags_t *flags )
{
	char	token[256];
	rgbdata_t *pic;

	*script = COM_ParseFile( *script, token );
	if( Q_stricmp( token, "(" ))
	{
		MsgDev( D_WARN, "expected '(', found '%s' instead for 'smoothNormals'\n", token );
		return NULL;
	}

	if(( *script = COM_ParseFile( *script, token )) == NULL )
	{
		MsgDev( D_WARN, "missing parameters for 'smoothNormals'\n" );
		return NULL;
	}

	pic = R_LoadImage( script, token, NULL, 0, samples, flags );
	if( !pic ) return NULL;

	*script = COM_ParseFile( *script, token );
	if( Q_stricmp( token, ")" ))
	{
		MsgDev( D_WARN, "expected ')', found '%s' instead for 'smoothNormals'\n", token );
		FS_FreeImage( pic );
		return NULL;
	}

	*samples = 3;
	*flags &= ~TF_INTENSITY;
	*flags &= ~TF_LUMINANCE;
	*flags |= TF_NORMALMAP;

	return R_SmoothNormals( pic );
}

/*
=================
R_ParseDepthmap
=================
*/
static rgbdata_t *R_ParseDepthmap( char **script, const byte *buf, size_t size, int *samples, texFlags_t *flags )
{
	char	token[256];
	rgbdata_t	*pic1, *pic2;
	int	samples1, samples2;

	*script = COM_ParseFile( *script, token );
	if( Q_stricmp( token, "(" ))
	{
		MsgDev( D_WARN, "expected '(', found '%s' instead for 'mergeDepthmap'\n", token );
		return NULL;
	}

	if(( *script = COM_ParseFile( *script, token )) == NULL )
	{
		MsgDev( D_WARN, "missing parameters for 'mergeDepthmap'\n" );
		return NULL;
	}

	pic1 = R_LoadImage( script, token, buf, size, &samples1, flags );
	if( !pic1 ) return NULL;

	*script = COM_ParseFile( *script, token );
	if( Q_stricmp( token, "," ))
	{
		MsgDev( D_WARN, "expected ',', found '%s' instead for 'mergeDepthmap'\n", token );
		FS_FreeImage( pic1 );
		return NULL;
	}

	if(( *script = COM_ParseFile( *script, token )) == NULL )
	{
		MsgDev( D_WARN, "missing parameters for 'mergeDepthmap'\n" );
		FS_FreeImage( pic1 );
		return NULL;
	}

	*samples = 3;
	pic2 = R_LoadImage( script, token, buf, size, &samples2, flags );
	if( !pic2 ) return pic1; // don't free normalmap

	*script = COM_ParseFile( *script, token );
	if( Q_stricmp( token, ")" ))
	{
		MsgDev( D_WARN, "expected ')', found '%s' instead for 'mergeDepthmap'\n", token );

		FS_FreeImage( pic1 );
		FS_FreeImage( pic2 );
		return NULL;
	}

	if( pic1->width != pic2->width || pic1->height != pic2->height )
	{
		MsgDev( D_WARN, "images for 'mergeDepthmap' have mismatched dimensions [%ix%i] != [%ix%i]\n",
			pic1->width, pic1->height, pic2->width, pic2->height );

		FS_FreeImage( pic2 );
		return pic1; // don't free normalmap
	}

	*samples = 4;
	*flags &= ~TF_INTENSITY;

	return R_IncludeDepthmap( pic1, pic2 );
}

/*
=================
R_ParseClearPixels
=================
*/
static rgbdata_t *R_ParseClearPixels( char **script, int *samples, texFlags_t *flags )
{
	char	token[256];
	qboolean	clearAlpha;
	rgbdata_t *pic;

	*script = COM_ParseFile( *script, token );
	if( Q_stricmp( token, "(" ))
	{
		MsgDev( D_WARN, "expected '(', found '%s' instead for 'clearPixels'\n", token );
		return NULL;
	}

	if(( *script = COM_ParseFile( *script, token )) == NULL )
	{
		MsgDev( D_WARN, "missing parameters for 'clearPixels'\n" );
		return NULL;
	}

	pic = R_LoadImage( script, token, NULL, 0, samples, flags );
	if( !pic ) return NULL;

	*script = COM_ParseFile( *script, token );
	if( !Q_stricmp( token, "alpha" ))
	{
		*script = COM_ParseFile( *script, token );
		clearAlpha = true;
	}
	else if( !Q_stricmp( token, "color" ))
	{
		*script = COM_ParseFile( *script, token );
		clearAlpha = false;
	}
	else if( !Q_stricmp( token, ")" ))
	{
		clearAlpha = false;	// clear color as default
	}
	else *script = COM_ParseFile( *script, token ); // skip unknown token
	
	if( Q_stricmp( token, ")" ))
	{
		MsgDev( D_WARN, "expected ')', found '%s' instead for 'clearPixels'\n", token );
		FS_FreeImage( pic );
		return NULL;
	}

	*samples = clearAlpha ? 3 : 1;
	if( clearAlpha ) *flags &= ~TF_HAS_ALPHA;
	*flags &= ~TF_INTENSITY;

	return R_ClearPixels( pic, clearAlpha );
}

/*
=================
R_ParseMovePixels
=================
*/
static rgbdata_t *R_ParseMovePixels( char **script, int *samples, texFlags_t *flags )
{
	char	token[256];
	qboolean	alphaToColor;
	rgbdata_t *pic;

	*script = COM_ParseFile( *script, token );
	if( Q_stricmp( token, "(" ))
	{
		MsgDev( D_WARN, "expected '(', found '%s' instead for 'movePixels'\n", token );
		return NULL;
	}

	if(( *script = COM_ParseFile( *script, token )) == NULL )
	{
		MsgDev( D_WARN, "missing parameters for 'movePixels'\n" );
		return NULL;
	}

	pic = R_LoadImage( script, token, NULL, 0, samples, flags );
	if( !pic ) return NULL;

	*script = COM_ParseFile( *script, token );
	if( !Q_stricmp( token, "AlphaToColor" ))
	{
		*script = COM_ParseFile( *script, token );
		alphaToColor = true;
	}
	else if( !Q_stricmp( token, "ColorToAlpha" ))
	{
		*script = COM_ParseFile( *script, token );
		alphaToColor = false;
	}
	else if( !Q_stricmp( token, ")" ))
	{
		alphaToColor = true; // move alpha to color as default
	}
	else *script = COM_ParseFile( *script, token ); // skip unknown token
	
	if( Q_stricmp( token, ")" ))
	{
		MsgDev( D_WARN, "expected ')', found '%s' instead for 'movePixels'\n", token );
		FS_FreeImage( pic );
		return NULL;
	}

	*samples = alphaToColor ? 3 : 1;
	if( alphaToColor ) *flags &= ~TF_HAS_ALPHA;
	*flags &= ~TF_INTENSITY;

	return R_MovePixels( pic, alphaToColor );
}

/*
=================
R_LoadImage
=================
*/
static rgbdata_t *R_LoadImage( char **script, const char *name, const byte *buf, size_t size, int *samples, texFlags_t *flags )
{
	if( !Q_stricmp( name, "add" ))
		return R_ParseAdd( script, samples, flags );
	else if( !Q_stricmp( name, "multiply" ))
		return R_ParseMultiply( script, samples, flags );
	else if( !Q_stricmp( name, "bias" ))
		return R_ParseBias( script, samples, flags );
	else if( !Q_stricmp( name, "scale"))
		return R_ParseScale( script, samples, flags );
	else if( !Q_stricmp( name, "invertColor" ))
		return R_ParseInvertColor( script, samples, flags );
	else if( !Q_stricmp( name, "invertAlpha" ))
		return R_ParseInvertAlpha( script, samples, flags );
	else if( !Q_stricmp( name, "makeIntensity" ))
		return R_ParseMakeIntensity( script, samples, flags );
	else if( !Q_stricmp( name, "makeLuminance" ))
		return R_ParseMakeLuminance( script, samples, flags);
	else if( !Q_stricmp( name, "makeAlpha" ))
		return R_ParseMakeAlpha( script, samples, flags );
	else if( !Q_stricmp( name, "makeLuma" ))
		return R_ParseMakeLuma( script, samples, flags );
	else if( !Q_stricmp( name, "heightMap" ))
		return R_ParseHeightMap( script, buf, size, samples, flags );
	else if( !Q_stricmp( name, "ScrapBlock" ))
		return R_ParseScrapBlock( script, samples, flags );
	else if( !Q_stricmp( name, "addNormals" ))
		return R_ParseAddNormals( script, samples, flags );
	else if( !Q_stricmp( name, "smoothNormals" ))
		return R_ParseSmoothNormals( script, samples, flags );
	else if( !Q_stricmp( name, "mergeDepthmap" ))
		return R_ParseDepthmap( script, buf, size, samples, flags );
	else if( !Q_stricmp( name, "clearPixels" ))
		return R_ParseClearPixels( script, samples, flags );
	else if( !Q_stricmp( name, "movePixels" ))
		return R_ParseMovePixels( script, samples, flags );
	else if( !Q_stricmp( name, "Studio" ))
		return R_ParseStudioSkin( script, buf, size, samples, flags );
	else if( !Q_stricmp( name, "Sprite" ))
		return R_ParseSpriteFrame( script, buf, size, samples, flags );
	else
	{	
		// loading form disk
		rgbdata_t	*image = FS_LoadImage( name, buf, size );
		if( image ) *samples = GL_CalcTextureSamples( image->flags );

		return image;
	}
	return NULL;
}

/*
================
R_FreeImage
================
*/
void R_FreeImage( gltexture_t *image )
{
	uint		hash;
	gltexture_t	*cur;
	gltexture_t	**prev;

	ASSERT( image != NULL );

	if( !image->name[0] )
	{
		if( image->texnum != 0 )
			MsgDev( D_ERROR, "trying to free unnamed texture with texnum %i\n", image->texnum );
		return;
	}

	// remove from hash table
	hash = Com_HashKey( image->name, TEXTURES_HASH_SIZE );
	prev = &r_texturesHashTable[hash];

	while( 1 )
	{
		cur = *prev;
		if( !cur ) break;

		if( cur == image )
		{
			*prev = cur->nextHash;
			break;
		}
		prev = &cur->nextHash;
	}

	// release source
	if( image->flags & (TF_KEEP_RGBDATA|TF_KEEP_8BIT) && image->original )
		FS_FreeImage( image->original );

	pglDeleteTextures( 1, &image->texnum );
	Q_memset( image, 0, sizeof( *image ));
}

/*
==============================================================================

INTERNAL TEXTURES

==============================================================================
*/
/*
==================
R_InitDefaultTexture
==================
*/
static rgbdata_t *R_InitDefaultTexture( texFlags_t *flags )
{
	int	x, y;

	// also use this for bad textures, but without alpha
	r_image.width = r_image.height = 16;
	r_image.buffer = data2D;
	r_image.flags = IMAGE_HAS_COLOR;
	r_image.type = PF_RGBA_32;
	r_image.size = r_image.width * r_image.height * 4;

	*flags = 0;

	// emo-texture from quake1
	for( y = 0; y < 16; y++ )
	{
		for( x = 0; x < 16; x++ )
		{
			if(( y < 8 ) ^ ( x < 8 ))
				((uint *)&data2D)[y*16+x] = 0xFFFF00FF;
			else ((uint *)&data2D)[y*16+x] = 0xFF000000;
		}
	}
	return &r_image;
}

/*
==================
R_InitParticleTexture
==================
*/
static rgbdata_t *R_InitParticleTexture( texFlags_t *flags )
{
	int	x, y;
	int	dx2, dy, d;

	// particle texture
	r_image.width = r_image.height = 16;
	r_image.buffer = data2D;
	r_image.flags = (IMAGE_HAS_COLOR|IMAGE_HAS_ALPHA);
	r_image.type = PF_RGBA_32;
	r_image.size = r_image.width * r_image.height * 4;

	*flags = TF_NOPICMIP|TF_NOMIPMAP;

	for( x = 0; x < 16; x++ )
	{
		dx2 = x - 8;
		dx2 = dx2 * dx2;

		for( y = 0; y < 16; y++ )
		{
			dy = y - 8;
			d = 255 - 35 * sqrt( dx2 + dy * dy );
			data2D[( y*16 + x ) * 4 + 3] = bound( 0, d, 255 );
		}
	}
	return &r_image;
}

/*
==================
R_InitParticleTexture2
==================
*/
static rgbdata_t *R_InitParticleTexture2( texFlags_t *flags )
{
	int	x, y;

	// particle texture
	r_image.width = r_image.height = 8;
	r_image.buffer = data2D;
	r_image.flags = (IMAGE_HAS_COLOR|IMAGE_HAS_ALPHA);
	r_image.type = PF_RGBA_32;
	r_image.size = r_image.width * r_image.height * 4;

	*flags = TF_NOPICMIP|TF_NOMIPMAP;

	for( x = 0; x < 8; x++ )
	{
		for( y = 0; y < 8; y++ )
		{
			data2D[(y * 8 + x) * 4 + 0] = 255;
			data2D[(y * 8 + x) * 4 + 1] = 255;
			data2D[(y * 8 + x) * 4 + 2] = 255;
			data2D[(y * 8 + x) * 4 + 3] = r_particleTexture[x][y] * 255;
		}
	}
	return &r_image;
}

/*
==================
R_InitSkyTexture
==================
*/
static rgbdata_t *R_InitSkyTexture( texFlags_t *flags )
{
	int	i;

	// skybox texture
	for( i = 0; i < 256; i++ )
		((uint *)&data2D)[i] = 0xFFFFDEB5;

	*flags = TF_NOPICMIP|TF_UNCOMPRESSED;

	r_image.buffer = data2D;
	r_image.width = r_image.height = 16;
	r_image.size = r_image.width * r_image.height * 4;
	r_image.flags = IMAGE_HAS_COLOR;
	r_image.type = PF_RGBA_32;

	return &r_image;
}

/*
==================
R_InitCinematicTexture
==================
*/
static rgbdata_t *R_InitCinematicTexture( texFlags_t *flags )
{
	r_image.buffer = data2D;
	r_image.type = PF_RGBA_32;
	r_image.flags = IMAGE_HAS_COLOR;
	r_image.width = r_image.height = 256;
	r_image.size = r_image.width * r_image.height * 4;

	*flags = TF_NOMIPMAP|TF_NOPICMIP|TF_UNCOMPRESSED|TF_CLAMP;

	return &r_image;
}

/*
==================
R_InitSolidColorTexture
==================
*/
static rgbdata_t *R_InitSolidColorTexture( texFlags_t *flags, int color )
{
	// solid color texture
	r_image.width = r_image.height = 1;
	r_image.buffer = data2D;
	r_image.flags = IMAGE_HAS_COLOR;
	r_image.type = PF_RGB_24;
	r_image.size = r_image.width * r_image.height * 3;

	*flags = TF_NOPICMIP|TF_UNCOMPRESSED;

	data2D[0] = data2D[1] = data2D[2] = color;
	return &r_image;
}

/*
==================
R_InitWhiteTexture
==================
*/
static rgbdata_t *R_InitWhiteTexture( texFlags_t *flags )
{
	return R_InitSolidColorTexture( flags, 255 );
}

/*
==================
R_InitGrayTexture
==================
*/
static rgbdata_t *R_InitGrayTexture( texFlags_t *flags )
{
	return R_InitSolidColorTexture( flags, 127 );
}

/*
==================
R_InitBlackTexture
==================
*/
static rgbdata_t *R_InitBlackTexture( texFlags_t *flags )
{
	return R_InitSolidColorTexture( flags, 0 );
}

/*
==================
R_InitBlankBumpTexture
==================
*/
static rgbdata_t *R_InitBlankBumpTexture( texFlags_t *flags )
{
	int	i;

	// default normalmap texture
	for( i = 0; i < 256; i++ )
	{
		data2D[i*4+0] = 127;
		data2D[i*4+1] = 127;
		data2D[i*4+2] = 255;
	}

	*flags = TF_NORMALMAP|TF_UNCOMPRESSED;

	r_image.buffer = data2D;
	r_image.width = r_image.height = 16;
	r_image.size = r_image.width * r_image.height * 4;
	r_image.flags = IMAGE_HAS_COLOR;
	r_image.type = PF_RGBA_32;

	return &r_image;
}

/*
==================
R_InitBlankDeluxeTexture
==================
*/
static rgbdata_t *R_InitBlankDeluxeTexture( texFlags_t *flags )
{
	int	i;

	// default normalmap texture
	for( i = 0; i < 256; i++ )
	{
		data2D[i*4+0] = 127;
		data2D[i*4+1] = 127;
		data2D[i*4+2] = 0;	// light from ceiling
	}

	*flags = TF_NORMALMAP|TF_UNCOMPRESSED;

	r_image.buffer = data2D;
	r_image.width = r_image.height = 16;
	r_image.size = r_image.width * r_image.height * 4;
	r_image.flags = IMAGE_HAS_COLOR;
	r_image.type = PF_RGBA_32;

	return &r_image;
}

/*
==================
R_InitAttenuationTexture
==================
*/
static rgbdata_t *R_InitAttenTextureGamma( texFlags_t *flags, float gamma )
{
	int	i;

	// 1d attenuation texture
	r_image.width = 256;
	r_image.height = 1;
	r_image.buffer = data2D;
	r_image.flags = IMAGE_HAS_COLOR;
	r_image.type = PF_RGBA_32;
	r_image.size = r_image.width * r_image.height * 4;

	for( i = 0; i < r_image.width; i++ )
	{
		float atten = 255 - bound( 0, 255 * pow((i + 0.5f) / r_image.width, gamma ) + 0.5f, 255 );

		// clear attenuation at ends to prevent light go outside
		if( i == (r_image.width - 1) || i == 0 )
			atten = 0.0f;

		data2D[(i * 4) + 0] = (byte)atten;
		data2D[(i * 4) + 1] = (byte)atten;
		data2D[(i * 4) + 2] = (byte)atten;
		data2D[(i * 4) + 3] = (byte)atten;
	}

	*flags = TF_UNCOMPRESSED|TF_NOMIPMAP|TF_CLAMP|TF_TEXTURE_1D;

	return &r_image;
}

static rgbdata_t *R_InitAttenuationTexture( texFlags_t *flags )
{
	return R_InitAttenTextureGamma( flags, 1.5f );
}

static rgbdata_t *R_InitAttenuationTexture2( texFlags_t *flags )
{
	return R_InitAttenTextureGamma( flags, 0.5f );
}

static rgbdata_t *R_InitAttenuationTexture3( texFlags_t *flags )
{
	return R_InitAttenTextureGamma( flags, 3.5f );
}

static rgbdata_t *R_InitAttenuationTextureNoAtten( texFlags_t *flags )
{
	// 1d attenuation texture
	r_image.width = 256;
	r_image.height = 1;
	r_image.buffer = data2D;
	r_image.flags = IMAGE_HAS_COLOR;
	r_image.type = PF_RGBA_32;
	r_image.size = r_image.width * r_image.height * 4;

	Q_memset( data2D, 0xFF, r_image.size );
	*flags = TF_UNCOMPRESSED|TF_NOMIPMAP|TF_CLAMP|TF_TEXTURE_1D;

	return &r_image;
}

/*
==================
R_InitAttenuationTexture3D
==================
*/
static rgbdata_t *R_InitAttenTexture3D( texFlags_t *flags )
{
	vec3_t	v = { 0, 0, 0 };
	int	x, y, z, d, size, size2, halfsize;
	float	intensity;

	if( !GL_Support( GL_TEXTURE_3D_EXT ))
		return NULL;

	// 3d attenuation texture
	r_image.width = 32;
	r_image.height = 32;
	r_image.depth = 32;
	r_image.buffer = data2D;
	r_image.flags = IMAGE_HAS_COLOR;
	r_image.type = PF_RGBA_32;
	r_image.size = r_image.width * r_image.height * r_image.depth * 4;

	size = 32;
	halfsize = size / 2;
	intensity = halfsize * halfsize;
	size2 = size * size;

	for( x = 0; x < r_image.width; x++ )
	{
		for( y = 0; y < r_image.height; y++ )
		{
			for( z = 0; z < r_image.depth; z++ )
			{
				v[0] = (( x + 0.5f ) * ( 2.0f / (float)size ) - 1.0f );
				v[1] = (( y + 0.5f ) * ( 2.0f / (float)size ) - 1.0f );
				if( r_image.depth > 1 ) v[2] = (( z + 0.5f ) * ( 2.0f / (float)size ) - 1.0f );

				intensity = 1.0f - sqrt( DotProduct( v, v ) );
				if( intensity > 0 ) intensity = intensity * intensity * 215.5f;
				d = bound( 0, intensity, 255 );

				data2D[((z * size + y) * size + x) * 4 + 0] = d;
				data2D[((z * size + y) * size + x) * 4 + 1] = d;
				data2D[((z * size + y) * size + x) * 4 + 2] = d;
			}
		}
	}

	*flags = TF_UNCOMPRESSED|TF_NOMIPMAP|TF_CLAMP|TF_TEXTURE_3D;

	return &r_image;
}

static rgbdata_t *R_InitDlightTexture( texFlags_t *flags )
{
	// solid color texture
	r_image.width = BLOCK_SIZE_DEFAULT; 
	r_image.height = BLOCK_SIZE_DEFAULT;
	r_image.flags = IMAGE_HAS_COLOR;
	r_image.type = PF_RGBA_32;
	r_image.size = r_image.width * r_image.height * 4;
	r_image.buffer = data2D;

	Q_memset( data2D, 0x00, r_image.size );

	*flags = TF_NOPICMIP|TF_UNCOMPRESSED|TF_NOMIPMAP;

	return &r_image;
}

static rgbdata_t *R_InitDlightTexture2( texFlags_t *flags )
{
	// solid color texture
	r_image.width = BLOCK_SIZE_MAX; 
	r_image.height = BLOCK_SIZE_MAX;
	r_image.flags = IMAGE_HAS_COLOR;
	r_image.type = PF_RGBA_32;
	r_image.size = r_image.width * r_image.height * 4;
	r_image.buffer = data2D;

	Q_memset( data2D, 0x00, r_image.size );

	*flags = TF_NOPICMIP|TF_UNCOMPRESSED|TF_NOMIPMAP;

	return &r_image;
}

/*
==================
R_InitNormalizeCubemap
==================
*/
static rgbdata_t *R_InitNormalizeCubemap( texFlags_t *flags )
{
	int	i, x, y, size = 32;
	byte	*dataCM = data2D;
	float	s, t;
	vec3_t	normal;

	if( !GL_Support( GL_TEXTURECUBEMAP_EXT ))
		return NULL;

	// normal cube map texture
	for( i = 0; i < 6; i++ )
	{
		for( y = 0; y < size; y++ )
		{
			for( x = 0; x < size; x++ )
			{
				s = (((float)x + 0.5f) * (2.0f / size )) - 1.0f;
				t = (((float)y + 0.5f) * (2.0f / size )) - 1.0f;

				switch( i )
				{
				case 0: VectorSet( normal, 1.0f, -t, -s ); break;
				case 1: VectorSet( normal, -1.0f, -t, s ); break;
				case 2: VectorSet( normal, s,  1.0f,  t ); break;
				case 3: VectorSet( normal, s, -1.0f, -t ); break;
				case 4: VectorSet( normal, s, -t, 1.0f  ); break;
				case 5: VectorSet( normal, -s, -t, -1.0f); break;
				}

				VectorNormalize( normal );

				dataCM[4*(y*size+x)+0] = (byte)(128 + 127 * normal[0]);
				dataCM[4*(y*size+x)+1] = (byte)(128 + 127 * normal[1]);
				dataCM[4*(y*size+x)+2] = (byte)(128 + 127 * normal[2]);
				dataCM[4*(y*size+x)+3] = 255;
			}
		}
		dataCM += (size*size*4); // move pointer
	}

	*flags = (TF_NOPICMIP|TF_NOMIPMAP|TF_UNCOMPRESSED|TF_CUBEMAP|TF_CLAMP);

	r_image.width = r_image.height = size;
	r_image.size = r_image.width * r_image.height * 4 * 6;
	r_image.flags |= (IMAGE_CUBEMAP|IMAGE_HAS_COLOR); // yes it's cubemap
	r_image.buffer = data2D;
	r_image.type = PF_RGBA_32;

	return &r_image;
}

/*
==================
R_InitDlightCubemap
==================
*/
static rgbdata_t *R_InitDlightCubemap( texFlags_t *flags )
{
	int	i, x, y, size = 4;
	byte	*dataCM = data2D;
	int	dx2, dy, d;

	if( !GL_Support( GL_TEXTURECUBEMAP_EXT ))
		return NULL;

	// normal cube map texture
	for( i = 0; i < 6; i++ )
	{
		for( x = 0; x < size; x++ )
		{
			dx2 = x - size / 2;
			dx2 = dx2 * dx2;

			for( y = 0; y < size; y++ )
			{
				dy = y - size / 2;
				d = 255 - 35 * sqrt( dx2 + dy * dy );
				dataCM[( y * size + x ) * 4 + 0] = bound( 0, d, 255 );
				dataCM[( y * size + x ) * 4 + 1] = bound( 0, d, 255 );
				dataCM[( y * size + x ) * 4 + 2] = bound( 0, d, 255 );
			}
		}
		dataCM += (size * size * 4); // move pointer
	}

	*flags = (TF_NOPICMIP|TF_NOMIPMAP|TF_UNCOMPRESSED|TF_CUBEMAP|TF_CLAMP);

	r_image.width = r_image.height = size;
	r_image.size = r_image.width * r_image.height * 4 * 6;
	r_image.flags |= (IMAGE_CUBEMAP|IMAGE_HAS_COLOR); // yes it's cubemap
	r_image.buffer = data2D;
	r_image.type = PF_RGBA_32;

	return &r_image;
}

/*
==================
R_InitGrayCubemap
==================
*/
static rgbdata_t *R_InitGrayCubemap( texFlags_t *flags )
{
	int	size = 4;
	byte	*dataCM = data2D;

	if( !GL_Support( GL_TEXTURECUBEMAP_EXT ))
		return NULL;

	// gray cubemap - just stub for pointlights
	Q_memset( dataCM, 0x7F, size * size * 6 * 4 );

	*flags = (TF_NOPICMIP|TF_NOMIPMAP|TF_UNCOMPRESSED|TF_CUBEMAP|TF_CLAMP);

	r_image.width = r_image.height = size;
	r_image.size = r_image.width * r_image.height * 4 * 6;
	r_image.flags |= (IMAGE_CUBEMAP|IMAGE_HAS_COLOR); // yes it's cubemap
	r_image.buffer = data2D;
	r_image.type = PF_RGBA_32;

	return &r_image;
}

/*
==================
R_InitWhiteCubemap
==================
*/
static rgbdata_t *R_InitWhiteCubemap( texFlags_t *flags )
{
	int	size = 4;
	byte	*dataCM = data2D;

	if( !GL_Support( GL_TEXTURECUBEMAP_EXT ))
		return NULL;

	// white cubemap - just stub for pointlights
	Q_memset( dataCM, 0xFF, size * size * 6 * 4 );

	*flags = (TF_NOPICMIP|TF_NOMIPMAP|TF_UNCOMPRESSED|TF_CUBEMAP|TF_CLAMP);

	r_image.width = r_image.height = size;
	r_image.size = r_image.width * r_image.height * 4 * 6;
	r_image.flags |= (IMAGE_CUBEMAP|IMAGE_HAS_COLOR); // yes it's cubemap
	r_image.buffer = data2D;
	r_image.type = PF_RGBA_32;

	return &r_image;
}

/*
==================
R_InitAlphaContrast
==================
*/
static rgbdata_t *R_InitAlphaContrast( texFlags_t *flags )
{
	int	size = 64;
	byte	*data = data2D;

	*flags = (TF_NOPICMIP|TF_UNCOMPRESSED|TF_ALPHACONTRAST|TF_INTENSITY);

	r_image.width = r_image.height = 64;
	r_image.size = r_image.width * r_image.height * 4;

	Q_memset( data, size, r_image.size );

	r_image.buffer = data2D;
	r_image.type = PF_RGBA_32;

	return &r_image;
}

/*
==================
R_InitBuiltinTextures
==================
*/
static void R_InitBuiltinTextures( void )
{
	rgbdata_t		*pic;
	texFlags_t	flags;

	const struct
	{
		char	*name;
		int	*texnum;
		rgbdata_t	*(*init)( texFlags_t *flags );
		int	texType;
	}
	textures[] =
	{
	{ "*default", &tr.defaultTexture, R_InitDefaultTexture, TEX_SYSTEM },
	{ "*white", &tr.whiteTexture, R_InitWhiteTexture, TEX_SYSTEM },
	{ "*gray", &tr.grayTexture, R_InitGrayTexture, TEX_SYSTEM },
	{ "*black", &tr.blackTexture, R_InitBlackTexture, TEX_SYSTEM },
	{ "*particle", &tr.particleTexture, R_InitParticleTexture, TEX_SYSTEM },
	{ "*particle2", &tr.particleTexture2, R_InitParticleTexture2, TEX_SYSTEM },
	{ "*cintexture", &tr.cinTexture, R_InitCinematicTexture, TEX_NOMIP },	// force linear filter
	{ "*dlight", &tr.dlightTexture, R_InitDlightTexture, TEX_LIGHTMAP },
	{ "*dlight2", &tr.dlightTexture2, R_InitDlightTexture2, TEX_LIGHTMAP },
	{ "*atten", &tr.attenuationTexture, R_InitAttenuationTexture, TEX_SYSTEM },
	{ "*atten2", &tr.attenuationTexture2, R_InitAttenuationTexture2, TEX_SYSTEM },
	{ "*atten3", &tr.attenuationTexture3, R_InitAttenuationTexture3, TEX_SYSTEM },
	{ "*attnno", &tr.attenuationStubTexture, R_InitAttenuationTextureNoAtten, TEX_SYSTEM },
	{ "*normalize", &tr.normalizeTexture, R_InitNormalizeCubemap, TEX_CUBEMAP },
	{ "*blankbump", &tr.blankbumpTexture, R_InitBlankBumpTexture, TEX_SYSTEM },
	{ "*blankdeluxe", &tr.blankdeluxeTexture, R_InitBlankDeluxeTexture, TEX_SYSTEM },
	{ "*lightCube", &tr.dlightCubeTexture, R_InitDlightCubemap, TEX_CUBEMAP },
	{ "*grayCube", &tr.grayCubeTexture, R_InitGrayCubemap, TEX_CUBEMAP },
	{ "*whiteCube", &tr.whiteCubeTexture, R_InitWhiteCubemap, TEX_CUBEMAP },
	{ "*atten3D", &tr.attenuationTexture3D, R_InitAttenTexture3D, TEX_SYSTEM },
	{ "*sky", &tr.skyTexture, R_InitSkyTexture, TEX_SYSTEM },
	{ "*alphaContrast", &tr.acontTexture, R_InitAlphaContrast, TEX_SYSTEM },
	{ NULL, NULL, NULL }
	};
	size_t	i, num_builtin_textures = sizeof( textures ) / sizeof( textures[0] ) - 1;

	for( i = 0; i < num_builtin_textures; i++ )
	{
		Q_memset( &r_image, 0, sizeof( rgbdata_t ));
		Q_memset( data2D, 0xFF, sizeof( data2D ));

		pic = textures[i].init( &flags );
		if( pic == NULL ) continue;
		*textures[i].texnum = GL_LoadTextureInternal( textures[i].name, pic, flags, false );

		GL_SetTextureType( *textures[i].texnum, textures[i].texType );
	}
}

/*
===============
R_InitImages
===============
*/
void R_InitImages( void )
{
	uint	i, hash;
	float	f;

	r_numTextures = 0;
	scaledImage = NULL;
	Q_memset( r_textures, 0, sizeof( r_textures ));
	Q_memset( r_texturesHashTable, 0, sizeof( r_texturesHashTable ));

	// create unused 0-entry
	Q_strncpy( r_textures->name, "*unused*", sizeof( r_textures->name ));
	hash = Com_HashKey( r_textures->name, TEXTURES_HASH_SIZE );
	r_textures->nextHash = r_texturesHashTable[hash];
	r_texturesHashTable[hash] = r_textures;
	r_numTextures = 1;

	// build luminance table
	for( i = 0; i < 256; i++ )
	{
		f = (float)i;
		r_luminanceTable[i][0] = f * 0.299f;
		r_luminanceTable[i][1] = f * 0.587f;
		r_luminanceTable[i][2] = f * 0.114f;
	}

	// set texture parameters
	R_SetTextureParameters();
	R_InitBuiltinTextures();

	R_ParseTexFilters( "scripts/texfilter.txt" );
}

/*
===============
R_ShutdownImages
===============
*/
void R_ShutdownImages( void )
{
	gltexture_t	*image;
	int		i;

	if( !glw_state.initialized ) return;

	for( i = ( MAX_TEXTURE_UNITS - 1); i >= 0; i-- )
	{
		if( i >= GL_MaxTextureUnits( ))
			continue;

		GL_SelectTexture( i );
		pglBindTexture( GL_TEXTURE_2D, 0 );

		if( GL_Support( GL_TEXTURECUBEMAP_EXT ))
			pglBindTexture( GL_TEXTURE_CUBE_MAP_ARB, 0 );
	}

	for( i = 0, image = r_textures; i < r_numTextures; i++, image++ )
	{
		if( !image->texnum ) continue;
		GL_FreeTexture( i );
	}

	Q_memset( tr.lightmapTextures, 0, sizeof( tr.lightmapTextures ));
	Q_memset( r_texturesHashTable, 0, sizeof( r_texturesHashTable ));
	Q_memset( r_textures, 0, sizeof( r_textures ));
	r_numTextures = 0;
}