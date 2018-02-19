#version 330 core
in vec2 layerID;
in vec2 geom_UV;

layout(location=0) out vec4 fragColor;

uniform float texWidth;
uniform float texHeight;
uniform float texDepth;
uniform float currTime;

uniform sampler3D velocity;

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

vec3 SF_cellIndex2TexCoord( vec3 index )
{
   // convert a value in the range [0, gridSize] to one in the range [0,1].
   return vec3( index.x/texWidth,
                index.y/texHeight,
                (index.z+0.5)/texDepth );
}

vec4 SF_advect_vel( in sim_output simCoord, in sampler3D velocityTex )
{
   vec3 pos = simCoord.cellIndex;
   vec3 cellVel = texture( velocityTex, simCoord.centerCell ).xyz; // samplePointClamp
   pos -= currTime * cellVel;
   pos = SF_cellIndex2TexCoord( pos );
   return texture( velocityTex, pos ); // sampling : linear
}

vec4 SF_advect_macCormack( in sim_output simCoord, 
                          in float timestep,
                          in sampler3D velocityTex,
                          in sampler3D phi_n_tex,
                          in sampler3D phi_n_hat_tex,
                          in sampler3D phi_n_1_hat_tex )
{
   // Trace back along the initial characteristic - we'll use
   // values near this semi-Lagrangian "particle" to clamp our
   // final advected value.
   // sampling : clamp
   vec3 cellVel = texture( velocityTex, simCoord.centerCell ).xyz;
   vec3 npos = simCoord.cellIndex - timestep * cellVel;
   // Find the cell corner closest to the "particle" and compute the
   // texture coordinate corresponding to that location.
   npos = floor( npos + vec3(0.5, 0.5, 0.5) );
   npos = SF_cellIndex2TexCoord(npos);
   // Get the values of nodes that contribute to the interpolated value.
   // Texel centers will be a half-texel away from the cell corner.
   vec3 ht = vec3( 0.5/texWidth,
                   0.5/texHeight,
                   0.5/texDepth );
   
   vec4 nodeValues[8];
   // sampling : clamp
   nodeValues[0] = texture( phi_n_tex, npos + vec3( -ht.x, -ht.y, -ht.z ) ); 
   nodeValues[1] = texture( phi_n_tex, npos + vec3( -ht.x, -ht.y, ht.z ) );
   nodeValues[2] = texture( phi_n_tex, npos + vec3( -ht.x, ht.y, -ht.z ) );
   nodeValues[3] = texture( phi_n_tex, npos + vec3( -ht.x, ht.y, ht.z ) );
   nodeValues[4] = texture( phi_n_tex, npos + vec3( ht.x, -ht.y, -ht.z ) );
   nodeValues[5] = texture( phi_n_tex, npos + vec3( ht.x, -ht.y, ht.z ) );
   nodeValues[6] = texture( phi_n_tex, npos + vec3( ht.x, ht.y, -ht.z ) );
   nodeValues[7] = texture( phi_n_tex, npos + vec3( ht.x, ht.y, ht.z ) );

   // Determine a valid range for the result.
   vec4 phiMin = min( nodeValues[7], 
                  min( nodeValues[6],
                   min( nodeValues[5],
                    min( nodeValues[4], 
                     min( nodeValues[3],
                      min( nodeValues[2], 
                        min( nodeValues[0], nodeValues[1] ) ) 
                     ) ) ) ) );
   vec4 phiMax = max( nodeValues[7],
                  max( nodeValues[6], 
                   max( nodeValues[5],
                    max( nodeValues[4], 
                      max( nodeValues[3], 
                        max( nodeValues[2], 
                          max( nodeValues[0], nodeValues[1] ) 
                 ) ) ) ) ) );

   // Perform final advection, combining values from intermediate advection steps.
   vec4 r = texture( phi_n_1_hat_tex, npos ) + // sampling linear
            0.5 * ( texture( phi_n_tex, simCoord.centerCell ) - // sampling clamp
                    texture( phi_n_hat_tex, simCoord.centerCell ) ); // sampling clamp

   // Clamp result to the desired range.
   r = max( min(r, phiMax), phiMin );
   return r;
}

void main(void)
{
   sim_output currCoord;
   currCoord.centerCell = vec3( geom_UV.xy, layerID );
   fragColor = SF_advect_vel( currCoord, velocity );
}
