/*
gl_export.h - opengl definition
Copyright (C) 2007 Uncle Mike

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/

#ifndef GL_EXPORT_H
#define GL_EXPORT_H

#ifdef XASH_NANOGL

#include "platform/android/gl_mangle.h"
#undef pglMultiTexCoord2f

#include <GL/gl.h>
#if defined XASH_WES && defined __EMSCRIPTEN__
GLvoid glDrawRangeElements( GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const GLvoid *indices );
#endif


#define GL_TEXTURE0_ARB                     0x84C0
#define GL_TEXTURE1_ARB                     0x84C1

#define glActiveTextureARB glActiveTexture
#define glClientActiveTextureARB glClientActiveTexture
#define glTexEnvi glTexEnvf

#define GL_TEXTURE_COMPARE_MODE_ARB       0x884C
#define GL_TEXTURE_COMPARE_FUNC_ARB       0x884D
#define GL_COMPARE_R_TO_TEXTURE_ARB       0x884E
#define GL_TEXTURE_CUBE_MAP_ARB           0x8513
#define GL_BGRA					0x80E1
#define GL_TEXTURE_3D				0x806F
#define GL_DEPTH_TEXTURE_MODE_ARB         0x884B
#define GL_COMPRESSED_ALPHA_ARB           0x84E9
#define GL_COMPRESSED_LUMINANCE_ARB       0x84EA
#define GL_COMPRESSED_LUMINANCE_ALPHA_ARB 0x84EB
#define GL_COMPRESSED_INTENSITY_ARB       0x84EC
#define GL_COMPRESSED_RGB_ARB             0x84ED
#define GL_COMPRESSED_RGBA_ARB            0x84EE
#define GL_RGBA32F_ARB                    0x8814
#define GL_RGB32F_ARB                     0x8815
#define GL_ALPHA32F_ARB                   0x8816
#define GL_INTENSITY32F_ARB               0x8817
#define GL_LUMINANCE32F_ARB               0x8818
#define GL_LUMINANCE_ALPHA32F_ARB         0x8819
#define GL_RGBA16F_ARB                    0x881A
#define GL_RGB16F_ARB                     0x881B
#define GL_ALPHA16F_ARB                   0x881C
#define GL_INTENSITY16F_ARB               0x881D
#define GL_LUMINANCE16F_ARB               0x881E
#define GL_LUMINANCE_ALPHA16F_ARB         0x881F
#define GL_COMPRESSED_RGB_S3TC_DXT1_EXT   0x83F0
#define GL_COMPRESSED_RGBA_S3TC_DXT1_EXT  0x83F1
#define GL_COMPRESSED_RGBA_S3TC_DXT3_EXT  0x83F2
#define GL_COMPRESSED_RGBA_S3TC_DXT5_EXT  0x83F3

#define GL_GENERATE_MIPMAP                0x8191
#define GL_GENERATE_MIPMAP_HINT           0x8192

#define GL_TEXTURE_CUBE_MAP			0x8513
#define GL_TEXTURE_BINDING_CUBE_MAP		0x8514
#define GL_TEXTURE_CUBE_MAP_POSITIVE_X		0x8515
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_X		0x8516
#define GL_TEXTURE_CUBE_MAP_POSITIVE_Y		0x8517
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_Y		0x8518
#define GL_TEXTURE_CUBE_MAP_POSITIVE_Z		0x8519
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_Z		0x851A
#define GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB 0x8515
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_X_ARB 0x8516
#define GL_TEXTURE_CUBE_MAP_POSITIVE_Y_ARB 0x8517
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_ARB 0x8518
#define GL_TEXTURE_CUBE_MAP_POSITIVE_Z_ARB 0x8519
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB 0x851A
#define GL_TEXTURE_RECTANGLE_EXT		0x84F5

#define GL_DEPTH_COMPONENT16			0x81A5
#define GL_DEPTH_COMPONENT24			0x81A6
#define GL_DEPTH_COMPONENT32			0x81A7
#define GL_DEPTH_COMPONENT32F	0x8CAC

#define GL_MAX_VERTEX_UNIFORM_COMPONENTS_ARB	0x8B4A
#define GL_MAX_VERTEX_ATTRIBS_ARB		0x8869


#define GL_GENERATE_MIPMAP_SGIS           0x8191
#define GL_GENERATE_MIPMAP_HINT_SGIS      0x8192

#define GL_TEXTURE_MAX_ANISOTROPY_EXT     0x84FE
#define GL_CLAMP_TO_BORDER			0x812D
#define GL_TEXTURE_WRAP_R			0x8072
#define GL_TEXTURE_LOD_BIAS_EXT           0x8501

#define GL_ADD_SIGNED				0x8574

#define GL_MAX_RECTANGLE_TEXTURE_SIZE_NV  0x84F8
#define GL_TEXTURE_RECTANGLE_NV           0x84F5
#define GL_MAX_TEXTURE_IMAGE_UNITS_ARB    0x8872
#define GL_MAX_TEXTURE_COORDS_ARB         0x8871
#define GL_MAX_3D_TEXTURE_SIZE_EXT        0x8073
#define GL_MAX_CUBE_MAP_TEXTURE_SIZE_ARB  0x851C
#define GL_MAX_TEXTURE_UNITS_ARB          0x84E2

#define GL_MAX_TEXTURE_LOD_BIAS_EXT       0x84FD
#define GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT 0x84FF
#define GL_MAX_3D_TEXTURE_SIZE            0x8073

#define GL_COMBINE_ARB			0x8570
#define GL_COMBINE_RGB_ARB			0x8571
#define GL_COMBINE_ALPHA_ARB			0x8572
#define GL_SOURCE0_RGB_ARB			0x8580
#define GL_SOURCE1_RGB_ARB			0x8581
#define GL_SOURCE2_RGB_ARB			0x8582
#define GL_SOURCE0_ALPHA_ARB			0x8588
#define GL_SOURCE1_ALPHA_ARB			0x8589
#define GL_SOURCE2_ALPHA_ARB			0x858A
#define GL_OPERAND0_RGB_ARB			0x8590
#define GL_OPERAND1_RGB_ARB			0x8591
#define GL_OPERAND2_RGB_ARB			0x8592
#define GL_OPERAND0_ALPHA_ARB			0x8598
#define GL_OPERAND1_ALPHA_ARB			0x8599
#define GL_OPERAND2_ALPHA_ARB			0x859A
#define GL_RGB_SCALE_ARB			0x8573
#define GL_ADD_SIGNED_ARB			0x8574
#define GL_INTERPOLATE_ARB			0x8575
#define GL_SUBTRACT_ARB			0x84E7
#define GL_CONSTANT_ARB			0x8576
#define GL_PRIMARY_COLOR_ARB			0x8577
#define GL_PREVIOUS_ARB			0x8578

#else
#ifndef APIENTRY
#define APIENTRY
#endif

typedef uint GLenum;
typedef byte GLboolean;
typedef uint GLbitfield;
typedef void GLvoid;
typedef signed char GLbyte;
typedef short GLshort;
typedef int GLint;
typedef byte GLubyte;
typedef word GLushort;
typedef uint GLuint;
typedef int GLsizei;
typedef float GLfloat;
typedef float GLclampf;
typedef double GLdouble;
typedef double GLclampd;
typedef int GLintptrARB;
typedef int GLsizeiptrARB;
typedef char GLcharARB;
typedef uint GLhandleARB;
typedef float GLmatrix[16];

#define GL_MODELVIEW			0x1700
#define GL_PROJECTION			0x1701
#define GL_TEXTURE				0x1702
#define GL_MATRIX_MODE			0x0BA0
#define GL_MODELVIEW_MATRIX			0x0BA6
#define GL_PROJECTION_MATRIX			0x0BA7
#define GL_TEXTURE_MATRIX			0x0BA8

#define GL_DONT_CARE			0x1100
#define GL_FASTEST				0x1101
#define GL_NICEST				0x1102

#define GL_DEPTH_TEST			0x0B71

#define GL_CULL_FACE			0x0B44
#define GL_CW				0x0900
#define GL_CCW				0x0901
#define GL_BLEND				0x0BE2
#define GL_ALPHA_TEST			0x0BC0

// shading model
#define GL_FLAT				0x1D00
#define GL_SMOOTH				0x1D01

#define GL_ZERO				0x0
#define GL_ONE				0x1
#define GL_SRC_COLOR			0x0300
#define GL_ONE_MINUS_SRC_COLOR		0x0301
#define GL_DST_COLOR			0x0306
#define GL_ONE_MINUS_DST_COLOR		0x0307
#define GL_SRC_ALPHA			0x0302
#define GL_ONE_MINUS_SRC_ALPHA		0x0303
#define GL_DST_ALPHA			0x0304
#define GL_ONE_MINUS_DST_ALPHA		0x0305
#define GL_SRC_ALPHA_SATURATE			0x0308
#define GL_CONSTANT_COLOR			0x8001
#define GL_ONE_MINUS_CONSTANT_COLOR		0x8002
#define GL_CONSTANT_ALPHA			0x8003
#define GL_ONE_MINUS_CONSTANT_ALPHA		0x8004

#define GL_TEXTURE_ENV			0x2300
#define GL_TEXTURE_ENV_MODE			0x2200
#define GL_TEXTURE_ENV_COLOR			0x2201
#define GL_TEXTURE_1D			0x0DE0
#define GL_TEXTURE_2D			0x0DE1
#define GL_TEXTURE_WRAP_S			0x2802
#define GL_TEXTURE_WRAP_T			0x2803
#define GL_TEXTURE_WRAP_R			0x8072
#define GL_TEXTURE_BORDER_COLOR		0x1004
#define GL_TEXTURE_MAG_FILTER			0x2800
#define GL_TEXTURE_MIN_FILTER			0x2801
#define GL_PACK_ALIGNMENT			0x0D05
#define GL_UNPACK_ALIGNMENT			0x0CF5
#define GL_TEXTURE_BINDING_1D			0x8068
#define GL_TEXTURE_BINDING_2D			0x8069
#define GL_CLAMP_TO_EDGE                  	0x812F
#define GL_CLAMP_TO_BORDER                	0x812D
#define GL_NEAREST				0x2600
#define GL_LINEAR				0x2601
#define GL_NEAREST_MIPMAP_NEAREST		0x2700
#define GL_NEAREST_MIPMAP_LINEAR		0x2702
#define GL_LINEAR_MIPMAP_NEAREST		0x2701
#define GL_LINEAR_MIPMAP_LINEAR		0x2703

#define GL_LINE				0x1B01
#define GL_FILL				0x1B02

#define GL_TEXTURE_MAX_ANISOTROPY_EXT		0x84FE
#define GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT	0x84FF

#define GL_MAX_TEXTURE_LOD_BIAS_EXT		0x84FD
#define GL_TEXTURE_FILTER_CONTROL_EXT		0x8500
#define GL_TEXTURE_LOD_BIAS_EXT		0x8501

#define GL_CLAMP_TO_BORDER_ARB		0x812D

#define GL_ADD				0x0104
#define GL_DECAL				0x2101
#define GL_MODULATE				0x2100

#define GL_REPEAT				0x2901
#define GL_CLAMP				0x2900

#define GL_POINTS				0x0000
#define GL_LINES				0x0001
#define GL_LINE_LOOP			0x0002
#define GL_LINE_STRIP			0x0003
#define GL_TRIANGLES			0x0004
#define GL_TRIANGLE_STRIP			0x0005
#define GL_TRIANGLE_FAN			0x0006
#define GL_QUADS				0x0007
#define GL_QUAD_STRIP			0x0008
#define GL_POLYGON				0x0009

#define GL_FALSE				0x0
#define GL_TRUE				0x1

#define GL_BYTE				0x1400
#define GL_UNSIGNED_BYTE			0x1401
#define GL_SHORT				0x1402
#define GL_UNSIGNED_SHORT			0x1403
#define GL_INT				0x1404
#define GL_UNSIGNED_INT			0x1405
#define GL_FLOAT				0x1406
#define GL_DOUBLE				0x140A
#define GL_2_BYTES				0x1407
#define GL_3_BYTES				0x1408
#define GL_4_BYTES				0x1409

#define GL_VERTEX_ARRAY			0x8074
#define GL_NORMAL_ARRAY			0x8075
#define GL_COLOR_ARRAY			0x8076
#define GL_INDEX_ARRAY			0x8077
#define GL_TEXTURE_COORD_ARRAY		0x8078
#define GL_EDGE_FLAG_ARRAY			0x8079

#define GL_NONE				0
#define GL_FRONT_LEFT			0x0400
#define GL_FRONT_RIGHT			0x0401
#define GL_BACK_LEFT			0x0402
#define GL_BACK_RIGHT			0x0403
#define GL_FRONT				0x0404
#define GL_BACK				0x0405
#define GL_LEFT				0x0406
#define GL_RIGHT				0x0407
#define GL_FRONT_AND_BACK			0x0408
#define GL_AUX0				0x0409
#define GL_AUX1				0x040A
#define GL_AUX2				0x040B
#define GL_AUX3				0x040C

#define GL_VENDOR				0x1F00
#define GL_RENDERER				0x1F01
#define GL_VERSION				0x1F02
#define GL_EXTENSIONS			0x1F03

#define GL_NO_ERROR 			0x0
#define GL_INVALID_VALUE			0x0501
#define GL_INVALID_ENUM			0x0500
#define GL_INVALID_OPERATION			0x0502
#define GL_STACK_OVERFLOW			0x0503
#define GL_STACK_UNDERFLOW			0x0504
#define GL_OUT_OF_MEMORY			0x0505

#define GL_DITHER				0x0BD0
#define GL_RGB				0x1907
#define GL_RGBA				0x1908
#define GL_BGR				0x80E0
#define GL_BGRA				0x80E1
#define GL_ALPHA4                         	0x803B
#define GL_ALPHA8                         	0x803C
#define GL_ALPHA12                        	0x803D
#define GL_ALPHA16                        	0x803E
#define GL_LUMINANCE4                     	0x803F
#define GL_LUMINANCE8                     	0x8040
#define GL_LUMINANCE12                    	0x8041
#define GL_LUMINANCE16                    	0x8042
#define GL_LUMINANCE4_ALPHA4              	0x8043
#define GL_LUMINANCE6_ALPHA2              	0x8044
#define GL_LUMINANCE8_ALPHA8              	0x8045
#define GL_LUMINANCE12_ALPHA4             	0x8046
#define GL_LUMINANCE12_ALPHA12            	0x8047
#define GL_LUMINANCE16_ALPHA16		0x8048
#define GL_LUMINANCE			0x1909
#define GL_LUMINANCE_ALPHA			0x190A
#define GL_DEPTH_COMPONENT			0x1902
#define GL_INTENSITY                      	0x8049
#define GL_INTENSITY4                     	0x804A
#define GL_INTENSITY8                     	0x804B
#define GL_INTENSITY12                    	0x804C
#define GL_INTENSITY16                    	0x804D
#define GL_R3_G3_B2                       	0x2A10
#define GL_RGB4                           	0x804F
#define GL_RGB5                           	0x8050
#define GL_RGB8                           	0x8051
#define GL_RGB10                          	0x8052
#define GL_RGB12                          	0x8053
#define GL_RGB16                          	0x8054
#define GL_RGBA2                          	0x8055
#define GL_RGBA4                          	0x8056
#define GL_RGB5_A1                        	0x8057
#define GL_RGBA8                          	0x8058
#define GL_RGB10_A2                       	0x8059
#define GL_RGBA12                         	0x805A
#define GL_RGBA16                         	0x805B
#define GL_TEXTURE_RED_SIZE               	0x805C
#define GL_TEXTURE_GREEN_SIZE             	0x805D
#define GL_TEXTURE_BLUE_SIZE              	0x805E
#define GL_TEXTURE_ALPHA_SIZE             	0x805F
#define GL_TEXTURE_LUMINANCE_SIZE         	0x8060
#define GL_TEXTURE_INTENSITY_SIZE         	0x8061
#define GL_PROXY_TEXTURE_1D               	0x8063
#define GL_PROXY_TEXTURE_2D               	0x8064
#define GL_MAX_TEXTURE_SIZE			0x0D33

// texture coord name
#define GL_S				0x2000
#define GL_T				0x2001
#define GL_R				0x2002
#define GL_Q				0x2003

// texture gen mode
#define GL_EYE_LINEAR			0x2400
#define GL_OBJECT_LINEAR			0x2401
#define GL_SPHERE_MAP			0x2402

// texture gen parameter
#define GL_TEXTURE_GEN_MODE			0x2500
#define GL_OBJECT_PLANE			0x2501
#define GL_EYE_PLANE			0x2502
#define GL_FOG_HINT				0x0C54
#define GL_TEXTURE_GEN_S			0x0C60
#define GL_TEXTURE_GEN_T			0x0C61
#define GL_TEXTURE_GEN_R			0x0C62
#define GL_TEXTURE_GEN_Q			0x0C63

#define GL_SCISSOR_BOX			0x0C10
#define GL_SCISSOR_TEST			0x0C11

#define GL_NEVER				0x0200
#define GL_LESS				0x0201
#define GL_EQUAL				0x0202
#define GL_LEQUAL				0x0203
#define GL_GREATER				0x0204
#define GL_NOTEQUAL				0x0205
#define GL_GEQUAL				0x0206
#define GL_ALWAYS				0x0207
#define GL_DEPTH_TEST			0x0B71

#define GL_RED_SCALE			0x0D14
#define GL_GREEN_SCALE			0x0D18
#define GL_BLUE_SCALE			0x0D1A
#define GL_ALPHA_SCALE			0x0D1C

/* AttribMask */
#define GL_CURRENT_BIT			0x00000001
#define GL_POINT_BIT			0x00000002
#define GL_LINE_BIT				0x00000004
#define GL_POLYGON_BIT			0x00000008
#define GL_POLYGON_STIPPLE_BIT		0x00000010
#define GL_PIXEL_MODE_BIT			0x00000020
#define GL_LIGHTING_BIT			0x00000040
#define GL_FOG_BIT				0x00000080
#define GL_DEPTH_BUFFER_BIT			0x00000100
#define GL_ACCUM_BUFFER_BIT			0x00000200
#define GL_STENCIL_BUFFER_BIT			0x00000400
#define GL_VIEWPORT_BIT			0x00000800
#define GL_TRANSFORM_BIT			0x00001000
#define GL_ENABLE_BIT			0x00002000
#define GL_COLOR_BUFFER_BIT			0x00004000
#define GL_HINT_BIT				0x00008000
#define GL_EVAL_BIT				0x00010000
#define GL_LIST_BIT				0x00020000
#define GL_TEXTURE_BIT			0x00040000
#define GL_SCISSOR_BIT			0x00080000
#define GL_ALL_ATTRIB_BITS			0x000fffff

