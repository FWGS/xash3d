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

#ifdef XASH_VGUI

#include "common.h"
#include "client.h"
#include "gl_local.h"
#include "vgui_draw.h"
#include "vgui_api.h"
#include "library.h"
#include <string.h>


int	g_textures[VGUI_MAX_TEXTURES];
int	g_textureId = 0;
int	g_iBoundTexture;
static enum VGUI_KeyCode s_pVirtualKeyTrans[256];
#ifdef XASH_SDL
static SDL_Cursor* s_pDefaultCursor[20];
#endif

void VGUI_DrawInit( void );
void VGUI_DrawShutdown( void );
void VGUI_SetupDrawingText( int *pColor );
void VGUI_SetupDrawingRect( int *pColor );
void VGUI_SetupDrawingImage( int *pColor );
void VGUI_BindTexture( int id );
void VGUI_EnableTexture( qboolean enable );
void VGUI_CreateTexture( int id, int width, int height );
void VGUI_UploadTexture( int id, const char *buffer, int width, int height );
void VGUI_UploadTextureBlock( int id, int drawX, int drawY, const byte *rgba, int blockWidth, int blockHeight );
void VGUI_DrawQuad( const vpoint_t *ul, const vpoint_t *lr );
void VGUI_GetTextureSizes( int *width, int *height );
int VGUI_GenerateTexture( void );

// Helper functions for vgui backend

/*void VGUI_HideCursor( void )
{
	host.mouse_visible = false;
	SDL_HideCursor();
}

void VGUI_ShowCursor( void )
{
	host.mouse_visible = true;
	SDL_ShowCursor();
}*/

void *VGUI_EngineMalloc(size_t size)
{
	return Z_Malloc( size );
}

qboolean VGUI_IsInGame()
{
	return cls.state == ca_active && cls.key_dest == key_game;
}

void VGUI_GetMousePos( int *x, int *y )
{
	CL_GetMousePosition( x, y );
}

void VGUI_InitCursors( void )
{
	// load up all default cursors
#ifdef XASH_SDL
	s_pDefaultCursor[dc_none] = NULL;
	s_pDefaultCursor[dc_arrow] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW);
	s_pDefaultCursor[dc_ibeam] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_IBEAM);
	s_pDefaultCursor[dc_hourglass]= SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_WAIT);
	s_pDefaultCursor[dc_crosshair]= SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_CROSSHAIR);
	s_pDefaultCursor[dc_up] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW);
	s_pDefaultCursor[dc_sizenwse] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENWSE);
	s_pDefaultCursor[dc_sizenesw] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENESW);
	s_pDefaultCursor[dc_sizewe] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZEWE);
	s_pDefaultCursor[dc_sizens] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENS);
	s_pDefaultCursor[dc_sizeall] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZEALL);
	s_pDefaultCursor[dc_no] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_NO);
	s_pDefaultCursor[dc_hand] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_HAND);
	//host.mouse_visible = true;
	SDL_SetCursor(s_pDefaultCursor[dc_arrow]);
#endif
}

void VGUI_CursorSelect(enum VGUI_DefaultCursor cursor )
{
	qboolean oldstate = host.mouse_visible;
	if( cls.key_dest != key_game || cl.refdef.paused )
		return;
#ifdef XASH_SDL
	
	switch( cursor )
	{
		case dc_user:
		case dc_none:
			host.mouse_visible = false;
			break;
		default:
		host.mouse_visible = true;
		SDL_SetRelativeMouseMode( SDL_FALSE );
		SDL_SetCursor(s_pDefaultCursor[cursor]);
			//SDL_SetCursor(s_pDefaultCursor[dc_arrow]);
			break;
	}
	if( host.mouse_visible )
	{
		SDL_ShowCursor( true );
	}
	else
	{
		SDL_ShowCursor( false );
		if( oldstate )
			SDL_GetRelativeMouseState( 0, 0 );
	}
	//SDL_SetRelativeMouseMode(false);
#endif
}

byte VGUI_GetColor( int i, int j)
{
	return g_color_table[i][j];
}

// Define and initialize vgui API

void VGUI_SetVisible ( qboolean state )
{
	host.input_enabled=state;
	host.mouse_visible=state;
#ifdef XASH_SDL
	SDL_ShowCursor( state );
	if(!state) SDL_GetRelativeMouseState( 0, 0 );
#endif
}
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
/*	VGUI_ShowCursor,
	VGUI_HideCursor,*/
	VGUI_CursorSelect,
	VGUI_GetColor,
	VGUI_IsInGame,
	VGUI_SetVisible,
	VGUI_GetMousePos,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
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
			F = Com_GetProcAddress(lib, "InitAPI");
			if(F)
			{
				F(&vgui);
				vgui.initialized = true;
			}
			else
				MsgDev(D_ERROR, "Failed to find vgui_support library entry point!\n");
		}
		VGUI_InitCursors();
	}
	if (vgui.initialized)
	{
		//host.mouse_visible = true;
		vgui.Startup(width, height);
	}
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
	vgui.initialized = false;
}


