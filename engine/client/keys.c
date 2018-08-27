/*
keys.c - console key events
Copyright (C) 2007 Uncle Mike

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
#include "client.h"
#include "joyinput.h"
#include "touch.h"
#include "vgui_draw.h"

#ifdef XASH_SDL
#include <SDL_keyboard.h>
#include <platform/sdl/events.h>
#endif

typedef struct key_s
{
	qboolean		down;
	int		repeats;	// if > 1, it is autorepeating
	const char	*binding;
} engine_key_t;

engine_key_t	keys[256];

keyname_t keynames[] =
{
{"TAB",		K_TAB,		""		},
{"ENTER",		K_ENTER,		""		},
{"ESCAPE",	K_ESCAPE, 	"cancelselect"	}, // hardcoded
{"SPACE",		K_SPACE,		"+moveup"		},
{"BACKSPACE",	K_BACKSPACE,	""		},
{"UPARROW",	K_UPARROW,	"+forward"	},
{"DOWNARROW",	K_DOWNARROW,	"+back"		},
{"LEFTARROW",	K_LEFTARROW,	"+left"		},
{"RIGHTARROW",	K_RIGHTARROW,	"+right"		},
{"ALT",		K_ALT,		"+strafe"		},
{"CTRL",		K_CTRL,		"+attack"		},
{"SHIFT",		K_SHIFT,		"+speed"		}, // replace with +attack2 ?
{"CAPSLOCK",	K_CAPSLOCK,	""		},
{"F1",		K_F1,		"cmd help"	},
{"F2",		K_F2,		"menu_savegame"	},
{"F3",		K_F3,		"menu_loadgame"	},
{"F4",		K_F4,		"menu_keys"	},
{"F5",		K_F5,		"menu_startserver"	},
{"F6",		K_F6,		"savequick"	},
{"F7",		K_F7,		"loadquick"	},
{"F8",		K_F8,		"stop"		},
{"F9",		K_F9,		""		},
{"F10",		K_F10,		"menu_quit"	},
{"F11",		K_F11,		""		},
{"F12",		K_F12,		"screenshot"	},
{"INS",		K_INS,		""		},
{"DEL",		K_DEL,		"+lookdown"	},
{"PGDN",		K_PGDN,		"+lookup"		},
{"PGUP",		K_PGUP,		""		},
{"HOME",		K_HOME,		""		},
{"END",		K_END,		"centerview"	},

// mouse buttouns
{"MOUSE1",	K_MOUSE1,		"+attack"		},
{"MOUSE2",	K_MOUSE2,		"+attack2"	},
{"MOUSE3",	K_MOUSE3,		""		},
{"MOUSE4",	K_MOUSE4,		""		},
{"MOUSE5",	K_MOUSE5,		""		},
{"MWHEELUP",	K_MWHEELUP,	""		},
{"MWHEELDOWN",	K_MWHEELDOWN,	""		},

// digital keyboard
{"KP_HOME",	K_KP_HOME,	""		},
{"KP_UPARROW",	K_KP_UPARROW,	"+forward"	},
{"KP_PGUP",	K_KP_PGUP,	""		},
{"KP_LEFTARROW",	K_KP_LEFTARROW,	"+left"		},
{"KP_5",		K_KP_5,		""		},
{"KP_RIGHTARROW",	K_KP_RIGHTARROW,	"+right"		},
{"KP_END",	K_KP_END,		"centerview"	},
{"KP_DOWNARROW",	K_KP_DOWNARROW,	"+back"		},
{"KP_PGDN",	K_KP_PGDN,	"+lookup" 	},
{"KP_ENTER",	K_KP_ENTER,	""		},
{"KP_INS",	K_KP_INS,		""		},
{"KP_DEL",	K_KP_DEL,		"+lookdown"	},
{"KP_SLASH",	K_KP_SLASH,	""		},
{"KP_MINUS",	K_KP_MINUS,	""		},
{"KP_PLUS",	K_KP_PLUS,	""		},
{"PAUSE",		K_PAUSE,		"pause"		},
{"A_BUTTON", K_A_BUTTON, ""}, // they match xbox controller
{"B_BUTTON", K_B_BUTTON, ""},
{"X_BUTTON", K_X_BUTTON, ""},
{"Y_BUTTON", K_Y_BUTTON, ""},
{"L1_BUTTON",  K_L1_BUTTON, ""},
{"R1_BUTTON",  K_R1_BUTTON, ""},
{"BACK",   K_BACK_BUTTON, ""},
{"MODE",   K_MODE_BUTTON, ""},
{"START",  K_START_BUTTON, ""},
{"STICK1", K_LSTICK, ""},
{"STICK2", K_RSTICK, ""},
{"L2_BUTTON", K_L2_BUTTON, ""}, // in case...
{"R2_BUTTON", K_R2_BUTTON, ""},
{"C_BUTTON", K_C_BUTTON, ""},
{"Z_BUTTON", K_Z_BUTTON, ""},
{"AUX16", K_AUX16, ""}, // generic
{"AUX17", K_AUX17, ""},
{"AUX18", K_AUX18, ""},
{"AUX19", K_AUX19, ""},
{"AUX20", K_AUX20, ""},
{"AUX21", K_AUX21, ""},
{"AUX22", K_AUX22, ""},
{"AUX23", K_AUX23, ""},
{"AUX24", K_AUX24, ""},
{"AUX25", K_AUX25, ""},
{"AUX26", K_AUX26, ""},
{"AUX27", K_AUX27, ""},
{"AUX28", K_AUX28, ""},
{"AUX29", K_AUX29, ""},
{"AUX30", K_AUX30, ""},
{"AUX31", K_AUX31, ""},
{"AUX32", K_AUX32, ""},
{"LTRIGGER" , K_JOY1 , ""},
{"RTRIGGER" , K_JOY2 , ""},
{"JOY3" , K_JOY3 , ""},
{"JOY4" , K_JOY4 , ""},
// raw semicolon seperates commands
{"SEMICOLON", ';', ""},
{NULL, 0, NULL},
};

/*
===================
Key_IsDown
===================
*/
qboolean Key_IsDown( int keynum )
{
	if ( keynum == -1 )
		return false;
	return keys[keynum].down;
}

