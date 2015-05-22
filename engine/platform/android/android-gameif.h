#define PORT_ACT_LEFT       1
#define PORT_ACT_RIGHT      2
#define PORT_ACT_FWD        3
#define PORT_ACT_BACK       4
#define PORT_ACT_LOOK_UP    5
#define PORT_ACT_LOOK_DOWN  6
#define PORT_ACT_MOVE_LEFT  7
#define PORT_ACT_MOVE_RIGHT 8
#define PORT_ACT_STRAFE     9
#define PORT_ACT_SPEED      10
#define PORT_ACT_USE        11
#define PORT_ACT_JUMP       12
#define PORT_ACT_ATTACK     13
#define PORT_ACT_UP         14
#define PORT_ACT_DOWN       15

#define PORT_ACT_NEXT_WEP   16
#define PORT_ACT_PREV_WEP   17

#define PORT_ACT_QUICKSAVE  20
#define PORT_ACT_QUICKLOAD  21


#define PORT_ACT_RELOAD     52
#define PORT_ACT_LIGHT      53
#define PORT_ACT_ATTACK_ALT     54

#define PORT_ACT_WEAP0              100
#define PORT_ACT_WEAP1              101
#define PORT_ACT_WEAP2              102
#define PORT_ACT_WEAP3              103
#define PORT_ACT_WEAP4              104
#define PORT_ACT_WEAP5              105
#define PORT_ACT_WEAP6              106
#define PORT_ACT_WEAP7              107
#define PORT_ACT_WEAP8              108
#define PORT_ACT_WEAP9              109
#define PORT_ACT_WEAP10             110
#define PORT_ACT_WEAP11             111
#define PORT_ACT_WEAP12             112
#define PORT_ACT_WEAP13             113



//Generic custom buttons
#define PORT_ACT_CUSTOM_0          150
#define PORT_ACT_CUSTOM_1          151
#define PORT_ACT_CUSTOM_2          152
#define PORT_ACT_CUSTOM_3          153
#define PORT_ACT_CUSTOM_4          154
#define PORT_ACT_CUSTOM_5          155
#define PORT_ACT_CUSTOM_6          156
#define PORT_ACT_CUSTOM_7          157


#define LOOK_MODE_MOUSE    0
#define LOOK_MODE_ABSOLUTE 1
#define LOOK_MODE_JOYSTICK 2

int PortableKeyEvent(int state, int code ,int unitcode);
void PortableAction(int state, int action);

void PortableMove(float fwd, float strafe);
void PortableMoveFwd(float fwd);
void PortableMoveSide(float strafe);
void PortableLookPitch(int mode, float pitch);
void PortableLookYaw(int mode, float pitch);
void PortableCommand(const char * cmd);

void PortableMenuMouse(int action,float x,float y);


void PortableInit(int argc,const char ** argv);

int PortableInMenu(void);
