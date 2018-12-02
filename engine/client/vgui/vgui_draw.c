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

#ifndef XASH_DEDICATED

#include "common.h"
#include "client.h"
#include "gl_local.h"
#include "vgui_draw.h"
#include "vgui_api.h"
#include "library.h"
#include <string.h>
#include "../keydefs.h"

int	g_textures[VGUI_MAX_TEXTURES];
int	g_textureId = 0;
int	g_iBoundTexture;
static enum VGUI_KeyCode s_pVirtualKeyTrans[256];
static enum VGUI_DefaultCursor s_currentCursor;
#ifdef XASH_SDL
#include <SDL_events.h>
#include "platform/sdl/events.h"
static SDL_Cursor* s_pDefaultCursor[20];
#endif
static void *s_pVGuiSupport; // vgui_support library

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

void GAME_EXPORT *VGUI_EngineMalloc(size_t size)
{
	return Z_Malloc( size );
}

qboolean GAME_EXPORT VGUI_IsInGame( void )
{
	return cls.state == ca_active && cls.key_dest == key_game;
}

void GAME_EXPORT VGUI_GetMousePos( int *_x, int *_y )
{
	float xscale = scr_width->value / (float)clgame.scrInfo.iWidth;
	float yscale = scr_height->value / (float)clgame.scrInfo.iHeight;
	int x, y;

	CL_GetMousePosition( &x, &y );
	*_x = x / xscale, *_y = y / yscale;
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
	SDL_SetCursor( s_pDefaultCursor[dc_arrow] );
#endif
}

void GAME_EXPORT VGUI_CursorSelect(enum VGUI_DefaultCursor cursor )
{
	qboolean visible;
	if( cls.key_dest != key_game || cl.refdef.paused )
		return;
	
	switch( cursor )
	{
		case dc_user:
		case dc_none:
			visible = false;
			break;
		default:
		visible = true;
		break;
	}

#ifdef XASH_SDL
	if( host.mouse_visible )
	{
		SDL_SetRelativeMouseMode( SDL_FALSE );
		SDL_SetCursor( s_pDefaultCursor[cursor] );
		SDL_ShowCursor( true );
	}
	else
	{
		SDL_ShowCursor( false );
		if( host.mouse_visible )
			SDL_GetRelativeMouseState( NULL, NULL );
	}
	//SDL_SetRelativeMouseMode(false);
#endif
	if( s_currentCursor == cursor )
		return;

	s_currentCursor = cursor;
	host.mouse_visible = visible;
}

byte GAME_EXPORT VGUI_GetColor( int i, int j)
{
	return g_color_table[i][j];
}

// Define and initialize vgui API

void GAME_EXPORT VGUI_SetVisible( qboolean state )
{
	host.mouse_visible=state;
#ifdef XASH_SDL
	SDL_ShowCursor( state );
	if( !state )
		SDL_GetRelativeMouseState( NULL, NULL );

	SDLash_EnableTextInput( state, true );
#endif
}

int GAME_EXPORT VGUI_UtfProcessChar( int in )
{
	if( vgui_utf8->integer )
		return Con_UtfProcessCharForce( in );
	else
		return in;
}

vguiapi_t vgui =
{
	false, // Not initialized yet
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
	VGUI_UtfProcessChar,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
};

qboolean VGui_IsActive( void )
{
	return vgui.initialized;
}

