typedef unsigned int GLenum;
typedef unsigned char GLboolean;
typedef unsigned int GLbitfield;
typedef void GLvoid;
typedef signed char GLbyte;
typedef short GLshort;
typedef int GLint;
typedef unsigned char GLubyte;
typedef unsigned short GLushort;
typedef unsigned int GLuint;
typedef int GLsizei;
typedef float GLfloat;
typedef float GLclampf;
typedef double GLdouble;
typedef double GLclampd;
typedef int GLintptrARB;
typedef int GLsizeiptrARB;
typedef char GLcharARB;
typedef char GLchar;
typedef unsigned int GLhandleARB;
#define STDCALL __attribute__((__stdcall__))
#define GL_MODELVIEW	0x1700
#define GL_PROJECTION	0x1701
#define GL_TEXTURE	0x1702
#define GL_MATRIX_MODE	0x0BA0
#define GL_MODELVIEW_MATRIX	0x0BA6
#define GL_PROJECTION_MATRIX	0x0BA7
#define GL_TEXTURE_MATRIX	0x0BA8
#define GL_DONT_CARE	0x1100
#define GL_FASTEST	0x1101
#define GL_NICEST	0x1102
#define GL_DEPTH_TEST	0x0B71
#define GL_CULL_FACE	0x0B44
#define GL_CW	0x0900
#define GL_CCW	0x0901
#define GL_BLEND	0x0BE2
#define GL_ALPHA_TEST	0x0BC0
// shading model
#define GL_FLAT	0x1D00
#define GL_SMOOTH	0x1D01
#define GL_ZERO	0
#define GL_ONE	1
#define GL_SRC_COLOR	0x0300
#define GL_ONE_MINUS_SRC_COLOR	0x0301
#define GL_DST_COLOR	0x0306
#define GL_ONE_MINUS_DST_COLOR	0x0307
#define GL_SRC_ALPHA	0x0302
#define GL_ONE_MINUS_SRC_ALPHA	0x0303
#define GL_DST_ALPHA	0x0304
#define GL_ONE_MINUS_DST_ALPHA	0x0305
#define GL_SRC_ALPHA_SATURATE	0x0308
#define GL_CONSTANT_COLOR	0x8001
#define GL_ONE_MINUS_CONSTANT_COLOR	0x8002
#define GL_CONSTANT_ALPHA	0x8003
#define GL_ONE_MINUS_CONSTANT_ALPHA	0x8004
#define GL_TEXTURE_ENV	0x2300
#define GL_TEXTURE_ENV_MODE	0x2200
#define GL_TEXTURE_ENV_COLOR	0x2201
#define GL_TEXTURE_1D	0x0DE0
#define GL_TEXTURE_2D	0x0DE1
#define GL_TEXTURE_WRAP_S	0x2802
#define GL_TEXTURE_WRAP_T	0x2803
#define GL_TEXTURE_WRAP_R	0x8072
#define GL_TEXTURE_BORDER_COLOR	0x1004
#define GL_TEXTURE_MAG_FILTER	0x2800
#define GL_TEXTURE_MIN_FILTER	0x2801
#define GL_PACK_ALIGNMENT	0x0D05
#define GL_UNPACK_ALIGNMENT	0x0CF5
#define GL_TEXTURE_BINDING_1D	0x8068
#define GL_TEXTURE_BINDING_2D	0x8069
#define GL_CLAMP_TO_EDGE 0x812F
#define GL_NEAREST	0x2600
#define GL_LINEAR	0x2601
#define GL_NEAREST_MIPMAP_NEAREST	0x2700
#define GL_NEAREST_MIPMAP_LINEAR	0x2702
#define GL_LINEAR_MIPMAP_NEAREST	0x2701
#define GL_LINEAR_MIPMAP_LINEAR	0x2703
#define GL_LINE	0x1B01
#define GL_FILL	0x1B02
#define GL_TEXTURE_MAX_ANISOTROPY_EXT	0x84FE
#define GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT	0x84FF
#define GL_MAX_TEXTURE_LOD_BIAS_EXT	0x84FD
#define GL_TEXTURE_FILTER_CONTROL_EXT	0x8500
#define GL_TEXTURE_LOD_BIAS_EXT	0x8501
#define GL_CLAMP_TO_BORDER_ARB	0x812D
#define GL_ADD	0x0104
#define GL_DECAL	0x2101
#define GL_MODULATE	0x2100
#define GL_REPEAT	0x2901
#define GL_CLAMP	0x2900
#define GL_POINTS	0x0000
#define GL_LINES	0x0001
#define GL_LINE_LOOP	0x0002
#define GL_LINE_STRIP	0x0003
#define GL_TRIANGLES	0x0004
#define GL_TRIANGLE_STRIP	0x0005
#define GL_TRIANGLE_FAN	0x0006
#define GL_QUADS	0x0007
#define GL_QUAD_STRIP	0x0008
#define GL_POLYGON	0x0009
#define GL_FALSE	0
#define GL_TRUE	1
#define GL_BYTE	0x1400
#define GL_UNSIGNED_BYTE	0x1401
#define GL_SHORT	0x1402
#define GL_UNSIGNED_SHORT	0x1403
#define GL_INT	0x1404
#define GL_UNSIGNED_INT	0x1405
#define GL_FLOAT	0x1406
#define GL_DOUBLE	0x140A
#define GL_2_BYTES	0x1407
#define GL_3_BYTES	0x1408
#define GL_4_BYTES	0x1409
#define GL_VERTEX_ARRAY	0x8074
#define GL_NORMAL_ARRAY	0x8075
#define GL_COLOR_ARRAY	0x8076
#define GL_INDEX_ARRAY	0x8077
#define GL_TEXTURE_COORD_ARRAY	0x8078
#define GL_EDGE_FLAG_ARRAY	0x8079
#define GL_NONE	0
#define GL_FRONT_LEFT	0x0400
#define GL_FRONT_RIGHT	0x0401
#define GL_BACK_LEFT	0x0402
#define GL_BACK_RIGHT	0x0403
#define GL_FRONT	0x0404
#define GL_BACK	0x0405
#define GL_LEFT	0x0406
#define GL_RIGHT	0x0407
#define GL_FRONT_AND_BACK	0x0408
#define GL_AUX0	0x0409
#define GL_AUX1	0x040A
#define GL_AUX2	0x040B
#define GL_AUX3	0x040C
#define GL_VENDOR	0x1F00
#define GL_RENDERER	0x1F01
#define GL_VERSION	0x1F02
#define GL_EXTENSIONS	0x1F03
#define GL_NO_ERROR 0
#define GL_INVALID_VALUE	0x0501
#define GL_INVALID_ENUM	0x0500
#define GL_INVALID_OPERATION	0x0502
#define GL_STACK_OVERFLOW	0x0503
#define GL_STACK_UNDERFLOW	0x0504
#define GL_OUT_OF_MEMORY	0x0505
#define GL_DITHER	0x0BD0
#define GL_RGB	0x1907
#define GL_RGBA	0x1908
#define GL_BGR	0x80E0
#define GL_BGRA	0x80E1
#define GL_ALPHA4 0x803B
#define GL_ALPHA8 0x803C
#define GL_ALPHA12 0x803D
#define GL_ALPHA16 0x803E
#define GL_LUMINANCE4 0x803F
#define GL_LUMINANCE8 0x8040
#define GL_LUMINANCE12 0x8041
#define GL_LUMINANCE16 0x8042
#define GL_LUMINANCE4_ALPHA4 0x8043
#define GL_LUMINANCE6_ALPHA2 0x8044
#define GL_LUMINANCE8_ALPHA8 0x8045
#define GL_LUMINANCE12_ALPHA4 0x8046
#define GL_LUMINANCE12_ALPHA12 0x8047
#define GL_LUMINANCE16_ALPHA16	0x8048
#define GL_LUMINANCE	0x1909
#define GL_LUMINANCE_ALPHA	0x190A
#define GL_DEPTH_COMPONENT	0x1902
#define GL_INTENSITY 0x8049
#define GL_INTENSITY4 0x804A
#define GL_INTENSITY8 0x804B
#define GL_INTENSITY12 0x804C
#define GL_INTENSITY16 0x804D
#define GL_R3_G3_B2 0x2A10
#define GL_RGB4 0x804F
#define GL_RGB5 0x8050
#define GL_RGB8 0x8051
#define GL_RGB10 0x8052
#define GL_RGB12 0x8053
#define GL_RGB16 0x8054
#define GL_RGBA2 0x8055
#define GL_RGBA4 0x8056
#define GL_RGB5_A1 0x8057
#define GL_RGBA8 0x8058
#define GL_RGB10_A2 0x8059
#define GL_RGBA12 0x805A
#define GL_RGBA16 0x805B
#define GL_TEXTURE_RED_SIZE 0x805C
#define GL_TEXTURE_GREEN_SIZE 0x805D
#define GL_TEXTURE_BLUE_SIZE 0x805E
#define GL_TEXTURE_ALPHA_SIZE 0x805F
#define GL_TEXTURE_LUMINANCE_SIZE 0x8060
#define GL_TEXTURE_INTENSITY_SIZE 0x8061
#define GL_PROXY_TEXTURE_1D 0x8063
#define GL_PROXY_TEXTURE_2D 0x8064
#define GL_MAX_TEXTURE_SIZE	0x0D33
// texture coord name
#define GL_S	0x2000
#define GL_T	0x2001
#define GL_R	0x2002
#define GL_Q	0x2003
// texture gen mode
#define GL_EYE_LINEAR	0x2400
#define GL_OBJECT_LINEAR	0x2401
#define GL_SPHERE_MAP	0x2402
// texture gen parameter
#define GL_TEXTURE_GEN_MODE	0x2500
#define GL_OBJECT_PLANE	0x2501
#define GL_EYE_PLANE	0x2502
#define GL_FOG_HINT	0x0C54
#define GL_TEXTURE_GEN_S	0x0C60
#define GL_TEXTURE_GEN_T	0x0C61
#define GL_TEXTURE_GEN_R	0x0C62
#define GL_TEXTURE_GEN_Q	0x0C63
#define GL_SCISSOR_BOX	0x0C10
#define GL_SCISSOR_TEST	0x0C11
#define GL_NEVER	0x0200
#define GL_LESS	0x0201
#define GL_EQUAL	0x0202
#define GL_LEQUAL	0x0203
#define GL_GREATER	0x0204
#define GL_NOTEQUAL	0x0205
#define GL_GEQUAL	0x0206
#define GL_ALWAYS	0x0207
#define GL_DEPTH_TEST	0x0B71
#define GL_RED_SCALE	0x0D14
#define GL_GREEN_SCALE	0x0D18
#define GL_BLUE_SCALE	0x0D1A
#define GL_ALPHA_SCALE	0x0D1C
/* AttribMask */
#define GL_CURRENT_BIT	0x00000001
#define GL_POINT_BIT	0x00000002
#define GL_LINE_BIT	0x00000004
#define GL_POLYGON_BIT	0x00000008
#define GL_POLYGON_STIPPLE_BIT	0x00000010
#define GL_PIXEL_MODE_BIT	0x00000020
#define GL_LIGHTING_BIT	0x00000040
#define GL_FOG_BIT	0x00000080
#define GL_DEPTH_BUFFER_BIT	0x00000100
#define GL_ACCUM_BUFFER_BIT	0x00000200
#define GL_STENCIL_BUFFER_BIT	0x00000400
#define GL_VIEWPORT_BIT	0x00000800
#define GL_TRANSFORM_BIT	0x00001000
#define GL_ENABLE_BIT	0x00002000
#define GL_COLOR_BUFFER_BIT	0x00004000
#define GL_HINT_BIT	0x00008000
#define GL_EVAL_BIT	0x00010000
#define GL_LIST_BIT	0x00020000
#define GL_TEXTURE_BIT	0x00040000
#define GL_SCISSOR_BIT	0x00080000
#define GL_ALL_ATTRIB_BITS	0x000fffff
#define GL_STENCIL_TEST	0x0B90
#define GL_KEEP	0x1E00
#define GL_REPLACE	0x1E01
#define GL_INCR	0x1E02
#define GL_DECR	0x1E03
// fog stuff
#define GL_FOG	0x0B60
#define GL_FOG_INDEX	0x0B61
#define GL_FOG_DENSITY	0x0B62
#define GL_FOG_START	0x0B63
#define GL_FOG_END	0x0B64
#define GL_FOG_MODE	0x0B65
#define GL_FOG_COLOR	0x0B66
#define GL_EXP	0x0800
#define GL_EXP2	0x0801
#define GL_POLYGON_OFFSET_FACTOR	0x8038
#define GL_POLYGON_OFFSET_UNITS	0x2A00
#define GL_POLYGON_OFFSET_POINT	0x2A01
#define GL_POLYGON_OFFSET_LINE	0x2A02
#define GL_POLYGON_OFFSET_FILL	0x8037
#define GL_POINT_SMOOTH	0x0B10
#define GL_LINE_SMOOTH	0x0B20
#define GL_POLYGON_SMOOTH	0x0B41
#define GL_POLYGON_STIPPLE	0x0B42
#define GL_CLIP_PLANE0	0x3000
#define GL_CLIP_PLANE1	0x3001
#define GL_CLIP_PLANE2	0x3002
#define GL_CLIP_PLANE3	0x3003
#define GL_CLIP_PLANE4	0x3004
#define GL_CLIP_PLANE5	0x3005
#define GL_POINT_SIZE_MIN_EXT	0x8126
#define GL_POINT_SIZE_MAX_EXT	0x8127
#define GL_POINT_FADE_THRESHOLD_SIZE_EXT	0x8128
#define GL_DISTANCE_ATTENUATION_EXT	0x8129
#define GL_ACTIVE_TEXTURE_ARB	0x84E0
#define GL_CLIENT_ACTIVE_TEXTURE_ARB	0x84E1
#define GL_MAX_TEXTURE_UNITS_ARB	0x84E2
#define GL_TEXTURE0_ARB	0x84C0
#define GL_TEXTURE1_ARB	0x84C1
#define GL_TEXTURE2_ARB	0x84C2
#define GL_TEXTURE3_ARB	0x84C3
#define GL_TEXTURE2_ARB	0x84C2
#define GL_TEXTURE0_SGIS	0x835E
#define GL_TEXTURE1_SGIS	0x835F
#define GL_GENERATE_MIPMAP_SGIS 0x8191
#define GL_GENERATE_MIPMAP_HINT_SGIS 0x8192
#define GL_TEXTURE_RECTANGLE_NV	0x84F5
#define GL_TEXTURE_BINDING_RECTANGLE_NV	0x84F6
#define GL_PROXY_TEXTURE_RECTANGLE_NV	0x84F7
#define GL_MAX_RECTANGLE_TEXTURE_SIZE_NV	0x84F8
#define GL_TEXTURE_RECTANGLE_EXT	0x84F5
#define GL_TEXTURE_BINDING_RECTANGLE_EXT	0x84F6
#define GL_PROXY_TEXTURE_RECTANGLE_EXT	0x84F7
#define GL_MAX_RECTANGLE_TEXTURE_SIZE_EXT	0x84F8
#define GL_MAX_TEXTURE_UNITS	0x84E2
#define GL_MAX_TEXTURE_UNITS_ARB	0x84E2
#define GL_COMPRESSED_RGB_S3TC_DXT1_EXT	0x83F0
#define GL_COMPRESSED_RGBA_S3TC_DXT1_EXT	0x83F1
#define GL_COMPRESSED_RGBA_S3TC_DXT3_EXT	0x83F2
#define GL_COMPRESSED_RGBA_S3TC_DXT5_EXT	0x83F3
#define GL_COMPRESSED_ALPHA_ARB	0x84E9
#define GL_COMPRESSED_LUMINANCE_ARB	0x84EA
#define GL_COMPRESSED_LUMINANCE_ALPHA_ARB	0x84EB
#define GL_COMPRESSED_INTENSITY_ARB	0x84EC
#define GL_COMPRESSED_RGB_ARB	0x84ED
#define GL_COMPRESSED_RGBA_ARB	0x84EE
#define GL_TEXTURE_COMPRESSION_HINT_ARB	0x84EF
#define GL_TEXTURE_COMPRESSED_IMAGE_SIZE_ARB	0x86A0
#define GL_TEXTURE_COMPRESSED_ARB	0x86A1
#define GL_NUM_COMPRESSED_TEXTURE_FORMATS_ARB	0x86A2
#define GL_COMPRESSED_TEXTURE_FORMATS_ARB	0x86A3
#define GL_UNSIGNED_BYTE_2_3_3_REV	0x8362
#define GL_UNSIGNED_SHORT_5_6_5	0x8363
#define GL_UNSIGNED_SHORT_5_6_5_REV	0x8364
#define GL_UNSIGNED_SHORT_4_4_4_4_REV	0x8365
#define GL_UNSIGNED_SHORT_1_5_5_5_REV	0x8366
#define GL_UNSIGNED_INT_8_8_8_8_REV	0x8367
#define GL_UNSIGNED_INT_2_10_10_10_REV	0x8368
#define GL_TEXTURE_MAX_LEVEL	0x813D
#define GL_GENERATE_MIPMAP	0x8191
#define GL_ADD_SIGNED	0x8574
#define GL_PROGRAM_OBJECT_ARB	0x8B40
#define GL_OBJECT_TYPE_ARB	0x8B4E
#define GL_OBJECT_SUBTYPE_ARB	0x8B4F
#define GL_OBJECT_DELETE_STATUS_ARB	0x8B80
#define GL_OBJECT_COMPILE_STATUS_ARB	0x8B81
#define GL_OBJECT_LINK_STATUS_ARB	0x8B82
#define GL_OBJECT_VALIDATE_STATUS_ARB	0x8B83
#define GL_OBJECT_INFO_LOG_LENGTH_ARB	0x8B84
#define GL_OBJECT_ATTACHED_OBJECTS_ARB	0x8B85
#define GL_OBJECT_ACTIVE_UNIFORMS_ARB	0x8B86
#define GL_OBJECT_ACTIVE_UNIFORM_MAX_LENGTH_ARB	0x8B87
#define GL_OBJECT_SHADER_SOURCE_LENGTH_ARB	0x8B88
#define GL_SHADER_OBJECT_ARB	0x8B48
#define GL_FLOAT_VEC2_ARB	0x8B50
#define GL_FLOAT_VEC3_ARB	0x8B51
#define GL_FLOAT_VEC4_ARB	0x8B52
#define GL_INT_VEC2_ARB	0x8B53
#define GL_INT_VEC3_ARB	0x8B54
#define GL_INT_VEC4_ARB	0x8B55
#define GL_BOOL_ARB	0x8B56
#define GL_BOOL_VEC2_ARB	0x8B57
#define GL_BOOL_VEC3_ARB	0x8B58
#define GL_BOOL_VEC4_ARB	0x8B59
#define GL_FLOAT_MAT2_ARB	0x8B5A
#define GL_FLOAT_MAT3_ARB	0x8B5B
#define GL_FLOAT_MAT4_ARB	0x8B5C
#define GL_SAMPLER_1D_ARB	0x8B5D
#define GL_SAMPLER_2D_ARB	0x8B5E
#define GL_SAMPLER_3D_ARB	0x8B5F
#define GL_SAMPLER_CUBE_ARB	0x8B60
#define GL_SAMPLER_1D_SHADOW_ARB	0x8B61
#define GL_SAMPLER_2D_SHADOW_ARB	0x8B62
#define GL_SAMPLER_2D_RECT_ARB	0x8B63
#define GL_SAMPLER_2D_RECT_SHADOW_ARB	0x8B64
#define GL_PACK_SKIP_IMAGES	0x806B
#define GL_PACK_IMAGE_HEIGHT	0x806C
#define GL_UNPACK_SKIP_IMAGES	0x806D
#define GL_UNPACK_IMAGE_HEIGHT	0x806E
#define GL_TEXTURE_3D	0x806F
#define GL_PROXY_TEXTURE_3D	0x8070
#define GL_TEXTURE_DEPTH	0x8071
#define GL_TEXTURE_WRAP_R	0x8072
#define GL_MAX_3D_TEXTURE_SIZE	0x8073
#define GL_TEXTURE_BINDING_3D	0x806A
#define GL_STENCIL_TEST_TWO_SIDE_EXT	0x8910
#define GL_ACTIVE_STENCIL_FACE_EXT	0x8911
#define GL_STENCIL_BACK_FUNC 0x8800
#define GL_STENCIL_BACK_FAIL 0x8801
#define GL_STENCIL_BACK_PASS_DEPTH_FAIL 0x8802
#define GL_STENCIL_BACK_PASS_DEPTH_PASS 0x8803
#define GL_DEPTH_TEXTURE_MODE_ARB	0x884B
#define GL_TEXTURE_COMPARE_MODE_ARB	0x884C
#define GL_TEXTURE_COMPARE_FUNC_ARB	0x884D
#define GL_COMPARE_R_TO_TEXTURE_ARB	0x884E
#define GL_TEXTURE_COMPARE_FAIL_VALUE_ARB	0x80BF
#define GL_QUERY_COUNTER_BITS_ARB	0x8864
#define GL_CURRENT_QUERY_ARB	0x8865
#define GL_QUERY_RESULT_ARB	0x8866
#define GL_QUERY_RESULT_AVAILABLE_ARB	0x8867
#define GL_SAMPLES_PASSED_ARB	0x8914
#define GL_FUNC_ADD_EXT	0x8006
#define GL_FUNC_SUBTRACT_EXT	0x800A
#define GL_FUNC_REVERSE_SUBTRACT_EXT	0x800B
#define GL_MIN_EXT	0x8007
#define GL_MAX_EXT	0x8008
#define GL_BLEND_EQUATION_EXT	0x8009
#define GL_DEPTH_COMPONENT16	0x81A5
#define GL_DEPTH_COMPONENT24	0x81A6
#define GL_DEPTH_COMPONENT32	0x81A7
#define GL_VERTEX_SHADER_ARB	0x8B31
#define GL_MAX_VERTEX_UNIFORM_COMPONENTS_ARB	0x8B4A
#define GL_MAX_VARYING_FLOATS_ARB	0x8B4B
#define GL_MAX_VERTEX_ATTRIBS_ARB	0x8869
#define GL_MAX_TEXTURE_IMAGE_UNITS_ARB	0x8872
#define GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS_ARB	0x8B4C
#define GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS_ARB	0x8B4D
#define GL_MAX_TEXTURE_COORDS_ARB	0x8871
#define GL_VERTEX_PROGRAM_POINT_SIZE_ARB	0x8642
#define GL_VERTEX_PROGRAM_TWO_SIDE_ARB	0x8643
#define GL_OBJECT_ACTIVE_ATTRIBUTES_ARB	0x8B89
#define GL_OBJECT_ACTIVE_ATTRIBUTE_MAX_LENGTH_ARB	0x8B8A
#define GL_VERTEX_ATTRIB_ARRAY_ENABLED_ARB	0x8622
#define GL_VERTEX_ATTRIB_ARRAY_SIZE_ARB	0x8623
#define GL_VERTEX_ATTRIB_ARRAY_STRIDE_ARB	0x8624
#define GL_VERTEX_ATTRIB_ARRAY_TYPE_ARB	0x8625
#define GL_VERTEX_ATTRIB_ARRAY_NORMALIZED_ARB	0x886A
#define GL_CURRENT_VERTEX_ATTRIB_ARB	0x8626
#define GL_VERTEX_ATTRIB_ARRAY_POINTER_ARB	0x8645
#define GL_FLOAT_VEC2_ARB	0x8B50
#define GL_FLOAT_VEC3_ARB	0x8B51
#define GL_FLOAT_VEC4_ARB	0x8B52
#define GL_FLOAT_MAT2_ARB	0x8B5A
#define GL_FLOAT_MAT3_ARB	0x8B5B
#define GL_FLOAT_MAT4_ARB	0x8B5C
#define GL_FRAGMENT_SHADER_ARB	0x8B30
#define GL_MAX_FRAGMENT_UNIFORM_COMPONENTS_ARB	0x8B49
#define GL_MAX_TEXTURE_COORDS_ARB	0x8871
#define GL_MAX_TEXTURE_IMAGE_UNITS_ARB	0x8872
#define GL_FRAGMENT_SHADER_DERIVATIVE_HINT_ARB	0x8B8B
//GL_ARB_vertex_buffer_object
#define GL_ARRAY_BUFFER_ARB	0x8892
#define GL_ELEMENT_ARRAY_BUFFER_ARB	0x8893
#define GL_ARRAY_BUFFER_BINDING_ARB	0x8894
#define GL_ELEMENT_ARRAY_BUFFER_BINDING_ARB	0x8895
#define GL_VERTEX_ARRAY_BUFFER_BINDING_ARB	0x8896
#define GL_NORMAL_ARRAY_BUFFER_BINDING_ARB	0x8897
#define GL_COLOR_ARRAY_BUFFER_BINDING_ARB	0x8898
#define GL_INDEX_ARRAY_BUFFER_BINDING_ARB	0x8899
#define GL_TEXTURE_COORD_ARRAY_BUFFER_BINDING_ARB	0x889A
#define GL_EDGE_FLAG_ARRAY_BUFFER_BINDING_ARB	0x889B
#define GL_WEIGHT_ARRAY_BUFFER_BINDING_ARB	0x889E
#define GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING_ARB	0x889F
#define GL_STREAM_DRAW_ARB	0x88E0
#define GL_STREAM_READ_ARB	0x88E1
#define GL_STREAM_COPY_ARB	0x88E2
#define GL_STATIC_DRAW_ARB	0x88E4
#define GL_STATIC_READ_ARB	0x88E5
#define GL_STATIC_COPY_ARB	0x88E6
#define GL_DYNAMIC_DRAW_ARB	0x88E8
#define GL_DYNAMIC_READ_ARB	0x88E9
#define GL_DYNAMIC_COPY_ARB	0x88EA
#define GL_READ_ONLY_ARB	0x88B8
#define GL_WRITE_ONLY_ARB	0x88B9
#define GL_READ_WRITE_ARB	0x88BA
#define GL_BUFFER_SIZE_ARB	0x8764
#define GL_BUFFER_USAGE_ARB	0x8765
#define GL_BUFFER_ACCESS_ARB	0x88BB
#define GL_BUFFER_MAPPED_ARB	0x88BC
#define GL_BUFFER_MAP_POINTER_ARB	0x88BD
#define GL_SECONDARY_COLOR_ARRAY_BUFFER_BINDING_ARB	0x889C
#define GL_FOG_COORDINATE_ARRAY_BUFFER_BINDING_ARB	0x889D
#define GL_NORMAL_MAP_ARB	0x8511
#define GL_REFLECTION_MAP_ARB	0x8512
#define GL_TEXTURE_CUBE_MAP_ARB	0x8513
#define GL_TEXTURE_BINDING_CUBE_MAP_ARB	0x8514
#define GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB	0x8515
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_X_ARB	0x8516
#define GL_TEXTURE_CUBE_MAP_POSITIVE_Y_ARB	0x8517
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_ARB	0x8518
#define GL_TEXTURE_CUBE_MAP_POSITIVE_Z_ARB	0x8519
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB	0x851A
#define GL_PROXY_TEXTURE_CUBE_MAP_ARB	0x851B
#define GL_MAX_CUBE_MAP_TEXTURE_SIZE_ARB	0x851C
#define GL_COMBINE_ARB	0x8570
#define GL_COMBINE_RGB_ARB	0x8571
#define GL_COMBINE_ALPHA_ARB	0x8572
#define GL_SOURCE0_RGB_ARB	0x8580
#define GL_SOURCE1_RGB_ARB	0x8581
#define GL_SOURCE2_RGB_ARB	0x8582
#define GL_SOURCE0_ALPHA_ARB	0x8588
#define GL_SOURCE1_ALPHA_ARB	0x8589
#define GL_SOURCE2_ALPHA_ARB	0x858A
#define GL_OPERAND0_RGB_ARB	0x8590
#define GL_OPERAND1_RGB_ARB	0x8591
#define GL_OPERAND2_RGB_ARB	0x8592
#define GL_OPERAND0_ALPHA_ARB	0x8598
#define GL_OPERAND1_ALPHA_ARB	0x8599
#define GL_OPERAND2_ALPHA_ARB	0x859A
#define GL_RGB_SCALE_ARB	0x8573
#define GL_ADD_SIGNED_ARB	0x8574
#define GL_INTERPOLATE_ARB	0x8575
#define GL_SUBTRACT_ARB	0x84E7
#define GL_CONSTANT_ARB	0x8576
#define GL_PRIMARY_COLOR_ARB	0x8577
#define GL_PREVIOUS_ARB	0x8578
#define GL_DOT3_RGB_ARB	0x86AE
#define GL_DOT3_RGBA_ARB	0x86AF
#define GL_MULTISAMPLE_ARB	0x809D
#define GL_SAMPLE_ALPHA_TO_COVERAGE_ARB	0x809E
#define GL_SAMPLE_ALPHA_TO_ONE_ARB	0x809F
#define GL_SAMPLE_COVERAGE_ARB	0x80A0
#define GL_SAMPLE_BUFFERS_ARB	0x80A8
#define GL_SAMPLES_ARB	0x80A9
#define GL_SAMPLE_COVERAGE_VALUE_ARB	0x80AA
#define GL_SAMPLE_COVERAGE_INVERT_ARB	0x80AB
#define GL_MULTISAMPLE_BIT_ARB	0x20000000
#define GL_COLOR_SUM_ARB	0x8458
#define GL_VERTEX_PROGRAM_ARB	0x8620
#define GL_VERTEX_ATTRIB_ARRAY_ENABLED_ARB	0x8622
#define GL_VERTEX_ATTRIB_ARRAY_SIZE_ARB	0x8623
#define GL_VERTEX_ATTRIB_ARRAY_STRIDE_ARB	0x8624
#define GL_VERTEX_ATTRIB_ARRAY_TYPE_ARB	0x8625
#define GL_CURRENT_VERTEX_ATTRIB_ARB	0x8626
#define GL_PROGRAM_LENGTH_ARB	0x8627
#define GL_PROGRAM_STRING_ARB	0x8628
#define GL_MAX_PROGRAM_MATRIX_STACK_DEPTH_ARB	0x862E
#define GL_MAX_PROGRAM_MATRICES_ARB	0x862F
#define GL_CURRENT_MATRIX_STACK_DEPTH_ARB	0x8640
#define GL_CURRENT_MATRIX_ARB	0x8641
#define GL_VERTEX_PROGRAM_POINT_SIZE_ARB	0x8642
#define GL_VERTEX_PROGRAM_TWO_SIDE_ARB	0x8643
#define GL_VERTEX_ATTRIB_ARRAY_POINTER_ARB	0x8645
#define GL_PROGRAM_ERROR_POSITION_ARB	0x864B
#define GL_PROGRAM_BINDING_ARB	0x8677
#define GL_MAX_VERTEX_ATTRIBS_ARB	0x8869
#define GL_VERTEX_ATTRIB_ARRAY_NORMALIZED_ARB	0x886A
#define GL_PROGRAM_ERROR_STRING_ARB	0x8874
#define GL_PROGRAM_FORMAT_ASCII_ARB	0x8875
#define GL_PROGRAM_FORMAT_ARB	0x8876
#define GL_PROGRAM_INSTRUCTIONS_ARB	0x88A0
#define GL_MAX_PROGRAM_INSTRUCTIONS_ARB	0x88A1
#define GL_PROGRAM_NATIVE_INSTRUCTIONS_ARB	0x88A2
#define GL_MAX_PROGRAM_NATIVE_INSTRUCTIONS_ARB	0x88A3
#define GL_PROGRAM_TEMPORARIES_ARB	0x88A4
#define GL_MAX_PROGRAM_TEMPORARIES_ARB	0x88A5
#define GL_PROGRAM_NATIVE_TEMPORARIES_ARB	0x88A6
#define GL_MAX_PROGRAM_NATIVE_TEMPORARIES_ARB	0x88A7
#define GL_PROGRAM_PARAMETERS_ARB	0x88A8
#define GL_MAX_PROGRAM_PARAMETERS_ARB	0x88A9
#define GL_PROGRAM_NATIVE_PARAMETERS_ARB	0x88AA
#define GL_MAX_PROGRAM_NATIVE_PARAMETERS_ARB	0x88AB
#define GL_PROGRAM_ATTRIBS_ARB	0x88AC
#define GL_MAX_PROGRAM_ATTRIBS_ARB	0x88AD
#define GL_PROGRAM_NATIVE_ATTRIBS_ARB	0x88AE
#define GL_MAX_PROGRAM_NATIVE_ATTRIBS_ARB	0x88AF
#define GL_PROGRAM_ADDRESS_REGISTERS_ARB	0x88B0
#define GL_MAX_PROGRAM_ADDRESS_REGISTERS_ARB	0x88B1
#define GL_PROGRAM_NATIVE_ADDRESS_REGISTERS_ARB	0x88B2
#define GL_MAX_PROGRAM_NATIVE_ADDRESS_REGISTERS_ARB	0x88B3
#define GL_MAX_PROGRAM_LOCAL_PARAMETERS_ARB	0x88B4
#define GL_MAX_PROGRAM_ENV_PARAMETERS_ARB	0x88B5
#define GL_PROGRAM_UNDER_NATIVE_LIMITS_ARB	0x88B6
#define GL_TRANSPOSE_CURRENT_MATRIX_ARB	0x88B7
#define GL_MATRIX0_ARB	0x88C0
#define GL_MATRIX1_ARB	0x88C1
#define GL_MATRIX2_ARB	0x88C2
#define GL_MATRIX3_ARB	0x88C3
#define GL_MATRIX4_ARB	0x88C4
#define GL_MATRIX5_ARB	0x88C5
#define GL_MATRIX6_ARB	0x88C6
#define GL_MATRIX7_ARB	0x88C7
#define GL_MATRIX8_ARB	0x88C8
#define GL_MATRIX9_ARB	0x88C9
#define GL_MATRIX10_ARB	0x88CA
#define GL_MATRIX11_ARB	0x88CB
#define GL_MATRIX12_ARB	0x88CC
#define GL_MATRIX13_ARB	0x88CD
#define GL_MATRIX14_ARB	0x88CE
#define GL_MATRIX15_ARB	0x88CF
#define GL_MATRIX16_ARB	0x88D0
#define GL_MATRIX17_ARB	0x88D1
#define GL_MATRIX18_ARB	0x88D2
#define GL_MATRIX19_ARB	0x88D3
#define GL_MATRIX20_ARB	0x88D4
#define GL_MATRIX21_ARB	0x88D5
#define GL_MATRIX22_ARB	0x88D6
#define GL_MATRIX23_ARB	0x88D7
#define GL_MATRIX24_ARB	0x88D8
#define GL_MATRIX25_ARB	0x88D9
#define GL_MATRIX26_ARB	0x88DA
#define GL_MATRIX27_ARB	0x88DB
#define GL_MATRIX28_ARB	0x88DC
#define GL_MATRIX29_ARB	0x88DD
#define GL_MATRIX30_ARB	0x88DE
#define GL_MATRIX31_ARB	0x88DF
#define GL_FRAGMENT_PROGRAM_ARB	0x8804
#define GL_PROGRAM_ALU_INSTRUCTIONS_ARB	0x8805
#define GL_PROGRAM_TEX_INSTRUCTIONS_ARB	0x8806
#define GL_PROGRAM_TEX_INDIRECTIONS_ARB	0x8807
#define GL_PROGRAM_NATIVE_ALU_INSTRUCTIONS_ARB	0x8808
#define GL_PROGRAM_NATIVE_TEX_INSTRUCTIONS_ARB	0x8809
#define GL_PROGRAM_NATIVE_TEX_INDIRECTIONS_ARB	0x880A
#define GL_MAX_PROGRAM_ALU_INSTRUCTIONS_ARB	0x880B
#define GL_MAX_PROGRAM_TEX_INSTRUCTIONS_ARB	0x880C
#define GL_MAX_PROGRAM_TEX_INDIRECTIONS_ARB	0x880D
#define GL_MAX_PROGRAM_NATIVE_ALU_INSTRUCTIONS_ARB	0x880E
#define GL_MAX_PROGRAM_NATIVE_TEX_INSTRUCTIONS_ARB	0x880F
#define GL_MAX_PROGRAM_NATIVE_TEX_INDIRECTIONS_ARB	0x8810
#define GL_MAX_TEXTURE_COORDS_ARB	0x8871
#define GL_MAX_TEXTURE_IMAGE_UNITS_ARB	0x8872
#define GL_INVALID_FRAMEBUFFER_OPERATION_EXT	0x0506
#define GL_MAX_RENDERBUFFER_SIZE_EXT	0x84E8
#define GL_FRAMEBUFFER_BINDING_EXT	0x8CA6
#define GL_RENDERBUFFER_BINDING_EXT	0x8CA7
#define GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE_EXT	0x8CD0
#define GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME_EXT	0x8CD1
#define GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL_EXT	0x8CD2
#define GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_CUBE_MAP_FACE_EXT	0x8CD3
#define GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_3D_ZOFFSET_EXT	0x8CD4
#define GL_FRAMEBUFFER_COMPLETE_EXT	0x8CD5
#define GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT	0x8CD6
#define GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT	0x8CD7
#define GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT	0x8CD9
#define GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT	0x8CDA
#define GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT	0x8CDB
#define GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT	0x8CDC
#define GL_FRAMEBUFFER_UNSUPPORTED_EXT	0x8CDD
#define GL_MAX_COLOR_ATTACHMENTS_EXT	0x8CDF
#define GL_COLOR_ATTACHMENT0_EXT	0x8CE0
#define GL_COLOR_ATTACHMENT1_EXT	0x8CE1
#define GL_COLOR_ATTACHMENT2_EXT	0x8CE2
#define GL_COLOR_ATTACHMENT3_EXT	0x8CE3
#define GL_COLOR_ATTACHMENT4_EXT	0x8CE4
#define GL_COLOR_ATTACHMENT5_EXT	0x8CE5
#define GL_COLOR_ATTACHMENT6_EXT	0x8CE6
#define GL_COLOR_ATTACHMENT7_EXT	0x8CE7
#define GL_COLOR_ATTACHMENT8_EXT	0x8CE8
#define GL_COLOR_ATTACHMENT9_EXT	0x8CE9
#define GL_COLOR_ATTACHMENT10_EXT	0x8CEA
#define GL_COLOR_ATTACHMENT11_EXT	0x8CEB
#define GL_COLOR_ATTACHMENT12_EXT	0x8CEC
#define GL_COLOR_ATTACHMENT13_EXT	0x8CED
#define GL_COLOR_ATTACHMENT14_EXT	0x8CEE
#define GL_COLOR_ATTACHMENT15_EXT	0x8CEF
#define GL_DEPTH_ATTACHMENT_EXT	0x8D00
#define GL_STENCIL_ATTACHMENT_EXT	0x8D20
#define GL_FRAMEBUFFER_EXT	0x8D40
#define GL_RENDERBUFFER_EXT	0x8D41
#define GL_RENDERBUFFER_WIDTH_EXT	0x8D42
#define GL_RENDERBUFFER_HEIGHT_EXT	0x8D43
#define GL_RENDERBUFFER_INTERNAL_FORMAT_EXT	0x8D44
#define GL_STENCIL_INDEX1_EXT	0x8D46
#define GL_STENCIL_INDEX4_EXT	0x8D47
#define GL_STENCIL_INDEX8_EXT	0x8D48
#define GL_STENCIL_INDEX16_EXT	0x8D49
#define GL_RENDERBUFFER_RED_SIZE_EXT	0x8D50
#define GL_RENDERBUFFER_GREEN_SIZE_EXT	0x8D51
#define GL_RENDERBUFFER_BLUE_SIZE_EXT	0x8D52
#define GL_RENDERBUFFER_ALPHA_SIZE_EXT	0x8D53
#define GL_RENDERBUFFER_DEPTH_SIZE_EXT	0x8D54
#define GL_RENDERBUFFER_STENCIL_SIZE_EXT	0x8D55

