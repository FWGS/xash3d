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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <jni.h>
#include <android/log.h>
#include <unistd.h>


#include "TouchControlsContainer.h"
#include "JNITouchControlsUtils.h"



#include "android-gameif.h"

void ResetNanoState();

extern "C"
{

void SDL_SetSwapBufferCallBack(void (*pt2Func)(void));

#include "SDL_keycode.h"

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
	ResetNanoState();

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


extern unsigned int Sys_Milliseconds(void);

static unsigned int reload_time_down;
void gameButton(int state,int code)
{
	if (code == KEY_SHOOT)
	{
		shooting = state;
		PortableAction(state,PORT_ACT_ATTACK);
	}
	else if (code == KEY_SHOW_WEAPONS)
	{
		if (state == 1)
			if (!tcGameWeapons->enabled)
			{

				tcGameWeapons->animateIn(5);
			}
	}
	else
	{
		PortableAction(state, code);
	}
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
	//float strafe = joy_x*joy_x;
	float strafe = joy_x;
	//if (joy_x < 0)
	//	strafe *= -1;

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

		//Menu
		/*
		tcMenuMain->addControl(new touchcontrols::Button("down_arrow",touchcontrols::RectF(20,13,23,16),"arrow_down",SDL_SCANCODE_DOWN));
		tcMenuMain->addControl(new touchcontrols::Button("up_arrow",touchcontrols::RectF(20,10,23,13),"arrow_up",SDL_SCANCODE_UP));
		tcMenuMain->addControl(new touchcontrols::Button("left_arrow",touchcontrols::RectF(17,13,20,16),"arrow_left",SDL_SCANCODE_LEFT));
		tcMenuMain->addControl(new touchcontrols::Button("right_arrow",touchcontrols::RectF(23,13,26,16),"arrow_right",SDL_SCANCODE_RIGHT));
		tcMenuMain->addControl(new touchcontrols::Button("enter",touchcontrols::RectF(0,12,4,16),"enter",SDL_SCANCODE_RETURN));
		*/
		touchcontrols::MultitouchMouse *mouseMenu = new touchcontrols::MultitouchMouse("mouse",touchcontrols::RectF(0,0,26,16),"");
		mouseMenu->setHideGraphics(true);
		tcMenuMain->addControl(mouseMenu);
		mouseMenu->signal_action.connect(sigc::ptr_fun(&menuMouse) );
		tcMenuMain->setPassThroughTouch(false);


		tcMenuMain->signal_button.connect(  sigc::ptr_fun(&menuButton) );

		tcMenuMain->setAlpha(0.8);


		//Game
		tcGameMain->setAlpha(gameControlsAlpha);
		tcGameMain->addControl(new touchcontrols::Button("attack",touchcontrols::RectF(20,7,23,10),"shoot",KEY_SHOOT));
		tcGameMain->addControl(new touchcontrols::Button("attack_alt",touchcontrols::RectF(20,4,23,7),"shoot_alt",PORT_ACT_ATTACK_ALT));
		tcGameMain->addControl(new touchcontrols::Button("use",touchcontrols::RectF(23,6,26,9),"use",PORT_ACT_USE));
		tcGameMain->addControl(new touchcontrols::Button("quick_save",touchcontrols::RectF(24,0,26,2),"save",PORT_ACT_QUICKSAVE));
		tcGameMain->addControl(new touchcontrols::Button("quick_load",touchcontrols::RectF(20,0,22,2),"load",PORT_ACT_QUICKLOAD));
		tcGameMain->addControl(new touchcontrols::Button("flashlight",touchcontrols::RectF(4,0,6,2),"show_weapons",PORT_ACT_LIGHT));
		tcGameMain->addControl(new touchcontrols::Button("jump",touchcontrols::RectF(23,3,26,6),"jump",PORT_ACT_JUMP));
		tcGameMain->addControl(new touchcontrols::Button("crouch",touchcontrols::RectF(24,14,26,16),"crouch",PORT_ACT_DOWN));

		tcGameMain->addControl(new touchcontrols::Button("show_weapons",touchcontrols::RectF(11,14,13,16),"show_weapons",KEY_SHOW_WEAPONS));

		nextWeapon = new touchcontrols::Button("next_weapon",touchcontrols::RectF(0,3,3,5),"next_weap",PORT_ACT_NEXT_WEP);
		tcGameMain->addControl(nextWeapon);
		prevWeapon = new touchcontrols::Button("prev_weapon",touchcontrols::RectF(0,7,3,9),"prev_weap",PORT_ACT_PREV_WEP);
		tcGameMain->addControl(prevWeapon);

		tcGameMain->addControl(new touchcontrols::Button("reload",touchcontrols::RectF(0,5,3,7),"reload",PORT_ACT_RELOAD));


		touchJoyLeft = new touchcontrols::TouchJoy("stick",touchcontrols::RectF(0,7,8,16),"strafe_arrow");
		tcGameMain->addControl(touchJoyLeft);
		touchJoyLeft->signal_move.connect(sigc::ptr_fun(&left_stick) );
		touchJoyLeft->signal_double_tap.connect(sigc::ptr_fun(&left_double_tap) );

		touchJoyRight = new touchcontrols::TouchJoy("touch",touchcontrols::RectF(17,4,26,16),"look_arrow");
		tcGameMain->addControl(touchJoyRight);
		touchJoyRight->signal_move.connect(sigc::ptr_fun(&right_stick) );
		touchJoyRight->signal_double_tap.connect(sigc::ptr_fun(&right_double_tap) );

		tcGameMain->signal_button.connect(  sigc::ptr_fun(&gameButton) );

		//Weapons
		tcGameWeapons->addControl(new touchcontrols::Button("weapon1",touchcontrols::RectF(1,14,3,16),"key_1",SDL_SCANCODE_1));
		tcGameWeapons->addControl(new touchcontrols::Button("weapon2",touchcontrols::RectF(3,14,5,16),"key_2",SDL_SCANCODE_2));
		tcGameWeapons->addControl(new touchcontrols::Button("weapon3",touchcontrols::RectF(5,14,7,16),"key_3",SDL_SCANCODE_3));
		tcGameWeapons->addControl(new touchcontrols::Button("weapon4",touchcontrols::RectF(7,14,9,16),"key_4",SDL_SCANCODE_4));
		tcGameWeapons->addControl(new touchcontrols::Button("weapon5",touchcontrols::RectF(9,14,11,16),"key_5",SDL_SCANCODE_5));

		tcGameWeapons->addControl(new touchcontrols::Button("weapon6",touchcontrols::RectF(15,14,17,16),"key_6",SDL_SCANCODE_6));
		tcGameWeapons->addControl(new touchcontrols::Button("weapon7",touchcontrols::RectF(17,14,19,16),"key_7",SDL_SCANCODE_7));
		tcGameWeapons->addControl(new touchcontrols::Button("weapon8",touchcontrols::RectF(19,14,21,16),"key_8",SDL_SCANCODE_8));
		tcGameWeapons->addControl(new touchcontrols::Button("weapon9",touchcontrols::RectF(21,14,23,16),"key_9",SDL_SCANCODE_9));
		tcGameWeapons->addControl(new touchcontrols::Button("weapon0",touchcontrols::RectF(23,14,25,16),"key_0",SDL_SCANCODE_0));
		tcGameWeapons->signal_button.connect(  sigc::ptr_fun(&selectWeaponButton) );
		tcGameWeapons->setAlpha(0.8);

		//Weapon wheel
		touchcontrols::WheelSelect *wheel = new touchcontrols::WheelSelect("weapon_wheel",touchcontrols::RectF(7,2,19,14),"weapon_wheel",10);
		wheel->signal_selected.connect(sigc::ptr_fun(&weaponWheel) );
		wheel->signal_enabled.connect(sigc::ptr_fun(&weaponWheelSelected));
		tcWeaponWheel->addControl(wheel);
		tcWeaponWheel->setAlpha(0.5);


		controlsContainer.addControlGroup(tcGameMain);
		controlsContainer.addControlGroup(tcGameWeapons);
		controlsContainer.addControlGroup(tcMenuMain);
		controlsContainer.addControlGroup(tcWeaponWheel);
		controlsCreated = 1;

		tcGameMain->setXMLFile(settings_file);
	}
	else
		LOGI("NOT creating controls");


}

