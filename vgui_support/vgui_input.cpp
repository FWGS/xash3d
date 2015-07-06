/*
vgui_input.cpp - handle kb & mouse
Copyright (C) 2011 Uncle Mike

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/
#ifdef XASH_VGUI
#define OEMRESOURCE		// for OCR_* cursor junk


#include "vgui_main.h"
#include "input.h"

static KeyCode s_pVirtualKeyTrans[256];
#ifdef XASH_SDL
static SDL_Cursor* s_pDefaultCursor[20];
#endif

void VGUI_Key(VGUI_KeyAction action, VGUI_KeyCode code)
{
	if(!surface)
		return;
	switch( action )
	{
		case KA_PRESSED:
			pApp->internalKeyPressed( (KeyCode) code, surface );
			break;
		case KA_RELEASED:
			pApp->internalKeyReleased( (KeyCode) code, surface );
			break;
		case KA_TYPED:
			pApp->internalKeyTyped( (KeyCode) code, surface );
			break;
	}
	//fprintf(stdout,"vgui_support: VGUI key action %d %d\n", action, code);
	//fflush(stdout);
}

void VGUI_Mouse(VGUI_MouseAction action, int code)
{
	if(!surface)
		return;
	switch( action )
	{
		case MA_PRESSED:
			pApp->internalMousePressed( (MouseCode) code, surface );
			break;
		case MA_RELEASED:
			pApp->internalMouseReleased( (MouseCode) code, surface );
			break;
		case MA_DOUBLE:
			pApp->internalMouseDoublePressed( (MouseCode) code, surface );
			break;
		case MA_WHEEL:
			//fprintf(stdout, "vgui_support: VGUI mouse wheeled %d %d\n", action, code);
			pApp->internalMouseWheeled( code*100, surface );
			break;
	}
	//fprintf(stdout, "vgui_support: VGUI mouse action %d %d\n", action, code);	
	//fflush(stdout);
}

void VGUI_MouseMove(int x, int y)
{
	//fprintf(stdout, "vgui_support: VGUI mouse move %d %d %p\n", x, y, surface);
	//fflush(stdout);
	if(!surface)
		return;
	pApp->internalCursorMoved( x, y, surface );
}
#endif
