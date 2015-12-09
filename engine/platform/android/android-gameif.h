#pragma once
#ifndef ANDROID_GAMEIF_H
#define ANDROID_GAMEIF_H

#define LOOK_MODE_MOUSE    0
#define LOOK_MODE_ABSOLUTE 1
#define LOOK_MODE_JOYSTICK 2

void Android_Vibrate( float life, char flags );
void Android_AddButton( touchbutton_t *button );
void Android_TouchInit( touchbutton_t *buttons );
void Android_RemoveButton( touchbutton_t *button );
void Android_EmitButton( int hButton );

int PortableKeyEvent(int state, int code ,int unitcode);
void PortableAction(int state, int action);

void PortableMove(float fwd, float strafe);
void PortableMoveFwd(float fwd);
void PortableMoveSide(float strafe);
void PortableLookPitch(int mode, float pitch);
void PortableLookYaw(int mode, float pitch);
void PortableCommand(const char * cmd);

void PortableMenuMouse(int action,float x,float y);

int PortableInMenu(void);
#endif
