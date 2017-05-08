#include "extdll.h"
#include "basemenu.h"
#include "utils.h"
#include "ui_title_anim.h"

#define BANNER_X_FIX	-16
#define BANNER_Y_FIX	-20

// Title Transition Time period
#define TTT_PERIOD		200.0f

ui_quad_t TitleLerpQuads[2];
int transition_initial_time;
int transition_state;
HIMAGE TransPic = 0;
int PreClickDepth;
bool hold_button_stack = false;

void UI_TACheckMenuDepth( void )
{
	PreClickDepth = uiStatic.menuDepth;
}

menuPicButton_s *ButtonStack[UI_MAX_MENUDEPTH];
int ButtonStackDepth;

void UI_PopPButtonStack()
{
	if( hold_button_stack ) return;

	if ( ButtonStack[ButtonStackDepth] ) UI_SetTitleAnim( AS_TO_BUTTON, ButtonStack[ButtonStackDepth] );
	if ( ButtonStackDepth ) ButtonStackDepth--;
}

void UI_PushPButtonStack( menuPicButton_s *button )
{
	if( ButtonStack[ButtonStackDepth] == button )
		return;

	ButtonStackDepth++;
	ButtonStack[ButtonStackDepth] = button;
}

float UI_GetTitleTransFraction( void )
{
	float fraction = (float)(uiStatic.realTime - transition_initial_time ) / TTT_PERIOD;

	if( fraction > 1.0f )
		fraction = 1.0f;

	return fraction;
}

void LerpQuad( ui_quad_t a, ui_quad_t b, float frac, ui_quad_t *c )
{
	c->x = a.x + (b.x - a.x) * frac;
	c->y = a.y + (b.y - a.y) * frac;
	c->lx = a.lx + (b.lx - a.lx) * frac;
	c->ly = a.ly + (b.ly - a.ly) * frac;
}

void UI_SetupTitleQuad()
{
	TitleLerpQuads[1].x  = UI_BANNER_POSX * ScreenHeight / 768;
	TitleLerpQuads[1].y  = UI_BANNER_POSY * ScreenHeight / 768;
	TitleLerpQuads[1].lx = UI_BANNER_WIDTH * ScreenHeight / 768;
	TitleLerpQuads[1].ly = UI_BANNER_HEIGHT * ScreenHeight / 768;
}

void UI_DrawTitleAnim()
{
	UI_SetupTitleQuad();

	if( !TransPic ) return;

	wrect_t r = { 0, uiStatic.buttons_width, 26, 51 };

	float frac = UI_GetTitleTransFraction();/*(sin(gpGlobals->time*4)+1)/2*/;

#ifdef TA_ALT_MODE
	if( frac == 1 && transition_state == AS_TO_BUTTON )
		return;
#else
	if( frac == 1 ) return;
#endif

	ui_quad_t c;
	
	int f_idx = (transition_state == AS_TO_TITLE) ? 0 : 1;
	int s_idx = (transition_state == AS_TO_TITLE) ? 1 : 0;

	LerpQuad( TitleLerpQuads[f_idx], TitleLerpQuads[s_idx], frac, &c );

	PIC_Set( TransPic, 255, 255, 255, 255 );
	PIC_DrawAdditive( c.x, c.y, c.lx, c.ly, &r );
}

void UI_SetTitleAnim( int anim_state, menuPicButton_s *button )
{
	// skip buttons which don't call new menu
	if( !button || ( PreClickDepth == uiStatic.menuDepth && anim_state == AS_TO_TITLE ) )
		return;

	// replace cancel\done button with button which called this menu 
	if( PreClickDepth > uiStatic.menuDepth && anim_state == AS_TO_TITLE ) 
	{
		anim_state = AS_TO_BUTTON;

		// HACK HACK HACK
		if ( ButtonStack[ButtonStackDepth + 1] )
			button = ButtonStack[ButtonStackDepth+1];
	}	

	// don't reset anim if dialog buttons pressed
	if( button->generic.id == ID_YES || button->generic.id == ID_NO )
		return;

	if( anim_state == AS_TO_TITLE )
		UI_PushPButtonStack( button );

	transition_state = anim_state;

	TitleLerpQuads[0].x = button->generic.x;
	TitleLerpQuads[0].y = button->generic.y;
	TitleLerpQuads[0].lx = button->generic.width;
	TitleLerpQuads[0].ly = button->generic.height;
	
	transition_initial_time = uiStatic.realTime;
	TransPic = button->pic;
}

void UI_InitTitleAnim()
{
	memset( TitleLerpQuads, 0, sizeof( ui_quad_t ) * 2 );

	UI_SetupTitleQuad();

	ButtonStackDepth = 0;
	memset( ButtonStack, 0, sizeof( ButtonStack ));
}

void UI_ClearButtonStack( void )
{
	ButtonStackDepth = 0;
	memset( ButtonStack, 0, sizeof( ButtonStack ));
}