/*
===================
Key_GetBind
===================
*/
const char *Key_IsBind( int keynum )
{
	if( keynum == -1 || !keys[keynum].binding )
		return NULL;
	return keys[keynum].binding;
}

/*
===================
Key_StringToKeynum

Returns a key number to be used to index keys[] by looking at
the given string.  Single ascii characters return themselves, while
the K_* names are matched up.

0x11 will be interpreted as raw hex, which will allow new controlers

to be configured even if they don't have defined names.
===================
*/
int Key_StringToKeynum( const char *str )
{
	keyname_t		*kn;
	
	if( !str || !str[0] ) return -1;
	if( !str[1] ) return str[0];

	// check for hex code
	if( str[0] == '0' && str[1] == 'x' && Q_strlen( str ) == 4 )
	{
		int	n1, n2;
		
		n1 = str[2];
		if( n1 >= '0' && n1 <= '9' )
		{
			n1 -= '0';
		}
		else if( n1 >= 'a' && n1 <= 'f' )
		{
			n1 = n1 - 'a' + 10;
		}
		else n1 = 0;

		n2 = str[3];
		if( n2 >= '0' && n2 <= '9' )
		{
			n2 -= '0';
		}
		else if( n2 >= 'a' && n2 <= 'f' )
		{
			n2 = n2 - 'a' + 10;
		}
		else n2 = 0;

		return n1 * 16 + n2;
	}

	// scan for a text match
	for( kn = keynames; kn->name; kn++ )
	{
		if( !Q_stricmp( str, kn->name ))
			return kn->keynum;
	}
	return -1;
}

