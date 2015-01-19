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


// ui_menu.c -- main menu interface
#define OEMRESOURCE		// for OCR_* cursor junk

#include "extdll.h"
#include "basemenu.h"
#include "keydefs.h"
#include "menufont.h"	// built-in menu font
#include "utils.h"
#include "menu_btnsbmp_table.h"
//CR
#include "ui_title_anim.h"

cvar_t		*ui_precache;
cvar_t		*ui_showmodels;

uiStatic_t	uiStatic;

char		uiEmptyString[256];
const char	*uiSoundIn	= "media/launch_upmenu1.wav";
const char	*uiSoundOut	= "media/launch_dnmenu1.wav";
const char	*uiSoundLaunch	= "media/launch_select2.wav";
const char	*uiSoundGlow	= "media/launch_glow1.wav";
const char	*uiSoundBuzz	= "media/launch_deny2.wav";
const char	*uiSoundKey	= "media/launch_select1.wav";
const char	*uiSoundRemoveKey	= "media/launch_deny1.wav";
const char	*uiSoundMove	= "";		// Xash3D not use movesound
const char	*uiSoundNull	= "";

int		uiColorHelp	= 0xFFFFFFFF;	// 255, 255, 255, 255	// hint letters color
int		uiPromptBgColor	= 0xFF404040;	// 64,  64,  64,  255	// dialog background color
int		uiPromptTextColor	= 0xFFF0B418;	// 255, 160,  0,  255	// dialog or button letters color
int		uiPromptFocusColor	= 0xFFFFFF00;	// 255, 255,  0,  255	// dialog or button focus letters color
int		uiInputTextColor	= 0xFFC0C0C0;	// 192, 192, 192, 255
int		uiInputBgColor	= 0xFF404040;	// 64,  64,  64,  255	// field, scrollist, checkbox background color
int		uiInputFgColor	= 0xFF555555;	// 85,  85,  85,  255	// field, scrollist, checkbox foreground color
int		uiColorWhite	= 0xFFFFFFFF;	// 255, 255, 255, 255	// useful for bitmaps
int		uiColorDkGrey	= 0xFF404040;	// 64,  64,  64,  255	// shadow and grayed items
int		uiColorBlack	= 0xFF000000;	//  0,   0,   0,  255	// some controls background
int		uiColorConsole	= 0xFFF0B418;	// just for reference 

// color presets (this is nasty hack to allow color presets to part of text)
const int g_iColorTable[8] =
{
0xFF000000, // black
0xFFFF0000, // red
0xFF00FF00, // green
0xFFFFFF00, // yellow
0xFF0000FF, // blue
0xFF00FFFF, // cyan
0xFFF0B418, // dialog or button letters color
0xFFFFFFFF, // white
};

/*
=================
UI_ScaleCoords

Any parameter can be NULL if you don't want it
=================
*/
void UI_ScaleCoords( int *x, int *y, int *w, int *h )
{
	if( x ) *x *= uiStatic.scaleX;
	if( y ) *y *= uiStatic.scaleY;
	if( w ) *w *= uiStatic.scaleX;
	if( h ) *h *= uiStatic.scaleY;
}

/*
=================
UI_CursorInRect
=================
*/
int UI_CursorInRect( int x, int y, int w, int h )
{
	if( uiStatic.cursorX < x )
		return FALSE;
	if( uiStatic.cursorX > x + w )
		return FALSE;
	if( uiStatic.cursorY < y )
		return FALSE;
	if( uiStatic.cursorY > y + h )
		return FALSE;
	return TRUE;
}

/*
=================
UI_DrawPic
=================
*/
void UI_DrawPic( int x, int y, int width, int height, const int color, const char *pic )
{
	HIMAGE hPic = PIC_Load( pic );

	int r, g, b, a;
	UnpackRGBA( r, g, b, a, color );

	PIC_Set( hPic, r, g, b, a );
	PIC_Draw( x, y, width, height );
}

/*
=================
UI_DrawPicAdditive
=================
*/
void UI_DrawPicAdditive( int x, int y, int width, int height, const int color, const char *pic )
{
	HIMAGE hPic = PIC_Load( pic );

	int r, g, b, a;
	UnpackRGBA( r, g, b, a, color );

	PIC_Set( hPic, r, g, b, a );
	PIC_DrawAdditive( x, y, width, height );
}

/*
=================
UI_FillRect
=================
*/
void UI_FillRect( int x, int y, int width, int height, const int color )
{
	int r, g, b, a;
	UnpackRGBA( r, g, b, a, color );

	FillRGBA( x, y, width, height, r, g, b, a );
}

/*
=================
UI_DrawRectangleExt
=================
*/
void UI_DrawRectangleExt( int in_x, int in_y, int in_w, int in_h, const int color, int outlineWidth )
{
	int	x, y, w, h;

	x = in_x - outlineWidth;
	y = in_y - outlineWidth;
	w = outlineWidth;
	h = in_h + outlineWidth + outlineWidth;

	// draw left
	UI_FillRect( x, y, w, h, color );

	x = in_x + in_w;
	y = in_y - outlineWidth;
	w = outlineWidth;
	h = in_h + outlineWidth + outlineWidth;

	// draw right
	UI_FillRect( x, y, w, h, color );

	x = in_x;
	y = in_y - outlineWidth;
	w = in_w;
	h = outlineWidth;

	// draw top
	UI_FillRect( x, y, w, h, color );

	// draw bottom
	x = in_x;
	y = in_y + in_h;
	w = in_w;
	h = outlineWidth;

	UI_FillRect( x, y, w, h, color );
}

