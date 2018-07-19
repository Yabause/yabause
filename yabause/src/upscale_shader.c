#include "ygl.h"
#include "hq4x_shader.h"
#include "hq4x_lut.h"
#include "4xbrz_shader.h"
#include "2xbrz_shader.h"

static const GLchar * fblithq4x_v[] = { Yglprg_blit_hq4x_v, NULL };
static const GLchar * fblithq4x_f[] = { Yglprg_blit_hq4x_f, NULL };
static const GLchar * fblit4xbrz_v[] = { Yglprg_blit_4xbrz_v, NULL };
static const GLchar * fblit4xbrz_f[] = { Yglprg_blit_4xbrz_f, NULL };
static const GLchar * fblit2xbrz_v[] = { Yglprg_blit_2xbrz_v, NULL };
static const GLchar * fblit2xbrz_f[] = { Yglprg_blit_2xbrz_f, NULL };

static int up_prg = -1;
static int up_mode = -1;
static int u_size = -1;
static int up_lut_tex = -1;

static void Ygl_printShaderError( GLuint shader )
{
  GLsizei bufSize;

  glGetShaderiv(shader, GL_INFO_LOG_LENGTH , &bufSize);

  if (bufSize > 1) {
    GLchar *infoLog;

    infoLog = (GLchar *)malloc(bufSize);
    if (infoLog != NULL) {
      GLsizei length;
      glGetShaderInfoLog(shader, bufSize, &length, infoLog);
      printf("Shaderlog:\n%s\n", infoLog);
      free(infoLog);
    }
  }
}

static void Ygl_printProgError( GLuint prog )
{
  GLint maxLength = 0;

  glGetProgramiv(prog, GL_INFO_LOG_LENGTH , &maxLength);

  if (maxLength > 1) {
    GLchar *infoLog;

    infoLog = (GLchar *)malloc(maxLength);
    if (infoLog != NULL) {
      GLsizei length;
      glGetProgramInfoLog(prog, maxLength, &length, infoLog);
      printf("Proglog:\n%s\n", infoLog);
      free(infoLog);
    }
  }
  // The program is useless now. So delete it.
  glDeleteProgram(prog);
}

int YglUpscaleFramebuffer(u32 srcTexture, u32 targetFbo, float w, float h) {
  glBindFramebuffer(GL_FRAMEBUFFER, targetFbo);

  if (up_lut_tex == -1) {
    glGenTextures(1, &up_lut_tex);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, up_lut_tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, hq4x_LUT.width, hq4x_LUT.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, hq4x_LUT.pixel_data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  }

  if ((up_prg == -1) || (up_mode != _Ygl->upmode)){
    GLuint vshader;
    GLuint fshader;
    GLint compiled, linked;

    if (up_prg != -1) glDeleteProgram(up_prg);
    up_prg = glCreateProgram();
    if (up_prg == 0){
      return -1;
    }

    up_mode = _Ygl->upmode;

    vshader = glCreateShader(GL_VERTEX_SHADER);
    fshader = glCreateShader(GL_FRAGMENT_SHADER);

    switch(up_mode) {
      case UP_HQ4X:
        glShaderSource(vshader, 1, fblithq4x_v, NULL);
        break;
      case UP_4XBRZ:
        glShaderSource(vshader, 1, fblit4xbrz_v, NULL);
        break;
      case UP_2XBRZ:
        glShaderSource(vshader, 1, fblit2xbrz_v, NULL);
        break;
    }
    glCompileShader(vshader);
    glGetShaderiv(vshader, GL_COMPILE_STATUS, &compiled);
    if (compiled == GL_FALSE) {
      printf("Compile error in vertex shader.\n");
      Ygl_printShaderError(vshader);
      up_prg = -1;
      return -1;
    }
    switch(up_mode) {
      case UP_HQ4X:
        glShaderSource(fshader, 1, fblithq4x_f, NULL);
        break;
      case UP_4XBRZ:
        glShaderSource(fshader, 1, fblit4xbrz_f, NULL);
        break;
      case UP_2XBRZ:
        glShaderSource(fshader, 1, fblit2xbrz_f, NULL);
        break;
    }
    glCompileShader(fshader);
    glGetShaderiv(fshader, GL_COMPILE_STATUS, &compiled);
    if (compiled == GL_FALSE) {
      printf("Compile error in fragment shader.\n");
      Ygl_printShaderError(fshader);
      up_prg = -1;
      abort();
    }

    glAttachShader(up_prg, vshader);
    glAttachShader(up_prg, fshader);
    glLinkProgram(up_prg);
    glGetProgramiv(up_prg, GL_LINK_STATUS, &linked);
    if (linked == GL_FALSE) {
      printf("Link error..\n");
      Ygl_printProgError(up_prg);
      up_prg = -1;
      abort();
    }

    glUseProgram(up_prg);
    glUniform1i(glGetUniformLocation(up_prg, "Texture"), 0);
    if (up_mode == UP_HQ4X)
      glUniform1i(glGetUniformLocation(up_prg, "LUT"), 1);
    u_size = glGetUniformLocation(up_prg, "TextureSize");

  }
  else{
    glUseProgram(up_prg);
  }


  glDisable(GL_DEPTH_TEST);
  glDisable(GL_BLEND);

  float const vertexPosition[] = {
    1.0, -1.0f,
    -1.0, -1.0f,
    1.0, 1.0f,
    -1.0, 1.0f };

  float const textureCoord[] = {
    1.0f, 0.0f,
    0.0f, 0.0f,
    1.0f, 1.0f,
    0.0f, 1.0f };

  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, vertexPosition);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, textureCoord);

  glUniform2f(u_size, (float)w, (float)h);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, srcTexture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  if (up_mode == UP_HQ4X) {
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, up_lut_tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  }
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);


  // Clean up
  glActiveTexture(GL_TEXTURE0);
  glDisableVertexAttribArray(0);
  glDisableVertexAttribArray(1);
  glBindFramebuffer(GL_FRAMEBUFFER, _Ygl->default_fbo);
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_BLEND);

  return 0;
}

