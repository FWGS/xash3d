#ifdef XASH_GLES2_RENDER
#include "common.h"
#include "gl_local.h"

typedef struct program_s
{
	GLuint id;
	GLint u_color;
	GLint u_mvMtx;
	GLint u_projMtx;
} program_t;

static program_t pr_2d;
static program_t pr_world;
static program_t pr_part;
static program_t pr_beam;
static program_t pr_studio;
static program_t *glsl_CurProgram;
GLint u_screen = -1;

GLuint R_CreateShader( const char *src, GLint type )
{
	GLuint id = pglCreateShader( type );
	pglShaderSource( id, 1, &src, NULL );
	pglCompileShader( id );

	GLint compile_ok = GL_FALSE;
	pglGetShaderiv( id, GL_COMPILE_STATUS, &compile_ok );
	if( compile_ok == GL_FALSE )
	{
		char log[1024];
		pglGetShaderInfoLog( id, 1024, NULL, log );
		MsgDev( D_ERROR, "Error glCompileShader: %d:\n%s\n", id, log );
		pglDeleteShader( id );
		return -1;
	}

	return id;
}

GLuint R_CreateProgram( const char *vert, const char *frag )
{
	GLuint vs = R_CreateShader( vert,GL_VERTEX_SHADER );
	GLuint fs = R_CreateShader( frag,GL_FRAGMENT_SHADER );
	GLuint id = pglCreateProgram();

	pglAttachShader( id, vs );
	pglAttachShader( id, fs );
	pglLinkProgram( id );
	GLint link_ok = GL_FALSE;
	pglGetProgramiv( id, GL_LINK_STATUS, &link_ok );
	if( !link_ok )
	{
		MsgDev( D_ERROR, "Error glLinkProgram: %x\n", id );
		return -1;
	}

	return id;
}