#define GL_STENCIL_TEST			0x0B90
#define GL_KEEP				0x1E00
#define GL_REPLACE				0x1E01
#define GL_INCR				0x1E02
#define GL_DECR				0x1E03

// fog stuff
#define GL_FOG				0x0B60
#define GL_FOG_INDEX			0x0B61
#define GL_FOG_DENSITY			0x0B62
#define GL_FOG_START			0x0B63
#define GL_FOG_END				0x0B64
#define GL_FOG_MODE				0x0B65
#define GL_FOG_COLOR			0x0B66
#define GL_EXP				0x0800
#define GL_EXP2				0x0801

#define GL_POLYGON_OFFSET_FACTOR		0x8038
#define GL_POLYGON_OFFSET_UNITS		0x2A00
#define GL_POLYGON_OFFSET_POINT		0x2A01
#define GL_POLYGON_OFFSET_LINE		0x2A02
#define GL_POLYGON_OFFSET_FILL		0x8037

#define GL_POINT_SMOOTH			0x0B10
#define GL_LINE_SMOOTH			0x0B20
#define GL_POLYGON_SMOOTH			0x0B41
#define GL_POLYGON_STIPPLE			0x0B42
#define GL_CLIP_PLANE0			0x3000
#define GL_CLIP_PLANE1			0x3001
#define GL_CLIP_PLANE2			0x3002
#define GL_CLIP_PLANE3			0x3003
#define GL_CLIP_PLANE4			0x3004
#define GL_CLIP_PLANE5			0x3005
#define GL_POINT_SIZE_MIN_EXT			0x8126
#define GL_POINT_SIZE_MAX_EXT			0x8127
#define GL_POINT_FADE_THRESHOLD_SIZE_EXT	0x8128
#define GL_DISTANCE_ATTENUATION_EXT		0x8129
#define GL_ACTIVE_TEXTURE_ARB			0x84E0
#define GL_CLIENT_ACTIVE_TEXTURE_ARB		0x84E1
#define GL_MAX_TEXTURE_UNITS_ARB		0x84E2
#define GL_TEXTURE0_ARB			0x84C0
#define GL_TEXTURE1_ARB			0x84C1
#define GL_TEXTURE2_ARB                   	0x84C2
#define GL_TEXTURE0_SGIS			0x835E
#define GL_TEXTURE1_SGIS			0x835F
#define GL_GENERATE_MIPMAP_SGIS           	0x8191
#define GL_GENERATE_MIPMAP_HINT_SGIS      	0x8192
#define GL_TEXTURE_RECTANGLE_NV		0x84F5
#define GL_TEXTURE_BINDING_RECTANGLE_NV		0x84F6
#define GL_PROXY_TEXTURE_RECTANGLE_NV		0x84F7
#define GL_MAX_RECTANGLE_TEXTURE_SIZE_NV	0x84F8
#define GL_TEXTURE_RECTANGLE_EXT		0x84F5
#define GL_TEXTURE_BINDING_RECTANGLE_EXT	0x84F6
#define GL_PROXY_TEXTURE_RECTANGLE_EXT		0x84F7
#define GL_MAX_RECTANGLE_TEXTURE_SIZE_EXT	0x84F8
#define GL_MAX_TEXTURE_UNITS			0x84E2
#define GL_MAX_TEXTURE_UNITS_ARB		0x84E2

