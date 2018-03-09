/*
r_strobe_api.h - Software based strobing implementation

Copyright (C) 2018 - fuzun * github/fuzun

For information:
	https://forums.blurbusters.com/viewtopic.php?f=7&t=3815&p=30401

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.
*/

#if !defined R_STROBE_API_H && !defined STROBE_DISABLED
#define R_STROBE_API_H

#define STROBE_ENABLED

#include "common.h"
#include "wrect.h"

#define STROBE_INVOKE(IMPL,CONSTRUCTOR,MAIN,DESTRUCTOR) (void**)(&IMPL),CONSTRUCTOR,MAIN,DESTRUCTOR

typedef struct StrobeAPI_s StrobeAPI_t;

typedef enum
{
	STROBE_CT_TotalFrame,
	STROBE_CT_PositiveFrame,
	STROBE_CT_PositiveNormalFrame,
	STROBE_CT_PositiveBlackFrame,
	STROBE_CT_NegativeFrame,
	STROBE_CT_NegativeNormalFrame,
	STROBE_CT_NegativeBlackFrame
} STROBE_counterType;

typedef struct StrobeAPI_funcs_BRIGHTNESSREDUCTION_s
{
	double ( *ActualBrightnessReduction )( StrobeAPI_t *self );
	double ( *LogarithmicBrightnessReduction )( StrobeAPI_t *self, double base );
	double ( *SquareBrightnessReduction )( StrobeAPI_t *self, double base );
	double ( *CubeBrightnessReduction )( StrobeAPI_t *self, double base );
	double ( *OtherBrightnessReduction )( StrobeAPI_t *self, double base, double ( *reductionFunction )( double ) );
} StrobeAPI_funcs_BRIGHTNESSREDUCTION_t;

typedef struct StrobeAPI_funcs_EXPERIMENTAL_s
{
	double ( *BADNESS )( StrobeAPI_t *self, qboolean PWMInvolved );
	double ( *BADNESS_REDUCTED )( StrobeAPI_t *self, qboolean PWMInvolved );
} StrobeAPI_funcs_EXPERIMENTAL_t;

typedef struct StrobeAPI_funcs_PWMSIMULATION_s
{
	int ( *Frequency )( StrobeAPI_t *self );
	double ( *DutyCycle )( void );
	double ( *PositivePhaseShift )( StrobeAPI_t *self );
	double ( *NegativePhaseShift )( StrobeAPI_t *self );
	double ( *Period )( StrobeAPI_t *self );
} StrobeAPI_funcs_PWMSIMULATION_t;

typedef struct StrobeAPI_funcs_HELPER_s
{
	qboolean ( *isPhaseInverted )( StrobeAPI_t *self );
	qboolean ( *isNormal )( StrobeAPI_t *self );
	qboolean ( *isPositive )( StrobeAPI_t *self );
	double ( *effectiveFPS )( StrobeAPI_t *self );
	double ( *CurrentFPS )( StrobeAPI_t *self );
	void ( *GenerateDebugStatistics )( StrobeAPI_t *self, char *src, int size );
	void ( *GenerateDiffBar )( StrobeAPI_t *self, char *src, int size, char type );
	double ( *GeometricMean )( double, double );
	double ( *ArithmeticMean )( double, double );
	double ( *StandardDeviation )( const double *data, int size );
	double ( *Cooldown )( StrobeAPI_t *self );
} StrobeAPI_funcs_HELPER_t;

typedef struct StrobeAPI_GET_s
{
	size_t ( *FrameCounter )( StrobeAPI_t *self, STROBE_counterType );
} StrobeAPI_GET_t;

typedef struct StrobeAPI_protected_s StrobeAPI_protected_t;

typedef struct StrobeAPI_s
{
	StrobeAPI_protected_t *protected; // r_strobe_base_protected_.h
	StrobeAPI_funcs_EXPERIMENTAL_t Experimentals;
	StrobeAPI_funcs_BRIGHTNESSREDUCTION_t BrightnessReductions;
	StrobeAPI_funcs_PWMSIMULATION_t PWM;
	StrobeAPI_funcs_HELPER_t Helpers;
	StrobeAPI_GET_t get;
	void ( *GenerateBlackFrame )( void );
	void ( *ProcessFrame )( StrobeAPI_t *self );
} StrobeAPI_t;

typedef struct StrobeAPI_EXPORTS_s
{
	void ( *Invoker )( void **self, void ( *constructor )( void ** ), void ( *main )( void ** ), void ( *destructor )( void ** ) ); // Strobe Invoker
	void ( *Constructor )( StrobeAPI_t *self );
	void ( *Destructor )( StrobeAPI_t *self );

	// Strobe related convars should be in StrobeAPI derived implementations!
	convar_t *r_strobe;
	convar_t *r_strobe_swapinterval;
	convar_t *r_strobe_debug;
	convar_t *r_strobe_cooldown;
} StrobeAPI_EXPORTS_t;

extern StrobeAPI_EXPORTS_t StrobeAPI;

void R_InitStrobeAPI( );

#endif
