/*
mathlib.c - internal mathlib
Copyright (C) 2010 Uncle Mike

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/

#if defined (__linux__) && !defined (__ANDROID__)
//sincosf
#define _GNU_SOURCE
#include <math.h>
#endif
#include "common.h"
#include "mathlib.h"

#ifdef XASH_VECTORIZE_SINCOS
// Test shown that this is not so effictively
#if defined(__SSE__) || defined(_M_IX86_FP)
#if defined(__SSE2__) || defined(_M_IX86_FP)
  #define USE_SSE2
 #endif
#include "sse_mathfun.h"
#endif
#if defined(__ARM_NEON__) || defined(__NEON__)
	#include "neon_mathfun.h"
#endif
#endif

vec3_t	vec3_origin = { 0, 0, 0 };

/*
=================
anglemod
=================
*/
float anglemod( const float a )
{
	return (360.0f/65536) * ((int)(a*(65536/360.0f)) & 65535);
}

word FloatToHalf( float v )
{
	unsigned int	i = *((unsigned int *)&v);
	unsigned int	e = (i >> 23) & 0x00ff;
	unsigned int	m = i & 0x007fffff;
	unsigned short	h;

	if( e <= 127 - 15 )
		h = ((m | 0x00800000) >> (127 - 14 - e)) >> 13;
	else h = (i >> 13) & 0x3fff;

	h |= (i >> 16) & 0xc000;

	return h;
}

float HalfToFloat( word h )
{
	unsigned int	f = (h << 16) & 0x80000000;
	unsigned int	em = h & 0x7fff;

	if( em > 0x03ff )
	{
		f |= (em << 13) + ((127 - 15) << 23);
	}
	else
	{
		unsigned int m = em & 0x03ff;

		if( m != 0 )
		{
			unsigned int e = (em >> 10) & 0x1f;

			while(( m & 0x0400 ) == 0 )
			{
				m <<= 1;
				e--;
			}

			m &= 0x3ff;
			f |= ((e + (127 - 14)) << 23) | (m << 13);
		}
	}

	return *((float *)&f);
}

/*
=================
SignbitsForPlane

fast box on planeside test
=================
*/
int SignbitsForPlane( const vec3_t normal )
{
	int	bits, i;

	for( bits = i = 0; i < 3; i++ )
		if( normal[i] < 0.0f ) bits |= 1<<i;
	return bits;
}

/*
=================
NearestPOW
=================
*/
int NearestPOW( int value, qboolean roundDown )
{
	int	n = 1;

	if( value <= 0 ) return 1;
	while( n < value ) n <<= 1;

	if( roundDown )
	{
		if( n > value ) n >>= 1;
	}
	return n;
}

// remap a value in the range [A,B] to [C,D].
float RemapVal( float val, float A, float B, float C, float D )
{
	return C + (D - C) * (val - A) / (B - A);
}

float ApproachVal( float target, float value, float speed )
{
	float	delta = target - value;

	if( delta > speed )
		value += speed;
	else if( delta < -speed )
		value -= speed;
	else value = target;

	return value;
}

/*
=================
rsqrt
=================
*/
float rsqrt( float number )
{
	int	i;
	float	x, y;

	if( number == 0.0f )
		return 0.0f;

	x = number * 0.5f;
	i = *(int *)&number;	// evil floating point bit level hacking
	i = 0x5f3759df - (i >> 1);	// what the fuck?
	y = *(float *)&i;
	y = y * (1.5f - (x * y * y));	// first iteration

	return y;
}

/*
=================
SinCos
=================
*/
void SinCos( float radians, float *sine, float *cosine )
{
#if _MSC_VER == 1200
	_asm
	{
		fld	dword ptr [radians]
		fsincos

		mov edx, dword ptr [cosine]
		mov eax, dword ptr [sine]

		fstp dword ptr [edx]
		fstp dword ptr [eax]
	}
#else
	// I think, better use math.h function, instead of ^
#if defined (__linux__) && !defined (__ANDROID__)
	sincosf(radians, sine, cosine);
#else
	*sine = sinf(radians);
	*cosine = cosf(radians);
#endif
#endif
}

