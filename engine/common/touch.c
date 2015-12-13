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

typedef struct touchbutton2_s
{
	// Touch button type: tap, stick or slider
	touchButtonType type;
	// Field of button in pixels
	float left, right, top, bottom;
	// Button texture
	int texture;
	byte r,g,b,a;
	char texturefile[256];
	char command[256];
	char name[32];
	int finger;
	// Double-linked list
	struct touchbutton2_s *next;
	struct touchbutton2_s *prev;
} touchbutton2_t;

struct touch_s
{
	void *mempool;
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
	int resize_finger;
} touch;

convar_t *touch_pitch;
convar_t *touch_yaw;
convar_t *touch_forwardzone;
convar_t *touch_sidezone;

void IN_TouchRemoveButton( const char *name )
{
	touchbutton2_t *button;
	if( !touch.first )
		return;
	for ( button = touch.first; button; button = button->next )
	{
		if( !Q_strncmp( button->name, name, 32 ) )
		{
			if ( button == touch.first )
				touch.first = button->next;
			if ( button == touch.last )
				touch.last = button->prev;
			Mem_Free( button );
			return;
		}
	}
}

void IN_TouchRemoveButton_f()
{
	IN_TouchRemoveButton( Cmd_Argv( 1 ) );
}

void IN_TouchRemoveAll_f()
{
	while( touch.first )
	{
		touchbutton2_t *remove = touch.first;
		touch.first = touch.first->next;
		Mem_Free ( remove );
	}
	touch.last = NULL;
}

void IN_AddButton( const char *name,  const char *texture, const char *command, float left, float top, float right, float bottom, byte r, byte g, byte b, byte a )
{
	touchbutton2_t *button = Mem_Alloc( touch.mempool, sizeof( touchbutton2_t ) );
	MsgDev( D_NOTE, "IN_AddButton()\n");
	button->texture = 0;
	Q_strncpy( button->texturefile, texture, 256 );
	Q_strncpy( button->name, name, 32 );
	IN_TouchRemoveButton( name ); //replace if exist
	button->left = left;
	button->top = top;
	button->right = right;
	button->bottom = bottom;
	button->r = r;
	button->g = g;
	button->b = b;
	button->a = a;
	button->command[0] = 0;
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
}

void IN_TouchAddButton_f()
{
	int argc = Cmd_Argc( );
	if( argc == 12 )
	{
		IN_AddButton( Cmd_Argv(1), Cmd_Argv(2), Cmd_Argv(3),
			Q_atof( Cmd_Argv(4) ), Q_atof( Cmd_Argv(5) ), 
			Q_atof( Cmd_Argv(6) ), Q_atof( Cmd_Argv(7) ) ,
			Q_atoi( Cmd_Argv(8) ), Q_atoi( Cmd_Argv(9) ), 
			Q_atoi( Cmd_Argv(10) ), Q_atoi( Cmd_Argv(11) ) );
		return;
	}	
	if( argc == 8 )
	{
		IN_AddButton( Cmd_Argv(1), Cmd_Argv(2), Cmd_Argv(3),
			Q_atof( Cmd_Argv(4) ), Q_atof( Cmd_Argv(5) ), 
			Q_atof( Cmd_Argv(6) ), Q_atof( Cmd_Argv(7) ),
			255, 255, 255, 255 );
		return;
	}
	if( argc == 4 )
	{
		IN_AddButton( Cmd_Argv(1), Cmd_Argv(2), Cmd_Argv(3), 0.4, 0.4, 0.6, 0.6, 255, 255, 255, 255 );
		return;
	}
	Msg( "Usage: touch_addbutton <name> <texture> <command> [<left> <top> <right> <bottom> [ r g b a] ]\n" );
}

void IN_TouchEnableEdit_f()
{
	if( touch.state == state_none )
		touch.state = state_edit;
	touch.resize_finger = touch.move_finger = touch.look_finger = -1;
}

void IN_TouchDisableEdit_f()
{
	touch.state = state_none;
	if( touch.edit )
		touch.edit->finger = -1;
	touch.resize_finger = touch.move_finger = touch.look_finger = -1;
}

void IN_TouchInit( void )
{
	touch.mempool = Mem_AllocPool( "Touch" );
	touch.first = NULL;
	MsgDev( D_NOTE, "IN_TouchInit()\n");
	touch.move_finger = touch.resize_finger = touch.look_finger = -1;
	touch.state = state_none;
	Cmd_AddCommand( "touch_addbutton", IN_TouchAddButton_f, "Add native touch button" );
	Cmd_AddCommand( "touch_removebutton", IN_TouchRemoveButton_f, "Remove native touch button" );
	Cmd_AddCommand( "touch_enableedit", IN_TouchEnableEdit_f, "Enable button editing mode" );
	Cmd_AddCommand( "touch_disableedit", IN_TouchDisableEdit_f, "Disable button editing mode" );
	Cmd_AddCommand( "touch_removeall", IN_TouchRemoveAll_f, "Remove all buttons" );
	touch_forwardzone = Cvar_Get( "touch_forwardzone", "0.5", CVAR_ARCHIVE, "forward touch zone" );
	touch_sidezone = Cvar_Get( "touch_sidezone", "0.3", CVAR_ARCHIVE, "side touch zone" );
	touch_pitch = Cvar_Get( "touch_pitch", "20", CVAR_ARCHIVE, "touch pitch sensitivity" );
	touch_yaw = Cvar_Get( "touch_yaw", "50", CVAR_ARCHIVE, "touch yaw sensitivity" );
}