/*
================
VGui_Startup

Load vgui_support library and call VGui_Startup
================
*/
void VGui_Startup( int width, int height )
{
	static qboolean failed = false;

	void (*F) ( vguiapi_t * );
	char vguiloader[256];
	char vguilib[256];

	vguiloader[0] = vguilib[0] = '\0';

	if( failed )
		return;

	if( !vgui.initialized )
	{
#ifdef XASH_INTERNAL_GAMELIBS
		s_pVGuiSupport = Com_LoadLibrary( "client", false );

		if( s_pVGuiSupport )
		{
			F = Com_GetProcAddress( s_pVGuiSupport, "InitVGUISupportAPI" );
			if( F )
			{
				F( &vgui );
				vgui.initialized = true;
				VGUI_InitCursors();
				MsgDev( D_INFO, "vgui_support: found internal client support\n" );
			}
		}
#endif // XASH_INTERNAL_GAMELIBS

		Com_ResetLibraryError();

		// HACKHACK: load vgui with correct path first if specified.
		// it will be reused while resolving vgui support and client deps
		if( Sys_GetParmFromCmdLine( "-vguilib", vguilib ) )
		{
			if( Q_strstr( vguilib, ".dll") )
				Q_strncpy( vguiloader, "vgui_support.dll", 256 );
			else
				Q_strncpy( vguiloader, VGUI_SUPPORT_DLL, 256 );

			if( !Com_LoadLibrary( vguilib, false ) )
				MsgDev( D_WARN, "VGUI preloading failed. Default library will be used!\n");
		}

		if( Q_strstr( GI->client_lib, ".dll" ) )
			Q_strncpy( vguiloader, "vgui_support.dll", 256 );

		if( !vguiloader[0] && !Sys_GetParmFromCmdLine( "-vguiloader", vguiloader ) )
			Q_strncpy( vguiloader, VGUI_SUPPORT_DLL, 256 );

		s_pVGuiSupport = Com_LoadLibrary( vguiloader, false );

		if( !s_pVGuiSupport )
		{
			s_pVGuiSupport = Com_LoadLibrary( va( "../%s", vguiloader ), false );
		}

		if( !s_pVGuiSupport )
		{
			if( FS_SysFileExists( vguiloader, false ) )
				MsgDev( D_ERROR, "Failed to load vgui_support library: %s", Com_GetLibraryError() );
			else
				MsgDev( D_INFO, "vgui_support: not found\n" );
		}
		else
		{
			F = Com_GetProcAddress( s_pVGuiSupport, "InitAPI" );
			if( F )
			{
				F( &vgui );
				vgui.initialized = true;
				VGUI_InitCursors();
			}
			else
				MsgDev( D_ERROR, "Failed to find vgui_support library entry point!\n" );
		}

	}

	if( height < 480 )
		height = 480;

	if( width <= 640 )
		width = 640;
	else if( width <= 800 )
		width = 800;
	else if( width <= 1024 )
		width = 1024;
	else if( width <= 1152 )
		width = 1152;
	else if( width <= 1280 )
		width = 1280;
	else if( width <= 1600 )
		width = 1600;
#ifdef DLL_LOADER
	else if ( Q_strstr( vguiloader, ".dll" ) )
		width = 1600;
#endif


	if( vgui.initialized )
	{
		//host.mouse_visible = true;
		vgui.Startup( width, height );
	}
	else failed = true;
}



/*
================
VGui_Shutdown

Unload vgui_support library and call VGui_Shutdown
================
*/
void VGui_Shutdown( void )
{
	if( vgui.Shutdown )
		vgui.Shutdown();

	if( s_pVGuiSupport )
		Com_FreeLibrary( s_pVGuiSupport );
	s_pVGuiSupport = NULL;

	vgui.initialized = false;
}


