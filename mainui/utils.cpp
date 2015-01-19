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


// ui_qmenu.c -- Quake menu framework

#include "extdll.h"
#include "basemenu.h"
#include "utils.h"
#include "keydefs.h"
#include "menu_btnsbmp_table.h"
//CR
#include "ui_title_anim.h"

#ifdef _DEBUG
void DBG_AssertFunction( BOOL fExpr, const char* szExpr, const char* szFile, int szLine, const char* szMessage )
{
	if( fExpr ) return;

	char szOut[512];
	if( szMessage != NULL )
		sprintf( szOut, "ASSERT FAILED:\n %s \n(%s@%d)\n%s", szExpr, szFile, szLine, szMessage );
	else sprintf( szOut, "ASSERT FAILED:\n %s \n(%s@%d)", szExpr, szFile, szLine );
	HOST_ERROR( szOut );
}
#endif	// DEBUG

int ColorStrlen( const char *str )
{
	const char *p;

	if( !str )
		return 0;

	int len = 0;
	p = str;

	while( *p )
	{
		if( IsColorString( p ))
		{
			p += 2;
			continue;
		}

		p++;
		len++;
	}

	return len;
}

void StringConcat( char *dst, const char *src, size_t size )
{
	register char *d = dst;
	register const char *s = src;
	register size_t n = size;
	size_t dlen;

	if( !dst || !src || !size )
		return;

	// find the end of dst and adjust bytes left but don't go past end
	while(n-- != 0 && *d != '\0') d++;
	dlen = d - dst;
	n = size - dlen;

	if ( n == 0 ) return;
	while ( *s != '\0' )
	{
		if ( n != 1 )
		{
			*d++ = *s;
			n--;
		}
		s++;
	}

	*d = '\0';
	return;
}

char *StringCopy( const char *input )
{
	if( !input ) return NULL;

	char *out = (char *)MALLOC( strlen( input ) + 1 );
	strcpy( out, input );

	return out;
}

/*
============
COM_CompareSaves
============
*/
int COM_CompareSaves( const void **a, const void **b )
{
	char *file1, *file2;

	file1 = (char *)*a;
	file2 = (char *)*b;

	int bResult;

	COMPARE_FILE_TIME( file2, file1, &bResult );

	return bResult;
}

/*
============
COM_FileBase
============
*/
// Extracts the base name of a file (no path, no extension, assumes '/' as path separator)
void COM_FileBase ( const char *in, char *out )
{
	int len, start, end;

	len = strlen( in );
	
	// scan backward for '.'
	end = len - 1;
	while ( end && in[end] != '.' && in[end] != '/' && in[end] != '\\' )
		end--;
	
	if ( in[end] != '.' )		// no '.', copy to end
		end = len-1;
	else 
		end--;			// Found ',', copy to left of '.'


	// Scan backward for '/'
	start = len-1;
	while ( start >= 0 && in[start] != '/' && in[start] != '\\' )
		start--;

	if ( in[start] != '/' && in[start] != '\\' )
		start = 0;
	else 
		start++;

	// Length of new sting
	len = end - start + 1;

	// Copy partial string
	strncpy( out, &in[start], len );
	// Terminate it
	out[len] = 0;
}

/*
===============
Info_ValueForKey

Searches the string for the given
key and returns the associated value, or an empty string.
===============
*/
char *Info_ValueForKey( const char *s, const char *key )
{
	static	char value[MAX_INFO_STRING];
	char	pkey[MAX_INFO_STRING];

	if ( *s == '\\' )
		s++;

	while ( 1 )
	{
		char *o = pkey;

		while ( *s != '\\' && *s != '\n' )
		{
			if ( !*s )
				return "";
			*o++ = *s++;
		}

		*o = 0;
		s++;

		o = value;

		while ( *s != '\\' && *s != '\n' && *s )
		{
			if ( !*s )
				return "";
			*o++ = *s++;
		}
		*o = 0;

		if ( !strcmp( key, pkey ))
			return value;

		if ( !*s )
			return "";
		s++;
	}
}

/* 
===================
Key_GetKey
===================
*/
int KEY_GetKey( const char *binding )
{
	const char *b;

	if ( !binding )
		return -1;

	for ( int i = 0; i < 256; i++ )
	{
		b = KEY_GetBinding( i );
		if( !b ) continue;

		if( !stricmp( binding, b ))
			return i;
	}
	return -1;
}

/*
================
UI_FadeAlpha
================
*/
int UI_FadeAlpha( int starttime, int endtime )
{
	int	time, fade_time;

	if( starttime == 0 )
	{
		return 0xFFFFFFFF;
	}

	time = ( gpGlobals->time * 1000 ) - starttime;

	if( time >= endtime )
	{
		return 0x00FFFFFF;
	}

	// fade time is 1/4 of endtime
	fade_time = endtime / 4;
	fade_time = bound( 300, fade_time, 10000 );

	int alpha;

	// fade out
	if(( endtime - time ) < fade_time )
		alpha = bound( 0, (( endtime - time ) * 1.0f / fade_time ) * 255, 255 );
	else alpha = 255;

	return PackRGBA( 255, 255, 255, alpha );
}

void UI_UtilSetupPicButton( menuPicButton_s *pic, int ID )
{
	if( ID < 0 || ID > PC_BUTTONCOUNT )
		return; // bad id

#if 0	// too different results on various games. disabled
	pic->generic.width = PicButtonWidth( ID ) * UI_BUTTON_CHARWIDTH;
#else
	pic->generic.width = UI_BUTTONS_WIDTH;
#endif
	pic->generic.height = UI_BUTTONS_HEIGHT;

	pic->pic = uiStatic.buttonsPics[ID];
	pic->button_id = ID;

	if( pic->pic ) // text buttons not use it
		pic->generic.flags|= QMF_ACT_ONRELEASE;
}

/*
=================
UI_ScrollList_Init
=================
*/
void UI_ScrollList_Init( menuScrollList_s *sl )
{
	if( !sl->generic.name ) sl->generic.name = "";

	if( sl->generic.flags & QMF_BIGFONT )
	{
		sl->generic.charWidth = UI_BIG_CHAR_WIDTH;
		sl->generic.charHeight = UI_BIG_CHAR_HEIGHT;
	}
	else if( sl->generic.flags & QMF_SMALLFONT )
	{
		sl->generic.charWidth = UI_SMALL_CHAR_WIDTH;
		sl->generic.charHeight = UI_SMALL_CHAR_HEIGHT;
	}
	else
	{
		if( sl->generic.charWidth < 1 ) sl->generic.charWidth = UI_MED_CHAR_WIDTH;
		if( sl->generic.charHeight < 1 ) sl->generic.charHeight = UI_MED_CHAR_HEIGHT;
	}

	UI_ScaleCoords( NULL, NULL, &sl->generic.charWidth, &sl->generic.charHeight );

	if(!(sl->generic.flags & (QMF_LEFT_JUSTIFY|QMF_CENTER_JUSTIFY|QMF_RIGHT_JUSTIFY)))
		sl->generic.flags |= QMF_LEFT_JUSTIFY;

	if( !sl->generic.color ) sl->generic.color = uiPromptTextColor;
	if( !sl->generic.focusColor ) sl->generic.focusColor = uiPromptFocusColor;
	if( !sl->upArrow ) sl->upArrow = UI_UPARROW;
	if( !sl->upArrowFocus ) sl->upArrowFocus = UI_UPARROWFOCUS;
	if( !sl->downArrow ) sl->downArrow = UI_DOWNARROW;
	if( !sl->downArrowFocus ) sl->downArrowFocus = UI_DOWNARROWFOCUS;

//	sl->curItem = 0;
	sl->topItem = 0;
	sl->numItems = 0;

	// count number of items
	while( sl->itemNames[sl->numItems] )
		sl->numItems++;

	// scale the center box
	sl->generic.x2 = sl->generic.x;
	sl->generic.y2 = sl->generic.y;
	sl->generic.width2 = sl->generic.width;
	sl->generic.height2 = sl->generic.height;
	UI_ScaleCoords( &sl->generic.x2, &sl->generic.y2, &sl->generic.width2, &sl->generic.height2 );

	// calculate number of visible rows
	sl->numRows = (sl->generic.height2 / sl->generic.charHeight) - 2;
	if( sl->numRows > sl->numItems ) sl->numRows = sl->numItems;

	// extend the height so it has room for the arrows
	sl->generic.height += (sl->generic.width / 4);

	// calculate new Y for the control
	sl->generic.y -= (sl->generic.width / 8);

	UI_ScaleCoords( &sl->generic.x, &sl->generic.y, &sl->generic.width, &sl->generic.height );
}