/*
=================
UI_DrawString
=================
*/
void UI_DrawString( int x, int y, int w, int h, const char *string, const int color, int forceColor, int charW, int charH, int justify, int shadow )
{
	int	modulate, shadowModulate;
	char	line[1024], *l;
	int	xx, yy, ofsX, ofsY, len, ch;

	if( !string || !string[0] )
		return;

#if 0	// g-cont. disabled 29/06/2011
	// this code do a bad things with prompt dialogues
	// vertically centered
	if( !strchr( string, '\n' ))
		y = y + (( h - charH ) / 2 );
#endif

	if( shadow )
	{
		shadowModulate = PackAlpha( uiColorBlack, UnpackAlpha( color ));

		ofsX = charW / 8;
		ofsY = charH / 8;
	}

	modulate = color;

	yy = y;
	while( *string )
	{
		// get a line of text
		len = 0;
		while( *string )
		{
			if( *string == '\n' )
			{
				string++;
				break;
			}

			line[len++] = *string++;
			if( len == sizeof( line ) - 1 )
				break;
		}
		line[len] = 0;

		// align the text as appropriate
		if( justify == 0 ) xx = x;
		if( justify == 1 ) xx = x + ((w - (ColorStrlen( line ) * charW )) / 2);
		if( justify == 2 ) xx = x + (w - (ColorStrlen( line ) * charW ));

		// draw it
		l = line;
		while( *l )
		{
			if( IsColorString( l ))
			{
				if( !forceColor )
				{
					int colorNum = ColorIndex( *(l+1) );
					modulate = PackAlpha( g_iColorTable[colorNum], UnpackAlpha( color ));
				}

				l += 2;
				continue;
			}

			ch = *l++;
			ch &= 255;

			// fix for letter ¸
			if( ch == 0xB8 ) ch = (byte)'å';
			if( ch == 0xA8 ) ch = (byte)'Å';

			if( ch != ' ' )
			{
				if( shadow ) TextMessageDrawChar( xx + ofsX, yy + ofsY, charW, charH, ch, shadowModulate, uiStatic.hFont );
				TextMessageDrawChar( xx, yy, charW, charH, ch, modulate, uiStatic.hFont );
			}
			xx += charW;
		}
          	yy += charH;
	}
}

/*
=================
UI_DrawMouseCursor
=================
*/
void UI_DrawMouseCursor( void )
{
	menuCommon_s	*item;
	HICON		hCursor = NULL;
	int		i;

	if( uiStatic.hideCursor ) return;

	for( i = 0; i < uiStatic.menuActive->numItems; i++ )
	{
		item = (menuCommon_s *)uiStatic.menuActive->items[i];

		if ( item->flags & (QMF_INACTIVE|QMF_HIDDEN))
			continue;

		if ( !UI_CursorInRect( item->x, item->y, item->width, item->height ))
			continue;

		if ( item->flags & QMF_GRAYED )
		{
			hCursor = (HICON)LoadCursor( NULL, (LPCTSTR)OCR_NO );
		}
		else
		{
			if( item->type == QMTYPE_FIELD )
				hCursor = (HICON)LoadCursor( NULL, (LPCTSTR)OCR_IBEAM );
		}
		break;
	}

	if( !hCursor ) hCursor = (HICON)LoadCursor( NULL, (LPCTSTR)OCR_NORMAL );

	SET_CURSOR( hCursor );
}

/*
=================
UI_DrawBackground_Callback
=================
*/
void UI_DrawBackground_Callback( void *self )
{
	if (!uiStatic.m_fHaveSteamBackground)
	{
		menuCommon_s *item = (menuCommon_s *)self;
		UI_DrawPic( item->x, item->y, item->width, item->height, uiColorWhite, ((menuBitmap_s *)self)->pic );
		return;
	}

	int xpos, ypos;
	float xScale, yScale;

	// work out scaling factors
	xScale = ScreenWidth / uiStatic.m_flTotalWidth;
	yScale = ScreenHeight / uiStatic.m_flTotalHeight;

	// iterate and draw all the background pieces
	ypos = 0;
	for (int y = 0; y < BACKGROUND_ROWS; y++)
	{
		xpos = 0;
		for (int x = 0; x < BACKGROUND_COLUMNS; x++)
		{
			bimage_t &bimage = uiStatic.m_SteamBackground[y][x];

			int dx = (int)ceil(xpos * xScale);
			int dy = (int)ceil(ypos * yScale);
			int dw = (int)ceil(bimage.width * xScale);
			int dt = (int)ceil(bimage.height * yScale);

			if (x == 0) dx = 0;
			if (y == 0) dy = 0;

			PIC_Set( bimage.hImage, 255, 255, 255, 255 );
			PIC_Draw( dx, dy, dw, dt );
			xpos += bimage.width;
		}
		ypos += uiStatic.m_SteamBackground[y][0].height;
	}
}

/*
=================
UI_LoadBackgroundImage
=================
*/
void UI_LoadBackgroundImage( void )
{
	int num_background_images = 0;
	char filename[512];

	for( int y = 0; y < BACKGROUND_ROWS; y++ )
	{
		for( int x = 0; x < BACKGROUND_COLUMNS; x++ )
		{
			sprintf( filename, "resource/background/800_%d_%c_loading.tga", y + 1, 'a' + x );
			if (g_engfuncs.pfnFileExists( filename, TRUE ))
				num_background_images++;
		}
	}

	if (num_background_images == (BACKGROUND_COLUMNS * BACKGROUND_ROWS))
		uiStatic.m_fHaveSteamBackground = TRUE;
	else uiStatic.m_fHaveSteamBackground = FALSE;

	if (uiStatic.m_fHaveSteamBackground)
	{
		uiStatic.m_flTotalWidth = uiStatic.m_flTotalHeight = 0.0f;

		for( int y = 0; y < BACKGROUND_ROWS; y++ )
		{
			for( int x = 0; x < BACKGROUND_COLUMNS; x++ )
			{
				bimage_t &bimage = uiStatic.m_SteamBackground[y][x];
				sprintf(filename, "resource/background/800_%d_%c_loading.tga", y + 1, 'a' + x);
				bimage.hImage = PIC_Load( filename, PIC_NOFLIP_TGA );
				bimage.width = PIC_Width( bimage.hImage );
				bimage.height = PIC_Height( bimage.hImage );

				if (y==0) uiStatic.m_flTotalWidth += bimage.width;
				if (x==0) uiStatic.m_flTotalHeight += bimage.height;
			}
		}
	}
	else
	{
		if( g_engfuncs.pfnFileExists( "gfx/shell/splash.bmp", TRUE ))
		{
			// if we doesn't have logo.avi in gamedir we don't want to draw it
			if( !g_engfuncs.pfnFileExists( "media/logo.avi", TRUE ))
				uiStatic.m_fDisableLogo = TRUE;
		}
	}
}