void VGUI_InitKeyTranslationTable( void )
{
	static qboolean bInitted = false;

	if( bInitted )
		return;
	bInitted = true;

	// set virtual key translation table
	Q_memset( s_pVirtualKeyTrans, -1, sizeof( s_pVirtualKeyTrans ) );

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

	s_pVirtualKeyTrans[K_KP_5 - 5] = KEY_PAD_0;
	s_pVirtualKeyTrans[K_KP_5 - 4] = KEY_PAD_1;
	s_pVirtualKeyTrans[K_KP_5 - 3] = KEY_PAD_2;
	s_pVirtualKeyTrans[K_KP_5 - 2] = KEY_PAD_3;
	s_pVirtualKeyTrans[K_KP_5 - 1] = KEY_PAD_4;
	s_pVirtualKeyTrans[K_KP_5 - 0] = KEY_PAD_5;
	s_pVirtualKeyTrans[K_KP_5 + 1] = KEY_PAD_6;
	s_pVirtualKeyTrans[K_KP_5 + 2] = KEY_PAD_7;
	s_pVirtualKeyTrans[K_KP_5 + 3] = KEY_PAD_8;
	s_pVirtualKeyTrans[K_KP_5 + 4] = KEY_PAD_9;
	s_pVirtualKeyTrans[K_KP_SLASH]	= KEY_PAD_DIVIDE;
	s_pVirtualKeyTrans['*'] = KEY_PAD_MULTIPLY;
	s_pVirtualKeyTrans[K_KP_MINUS] = KEY_PAD_MINUS;
	s_pVirtualKeyTrans[K_KP_PLUS] = KEY_PAD_PLUS;
	s_pVirtualKeyTrans[K_KP_ENTER]	= KEY_PAD_ENTER;
	//s_pVirtualKeyTrans[K_KP_DECIMAL] = KEY_PAD_DECIMAL;
	s_pVirtualKeyTrans['['] = KEY_LBRACKET;
	s_pVirtualKeyTrans[']'] = KEY_RBRACKET;
	s_pVirtualKeyTrans[';'] = KEY_SEMICOLON;
	s_pVirtualKeyTrans['\''] = KEY_APOSTROPHE;
	s_pVirtualKeyTrans['`'] = KEY_BACKQUOTE;
	s_pVirtualKeyTrans[','] = KEY_COMMA;
	s_pVirtualKeyTrans['.'] = KEY_PERIOD;
	s_pVirtualKeyTrans[K_KP_SLASH] = KEY_SLASH;
	s_pVirtualKeyTrans['\\'] = KEY_BACKSLASH;
	s_pVirtualKeyTrans['-'] = KEY_MINUS;
	s_pVirtualKeyTrans['='] = KEY_EQUAL;
	s_pVirtualKeyTrans[K_ENTER]	= KEY_ENTER;
	s_pVirtualKeyTrans[K_SPACE] = KEY_SPACE;
	s_pVirtualKeyTrans[K_BACKSPACE] = KEY_BACKSPACE;
	s_pVirtualKeyTrans[K_TAB] = KEY_TAB;
	s_pVirtualKeyTrans[K_CAPSLOCK] = KEY_CAPSLOCK;
	s_pVirtualKeyTrans[K_KP_NUMLOCK] = KEY_NUMLOCK;
	s_pVirtualKeyTrans[K_ESCAPE]	= KEY_ESCAPE;
	//s_pVirtualKeyTrans[K_KP_SCROLLLOCK]	= KEY_SCROLLLOCK;
	s_pVirtualKeyTrans[K_INS]	= KEY_INSERT;
	s_pVirtualKeyTrans[K_DEL]	= KEY_DELETE;
	s_pVirtualKeyTrans[K_HOME] = KEY_HOME;
	s_pVirtualKeyTrans[K_END] = KEY_END;
	s_pVirtualKeyTrans[K_PGUP] = KEY_PAGEUP;
	s_pVirtualKeyTrans[K_PGDN] = KEY_PAGEDOWN;
	s_pVirtualKeyTrans[K_PAUSE] = KEY_BREAK;
	//s_pVirtualKeyTrans[K_SHIFT] = KEY_RSHIFT;
	s_pVirtualKeyTrans[K_SHIFT] = KEY_LSHIFT;	// SHIFT -> left SHIFT
	//s_pVirtualKeyTrans[SDLK_RALT] = KEY_RALT;
	s_pVirtualKeyTrans[K_ALT] = KEY_LALT;		// ALT -> left ALT
	//s_pVirtualKeyTrans[SDLK_RCTRL] = KEY_RCONTROL;
	s_pVirtualKeyTrans[K_CTRL] = KEY_LCONTROL;	// CTRL -> left CTRL
	s_pVirtualKeyTrans[K_WIN] = KEY_LWIN;
	//s_pVirtualKeyTrans[SDLK_APPLICATION] = KEY_RWIN;
	//s_pVirtualKeyTrans[K_WIN] = KEY_APP;
	s_pVirtualKeyTrans[K_UPARROW] = KEY_UP;
	s_pVirtualKeyTrans[K_LEFTARROW] = KEY_LEFT;
	s_pVirtualKeyTrans[K_DOWNARROW] = KEY_DOWN;
	s_pVirtualKeyTrans[K_RIGHTARROW] = KEY_RIGHT;
	s_pVirtualKeyTrans[K_F1] = KEY_F1;
	s_pVirtualKeyTrans[K_F2] = KEY_F2;
	s_pVirtualKeyTrans[K_F3] = KEY_F3;
	s_pVirtualKeyTrans[K_F4] = KEY_F4;
	s_pVirtualKeyTrans[K_F5] = KEY_F5;
	s_pVirtualKeyTrans[K_F6] = KEY_F6;
	s_pVirtualKeyTrans[K_F7] = KEY_F7;
	s_pVirtualKeyTrans[K_F8] = KEY_F8;
	s_pVirtualKeyTrans[K_F9] = KEY_F9;
	s_pVirtualKeyTrans[K_F10] = KEY_F10;
	s_pVirtualKeyTrans[K_F11] = KEY_F11;
	s_pVirtualKeyTrans[K_F12] = KEY_F12;
}

