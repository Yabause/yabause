/*  Copyright 2006 Guillaume Duhamel
    Copyright 2006 Fabien Coulon
    Copyright 2005 Joost Peters

    This file is part of Yabause.

    Yabause is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    Yabause is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Yabause; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
*/

#include <assert.h>
#include <unistd.h>
#include <sys/time.h>

#include "platform.h"

#include "../yabause.h"
#include "../gameinfo.h"
#include "../yui.h"
#include "../peripheral.h"
#include "../sh2core.h"
#include "../sh2int.h"
#ifdef HAVE_LIBGL
#include "../vidogl.h"
#include "../ygl.h"
#endif
#include "../vidsoft.h"
#include "../cs0.h"
#include "../cs2.h"
#include "../cdbase.h"
#include "../scsp.h"
#include "../sndsdl.h"
#include "../sndal.h"
#include "../persdljoy.h"
#ifdef ARCH_IS_LINUX
#include "../perlinuxjoy.h"
#endif
#include "../debug.h"
#include "../m68kcore.h"
#include "../m68kc68k.h"
#include "../vdp1.h"
#include "../vdp2.h"
#include "../cdbase.h"
#include "../peripheral.h"
#ifdef DYNAREC_KRONOS
#include "../sh2_kronos/sh2int_kronos.h"
#endif

M68K_struct * M68KCoreList[] = {
&M68KDummy,
NULL
};

SH2Interface_struct *SH2CoreList[] = {
&SH2Interpreter,
NULL
};

PerInterface_struct *PERCoreList[] = {
&PERDummy,
NULL
};

CDInterface *CDCoreList[] = {
&DummyCD,
NULL
};

SoundInterface_struct *SNDCoreList[] = {
&SNDDummy,
NULL
};

VideoInterface_struct *VIDCoreList[] = {
&VIDDummy,
NULL
};

#ifdef YAB_PORT_OSD
#include "nanovg/nanovg_osdcore.h"
OSD_struct *OSDCoreList[] = {
&OSDNnovg,
NULL
};
#endif


#define countof(x) (sizeof(x) / sizeof(0[x]))

#define M_PI 3.141592653589793
#define ATTRIB_POINT 0

static GLuint
compile_shader(GLenum type, const GLchar *source)
{
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);
    GLint param;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &param);
    if (!param) {
        GLchar log[4096];
        glGetShaderInfoLog(shader, sizeof(log), NULL, log);
        fprintf(stdout, "error: %s: %s\n",
                type == GL_FRAGMENT_SHADER ? "frag" : "vert", (char *) log);
        exit(EXIT_FAILURE);
    }
    return shader;
}

static GLuint
link_program(GLuint vert, GLuint frag)
{
    GLuint program = glCreateProgram();
    glAttachShader(program, vert);
    glAttachShader(program, frag);
    glLinkProgram(program);
    GLint param;
    glGetProgramiv(program, GL_LINK_STATUS, &param);
    if (!param) {
        GLchar log[4096];
        glGetProgramInfoLog(program, sizeof(log), NULL, log);
        fprintf(stdout, "error: link: %s\n", (char *) log);
        exit(EXIT_FAILURE);
    }
    return program;
}

struct graphics_context {
    GLuint program;
    GLuint vbo_point;
    GLuint vao_point;
    double angle;
    long framecount;
    double lastframe;
};

const float SQUARE[] = {
    -1.0f,  1.0f,
    -1.0f, -1.0f,
     1.0f,  1.0f,
     1.0f, -1.0f
};

static void
render(struct graphics_context *context)
{
    glClearColor(0.15, 0.15, 0.15, 1);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(context->program);
    glBindVertexArray(context->vao_point);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, countof(SQUARE) / 2);
    glBindVertexArray(0);
    glUseProgram(0);

    /* Physics */
    double now = glfwGetTime();
    double udiff = now - context->lastframe;
    context->angle += 1.0 * udiff;
    if (context->angle > 2 * M_PI)
        context->angle -= 2 * M_PI;
    context->framecount++;
    if ((long)now != (long)context->lastframe) {
        printf("FPS: %ld\n", context->framecount);
        context->framecount = 0;
    }
    context->lastframe = now;

    platform_swapBuffers();
}

static int SetupOpenGL() {
  if (!platform_SetupOpenGL(800,600,0))
    exit(EXIT_FAILURE);
#if defined(_USEGLEW_)
  glewExperimental=GL_TRUE;
  if (glewInit() != 0) {
    printf("Glew can not init\n");
    exit(EXIT_FAILURE);
  }
#endif
}

void YuiMsg(const char *format, ...) {
}

void YuiErrorMsg(const char *error_text) {
}

int YuiRevokeOGLOnThisThread(){
  return 0;
}

int YuiUseOGLOnThisThread(){
  return 0;
}

void YuiSwapBuffers(void) {
}

int YuiGetFB(void) {
  return 0;
}

int main(int argc, char *argv[]) {
  int i;
  struct graphics_context context;

  LogStart();
  LogChangeOutput( DEBUG_STDERR, NULL );

  SetupOpenGL();

  const GLchar *vert_shader =
        "#version 330\n"
        "layout(location = 0) in vec2 point;\n"
        "void main() {\n"
        "    mat2 rotate = mat2(0.5, -0.5,\n"
        "                       0.5, 0.5);\n"
        "    gl_Position = vec4(0.75 * rotate * point, 0.0, 1.0);\n"
        "}\n";
    const GLchar *frag_shader =
        "#version 330\n"
        "out vec4 color;\n"
        "void main() {\n"
        "    color = vec4(1, 0.15, 0.15, 0);\n"
        "}\n";

    /* Compile and link OpenGL program */
    GLuint vert = compile_shader(GL_VERTEX_SHADER, vert_shader);
    GLuint frag = compile_shader(GL_FRAGMENT_SHADER, frag_shader);
    context.program = link_program(vert, frag);
    glDeleteShader(frag);
    glDeleteShader(vert);

    /* Prepare vertex buffer object (VBO) */
    glGenBuffers(1, &context.vbo_point);
    glBindBuffer(GL_ARRAY_BUFFER, context.vbo_point);
    glBufferData(GL_ARRAY_BUFFER, sizeof(SQUARE), SQUARE, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    /* Prepare vertrex array object (VAO) */
    glGenVertexArrays(1, &context.vao_point);
    glBindVertexArray(context.vao_point);
    glBindBuffer(GL_ARRAY_BUFFER, context.vbo_point);
    glVertexAttribPointer(ATTRIB_POINT, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(ATTRIB_POINT);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    /* Start main loop */
    context.lastframe = glfwGetTime();
    context.framecount = 0;

    render(&context);
    sleep(10);

    fprintf(stdout, "Exiting ...\n");

    /* Cleanup and exit */
    glDeleteVertexArrays(1, &context.vao_point);
    glDeleteBuffers(1, &context.vbo_point);
    glDeleteProgram(context.program);



  platform_Deinit();

  return 0;
}