/*
=================
UI_StartSound
=================
*/
void UI_StartSound( const char *sound )
{
	PLAY_SOUND( sound );
}

// =====================================================================


/*
=================
UI_AddItem
=================
*/
void UI_AddItem( menuFramework_s *menu, void *item )
{
	menuCommon_s	*generic = (menuCommon_s *)item;

	if( menu->numItems >= UI_MAX_MENUITEMS )
		HOST_ERROR( "UI_AddItem: UI_MAX_MENUITEMS limit exceeded\n" );

	menu->items[menu->numItems] = item;
	((menuCommon_s *)menu->items[menu->numItems])->parent = menu;
	((menuCommon_s *)menu->items[menu->numItems])->flags &= ~QMF_HASMOUSEFOCUS;
	menu->numItems++;

	switch( generic->type )
	{
	case QMTYPE_SCROLLLIST:
		UI_ScrollList_Init((menuScrollList_s *)item );
		break;
	case QMTYPE_SPINCONTROL:
		UI_SpinControl_Init((menuSpinControl_s *)item );
		break;
	case QMTYPE_CHECKBOX:
		UI_CheckBox_Init((menuCheckBox_s *)item );
		break;
	case QMTYPE_SLIDER:
		UI_Slider_Init((menuSlider_s *)item );
		break;
	case QMTYPE_FIELD:
		UI_Field_Init((menuField_s *)item );
		break;
	case QMTYPE_ACTION:
		UI_Action_Init((menuAction_s *)item );
		break;
	case QMTYPE_BITMAP:
		UI_Bitmap_Init((menuBitmap_s *)item );
		break;
	case QMTYPE_BM_BUTTON:
		UI_PicButton_Init((menuPicButton_s *)item );
		break;
	default:
		HOST_ERROR( "UI_AddItem: unknown item type (%i)\n", generic->type );
	}
}

/*
=================
UI_CursorMoved
=================
*/
void UI_CursorMoved( menuFramework_s *menu )
{
	void (*callback)( void *self, int event );

	if( menu->cursor == menu->cursorPrev )
		return;

	if( menu->cursorPrev >= 0 && menu->cursorPrev < menu->numItems )
	{
		callback = ((menuCommon_s *)menu->items[menu->cursorPrev])->callback;
		if( callback ) callback( menu->items[menu->cursorPrev], QM_LOSTFOCUS );
	}

	if( menu->cursor >= 0 && menu->cursor < menu->numItems )
	{
		callback = ((menuCommon_s *)menu->items[menu->cursor])->callback;
		if( callback ) callback( menu->items[menu->cursor], QM_GOTFOCUS );
	}
}

/*
=================
UI_SetCursor
=================
*/
void UI_SetCursor( menuFramework_s *menu, int cursor )
{
	if(((menuCommon_s *)(menu->items[cursor]))->flags & (QMF_GRAYED|QMF_INACTIVE|QMF_HIDDEN))
		return;

	menu->cursorPrev = menu->cursor;
	menu->cursor = cursor;

	UI_CursorMoved( menu );
}

/*
=================
UI_SetCursorToItem
=================
*/
void UI_SetCursorToItem( menuFramework_s *menu, void *item )
{
	for( int i = 0; i < menu->numItems; i++ )
	{
		if( menu->items[i] == item )
		{
			UI_SetCursor( menu, i );
			return;
		}
	}
}

/*
=================
UI_ItemAtCursor
=================
*/
void *UI_ItemAtCursor( menuFramework_s *menu )
{
	if( menu->cursor < 0 || menu->cursor >= menu->numItems )
		return 0;

	// inactive items can't be has focus
	if( ((menuCommon_s *)menu->items[menu->cursor])->flags & QMF_INACTIVE )
		return 0;

	return menu->items[menu->cursor];
}

/*
=================
UI_AdjustCursor

This functiont takes the given menu, the direction, and attempts to
adjust the menu's cursor so that it's at the next available slot
=================
*/
void UI_AdjustCursor( menuFramework_s *menu, int dir )
{
	menuCommon_s	*item;
	int		wrapped = false;
wrap:
	while( menu->cursor >= 0 && menu->cursor < menu->numItems )
	{
		item = (menuCommon_s *)menu->items[menu->cursor];
		if( item->flags & (QMF_GRAYED|QMF_INACTIVE|QMF_HIDDEN|QMF_MOUSEONLY))
			menu->cursor += dir;
		else break;
	}

	if( dir == 1 )
	{
		if( menu->cursor >= menu->numItems )
		{
			if( wrapped )
			{
				menu->cursor = menu->cursorPrev;
				return;
			}

			menu->cursor = 0;
			wrapped = true;
			goto wrap;
		}
	}
	else if( dir == -1 )
	{
		if( menu->cursor < 0 )
		{
			if( wrapped )
			{
				menu->cursor = menu->cursorPrev;
				return;
			}
			menu->cursor = menu->numItems - 1;
			wrapped = true;
			goto wrap;
		}
	}
}