/*
===================
Key_KeynumToString

Returns a string (either a single ascii char, a K_* name, or a 0x11 hex string) for the
given keynum.
===================
*/
const char *Key_KeynumToString( int keynum )
{
	keyname_t		*kn;	
	static char	tinystr[5];
	int		i, j;

	if ( keynum == -1 ) return "<KEY NOT FOUND>";
	if ( keynum < 0 || keynum > 255 ) return "<OUT OF RANGE>";

	// check for printable ascii (don't use quote)
	if( keynum > 32 && keynum < 127 && keynum != '"' && keynum != ';' )
	{
		tinystr[0] = keynum;
		tinystr[1] = 0;
		return tinystr;
	}

	// check for a key string
	for( kn = keynames; kn->name; kn++ )
	{
		if( keynum == kn->keynum )
			return kn->name;
	}

	// make a hex string
	i = keynum >> 4;
	j = keynum & 15;

	tinystr[0] = '0';
	tinystr[1] = 'x';
	tinystr[2] = i > 9 ? i - 10 + 'a' : i + '0';
	tinystr[3] = j > 9 ? j - 10 + 'a' : j + '0';
	tinystr[4] = 0;

	return tinystr;
}

/*
===================
Key_SetBinding
===================
*/
void Key_SetBinding( int keynum, const char *binding )
{
	if( keynum == -1 ) return;

	// free old binding
	if( keys[keynum].binding )
	{
		// GoldSrc doesn't touch ESC
		if( keynum == K_ESCAPE ) return;

		if( host.state != HOST_INIT && Q_stricmp( keys[keynum].binding, binding ) )
			cls.keybind_changed = true;

		Mem_Free((char *)keys[keynum].binding );
		keys[keynum].binding = NULL;
	}

	// don't bind to empty string
	if( !binding[0] ) return;

	// allocate memory for new binding
	keys[keynum].binding = copystring( binding );
}


/*
===================
Key_GetBinding
===================
*/
const char *Key_GetBinding( int keynum )
{
	if( keynum == -1 ) return NULL;
	return keys[keynum].binding;
}

/* 
===================
Key_GetKey
===================
*/
int Key_GetKey( const char *binding )
{
	int	i;

	if( !binding ) return -1;

	for( i = 0; i < 256; i++ )
	{
		if( keys[i].binding && !Q_stricmp( binding, keys[i].binding ))
			return i;
	}
	return -1;
}

/*
===================
Key_Unbind_f
===================
*/
void Key_Unbind_f( void )
{
	int	b;

	if( Cmd_Argc() != 2 )
	{
		Msg( "Usage: unbind <key> : remove commands from a key\n" );
		return;
	}
	
	b = Key_StringToKeynum( Cmd_Argv( 1 ));
	if( b == -1 )
	{
		Msg( "\"%s\" isn't a valid key\n", Cmd_Argv( 1 ));
		return;
	}
	if( b == K_ESCAPE )
	{
		Msg( "Can't unbind ESCAPE key\n" );
		return;
	}
	Key_SetBinding( b, "" );
}

/*
===================
Key_Unbindall_f
===================
*/
void Key_Unbindall_f( void )
{
	int	i;
	
	for( i = 0; i < 256; i++ )
	{
		if( i != K_ESCAPE && keys[i].binding )
			Key_SetBinding( i, "" );
	}
}

/*
===================
Key_Reset_f
===================
*/
void Key_Reset_f( void )
{
	int	i;
	keyname_t	*kn;

	// clear all keys first	
	for( i = 0; i < 256; i++ )
	{
		if( keys[i].binding )
			Key_SetBinding( i, "" );
	}

	// apply default values
	for( kn = keynames; kn->name; kn++ )
		Key_SetBinding( kn->keynum, kn->binding ); 
}

