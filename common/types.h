// basic typedefs

typedef int		sound_t;
typedef float		vec_t;
typedef vec_t		vec2_t[2];
typedef vec_t		vec3_t[3];
typedef vec_t		vec4_t[4];
typedef vec_t		quat_t[4];
typedef byte		rgba_t[4];	// unsigned byte colorpack
typedef byte		rgb_t[3];		// unsigned byte colorpack
typedef vec_t		matrix3x4[3][4];
typedef vec_t		matrix4x4[4][4];
#if _MSC_VER == 1200
typedef __int64 integer64; //msvc6
#elif defined (XASH_SDL)
typedef Uint64 integer64;
#else
typedef unsigned long long integer64;
#endif
typedef integer64 longtime_t;
