#ifdef __APPLE__
#include <OpenGL/gl3.h>
#define __gl_h_
#include <OpenGL/glu.h>
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <algorithm>
#include <cmath>
#include <chrono>

#define TEX_WIDTH 640
#define TEX_HEIGHT 480
#define TEX_DEPTH 40

struct geomData
{
   //std::vector<GLuint> vboIds;

    GLuint vaoId;
    GLuint fboId;
    GLuint velTexIds[2];  // velocity0+1
    GLuint presTexIds[2];  // [p]ressure0+1
    GLuint divTexId;      // divergence 
    //TODO: GLuint extraTexIds[3]; // ? phi, phi_n_hat, phi_n_1_hat
};

struct simTexData
{
  GLuint inputFboId;
  std::vector<GLuint> inputTexIds;
  std::vector<std::string> inputTexNames;
  std::vector<GLuint> outputTexIds;
  std::string fragShader;
  std::map<std::string, GLfloat> uniforms;
};

std::chrono::system_clock::duration deltaT;
std::chrono::system_clock::time_point lastT;

geomData gData;
int currVelID = 0, resultVelID = 1;
int currPresID = 0, resultPresID = 1;

double count = 0, angle = 0;
glm::vec2 viewport(640,480);
glm::mat4 window_invert_mvp;
glm::vec3 force_point( 0, 0, 0 );

std::string readFile(const char *filePath) 
{
    if( !filePath ) return std::string();

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

const char *dlGetErrorString( int error )
{
    switch ( error )
    {
  case GL_INVALID_ENUM    : return "GL_INVALID_ENUM";
  case GL_INVALID_VALUE     : return "GL_INVALID_VALUE";
  case GL_INVALID_OPERATION   : return "GL_INVALID_OPERATION";
  //case GL_STACK_OVERFLOW    : return "GL_STACK_OVERFLOW";
  //case GL_STACK_UNDERFLOW   : return "GL_STACK_UNDERFLOW";
  case GL_OUT_OF_MEMORY     : return "GL_OUT_OF_MEMORY";
  //case GL_TABLE_TOO_LARGE       : return "GL_TABLE_TOO_LARGE";
  case GL_INVALID_FRAMEBUFFER_OPERATION : return "GL_INVALID_FRAMEBUFFER_OPERATION";
  case GL_FRAMEBUFFER_COMPLETE: return "GL_FRAMEBUFFER_COMPLETE";
#ifdef GL_TEXTURE_TOO_LARGE_EXT
  case GL_TEXTURE_TOO_LARGE_EXT   : return "GL_TEXTURE_TOO_LARGE_EXT";
#endif
  case GL_NO_ERROR                : return "GL_NO_ERROR";
  default       : return "UNKNOWN GL ERROR";
    }
}

const char *dlGetProvokingMode( int error )
{
    switch( error )
    {
        case GL_FIRST_VERTEX_CONVENTION: return "GL_FIRST_VERTEX_CONVENTION";
        case GL_PROVOKING_VERTEX: return "GL_PROVOKING_VERTEX";
        case GL_LAST_VERTEX_CONVENTION: return "GL_LAST_VERTEX_CONVENTION";                          
        case GL_UNDEFINED_VERTEX: return "GL_UNDEFINED_VERTEX";
        default : return "UNKNOWN GL ERROR";
    }
}

GLuint LoadShader(const char *vertex_path, const char *fragment_path, const char *geom_path) 
{
    // Read shaders
    std::string vertShaderStr = readFile(vertex_path);
    std::string fragShaderStr = readFile(fragment_path);
    std::string geomShaderStr = readFile(geom_path);

    const char *vertShaderSrc = vertShaderStr.c_str();
    const char *fragShaderSrc = fragShaderStr.c_str();
    const char *geomShaderSrc = geomShaderStr.c_str();

    GLint result = GL_FALSE;
    int logLength;

    GLuint vertShader = -1;
    GLuint fragShader = -1;
    GLuint geomShader = -1;

    if( !vertShaderStr.empty() )
    {
      vertShader = glCreateShader(GL_VERTEX_SHADER);
      // Compile vertex shader
      std::cout << "Compiling vertex shader:" << vertex_path << std::endl;
      glShaderSource(vertShader, 1, &vertShaderSrc, NULL);
      glCompileShader(vertShader);

      // Check vertex shader
      glGetShaderiv(vertShader, GL_COMPILE_STATUS, &result);
      glGetShaderiv(vertShader, GL_INFO_LOG_LENGTH, &logLength);
      std::vector<GLchar> vertShaderError((logLength > 1) ? logLength : 1);
      glGetShaderInfoLog(vertShader, logLength, NULL, &vertShaderError[0]);
      std::cout << &vertShaderError[0] << std::endl;
    }

    if( !fragShaderStr.empty() )
    {
      fragShader = glCreateShader(GL_FRAGMENT_SHADER);
      // Compile fragment shader
      std::cout << "Compiling fragment shader:" << fragment_path << std::endl;
      glShaderSource(fragShader, 1, &fragShaderSrc, NULL);
      glCompileShader(fragShader);

      // Check fragment shader
      glGetShaderiv(fragShader, GL_COMPILE_STATUS, &result);
      glGetShaderiv(fragShader, GL_INFO_LOG_LENGTH, &logLength);
      std::vector<GLchar> fragShaderError((logLength > 1) ? logLength : 1);
      glGetShaderInfoLog(fragShader, logLength, NULL, &fragShaderError[0]);
      std::cout << &fragShaderError[0] << std::endl;
    }

    if( !geomShaderStr.empty() )
    {
       geomShader = glCreateShader(GL_GEOMETRY_SHADER);
      // Compile geometry shader
      std::cout << "Compiling geometry shader:" << geom_path << std::endl;
      glShaderSource(geomShader, 1, &geomShaderSrc, NULL);
      glCompileShader(geomShader);

      // Check geometry shader
      glGetShaderiv(geomShader, GL_COMPILE_STATUS, &result);
      glGetShaderiv(geomShader, GL_INFO_LOG_LENGTH, &logLength);
      std::vector<GLchar> geomShaderError((logLength > 1)? logLength: 1);
      glGetShaderInfoLog(geomShader, logLength, NULL, &geomShaderError[0]);
      std::cout << &geomShaderError[0] << std::endl;
    }

    std::cout << "Linking program" << std::endl;
    GLuint program = glCreateProgram();
    if( vertShader != -1 ) glAttachShader(program, vertShader);
    if( fragShader != -1 ) glAttachShader(program, fragShader);
    if( geomShader != -1 ) glAttachShader(program, geomShader);

    glLinkProgram(program);

    glGetProgramiv(program, GL_LINK_STATUS, &result);
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);
    std::vector<char> programError( (logLength > 1) ? logLength : 1 );
    glGetProgramInfoLog(program, logLength, NULL, &programError[0]);
    std::cout << &programError[0] << std::endl;
  
    if(vertShader != -1) glDeleteShader(vertShader);
    if(fragShader != -1) glDeleteShader(fragShader);
    if(geomShader != -1) glDeleteShader(geomShader);

    return program;
}

