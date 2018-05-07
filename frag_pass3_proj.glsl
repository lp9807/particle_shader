#version 330 core
in vec2 layerID;
in vec2 geom_UV;

layout(location=0) out vec4 v_pass3;
layout(location=1) out vec4 pd_pass3;

uniform float texWidth;
uniform float texHeight;
uniform float texDepth;

uniform sampler3D velocity;
uniform sampler3D pdtex; // [0]pressure, [1]divergence

struct sim_output
{
   // Index of current grid cell( i, j, k in [0, gridSize] range ) 
   vec3 cellIndex;
   // texture coordinates (x,y,z in [0,1] range 
   // for the current gird cell and its immediate neighbors )
   vec3 centerCell;
   vec3 leftCell;
   vec3 rightCell;
   vec3 bottomCell;
   vec3 topCell;
   vec3 downCell;
   vec3 upCell;
   vec4 pos;  // ? 2D slice vertex in homogenous clip space
   //int rtIndex; // specifies destination slice : the output 3D texture == layerID[0]
};

vec3 SF_cellIndex2TexCoord( in vec3 index )
{
   // convert a value in the range [0, gridSize] to one in the range [0,1].
   return vec3( index.x/texWidth,
                index.y/texHeight,
                (index.z+0.5)/texDepth );
}

sim_output SF_initCellPos( in vec3 centerPos )
{
  sim_output simCoord;
  simCoord.cellIndex = centerPos;

  vec3 center = SF_cellIndex2TexCoord( centerPos );
  simCoord.centerCell = center;
  simCoord.leftCell = vec3( center.x-1.0/texWidth, center.y, center.z );
  simCoord.rightCell = vec3( center.x+1.0/texWidth, center.y, center.z );
  simCoord.bottomCell = vec3( center.x, center.y-1.0/texHeight, center.z );
  simCoord.topCell = vec3( center.x-1, center.y+1.0/texHeight, center.z );
  simCoord.downCell = vec3( center.x-1, center.y, center.z-1.0/texDepth );
  simCoord.upCell = vec3( center.x-1, center.y, center.z+1.0/texDepth );

  return simCoord;
}

float SF_divergence( in sim_output simCoord, in sampler3D velocityTex )
{
   // Get velocicty values from neighboring cells
   // sampling : clamp
   vec4 fieldL = texture( velocityTex, simCoord.leftCell );
   vec4 fieldR = texture( velocityTex, simCoord.rightCell );
   vec4 fieldB = texture( velocityTex, simCoord.bottomCell );
   vec4 fieldT = texture( velocityTex, simCoord.topCell );
   vec4 fieldD = texture( velocityTex, simCoord.downCell );
   vec4 fieldU = texture( velocityTex, simCoord.upCell );
   // compute the velocity's divergence using central differences.
   float divergence = 0.5 *( (fieldR.x - fieldL.x) + 
                             (fieldT.y - fieldB.y) +
                             (fieldU.z - fieldD.z) );

   return divergence;
}

vec4  SF_project( in sim_output simCoord,
                  in sampler3D velocityTex,
                  in sampler3D PDTex )
{
   // compute the gradient of pressure at the current cell by taking
   // central differences of neighboring pressure values.
   // sampling : clamp
   float pL = texture( PDTex, simCoord.leftCell ).x; 
   float pR = texture( PDTex, simCoord.rightCell ).x;
   float pB = texture( PDTex, simCoord.bottomCell ).x;
   float pT = texture( PDTex, simCoord.topCell ).x;
   float pD = texture( PDTex, simCoord.downCell ).x;
   float pU = texture( PDTex, simCoord.upCell ).x;
   vec3 gradP = 0.5*vec3( pR-pL, pT-pB, pU-pD);
   // project the velocity onto its divergence-free component by subtracting 
   // the gradient of pressure.
   vec3 vOld = texture( velocityTex, simCoord.centerCell ).xyz;
   vec3 vNew = vOld - gradP;

   return vec4( vNew, 0);
}

void main(void)
{
  sim_output currCoord = SF_initCellPos( vec3( geom_UV.xy*vec2(texWidth, texHeight), layerID.x ) );
  // udpate velocity
  v_pass3 = SF_project( currCoord, velocity, pdtex );
  // update divergence
  pd_pass3.y = SF_divergence(currCoord, velocity);
}
