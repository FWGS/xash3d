
#include "common.h"
#include "gl_local.h"

GLuint glsl_CurProgramId = 0;
GLuint glsl_2DprogramId = 0;
GLuint glsl_WorldProgramId = 0;
GLuint glsl_ParticlesProgramId = 0;
GLuint glsl_BeamProgramId = 0;
GLuint glsl_StudioProgramId = 0;
GLint u_color = -1;
GLint u_colorPart = -1;
GLint u_mvMtx = -1;
GLint u_projMtx = -1;
GLint u_mvMtxPart = -1;
GLint u_projMtxPart = -1;

GLuint R_CreateShader(const char *src, GLint type)
{
	GLuint id = pglCreateShader(type);
	pglShaderSource(id,1,&src,NULL);
	pglCompileShader(id);

	GLint compile_ok=GL_FALSE;
	pglGetShaderiv(id,GL_COMPILE_STATUS, &compile_ok);
	if(compile_ok==GL_FALSE)
	{
		MsgDev( D_ERROR, "Error glCompileShader: %x\n",id);
		//print_log(id);
		pglDeleteShader(id);
		return -1;
	}

	return id;
}

GLuint R_CreateProgram(char *vert, char *frag)
{
	GLuint vs = R_CreateShader(vert,GL_VERTEX_SHADER);
	GLuint fs = R_CreateShader(frag,GL_FRAGMENT_SHADER);
	GLuint id = pglCreateProgram();

	pglAttachShader(id,vs);
	pglAttachShader(id,fs);
	pglLinkProgram(id);
	GLint link_ok=GL_FALSE;
	pglGetProgramiv(id,GL_LINK_STATUS,&link_ok);
	if(!link_ok)
	{
		MsgDev( D_ERROR, "Error glLinkProgram: %x\n",id);
		//print_log(id);
		return -1;
	}

	return id;
}

void R_InitShaders()
{
	char Vert2DS[]=
	"attribute vec4 a_position;\n"\
	"attribute vec2 a_uv;\n"\
	"varying vec2 v_uv;\n"\
	"const vec2 screen= vec2(0.0025,0.003333333);\n"\
	"void main(){\n"\
	"	gl_Position = vec4((a_position.xy*screen)-1.0,0.0,1.0);\n"\
	"	gl_Position.y*=-1.0;\n"\
	"	v_uv=a_uv;\n"\
	"}";
	char Frag2DS[]=
	"varying vec2 v_uv;\n"\
	"uniform sampler2D u_tex;\n"\
	"uniform vec4 u_color;\n"\
	"void main(){\n"\
	"	gl_FragColor = texture2D(u_tex,v_uv)*u_color;\n"\
	"}";

	char WorldVertS[] =
	"attribute vec4 a_position;\n"\
	"attribute vec2 a_uv;\n"\
	"varying vec2 v_uv;\n"\
	"uniform mat4 u_mvMtx;\n"\
	"uniform mat4 u_projMtx;\n"\
	"void main(){\n"\
	"	gl_Position = u_projMtx*u_mvMtx*a_position;\n"\
	"	v_uv = a_uv;\n"\
	"}";
	char WorldFragS[] =
	"varying vec2 v_uv;\n"\
	"uniform sampler2D u_tex;\n"\
	"void main(){\n"\
	"	gl_FragColor = texture2D(u_tex,v_uv);\n"\
	"}";

	char ParticlesVertS[] =
	"attribute vec4 a_position;\n"\
	"attribute vec2 a_uv;\n"\
	"varying vec2 v_uv;\n"\
	"uniform mat4 u_mvMtx;\n"\
	"uniform mat4 u_projMtx;\n"\
	"void main(){\n"\
	"	gl_Position = u_projMtx*u_mvMtx*a_position;\n"\
	"	v_uv = a_uv;\n"\
	"}";
	char ParticlesFragS[] =
	"varying vec2 v_uv;\n"\
	"uniform sampler2D u_tex;\n"\
	"uniform vec4 u_color;\n"\
	"void main(){\n"\
	"	vec4 tex = texture2D(u_tex,v_uv);\n"\
	"	if(tex.a==0.0)\n"\
	"		discard;\n"\
	"	gl_FragColor = tex*u_color;\n"\
	"}";

	char BeamVertS[] =
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
	char BeamFragS[]=
	"varying vec2 v_uv;\n"\
	"varying vec4 v_color;\n"\
	"uniform sampler2D u_tex;\n"\
	"void main(){\n"\
	"	gl_FragColor = vec4(0.0,1.0,0.0,1.0)+(texture2D(u_tex,v_uv)*v_color*0.01);\n"\
	"}";

	char StudioVertS[] =
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
	char StudioFragS[] =
	"varying vec2 v_uv;\n"\
	"varying vec4 v_color;\n"\
	"uniform sampler2D u_tex;\n"\
	"void main(){\n"\
	"	gl_FragColor = texture2D(u_tex,v_uv)*v_color;\n"\
	"}";

	glsl_2DprogramId = R_CreateProgram(Vert2DS,Frag2DS);
	u_color = pglGetUniformLocation(glsl_2DprogramId,"u_color");

	glsl_WorldProgramId = R_CreateProgram(WorldVertS,WorldFragS);
	u_mvMtx = pglGetUniformLocation(glsl_WorldProgramId,"u_mvMtx");
	u_projMtx = pglGetUniformLocation(glsl_WorldProgramId,"u_projMtx");

	glsl_BeamProgramId = R_CreateProgram(BeamVertS,BeamFragS);

	glsl_ParticlesProgramId = R_CreateProgram(ParticlesVertS,ParticlesFragS);
	u_mvMtxPart = pglGetUniformLocation(glsl_ParticlesProgramId,"u_mvMtx");
	u_projMtxPart = pglGetUniformLocation(glsl_ParticlesProgramId,"u_projMtx");
	u_colorPart = pglGetUniformLocation(glsl_ParticlesProgramId,"u_color");
	if(u_color != u_colorPart)
		MsgDev( D_ERROR, "u_color!=u_colorPart %i %i\n",u_color,u_colorPart);

	glsl_StudioProgramId = R_CreateProgram(StudioVertS, StudioFragS);

	MsgDev( D_INFO, "Init shaders\n" );
}