/*
=================
UI_ScrollList_Key
=================
*/
const char *UI_ScrollList_Key( menuScrollList_s *sl, int key, int down )
{
	const char	*sound = 0;
	int		arrowWidth, arrowHeight, upX, upY, downX, downY;
	int		i, y;

	if( !down ) 
	{
		sl->scrollBarSliding = false;
		return uiSoundNull;
	}

	switch( key )
	{
	case K_MOUSE1:
		if(!( sl->generic.flags & QMF_HASMOUSEFOCUS ))
			break;

		// use fixed size for arrows
		arrowWidth = 24;
		arrowHeight = 24;

		UI_ScaleCoords( NULL, NULL, &arrowWidth, &arrowHeight );

		// glue with right top and right bottom corners
		upX = sl->generic.x2 + sl->generic.width2 - arrowWidth;
		upY = sl->generic.y2 + UI_OUTLINE_WIDTH;
		downX = sl->generic.x2 + sl->generic.width2 - arrowWidth;
		downY = sl->generic.y2 + (sl->generic.height2 - arrowHeight) - UI_OUTLINE_WIDTH;

		// ADAMIX
		if( UI_CursorInRect( sl->scrollBarX, sl->scrollBarY, sl->scrollBarWidth, sl->scrollBarHeight ))
		{
			sl->scrollBarSliding = true;
			break;
		}
		// ADAMIX END

		// Now see if either up or down has focus
		if( UI_CursorInRect( upX, upY, arrowWidth, arrowHeight ))
		{
			if( sl->curItem != 0 )
			{
				sl->curItem--;
				sound = uiSoundMove;
			}
			else sound = uiSoundBuzz;
			break;
		}
		else if( UI_CursorInRect( downX, downY, arrowWidth, arrowHeight ))
		{
			if( sl->curItem != sl->numItems - 1 )
			{
				sl->curItem++;
				sound = uiSoundMove;
			}
			else sound = uiSoundBuzz;
			break;
		}

		// see if an item has been selected
		y = sl->generic.y2 + sl->generic.charHeight;
		for( i = sl->topItem; i < sl->topItem + sl->numRows; i++, y += sl->generic.charHeight )
		{
			if( !sl->itemNames[i] )
				break; // done

			if( UI_CursorInRect( sl->generic.x, y, sl->generic.width, sl->generic.charHeight ))
			{
				sl->curItem = i;
				sound = uiSoundNull;
				break;
			}
		}
		break;
	case K_HOME:
	case K_KP_HOME:
		if( sl->curItem != 0 )
		{
			sl->curItem = 0;
			sound = uiSoundMove;
		}
		else sound = uiSoundBuzz;
		break;
	case K_END:
	case K_KP_END:
		if( sl->curItem != sl->numItems - 1 )
		{
			sl->curItem = sl->numItems - 1;
			sound = uiSoundMove;
		}
		else sound = uiSoundBuzz;
		break;
	case K_PGUP:
	case K_KP_PGUP:
		if( sl->curItem != 0 )
		{
			sl->curItem -= 2;
			if( sl->curItem < 0 )
				sl->curItem = 0;
			sound = uiSoundMove;
		}
		else sound = uiSoundBuzz;
		break;
	case K_PGDN:
	case K_KP_PGDN:
		if( sl->curItem != sl->numItems - 1 )
		{
			sl->curItem += 2;
			if( sl->curItem > sl->numItems - 1 )
				sl->curItem = sl->numItems - 1;
			sound = uiSoundMove;
		}
		else sound = uiSoundBuzz;
		break;
	case K_UPARROW:
	case K_KP_UPARROW:
	case K_MWHEELUP:
		if( sl->curItem != 0 )
		{
			sl->curItem--;
			sound = uiSoundMove;
		}
		else sound = uiSoundBuzz;
		break;
	case K_DOWNARROW:
	case K_MWHEELDOWN:
		if( sl->numItems > 0 && sl->curItem != sl->numItems - 1 )
		{
			sl->curItem++;
			sound = uiSoundMove;
		}
		else sound = uiSoundBuzz;
		break;
	}

	sl->topItem = sl->curItem - sl->numRows + 1;
	if( sl->topItem < 0 ) sl->topItem = 0;
	if( sl->topItem > sl->numItems - sl->numRows )
		sl->topItem = sl->numItems - sl->numRows;

	if( sound && ( sl->generic.flags & QMF_SILENT ))
		sound = uiSoundNull;

	if( sound && sl->generic.callback )
	{
		if( sound != uiSoundBuzz )
			sl->generic.callback( sl, QM_CHANGED );
	}
	return sound;
}

/*
=================
UI_ScrollList_Draw
=================
*/
void UI_ScrollList_Draw( menuScrollList_s *sl )
{
	int	justify;
	int	shadow;
	int	i, x, y, w, h;
	int	selColor = 0xFF503818; // Red 80, Green 56, Blue 24, Alpha 255
	int	arrowWidth, arrowHeight, upX, upY, downX, downY;
	int	upFocus, downFocus, scrollbarFocus;

	if( sl->generic.flags & QMF_LEFT_JUSTIFY )
		justify = 0;
	else if( sl->generic.flags & QMF_CENTER_JUSTIFY )
		justify = 1;
	else if( sl->generic.flags & QMF_RIGHT_JUSTIFY )
		justify = 2;

	shadow = (sl->generic.flags & QMF_DROPSHADOW);

	// use fixed size for arrows
	arrowWidth = 24;
	arrowHeight = 24;

	UI_ScaleCoords( NULL, NULL, &arrowWidth, &arrowHeight );

	x = sl->generic.x2;
	y = sl->generic.y2;
	w = sl->generic.width2;
	h = sl->generic.height2;

	if( !sl->background )
	{
		// draw the opaque outlinebox first 
		UI_FillRect( x, y, w, h, uiColorBlack );
	}

	// hightlight the selected item
	if( !( sl->generic.flags & QMF_GRAYED ))
	{
		y = sl->generic.y2 + sl->generic.charHeight;
		for( i = sl->topItem; i < sl->topItem + sl->numRows; i++, y += sl->generic.charHeight )
		{
			if( !sl->itemNames[i] )
				break;		// Done

			if( i == sl->curItem )
			{
				UI_FillRect( sl->generic.x, y, sl->generic.width - arrowWidth, sl->generic.charHeight, selColor );
				break;
			}
		}
	}

	if( sl->background )
	{
		// get size and position for the center box
		UI_DrawPic( x, y, w, h, uiColorWhite, sl->background );
	}
	else
	{
		x = sl->generic.x2 - UI_OUTLINE_WIDTH;
		y = sl->generic.y2;
		w = UI_OUTLINE_WIDTH;
		h = sl->generic.height2;

		// draw left
		UI_FillRect( x, y, w, h, uiInputFgColor );

		x = sl->generic.x2 + sl->generic.width2;
		y = sl->generic.y2;
		w = UI_OUTLINE_WIDTH;
		h = sl->generic.height2;

		// draw right
		UI_FillRect( x, y, w, h, uiInputFgColor );

		x = sl->generic.x2;
		y = sl->generic.y2;
		w = sl->generic.width2 + UI_OUTLINE_WIDTH;
		h = UI_OUTLINE_WIDTH;

		// draw top
		UI_FillRect( x, y, w, h, uiInputFgColor );

		// draw bottom
		x = sl->generic.x2;
		y = sl->generic.y2 + sl->generic.height2 - UI_OUTLINE_WIDTH;
		w = sl->generic.width2 + UI_OUTLINE_WIDTH;
		h = UI_OUTLINE_WIDTH;

		UI_FillRect( x, y, w, h, uiInputFgColor );
	}

	// glue with right top and right bottom corners
	upX = sl->generic.x2 + sl->generic.width2 - arrowWidth;
	upY = sl->generic.y2 + UI_OUTLINE_WIDTH;
	downX = sl->generic.x2 + sl->generic.width2 - arrowWidth;
	downY = sl->generic.y2 + (sl->generic.height2 - arrowHeight) - UI_OUTLINE_WIDTH;

	// draw the arrows base
	UI_FillRect( upX, upY + arrowHeight, arrowWidth, downY - upY - arrowHeight, uiInputFgColor );

	// ADAMIX
	sl->scrollBarX = upX + sl->generic.charHeight/4;
	sl->scrollBarWidth = arrowWidth - sl->generic.charHeight/4;
	
	int step = (sl->numItems <= 1 ) ? 1 : (downY - upY - arrowHeight) / (sl->numItems - 1);

	if(((downY - upY - arrowHeight) - (((sl->numItems-1)*sl->generic.charHeight)/2)) < 2)
	{
		sl->scrollBarHeight = (downY - upY - arrowHeight) - (step * (sl->numItems-1));
		sl->scrollBarY = upY + arrowHeight + (step*sl->curItem);
	}
	else
	{
		sl->scrollBarHeight = downY - upY - arrowHeight - (((sl->numItems-1) * sl->generic.charHeight) / 2);
		sl->scrollBarY = upY + arrowHeight + (((sl->curItem) * sl->generic.charHeight)/2);
	}

	if( sl->scrollBarSliding )
	{
		int dist = uiStatic.cursorY - sl->scrollBarY - (sl->scrollBarHeight>>2);

		if((((dist / 2) > (sl->generic.charHeight / 2)) || ((dist / 2) < (sl->generic.charHeight / 2))) && sl->curItem <= (sl->numItems - 1) && sl->curItem >= 0)
		{
			if(sl->generic.callback)
				sl->generic.callback( sl, QM_CHANGED );

			if((dist / 2) > ( sl->generic.charHeight / 2 ) && sl->curItem < ( sl->numItems - 1 ))
			{
				sl->curItem++;
			}
			
			if((dist / 2) < -(sl->generic.charHeight / 2) && sl->curItem > 0 )
			{
				sl->curItem--;
			}
		}

		sl->topItem = sl->curItem - sl->numRows + 1;
		if( sl->topItem < 0 ) sl->topItem = 0;
		if( sl->topItem > ( sl->numItems - sl->numRows ))
			sl->topItem = sl->numItems - sl->numRows;
	}

	if( sl->scrollBarSliding )
	{
		// Draw scrollbar background
		UI_FillRect ( sl->scrollBarX, upY + arrowHeight, sl->scrollBarWidth, downY - upY - arrowHeight, uiColorBlack);
	}

	// ADAMIX END
	// draw the arrows
	if( sl->generic.flags & QMF_GRAYED )
	{
		UI_DrawPic( upX, upY, arrowWidth, arrowHeight, uiColorDkGrey, sl->upArrow );
		UI_DrawPic( downX, downY, arrowWidth, arrowHeight, uiColorDkGrey, sl->downArrow );
	}
	else
	{
		scrollbarFocus = UI_CursorInRect( sl->scrollBarX, sl->scrollBarY, sl->scrollBarWidth, sl->scrollBarHeight );

		// special case if we sliding but lost focus
		if( sl->scrollBarSliding ) scrollbarFocus = true;

		// Draw scrollbar itself
		UI_FillRect( sl->scrollBarX, sl->scrollBarY, sl->scrollBarWidth, sl->scrollBarHeight, scrollbarFocus ? uiInputTextColor : uiColorBlack );
	
		if((menuCommon_s *)sl != (menuCommon_s *)UI_ItemAtCursor(sl->generic.parent))
		{
			UI_DrawPic( upX, upY, arrowWidth, arrowHeight, uiColorWhite, sl->upArrow );
			UI_DrawPic( downX, downY, arrowWidth, arrowHeight, uiColorWhite, sl->downArrow );
		}
		else
		{
			// see which arrow has the mouse focus
			upFocus = UI_CursorInRect( upX, upY, arrowWidth, arrowHeight );
			downFocus = UI_CursorInRect( downX, downY, arrowWidth, arrowHeight );

			if(!( sl->generic.flags & QMF_FOCUSBEHIND ))
			{
				UI_DrawPic( upX, upY, arrowWidth, arrowHeight, uiColorWhite, sl->upArrow );
				UI_DrawPic( downX, downY, arrowWidth, arrowHeight, uiColorWhite, sl->downArrow );
			}

			if( sl->generic.flags & QMF_HIGHLIGHTIFFOCUS )
			{
				UI_DrawPic( upX, upY, arrowWidth, arrowHeight, uiColorWhite, (upFocus) ? sl->upArrowFocus : sl->upArrow );
				UI_DrawPic( downX, downY, arrowWidth, arrowHeight, uiColorWhite, (downFocus) ? sl->downArrowFocus : sl->downArrow );
			}
			else if( sl->generic.flags & QMF_PULSEIFFOCUS )
			{
				int	color;

				color = PackAlpha( sl->generic.color, 255 * (0.5 + 0.5 * sin( uiStatic.realTime / UI_PULSE_DIVISOR )));

				UI_DrawPic( upX, upY, arrowWidth, arrowHeight, (upFocus) ? color : sl->generic.color, (upFocus) ? sl->upArrowFocus : sl->upArrow );
				UI_DrawPic( downX, downY, arrowWidth, arrowHeight, (downFocus) ? color : sl->generic.color, (downFocus) ? sl->downArrowFocus : sl->downArrow );
			}

			if( sl->generic.flags & QMF_FOCUSBEHIND )
			{
				UI_DrawPic( upX, upY, arrowWidth, arrowHeight, sl->generic.color, sl->upArrow );
				UI_DrawPic( downX, downY, arrowWidth, arrowHeight, sl->generic.color, sl->downArrow );
			}
		}
	}
	
	// Draw the list
	x = sl->generic.x2;
	w = sl->generic.width2;
	h = sl->generic.charHeight;
	y = sl->generic.y2 + sl->generic.charHeight;

	// prevent the columns out of rectangle bounds
	PIC_EnableScissor( x, y, sl->generic.width - arrowWidth - uiStatic.outlineWidth, sl->generic.height );

	for( i = sl->topItem; i < sl->topItem + sl->numRows; i++, y += sl->generic.charHeight )
	{
		if( !sl->itemNames[i] )
			break;	// done

		if( sl->generic.flags & QMF_GRAYED )
		{
			UI_DrawString( x, y, w, h, sl->itemNames[i], uiColorDkGrey, true, sl->generic.charWidth, sl->generic.charHeight, justify, shadow );
			continue;	// grayed
		}

		if( i != sl->curItem )
		{
			UI_DrawString( x, y, w, h, sl->itemNames[i], sl->generic.color, false, sl->generic.charWidth, sl->generic.charHeight, justify, shadow );
			continue;	// no focus
		}

		if(!( sl->generic.flags & QMF_FOCUSBEHIND ))
			UI_DrawString( x, y, w, h, sl->itemNames[i], sl->generic.color, false, sl->generic.charWidth, sl->generic.charHeight, justify, shadow );

		if( sl->generic.flags & QMF_HIGHLIGHTIFFOCUS )
			UI_DrawString( x, y, w, h, sl->itemNames[i], sl->generic.focusColor, false, sl->generic.charWidth, sl->generic.charHeight, justify, shadow );
		else if( sl->generic.flags & QMF_PULSEIFFOCUS )
		{
			int	color;

			color = PackAlpha( sl->generic.color, 255 * (0.5 + 0.5 * sin( uiStatic.realTime / UI_PULSE_DIVISOR )));

			UI_DrawString( x, y, w, h, sl->itemNames[i], color, false, sl->generic.charWidth, sl->generic.charHeight, justify, shadow );
		}

		if( sl->generic.flags & QMF_FOCUSBEHIND )
			UI_DrawString( x, y, w, h, sl->itemNames[i], sl->generic.color, false, sl->generic.charWidth, sl->generic.charHeight, justify, shadow );
	}

	PIC_DisableScissor();
}

