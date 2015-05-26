#ifdef __ANDROID__
#include "android-gameif.h"

#include "port.h"

extern "C"
{
#include "common.h"
#include "client.h"
#include "input.h"
}

#include "MultitouchMouse.h"

#include "SDL.h"
#include "SDL_keycode.h"


#include <android/log.h>
#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO,"JNI", __VA_ARGS__))
//#define LOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN, "JNI", __VA_ARGS__))
//#define LOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR,"JNI", __VA_ARGS__))



// FIFO STUFF ////////////////////
// Copied from FTEQW, I don't know if this is thread safe, but it's safe enough for a game :)
#define COMMAND 0x123

#define EVENTQUEUELENGTH 128
struct eventlist_s
{

	int action;
	int x,y;
	std::string command;

} eventlist[EVENTQUEUELENGTH];

volatile int events_avail; /*volatile to make sure the cc doesn't try leaving these cached in a register*/
volatile int events_used;

static struct eventlist_s *in_newevent(void)
{
	if (events_avail >= events_used + EVENTQUEUELENGTH)
		return 0;
	return &eventlist[events_avail & (EVENTQUEUELENGTH-1)];
}

static void in_finishevent(void)
{
	events_avail++;
}
///////////////////////

extern "C"
{
extern int SDL_SendKeyboardKey(Uint8 state, SDL_Scancode scancode);
}

int PortableKeyEvent(int state, int code, int unicode){
	//NOT USED
	LOGI("PortableKeyEvent %d %d %d",state,code,unicode);

	if (state)
		SDL_SendKeyboardKey(SDL_PRESSED, (SDL_Scancode)code);
	else
		SDL_SendKeyboardKey(SDL_RELEASED, (SDL_Scancode) code);

	return 0;
}


extern "C" void UI_MouseMove( int x, int y );

void PortableMenuMouse(int action,float x,float y){

	struct eventlist_s *ev = in_newevent();

	if (!ev)
		return; //Queue full

	ev->action = action;
	ev->x = x;
	ev->y = y;
	in_finishevent();
}



static void PostCommand(const char * cmd)
{
	struct eventlist_s *ev = in_newevent();
	if (!ev)
		return;

	ev->action = COMMAND;
	ev->command = cmd;
	in_finishevent();
}

//TODO ASAP, This needs to changed to a threadsafe fifo, this will probably
//cause random crashes!
static void DoCommand(int state,const char * cmd)
{
	char cmdfull[50];

	if (state)
		sprintf(cmdfull,"+%s",cmd);
	else
		sprintf(cmdfull,"-%s",cmd);

	PostCommand(cmdfull);
}
void PortableAction(int state, int action)
{
	LOGI("PortableAction %d   %d",state,action);
	switch (action)
	{
	case PORT_ACT_USE:
		DoCommand(state,"use");
		break;
	case PORT_ACT_JUMP:
		DoCommand(state,"jump");
		break;
	case PORT_ACT_DOWN:
		DoCommand(state,"duck");
		break;
	case PORT_ACT_ATTACK:
		DoCommand(state,"attack");
		break;
	case PORT_ACT_ATTACK_ALT:
		DoCommand(state,"attack2");
		break;
	case PORT_ACT_RELOAD:
		DoCommand(state,"reload");
		break;
	case PORT_ACT_LIGHT:
		if (state)
			PostCommand("impulse 100");
		break;
	case PORT_ACT_QUICKSAVE:
		if (state)
			PostCommand("savequick");
		break;
	case PORT_ACT_QUICKLOAD:
		if (state)
			PostCommand("loadquick");
		break;
	case PORT_ACT_NEXT_WEP:
		if (state)
			PostCommand("invprev");
		break;
	case PORT_ACT_PREV_WEP:
		if (state)
			PostCommand("invnext");
		break;
	}
}

int mdx=0,mdy=0;
void PortableMouse(float dx,float dy)
{
	dx *= 1500;
	dy *= 1200;

	mdx += dx;
	mdy += dy;
}

int absx=0,absy=0;
void PortableMouseAbs(float x,float y)
{
	absx = x;
	absy = y;
}


// =================== FORWARD and SIDE MOVMENT ==============

float forwardmove, sidemove; //Joystick mode

void PortableMoveFwd(float fwd)
{
	if (fwd > 1)
		fwd = 1;
	else if (fwd < -1)
		fwd = -1;

	forwardmove = fwd;
}

void PortableMoveSide(float strafe)
{
	if (strafe > 1)
		strafe = 1;
	else if (strafe < -1)
		strafe = -1;

	sidemove = strafe;
}

void PortableMove(float fwd, float strafe)
{
	PortableMoveFwd(fwd);
	PortableMoveSide(strafe);
}

