#pragma once
#ifndef JOYINPUT_H
#define JOYINPUT_H

/*
 * Same as SDL hat values
 * All positions is mapped to keyboard arrows
 * Zero means unpressing keyboard arrows
 */
enum
{
	JOY_HAT_CENTERED = 0,
	JOY_HAT_UP    = BIT(0),
	JOY_HAT_RIGHT = BIT(1),
	JOY_HAT_DOWN  = BIT(2),
	JOY_HAT_LEFT  = BIT(3),
	JOY_HAT_RIGHTUP   = JOY_HAT_RIGHT | JOY_HAT_UP,
	JOY_HAT_RIGHTDOWN = JOY_HAT_RIGHT | JOY_HAT_DOWN,
	JOY_HAT_LEFTUP    = JOY_HAT_LEFT  | JOY_HAT_UP,
	JOY_HAT_LEFTDOWN  = JOY_HAT_LEFT  | JOY_HAT_DOWN
};

qboolean Joy_IsActive( void );
void Joy_HatMotionEvent( int id, byte hat, byte value );
void Joy_AxisMotionEvent( int id, byte axis, short value );
void Joy_BallMotionEvent( int id, byte ball, short xrel, short yrel );
void Joy_ButtonEvent( int id, byte button, byte down );

void Joy_AddEvent( int id );
void Joy_RemoveEvent( int id );

void Joy_FinalizeMove( float *fw, float *side, float *dpitch, float *dyaw );

void Joy_Init( void );
void Joy_Shutdown( void );

void Joy_EnableTextInput(qboolean enable, qboolean force);

void Joy_DrawOnScreenKeyboard( void );

extern convar_t *joy_found;

#endif // JOYINPUT_H