void bindInputTexture( const simTexData& texData )
{
     // bind input texture
   for( int id = 0; id < texData.inputTexIds.size(); id++ )
   {
       glActiveTexture(GL_TEXTURE0 + id);
       glBindTexture(GL_TEXTURE_3D, texData.inputTexIds[id]);
   }

   if( texData.inputTexIds.empty() )
   {
      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_3D, 0);
   }
}

void setupUnifom( GLint program, const simTexData& texData )
{
     // set uniform
   GLuint loc = -1;
   for( auto& uniform : texData.uniforms ) {
     loc = glGetUniformLocation(program, uniform.first.c_str());
     if( loc != -1 ) {
       glUniform1f(loc, uniform.second);
     }
   }

   for( auto i = 0; i < texData.inputTexIds.size(); i++ )
   {
     loc = glGetUniformLocation(program, texData.inputTexNames[i].c_str());
     if( loc != -1 ) {
      glUniform1i(loc, i);
     }
   }

   GLuint forcePointID = glGetUniformLocation(program, "forcepoint");
   if( forcePointID != -1 )
   {
     glUniform3fv(forcePointID, 1, glm::value_ptr(force_point));
     force_point = glm::vec3(0.0);
   }
}

void drawToTexture( const simTexData& texData )
{
   GLfloat quads[] = {
    -1.0f, -1.0f, 0.0f,
    -1.0f, 1.0f, 0.0f,
    1.0f, -1.0f, 0.0f,
    -1.0f, 1.0f, 0.0f,
    1.0f, -1.0f, 0.0f,
    1.0f, 1.0f, 0.0f,
   };

  const GLfloat quad_colors[] = { 0.583f, 0.771f, 0.014f, 
                                  0.609f, 0.115f, 0.436f, 
                                  0.327f, 0.483f, 0.844f,
                                  0.822f, 0.569f, 0.201f, 
                                  0.435f, 0.602f, 0.223f, 
                                  0.310f, 0.747f, 0.185f,};

  GLfloat quad_uvs[] = {
    0.0f, 0.0f,
    0.0f, 1.0f,
    1.0f, 0.0f,
    0.0f, 1.0f,
    1.0f, 0.0f,
    1.0f, 1.0f };

   printf("quad size: %lu, %lu\n", sizeof(quads), sizeof(GLfloat) );
   
   GLenum error;

   // define VBO
   GLuint vboId[3];
   glGenBuffers(3, vboId);

   glBindBuffer(GL_ARRAY_BUFFER, vboId[0]);
   glBufferData(GL_ARRAY_BUFFER, sizeof(quads), quads, GL_STATIC_DRAW);
   // 1st parameter: attribute index, corresponding to layout number in shader
   glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
   glEnableVertexAttribArray(0);

   glBindBuffer(GL_ARRAY_BUFFER, vboId[1]);
   glBufferData(GL_ARRAY_BUFFER, sizeof(quad_colors), quad_colors, GL_STATIC_DRAW);
   glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
   glEnableVertexAttribArray(1);

   glBindBuffer(GL_ARRAY_BUFFER, vboId[2]);
   glBufferData(GL_ARRAY_BUFFER, sizeof(quad_uvs), quad_uvs, GL_STATIC_DRAW);
   glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
   glEnableVertexAttribArray(2);

   glBindFramebuffer(GL_FRAMEBUFFER, texData.inputFboId);
   glViewport( 0.0, 0.0, TEX_WIDTH, TEX_HEIGHT );

   std::vector<GLenum> attachments;
   attachments.reserve(texData.outputTexIds.size());

   // define the attachment to simulation Texture 
   for( int id = 0; id < texData.outputTexIds.size(); id++ )
   {
      glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0+id, texData.outputTexIds[id], 0 );
      attachments.push_back(GLenum(GL_COLOR_ATTACHMENT0+id));
   }
   glDrawBuffers(attachments.size(), attachments.data());

   error = glCheckFramebufferStatus(GL_FRAMEBUFFER);
   printf("glCheckFramebufferStatus: %s\n", dlGetErrorString(error) );

   // bind input data
   bindInputTexture( texData );

   // set uniform variables if any
   GLint program = LoadShader("vertex.glsl",texData.fragShader.c_str(),"geom.glsl");
   glUseProgram( program );

   setupUnifom( program, texData );

   // clear FBO
   //glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
   //glClear(GL_COLOR_BUFFER_BIT);

   // draw elements
   glDrawArrays(GL_TRIANGLES, 0, 6);
   
   // GL3 requires shader anyway.
   error = glGetError();
   printf("glGetError after glDrawArrays: %s\n", dlGetErrorString(error) );
   
   glBindFramebuffer(GL_FRAMEBUFFER, 0);
   glUseProgram(0);

   glDisableVertexAttribArray(0);
   glDisableVertexAttribArray(1);
   glDisableVertexAttribArray(2);

   glDeleteBuffers(3, vboId);
}