void R_Use2DProgram()
{
	glsl_CurProgramId = glsl_2DprogramId;
	pglUseProgram(glsl_2DprogramId);
}

void R_UseWorldProgram()
{
	glsl_CurProgramId = glsl_WorldProgramId;
	pglUseProgram(glsl_WorldProgramId);
}

void R_UseParticlesProgram()
{
	glsl_CurProgramId = glsl_ParticlesProgramId;
	pglUseProgram(glsl_ParticlesProgramId);
}

void R_UseBeamProgram()
{
	glsl_CurProgramId = glsl_BeamProgramId;
	pglUseProgram(glsl_BeamProgramId);
}

void R_UseStudioProgram()
{
	glsl_CurProgramId = glsl_StudioProgramId;
	pglUseProgram(glsl_StudioProgramId);
}

void R_ColorUniform(GLfloat r,GLfloat g, GLfloat b, GLfloat a)
{
	if(!glsl_CurProgramId)
		return;

	if(glsl_CurProgramId==glsl_ParticlesProgramId)
		pglUniform4f(u_colorPart,r,g,b,a);
	else
		pglUniform4f(u_color,r,g,b,a);
}

void R_ProjMtxUniform(const matrix4x4 source)
{
	GLfloat	dest[16];

	Matrix4x4_ToArrayFloatGL( source, dest );
	R_UseWorldProgram();
	pglUniformMatrix4fv(u_projMtx,1,GL_FALSE,dest);
	R_UseBeamProgram();
	pglUniformMatrix4fv(u_projMtx,1,GL_FALSE,dest);
	R_UseParticlesProgram();
	pglUniformMatrix4fv(u_projMtxPart,1,GL_FALSE,dest);
	R_UseStudioProgram();
	pglUniformMatrix4fv(u_projMtx,1,GL_FALSE,dest);
}

void R_ModelViewMtxUniform(const matrix4x4 source)
{
	GLfloat	dest[16];

	Matrix4x4_ToArrayFloatGL( source, dest );
	R_UseWorldProgram();
	pglUniformMatrix4fv(u_mvMtx,1,GL_FALSE,dest);
	R_UseBeamProgram();
	pglUniformMatrix4fv(u_mvMtx,1,GL_FALSE,dest);
	R_UseParticlesProgram();
	pglUniformMatrix4fv(u_mvMtxPart,1,GL_FALSE,dest);
	R_UseStudioProgram();
	pglUniformMatrix4fv(u_mvMtx,1,GL_FALSE,dest);
}