/*
=================
UI_SpinControl_Init
=================
*/
void UI_SpinControl_Init( menuSpinControl_s *sc )
{
	if( !sc->generic.name ) sc->generic.name = "";	// this is also the text displayed

	if( sc->generic.flags & QMF_BIGFONT )
	{
		sc->generic.charWidth = UI_BIG_CHAR_WIDTH;
		sc->generic.charHeight = UI_BIG_CHAR_HEIGHT;
	}
	else if( sc->generic.flags & QMF_SMALLFONT )
	{
		sc->generic.charWidth = UI_SMALL_CHAR_WIDTH;
		sc->generic.charHeight = UI_SMALL_CHAR_HEIGHT;
	}
	else
	{
		if( sc->generic.charWidth < 1 ) sc->generic.charWidth = UI_MED_CHAR_WIDTH;
		if( sc->generic.charHeight < 1 ) sc->generic.charHeight = UI_MED_CHAR_HEIGHT;
	}

	UI_ScaleCoords( NULL, NULL, &sc->generic.charWidth, &sc->generic.charHeight );

	if(!( sc->generic.flags & (QMF_LEFT_JUSTIFY|QMF_CENTER_JUSTIFY|QMF_RIGHT_JUSTIFY)))
		sc->generic.flags |= QMF_LEFT_JUSTIFY;

	if( !sc->generic.color ) sc->generic.color = uiColorHelp;
	if( !sc->generic.focusColor ) sc->generic.focusColor = uiPromptTextColor;
	if( !sc->leftArrow ) sc->leftArrow = UI_LEFTARROW;
	if( !sc->leftArrowFocus ) sc->leftArrowFocus = UI_LEFTARROWFOCUS;
	if( !sc->rightArrow ) sc->rightArrow = UI_RIGHTARROW;
	if( !sc->rightArrowFocus ) sc->rightArrowFocus = UI_RIGHTARROWFOCUS;

	// scale the center box
	sc->generic.x2 = sc->generic.x;
	sc->generic.y2 = sc->generic.y;
	sc->generic.width2 = sc->generic.width;
	sc->generic.height2 = sc->generic.height;
	UI_ScaleCoords( &sc->generic.x2, &sc->generic.y2, &sc->generic.width2, &sc->generic.height2 );

	// extend the width so it has room for the arrows
	sc->generic.width += (sc->generic.height * 3);

	// calculate new X for the control
	sc->generic.x -= (sc->generic.height + (sc->generic.height/2));

	UI_ScaleCoords( &sc->generic.x, &sc->generic.y, &sc->generic.width, &sc->generic.height );
}

/*
=================
UI_SpinControl_Key
=================
*/
const char *UI_SpinControl_Key( menuSpinControl_s *sc, int key, int down )
{
	const char	*sound = 0;
	int		arrowWidth, arrowHeight, leftX, leftY, rightX, rightY;

	if( !down ) return uiSoundNull;

	switch( key )
	{
	case K_MOUSE1:
	case K_MOUSE2:
		if( !( sc->generic.flags & QMF_HASMOUSEFOCUS ))
			break;

		// calculate size and position for the arrows
		arrowWidth = sc->generic.height + (UI_OUTLINE_WIDTH * 2);
		arrowHeight = sc->generic.height + (UI_OUTLINE_WIDTH * 2);

		leftX = sc->generic.x + UI_OUTLINE_WIDTH;
		leftY = sc->generic.y - UI_OUTLINE_WIDTH;
		rightX = sc->generic.x + (sc->generic.width - arrowWidth) - UI_OUTLINE_WIDTH;
		rightY = sc->generic.y - UI_OUTLINE_WIDTH;

		// now see if either left or right arrow has focus
		if( UI_CursorInRect( leftX, leftY, arrowWidth, arrowHeight ))
		{
			if( sc->curValue > sc->minValue )
			{
				sc->curValue -= sc->range;
				if( sc->curValue < sc->minValue )
					sc->curValue = sc->minValue;
				sound = uiSoundMove;
			}
			else sound = uiSoundBuzz;
		}
		else if( UI_CursorInRect( rightX, rightY, arrowWidth, arrowHeight ))
		{
			if( sc->curValue < sc->maxValue )
			{
				sc->curValue += sc->range;
				if( sc->curValue > sc->maxValue )
					sc->curValue = sc->maxValue;
				sound = uiSoundMove;
			}
			else sound = uiSoundBuzz;
		}
		break;
	case K_LEFTARROW:
	case K_KP_LEFTARROW:
		if( sc->generic.flags & QMF_MOUSEONLY )
			break;
		if( sc->curValue > sc->minValue )
		{
			sc->curValue -= sc->range;
			if( sc->curValue < sc->minValue )
				sc->curValue = sc->minValue;
			sound = uiSoundMove;
		}
		else sound = uiSoundBuzz;
		break;
	case K_RIGHTARROW:
	case K_KP_RIGHTARROW:
		if( sc->generic.flags & QMF_MOUSEONLY )
			break;

		if( sc->curValue < sc->maxValue )
		{
			sc->curValue += sc->range;
			if( sc->curValue > sc->maxValue )
				sc->curValue = sc->maxValue;
			sound = uiSoundMove;
		}
		else sound = uiSoundBuzz;
		break;
	}

	if( sound && ( sc->generic.flags & QMF_SILENT ))
		sound = uiSoundNull;

	if( sound && sc->generic.callback )
	{
		if( sound != uiSoundBuzz )
			sc->generic.callback( sc, QM_CHANGED );
	}
	return sound;
}

