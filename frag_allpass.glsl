#version 330 core
in vec3 geom_Color;
in vec2 layerID;

layout(location=0) out vec4 fragColor;

uniform float texWidth;
uniform float texHeight;
uniform float texDepth;
uniform float currTime;

uniform sampler3D velocity;
uniform sampler3D pdtex; // [0]pressure, [1]divergence
uniform sampler3D phi_n;
uniform sampler3D phi_n_hat;
uniform sampler3D phi_n_1_hat;

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

// TODO:
// 1. figure out how to combine project and advect result.
// 2. output velocity/divergence/pressure to layered texture. 

void main(void)
{
	sim_output currCoord;
	float divergence = SF_divergence( currCoord, velocity );
	float pressure = SF_jacobi( currCoord, pdtex );
	// write to pdTex grid.

    // qA = advect(q(n), delta(t), u(n))
    // qB = qA + delta(t)*g -> add force
    // q(n+1) = project(delta(t), qB)

    // A. advect: macCormack, to solve dissipation.
    //vec4 newVel = SF_advect_macCormack( currCoord, currTime, velocity, phi_n, phi_n_hat, phi_n_1_hat );
    // B. advect: semi-Lagrangian advection
    vec4 velA = SF_advect_vel( currCoord, velocity ); 
    velA = SF_diffuse( velA ); // jacobi
    vec4 velN = SF_project( currCoord, velocity, pdtex ); // divengence + gradient

    //fragColor = vec4(geom_Color.xy, layerID[0], 1.0);
	//fragColor = vec4(1.0, 0.0, 0.0, 1.0);
}

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
   pos -= timeStep * cellVel;
   pos = cellIndex2TexCoord( pos );
   return texture( velocityTex, pos ); // sampling : linear
}

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

float SF_divergence( in sim_output simCoord, in sampler velocityTex )
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
                             (fieldU.z - filedD.z ) );

   return divergence;
}

vec4 SF_project( in sim_output simCoord,
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
   vec3 vOld = texture( velocityTex, simCoord.centerCell);
   vec3 vNew = vOld - gradP;

   return vec4( vNew, 0);
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
   npos = cellIndex2TextCoord(npos);
   // Get the values of nodes that contribute to the interpolated value.
   // Texel centers will be a half-texel away from the cell corner.
   vec3 ht = vec3( 0.5/textureWidth,
                   0.5/textureHeight,
                   0.5/textureDepth );
   
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