#include "vdp1_compute.h"

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include "vdp1.h"
#include "yui.h"

//#define VDP1CDEBUG
#ifdef VDP1CDEBUG
#define VDP1CPRINT printf
#else
#define VDP1CPRINT
#endif

#define NB_COARSE_RAST (NB_COARSE_RAST_X * NB_COARSE_RAST_Y)

extern vdp2rotationparameter_struct  Vdp1ParaA;

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
static int* hasDrawingCmd;

static int cmdRam_update_start[2] = {0x0};
static int cmdRam_update_end[2] = {0x80000};

static int generateComputeBuffer(int w, int h);

static GLuint compute_tex[2] = {0};
static GLuint ssbo_cmd_ = 0;
static GLuint ssbo_vdp1ram_[2] = {0};
static GLuint ssbo_nbcmd_ = 0;
static GLuint ssbo_vdp1access_ = 0;
static GLuint prg_vdp1[NB_PRG] = {0};

static YabEventQueue *cmdq[2] = {NULL};
static int vdp1_generate_run = 0;

static u32 write_fb[512*256];

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
	//WRITE
	{
		vdp1_write_f,
		NULL,
		NULL,
		NULL
	},
	//READ
	{
		vdp1_read_f,
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
    YuiMsg("GLErrorLayer:ERROR:%04x'%s' %s\n", error_code, msg, name);
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
    YuiMsg("[COMPILE] %s\n", info);
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
    YuiMsg("[LINK] %s\n", info);
    free(info);
    abort();
  }
  return program;
}

static int generateComputeBuffer(int w, int h) {
  if (compute_tex[0] != 0) {
    glDeleteTextures(2,&compute_tex[0]);
  }
	if (ssbo_vdp1ram_[0] != 0) {
    glDeleteBuffers(2, &ssbo_vdp1ram_[0]);
	}

	glGenBuffers(2, &ssbo_vdp1ram_[0]);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_vdp1ram_[0]);
	glBufferData(GL_SHADER_STORAGE_BUFFER, 0x80000, NULL, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_vdp1ram_[1]);
	glBufferData(GL_SHADER_STORAGE_BUFFER, 0x80000, NULL, GL_DYNAMIC_DRAW);

  if (ssbo_cmd_ != 0) {
    glDeleteBuffers(1, &ssbo_cmd_);
  }
  glGenBuffers(1, &ssbo_cmd_);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_cmd_);
  glBufferData(GL_SHADER_STORAGE_BUFFER, struct_size*QUEUE_SIZE*NB_COARSE_RAST, NULL, GL_DYNAMIC_DRAW);

  if (ssbo_nbcmd_ != 0) {
    glDeleteBuffers(1, &ssbo_nbcmd_);
  }
  glGenBuffers(1, &ssbo_nbcmd_);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_nbcmd_);
  glBufferData(GL_SHADER_STORAGE_BUFFER, NB_COARSE_RAST * sizeof(int),NULL,GL_DYNAMIC_DRAW);

	if (ssbo_vdp1access_ != 0) {
    glDeleteBuffers(1, &ssbo_vdp1access_);
	}

	glGenBuffers(1, &ssbo_vdp1access_);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_vdp1access_);
	glBufferData(GL_SHADER_STORAGE_BUFFER, 512*256*4, NULL, GL_DYNAMIC_DRAW);

  glGenTextures(2, &compute_tex[0]);
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

  return 0;
}

u8 cmdBuffer[2][0x80000];

vdp1cmd_struct cmdBufferToProcess[2000];
int nbCmdToProcess = 0;