#ifdef XASH_VECTORIZE_SINCOS
void SinCosFastVector4(float r1, float r2, float r3, float r4,
					  float *s0, float *s1, float *s2, float *s3,
					  float *c0, float *c1, float *c2, float *c3)
{
	v4sf rad_vector = {r1, r2, r3, r4};
	v4sf sin_vector, cos_vector;

	sincos_ps(rad_vector, &sin_vector, &cos_vector);

	*s0 = s4f_x(sin_vector);
	*s1 = s4f_y(sin_vector);
	*s2 = s4f_z(sin_vector);
	*s3 = s4f_w(sin_vector);

	*c0 = s4f_x(cos_vector);
	*c1 = s4f_y(cos_vector);
	*c2 = s4f_z(cos_vector);
	*c3 = s4f_w(cos_vector);
}

void SinCosFastVector3(float r1, float r2, float r3,
					  float *s0, float *s1, float *s2,
					  float *c0, float *c1, float *c2)
{
	v4sf rad_vector = {r1, r2, r3, 0};
	v4sf sin_vector, cos_vector;

	sincos_ps(rad_vector, &sin_vector, &cos_vector);

	*s0 = s4f_x(sin_vector);
	*s1 = s4f_y(sin_vector);
	*s2 = s4f_z(sin_vector);

	*c0 = s4f_x(cos_vector);
	*c1 = s4f_y(cos_vector);
	*c2 = s4f_z(cos_vector);
}

void SinCosFastVector2(float r1, float r2,
					  float *s0, float *s1,
					  float *c0, float *c1)
{
	v4sf rad_vector = {r1, r2, 0, 0};
	v4sf sin_vector, cos_vector;

	sincos_ps(rad_vector, &sin_vector, &cos_vector);

	*s0 = s4f_x(sin_vector);
	*s1 = s4f_y(sin_vector);

	*c0 = s4f_x(cos_vector);
	*c1 = s4f_y(cos_vector);
}

void SinFastVector3(float r1, float r2, float r3,
					  float *s0, float *s1, float *s2)
{
	v4sf rad_vector = {r1, r2, r3, 0};
	v4sf sin_vector;

	sin_vector = sin_ps(rad_vector);

	*s0 = s4f_x(sin_vector);
	*s1 = s4f_y(sin_vector);
	*s2 = s4f_z(sin_vector);
}
#endif

float VectorNormalizeLength2( const vec3_t v, vec3_t out )
{
	float	length, ilength;

	length = v[0] * v[0] + v[1] * v[1] + v[2] * v[2];
	length = sqrt( length );

	if( length )
	{
		ilength = 1.0f / length;
		out[0] = v[0] * ilength;
		out[1] = v[1] * ilength;
		out[2] = v[2] * ilength;
	}

	return length;
}

void VectorVectors( const vec3_t forward, vec3_t right, vec3_t up )
{
	float	d;

	right[0] = forward[2];
	right[1] = -forward[0];
	right[2] = forward[1];

	d = DotProduct( forward, right );
	VectorMA( right, -d, forward, right );
	VectorNormalize( right );
	CrossProduct( right, forward, up );
}

/*
=================
AngleVectors

=================
*/
void GAME_EXPORT AngleVectors( const vec3_t angles, vec3_t forward, vec3_t right, vec3_t up )
{
	static float	sr, sp, sy, cr, cp, cy;

#ifdef XASH_VECTORIZE_SINCOS
	SinCosFastVector3( DEG2RAD(angles[YAW]), DEG2RAD(angles[PITCH]), DEG2RAD(angles[ROLL]),
		&sy, &sp, &sr,
		&cy, &cp, &cr);
#else
	SinCos( DEG2RAD( angles[YAW] ), &sy, &cy );
	SinCos( DEG2RAD( angles[PITCH] ), &sp, &cp );
	SinCos( DEG2RAD( angles[ROLL] ), &sr, &cr );
#endif

	if( forward )
	{
		forward[0] = cp * cy;
		forward[1] = cp * sy;
		forward[2] = -sp;
	}

	if( right )
	{
		right[0] = (-1.0f * sr * sp * cy + -1.0f * cr * -sy );
		right[1] = (-1.0f * sr * sp * sy + -1.0f * cr * cy );
		right[2] = (-1.0f * sr * cp);
	}

	if( up )
	{
		up[0] = (cr * sp * cy + -sr * -sy );
		up[1] = (cr * sp * sy + -sr * cy );
		up[2] = (cr * cp);
	}
}

