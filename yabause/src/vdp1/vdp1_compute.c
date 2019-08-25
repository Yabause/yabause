#include "vdp1_compute.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "vdp1.h"

//#define VDP1CDEBUG
#ifdef VDP1CDEBUG
#define VDP1CPRINT printf
#else
#define VDP1CPRINT
#endif

#define NB_COARSE_RAST (NB_COARSE_RAST_X * NB_COARSE_RAST_Y)

static int local_size_x = LOCAL_SIZE_X;
static int local_size_y = LOCAL_SIZE_Y;

static int tex_width;
static int tex_height;
static float tex_ratiow;
static float tex_ratioh;
static int struct_size;

static int work_groups_x;
static int work_groups_y;

static vdp1cmd_struct* cmdVdp1;
static int* nbCmd;

static int* clear;

static int generateComputeBuffer(int w, int h);

static GLuint compute_tex[4] = {0};
static GLuint ssbo_cmd_ = 0;
static GLuint ssbo_vdp1ram_ = 0;
static GLuint ssbo_nbcmd_ = 0;
static GLuint prg_vdp1[NB_PRG] = {0};

static const GLchar * a_prg_vdp1[NB_PRG][4] = {
  //VDP1_MESH_STANDARD
	{
		vdp1_start_f,
		vdp1_standard_mesh_f,
		vdp1_continue_f,
		vdp1_end_f
	},
	//VDP1_MESH_IMPROVED
	{
		vdp1_start_f,
		vdp1_improved_mesh_f,
		vdp1_continue_f,
		vdp1_end_f
	},
	//BLIT
	{
		vdp1_blit_f,
		NULL,
		NULL,
		NULL
	},
	//CLEAR
	{
		vdp1_clear_f,
		NULL,
		NULL,
		NULL
  },
};

static int getProgramId() {
	if (_Ygl->meshmode == ORIGINAL_MESH)
    return VDP1_MESH_STANDARD;
	else
	  return VDP1_MESH_IMPROVED;
}

int ErrorHandle(const char* name)
{
#ifdef VDP1CDEBUG
  GLenum   error_code = glGetError();
  if (error_code == GL_NO_ERROR) {
    return  1;
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
    VDP1CPRINT("GLErrorLayer:ERROR:%04x'%s' %s\n", error_code, msg, name);
    error_code = glGetError();
  } while (error_code != GL_NO_ERROR);
  abort();
  return 0;
#else
  return 1;
#endif
}

static GLuint createProgram(int count, const GLchar** prg_strs) {
  GLint status;
	int exactCount = 0;
  GLuint result = glCreateShader(GL_COMPUTE_SHADER);

  for (int id = 0; id < count; id++) {
		if (prg_strs[id] != NULL) exactCount++;
		else break;
	}
  glShaderSource(result, exactCount, prg_strs, NULL);
  glCompileShader(result);
  glGetShaderiv(result, GL_COMPILE_STATUS, &status);

  if (status == GL_FALSE) {
    GLint length;
    glGetShaderiv(result, GL_INFO_LOG_LENGTH, &length);
    GLchar *info = malloc(sizeof(GLchar) *length);
    glGetShaderInfoLog(result, length, NULL, info);
    VDP1CPRINT("[COMPILE] %s\n", info);
    free(info);
    abort();
  }
  GLuint program = glCreateProgram();
  glAttachShader(program, result);
  glLinkProgram(program);
  glDetachShader(program, result);
  glGetProgramiv(program, GL_LINK_STATUS, &status);
  if (status == GL_FALSE) {
    GLint length;
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
    GLchar *info = malloc(sizeof(GLchar) *length);
    glGetProgramInfoLog(program, length, NULL, info);
    VDP1CPRINT("[LINK] %s\n", info);
    free(info);
    abort();
  }
  return program;
}

static int generateComputeBuffer(int w, int h) {
  if (compute_tex[0] != 0) {
    glDeleteTextures(4,&compute_tex[0]);
  }
	if (ssbo_vdp1ram_ != 0) {
    glDeleteBuffers(1, &ssbo_vdp1ram_);
	}

	glGenBuffers(1, &ssbo_vdp1ram_);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_vdp1ram_);
	glBufferData(GL_SHADER_STORAGE_BUFFER, 0x80000, NULL, GL_DYNAMIC_DRAW);

  if (ssbo_cmd_ != 0) {
    glDeleteBuffers(1, &ssbo_cmd_);
  }
  glGenBuffers(1, &ssbo_cmd_);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_cmd_);
  glBufferData(GL_SHADER_STORAGE_BUFFER, struct_size*2000*NB_COARSE_RAST, NULL, GL_DYNAMIC_DRAW);

  if (ssbo_nbcmd_ != 0) {
    glDeleteBuffers(1, &ssbo_nbcmd_);
  }
  glGenBuffers(1, &ssbo_nbcmd_);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_nbcmd_);
  glBufferData(GL_SHADER_STORAGE_BUFFER, NB_COARSE_RAST * sizeof(int),NULL,GL_DYNAMIC_DRAW);

  glGenTextures(4, &compute_tex[0]);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, compute_tex[0]);
  glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
  glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, w, h);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glBindTexture(GL_TEXTURE_2D, compute_tex[1]);
  glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
  glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, w, h);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glBindTexture(GL_TEXTURE_2D, compute_tex[2]);
  glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
  glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, w, h);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glBindTexture(GL_TEXTURE_2D, compute_tex[3]);
  glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
  glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, w, h);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	if (clear != NULL) free(clear);
	clear = (int*)malloc(w*h * sizeof(int));
	memset(clear, 0, w*h * sizeof(int));

  return 0;
}

