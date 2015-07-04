/*
vgui_draw.c - vgui draw methods
Copyright (C) 2011 Uncle Mike

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
#include "vgui_draw.h"
#include "library.h"


int	g_textures[VGUI_MAX_TEXTURES];
int	g_textureId = 0;
int	g_iBoundTexture;

// Helper functions for vgui backend

void VGUI_HideCursor( void )
{
	host.mouse_visible = false;
}

void VGUI_ShowCursor( void )
{
	host.mouse_visible = true;
}

void *VGUI_EngineMalloc(size_t size)
{
	return Z_Malloc( size );
}

qboolean VGUI_IsInGame()
{
	return true; // cls.state == ca_active && cls.key_dest == key_game ;
}

void VGUI_ActivateCurrentCursor( void )
{
	if( cls.key_dest != key_game || cl.refdef.paused )
		return;
#ifdef XASH_SDL
	if( host.mouse_visible )
	{
		SDL_ShowCursor( true );
	}
	else
	{
		SDL_ShowCursor( false );
	}
#endif
}

byte VGUI_GetColor( int i, int j)
{
	return g_color_table[i][j];
}

// Define and initialize vgui API

vguiapi_t vgui =
{
	false, //Not initialized yet
	VGUI_DrawInit,
	VGUI_DrawShutdown,
	VGUI_SetupDrawingText,
	VGUI_SetupDrawingRect,
	VGUI_SetupDrawingImage,
	VGUI_BindTexture,
	VGUI_EnableTexture,
	VGUI_CreateTexture,
	VGUI_UploadTexture,
	VGUI_UploadTextureBlock,
	VGUI_DrawQuad,
	VGUI_GetTextureSizes,
	VGUI_GenerateTexture,
	VGUI_EngineMalloc,
	VGUI_ShowCursor,
	VGUI_HideCursor,
	VGUI_ActivateCurrentCursor,
	VGUI_GetColor,
	VGUI_IsInGame,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
} ;


void *lib; //vgui_support library


/*
================
VGui_Startup

Load vgui_support library and call VGui_Startup
================
*/
void VGui_Startup( int width, int height )
{
	if(!vgui.initialized)
	{
		void (*F) (vguiapi_t *);
		lib = Com_LoadLibrary(host.vguiloader, false);
		if(!lib)
			MsgDev(D_ERROR, "Failed to load vgui_support library!\n");
		else
		{
			F = Com_GetProcAddress(lib, "F");
			if(F)
			{
				F(&vgui);
				vgui.initialized = true;
			}
			else
				MsgDev(D_ERROR, "Failed to find vgui_support library entry point!\n");
		}
	}
	if (vgui.initialized)
		vgui.Startup(width, height);
}



/*
================
VGui_Shutdown

Unload vgui_support library and call VGui_Shutdown
================
*/
void VGui_Shutdown( void )
{
	if( vgui.Shutdown) 
		vgui.Shutdown();
	if(lib)
		Com_FreeLibrary(lib);
}



/*
================
VGUI_DrawInit

Startup VGUI backend
================
*/
void VGUI_DrawInit( void )
{
	Q_memset( g_textures, 0, sizeof( g_textures ));
	g_textureId = g_iBoundTexture = 0;
}

/*
================
VGUI_DrawShutdown

Release all textures
================
*/
void VGUI_DrawShutdown( void )
{
	int	i;

	for( i = 1; i < g_textureId; i++ )
	{
		GL_FreeImage( va(  "*vgui%i", i ));
	}
}

/*
================
VGUI_GenerateTexture

generate unique texture number
================
*/
int VGUI_GenerateTexture( void )
{
	if( ++g_textureId >= VGUI_MAX_TEXTURES )
		Sys_Error( "VGUI_GenerateTexture: VGUI_MAX_TEXTURES limit exceeded\n" );
	return g_textureId;
}

/*
================
VGUI_UploadTexture

Upload texture into video memory
================
*/
void VGUI_UploadTexture( int id, const char *buffer, int width, int height )
{
	rgbdata_t	r_image;
	char	texName[32];

	if( id <= 0 || id >= VGUI_MAX_TEXTURES )
	{
		MsgDev( D_ERROR, "VGUI_UploadTexture: bad texture %i. Ignored\n", id );
		return;
	}

	Q_snprintf( texName, sizeof( texName ), "*vgui%i", id );
	Q_memset( &r_image, 0, sizeof( r_image ));

	r_image.width = width;
	r_image.height = height;
	r_image.type = PF_RGBA_32;
	r_image.size = r_image.width * r_image.height * 4;
	r_image.flags = IMAGE_HAS_COLOR|IMAGE_HAS_ALPHA;
	r_image.buffer = (byte *)buffer;

	g_textures[id] = GL_LoadTextureInternal( texName, &r_image, TF_IMAGE, false );
	GL_SetTextureType( g_textures[id], TEX_VGUI );
	g_iBoundTexture = id;
}