/*
=================
UI_SpinControl_Draw
=================
*/
void UI_SpinControl_Draw( menuSpinControl_s *sc )
{
	int	justify;
	int	shadow;
	int	x, y, w, h;
	int	arrowWidth, arrowHeight, leftX, leftY, rightX, rightY;
	int	leftFocus, rightFocus;
	
	if( sc->generic.flags & QMF_LEFT_JUSTIFY )
		justify = 0;
	else if( sc->generic.flags & QMF_CENTER_JUSTIFY )
		justify = 1;
	else if( sc->generic.flags & QMF_RIGHT_JUSTIFY )
		justify = 2;

	shadow = (sc->generic.flags & QMF_DROPSHADOW);

	// calculate size and position for the arrows
	arrowWidth = sc->generic.height + (UI_OUTLINE_WIDTH * 2);
	arrowHeight = sc->generic.height + (UI_OUTLINE_WIDTH * 2);

	leftX = sc->generic.x + UI_OUTLINE_WIDTH;
	leftY = sc->generic.y - UI_OUTLINE_WIDTH;
	rightX = sc->generic.x + (sc->generic.width - arrowWidth) - UI_OUTLINE_WIDTH;
	rightY = sc->generic.y - UI_OUTLINE_WIDTH;

	// get size and position for the center box
	w = sc->generic.width2;
	h = sc->generic.height2;
	x = sc->generic.x2;
	y = sc->generic.y2;

	if( sc->background )
	{
		UI_DrawPic( x, y, w, h, uiColorWhite, sc->background );
	}
	else
	{
		// draw the background
		UI_FillRect( x, y, w, h, uiColorBlack );

		// draw the rectangle
		UI_DrawRectangle( x, y, w, h, uiInputFgColor );
	}

	if( sc->generic.flags & QMF_GRAYED )
	{
		UI_DrawString( x, y, w, h, sc->generic.name, uiColorDkGrey, true, sc->generic.charWidth, sc->generic.charHeight, justify, shadow );
		UI_DrawPic( leftX, leftY, arrowWidth, arrowHeight, uiColorDkGrey, sc->leftArrow );
		UI_DrawPic( rightX, rightY, arrowWidth, arrowHeight, uiColorDkGrey, sc->rightArrow );
		return; // grayed
	}

	if((menuCommon_s *)sc != (menuCommon_s *)UI_ItemAtCursor(sc->generic.parent ))
	{
		UI_DrawString(x, y, w, h, sc->generic.name, sc->generic.color, false, sc->generic.charWidth, sc->generic.charHeight, justify, shadow );
		UI_DrawPic(leftX, leftY, arrowWidth, arrowHeight, sc->generic.color, sc->leftArrow);
		UI_DrawPic(rightX, rightY, arrowWidth, arrowHeight, sc->generic.color, sc->rightArrow);
		return;		// No focus
	}

	// see which arrow has the mouse focus
	leftFocus = UI_CursorInRect( leftX, leftY, arrowWidth, arrowHeight );
	rightFocus = UI_CursorInRect( rightX, rightY, arrowWidth, arrowHeight );

	if( !( sc->generic.flags & QMF_FOCUSBEHIND ))
	{
		UI_DrawString( x, y, w, h, sc->generic.name, sc->generic.color, false, sc->generic.charWidth, sc->generic.charHeight, justify, shadow );
		UI_DrawPic( leftX, leftY, arrowWidth, arrowHeight, sc->generic.color, sc->leftArrow );
		UI_DrawPic( rightX, rightY, arrowWidth, arrowHeight, sc->generic.color, sc->rightArrow );
	}

	if( sc->generic.flags & QMF_HIGHLIGHTIFFOCUS )
	{
		UI_DrawString( x, y, w, h, sc->generic.name, sc->generic.focusColor, false, sc->generic.charWidth, sc->generic.charHeight, justify, shadow );
		UI_DrawPic( leftX, leftY, arrowWidth, arrowHeight, (leftFocus) ? sc->generic.color : sc->generic.color, (leftFocus) ? sc->leftArrowFocus : sc->leftArrow );
		UI_DrawPic( rightX, rightY, arrowWidth, arrowHeight, (rightFocus) ? sc->generic.color : sc->generic.color, (rightFocus) ? sc->rightArrowFocus : sc->rightArrow );
	}
	else if( sc->generic.flags & QMF_PULSEIFFOCUS )
	{
		int	color;

		color = PackAlpha( sc->generic.color, 255 * (0.5 + 0.5 * sin( uiStatic.realTime / UI_PULSE_DIVISOR )));

		UI_DrawString( x, y, w, h, sc->generic.name, color, false, sc->generic.charWidth, sc->generic.charHeight, justify, shadow );
		UI_DrawPic( leftX, leftY, arrowWidth, arrowHeight, (leftFocus) ? color : sc->generic.color, (leftFocus) ? sc->leftArrowFocus : sc->leftArrow );
		UI_DrawPic( rightX, rightY, arrowWidth, arrowHeight, (rightFocus) ? color : sc->generic.color, (rightFocus) ? sc->rightArrowFocus : sc->rightArrow );
	}

	if( sc->generic.flags & QMF_FOCUSBEHIND )
	{
		UI_DrawString( x, y, w, h, sc->generic.name, sc->generic.color, false, sc->generic.charWidth, sc->generic.charHeight, justify, shadow );
		UI_DrawPic( leftX, leftY, arrowWidth, arrowHeight, sc->generic.color, sc->leftArrow );
		UI_DrawPic( rightX, rightY, arrowWidth, arrowHeight, sc->generic.color, sc->rightArrow );
	}
}

/*
=================
UI_Slider_Init
=================
*/
void UI_Slider_Init( menuSlider_s *sl )
{
	if( !sl->generic.name ) sl->generic.name = "";	// this is also the text displayed

	if( !sl->generic.width ) sl->generic.width = 200;
	if( !sl->generic.height) sl->generic.height = 4;
	if( !sl->generic.color ) sl->generic.color = uiColorWhite;
	if( !sl->generic.focusColor ) sl->generic.focusColor = uiColorWhite;
	if( !sl->range ) sl->range = 1.0f;
	if( sl->range < 0.05f ) sl->range = 0.05f;

	if( sl->generic.flags & QMF_BIGFONT )
	{
		sl->generic.charWidth = UI_BIG_CHAR_WIDTH;
		sl->generic.charHeight = UI_BIG_CHAR_HEIGHT;
	}
	else if( sl->generic.flags & QMF_SMALLFONT )
	{
		sl->generic.charWidth = UI_SMALL_CHAR_WIDTH;
		sl->generic.charHeight = UI_SMALL_CHAR_HEIGHT;
	}
	else
	{
		if( sl->generic.charWidth < 1 ) sl->generic.charWidth = 12;
		if( sl->generic.charHeight < 1 ) sl->generic.charHeight = 24;
	}

	UI_ScaleCoords( NULL, NULL, &sl->generic.charWidth, &sl->generic.charHeight );

	if(!(sl->generic.flags & (QMF_LEFT_JUSTIFY|QMF_CENTER_JUSTIFY|QMF_RIGHT_JUSTIFY)))
		sl->generic.flags |= QMF_LEFT_JUSTIFY;

	// scale the center box
	sl->generic.x2 = sl->generic.x;
	sl->generic.y2 = sl->generic.y;
	sl->generic.width2 = sl->generic.width / 5;
	sl->generic.height2 = 4;

	UI_ScaleCoords( &sl->generic.x2, &sl->generic.y2, &sl->generic.width2, &sl->generic.height2 );
	UI_ScaleCoords( &sl->generic.x, &sl->generic.y, &sl->generic.width, &sl->generic.height );

	sl->generic.y -= uiStatic.sliderWidth;
	sl->generic.height += uiStatic.sliderWidth * 2;
	sl->generic.y2 -= uiStatic.sliderWidth;

	sl->drawStep = (sl->generic.width - sl->generic.width2) / ((sl->maxValue - sl->minValue) / sl->range);
	sl->numSteps = ((sl->maxValue - sl->minValue) / sl->range) + 1;
}

/*
=================
UI_Slider_Key
=================
*/
const char *UI_Slider_Key( menuSlider_s *sl, int key, int down )
{
	int	sliderX;

	if( !down )
	{
		if( sl->keepSlider )
		{
			// tell menu about changes
			if( sl->generic.callback )
				sl->generic.callback( sl, QM_CHANGED );
			sl->keepSlider = false; // button released
		}
		return uiSoundNull;
	}

	switch( key )
	{
	case K_MOUSE1:
		if(!( sl->generic.flags & QMF_HASMOUSEFOCUS ))
			return uiSoundNull;

		// find the current slider position
		sliderX = sl->generic.x2 + (sl->drawStep * (sl->curValue * sl->numSteps));		
                    if( UI_CursorInRect( sliderX, sl->generic.y2, sl->generic.width2, sl->generic.height ))
                    {
			sl->keepSlider = true;
		}
		else
		{
			int	dist, numSteps;

			// immediately move slider into specified place
			dist = uiStatic.cursorX - sl->generic.x2 - (sl->generic.width2>>2);
			numSteps = dist / (int)sl->drawStep;
			sl->curValue = bound( sl->minValue, numSteps * sl->range, sl->maxValue );

			// tell menu about changes
			if( sl->generic.callback )
				sl->generic.callback( sl, QM_CHANGED );
		}
		break;
	}
	return uiSoundNull;
}

