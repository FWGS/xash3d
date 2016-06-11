char Vert2DS[]=
"precision mediump float;\n"\
"attribute vec4 a_position;\n"\
"attribute vec2 a_uv;\n"\
"varying vec2 v_uv;\n"\
"uniform vec2 u_screen;\n"\
"void main(){\n"\
"	gl_Position = vec4((a_position.xy*u_screen) - 1.0, 0.0, 1.0);\n"\
"	gl_Position.y *= -1.0;\n"\
"	v_uv = a_uv;\n"\
"}";
char Frag2DS[]=
"precision mediump float;\n"\
"varying vec2 v_uv;\n"\
"uniform sampler2D u_tex;\n"\
"uniform vec4 u_color;\n"\
"void main(){\n"\
"	gl_FragColor = texture2D(u_tex,v_uv)*u_color;\n"\
"}";

char WorldNormalVertS[] =
"precision mediump float;\n"\
"attribute vec4 a_position;\n"\
"attribute vec2 a_uv;\n"\
"varying vec2 v_uv;\n"\
"uniform mat4 u_mvMtx;\n"\
"uniform mat4 u_projMtx;\n"\
"void main(){\n"\
"	gl_Position = u_projMtx*u_mvMtx*a_position;\n"\
"	v_uv = a_uv;\n"\
"}";
char WorldNormalFragS[] =
"precision mediump float;\n"\
"varying vec2 v_uv;\n"\
"uniform sampler2D u_tex;\n"\
"void main(){\n"\
"	vec4 col = texture2D(u_tex,v_uv);\n"\
"	gl_FragColor = col;\n"\
"}";

char WorldAlphatestFragS[] =
"precision mediump float;\n"\
"varying vec2 v_uv;\n"\
"uniform sampler2D u_tex;\n"\
"void main(){\n"\
"	vec4 col = texture2D(u_tex,v_uv);\n"\
"	if(col.a <= 0.5) discard;\n"\
"	gl_FragColor = col;\n"\
"}";

char WorldTransColorFragS[] =
"precision mediump float;\n"\
"uniform vec4 u_color;\n"\
"void main(){\n"\
"	gl_FragColor = u_color;\n"\
"}";

char WorldTransTextureFragS[] =
"precision mediump float;\n"\
"varying vec2 v_uv;\n"\
"uniform sampler2D u_tex;\n"\
"uniform vec4 u_color;\n"\
"void main(){\n"\
"	vec4 col = texture2D(u_tex,v_uv);\n"\
"	gl_FragColor = col * u_color;\n"\
"}";

char ParticlesVertS[] =
"precision mediump float;\n"\
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
"precision mediump float;\n"\
"varying vec2 v_uv;\n"\
"uniform sampler2D u_tex;\n"\
"uniform vec4 u_color;\n"\
"void main(){\n"\
"	vec4 tex = texture2D(u_tex,v_uv);\n"\
"	if(tex.a == 0.0)\n"\
"		discard;\n"\
"	gl_FragColor = tex*u_color;\n"\
"}";

char BeamVertS[] =
"precision mediump float;\n"\
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
"precision mediump float;\n"\
"varying vec2 v_uv;\n"\
"varying vec4 v_color;\n"\
"uniform sampler2D u_tex;\n"\
"void main(){\n"\
"	gl_FragColor = texture2D(u_tex,v_uv)*v_color;\n"\
"}";

char StudioVertS[] =
"precision mediump float;\n"\
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
"precision mediump float;\n"\
"varying vec2 v_uv;\n"\
"varying vec4 v_color;\n"\
"uniform sampler2D u_tex;\n"\
"void main(){\n"\
"	gl_FragColor = texture2D(u_tex,v_uv)*v_color;\n"\
"}";
