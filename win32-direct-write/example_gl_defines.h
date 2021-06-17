// DirectWrite rasterization example: opengl types and constants that we need that are not in GL\gl.h

typedef char GLchar;
typedef ptrdiff_t GLsizeiptr;
typedef ptrdiff_t GLintptr;

typedef void GLDEBUGPROC_Type(GLenum source,GLenum type,GLuint id,GLenum severity,GLsizei length,const GLchar *message,const void *userParam);
typedef GLDEBUGPROC_Type *GLDEBUGPROC;

#define GL_FUNC(N,R,P) typedef R N##_Type P; static N##_Type *N = 0;
#include "example_gl_funcs.h"

#define GL_CONSTANT_COLOR                 0x8001

#define GL_CLAMP_TO_EDGE                  0x812F

#define GL_FRAMEBUFFER_UNDEFINED          0x8219

#define GL_DEBUG_OUTPUT_SYNCHRONOUS       0x8242
#define GL_DEBUG_SEVERITY_NOTIFICATION    0x826B

#define GL_TEXTURE0                       0x84C0

#define GL_ARRAY_BUFFER                   0x8892

#define GL_DYNAMIC_DRAW                   0x88E8

#define GL_FRAGMENT_SHADER                0x8B30
#define GL_VERTEX_SHADER                  0x8B31

#define GL_TEXTURE_2D_ARRAY               0x8C1A

#define GL_SRGB                           0x8C40
#define GL_SRGB8                          0x8C41

#define GL_READ_FRAMEBUFFER               0x8CA8
#define GL_DRAW_FRAMEBUFFER               0x8CA9

#define GL_FRAMEBUFFER_COMPLETE                      0x8CD5
#define GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT         0x8CD6
#define GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT 0x8CD7
#define GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER        0x8CDB
#define GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER        0x8CDC
#define GL_FRAMEBUFFER_UNSUPPORTED                   0x8CDD

#define GL_COLOR_ATTACHMENT0              0x8CE0

#define GL_FRAMEBUFFER                    0x8D40

#define GL_FRAMEBUFFER_SRGB               0x8DB9

#define GL_DEBUG_SEVERITY_HIGH            0x9146
#define GL_DEBUG_SEVERITY_MEDIUM          0x9147
#define GL_DEBUG_SEVERITY_LOW             0x9148