void vdp1GenerateBuffer_sync(vdp1cmd_struct* cmd, int id) {
	int endcnt;
	u32 dot;
	int pos = (cmd->CMDSRCA * 8) & 0x7FFFF;
  u8 END = ((cmd->CMDPMOD & 0x80) != 0);
	u8* buf = &cmdBuffer[id][0];
	switch ((cmd->CMDPMOD >> 3) & 0x7) {
    case 0:
    case 1:
			for(int h=0; h < cmd->h; h++) {
				endcnt = 0;
				for(int w=0; w < cmd->w/2; w++)
				{
					dot = Vdp1RamReadByte(NULL, Vdp1Ram, pos);
					if (!END && (endcnt >= 2)) {
          	dot |= 0xF0;
					}
					else if (((dot & 0xF0) == 0xF0) && !END) {
          	endcnt++;
        	} else {
						if (((cmd->CMDPMOD >> 3) & 0x7)==1) {
							//ColorLut
							u32 addr = ((dot>>4) * 2 + cmd->CMDCOLR * 8);
							u16 val = Vdp1RamReadWord(NULL, Vdp1Ram, addr);
							if (cmdRam_update_start[id] > addr) cmdRam_update_start[id] = addr;
							if (cmdRam_update_end[id] < (addr + 2)) cmdRam_update_end[id] = addr + 2;
							T1WriteWord(buf, addr, val);
						}
					}
					if (!END && (endcnt >= 2)) {
          	dot |= 0xF;
					}
					else if (((dot & 0xF) == 0xF) && !END) {
          	endcnt++;
					} else {
						if (((cmd->CMDPMOD >> 3) & 0x7)==1) {
							//ColorLut
							u32 addr = ((dot&0xF) * 2 + cmd->CMDCOLR * 8);
							u16 val = Vdp1RamReadWord(NULL, Vdp1Ram, addr);
							if (cmdRam_update_start[id] > addr) cmdRam_update_start[id] = addr;
							if (cmdRam_update_end[id] < (addr + 2)) cmdRam_update_end[id] = addr + 2;
							T1WriteWord(buf, addr, val);
						}
        	}
					if (cmdRam_update_start[id] > pos) cmdRam_update_start[id] = pos;
					if (cmdRam_update_end[id] < (pos + 1)) cmdRam_update_end[id] = pos + 1;
					T1WriteByte(buf, pos, dot);
					pos += 1;
	    	}
			}
	    break;
	    case 2:
	    case 3:
	    case 4:
				for(int h=0; h < cmd->h; h++) {
					endcnt = 0;
					for(int w = 0; w < cmd->w; w++)
					{
						dot = Vdp1RamReadByte(NULL, Vdp1Ram, pos);
						if (!END && (endcnt >= 2)) {
							dot = 0xFF;
						}
						else if ((dot == 0xFF) && !END) {
							endcnt++;
						}
						if (cmdRam_update_start[id] > pos) cmdRam_update_start[id] = pos;
						if (cmdRam_update_end[id] < (pos + 1)) cmdRam_update_end[id] = pos + 1;
						T1WriteByte(buf, pos, dot);
						pos += 1;
		    	}
				}
	    break;
	    case 5:
			for(int h=0; h < cmd->h; h++) {
				endcnt = 0;
				for(int w = 0; w < cmd->w; w++)
	    	{
					u16 dot = Vdp1RamReadWord(NULL, Vdp1Ram, pos);
					if (!END && (endcnt >= 2)) {
						dot = 0x7FFF;
					}
					else if ((dot == 0x7FFF) && !END) {
						endcnt++;
					}
					if (cmdRam_update_start[id] > pos) cmdRam_update_start[id] = pos;
					if (cmdRam_update_end[id] < (pos + 2)) cmdRam_update_end[id] = pos + 2;
					T1WriteWord(buf, pos, dot);
					pos += 2;
	    	}
			}
	    break;
	  }
}

void vdp1GenerateBuffer_async_0(void *p){
	while(vdp1_generate_run != 0){
		vdp1cmd_struct* cmd = (vdp1cmd_struct*)YabWaitEventQueue(cmdq[0]);
		if (cmd != NULL){
			vdp1GenerateBuffer_sync(cmd, 0);
			free(cmd);
		}
	}
}
void vdp1GenerateBuffer_async_1(void *p){
	while(vdp1_generate_run != 0){
		vdp1cmd_struct* cmd = (vdp1cmd_struct*)YabWaitEventQueue(cmdq[1]);
		if (cmd != NULL){
			vdp1GenerateBuffer_sync(cmd, 1);
			free(cmd);
		}
	}
}

void vdp1GenerateBuffer(vdp1cmd_struct* cmd){
	vdp1cmd_struct* cmdToSent = malloc(sizeof(vdp1cmd_struct));
	memcpy(cmdToSent, cmd, sizeof(vdp1cmd_struct));
	YabAddEventQueue(cmdq[_Ygl->drawframe], cmdToSent);
}

void regenerateVdp1Buffer(void) {
	for (int i = 0; i < nbCmdToProcess; i++) {
		vdp1GenerateBuffer(&cmdBufferToProcess[i]);
	}
	vdp1_setup();
}

