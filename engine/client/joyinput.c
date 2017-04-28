/*
joyinput.c - joystick common input code

Copyright (C) 2016 a1batross

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
#include "input.h"
#include "keydefs.h"
#include "joyinput.h"
#include "client.h"
#include "gl_local.h"

#if defined(XASH_SDL)
#include "platform/sdl/events.h"
#endif

#ifndef SHRT_MAX
#define SHRT_MAX 0x7FFF
#endif

typedef enum engineAxis_e
{
	JOY_AXIS_SIDE = 0,
	JOY_AXIS_FWD,
	JOY_AXIS_PITCH,
	JOY_AXIS_YAW,
	JOY_AXIS_RT,
	JOY_AXIS_LT,
	JOY_AXIS_NULL
} engineAxis_t;

#define MAX_AXES JOY_AXIS_NULL

// index - axis num come from event
// value - inner axis
static engineAxis_t joyaxesmap[MAX_AXES] =
{
	JOY_AXIS_SIDE,  // left stick, x
	JOY_AXIS_FWD,   // left stick, y
	JOY_AXIS_PITCH, // right stick, y
	JOY_AXIS_YAW,   // right stick, x
	JOY_AXIS_RT,    // right trigger
	JOY_AXIS_LT     // left trigger
};

static struct joy_axis_s
{
	short val;
	short prevval;
} joyaxis[MAX_AXES] = { 0 };
static qboolean initialized = false, forcedisable = false;
static convar_t *joy_enable;
static byte currentbinding; // add posibility to remap keys, to place it in joykeys[]

/* On-screen keyboard:
 *
 * 4 lines with 13 buttons each
 * Left trigger == backspace
 * Right trigger == space
 * Any button press is button press on keyboard
 *
 * Our layout:
 *  0  1  2  3  4  5  6  7  8  9  10 11 12
 * +--+--+--+--+--+--+--+--+--+--+--+--+--+
 * |` |1 |2 |3 |4 |5 |6 |7 |8 |9 |0 |- |= | 0
 * +--+--+--+--+--+--+--+--+--+--+--+--+--+
 * |q |w |e |r |t |y |u |i |o |p |[ |] |\ | 1
 * +--+--+--+--+--+--+--+--+--+--+--+--+--+
 * |CL|a |s |d |f |g |h |j |k |l |; |' |BS| 2
 * +--+--+--+--+--+--+--+--+--+--+--+--+--+
 * |SH|z |x |c |v |b |n |m |, |. |/ |SP|EN| 3
 * +--+--+--+--+--+--+--+--+--+--+--+--+--+
 */

#define MAX_OSK_ROWS 13
#define MAX_OSK_LINES 4

enum
{
	OSK_DEFAULT = 0,
	OSK_UPPER, // on caps, shift
	/*
	OSK_RUSSIAN,
	OSK_RUSSIAN_UPPER,
	*/
	OSK_LAST
};

enum
{
	OSK_CAPS_LOCK = 16,
	OSK_SHIFT,
	OSK_BACKSPACE,
	OSK_ENTER,
	OSK_SPECKEY_LAST
};
static const char *osk_keylayout[][4] =
{
	{
			  "`1234567890-=",  // 13
			  "qwertyuiop[]\\", // 13
		"\x10" "asdfghjkl;'" "\x12",   // 11 + caps on a left, enter on a right
		"\x11" "zxcvbnm,./ " "\x13"     // 10 + esc on left + shift on a left/right
	},
	{
			  "~!@#$%^&*()_+",
			  "QWERTYUIOP{}|",
		"\x10" "ASDFGHJKL:\"" "\x12",
		"\x11" "ZXCVBNM<>? "  "\x13"
	}
};
static struct {
	char x;
	char y;
	char val;
} osk_curbutton;
static byte osk_enable = false; // debugging, remove later
static byte osk_curlayout;
static byte osk_shift; // if true, come back to first layout

