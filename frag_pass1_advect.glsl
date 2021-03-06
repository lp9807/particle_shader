#version 330 core
in vec2 layerID;
in vec2 geom_UV;

layout(location=0) out vec4 v_pass1;
layout(location=1) out vec4 td_pass2;

uniform float texWidth;
uniform float texHeight;
uniform float texDepth;
uniform float currTime;
uniform float ambT;
uniform float buoyAlpha;
uniform float buoyBeta;
uniform vec3 forcepoint;

uniform sampler3D velocity;
uniform sampler3D scalar;


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

vec4 SF_force( in sim_output simCoord, in sampler3D temperatureTex )
{
   if( all( greaterThan(forcepoint,vec3(0.0003)) ) )
   {
      vec3 dir = ( simCoord.centerCell - forcepoint );
      return vec4( normalize(dir)*0.03/length(dir), 0.0);
   }
   // add buoyancy for smoke
   vec3 td = texture( temperatureTex, simCoord.centerCell ).xyz;
   float buoy = -buoyAlpha*td.y + buoyBeta*(td.x - ambT);
   return vec4( 0.0, buoy, 0.0, 0.0 ); 

   //return vec4(0.0);
}

vec4 SF_advect_scal( in sim_output simCoord, in sampler3D velocityTex, in sampler3D scalarTex )
{
   vec3 pos = simCoord.cellIndex;
   vec3 cellVel = texture( velocityTex, simCoord.centerCell ).xyz * vec3(texWidth, texHeight, texDepth) ; // samplePointClamp
   vec3 advectCell = SF_cellIndex2TexCoord( pos-currTime*cellVel );
   return texture( scalarTex, advectCell ); // sampling : linear
}

vec4 SF_advect_vel( in sim_output simCoord, in sampler3D velocityTex )
{
   vec3 pos = simCoord.cellIndex;
   vec3 cellVel = texture( velocityTex, simCoord.centerCell ).xyz * vec3(texWidth, texHeight, texDepth) ; // samplePointClamp
   vec3 advectCell = SF_cellIndex2TexCoord( pos-currTime*cellVel );
   return texture( velocityTex, advectCell ); // sampling : linear
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
   sim_output currCoord = SF_initCellPos( vec3( geom_UV.xy*vec2(texWidth, texHeight), layerID.x ) );

   // advect velocity
   v_pass1 = SF_advect_vel( currCoord, velocity ) + SF_force( currCoord, scalar );
   // advect temperature
   td_pass2 = SF_advect_scal( currCoord, velocity, scalar );
}