int vdp1_add(vdp1cmd_struct* cmd, int clipcmd) {
	int minx = 1024;
	int miny = 1024;
	int maxx = 0;
	int maxy = 0;

	int intersectX = -1;
	int intersectY = -1;
	int requireCompute = 0;

	if (_Ygl->wireframe_mode != 0) {
		int pos = (cmd->CMDSRCA * 8) & 0x7FFFF;
		switch(cmd->type ) {
			case DISTORTED:
			//By default use the central pixel code
			switch ((cmd->CMDPMOD >> 3) & 0x7) {
				case 0:
				case 1:
				  pos += (cmd->h/2) * cmd->w/2 + cmd->w/4;
					break;
				case 2:
				case 3:
				case 4:
					pos += (cmd->h/2) * cmd->w + cmd->w/2;
					break;
				case 5:
					pos += (cmd->h/2) * cmd->w*2 + cmd->w;
					break;
			}
			cmd->COLOR[0] = cmdBuffer[_Ygl->drawframe][pos];
			cmd->type = POLYLINE;
			break;
			case POLYGON:
				cmd->type = POLYLINE;
			break;
			case QUAD:
			case QUAD_POLY:
				if ((abs(cmd->CMDXA - cmd->CMDXB) <= ((2*_Ygl->rwidth)/3)) && (abs(cmd->CMDYA - cmd->CMDYD) <= ((_Ygl->rheight)/2)))
					cmd->type = POLYLINE;
			break;
			default:
				break;
		}
	}
	if (clipcmd == 0) {
		memcpy(&cmdBufferToProcess[nbCmdToProcess++], cmd, sizeof(vdp1cmd_struct));
		vdp1GenerateBuffer(cmd);

	  float Ax = cmd->CMDXA;
		float Ay = cmd->CMDYA;
		float Bx = cmd->CMDXB;
		float By = cmd->CMDYB;
		float Cx = cmd->CMDXC;
		float Cy = cmd->CMDYC;
		float Dx = cmd->CMDXD;
		float Dy = cmd->CMDYD ;

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
	  cmd->B[1] = (maxx + 1)*tex_ratiow;
	  cmd->B[2] = miny*tex_ratioh;
	  cmd->B[3] = (maxy + 1)*tex_ratioh;

	}
  for (int i = 0; i<NB_COARSE_RAST_X; i++) {
    int blkx = i * (tex_width/NB_COARSE_RAST_X);
    for (int j = 0; j<NB_COARSE_RAST_Y; j++) {
      int blky = j*(tex_height/NB_COARSE_RAST_Y);
      if (!(blkx > maxx*_Ygl->vdp1wratio
        || (blkx + (tex_width/NB_COARSE_RAST_X)) < minx*_Ygl->vdp1wratio
        || (blky + (tex_height/NB_COARSE_RAST_Y)) < miny*_Ygl->vdp1hratio
        || blky > maxy*_Ygl->vdp1hratio)
			  || (clipcmd!=0)) {
					memcpy(&cmdVdp1[(i+j*NB_COARSE_RAST_X)*QUEUE_SIZE + nbCmd[i+j*NB_COARSE_RAST_X]], cmd, sizeof(vdp1cmd_struct));
          nbCmd[i+j*NB_COARSE_RAST_X]++;
					if (clipcmd == 0) hasDrawingCmd[i+j*NB_COARSE_RAST_X] = 1;
					if (nbCmd[i+j*NB_COARSE_RAST_X] == QUEUE_SIZE) {
						requireCompute = 1;
					}
      }
    }
  }
	if (requireCompute != 0){
		vdp1_compute();
  }
  return 0;
}

void vdp1_clear(int id, float *col) {
	int progId = CLEAR;
	if (prg_vdp1[progId] == 0)
    prg_vdp1[progId] = createProgram(sizeof(a_prg_vdp1[progId]) / sizeof(char*), (const GLchar**)a_prg_vdp1[progId]);
  glUseProgram(prg_vdp1[progId]);

	glBindImageTexture(0, compute_tex[id], 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);
	glUniform4fv(2, 1, col);
	glDispatchCompute(work_groups_x, work_groups_y, 1); //might be better to launch only the right number of workgroup
}

void vdp1_write() {
	int progId = WRITE;
	if (prg_vdp1[progId] == 0) {
    prg_vdp1[progId] = createProgram(sizeof(a_prg_vdp1[progId]) / sizeof(char*), (const GLchar**)a_prg_vdp1[progId]);
	}
  glUseProgram(prg_vdp1[progId]);

	glBindImageTexture(0, compute_tex[_Ygl->drawframe], 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssbo_vdp1access_);
	glUniform2f(2, 1.0f/_Ygl->vdp1wratio, 1.0/_Ygl->vdp1hratio);

	glDispatchCompute(work_groups_x, work_groups_y, 1); //might be better to launch only the right number of workgroup
}

