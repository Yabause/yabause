FIND_PATH(EGL_INCLUDE_DIR EGL/egl.h
      /usr/openwin/share/include
      /opt/graphics/OpenGL/include /usr/X11R6/include
      /usr/include
      /opt/vc/include
    )

FIND_LIBRARY(EGL_gl_LIBRARY
      NAMES EGL
      PATHS /opt/graphics/OpenGL/lib
            /usr/openwin/lib
            /usr/shlib /usr/X11R6/lib
            /usr/lib
            /opt/vc/lib
    )

SET( EGL_FOUND "NO" )
IF(EGL_gl_LIBRARY)

    SET( EGL_LIBRARIES ${EGL_gl_LIBRARY} ${EGL_LIBRARIES})

    SET( EGL_FOUND "YES" )

ENDIF(EGL_gl_LIBRARY)

MARK_AS_ADVANCED(
  EGL_INCLUDE_DIR
  EGL_gl_LIBRARY
)
