#ifdef XASH_GLES2_RENDER
#include "common.h"
#include "gl_local.h"

GLuint glsl_CurProgramId = 0;
GLuint glsl_2DprogramId = 0;
GLuint glsl_WorldProgramId = 0;
GLuint glsl_ParticlesProgramId = 0;
GLuint glsl_BeamProgramId = 0;
GLuint glsl_StudioProgramId = 0;
GLint u_color = -1;
GLint u_screen = -1;
GLint u_colorPart = -1;
GLint u_mvMtx = -1;
GLint u_projMtx = -1;
GLint u_mvMtxPart = -1;
GLint u_projMtxPart = -1;

GLuint R_CreateShader( const char *src, GLint type )
{
	GLuint id = pglCreateShader( type );
	pglShaderSource( id, 1, &src, NULL );
	pglCompileShader( id );

	GLint compile_ok = GL_FALSE;
	pglGetShaderiv( id, GL_COMPILE_STATUS, &compile_ok );
	if( compile_ok == GL_FALSE )
	{
		MsgDev( D_ERROR, "Error glCompileShader: %x\n", id );
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
	"varying vec2 v_uv;\n"\
	"uniform sampler2D u_tex;\n"\
	"uniform vec4 u_color;\n"\
	"void main(){\n"\
	"	gl_FragColor = texture2D(u_tex,v_uv)*u_color;\n"\
	"}";

	const char WorldVertS[] =
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
	"varying vec2 v_uv;\n"\
	"uniform sampler2D u_tex;\n"\
	"void main(){\n"\
	"	gl_FragColor = texture2D(u_tex,v_uv);\n"\
	"}";

	const char ParticlesVertS[] =
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
	"varying vec2 v_uv;\n"\
	"varying vec4 v_color;\n"\
	"uniform sampler2D u_tex;\n"\
	"void main(){\n"\
	"	gl_FragColor = texture2D(u_tex,v_uv)*v_color;\n"\
	"}";

	const char StudioVertS[] =
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
	"varying vec2 v_uv;\n"\
	"varying vec4 v_color;\n"\
	"uniform sampler2D u_tex;\n"\
	"void main(){\n"\
	"	gl_FragColor = texture2D(u_tex,v_uv)*v_color;\n"\
	"}";

	glsl_2DprogramId = R_CreateProgram( Vert2DS, Frag2DS );
	u_color = pglGetUniformLocation( glsl_2DprogramId, "u_color" );
	u_screen = pglGetUniformLocation( glsl_2DprogramId, "u_screen" );

	glsl_WorldProgramId = R_CreateProgram( WorldVertS,WorldFragS );
	u_mvMtx = pglGetUniformLocation( glsl_WorldProgramId, "u_mvMtx" );
	u_projMtx = pglGetUniformLocation( glsl_WorldProgramId, "u_projMtx" );

	glsl_BeamProgramId = R_CreateProgram( BeamVertS, BeamFragS );

	glsl_ParticlesProgramId = R_CreateProgram( ParticlesVertS, ParticlesFragS );
	u_mvMtxPart = pglGetUniformLocation( glsl_ParticlesProgramId, "u_mvMtx" );
	u_projMtxPart = pglGetUniformLocation( glsl_ParticlesProgramId, "u_projMtx" );
	u_colorPart = pglGetUniformLocation( glsl_ParticlesProgramId, "u_color" );
	if( u_color != u_colorPart )
		MsgDev( D_ERROR, "u_color!=u_colorPart %i %i\n", u_color, u_colorPart );

	glsl_StudioProgramId = R_CreateProgram( StudioVertS, StudioFragS );

	MsgDev( D_INFO, "Init shaders\n" );
}
void R_UseProgram( progtype_t program )
{
	switch( program ) {
	case PROGRAM_2D:
		glsl_CurProgramId = glsl_2DprogramId;
		break;
	case PROGRAM_WORLD:
		glsl_CurProgramId = glsl_WorldProgramId;
		break;
	case PROGRAM_PARTICLES:
		glsl_CurProgramId = glsl_ParticlesProgramId;
		break;
	case PROGRAM_BEAM:
		glsl_CurProgramId = glsl_BeamProgramId;
		break;
	case PROGRAM_STUDIO:
		glsl_CurProgramId = glsl_StudioProgramId;
		break;
	default:
		return;
	}
	pglUseProgram( glsl_CurProgramId );
}

void R_ColorUniform (GLfloat r, GLfloat g, GLfloat b, GLfloat a )
{
	if( !glsl_CurProgramId )
		return;

	if( glsl_CurProgramId == glsl_ParticlesProgramId )
		pglUniform4f( u_colorPart, r, g, b, a );
	else
		pglUniform4f( u_color, r, g, b, a );
}

void R_ScreenUniform( GLfloat w, GLfloat h )
{
	if( !glsl_CurProgramId )
		return;

	pglUniform2f( u_screen, w, h );
}

void R_ProjMtxUniform( cmatrix4x4 source )
{
	GLfloat	dest[16];

	Matrix4x4_ToArrayFloatGL( source, dest );
	R_UseProgram( PROGRAM_WORLD );
	pglUniformMatrix4fv( u_projMtx, 1, GL_FALSE, dest );
	R_UseProgram( PROGRAM_BEAM );
	pglUniformMatrix4fv( u_projMtx, 1, GL_FALSE, dest );
	R_UseProgram( PROGRAM_PARTICLES );
	pglUniformMatrix4fv( u_projMtxPart, 1, GL_FALSE, dest );
	R_UseProgram( PROGRAM_STUDIO );
	pglUniformMatrix4fv( u_projMtx, 1, GL_FALSE, dest);
}

void R_ModelViewMtxUniform( cmatrix4x4 source )
{
	GLfloat	dest[16];

	Matrix4x4_ToArrayFloatGL( source, dest );
	R_UseProgram( PROGRAM_WORLD );
	pglUniformMatrix4fv( u_mvMtx, 1, GL_FALSE, dest );
	R_UseProgram( PROGRAM_BEAM );
	pglUniformMatrix4fv( u_mvMtx, 1, GL_FALSE, dest );
	R_UseProgram( PROGRAM_PARTICLES );
	pglUniformMatrix4fv( u_mvMtxPart, 1, GL_FALSE, dest );
	R_UseProgram( PROGRAM_STUDIO );
	pglUniformMatrix4fv( u_mvMtx, 1, GL_FALSE, dest );
}
#endif