u32* vdp1_read() {
	int progId = READ;
	if (prg_vdp1[progId] == 0)
    prg_vdp1[progId] = createProgram(sizeof(a_prg_vdp1[progId]) / sizeof(char*), (const GLchar**)a_prg_vdp1[progId]);
  glUseProgram(prg_vdp1[progId]);

	glBindImageTexture(0, compute_tex[_Ygl->drawframe], 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA8);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssbo_vdp1access_);
	glUniform2f(2, 1.0f/_Ygl->vdp1wratio, 1.0/_Ygl->vdp1hratio);

	//waitVdp1End
	{
	  int end = 0;
	  if (_Ygl->syncVdp1[_Ygl->drawframe] != 0) {
	    while (end == 0) {
	      int ret;
	      ret = glClientWaitSync(_Ygl->syncVdp1[_Ygl->drawframe], GL_SYNC_FLUSH_COMMANDS_BIT, 20000000);
	      if ((ret == GL_CONDITION_SATISFIED) || (ret == GL_ALREADY_SIGNALED)) end = 1;
	    }
	    glDeleteSync(_Ygl->syncVdp1[_Ygl->drawframe]);
	    _Ygl->syncVdp1[_Ygl->drawframe] = 0;
	  }
	}

	glDispatchCompute(work_groups_x, work_groups_y, 1); //might be better to launch only the right number of workgroup

  glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);

#ifdef _OGL3_
	glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0x0, 512*256*4, (void*)(&write_fb[0]));
#endif

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	return &write_fb[0];
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
	if (vdp1_generate_run == 0) {
		vdp1_generate_run = 1;
		cmdq[0] = YabThreadCreateQueue(512);
		cmdq[1] = YabThreadCreateQueue(512);
		YabThreadStart(YAB_THREAD_CS_CMD_0, vdp1GenerateBuffer_async_0, 0);
		YabThreadStart(YAB_THREAD_CS_CMD_1, vdp1GenerateBuffer_async_1, 0);
	}

  work_groups_x = _Ygl->vdp1width / local_size_x;
  work_groups_y = _Ygl->vdp1height / local_size_y;
  generateComputeBuffer(_Ygl->vdp1width, _Ygl->vdp1height);
	if (nbCmd == NULL)
  	nbCmd = (int*)malloc(NB_COARSE_RAST *sizeof(int));
	if (hasDrawingCmd == NULL)
		hasDrawingCmd = (int*)malloc(NB_COARSE_RAST *sizeof(int));
  if (cmdVdp1 == NULL)
		cmdVdp1 = (vdp1cmd_struct*)malloc(NB_COARSE_RAST*QUEUE_SIZE*sizeof(vdp1cmd_struct));
  memset(nbCmd, 0, NB_COARSE_RAST*sizeof(int));
	memset(hasDrawingCmd, 0, NB_COARSE_RAST*sizeof(int));
	memset(cmdVdp1, 0, NB_COARSE_RAST*QUEUE_SIZE*sizeof(vdp1cmd_struct*));
	return;
}

void vdp1_set_directFB() {
	if (_Ygl->vdp1IsNotEmpty == 1) {
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_vdp1access_);
		glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0x0, 512*256*4, (void*)(_Ygl->vdp1fb_buf));
		vdp1_write();
		_Ygl->vdp1On[_Ygl->drawframe] = 1;
		//vdp1_fb_map = NULL;
		_Ygl->vdp1IsNotEmpty = 0;
	}
}
void vdp1_setup(void) {
	if (ssbo_vdp1ram_[_Ygl->drawframe] == 0) return;
	if (cmdRam_update_start[_Ygl->drawframe] < cmdRam_update_end[_Ygl->drawframe]) {
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_vdp1ram_[_Ygl->drawframe]);
		glBufferSubData(GL_SHADER_STORAGE_BUFFER, cmdRam_update_start[_Ygl->drawframe], (cmdRam_update_end[_Ygl->drawframe] - cmdRam_update_start[_Ygl->drawframe]), (void*)(&(cmdBuffer[_Ygl->drawframe])[cmdRam_update_start[_Ygl->drawframe]]));
		cmdRam_update_start[_Ygl->drawframe] = 0x80000;
		cmdRam_update_end[_Ygl->drawframe] = 0x0;
	}
}