/*
=================
VectorAngles

=================
*/
void VectorAngles( const float *forward, float *angles )
{
	float	tmp, yaw, pitch;

	if( !forward || !angles )
	{
		if( angles ) VectorClear( angles );
		return;
	}

	if( forward[1] == 0 && forward[0] == 0 )
	{
		// fast case
		yaw = 0;
		if( forward[2] > 0 )
			pitch = 90.0f;
		else pitch = 270.0f;
	}
	else
	{
		yaw = ( atan2( forward[1], forward[0] ) * 180 / M_PI );
		if( yaw < 0 ) yaw += 360;

		tmp = sqrt( forward[0] * forward[0] + forward[1] * forward[1] );
		pitch = ( atan2( forward[2], tmp ) * 180 / M_PI );
		if( pitch < 0 ) pitch += 360;
	}

	VectorSet( angles, pitch, yaw, 0 ); 
}

/*
=================
VectorsAngles

=================
*/
void VectorsAngles( const vec3_t forward, const vec3_t right, const vec3_t up, vec3_t angles )
{
	float	pitch, cpitch, yaw, roll;

	pitch = -asin( forward[2] );
	cpitch = cos( pitch );

	if( fabs( cpitch ) > EQUAL_EPSILON )	// gimball lock?
	{
		cpitch = 1.0f / cpitch;
		pitch = RAD2DEG( pitch );
		yaw = RAD2DEG( atan2( forward[1] * cpitch, forward[0] * cpitch ));
		roll = RAD2DEG( atan2( -right[2] * cpitch, up[2] * cpitch ));
	}
	else
	{
		pitch = forward[2] > 0 ? -90.0f : 90.0f;
		yaw = RAD2DEG( atan2( right[0], -right[1] ));
		roll = 180.0f;
	}

	angles[PITCH] = pitch;
	angles[YAW] = yaw;
	angles[ROLL] = roll;
}

/*
=================
InterpolateAngles
=================
*/
void InterpolateAngles( vec3_t start, vec3_t end, vec3_t out, float frac )
{
	float	d, ang1, ang2;
	int i;
	for( i = 0; i < 3; i++ )
	{
		ang1 = start[i];
		ang2 = end[i];
		d = ang1 - ang2;

		if( d > 180.0f ) d -= 360.0f;
		else if( d < -180.0f ) d += 360.0f;

		out[i] = ang2 + d * frac;
	}
}

//
// bounds operations
//
/*
=================
ClearBounds
=================
*/
void ClearBounds( vec3_t mins, vec3_t maxs )
{
	// make bogus range
	mins[0] = mins[1] = mins[2] =  999999.0f;
	maxs[0] = maxs[1] = maxs[2] = -999999.0f;
}

/*
=================
AddPointToBounds
=================
*/
void AddPointToBounds( const vec3_t v, vec3_t mins, vec3_t maxs )
{
	float	val;
	int	i;

	for( i = 0; i < 3; i++ )
	{
		val = v[i];
		if( val < mins[i] ) mins[i] = val;
		if( val > maxs[i] ) maxs[i] = val;
	}
}

/*
=================
BoundsIntersect
=================
*/
qboolean BoundsIntersect( const vec3_t mins1, const vec3_t maxs1, const vec3_t mins2, const vec3_t maxs2 )
{
	if( mins1[0] > maxs2[0] || mins1[1] > maxs2[1] || mins1[2] > maxs2[2] )
		return false;
	if( maxs1[0] < mins2[0] || maxs1[1] < mins2[1] || maxs1[2] < mins2[2] )
		return false;
	return true;
}

/*
=================
BoundsAndSphereIntersect
=================
*/
qboolean BoundsAndSphereIntersect( const vec3_t mins, const vec3_t maxs, const vec3_t origin, float radius )
{
	if( mins[0] > origin[0] + radius || mins[1] > origin[1] + radius || mins[2] > origin[2] + radius )
		return false;
	if( maxs[0] < origin[0] - radius || maxs[1] < origin[1] - radius || maxs[2] < origin[2] - radius )
		return false;
	return true;
}

/*
=================
RadiusFromBounds
=================
*/
float RadiusFromBounds( const vec3_t mins, const vec3_t maxs )
{
	vec3_t	corner;
	int	i;

	for( i = 0; i < 3; i++ )
	{
		corner[i] = fabs( mins[i] ) > fabs( maxs[i] ) ? fabs( mins[i] ) : fabs( maxs[i] );
	}
	return VectorLength( corner );
}