/*
=================
UI_Slider_Draw
=================
*/
void UI_Slider_Draw( menuSlider_s *sl )
{
	int	justify;
	int	shadow;
	int	textHeight, sliderX;

	if( sl->generic.flags & QMF_LEFT_JUSTIFY )
		justify = 0;
	else if( sl->generic.flags & QMF_CENTER_JUSTIFY )
		justify = 1;
	else if( sl->generic.flags & QMF_RIGHT_JUSTIFY )
		justify = 2;

	shadow = (sl->generic.flags & QMF_DROPSHADOW);

	if( sl->keepSlider )
	{
		int	dist, numSteps;

		// move slider follow the holded mouse button
		dist = uiStatic.cursorX - sl->generic.x2 - (sl->generic.width2>>2);
		numSteps = dist / (int)sl->drawStep;
		sl->curValue = bound( sl->minValue, numSteps * sl->range, sl->maxValue );
		
		// tell menu about changes
		if( sl->generic.callback ) sl->generic.callback( sl, QM_CHANGED );
	}

	// keep value in range
	sl->curValue = bound( sl->minValue, sl->curValue, sl->maxValue );

	// calc slider position
	sliderX = sl->generic.x2 + (sl->drawStep * (sl->curValue * sl->numSteps));

	UI_DrawRectangleExt( sl->generic.x, sl->generic.y + uiStatic.sliderWidth, sl->generic.width, sl->generic.height2, uiInputBgColor, uiStatic.sliderWidth );
	UI_DrawPic( sliderX, sl->generic.y2, sl->generic.width2, sl->generic.height, uiColorWhite, UI_SLIDER_MAIN );

	textHeight = sl->generic.y - (sl->generic.charHeight * 1.5f);
	UI_DrawString( sl->generic.x, textHeight, sl->generic.width, sl->generic.charHeight, sl->generic.name, uiColorHelp, true, sl->generic.charWidth, sl->generic.charHeight, justify, shadow );
}

/*
=================
UI_CheckBox_Init
=================
*/
void UI_CheckBox_Init( menuCheckBox_s *cb )
{
	if( !cb->generic.name ) cb->generic.name = "";

	if( cb->generic.flags & QMF_BIGFONT )
	{
		cb->generic.charWidth = UI_BIG_CHAR_WIDTH;
		cb->generic.charHeight = UI_BIG_CHAR_HEIGHT;
	}
	else if( cb->generic.flags & QMF_SMALLFONT )
	{
		cb->generic.charWidth = UI_SMALL_CHAR_WIDTH;
		cb->generic.charHeight = UI_SMALL_CHAR_HEIGHT;
	}
	else
	{
		if( cb->generic.charWidth < 1 ) cb->generic.charWidth = 12;
		if( cb->generic.charHeight < 1 ) cb->generic.charHeight = 24;
	}

	UI_ScaleCoords( NULL, NULL, &cb->generic.charWidth, &cb->generic.charHeight );

	if(!(cb->generic.flags & (QMF_LEFT_JUSTIFY|QMF_CENTER_JUSTIFY|QMF_RIGHT_JUSTIFY)))
		cb->generic.flags |= QMF_LEFT_JUSTIFY;

	if( !cb->emptyPic ) cb->emptyPic = UI_CHECKBOX_EMPTY;
	if( !cb->focusPic ) cb->focusPic = UI_CHECKBOX_FOCUS;
	if( !cb->checkPic ) cb->checkPic = UI_CHECKBOX_ENABLED;
	if( !cb->grayedPic ) cb->grayedPic = UI_CHECKBOX_GRAYED;
	if( !cb->generic.color ) cb->generic.color = uiColorWhite;
	if( !cb->generic.focusColor ) cb->generic.focusColor = uiColorWhite;

	if( !cb->generic.width ) cb->generic.width = 32;
	if( !cb->generic.height ) cb->generic.height = 32;

	UI_ScaleCoords( &cb->generic.x, &cb->generic.y, &cb->generic.width, &cb->generic.height );
}

/*
=================
UI_CheckBox_Key
=================
*/
const char *UI_CheckBox_Key( menuCheckBox_s *cb, int key, int down )
{
	const char	*sound = 0;

	switch( key )
	{
	case K_MOUSE1:
		if(!( cb->generic.flags & QMF_HASMOUSEFOCUS ))
			break;
		sound = uiSoundGlow;
		break;
	case K_ENTER:
	case K_KP_ENTER:
		if( !down ) return sound;
		if( cb->generic.flags & QMF_MOUSEONLY )
			break;
		sound = uiSoundGlow;
		break;
	}
	if( sound && ( cb->generic.flags & QMF_SILENT ))
		sound = uiSoundNull;

	if( cb->generic.flags & QMF_ACT_ONRELEASE )
	{
		if( sound && cb->generic.callback )
		{
			int	event;

			if( down )
			{
				event = QM_PRESSED;
				cb->generic.bPressed = true;
			}
			else event = QM_CHANGED;
			if( !down ) cb->enabled = !cb->enabled;	// apply on release
			cb->generic.callback( cb, event );
		}
	}
	else if( down )
	{
		if( sound && cb->generic.callback )
		{
			cb->enabled = !cb->enabled;
			cb->generic.callback( cb, QM_CHANGED );
          	}
          }
	return sound;
}

/*
=================
UI_CheckBox_Draw
=================
*/
void UI_CheckBox_Draw( menuCheckBox_s *cb )
{
	int	justify;
	int	shadow;
	int	textOffset, y;

	if( cb->generic.flags & QMF_LEFT_JUSTIFY )
		justify = 0;
	else if( cb->generic.flags & QMF_CENTER_JUSTIFY )
		justify = 1;
	else if( cb->generic.flags & QMF_RIGHT_JUSTIFY )
		justify = 2;

	shadow = (cb->generic.flags & QMF_DROPSHADOW);

	y = cb->generic.y + (cb->generic.height>>2);
	textOffset = cb->generic.x + (cb->generic.width * 1.7f);
	UI_DrawString( textOffset, y, strlen( cb->generic.name ) * cb->generic.charWidth, cb->generic.charHeight, cb->generic.name, uiColorHelp, true, cb->generic.charWidth, cb->generic.charHeight, justify, shadow );

	if( cb->generic.statusText && cb->generic.flags & QMF_NOTIFY )
	{
		int	charW, charH;
		int	x, w;

		charW = UI_SMALL_CHAR_WIDTH;
		charH = UI_SMALL_CHAR_HEIGHT;

		UI_ScaleCoords( NULL, NULL, &charW, &charH );

		x = 250;
		w = UI_SMALL_CHAR_WIDTH * strlen( cb->generic.statusText );
		UI_ScaleCoords( &x, NULL, &w, NULL );
		x += cb->generic.x;

		int	r, g, b;

		UnpackRGB( r, g, b, uiColorHelp );
		TextMessageSetColor( r, g, b );
		DrawConsoleString( x, cb->generic.y, cb->generic.statusText );
	}

	if( cb->generic.flags & QMF_GRAYED )
	{
		UI_DrawPic( cb->generic.x, cb->generic.y, cb->generic.width, cb->generic.height, uiColorWhite, cb->grayedPic );
		return; // grayed
	}

	if(( cb->generic.flags & QMF_MOUSEONLY ) && !( cb->generic.flags & QMF_HASMOUSEFOCUS ))
	{
		if( !cb->enabled )
			UI_DrawPic( cb->generic.x, cb->generic.y, cb->generic.width, cb->generic.height, cb->generic.color, cb->emptyPic );
		else UI_DrawPic( cb->generic.x, cb->generic.y, cb->generic.width, cb->generic.height, cb->generic.color, cb->checkPic );
		return; // no focus
	}

	if((menuCommon_s *)cb != (menuCommon_s *)UI_ItemAtCursor( cb->generic.parent ))
	{
		if( !cb->enabled )
			UI_DrawPic( cb->generic.x, cb->generic.y, cb->generic.width, cb->generic.height, cb->generic.color, cb->emptyPic );
		else UI_DrawPic( cb->generic.x, cb->generic.y, cb->generic.width, cb->generic.height, cb->generic.color, cb->checkPic );
		return; // no focus
	}

	if( cb->generic.flags & QMF_HIGHLIGHTIFFOCUS && !cb->enabled )
	{
		UI_DrawPic( cb->generic.x, cb->generic.y, cb->generic.width, cb->generic.height, cb->generic.focusColor, cb->focusPic );
	}
	else
	{
		if( !cb->enabled )
			UI_DrawPic( cb->generic.x, cb->generic.y, cb->generic.width, cb->generic.height, cb->generic.color, cb->emptyPic );
		else UI_DrawPic( cb->generic.x, cb->generic.y, cb->generic.width, cb->generic.height, cb->generic.color, cb->checkPic );
	}
}

/*
=================
UI_Field_Init
=================
*/
void UI_Field_Init( menuField_s *f )
{
	if( !f->generic.name ) f->generic.name = "";

	if( f->generic.flags & QMF_BIGFONT )
	{
		f->generic.charWidth = UI_BIG_CHAR_WIDTH;
		f->generic.charHeight = UI_BIG_CHAR_HEIGHT;
	}
	else if( f->generic.flags & QMF_SMALLFONT )
	{
		f->generic.charWidth = UI_SMALL_CHAR_WIDTH;
		f->generic.charHeight = UI_SMALL_CHAR_HEIGHT;
	}
	else
	{
		if( f->generic.charWidth < 1 ) f->generic.charWidth = UI_MED_CHAR_WIDTH;
		if( f->generic.charHeight < 1 ) f->generic.charHeight = UI_MED_CHAR_HEIGHT;
	}

	UI_ScaleCoords( NULL, NULL, &f->generic.charWidth, &f->generic.charHeight );

	if( !(f->generic.flags & (QMF_LEFT_JUSTIFY|QMF_CENTER_JUSTIFY|QMF_RIGHT_JUSTIFY)))
		f->generic.flags |= QMF_LEFT_JUSTIFY;

	if( !f->generic.color ) f->generic.color = uiInputTextColor;
	if( !f->generic.focusColor ) f->generic.focusColor = uiInputTextColor;

	f->maxLength++;
	if( f->maxLength <= 1 || f->maxLength >= UI_MAX_FIELD_LINE )
		f->maxLength = UI_MAX_FIELD_LINE - 1;

	UI_ScaleCoords( &f->generic.x, &f->generic.y, &f->generic.width, &f->generic.height );

	// calculate number of visible characters
	f->widthInChars = (f->generic.width / f->generic.charWidth);

	f->cursor = strlen( f->buffer );
}

