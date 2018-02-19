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
#include <map>
#include <algorithm>
#include <cmath>

#define TEX_WIDTH 640
#define TEX_HEIGHT 480
#define TEX_DEPTH 100

struct geomData
{
   std::vector<GLuint> vboIds;
};

struct simTexData
{
  std::vector<GLuint> inputTexIds;
  std::vector<std::string> inputTexNames;
  std::vector<GLuint> outputTexIds;
  std::string fragShader;
  std::map<std::string, GLfloat> uniforms;
};

std::string readFile(const char *filePath) {
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

   // define FBO
   GLuint fboId;
   glGenFramebuffers(1, &fboId);
   glBindFramebuffer(GL_FRAMEBUFFER, fboId);

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

   // clear FBO
   glClearColor(1.0f, 0.0f, 0.0f, 0.0f);
   glClear(GL_COLOR_BUFFER_BIT);

   // set uniform variables if any
   GLint program = LoadShader("vertex.glsl",texData.fragShader.c_str(),"geom.glsl");
   glUseProgram( program );

   setupUnifom( program, texData );

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
   glDeleteFramebuffers(1, &fboId);
}

void drawToScreen( const simTexData& texData )
{
   GLfloat quads[] = {
    -1.0f, -1.0f, 0.0f,
    -1.0f, 1.0f, 0.0f,
    1.0f, -1.0f, 0.0f,
    -1.0f, 1.0f, 0.0f,
    1.0f, -1.0f, 0.0f,
    1.0f, 1.0f, 0.0f,
   };

   GLfloat quad_uvs[] = {
    0.0f, 0.0f,
    0.0f, 1.0f,
    1.0f, 0.0f,
    0.0f, 1.0f,
    1.0f, 0.0f,
    1.0f, 1.0f };

    GLenum error;

   // define VBO
   GLuint vboId[2];
   glGenBuffers(2, vboId);

   // vertex
   glBindBuffer(GL_ARRAY_BUFFER, vboId[0]);
   glBufferData(GL_ARRAY_BUFFER, sizeof(quads), quads, GL_STATIC_DRAW );
   glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
   glEnableVertexAttribArray(0);

   // UV
   glBindBuffer(GL_ARRAY_BUFFER, vboId[1]);
   glBufferData(GL_ARRAY_BUFFER, sizeof(quad_uvs), quad_uvs, GL_STATIC_DRAW);
   glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
   glEnableVertexAttribArray(1);

   // bind texture
   bindInputTexture( texData );

   error = glGetError();
   printf("glGetError: %s\n", dlGetErrorString(error) );

   glClear(GL_COLOR_BUFFER_BIT);

   // load shader
   GLint program = LoadShader("vertex_screen.glsl",texData.fragShader.c_str(),NULL);
   glUseProgram( program );

   setupUnifom( program, texData );

   error = glGetError();
   printf("glGetError: %s\n", dlGetErrorString(error) );

   glDrawArrays(GL_TRIANGLES, 0, 6);

   error = glGetError();
   printf("glGetError: %s\n", dlGetErrorString(error) );
   glUseProgram(0);

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

void initTextureData()
{

}

void display()
{
  // default VAO for gl3 core-profile.
  GLuint vaoId;
  glGenVertexArrays(1, &vaoId);
  glBindVertexArray(vaoId);

  // all textures: input & output
  GLuint velTexIds[2]; // velocity0+1
  GLuint pdTexIds[2]; // [p]ressure[d]ivergence0+1 
  //TODO: GLuint extraTexIds[3]; // ? phi, phi_n_hat, phi_n_1_hat
  glGenTextures(2, velTexIds);
  glGenTextures(2, pdTexIds);

  for( int i = 0; i < 2; i++) 
  {
    glBindTexture(GL_TEXTURE_3D, velTexIds[i]);
    glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA, TEX_WIDTH, TEX_HEIGHT, TEX_DEPTH, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    glBindTexture(GL_TEXTURE_3D, pdTexIds[i]);
    glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA, TEX_WIDTH, TEX_HEIGHT, TEX_DEPTH, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  }

  // hard-coded time step
  const int limit = 2;
  int currVelID = 0, resultVelID = 1;
  int currPDID = 0, resultPDID = 1;

  simTexData texData;

  texData.uniforms["texWidth"] = TEX_WIDTH;
  texData.uniforms["texHeight"] = TEX_HEIGHT;
  texData.uniforms["texDepth"] = TEX_DEPTH;
  
  //init state of velocity and pressure texture
  texData.inputTexIds = {};
  texData.outputTexIds = {velTexIds[currVelID], pdTexIds[currPDID]};
  texData.fragShader = "frag_init_all.glsl";
  drawToTexture( texData );

  for( int i = 0; i < limit; i++ )
  {
    // 1. advect: 
    // input: velocity
    // output: intermediate velocity
    texData.inputTexIds = { velTexIds[currVelID] };
    texData.inputTexNames = { "velocity" };
    texData.outputTexIds = { velTexIds[resultVelID] };
    texData.fragShader = "frag_pass1_advect.glsl";
    drawToTexture( texData );

    // 2. diffuse: 
    // input: pressure
    // output: new pressure
    texData.inputTexIds = { pdTexIds[currPDID] };
    texData.inputTexNames = { "pdtex" };
    texData.outputTexIds = { pdTexIds[resultPDID] };
    texData.fragShader = "frag_pass2_diffuse.glsl";
    drawToTexture( texData );

    // 3. projection: 
    // input: intermediate velocity & pressure
    // output: divengence & final velocity
    texData.inputTexIds = { velTexIds[resultPDID], pdTexIds[currPDID] };
    texData.inputTexNames = { "velocity", "pdtex" };
    texData.outputTexIds = { velTexIds[currVelID], pdTexIds[resultPDID] };
    texData.fragShader = "frag_pass3_proj.glsl";
    drawToTexture( texData );


    // ray march to draw 3D texture
    texData.inputTexIds = { velTexIds[currVelID] };
    texData.inputTexNames = { "velocity" };
    texData.outputTexIds = {};
    texData.fragShader = "frag_screen.glsl";
    texData.uniforms["absorption"] = 0.4;
    drawToScreen( texData );
    
    // swap texture
    currPDID = resultPDID;
    resultPDID = (1-currPDID);
  }

  glDeleteTextures(2, velTexIds);
  glDeleteTextures(2, pdTexIds);
  glDeleteVertexArrays(1, &vaoId);
}

int main(int argc, char** argv)
{
   glutInit(&argc, argv);
   glutInitDisplayMode(GLUT_3_2_CORE_PROFILE | GLUT_DOUBLE | GLUT_RGBA);
   glutInitWindowSize(640,480);
   glutInitWindowPosition(100,100);
   glutCreateWindow("Ray March!");

   const GLubyte* renderer = glGetString(GL_RENDERER);
   const GLubyte* version = glGetString(GL_VERSION);
   printf("Renderer: %s\n", renderer);
   printf("OpenGL version supported: %s\n", version);

   GLint value;
   glGetIntegerv(GL_MAX_3D_TEXTURE_SIZE, &value);
   printf("max 3D texture size: %d\n", value);
   glGetIntegerv(GL_LAYER_PROVOKING_VERTEX, &value);
   printf("layer provoking vertex: %s\n", dlGetProvokingMode(value));

   glutDisplayFunc(display);
   glutMainLoop();

   return 0;
}