int * get_vdp1_tex() {
	return &compute_tex[_Ygl->readframe];
}

void vdp1_compute() {
  GLuint error;
	int progId = getProgramId();
	int needRender = _Ygl->vdp1IsNotEmpty;

	if (prg_vdp1[progId] == 0)
    prg_vdp1[progId] = createProgram(sizeof(a_prg_vdp1[progId]) / sizeof(char*), (const GLchar**)a_prg_vdp1[progId]);
  glUseProgram(prg_vdp1[progId]);

	VDP1CPRINT("Draw VDP1\n");

  glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_cmd_);
  for (int i = 0; i < NB_COARSE_RAST; i++) {
    if (hasDrawingCmd[i] == 0) nbCmd[i] = 0;
    if (nbCmd[i] != 0) {
			// printf("%d\n", nbCmd[i]);
    	glBufferSubData(GL_SHADER_STORAGE_BUFFER, struct_size*i*QUEUE_SIZE, nbCmd[i]*sizeof(vdp1cmd_struct), (void*)&cmdVdp1[QUEUE_SIZE*i]);
			needRender = 1;
		}
  }
  if (needRender == 0) {
		return;
	}
	if ((Vdp1External.updateVdp1Ram == 1)&&(Vdp1External.checkEDSR == 0)) {
		//The game source code has modified the content of vdp1Ram since the drawcommands order without checking the EDSR register
		//Let's assume the game was taking care of vdp1 drawing delay to update the vdp1Ram
		//So update the texture just to be sure all texture are the latest one
		regenerateVdp1Buffer();
	}
	Vdp1External.updateVdp1Ram = 0;
	nbCmdToProcess = 0;
	_Ygl->vdp1On[_Ygl->drawframe] = 1;

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_nbcmd_);
  glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(int)*NB_COARSE_RAST, (void*)nbCmd);

	glBindImageTexture(0, compute_tex[_Ygl->drawframe], 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);

	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, ssbo_vdp1ram_[_Ygl->drawframe]);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, ssbo_nbcmd_);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, ssbo_cmd_);
	glUniform2f(6, tex_ratiow, tex_ratioh);
	glUniform2i(7, Vdp1Regs->systemclipX2, Vdp1Regs->systemclipY2);
	glUniform4i(8, Vdp1Regs->userclipX1, Vdp1Regs->userclipY1, Vdp1Regs->userclipX2, Vdp1Regs->userclipY2);
	YglMatrix m, mat;
	YglLoadIdentity(&m);
  if (Vdp1Regs->TVMR & 0x02) {
    YglMatrix rotate, scale;
    YglLoadIdentity(&rotate);
    rotate.m[0][0] = Vdp1ParaA.deltaX;
    rotate.m[0][1] = Vdp1ParaA.deltaY;
    rotate.m[1][0] = Vdp1ParaA.deltaXst;
    rotate.m[1][1] = Vdp1ParaA.deltaYst;
		rotate.m[0][3] = Vdp1ParaA.Xst;
		rotate.m[1][3] = -Vdp1ParaA.Yst;

    YglMatrixMultiply(&mat, &m, &rotate);
    YglLoadIdentity(&scale);
    // scale.m[0][0] = 1.0;
    // scale.m[1][1] = 1.0 / (1.0 + Vdp1ParaA.deltaY);
    // scale.m[0][3] = 0.0;
    // scale.m[1][3] = 1.0 - scale.m[1][1];

    YglMatrixMultiply(&m, &scale, &mat);

  }
  glUniformMatrix4fv(9, 1, 0, (GLfloat*)m.m);

	vdp1_set_directFB();
  glDispatchCompute(work_groups_x, work_groups_y, 1); //might be better to launch only the right number of workgroup
	if (_Ygl->syncVdp1[_Ygl->drawframe] != 0) {
		glDeleteSync(_Ygl->syncVdp1[_Ygl->drawframe]);
		_Ygl->syncVdp1[_Ygl->drawframe] = 0;
	}
	_Ygl->syncVdp1[_Ygl->drawframe] = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE,0);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
  ErrorHandle("glDispatchCompute");

	//glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BI
  memset(nbCmd, 0, NB_COARSE_RAST*sizeof(int));
	memset(hasDrawingCmd, 0, NB_COARSE_RAST*sizeof(int));
  glBindBuffer(GL_UNIFORM_BUFFER, 0);
	glBindTexture(GL_TEXTURE_2D, 0);
  return;
}