void VGUI_InitKeyTranslationTable( void )
{
	static qboolean bInitted = false;

	if( bInitted ) return;
	bInitted = true;

	// set virtual key translation table
	memset( s_pVirtualKeyTrans, -1, sizeof( s_pVirtualKeyTrans ));	

	s_pVirtualKeyTrans['0'] = KEY_0;
	s_pVirtualKeyTrans['1'] = KEY_1;
	s_pVirtualKeyTrans['2'] = KEY_2;
	s_pVirtualKeyTrans['3'] = KEY_3;
	s_pVirtualKeyTrans['4'] = KEY_4;
	s_pVirtualKeyTrans['5'] = KEY_5;
	s_pVirtualKeyTrans['6'] = KEY_6;
	s_pVirtualKeyTrans['7'] = KEY_7;
	s_pVirtualKeyTrans['8'] = KEY_8;
	s_pVirtualKeyTrans['9'] = KEY_9;
	s_pVirtualKeyTrans['A'] = s_pVirtualKeyTrans['a'] = KEY_A;
	s_pVirtualKeyTrans['B'] = s_pVirtualKeyTrans['b'] = KEY_B;
	s_pVirtualKeyTrans['C'] = s_pVirtualKeyTrans['c'] = KEY_C;
	s_pVirtualKeyTrans['D'] = s_pVirtualKeyTrans['d'] = KEY_D;
	s_pVirtualKeyTrans['E'] = s_pVirtualKeyTrans['e'] = KEY_E;
	s_pVirtualKeyTrans['F'] = s_pVirtualKeyTrans['f'] = KEY_F;
	s_pVirtualKeyTrans['G'] = s_pVirtualKeyTrans['g'] = KEY_G;
	s_pVirtualKeyTrans['H'] = s_pVirtualKeyTrans['h'] = KEY_H;
	s_pVirtualKeyTrans['I'] = s_pVirtualKeyTrans['i'] = KEY_I;
	s_pVirtualKeyTrans['J'] = s_pVirtualKeyTrans['j'] = KEY_J;
	s_pVirtualKeyTrans['K'] = s_pVirtualKeyTrans['k'] = KEY_K;
	s_pVirtualKeyTrans['L'] = s_pVirtualKeyTrans['l'] = KEY_L;
	s_pVirtualKeyTrans['M'] = s_pVirtualKeyTrans['m'] = KEY_M;
	s_pVirtualKeyTrans['N'] = s_pVirtualKeyTrans['n'] = KEY_N;
	s_pVirtualKeyTrans['O'] = s_pVirtualKeyTrans['o'] = KEY_O;
	s_pVirtualKeyTrans['P'] = s_pVirtualKeyTrans['p'] = KEY_P;
	s_pVirtualKeyTrans['Q'] = s_pVirtualKeyTrans['q'] = KEY_Q;
	s_pVirtualKeyTrans['R'] = s_pVirtualKeyTrans['r'] = KEY_R;
	s_pVirtualKeyTrans['S'] = s_pVirtualKeyTrans['s'] = KEY_S;
	s_pVirtualKeyTrans['T'] = s_pVirtualKeyTrans['t'] = KEY_T;
	s_pVirtualKeyTrans['U'] = s_pVirtualKeyTrans['u'] = KEY_U;
	s_pVirtualKeyTrans['V'] = s_pVirtualKeyTrans['v'] = KEY_V;
	s_pVirtualKeyTrans['W'] = s_pVirtualKeyTrans['w'] = KEY_W;
	s_pVirtualKeyTrans['X'] = s_pVirtualKeyTrans['x'] = KEY_X;
	s_pVirtualKeyTrans['Y'] = s_pVirtualKeyTrans['y'] = KEY_Y;
	s_pVirtualKeyTrans['Z'] = s_pVirtualKeyTrans['z'] = KEY_Z;
#ifdef XASH_SDL
	s_pVirtualKeyTrans[SDLK_KP_0] = KEY_PAD_0;
	s_pVirtualKeyTrans[SDLK_KP_1] = KEY_PAD_1;
	s_pVirtualKeyTrans[SDLK_KP_2] = KEY_PAD_2;
	s_pVirtualKeyTrans[SDLK_KP_3] = KEY_PAD_3;
	s_pVirtualKeyTrans[SDLK_KP_4] = KEY_PAD_4;
	s_pVirtualKeyTrans[SDLK_KP_5] = KEY_PAD_5;
	s_pVirtualKeyTrans[SDLK_KP_6] = KEY_PAD_6;
	s_pVirtualKeyTrans[SDLK_KP_7] = KEY_PAD_7;
	s_pVirtualKeyTrans[SDLK_KP_8] = KEY_PAD_8;
	s_pVirtualKeyTrans[SDLK_KP_9] = KEY_PAD_9;
	s_pVirtualKeyTrans[SDLK_KP_DIVIDE]	= KEY_PAD_DIVIDE;
	s_pVirtualKeyTrans[SDLK_KP_MULTIPLY] = KEY_PAD_MULTIPLY;
	s_pVirtualKeyTrans[SDLK_KP_MINUS] = KEY_PAD_MINUS;
	s_pVirtualKeyTrans[SDLK_KP_PLUS] = KEY_PAD_PLUS;
	s_pVirtualKeyTrans[SDLK_KP_ENTER]	= KEY_PAD_ENTER;
	s_pVirtualKeyTrans[SDLK_KP_DECIMAL] = KEY_PAD_DECIMAL;
	s_pVirtualKeyTrans[SDLK_LEFTBRACKET] = KEY_LBRACKET;
	s_pVirtualKeyTrans[SDLK_RIGHTBRACKET] = KEY_RBRACKET;
	s_pVirtualKeyTrans[SDLK_SEMICOLON] = KEY_SEMICOLON;
	s_pVirtualKeyTrans[SDLK_QUOTE] = KEY_APOSTROPHE;
	s_pVirtualKeyTrans[SDLK_BACKQUOTE] = KEY_BACKQUOTE;
	s_pVirtualKeyTrans[SDLK_COMMA] = KEY_COMMA;
	s_pVirtualKeyTrans[SDLK_PERIOD] = KEY_PERIOD;
	s_pVirtualKeyTrans[SDLK_SLASH] = KEY_SLASH;
	s_pVirtualKeyTrans[SDLK_BACKSLASH] = KEY_BACKSLASH;
	s_pVirtualKeyTrans[SDLK_MINUS] = KEY_MINUS;
	s_pVirtualKeyTrans[SDLK_EQUALS] = KEY_EQUAL;
	s_pVirtualKeyTrans[SDLK_RETURN]	= KEY_ENTER;
	s_pVirtualKeyTrans[SDLK_SPACE] = KEY_SPACE;
	s_pVirtualKeyTrans[SDLK_BACKSPACE] = KEY_BACKSPACE;
	s_pVirtualKeyTrans[SDLK_TAB] = KEY_TAB;
	s_pVirtualKeyTrans[SDLK_CAPSLOCK] = KEY_CAPSLOCK;
	s_pVirtualKeyTrans[SDLK_NUMLOCKCLEAR] = KEY_NUMLOCK;
	s_pVirtualKeyTrans[SDLK_ESCAPE]	= KEY_ESCAPE;
	s_pVirtualKeyTrans[SDLK_SCROLLLOCK]	= KEY_SCROLLLOCK;
	s_pVirtualKeyTrans[SDLK_INSERT]	= KEY_INSERT;
	s_pVirtualKeyTrans[SDLK_DELETE]	= KEY_DELETE;
	s_pVirtualKeyTrans[SDLK_HOME] = KEY_HOME;
	s_pVirtualKeyTrans[SDLK_END] = KEY_END;
	s_pVirtualKeyTrans[SDLK_PRIOR] = KEY_PAGEUP;
	s_pVirtualKeyTrans[SDLK_PAGEDOWN] = KEY_PAGEDOWN;
	s_pVirtualKeyTrans[SDLK_PAUSE] = KEY_BREAK;
	s_pVirtualKeyTrans[SDLK_RSHIFT] = KEY_RSHIFT;
	s_pVirtualKeyTrans[SDLK_LSHIFT] = KEY_LSHIFT;	// SHIFT -> left SHIFT
	s_pVirtualKeyTrans[SDLK_RALT] = KEY_RALT;
	s_pVirtualKeyTrans[SDLK_LALT] = KEY_LALT;		// ALT -> left ALT
	s_pVirtualKeyTrans[SDLK_RCTRL] = KEY_RCONTROL;
	s_pVirtualKeyTrans[SDLK_LCTRL] = KEY_LCONTROL;	// CTRL -> left CTRL
	s_pVirtualKeyTrans[SDLK_APPLICATION] = KEY_LWIN;
	s_pVirtualKeyTrans[SDLK_APPLICATION] = KEY_RWIN;
	s_pVirtualKeyTrans[SDLK_APPLICATION] = KEY_APP;
	s_pVirtualKeyTrans[SDLK_UP] = KEY_UP;
	s_pVirtualKeyTrans[SDLK_LEFT] = KEY_LEFT;
	s_pVirtualKeyTrans[SDLK_DOWN] = KEY_DOWN;
	s_pVirtualKeyTrans[SDLK_RIGHT] = KEY_RIGHT;
	s_pVirtualKeyTrans[SDLK_F1] = KEY_F1;
	s_pVirtualKeyTrans[SDLK_F2] = KEY_F2;
	s_pVirtualKeyTrans[SDLK_F3] = KEY_F3;
	s_pVirtualKeyTrans[SDLK_F4] = KEY_F4;
	s_pVirtualKeyTrans[SDLK_F5] = KEY_F5;
	s_pVirtualKeyTrans[SDLK_F6] = KEY_F6;
	s_pVirtualKeyTrans[SDLK_F7] = KEY_F7;
	s_pVirtualKeyTrans[SDLK_F8] = KEY_F8;
	s_pVirtualKeyTrans[SDLK_F9] = KEY_F9;
	s_pVirtualKeyTrans[SDLK_F10] = KEY_F10;
	s_pVirtualKeyTrans[SDLK_F11] = KEY_F11;
	s_pVirtualKeyTrans[SDLK_F12] = KEY_F12;
#endif
}