/*
====================
RotatePointAroundVector
====================
*/
void RotatePointAroundVector( vec3_t dst, const vec3_t dir, const vec3_t point, float degrees )
{
	float	t0, t1;
	float	angle, c, s;
	vec3_t	vr, vu, vf;

	angle = DEG2RAD( degrees );
	SinCos( angle, &s, &c );
	VectorCopy( dir, vf );
	VectorVectors( vf, vr, vu );

	t0 = vr[0] *  c + vu[0] * -s;
	t1 = vr[0] *  s + vu[0] *  c;
	dst[0] = (t0 * vr[0] + t1 * vu[0] + vf[0] * vf[0]) * point[0]
	       + (t0 * vr[1] + t1 * vu[1] + vf[0] * vf[1]) * point[1]
	       + (t0 * vr[2] + t1 * vu[2] + vf[0] * vf[2]) * point[2];

	t0 = vr[1] *  c + vu[1] * -s;
	t1 = vr[1] *  s + vu[1] *  c;
	dst[1] = (t0 * vr[0] + t1 * vu[0] + vf[1] * vf[0]) * point[0]
	       + (t0 * vr[1] + t1 * vu[1] + vf[1] * vf[1]) * point[1]
	       + (t0 * vr[2] + t1 * vu[2] + vf[1] * vf[2]) * point[2];

	t0 = vr[2] *  c + vu[2] * -s;
	t1 = vr[2] *  s + vu[2] *  c;
	dst[2] = (t0 * vr[0] + t1 * vu[0] + vf[2] * vf[0]) * point[0]
	       + (t0 * vr[1] + t1 * vu[1] + vf[2] * vf[1]) * point[1]
	       + (t0 * vr[2] + t1 * vu[2] + vf[2] * vf[2]) * point[2];
}

//
// studio utils
//
/*
====================
AngleQuaternion

====================
*/
void AngleQuaternion( const vec3_t angles, vec4_t q )
{
	float	sr, sp, sy, cr, cp, cy;

#ifdef XASH_VECTORIZE_SINCOS
	SinCosFastVector3( angles[2] * 0.5f, angles[1] * 0.5f, angles[0] * 0.5f,
		&sy, &sp, &sr,
		&cy, &cp, &cr);
#else
	float	angle;

	angle = angles[2] * 0.5f;
	SinCos( angle, &sy, &cy );
	angle = angles[1] * 0.5f;
	SinCos( angle, &sp, &cp );
	angle = angles[0] * 0.5f;
	SinCos( angle, &sr, &cr );
#endif

	q[0] = sr * cp * cy - cr * sp * sy; // X
	q[1] = cr * sp * cy + sr * cp * sy; // Y
	q[2] = cr * cp * sy - sr * sp * cy; // Z
	q[3] = cr * cp * cy + sr * sp * sy; // W
}

/*
====================
QuaternionSlerp

====================
*/
void QuaternionSlerp( const vec4_t p, vec4_t q, float t, vec4_t qt )
{
	float	omega, sclp, sclq;
	float	cosom, sinom;
	float	a = 0.0f;
	float	b = 0.0f;
	int	i;

	// decide if one of the quaternions is backwards
	for( i = 0; i < 4; i++ )
	{
		a += (p[i] - q[i]) * (p[i] - q[i]);
		b += (p[i] + q[i]) * (p[i] + q[i]);
	}

	if( a > b )
	{
		for( i = 0; i < 4; i++ )
		{
			q[i] = -q[i];
		}
	}

	cosom = p[0] * q[0] + p[1] * q[1] + p[2] * q[2] + p[3] * q[3];

	if(( 1.0 + cosom ) > 0.000001f )
	{
		if(( 1.0f - cosom ) > 0.000001f )
		{
			omega = acos( cosom );

#ifdef XASH_VECTORIZE_SINCOS
			SinFastVector3( omega, ( 1.0f - t ) * omega, t * omega,
				&sinom, &sclp, &sclq );
			sclp /= sinom;
			sclq /= sinom;
#else
			sinom = sin( omega );
			sclp = sin(( 1.0f - t ) * omega ) / sinom;
			sclq = sin( t * omega ) / sinom;
#endif

		}
		else
		{
			sclp = 1.0f - t;
			sclq = t;
		}

		for( i = 0; i < 4; i++ )
			qt[i] = sclp * p[i] + sclq * q[i];
	}
	else
	{
		qt[0] = -q[1];
		qt[1] = q[0];
		qt[2] = -q[3];
		qt[3] = q[2];
		sclp = sin(( 1.0f - t ) * ( 0.5f * M_PI ));
		sclq = sin( t * ( 0.5f * M_PI ));

		for( i = 0; i < 3; i++ )
			qt[i] = sclp * p[i] + sclq * qt[i];
	}
}