float IN_TouchDrawText( float x1, float y1, float x2, float y2, const char *s, byte *color, float size );
float IN_TouchDrawCharacter( float x, float y, int number, float size );

convar_t *joy_pitch;
convar_t *joy_yaw;
convar_t *joy_forward;
convar_t *joy_side;
convar_t *joy_found;
convar_t *joy_index;
convar_t *joy_lt_threshold;
convar_t *joy_rt_threshold;
convar_t *joy_osk_enable;
convar_t *joy_axis_binding;

/*
============
Joy_HatMotionEvent

DPad events
============
*/
void Joy_HatMotionEvent( int id, byte hat, byte value )
{
	if( !initialized )
		return;

	if( osk_enable )
	{
		if( value & JOY_HAT_UP && --osk_curbutton.y < 0 )
			osk_curbutton.y = 0;
		else if( value & JOY_HAT_DOWN && ++osk_curbutton.y >= MAX_OSK_LINES )
			osk_curbutton.y = MAX_OSK_LINES - 1;

		if( value & JOY_HAT_LEFT && --osk_curbutton.x < 0 )
			osk_curbutton.x = 0;
		else if( value & JOY_HAT_RIGHT && ++osk_curbutton.x >= MAX_OSK_ROWS )
			osk_curbutton.x = MAX_OSK_ROWS - 1;

		osk_curbutton.val = osk_keylayout[osk_curlayout][osk_curbutton.y][osk_curbutton.x];
	}
	else
	{
		if( value & JOY_HAT_UP )
			Key_Event( K_UPARROW, true );
		else Key_Event( K_UPARROW, false );

		if( value & JOY_HAT_DOWN )
			Key_Event( K_DOWNARROW, true );
		else Key_Event( K_DOWNARROW, false );

		if( value & JOY_HAT_LEFT )
			Key_Event( K_LEFTARROW, true );
		else Key_Event( K_LEFTARROW, false );

		if( value & JOY_HAT_RIGHT )
			Key_Event( K_RIGHTARROW, true );
		else Key_Event( K_RIGHTARROW, false );
	}
}

/*
=============
Joy_AxisMotionEvent

Axis events
=============
*/
void Joy_AxisMotionEvent( int id, byte axis, short value )
{
	byte engineAxis;
	int trigButton, trigThreshold;

	if( !initialized )
		return;

	if( axis >= MAX_AXES )
	{
		MsgDev( D_INFO, "Only 6 axes is supported\n" );
		return;
	}

	engineAxis = joyaxesmap[axis]; // convert to engine inner axis control

	if( engineAxis == JOY_AXIS_NULL )
		return;

	if( value == joyaxis[engineAxis].val )
		return; // it is not an update

	// update axis values
	joyaxis[engineAxis].prevval = joyaxis[engineAxis].val;
	joyaxis[engineAxis].val = value;

	switch( engineAxis )
	{
	case JOY_AXIS_RT:
		trigButton = K_JOY2;
		trigThreshold = joy_rt_threshold->integer;
		break;
	case JOY_AXIS_LT:
		trigButton = K_JOY1;
		trigThreshold = joy_lt_threshold->integer;
		break;
	default:
		return; // next code only for triggers
	}

	if( joyaxis[engineAxis].val > trigThreshold &&
		joyaxis[engineAxis].prevval <= trigThreshold ) // ignore random press
	{
		if( osk_enable )
		{
			if( engineAxis == JOY_AXIS_LT )
				Key_Event( K_BACKSPACE, true );
			else
			{
				Con_CharEvent( ' ' );
				if( cls.key_dest == key_menu )
					UI_CharEvent ( ' ' );
			}
		}
		else Key_Event( trigButton, true );
	}
	else if( joyaxis[engineAxis].val < trigThreshold &&
			 joyaxis[engineAxis].prevval >= trigThreshold ) // we're unpressing (inverted)
	{
		if( osk_enable && engineAxis == JOY_AXIS_LT )
			Key_Event( K_BACKSPACE, false );
		else Key_Event( trigButton, false );
	}
}