/*
================
VGUI_CreateTexture

Create empty rgba texture and upload them into video memory
================
*/
void VGUI_CreateTexture( int id, int width, int height )
{
	rgbdata_t	r_image;
	char	texName[32];

	if( id <= 0 || id >= VGUI_MAX_TEXTURES )
	{
		MsgDev( D_ERROR, "VGUI_CreateTexture: bad texture %i. Ignored\n", id );
		return;
	}

	Q_snprintf( texName, sizeof( texName ), "*vgui%i", id );
	Q_memset( &r_image, 0, sizeof( r_image ));

	r_image.width = width;
	r_image.height = height;
	r_image.type = PF_RGBA_32;
	r_image.size = r_image.width * r_image.height * 4;
	r_image.flags = IMAGE_HAS_ALPHA;
	r_image.buffer = NULL;

	g_textures[id] = GL_LoadTextureInternal( texName, &r_image, TF_IMAGE|TF_NEAREST, false );
	GL_SetTextureType( g_textures[id], TEX_VGUI );
	g_iBoundTexture = id;
}

void VGUI_UploadTextureBlock( int id, int drawX, int drawY, const byte *rgba, int blockWidth, int blockHeight )
{
	if( id <= 0 || id >= VGUI_MAX_TEXTURES || g_textures[id] == 0 || g_textures[id] == cls.fillImage )
	{
		MsgDev( D_ERROR, "VGUI_UploadTextureBlock: bad texture %i. Ignored\n", id );
		return;
	}

	pglTexSubImage2D( GL_TEXTURE_2D, 0, drawX, drawY, blockWidth, blockHeight, GL_RGBA, GL_UNSIGNED_BYTE, rgba );
	g_iBoundTexture = id;
}

void VGUI_SetupDrawingRect( int *pColor )
{
	pglEnable( GL_BLEND );
	pglDisable( GL_ALPHA_TEST );
	pglBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	pglColor4ub( pColor[0], pColor[1], pColor[2], 255 - pColor[3] );
}

void VGUI_SetupDrawingText( int *pColor )
{
	pglEnable( GL_BLEND );
	pglEnable( GL_ALPHA_TEST );
	pglAlphaFunc( GL_GREATER, 0.0f );
	pglBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	pglTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
	pglColor4ub( pColor[0], pColor[1], pColor[2], 255 - pColor[3] );
}

void VGUI_SetupDrawingImage( int *pColor )
{
	pglEnable( GL_BLEND );
	pglEnable( GL_ALPHA_TEST );
	pglAlphaFunc( GL_GREATER, 0.0f );
	pglBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	pglTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
	pglColor4ub( pColor[0], pColor[1], pColor[2], 255 - pColor[3] );
}

void VGUI_BindTexture( int id )
{
	if( id > 0 && id < VGUI_MAX_TEXTURES && g_textures[id] )
	{
		GL_Bind( GL_TEXTURE0, g_textures[id] );
		g_iBoundTexture = id;
	}
	else
	{
		// NOTE: same as bogus index 2700 in GoldSrc
		id = g_iBoundTexture = 1;
		GL_Bind( GL_TEXTURE0, g_textures[id] );
	}
}

/*
================
VGUI_GetTextureSizes

returns wide and tall for currently binded texture
================
*/
void VGUI_GetTextureSizes( int *width, int *height )
{
	gltexture_t	*glt;
	int		texnum;

	if( g_iBoundTexture )
		texnum = g_textures[g_iBoundTexture];
	else texnum = tr.defaultTexture;

	glt = R_GetTexture( texnum );
	if( width ) *width = glt->srcWidth;
	if( height ) *height = glt->srcHeight;
}

/*
================
VGUI_EnableTexture

disable texturemode for fill rectangle
================
*/
void VGUI_EnableTexture( qboolean enable )
{
	if( enable ) pglEnable( GL_TEXTURE_2D );
	else pglDisable( GL_TEXTURE_2D );
}

/*
================
VGUI_DrawQuad

generic method to fill rectangle
================
*/
void VGUI_DrawQuad( const vpoint_t *ul, const vpoint_t *lr )
{
	ASSERT( ul != NULL && lr != NULL );

	pglBegin( GL_QUADS );
		pglTexCoord2f( ul->coord[0], ul->coord[1] );
		pglVertex2f( ul->point[0], ul->point[1] );

		pglTexCoord2f( lr->coord[0], ul->coord[1] );
		pglVertex2f( lr->point[0], ul->point[1] );

		pglTexCoord2f( lr->coord[0], lr->coord[1] );
		pglVertex2f( lr->point[0], lr->point[1] );

		pglTexCoord2f( ul->coord[0], lr->coord[1] );
		pglVertex2f( ul->point[0], lr->point[1] );
	pglEnd();
}
