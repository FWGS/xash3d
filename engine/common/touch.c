/*
touchscreen.c - touchscreen support prototype
Copyright (C) 2015 a1batross

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
#include "gl_local.h"
#include "input.h"
#include "client.h"
#include "touch.h"
#include "math.h"
#ifdef XASH_SDL
#include <SDL_hints.h>
#include <SDL_keyboard.h>
#endif

typedef enum
{
	touch_command,		// Just tap a button
	touch_move,		// Like a joystick stick.
	touch_look		// Like a joystick stick.
} touchButtonType;

typedef enum
{
	event_down = 0,
	event_up,
	event_motion
} touchEventType;

typedef enum
{
	state_none = 0,
	state_edit,
	state_edit_move
} touchState;

typedef enum
{
	round_none = 0,
	round_grid,
	round_aspect
} touchRound;

typedef struct touchbutton2_s
{
	// Touch button type: tap, stick or slider
	touchButtonType type;
	// Field of button in pixels
	float x1, y1, x2, y2;
	// Button texture
	int texture;
	rgba_t color;
	char texturefile[256];
	char command[256];
	char name[32];
	int finger;
	int flags;
	// Double-linked list
	struct touchbutton2_s *next;
	struct touchbutton2_s *prev;
} touchbutton2_t;

struct touch_s
{
	qboolean initialized;
	byte *mempool;
	touchbutton2_t *first;
	touchbutton2_t *last;
	touchState state;
	int look_finger;
	int move_finger;
	float move_start_x;
	float move_start_y;
	float forward;
	float side;
	float yaw;
	float pitch;
	// editing
	touchbutton2_t *edit;
	touchbutton2_t *selection;
	int resize_finger;
	qboolean showbuttons;
	qboolean clientonly;
	rgba_t scolor;
	int swidth;
	// textures
	int showtexture;
	int hidetexture;
	int resettexture;
	int closetexture;
} touch;

typedef struct touchdefaultbutton_s
{
	char name[32];
	char texturefile[256];
	char command[256];
	float x1, y1, x2, y2;
	rgba_t color;
	touchRound round;
	float aspect;
	int flags;
} touchdefaultbutton_t;

touchdefaultbutton_t g_DefaultButtons[256] = {
{"look", "", "_look", 0.5, 0.0, 1.0, 1.0, { 255, 255, 255, 255 }, round_none, 0, 0 },
{"move", "", "_move", 0.0, 0.0, 0.5, 1.0, { 255, 255, 255, 255 }, round_none, 0, 0 },
{"invnext", "touch_default/next_weap.tga", "invnext", 0.000000, 0.530200, 0.120000, 0.757428, { 255, 255, 255, 255 }, round_aspect, 1, 0 },
{"invprev", "touch_default/prev_weap.tga", "invprev", 0.000000, 0.075743, 0.120000, 0.302971, { 255, 255, 255, 255 }, round_aspect, 1, 0 },
{"edit", "touch_default/settings.tga", "touch_enableedit", 0.420000, 0.000000, 0.500000, 0.135271, { 255, 255, 255, 255 }, round_aspect, 1, TOUCH_FL_DEF_SHOW },
{"use", "touch_default/use.tga", "+use", 0.880000, 0.454457, 1.000000, 0.681685, { 255, 255, 255, 255 }, round_aspect, 1, 0 },
{"jump", "touch_default/jump.tga", "+jump", 0.880000, 0.227228, 1.000000, 0.454457, { 255, 255, 255, 255 }, round_aspect, 1, 0 },
{"attack", "touch_default/shoot.tga", "+attack", 0.760000, 0.530200, 0.880000, 0.757428, { 255, 255, 255, 255 }, round_aspect, 1, 0 },
{"attack2", "touch_default/shoot_alt.tga", "+attack2", 0.760000, 0.302971, 0.880000, 0.530200, { 255, 255, 255, 255 }, round_aspect, 1, 0 },
{"loadquick", "touch_default/load.tga", "loadquick", 0.760000, 0.000000, 0.840000, 0.135271, { 255, 255, 255, 255 }, round_aspect, 1, TOUCH_FL_SP },
{"savequick", "touch_default/save.tga", "savequick", 0.840000, 0.000000, 0.920000, 0.135271, { 255, 255, 255, 255 }, round_aspect, 1, TOUCH_FL_SP },
{"duck", "touch_default/crouch.tga", "+duck", 0.280000, 0.757428, 0.400000, 0.984656, { 255, 255, 255, 255 }, round_aspect, 1, 0 },
{"messagemode", "touch_default/keyboard.tga", "messagemode", 0.840000, 0.000000, 0.920000, 0.135271, { 255, 255, 255, 255 }, round_aspect, 1, TOUCH_FL_MP },
{"reload", "touch_default/reload.tga", "+reload", 0.000000, 0.302971, 0.120000, 0.530200, { 255, 255, 255, 255 }, round_aspect, 1, 0 },
{"show_numbers", "touch_default/show_weapons.tga", "exec touch_default/numbers.cfg", 0.440000, 0.833171, 0.520000, 0.984656, { 255, 255, 255, 255 }, round_aspect, 1, TOUCH_FL_HIDE },
{"tduck", "touch_default/tduck.tga", ";+duck", 0.220000, 0.871042, 0.280000, 0.984656, { 255, 255, 255, 255 }, round_aspect, 1, TOUCH_FL_HIDE },
};
int g_LastDefaultButton = 16;

convar_t *touch_pitch;
convar_t *touch_yaw;
convar_t *touch_forwardzone;
convar_t *touch_sidezone;
convar_t *touch_grid_enable;
convar_t *touch_grid_count;
convar_t *touch_config_file;
convar_t *touch_enable;
convar_t *touch_in_menu;

// enable on android by default
#ifdef __ANDROID__
#define TOUCH_ENABLE "1"
#else
#define TOUCH_ENABLE "0"
#endif

// code looks smaller with it
#define B(x) button->x
#define SCR_W (scr_width->value)
#define SCR_H (scr_height->value)
#define TO_SCRN_Y(x) (scr_height->integer * (x))
#define TO_SCRN_X(x) (scr_width->integer * (x))

int pfnDrawCharacter( int x, int y, int number, int r, int g, int b );
static void IN_TouchCheckCoords( float *x1, float *y1, float *x2, float *y2  );

void IN_TouchWriteConfig( void )
{
	file_t	*f;

	if( !touch.first ) return;

	MsgDev( D_NOTE, "IN_TouchWriteConfig(): %s\n", touch_config_file->string );
	f = FS_Open( touch_config_file->string, "w", false );
	if( f )
	{
		touchbutton2_t *button;
		FS_Printf( f, "//=======================================================================\n");
		FS_Printf( f, "//\t\t\tCopyright XashXT Group %s ©\n", Q_timestamp( TIME_YEAR_ONLY ));
		FS_Printf( f, "//\t\t\ttouch.cfg - touchscreen config\n" );
		FS_Printf( f, "//=======================================================================\n" );
		FS_Printf( f, "\ntouch_config_file \"%s\"\n", touch_config_file->string );
		FS_Printf( f, "\n// touch cvars\n" );
		FS_Printf( f, "touch_forwardzone \"%f\"\n", touch_forwardzone->value );
		FS_Printf( f, "touch_sidezone \"%f\"\n", touch_sidezone->value );
		FS_Printf( f, "touch_pitch \"%f\"\n", touch_pitch->value );
		FS_Printf( f, "touch_yaw \"%f\"\n", touch_yaw->value );
		FS_Printf( f, "touch_grid_count \"%d\"\n", touch_grid_count->integer );
		FS_Printf( f, "touch_grid_enable \"%d\"\n", touch_grid_enable->integer );
		FS_Printf( f, "touch_set_stroke %d %d %d %d %d\n", touch.swidth, touch.scolor[0], touch.scolor[1], touch.scolor[2], touch.scolor[3] );
		FS_Printf( f, "touch_setclientonly 0\n" );
		FS_Printf( f, "\n// touch buttons\n" );
		FS_Printf( f, "touch_removeall\n" );
		for( button = touch.first; button; button = button->next )
		{
			int flags = button->flags;
			if( flags & TOUCH_FL_CLIENT )
				continue; //skip temporary buttons
			if( flags & TOUCH_FL_DEF_SHOW )
				flags &= ~TOUCH_FL_HIDE;
			if( flags & TOUCH_FL_DEF_HIDE )
				flags |= TOUCH_FL_HIDE;

			FS_Printf( f, "touch_addbutton \"%s\" \"%s\" \"%s\" %f %f %f %f %d %d %d %d %d\n", 
				B(name), B(texturefile), B(command),
				B(x1), B(y1), B(x2), B(y2),
				B(color[0]), B(color[1]), B(color[2]), B(color[3]), flags );
		}

		FS_Close( f );
	}
	else MsgDev( D_ERROR, "Couldn't write %s.\n", touch_config_file->string );
}

void IN_TouchExportConfig_f( void )
{
	file_t	*f;
	char *name;

	if( Cmd_Argc() != 2 )
	{
		Msg( "Usage: touch_exportconfig <name>\n" );
		return;
	}

	if( !touch.first ) return;

	name = Cmd_Argv( 1 );

	MsgDev( D_NOTE, "Exporting config to %s\n", name );
	f = FS_Open( name, "w", false );
	if( f )
	{
		char profilename[256];
		char profilebase[256];
		touchbutton2_t *button;
		if( Q_strstr( name, "touch_presets/" ) )
		{
			FS_FileBase( name, profilebase );
			Q_snprintf( profilename, 256, "touch_profiles/%s (copy).cfg", profilebase );
		}
		else Q_strncpy( profilename, name, 256 );
		FS_Printf( f, "//=======================================================================\n");
		FS_Printf( f, "//\t\t\tCopyright XashXT Group %s ©\n", Q_timestamp( TIME_YEAR_ONLY ));
		FS_Printf( f, "//\t\t\ttouch.cfg - touchscreen config\n" );
		FS_Printf( f, "//=======================================================================\n" );
		FS_Printf( f, "\ntouch_config_file \"%s\"\n", profilename );
		FS_Printf( f, "\n// touch cvars\n" );
		FS_Printf( f, "touch_forwardzone \"%f\"\n", touch_forwardzone->value );
		FS_Printf( f, "touch_sidezone \"%f\"\n", touch_sidezone->value );
		FS_Printf( f, "touch_pitch \"%f\"\n", touch_pitch->value );
		FS_Printf( f, "touch_yaw \"%f\"\n", touch_yaw->value );
		FS_Printf( f, "touch_grid_count \"%d\"\n", touch_grid_count->integer );
		FS_Printf( f, "touch_grid_enable \"%d\"\n", touch_grid_enable->integer );
		FS_Printf( f, "touch_set_stroke %d %d %d %d %d\n", touch.swidth, touch.scolor[0], touch.scolor[1], touch.scolor[2], touch.scolor[3] );
		FS_Printf( f, "touch_setclientonly 0\n" );
		FS_Printf( f, "\n// touch buttons\n" );
		FS_Printf( f, "touch_removeall\n" );
		for( button = touch.first; button; button = button->next )
		{
			float aspect;
			int flags = button->flags;
			if( flags & TOUCH_FL_CLIENT )
				continue; //skip temporary buttons
			if( flags & TOUCH_FL_DEF_SHOW )
				flags &= ~TOUCH_FL_HIDE;
			if( flags & TOUCH_FL_DEF_HIDE )
				flags |= TOUCH_FL_HIDE;

			aspect = ( B(y2) - B(y1) ) / ( ( B(x2) - B(x1) ) /(SCR_H/SCR_W) );

			FS_Printf( f, "touch_addbutton \"%s\" \"%s\" \"%s\" %f %f %f %f %d %d %d %d %d %f\n", 
				B(name), B(texturefile), B(command),
				B(x1), B(y1), B(x2), B(y2),
				B(color[0]), B(color[1]), B(color[2]), B(color[3]), flags, aspect );
		}
		FS_Printf( f, "\n// round button coordinates to grid\n" );
		FS_Printf( f, "touch_roundall\n" );
		FS_Close( f );
	}
	else MsgDev( D_ERROR, "Couldn't write %s.\n", name );
}

void IN_TouchRoundAll_f( void )
{
	touchbutton2_t *button;
	if( !touch_grid_enable->value )
		return;
	for( button = touch.first; button; button = button->next )
		IN_TouchCheckCoords( &B(x1), &B(y1), &B(x2), &B(y2) );
}

void IN_TouchListButtons_f( void )
{
	touchbutton2_t *button;
	for( button = touch.first; button; button = button->next )
	{
		Msg( "%s %s %s %f %f %f %f %d %d %d %d %d\n", 
			B(name), B(texturefile), B(command),
			B(x1), B(y1), B(x2), B(y2),
			B(color[0]), B(color[1]), B(color[2]), B(color[3]), B(flags) );
		if( B(flags) & TOUCH_FL_CLIENT)
			continue;
		UI_AddTouchButtonToList( B(name), B(texturefile), B(command),B(color), B(flags) );
	}
}

void IN_TouchStroke_f( void )
{
	touch.swidth = Q_atoi( Cmd_Argv( 1 ) );
	MakeRGBA( touch.scolor, Q_atoi( Cmd_Argv( 2 ) ), Q_atoi( Cmd_Argv( 3 ) ), Q_atoi( Cmd_Argv( 4 ) ), Q_atoi( Cmd_Argv( 5 ) ) );
}

touchbutton2_t *IN_TouchFindButton( const char *name )
{
	touchbutton2_t *button;
	if( !touch.first )
		return NULL;
	for ( button = touch.first; button; button = button->next )
		if( !Q_strncmp( button->name, name, 32 ) )
			return button;
	return NULL;
}

touchbutton2_t *IN_TouchFindFirst( const char *name )
{
	touchbutton2_t *button;
	if( !touch.first )
		return NULL;
	for ( button = touch.first; button; button = button->next )
		if( ( Q_strstr( name, "*" ) && Q_stricmpext( name, button->name ) ) || !Q_strncmp( name, button->name, 32 ) )
			return button;
	return NULL;
}

void IN_TouchSetClientOnly( qboolean state )
{
	touch.clientonly = state;
}

void IN_TouchSetClientOnly_f( void )
{
	touch.clientonly = Q_atoi( Cmd_Argv( 1 ) );
}

void IN_TouchRemoveButton( const char *name )
{
	touchbutton2_t *button;

	IN_TouchEditClear();

	while( button = IN_TouchFindFirst( name ) )
	{
		if( button->prev )
			button->prev->next = button->next;
		else
			touch.first = button->next;
		if( button->next )
			button->next->prev = button->prev;
		else
			touch.last = button->prev;
		Mem_Free( button );
	}

}

void IN_TouchRemoveButton_f( void )
{
	IN_TouchRemoveButton( Cmd_Argv( 1 ) );
}

void IN_TouchRemoveAll_f( void )
{
	IN_TouchEditClear();
	while( touch.first )
	{
		touchbutton2_t *remove = touch.first;
		touch.first = touch.first->next;
		Mem_Free ( remove );
	}
	touch.last = NULL;
}

void IN_TouchSetColor( const char *name, byte *color )
{
	touchbutton2_t *button;
	for( button = touch.first; button; button = button->next )
	{
		if( ( Q_strstr( name, "*" ) && Q_stricmpext( name, button->name ) ) || !Q_strncmp( name, button->name, 32 ) )
			MakeRGBA( button->color, color[0], color[1], color[2], color[3] );
	}
}

void IN_TouchSetTexture( const char *name, const char *texture )
{
	touchbutton2_t *button = IN_TouchFindButton( name );
	if( !button )
		return;
	button->texture = -1; // mark for texture load
	Q_strncpy( button->texturefile, texture, sizeof( button->texturefile ) );
}

void IN_TouchSetCommand( const char *name, const char *command )
{
	touchbutton2_t *button = IN_TouchFindButton( name );
	if( !button )
		return;
	Q_strncpy( button->command, command, sizeof( button->command ) );
}

void IN_TouchHideButtons( const char *name, qboolean hide )
{
	touchbutton2_t *button;
	for( button = touch.first; button; button = button->next)
	{
		if( ( Q_strstr( name, "*" ) && Q_stricmpext( name, button->name ) ) || !Q_strncmp( name, button->name, 32 ) )
		{
			if( hide )
				button->flags |= TOUCH_FL_HIDE;
			else
				button->flags &= ~TOUCH_FL_HIDE;
		}
	}
	
}
void IN_TouchHide_f( void )
{
	IN_TouchHideButtons( Cmd_Argv( 1 ), true );
}

void IN_TouchShow_f( void )
{
	IN_TouchHideButtons( Cmd_Argv( 1 ), false );
}

void IN_TouchSetColor_f( void )
{
	rgba_t color;
	if( Cmd_Argc() == 6 )
	{
		MakeRGBA( color,  Q_atoi( Cmd_Argv(2) ), Q_atoi( Cmd_Argv(3) ), Q_atoi( Cmd_Argv(4) ), Q_atoi( Cmd_Argv(5) ) );
		IN_TouchSetColor( Cmd_Argv(1), color );
		return;
	}
	Msg( "Usage: touch_setcolor <pattern> <r> <g> <b> <a>\n" );
}

void IN_TouchSetTexture_f( void )
{
	if( Cmd_Argc() == 3 )
	{
		IN_TouchSetTexture( Cmd_Argv( 1 ), Cmd_Argv( 2 ) );
		return;
	}
	Msg( "Usage: touch_settexture <name> <file>\n" );
}

void IN_TouchSetFlags_f( void )
{
	if( Cmd_Argc() == 3 )
	{
		touchbutton2_t *button = IN_TouchFindButton( Cmd_Argv( 1 ) );
		if( button )
			button->flags = Q_atoi( Cmd_Argv( 2 ) );
		return;
	}
	Msg( "Usage: touch_setflags <name> <file>\n" );
}

void IN_TouchSetCommand_f( void )
{
	if( Cmd_Argc() == 3 )
	{
		IN_TouchSetCommand( Cmd_Argv( 1 ), Cmd_Argv( 2 ) );
		return;
	}
	Msg( "Usage: touch_command <name> <command>\n" );
}
void IN_TouchReloadConfig_f( void )
{
	Cbuf_AddText( va("exec %s\n", touch_config_file->string ) );
}

touchbutton2_t *IN_AddButton( const char *name,  const char *texture, const char *command, float x1, float y1, float x2, float y2, byte *color )
{
	touchbutton2_t *button = Mem_Alloc( touch.mempool, sizeof( touchbutton2_t ) );
	button->texture = -1;
	Q_strncpy( button->texturefile, texture, sizeof( button->texturefile ) );
	Q_strncpy( button->name, name, 32 );
	IN_TouchRemoveButton( name ); //replace if exist
	button->x1 = x1;
	button->y1 = y1;
	button->x2 = x2;
	button->y2 = y2;
	MakeRGBA( button->color, color[0], color[1], color[2], color[3] );
	button->command[0] = 0;
	button->flags = 0;
	// check keywords
	if( !Q_strcmp( command, "_look" ) )
		button->type = touch_look;
	if( !Q_strcmp( command, "_move" ) )
		button->type = touch_move;
	Q_strncpy( button->command, command, sizeof( button->command ) );
	button->finger = -1;
	button->next = NULL;
	button->prev = touch.last;
	if( touch.last )
		touch.last->next = button;
	touch.last = button;
	if( !touch.first )
		touch.first = button;
	return button;
}

void IN_TouchAddClientButton( const char *name, const char *texture, const char *command, float x1, float y1, float x2, float y2, byte *color, int round, float aspect, int flags )
{
	touchbutton2_t *button;
	if( !touch.initialized )
		return;
	if( round )
		IN_TouchCheckCoords( &x1, &y1, &x2, &y2 );
	if( round == round_aspect )
	{
		if( texture[0] == '#' )
			// on text fields aspect ratio specifies line count
			y2 = y1 + ( (float)clgame.scrInfo.iCharHeight / (float)clgame.scrInfo.iHeight ) * aspect + touch.swidth*2/SCR_H;
		else
			y2 = y1 + ( x2 - x1 ) * (SCR_W/SCR_H) * aspect;
	}
	button = IN_AddButton( name, texture, command, x1, y1, x2, y2, color );
	button->flags |= flags | TOUCH_FL_CLIENT | TOUCH_FL_NOEDIT;
}

void IN_TouchLoadDefaults_f( void )
{
	int i;
	for( i = 0; i < g_LastDefaultButton; i++ )
	{
		touchbutton2_t *button;
		float x1 = g_DefaultButtons[i].x1, 
			  y1 = g_DefaultButtons[i].y1,
			  x2 = g_DefaultButtons[i].x2,
			  y2 = g_DefaultButtons[i].y2; 
		
		IN_TouchCheckCoords( &x1, &y1, &x2, &y2 );
		if( g_DefaultButtons[i].round == round_aspect )
		if( g_DefaultButtons[i].texturefile[0] == '#' )
			y2 = y1 + ( (float)clgame.scrInfo.iCharHeight / (float)clgame.scrInfo.iHeight ) * g_DefaultButtons[i].aspect + touch.swidth*2/SCR_H;
		else
			y2 = y1 + ( x2 - x1 ) * (SCR_W/SCR_H) * g_DefaultButtons[i].aspect;
		IN_TouchCheckCoords( &x1, &y1, &x2, &y2 );
		button = IN_AddButton( g_DefaultButtons[i].name, g_DefaultButtons[i].texturefile, g_DefaultButtons[i].command, x1, y1, x2, y2, g_DefaultButtons[i].color );
		button->flags |= g_DefaultButtons[i].flags;
	}
}


// Add default button from client
void IN_TouchAddDefaultButton( const char *name, const char *texturefile, const char *command, float x1, float y1, float x2, float y2, byte *color, int round, float aspect, int flags )
{
	if( g_LastDefaultButton >= 255 )
		return;
	Q_strncpy( g_DefaultButtons[g_LastDefaultButton].name, name, 32 );
	Q_strncpy( g_DefaultButtons[g_LastDefaultButton].texturefile, texturefile, 256 );
	Q_strncpy( g_DefaultButtons[g_LastDefaultButton].command, command, 256 );
	g_DefaultButtons[g_LastDefaultButton].x1 = x1;
	g_DefaultButtons[g_LastDefaultButton].y1 = y1;
	g_DefaultButtons[g_LastDefaultButton].x2 = x2;
	g_DefaultButtons[g_LastDefaultButton].y2 = y2;
	MakeRGBA( g_DefaultButtons[g_LastDefaultButton].color, color[0], color[1], color[2], color[3] );
	g_DefaultButtons[g_LastDefaultButton].round = round;
	g_DefaultButtons[g_LastDefaultButton].aspect = aspect;
	g_DefaultButtons[g_LastDefaultButton].flags = flags;
	g_LastDefaultButton++;
}

// Client may remove all default buttons from engine
void IN_TouchResetDefaultButtons( void )
{
	g_LastDefaultButton = 0;
}
void IN_TouchAddButton_f( void )
{
	rgba_t color;
	int argc = Cmd_Argc( );

	if( argc >= 12 )
	{
		touchbutton2_t *button;
		MakeRGBA( color, Q_atoi( Cmd_Argv(8) ), Q_atoi( Cmd_Argv(9) ), 
			Q_atoi( Cmd_Argv(10) ), Q_atoi( Cmd_Argv(11) ) );
		button = IN_AddButton( Cmd_Argv(1), Cmd_Argv(2), Cmd_Argv(3),
			Q_atof( Cmd_Argv(4) ), Q_atof( Cmd_Argv(5) ), 
			Q_atof( Cmd_Argv(6) ), Q_atof( Cmd_Argv(7) ) ,
			color );
		if( argc >= 13 )
			button->flags = Q_atoi( Cmd_Argv(12) );
		if( argc >= 14 )
		{
			// Recalculate button coordinates aspect ratio
			// This is feature for distributed configs
			float aspect = Q_atof( Cmd_Argv(13) );
			IN_TouchCheckCoords( &B(x1), &B(y1), &B(x2), &B(y2) );
			if( aspect )
			{
				if( B(texturefile)[0] == '#' )
					B(y2) = B(y1) + ( (float)(clgame.scrInfo.iCharHeight) / (float)(clgame.scrInfo.iHeight) ) * aspect + touch.swidth*2/SCR_H;
				else
					B(y2) = B(y1) + ( B(x2) - B(x1) ) * (SCR_W/SCR_H) * aspect;
			}
		}
				
		return;
	}	
	if( argc == 8 )
	{
		MakeRGBA( color, 255, 255, 255, 255 );
		IN_AddButton( Cmd_Argv(1), Cmd_Argv(2), Cmd_Argv(3),
			Q_atof( Cmd_Argv(4) ), Q_atof( Cmd_Argv(5) ), 
			Q_atof( Cmd_Argv(6) ), Q_atof( Cmd_Argv(7) ),
			color );
		return;
	}
	if( argc == 4 )
	{
		MakeRGBA( color, 255, 255, 255, 255 );
		IN_AddButton( Cmd_Argv(1), Cmd_Argv(2), Cmd_Argv(3), 0.4, 0.4, 0.6, 0.6, color );
		return;
	}
	Msg( "Usage: touch_addbutton <name> <texture> <command> [<x1> <y1> <x2> <y2> [ r g b a] ]\n" );
}

void IN_TouchEnableEdit_f( void )
{
	if( touch.state == state_none )
		touch.state = state_edit;
	touch.resize_finger = touch.move_finger = touch.look_finger = -1;
}

void IN_TouchDisableEdit_f( void )
{
	touch.state = state_none;
	if( touch.edit )
		touch.edit->finger = -1;
	if( touch.selection )
		touch.selection->finger = -1;
	touch.edit = touch.selection = NULL;
	touch.resize_finger = touch.move_finger = touch.look_finger = -1;
}

void IN_TouchDeleteProfile_f( void )
{
	if( Cmd_Argc() != 2 )
	{
		Msg( "Usage: touch_deleteprofile <name>\n" );
		return;
	}

	// delete profile
	FS_Delete( va( "touch_profiles/%s.cfg", Cmd_Argv( 1 )));
}


void IN_TouchInit( void )
{
	if( touch.initialized )
		return;
	touch.mempool = Mem_AllocPool( "Touch" );
	touch.first = touch.last = NULL;
	MsgDev( D_NOTE, "IN_TouchInit()\n");
	touch.move_finger = touch.resize_finger = touch.look_finger = -1;
	touch.state = state_none;
	touch.showbuttons = true;
	touch.clientonly = false;
	MakeRGBA( touch.scolor, 255, 255, 255, 255 );
	touch.swidth = 1;
	
	Cmd_AddCommand( "touch_addbutton", IN_TouchAddButton_f, "Add native touch button" );
	Cmd_AddCommand( "touch_removebutton", IN_TouchRemoveButton_f, "Remove native touch button" );
	Cmd_AddCommand( "touch_enableedit", IN_TouchEnableEdit_f, "Enable button editing mode" );
	Cmd_AddCommand( "touch_disableedit", IN_TouchDisableEdit_f, "Disable button editing mode" );
	Cmd_AddCommand( "touch_settexture", IN_TouchSetTexture_f, "Change button texture" );
	Cmd_AddCommand( "touch_setcolor", IN_TouchSetColor_f, "Change button color" );
	Cmd_AddCommand( "touch_setcommand", IN_TouchSetCommand_f, "Change button command" );
	Cmd_AddCommand( "touch_setflags", IN_TouchSetFlags_f, "Change button flags (be careful)" );
	Cmd_AddCommand( "touch_show", IN_TouchShow_f, "show button" );
	Cmd_AddCommand( "touch_hide", IN_TouchHide_f, "hide button" );
	Cmd_AddCommand( "touch_list", IN_TouchListButtons_f, "list buttons" );
	Cmd_AddCommand( "touch_removeall", IN_TouchRemoveAll_f, "Remove all buttons" );
	Cmd_AddCommand( "touch_loaddefaults", IN_TouchLoadDefaults_f, "Generate config from defaults" );
	Cmd_AddCommand( "touch_roundall", IN_TouchRoundAll_f, "Round all buttons coordinates to grid" );
	Cmd_AddCommand( "touch_exportconfig", IN_TouchExportConfig_f, "Export config keeping aspect ratio" );
	Cmd_AddCommand( "touch_set_stroke", IN_TouchStroke_f, "Set global stroke width and color" );
	Cmd_AddCommand( "touch_setclientonly", IN_TouchSetClientOnly_f, "When 1, only client buttons are shown" );
	Cmd_AddCommand( "touch_reloadconfig", IN_TouchReloadConfig_f, "load config, not saving changes" );
	Cmd_AddCommand( "touch_writeconfig", IN_TouchWriteConfig, "save current config" );
	Cmd_AddCommand( "touch_deleteprofile", IN_TouchDeleteProfile_f, "delete profile by name" );
	touch_forwardzone = Cvar_Get( "touch_forwardzone", "0.06", 0, "forward touch zone" );
	touch_in_menu = Cvar_Get( "touch_in_menu", "0", 0, "draw touch in menu (for internal use only)" );
	touch_sidezone = Cvar_Get( "touch_sidezone", "0.06", 0, "side touch zone" );
	touch_pitch = Cvar_Get( "touch_pitch", "90", 0, "touch pitch sensitivity" );
	touch_yaw = Cvar_Get( "touch_yaw", "120", 0, "touch yaw sensitivity" );
	touch_grid_count = Cvar_Get( "touch_grid_count", "50", 0, "touch grid count" );
	touch_grid_enable = Cvar_Get( "touch_grid_enable", "1", 0, "enable touch grid" );
	touch_config_file = Cvar_Get( "touch_config_file", "touch.cfg", CVAR_ARCHIVE, "current touch profile file" );
	touch_enable = Cvar_Get( "touch_enable", TOUCH_ENABLE, CVAR_ARCHIVE, "enable touch controls" );
#if defined(XASH_SDL) && defined(__ANDROID__)
	SDL_SetHint( SDL_HINT_ANDROID_SEPARATE_MOUSE_AND_TOUCH, "1" );
#endif
	touch.initialized = true;
}
int pfnGetScreenInfo( SCREENINFO *pscrinfo );
// must be called after executing config.cfg
void IN_TouchInitConfig( void )
{
	if( !touch.initialized )
		return;

	pfnGetScreenInfo( NULL ); //HACK: update hud screen parameters like iHeight
	if( FS_FileExists( touch_config_file->string, true ) )
		Cbuf_AddText( va( "exec \"%s\"\n", touch_config_file->string ) );
	else IN_TouchLoadDefaults_f( );
	touch.closetexture = GL_LoadTexture( "touch_default/edit_close.tga", NULL, 0, TF_NOPICMIP, NULL );
	touch.hidetexture = GL_LoadTexture( "touch_default/edit_hide.tga", NULL, 0, TF_NOPICMIP, NULL );
	touch.showtexture = GL_LoadTexture( "touch_default/edit_show.tga", NULL, 0, TF_NOPICMIP, NULL );
	touch.resettexture = GL_LoadTexture( "touch_default/edit_reset.tga", NULL, 0, TF_NOPICMIP, NULL );
}
qboolean IN_TouchIsVisible( touchbutton2_t *button )
{
	return ( !touch.clientonly || ( button->flags & TOUCH_FL_CLIENT) ) && 
	( 
	( touch.state >= state_edit )
	||( !( button->flags & TOUCH_FL_HIDE ) 
	&& ( !(button->flags & TOUCH_FL_SP) || ( CL_GetMaxClients() == 1 ) ) 
	&& ( !(button->flags & TOUCH_FL_MP) || ( CL_GetMaxClients() !=  1 ) ) ) 
	 );
}

void IN_TouchDrawTexture ( float x1, float y1, float x2, float y2, int texture, byte r, byte g, byte b, byte a )
{
	pglColor4ub( r, g, b, a );
	if( x1 >= x2 )
		return;
	if( y1 >= y2 )
		return;
	pglColor4ub( r, g, b, a );
	R_DrawStretchPic( TO_SCRN_X(x1),
		TO_SCRN_Y(y1),
		TO_SCRN_X(x2 - x1),
		TO_SCRN_Y(y2 - y1),
		0, 0, 1, 1, texture );
}

#if defined(_MSC_VER) && (_MSC_VER < 1700) 
static __inline int round(float f)
{
    return (int)(f + 0.5);

}
#endif

#define GRID_COUNT_X (touch_grid_count->integer)
#define GRID_COUNT_Y (touch_grid_count->integer * SCR_H / SCR_W)
#define GRID_X (1.0/GRID_COUNT_X)
#define GRID_Y (SCR_W/SCR_H/GRID_COUNT_X)
#define GRID_ROUND_X(x) ((float)round( x * GRID_COUNT_X ) / GRID_COUNT_X)
#define GRID_ROUND_Y(x) ((float)round( x * GRID_COUNT_Y ) / GRID_COUNT_Y)

static void IN_TouchCheckCoords( float *x1, float *y1, float *x2, float *y2  )
{
	/// TODO: grid check here
	if( *x2 - *x1 < GRID_X * 2 )
		*x2 = *x1 + GRID_X * 2;
	if( *y2 - *y1 < GRID_Y * 2)
		*y2 = *y1 + GRID_Y * 2;
	if( *x1 < 0 )
		*x2 -= *x1, *x1 = 0;
	if( *y1 < 0 )
		*y2 -= *y1, *y1 = 0;
	if( *y2 > 1 )
		*y1 -= *y2 - 1, *y2 = 1;
	if( *x2 > 1 )
		*x1 -= *x2 - 1, *x2 = 1;
	if ( touch_grid_enable->value )
	{
		*x1 = GRID_ROUND_X( *x1 );
		*x2 = GRID_ROUND_X( *x2 );
		*y1 = GRID_ROUND_Y( *y1 );
		*y2 = GRID_ROUND_Y( *y2 );
	}
}

float IN_TouchDrawText( float x1, float y1, float x2, float y2, const char *s, byte *color)
{
	float x = x1 * clgame.scrInfo.iWidth;
	float y = y1 * clgame.scrInfo.iHeight;
	float maxy = y2 * clgame.scrInfo.iHeight - clgame.scrInfo.iCharHeight;
	float maxx;
	if( x2 )
		maxx = x2 * clgame.scrInfo.iWidth - clgame.scrInfo.charWidths['M'];
	else
		maxx = clgame.scrInfo.iWidth;
	
	if( !clgame.scrInfo.iWidth || !clgame.scrInfo.iHeight )
		return GRID_X * 2;
	Con_UtfProcessChar( 0 );
	while( *s )
	{
		while( *s && ( *s != '\n' ) && ( *s != ';' ) && ( x < maxx ) )
			x += pfnDrawCharacter( x, y, *s++, color[0], color[1], color[2] );
		y += clgame.scrInfo.iCharHeight;

		if( y >= maxy )
			break;
		
		x = x1 * clgame.scrInfo.iWidth;
		if( *s=='\n' || *s == ';' )
			s++;
	}
	GL_SetRenderMode( kRenderTransTexture );
	return ( x / clgame.scrInfo.iWidth );
}

void IN_TouchDraw( void )
{
	touchbutton2_t *button;

	if( !touch.initialized || !touch_enable->value )
		return;

	if( cls.key_dest != key_game && touch_in_menu->value == 0 )
		return;

	GL_SetRenderMode( kRenderTransTexture );

	if( touch.state >= state_edit && touch_grid_enable->value )
	{
		float x;
		if( touch_in_menu->value )
			IN_TouchDrawTexture( 0, 0, 1, 1, cls.fillImage, 32, 32, 32, 255 );
		else
			IN_TouchDrawTexture( 0, 0, 1, 1, cls.fillImage, 0, 0, 0, 112 );
		pglColor4ub( 0, 224, 224, 112 );
		for ( x = 0; x < 1 ; x += GRID_X )
			R_DrawStretchPic( TO_SCRN_X(x),
				0,
				1,
				TO_SCRN_Y(1),
				0, 0, 1, 1, cls.fillImage );
		for ( x = 0; x < 1 ; x += GRID_Y )
			R_DrawStretchPic( 0,
				TO_SCRN_Y(x),
				TO_SCRN_X(1),
				1,
				0, 0, 1, 1, cls.fillImage );
	}

	for( button = touch.first; button; button = button->next )
	{
		if( IN_TouchIsVisible( button ) )
		{
			if( button->texturefile[0] == '#' )
				IN_TouchDrawText( touch.swidth/SCR_W + B(x1), touch.swidth/SCR_H + B(y1), B(x2), B(y2), button->texturefile + 1, button->color );
			else if( button->texturefile[0] )
			{
				if( button->texture == -1 )
				{
					button->texture = GL_LoadTexture( button->texturefile, NULL, 0, TF_NOPICMIP, NULL );
				}
				if( B(flags) & TOUCH_FL_DRAW_ADDITIVE )
					GL_SetRenderMode( kRenderTransAdd );
				IN_TouchDrawTexture( B(x1), B(y1), B(x2), B(y2), B(texture), B(color[0]), B(color[1]), B(color[2]), B(color[3]) );
				GL_SetRenderMode( kRenderTransTexture );
			}
			if( B(flags) & TOUCH_FL_STROKE )
			{
				pglColor4ub( touch.scolor[0], touch.scolor[1], touch.scolor[2], touch.scolor[3] );
				R_DrawStretchPic( TO_SCRN_X(B(x1)),
					TO_SCRN_Y(B(y1)),
					touch.swidth,
					TO_SCRN_Y(B(y2)-B(y1)) - touch.swidth,
					0, 0, 1, 1, cls.fillImage );
				R_DrawStretchPic( TO_SCRN_X(B(x1)) + touch.swidth,
					TO_SCRN_Y(B(y1)),
					TO_SCRN_X(B(x2)-B(x1)) - touch.swidth,
					touch.swidth,
					0, 0, 1, 1, cls.fillImage );
				R_DrawStretchPic( TO_SCRN_X(B(x2))-touch.swidth,
					TO_SCRN_Y(B(y1)) + touch.swidth,
					touch.swidth,
					TO_SCRN_Y(B(y2)-B(y1)) - touch.swidth,
					0, 0, 1, 1, cls.fillImage );
				R_DrawStretchPic( TO_SCRN_X(B(x1)),
					TO_SCRN_Y(B(y2))-touch.swidth,
					TO_SCRN_X(B(x2)-B(x1)) - touch.swidth,
					touch.swidth,
					0, 0, 1, 1, cls.fillImage );
				pglColor4ub( 255, 255, 255, 255 );
			}
		}
		if( touch.state >= state_edit )
		{
			rgba_t color;
			if( !( button->flags & TOUCH_FL_HIDE ) )
				IN_TouchDrawTexture( B(x1), B(y1), B(x2), B(y2), cls.fillImage, 255, 255, 0, 32 );
			else
				IN_TouchDrawTexture( B(x1), B(y1), B(x2), B(y2), cls.fillImage, 128, 128, 128, 128 );
			MakeRGBA( color, 255, 255,127, 255 );
			Con_DrawString( TO_SCRN_X( B(x1) ), TO_SCRN_Y( B(y1) ), B(name), color );
		}
	}
	if( touch.state >= state_edit )
	{
		rgba_t color;
		MakeRGBA( color, 255, 255, 255, 255 );
		if( touch.edit )
		{
			float	x1 = touch.edit->x1,
					y1 = touch.edit->y1,
					x2 = touch.edit->x2,
					y2 = touch.edit->y2;
			IN_TouchCheckCoords( &x1, &y1, &x2, &y2 );
			IN_TouchDrawTexture( x1, y1, x2, y2, cls.fillImage, 0, 255, 0, 32 );
		}
		IN_TouchDrawTexture( 0, 0, GRID_X, GRID_Y, cls.fillImage, 255, 255, 255, 64 );
		if( touch.selection )
		{
			button = touch.selection;
			IN_TouchDrawTexture( B(x1), B(y1), B(x2), B(y2), cls.fillImage, 255, 0, 0, 64 );
			if( touch.showbuttons )
			{
				if( button->flags & TOUCH_FL_HIDE )
				{
					IN_TouchDrawTexture( 0, GRID_Y * 8, GRID_X * 2, GRID_Y * 10, touch.showtexture, 255, 255, 255, 255 );
					IN_TouchDrawText( GRID_X * 2.5, GRID_Y * 8.5, 0, 0, "Show", color );
				}
				else
				{
					IN_TouchDrawTexture( 0, GRID_Y * 8, GRID_X * 2, GRID_Y * 10, touch.hidetexture, 255, 255, 255, 255 );
					IN_TouchDrawText( GRID_X * 2.5, GRID_Y * 8.5, 0, 0, "Hide", color );
				}
			}
			Con_DrawString( 0, TO_SCRN_Y(GRID_Y * 11), "Selection:", color );
			Con_DrawString( Con_DrawString( 0, TO_SCRN_Y(GRID_Y*12), "Name: ", color ),
											   TO_SCRN_Y(GRID_Y*12), B(name), color );
			Con_DrawString( Con_DrawString( 0, TO_SCRN_Y(GRID_Y*13), "Texture: ", color ),
											   TO_SCRN_Y(GRID_Y*13), B(texturefile), color );
			Con_DrawString( Con_DrawString( 0, TO_SCRN_Y(GRID_Y*14), "Command: ", color ),
											   TO_SCRN_Y(GRID_Y*14), B(command), color );
		}
		if( touch.showbuttons )
		{
			// close
			IN_TouchDrawTexture( 0, GRID_Y * 2, GRID_X * 2, GRID_Y * 4, touch.closetexture, 255, 255, 255, 255 );
			//Con_DrawString( TO_SCRN_X( GRID_X * 2.5 ), TO_SCRN_Y( GRID_Y * 2.5 ), "Close", color );
			IN_TouchDrawText( GRID_X * 2.5, GRID_Y * 2.5, 0, 0, "Close", color );
			// reset
			IN_TouchDrawTexture( 0, GRID_Y * 5, GRID_X * 2, GRID_Y * 7, touch.resettexture, 255, 255, 255, 255 );
			//Con_DrawString( TO_SCRN_X( GRID_X * 2.5 ), TO_SCRN_Y( GRID_Y * 5.5 ), "Reset", color );
			IN_TouchDrawText( GRID_X * 2.5, GRID_Y * 5.5, 0, 0, "Reset", color );
		}
	}
	pglColor4ub( 255, 255, 255, 255 );
}

// clear move and selection state
void IN_TouchEditClear( void )
{
	if( touch.state < state_edit )
		return;
	touch.state = state_edit;
	if( touch.edit )
		touch.edit->finger = -1;
	touch.resize_finger = -1;
	touch.edit = NULL;
	touch.selection = NULL;
}

static void IN_TouchEditMove( touchEventType type, int fingerID, float x, float y, float dx, float dy )
{
	if( touch.edit->finger == fingerID )
	{
		if( type == event_up ) // shutdown button move
		{
			touchbutton2_t *button = touch.edit;
			IN_TouchCheckCoords( &B(x1), &B(y1), &B(x2), &B(y2) );
			IN_TouchEditClear();
			if( button->type == touch_command )
				touch.selection = button;
		}
		if( type == event_motion ) // shutdown button move
		{
			touch.edit->y1 += dy;
			touch.edit->y2 += dy;
			touch.edit->x1 += dx;
			touch.edit->x2 += dx;
		}
	}
	else 
	{
		if( type == event_down ) // enable resizing
		{
			if( touch.resize_finger == -1 )
			{
				touch.resize_finger = fingerID;
			}
		}
		if( type == event_up ) // disable resizing
		{
			if( touch.resize_finger == fingerID )
			{
				touch.resize_finger = -1;
			}
		}
		if( type == event_motion ) // perform resizing
		{
			if( touch.resize_finger == fingerID )
			{
				touch.edit->y2 += dy;
				touch.edit->x2 += dx;
			}
		}
	}
}

int IN_TouchEvent( touchEventType type, int fingerID, float x, float y, float dx, float dy )
{
	touchbutton2_t *button;
	
	// simulate menu mouse click
	if( cls.key_dest != key_game && !touch_in_menu->value )
	{
		// Hack for keyboard, hope it help
		if( cls.key_dest == key_console || cls.key_dest == key_message ) 
		{
#ifdef XASH_SDL
			SDL_StartTextInput();
#endif
			if( cls.key_dest == key_console )
			{
				static float y = 0;
				y += dy;
				if( dy > 0.4 )
					Con_Bottom();
				if( y > 0.01 )
				{
					Con_PageUp( 1 );
					y = 0;
				}
				if( y < -0.01 )
				{
					Con_PageDown( 1 );
					y = 0;
				}
			}
		}
		UI_MouseMove( TO_SCRN_X(x), TO_SCRN_Y(y) );
		//MsgDev( D_NOTE, "touch %d %d\n", TO_SCRN_X(x), TO_SCRN_Y(y) );
		if( type == event_down )
			Key_Event(241, 1);
		if( type == event_up )
			Key_Event(241, 0);
		return 0;
	}
	
	if( !touch.initialized || !touch_enable->value )
		return 0;

	if( touch.state == state_edit_move )
	{
		IN_TouchEditMove( type, fingerID, x, y, dx, dy );
		return 1;
	}

	// edit buttons are on y1
	if( ( type == event_down ) && ( touch.state == state_edit ) )
	{
		if( (x < GRID_X) && (y < GRID_Y) )
		{
			touch.showbuttons ^= true;
			return 1;
		}
		if( touch.showbuttons && ( x < GRID_X * 2 ) )
		{
			if( ( y > GRID_Y * 2 ) && ( y < GRID_Y * 4 )  ) // close button
			{
				IN_TouchDisableEdit_f();
				if( touch_in_menu->value )
				{
					Cvar_Set("touch_in_menu","0");
				}
				else
					IN_TouchWriteConfig();
			}
			if( ( y > GRID_Y * 5 ) && ( y < GRID_Y * 7 ) ) // reset button
			{
				IN_TouchReloadConfig_f();
			}
			if( ( y > GRID_Y * 8 ) && ( y < GRID_Y * 10 ) && touch.selection ) // hide button
				touch.selection->flags ^= TOUCH_FL_HIDE;
			return 1;
		}
		
	}
	for( button = touch.last; button  ; button = button->prev )
	{
		if( type == event_down )
		{
			if( ( x > button->x1 &&
				 x < button->x2 ) &&
				( y < button->y2 &&
				  y > button->y1 ) )
			{
				button->finger = fingerID;
				if( touch.state == state_edit )
				{
					// do not edit NOEDIT buttons
					if( button->flags & TOUCH_FL_NOEDIT )
						continue;
					touch.edit = button;
					touch.selection = NULL;
					// Make button last to bring it up
					if( ( button->next ) && ( button->type == touch_command ) )
					{
						if( button->prev )
							button->prev->next = button->next;
						else 
							touch.first = button->next;
						button->next->prev = button->prev;
						touch.last->next = button;
						button->prev = touch.last;
						button->next = NULL;
						touch.last = button;
					}
					touch.state = state_edit_move;
					return 1;
				}
				if( !IN_TouchIsVisible( button ) )
					continue;
				if( button->type == touch_command )
				{
					char command[256];
					Q_snprintf( command, 256, "%s\n", button->command, 256 );
					Cbuf_AddText( command );
				}
				if( button->type == touch_move )
				{
					if( touch.move_finger !=-1 )
					{
						button->finger = touch.move_finger;
						continue;
					}
					if( touch.look_finger == fingerID )
					{
						touchbutton2_t *newbutton;
						touch.move_finger = touch.look_finger = -1;
						//touch.move_finger = button->finger = -1;
						for( newbutton = touch.first; newbutton; newbutton = newbutton->next )
							if( ( newbutton->type == touch_move ) || ( newbutton->type == touch_look ) ) newbutton->finger = -1;
						MsgDev( D_NOTE, "Touch: touch_move on look finger %d!\n", fingerID );
						continue;
					}
					touch.move_finger = fingerID;
					touch.move_start_x = x;
					touch.move_start_y = y;
				}
				if( button->type == touch_look )
				{
					if( touch.look_finger !=-1 )
					{
						button->finger = touch.look_finger;
						continue;
					}
					if( touch.move_finger == fingerID )
					{
						touchbutton2_t *newbutton;
						// This is an error, try recover
						touch.move_finger = touch.look_finger = -1;
						for( newbutton = touch.first; newbutton; newbutton = newbutton->next )
							if( ( newbutton->type == touch_move ) || ( newbutton->type == touch_look ) ) newbutton->finger = -1;
						MsgDev( D_NOTE, "Touch: touch_look on move finger %d!\n", fingerID );
						continue;
					}
					touch.look_finger = fingerID;
				}
			}
		}
		if( type == event_up )
		{
			if( fingerID == button->finger )
			{
				button->finger = -1;
				if( ( button->type == touch_command ) && ( button->command[0] == '+' ) )
				{
					char command[256];
					Q_snprintf( command, 256, "%s\n", button->command, 256 );
					command[0] = '-';
					Cbuf_AddText( command );
				}
				if( button->type == touch_move )
				{
					touch.move_finger = -1;
					touch.forward = touch.side = 0;
				}
				if( button->type == touch_look )
				{
					touch.look_finger = -1;
				}
			}
		}
	}
	if( ( type == event_down ) && ( touch.state == state_edit ) )
		touch.selection = NULL;
	if( type == event_motion )
	{
		if( fingerID == touch.move_finger )
		{
			if( !touch_forwardzone->value )
				Cvar_SetFloat( "touch_forwardzone", 0.5 );
			if( !touch_sidezone->value )
				Cvar_SetFloat( "touch_sidezone", 0.3 );
			touch.forward = (touch.move_start_y - y) / touch_forwardzone->value;
			touch.side = (x - touch.move_start_x) / touch_sidezone->value ;
		}
		if( fingerID == touch.look_finger )
			touch.yaw -=dx * touch_yaw->value, touch.pitch +=dy * touch_pitch->value;
	}
	return 1;
}

void IN_TouchMove( float * forward, float *side, float *yaw, float *pitch )
{
	*forward += touch.forward;
	*side += touch.side;
	*yaw += touch.yaw;
	*pitch += touch.pitch;
	touch.yaw = touch.pitch = 0;
}

void IN_TouchShutdown( void )
{
	if( !touch.initialized ) 
		return;
	IN_TouchRemoveAll_f();
	Cmd_RemoveCommand( "touch_addbutton" );
	Cmd_RemoveCommand( "touch_removebutton" );
	Cmd_RemoveCommand( "touch_enableedit" );
	Cmd_RemoveCommand( "touch_disableedit" );
	Cmd_RemoveCommand( "touch_settexture" );
	Cmd_RemoveCommand( "touch_setcolor" );
	Cmd_RemoveCommand( "touch_setcommand" );
	Cmd_RemoveCommand( "touch_setflags" );
	Cmd_RemoveCommand( "touch_show" );
	Cmd_RemoveCommand( "touch_hide" );
	Cmd_RemoveCommand( "touch_list" );
	Cmd_RemoveCommand( "touch_removeall" );
	Cmd_RemoveCommand( "touch_loaddefaults" );
	Cmd_RemoveCommand( "touch_roundall" );
	Cmd_RemoveCommand( "touch_exportconfig" );
	Cmd_RemoveCommand( "touch_set_stroke" );
	Cmd_RemoveCommand( "touch_setclientonly" );
	Cmd_RemoveCommand( "touch_reloadconfig" );
	Cmd_RemoveCommand( "touch_writeconfig" );

	touch.initialized = false;
	Mem_FreePool( &touch.mempool );
}