/*
=============
Joy_BallMotionEvent

Trackball events. UNDONE
=============
*/
void Joy_BallMotionEvent( int id, byte ball, short xrel, short yrel )
{
	if( !initialized )
		return;

	MsgDev( D_INFO, "TODO: Write ball motion code!\n" );
}

/*
=============
Joy_ButtonEvent

Button events
=============
*/
void Joy_ButtonEvent( int id, byte button, byte down )
{
	if( !initialized )
		return;

	if( osk_enable ) // On-Screen Keyboard code.
	{
		switch( osk_curbutton.val )
		{
		case OSK_ENTER:
			Key_Event( K_ENTER, down );
			osk_enable = false; // TODO: handle multiline
			break;
		case OSK_SHIFT:
		case OSK_CAPS_LOCK:
			if( !down )
				break;

			if( osk_curlayout & 1 )
				osk_curlayout--;
			else
				osk_curlayout++;

			osk_shift = osk_curbutton.val == OSK_SHIFT;
			osk_curbutton.val = osk_keylayout[osk_curlayout][osk_curbutton.y][osk_curbutton.x];
			break;
		case OSK_BACKSPACE:
			Key_Event( K_BACKSPACE, down ); break;
		default:
		{
			int ch;

			if( !down )
			{
				if( osk_shift && osk_curlayout & 1 )
					osk_curlayout--;

				osk_shift = false;
				osk_curbutton.val = osk_keylayout[osk_curlayout][osk_curbutton.y][osk_curbutton.x];
				break;
			}

			if( !Q_stricmp( cl_charset->string, "utf-8" ) )
				ch = (unsigned char)osk_curbutton.val;
			else
				ch = Con_UtfProcessCharForce( (unsigned char)osk_curbutton.val );

			if( !ch )
				break;

			Con_CharEvent( ch );
			if( cls.key_dest == key_menu )
				UI_CharEvent ( ch );

			break;
		}
		}
		return;
	}

	// generic game button code.
	if( button > 32 )
	{
		int origbutton = button;
		button = button % 32 + K_AUX1;

		MsgDev( D_INFO, "Only 32 joybuttons is supported, converting %i button ID to %s\n", origbutton, Key_KeynumToString( button ) );
	}
	else button += K_AUX1;

	Key_Event( button, down );
}

/*
=============
Joy_RemoveEvent

Called when joystick is removed. For future expansion
=============
*/
void Joy_RemoveEvent( int id )
{
	if( !forcedisable && initialized && joy_found->integer )
		Cvar_SetFloat("joy_found", joy_found->value - 1.0f);
}

/*
=============
Joy_RemoveEvent

Called when joystick is removed. For future expansion
=============
*/
void Joy_AddEvent( int id )
{
	if( forcedisable )
		return;

	initialized = true;

	Cvar_SetFloat("joy_found", joy_found->value + 1.0f);
}

