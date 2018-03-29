/*
vid_sdl.c - SDL vid component
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
#if XASH_VIDEO == VIDEO_SDL
#include "client.h"
#include "gl_local.h"
#include "mod_local.h"
#include "input.h"
#include "gl_vidnt.h"
#include <SDL.h>
#include <SDL_syswm.h>

typedef enum
{
	rserr_ok,
	rserr_invalid_fullscreen,
	rserr_invalid_mode,
	rserr_unknown
} rserr_t;

void VID_StartupGamma( void )
{
	BuildGammaTable( vid_gamma->value, vid_texgamma->value );
	MsgDev( D_NOTE, "VID_StartupGamma: software gamma initialized\n" );
}

void R_ChangeDisplaySettingsFast( int width, int height );

void *SDL_GetVideoDevice( void );

#if 0
#ifdef _WIN32
#define XASH_SDL_WINDOW_RECREATE
#elif defined XASH_X11
#define XASH_SDL_USE_FAKEWND
#endif
#endif

#ifdef XASH_SDL_USE_FAKEWND

SDL_Window *fakewnd;

qboolean VID_SetScreenResolution( int width, int height )
{
	SDL_DisplayMode want, got;

	want.w = width;
	want.h = height;
	want.driverdata = NULL;
	want.format = want.refresh_rate = 0; // don't care

	if( !SDL_GetClosestDisplayMode(0, &want, &got) )
		return false;

	MsgDev(D_NOTE, "Got closest display mode: %ix%i@%i\n", got.w, got.h, got.refresh_rate);

	if( fakewnd )
		SDL_DestroyWindow( fakewnd );

	fakewnd = SDL_CreateWindow("fakewnd", SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED, got.h, got.w, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_BORDERLESS | SDL_WINDOW_INPUT_GRABBED | SDL_WINDOW_FULLSCREEN );

	if( !fakewnd )
		return false;

	if( SDL_SetWindowDisplayMode( fakewnd, &got) == -1 )
		return false;

	//SDL_ShowWindow( fakewnd );
	if( SDL_SetWindowFullscreen( fakewnd, SDL_WINDOW_FULLSCREEN) == -1 )
		return false;
	SDL_SetWindowBordered( host.hWnd, SDL_FALSE );
	SDL_SetWindowPosition( host.hWnd, 0, 0 );
	SDL_SetWindowGrab( host.hWnd, SDL_TRUE );
	SDL_HideWindow( host.hWnd );
	SDL_ShowWindow( host.hWnd );
	SDL_SetWindowSize( host.hWnd, got.w, got.h );

	SDL_GL_GetDrawableSize( host.hWnd, &got.w, &got.h );

	R_ChangeDisplaySettingsFast( got.w, got.h );
	SDL_HideWindow( fakewnd );
	return true;
}

void VID_RestoreScreenResolution( void )
{
	if( fakewnd )
	{

		SDL_ShowWindow( fakewnd );
		SDL_DestroyWindow( fakewnd );
	}
	fakewnd = NULL;
	if( !Cvar_VariableInteger("fullscreen") )
	{
		SDL_SetWindowBordered( host.hWnd, SDL_TRUE );
		//SDL_SetWindowPosition( host.hWnd, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED  );
		SDL_SetWindowGrab( host.hWnd, SDL_FALSE );
	}
	else
	{
		SDL_MinimizeWindow( host.hWnd );
	}
}
#else

static qboolean recreate = false;
qboolean VID_SetScreenResolution( int width, int height )
{
	SDL_DisplayMode want, got;
	Uint32 wndFlags = 0;
	static string wndname;

	if( vid_highdpi->integer ) wndFlags |= SDL_WINDOW_ALLOW_HIGHDPI;
	Q_strncpy( wndname, GI->title, sizeof( wndname ));

	want.w = width;
	want.h = height;
	want.driverdata = NULL;
	want.format = want.refresh_rate = 0; // don't care

	if( !SDL_GetClosestDisplayMode(0, &want, &got) )
		return false;

	MsgDev(D_NOTE, "Got closest display mode: %ix%i@%i\n", got.w, got.h, got.refresh_rate);

#ifdef XASH_SDL_WINDOW_RECREATE
	if( recreate )
	{
		SDL_DestroyWindow( host.hWnd );
		host.hWnd = SDL_CreateWindow(wndname, 0, 0, width, height, wndFlags | SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_BORDERLESS | SDL_WINDOW_INPUT_GRABBED );
		SDL_GL_MakeCurrent( host.hWnd, glw_state.context );
		recreate = false;
	}
#endif

	if( SDL_SetWindowDisplayMode( host.hWnd, &got) == -1 )
		return false;

	if( SDL_SetWindowFullscreen( host.hWnd, SDL_WINDOW_FULLSCREEN) == -1 )
		return false;
	SDL_SetWindowBordered( host.hWnd, SDL_FALSE );
	//SDL_SetWindowPosition( host.hWnd, 0, 0 );
	SDL_SetWindowGrab( host.hWnd, SDL_TRUE );
	SDL_SetWindowSize( host.hWnd, got.w, got.h );

	SDL_GL_GetDrawableSize( host.hWnd, &got.w, &got.h );

	R_ChangeDisplaySettingsFast( got.w, got.h );
	return true;
}

void VID_RestoreScreenResolution( void )
{
	if( !Cvar_VariableInteger("fullscreen") )
	{
		SDL_SetWindowBordered( host.hWnd, SDL_TRUE );
		//SDL_SetWindowPosition( host.hWnd, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED  );
		SDL_SetWindowGrab( host.hWnd, SDL_FALSE );
	}
	else
	{
		SDL_MinimizeWindow( host.hWnd );
		SDL_SetWindowFullscreen( host.hWnd, 0 );
		recreate = true;
	}
}
#endif

#if defined(_WIN32) && !defined(XASH_64BIT) // ICO support only for Win32
static void WIN_SetWindowIcon( HICON ico )
{
	SDL_SysWMinfo wminfo;

	if( !ico )
		return;

	if( SDL_GetWindowWMInfo( host.hWnd, &wminfo ) )
	{
		SetClassLong( wminfo.info.win.window, GCL_HICON, (LONG)ico );
	}
}
#endif

qboolean VID_CreateWindow( int width, int height, qboolean fullscreen )
{
	static string	wndname;
	Uint32 wndFlags = SDL_WINDOW_OPENGL;
	rgbdata_t *icon = NULL;
	char iconpath[MAX_STRING];

	if( vid_highdpi->integer ) wndFlags |= SDL_WINDOW_ALLOW_HIGHDPI;
	Q_strncpy( wndname, GI->title, sizeof( wndname ));

	if( !fullscreen )
	{
#ifndef XASH_SDL_DISABLE_RESIZE
		wndFlags |= SDL_WINDOW_RESIZABLE;
#endif
		host.hWnd = SDL_CreateWindow( wndname, r_xpos->integer,
			r_ypos->integer, width, height, wndFlags | SDL_WINDOW_MOUSE_FOCUS );
	}
	else
	{
		host.hWnd = SDL_CreateWindow( wndname, 0, 0, width, height, wndFlags | SDL_WINDOW_FULLSCREEN | SDL_WINDOW_SHOWN | SDL_WINDOW_BORDERLESS | SDL_WINDOW_INPUT_GRABBED );
	}


	if( !host.hWnd )
	{
		MsgDev( D_ERROR, "VID_CreateWindow: couldn't create '%s': %s\n", wndname, SDL_GetError());

		// remove MSAA, if it present, because
		// window creating may fail on GLX visual choose
		if( gl_msaa->integer )
		{
			Cvar_Set("gl_msaa", "0");
			GL_SetupAttributes(); // re-choose attributes

			// try again
			return VID_CreateWindow( width, height, fullscreen );
		}
		return false;
	}

	if( fullscreen )
	{
		if( !VID_SetScreenResolution( width, height ) )
		{
			return false;
		}
	}
	else
	{
		VID_RestoreScreenResolution();
	}

#if defined(_WIN32) && !defined(XASH_64BIT) // ICO support only for Win32
	if( FS_FileExists( GI->iconpath, true ) )
	{
		HICON ico;
		char	localPath[MAX_PATH];

		Q_snprintf( localPath, sizeof( localPath ), "%s/%s", GI->gamefolder, GI->iconpath );
		ico = (HICON)LoadImage( NULL, localPath, IMAGE_ICON, 0, 0, LR_LOADFROMFILE|LR_DEFAULTSIZE );

		if( !ico )
		{
			MsgDev( D_INFO, "Extract %s from pak if you want to see it.\n", GI->iconpath );
			ico = LoadIcon( host.hInst, MAKEINTRESOURCE( 101 ) );
		}

		WIN_SetWindowIcon( ico );
	}
	else
#endif // _WIN32 && !XASH_64BIT
	{
		Q_strcpy( iconpath, GI->iconpath );
		FS_StripExtension( iconpath );
		FS_DefaultExtension( iconpath, ".tga") ;

		icon = FS_LoadImage( iconpath, NULL, 0 );

		if( icon )
		{
			SDL_Surface *surface = SDL_CreateRGBSurfaceFrom( icon->buffer,
				icon->width, icon->height, 32, 4 * icon->width,
				0x000000ff, 0x0000ff00, 0x00ff0000,	0xff000000 );

			if( surface )
			{
				SDL_SetWindowIcon( host.hWnd, surface );
				SDL_FreeSurface( surface );
			}

			FS_FreeImage( icon );
		}
#if defined(_WIN32) && !defined(XASH_64BIT) // ICO support only for Win32
		else
		{
			WIN_SetWindowIcon( LoadIcon( host.hInst, MAKEINTRESOURCE( 101 ) ) );
		}
#endif
	}

	SDL_ShowWindow( host.hWnd );
	if( !glw_state.initialized )
	{
		if( !GL_CreateContext( ))
		{
			return false;
		}

		VID_StartupGamma();
	}
	else
	{
		if( !GL_UpdateContext( ))
			return false;		
	}

	SDL_GL_GetDrawableSize( host.hWnd, &width, &height );
	R_ChangeDisplaySettingsFast( width, height );

	return true;
}

void VID_DestroyWindow( void )
{
	GL_DeleteContext();

	VID_RestoreScreenResolution();
	if( host.hWnd )
	{
		SDL_DestroyWindow ( host.hWnd );
		host.hWnd = NULL;
	}

	if( glState.fullScreen )
	{
		glState.fullScreen = false;
	}
}


/*
==================
R_ChangeDisplaySettingsFast

Change window size fastly to custom values, without setting vid mode
==================
*/
void R_ChangeDisplaySettingsFast( int width, int height )
{
	//Cvar_SetFloat("vid_mode", VID_NOMODE);
	Cvar_SetFloat("width", width);
	Cvar_SetFloat("height", height);

	// as we don't recreate window here, update center positions by hand
	host.window_center_x = glState.width / 2;
	host.window_center_y = glState.height / 2;

	if( glState.width != width || glState.height != height )
	{
		glState.width = width;
		glState.height = height;
		if( width * 3 != height * 4 && width * 4 != height * 5 )
			glState.wideScreen = true;
		else glState.wideScreen = false;

		SCR_VidInit();
	}
}

