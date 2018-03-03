/*
r_strobe.c - Software based strobing implementation

Copyright (C) 2018 * fuzun

For information:
	https://forums.blurbusters.com

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

#define STROBE_ENABLED // Integrate with cmake?

#include "common.h"
#include "wrect.h"

typedef struct StrobeAPI_s StrobeAPI_t;

typedef enum {
	CT_TotalFrame,
	CT_PositiveFrame,
	CT_PositiveNormalFrame,
	CT_PositiveBlackFrame,
	CT_NegativeFrame,
	CT_NegativeNormalFrame,
	CT_NegativeBlackFrame
}counterType;

typedef struct StrobeAPI_exported_funcs_BRIGHTNESSREDUCTION_s {
	double(*ActualBrightnessReduction)(StrobeAPI_t *self);
	double(*LogarithmicBrightnessReduction)(StrobeAPI_t *self, double base);
	double(*SquareBrightnessReduction)(StrobeAPI_t *self, double base);
	double(*CubeBrightnessReduction)(StrobeAPI_t *self, double base);
	double(*OtherBrightnessReduction)(StrobeAPI_t *self, double base, double(*reductionFunction)(double));
} StrobeAPI_exported_funcs_BRIGHTNESSREDUCTION_t;

typedef struct StrobeAPI_exported_funcs_EXPERIMENTAL_s {
	double(*BADNESS)(StrobeAPI_t *self, qboolean PWMInvolved);
	double(*BADNESS_REDUCTED)(StrobeAPI_t *self, qboolean PWMInvolved);
} StrobeAPI_exported_funcs_EXPERIMENTAL_t;

typedef struct StrobeAPI_exported_funcs_PWMSIMULATION_s {
	int(*Frequency)(StrobeAPI_t *self);
	double(*DutyCycle)(void);
	double(*PositivePhaseShift)(StrobeAPI_t *self);
	double(*NegativePhaseShift)(StrobeAPI_t *self);
	double(*Period)(StrobeAPI_t *self);
} StrobeAPI_exported_funcs_PWMSIMULATION_t;

typedef struct StrobeAPI_exported_funcs_HELPER_s {
	qboolean(*isPhaseInverted)(StrobeAPI_t *self);
	qboolean(*isNormal)(StrobeAPI_t *self);
	qboolean(*isPositive)(StrobeAPI_t *self);
	double(*effectiveFPS)(StrobeAPI_t *self);
	void(*GenerateDebugStatistics)(StrobeAPI_t *self, char *src, int size);
	void(*GenerateDiffBar)(StrobeAPI_t *self, char *src, int size, char type);
	double(*GeometricMean)(double, double);
	double(*ArithmeticMean)(double, double);
	double(*StandardDeviation)(const double *data, int size);
} StrobeAPI_exported_funcs_HELPER_t;

typedef struct StrobeAPI_get_s {
	size_t(*FrameCounter)(StrobeAPI_t *self, counterType);
	double(*Deviation)(StrobeAPI_t *self);
	double(*CooldownTimer)(StrobeAPI_t *self);
	double(*CurrentFPS)(StrobeAPI_t *self);
	double(*Cooldown)(StrobeAPI_t *self);
} StrobeAPI_get_t;

typedef struct StrobeAPI_protected_s StrobeAPI_protected_t;

typedef struct StrobeAPI_s {
	StrobeAPI_protected_t *protected; // r_strobe_base_protected_.h
	StrobeAPI_exported_funcs_EXPERIMENTAL_t Experimentals;
	StrobeAPI_exported_funcs_BRIGHTNESSREDUCTION_t BrightnessReductions;
	StrobeAPI_exported_funcs_PWMSIMULATION_t PWM;
	StrobeAPI_exported_funcs_HELPER_t Helpers;
	StrobeAPI_get_t get;
	void(*GenerateBlackFrame)(void);
	void(*ProcessFrame)(StrobeAPI_t *self);
} StrobeAPI_t;


void Strobe_Invoker(void **self, void(*constructor)(void **), void(*main)(void **), void(*destructor)(void **)); // Strobe Invoker

void StrobeAPI_constructor(StrobeAPI_t *self);
void StrobeAPI_destructor(StrobeAPI_t *self);

void R_InitStrobe();

#define STROBE_DEVIATION_LIMIT 2.75
#define STROBE_DEVIATION_SIZE 60

extern convar_t *r_strobe;
extern convar_t *r_strobe_swapinterval;
extern convar_t *r_strobe_debug;
extern convar_t *r_strobe_cooldown;


#endif