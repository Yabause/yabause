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

extern RGLSYMGLMEMORYBARRIERPROC __rglgen_glMemoryBarrier;
extern RGLSYMGLBINDFRAGDATALOCATIONPROC __rglgen_glBindFragDataLocation;
extern RGLSYMGLPATCHPARAMETERIPROC __rglgen_glPatchParameteri;
extern RGLSYMGLDISPATCHCOMPUTEPROC __rglgen_glDispatchCompute;
extern RGLSYMGLBINDIMAGETEXTUREPROC __rglgen_glBindImageTexture;
#else
typedef void (APIENTRYP RGLSYMGLTEXTUREBARRIERNVPROC) (void);
typedef void (APIENTRYP RGLSYMGLTEXTUREBARRIERPROC) (void);

#define glTextureBarrierNV __rglgen_glTextureBarrierNV
#define glTextureBarrier __rglgen_glTextureBarrier

extern RGLSYMGLTEXTUREBARRIERNVPROC __rglgen_glTextureBarrierNV;
extern RGLSYMGLTEXTUREBARRIERPROC __rglgen_glTextureBarrier;
#endif

#ifndef GL_PATCHES
#define GL_PATCHES                         0x000E
#endif
#ifndef GL_FRAMEBUFFER_BARRIER_BIT
#define GL_FRAMEBUFFER_BARRIER_BIT         0x00000400
#endif
#ifndef GL_TEXTURE_UPDATE_BARRIER_BIT
#define GL_TEXTURE_UPDATE_BARRIER_BIT      0x00000100
#endif
#ifndef GL_TEXTURE_FETCH_BARRIER_BIT
#define GL_TEXTURE_FETCH_BARRIER_BIT       0x00000008
#endif
#ifndef GL_TESS_CONTROL_SHADER
#define GL_TESS_CONTROL_SHADER             0x8E88
#endif
#ifndef GL_TESS_EVALUATION_SHADER
#define GL_TESS_EVALUATION_SHADER          0x8E87
#endif
#ifndef GL_GEOMETRY_SHADER
#define GL_GEOMETRY_SHADER                 0x8DD9
#endif
#ifndef GL_PATCH_VERTICES
#define GL_PATCH_VERTICES                  0x8E72
#endif
#ifndef GL_COMPUTE_SHADER
#define GL_COMPUTE_SHADER                  0x91B9
#endif
#ifndef GL_SHADER_STORAGE_BUFFER
#define GL_SHADER_STORAGE_BUFFER           0x90D2
#endif
#ifndef GL_WRITE_ONLY
#define GL_WRITE_ONLY                      0x88B9
#endif
#ifndef GL_SHADER_IMAGE_ACCESS_BARRIER_BIT
#define GL_SHADER_IMAGE_ACCESS_BARRIER_BIT 0x00000020
#endif
#ifndef GL_BUFFER_UPDATE_BARRIER_BIT
#define GL_BUFFER_UPDATE_BARRIER_BIT       0x00000200
#endif
#ifndef GL_READ_ONLY
#define GL_READ_ONLY                       0x88B8
#endif
#ifndef GL_SHADER_STORAGE_BARRIER_BIT
#define GL_SHADER_STORAGE_BARRIER_BIT      0x2000
#endif

#ifdef __cplusplus
}
#endif
#endif
