// DirectWrite rasterization example: opengl functions we need to manually load

GL_FUNC(glDebugMessageControl, void, (GLenum source, GLenum type, GLenum severity, GLsizei count, const GLuint *ids, GLboolean enabled))
GL_FUNC(glDebugMessageCallback, void, (GLDEBUGPROC callback, const void *userParam))

GL_FUNC(glGenFramebuffers, void, (GLsizei n, GLuint *framebuffers))
GL_FUNC(glBindFramebuffer, void, (GLenum target, GLuint framebuffer))
GL_FUNC(glFramebufferTexture2D, void, (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level))
GL_FUNC(glCheckFramebufferStatus, GLenum, (GLenum target))
GL_FUNC(glBlitFramebuffer, void, (GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter))

GL_FUNC(glTexImage3D, void, (GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const void *pixels));

GL_FUNC(glCreateProgram, GLuint, (void))
GL_FUNC(glCreateShader, GLuint, (GLenum type))
GL_FUNC(glCompileShader, void, (GLuint shader))
GL_FUNC(glAttachShader, void, (GLuint program, GLuint shader))
GL_FUNC(glLinkProgram, void, (GLuint program))
GL_FUNC(glShaderSource, void, (GLuint shader, GLsizei count, const GLchar *const*string, const GLint *length))
GL_FUNC(glUseProgram, void, (GLuint program))
GL_FUNC(glGetUniformLocation, GLint, (GLuint program, const GLchar *name))
GL_FUNC(glGetAttribLocation, GLint, (GLuint program, const GLchar *name))

GL_FUNC(glGenBuffers, void, (GLsizei n, GLuint *buffers))
GL_FUNC(glBindBuffer, void, (GLenum target, GLuint buffer))
GL_FUNC(glBufferData, void, (GLenum target, GLsizeiptr size, const void *data, GLenum usage))

GL_FUNC(glEnableVertexAttribArray, void, (GLuint index))
GL_FUNC(glVertexAttribPointer, void, (GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void *pointer))

GL_FUNC(glActiveTexture, void, (GLenum texture))

GL_FUNC(glUniform4f, void, (GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3))
GL_FUNC(glUniform1i, void, (GLint location, GLint v0))
GL_FUNC(glUniform1fv, void, (GLint location, GLsizei count, const GLfloat *value))
GL_FUNC(glUniformMatrix3fv, void, (GLint location, GLsizei count, GLboolean transpose, const GLfloat *value))

GL_FUNC(glGenVertexArrays, void, (GLsizei n, GLuint *arrays))
GL_FUNC(glBindVertexArray, void, (GLuint array))

GL_FUNC(glBlendColor, void, (GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha))

#undef GL_FUNC