void drawToScreen( const simTexData& texData )
{
   static const GLfloat quads[] = {
    -1.0f, -1.0f, 0.0f,
    -1.0f, 1.0f, 0.0f,
    1.0f, -1.0f, 0.0f,
    -1.0f, 1.0f, 0.0f,
    1.0f, -1.0f, 0.0f,
    1.0f, 1.0f, 0.0f
   };

   static const GLfloat quad_uvs[] = {
    0.0f, 0.0f,
    0.0f, 1.0f,
    1.0f, 0.0f,
    0.0f, 1.0f,
    1.0f, 0.0f,
    1.0f, 1.0f 
   };

   static const GLfloat cube_vertices[] = {
    // front
    -1.0, -1.0,  1.0,
     1.0, -1.0,  1.0,
     1.0,  1.0,  1.0,
    -1.0,  1.0,  1.0,
    // back
    -1.0, -1.0, -1.0,
     1.0, -1.0, -1.0,
     1.0,  1.0, -1.0,
    -1.0,  1.0, -1.0,
   };

   static const GLushort cube_elements[] = {
    // front
    0, 1, 2,
    2, 3, 0,
    // right
    1, 5, 6,
    6, 2, 1,
    // back
    7, 6, 5,
    5, 4, 7,
    // left
    4, 0, 3,
    3, 7, 4,
    // bottom
    4, 5, 1,
    1, 0, 4,
    // top
    3, 2, 6,
    6, 7, 3,
   };

   static const GLfloat cube_colors[] = {
    // front colors
    1.0, 0.0, 0.0,
    0.0, 1.0, 0.0,
    0.0, 0.0, 1.0,
    1.0, 1.0, 1.0,
    // back colors
    1.0, 0.0, 0.0,
    0.0, 1.0, 0.0,
    0.0, 0.0, 1.0,
    1.0, 1.0, 1.0,
   };

   GLenum error;

   // define VBO
   GLuint vboId[2];
   glGenBuffers(2, vboId);

   // vertex
   glBindBuffer(GL_ARRAY_BUFFER, vboId[0]);
   glBufferData(GL_ARRAY_BUFFER, sizeof(cube_vertices), cube_vertices, GL_STATIC_DRAW );
   glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
   glEnableVertexAttribArray(0);

   // color
   glBindBuffer(GL_ARRAY_BUFFER, vboId[1]);
   glBufferData(GL_ARRAY_BUFFER, sizeof(cube_colors), cube_colors, GL_STATIC_DRAW);
   glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
   glEnableVertexAttribArray(1);

   // define IBO
   GLuint iboId;
   glGenBuffers(1, &iboId);
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, iboId);
   glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cube_elements), cube_elements, GL_STATIC_DRAW );

   // bind texture
   bindInputTexture( texData );

   error = glGetError();
   printf("glGetError: %s\n", dlGetErrorString(error) );

   // load shader
   GLint program = LoadShader("vertex_screen.glsl",texData.fragShader.c_str(),nullptr);
   glUseProgram( program );

   // matrix
   glm::mat4 projection = glm::perspective( glm::radians(90.0f), (float)viewport[0]/viewport[1], 0.1f, 100.0f);
   glm::vec3 eyePos = glm::vec3( 2.0*cos(angle), 0.0, 2.0f*sin(angle) );
   glm::mat4 view = glm::lookAt( eyePos, glm::vec3( 0, 0, 0 ), glm::vec3( 0, 1, 0) );
   glm::mat4 model = glm::mat4(1.0f);
   glm::mat4 mvp = projection * view * model;
   glm::mat4 invertMvp = glm::inverse(mvp);

   window_invert_mvp = invertMvp;

   // setup matrix uniform
   GLuint eyePosID = glGetUniformLocation(program, "eyePos");
   glUniform3fv(eyePosID, 1, glm::value_ptr(eyePos));
   GLuint viewportID = glGetUniformLocation(program, "viewport");
   glUniform2fv(viewportID, 1, glm::value_ptr(viewport));
   GLuint MatrixID = glGetUniformLocation(program, "MVP");
   glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &mvp[0][0]);
   GLuint invertMatrixID = glGetUniformLocation(program, "invertMVP");
   glUniformMatrix4fv(invertMatrixID, 1, GL_FALSE, &invertMvp[0][0]);

   setupUnifom( program, texData );

   glEnable(GL_CULL_FACE );
   glEnable(GL_DEPTH_TEST);
   glDepthFunc(GL_LESS);
   glViewport(0, 0, viewport[0], viewport[1]);

   error = glGetError();
   printf("glGetError: %s\n", dlGetErrorString(error) );

   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

   glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_SHORT, 0);

   error = glGetError();
   printf("glGetError: %s\n", dlGetErrorString(error) );
   glUseProgram(0);

   glDisable(GL_CULL_FACE);
   glDisable(GL_DEPTH_TEST);

   glutSwapBuffers();

   glDisableVertexAttribArray(0);
   glDisableVertexAttribArray(1);
   glDeleteBuffers(2, vboId);
}

