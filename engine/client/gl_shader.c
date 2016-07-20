
#include "common.h"
#include "gl_local.h"

typedef struct program_s
{
	GLuint id;
	GLint u_mvMtx;
	GLint u_projMtx;
	GLint u_color;
} program_t;

static program_t *glsl_CurProgram=NULL;
static program_t glslpr_2D={0,-1,-1,-1};

static program_t glslpr_WorldNormal={0,-1,-1,-1};
static program_t glslpr_WorldAlphatest={0,-1,-1,-1};
static program_t glslpr_WorldTransColor={0,-1,-1,-1};
static program_t glslpr_WorldTransTexture={0,-1,-1,-1};

static program_t glslpr_WorldFogNormal={0,-1,-1,-1};

static program_t glslpr_Particles={0,-1,-1,-1};
static program_t glslpr_Beam={0,-1,-1,-1};
static program_t glslpr_Studio={0,-1,-1,-1};
GLint u_screen = -1;
GLint u_fogParams = -1;

int lastRendermode=kRenderNormal;
static vec4_t g_lastColor = {0.0f,0.0f,0.0f,0.0f};
static qboolean needFog = false;
static vec4_t g_lastFogParams = {0.5f,0.5f,0.5f,0.002f};

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
#include "gl_shader_strings.h"

	glslpr_2D.id = R_CreateProgram(Vert2DS,Frag2DS);
	glslpr_2D.u_color = pglGetUniformLocation(glslpr_2D.id,"u_color");
	u_screen = pglGetUniformLocation(glslpr_2D.id,"u_screen");

	glslpr_WorldNormal.id = R_CreateProgram(WorldNormalVertS,WorldNormalFragS);
	glslpr_WorldNormal.u_mvMtx = pglGetUniformLocation(glslpr_WorldNormal.id,"u_mvMtx");
	glslpr_WorldNormal.u_projMtx = pglGetUniformLocation(glslpr_WorldNormal.id,"u_projMtx");

	glslpr_WorldAlphatest.id = R_CreateProgram(WorldNormalVertS,WorldAlphatestFragS);
	glslpr_WorldAlphatest.u_mvMtx = pglGetUniformLocation(glslpr_WorldAlphatest.id,"u_mvMtx");
	glslpr_WorldAlphatest.u_projMtx = pglGetUniformLocation(glslpr_WorldAlphatest.id,"u_projMtx");

	glslpr_WorldTransColor.id = R_CreateProgram(WorldNormalVertS,WorldTransColorFragS);
	glslpr_WorldTransColor.u_mvMtx = pglGetUniformLocation(glslpr_WorldTransColor.id,"u_mvMtx");
	glslpr_WorldTransColor.u_projMtx = pglGetUniformLocation(glslpr_WorldTransColor.id,"u_projMtx");
	glslpr_WorldTransColor.u_color = pglGetUniformLocation(glslpr_WorldTransColor.id,"u_color");

	glslpr_WorldTransTexture.id = R_CreateProgram(WorldNormalVertS,WorldTransTextureFragS);
	glslpr_WorldTransTexture.u_mvMtx = pglGetUniformLocation(glslpr_WorldTransTexture.id,"u_mvMtx");
	glslpr_WorldTransTexture.u_projMtx = pglGetUniformLocation(glslpr_WorldTransTexture.id,"u_projMtx");
	glslpr_WorldTransTexture.u_color = pglGetUniformLocation(glslpr_WorldTransTexture.id,"u_color");

	glslpr_WorldFogNormal.id = R_CreateProgram(WorldFogNormalVertS,WorldFogNormalFragS);
	glslpr_WorldFogNormal.u_mvMtx = pglGetUniformLocation(glslpr_WorldFogNormal.id,"u_mvMtx");
	glslpr_WorldFogNormal.u_projMtx = pglGetUniformLocation(glslpr_WorldFogNormal.id,"u_projMtx");
	u_fogParams = pglGetUniformLocation(glslpr_WorldFogNormal.id,"u_fogParams");

	glslpr_Beam.id = R_CreateProgram(BeamVertS,BeamFragS);
	glslpr_Beam.u_mvMtx = pglGetUniformLocation(glslpr_Beam.id,"u_mvMtx");
	glslpr_Beam.u_projMtx = pglGetUniformLocation(glslpr_Beam.id,"u_projMtx");

	glslpr_Particles.id = R_CreateProgram(ParticlesVertS,ParticlesFragS);
	glslpr_Particles.u_mvMtx = pglGetUniformLocation(glslpr_Particles.id,"u_mvMtx");
	glslpr_Particles.u_projMtx = pglGetUniformLocation(glslpr_Particles.id,"u_projMtx");
	glslpr_Particles.u_color = pglGetUniformLocation(glslpr_Particles.id,"u_color");

	glslpr_Studio.id = R_CreateProgram(StudioVertS, StudioFragS);
	glslpr_Studio.u_mvMtx = pglGetUniformLocation(glslpr_Studio.id,"u_mvMtx");
	glslpr_Studio.u_projMtx = pglGetUniformLocation(glslpr_Studio.id,"u_projMtx");

	MsgDev( D_INFO, "Init shaders\n" );
}