rserr_t R_ChangeDisplaySettings( int width, int height, qboolean fullscreen )
{
	SDL_DisplayMode displayMode;

	SDL_GetCurrentDisplayMode(0, &displayMode);
#ifdef XASH_NOMODESWITCH
	width = displayMode.w;
	height = displayMode.h;
	fullscreen = false;
#endif

	MsgDev(D_INFO, "R_ChangeDisplaySettings: Setting video mode to %dx%d %s\n", width, height, fullscreen ? "fullscreen" : "windowed");
	R_SaveVideoMode( width, height );

	// check our desktop attributes
	glw_state.desktopBitsPixel = SDL_BITSPERPIXEL(displayMode.format);
	glw_state.desktopWidth = displayMode.w;
	glw_state.desktopHeight = displayMode.h;

	glState.fullScreen = fullscreen;

	// check for 4:3 or 5:4
	if( width * 3 != height * 4 && width * 4 != height * 5 )
		glState.wideScreen = true;
	else glState.wideScreen = false;


	if(!host.hWnd)
	{
		if( !VID_CreateWindow( width, height, fullscreen ) )
			return rserr_invalid_mode;
	}
#ifndef XASH_NOMODESWITCH
	else if( fullscreen )
	{
		if( !VID_SetScreenResolution( width, height ) )
			return rserr_invalid_fullscreen;
	}
	else
	{
		VID_RestoreScreenResolution();
		if( SDL_SetWindowFullscreen(host.hWnd, 0) )
			return rserr_invalid_fullscreen;
		SDL_RestoreWindow( host.hWnd );
#if SDL_VERSION_ATLEAST( 2, 0, 5 )
		SDL_SetWindowResizable( host.hWnd, true );
#endif
		SDL_SetWindowBordered( host.hWnd, true );
		SDL_SetWindowSize( host.hWnd, width, height );
		SDL_GL_GetDrawableSize( host.hWnd, &width, &height );
		R_ChangeDisplaySettingsFast( width, height );
	}
#endif

	return rserr_ok;
}