void initGeomData()
{
   const GLfloat vertices[] = {-1.0f, -1.0f, 0.0f, 
                                1.0f, -1.0f, 0.0f, 
                                0.0f, 1.0f, 0.0f }; /*{-1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f, -1.0f, 1.0f, 1.0f,
                               1.0f, 1.0f, -1.0f, -1.0f, -1.0f,-1.0f, -1.0f,1.0f, -1.0f,
                               1.0f, -1.0f, 1.0f, -1.0f, -1.0f,-1.0f,1.0f,-1.0f,-1.0f};*/
   const GLfloat vertice_colors[] = {0.583f, 0.771f, 0.014f, 
                                     0.609f, 0.115f, 0.436f, 
                                     0.327f, 0.483f, 0.844f};
   /*const GLfloat vertice_colors[] = {0.583f, 0.771f, 0.014f, 0.609f, 0.115f, 0.436f, 0.327f, 0.483f, 0.844f,
                                    0.822f, 0.569f, 0.201f, 0.435f, 0.602f, 0.223f, 0.310f, 0.747f, 0.185f,
                                    0.597f, 0.770f, 0.761f, 0.559f, 0.436f, 0.730f, 0.359f, 0.583f, 0.152f};*/


   GLuint vboId[2];
   glGenBuffers(2, vboId);

   glBindBuffer(GL_ARRAY_BUFFER, vboId[0]);
   glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
   // 1st parameter: attribute index, corresponding to layout number in shader
   glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
   glEnableVertexAttribArray(0);

   glBindBuffer(GL_ARRAY_BUFFER, vboId[1]);
   glBufferData(GL_ARRAY_BUFFER, sizeof(vertice_colors), vertice_colors, GL_STATIC_DRAW);
   glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
   glEnableVertexAttribArray(1);
}

