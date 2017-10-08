#pragma once
#ifndef GL_VIDNT_H
#define GL_VIDNT_H

#define VID_NOMODE -2.0f
#define VID_AUTOMODE	"-1"
#define VID_DEFAULTMODE	2.0f
#define DISP_CHANGE_BADDUALVIEW	-6 // MSVC 6.0 doesn't
#define WINDOW_NAME			"Xash Window" // Half-Life

extern int num_vidmodes;



typedef struct vidmode_s
{
	const char	*desc;
	int		width;
	int		height;
	qboolean		wideScreen;
} vidmode_t;

extern vidmode_t vidmode[];

// backend functions
void GL_InitExtensions( void );
qboolean GL_CreateContext( void );
qboolean GL_UpdateContext( void );
void GL_SetupAttributes( void );
void R_Free_OpenGL( void );
qboolean R_Init_OpenGL( void );
qboolean VID_SetMode( void );
qboolean VID_SetScreenResolution( int width, int height );
void VID_DestroyWindow( void );
void VID_RestoreScreenResolution( void );

void GL_CheckExtension( const char *name, const dllfunc_t *funcs, const char *cvarname, int r_ext );

const char *VID_GetModeString( int vid_mode );

// common functions
void R_SaveVideoMode( int w, int h );
void GL_SetExtension( int r_ext, int enable );

#endif