#define GL_DEPTH_COMPONENT16			0x81A5
#define GL_DEPTH_COMPONENT24			0x81A6
#define GL_DEPTH_COMPONENT32			0x81A7
#define GL_DEPTH_COMPONENT32F	0x8CAC
#define GL_DEPTH32F_STENCIL8	0x8CAD
#define GL_FLOAT_32_UNSIGNED_INT_24_8_REV	0x8DAD

#define GL_COMPRESSED_RGB_S3TC_DXT1_EXT		0x83F0
#define GL_COMPRESSED_RGBA_S3TC_DXT1_EXT	0x83F1
#define GL_COMPRESSED_RGBA_S3TC_DXT3_EXT	0x83F2
#define GL_COMPRESSED_RGBA_S3TC_DXT5_EXT	0x83F3
#define GL_COMPRESSED_ALPHA_ARB		0x84E9
#define GL_COMPRESSED_LUMINANCE_ARB		0x84EA
#define GL_COMPRESSED_LUMINANCE_ALPHA_ARB	0x84EB
#define GL_COMPRESSED_INTENSITY_ARB		0x84EC
#define GL_COMPRESSED_RGB_ARB			0x84ED
#define GL_COMPRESSED_RGBA_ARB		0x84EE
#define GL_TEXTURE_COMPRESSION_HINT_ARB		0x84EF
#define GL_TEXTURE_COMPRESSED_IMAGE_SIZE_ARB	0x86A0
#define GL_TEXTURE_COMPRESSED_ARB		0x86A1
#define GL_NUM_COMPRESSED_TEXTURE_FORMATS_ARB	0x86A2
#define GL_COMPRESSED_TEXTURE_FORMATS_ARB	0x86A3
#define GL_UNSIGNED_BYTE_2_3_3_REV		0x8362
#define GL_UNSIGNED_SHORT_5_6_5		0x8363
#define GL_UNSIGNED_SHORT_5_6_5_REV		0x8364
#define GL_UNSIGNED_SHORT_4_4_4_4_REV		0x8365
#define GL_UNSIGNED_SHORT_1_5_5_5_REV		0x8366
#define GL_UNSIGNED_INT_8_8_8_8_REV		0x8367
#define GL_UNSIGNED_INT_2_10_10_10_REV		0x8368
#define GL_TEXTURE_MAX_LEVEL			0x813D
#define GL_GENERATE_MIPMAP			0x8191
#define GL_ADD_SIGNED			0x8574

#define GL_PROGRAM_OBJECT_ARB			0x8B40
#define GL_OBJECT_TYPE_ARB			0x8B4E
#define GL_OBJECT_SUBTYPE_ARB			0x8B4F
#define GL_OBJECT_DELETE_STATUS_ARB		0x8B80
#define GL_OBJECT_COMPILE_STATUS_ARB		0x8B81
#define GL_OBJECT_LINK_STATUS_ARB		0x8B82
#define GL_OBJECT_VALIDATE_STATUS_ARB		0x8B83
#define GL_OBJECT_INFO_LOG_LENGTH_ARB		0x8B84
#define GL_OBJECT_ATTACHED_OBJECTS_ARB		0x8B85
#define GL_OBJECT_ACTIVE_UNIFORMS_ARB		0x8B86
#define GL_OBJECT_ACTIVE_UNIFORM_MAX_LENGTH_ARB	0x8B87
#define GL_OBJECT_SHADER_SOURCE_LENGTH_ARB	0x8B88
#define GL_SHADER_OBJECT_ARB			0x8B48
#define GL_FLOAT_VEC2_ARB			0x8B50
#define GL_FLOAT_VEC3_ARB			0x8B51
#define GL_FLOAT_VEC4_ARB			0x8B52
#define GL_INT_VEC2_ARB			0x8B53
#define GL_INT_VEC3_ARB			0x8B54
#define GL_INT_VEC4_ARB			0x8B55
#define GL_BOOL_ARB				0x8B56
#define GL_BOOL_VEC2_ARB			0x8B57
#define GL_BOOL_VEC3_ARB			0x8B58
#define GL_BOOL_VEC4_ARB			0x8B59
#define GL_FLOAT_MAT2_ARB			0x8B5A
#define GL_FLOAT_MAT3_ARB			0x8B5B
#define GL_FLOAT_MAT4_ARB			0x8B5C
#define GL_SAMPLER_1D_ARB			0x8B5D
#define GL_SAMPLER_2D_ARB			0x8B5E
#define GL_SAMPLER_3D_ARB			0x8B5F
#define GL_SAMPLER_CUBE_ARB			0x8B60
#define GL_SAMPLER_1D_SHADOW_ARB		0x8B61
#define GL_SAMPLER_2D_SHADOW_ARB		0x8B62
#define GL_SAMPLER_2D_RECT_ARB		0x8B63
#define GL_SAMPLER_2D_RECT_SHADOW_ARB		0x8B64

#define GL_PACK_SKIP_IMAGES			0x806B
#define GL_PACK_IMAGE_HEIGHT			0x806C
#define GL_UNPACK_SKIP_IMAGES			0x806D
#define GL_UNPACK_IMAGE_HEIGHT		0x806E
#define GL_TEXTURE_3D			0x806F
#define GL_PROXY_TEXTURE_3D			0x8070
#define GL_TEXTURE_DEPTH			0x8071
#define GL_TEXTURE_WRAP_R			0x8072
#define GL_MAX_3D_TEXTURE_SIZE		0x8073
#define GL_TEXTURE_BINDING_3D			0x806A

/* multisample */
#define GL_MULTISAMPLE				0x809D
#define GL_SAMPLE_ALPHA_TO_COVERAGE		0x809E
#define GL_SAMPLE_ALPHA_TO_ONE			0x809F
#define GL_SAMPLE_COVERAGE			0x80A0
#define GL_SAMPLE_BUFFERS			0x80A8
#define GL_SAMPLES				0x80A9
#define GL_SAMPLE_COVERAGE_VALUE		0x80AA
#define GL_SAMPLE_COVERAGE_INVERT		0x80AB
#define GL_MULTISAMPLE_BIT			0x20000000


#define GL_STENCIL_TEST_TWO_SIDE_EXT		0x8910
#define GL_ACTIVE_STENCIL_FACE_EXT		0x8911
#define GL_STENCIL_BACK_FUNC              	0x8800
#define GL_STENCIL_BACK_FAIL              	0x8801
#define GL_STENCIL_BACK_PASS_DEPTH_FAIL   	0x8802
#define GL_STENCIL_BACK_PASS_DEPTH_PASS   	0x8803

#define GL_DEPTH_TEXTURE_MODE_ARB		0x884B
#define GL_TEXTURE_COMPARE_MODE_ARB		0x884C
#define GL_TEXTURE_COMPARE_FUNC_ARB		0x884D
#define GL_COMPARE_R_TO_TEXTURE_ARB		0x884E
#define GL_TEXTURE_COMPARE_FAIL_VALUE_ARB	0x80BF

#define GL_QUERY_COUNTER_BITS_ARB		0x8864
#define GL_CURRENT_QUERY_ARB			0x8865
#define GL_QUERY_RESULT_ARB			0x8866
#define GL_QUERY_RESULT_AVAILABLE_ARB		0x8867
#define GL_SAMPLES_PASSED_ARB			0x8914

#define GL_FUNC_ADD_EXT			0x8006
#define GL_FUNC_SUBTRACT_EXT			0x800A
#define GL_FUNC_REVERSE_SUBTRACT_EXT		0x800B
#define GL_MIN_EXT				0x8007
#define GL_MAX_EXT				0x8008
#define GL_BLEND_EQUATION_EXT			0x8009

#define GL_VERTEX_SHADER_ARB			0x8B31
#define GL_MAX_VERTEX_UNIFORM_COMPONENTS_ARB	0x8B4A
#define GL_MAX_VARYING_FLOATS_ARB		0x8B4B
#define GL_MAX_VERTEX_ATTRIBS_ARB		0x8869
#define GL_MAX_TEXTURE_IMAGE_UNITS_ARB		0x8872
#define GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS_ARB	0x8B4C
#define GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS_ARB	0x8B4D
#define GL_MAX_TEXTURE_COORDS_ARB		0x8871
#define GL_VERTEX_PROGRAM_POINT_SIZE_ARB	0x8642
#define GL_VERTEX_PROGRAM_TWO_SIDE_ARB		0x8643
#define GL_OBJECT_ACTIVE_ATTRIBUTES_ARB		0x8B89
#define GL_OBJECT_ACTIVE_ATTRIBUTE_MAX_LENGTH_ARB	0x8B8A
#define GL_VERTEX_ATTRIB_ARRAY_ENABLED_ARB	0x8622
#define GL_VERTEX_ATTRIB_ARRAY_SIZE_ARB		0x8623
#define GL_VERTEX_ATTRIB_ARRAY_STRIDE_ARB	0x8624
#define GL_VERTEX_ATTRIB_ARRAY_TYPE_ARB		0x8625
#define GL_VERTEX_ATTRIB_ARRAY_NORMALIZED_ARB	0x886A
#define GL_CURRENT_VERTEX_ATTRIB_ARB		0x8626
#define GL_VERTEX_ATTRIB_ARRAY_POINTER_ARB	0x8645
#define GL_FLOAT_VEC2_ARB			0x8B50
#define GL_FLOAT_VEC3_ARB			0x8B51
#define GL_FLOAT_VEC4_ARB			0x8B52
#define GL_FLOAT_MAT2_ARB			0x8B5A
#define GL_FLOAT_MAT3_ARB			0x8B5B
#define GL_FLOAT_MAT4_ARB			0x8B5C

#define GL_FLOAT_R_NV			0x8880
#define GL_FLOAT_RG_NV			0x8881
#define GL_FLOAT_RGB_NV			0x8882
#define GL_FLOAT_RGBA_NV			0x8883
#define GL_FLOAT_R16_NV			0x8884
#define GL_FLOAT_R32_NV			0x8885
#define GL_FLOAT_RG16_NV			0x8886
#define GL_FLOAT_RG32_NV			0x8887
#define GL_FLOAT_RGB16_NV			0x8888
#define GL_FLOAT_RGB32_NV			0x8889
#define GL_FLOAT_RGBA16_NV			0x888A
#define GL_FLOAT_RGBA32_NV			0x888B
#define GL_TEXTURE_FLOAT_COMPONENTS_NV		0x888C
#define GL_FLOAT_CLEAR_COLOR_VALUE_NV		0x888D
#define GL_FLOAT_RGBA_MODE_NV			0x888E

#define GL_FRAGMENT_SHADER_ARB		0x8B30
#define GL_MAX_FRAGMENT_UNIFORM_COMPONENTS_ARB	0x8B49
#define GL_MAX_TEXTURE_COORDS_ARB		0x8871
#define GL_MAX_TEXTURE_IMAGE_UNITS_ARB		0x8872
#define GL_FRAGMENT_SHADER_DERIVATIVE_HINT_ARB	0x8B8B