void initialize()
{
  // default VAO for gl3 core-profile.
  glGenVertexArrays(1, &gData.vaoId);
  glBindVertexArray(gData.vaoId);

  // define FBO
  glGenFramebuffers(1, &gData.fboId);

  // all textures: input & output
  glGenTextures(2, gData.velTexIds);
  glGenTextures(2, gData.presTexIds);
  glGenTextures(1, &gData.divTexId);
  for( int i = 0; i < 2; i++) 
  {
    glBindTexture(GL_TEXTURE_3D, gData.velTexIds[i]);
    glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA, TEX_WIDTH, TEX_HEIGHT, TEX_DEPTH, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    glBindTexture(GL_TEXTURE_3D, gData.presTexIds[i]);
    glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA, TEX_WIDTH, TEX_HEIGHT, TEX_DEPTH, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  }
  
  // temp divergence tex
  glBindTexture(GL_TEXTURE_3D, gData.divTexId);
  glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA, TEX_WIDTH, TEX_HEIGHT, TEX_DEPTH, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

  glBindTexture(GL_TEXTURE_3D, 0);

  //init state of velocity and pressure texture
  simTexData initData;

  initData.uniforms["texWidth"] = TEX_WIDTH;
  initData.uniforms["texHeight"] = TEX_HEIGHT;
  initData.uniforms["texDepth"] = TEX_DEPTH;

  initData.inputFboId = gData.fboId;
  initData.inputTexIds = {};
  initData.outputTexIds = {gData.velTexIds[0], gData.presTexIds[0]};
  initData.fragShader = "frag_init_all.glsl";
  drawToTexture( initData );
}