void R_InitShaders()
{
	const char Vert2DS[]=
	"#ifdef GL_ES\n"
	"precision mediump float;\n"
	"precision mediump int;\n"
	"#endif\n"
	"attribute vec4 a_position;\n"\
	"attribute vec2 a_uv;\n"\
	"varying vec2 v_uv;\n"\
	"uniform vec2 u_screen;\n"\
	"void main(){\n"\
	"	gl_Position = vec4((a_position.xy*u_screen) - 1.0, 0.0, 1.0);\n"\
	"	gl_Position.y *= -1.0;\n"\
	"	v_uv = a_uv;\n"\
	"}";
	const char Frag2DS[]=
	"#ifdef GL_ES\n"
	"precision mediump float;\n"
	"precision mediump int;\n"
	"#endif\n"
	"varying vec2 v_uv;\n"\
	"uniform sampler2D u_tex;\n"\
	"uniform vec4 u_color;\n"\
	"void main(){\n"\
	"	gl_FragColor = texture2D(u_tex,v_uv)*u_color;\n"\
	"}";

	const char WorldVertS[] =
	"#ifdef GL_ES\n"
	"precision mediump float;\n"
	"precision mediump int;\n"
	"#endif\n"
	"attribute vec4 a_position;\n"\
	"attribute vec2 a_uv;\n"\
	"varying vec2 v_uv;\n"\
	"uniform mat4 u_mvMtx;\n"\
	"uniform mat4 u_projMtx;\n"\
	"void main(){\n"\
	"	gl_Position = u_projMtx*u_mvMtx*a_position;\n"\
	"	v_uv = a_uv;\n"\
	"}";
	const char WorldFragS[] =
	"#ifdef GL_ES\n"
	"precision mediump float;\n"
	"precision mediump int;\n"
	"#endif\n"
	"varying vec2 v_uv;\n"\
	"uniform sampler2D u_tex;\n"\
	"void main(){\n"\
	"	gl_FragColor = texture2D(u_tex,v_uv);\n"\
	"}";

	const char ParticlesVertS[] =
	"#ifdef GL_ES\n"
	"precision mediump float;\n"
	"precision mediump int;\n"
	"#endif\n"
	"attribute vec4 a_position;\n"\
	"attribute vec2 a_uv;\n"\
	"varying vec2 v_uv;\n"\
	"uniform mat4 u_mvMtx;\n"\
	"uniform mat4 u_projMtx;\n"\
	"void main(){\n"\
	"	gl_Position = u_projMtx*u_mvMtx*a_position;\n"\
	"	v_uv = a_uv;\n"\
	"}";
	const char ParticlesFragS[] =
	"#ifdef GL_ES\n"
	"precision mediump float;\n"
	"precision mediump int;\n"
	"#endif\n"
	"varying vec2 v_uv;\n"\
	"uniform sampler2D u_tex;\n"\
	"uniform vec4 u_color;\n"\
	"void main(){\n"\
	"	vec4 tex = texture2D(u_tex,v_uv);\n"\
	"	if(tex.a == 0.0)\n"\
	"		discard;\n"\
	"	gl_FragColor = tex*u_color;\n"\
	"}";

	const char BeamVertS[] =
	"#ifdef GL_ES\n"
	"precision mediump float;\n"
	"precision mediump int;\n"
	"#endif\n"
	"attribute vec4 a_position;\n"\
	"attribute vec2 a_uv;\n"\
	"attribute vec4 a_color;\n"\
	"varying vec2 v_uv;\n"\
	"varying vec4 v_color;\n"\
	"uniform mat4 u_mvMtx;\n"\
	"uniform mat4 u_projMtx;\n"\
	"void main(){\n"\
	"	gl_Position = u_projMtx*u_mvMtx*a_position;\n"\
	"	v_color = a_color;\n"\
	"	v_uv = a_uv;\n"\
	"}";
	const char BeamFragS[]=
	"#ifdef GL_ES\n"
	"precision mediump float;\n"
	"precision mediump int;\n"
	"#endif\n"
	"varying vec2 v_uv;\n"\
	"varying vec4 v_color;\n"\
	"uniform sampler2D u_tex;\n"\
	"void main(){\n"\
	"	gl_FragColor = texture2D(u_tex,v_uv)*v_color;\n"\
	"}";

	const char StudioVertS[] =
	"#ifdef GL_ES\n"
	"precision mediump float;\n"
	"precision mediump int;\n"
	"#endif\n"
	"attribute vec4 a_position;\n"\
	"attribute vec2 a_uv;\n"\
	"attribute vec4 a_color;\n"\
	"varying vec2 v_uv;\n"\
	"varying vec4 v_color;\n"\
	"uniform mat4 u_mvMtx;\n"\
	"uniform mat4 u_projMtx;\n"\
	"void main(){\n"\
	"	gl_Position = u_projMtx*u_mvMtx*a_position;\n"\
	"	v_uv = a_uv;\n"\
	"	v_color = a_color;\n"\
	"}";
	const char StudioFragS[] =
	"#ifdef GL_ES\n"
	"precision mediump float;\n"
	"precision mediump int;\n"
	"#endif\n"
	"varying vec2 v_uv;\n"\
	"varying vec4 v_color;\n"\
	"uniform sampler2D u_tex;\n"\
	"void main(){\n"\
	"	gl_FragColor = texture2D(u_tex,v_uv)*v_color;\n"\
	"}";

	/*glsl_2DprogramId = R_CreateProgram( Vert2DS, Frag2DS );
	u_color = pglGetUniformLocation( glsl_2DprogramId, "u_color" );
	u_screen = pglGetUniformLocation( glsl_2DprogramId, "u_screen" );*/
	pr_2d.id = R_CreateProgram( Vert2DS, Frag2DS );
	pr_2d.u_color = pglGetUniformLocation( pr_2d.id, "u_color" );
	u_screen = pglGetUniformLocation( pr_2d.id, "u_screen" );

	/*glsl_WorldProgramId = R_CreateProgram( WorldVertS,WorldFragS );
	u_mvMtx = pglGetUniformLocation( glsl_WorldProgramId, "u_mvMtx" );
	u_projMtx = pglGetUniformLocation( glsl_WorldProgramId, "u_projMtx" );*/
	pr_world.id = R_CreateProgram( WorldVertS,WorldFragS );
	pr_world.u_mvMtx = pglGetUniformLocation( pr_world.id, "u_mvMtx" );
	pr_world.u_projMtx = pglGetUniformLocation( pr_world.id, "u_projMtx" );

	pr_beam.id = R_CreateProgram( BeamVertS, BeamFragS );
	pr_beam.u_mvMtx = pglGetUniformLocation( pr_beam.id, "u_mvMtx" );
	pr_beam.u_projMtx = pglGetUniformLocation( pr_beam.id, "u_projMtx" );

	pr_part.id = R_CreateProgram( ParticlesVertS, ParticlesFragS );
	pr_part.u_color = pglGetUniformLocation( pr_part.id, "u_color" );
	pr_part.u_projMtx = pglGetUniformLocation( pr_part.id, "u_projMtx" );
	pr_part.u_mvMtx = pglGetUniformLocation( pr_part.id, "u_mvMtx" );

	pr_studio.id = R_CreateProgram( StudioVertS, StudioFragS );
	pr_studio.u_projMtx = pglGetUniformLocation( pr_studio.id, "u_projMtx" );
	pr_studio.u_mvMtx = pglGetUniformLocation( pr_studio.id, "u_mvMtx" );

	MsgDev( D_INFO, "Init shaders\n" );
}