void R_ShaderSetRendermode(int m)
{
	lastRendermode = m;
}

void R_UseProgram(progtype_t type)
{
	switch (type)
	{
	case PROGRAM_2D:
		glsl_CurProgram = &glslpr_2D;
		pglUseProgram(glsl_CurProgram->id);
		pglUniform4f(glsl_CurProgram->u_color,g_lastColor[0],g_lastColor[1],g_lastColor[2],g_lastColor[3]);
		break;
	case PROGRAM_WORLD:
		if(needFog)
		{
			glsl_CurProgram = &glslpr_WorldFogNormal;
			pglUseProgram(glsl_CurProgram->id);
			pglUniform4f(u_fogParams,g_lastFogParams[0],g_lastFogParams[1],g_lastFogParams[2],g_lastFogParams[3]);
		}
		else
		if(lastRendermode == kRenderTransAlpha )
		{
			glsl_CurProgram = &glslpr_WorldAlphatest;
			pglUseProgram(glsl_CurProgram->id);
		}
		else if(lastRendermode == kRenderTransColor)
		{
			glsl_CurProgram = &glslpr_WorldTransColor;
			pglUseProgram(glsl_CurProgram->id);
			pglUniform4f(glsl_CurProgram->u_color,g_lastColor[0],g_lastColor[1],g_lastColor[2],g_lastColor[3]);
		}
		else if(lastRendermode == kRenderTransTexture)
		{
			glsl_CurProgram = &glslpr_WorldTransTexture;
			pglUseProgram(glsl_CurProgram->id);
			pglUniform4f(glsl_CurProgram->u_color,g_lastColor[0],g_lastColor[1],g_lastColor[2],g_lastColor[3]);
		}
		else
		{
			glsl_CurProgram = &glslpr_WorldNormal;
			pglUseProgram(glsl_CurProgram->id);
		}
		break;
	case PROGRAM_PARTICLES:
		glsl_CurProgram = &glslpr_Particles;
		pglUseProgram(glsl_CurProgram->id);
		pglUniform4f(glsl_CurProgram->u_color,g_lastColor[0],g_lastColor[1],g_lastColor[2],g_lastColor[3]);
		break;
	case PROGRAM_BEAM:
		glsl_CurProgram = &glslpr_Beam;
		pglUseProgram(glsl_CurProgram->id);
		break;
	case PROGRAM_STUDIO:
		glsl_CurProgram = &glslpr_Studio;
		pglUseProgram(glsl_CurProgram->id);
		break;
	}
}

void R_ColorUniform(GLfloat r,GLfloat g, GLfloat b, GLfloat a)
{
	g_lastColor[0] = r;
	g_lastColor[1] = g;
	g_lastColor[2] = b;
	g_lastColor[3] = a;

	if(!glsl_CurProgram)
		return;

	if(glsl_CurProgram==&glslpr_Particles || glsl_CurProgram==&glslpr_WorldTransTexture || glsl_CurProgram==&glslpr_WorldTransColor || glsl_CurProgram==&glslpr_2D)
		pglUniform4f(glsl_CurProgram->u_color,r,g,b,a);

}

