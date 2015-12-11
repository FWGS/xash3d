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
	float left, right, top, bottom, alpha;
	// Button texture
	int texture;
	char texturefile[256];
	char command[256];
	char name[32];
	int finger;
	// Next
	struct touchbutton2_s *next;
} touchbutton2_t;

struct touch_s
{
	void *mempool;
	touchbutton2_t *first;
	touchState state;
	int resize_finger;
	int look_finger;
	int move_finger;
	float move_start_x;
	float move_start_y;
	touchbutton2_t *edit;
} touch;

void IN_AddButton( const char *name,  const char *texture, const char *command, float left, float top, float right, float bottom )
{
	touchbutton2_t *button = Mem_Alloc( touch.mempool, sizeof( touchbutton2_t ) );
	MsgDev( D_NOTE, "IN_AddButton()\n");
	button->texture = 0;
	Q_strncpy( button->texturefile, texture, 256 );
	Q_strncpy( button->name, name, 32 );
	button->left = left;
	button->top = top;
	button->right = right;
	button->bottom = bottom;
	button->command[0] = 0;
	// check keywords
	if( !Q_strcmp( command, "_look" ) )
		button->type = touch_look;
	if( !Q_strcmp( command, "_move" ) )
		button->type = touch_move;
	Q_strncpy( button->command, command, sizeof( button->command ) );
	button->finger = -1;
	button->next = touch.first;
	touch.first = button;
}

void IN_TouchAddButton_f()
{
	if( Cmd_Argc( ) != 8 )
		Msg( "Usage: touch_addbutton <name> <texture> <command> <left> <top> <right> <bottom>\n");
	IN_AddButton( Cmd_Argv(1), Cmd_Argv(2), Cmd_Argv(3),
		Q_atof( Cmd_Argv(4) ), Q_atof( Cmd_Argv(5) ), 
		Q_atof( Cmd_Argv(6) ), Q_atof( Cmd_Argv(7) ) );
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

void IN_TouchRemoveButton_f()
{
	touchbutton2_t *button;
	char *name;
	if( !touch.first )
		return;
	name = Cmd_Argv( 1 );
	if( !Q_strncmp( touch.first->name, name, 32 ) )
	{
		touchbutton2_t *remove = touch.first;
		touch.first = remove->next;
		Mem_Free( remove );
	}
	for ( button = touch.first; button->next; button = button->next )
	{
		if( !Q_strncmp( button->next->name, name, 32 ) )
		{
			touchbutton2_t *remove = button->next;
			button->next = remove->next;
			Mem_Free( remove );
			return;
		}
	}
	
}

void IN_TouchRemoveAll_f()
{
	while( touch.first )
	{
		touchbutton2_t *remove = touch.first;
		touch.first = touch.first->next;
		Mem_Free ( remove );
	}
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
}

void IN_TouchDraw( void )
{
	touchbutton2_t *button;
	GL_SetRenderMode( kRenderTransAdd );
	for( button = touch.first; button; button = button->next )
	{
		if( button->texturefile[0] )
		{
			if( !button->texture )
				button->texture = GL_LoadTexture( button->texturefile, NULL, 0, 0, NULL );
			//MsgDev( D_NOTE, "IN_TouchDraw : %d\n", button->texture );
			R_DrawStretchPic( scr_width->integer * button->left,
				scr_height->integer * button->top,
				scr_width->integer * (button->right - button->left),
				scr_height->integer * (button->bottom - button->top),
				0, 0, 1, 1, button->texture);
		}
		if( touch.state >= state_edit )
		{
			GL_SetRenderMode( kRenderTransAdd );
			pglColor4ub( 255, 255, 0, 32 );
			R_DrawStretchPic( scr_width->integer * button->left,
				scr_height->integer * button->top,
				scr_width->integer * (button->right - button->left),
				scr_height->integer * (button->bottom - button->top),
				0, 0, 1, 1, cls.fillImage );
			pglColor4ub( 255, 255, 255, 255 );
			GL_SetRenderMode( kRenderTransAlpha );
		}
	}
	if( touch.edit )
	{
		GL_SetRenderMode( kRenderTransAdd );
		pglColor4ub( 255, 255, 0, 64 );
		R_DrawStretchPic( scr_width->integer * touch.edit->left,
			scr_height->integer * touch.edit->top,
			scr_width->integer * (touch.edit->right - touch.edit->left),
			scr_height->integer * (touch.edit->bottom - touch.edit->top),
			0, 0, 1, 1, cls.fillImage );
		pglColor4ub( 255, 255, 255, 255 );
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
		return;
	if( touch.state == state_edit_move )
	{
		if( touch.edit->finger == fingerID )
		{
			if( type == event_up ) // shutdown button move
			{
				touch.state = state_edit;
				touch.edit->finger = -1;
				touch.resize_finger = -1;
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
			if( type == event_motion ) // disable resizing
			{
				if( touch.resize_finger == fingerID )
				{
					touch.edit->bottom += dy;
					touch.edit->right += dx;
				}
			}
		}
		return;
	}
	for( button = touch.first; button  ; button = button->next )
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
					touch.state = state_edit_move;
					return;
				}
				if( button->type == touch_command )
				{
					Cbuf_AddText( button->command );
				}
				if( button->type == touch_move )
				{
					MsgDev( D_NOTE, "StartMove\n");
					if( touch.look_finger == fingerID )
					{
						touch.move_finger = touch.look_finger = -1;
						return;
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
					MsgDev( D_NOTE, "StartLook\n");
					if( touch.move_finger == fingerID )
					{
						touch.move_finger = touch.look_finger = -1;
						return;
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
			clgame.dllFuncs.pfnMoveEvent( (touch.move_start_y - y) * 2, (x - touch.move_start_x) * 2 );
		if( fingerID == touch.look_finger )
			clgame.dllFuncs.pfnLookEvent( -dx * 300, dy * 300 );
	}
	return 0;
}

void IN_TouchMove( float * forward, float *side, float *pitch, float *yaw )
{
}