enum VGUI_KeyCode VGUI_MapKey( int keyCode )
{
	VGUI_InitKeyTranslationTable();

	if( keyCode < 0 || keyCode >= sizeof( s_pVirtualKeyTrans ) / sizeof( s_pVirtualKeyTrans[0] ))
	{
		//Assert( false );
		return (enum VGUI_KeyCode)-1;
	}
	else
	{
		return s_pVirtualKeyTrans[keyCode];
	}
}


enum VGUI_MouseCode VGUI_MapMouseButton( byte button)
{
#ifdef XASH_SDL
	switch(button)
	{
	case SDL_BUTTON_LEFT:
		return MOUSE_LEFT;
	case SDL_BUTTON_MIDDLE:
		return MOUSE_MIDDLE;
	case SDL_BUTTON_RIGHT:
		return MOUSE_RIGHT;
	}
#else
	return (enum VGUI_MouseCode)button;
#endif
	return MOUSE_LAST; // What is MOUSE_LAST? Is it really used?
}



qboolean VGUI_SurfaceWndProc( Xash_Event *event )
{
/* When false returned, event passed to engine, else only to vgui*/
/* NOTE: this disabled because VGUI shows it's cursor on engine start*/
	if( !vgui.initialized )
		return;
#ifdef XASH_SDL
	switch( event->type )
	{
	/*case :
		VGUI_ActivateCurrentCursor();
		break;*/
	case SDL_MOUSEMOTION:
		vgui.MouseMove(event->motion.x, event->motion.y);
		return false;
	case SDL_MOUSEBUTTONDOWN:
		//if(event->button.clicks == 1)
		vgui.Mouse(MA_PRESSED, VGUI_MapMouseButton(event->button.button));
	//	else
	//	vgui.Mouse(MA_DOUBLE, VGUI_MapMouseButton(event->button.button));
		return true;
	case SDL_MOUSEBUTTONUP:
		vgui.Mouse(MA_RELEASED, VGUI_MapMouseButton(event->button.button));
		return true;
	case SDL_MOUSEWHEEL:
		vgui.Mouse(MA_WHEEL, event->wheel.y);
		return true;
	case SDL_KEYDOWN:
		if(!( event->key.keysym.sym & ( 1 << 30 )))
			vgui.Key( KA_PRESSED, VGUI_MapKey( event->key.keysym.sym ));
		vgui.Key( KA_TYPED, VGUI_MapKey( event->key.keysym.sym ));
		return false;
	case SDL_KEYUP:
		vgui.Key( KA_RELEASED, VGUI_MapKey( event->key.keysym.sym ) );
		return false;
	}
	
#endif
	return false;
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

void VGui_Paint()
{
	if(vgui.initialized)
		vgui.Paint();
}

void *VGui_GetPanel()
{
	if(vgui.initialized)
		return vgui.GetPanel();
	return 0;
}
void VGui_ViewportPaintBackground( int extents[4])
{
	//stub
}

void VGui_RunFrame()
{
	//stub
}
#endif