/*
================
UI_Field_Paste
================
*/
void UI_Field_Paste( void )
{
	char	*str;
	int	pasteLen, i;

	str = GET_CLIPBOARD ();
	if( !str ) return;

	// send as if typed, so insert / overstrike works properly
	pasteLen = strlen( str );
	for( i = 0; i < pasteLen; i++ )
		UI_CharEvent( str[i] );
	FREE( str );
}

/*
================
UI_Field_Clear
================
*/
void UI_Field_Clear( menuField_s *f )
{
	memset( f->buffer, 0, UI_MAX_FIELD_LINE );
	f->cursor = 0;
	f->scroll = 0;
}


/*
=================
UI_Field_Key
=================
*/
const char *UI_Field_Key( menuField_s *f, int key, int down )
{
	int	len;

	if( !down ) return 0;

	// clipboard paste
	if((( key == K_INS ) || ( key == K_KP_INS )) && KEY_IsDown( K_SHIFT ))
	{
		UI_Field_Paste();
		return 0;
	}

	len = strlen( f->buffer );

	if( key == K_INS )
	{
		// toggle overstrike mode
		KEY_SetOverstrike( !KEY_GetOverstrike( ));
		return uiSoundNull; // handled
	}

	// previous character
	if( key == K_LEFTARROW )
	{
		if( f->cursor > 0 ) f->cursor--;
		if( f->cursor < f->scroll ) f->scroll--;
		return uiSoundNull;
	}

	// next character
	if( key == K_RIGHTARROW )
	{
		if( f->cursor < len ) f->cursor++;
		if( f->cursor >= f->scroll + f->widthInChars && f->cursor <= len )
			f->scroll++;
		return uiSoundNull;
	}

	// first character
	if( key == K_HOME )
	{
		f->cursor = 0;
		return uiSoundNull;
	}

	// last character
	if( key == K_END )
	{
		f->cursor = len;
		return uiSoundNull;
	}

	if( key == K_BACKSPACE )
	{
		if( f->cursor > 0 )
		{
			memmove( f->buffer + f->cursor - 1, f->buffer + f->cursor, len - f->cursor + 1 );
			f->cursor--;
			if( f->scroll ) f->scroll--;
		}
	}
	if( key == K_DEL )
	{	
		if( f->cursor < len )
			memmove( f->buffer + f->cursor, f->buffer + f->cursor + 1, len - f->cursor );
	}

	if( f->generic.callback )
		f->generic.callback( f, QM_CHANGED );
	return 0;
}

/*
=================
UI_Field_Char
=================
*/
void UI_Field_Char( menuField_s *f, int key )
{
	int	len;

	if( key == 'v' - 'a' + 1 )
	{
		// ctrl-v is paste
		UI_Field_Paste();
		return;
	}

	if( key == 'c' - 'a' + 1 )
	{
		// ctrl-c clears the field
		UI_Field_Clear( f );
		return;
	}

	len = strlen( f->buffer );

	if( key == 'a' - 'a' + 1 )
	{
		// ctrl-a is home
		f->cursor = 0;
		f->scroll = 0;
		return;
	}

	if( key == 'e' - 'a' + 1 )
	{
		// ctrl-e is end
		f->cursor = len;
		f->scroll = f->cursor - f->widthInChars;
		return;
	}

	// ignore any other non printable chars
	if( key < 32 ) return;

	if( key == '^' && !( f->generic.flags & QMF_ALLOW_COLORSTRINGS ))
	{
		// ignore color key-symbol
		return;
	}

	if( f->generic.flags & QMF_NUMBERSONLY )
	{
		if( key < '0' || key > '9' )
			return;
	}

	if( f->generic.flags & QMF_LOWERCASE )
		key = tolower( key );
	else if( f->generic.flags & QMF_UPPERCASE )
		key = toupper( key );

	if( KEY_GetOverstrike( ))
	{	
		if( f->cursor == f->maxLength - 1 ) return;
		f->buffer[f->cursor] = key;
		f->cursor++;
	}
	else
	{
		// insert mode
		if( len == f->maxLength - 1 ) return; // all full
		memmove( f->buffer + f->cursor + 1, f->buffer + f->cursor, len + 1 - f->cursor );
		f->buffer[f->cursor] = key;
		f->cursor++;
	}

	if( f->cursor >= f->widthInChars ) f->scroll++;
	if( f->cursor == len + 1 ) f->buffer[f->cursor] = 0;

	if( f->generic.callback )
		f->generic.callback( f, QM_CHANGED );
}

/*
=================
UI_Field_Draw
=================
*/
void UI_Field_Draw( menuField_s *f )
{
	int	justify;
	int	shadow;
	char	text[UI_MAX_FIELD_LINE];
	int	len, drawLen, prestep;
	int	cursor, x, textHeight;
	char	cursor_char[3];

	if( f->generic.flags & QMF_LEFT_JUSTIFY )
		justify = 0;
	else if( f->generic.flags & QMF_CENTER_JUSTIFY )
		justify = 1;
	else if( f->generic.flags & QMF_RIGHT_JUSTIFY )
		justify = 2;

	shadow = (f->generic.flags & QMF_DROPSHADOW);

	cursor_char[1] = '\0';
	if( KEY_GetOverstrike( ))
		cursor_char[0] = 11;
	else cursor_char[0] = 95;

	drawLen = f->widthInChars;
	len = strlen( f->buffer ) + 1;

	// guarantee that cursor will be visible
	if( len <= drawLen )
	{
		prestep = 0;
	}
	else
	{
		if( f->scroll + drawLen > len )
		{
			f->scroll = len - drawLen;
			if( f->scroll < 0 ) f->scroll = 0;
		}
		prestep = f->scroll;
	}

	if( prestep + drawLen > len )
		drawLen = len - prestep;

	// extract <drawLen> characters from the field at <prestep>
	if( drawLen >= UI_MAX_FIELD_LINE )
		HOST_ERROR( "UI_Field_Draw: drawLen >= UI_MAX_FIELD_LINE\n" );

	memcpy( text, f->buffer + prestep, drawLen );
	text[drawLen] = 0;

	if( f->generic.flags & QMF_HIDEINPUT )
	{
		for( int i = 0; i < drawLen; i++ )
			if( text[i] ) text[i] = '*';
	}

	// find cursor position
	x = drawLen - (ColorStrlen( text ) + 1 );
	if( x < 0 ) x = 0;
	cursor = ( f->cursor - prestep - x );
	if( cursor < 0 ) cursor = 0;

	if( justify == 0 ) x = f->generic.x;
	else if( justify == 1 )
		x = f->generic.x + ((f->generic.width - (ColorStrlen( text ) * f->generic.charWidth )) / 2 );
	else if( justify == 2 )
		x = f->generic.x + (f->generic.width - (ColorStrlen( text ) * f->generic.charWidth ));

	if( f->background )
	{
		UI_DrawPic( f->generic.x, f->generic.y, f->generic.width, f->generic.height, uiColorWhite, f->background );
	}
	else
	{
		// draw the background
		UI_FillRect( f->generic.x, f->generic.y, f->generic.width, f->generic.height, uiInputBgColor );

		// draw the rectangle
		UI_DrawRectangle( f->generic.x, f->generic.y, f->generic.width, f->generic.height, uiInputFgColor );
	}

	textHeight = f->generic.y - (f->generic.charHeight * 1.5f);
	UI_DrawString( f->generic.x, textHeight, f->generic.width, f->generic.charHeight, f->generic.name, uiColorHelp, true, f->generic.charWidth, f->generic.charHeight, 0, shadow );

	if( f->generic.flags & QMF_GRAYED )
	{
		UI_DrawString( f->generic.x, f->generic.y, f->generic.width, f->generic.height, text, uiColorDkGrey, true, f->generic.charWidth, f->generic.charHeight, justify, shadow );
		return; // grayed
	}

	if((menuCommon_s *)f != (menuCommon_s *)UI_ItemAtCursor( f->generic.parent ))
	{
		UI_DrawString( f->generic.x, f->generic.y, f->generic.width, f->generic.height, text, f->generic.color, false, f->generic.charWidth, f->generic.charHeight, justify, shadow );
		return; // no focus
	}

	if( !( f->generic.flags & QMF_FOCUSBEHIND ))
	{
		UI_DrawString( f->generic.x, f->generic.y, f->generic.width, f->generic.height, text, f->generic.color, false, f->generic.charWidth, f->generic.charHeight, justify, shadow );

		if(( uiStatic.realTime & 499 ) < 250 )
			UI_DrawString( x + (cursor * f->generic.charWidth), f->generic.y, f->generic.charWidth, f->generic.height, cursor_char, f->generic.color, true, f->generic.charWidth, f->generic.charHeight, 0, shadow );
	}

	if( f->generic.flags & QMF_HIGHLIGHTIFFOCUS )
	{
		UI_DrawString( f->generic.x, f->generic.y, f->generic.width, f->generic.height, text, f->generic.focusColor, false, f->generic.charWidth, f->generic.charHeight, justify, shadow );

		if(( uiStatic.realTime & 499 ) < 250 )
			UI_DrawString( x + (cursor * f->generic.charWidth), f->generic.y, f->generic.charWidth, f->generic.height, cursor_char, f->generic.focusColor, true, f->generic.charWidth, f->generic.charHeight, 0, shadow );
	}
	else if( f->generic.flags & QMF_PULSEIFFOCUS )
	{
		int	color;

		color = PackAlpha( f->generic.color, 255 * (0.5 + 0.5 * sin( uiStatic.realTime / UI_PULSE_DIVISOR )));
		UI_DrawString( f->generic.x, f->generic.y, f->generic.width, f->generic.height, text, color, false, f->generic.charWidth, f->generic.charHeight, justify, shadow );

		if(( uiStatic.realTime & 499 ) < 250 )
			UI_DrawString( x + (cursor * f->generic.charWidth), f->generic.y, f->generic.charWidth, f->generic.height, cursor_char, color, true, f->generic.charWidth, f->generic.charHeight, 0, shadow );
	}

	if( f->generic.flags & QMF_FOCUSBEHIND )
	{
		UI_DrawString( f->generic.x, f->generic.y, f->generic.width, f->generic.height, text, f->generic.color, false, f->generic.charWidth, f->generic.charHeight, justify, shadow );

		if(( uiStatic.realTime & 499 ) < 250 )
			UI_DrawString( x + (cursor * f->generic.charWidth), f->generic.y, f->generic.charWidth, f->generic.height, cursor_char, f->generic.color, true, f->generic.charWidth, f->generic.charHeight, 0, shadow );
	}
}

