/*
vgui_draw.h - vgui draw methods
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

#ifndef VGUI_DRAW_H
#define VGUI_DRAW_H

#ifdef __cplusplus
extern "C" {
#endif

#include "port.h"

#define VGUI_MAX_TEXTURES	2048	// a half of total textures count

//#include "vgui_api.h"

#ifdef XASH_SDL
#include <SDL_events.h>
typedef SDL_Event Xash_Event;
#else
typedef void Xash_Event;
#endif

//extern vguiapi_t vgui;
/*
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
*/

//
// vgui_draw.c
//
void VGui_Startup( int width, int height );
void VGui_Shutdown( void );
void VGui_Paint();
void VGui_RunFrame();
void VGUI_SurfaceWndProc( Xash_Event *event );
void *VGui_GetPanel();
void VGui_ViewportPaintBackground();

#ifdef __cplusplus
}
#endif
#endif//VGUI_DRAW_H