/*
===================
Key_Bind_f
===================
*/
void Key_Bind_f( void )
{
	int	i, c, b;
	char	cmd[1024];
	
	c = Cmd_Argc();

	if( c < 2 )
	{
		Msg( "Usage: bind <key> [command] : attach a command to a key\n" );
		return;
	}

	b = Key_StringToKeynum( Cmd_Argv( 1 ));

	if( b == -1 )
	{
		// Trigger +mlook when changed from menu
		if( !Q_strcmp( Cmd_Argv( 1 ), "_force_write" ) )
		{
			cls.keybind_changed = true;
			return;
		}
		Msg( "\"%s\" isn't a valid key\n", Cmd_Argv( 1 ));
		return;
	}

	if( c == 2 )
	{
		if( keys[b].binding )
			Msg( "\"%s\" = \"%s\"\n", Cmd_Argv( 1 ), keys[b].binding );
		else Msg( "\"%s\" is not bound\n", Cmd_Argv( 1 ));
		return;
	}
	
	// copy the rest of the command line
	cmd[0] = 0; // start out with a null string

	for( i = 2; i < c; i++ )
	{
		Q_strcat( cmd, Cmd_Argv( i ));
		if( i != ( c - 1 )) Q_strcat( cmd, " " );
	}

	Key_SetBinding( b, cmd );
}

/*
============
Key_WriteBindings

Writes lines containing "bind key value"
============
*/
void Key_WriteBindings( file_t *f )
{
	int	i;

	if( !f ) return;
	FS_Printf( f, "unbindall\n" );

	for( i = 0; i < 256; i++ )
	{
		string command;

		if( keys[i].binding && keys[i].binding[0] )
		{
			Com_EscapeCommand( command, keys[i].binding, MAX_STRING );
			FS_Printf( f, "bind %s \"%s\"\n", Key_KeynumToString(i), command );
		}
	}
}

/*
============
Key_Bindlist_f

============
*/
void Key_Bindlist_f( void )
{
	int	i;

	for( i = 0; i < 256; i++ )
	{
		if( keys[i].binding && keys[i].binding[0] )
			Msg( "%s \"%s\"\n", Key_KeynumToString( i ), keys[i].binding );
	}
}

/*
==============================================================================

			LINE TYPING INTO THE CONSOLE

==============================================================================
*/
/*
===================
Key_Init
===================
*/
void Key_Init( void )
{
	keyname_t	*kn;

	cls.key_dest = key_console;

	// register our functions
	Cmd_AddCommand( "bind", Key_Bind_f, "binds a command to the specified key in bindmap" );
	Cmd_AddCommand( "unbind", Key_Unbind_f, "removes a command on the specified key in bindmap" );
	Cmd_AddCommand( "unbindall", Key_Unbindall_f, "removes all commands from all keys in bindmap" );
	Cmd_AddCommand( "resetkeys", Key_Reset_f, "reset all keys to their default values" );
	Cmd_AddCommand( "bindlist", Key_Bindlist_f, "display current key bindings" );
	Cmd_AddCommand( "makehelp", Key_EnumCmds_f, "write help.txt that contains all console cvars and cmds" );
	Q_memset( keys, 0, sizeof( keys ) );

	// setup default binding. "unbindall" from config.cfg will reset it
	for( kn = keynames; kn->name; kn++ ) Key_SetBinding( kn->keynum, kn->binding ); 
}

/*
===================
Key_AddKeyUpCommands
===================
*/
void Key_AddKeyUpCommands( int key, const char *kb )
{
	int	i;
	char	button[1024], *buttonPtr;
	char	cmd[1024];
	qboolean	keyevent;

	if( !kb ) return;
	keyevent = false;
	buttonPtr = button;

	for( i = 0; ; i++ )
	{
		if( kb[i] == ';' || !kb[i] )
		{
			*buttonPtr = '\0';
			if( button[0] == '+' )
			{
				// button commands add keynum as a parm
				Q_sprintf( cmd, "-%s %i\n", button+1, key );
				Cbuf_AddText( cmd );
				keyevent = true;
			}
			else
			{
				if( keyevent )
				{
					// down-only command
					Cbuf_AddText( button );
					Cbuf_AddText( "\n" );
				}
			}

			buttonPtr = button;
			while(( kb[i] <= ' ' || kb[i] == ';' ) && kb[i] != 0 )
				i++;
		}

		*buttonPtr++ = kb[i];
		if( !kb[i] ) break;
	}
}

