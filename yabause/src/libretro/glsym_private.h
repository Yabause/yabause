#ifndef RGLGEN_PRIV_DECL_H__
#define RGLGEN_PRIV_DECL_H__
#ifdef __cplusplus
extern "C" {
#endif

#if defined(_OGLES3_)
typedef double GLclampd;
typedef double GLdouble;
typedef void (GL_APIENTRYP RGLSYMGLMEMORYBARRIERPROC) (GLbitfield barriers);
typedef void (GL_APIENTRYP RGLSYMGLBINDFRAGDATALOCATIONPROC) (GLuint program, GLuint color, const GLchar *name);
typedef void (GL_APIENTRYP RGLSYMGLPATCHPARAMETERIPROC) (GLenum pname, GLint value);
typedef void (GL_APIENTRYP RGLSYMGLDISPATCHCOMPUTEPROC) (GLuint num_groups_x, GLuint num_groups_y, GLuint num_groups_z);
typedef void (GL_APIENTRYP RGLSYMGLBINDIMAGETEXTUREPROC) (GLuint unit, GLuint texture, GLint level, GLboolean layered, GLint layer, GLenum access, GLenum format);

#define glMemoryBarrier __rglgen_glMemoryBarrier
#define glBindFragDataLocation __rglgen_glBindFragDataLocation
#define glPatchParameteri __rglgen_glPatchParameteri
#define glDispatchCompute __rglgen_glDispatchCompute
#define glBindImageTexture __rglgen_glBindImageTexture

#define GL_PATCHES                         0x000E
#define GL_FRAMEBUFFER_BARRIER_BIT         0x00000400
#define GL_TEXTURE_UPDATE_BARRIER_BIT      0x00000100
#define GL_TEXTURE_FETCH_BARRIER_BIT       0x00000008
#define GL_TESS_CONTROL_SHADER             0x8E88
#define GL_TESS_EVALUATION_SHADER          0x8E87
#define GL_GEOMETRY_SHADER                 0x8DD9
#define GL_PATCH_VERTICES                  0x8E72
#define GL_COMPUTE_SHADER                  0x91B9
#define GL_SHADER_STORAGE_BUFFER           0x90D2
#define GL_WRITE_ONLY                      0x88B9
#define GL_SHADER_IMAGE_ACCESS_BARRIER_BIT 0x00000020

extern RGLSYMGLMEMORYBARRIERPROC __rglgen_glMemoryBarrier;
extern RGLSYMGLBINDFRAGDATALOCATIONPROC __rglgen_glBindFragDataLocation;
extern RGLSYMGLPATCHPARAMETERIPROC __rglgen_glPatchParameteri;
extern RGLSYMGLDISPATCHCOMPUTEPROC __rglgen_glDispatchCompute;
extern RGLSYMGLBINDIMAGETEXTUREPROC __rglgen_glBindImageTexture;
#else
typedef void (APIENTRYP RGLSYMGLTEXTUREBARRIERNVPROC) (void);

#define glTextureBarrierNV __rglgen_glTextureBarrierNV

extern RGLSYMGLTEXTUREBARRIERNVPROC __rglgen_glTextureBarrierNV;
#endif

#ifdef __cplusplus
}
#endif
#endif
