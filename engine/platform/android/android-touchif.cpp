/*
 * Copyright (C) 2009 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */
#ifdef __ANDROID__
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <jni.h>
#include <android/log.h>
#include <unistd.h>


#include "TouchControlsContainer.h"
#include "JNITouchControlsUtils.h"


extern "C"
{
#include "wrect.h"
#include "mobility_int.h"
#include "android-gameif.h"
#include "common.h"
#include "nanogl.h"

#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO,"JNI", __VA_ARGS__))
#define LOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN, "JNI", __VA_ARGS__))
#define LOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR,"JNI", __VA_ARGS__))

#define JAVA_FUNC(x) Java_com_beloko_games_hl_NativeLib_##x

int android_screen_width;
int android_screen_height;


#define KEY_SHOW_WEAPONS 0x1000
#define KEY_SHOOT        0x1001

#define KEY_SHOW_INV     0x1006
#define KEY_QUICK_CMD    0x1007

#define KEY_SHOW_KEYB    0x1009

float gameControlsAlpha = 0.5;
bool showWeaponCycle = false;
bool turnMouseMode = true;
bool invertLook = false;
bool precisionShoot = false;
bool showSticks = true;
bool hideTouchControls = true;
bool enableWeaponWheel = true;

bool shooting = false;

//set when holding down reload
bool sniperMode = false;

static int controlsCreated = 0;
touchcontrols::TouchControlsContainer controlsContainer;

touchcontrols::TouchControls *tcMenuMain=0;
touchcontrols::TouchControls *tcGameMain=0;
touchcontrols::TouchControls *tcGameWeapons=0;
touchcontrols::TouchControls *tcWeaponWheel=0;

int ScaleX, ScaleY;

//So can hide and show these buttons
touchcontrols::Button *nextWeapon=0;
touchcontrols::Button *prevWeapon=0;
touchcontrols::TouchJoy *touchJoyLeft;
touchcontrols::TouchJoy *touchJoyRight;

extern JNIEnv* env_;

GLint     matrixMode;
GLfloat   projection[16];
GLfloat   model[16];
void glOrtho (GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat zNear, GLfloat zFar);

void openGLStart()
{

	glGetIntegerv(GL_MATRIX_MODE, &matrixMode);
	glGetFloatv(GL_PROJECTION_MATRIX, projection);
	glGetFloatv(GL_MODELVIEW_MATRIX, model);

	//glClearColor(1.0f, 1.0f, 0.0f, 1.0f);
	//glClear(GL_COLOR_BUFFER_BIT);
	//LOGI("openGLStart");
	glMatrixMode(GL_PROJECTION);

	glLoadIdentity();

	glViewport(0, 0, android_screen_width, android_screen_height);
	glOrtho(0.0f, (float)android_screen_width,  (float)android_screen_height, 0.0f, -1.0f, 1.0f);

	glMatrixMode(GL_MODELVIEW);

	glLoadIdentity();

	glDisable(GL_ALPHA_TEST);
	glDisableClientState(GL_COLOR_ARRAY);
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY );

	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glEnable (GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_TEXTURE_2D);
	glDisable(GL_CULL_FACE);
	glMatrixMode(GL_MODELVIEW);

}

void openGLEnd()
{
	nanoGL_Reset();

	glMatrixMode(GL_MODELVIEW);
	glLoadMatrixf(model);

	glMatrixMode(GL_PROJECTION);
	glLoadMatrixf(projection);


	if (matrixMode == GL_MODELVIEW)
	{
		glMatrixMode(GL_MODELVIEW);
	}
	else if (matrixMode == GL_TEXTURE)
	{
		glMatrixMode(GL_TEXTURE);
	}
}

void gameSettingsButton(int state)
{
	//LOGTOUCH("gameSettingsButton %d",state);
	if (state == 1)
	{
		showTouchSettings();
	}
}


static jclass NativeLibClass = 0;
static jmethodID swapBuffersMethod = 0;


//extern unsigned int Sys_Milliseconds(void);

static unsigned int reload_time_down;
void gameButton(int state,int code)
{
	if (code == TOUCH_ACT_SHOW_NUMBERS)
	{
		if( state == 1 && !tcGameWeapons->enabled)
		{
			tcGameWeapons->animateIn(5);
		}
		return;
	}

	if (code == TOUCH_ACT_SHOOT)
		shooting = state;

	PortableAction(state, code);
}


//Weapon wheel callbacks
void weaponWheelSelected(int enabled)
{
	if (enabled)
		tcWeaponWheel->fade(touchcontrols::FADE_IN,5); //fade in
}