/*
=================
UI_DrawMenu
=================
*/
void UI_DrawMenu( menuFramework_s *menu )
{
	static long	statusFadeTime;
	static menuCommon_s	*lastItem;
	menuCommon_s	*item;
	int		i;

	// draw contents
	for( i = 0; i < menu->numItems; i++ )
	{
		item = (menuCommon_s *)menu->items[i];

		if( item->flags & QMF_HIDDEN )
			continue;

		if( item->ownerdraw )
		{
			// total subclassing, owner draws everything
			item->ownerdraw( item );
			continue;
		}

		switch( item->type )
		{
		case QMTYPE_SCROLLLIST:
			UI_ScrollList_Draw((menuScrollList_s *)item );
			break;
		case QMTYPE_SPINCONTROL:
			UI_SpinControl_Draw((menuSpinControl_s *)item );
			break;
		case QMTYPE_CHECKBOX:
			UI_CheckBox_Draw((menuCheckBox_s *)item );
			break;
		case QMTYPE_SLIDER:
			UI_Slider_Draw((menuSlider_s *)item );
			break;
		case QMTYPE_FIELD:
			UI_Field_Draw((menuField_s *)item );
			break;
		case QMTYPE_ACTION:
			UI_Action_Draw((menuAction_s *)item );
			break;
		case QMTYPE_BITMAP:
			UI_Bitmap_Draw((menuBitmap_s *)item );
			break;
		case QMTYPE_BM_BUTTON:
			UI_PicButton_Draw((menuPicButton_s *)item );
			break;
		}
	}

	// draw status bar
	item = (menuCommon_s *)UI_ItemAtCursor( menu );
	if( item != lastItem )
	{
		// flash on selected button (like in GoldSrc)
		if( item ) item->lastFocusTime = uiStatic.realTime;
		statusFadeTime = uiStatic.realTime;
		lastItem = item;
	}

	if( item && ( item->flags & QMF_HASMOUSEFOCUS && !( item->flags & QMF_NOTIFY )) && ( item->statusText != NULL ))
	{
		// fade it in, but wait a second
		int alpha = bound( 0, ((( uiStatic.realTime - statusFadeTime ) - 1000 ) * 0.001f ) * 255, 255 );
		int r, g, b, x, len;

		GetConsoleStringSize( item->statusText, &len, NULL );

		UnpackRGB( r, g, b, uiColorHelp );
		TextMessageSetColor( r, g, b, alpha );
		x = ( ScreenWidth - len ) * 0.5; // centering

		DrawConsoleString( x, 720 * uiStatic.scaleY, item->statusText );
	}
	else statusFadeTime = uiStatic.realTime;
}

/*
=================
UI_DefaultKey
=================
*/
const char *UI_DefaultKey( menuFramework_s *menu, int key, int down )
{
	const char	*sound = NULL;
	menuCommon_s	*item;
	int		cursorPrev;

	// menu system key
	if( down && key == K_ESCAPE )
	{
		UI_PopMenu();
		return uiSoundOut;
	}

	if( !menu || !menu->numItems )
		return 0;

	item = (menuCommon_s *)UI_ItemAtCursor( menu );
	if( item && !(item->flags & (QMF_GRAYED|QMF_INACTIVE|QMF_HIDDEN)))
	{
		switch( item->type )
		{
		case QMTYPE_SCROLLLIST:
			sound = UI_ScrollList_Key((menuScrollList_s *)item, key, down );
			break;
		case QMTYPE_SPINCONTROL:
			sound = UI_SpinControl_Key((menuSpinControl_s *)item, key, down );
			break;
		case QMTYPE_CHECKBOX:
			sound = UI_CheckBox_Key((menuCheckBox_s *)item, key, down );
			break;
		case QMTYPE_SLIDER:
			sound = UI_Slider_Key((menuSlider_s *)item, key, down );
			break;
		case QMTYPE_FIELD:
			sound = UI_Field_Key((menuField_s *)item, key, down );
			break;
		case QMTYPE_ACTION:
			sound = UI_Action_Key((menuAction_s *)item, key, down );
			break;
		case QMTYPE_BITMAP:
			sound = UI_Bitmap_Key((menuBitmap_s *)item, key, down );
			break;
		case QMTYPE_BM_BUTTON:
			sound = UI_PicButton_Key((menuPicButton_s *)item, key, down );
			break;
		}
		if( sound ) return sound; // key was handled
	}

	// system keys are always wait for keys down and never keys up
	if( !down ) return 0;

	// default handling
	switch( key )
	{
	case K_UPARROW:
	case K_KP_UPARROW:
	case K_LEFTARROW:
	case K_KP_LEFTARROW:
		cursorPrev = menu->cursor;
		menu->cursorPrev = menu->cursor;
		menu->cursor--;

		UI_AdjustCursor( menu, -1 );
		if( cursorPrev != menu->cursor )
		{
			UI_CursorMoved( menu );
			if( !(((menuCommon_s *)menu->items[menu->cursor])->flags & QMF_SILENT ))
				sound = uiSoundMove;
		}
		break;
	case K_DOWNARROW:
	case K_KP_DOWNARROW:
	case K_RIGHTARROW:
	case K_KP_RIGHTARROW:
	case K_TAB:
		cursorPrev = menu->cursor;
		menu->cursorPrev = menu->cursor;
		menu->cursor++;

		UI_AdjustCursor(menu, 1);
		if( cursorPrev != menu->cursor )
		{
			UI_CursorMoved(menu);
			if( !(((menuCommon_s *)menu->items[menu->cursor])->flags & QMF_SILENT ))
				sound = uiSoundMove;
		}
		break;
	case K_MOUSE1:
		if( item )
		{
			if((item->flags & QMF_HASMOUSEFOCUS) && !(item->flags & (QMF_GRAYED|QMF_INACTIVE|QMF_HIDDEN)))
				return UI_ActivateItem( menu, item );
		}

		break;
	case K_ENTER:
	case K_KP_ENTER:
		if( item )
		{
			if( !(item->flags & (QMF_GRAYED|QMF_INACTIVE|QMF_HIDDEN|QMF_MOUSEONLY)))
				return UI_ActivateItem( menu, item );
		}
		break;
	}
	return sound;
}		

/*
=================
UI_ActivateItem
=================
*/
const char *UI_ActivateItem( menuFramework_s *menu, menuCommon_s *item )
{
	if( item->callback )
	{
		item->callback( item, QM_ACTIVATED );

		if( !( item->flags & QMF_SILENT ))
			return uiSoundMove;
	}
	return 0;
}