/*
===================
Key_Event

Called by the system for both key up and key down events
===================
*/
void GAME_EXPORT Key_Event( int key, qboolean down )
{
	const char	*kb;
	char		cmd[1024];
	//Con_Printf( "Keycode %d\n", key );
	if ( key > 255 || key < 0) 
	{
		MsgDev (D_NOTE, "Keynum %d out of range\n", key);
		return;
	}
	// update auto-repeat status and BUTTON_ANY status
	keys[key].down = down;

	if( down )
	{
		keys[key].repeats++;

		if( key != K_BACKSPACE && key != K_PAUSE && keys[key].repeats > 1 )
		{
			if( cls.key_dest == key_game )
			{
				// ignore most autorepeats
				return;
			}
		}
	}
	else
	{
		keys[key].repeats = 0;
	}

	VGui_KeyEvent( key, down );
	Touch_KeyEvent( key, down );

	// console key is hardcoded, so the user can never unbind it
	if( key == '`' || key == '~' )
	{
		// we are in typing mode, so don't switch to console
		if( cls.key_dest == key_message || !down )
			return;

		Con_ToggleConsole_f();
		return;
	}

	// escape is always handled specially
	if( key == K_ESCAPE && down )
	{
		switch( cls.key_dest )
		{
		case key_game:
			if( host.mouse_visible && cls.state != ca_cinematic )
			{
				clgame.dllFuncs.pfnKey_Event( down, key, keys[key].binding );

				// enable anti-The_Swank system
				if( cls.state == ca_connected && Con_Visible() )
					Key_SetKeyDest( key_console );

				return; // handled in client.dll
			}
			break;
		case key_message:
			Key_Message( key );
			return;
		case key_console:
			if( cls.state == ca_active && !cl.background )
				Key_SetKeyDest( key_game );
			else UI_SetActiveMenu( true );
			return;
		case key_menu:
			UI_KeyEvent( key, true );
			return;
		default:
			MsgDev( D_ERROR, "Key_Event: bad cls.key_dest\n" );
			return;
		}
	}

	if( cls.key_dest == key_menu )
	{
		// only non printable keys passed
		// shift + click enables text mode (old menu compatibility)
		/*if( down && !host.textmode && Key_IsDown( K_SHIFT ) && ( key == 241 ) )
		{
			SDLash_EnableTextInput( true );
			return;
		}
		// and click disables
		else if( down && ( key == 241 ) )
			SDLash_EnableTextInput( false );
		*/
		if( !menu.use_text_api )
			Key_EnableTextInput( true, false );
		//pass printable chars for old menus
		if( !menu.use_text_api && !host.textmode && down && ( key >= 32 ) && ( key <= 'z' ) )
		{
			if( Key_IsDown( K_SHIFT ) )
			{
				key += 'A'-'a';
			}
			UI_CharEvent( key );
		}
		UI_KeyEvent( key, down );
		return;
	}

	// key up events only perform actions if the game key binding is
	// a button command (leading + sign).  These will be processed even in
	// console mode and menu mode, to keep the character from continuing 
	// an action started before a mode switch.
	if( !down )
	{
		kb = keys[key].binding;

		if( cls.key_dest == key_game && ( key != K_ESCAPE ))
			clgame.dllFuncs.pfnKey_Event( down, key, kb );

		Key_AddKeyUpCommands( key, kb );
		return;
	}

	// distribute the key down event to the apropriate handler
	if( cls.key_dest == key_game )
	{
		if( cls.state == ca_cinematic )
		{
			// only escape passed when cinematic is playing
			// HLFX 0.6 bug: crash in vgui3.dll while press +attack during movie playback
			if( key != K_ESCAPE || !down )
				return;
		}
		else if( host.mouse_visible )
			return;

		// send the bound action
		kb = keys[key].binding;

		if( !clgame.dllFuncs.pfnKey_Event( down, key, keys[key].binding ))
		{
			// handled in client.dll
		}
		else if( kb != NULL )
		{
			if( kb[0] == '+' )
			{	
				int	i;
				char	button[1024], *buttonPtr;

				for( i = 0, buttonPtr = button; ; i++ )
				{
					if( kb[i] == ';' || !kb[i] )
					{
						*buttonPtr = '\0';
						if( button[0] == '+' )
						{
							Q_sprintf( cmd, "%s %i\n", button, key );
							Cbuf_AddText( cmd );
						}
						else
						{
							// down-only command
							Cbuf_AddText( button );
							Cbuf_AddText( "\n" );
						}

						buttonPtr = button;
						while (( kb[i] <= ' ' || kb[i] == ';' ) && kb[i] != 0 )
							i++;
					}

					*buttonPtr++ = kb[i];
					if( !kb[i] ) break;
				}
			}
			else
			{
				// down-only command
				Cbuf_AddText( kb );
				Cbuf_AddText( "\n" );
			}
		}
	}
	else if( cls.key_dest == key_console )
	{
		Key_Console( key );
	}
	else if( cls.key_dest == key_message )
	{
		Key_Message( key );
	}
}