enum VGUI_KeyCode VGUI_MapKey( int keyCode )
{
	VGUI_InitKeyTranslationTable();

	if( keyCode < 0 || keyCode >= (int)sizeof( s_pVirtualKeyTrans ) / (int)sizeof( s_pVirtualKeyTrans[0] ))
	{
		//Assert( false );
		return (enum VGUI_KeyCode)-1;
	}
	else
	{
		return s_pVirtualKeyTrans[keyCode];
	}
}

void VGui_KeyEvent( int key, int down )
{
	if( !vgui.initialized )
		return;

#ifdef XASH_SDL
	if( host.mouse_visible )
		SDLash_EnableTextInput( 1, false );
#endif

	switch( key )
	{
	case K_MOUSE1:
		vgui.Mouse( down?MA_PRESSED:MA_RELEASED, MOUSE_LEFT );
		return;
	case K_MOUSE2:
		vgui.Mouse( down?MA_PRESSED:MA_RELEASED, MOUSE_RIGHT );
		return;
	case K_MOUSE3:
		vgui.Mouse( down?MA_PRESSED:MA_RELEASED, MOUSE_MIDDLE );
		return;
	case K_MWHEELDOWN:
		vgui.Mouse( MA_WHEEL, 1 );
		return;
	case K_MWHEELUP:
		vgui.Mouse( MA_WHEEL, -1 );
		return;
	default:
		break;
	}

	if( down == 2 )
		vgui.Key( KA_TYPED, VGUI_MapKey( key ) );
	else
		vgui.Key( down?KA_PRESSED:KA_RELEASED, VGUI_MapKey( key ) );
	//Msg("VGui_KeyEvent %d %d %d\n", key, VGUI_MapKey( key ), down );
}

void VGui_MouseMove( int x, int y )
{
	float xscale = scr_width->value / (float)clgame.scrInfo.iWidth;
	float yscale = scr_height->value / (float)clgame.scrInfo.iHeight;
	if( vgui.initialized )
		vgui.MouseMove( x / xscale, y / yscale );
}

/*
================
VGUI_DrawInit

Startup VGUI backend
================
*/
void GAME_EXPORT VGUI_DrawInit( void )
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
void GAME_EXPORT VGUI_DrawShutdown( void )
{
	int	i;

	for( i = 1; i < g_textureId; i++ )
	{
		GL_FreeImage( va( "*vgui%i", i ));
	}
}

/*
================
VGUI_GenerateTexture

generate unique texture number
================
*/
int GAME_EXPORT VGUI_GenerateTexture( void )
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
void GAME_EXPORT VGUI_UploadTexture( int id, const char *buffer, int width, int height )
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
void GAME_EXPORT VGUI_CreateTexture( int id, int width, int height )
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