/*
=================
UI_RefreshServerList
=================
*/
void UI_RefreshServerList( void )
{
	uiStatic.numServers = 0;
	memset( uiStatic.serverAddresses, 0, sizeof( uiStatic.serverAddresses ));
	memset( uiStatic.serverNames, 0, sizeof( uiStatic.serverNames ));

	CLIENT_COMMAND( FALSE, "localservers\n" );
}

/*
=================
UI_RefreshInternetServerList
=================
*/
void UI_RefreshInternetServerList( void )
{
	uiStatic.numServers = 0;
	memset( uiStatic.serverAddresses, 0, sizeof( uiStatic.serverAddresses ));
	memset( uiStatic.serverNames, 0, sizeof( uiStatic.serverNames ));

	CLIENT_COMMAND( FALSE, "internetservers\n" );
}

/*
=================
UI_StartBackGroundMap
=================
*/
bool UI_StartBackGroundMap( void )
{
	static bool	first = TRUE;

	if( !first ) return FALSE;

	first = FALSE;

	// some map is already running
	if( !uiStatic.bgmapcount || CVAR_GET_FLOAT( "host_serverstate" ) || gpGlobals->demoplayback )
		return FALSE;

	int bgmapid = RANDOM_LONG( 0, uiStatic.bgmapcount - 1 );

	char cmd[128];
	sprintf( cmd, "maps/%s.bsp", uiStatic.bgmaps[bgmapid] );
	if( !FILE_EXISTS( cmd )) return FALSE; 

	sprintf( cmd, "map_background %s\n", uiStatic.bgmaps[bgmapid] );
	CLIENT_COMMAND( FALSE, cmd );

	return TRUE;
}

// =====================================================================

/*
=================
UI_CloseMenu
=================
*/
void UI_CloseMenu( void )
{
	uiStatic.menuActive = NULL;
	uiStatic.menuDepth = 0;
	uiStatic.visible = false;

	// clearing serverlist
	uiStatic.numServers = 0;
	memset( uiStatic.serverAddresses, 0, sizeof( uiStatic.serverAddresses ));
	memset( uiStatic.serverNames, 0, sizeof( uiStatic.serverNames ));

	UI_ClearButtonStack ();

//	KEY_ClearStates ();
	KEY_SetDest ( KEY_GAME );
}

/*
=================
UI_PushMenu
=================
*/
void UI_PushMenu( menuFramework_s *menu )
{
	int		i;
	menuCommon_s	*item;

	// if this menu is already present, drop back to that level to avoid stacking menus by hotkeys
	for( i = 0; i < uiStatic.menuDepth; i++ )
	{
		if( uiStatic.menuStack[i] == menu )
		{
			uiStatic.menuDepth = i;
			break;
		}
	}

	if( i == uiStatic.menuDepth )
	{
		if( uiStatic.menuDepth >= UI_MAX_MENUDEPTH )
			HOST_ERROR( "UI_PushMenu: menu stack overflow\n" );
		uiStatic.menuStack[uiStatic.menuDepth++] = menu;
	}

	uiStatic.menuActive = menu;
	uiStatic.firstDraw = true;
	uiStatic.enterSound = gpGlobals->time + 0.15;	// make some delay
	uiStatic.visible = true;

	KEY_SetDest ( KEY_MENU );

	menu->cursor = 0;
	menu->cursorPrev = 0;

	// force first available item to have focus
	for( i = 0; i < menu->numItems; i++ )
	{
		item = (menuCommon_s *)menu->items[i];

		if( item->flags & (QMF_GRAYED|QMF_INACTIVE|QMF_HIDDEN|QMF_MOUSEONLY))
			continue;

		menu->cursorPrev = -1;
		UI_SetCursor( menu, i );
		break;
	}
}

/*
=================
UI_PopMenu
=================
*/
void UI_PopMenu( void )
{
	UI_StartSound( uiSoundOut );

	uiStatic.menuDepth--;

	if( uiStatic.menuDepth < 0 )
		HOST_ERROR( "UI_PopMenu: menu stack underflow\n" );

	UI_PopPButtonStack();

	if( uiStatic.menuDepth )
	{
		uiStatic.menuActive = uiStatic.menuStack[uiStatic.menuDepth-1];
		uiStatic.firstDraw = true;
	}
	else if ( CL_IsActive( ))
	{
		UI_CloseMenu();
	}
	else
	{
		// never trying the close menu when client isn't connected
		KEY_SetDest( KEY_MENU );
		UI_Main_Menu();
	}

	if( uiStatic.m_fDemosPlayed && uiStatic.m_iOldMenuDepth == uiStatic.menuDepth )
	{
		CLIENT_COMMAND( FALSE, "demos\n" );
		uiStatic.m_fDemosPlayed = false;
		uiStatic.m_iOldMenuDepth = 0;
	}
}

// =====================================================================

/*
=================
UI_UpdateMenu
=================
*/
void UI_UpdateMenu( float flTime )
{
	if( !uiStatic.initialized )
		return;

	UI_DrawFinalCredits ();

	if( !uiStatic.visible )
		return;

	if( !uiStatic.menuActive )
		return;

	uiStatic.realTime = flTime * 1000;
	uiStatic.framecount++;

	if( CVAR_GET_FLOAT( "cl_background" ) && !g_engfuncs.pfnClientInGame())
		return;	// don't draw menu while level is loading

	if( uiStatic.firstDraw )
	{
		// we loading background so skip SCR_Update
		if( UI_StartBackGroundMap( )) return;

		if( uiStatic.menuActive->activateFunc )
			uiStatic.menuActive->activateFunc();
	}

	// draw menu
	if( uiStatic.menuActive->drawFunc )
		uiStatic.menuActive->drawFunc();
	else UI_DrawMenu( uiStatic.menuActive );

	if( uiStatic.firstDraw )
	{
		uiStatic.firstDraw = false;
		static int first = TRUE;
                    
		if( first )
		{
			// if game was launched with commandline e.g. +map or +load ignore the music
			if( !CL_IsActive( ))
				BACKGROUND_TRACK( "gamestartup", "gamestartup" );
			first = FALSE;
		}
	}

	//CR
	UI_DrawTitleAnim();
	//

	// draw cursor
	UI_DrawMouseCursor();

	// delay playing the enter sound until after the menu has been
	// drawn, to avoid delay while caching images
	if( uiStatic.enterSound > 0.0f && uiStatic.enterSound <= gpGlobals->time )
	{
		UI_StartSound( uiSoundIn );
		uiStatic.enterSound = -1;
	}
}