int vdp1_add(vdp1cmd_struct* cmd, int clipcmd) {
	int minx = 1024;
	int miny = 1024;
	int maxx = 0;
	int maxy = 0;

  int *C = &cmd->CMDXA;
	for (int i = 0; i<4; i++) C[i*2] = (C[i*2] * _Ygl->vdp1wratio);
	for (int i = 0; i<4; i++) C[i*2+1] = (C[i*2+1] * _Ygl->vdp1hratio);
	if (clipcmd == 0) {

		memcpy(cmd->P,&cmd->CMDXA,8*sizeof(int));

		for (int i = 0; i<8; i++) cmd->P[i] = cmd->P[i] * 2 - 1;

		int right = 0;
		int rightindex = -1;
		int top = 0;
		int topindex = -1;

		for (int i = 0; i<4; i++) {
			if ((cmd->P[i*2]+cmd->P[((i+1)%4)*2]) > right) {
				right = (cmd->P[i*2]+cmd->P[((i+1)%4)*2]);
				rightindex = i;
			}
		}
		cmd->P[rightindex*2] += 2;
		cmd->P[((rightindex+1)%4)*2] += 2;

		for (int i = 0; i<4; i++) {
			if ((cmd->P[i*2+1]+cmd->P[((i+1)%4)*2+1]) > top) {
				top = (cmd->P[i*2+1]+cmd->P[((i+1)%4)*2+1]);
				topindex = i;
			}
		}
		cmd->P[topindex*2+1] += 2;
		cmd->P[((topindex+1)%4)*2+1] += 2;

	  float Ax = cmd->P[0]/2.0;
		float Ay = cmd->P[1]/2.0;
		float Bx = cmd->P[2]/2.0;
		float By = cmd->P[3]/2.0;
		float Cx = cmd->P[4]/2.0;
		float Cy = cmd->P[5]/2.0;
		float Dx = cmd->P[6]/2.0;
		float Dy = cmd->P[7]/2.0;

	  minx = (Ax < Bx)?Ax:Bx;
	  miny = (Ay < By)?Ay:By;
	  maxx = (Ax > Bx)?Ax:Bx;
	  maxy = (Ay > By)?Ay:By;

	  minx = (minx < Cx)?minx:Cx;
	  minx = (minx < Dx)?minx:Dx;
	  miny = (miny < Cy)?miny:Cy;
	  miny = (miny < Dy)?miny:Dy;
	  maxx = (maxx > Cx)?maxx:Cx;
	  maxx = (maxx > Dx)?maxx:Dx;
	  maxy = (maxy > Cy)?maxy:Cy;
	  maxy = (maxy > Dy)?maxy:Dy;

	//Add a bounding box
	  cmd->B[0] = minx*tex_ratiow;
	  cmd->B[1] = maxx*tex_ratiow;
	  cmd->B[2] = miny*tex_ratioh;
	  cmd->B[3] = maxy*tex_ratioh;
	}
  int intersectX = -1;
  int intersectY = -1;
  for (int i = 0; i<NB_COARSE_RAST_X; i++) {
    int blkx = i * (tex_width/NB_COARSE_RAST_X);
    for (int j = 0; j<NB_COARSE_RAST_Y; j++) {
      int blky = j*(tex_height/NB_COARSE_RAST_Y);
      if (!(blkx > maxx
        || (blkx + (tex_width/NB_COARSE_RAST_X)) < minx
        || (blky + (tex_height/NB_COARSE_RAST_Y)) < miny
        || blky > maxy)
			  || (clipcmd!=0)) {
					memcpy(&cmdVdp1[(i+j*NB_COARSE_RAST_X)*2000 + nbCmd[i+j*NB_COARSE_RAST_X]], cmd, sizeof(vdp1cmd_struct));
          nbCmd[i+j*NB_COARSE_RAST_X]++;
      }
    }
  }
}

void vdp1_clear(int id) {
	int progId = CLEAR;
	//printf("USe Prog %d\n", progId);
	if (prg_vdp1[progId] == 0)
    prg_vdp1[progId] = createProgram(sizeof(a_prg_vdp1[progId]) / sizeof(char*), (const GLchar**)a_prg_vdp1[progId]);
  glUseProgram(prg_vdp1[progId]);

	glBindImageTexture(0, compute_tex[id*2], 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);
	glBindImageTexture(1, compute_tex[id*2+1], 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);
	glDispatchCompute(work_groups_x, work_groups_y, 1); //might be better to launch only the right number of workgroup
}