void simulate()
{
  simTexData texData;
  texData.uniforms["texWidth"] = TEX_WIDTH;
  texData.uniforms["texHeight"] = TEX_HEIGHT;
  texData.uniforms["texDepth"] = TEX_DEPTH;
  
  {
    texData.uniforms["currTime"] = count;
    //std::chrono::duration_cast<std::chrono::seconds>(deltaT).count();

    // 1. advect: 
    // input: velocity
    // output: intermediate velocity
    texData.inputFboId = gData.fboId;
    texData.inputTexIds = { gData.velTexIds[currVelID] };
    texData.inputTexNames = { "velocity" };
    texData.outputTexIds = { gData.velTexIds[resultVelID] };
    texData.fragShader = "frag_pass1_advect.glsl";
    drawToTexture( texData );

    // 2. divergence: 
    // input: intermediate velocity
    // output: intermediate divergence
    texData.inputFboId = gData.fboId;
    texData.inputTexIds = { gData.velTexIds[resultVelID] };
    texData.inputTexNames = { "velocity" };
    texData.outputTexIds = { gData.divTexId };
    texData.fragShader = "frag_pass2_divergence.glsl";
    drawToTexture( texData );

    // to add: apply force

    for( int i = 0; i <3; i++ )
    {
      // 3. diffuse
      // input: pressure & intermediate divergence
      // output: updated pressure
      texData.inputFboId = gData.fboId;
      texData.inputTexIds = { gData.presTexIds[currPresID], gData.divTexId };
      texData.inputTexNames = { "pressure", "divergence" };
      texData.outputTexIds = { gData.presTexIds[resultPresID] };
      texData.uniforms["rAlpha"] = 1.0/count;
      texData.uniforms["rBeta"] = 1.0f/(4+texData.uniforms["rAlpha"]);
      texData.fragShader = "frag_pass3_diffuse.glsl";
      drawToTexture( texData );

      currPresID = resultPresID;
      resultPresID = (1-currPresID);
    }

    // 4. projection: 
    // input: intermediate velocity & pressure
    // output: final velocity
    texData.inputFboId = gData.fboId;
    texData.inputTexIds = { gData.velTexIds[resultVelID], gData.presTexIds[currPresID] };
    texData.inputTexNames = { "velocity", "pressure" };
    texData.outputTexIds = { gData.velTexIds[currVelID] };
    texData.fragShader = "frag_pass4_proj.glsl";
    drawToTexture( texData );

    // ray march to draw 3D texture
    texData.inputTexIds = { gData.velTexIds[currVelID], gData.presTexIds[currPresID] };
    texData.inputTexNames = { "velocity", "pressure" };
    texData.outputTexIds = {};
    texData.fragShader = "frag_screen.glsl";
    texData.uniforms["absorption"] = 0.4;
    drawToScreen( texData );
    
    // swap texture
    currPresID = resultPresID;
    resultPresID = (1-currPresID);
  }

  /*glDeleteTextures(2, velTexIds);
  glDeleteTextures(2, pdTexIds);
  glDeleteFramebuffers(1, &fboId);
  glDeleteVertexArrays(1, &vaoId);*/

  count += 0.001f;
}

void idle(void)
{
    auto nowT = std::chrono::system_clock::now();
    deltaT = nowT - lastT;
    //lastT = nowT;
    //count += 0.0000001f;

    //glutPostRedisplay();
}

void timer(int t)
{
    glutTimerFunc( 10, timer, 0 );
    angle += 0.01;
    glutPostRedisplay();
}

void reshape( int screen_width, int screen_height )
{
    viewport = glm::vec2( screen_width, screen_height );
    glViewport(0, 0, screen_width, screen_height);
}

void click( int button, int state, int x, int y )
{
    if( button == GLUT_LEFT_BUTTON && state == GLUT_DOWN )
    {
      glm::vec3 worldPos = window_invert_mvp * glm::vec4( x/viewport.x, y/viewport.y, 0.0, 0.0 );
      if( glm::all( glm::greaterThan(worldPos, glm::vec3(-1,-1,-1)) ) &&
          glm::all( glm::lessThan(worldPos, glm::vec3(1,1,1)) ) )
      {
         force_point = (worldPos + 1.0f)/2.0f;
      }
    }
}

int main(int argc, char** argv)
{
   glutInit(&argc, argv);
   glutInitDisplayMode(GLUT_3_2_CORE_PROFILE | GLUT_DOUBLE | GLUT_RGBA);
   glutInitWindowSize(viewport[0],viewport[1]);
   glutInitWindowPosition((glutGet(GLUT_SCREEN_WIDTH)-viewport[0])/2,
                          (glutGet(GLUT_SCREEN_HEIGHT)-viewport[1])/2);
   int window = glutCreateWindow("Noise Sim");

   const GLubyte* renderer = glGetString(GL_RENDERER);
   const GLubyte* version = glGetString(GL_VERSION);
   const GLubyte* glsl_version = glGetString(GL_SHADING_LANGUAGE_VERSION);
   printf("Renderer: %s\n", renderer);
   printf("OpenGL version supported: %s\n", version);
   printf("GLSL version supported: %s\n", glsl_version);

   lastT = std::chrono::system_clock::now();

   GLint value;
   glGetIntegerv(GL_MAX_3D_TEXTURE_SIZE, &value);
   printf("max 3D texture size: %d\n", value);
   glGetIntegerv(GL_LAYER_PROVOKING_VERTEX, &value);
   printf("layer provoking vertex: %s\n", dlGetProvokingMode(value));
   glGetIntegerv(GL_MAX_GEOMETRY_OUTPUT_VERTICES, &value);
   printf("max output vertices number : %d\n", value);

   initialize();
   glutDisplayFunc(simulate);
   glutIdleFunc(idle);
   glutTimerFunc( 10, timer, 0);
   glutReshapeFunc( reshape );
   glutMouseFunc( click );
   //glutPostRedisplay();

   glutMainLoop();

   return 0;
}