/*
=============
Joy_FinalizeMove

Append movement from axis. Called everyframe
=============
*/
void Joy_FinalizeMove( float *fw, float *side, float *dpitch, float *dyaw )
{
	if( !initialized || !joy_enable->integer )
		return;

	if( joy_axis_binding->modified )
	{
		char bind[7] = { 0 }; // fill it with zeros
		int i;
		Q_strncpy( bind, joy_axis_binding->string, sizeof(bind) );

		for( i = 0; i < sizeof(bind); i++ )
		{
			switch( bind[i] )
			{
			case 's': joyaxesmap[i] = JOY_AXIS_SIDE; break;
			case 'f': joyaxesmap[i] = JOY_AXIS_FWD; break;
			case 'y': joyaxesmap[i] = JOY_AXIS_YAW; break;
			case 'p': joyaxesmap[i] = JOY_AXIS_PITCH; break;
			case 'r': joyaxesmap[i] = JOY_AXIS_RT; break;
			case 'l': joyaxesmap[i] = JOY_AXIS_LT; break;
			default : joyaxesmap[i] = JOY_AXIS_NULL; break;
			}
		}
		joy_axis_binding->modified = false;
	}

	*fw     -= joy_forward->value * (float)joyaxis[JOY_AXIS_FWD ].val/(float)SHRT_MAX;  // must be form -1.0 to 1.0
	*side   += joy_side->value    * (float)joyaxis[JOY_AXIS_SIDE].val/(float)SHRT_MAX;
#if !defined(XASH_SDL)
	*dpitch += joy_pitch->value * (float)joyaxis[JOY_AXIS_PITCH].val/(float)SHRT_MAX * host.realframetime;  // abs axis rotate is frametime related
	*dyaw   -= joy_yaw->value   * (float)joyaxis[JOY_AXIS_YAW  ].val/(float)SHRT_MAX * host.realframetime;
#else
	// HACKHACK: SDL have inverted look axis.
	*dpitch -= joy_pitch->value * (float)joyaxis[JOY_AXIS_PITCH].val/(float)SHRT_MAX * host.realframetime;
	*dyaw   += joy_yaw->value   * (float)joyaxis[JOY_AXIS_YAW  ].val/(float)SHRT_MAX * host.realframetime;
#endif
}

/*
=============
Joy_Init

Main init procedure
=============
*/
void Joy_Init( void )
{


	joy_pitch   = Cvar_Get( "joy_pitch",   "100.0", CVAR_ARCHIVE, "joystick pitch sensitivity" );
	joy_yaw     = Cvar_Get( "joy_yaw",     "100.0", CVAR_ARCHIVE, "joystick yaw sensitivity" );
	joy_side    = Cvar_Get( "joy_side",    "1.0", CVAR_ARCHIVE, "joystick side sensitivity. Values from -1.0 to 1.0" );
	joy_forward = Cvar_Get( "joy_forward", "1.0", CVAR_ARCHIVE, "joystick forward sensitivity. Values from -1.0 to 1.0" );

	joy_lt_threshold = Cvar_Get( "joy_lt_threshold", "-16384", CVAR_ARCHIVE, "left trigger threshold. Value from -32768 to 32767");
	joy_rt_threshold = Cvar_Get( "joy_rt_threshold", "-16384", CVAR_ARCHIVE, "right trigger threshold. Value from -32768 to 32767" );
	joy_osk_enable   = Cvar_Get( "joy_osk_enable", "0", CVAR_ARCHIVE, "enable built-in on-screen keyboard" );
	joy_axis_binding = Cvar_Get( "joy_axis_binding", "sfpyrl", CVAR_ARCHIVE, "axis hardware id to engine inner axis binding, "
																			 "s - side, f - forward, y - yaw, p - pitch, r - left trigger, l - right trigger" );
	joy_found   = Cvar_Get( "joy_found", "0", CVAR_READ_ONLY, "total num of connected joysticks" );
	// we doesn't loaded config.cfg yet, so this cvar is not archive.
	// change by +set joy_index in cmdline
	joy_index   = Cvar_Get( "joy_index", "0", CVAR_READ_ONLY, "current active joystick" );

	joy_enable = Cvar_Get( "joy_enable", "1", CVAR_ARCHIVE, "enable joystick" );

	if( Sys_CheckParm("-nojoy" ) )
	{
		forcedisable = true;
		return;
	}

#if defined(XASH_SDL)
	// SDL can tell us about connected joysticks
	Cvar_SetFloat( "joy_found", SDLash_JoyInit( joy_index->integer ) );

#elif defined(ANDROID)
	// Initalized after first Joy_AddEvent
#else
#warning "Any platform must implement platform-dependent JoyInit, start event system. Otherwise no joystick support"
#endif

	if( joy_found->integer > 0 )
		initialized = true;
	else
		initialized = false;
}

/*
===========
Joy_Shutdown

Shutdown joystick code
===========
*/
void Joy_Shutdown( void )
{
	Cvar_SetFloat( "joy_found", 0 );

	initialized = false;
}