static vec4_t g_lastColor;
void R_UseProgram( progtype_t program )
{
	switch( program ) {
	case PROGRAM_2D:
		glsl_CurProgram = &pr_2d;
		pglUseProgram( glsl_CurProgram->id );
		pglUniform4f( glsl_CurProgram->u_color, g_lastColor[0], g_lastColor[1], g_lastColor[2], g_lastColor[3] );
		break;
	case PROGRAM_WORLD:
		glsl_CurProgram = &pr_world;
		pglUseProgram( glsl_CurProgram->id );
		break;
	case PROGRAM_PARTICLES:
		glsl_CurProgram = &pr_part;
		pglUseProgram( glsl_CurProgram->id );
		pglUniform4f( glsl_CurProgram->u_color, g_lastColor[0], g_lastColor[1], g_lastColor[2], g_lastColor[3] );
		break;
	case PROGRAM_BEAM:
		glsl_CurProgram = &pr_beam;
		pglUseProgram( glsl_CurProgram->id );
		break;
	case PROGRAM_STUDIO:
		glsl_CurProgram = &pr_studio;
		pglUseProgram( glsl_CurProgram->id );
		break;
	default:
		return;
	}
}

void R_ColorUniform( GLfloat r, GLfloat g, GLfloat b, GLfloat a )
{
	g_lastColor[0] = r;
	g_lastColor[1] = g;
	g_lastColor[2] = b;
	g_lastColor[3] = a;

	if( !glsl_CurProgram )
		return;

	if( glsl_CurProgram == &pr_2d || glsl_CurProgram == &pr_part )
		pglUniform4f( glsl_CurProgram->u_color, r, g, b, a );

}

void R_ScreenUniform( GLfloat w, GLfloat h )
{
	if( !glsl_CurProgram )
		return;

	pglUniform2f( u_screen, w, h );
}

void R_ProjMtxUniform( cmatrix4x4 source )
{
	GLfloat	dest[16];

	Matrix4x4_ToArrayFloatGL( source, dest );
	R_UseProgram( PROGRAM_WORLD );
	pglUniformMatrix4fv( glsl_CurProgram->u_projMtx, 1, GL_FALSE, dest );
	R_UseProgram( PROGRAM_BEAM );
	pglUniformMatrix4fv( glsl_CurProgram->u_projMtx, 1, GL_FALSE, dest );
	R_UseProgram( PROGRAM_PARTICLES );
	pglUniformMatrix4fv( glsl_CurProgram->u_projMtx, 1, GL_FALSE, dest );
	R_UseProgram( PROGRAM_STUDIO );
	pglUniformMatrix4fv( glsl_CurProgram->u_projMtx, 1, GL_FALSE, dest);
}

void R_ModelViewMtxUniform( cmatrix4x4 source )
{
	GLfloat	dest[16];

	Matrix4x4_ToArrayFloatGL( source, dest );
	R_UseProgram( PROGRAM_WORLD );
	pglUniformMatrix4fv( glsl_CurProgram->u_mvMtx, 1, GL_FALSE, dest );
	R_UseProgram( PROGRAM_BEAM );
	pglUniformMatrix4fv( glsl_CurProgram->u_mvMtx, 1, GL_FALSE, dest );
	R_UseProgram( PROGRAM_PARTICLES );
	pglUniformMatrix4fv( glsl_CurProgram->u_mvMtx, 1, GL_FALSE, dest );
	R_UseProgram( PROGRAM_STUDIO );
	pglUniformMatrix4fv( glsl_CurProgram->u_mvMtx, 1, GL_FALSE, dest );
}
#endif
