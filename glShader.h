#ifndef GLSHADER_H
#define GLSHADER_H

#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#include <GLUT/GLUT.h>

GLuint LoadShader(const char *vertex_path, const char *fragment_path);

#endif