void menuMouse(int action,float x, float y,float dx, float dy)
{
	PortableMenuMouse(action,x * android_screen_width,y*android_screen_height);
}

void weaponWheel(int segment)
{
	LOGI("weaponWheel %d",segment);
	int code;
	if (segment == 9)
		code = '0';
	else
		code = '1' + segment;
	PortableKeyEvent(1,code,0);
	PortableKeyEvent(0, code,0);
}

void menuButton(int state,int code)
{
	if (code == KEY_SHOW_KEYB)
	{
		if (state)
			toggleKeyboard();
		return;
	}
	PortableKeyEvent(state, code, 0);
}



int left_double_action;
int right_double_action;

void left_double_tap(int state)
{
	//LOGTOUCH("L double %d",state);
	if (left_double_action)
		PortableAction(state,left_double_action);
}

void right_double_tap(int state)
{
	//LOGTOUCH("R double %d",state);
	if (right_double_action)
		PortableAction(state,right_double_action);
}



//To be set by android
float strafe_sens,forward_sens;
float pitch_sens,yaw_sens;

void left_stick(float joy_x, float joy_y,float mouse_x, float mouse_y)
{
	joy_x *=10;
	float strafe = joy_x;

	PortableMove(joy_y * 15 * forward_sens,-strafe * strafe_sens);
}
void right_stick(float joy_x, float joy_y,float mouse_x, float mouse_y)
{
	//LOGI(" mouse x = %f",mouse_x);
	int invert = invertLook?-1:1;

	float scale;

	if (sniperMode)
		scale = 0.1;
	else
		scale = (shooting && precisionShoot)?0.3:1;

	PortableLookPitch(LOOK_MODE_MOUSE,-mouse_y  * pitch_sens * invert * scale);

	if (turnMouseMode)
		PortableLookYaw(LOOK_MODE_MOUSE,mouse_x*2*yaw_sens * scale);
	else
		PortableLookYaw(LOOK_MODE_JOYSTICK,joy_x*6*yaw_sens * scale);

}

//Weapon select callbacks
void selectWeaponButton(int state, int code)
{
	PortableKeyEvent(state, code, 0);
	if (state == 0)
		tcGameWeapons->animateOut(5);
}

void weaponCycle(bool v)
{
	if (v)
	{
		if (nextWeapon) nextWeapon->setEnabled(true);
		if (prevWeapon) prevWeapon->setEnabled(true);
	}
	else
	{
		if (nextWeapon) nextWeapon->setEnabled(false);
		if (prevWeapon) prevWeapon->setEnabled(false);
	}
}

void setHideSticks(bool v)
{
	if (touchJoyLeft) touchJoyLeft->setHideGraphics(v);
	if (touchJoyRight) touchJoyRight->setHideGraphics(v);
}