/*
=================
UI_Action_Init
=================
*/
void UI_Action_Init( menuAction_s *a )
{
	if( !a->generic.name ) a->generic.name = ""; // this is also the text displayed

	if( a->generic.flags & QMF_BIGFONT )
	{
		a->generic.charWidth = UI_BIG_CHAR_WIDTH;
		a->generic.charHeight = UI_BIG_CHAR_HEIGHT;
	}
	else if( a->generic.flags & QMF_SMALLFONT )
	{
		a->generic.charWidth = UI_SMALL_CHAR_WIDTH;
		a->generic.charHeight = UI_SMALL_CHAR_HEIGHT;
	}
	else
	{
		if( a->generic.charWidth < 1 ) a->generic.charWidth = UI_MED_CHAR_WIDTH;
		if( a->generic.charHeight < 1 ) a->generic.charHeight = UI_MED_CHAR_HEIGHT;
	}

	if(!( a->generic.flags & ( QMF_LEFT_JUSTIFY|QMF_CENTER_JUSTIFY|QMF_RIGHT_JUSTIFY )))
		a->generic.flags |= QMF_LEFT_JUSTIFY;

	if( !a->generic.color ) a->generic.color = uiPromptTextColor;
	if( !a->generic.focusColor ) a->generic.focusColor = uiPromptFocusColor;

	if( a->generic.width < 1 || a->generic.height < 1 )
	{
		if( a->background )
		{
			HIMAGE handle = PIC_Load( a->background );
			a->generic.width = PIC_Width( handle );
			a->generic.height = PIC_Height( handle );
		}
		else
		{
			if( a->generic.width < 1 )
				a->generic.width = a->generic.charWidth * strlen( a->generic.name );

			if( a->generic.height < 1 )
				a->generic.height = a->generic.charHeight * 1.5;
		}
	}

	UI_ScaleCoords( NULL, NULL, &a->generic.charWidth, &a->generic.charHeight );
	UI_ScaleCoords( &a->generic.x, &a->generic.y, &a->generic.width, &a->generic.height );
}

/*
=================
UI_Action_Key
=================
*/
const char *UI_Action_Key( menuAction_s *a, int key, int down )
{
	const char	*sound = 0;

	switch( key )
	{
	case K_MOUSE1:
		if(!( a->generic.flags & QMF_HASMOUSEFOCUS ))
			break;
		sound = uiSoundLaunch;
		break;
	case K_ENTER:
	case K_KP_ENTER:
		if( !down ) return sound;
		if( a->generic.flags & QMF_MOUSEONLY )
			break;
		sound = uiSoundLaunch;
		break;
	}

	if( sound && ( a->generic.flags & QMF_SILENT ))
		sound = uiSoundNull;

	if( a->generic.flags & QMF_ACT_ONRELEASE )
	{
		if( sound && a->generic.callback )
		{
			int	event;

			if( down )
			{
				event = QM_PRESSED;
				a->generic.bPressed = true;
			}
			else event = QM_ACTIVATED;
			a->generic.callback( a, event );
		}
	}
	else if( down )
	{
		if( sound && a->generic.callback )
			a->generic.callback( a, QM_ACTIVATED );
          }

	return sound;
}

/*
=================
UI_Action_Draw
=================
*/
void UI_Action_Draw( menuAction_s *a )
{
	int	justify;
	int	shadow;

	if( a->generic.flags & QMF_LEFT_JUSTIFY )
		justify = 0;
	else if( a->generic.flags & QMF_CENTER_JUSTIFY )
		justify = 1;
	else if( a->generic.flags & QMF_RIGHT_JUSTIFY )
		justify = 2;

	shadow = (a->generic.flags & QMF_DROPSHADOW);

	if( a->background )
		UI_DrawPic( a->generic.x, a->generic.y, a->generic.width, a->generic.height, uiColorWhite, a->background );

	if( a->generic.statusText && a->generic.flags & QMF_NOTIFY )
	{
		int	charW, charH;
		int	x, w;

		charW = UI_SMALL_CHAR_WIDTH;
		charH = UI_SMALL_CHAR_HEIGHT;

		UI_ScaleCoords( NULL, NULL, &charW, &charH );

		x = 290;
		w = UI_SMALL_CHAR_WIDTH * strlen( a->generic.statusText );
		UI_ScaleCoords( &x, NULL, &w, NULL );
		x += a->generic.x;

		int	r, g, b;

		UnpackRGB( r, g, b, uiColorHelp );
		TextMessageSetColor( r, g, b );
		DrawConsoleString( x, a->generic.y, a->generic.statusText );
	}

	if( a->generic.flags & QMF_GRAYED )
	{
		UI_DrawString( a->generic.x, a->generic.y, a->generic.width, a->generic.height, a->generic.name, uiColorDkGrey, true, a->generic.charWidth, a->generic.charHeight, justify, shadow );
		return; // grayed
	}

	if((menuCommon_s *)a != (menuCommon_s *)UI_ItemAtCursor( a->generic.parent ))
	{
		UI_DrawString( a->generic.x, a->generic.y, a->generic.width, a->generic.height, a->generic.name, a->generic.color, false, a->generic.charWidth, a->generic.charHeight, justify, shadow );
		return; // no focus
	}

	if(!( a->generic.flags & QMF_FOCUSBEHIND ))
		UI_DrawString( a->generic.x, a->generic.y, a->generic.width, a->generic.height, a->generic.name, a->generic.color, false, a->generic.charWidth, a->generic.charHeight, justify, shadow );

	if( a->generic.flags & QMF_HIGHLIGHTIFFOCUS )
		UI_DrawString( a->generic.x, a->generic.y, a->generic.width, a->generic.height, a->generic.name, a->generic.focusColor, false, a->generic.charWidth, a->generic.charHeight, justify, shadow );
	else if( a->generic.flags & QMF_PULSEIFFOCUS )
	{
		int	color;

		color = PackAlpha( a->generic.color, 255 * (0.5 + 0.5 * sin( uiStatic.realTime / UI_PULSE_DIVISOR )));

		UI_DrawString( a->generic.x, a->generic.y, a->generic.width, a->generic.height, a->generic.name, color, false, a->generic.charWidth, a->generic.charHeight, justify, shadow );
	}

	if( a->generic.flags & QMF_FOCUSBEHIND )
		UI_DrawString( a->generic.x, a->generic.y, a->generic.width, a->generic.height, a->generic.name, a->generic.color, false, a->generic.charWidth, a->generic.charHeight, justify, shadow );
}

/*
=================
UI_Bitmap_Init
=================
*/
void UI_Bitmap_Init( menuBitmap_s *b )
{
	if( !b->generic.name ) b->generic.name = "";
	if( !b->focusPic ) b->focusPic = b->pic;
	if( !b->generic.color ) b->generic.color = uiColorWhite;

	UI_ScaleCoords( &b->generic.x, &b->generic.y, &b->generic.width, &b->generic.height );
}

/*
=================
UI_Bitmap_Key
=================
*/
const char *UI_Bitmap_Key( menuBitmap_s *b, int key, int down )
{
	const char	*sound = 0;

	switch( key )
	{
	case K_MOUSE1:
		if(!( b->generic.flags & QMF_HASMOUSEFOCUS ))
			break;
		sound = uiSoundLaunch;
		break;
	case K_ENTER:
	case K_KP_ENTER:
		if( !down ) return sound;
		if( b->generic.flags & QMF_MOUSEONLY )
			break;
		sound = uiSoundLaunch;
		break;
	}
	if( sound && ( b->generic.flags & QMF_SILENT ))
		sound = uiSoundNull;

	if( b->generic.flags & QMF_ACT_ONRELEASE )
	{
		if( sound && b->generic.callback )
		{
			int	event;

			if( down )
			{
				event = QM_PRESSED;
				b->generic.bPressed = true;
			}
			else event = QM_ACTIVATED;
			b->generic.callback( b, event );
		}
	}
	else if( down )
	{
		if( sound && b->generic.callback )
			b->generic.callback( b, QM_ACTIVATED );
          }

	return sound;
}