static void* STDCALL expwglGetProcAddress(const char *name);

// helper opengl functions
extern GLenum ( *ldrpglGetError )(void);
extern const GLcharARB * ( *ldrpglGetString )(GLenum name);
// base gl functions
extern void ( *ldrpglAccum )(GLenum op, GLfloat value);
extern void ( *ldrpglAlphaFunc )(GLenum func, GLclampf ref);
extern void ( *ldrpglBegin )(GLenum mode);
extern void ( *ldrpglBindTexture )(GLenum target, GLuint texture);
extern void ( *ldrpglBitmap )(GLsizei width, GLsizei height, GLfloat xorig, GLfloat yorig, GLfloat xmove, GLfloat ymove, const GLubyte *bitmap);
extern void ( *ldrpglBlendFunc )(GLenum sfactor, GLenum dfactor);
extern void ( *ldrpglCallList )(GLuint list);
extern void ( *ldrpglCallLists )(GLsizei n, GLenum type, const GLvoid *lists);
extern void ( *ldrpglClear )(GLbitfield mask);
extern void ( *ldrpglClearAccum )(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
extern void ( *ldrpglClearColor )(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
extern void ( *ldrpglClearDepth )(GLclampd depth);
extern void ( *ldrpglClearIndex )(GLfloat c);
extern void ( *ldrpglClearStencil )(GLint s);
extern GLboolean ( *ldrpglIsEnabled )( GLenum cap );
extern GLboolean ( *ldrpglIsList )( GLuint list );
extern GLboolean ( *ldrpglIsTexture )( GLuint texture );
extern void ( *ldrpglClipPlane )(GLenum plane, const GLdouble *equation);
extern void ( *ldrpglColor3b )(GLbyte red, GLbyte green, GLbyte blue);
extern void ( *ldrpglColor3bv )(const GLbyte *v);
extern void ( *ldrpglColor3d )(GLdouble red, GLdouble green, GLdouble blue);
extern void ( *ldrpglColor3dv )(const GLdouble *v);
extern void ( *ldrpglColor3f )(GLfloat red, GLfloat green, GLfloat blue);
extern void ( *ldrpglColor3fv )(const GLfloat *v);
extern void ( *ldrpglColor3i )(GLint red, GLint green, GLint blue);
extern void ( *ldrpglColor3iv )(const GLint *v);
extern void ( *ldrpglColor3s )(GLshort red, GLshort green, GLshort blue);
extern void ( *ldrpglColor3sv )(const GLshort *v);
extern void ( *ldrpglColor3ub )(GLubyte red, GLubyte green, GLubyte blue);
extern void ( *ldrpglColor3ubv )(const GLubyte *v);
extern void ( *ldrpglColor3ui )(GLuint red, GLuint green, GLuint blue);
extern void ( *ldrpglColor3uiv )(const GLuint *v);
extern void ( *ldrpglColor3us )(GLushort red, GLushort green, GLushort blue);
extern void ( *ldrpglColor3usv )(const GLushort *v);
extern void ( *ldrpglColor4b )(GLbyte red, GLbyte green, GLbyte blue, GLbyte alpha);
extern void ( *ldrpglColor4bv )(const GLbyte *v);
extern void ( *ldrpglColor4d )(GLdouble red, GLdouble green, GLdouble blue, GLdouble alpha);
extern void ( *ldrpglColor4dv )(const GLdouble *v);
extern void ( *ldrpglColor4f )(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
extern void ( *ldrpglColor4fv )(const GLfloat *v);
extern void ( *ldrpglColor4i )(GLint red, GLint green, GLint blue, GLint alpha);
extern void ( *ldrpglColor4iv )(const GLint *v);
extern void ( *ldrpglColor4s )(GLshort red, GLshort green, GLshort blue, GLshort alpha);
extern void ( *ldrpglColor4sv )(const GLshort *v);
extern void ( *ldrpglColor4ub )(GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha);
extern void ( *ldrpglColor4ubv )(const GLubyte *v);
extern void ( *ldrpglColor4ui )(GLuint red, GLuint green, GLuint blue, GLuint alpha);
extern void ( *ldrpglColor4uiv )(const GLuint *v);
extern void ( *ldrpglColor4us )(GLushort red, GLushort green, GLushort blue, GLushort alpha);
extern void ( *ldrpglColor4usv )(const GLushort *v);
extern void ( *ldrpglColorMask )(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha);
extern void ( *ldrpglColorMaterial )(GLenum face, GLenum mode);
extern void ( *ldrpglCopyPixels )(GLint x, GLint y, GLsizei width, GLsizei height, GLenum type);
extern void ( *ldrpglCopyTexImage1D )(GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y, GLsizei width, GLint border);
extern void ( *ldrpglCopyTexImage2D )(GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border);
extern void ( *ldrpglCopyTexSubImage1D )(GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width);
extern void ( *ldrpglCopyTexSubImage2D )(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height);
extern void ( *ldrpglCullFace )(GLenum mode);
extern void ( *ldrpglDeleteLists )(GLuint list, GLsizei range);
extern void ( *ldrpglDeleteTextures )(GLsizei n, const GLuint *textures);
extern void ( *ldrpglDepthFunc )(GLenum func);
extern void ( *ldrpglDepthMask )(GLboolean flag);
extern void ( *ldrpglDepthRange )(GLclampd zNear, GLclampd zFar);
extern void ( *ldrpglDisable )(GLenum cap);
extern void ( *ldrpglDisableClientState )(GLenum array);
extern void ( *ldrpglDrawArrays )(GLenum mode, GLint first, GLsizei count);
extern void ( *ldrpglDrawBuffer )(GLenum mode);
extern void ( *ldrpglDrawPixels )(GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels);
extern void ( *ldrpglEdgeFlag )(GLboolean flag);
extern void ( *ldrpglEdgeFlagPointer )(GLsizei stride, const GLvoid *pointer);
extern void ( *ldrpglEdgeFlagv )(const GLboolean *flag);
extern void ( *ldrpglEnable )(GLenum cap);
extern void ( *ldrpglEnableClientState )(GLenum array);
extern void ( *ldrpglEnd )(void);
extern void ( *ldrpglEndList )(void);
extern void ( *ldrpglEvalCoord1d )(GLdouble u);
extern void ( *ldrpglEvalCoord1dv )(const GLdouble *u);
extern void ( *ldrpglEvalCoord1f )(GLfloat u);
extern void ( *ldrpglEvalCoord1fv )(const GLfloat *u);
extern void ( *ldrpglEvalCoord2d )(GLdouble u, GLdouble v);
extern void ( *ldrpglEvalCoord2dv )(const GLdouble *u);
extern void ( *ldrpglEvalCoord2f )(GLfloat u, GLfloat v);
extern void ( *ldrpglEvalCoord2fv )(const GLfloat *u);
extern void ( *ldrpglEvalMesh1 )(GLenum mode, GLint i1, GLint i2);
extern void ( *ldrpglEvalMesh2 )(GLenum mode, GLint i1, GLint i2, GLint j1, GLint j2);
extern void ( *ldrpglEvalPoint1 )(GLint i);
extern void ( *ldrpglEvalPoint2 )(GLint i, GLint j);
extern void ( *ldrpglFeedbackBuffer )(GLsizei size, GLenum type, GLfloat *buffer);
extern void ( *ldrpglFinish )(void);
extern void ( *ldrpglFlush )(void);
extern void ( *ldrpglFogf )(GLenum pname, GLfloat param);
extern void ( *ldrpglFogfv )(GLenum pname, const GLfloat *params);
extern void ( *ldrpglFogi )(GLenum pname, GLint param);
extern void ( *ldrpglFogiv )(GLenum pname, const GLint *params);
extern void ( *ldrpglFrontFace )(GLenum mode);
extern void ( *ldrpglFrustum )(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar);
extern void ( *ldrpglGenTextures )(GLsizei n, GLuint *textures);
extern void ( *ldrpglGetBooleanv )(GLenum pname, GLboolean *params);
extern void ( *ldrpglGetClipPlane )(GLenum plane, GLdouble *equation);
extern void ( *ldrpglGetDoublev )(GLenum pname, GLdouble *params);
extern void ( *ldrpglGetFloatv )(GLenum pname, GLfloat *params);
extern void ( *ldrpglGetIntegerv )(GLenum pname, GLint *params);
extern void ( *ldrpglGetLightfv )(GLenum light, GLenum pname, GLfloat *params);
extern void ( *ldrpglGetLightiv )(GLenum light, GLenum pname, GLint *params);
extern void ( *ldrpglGetMapdv )(GLenum target, GLenum query, GLdouble *v);
extern void ( *ldrpglGetMapfv )(GLenum target, GLenum query, GLfloat *v);
extern void ( *ldrpglGetMapiv )(GLenum target, GLenum query, GLint *v);
extern void ( *ldrpglGetMaterialfv )(GLenum face, GLenum pname, GLfloat *params);
extern void ( *ldrpglGetMaterialiv )(GLenum face, GLenum pname, GLint *params);
extern void ( *ldrpglGetPixelMapfv )(GLenum map, GLfloat *values);
extern void ( *ldrpglGetPixelMapuiv )(GLenum map, GLuint *values);
extern void ( *ldrpglGetPixelMapusv )(GLenum map, GLushort *values);
extern void ( *ldrpglGetPointerv )(GLenum pname, GLvoid* *params);
extern void ( *ldrpglGetPolygonStipple )(GLubyte *mask);
extern void ( *ldrpglGetTexEnvfv )(GLenum target, GLenum pname, GLfloat *params);
extern void ( *ldrpglGetTexEnviv )(GLenum target, GLenum pname, GLint *params);
extern void ( *ldrpglGetTexGendv )(GLenum coord, GLenum pname, GLdouble *params);
extern void ( *ldrpglGetTexGenfv )(GLenum coord, GLenum pname, GLfloat *params);
extern void ( *ldrpglGetTexGeniv )(GLenum coord, GLenum pname, GLint *params);
extern void ( *ldrpglGetTexImage )(GLenum target, GLint level, GLenum format, GLenum type, GLvoid *pixels);
extern void ( *ldrpglGetTexLevelParameterfv )(GLenum target, GLint level, GLenum pname, GLfloat *params);
extern void ( *ldrpglGetTexLevelParameteriv )(GLenum target, GLint level, GLenum pname, GLint *params);
extern void ( *ldrpglGetTexParameterfv )(GLenum target, GLenum pname, GLfloat *params);
extern void ( *ldrpglGetTexParameteriv )(GLenum target, GLenum pname, GLint *params);
extern void ( *ldrpglHint )(GLenum target, GLenum mode);
extern void ( *ldrpglIndexMask )(GLuint mask);
extern void ( *ldrpglIndexPointer )(GLenum type, GLsizei stride, const GLvoid *pointer);
extern void ( *ldrpglIndexd )(GLdouble c);
extern void ( *ldrpglIndexdv )(const GLdouble *c);
extern void ( *ldrpglIndexf )(GLfloat c);
extern void ( *ldrpglIndexfv )(const GLfloat *c);
extern void ( *ldrpglIndexi )(GLint c);
extern void ( *ldrpglIndexiv )(const GLint *c);
extern void ( *ldrpglIndexs )(GLshort c);
extern void ( *ldrpglIndexsv )(const GLshort *c);
extern void ( *ldrpglIndexub )(GLubyte c);
extern void ( *ldrpglIndexubv )(const GLubyte *c);
extern void ( *ldrpglInitNames )(void);
extern void ( *ldrpglInterleavedArrays )(GLenum format, GLsizei stride, const GLvoid *pointer);
extern void ( *ldrpglLightModelf )(GLenum pname, GLfloat param);
extern void ( *ldrpglLightModelfv )(GLenum pname, const GLfloat *params);
extern void ( *ldrpglLightModeli )(GLenum pname, GLint param);
extern void ( *ldrpglLightModeliv )(GLenum pname, const GLint *params);
extern void ( *ldrpglLightf )(GLenum light, GLenum pname, GLfloat param);
extern void ( *ldrpglLightfv )(GLenum light, GLenum pname, const GLfloat *params);
extern void ( *ldrpglLighti )(GLenum light, GLenum pname, GLint param);
extern void ( *ldrpglLightiv )(GLenum light, GLenum pname, const GLint *params);
extern void ( *ldrpglLineStipple )(GLint factor, GLushort pattern);
extern void ( *ldrpglLineWidth )(GLfloat width);
extern void ( *ldrpglListBase )(GLuint base);
extern void ( *ldrpglLoadIdentity )(void);
extern void ( *ldrpglLoadMatrixd )(const GLdouble *m);
extern void ( *ldrpglLoadMatrixf )(const GLfloat *m);
extern void ( *ldrpglLoadName )(GLuint name);
extern void ( *ldrpglLogicOp )(GLenum opcode);
extern void ( *ldrpglMap1d )(GLenum target, GLdouble u1, GLdouble u2, GLint stride, GLint order, const GLdouble *points);
extern void ( *ldrpglMap1f )(GLenum target, GLfloat u1, GLfloat u2, GLint stride, GLint order, const GLfloat *points);
extern void ( *ldrpglMap2d )(GLenum target, GLdouble u1, GLdouble u2, GLint ustride, GLint uorder, GLdouble v1, GLdouble v2, GLint vstride, GLint vorder, const GLdouble *points);
extern void ( *ldrpglMap2f )(GLenum target, GLfloat u1, GLfloat u2, GLint ustride, GLint uorder, GLfloat v1, GLfloat v2, GLint vstride, GLint vorder, const GLfloat *points);
extern void ( *ldrpglMapGrid1d )(GLint un, GLdouble u1, GLdouble u2);
extern void ( *ldrpglMapGrid1f )(GLint un, GLfloat u1, GLfloat u2);
extern void ( *ldrpglMapGrid2d )(GLint un, GLdouble u1, GLdouble u2, GLint vn, GLdouble v1, GLdouble v2);
extern void ( *ldrpglMapGrid2f )(GLint un, GLfloat u1, GLfloat u2, GLint vn, GLfloat v1, GLfloat v2);
extern void ( *ldrpglMaterialf )(GLenum face, GLenum pname, GLfloat param);
extern void ( *ldrpglMaterialfv )(GLenum face, GLenum pname, const GLfloat *params);
extern void ( *ldrpglMateriali )(GLenum face, GLenum pname, GLint param);
extern void ( *ldrpglMaterialiv )(GLenum face, GLenum pname, const GLint *params);
extern void ( *ldrpglMatrixMode )(GLenum mode);
extern void ( *ldrpglMultMatrixd )(const GLdouble *m);
extern void ( *ldrpglMultMatrixf )(const GLfloat *m);
extern void ( *ldrpglNewList )(GLuint list, GLenum mode);
extern void ( *ldrpglNormal3b )(GLbyte nx, GLbyte ny, GLbyte nz);
extern void ( *ldrpglNormal3bv )(const GLbyte *v);
extern void ( *ldrpglNormal3d )(GLdouble nx, GLdouble ny, GLdouble nz);
extern void ( *ldrpglNormal3dv )(const GLdouble *v);
extern void ( *ldrpglNormal3f )(GLfloat nx, GLfloat ny, GLfloat nz);
extern void ( *ldrpglNormal3fv )(const GLfloat *v);
extern void ( *ldrpglNormal3i )(GLint nx, GLint ny, GLint nz);
extern void ( *ldrpglNormal3iv )(const GLint *v);
extern void ( *ldrpglNormal3s )(GLshort nx, GLshort ny, GLshort nz);
extern void ( *ldrpglNormal3sv )(const GLshort *v);
extern void ( *ldrpglOrtho )(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar);
extern void ( *ldrpglPassThrough )(GLfloat token);
extern void ( *ldrpglPixelMapfv )(GLenum map, GLsizei mapsize, const GLfloat *values);
extern void ( *ldrpglPixelMapuiv )(GLenum map, GLsizei mapsize, const GLuint *values);
extern void ( *ldrpglPixelMapusv )(GLenum map, GLsizei mapsize, const GLushort *values);
extern void ( *ldrpglPixelStoref )(GLenum pname, GLfloat param);
extern void ( *ldrpglPixelStorei )(GLenum pname, GLint param);
extern void ( *ldrpglPixelTransferf )(GLenum pname, GLfloat param);
extern void ( *ldrpglPixelTransferi )(GLenum pname, GLint param);
extern void ( *ldrpglPixelZoom )(GLfloat xfactor, GLfloat yfactor);
extern void ( *ldrpglPointSize )(GLfloat size);
extern void ( *ldrpglPolygonMode )(GLenum face, GLenum mode);
extern void ( *ldrpglPolygonOffset )(GLfloat factor, GLfloat units);
extern void ( *ldrpglPolygonStipple )(const GLubyte *mask);
extern void ( *ldrpglPopAttrib )(void);
extern void ( *ldrpglPopClientAttrib )(void);
extern void ( *ldrpglPopMatrix )(void);
extern void ( *ldrpglPopName )(void);
extern void ( *ldrpglPushAttrib )(GLbitfield mask);
extern void ( *ldrpglPushClientAttrib )(GLbitfield mask);
extern void ( *ldrpglPushMatrix )(void);
extern void ( *ldrpglPushName )(GLuint name);
extern void ( *ldrpglRasterPos2d )(GLdouble x, GLdouble y);
extern void ( *ldrpglRasterPos2dv )(const GLdouble *v);
extern void ( *ldrpglRasterPos2f )(GLfloat x, GLfloat y);
extern void ( *ldrpglRasterPos2fv )(const GLfloat *v);
extern void ( *ldrpglRasterPos2i )(GLint x, GLint y);
extern void ( *ldrpglRasterPos2iv )(const GLint *v);
extern void ( *ldrpglRasterPos2s )(GLshort x, GLshort y);
extern void ( *ldrpglRasterPos2sv )(const GLshort *v);
extern void ( *ldrpglRasterPos3d )(GLdouble x, GLdouble y, GLdouble z);
extern void ( *ldrpglRasterPos3dv )(const GLdouble *v);
extern void ( *ldrpglRasterPos3f )(GLfloat x, GLfloat y, GLfloat z);
extern void ( *ldrpglRasterPos3fv )(const GLfloat *v);
extern void ( *ldrpglRasterPos3i )(GLint x, GLint y, GLint z);
extern void ( *ldrpglRasterPos3iv )(const GLint *v);
extern void ( *ldrpglRasterPos3s )(GLshort x, GLshort y, GLshort z);
extern void ( *ldrpglRasterPos3sv )(const GLshort *v);
extern void ( *ldrpglRasterPos4d )(GLdouble x, GLdouble y, GLdouble z, GLdouble w);
extern void ( *ldrpglRasterPos4dv )(const GLdouble *v);
extern void ( *ldrpglRasterPos4f )(GLfloat x, GLfloat y, GLfloat z, GLfloat w);
extern void ( *ldrpglRasterPos4fv )(const GLfloat *v);
extern void ( *ldrpglRasterPos4i )(GLint x, GLint y, GLint z, GLint w);
extern void ( *ldrpglRasterPos4iv )(const GLint *v);
extern void ( *ldrpglRasterPos4s )(GLshort x, GLshort y, GLshort z, GLshort w);
extern void ( *ldrpglRasterPos4sv )(const GLshort *v);
extern void ( *ldrpglReadBuffer )(GLenum mode);
extern void ( *ldrpglReadPixels )(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels);
extern void ( *ldrpglRectd )(GLdouble x1, GLdouble y1, GLdouble x2, GLdouble y2);
extern void ( *ldrpglRectdv )(const GLdouble *v1, const GLdouble *v2);
extern void ( *ldrpglRectf )(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2);
extern void ( *ldrpglRectfv )(const GLfloat *v1, const GLfloat *v2);
extern void ( *ldrpglRecti )(GLint x1, GLint y1, GLint x2, GLint y2);
extern void ( *ldrpglRectiv )(const GLint *v1, const GLint *v2);
extern void ( *ldrpglRects )(GLshort x1, GLshort y1, GLshort x2, GLshort y2);
extern void ( *ldrpglRectsv )(const GLshort *v1, const GLshort *v2);
extern void ( *ldrpglRotated )(GLdouble angle, GLdouble x, GLdouble y, GLdouble z);
extern void ( *ldrpglRotatef )(GLfloat angle, GLfloat x, GLfloat y, GLfloat z);
extern void ( *ldrpglScaled )(GLdouble x, GLdouble y, GLdouble z);
extern void ( *ldrpglScalef )(GLfloat x, GLfloat y, GLfloat z);
extern void ( *ldrpglScissor )(GLint x, GLint y, GLsizei width, GLsizei height);
extern void ( *ldrpglSelectBuffer )(GLsizei size, GLuint *buffer);
extern void ( *ldrpglShadeModel )(GLenum mode);
extern void ( *ldrpglStencilFunc )(GLenum func, GLint ref, GLuint mask);
extern void ( *ldrpglStencilMask )(GLuint mask);
extern void ( *ldrpglStencilOp )(GLenum fail, GLenum zfail, GLenum zpass);
extern void ( *ldrpglTexCoord1d )(GLdouble s);
extern void ( *ldrpglTexCoord1dv )(const GLdouble *v);
extern void ( *ldrpglTexCoord1f )(GLfloat s);
extern void ( *ldrpglTexCoord1fv )(const GLfloat *v);
extern void ( *ldrpglTexCoord1i )(GLint s);
extern void ( *ldrpglTexCoord1iv )(const GLint *v);
extern void ( *ldrpglTexCoord1s )(GLshort s);
extern void ( *ldrpglTexCoord1sv )(const GLshort *v);
extern void ( *ldrpglTexCoord2d )(GLdouble s, GLdouble t);
extern void ( *ldrpglTexCoord2dv )(const GLdouble *v);
extern void ( *ldrpglTexCoord2f )(GLfloat s, GLfloat t);
extern void ( *ldrpglTexCoord2fv )(const GLfloat *v);
extern void ( *ldrpglTexCoord2i )(GLint s, GLint t);
extern void ( *ldrpglTexCoord2iv )(const GLint *v);
extern void ( *ldrpglTexCoord2s )(GLshort s, GLshort t);
extern void ( *ldrpglTexCoord2sv )(const GLshort *v);
extern void ( *ldrpglTexCoord3d )(GLdouble s, GLdouble t, GLdouble r);
extern void ( *ldrpglTexCoord3dv )(const GLdouble *v);
extern void ( *ldrpglTexCoord3f )(GLfloat s, GLfloat t, GLfloat r);
extern void ( *ldrpglTexCoord3fv )(const GLfloat *v);
extern void ( *ldrpglTexCoord3i )(GLint s, GLint t, GLint r);
extern void ( *ldrpglTexCoord3iv )(const GLint *v);
extern void ( *ldrpglTexCoord3s )(GLshort s, GLshort t, GLshort r);
extern void ( *ldrpglTexCoord3sv )(const GLshort *v);
extern void ( *ldrpglTexCoord4d )(GLdouble s, GLdouble t, GLdouble r, GLdouble q);
extern void ( *ldrpglTexCoord4dv )(const GLdouble *v);
extern void ( *ldrpglTexCoord4f )(GLfloat s, GLfloat t, GLfloat r, GLfloat q);
extern void ( *ldrpglTexCoord4fv )(const GLfloat *v);
extern void ( *ldrpglTexCoord4i )(GLint s, GLint t, GLint r, GLint q);
extern void ( *ldrpglTexCoord4iv )(const GLint *v);
extern void ( *ldrpglTexCoord4s )(GLshort s, GLshort t, GLshort r, GLshort q);
extern void ( *ldrpglTexCoord4sv )(const GLshort *v);
extern void ( *ldrpglTexEnvf )(GLenum target, GLenum pname, GLfloat param);
extern void ( *ldrpglTexEnvfv )(GLenum target, GLenum pname, const GLfloat *params);
extern void ( *ldrpglTexEnvi )(GLenum target, GLenum pname, GLint param);
extern void ( *ldrpglTexEnviv )(GLenum target, GLenum pname, const GLint *params);
extern void ( *ldrpglTexGend )(GLenum coord, GLenum pname, GLdouble param);
extern void ( *ldrpglTexGendv )(GLenum coord, GLenum pname, const GLdouble *params);
extern void ( *ldrpglTexGenf )(GLenum coord, GLenum pname, GLfloat param);
extern void ( *ldrpglTexGenfv )(GLenum coord, GLenum pname, const GLfloat *params);
extern void ( *ldrpglTexGeni )(GLenum coord, GLenum pname, GLint param);
extern void ( *ldrpglTexGeniv )(GLenum coord, GLenum pname, const GLint *params);
extern void ( *ldrpglTexImage1D )(GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
extern void ( *ldrpglTexImage2D )(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
extern void ( *ldrpglTexParameterf )(GLenum target, GLenum pname, GLfloat param);
extern void ( *ldrpglTexParameterfv )(GLenum target, GLenum pname, const GLfloat *params);
extern void ( *ldrpglTexParameteri )(GLenum target, GLenum pname, GLint param);
extern void ( *ldrpglTexParameteriv )(GLenum target, GLenum pname, const GLint *params);
extern void ( *ldrpglTexSubImage1D )(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const GLvoid *pixels);
extern void ( *ldrpglTexSubImage2D )(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels);
extern void ( *ldrpglTranslated )(GLdouble x, GLdouble y, GLdouble z);
extern void ( *ldrpglTranslatef )(GLfloat x, GLfloat y, GLfloat z);
extern void ( *ldrpglVertex2d )(GLdouble x, GLdouble y);
extern void ( *ldrpglVertex2dv )(const GLdouble *v);
extern void ( *ldrpglVertex2f )(GLfloat x, GLfloat y);
extern void ( *ldrpglVertex2fv )(const GLfloat *v);
extern void ( *ldrpglVertex2i )(GLint x, GLint y);
extern void ( *ldrpglVertex2iv )(const GLint *v);
extern void ( *ldrpglVertex2s )(GLshort x, GLshort y);
extern void ( *ldrpglVertex2sv )(const GLshort *v);
extern void ( *ldrpglVertex3d )(GLdouble x, GLdouble y, GLdouble z);
extern void ( *ldrpglVertex3dv )(const GLdouble *v);
extern void ( *ldrpglVertex3f )(GLfloat x, GLfloat y, GLfloat z);
extern void ( *ldrpglVertex3fv )(const GLfloat *v);
extern void ( *ldrpglVertex3i )(GLint x, GLint y, GLint z);
extern void ( *ldrpglVertex3iv )(const GLint *v);
extern void ( *ldrpglVertex3s )(GLshort x, GLshort y, GLshort z);
extern void ( *ldrpglVertex3sv )(const GLshort *v);
extern void ( *ldrpglVertex4d )(GLdouble x, GLdouble y, GLdouble z, GLdouble w);
extern void ( *ldrpglVertex4dv )(const GLdouble *v);
extern void ( *ldrpglVertex4f )(GLfloat x, GLfloat y, GLfloat z, GLfloat w);
extern void ( *ldrpglVertex4fv )(const GLfloat *v);
extern void ( *ldrpglVertex4i )(GLint x, GLint y, GLint z, GLint w);
extern void ( *ldrpglVertex4iv )(const GLint *v);
extern void ( *ldrpglVertex4s )(GLshort x, GLshort y, GLshort z, GLshort w);
extern void ( *ldrpglVertex4sv )(const GLshort *v);
extern void ( *ldrpglViewport )(GLint x, GLint y, GLsizei width, GLsizei height);
extern void ( *ldrpglPointParameterfEXT)( GLenum param, GLfloat value );
extern void ( *ldrpglPointParameterfvEXT)( GLenum param, const GLfloat *value );
extern void ( *ldrpglLockArraysEXT) (int , int);
extern void ( *ldrpglUnlockArraysEXT) (void);
extern void ( *ldrpglActiveTextureARB)( GLenum );
extern void ( *ldrpglClientActiveTextureARB)( GLenum );
extern void ( *ldrpglGetCompressedTexImage)( GLenum target, GLint lod, const void* data );
extern void ( *ldrpglDrawRangeElements)( GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const void *indices );
extern void ( *ldrpglDrawRangeElementsEXT)( GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const void *indices );
extern void ( *ldrpglDrawElements)(GLenum mode, GLsizei count, GLenum type, const void *indices);
extern void ( *ldrpglVertexPointer)(GLint size, GLenum type, GLsizei stride, const void *ptr);
extern void ( *ldrpglNormalPointer)(GLenum type, GLsizei stride, const void *ptr);
extern void ( *ldrpglColorPointer)(GLint size, GLenum type, GLsizei stride, const void *ptr);
extern void ( *ldrpglTexCoordPointer)(GLint size, GLenum type, GLsizei stride, const void *ptr);
extern void ( *ldrpglArrayElement)(GLint i);
extern void ( *ldrpglMultiTexCoord1f) (GLenum, GLfloat);
extern void ( *ldrpglMultiTexCoord2f) (GLenum, GLfloat, GLfloat);
extern void ( *ldrpglMultiTexCoord3f) (GLenum, GLfloat, GLfloat, GLfloat);
extern void ( *ldrpglMultiTexCoord4f) (GLenum, GLfloat, GLfloat, GLfloat, GLfloat);
extern void ( *ldrpglActiveTexture) (GLenum);
extern void ( *ldrpglClientActiveTexture) (GLenum);
extern void ( *ldrpglCompressedTexImage3DARB)(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const void *data);
extern void ( *ldrpglCompressedTexImage2DARB)(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const void *data);
extern void ( *ldrpglCompressedTexImage1DARB)(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLint border, GLsizei imageSize, const void *data);
extern void ( *ldrpglCompressedTexSubImage3DARB)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const void *data);
extern void ( *ldrpglCompressedTexSubImage2DARB)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void *data);
extern void ( *ldrpglCompressedTexSubImage1DARB)(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, const void *data);
extern void ( *ldrpglDeleteObjectARB)(GLhandleARB obj);
extern GLhandleARB ( *ldrpglGetHandleARB)(GLenum pname);
extern void ( *ldrpglDetachObjectARB)(GLhandleARB containerObj, GLhandleARB attachedObj);
extern GLhandleARB ( *ldrpglCreateShaderObjectARB)(GLenum shaderType);
extern void ( *ldrpglShaderSourceARB)(GLhandleARB shaderObj, GLsizei count, const GLcharARB **string, const GLint *length);
extern void ( *ldrpglCompileShaderARB)(GLhandleARB shaderObj);
extern GLhandleARB ( *ldrpglCreateProgramObjectARB)(void);
extern void ( *ldrpglAttachObjectARB)(GLhandleARB containerObj, GLhandleARB obj);
extern void ( *ldrpglLinkProgramARB)(GLhandleARB programObj);
extern void ( *ldrpglUseProgramObjectARB)(GLhandleARB programObj);
extern void ( *ldrpglValidateProgramARB)(GLhandleARB programObj);
extern void ( *ldrpglBindProgramARB)(GLenum target, GLuint program);
extern void ( *ldrpglDeleteProgramsARB)(GLsizei n, const GLuint *programs);
extern void ( *ldrpglGenProgramsARB)(GLsizei n, GLuint *programs);
extern void ( *ldrpglProgramStringARB)(GLenum target, GLenum format, GLsizei len, const void *string);
extern void ( *ldrpglProgramEnvParameter4fARB)(GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
extern void ( *ldrpglProgramLocalParameter4fARB)(GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
extern void ( *ldrpglGetProgramivARB)( GLenum target, GLenum pname, GLint *params );
extern void ( *ldrpglUniform1fARB)(GLint location, GLfloat v0);
extern void ( *ldrpglUniform2fARB)(GLint location, GLfloat v0, GLfloat v1);
extern void ( *ldrpglUniform3fARB)(GLint location, GLfloat v0, GLfloat v1, GLfloat v2);
extern void ( *ldrpglUniform4fARB)(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);
extern void ( *ldrpglUniform1iARB)(GLint location, GLint v0);
extern void ( *ldrpglUniform2iARB)(GLint location, GLint v0, GLint v1);
extern void ( *ldrpglUniform3iARB)(GLint location, GLint v0, GLint v1, GLint v2);
extern void ( *ldrpglUniform4iARB)(GLint location, GLint v0, GLint v1, GLint v2, GLint v3);
extern void ( *ldrpglUniform1fvARB)(GLint location, GLsizei count, const GLfloat *value);
extern void ( *ldrpglUniform2fvARB)(GLint location, GLsizei count, const GLfloat *value);
extern void ( *ldrpglUniform3fvARB)(GLint location, GLsizei count, const GLfloat *value);
extern void ( *ldrpglUniform4fvARB)(GLint location, GLsizei count, const GLfloat *value);
extern void ( *ldrpglUniform1ivARB)(GLint location, GLsizei count, const GLint *value);
extern void ( *ldrpglUniform2ivARB)(GLint location, GLsizei count, const GLint *value);
extern void ( *ldrpglUniform3ivARB)(GLint location, GLsizei count, const GLint *value);
extern void ( *ldrpglUniform4ivARB)(GLint location, GLsizei count, const GLint *value);
extern void ( *ldrpglUniformMatrix2fvARB)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
extern void ( *ldrpglUniformMatrix3fvARB)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
extern void ( *ldrpglUniformMatrix4fvARB)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
extern void ( *ldrpglGetObjectParameterfvARB)(GLhandleARB obj, GLenum pname, GLfloat *params);
extern void ( *ldrpglGetObjectParameterivARB)(GLhandleARB obj, GLenum pname, GLint *params);
extern void ( *ldrpglGetInfoLogARB)(GLhandleARB obj, GLsizei maxLength, GLsizei *length, GLcharARB *infoLog);
extern void ( *ldrpglGetAttachedObjectsARB)(GLhandleARB containerObj, GLsizei maxCount, GLsizei *count, GLhandleARB *obj);
extern GLint ( *ldrpglGetUniformLocationARB)(GLhandleARB programObj, const GLcharARB *name);
extern void ( *ldrpglGetActiveUniformARB)(GLhandleARB programObj, GLuint index, GLsizei maxLength, GLsizei *length, GLint *size, GLenum *type, GLcharARB *name);
extern void ( *ldrpglGetUniformfvARB)(GLhandleARB programObj, GLint location, GLfloat *params);
extern void ( *ldrpglGetUniformivARB)(GLhandleARB programObj, GLint location, GLint *params);
extern void ( *ldrpglGetShaderSourceARB)(GLhandleARB obj, GLsizei maxLength, GLsizei *length, GLcharARB *source);
extern void ( *ldrpglTexImage3D)( GLenum target, GLint level, GLenum internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const void *pixels );
extern void ( *ldrpglTexSubImage3D)( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void *pixels );
extern void ( *ldrpglCopyTexSubImage3D)( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height );
extern void ( *ldrpglBlendEquationEXT)(GLenum);
extern void ( *ldrpglStencilOpSeparate)(GLenum, GLenum, GLenum, GLenum);
extern void ( *ldrpglStencilFuncSeparate)(GLenum, GLenum, GLint, GLuint);
extern void ( *ldrpglActiveStencilFaceEXT)(GLenum);
extern void ( *ldrpglVertexAttribPointerARB)(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void *pointer);
extern void ( *ldrpglEnableVertexAttribArrayARB)(GLuint index);
extern void ( *ldrpglDisableVertexAttribArrayARB)(GLuint index);
extern void ( *ldrpglBindAttribLocationARB)(GLhandleARB programObj, GLuint index, const GLcharARB *name);
extern void ( *ldrpglGetActiveAttribARB)(GLhandleARB programObj, GLuint index, GLsizei maxLength, GLsizei *length, GLint *size, GLenum *type, GLcharARB *name);
extern GLint ( *ldrpglGetAttribLocationARB)(GLhandleARB programObj, const GLcharARB *name);
extern void ( *ldrpglBindBufferARB) (GLenum target, GLuint buffer);
extern void ( *ldrpglDeleteBuffersARB) (GLsizei n, const GLuint *buffers);
extern void ( *ldrpglGenBuffersARB) (GLsizei n, GLuint *buffers);
extern GLboolean ( *ldrpglIsBufferARB) (GLuint buffer);
extern void* ( *ldrpglMapBufferARB) (GLenum target, GLenum access);
extern GLboolean ( *ldrpglUnmapBufferARB) (GLenum target);
extern void ( *ldrpglBufferDataARB) (GLenum target, GLsizeiptrARB size, const void *data, GLenum usage);
extern void ( *ldrpglBufferSubDataARB) (GLenum target, GLintptrARB offset, GLsizeiptrARB size, const void *data);
extern void ( *ldrpglGenQueriesARB) (GLsizei n, GLuint *ids);
extern void ( *ldrpglDeleteQueriesARB) (GLsizei n, const GLuint *ids);
extern GLboolean ( *ldrpglIsQueryARB) (GLuint id);
extern void ( *ldrpglBeginQueryARB) (GLenum target, GLuint id);
extern void ( *ldrpglEndQueryARB) (GLenum target);
extern void ( *ldrpglGetQueryivARB) (GLenum target, GLenum pname, GLint *params);
extern void ( *ldrpglGetQueryObjectivARB) (GLuint id, GLenum pname, GLint *params);
extern void ( *ldrpglGetQueryObjectuivARB) (GLuint id, GLenum pname, GLuint *params);
extern void ( * ldrpglSelectTextureSGIS )( GLenum );
extern void ( * ldrpglMTexCoord2fSGIS )( GLenum, GLfloat, GLfloat );
extern void ( * ldrpglSwapInterval )( int interval );
extern GLboolean ( *ldrpglIsRenderbuffer )(GLuint renderbuffer);
extern void ( *ldrpglBindRenderbuffer )(GLenum target, GLuint renderbuffer);
extern void ( *ldrpglDeleteRenderbuffers )(GLsizei n, const GLuint *renderbuffers);
extern void ( *ldrpglGenRenderbuffers )(GLsizei n, GLuint *renderbuffers);
extern void ( *ldrpglRenderbufferStorage )(GLenum target, GLenum internalformat, GLsizei width, GLsizei height);
extern void ( *ldrpglRenderbufferStorageMultisample )(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height);
extern void ( *ldrpglGetRenderbufferParameteriv )(GLenum target, GLenum pname, GLint *params);
extern GLboolean ( *ldrpglIsFramebuffer )(GLuint framebuffer);
extern void ( *ldrpglBindFramebuffer )(GLenum target, GLuint framebuffer);
extern void ( *ldrpglDeleteFramebuffers )(GLsizei n, const GLuint *framebuffers);
extern void ( *ldrpglGenFramebuffers )(GLsizei n, GLuint *framebuffers);
extern GLenum ( *ldrpglCheckFramebufferStatus )(GLenum target);
extern void ( *ldrpglFramebufferTexture1D )(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
extern void ( *ldrpglFramebufferTexture2D )(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
extern void ( *ldrpglFramebufferTexture3D )(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLint layer);
extern void ( *ldrpglFramebufferTextureLayer )(GLenum target, GLenum attachment, GLuint texture, GLint level, GLint layer);
extern void ( *ldrpglFramebufferRenderbuffer )(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer);
extern void ( *ldrpglGetFramebufferAttachmentParameteriv )(GLenum target, GLenum attachment, GLenum pname, GLint *params);
extern void ( *ldrpglBlitFramebuffer )(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter);
extern void ( *ldrpglGenerateMipmap )(GLenum target);
extern void ( *ldrpglGetCompressedTexImageARB) (GLenum target, GLint level, GLvoid *img);
extern void ( *ldrpglCopyTexSubImage3DEXT) (GLenum a, GLint b, GLint c, GLint d, GLint e, GLint f, GLint g, GLsizei h, GLsizei i);
extern void ( *ldrpglTexSubImage3DEXT)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const GLvoid *pixels);
extern void ( *ldrpglTexImage3DEXT)(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
extern void ( *ldrpglMultiTexCoord1fARB)(GLenum target, GLfloat s);
extern void ( *ldrpglMultiTexCoord2fARB)(GLenum target, GLfloat s, GLfloat t);
extern void ( *ldrpglMultiTexCoord3fARB)(GLenum target, GLfloat s, GLfloat t, GLfloat r);
extern void ( *ldrpglMultiTexCoord4fARB)(GLenum target, GLfloat s, GLfloat t, GLfloat r, GLfloat q);
extern void ( *ldrpglBindVertexArray)(GLuint array);
extern void ( *ldrpglDeleteVertexArrays)(GLsizei n,  const GLuint *arrays);
extern void ( *ldrpglGenVertexArrays)(GLsizei n, GLuint *arrays);
extern GLboolean ( *ldrpglIsVertexArray)(GLuint array);
extern void ( *ldrpglDebugMessageControlARB)(GLenum source, GLenum type, GLenum severity, GLsizei count, const GLuint* ids, GLboolean enabled);
extern void ( *ldrpglDebugMessageInsertARB)(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* buf);
extern void ( *ldrpglDebugMessageCallbackARB)(GLvoid *callback, GLvoid *userParam);
extern GLuint ( *ldrpglGetDebugMessageLogARB)(GLuint count, GLsizei bufsize, GLenum *sources, GLenum *types, GLuint *ids, GLenum *severities, GLsizei *lengths, GLchar *messageLog);
extern void ( *ldrpglVertexAttrib2f)(GLuint index, GLfloat v0, GLfloat v1);
extern void ( *ldrpglVertexAttrib2fv)(GLuint index, const GLfloat *v);
extern void ( *ldrpglVertexAttrib3fv)(GLuint index, const GLfloat *v);
