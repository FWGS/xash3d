/*
mobility_int.h - interface between engine and client for mobile platforms
Copyright (C) 2015 a1batross

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/

#pragma once
#ifndef MOBILITY_INT_H
#define MOBILITY_INT_H
#ifdef __cplusplus
extern "C" {
#endif

#define MOBILITY_API_VERSION 1
#define MOBILITY_CLIENT_EXPORT "HUD_MobilityInterface"

#define VIBRATE_NORMAL (1 << 0) // just vibrate for given "life"

#define TOUCH_ACT_SHOOT_NAME		"shoot"
#define TOUCH_ACT_SHOOT_ALT_NAME	"shoot_alt"
#define TOUCH_ACT_USE_NAME			"use"
#define TOUCH_ACT_JUMP_NAME			"jump"
#define TOUCH_ACT_CROUCH_NAME		"crouch"
#define TOUCH_ACT_RELOAD_NAME		"reload"
#define TOUCH_ACT_INVPREV_NAME		"prev_weap"
#define TOUCH_ACT_INVNEXT_NAME		"next_weap"
#define TOUCH_ACT_LIGHT_NAME		"flash_light_filled"
#define TOUCH_ACT_QUICKSAVE_NAME	"save"
#define TOUCH_ACT_QUICKLOAD_NAME	"load"
#define TOUCH_ACT_SHOW_NUMBERS_NAME "show_weapons"

// these controls are added by engine.
enum {
	TOUCH_ACT_SHOOT = 0,
	TOUCH_ACT_SHOOT_ALT,
	TOUCH_ACT_USE,
	TOUCH_ACT_JUMP,
	TOUCH_ACT_CROUCH,
	TOUCH_ACT_RELOAD,
	TOUCH_ACT_INVPREV,
	TOUCH_ACT_INVNEXT,
	TOUCH_ACT_LIGHT,
	TOUCH_ACT_QUICKSAVE,
	TOUCH_ACT_QUICKLOAD,
	TOUCH_ACT_SHOW_NUMBERS,
	TOUCH_ACT_0,
	TOUCH_ACT_1,
	TOUCH_ACT_2,
	TOUCH_ACT_3,
	TOUCH_ACT_4,
	TOUCH_ACT_5,
	TOUCH_ACT_6,			// numbers always binded to real keyboard numbers
	TOUCH_ACT_7,
	TOUCH_ACT_8,
	TOUCH_ACT_9,
	TOUCH_ACT_LAST,
	TOUCH_ACT_USER = 24,
	TOUCH_ACT_MAX = 64
};

typedef struct touchbutton_s
{
	wrect_t sRect;	// position and size
	int iType;			// button type. Unused. All buttons in "tap" mode
	int iState;
	int hButton;		// button handle
	char *pszCommand;	// command executed by button
	char *pszImageName; // path to image
	void *object;		// private object created by Beloko or SDLash controls
} touchbutton_t;

typedef struct mobile_engfuncs_s
{
	// indicates version of API. Should be equal to MOBILITY_API_VERSION
	// version changes when existing functions are changes
	int version;

	// vibration control
	// life -- time to vibrate in ms
	void (*pfnVibrate)( float life, char flags );

	// enable text input
	void (*pfnEnableTextInput)( int enable );

	// touch controls
	// This func will put your command to button array and return unique number
	// Will return -1 on error
	int (*Touch_RegisterButton)( const char *cmdname );

	// xpos, ypos -- position on screen
	// xsize, ysize -- size
	// hButton -- integer given by Touch_RegiserButton
	// pszImage -- path to image for button. May be in any format that supported by Xash3D
	touchbutton_t *(*Touch_AddCustomButton)( int xpos, int ypos, int xsize, int ysize, int hButton, const char *pszImageName);

	// get touchbutton_t struct for button ID
	// even for predefined buttons
	touchbutton_t *(*Touch_GetButtonByID)( int hButton );

	// remove button
	void (*Touch_RemoveButton)( touchbutton_t *pButton );

	// simulate button touching
	void (*Touch_EmitButton)( int hButton );

	// temporarily disable touch controls
	void (*Touch_Disable)( int disable );

	// add to weapon wheel custom commands
	// cmds -- array of commands
	// segments -- sizeof(cmds)
	// pszImage -- path to image
	void (*Touch_SetWeaponWheel)( const char **cmds, int segments, const char *pszImageName );

	// To be continued...
} mobile_engfuncs_t;

extern mobile_engfuncs_t *gMobileEngfuncs;

// function exported from client
// returns 0 on no error otherwise error
typedef int (*pfnMobilityInterface)( mobile_engfuncs_t *gMobileEngfuncs );

#ifdef __cplusplus
}
#endif
#endif