/*
=================
UI_KeyEvent
=================
*/
void UI_KeyEvent( int key, int down )
{
	const char	*sound;

	if( !uiStatic.initialized )
		return;

	if( !uiStatic.visible )
		return;

	if( !uiStatic.menuActive )
		return;

	if( uiStatic.menuActive->keyFunc )
		sound = uiStatic.menuActive->keyFunc( key, down );
	else sound = UI_DefaultKey( uiStatic.menuActive, key, down );

	if( !down ) return;
	if( sound && sound != uiSoundNull )
		UI_StartSound( sound );
}

/*
=================
UI_CharEvent
=================
*/
void UI_CharEvent( int key )
{
	menuFramework_s	*menu;
	menuCommon_s	*item;

	if( !uiStatic.initialized )
		return;

	if( !uiStatic.visible )
		return;

	if( !uiStatic.menuActive )
		return;

	menu = uiStatic.menuActive;

	if( !menu || !menu->numItems )
		return;

	item = (menuCommon_s *)UI_ItemAtCursor( menu );

	if( item && !(item->flags & (QMF_GRAYED|QMF_INACTIVE|QMF_HIDDEN)))
	{
		switch( item->type )
		{
		case QMTYPE_FIELD:
			UI_Field_Char((menuField_s *)item, key );
			break;
		}
	}
}

/*
=================
UI_MouseMove
=================
*/
void UI_MouseMove( int x, int y )
{
	int		i;
	menuCommon_s	*item;

	if( !uiStatic.initialized )
		return;

	if( !uiStatic.visible )
		return;

	if( !uiStatic.menuActive )
		return;

	// now menu uses absolute coordinates
	uiStatic.cursorX = x;
	uiStatic.cursorY = y;

	if( UI_CursorInRect( 1, 1, ScreenWidth - 1, ScreenHeight - 1 ))
		uiStatic.mouseInRect = true;
	else uiStatic.mouseInRect = false;

	uiStatic.cursorX = bound( 0, uiStatic.cursorX, ScreenWidth );
	uiStatic.cursorY = bound( 0, uiStatic.cursorY, ScreenHeight );

	// region test the active menu items
	for( i = 0; i < uiStatic.menuActive->numItems; i++ )
	{
		item = (menuCommon_s *)uiStatic.menuActive->items[i];

		if( item->flags & (QMF_GRAYED|QMF_INACTIVE|QMF_HIDDEN))
		{
			if( item->flags & QMF_HASMOUSEFOCUS )
			{
				if( !UI_CursorInRect( item->x, item->y, item->width, item->height ))
					item->flags &= ~QMF_HASMOUSEFOCUS;
				else item->lastFocusTime = uiStatic.realTime;
			}
			continue;
		}

		if( !UI_CursorInRect( item->x, item->y, item->width, item->height ))
		{
			item->bPressed = false;
			continue;
		}

		// set focus to item at cursor
		if( uiStatic.menuActive->cursor != i )
		{
			UI_SetCursor( uiStatic.menuActive, i );
			((menuCommon_s *)(uiStatic.menuActive->items[uiStatic.menuActive->cursorPrev]))->flags &= ~QMF_HASMOUSEFOCUS;

			if (!(((menuCommon_s *)(uiStatic.menuActive->items[uiStatic.menuActive->cursor]))->flags & QMF_SILENT ))
				UI_StartSound( uiSoundMove );
		}

		((menuCommon_s *)(uiStatic.menuActive->items[uiStatic.menuActive->cursor]))->flags |= QMF_HASMOUSEFOCUS;
		((menuCommon_s *)(uiStatic.menuActive->items[uiStatic.menuActive->cursor]))->lastFocusTime = uiStatic.realTime;
		return;
	}

	// out of any region
	if( uiStatic.menuActive->numItems )
	{
		((menuCommon_s *)(uiStatic.menuActive->items[uiStatic.menuActive->cursor]))->flags &= ~QMF_HASMOUSEFOCUS;
		((menuCommon_s *)(uiStatic.menuActive->items[uiStatic.menuActive->cursor]))->bPressed = false;

		// a mouse only item restores focus to the previous item
		if(((menuCommon_s *)(uiStatic.menuActive->items[uiStatic.menuActive->cursor]))->flags & QMF_MOUSEONLY )
		{
			if( uiStatic.menuActive->cursorPrev != -1 )
				uiStatic.menuActive->cursor = uiStatic.menuActive->cursorPrev;
		}
	}
}

/*
=================
UI_SetActiveMenu
=================
*/
void UI_SetActiveMenu( int fActive )
{
	if( !uiStatic.initialized )
		return;

	// don't continue firing if we leave game
	KEY_ClearStates();
	uiStatic.framecount = 0;

	if( fActive )
	{
		KEY_SetDest( KEY_MENU );
		UI_Main_Menu();
	}
	else
	{
		UI_CloseMenu();
	}
}