void initControls(int width, int height,const char * graphics_path,const char *settings_file)
{
	touchcontrols::GLScaleWidth = (float)width;
	touchcontrols::GLScaleHeight = (float)height;
	int X = 26;
	int Y = (int)(26.0f*(-height)/width);
	touchcontrols::ScaleX = ScaleX = X;
	touchcontrols::ScaleY = ScaleY = Y;


	LOGI("initControls %d x %d,x path = %s, settings = %s",width,height,graphics_path,settings_file);

	if (!controlsCreated)
	{
		LOGI("creating controls");

		touchcontrols::setGraphicsBasePath(graphics_path);

		setControlsContainer(&controlsContainer);

		controlsContainer.openGL_start.connect( sigc::ptr_fun(&openGLStart));
		controlsContainer.openGL_end.connect( sigc::ptr_fun(&openGLEnd));


		tcMenuMain = new touchcontrols::TouchControls("menu",true,false);
		tcGameMain = new touchcontrols::TouchControls("game",false,true);
		tcGameWeapons = new touchcontrols::TouchControls("weapons",false,false);
		tcWeaponWheel = new touchcontrols::TouchControls("weapon_wheel",false,false);

		tcGameMain->signal_settingsButton.connect(  sigc::ptr_fun(&gameSettingsButton) );
		touchcontrols::MultitouchMouse *mouseMenu = new touchcontrols::MultitouchMouse("mouse",touchcontrols::RectF(0,0,X,Y),"");
		//mouseMenu->setHideGraphics(true);
		tcMenuMain->addControl(mouseMenu);
		mouseMenu->signal_action.connect(sigc::ptr_fun(&menuMouse) );
		tcMenuMain->setPassThroughTouch(false);


		tcMenuMain->signal_button.connect(  sigc::ptr_fun(&menuButton) );

		tcMenuMain->setAlpha(0.8);


		//Game
		tcGameMain->setAlpha(gameControlsAlpha);


		tcGameMain->addControl(new touchcontrols::Button("attack",
			touchcontrols::RectF(X-6, 7, X-3, 10), "shoot", TOUCH_ACT_SHOOT));

		tcGameMain->addControl(new touchcontrols::Button("attack_alt",
			touchcontrols::RectF(X-6, 4, X-3, 7), "shoot_alt", TOUCH_ACT_SHOOT_ALT));

		tcGameMain->addControl(new touchcontrols::Button("use",
			touchcontrols::RectF(X-3, 6, X, 9),   "use", TOUCH_ACT_USE));

		tcGameMain->addControl(new touchcontrols::Button("quick_save",
			touchcontrols::RectF(X-4, 0, X-2, 2), "save", TOUCH_ACT_QUICKSAVE));

		tcGameMain->addControl(new touchcontrols::Button("quick_load",
			touchcontrols::RectF(X-6, 0, X-4, 2), "load", TOUCH_ACT_QUICKLOAD));

		tcGameMain->addControl(new touchcontrols::Button("flashlight",
			touchcontrols::RectF(X-2, 0, X, 2),   "flash_light_filled", TOUCH_ACT_LIGHT));

		tcGameMain->addControl(new touchcontrols::Button("jump",
			touchcontrols::RectF(X-3, 3, X, 6),   "jump", TOUCH_ACT_JUMP));

		tcGameMain->addControl(new touchcontrols::Button("crouch",
			touchcontrols::RectF(X-3, Y-3, X, Y), "crouch", TOUCH_ACT_CROUCH));

		tcGameMain->addControl(new touchcontrols::Button("show_weapons",
			touchcontrols::RectF(11, Y-2, 13, Y), "show_weapons", TOUCH_ACT_SHOW_NUMBERS));

		tcGameMain->addControl(new touchcontrols::Button("reload",
			touchcontrols::RectF(0, 5, 3, 8),     "reload", TOUCH_ACT_RELOAD));


		nextWeapon = new touchcontrols::Button("next_weapon",touchcontrols::RectF(0,8,3,11),"next_weap", TOUCH_ACT_INVNEXT);
		prevWeapon = new touchcontrols::Button("prev_weapon",touchcontrols::RectF(0,2,3,5),"prev_weap", TOUCH_ACT_INVPREV);

		tcGameMain->addControl(nextWeapon);
		tcGameMain->addControl(prevWeapon);


		touchJoyLeft = new touchcontrols::TouchJoy("stick",touchcontrols::RectF(0,7,8,Y),"strafe_arrow");
		tcGameMain->addControl(touchJoyLeft);
		touchJoyLeft->signal_move.connect(sigc::ptr_fun(&left_stick) );
		touchJoyLeft->signal_double_tap.connect(sigc::ptr_fun(&left_double_tap) );

		touchJoyRight = new touchcontrols::TouchJoy("touch",touchcontrols::RectF(17,4,26,Y),"look_arrow");
		tcGameMain->addControl(touchJoyRight);
		touchJoyRight->signal_move.connect(sigc::ptr_fun(&right_stick) );
		touchJoyRight->signal_double_tap.connect(sigc::ptr_fun(&right_double_tap) );

		tcGameMain->signal_button.connect(  sigc::ptr_fun(&gameButton) );

		// Weapon
		tcGameWeapons->addControl(new touchcontrols::Button("weapon1",touchcontrols::RectF(1,Y-2,3,Y),"key_1", TOUCH_ACT_1));
		tcGameWeapons->addControl(new touchcontrols::Button("weapon2",touchcontrols::RectF(3,Y-2,5,Y),"key_2", TOUCH_ACT_2));
		tcGameWeapons->addControl(new touchcontrols::Button("weapon3",touchcontrols::RectF(5,Y-2,7,Y),"key_3", TOUCH_ACT_3));
		tcGameWeapons->addControl(new touchcontrols::Button("weapon4",touchcontrols::RectF(7,Y-2,9,Y),"key_4", TOUCH_ACT_4));
		tcGameWeapons->addControl(new touchcontrols::Button("weapon5",touchcontrols::RectF(9,Y-2,11,Y),"key_5", TOUCH_ACT_5));

		tcGameWeapons->addControl(new touchcontrols::Button("weapon6",touchcontrols::RectF(15,Y-2,17,Y),"key_6", TOUCH_ACT_6));
		tcGameWeapons->addControl(new touchcontrols::Button("weapon7",touchcontrols::RectF(17,Y-2,19,Y),"key_7", TOUCH_ACT_7));
		tcGameWeapons->addControl(new touchcontrols::Button("weapon8",touchcontrols::RectF(19,Y-2,21,Y),"key_8", TOUCH_ACT_8));
		tcGameWeapons->addControl(new touchcontrols::Button("weapon9",touchcontrols::RectF(21,Y-2,23,Y),"key_9", TOUCH_ACT_9));
		tcGameWeapons->addControl(new touchcontrols::Button("weapon0",touchcontrols::RectF(23,Y-2,25,Y),"key_0", TOUCH_ACT_0));
		tcGameWeapons->signal_button.connect(  sigc::ptr_fun(&selectWeaponButton) );
		tcGameWeapons->setAlpha(0.8);

		//Weapon wheel
		touchcontrols::WheelSelect *wheel = new touchcontrols::WheelSelect("weapon_wheel",touchcontrols::RectF(7,2,19,Y-2),"weapon_wheel",10);
		wheel->signal_selected.connect(sigc::ptr_fun(&weaponWheel) );
		wheel->signal_enabled.connect(sigc::ptr_fun(&weaponWheelSelected));
		tcWeaponWheel->addControl(wheel);
		tcWeaponWheel->setAlpha(0.5);


		controlsContainer.addControlGroup(tcGameMain);
		controlsContainer.addControlGroup(tcGameWeapons);
		controlsContainer.addControlGroup(tcMenuMain);
		controlsContainer.addControlGroup(tcWeaponWheel);
		controlsCreated = 1;

#if 0
		// global settings
		tcGameMain->setXMLFile(settings_file);
#endif
	}
	else
		LOGI("NOT creating controls");
}