void IN_TouchDrawTexture ( float left, float top, float right, float bottom, int texture, byte r, byte g, byte b, byte a )
{
	pglColor4ub( r, g, b, a );
	if( left >= right )
		return;
	if( top >= bottom )
		return;
	pglColor4ub( r, g, b, a );
	R_DrawStretchPic( scr_width->integer * left,
		scr_height->integer * top,
		scr_width->integer * (right - left),
		scr_height->integer * (bottom - top),
		0, 0, 1, 1, texture );
}

static void IN_TouchCheckCoords( float *left, float *top, float *right, float *bottom  )
{
	/// TODO: grid check here
	if( *right - *left < 0.1 )
		*right = *left + 0.1;
	if( *bottom - *top < 0.1 )
		*bottom = *top + 0.1;
	if( *left < 0 )
		*right -= *left, *left = 0;
	if( *top < 0 )
		*bottom -= *top, *top = 0;
	if( *bottom > 1 )
		*top -= *bottom - 1, *bottom = 1;
	if( *right > 1 )
		*left -= *right - 1, *right = 1;
		
}
// code looks smaller with it
#define B(x) button->x
void IN_TouchDraw( void )
{
	touchbutton2_t *button;
	GL_SetRenderMode( kRenderTransTexture );
	for( button = touch.first; button; button = button->next )
	{
		if( button->texturefile[0] )
		{
			if( !button->texture )
				button->texture = GL_LoadTexture( button->texturefile, NULL, 0, 0, NULL );
			IN_TouchDrawTexture( B(left), B(top), B(right), B(bottom), B(texture), B(r), B(g), B(b), B(a) );
		}
		if( touch.state >= state_edit )
		{
			IN_TouchDrawTexture( B(left), B(top), B(right), B(bottom), cls.fillImage, 255, 255, 0, 32 );
		}
	}
	if( touch.edit )
	{
		float	left = touch.edit->left,
				top = touch.edit->top,
				right = touch.edit->right,
				bottom = touch.edit->bottom;
		IN_TouchCheckCoords( &left, &top, &right, &bottom );
		IN_TouchDrawTexture( left, top, right, bottom, cls.fillImage, 255, 255, 0, 64 );
	}
	pglColor4ub( 255, 255, 255, 255 );
}

static void IN_TouchEditMove( touchEventType type, int fingerID, float x, float y, float dx, float dy )
{
	if( touch.edit->finger == fingerID )
	{
		if( type == event_up ) // shutdown button move
		{
			touch.state = state_edit;
			touch.edit->finger = -1;
			touch.resize_finger = -1;
			IN_TouchCheckCoords( &touch.edit->left, &touch.edit->top,
				&touch.edit->right, &touch.edit->bottom );
			touch.edit = NULL;
		}
		if( type == event_motion ) // shutdown button move
		{
			touch.edit->top += dy;
			touch.edit->bottom += dy;
			touch.edit->left += dx;
			touch.edit->right += dx;
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
				touch.edit->bottom += dy;
				touch.edit->right += dx;
			}
		}
	}
}

int IN_TouchEvent( touchEventType type, int fingerID, float x, float y, float dx, float dy )
{
	touchbutton2_t *button;
	// Find suitable button
	//MsgDev( D_NOTE, "IN_TouchEvent( %d, %d, %f, %f, %f, %f )\n", type, fingerID, x, y, dx, dy );
	UI_MouseMove( scr_width->integer * x, scr_height->integer * y );
	// simulate mouse click
	/*if( type == event_down )
		Key_Event(241, 1);
	if( type == event_up )
		Key_Event(241, 0);*/
	if( cls.key_dest != key_game )
		return 0;
	if( touch.state == state_edit_move )
	{
		IN_TouchEditMove( type, fingerID, x, y, dx, dy );
		return 1;
	}
	for( button = touch.last; button  ; button = button->prev )
	{
		if( type == event_down )
		{
			if( ( x > button->left &&
				 x < button->right ) &&
				( y < button->bottom &&
				  y > button->top ) )
			{
				button->finger = fingerID;
				if( touch.state == state_edit )
				{
					touch.edit = button;
					// Make button last to bring it up
					if( button->next )
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
				if( button->type == touch_command )
				{
					Cbuf_AddText( button->command );
				}
				if( button->type == touch_move )
				{
					if( touch.look_finger == fingerID )
					{
						touch.move_finger = touch.look_finger = -1;
						return 1;
					}
					if( touch.move_finger !=-1 )
						button->finger = -1;
					else
					{
						touch.move_finger = fingerID;
						touch.move_start_x = x;
						touch.move_start_y = y;
					}
				}
				if( button->type == touch_look )
				{
					if( touch.move_finger == fingerID )
					{
						touch.move_finger = touch.look_finger = -1;
						return 1;
					}
					if( touch.look_finger !=-1 )
						button->finger = -1;
					else
						touch.look_finger = fingerID;
				}
			}
		}
		if( type == event_up )
		{
			if( fingerID == button->finger )
			{
				button->finger = -1;
				if( button->type == touch_command )
				{
					char command[256];
					Q_strncpy( command,  button->command, 256 );
					if( command[0] == '+' ) command [0] = '-';
					Cbuf_AddText( command );
				}
				if( button->type == touch_move )
				{
					MsgDev( D_NOTE, "StopMove\n");
					touch.move_finger = -1;
					touch.forward = touch.side = 0;
				}
				if( button->type == touch_look )
				{
					MsgDev( D_NOTE, "StopLook\n");
					touch.look_finger = -1;
				}
			}
		}
	}
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
