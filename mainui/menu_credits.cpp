/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include "extdll.h"
#include "basemenu.h"
#include "utils.h"

#define UI_CREDITS_PATH		"credits.txt"
#define UI_CREDITS_MAXLINES		2048

static const char *uiCreditsDefault[] = 
{
	"",
	"Copyright XashXT Group 2014 (C)",
	0
};

typedef struct
{
	const char	**credits;
	int		startTime;
	int		showTime;
	int		fadeTime;
	int		numLines;
	int		active;
	int		finalCredits;
	char		*index[UI_CREDITS_MAXLINES];
	char		*buffer;

	menuFramework_s	menu;
} uiCredits_t;

static uiCredits_t		uiCredits;


/*
=================
UI_Credits_DrawFunc
=================
*/
static void UI_Credits_DrawFunc( void )
{
	int	i, y;
	float	speed = 40.0f;
	int	w = UI_MED_CHAR_WIDTH;
	int	h = UI_MED_CHAR_HEIGHT;
	int	color = 0;

	// draw the background first
	if( !uiCredits.finalCredits && !CVAR_GET_FLOAT( "cl_background" ))
		UI_DrawPic( 0, 0, 1024 * uiStatic.scaleX, 768 * uiStatic.scaleY, uiColorWhite, ART_BACKGROUND );
	else speed = 45.0f;	// syncronize with final background track :-)

	// otherwise running on cutscene
	speed = 32.0f * (768.0f / ScreenHeight);

	// now draw the credits
	UI_ScaleCoords( NULL, NULL, &w, &h );

	y = ScreenHeight - (((gpGlobals->time * 1000) - uiCredits.startTime ) / speed );

	// draw the credits
	for ( i = 0; i < uiCredits.numLines && uiCredits.credits[i]; i++, y += h )
	{
		// skip not visible lines, but always draw end line
		if( y <= -h && i != uiCredits.numLines - 1 ) continue;

		if(( y < ( ScreenHeight - h ) / 2 ) && i == uiCredits.numLines - 1 )
		{
			if( !uiCredits.fadeTime ) uiCredits.fadeTime = (gpGlobals->time * 1000);
			color = UI_FadeAlpha( uiCredits.fadeTime, uiCredits.showTime );
			if( UnpackAlpha( color ))
				UI_DrawString( 0, ( ScreenHeight - h ) / 2, 1024 * uiStatic.scaleX, h, uiCredits.credits[i], color, true, w, h, 1, true );
		}
		else UI_DrawString( 0, y, 1024 * uiStatic.scaleX, h, uiCredits.credits[i], uiColorWhite, false, w, h, 1, true );
	}

	if( y < 0 && UnpackAlpha( color ) == 0 )
	{
		uiCredits.active = false; // end of credits
		if( uiCredits.finalCredits )
			HOST_ENDGAME( gMenu.m_gameinfo.title );
	}

	if( !uiCredits.active )
		UI_PopMenu();
}

/*
=================
UI_Credits_KeyFunc
=================
*/
static const char *UI_Credits_KeyFunc( int key, int down )
{
	if( !down ) return uiSoundNull;

	// final credits can't be intterupted
	if( uiCredits.finalCredits )
		return uiSoundNull;

	uiCredits.active = false;
	return uiSoundNull;
}

/*
=================
UI_Credits_Init
=================
*/
static void UI_Credits_Init( void )
{
	uiCredits.menu.drawFunc = UI_Credits_DrawFunc;
	uiCredits.menu.keyFunc = UI_Credits_KeyFunc;

	if( !uiCredits.buffer )
	{
		int	count;
		char	*p;

		// load credits if needed
		uiCredits.buffer = (char *)LOAD_FILE( UI_CREDITS_PATH, &count );
		if( count )
		{
			if( uiCredits.buffer[count - 1] != '\n' && uiCredits.buffer[count - 1] != '\r' )
			{
				char *tmp = (char *)MALLOC( count + 2 );
				memcpy( tmp, uiCredits.buffer, count ); 
				FREE_FILE( uiCredits.buffer );
				uiCredits.buffer = tmp; 
				strncpy( uiCredits.buffer + count, "\r", 1 ); // add terminator
				count += 2; // added "\r\0"
                    	}
			p = uiCredits.buffer;

			// convert customs credits to 'ideal' strings array
			for ( uiCredits.numLines = 0; uiCredits.numLines < UI_CREDITS_MAXLINES; uiCredits.numLines++ )
			{
				uiCredits.index[uiCredits.numLines] = p;
				while ( *p != '\r' && *p != '\n' )
				{
					p++;
					if ( --count == 0 )
						break;
				}

				if ( *p == '\r' )
				{
					*p++ = 0;
					if( --count == 0 ) break;
				}

				*p++ = 0;
				if( --count == 0 ) break;
			}
			uiCredits.index[++uiCredits.numLines] = 0;
			uiCredits.credits = (const char **)uiCredits.index;
		}
		else
		{
			// use built-in credits
			uiCredits.credits =  uiCreditsDefault;
			uiCredits.numLines = ( sizeof( uiCreditsDefault ) / sizeof( uiCreditsDefault[0] )) - 1; // skip term
		}
	}

	// run credits
	uiCredits.startTime = (gpGlobals->time * 1000) + 500; // make half-seconds delay
	uiCredits.showTime = bound( 1000, strlen( uiCredits.credits[uiCredits.numLines - 1]) * 1000, 10000 );
	uiCredits.fadeTime = 0; // will be determined later
	uiCredits.active = true;
}

void UI_DrawFinalCredits( void )
{
	if( uiCredits.finalCredits && uiCredits.active )
		UI_Credits_DrawFunc ();
}

int UI_CreditsActive( void )
{
	return uiCredits.active && uiCredits.finalCredits;
}

/*
=================
UI_Credits_Precache
=================
*/
void UI_Credits_Precache( void )
{
	PIC_Load( ART_BACKGROUND );
}

/*
=================
UI_Credits_Menu
=================
*/
void UI_Credits_Menu( void )
{
	UI_Credits_Precache();
	UI_Credits_Init();

	UI_PushMenu( &uiCredits.menu );
}

void UI_FinalCredits( void )
{
	uiCredits.finalCredits = true;
	UI_Credits_Init();
}