/*
=================
UI_AddServerToList
=================
*/
void UI_AddServerToList( netadr_t adr, const char *info )
{
	int	i;

	if( !uiStatic.initialized )
		return;

	if( uiStatic.numServers == UI_MAX_SERVERS )
		return;	// full

	// ignore if duplicated
	for( i = 0; i < uiStatic.numServers; i++ )
	{
		if( !stricmp( uiStatic.serverNames[i], info ))
			return;
	}

	// add it to the list
	uiStatic.updateServers = true; // info has been updated
	uiStatic.serverAddresses[uiStatic.numServers] = adr;
	strncpy( uiStatic.serverNames[uiStatic.numServers], info, sizeof( uiStatic.serverNames[uiStatic.numServers] ));
	uiStatic.numServers++;
}

/*
=================
UI_IsVisible

Some systems may need to know if it is visible or not
=================
*/
int UI_IsVisible( void )
{
	if( !uiStatic.initialized )
		return false;
	return uiStatic.visible;
}

void UI_GetCursorPos( int *pos_x, int *pos_y )
{
	if( pos_x ) *pos_x = uiStatic.cursorX;
	if( pos_y ) *pos_y = uiStatic.cursorY;
}

void UI_SetCursorPos( int pos_x, int pos_y )
{
//	uiStatic.cursorX = bound( 0, pos_x, ScreenWidth );
//	uiStatic.cursorY = bound( 0, pos_y, ScreenHeight );
	uiStatic.mouseInRect = true;
}

void UI_ShowCursor( int show )
{
	uiStatic.hideCursor = (show) ? false : true;
}

int UI_MouseInRect( void )
{
	return uiStatic.mouseInRect;
}

/*
=================
UI_Precache
=================
*/
void UI_Precache( void )
{
	if( !uiStatic.initialized )
		return;

	if( !ui_precache->value )
		return;

	PIC_Load( UI_LEFTARROW );
	PIC_Load( UI_LEFTARROWFOCUS );
	PIC_Load( UI_RIGHTARROW );
	PIC_Load( UI_RIGHTARROWFOCUS );
	PIC_Load( UI_UPARROW );
	PIC_Load( UI_UPARROWFOCUS );
	PIC_Load( UI_DOWNARROW );
	PIC_Load( UI_DOWNARROWFOCUS );

	if( ui_precache->value == 1 )
		return;

	UI_Main_Precache();
	UI_NewGame_Precache();
	UI_LoadGame_Precache();
	UI_SaveGame_Precache();
	UI_SaveLoad_Precache();
	UI_MultiPlayer_Precache();
	UI_Options_Precache();
	UI_InternetGames_Precache();
	UI_LanGame_Precache();
	UI_PlayerSetup_Precache();
	UI_Controls_Precache();
	UI_AdvControls_Precache();
	UI_GameOptions_Precache();
	UI_CreateGame_Precache();
	UI_Audio_Precache();
	UI_Video_Precache();
	UI_VidOptions_Precache();
	UI_VidModes_Precache();
	UI_CustomGame_Precache();
	UI_Credits_Precache();
}

void UI_ParseColor( char *&pfile, int *outColor )
{
	int	i, color[3];
	char	token[1024];

	memset( color, 0xFF, sizeof( color ));

	for( i = 0; i < 3; i++ )
	{
		pfile = COM_ParseFile( pfile, token );
		if( !pfile ) break;
		color[i] = atoi( token );
	}

	*outColor = PackRGB( color[0], color[1], color[2] );
}

void UI_ApplyCustomColors( void )
{
	char *afile = (char *)LOAD_FILE( "gfx/shell/colors.lst", NULL );
	char *pfile = afile;
	char token[1024];

	if( !afile )
	{
		// not error, not warning, just notify
		Con_Printf( "UI_SetColors: colors.lst not found\n" );
		return;
	}

	while(( pfile = COM_ParseFile( pfile, token )) != NULL )
	{
		if( !stricmp( token, "HELP_COLOR" ))
		{
			UI_ParseColor( pfile, &uiColorHelp );
		}
		else if( !stricmp( token, "PROMPT_BG_COLOR" ))
		{
			UI_ParseColor( pfile, &uiPromptBgColor );
		}
		else if( !stricmp( token, "PROMPT_TEXT_COLOR" ))
		{
			UI_ParseColor( pfile, &uiPromptTextColor );
		}
		else if( !stricmp( token, "PROMPT_FOCUS_COLOR" ))
		{
			UI_ParseColor( pfile, &uiPromptFocusColor );
		}
		else if( !stricmp( token, "INPUT_TEXT_COLOR" ))
		{
			UI_ParseColor( pfile, &uiInputTextColor );
		}
		else if( !stricmp( token, "INPUT_BG_COLOR" ))
		{
			UI_ParseColor( pfile, &uiInputBgColor );
		}
		else if( !stricmp( token, "INPUT_FG_COLOR" ))
		{
			UI_ParseColor( pfile, &uiInputFgColor );
		}
		else if( !stricmp( token, "CON_TEXT_COLOR" ))
		{
			UI_ParseColor( pfile, &uiColorConsole );
		}
	}

	int	r, g, b;

	UnpackRGB( r, g, b, uiColorConsole );
	ConsoleSetColor( r, g, b );

	FREE_FILE( afile );
}

static void UI_LoadBackgroundMapList( void )
{
	if( !g_engfuncs.pfnFileExists( "scripts/chapterbackgrounds.txt", TRUE ))
		return;

	char *afile = (char *)LOAD_FILE( "scripts/chapterbackgrounds.txt", NULL );
	char *pfile = afile;
	char token[1024];

	uiStatic.bgmapcount = 0;

	if( !afile )
	{
		Con_Printf( "UI_LoadBackgroundMapList: chapterbackgrounds.txt not found\n" );
		return;
	}

	while(( pfile = COM_ParseFile( pfile, token )) != NULL )
	{
		// skip the numbers (old format list)
		if( isdigit( token[0] )) continue;

		strncpy( uiStatic.bgmaps[uiStatic.bgmapcount], token, sizeof( uiStatic.bgmaps[0] ));
		if( ++uiStatic.bgmapcount > UI_MAX_BGMAPS )
			break; // list is full
	}

	FREE_FILE( afile );
}

