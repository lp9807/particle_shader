#ifdef __APPLE__
#include <OpenGL/gl3.h>
#define __gl_h_
#include <OpenGL/glu.h>
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif

#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>

std::string readFile(const char *filePath) {
    std::string content;
    std::ifstream fileStream(filePath, std::ios::in);

    if(!fileStream.is_open()) {
        std::cerr << "Could not read file " << filePath << ". File does not exist." << std::endl;
        return "";
    }

    std::string line = "";
    while(!fileStream.eof()) {
        std::getline(fileStream, line);
        content.append(line + "\n");
    }

    fileStream.close();
    return content;
}


GLuint LoadShader(const char *vertex_path, const char *geom_path, const char *fragment_path) 
{
    GLuint vertShader = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragShader = glCreateShader(GL_FRAGMENT_SHADER);
    GLuint geomShader = glCreateShader(GL_GEOMETRY_SHADER);

    // Read shaders
    std::string vertShaderStr = readFile(vertex_path);
    std::string fragShaderStr = readFile(fragment_path);
    std::string geomShaderStr = readFile(geom_path);
    const char *vertShaderSrc = vertShaderStr.c_str();
    const char *fragShaderSrc = fragShaderStr.c_str();
    const char *geomShaderSrc = geomShaderStr.c_str();

    GLint result = GL_FALSE;
    int logLength;

    // Compile vertex shader
    std::cout << "Compiling vertex shader." << std::endl;
    glShaderSource(vertShader, 1, &vertShaderSrc, NULL);
    glCompileShader(vertShader);

    // Check vertex shader
    glGetShaderiv(vertShader, GL_COMPILE_STATUS, &result);
    glGetShaderiv(vertShader, GL_INFO_LOG_LENGTH, &logLength);
    std::vector<GLchar> vertShaderError((logLength > 1) ? logLength : 1);
    glGetShaderInfoLog(vertShader, logLength, NULL, &vertShaderError[0]);
    std::cout << &vertShaderError[0] << std::endl;

    // Compile fragment shader
    std::cout << "Compiling fragment shader." << std::endl;
    glShaderSource(fragShader, 1, &fragShaderSrc, NULL);
    glCompileShader(fragShader);

    // Check fragment shader
    glGetShaderiv(fragShader, GL_COMPILE_STATUS, &result);
    glGetShaderiv(fragShader, GL_INFO_LOG_LENGTH, &logLength);
    std::vector<GLchar> fragShaderError((logLength > 1) ? logLength : 1);
    glGetShaderInfoLog(fragShader, logLength, NULL, &fragShaderError[0]);
    std::cout << &fragShaderError[0] << std::endl;

    // Compile geometry shader
    std::cout << "Compiling geometry shader." << std::endl;
    glShaderSource(geomShader, 1, &geomShaderSrc, NULL);
    glCompileShader(geomShader);

    // Check geometry shader
    glGetShaderiv(geomShader, GL_COMPILE_STATUS, &result);
    glGetShaderiv(geomShader, GL_INFO_LOG_LENGTH, &logLength);
    std::vector<GLchar> geomShaderError((logLength > 1)? logLength: 1);
    glGetShaderInfoLog(geomShader, logLength, NULL, &geomShaderError[0]);
    std::cout << &geomShaderError[0] << std::endl;

    std::cout << "Linking program" << std::endl;
    GLuint program = glCreateProgram();
    glAttachShader(program, vertShader);
    glAttachShader(program, fragShader);
    glAttachShader(program, geomShader);
    glLinkProgram(program);

    glGetProgramiv(program, GL_LINK_STATUS, &result);
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);
    std::vector<char> programError( (logLength > 1) ? logLength : 1 );
    glGetProgramInfoLog(program, logLength, NULL, &programError[0]);
    std::cout << &programError[0] << std::endl;
  
    glDeleteShader(vertShader);
    glDeleteShader(fragShader);
    glDeleteShader(geomShader);

    return program;
}

void display()
{
   //glClearColor(1.0f, 0.0f, 0.0f, 0.0f);
   glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

   //GLint program = LoadShader("vertex.glsl","frag.glsl","geom.glsl");
   //glUseProgram( program );

   const GLfloat vertices[] = {-1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f, -1.0f, 1.0f, 1.0f,
                               1.0f, 1.0f, -1.0f, -1.0f, -1.0f,-1.0f, -1.0f,1.0f, -1.0f,
                               1.0f, -1.0f, 1.0f, -1.0f, -1.0f,-1.0f,1.0f,-1.0f,-1.0f};
   const GLfloat vertice_colors[] = {0.583f, 0.771f, 0.014f, 0.609f, 0.115f, 0.436f, 0.327f, 0.483f, 0.844f,
                                    0.822f, 0.569f, 0.201f, 0.435f, 0.602f, 0.223f, 0.310f, 0.747f, 0.185f,
                                    0.597f, 0.770f, 0.761f, 0.559f, 0.436f, 0.730f, 0.359f, 0.583f, 0.152f};
   // set uniform variables if any

   // draw elements
   GLuint vaoId, vboId;
   
   glGenVertexArrays(1, &vaoId);
   glBindVertexArray(vaoId);

   glGenBuffers(1, &vboId);
   glBindBuffer(GL_ARRAY_BUFFER, vboId);
   glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

   glEnableVertexAttribArray(vaoId);
   glBindBuffer(GL_ARRAY_BUFFER, vboId); //?
   glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
   glDrawArrays(GL_TRIANGLES, 0, 3);
   glDisableVertexAttribArray(vaoId);

   glFlush();

   /*glBegin(GL_TRIANGLES);
     glVertex3f(1.0f, 1.0f, 0.0f);
     glVertex3f(-1.0f, -1.0f, 0.0f);
     glVertex3f(-0.75f, 0.25f, 0.0f);
     glVertex3f();
   glEnd();*/

   //glUseProgram(0);

   glutSwapBuffers();
}

int main(int argc, char** argv)
{
   glutInit(&argc, argv);
   glutInitDisplayMode(GLUT_3_2_CORE_PROFILE | GLUT_DOUBLE | GLUT_RGBA);
   glutInitWindowSize(640,480);
   glutInitWindowPosition(100,100);
   glutCreateWindow("Geometry Shader");

   const GLubyte* renderer = glGetString(GL_RENDERER);
   const GLubyte* version = glGetString(GL_VERSION);
   printf("Renderer: %s\n", renderer);
   printf("OpenGL version supported: %s\n", version);

   glutDisplayFunc(display);
   glutMainLoop();

   return 0;
}