/*
=================
UI_Bitmap_Draw
=================
*/
void UI_Bitmap_Draw( menuBitmap_s *b )
{
	if( b->generic.id == ID_BACKGROUND )	// background is always 0!
	{
		if( CVAR_GET_FLOAT( "cl_background" ))
			return;	// has background map disable images

		// UGLY HACK for replace all backgrounds
		UI_DrawBackground_Callback( b );
		return;
	}

	//CR
	if( b->generic.id == 1 )
	{
		// don't draw banners until transition is done
#ifdef TA_ALT_MODE
		if( UI_GetTitleTransFraction() != 10 ) return;
#else
		if( UI_GetTitleTransFraction() < 1.0f ) return;
#endif
	}

	if( b->generic.flags & QMF_GRAYED )
	{
		UI_DrawPic( b->generic.x, b->generic.y, b->generic.width, b->generic.height, uiColorDkGrey, b->pic );
		return; // grayed
	}

	if(( b->generic.flags & QMF_MOUSEONLY ) && !( b->generic.flags & QMF_HASMOUSEFOCUS ))
	{
		UI_DrawPic( b->generic.x, b->generic.y, b->generic.width, b->generic.height, b->generic.color, b->pic );
		return; // no focus
	}
	
	if((menuCommon_s *)b != (menuCommon_s *)UI_ItemAtCursor( b->generic.parent ))
	{
		// UNDONE: only inactive bitmaps supported
		if( b->generic.flags & QMF_DRAW_ADDITIVE )
			UI_DrawPicAdditive( b->generic.x, b->generic.y, b->generic.width, b->generic.height, b->generic.color, b->pic );
		else UI_DrawPic( b->generic.x, b->generic.y, b->generic.width, b->generic.height, b->generic.color, b->pic );
		return; // no focus
	}

	if(!( b->generic.flags & QMF_FOCUSBEHIND ))
		UI_DrawPic( b->generic.x, b->generic.y, b->generic.width, b->generic.height, b->generic.color, b->pic );
	if( b->generic.flags & QMF_HIGHLIGHTIFFOCUS )
		UI_DrawPic( b->generic.x, b->generic.y, b->generic.width, b->generic.height, b->generic.color, b->focusPic );
	else if( b->generic.flags & QMF_PULSEIFFOCUS )
	{
		int	color;

		color = PackAlpha( b->generic.color, 255 * (0.5 + 0.5 * sin( uiStatic.realTime / UI_PULSE_DIVISOR )));
		UI_DrawPic( b->generic.x, b->generic.y, b->generic.width, b->generic.height, color, b->focusPic );
	}

	if( b->generic.flags & QMF_FOCUSBEHIND )
		UI_DrawPic( b->generic.x, b->generic.y, b->generic.width, b->generic.height, b->generic.color, b->pic );
}

/*
=================
UI_PicButton_Init
=================
*/
void UI_PicButton_Init( menuPicButton_s *pb )
{
	if( !pb->generic.name ) pb->generic.name = "";

	if( pb->generic.flags & QMF_BIGFONT )
	{
		pb->generic.charWidth = UI_BIG_CHAR_WIDTH;
		pb->generic.charHeight = UI_BIG_CHAR_HEIGHT;
	}
	else if( pb->generic.flags & QMF_SMALLFONT )
	{
		pb->generic.charWidth = UI_SMALL_CHAR_WIDTH;
		pb->generic.charHeight = UI_SMALL_CHAR_HEIGHT;
	}
	else
	{
		if( pb->generic.charWidth < 1 ) pb->generic.charWidth = UI_MED_CHAR_WIDTH;
		if( pb->generic.charHeight < 1 ) pb->generic.charHeight = UI_MED_CHAR_HEIGHT;
	}

	if(!( pb->generic.flags & ( QMF_LEFT_JUSTIFY|QMF_CENTER_JUSTIFY|QMF_RIGHT_JUSTIFY )))
		pb->generic.flags |= QMF_LEFT_JUSTIFY;

	if( !pb->generic.color ) pb->generic.color = uiPromptTextColor;
	if( !pb->generic.focusColor ) pb->generic.focusColor = uiPromptFocusColor;

	if( pb->generic.width < 1 || pb->generic.height < 1 )
	{
		if( pb->generic.width < 1 )
			pb->generic.width = pb->generic.charWidth * strlen( pb->generic.name );

		if( pb->generic.height < 1 )
			pb->generic.height = pb->generic.charHeight * 1.5;
	}

	UI_ScaleCoords( &pb->generic.x, &pb->generic.y, &pb->generic.width, &pb->generic.height );
	UI_ScaleCoords( NULL, NULL, &pb->generic.charWidth, &pb->generic.charHeight );
}

/*
=================
UI_PicButton_Key
=================
*/
const char *UI_PicButton_Key( menuPicButton_s *b, int key, int down )
{
	const char	*sound = 0;

	switch( key )
	{
	case K_MOUSE1:
		if(!( b->generic.flags & QMF_HASMOUSEFOCUS ))
			break;
		sound = uiSoundLaunch;
		break;
	case K_ENTER:
	case K_KP_ENTER:
		if( b->generic.flags & QMF_MOUSEONLY )
			break;
		sound = uiSoundLaunch;
		break;
	}
	if( sound && ( b->generic.flags & QMF_SILENT ))
		sound = uiSoundNull;

	if( b->generic.flags & QMF_ACT_ONRELEASE )
	{
		if( sound && b->generic.callback )
		{
			int	event;
			
			if( down ) 
			{
				event = QM_PRESSED;
				b->generic.bPressed = true;
			}
			else event = QM_ACTIVATED;
			//CR
			UI_TACheckMenuDepth();
			b->generic.callback( b, event );
			UI_SetTitleAnim( AS_TO_TITLE, b );
		}
	}
	else if( down )
	{
		if( sound && b->generic.callback )
			b->generic.callback( b, QM_ACTIVATED );
          }

	return sound;
}

/*
=================
UI_PicButton_Draw
=================
*/
void UI_PicButton_Draw( menuPicButton_s *item )
{
	int state = BUTTON_NOFOCUS;

	if( item->generic.flags & QMF_HASMOUSEFOCUS )
		state = BUTTON_FOCUS;

	// make sure what cursor in rect
 	if( item->generic.bPressed )
 		state = BUTTON_PRESSED;

	if( item->generic.statusText && item->generic.flags & QMF_NOTIFY )
	{
		int	charW, charH;
		int	x, w;
		
		charW = UI_SMALL_CHAR_WIDTH;
		charH = UI_SMALL_CHAR_HEIGHT;
		
		UI_ScaleCoords( NULL, NULL, &charW, &charH );
		
		x = 290;
		w = UI_SMALL_CHAR_WIDTH * strlen( item->generic.statusText );
		UI_ScaleCoords( &x, NULL, &w, NULL );
		x += item->generic.x;
		
		int	r, g, b;
		
		UnpackRGB( r, g, b, uiColorHelp );
		TextMessageSetColor( r, g, b );
		DrawConsoleString( x, item->generic.y, item->generic.statusText );
	}

	if( item->pic )
	{
		int r, g, b, a;

		UnpackRGB( r, g, b, item->generic.flags & QMF_GRAYED ? uiColorDkGrey : uiColorWhite );

		wrect_t rects[]=
		{
		{ 0, uiStatic.buttons_width, 0, 26 },
		{ 0, uiStatic.buttons_width, 26, 52 },
		{ 0, uiStatic.buttons_width, 52, 78 }
		};

		PIC_Set( item->pic, r, g, b, 255 );
		PIC_DrawAdditive( item->generic.x, item->generic.y, uiStatic.buttons_draw_width, uiStatic.buttons_draw_height, &rects[state] );

		a = (512 - (uiStatic.realTime - item->generic.lastFocusTime)) >> 1;

		if( state == BUTTON_NOFOCUS && a > 0 )
		{	
			PIC_Set( item->pic, r, g, b, a );
			PIC_DrawAdditive( item->generic.x, item->generic.y, uiStatic.buttons_draw_width, uiStatic.buttons_draw_height, &rects[BUTTON_FOCUS] );
		}
	}
	else
	{
		int	justify;
		int	shadow;
		
		if( item->generic.flags & QMF_LEFT_JUSTIFY )
			justify = 0;
		else if( item->generic.flags & QMF_CENTER_JUSTIFY )
			justify = 1;
		else if( item->generic.flags & QMF_RIGHT_JUSTIFY )
			justify = 2;
		
		shadow = (item->generic.flags & QMF_DROPSHADOW);

		if( item->generic.flags & QMF_GRAYED )
		{
			UI_DrawString( item->generic.x, item->generic.y, item->generic.width, item->generic.height, item->generic.name, uiColorDkGrey, true, item->generic.charWidth, item->generic.charHeight, justify, shadow );
			return; // grayed
		}
		
		if((menuCommon_s *)item != (menuCommon_s *)UI_ItemAtCursor( item->generic.parent ))
		{
			UI_DrawString( item->generic.x, item->generic.y, item->generic.width, item->generic.height, item->generic.name, item->generic.color, false, item->generic.charWidth, item->generic.charHeight, justify, shadow );
			return; // no focus
		}
		
		if(!( item->generic.flags & QMF_FOCUSBEHIND ))
			UI_DrawString( item->generic.x, item->generic.y, item->generic.width, item->generic.height, item->generic.name, item->generic.color, false, item->generic.charWidth, item->generic.charHeight, justify, shadow );
		
		if( item->generic.flags & QMF_HIGHLIGHTIFFOCUS )
			UI_DrawString( item->generic.x, item->generic.y, item->generic.width, item->generic.height, item->generic.name, item->generic.focusColor, false, item->generic.charWidth, item->generic.charHeight, justify, shadow );
		else if( item->generic.flags & QMF_PULSEIFFOCUS )
		{
			int	color;
			
			color = PackAlpha( item->generic.color, 255 * (0.5 + 0.5 * sin( uiStatic.realTime / UI_PULSE_DIVISOR )));
			
			UI_DrawString( item->generic.x, item->generic.y, item->generic.width, item->generic.height, item->generic.name, color, false, item->generic.charWidth, item->generic.charHeight, justify, shadow );
		}
		
		if( item->generic.flags & QMF_FOCUSBEHIND )
			UI_DrawString( item->generic.x, item->generic.y, item->generic.width, item->generic.height, item->generic.name, item->generic.color, false, item->generic.charWidth, item->generic.charHeight, justify, shadow );
	}
}