extern void flush(); //in glshim/arc/gl/gl.c

int inMenuLast = 1;
int inAutomapLast = 0;
static int glInit = 0;
void frameControls()
{
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

	//flush(); //draw out game from glshim

	controlsContainer.draw();

	//flush(); //draw out controls from glshim
	// glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	// glClear(GL_COLOR_BUFFER_BIT);
}


void setTouchSettings(float alpha,float strafe,float fwd,float pitch,float yaw,int other)
{

	gameControlsAlpha = alpha;
	if (tcGameMain)
		tcGameMain->setAlpha(gameControlsAlpha);

	showWeaponCycle = other & 0x1?true:false;
	turnMouseMode   = other & 0x2?true:false;
	invertLook      = other & 0x4?true:false;
	precisionShoot  = other & 0x8?true:false;
	showSticks      = other & 0x1000?true:false;
	enableWeaponWheel  = other & 0x2000?true:false;

	if (tcWeaponWheel)
		tcWeaponWheel->setEnabled(enableWeaponWheel);


	hideTouchControls = other & 0x80000000?true:false;


	switch ((other>>4) & 0xF)
	{
	case 1:
		left_double_action = PORT_ACT_ATTACK;
		break;
	case 2:
		left_double_action = PORT_ACT_JUMP;
		break;
	default:
		left_double_action = 0;
	}

	switch ((other>>8) & 0xF)
	{
	case 1:
		right_double_action = PORT_ACT_ATTACK;
		break;
	case 2:
		right_double_action = PORT_ACT_JUMP;
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

	SDL_SetSwapBufferCallBack(frameControls);

	return 0;
}


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
