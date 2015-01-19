/*
utils.h - draw helper
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

#ifndef UTILS_H
#define UTILS_H

extern ui_enginefuncs_t g_engfuncs;

#include "enginecallback.h"
#include "gameinfo.h"

#define FILE_GLOBAL	static
#define DLL_GLOBAL

#define MAX_INFO_STRING	256	// engine limit

#define RAD2DEG( x )	((float)(x) * (float)(180.f / M_PI))
#define DEG2RAD( x )	((float)(x) * (float)(M_PI / 180.f))

//
// How did I ever live without ASSERT?
//
#ifdef _DEBUG
void DBG_AssertFunction( BOOL fExpr, const char* szExpr, const char* szFile, int szLine, const char* szMessage );
#define ASSERT( f )		DBG_AssertFunction( f, #f, __FILE__, __LINE__, NULL )
#define ASSERTSZ( f, sz )	DBG_AssertFunction( f, #f, __FILE__, __LINE__, sz )
#else
#define ASSERT( f )
#define ASSERTSZ( f, sz )
#endif

extern ui_globalvars_t		*gpGlobals;

// exports
extern int UI_VidInit( void );
extern void UI_Init( void );
extern void UI_Shutdown( void );
extern void UI_UpdateMenu( float flTime );
extern void UI_KeyEvent( int key, int down );
extern void UI_MouseMove( int x, int y );
extern void UI_SetActiveMenu( int fActive );
extern void UI_AddServerToList( netadr_t adr, const char *info );
extern void UI_GetCursorPos( int *pos_x, int *pos_y );
extern void UI_SetCursorPos( int pos_x, int pos_y );
extern void UI_ShowCursor( int show );
extern void UI_CharEvent( int key );
extern int UI_MouseInRect( void );
extern int UI_IsVisible( void );
extern int UI_CreditsActive( void );
extern void UI_FinalCredits( void );

#include "cvardef.h"

// ScreenHeight returns the height of the screen, in pixels
#define ScreenHeight	(gpGlobals->scrHeight)
// ScreenWidth returns the width of the screen, in pixels
#define ScreenWidth		(gpGlobals->scrWidth)

inline unsigned int PackRGB( int r, int g, int b )
{
	return ((0xFF)<<24|(r)<<16|(g)<<8|(b));
}

inline unsigned int PackRGBA( int r, int g, int b, int a )
{
	return ((a)<<24|(r)<<16|(g)<<8|(b));
}

inline void UnpackRGB( int &r, int &g, int &b, unsigned int ulRGB )
{
	r = (ulRGB & 0xFF0000) >> 16;
	g = (ulRGB & 0xFF00) >> 8;
	b = (ulRGB & 0xFF) >> 0;
}

inline void UnpackRGBA( int &r, int &g, int &b, int &a, unsigned int ulRGBA )
{
	a = (ulRGBA & 0xFF000000) >> 24;
	r = (ulRGBA & 0xFF0000) >> 16;
	g = (ulRGBA & 0xFF00) >> 8;
	b = (ulRGBA & 0xFF) >> 0;
}

inline int PackAlpha( unsigned int ulRGB, unsigned int ulAlpha )
{
	return (ulRGB)|(ulAlpha<<24);
}

inline int UnpackAlpha( unsigned int ulRGBA )
{
	return ((ulRGBA & 0xFF000000) >> 24);	
}

inline float RemapVal( float val, float A, float B, float C, float D)
{
	return C + (D - C) * (val - A) / (B - A);
}

extern int ColorStrlen( const char *str );	// returns string length without color symbols
extern const int g_iColorTable[8];
extern void COM_FileBase( const char *in, char *out );		// ripped out from hlsdk 2.3
extern int UI_FadeAlpha( int starttime, int endtime );
extern void StringConcat( char *dst, const char *src, size_t size );	// strncat safe prototype
extern char *Info_ValueForKey( const char *s, const char *key );
extern int KEY_GetKey( const char *binding );			// ripped out from engine
extern char *StringCopy( const char *input );			// copy string into new memory
extern int COM_CompareSaves( const void **a, const void **b );

extern void UI_LoadCustomStrings( void );

#endif//UTILS_H