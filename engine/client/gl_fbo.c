
#include "common.h"
#include "gl_local.h"

GLuint InitFrameBufferObject(GLuint w,GLuint h,GLuint tex)
{
	GLuint id;
	pglGenFramebuffers(1,&id);

	pglBindTexture(GL_TEXTURE_2D, tex);
	pglTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,w,h,0,GL_RGBA,GL_UNSIGNED_BYTE,0);
	pglBindFramebuffer(GL_FRAMEBUFFER,id);
	pglFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,tex,0);

	return id;
}