/*
=================
UI_VidInit
=================
*/
int UI_VidInit( void )
{
	UI_Precache ();
		
	uiStatic.scaleX = ScreenWidth / 1024.0f;
	uiStatic.scaleY = ScreenHeight / 768.0f;

	// move cursor to screen center
	uiStatic.cursorX = ScreenWidth >> 1;
	uiStatic.cursorY = ScreenHeight >> 1;
	uiStatic.outlineWidth = 4;
	uiStatic.sliderWidth = 6;

	// all menu buttons have the same view sizes
	uiStatic.buttons_draw_width = UI_BUTTONS_WIDTH;
	uiStatic.buttons_draw_height = UI_BUTTONS_HEIGHT;

	UI_ScaleCoords( NULL, NULL, &uiStatic.outlineWidth, NULL );
	UI_ScaleCoords( NULL, NULL, &uiStatic.sliderWidth, NULL );
	UI_ScaleCoords( NULL, NULL, &uiStatic.buttons_draw_width, &uiStatic.buttons_draw_height );

	// trying to load colors.lst
	UI_ApplyCustomColors ();

	// trying to load chapterbackgrounds.txt
	UI_LoadBackgroundMapList ();

	// register menu font
	uiStatic.hFont = PIC_Load( "#XASH_SYSTEMFONT_001.bmp", menufont_bmp, sizeof( menufont_bmp ));

	UI_LoadBackgroundImage ();
#if 0
	FILE *f;

	// dump menufont onto disk
	f = fopen( "menufont.bmp", "wb" );
	fwrite( menufont_bmp, sizeof( menufont_bmp ), 1, f );
	fclose( f );
#endif

	// reload all menu buttons
	UI_LoadBmpButtons ();

	// now recalc all the menus in stack
	for( int i = 0; i < uiStatic.menuDepth; i++ )
	{
		menuFramework_s *item = uiStatic.menuStack[i];

		// do vid restart for all pushed elements
		if( item && item->vidInitFunc )
			item->vidInitFunc();
	}

	return 1;
}

/*
=================
UI_Init
=================
*/
void UI_Init( void )
{
	// register our cvars and commands
	ui_precache = CVAR_REGISTER( "ui_precache", "0", FCVAR_ARCHIVE );
	ui_showmodels = CVAR_REGISTER( "ui_showmodels", "0", FCVAR_ARCHIVE );

	Cmd_AddCommand( "menu_main", UI_Main_Menu );
	Cmd_AddCommand( "menu_newgame", UI_NewGame_Menu );
	Cmd_AddCommand( "menu_loadgame", UI_LoadGame_Menu );
	Cmd_AddCommand( "menu_savegame", UI_SaveGame_Menu );
	Cmd_AddCommand( "menu_saveload", UI_SaveLoad_Menu );
	Cmd_AddCommand( "menu_multiplayer", UI_MultiPlayer_Menu );
	Cmd_AddCommand( "menu_options", UI_Options_Menu );
	Cmd_AddCommand( "menu_langame", UI_LanGame_Menu );
	Cmd_AddCommand( "menu_intenetgames", UI_InternetGames_Menu );
	Cmd_AddCommand( "menu_playersetup", UI_PlayerSetup_Menu );
	Cmd_AddCommand( "menu_controls", UI_Controls_Menu );
	Cmd_AddCommand( "menu_advcontrols", UI_AdvControls_Menu );
	Cmd_AddCommand( "menu_gameoptions", UI_GameOptions_Menu );
	Cmd_AddCommand( "menu_creategame", UI_CreateGame_Menu );
	Cmd_AddCommand( "menu_audio", UI_Audio_Menu );
	Cmd_AddCommand( "menu_video", UI_Video_Menu );
	Cmd_AddCommand( "menu_vidoptions", UI_VidOptions_Menu );
	Cmd_AddCommand( "menu_vidmodes", UI_VidModes_Menu );
	Cmd_AddCommand( "menu_customgame", UI_CustomGame_Menu );

	CHECK_MAP_LIST( TRUE );

	memset( uiEmptyString, ' ', sizeof( uiEmptyString ));	// HACKHACK
	uiStatic.initialized = true;

	// setup game info
	GetGameInfo( &gMenu.m_gameinfo );

	// load custom strings
	UI_LoadCustomStrings();

	//CR
	UI_InitTitleAnim();
}

/*
=================
UI_Shutdown
=================
*/
void UI_Shutdown( void )
{
	if( !uiStatic.initialized )
		return;

	Cmd_RemoveCommand( "menu_main" );
	Cmd_RemoveCommand( "menu_newgame" );
	Cmd_RemoveCommand( "menu_loadgame" );
	Cmd_RemoveCommand( "menu_savegame" );
	Cmd_RemoveCommand( "menu_saveload" );
	Cmd_RemoveCommand( "menu_multiplayer" );
	Cmd_RemoveCommand( "menu_options" );
	Cmd_RemoveCommand( "menu_intenetgames" );
	Cmd_RemoveCommand( "menu_langame" );
	Cmd_RemoveCommand( "menu_playersetup" );
	Cmd_RemoveCommand( "menu_controls" );
	Cmd_RemoveCommand( "menu_advcontrols" );
	Cmd_RemoveCommand( "menu_gameoptions" );
	Cmd_RemoveCommand( "menu_creategame" );
	Cmd_RemoveCommand( "menu_audio" );
	Cmd_RemoveCommand( "menu_video" );
	Cmd_RemoveCommand( "menu_vidoptions" );
	Cmd_RemoveCommand( "menu_vidmodes" );
	Cmd_RemoveCommand( "menu_advanced" );
	Cmd_RemoveCommand( "menu_performance" );
	Cmd_RemoveCommand( "menu_network" );
	Cmd_RemoveCommand( "menu_defaults" );
	Cmd_RemoveCommand( "menu_cinematics" );
	Cmd_RemoveCommand( "menu_customgame" );
	Cmd_RemoveCommand( "menu_quit" );

	memset( &uiStatic, 0, sizeof( uiStatic_t ));
}