/*
==================
VID_SetMode

Set the described video mode
==================
*/
qboolean VID_SetMode( void )
{
	qboolean	fullscreen = false;
	int iScreenWidth, iScreenHeight;
	rserr_t	err;

	if( vid_mode->integer == -1 )	// trying to get resolution automatically by default
	{
		SDL_DisplayMode mode;
#if !defined DEFAULT_MODE_WIDTH || !defined DEFAULT_MODE_HEIGHT
		SDL_GetDesktopDisplayMode( 0, &mode );
#else
		mode.w = DEFAULT_MODE_WIDTH;
		mode.h = DEFAULT_MODE_HEIGHT;
#endif

		iScreenWidth = mode.w;
		iScreenHeight = mode.h;

		if( !vid_fullscreen->modified )
			Cvar_SetFloat( "fullscreen", DEFAULT_FULLSCREEN );
		vid_fullscreen->modified = false;
	}
	else if( vid_mode->modified && vid_mode->integer >= 0 && vid_mode->integer <= num_vidmodes )
	{
		iScreenWidth = vidmode[vid_mode->integer].width;
		iScreenHeight = vidmode[vid_mode->integer].height;
	}
	else
	{
		iScreenHeight = scr_height->integer;
		iScreenWidth = scr_width->integer;
	}

	gl_swapInterval->modified = true;
	fullscreen = Cvar_VariableInteger("fullscreen") != 0;

	if(( err = R_ChangeDisplaySettings( iScreenWidth, iScreenHeight, fullscreen )) == rserr_ok )
	{
		glConfig.prev_width = iScreenWidth;
		glConfig.prev_height = iScreenHeight;
	}
	else
	{
		if( err == rserr_invalid_fullscreen )
		{
			Cvar_SetFloat( "fullscreen", 0 );
			MsgDev( D_ERROR, "VID_SetMode: fullscreen unavailable in this mode\n" );
			Sys_Warn("fullscreen unavailable in this mode!");
			if(( err = R_ChangeDisplaySettings( iScreenWidth, iScreenHeight, false )) == rserr_ok )
				return true;
		}
		else if( err == rserr_invalid_mode )
		{
			Cvar_SetFloat( "vid_mode", glConfig.prev_mode );
			MsgDev( D_ERROR, "VID_SetMode: invalid mode\n" );
			Sys_Warn("invalid mode");
		}

		// try setting it back to something safe
		if(( err = R_ChangeDisplaySettings( glConfig.prev_width, glConfig.prev_height, false )) != rserr_ok )
		{
			MsgDev( D_ERROR, "VID_SetMode: could not revert to safe mode\n" );
			Sys_Warn("could not revert to safe mode!");
			return false;
		}
	}
	return true;
}

#endif // XASH_VIDEO