void GAME_EXPORT VGUI_UploadTextureBlock( int id, int drawX, int drawY, const byte *rgba, int blockWidth, int blockHeight )
{
	if( id <= 0 || id >= VGUI_MAX_TEXTURES || g_textures[id] == 0 || g_textures[id] == cls.fillImage )
	{
		MsgDev( D_ERROR, "VGUI_UploadTextureBlock: bad texture %i. Ignored\n", id );
		return;
	}

	pglTexSubImage2D( GL_TEXTURE_2D, 0, drawX, drawY, blockWidth, blockHeight, GL_RGBA, GL_UNSIGNED_BYTE, rgba );
	g_iBoundTexture = id;
}

void GAME_EXPORT VGUI_SetupDrawingRect( int *pColor )
{
	pglEnable( GL_BLEND );
	pglDisable( GL_ALPHA_TEST );
	pglBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	pglColor4ub( pColor[0], pColor[1], pColor[2], 255 - pColor[3] );
}

void GAME_EXPORT VGUI_SetupDrawingText( int *pColor )
{
	pglEnable( GL_BLEND );
	pglEnable( GL_ALPHA_TEST );
	pglAlphaFunc( GL_GREATER, 0.0f );
	pglBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	pglTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
	pglColor4ub( pColor[0], pColor[1], pColor[2], 255 - pColor[3] );
}

void GAME_EXPORT VGUI_SetupDrawingImage( int *pColor )
{
	pglEnable( GL_BLEND );
	pglEnable( GL_ALPHA_TEST );
	pglAlphaFunc( GL_GREATER, 0.0f );
	pglBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	pglTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
	pglColor4ub( pColor[0], pColor[1], pColor[2], 255 - pColor[3] );
}

void GAME_EXPORT VGUI_BindTexture( int id )
{
	if( id > 0 && id < VGUI_MAX_TEXTURES && g_textures[id] )
	{
		GL_Bind( XASH_TEXTURE0, g_textures[id] );
		g_iBoundTexture = id;
	}
	else
	{
		// NOTE: same as bogus index 2700 in GoldSrc
		id = g_iBoundTexture = 1;
		GL_Bind( XASH_TEXTURE0, g_textures[id] );
	}
}

/*
================
VGUI_GetTextureSizes

returns wide and tall for currently binded texture
================
*/
void GAME_EXPORT VGUI_GetTextureSizes( int *width, int *height )
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
void GAME_EXPORT VGUI_EnableTexture( qboolean enable )
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
void GAME_EXPORT VGUI_DrawQuad( const vpoint_t *ul, const vpoint_t *lr )
{
	float xscale = scr_width->value / (float)clgame.scrInfo.iWidth;
	float yscale = scr_height->value / (float)clgame.scrInfo.iHeight;

	ASSERT( ul != NULL && lr != NULL );

	pglBegin( GL_QUADS );
		pglTexCoord2f( ul->coord[0], ul->coord[1] );
		pglVertex2f( ul->point[0] * xscale, ul->point[1] * yscale );

		pglTexCoord2f( lr->coord[0], ul->coord[1] );
		pglVertex2f( lr->point[0] * xscale, ul->point[1] * yscale );

		pglTexCoord2f( lr->coord[0], lr->coord[1] );
		pglVertex2f( lr->point[0] * xscale, lr->point[1] * yscale );

		pglTexCoord2f( ul->coord[0], lr->coord[1] );
		pglVertex2f( ul->point[0] * xscale, lr->point[1] * yscale );
	pglEnd();
}

void VGui_Paint()
{
	if(vgui.initialized)
		vgui.Paint();
}

void VGui_RunFrame()
{
	//stub
}


void *GAME_EXPORT pfnVGui_GetPanel()
{
	if( vgui.initialized )
		return vgui.GetPanel();
	return NULL;
}
#endif