void Key_EnableTextInput( qboolean enable, qboolean force )
{
#if XASH_INPUT == INPUT_SDL
	SDLash_EnableTextInput( enable, force );
#elif XASH_INPUT == INPUT_ANDROID
	Android_EnableTextInput( enable, force );
#endif
#if 0
	Joy_EnableTextInput( enable, force );
#endif
}

/*
=========
Key_SetKeyDest
=========
*/
void Key_SetKeyDest( int key_dest )
{
	IN_ToggleClientMouse( key_dest, cls.key_dest );

	switch( key_dest )
	{
	case key_game:
		Key_EnableTextInput( false, false );
		if( host_xashds_hacks->integer )
		{
			Cbuf_Execute();
			if( cl.refdef.paused )
			{
				Cbuf_InsertText("pause\n");
				Cbuf_Execute();
			}
			cl.refdef.paused = 0;
		}
		cls.key_dest = key_game;
		break;
	case key_menu:
		Key_EnableTextInput( false, false );
		cls.key_dest = key_menu;
		break;
	case key_console:
		Key_EnableTextInput( true, false );
		cls.key_dest = key_console;
		break;
	case key_message:
		Key_EnableTextInput( true, false );
		cls.key_dest = key_message;
		break;
	default:
		Host_Error( "Key_SetKeyDest: wrong destination (%i)\n", key_dest );
		break;
	}
}

/*
===================
Key_ClearStates
===================
*/
void Key_ClearStates( void )
{
	int	i;

	for( i = 0; i < 256; i++ )
	{
		if( keys[i].down )
			Key_Event( i, false );

		keys[i].down = 0;
		keys[i].repeats = 0;
	}

	if( clgame.hInstance )
	{
		clgame.dllFuncs.IN_ClearStates();
	}
}

/*
===================
CL_CharEvent

Normal keyboard characters, already shifted / capslocked / etc
===================
*/
void CL_CharEvent( int ch )
{
	// the console key should never be used as a char
#ifdef _WIN32
	if( ch == '`' || ch == '~' ) return;

#if 0
	if( cls.key_dest == key_console && !Con_Visible( ))
	{
		if((char)key == '�' || (char)key == '�' )
			return; // don't pass '�' when we open the console 
	}
#endif
#endif
	// distribute the key down event to the apropriate handler

	Con_CharEvent( ch ); // a1ba: no need for checks, as Con_CharEvent already it does

	if( cls.key_dest == key_menu )
	{
		UI_CharEvent( ch );
	}
	else if( cls.key_dest == key_game ) // typing support for VGUI
	{
		VGui_KeyEvent( ch, 2 );
	}

}
#endif