#define GL_TEXTURE_RED_TYPE_ARB                   0x8C10
#define GL_TEXTURE_GREEN_TYPE_ARB                 0x8C11
#define GL_TEXTURE_BLUE_TYPE_ARB                  0x8C12
#define GL_TEXTURE_ALPHA_TYPE_ARB                 0x8C13
#define GL_TEXTURE_LUMINANCE_TYPE_ARB             0x8C14
#define GL_TEXTURE_INTENSITY_TYPE_ARB             0x8C15
#define GL_TEXTURE_DEPTH_TYPE_ARB                 0x8C16
#define GL_UNSIGNED_NORMALIZED_ARB                0x8C17
#define GL_RGBA32F_ARB                            0x8814
#define GL_RGB32F_ARB                             0x8815
#define GL_ALPHA32F_ARB                           0x8816
#define GL_INTENSITY32F_ARB                       0x8817
#define GL_LUMINANCE32F_ARB                       0x8818
#define GL_LUMINANCE_ALPHA32F_ARB                 0x8819
#define GL_RGBA16F_ARB                            0x881A
#define GL_RGB16F_ARB                             0x881B
#define GL_ALPHA16F_ARB                           0x881C
#define GL_INTENSITY16F_ARB                       0x881D
#define GL_LUMINANCE16F_ARB                       0x881E
#define GL_LUMINANCE_ALPHA16F_ARB                 0x881F

#define GL_RGBA_FLOAT32_ATI                       0x8814
#define GL_RGB_FLOAT32_ATI                        0x8815
#define GL_ALPHA_FLOAT32_ATI                      0x8816
#define GL_INTENSITY_FLOAT32_ATI                  0x8817
#define GL_LUMINANCE_FLOAT32_ATI                  0x8818
#define GL_LUMINANCE_ALPHA_FLOAT32_ATI            0x8819
#define GL_RGBA_FLOAT16_ATI                       0x881A
#define GL_RGB_FLOAT16_ATI                        0x881B
#define GL_ALPHA_FLOAT16_ATI                      0x881C
#define GL_INTENSITY_FLOAT16_ATI                  0x881D
#define GL_LUMINANCE_FLOAT16_ATI                  0x881E
#define GL_LUMINANCE_ALPHA_FLOAT16_ATI            0x881F

//GL_ARB_vertex_buffer_object
#define GL_ARRAY_BUFFER_ARB			0x8892
#define GL_ELEMENT_ARRAY_BUFFER_ARB		0x8893
#define GL_ARRAY_BUFFER_BINDING_ARB		0x8894
#define GL_ELEMENT_ARRAY_BUFFER_BINDING_ARB	0x8895
#define GL_VERTEX_ARRAY_BUFFER_BINDING_ARB	0x8896
#define GL_NORMAL_ARRAY_BUFFER_BINDING_ARB	0x8897
#define GL_COLOR_ARRAY_BUFFER_BINDING_ARB	0x8898
#define GL_INDEX_ARRAY_BUFFER_BINDING_ARB	0x8899
#define GL_TEXTURE_COORD_ARRAY_BUFFER_BINDING_ARB	0x889A
#define GL_EDGE_FLAG_ARRAY_BUFFER_BINDING_ARB	0x889B
#define GL_WEIGHT_ARRAY_BUFFER_BINDING_ARB	0x889E
#define GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING_ARB	0x889F
#define GL_STREAM_DRAW_ARB			0x88E0
#define GL_STREAM_READ_ARB			0x88E1
#define GL_STREAM_COPY_ARB			0x88E2
#define GL_STATIC_DRAW_ARB			0x88E4
#define GL_STATIC_READ_ARB			0x88E5
#define GL_STATIC_COPY_ARB			0x88E6
#define GL_DYNAMIC_DRAW_ARB			0x88E8
#define GL_DYNAMIC_READ_ARB			0x88E9
#define GL_DYNAMIC_COPY_ARB			0x88EA
#define GL_READ_ONLY_ARB			0x88B8
#define GL_WRITE_ONLY_ARB			0x88B9
#define GL_READ_WRITE_ARB			0x88BA
#define GL_BUFFER_SIZE_ARB			0x8764
#define GL_BUFFER_USAGE_ARB			0x8765
#define GL_BUFFER_ACCESS_ARB			0x88BB
#define GL_BUFFER_MAPPED_ARB			0x88BC
#define GL_BUFFER_MAP_POINTER_ARB		0x88BD
#define GL_SECONDARY_COLOR_ARRAY_BUFFER_BINDING_ARB	0x889C
#define GL_FOG_COORDINATE_ARRAY_BUFFER_BINDING_ARB	0x889D

#define GL_NORMAL_MAP_ARB			0x8511
#define GL_REFLECTION_MAP_ARB			0x8512
#define GL_TEXTURE_CUBE_MAP_ARB		0x8513
#define GL_TEXTURE_BINDING_CUBE_MAP_ARB		0x8514
#define GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB	0x8515
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_X_ARB	0x8516
#define GL_TEXTURE_CUBE_MAP_POSITIVE_Y_ARB	0x8517
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_ARB	0x8518
#define GL_TEXTURE_CUBE_MAP_POSITIVE_Z_ARB	0x8519
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB	0x851A
#define GL_PROXY_TEXTURE_CUBE_MAP_ARB		0x851B
#define GL_MAX_CUBE_MAP_TEXTURE_SIZE_ARB	0x851C

#define GL_COMBINE_ARB			0x8570
#define GL_COMBINE_RGB_ARB			0x8571
#define GL_COMBINE_ALPHA_ARB			0x8572
#define GL_SOURCE0_RGB_ARB			0x8580
#define GL_SOURCE1_RGB_ARB			0x8581
#define GL_SOURCE2_RGB_ARB			0x8582
#define GL_SOURCE0_ALPHA_ARB			0x8588
#define GL_SOURCE1_ALPHA_ARB			0x8589
#define GL_SOURCE2_ALPHA_ARB			0x858A
#define GL_OPERAND0_RGB_ARB			0x8590
#define GL_OPERAND1_RGB_ARB			0x8591
#define GL_OPERAND2_RGB_ARB			0x8592
#define GL_OPERAND0_ALPHA_ARB			0x8598
#define GL_OPERAND1_ALPHA_ARB			0x8599
#define GL_OPERAND2_ALPHA_ARB			0x859A
#define GL_RGB_SCALE_ARB			0x8573
#define GL_ADD_SIGNED_ARB			0x8574
#define GL_INTERPOLATE_ARB			0x8575
#define GL_SUBTRACT_ARB			0x84E7
#define GL_CONSTANT_ARB			0x8576
#define GL_PRIMARY_COLOR_ARB			0x8577
#define GL_PREVIOUS_ARB			0x8578

#define GL_DOT3_RGB_ARB			0x86AE
#define GL_DOT3_RGBA_ARB			0x86AF

#define GL_MULTISAMPLE_ARB			0x809D
#define GL_SAMPLE_ALPHA_TO_COVERAGE_ARB		0x809E
#define GL_SAMPLE_ALPHA_TO_ONE_ARB		0x809F
#define GL_SAMPLE_COVERAGE_ARB		0x80A0
#define GL_SAMPLE_BUFFERS_ARB			0x80A8
#define GL_SAMPLES_ARB			0x80A9
#define GL_SAMPLE_COVERAGE_VALUE_ARB		0x80AA
#define GL_SAMPLE_COVERAGE_INVERT_ARB		0x80AB
#define GL_MULTISAMPLE_BIT_ARB		0x20000000

#define GL_COLOR_SUM_ARB			0x8458
#define GL_VERTEX_PROGRAM_ARB			0x8620
#define GL_VERTEX_ATTRIB_ARRAY_ENABLED_ARB	0x8622
#define GL_VERTEX_ATTRIB_ARRAY_SIZE_ARB		0x8623
#define GL_VERTEX_ATTRIB_ARRAY_STRIDE_ARB	0x8624
#define GL_VERTEX_ATTRIB_ARRAY_TYPE_ARB		0x8625
#define GL_CURRENT_VERTEX_ATTRIB_ARB		0x8626
#define GL_PROGRAM_LENGTH_ARB			0x8627
#define GL_PROGRAM_STRING_ARB			0x8628
#define GL_MAX_PROGRAM_MATRIX_STACK_DEPTH_ARB	0x862E
#define GL_MAX_PROGRAM_MATRICES_ARB		0x862F
#define GL_CURRENT_MATRIX_STACK_DEPTH_ARB	0x8640
#define GL_CURRENT_MATRIX_ARB			0x8641
#define GL_VERTEX_PROGRAM_POINT_SIZE_ARB	0x8642
#define GL_VERTEX_PROGRAM_TWO_SIDE_ARB		0x8643
#define GL_VERTEX_ATTRIB_ARRAY_POINTER_ARB	0x8645
#define GL_PROGRAM_ERROR_POSITION_ARB		0x864B
#define GL_PROGRAM_BINDING_ARB		0x8677
#define GL_MAX_VERTEX_ATTRIBS_ARB		0x8869
#define GL_VERTEX_ATTRIB_ARRAY_NORMALIZED_ARB	0x886A
#define GL_PROGRAM_ERROR_STRING_ARB		0x8874
#define GL_PROGRAM_FORMAT_ASCII_ARB		0x8875
#define GL_PROGRAM_FORMAT_ARB			0x8876
#define GL_PROGRAM_INSTRUCTIONS_ARB		0x88A0
#define GL_MAX_PROGRAM_INSTRUCTIONS_ARB		0x88A1
#define GL_PROGRAM_NATIVE_INSTRUCTIONS_ARB	0x88A2
#define GL_MAX_PROGRAM_NATIVE_INSTRUCTIONS_ARB	0x88A3
#define GL_PROGRAM_TEMPORARIES_ARB		0x88A4
#define GL_MAX_PROGRAM_TEMPORARIES_ARB		0x88A5
#define GL_PROGRAM_NATIVE_TEMPORARIES_ARB	0x88A6
#define GL_MAX_PROGRAM_NATIVE_TEMPORARIES_ARB	0x88A7
#define GL_PROGRAM_PARAMETERS_ARB		0x88A8
#define GL_MAX_PROGRAM_PARAMETERS_ARB		0x88A9
#define GL_PROGRAM_NATIVE_PARAMETERS_ARB	0x88AA
#define GL_MAX_PROGRAM_NATIVE_PARAMETERS_ARB	0x88AB
#define GL_PROGRAM_ATTRIBS_ARB		0x88AC
#define GL_MAX_PROGRAM_ATTRIBS_ARB		0x88AD
#define GL_PROGRAM_NATIVE_ATTRIBS_ARB		0x88AE
#define GL_MAX_PROGRAM_NATIVE_ATTRIBS_ARB	0x88AF
#define GL_PROGRAM_ADDRESS_REGISTERS_ARB	0x88B0
#define GL_MAX_PROGRAM_ADDRESS_REGISTERS_ARB	0x88B1
#define GL_PROGRAM_NATIVE_ADDRESS_REGISTERS_ARB	0x88B2
#define GL_MAX_PROGRAM_NATIVE_ADDRESS_REGISTERS_ARB	0x88B3
#define GL_MAX_PROGRAM_LOCAL_PARAMETERS_ARB	0x88B4
#define GL_MAX_PROGRAM_ENV_PARAMETERS_ARB	0x88B5
#define GL_PROGRAM_UNDER_NATIVE_LIMITS_ARB	0x88B6
#define GL_TRANSPOSE_CURRENT_MATRIX_ARB		0x88B7
#define GL_MATRIX0_ARB			0x88C0
#define GL_MATRIX1_ARB			0x88C1
#define GL_MATRIX2_ARB			0x88C2
#define GL_MATRIX3_ARB			0x88C3
#define GL_MATRIX4_ARB			0x88C4
#define GL_MATRIX5_ARB			0x88C5
#define GL_MATRIX6_ARB			0x88C6
#define GL_MATRIX7_ARB			0x88C7
#define GL_MATRIX8_ARB			0x88C8
#define GL_MATRIX9_ARB			0x88C9
#define GL_MATRIX10_ARB			0x88CA
#define GL_MATRIX11_ARB			0x88CB
#define GL_MATRIX12_ARB			0x88CC
#define GL_MATRIX13_ARB			0x88CD
#define GL_MATRIX14_ARB			0x88CE
#define GL_MATRIX15_ARB			0x88CF
#define GL_MATRIX16_ARB			0x88D0
#define GL_MATRIX17_ARB			0x88D1
#define GL_MATRIX18_ARB			0x88D2
#define GL_MATRIX19_ARB			0x88D3
#define GL_MATRIX20_ARB			0x88D4
#define GL_MATRIX21_ARB			0x88D5
#define GL_MATRIX22_ARB			0x88D6
#define GL_MATRIX23_ARB			0x88D7
#define GL_MATRIX24_ARB			0x88D8
#define GL_MATRIX25_ARB			0x88D9
#define GL_MATRIX26_ARB			0x88DA
#define GL_MATRIX27_ARB			0x88DB
#define GL_MATRIX28_ARB			0x88DC
#define GL_MATRIX29_ARB			0x88DD
#define GL_MATRIX30_ARB			0x88DE
#define GL_MATRIX31_ARB			0x88DF
#define GL_FRAGMENT_PROGRAM_ARB		0x8804
#define GL_PROGRAM_ALU_INSTRUCTIONS_ARB		0x8805
#define GL_PROGRAM_TEX_INSTRUCTIONS_ARB		0x8806
#define GL_PROGRAM_TEX_INDIRECTIONS_ARB		0x8807
#define GL_PROGRAM_NATIVE_ALU_INSTRUCTIONS_ARB	0x8808
#define GL_PROGRAM_NATIVE_TEX_INSTRUCTIONS_ARB	0x8809
#define GL_PROGRAM_NATIVE_TEX_INDIRECTIONS_ARB	0x880A
#define GL_MAX_PROGRAM_ALU_INSTRUCTIONS_ARB	0x880B
#define GL_MAX_PROGRAM_TEX_INSTRUCTIONS_ARB	0x880C
#define GL_MAX_PROGRAM_TEX_INDIRECTIONS_ARB	0x880D
#define GL_MAX_PROGRAM_NATIVE_ALU_INSTRUCTIONS_ARB	0x880E
#define GL_MAX_PROGRAM_NATIVE_TEX_INSTRUCTIONS_ARB	0x880F
#define GL_MAX_PROGRAM_NATIVE_TEX_INDIRECTIONS_ARB	0x8810
#define GL_MAX_TEXTURE_COORDS_ARB		0x8871
#define GL_MAX_TEXTURE_IMAGE_UNITS_ARB		0x8872