void vdp1_blit(int tex, int id) {
	int progId = BLIT;
	//printf("USe Prog %d\n", progId);
	if (prg_vdp1[progId] == 0)
    prg_vdp1[progId] = createProgram(sizeof(a_prg_vdp1[progId]) / sizeof(char*), (const GLchar**)a_prg_vdp1[progId]);
  glUseProgram(prg_vdp1[progId]);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, tex);

	glBindImageTexture(0, compute_tex[id*2], 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);
	glUniform1i(1, 0);
	glUniform2f(2, (float)_Ygl->rwidth/(float)(tex_width*tex_ratiow), (float)_Ygl->rheight/(float)(tex_height*tex_ratioh));

	glDispatchCompute(work_groups_x, work_groups_y, 1); //might be better to launch only the right number of workgroup
}


void vdp1_compute_init(int width, int height, float ratiow, float ratioh)
{
  int am = sizeof(vdp1cmd_struct) % 16;
  tex_width = width;
  tex_height = height;
	tex_ratiow = ratiow;
	tex_ratioh = ratioh;
  struct_size = sizeof(vdp1cmd_struct);
  if (am != 0) {
    struct_size += 16 - am;
  }

  work_groups_x = (tex_width*tex_ratiow) / local_size_x;
  work_groups_y = (tex_height*tex_ratioh) / local_size_y;
  generateComputeBuffer(tex_width*tex_ratiow, tex_height*tex_ratioh);
	if (nbCmd == NULL)
  	nbCmd = (int*)malloc(NB_COARSE_RAST *sizeof(int));
  if (cmdVdp1 == NULL)
		cmdVdp1 = (vdp1cmd_struct*)malloc(NB_COARSE_RAST*2000*sizeof(vdp1cmd_struct));
  memset(nbCmd, 0, NB_COARSE_RAST*sizeof(int));
	memset(cmdVdp1, 0, NB_COARSE_RAST*2000*sizeof(vdp1cmd_struct*));
	return;
}

void vdp1_setup(void) {
	if (ssbo_vdp1ram_ == 0) return;
	if (vdp1Ram_update_start < vdp1Ram_update_end) {
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_vdp1ram_);
		glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0x0, 0x80000, (void*)(&Vdp1Ram[0]));
		vdp1Ram_update_start = 0x80000;
		vdp1Ram_update_end = 0x0;
	}
}

int* vdp1_compute(Vdp2 *varVdp2Regs, int id) {
  GLuint error;
	int progId = getProgramId();
	int needRender = 0;
	//printf("USe Prog %d\n", progId);
	if (prg_vdp1[progId] == 0)
    prg_vdp1[progId] = createProgram(sizeof(a_prg_vdp1[progId]) / sizeof(char*), (const GLchar**)a_prg_vdp1[progId]);
  glUseProgram(prg_vdp1[progId]);

	VDP1CPRINT("Draw VDP1\n");

  glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_cmd_);
  for (int i = 0; i < NB_COARSE_RAST; i++) {
    if (nbCmd[i] != 0) {
    	glBufferSubData(GL_SHADER_STORAGE_BUFFER, struct_size*i*2000, nbCmd[i]*sizeof(vdp1cmd_struct), (void*)&cmdVdp1[2000*i]);
			needRender = 1;
		}
  }

  if (needRender == 0) return &compute_tex[id*2];

	_Ygl->vdp1On[_Ygl->drawframe] = 1;

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_nbcmd_);
  glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(int)*NB_COARSE_RAST, (void*)nbCmd);

	glBindImageTexture(0, compute_tex[id*2], 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);
	glBindImageTexture(1, compute_tex[id*2+1], 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);

#ifdef USE_VDP1_TEX
	glUniform1i(2, 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, YglTM_vdp1[id]->textureID);
#endif
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, ssbo_vdp1ram_);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, ssbo_nbcmd_);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, ssbo_cmd_);
	glUniform2f(6, tex_ratiow, tex_ratioh);
	glUniform2i(7, Vdp1Regs->systemclipX2, Vdp1Regs->systemclipY2);
	glUniform4i(8, Vdp1Regs->userclipX1, Vdp1Regs->userclipY1, Vdp1Regs->userclipX2, Vdp1Regs->userclipY2);

  glDispatchCompute(work_groups_x, work_groups_y, 1); //might be better to launch only the right number of workgroup
	// glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
  ErrorHandle("glDispatchCompute");
	//glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BI
  memset(nbCmd, 0, NB_COARSE_RAST*sizeof(int));
  glBindBuffer(GL_UNIFORM_BUFFER, 0);
	glBindTexture(GL_TEXTURE_2D, 0);

  return &compute_tex[id*2];
}