void R_ScreenUniform(GLfloat w, GLfloat h)
{
	if(!glsl_CurProgram)
		return;

	pglUniform2f(u_screen,w,h);
}

void R_SetFogEnable(qboolean state)
{
	needFog=state;
}

void R_SetFogColor(vec3_t fogColor)
{
	g_lastFogParams[0] = fogColor[0];
	g_lastFogParams[1] = fogColor[1];
	g_lastFogParams[2] = fogColor[2];

	if(glsl_CurProgram == &glslpr_WorldFogNormal )
		pglUniform4f(u_fogParams,g_lastFogParams[0],g_lastFogParams[1],g_lastFogParams[2],g_lastFogParams[3]);
}

void R_SetFogDensity(float fogDensity)
{
	g_lastFogParams[3] = fogDensity;

	if(glsl_CurProgram == &glslpr_WorldFogNormal )
		pglUniform4f(u_fogParams,g_lastFogParams[0],g_lastFogParams[1],g_lastFogParams[2],g_lastFogParams[3]);
}

void R_ProjMtxUniform(const matrix4x4 source)
{
	GLfloat	dest[16];

	Matrix4x4_ToArrayFloatGL( source, dest );
	pglUseProgram(glslpr_WorldNormal.id);
	pglUniformMatrix4fv(glslpr_WorldNormal.u_projMtx,1,GL_FALSE,dest);
	pglUseProgram(glslpr_WorldAlphatest.id);
	pglUniformMatrix4fv(glslpr_WorldAlphatest.u_projMtx,1,GL_FALSE,dest);
	pglUseProgram(glslpr_WorldTransColor.id);
	pglUniformMatrix4fv(glslpr_WorldTransColor.u_projMtx,1,GL_FALSE,dest);
	pglUseProgram(glslpr_WorldTransTexture.id);
	pglUniformMatrix4fv(glslpr_WorldTransTexture.u_projMtx,1,GL_FALSE,dest);
	pglUseProgram(glslpr_WorldFogNormal.id);
	pglUniformMatrix4fv(glslpr_WorldFogNormal.u_projMtx,1,GL_FALSE,dest);
	pglUseProgram(glslpr_Beam.id);
	pglUniformMatrix4fv(glslpr_Beam.u_projMtx,1,GL_FALSE,dest);
	pglUseProgram(glslpr_Particles.id);
	pglUniformMatrix4fv(glslpr_Particles.u_projMtx,1,GL_FALSE,dest);
	pglUseProgram(glslpr_Studio.id);
	pglUniformMatrix4fv(glslpr_Studio.u_projMtx,1,GL_FALSE,dest);
}

void R_ModelViewMtxUniform(const matrix4x4 source)
{
	GLfloat	dest[16];

	Matrix4x4_ToArrayFloatGL( source, dest );
	pglUseProgram(glslpr_WorldNormal.id);
	pglUniformMatrix4fv(glslpr_WorldNormal.u_mvMtx,1,GL_FALSE,dest);
	pglUseProgram(glslpr_WorldAlphatest.id);
	pglUniformMatrix4fv(glslpr_WorldAlphatest.u_mvMtx,1,GL_FALSE,dest);
	pglUseProgram(glslpr_WorldTransColor.id);
	pglUniformMatrix4fv(glslpr_WorldTransColor.u_mvMtx,1,GL_FALSE,dest);
	pglUseProgram(glslpr_WorldTransTexture.id);
	pglUniformMatrix4fv(glslpr_WorldTransTexture.u_mvMtx,1,GL_FALSE,dest);
	pglUseProgram(glslpr_WorldFogNormal.id);
	pglUniformMatrix4fv(glslpr_WorldFogNormal.u_mvMtx,1,GL_FALSE,dest);
	pglUseProgram(glslpr_Beam.id);
	pglUniformMatrix4fv(glslpr_Beam.u_mvMtx,1,GL_FALSE,dest);
	pglUseProgram(glslpr_Particles.id);
	pglUniformMatrix4fv(glslpr_Particles.u_mvMtx,1,GL_FALSE,dest);
	pglUseProgram(glslpr_Studio.id);
	pglUniformMatrix4fv(glslpr_Studio.u_mvMtx,1,GL_FALSE,dest);
}