#define WGL_CONTEXT_MAJOR_VERSION_ARB		0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB		0x2092
#define WGL_CONTEXT_LAYER_PLANE_ARB		0x2093
#define WGL_CONTEXT_FLAGS_ARB			0x2094
#define WGL_CONTEXT_DEBUG_BIT_ARB		0x0001
#define WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB	0x0002
#define WGL_CONTEXT_PROFILE_MASK_ARB		0x9126
#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB	0x00000001
#define WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB	0x00000002
#define WGL_CONTEXT_ES2_PROFILE_BIT_EXT		0x00000004	/*WGL_CONTEXT_ES2_PROFILE_BIT_EXT*/
#define ERROR_INVALID_VERSION_ARB		0x2095
#define ERROR_INVALID_PROFILE_ARB		0x2096

// helper opengl functions
extern GLenum ( APIENTRY *pglGetError )(void);
extern const GLubyte * ( APIENTRY *pglGetString )(GLenum name);

// base gl functions
extern void ( APIENTRY *pglAccum )(GLenum op, GLfloat value);
extern void ( APIENTRY *pglAlphaFunc )(GLenum func, GLclampf ref);
extern void ( APIENTRY *pglArrayElement )(GLint i);
extern void ( APIENTRY *pglBegin )(GLenum mode);
extern void ( APIENTRY *pglBindTexture )(GLenum target, GLuint texture);
extern void ( APIENTRY *pglBitmap )(GLsizei width, GLsizei height, GLfloat xorig, GLfloat yorig, GLfloat xmove, GLfloat ymove, const GLubyte *bitmap);
extern void ( APIENTRY *pglBlendFunc )(GLenum sfactor, GLenum dfactor);
extern void ( APIENTRY *pglCallList )(GLuint list);
extern void ( APIENTRY *pglCallLists )(GLsizei n, GLenum type, const GLvoid *lists);
extern void ( APIENTRY *pglClear )(GLbitfield mask);
extern void ( APIENTRY *pglClearAccum )(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
extern void ( APIENTRY *pglClearColor )(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
extern void ( APIENTRY *pglClearDepth )(GLclampd depth);
extern void ( APIENTRY *pglClearIndex )(GLfloat c);
extern void ( APIENTRY *pglClearStencil )(GLint s);
extern GLboolean ( APIENTRY *pglIsEnabled )( GLenum cap );
extern GLboolean ( APIENTRY *pglIsList )( GLuint list );
extern GLboolean ( APIENTRY *pglIsTexture )( GLuint texture );
extern void ( APIENTRY *pglClipPlane )(GLenum plane, const GLdouble *equation);
extern void ( APIENTRY *pglColor3b )(GLbyte red, GLbyte green, GLbyte blue);
extern void ( APIENTRY *pglColor3bv )(const GLbyte *v);
extern void ( APIENTRY *pglColor3d )(GLdouble red, GLdouble green, GLdouble blue);
extern void ( APIENTRY *pglColor3dv )(const GLdouble *v);
extern void ( APIENTRY *pglColor3f )(GLfloat red, GLfloat green, GLfloat blue);
extern void ( APIENTRY *pglColor3fv )(const GLfloat *v);
extern void ( APIENTRY *pglColor3i )(GLint red, GLint green, GLint blue);
extern void ( APIENTRY *pglColor3iv )(const GLint *v);
extern void ( APIENTRY *pglColor3s )(GLshort red, GLshort green, GLshort blue);
extern void ( APIENTRY *pglColor3sv )(const GLshort *v);
extern void ( APIENTRY *pglColor3ub )(GLubyte red, GLubyte green, GLubyte blue);
extern void ( APIENTRY *pglColor3ubv )(const GLubyte *v);
extern void ( APIENTRY *pglColor3ui )(GLuint red, GLuint green, GLuint blue);
extern void ( APIENTRY *pglColor3uiv )(const GLuint *v);
extern void ( APIENTRY *pglColor3us )(GLushort red, GLushort green, GLushort blue);
extern void ( APIENTRY *pglColor3usv )(const GLushort *v);
extern void ( APIENTRY *pglColor4b )(GLbyte red, GLbyte green, GLbyte blue, GLbyte alpha);
extern void ( APIENTRY *pglColor4bv )(const GLbyte *v);
extern void ( APIENTRY *pglColor4d )(GLdouble red, GLdouble green, GLdouble blue, GLdouble alpha);
extern void ( APIENTRY *pglColor4dv )(const GLdouble *v);
extern void ( APIENTRY *pglColor4f )(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
extern void ( APIENTRY *pglColor4fv )(const GLfloat *v);
extern void ( APIENTRY *pglColor4i )(GLint red, GLint green, GLint blue, GLint alpha);
extern void ( APIENTRY *pglColor4iv )(const GLint *v);
extern void ( APIENTRY *pglColor4s )(GLshort red, GLshort green, GLshort blue, GLshort alpha);
extern void ( APIENTRY *pglColor4sv )(const GLshort *v);
extern void ( APIENTRY *pglColor4ub )(GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha);
extern void ( APIENTRY *pglColor4ubv )(const GLubyte *v);
extern void ( APIENTRY *pglColor4ui )(GLuint red, GLuint green, GLuint blue, GLuint alpha);
extern void ( APIENTRY *pglColor4uiv )(const GLuint *v);
extern void ( APIENTRY *pglColor4us )(GLushort red, GLushort green, GLushort blue, GLushort alpha);
extern void ( APIENTRY *pglColor4usv )(const GLushort *v);
extern void ( APIENTRY *pglColorMask )(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha);
extern void ( APIENTRY *pglColorMaterial )(GLenum face, GLenum mode);
extern void ( APIENTRY *pglColorPointer )(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
extern void ( APIENTRY *pglCopyPixels )(GLint x, GLint y, GLsizei width, GLsizei height, GLenum type);
extern void ( APIENTRY *pglCopyTexImage1D )(GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y, GLsizei width, GLint border);
extern void ( APIENTRY *pglCopyTexImage2D )(GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border);
extern void ( APIENTRY *pglCopyTexSubImage1D )(GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width);
extern void ( APIENTRY *pglCopyTexSubImage2D )(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height);
extern void ( APIENTRY *pglCullFace )(GLenum mode);
extern void ( APIENTRY *pglDeleteLists )(GLuint list, GLsizei range);
extern void ( APIENTRY *pglDeleteTextures )(GLsizei n, const GLuint *textures);
extern void ( APIENTRY *pglDepthFunc )(GLenum func);
extern void ( APIENTRY *pglDepthMask )(GLboolean flag);
extern void ( APIENTRY *pglDepthRange )(GLclampd zNear, GLclampd zFar);
extern void ( APIENTRY *pglDisable )(GLenum cap);
extern void ( APIENTRY *pglDisableClientState )(GLenum array);
extern void ( APIENTRY *pglDrawArrays )(GLenum mode, GLint first, GLsizei count);
extern void ( APIENTRY *pglDrawBuffer )(GLenum mode);
extern void ( APIENTRY *pglDrawElements )(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices);
extern void ( APIENTRY *pglDrawPixels )(GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels);
extern void ( APIENTRY *pglEdgeFlag )(GLboolean flag);
extern void ( APIENTRY *pglEdgeFlagPointer )(GLsizei stride, const GLvoid *pointer);
extern void ( APIENTRY *pglEdgeFlagv )(const GLboolean *flag);
extern void ( APIENTRY *pglEnable )(GLenum cap);
extern void ( APIENTRY *pglEnableClientState )(GLenum array);
extern void ( APIENTRY *pglEnd )(void);
extern void ( APIENTRY *pglEndList )(void);
extern void ( APIENTRY *pglEvalCoord1d )(GLdouble u);
extern void ( APIENTRY *pglEvalCoord1dv )(const GLdouble *u);
extern void ( APIENTRY *pglEvalCoord1f )(GLfloat u);
extern void ( APIENTRY *pglEvalCoord1fv )(const GLfloat *u);
extern void ( APIENTRY *pglEvalCoord2d )(GLdouble u, GLdouble v);
extern void ( APIENTRY *pglEvalCoord2dv )(const GLdouble *u);
extern void ( APIENTRY *pglEvalCoord2f )(GLfloat u, GLfloat v);
extern void ( APIENTRY *pglEvalCoord2fv )(const GLfloat *u);
extern void ( APIENTRY *pglEvalMesh1 )(GLenum mode, GLint i1, GLint i2);
extern void ( APIENTRY *pglEvalMesh2 )(GLenum mode, GLint i1, GLint i2, GLint j1, GLint j2);
extern void ( APIENTRY *pglEvalPoint1 )(GLint i);
extern void ( APIENTRY *pglEvalPoint2 )(GLint i, GLint j);
extern void ( APIENTRY *pglFeedbackBuffer )(GLsizei size, GLenum type, GLfloat *buffer);
extern void ( APIENTRY *pglFinish )(void);
extern void ( APIENTRY *pglFlush )(void);
extern void ( APIENTRY *pglFogf )(GLenum pname, GLfloat param);
extern void ( APIENTRY *pglFogfv )(GLenum pname, const GLfloat *params);
extern void ( APIENTRY *pglFogi )(GLenum pname, GLint param);
extern void ( APIENTRY *pglFogiv )(GLenum pname, const GLint *params);
extern void ( APIENTRY *pglFrontFace )(GLenum mode);
extern void ( APIENTRY *pglFrustum )(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar);
extern void ( APIENTRY *pglGenTextures )(GLsizei n, GLuint *textures);
extern void ( APIENTRY *pglGetBooleanv )(GLenum pname, GLboolean *params);
extern void ( APIENTRY *pglGetClipPlane )(GLenum plane, GLdouble *equation);
extern void ( APIENTRY *pglGetDoublev )(GLenum pname, GLdouble *params);
extern void ( APIENTRY *pglGetFloatv )(GLenum pname, GLfloat *params);
extern void ( APIENTRY *pglGetIntegerv )(GLenum pname, GLint *params);
extern void ( APIENTRY *pglGetLightfv )(GLenum light, GLenum pname, GLfloat *params);
extern void ( APIENTRY *pglGetLightiv )(GLenum light, GLenum pname, GLint *params);
extern void ( APIENTRY *pglGetMapdv )(GLenum target, GLenum query, GLdouble *v);
extern void ( APIENTRY *pglGetMapfv )(GLenum target, GLenum query, GLfloat *v);
extern void ( APIENTRY *pglGetMapiv )(GLenum target, GLenum query, GLint *v);
extern void ( APIENTRY *pglGetMaterialfv )(GLenum face, GLenum pname, GLfloat *params);
extern void ( APIENTRY *pglGetMaterialiv )(GLenum face, GLenum pname, GLint *params);
extern void ( APIENTRY *pglGetPixelMapfv )(GLenum map, GLfloat *values);
extern void ( APIENTRY *pglGetPixelMapuiv )(GLenum map, GLuint *values);
extern void ( APIENTRY *pglGetPixelMapusv )(GLenum map, GLushort *values);
extern void ( APIENTRY *pglGetPointerv )(GLenum pname, GLvoid* *params);
extern void ( APIENTRY *pglGetPolygonStipple )(GLubyte *mask);
extern void ( APIENTRY *pglGetTexEnvfv )(GLenum target, GLenum pname, GLfloat *params);
extern void ( APIENTRY *pglGetTexEnviv )(GLenum target, GLenum pname, GLint *params);
extern void ( APIENTRY *pglGetTexGendv )(GLenum coord, GLenum pname, GLdouble *params);
extern void ( APIENTRY *pglGetTexGenfv )(GLenum coord, GLenum pname, GLfloat *params);
extern void ( APIENTRY *pglGetTexGeniv )(GLenum coord, GLenum pname, GLint *params);
extern void ( APIENTRY *pglGetTexImage )(GLenum target, GLint level, GLenum format, GLenum type, GLvoid *pixels);
extern void ( APIENTRY *pglGetTexLevelParameterfv )(GLenum target, GLint level, GLenum pname, GLfloat *params);
extern void ( APIENTRY *pglGetTexLevelParameteriv )(GLenum target, GLint level, GLenum pname, GLint *params);
extern void ( APIENTRY *pglGetTexParameterfv )(GLenum target, GLenum pname, GLfloat *params);
extern void ( APIENTRY *pglGetTexParameteriv )(GLenum target, GLenum pname, GLint *params);
extern void ( APIENTRY *pglHint )(GLenum target, GLenum mode);
extern void ( APIENTRY *pglIndexMask )(GLuint mask);
extern void ( APIENTRY *pglIndexPointer )(GLenum type, GLsizei stride, const GLvoid *pointer);
extern void ( APIENTRY *pglIndexd )(GLdouble c);
extern void ( APIENTRY *pglIndexdv )(const GLdouble *c);
extern void ( APIENTRY *pglIndexf )(GLfloat c);
extern void ( APIENTRY *pglIndexfv )(const GLfloat *c);
extern void ( APIENTRY *pglIndexi )(GLint c);
extern void ( APIENTRY *pglIndexiv )(const GLint *c);
extern void ( APIENTRY *pglIndexs )(GLshort c);
extern void ( APIENTRY *pglIndexsv )(const GLshort *c);
extern void ( APIENTRY *pglIndexub )(GLubyte c);
extern void ( APIENTRY *pglIndexubv )(const GLubyte *c);
extern void ( APIENTRY *pglInitNames )(void);
extern void ( APIENTRY *pglInterleavedArrays )(GLenum format, GLsizei stride, const GLvoid *pointer);
extern void ( APIENTRY *pglLightModelf )(GLenum pname, GLfloat param);
extern void ( APIENTRY *pglLightModelfv )(GLenum pname, const GLfloat *params);
extern void ( APIENTRY *pglLightModeli )(GLenum pname, GLint param);
extern void ( APIENTRY *pglLightModeliv )(GLenum pname, const GLint *params);
extern void ( APIENTRY *pglLightf )(GLenum light, GLenum pname, GLfloat param);
extern void ( APIENTRY *pglLightfv )(GLenum light, GLenum pname, const GLfloat *params);
extern void ( APIENTRY *pglLighti )(GLenum light, GLenum pname, GLint param);
extern void ( APIENTRY *pglLightiv )(GLenum light, GLenum pname, const GLint *params);
extern void ( APIENTRY *pglLineStipple )(GLint factor, GLushort pattern);
extern void ( APIENTRY *pglLineWidth )(GLfloat width);
extern void ( APIENTRY *pglListBase )(GLuint base);
extern void ( APIENTRY *pglLoadIdentity )(void);
extern void ( APIENTRY *pglLoadMatrixd )(const GLdouble *m);
extern void ( APIENTRY *pglLoadMatrixf )(const GLfloat *m);
extern void ( APIENTRY *pglLoadName )(GLuint name);
extern void ( APIENTRY *pglLogicOp )(GLenum opcode);
extern void ( APIENTRY *pglMap1d )(GLenum target, GLdouble u1, GLdouble u2, GLint stride, GLint order, const GLdouble *points);
extern void ( APIENTRY *pglMap1f )(GLenum target, GLfloat u1, GLfloat u2, GLint stride, GLint order, const GLfloat *points);
extern void ( APIENTRY *pglMap2d )(GLenum target, GLdouble u1, GLdouble u2, GLint ustride, GLint uorder, GLdouble v1, GLdouble v2, GLint vstride, GLint vorder, const GLdouble *points);
extern void ( APIENTRY *pglMap2f )(GLenum target, GLfloat u1, GLfloat u2, GLint ustride, GLint uorder, GLfloat v1, GLfloat v2, GLint vstride, GLint vorder, const GLfloat *points);
extern void ( APIENTRY *pglMapGrid1d )(GLint un, GLdouble u1, GLdouble u2);
extern void ( APIENTRY *pglMapGrid1f )(GLint un, GLfloat u1, GLfloat u2);
extern void ( APIENTRY *pglMapGrid2d )(GLint un, GLdouble u1, GLdouble u2, GLint vn, GLdouble v1, GLdouble v2);
extern void ( APIENTRY *pglMapGrid2f )(GLint un, GLfloat u1, GLfloat u2, GLint vn, GLfloat v1, GLfloat v2);
extern void ( APIENTRY *pglMaterialf )(GLenum face, GLenum pname, GLfloat param);
extern void ( APIENTRY *pglMaterialfv )(GLenum face, GLenum pname, const GLfloat *params);
extern void ( APIENTRY *pglMateriali )(GLenum face, GLenum pname, GLint param);
extern void ( APIENTRY *pglMaterialiv )(GLenum face, GLenum pname, const GLint *params);
extern void ( APIENTRY *pglMatrixMode )(GLenum mode);
extern void ( APIENTRY *pglMultMatrixd )(const GLdouble *m);
extern void ( APIENTRY *pglMultMatrixf )(const GLfloat *m);
extern void ( APIENTRY *pglNewList )(GLuint list, GLenum mode);
extern void ( APIENTRY *pglNormal3b )(GLbyte nx, GLbyte ny, GLbyte nz);
extern void ( APIENTRY *pglNormal3bv )(const GLbyte *v);
extern void ( APIENTRY *pglNormal3d )(GLdouble nx, GLdouble ny, GLdouble nz);
extern void ( APIENTRY *pglNormal3dv )(const GLdouble *v);
extern void ( APIENTRY *pglNormal3f )(GLfloat nx, GLfloat ny, GLfloat nz);
extern void ( APIENTRY *pglNormal3fv )(const GLfloat *v);
extern void ( APIENTRY *pglNormal3i )(GLint nx, GLint ny, GLint nz);
extern void ( APIENTRY *pglNormal3iv )(const GLint *v);
extern void ( APIENTRY *pglNormal3s )(GLshort nx, GLshort ny, GLshort nz);
extern void ( APIENTRY *pglNormal3sv )(const GLshort *v);
extern void ( APIENTRY *pglNormalPointer )(GLenum type, GLsizei stride, const GLvoid *pointer);
extern void ( APIENTRY *pglOrtho )(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar);
extern void ( APIENTRY *pglPassThrough )(GLfloat token);
extern void ( APIENTRY *pglPixelMapfv )(GLenum map, GLsizei mapsize, const GLfloat *values);
extern void ( APIENTRY *pglPixelMapuiv )(GLenum map, GLsizei mapsize, const GLuint *values);
extern void ( APIENTRY *pglPixelMapusv )(GLenum map, GLsizei mapsize, const GLushort *values);
extern void ( APIENTRY *pglPixelStoref )(GLenum pname, GLfloat param);
extern void ( APIENTRY *pglPixelStorei )(GLenum pname, GLint param);
extern void ( APIENTRY *pglPixelTransferf )(GLenum pname, GLfloat param);
extern void ( APIENTRY *pglPixelTransferi )(GLenum pname, GLint param);
extern void ( APIENTRY *pglPixelZoom )(GLfloat xfactor, GLfloat yfactor);
extern void ( APIENTRY *pglPointSize )(GLfloat size);
extern void ( APIENTRY *pglPolygonMode )(GLenum face, GLenum mode);
extern void ( APIENTRY *pglPolygonOffset )(GLfloat factor, GLfloat units);
extern void ( APIENTRY *pglPolygonStipple )(const GLubyte *mask);
extern void ( APIENTRY *pglPopAttrib )(void);
extern void ( APIENTRY *pglPopClientAttrib )(void);
extern void ( APIENTRY *pglPopMatrix )(void);
extern void ( APIENTRY *pglPopName )(void);
extern void ( APIENTRY *pglPushAttrib )(GLbitfield mask);
extern void ( APIENTRY *pglPushClientAttrib )(GLbitfield mask);
extern void ( APIENTRY *pglPushMatrix )(void);
extern void ( APIENTRY *pglPushName )(GLuint name);
extern void ( APIENTRY *pglRasterPos2d )(GLdouble x, GLdouble y);
extern void ( APIENTRY *pglRasterPos2dv )(const GLdouble *v);
extern void ( APIENTRY *pglRasterPos2f )(GLfloat x, GLfloat y);
extern void ( APIENTRY *pglRasterPos2fv )(const GLfloat *v);
extern void ( APIENTRY *pglRasterPos2i )(GLint x, GLint y);
extern void ( APIENTRY *pglRasterPos2iv )(const GLint *v);
extern void ( APIENTRY *pglRasterPos2s )(GLshort x, GLshort y);
extern void ( APIENTRY *pglRasterPos2sv )(const GLshort *v);
extern void ( APIENTRY *pglRasterPos3d )(GLdouble x, GLdouble y, GLdouble z);
extern void ( APIENTRY *pglRasterPos3dv )(const GLdouble *v);
extern void ( APIENTRY *pglRasterPos3f )(GLfloat x, GLfloat y, GLfloat z);
extern void ( APIENTRY *pglRasterPos3fv )(const GLfloat *v);
extern void ( APIENTRY *pglRasterPos3i )(GLint x, GLint y, GLint z);
extern void ( APIENTRY *pglRasterPos3iv )(const GLint *v);
extern void ( APIENTRY *pglRasterPos3s )(GLshort x, GLshort y, GLshort z);
extern void ( APIENTRY *pglRasterPos3sv )(const GLshort *v);
extern void ( APIENTRY *pglRasterPos4d )(GLdouble x, GLdouble y, GLdouble z, GLdouble w);
extern void ( APIENTRY *pglRasterPos4dv )(const GLdouble *v);
extern void ( APIENTRY *pglRasterPos4f )(GLfloat x, GLfloat y, GLfloat z, GLfloat w);
extern void ( APIENTRY *pglRasterPos4fv )(const GLfloat *v);
extern void ( APIENTRY *pglRasterPos4i )(GLint x, GLint y, GLint z, GLint w);
extern void ( APIENTRY *pglRasterPos4iv )(const GLint *v);
extern void ( APIENTRY *pglRasterPos4s )(GLshort x, GLshort y, GLshort z, GLshort w);
extern void ( APIENTRY *pglRasterPos4sv )(const GLshort *v);
extern void ( APIENTRY *pglReadBuffer )(GLenum mode);
extern void ( APIENTRY *pglReadPixels )(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels);
extern void ( APIENTRY *pglRectd )(GLdouble x1, GLdouble y1, GLdouble x2, GLdouble y2);
extern void ( APIENTRY *pglRectdv )(const GLdouble *v1, const GLdouble *v2);
extern void ( APIENTRY *pglRectf )(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2);
extern void ( APIENTRY *pglRectfv )(const GLfloat *v1, const GLfloat *v2);
extern void ( APIENTRY *pglRecti )(GLint x1, GLint y1, GLint x2, GLint y2);
extern void ( APIENTRY *pglRectiv )(const GLint *v1, const GLint *v2);
extern void ( APIENTRY *pglRects )(GLshort x1, GLshort y1, GLshort x2, GLshort y2);
extern void ( APIENTRY *pglRectsv )(const GLshort *v1, const GLshort *v2);
extern void ( APIENTRY *pglRotated )(GLdouble angle, GLdouble x, GLdouble y, GLdouble z);
extern void ( APIENTRY *pglRotatef )(GLfloat angle, GLfloat x, GLfloat y, GLfloat z);
extern void ( APIENTRY *pglScaled )(GLdouble x, GLdouble y, GLdouble z);
extern void ( APIENTRY *pglScalef )(GLfloat x, GLfloat y, GLfloat z);
extern void ( APIENTRY *pglScissor )(GLint x, GLint y, GLsizei width, GLsizei height);
extern void ( APIENTRY *pglSelectBuffer )(GLsizei size, GLuint *buffer);
extern void ( APIENTRY *pglShadeModel )(GLenum mode);
extern void ( APIENTRY *pglStencilFunc )(GLenum func, GLint ref, GLuint mask);
extern void ( APIENTRY *pglStencilMask )(GLuint mask);
extern void ( APIENTRY *pglStencilOp )(GLenum fail, GLenum zfail, GLenum zpass);
extern void ( APIENTRY *pglTexCoord1d )(GLdouble s);
extern void ( APIENTRY *pglTexCoord1dv )(const GLdouble *v);
extern void ( APIENTRY *pglTexCoord1f )(GLfloat s);
extern void ( APIENTRY *pglTexCoord1fv )(const GLfloat *v);
extern void ( APIENTRY *pglTexCoord1i )(GLint s);
extern void ( APIENTRY *pglTexCoord1iv )(const GLint *v);
extern void ( APIENTRY *pglTexCoord1s )(GLshort s);
extern void ( APIENTRY *pglTexCoord1sv )(const GLshort *v);
extern void ( APIENTRY *pglTexCoord2d )(GLdouble s, GLdouble t);
extern void ( APIENTRY *pglTexCoord2dv )(const GLdouble *v);
extern void ( APIENTRY *pglTexCoord2f )(GLfloat s, GLfloat t);
extern void ( APIENTRY *pglTexCoord2fv )(const GLfloat *v);
extern void ( APIENTRY *pglTexCoord2i )(GLint s, GLint t);
extern void ( APIENTRY *pglTexCoord2iv )(const GLint *v);
extern void ( APIENTRY *pglTexCoord2s )(GLshort s, GLshort t);
extern void ( APIENTRY *pglTexCoord2sv )(const GLshort *v);
extern void ( APIENTRY *pglTexCoord3d )(GLdouble s, GLdouble t, GLdouble r);
extern void ( APIENTRY *pglTexCoord3dv )(const GLdouble *v);
extern void ( APIENTRY *pglTexCoord3f )(GLfloat s, GLfloat t, GLfloat r);
extern void ( APIENTRY *pglTexCoord3fv )(const GLfloat *v);
extern void ( APIENTRY *pglTexCoord3i )(GLint s, GLint t, GLint r);
extern void ( APIENTRY *pglTexCoord3iv )(const GLint *v);
extern void ( APIENTRY *pglTexCoord3s )(GLshort s, GLshort t, GLshort r);
extern void ( APIENTRY *pglTexCoord3sv )(const GLshort *v);
extern void ( APIENTRY *pglTexCoord4d )(GLdouble s, GLdouble t, GLdouble r, GLdouble q);
extern void ( APIENTRY *pglTexCoord4dv )(const GLdouble *v);
extern void ( APIENTRY *pglTexCoord4f )(GLfloat s, GLfloat t, GLfloat r, GLfloat q);
extern void ( APIENTRY *pglTexCoord4fv )(const GLfloat *v);
extern void ( APIENTRY *pglTexCoord4i )(GLint s, GLint t, GLint r, GLint q);
extern void ( APIENTRY *pglTexCoord4iv )(const GLint *v);
extern void ( APIENTRY *pglTexCoord4s )(GLshort s, GLshort t, GLshort r, GLshort q);
extern void ( APIENTRY *pglTexCoord4sv )(const GLshort *v);
extern void ( APIENTRY *pglTexCoordPointer )(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
extern void ( APIENTRY *pglTexEnvf )(GLenum target, GLenum pname, GLfloat param);
extern void ( APIENTRY *pglTexEnvfv )(GLenum target, GLenum pname, const GLfloat *params);
extern void ( APIENTRY *pglTexEnvi )(GLenum target, GLenum pname, GLint param);
extern void ( APIENTRY *pglTexEnviv )(GLenum target, GLenum pname, const GLint *params);
extern void ( APIENTRY *pglTexGend )(GLenum coord, GLenum pname, GLdouble param);
extern void ( APIENTRY *pglTexGendv )(GLenum coord, GLenum pname, const GLdouble *params);
extern void ( APIENTRY *pglTexGenf )(GLenum coord, GLenum pname, GLfloat param);
extern void ( APIENTRY *pglTexGenfv )(GLenum coord, GLenum pname, const GLfloat *params);
extern void ( APIENTRY *pglTexGeni )(GLenum coord, GLenum pname, GLint param);
extern void ( APIENTRY *pglTexGeniv )(GLenum coord, GLenum pname, const GLint *params);
extern void ( APIENTRY *pglTexImage1D )(GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
extern void ( APIENTRY *pglTexImage2D )(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
extern void ( APIENTRY *pglTexParameterf )(GLenum target, GLenum pname, GLfloat param);
extern void ( APIENTRY *pglTexParameterfv )(GLenum target, GLenum pname, const GLfloat *params);
extern void ( APIENTRY *pglTexParameteri )(GLenum target, GLenum pname, GLint param);
extern void ( APIENTRY *pglTexParameteriv )(GLenum target, GLenum pname, const GLint *params);
extern void ( APIENTRY *pglTexSubImage1D )(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const GLvoid *pixels);
extern void ( APIENTRY *pglTexSubImage2D )(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels);
extern void ( APIENTRY *pglTranslated )(GLdouble x, GLdouble y, GLdouble z);
extern void ( APIENTRY *pglTranslatef )(GLfloat x, GLfloat y, GLfloat z);
extern void ( APIENTRY *pglVertex2d )(GLdouble x, GLdouble y);
extern void ( APIENTRY *pglVertex2dv )(const GLdouble *v);
extern void ( APIENTRY *pglVertex2f )(GLfloat x, GLfloat y);
extern void ( APIENTRY *pglVertex2fv )(const GLfloat *v);
extern void ( APIENTRY *pglVertex2i )(GLint x, GLint y);
extern void ( APIENTRY *pglVertex2iv )(const GLint *v);
extern void ( APIENTRY *pglVertex2s )(GLshort x, GLshort y);
extern void ( APIENTRY *pglVertex2sv )(const GLshort *v);
extern void ( APIENTRY *pglVertex3d )(GLdouble x, GLdouble y, GLdouble z);
extern void ( APIENTRY *pglVertex3dv )(const GLdouble *v);
extern void ( APIENTRY *pglVertex3f )(GLfloat x, GLfloat y, GLfloat z);
extern void ( APIENTRY *pglVertex3fv )(const GLfloat *v);
extern void ( APIENTRY *pglVertex3i )(GLint x, GLint y, GLint z);
extern void ( APIENTRY *pglVertex3iv )(const GLint *v);
extern void ( APIENTRY *pglVertex3s )(GLshort x, GLshort y, GLshort z);
extern void ( APIENTRY *pglVertex3sv )(const GLshort *v);
extern void ( APIENTRY *pglVertex4d )(GLdouble x, GLdouble y, GLdouble z, GLdouble w);
extern void ( APIENTRY *pglVertex4dv )(const GLdouble *v);
extern void ( APIENTRY *pglVertex4f )(GLfloat x, GLfloat y, GLfloat z, GLfloat w);
extern void ( APIENTRY *pglVertex4fv )(const GLfloat *v);
extern void ( APIENTRY *pglVertex4i )(GLint x, GLint y, GLint z, GLint w);
extern void ( APIENTRY *pglVertex4iv )(const GLint *v);
extern void ( APIENTRY *pglVertex4s )(GLshort x, GLshort y, GLshort z, GLshort w);
extern void ( APIENTRY *pglVertex4sv )(const GLshort *v);
extern void ( APIENTRY *pglVertexPointer )(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
extern void ( APIENTRY *pglViewport )(GLint x, GLint y, GLsizei width, GLsizei height);
extern void ( APIENTRY *pglPointParameterfEXT)( GLenum param, GLfloat value );
extern void ( APIENTRY *pglPointParameterfvEXT)( GLenum param, const GLfloat *value );
extern void ( APIENTRY *pglLockArraysEXT) (int , int);
extern void ( APIENTRY *pglUnlockArraysEXT) (void);
extern void ( APIENTRY *pglActiveTextureARB)( GLenum );
extern void ( APIENTRY *pglClientActiveTextureARB)( GLenum );
extern void ( APIENTRY *pglGetCompressedTexImage)( GLenum target, GLint lod, const GLvoid* data );
extern void ( APIENTRY *pglDrawRangeElements)( GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const GLvoid *indices );
extern void ( APIENTRY *pglDrawRangeElementsEXT)( GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const GLvoid *indices );
extern void ( APIENTRY *pglMultiTexCoord1f) (GLenum, GLfloat);
extern void ( APIENTRY *pglMultiTexCoord2f) (GLenum, GLfloat, GLfloat);
extern void ( APIENTRY *pglMultiTexCoord3f) (GLenum, GLfloat, GLfloat, GLfloat);
extern void ( APIENTRY *pglMultiTexCoord4f) (GLenum, GLfloat, GLfloat, GLfloat, GLfloat);
extern void ( APIENTRY *pglActiveTexture) (GLenum);
extern void ( APIENTRY *pglClientActiveTexture) (GLenum);
extern void ( APIENTRY *pglCompressedTexImage3DARB)(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const void *data);
extern void ( APIENTRY *pglCompressedTexImage2DARB)(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border,  GLsizei imageSize, const void *data);
extern void ( APIENTRY *pglCompressedTexImage1DARB)(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLint border, GLsizei imageSize, const void *data);
extern void ( APIENTRY *pglCompressedTexSubImage3DARB)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const void *data);
extern void ( APIENTRY *pglCompressedTexSubImage2DARB)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void *data);
extern void ( APIENTRY *pglCompressedTexSubImage1DARB)(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, const void *data);
extern void ( APIENTRY *pglDeleteObjectARB)(GLhandleARB obj);
extern GLhandleARB ( APIENTRY *pglGetHandleARB)(GLenum pname);
extern void ( APIENTRY *pglDetachObjectARB)(GLhandleARB containerObj, GLhandleARB attachedObj);
extern GLhandleARB ( APIENTRY *pglCreateShaderObjectARB)(GLenum shaderType);
extern void ( APIENTRY *pglShaderSourceARB)(GLhandleARB shaderObj, GLsizei count, const GLcharARB **string, const GLint *length);
extern void ( APIENTRY *pglCompileShaderARB)(GLhandleARB shaderObj);
extern GLhandleARB ( APIENTRY *pglCreateProgramObjectARB)(void);
extern void ( APIENTRY *pglAttachObjectARB)(GLhandleARB containerObj, GLhandleARB obj);
extern void ( APIENTRY *pglLinkProgramARB)(GLhandleARB programObj);
extern void ( APIENTRY *pglUseProgramObjectARB)(GLhandleARB programObj);
extern void ( APIENTRY *pglValidateProgramARB)(GLhandleARB programObj);
extern void ( APIENTRY *pglBindProgramARB)(GLenum target, GLuint program);
extern void ( APIENTRY *pglDeleteProgramsARB)(GLsizei n, const GLuint *programs);
extern void ( APIENTRY *pglGenProgramsARB)(GLsizei n, GLuint *programs);
extern void ( APIENTRY *pglProgramStringARB)(GLenum target, GLenum format, GLsizei len, const GLvoid *string);
extern void ( APIENTRY *pglProgramEnvParameter4fARB)(GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
extern void ( APIENTRY *pglProgramLocalParameter4fARB)(GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
extern void ( APIENTRY *pglUniform1fARB)(GLint location, GLfloat v0);
extern void ( APIENTRY *pglUniform2fARB)(GLint location, GLfloat v0, GLfloat v1);
extern void ( APIENTRY *pglUniform3fARB)(GLint location, GLfloat v0, GLfloat v1, GLfloat v2);
extern void ( APIENTRY *pglUniform4fARB)(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);
extern void ( APIENTRY *pglUniform1iARB)(GLint location, GLint v0);
extern void ( APIENTRY *pglUniform2iARB)(GLint location, GLint v0, GLint v1);
extern void ( APIENTRY *pglUniform3iARB)(GLint location, GLint v0, GLint v1, GLint v2);
extern void ( APIENTRY *pglUniform4iARB)(GLint location, GLint v0, GLint v1, GLint v2, GLint v3);
extern void ( APIENTRY *pglUniform1fvARB)(GLint location, GLsizei count, const GLfloat *value);
extern void ( APIENTRY *pglUniform2fvARB)(GLint location, GLsizei count, const GLfloat *value);
extern void ( APIENTRY *pglUniform3fvARB)(GLint location, GLsizei count, const GLfloat *value);
extern void ( APIENTRY *pglUniform4fvARB)(GLint location, GLsizei count, const GLfloat *value);
extern void ( APIENTRY *pglUniform1ivARB)(GLint location, GLsizei count, const GLint *value);
extern void ( APIENTRY *pglUniform2ivARB)(GLint location, GLsizei count, const GLint *value);
extern void ( APIENTRY *pglUniform3ivARB)(GLint location, GLsizei count, const GLint *value);
extern void ( APIENTRY *pglUniform4ivARB)(GLint location, GLsizei count, const GLint *value);
extern void ( APIENTRY *pglUniformMatrix2fvARB)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
extern void ( APIENTRY *pglUniformMatrix3fvARB)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
extern void ( APIENTRY *pglUniformMatrix4fvARB)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
extern void ( APIENTRY *pglGetObjectParameterfvARB)(GLhandleARB obj, GLenum pname, GLfloat *params);
extern void ( APIENTRY *pglGetObjectParameterivARB)(GLhandleARB obj, GLenum pname, GLint *params);
extern void ( APIENTRY *pglGetInfoLogARB)(GLhandleARB obj, GLsizei maxLength, GLsizei *length, GLcharARB *infoLog);
extern void ( APIENTRY *pglGetAttachedObjectsARB)(GLhandleARB containerObj, GLsizei maxCount, GLsizei *count, GLhandleARB *obj);
extern GLint ( APIENTRY *pglGetUniformLocationARB)(GLhandleARB programObj, const GLcharARB *name);
extern void ( APIENTRY *pglGetActiveUniformARB)(GLhandleARB programObj, GLuint index, GLsizei maxLength, GLsizei *length, GLint *size, GLenum *type, GLcharARB *name);
extern void ( APIENTRY *pglGetUniformfvARB)(GLhandleARB programObj, GLint location, GLfloat *params);
extern void ( APIENTRY *pglGetUniformivARB)(GLhandleARB programObj, GLint location, GLint *params);
extern void ( APIENTRY *pglGetShaderSourceARB)(GLhandleARB obj, GLsizei maxLength, GLsizei *length, GLcharARB *source);
extern void ( APIENTRY *pglTexImage3D)( GLenum target, GLint level, GLenum internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const GLvoid *pixels );
extern void ( APIENTRY *pglTexSubImage3D)( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const GLvoid *pixels );
extern void ( APIENTRY *pglCopyTexSubImage3D)( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height );
extern void ( APIENTRY *pglBlendEquationEXT)(GLenum);
extern void ( APIENTRY *pglStencilOpSeparate)(GLenum, GLenum, GLenum, GLenum);
extern void ( APIENTRY *pglStencilFuncSeparate)(GLenum, GLenum, GLint, GLuint);
extern void ( APIENTRY *pglActiveStencilFaceEXT)(GLenum);
extern void ( APIENTRY *pglVertexAttribPointerARB)(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid *pointer);
extern void ( APIENTRY *pglEnableVertexAttribArrayARB)(GLuint index);
extern void ( APIENTRY *pglDisableVertexAttribArrayARB)(GLuint index);
extern void ( APIENTRY *pglBindAttribLocationARB)(GLhandleARB programObj, GLuint index, const GLcharARB *name);
extern void ( APIENTRY *pglGetActiveAttribARB)(GLhandleARB programObj, GLuint index, GLsizei maxLength, GLsizei *length, GLint *size, GLenum *type, GLcharARB *name);
extern GLint ( APIENTRY *pglGetAttribLocationARB)(GLhandleARB programObj, const GLcharARB *name);
extern void ( APIENTRY *pglBindBufferARB) (GLenum target, GLuint buffer);
extern void ( APIENTRY *pglDeleteBuffersARB) (GLsizei n, const GLuint *buffers);
extern void ( APIENTRY *pglGenBuffersARB) (GLsizei n, GLuint *buffers);
extern GLboolean ( APIENTRY *pglIsBufferARB) (GLuint buffer);
extern GLvoid* ( APIENTRY *pglMapBufferARB) (GLenum target, GLenum access);
extern GLboolean ( APIENTRY *pglUnmapBufferARB) (GLenum target);
extern void ( APIENTRY *pglBufferDataARB) (GLenum target, GLsizeiptrARB size, const GLvoid *data, GLenum usage);
extern void ( APIENTRY *pglBufferSubDataARB) (GLenum target, GLintptrARB offset, GLsizeiptrARB size, const GLvoid *data);
extern void ( APIENTRY *pglGenQueriesARB) (GLsizei n, GLuint *ids);
extern void ( APIENTRY *pglDeleteQueriesARB) (GLsizei n, const GLuint *ids);
extern GLboolean ( APIENTRY *pglIsQueryARB) (GLuint id);
extern void ( APIENTRY *pglBeginQueryARB) (GLenum target, GLuint id);
extern void ( APIENTRY *pglEndQueryARB) (GLenum target);
extern void ( APIENTRY *pglGetQueryivARB) (GLenum target, GLenum pname, GLint *params);
extern void ( APIENTRY *pglGetQueryObjectivARB) (GLuint id, GLenum pname, GLint *params);
extern void ( APIENTRY *pglGetQueryObjectuivARB) (GLuint id, GLenum pname, GLuint *params);
extern void ( APIENTRY * pglSelectTextureSGIS) ( GLenum );
extern void ( APIENTRY * pglMTexCoord2fSGIS) ( GLenum, GLfloat, GLfloat );
extern void ( APIENTRY * pglSwapInterval) ( int interval );
extern void *pglGetProcAddress( const GLubyte * );

#endif

#endif//GL_EXPORT_H