//======================================================================

//Look up and down
int look_pitch_mode;
float look_pitch_mouse,look_pitch_abs,look_pitch_joy;
void PortableLookPitch(int mode, float pitch)
{
	look_pitch_mode = mode;
	switch(mode)
	{
	case LOOK_MODE_MOUSE:
		look_pitch_mouse += pitch;
		break;
	case LOOK_MODE_ABSOLUTE:
		look_pitch_abs = pitch;
		break;
	case LOOK_MODE_JOYSTICK:
		look_pitch_joy = pitch;
		break;
	}
}

//left right
int look_yaw_mode;
float look_yaw_mouse,look_yaw_joy;
void PortableLookYaw(int mode, float yaw)
{
	look_yaw_mode = mode;
	switch(mode)
	{
	case LOOK_MODE_MOUSE:
		look_yaw_mouse += yaw;
		break;
	case LOOK_MODE_JOYSTICK:
		look_yaw_joy = yaw;
		break;
	}
}



typedef void (*pfnChangeGame)( const char *progname );
extern "C" int EXPORT Host_Main( int argc, const char **argv, const char *progname, int bChangeGame, pfnChangeGame func );

#define GAME_PATH	"valve"	// default dir to start from
void PortableInit(int argc,const char ** argv){
	Host_Main(argc, argv, GAME_PATH, false, NULL );
}



int PortableInMenu(void)
{
	// Don't draw controls, when you are not in game
	return cls.key_dest != key_game ? 1: 0;
}

// CALLED FROM GAME//////////////////////


extern "C" void AndroidEvents()
{
	if (events_used != events_avail)
	{
		struct eventlist_s *ev = &eventlist[events_used & (EVENTQUEUELENGTH-1)];

		LOGI("Queue event");
		if (ev->action == COMMAND)
		{
			Cmd_ExecuteString((char*)ev->command.c_str(),src_command);
		}
		else if (ev->action == MULTITOUCHMOUSE_MOVE)
		{
			UI_MouseMove( ev->x,  ev->y );
		}
		else if (ev->action == MULTITOUCHMOUSE_DOWN)
		{
			UI_MouseMove( ev->x,  ev->y );
			Key_Event( K_MOUSE1, true );
		}
		else if (ev->action == MULTITOUCHMOUSE_UP)
		{
			UI_MouseMove( ev->x,  ev->y );
			Key_Event( K_MOUSE1, false );
		}

		events_used++;
	}
}

static bool forwardHackActive = false;
static bool backwardHackActive = false;
extern cvar_t *cl_sidespeed;
extern cvar_t *cl_forwardspeed;

void IN_MobileMove ( float frametime, usercmd_t *cmd)
{

	vec3_t viewangles;

	//gEngfuncs.GetViewAngles( (float *)viewangles );

	//cmd->forwardmove  += forwardmove * cl_forwardspeed->value;
	//cmd->sidemove  += sidemove   * cl_sidespeed->value;

	cmd->forwardmove  += forwardmove * 300;
	cmd->sidemove  += sidemove   * 250;

	if (forwardmove > 0.9 && !forwardHackActive)
	{
		//LOGI("Fwd hack on");
		Cmd_ExecuteString("+forward",src_command);
		forwardHackActive = true;
	}
	else if (forwardmove < 0.8 && forwardHackActive)
	{
		//LOGI("Fwd hack off");
		Cmd_ExecuteString("-forward",src_command);
		forwardHackActive = false;
	}
	else if (forwardmove < -0.9 && !backwardHackActive)
	{
		Cmd_ExecuteString("+back",src_command);
		backwardHackActive = true;
	}
	else if (forwardmove > -0.8 && backwardHackActive)
	{
		Cmd_ExecuteString("-back",src_command);
		backwardHackActive = false;
	}
}


void IN_MobileAngles(float *viewangles)
{
	switch(look_pitch_mode)
	{
	case LOOK_MODE_MOUSE:
		viewangles[0] += look_pitch_mouse * 200;
		look_pitch_mouse = 0;
		break;
	case LOOK_MODE_ABSOLUTE:
		viewangles[0] = look_pitch_abs * 80;
		break;
	case LOOK_MODE_JOYSTICK:
		viewangles[0] += look_pitch_joy * 6;
		break;
	}


	switch(look_yaw_mode)
	{
	case LOOK_MODE_MOUSE:
		viewangles[1] += look_yaw_mouse * 300;
		look_yaw_mouse = 0;
		break;
	case LOOK_MODE_JOYSTICK:
		viewangles[1] += look_yaw_joy * 6;
		break;
	}
}
#endif
