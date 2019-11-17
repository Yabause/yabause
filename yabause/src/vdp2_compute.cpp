/*
Copyright 2011-2015 Shinya Miyamoto(devmiyax)

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

extern "C"{
#include "ygl.h"
#include "yui.h"
#include "vidshared.h"

#include "common_glshader.h"
}

#define YGLDEBUG

#define DEBUGWIP

static const GLchar vdp2blit_cs_start_f[] =
SHADER_VERSION_COMPUTE
"#ifdef GL_ES\n"
"precision highp float; \n"
"#endif\n"
"layout(local_size_x = 16, local_size_y = 16) in;\n"
"layout(std430, binding = 13) readonly buffer VDP2reg { int s_vdp2reg[]; }; \n"
"layout(rgba8, binding = 14) writeonly highp uniform image2D outSurface;\n"
"layout(std430, binding = 15) readonly buffer VDP2DrawInfo { \n"
"  float u_emu_height;\n"
"  float u_emu_vdp1_width;\n"
"  float u_emu_vdp2_width;\n"
"  float u_vheight; \n"
"  vec2 vdp1Ratio; \n"
"  int fbon;  \n"
"  int screen_nb;  \n"
"  int ram_mode; \n"
"  int extended_cc; \n"
"  int use_cc_win; \n"
"  int u_lncl[7];  \n"
"  int mode[7];  \n"
"  int isRGB[6]; \n"
"  int isBlur[7]; \n"
"  int isShadow[6]; \n"
"  int use_sp_win;\n"
"  mat4 fbMat;\n"
"};\n"

"vec4 finalColor;\n"
"ivec2 texel = ivec2(0,0);\n"
"int PosY = texel.y;\n"
"int PosX = texel.x;\n"

"float getVdp2RegAsFloat(int id) {\n"
"  return float(s_vdp2reg[id])/255.0;\n"
"};\n"
"layout(binding = 0) uniform sampler2D s_texture0;  \n"
"layout(binding = 1) uniform sampler2D s_texture1;  \n"
"layout(binding = 2) uniform sampler2D s_texture2;  \n"
"layout(binding = 3) uniform sampler2D s_texture3;  \n"
"layout(binding = 4) uniform sampler2D s_texture4;  \n"
"layout(binding = 5) uniform sampler2D s_texture5;  \n"
"layout(binding = 7) uniform sampler2D s_back;  \n"
"layout(binding = 8) uniform sampler2D s_lncl;  \n"
"layout(binding = 9) uniform sampler2D s_vdp1FrameBuffer;\n"
"layout(binding = 10) uniform sampler2D s_win0;  \n"
"layout(binding = 11) uniform sampler2D s_win1;  \n"
"layout(binding = 12) uniform sampler2D s_color; \n";

const GLchar Yglprg_vdp2_drawfb_cs_cram_f[] =
"int getVDP2Reg(int id) {\n"
"  return s_vdp2reg[id];\n"
"}\n"
"FBCol getFB(int x, ivec2 addr){ \n"
"  vec4 lineCoord = vec4(texel.x, (u_vheight-texel.y), 0.0, 0.0);\n"
"  int line = int(lineCoord.y * u_emu_height)*24;\n"
"  vec3 u_coloroffset = vec3(getVdp2RegAsFloat(17 + line), getVdp2RegAsFloat(18 + line), getVdp2RegAsFloat(19 + line));\n"
"  vec3 u_coloroffset_sign = vec3(getVdp2RegAsFloat(20 + line), getVdp2RegAsFloat(21 + line), getVdp2RegAsFloat(22 + line));\n";

static const GLchar vdp2blit_cs_end_f[] =

"  texel = ivec2(gl_GlobalInvocationID.xy);\n"
"  ivec2 size = imageSize(outSurface);\n"
"  if (texel.x >= size.x || texel.y >= size.y ) return;\n"
"  vec2 v_texcoord = vec2(float(texel.x)/float(size.x),float(texel.y)/float(size.y));\n";

static const GLchar vdp2blit_cs_final_f[] =
"  imageStore(outSurface,texel,finalColor);\n"
"}\n";

void initDrawShaderCode() {
  initVDP2DrawCode(vdp2blit_cs_start_f, Yglprg_vdp2_drawfb_cs_cram_f, vdp2blit_cs_end_f, vdp2blit_cs_final_f);
}

struct VDP2DrawInfo {
	float u_emu_height;
	float u_emu_vdp1_width;
	float u_emu_vdp2_width;
	float u_vheight;
	float vdp1Ratio[2];
	int fbon;
	int screen_nb;
	int ram_mode;
	int extended_cc;
	int u_lncl[7];
	int mode[7];
	int isRGB[6];
	int isBlur[7];
	int isShadow[6];
	int use_sp_win;
  float fbMat[4];
};

class VDP2Generator{
  GLuint prg_vdp2_composer[20] = {0};

  int tex_width_ = 0;
  int tex_height_ = 0;
  static VDP2Generator * instance_;
  GLuint scene_uniform = 0;
	GLuint ssbo_vdp2reg_ = 0;
  VDP2DrawInfo uniform;
	int struct_size_;

protected:
  VDP2Generator() {
    tex_width_ = 0;
    tex_height_ = 0;
		scene_uniform = 0;
		ssbo_vdp2reg_ = 0;
		struct_size_ = sizeof(VDP2DrawInfo);
		int am = sizeof(VDP2DrawInfo) % 4;
		if (am != 0) {
			struct_size_ += 4 - am;
		}
  }

public:
  static VDP2Generator * getInstance() {
    if (instance_ == nullptr) {
      instance_ = new VDP2Generator();
    }
    return instance_;
  }

  void resize(int width, int height) {
	if (tex_width_ == width && tex_height_ == height) return;

	DEBUGWIP("resize %d, %d\n",width,height);

	tex_width_ = width;
	tex_height_ = height;
  }

  //-----------------------------------------------
  void init( int width, int height ) {

		if (scene_uniform != 0) return; // always inisialized!

		tex_width_ = width;
		tex_height_ = height;

		initDrawShaderCode();

		glGenBuffers(1, &scene_uniform);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, scene_uniform);
		glBufferData(GL_SHADER_STORAGE_BUFFER, struct_size_, NULL, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

		glGenBuffers(1, &ssbo_vdp2reg_);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_vdp2reg_);
		glBufferData(GL_SHADER_STORAGE_BUFFER, 512*sizeof(int)*NB_VDP2_REG,(void*)YglGetVDP2RegPointer(), GL_DYNAMIC_DRAW);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	}

  bool ErrorHandle(const char* name)
  {
  #ifdef DEBUG
    GLenum   error_code = glGetError();
    if (error_code == GL_NO_ERROR) {
      return  true;
    }
    do {
      const char* msg = "";
      switch (error_code) {
      case GL_INVALID_ENUM:      msg = "INVALID_ENUM";      break;
      case GL_INVALID_VALUE:     msg = "INVALID_VALUE";     break;
      case GL_INVALID_OPERATION: msg = "INVALID_OPERATION"; break;
      case GL_OUT_OF_MEMORY:     msg = "OUT_OF_MEMORY";     break;
      case GL_INVALID_FRAMEBUFFER_OPERATION:  msg = "INVALID_FRAMEBUFFER_OPERATION"; break;
      default:  msg = "Unknown"; break;
      }
      YGLDEBUG("GLErrorLayer:ERROR:%04x'%s' %s\n", error_code, msg, name);
	  abort();
      error_code = glGetError();
    } while (error_code != GL_NO_ERROR);
    return  false;
	#else
	  return true;
	#endif
  }

  template<typename T>
  T Add(T a, T b) {
	  return a + b;
  }


//#define COMPILE_COLOR_DOT( BASE, COLOR , DOT ) ({ GLuint PRG; BASE[sizeof(BASE)/sizeof(char*) - 2] = COLOR; BASE[sizeof(BASE)/sizeof(char*) - 1] = DOT; PRG=createProgram(sizeof(BASE)/sizeof(char*), (const GLchar**)BASE);})

#define COMPILE_COLOR_DOT( BASE, COLOR , DOT )
#define S(A) A, sizeof(A)/sizeof(char*)

	void update( int outputTex, YglPerLineInfo *bg, int* prioscreens, int* modescreens, int* isRGB, int* isShadow, int * isBlur, int* lncl, GLuint* vdp1fb, Vdp2 *varVdp2Regs) {

    GLuint error;
    int local_size_x = 16;
    int local_size_y = 16;
	  int nbScreen = 6;

		int work_groups_x = (tex_width_) / local_size_x;
    int work_groups_y = (tex_height_) / local_size_y;

		int gltext[6] = {GL_TEXTURE0, GL_TEXTURE1, GL_TEXTURE2, GL_TEXTURE3, GL_TEXTURE4, GL_TEXTURE5};

    error = glGetError();

	  DEBUGWIP("prog %d\n", __LINE__);
		setupVDP2Prog(varVdp2Regs, 1);

		memcpy(uniform.u_lncl,lncl, 7*sizeof(int));
		memcpy(uniform.mode, modescreens, 7*sizeof(int));
		memcpy(uniform.isRGB, isRGB, 6*sizeof(int));
		memcpy(uniform.isBlur, isBlur, 7*sizeof(int));
		memcpy(uniform.isShadow, isShadow, 6*sizeof(int));
		uniform.u_emu_height = (float)_Ygl->rheight / (float)_Ygl->height;
		uniform.u_vheight = (float)_Ygl->height;
		uniform.fbon = (_Ygl->vdp1On[_Ygl->readframe] != 0);
		uniform.ram_mode = Vdp2Internal.ColorMode;
		uniform.extended_cc = ((varVdp2Regs->CCCTL & 0x400) != 0);
		uniform.use_sp_win = ((varVdp2Regs->SPCTL>>4)&0x1);

		glActiveTexture(GL_TEXTURE7);
	  glBindTexture(GL_TEXTURE_2D, _Ygl->back_fbotex[0]);

	  glActiveTexture(GL_TEXTURE8);
	  glBindTexture(GL_TEXTURE_2D, _Ygl->lincolor_tex);

		glActiveTexture(GL_TEXTURE9);
		glBindTexture(GL_TEXTURE_2D, vdp1fb[0]);

		glActiveTexture(GL_TEXTURE10);
		glBindTexture(GL_TEXTURE_2D, _Ygl->window_tex[0]);

		glActiveTexture(GL_TEXTURE11);
		glBindTexture(GL_TEXTURE_2D, _Ygl->window_tex[1]);

		glActiveTexture(GL_TEXTURE12);
	  glBindTexture(GL_TEXTURE_2D, _Ygl->cram_tex);

#if 0
		glActiveTexture(GL_TEXTURE13);
	  glBindTexture(GL_TEXTURE_2D, _Ygl->vdp2reg_tex);
#else
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_vdp2reg_);
		u8* vdp2buf = YglGetVDP2RegPointer();
		glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, 512*sizeof(int)*NB_VDP2_REG,(void*)vdp2buf);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 13, ssbo_vdp2reg_);
#endif

		int id = 0;
	  for (int i=0; i<nbScreen; i++) {
	    if (prioscreens[i] != 0) {
	      glActiveTexture(gltext[i]);
	      glBindTexture(GL_TEXTURE_2D, prioscreens[i]);
	      id++;
	    }
	  }
	  uniform.screen_nb = id;

		DEBUGWIP("Draw RBG0\n");
		glBindImageTexture(14, outputTex, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);
		ErrorHandle("glBindImageTexture 0");

		glBindBuffer(GL_SHADER_STORAGE_BUFFER, scene_uniform);
		glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, struct_size_, (void*)&uniform);
		ErrorHandle("glBufferSubData");
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 15, scene_uniform);


	  glDispatchCompute(work_groups_x, work_groups_y, 1);
		// glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
	  ErrorHandle("glDispatchCompute");

	  glBindBuffer(GL_UNIFORM_BUFFER, 0);
  }

  //-----------------------------------------------
  void onFinish() {
  }

};

VDP2Generator * VDP2Generator::instance_ = nullptr;

extern "C" {
  void VDP2Generator_init(int width, int height) {
    if (_Ygl->vdp2_use_compute_shader == 0) return;

    VDP2Generator * instance = VDP2Generator::getInstance();
    instance->init( width, height);
  }
  void VDP2Generator_resize(int width, int height) {
    if (_Ygl->vdp2_use_compute_shader == 0) return;
    YGLDEBUG("VDP2Generator_resize\n");
	  VDP2Generator * instance = VDP2Generator::getInstance();
	  instance->resize(width, height);
  }
  void VDP2Generator_update(int tex, YglPerLineInfo *bg, int* prioscreens, int* modescreens, int* isRGB, int* isShadow, int * isBlur, int* lncl, GLuint* vdp1fb, Vdp2 *varVdp2Regs ) {
    if (_Ygl->vdp2_use_compute_shader == 0) return;
    VDP2Generator * instance = VDP2Generator::getInstance();
    instance->update(tex, bg, prioscreens, modescreens, isRGB, isShadow, isBlur, lncl, vdp1fb, varVdp2Regs);
  }
  void VDP2Generator_onFinish() {

    if (_Ygl->vdp2_use_compute_shader == 0) return;
    VDP2Generator * instance = VDP2Generator::getInstance();
    instance->onFinish();
  }
}