int inMenuLast = 1;
int inAutomapLast = 0;
static int glInit = 0;
extern "C" void Android_DrawControls()
{
	if(!controlsCreated)
		return;
	//LOGI("frameControls");
	if (!glInit)
	{
		controlsContainer.initGL();
		glInit = 1;
	}

	int inMenuNew = PortableInMenu();
	if (inMenuLast != inMenuNew)
	{
		inMenuLast = inMenuNew;
		if (!inMenuNew)
		{
			tcGameMain->setEnabled(true);
			tcGameMain->fade(touchcontrols::FADE_IN,10);
			if (enableWeaponWheel)
				tcWeaponWheel->setEnabled(true);
			//tcMenuMain->setEnabled(false);
			tcMenuMain->fade(touchcontrols::FADE_OUT,10);
		}
		else
		{

			tcGameMain->fade(touchcontrols::FADE_OUT,10);
			tcGameWeapons->setEnabled(false);
			tcWeaponWheel->setEnabled(false);
			tcMenuMain->setEnabled(true);
			tcMenuMain->fade(touchcontrols::FADE_IN,10);
		}
	}


	weaponCycle(showWeaponCycle);
	setHideSticks(!showSticks);
	
	controlsContainer.draw();
}

void Android_AddButton( touchbutton_t *button )
{
	touchcontrols::RectF rect = touchcontrols::RectF( button->sRect.left,
		button->sRect.top, button->sRect.right, button->sRect.bottom );

	touchcontrols::Button *butt = new touchcontrols::Button(button->pszCommand,
		rect,
		button->pszImageName,
		button->hButton);

	tcGameMain->addControl( butt );

	button->object = (void*)butt;
}

void Android_TouchInit( touchbutton_t *button )
{
	char custom_settings_file[MAX_SYSPATH];
	sprintf(custom_settings_file, "%s/%s/game_controls.xml", getenv( "XASH3D_BASEDIR" ), GI->gamedir);
	tcGameMain->setXMLFile(custom_settings_file);
}


void Android_TouchDisable( bool disable )
{

}


void setTouchSettings(float alpha,float strafe,float fwd,float pitch,float yaw,int other)
{

	gameControlsAlpha = alpha;
	if (tcGameMain)
		tcGameMain->setAlpha(gameControlsAlpha);

	showWeaponCycle = other & 0x1;
	turnMouseMode   = other & 0x2;
	invertLook      = other & 0x4;
	precisionShoot  = other & 0x8;
	showSticks      = other & 0x1000;
	enableWeaponWheel  = other & 0x2000;

	if (tcWeaponWheel)
		tcWeaponWheel->setEnabled(enableWeaponWheel);


	hideTouchControls = other & 0x80000000;


	switch ((other>>4) & 0xF)
	{
	case 1:
		left_double_action = TOUCH_ACT_SHOOT;
		break;
	case 2:
		left_double_action = TOUCH_ACT_JUMP;
		break;
	default:
		left_double_action = 0;
	}

	switch ((other>>8) & 0xF)
	{
	case 1:
		right_double_action = TOUCH_ACT_SHOOT;
		break;
	case 2:
		right_double_action = TOUCH_ACT_JUMP;
		break;
	default:
		right_double_action = 0;
	}

	strafe_sens = strafe;
	forward_sens = fwd;
	pitch_sens = pitch;
	yaw_sens = yaw;

}

