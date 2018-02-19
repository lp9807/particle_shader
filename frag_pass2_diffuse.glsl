#version 330 core
in vec2 layerID;
in vec2 geom_UV;

layout(location=0) out vec4 fragColor;

uniform float texWidth;
uniform float texHeight;
uniform float texDepth;

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

float SF_jacobi( in sim_output simCoord, 
                 in sampler3D PDTex )
{
   // Get the divergence at the current cell.
   // sampling : clamp
   float dC = texture( PDTex, simCoord.centerCell ).y;
   // Get pressure values from neighboring cells.
   // sampling : clamp
   float pL = texture( PDTex, simCoord.leftCell ).x; 
   float pR = texture( PDTex, simCoord.rightCell ).x; 
   float pB = texture( PDTex, simCoord.bottomCell ).x; 
   float pT = texture( PDTex, simCoord.topCell ).x; 
   float pD = texture( PDTex, simCoord.downCell ).x; 
   float pU = texture( PDTex, simCoord.upCell ).x; 

   // compute the new pressure value for the center cell.
   return (pL + pR + pB + pT + pU + pD - dC)/6.0;
}

void main(void)
{
   sim_output currCoord;
   currCoord.centerCell = vec3( geom_UV.xy, layerID );
   float pressure = SF_jacobi( currCoord, pdtex );
   fragColor.x = pressure;
}
