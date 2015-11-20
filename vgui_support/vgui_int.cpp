/*
vgui_int.c - vgui dll interaction
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
#include "vgui_main.h"

vguiapi_t *g_api;

FontCache *g_FontCache = 0;

CEnginePanel	*rootpanel = NULL;
CEngineSurface	*surface = NULL;
CEngineApp          *pApp = NULL;

SurfaceBase* CEnginePanel::getSurfaceBase( void )
{
	return surface;
}

App* CEnginePanel::getApp( void )
{
	return pApp;
}

void CEngineApp :: setCursorPos( int x, int y )
{
#ifdef XASH_SDL
	//SDL_WarpMouseInWindow(host.hWnd, x, y);
#endif
}

void CEngineApp :: getCursorPos( int &x,int &y )
{
	g_api->GetCursorPos( &x, &y );
}

void CEnginePanel :: setVisible(bool state)
{
	g_api->SetVisible(state);
}


void VGui_Startup( int width, int height )
{
	if(!g_FontCache)
		g_FontCache = new FontCache();

	if( rootpanel )
	{
		rootpanel->setSize( width, height );
		return;
	}

	rootpanel = new CEnginePanel;
	rootpanel->setSize( width, height );
	rootpanel->setPaintBorderEnabled( false );
	rootpanel->setPaintBackgroundEnabled( false );
	rootpanel->setVisible( true );
	rootpanel->setCursor( new Cursor( Cursor::dc_none ));

	pApp = new CEngineApp;
	pApp->start();
	pApp->setMinimumTickMillisInterval( 0 );

	surface = new CEngineSurface( rootpanel );
	rootpanel->setSurfaceBaseTraverse( surface );


	//ASSERT( rootpanel->getApp() != NULL );
	//ASSERT( rootpanel->getSurfaceBase() != NULL );

	g_api->DrawInit ();
}

void VGui_Shutdown( void )
{
	if( pApp ) pApp->stop();

	delete rootpanel;
	delete surface;
	delete pApp;

	rootpanel = NULL;
	surface = NULL;
	pApp = NULL;
}

void VGui_Paint( void )
{
	int w, h;

	//if( cls.state != ca_active || !rootpanel )
	//	return;
	if( !g_api->IsInGame() || !rootpanel )
		return;

	// setup the base panel to cover the screen
	Panel *pVPanel = surface->getEmbeddedPanel();
	if( !pVPanel ) return;
	//SDL_GetWindowSize(host.hWnd, &w, &h);
	//host.input_enabled = rootpanel->isVisible();
	rootpanel->getSize(w, h);
	EnableScissor( true );

	pApp->externalTick ();

	pVPanel->setBounds( 0, 0, w, h );
	pVPanel->repaint();

	// paint everything 
	pVPanel->paintTraverse();

	EnableScissor( false );
}
void *VGui_GetPanel( void )
{
	return (void *)rootpanel;
}


#ifdef _WIN32
extern "C" void _declspec( dllexport ) InitAPI(vguiapi_t * api)
#else
extern "C" void InitAPI(vguiapi_t * api)
#endif
{
	g_api = api;
	g_api->Startup = VGui_Startup;
	g_api->Shutdown = VGui_Shutdown;
	g_api->GetPanel = VGui_GetPanel;
	g_api->Paint = VGui_Paint;
	g_api->Mouse = VGUI_Mouse;
	g_api->MouseMove = VGUI_MouseMove;
	g_api->Key = VGUI_Key;
}
#endif