/*
=============
Joy_EnableTextInput

Enables built-in IME
=============
*/
void Joy_EnableTextInput( qboolean enable, qboolean force )
{
	qboolean old = osk_enable;

	if( !initialized || !joy_osk_enable->integer )
		return;

	osk_enable = enable;

	if( osk_enable && (!old || force) )
	{
		osk_curlayout = 0;
		osk_curbutton.x = 0;
		osk_curbutton.y = 1;
		osk_curbutton.val = 'q';
	}
}

/*
============
Joy_DrawSymbolButton

Draw button with symbol on it
============
*/
void Joy_DrawSymbolButton( int symb, float x, float y, float width, float height )
{
	char str[] = {symb & 255, 0};
	byte color[] = { 255, 255, 255, 255 };
	int x1 = x * scr_width->integer,
		y1 = y * scr_height->integer,
		w = width * scr_width->integer,
		h = height * scr_height->integer;

	if( symb == osk_curbutton.val )
	{
		CL_FillRGBABlend( x1, y1, w, h, 255, 160, 0, 100 );
	}

	if( !symb || symb == ' ' || symb >= OSK_CAPS_LOCK && symb < OSK_SPECKEY_LAST )
		return;

	if( symb != ';' ) // HACKHACK: override IN_TouchDrawText bug
	{
		IN_TouchDrawText( x, y, x + width, y + height, str, color, 1.3f );
	}
	else
	{
		GL_SetRenderMode( kRenderTransAdd );
		pglColor4ub( color[0] * ( (float)color[3] /255.0f ), color[1] * ( (float)color[3] /255.0f ),
				color[2] * ( (float)color[3] /255.0f ), 255 );

		IN_TouchDrawCharacter( x, y, ';', 1.3f );
		GL_SetRenderMode( kRenderTransTexture );
	}
}

/*
=============
Joy_DrawSpecialButton

Draw special button, like shift, enter or esc
=============
*/
void Joy_DrawSpecialButton( const char *name, float x, float y, float width, float height )
{
	byte color[] = { 255, 255, 255, 255 };

	IN_TouchDrawText( x, y, x + width, y + height, name, color, 1.3f );
}


/*
=============
Joy_DrawOnScreenKeyboard

Draw on screen keyboard, if enabled
=============
*/
#define X_START 0.1347475f
#define Y_START 0.667f
#define X_STEP 0.05625
#define Y_STEP 0.0625
void Joy_DrawOnScreenKeyboard( void )
{
	const char **curlayout = osk_keylayout[osk_curlayout]; // shortcut :)
	float  x, y;
	int i, j;

	if( !osk_enable )
		return;

	// draw keyboard
	CL_FillRGBABlend( X_START * scr_width->integer, Y_START * scr_height->integer,
					  X_STEP * MAX_OSK_ROWS * scr_width->integer,
					  Y_STEP * MAX_OSK_LINES * scr_height->integer, 100, 100, 100, 100 );

	for( y = Y_START,     j = 0; j < MAX_OSK_LINES; j++, y += Y_STEP )
		for( x = X_START, i = 0; i < MAX_OSK_ROWS;  i++, x += X_STEP )
			Joy_DrawSymbolButton( curlayout[j][i], x, y, X_STEP, Y_STEP );

	Joy_DrawSpecialButton( "Caps\nLock",   X_START,               Y_START + Y_STEP * 2, X_STEP, Y_STEP );
	Joy_DrawSpecialButton( "Back\nSpace",  X_START + X_STEP * 12, Y_START + Y_STEP * 2, X_STEP, Y_STEP );

	Joy_DrawSpecialButton( "Shift", X_START,               Y_START + Y_STEP * 3, X_STEP, Y_STEP );
	Joy_DrawSpecialButton( "Enter", X_START + X_STEP * 12, Y_START + Y_STEP * 3, X_STEP, Y_STEP );
}

#endif
