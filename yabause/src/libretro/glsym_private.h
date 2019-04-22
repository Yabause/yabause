#ifndef RGLGEN_PRIV_DECL_H__
#define RGLGEN_PRIV_DECL_H__
#ifdef __cplusplus
extern "C" {
#endif

#if defined(HAVE_GLES)
typedef double GLclampd;
typedef double GLdouble;
typedef void (GL_APIENTRYP RGLSYMGLMEMORYBARRIERPROC) (GLbitfield barriers);
typedef void (GL_APIENTRYP RGLSYMGLBINDFRAGDATALOCATIONPROC) (GLuint program, GLuint color, const GLchar *name);

#define glMemoryBarrier __rglgen_glMemoryBarrier
#define glBindFragDataLocation __rglgen_glBindFragDataLocation

#define GL_PATCHES                        0x000E
#define GL_FRAMEBUFFER_BARRIER_BIT        0x00000400
#define GL_TEXTURE_UPDATE_BARRIER_BIT     0x00000100
#define GL_TEXTURE_FETCH_BARRIER_BIT      0x00000008
#define GL_TESS_CONTROL_SHADER            0x8E88
#define GL_TESS_EVALUATION_SHADER         0x8E87
#define GL_GEOMETRY_SHADER                0x8DD9

extern RGLSYMGLMEMORYBARRIERPROC __rglgen_glMemoryBarrier;
extern RGLSYMGLBINDFRAGDATALOCATIONPROC __rglgen_glBindFragDataLocation;
#elif !defined(_OGLES3_)
typedef void (APIENTRYP RGLSYMGLTEXTUREBARRIERNVPROC) (void);

#define glTextureBarrierNV __rglgen_glTextureBarrierNV

extern RGLSYMGLTEXTUREBARRIERNVPROC __rglgen_glTextureBarrierNV;
#endif

#ifdef __cplusplus
}
#endif
#endif