int quit_now = 0;

#define EXPORT_ME __attribute__ ((visibility("default")))

JNIEnv* env_;

int argc=1;
const char * argv[32];
std::string graphicpath;


std::string game_path;

const char * getGamePath()
{
	return game_path.c_str();
}

std::string home_env;


extern void Android_SetGameResolution(int width, int height);

jint EXPORT_ME
JAVA_FUNC(initTouchControls) ( JNIEnv* env,	jobject thiz,jstring graphics_dir,jint width,jint height )
{
	LOGI("initTouchControls");

	env_ = env;

	const char * p = env->GetStringUTFChars(graphics_dir,NULL);
	graphicpath =  std::string(p);

	android_screen_width = width;
	android_screen_height = height;

	initControls(android_screen_width,-android_screen_height,graphicpath.c_str(),(graphicpath + "/game_controls.xml").c_str());
	return 0;
}

#ifdef SOFTFP_LINK
void EXPORT_ME
JAVA_FUNC(setTouchSettings) (JNIEnv *env, jobject obj,	jfloat alpha,jfloat strafe,jfloat fwd,jfloat pitch,jfloat yaw,int other) __attribute__((pcs("aapcs")));
void EXPORT_ME
JAVA_FUNC(touchEvent) (JNIEnv *env, jobject obj,jint action, jint pid, jfloat x, jfloat y) __attribute__((pcs("aapcs")));
#endif


jint EXPORT_ME
JAVA_FUNC(frame) ( JNIEnv* env,	jobject thiz )
{
	//NOT CALLED
}

__attribute__((visibility("default"))) jint JNI_OnLoad(JavaVM* vm, void* reserved)
{
	LOGI("JNI_OnLoad");

	setTCJNIEnv(vm);

	return JNI_VERSION_1_4;
}


void EXPORT_ME
JAVA_FUNC(keypress) (JNIEnv *env, jobject obj,jint down, jint keycode, jint unicode)
{
	LOGI("keypress %d",keycode);
	PortableKeyEvent(down,keycode,unicode);
}

void EXPORT_ME
JAVA_FUNC(touchEvent) (JNIEnv *env, jobject obj,jint action, jint pid, jfloat x, jfloat y)
{
	//LOGI("TOUCHED");
	controlsContainer.processPointer(action,pid,x,y);
}


void EXPORT_ME
JAVA_FUNC(doAction) (JNIEnv *env, jobject obj,	jint state, jint action)
{
	//gamepadButtonPressed();
	if (hideTouchControls)
		if (tcGameMain)
			if (tcGameMain->isEnabled())
				tcGameMain->animateOut(30);
	LOGI("doAction %d %d",state,action);
	PortableAction(state,action);
}

void EXPORT_ME
JAVA_FUNC(analogFwd) (JNIEnv *env, jobject obj,	jfloat v)
{
	PortableMoveFwd(v);
}

void EXPORT_ME
JAVA_FUNC(analogSide) (JNIEnv *env, jobject obj,jfloat v)
{
	PortableMoveSide(v);
}

void EXPORT_ME
JAVA_FUNC(analogPitch) (JNIEnv *env, jobject obj,
		jint mode,jfloat v)
		{
	PortableLookPitch(mode, v);
		}

void EXPORT_ME
JAVA_FUNC(analogYaw) (JNIEnv *env, jobject obj,	jint mode,jfloat v)
{
	PortableLookYaw(mode, v);
}
void EXPORT_ME
JAVA_FUNC(setTouchSettings) (JNIEnv *env, jobject obj,	jfloat alpha,jfloat strafe,jfloat fwd,jfloat pitch,jfloat yaw,int other)
{
	setTouchSettings(alpha,strafe,fwd,pitch,yaw,other);
}

std::string quickCommandString;
jint EXPORT_ME
JAVA_FUNC(quickCommand) (JNIEnv *env, jobject obj,	jstring command)
{
	const char * p = env->GetStringUTFChars(command,NULL);
	quickCommandString =  std::string(p) + "\n";
	env->ReleaseStringUTFChars(command, p);

	//PortableCommand(quickCommandString.c_str());
	return 0;
}




void EXPORT_ME
JAVA_FUNC(setScreenSize) ( JNIEnv* env,	jobject thiz, jint width, jint height)
{
	android_screen_width = width;
	android_screen_height = height;
}


}
#endif
