struct sim_output
{
   vec3 cellIndex;
   // texture coordinates (x,y,z in [0,1] range 
   //        for the current gird cell and its immediate neighbors )
   vec3 centerCell;
   vec3 leftCell;
   vec3 rightCell;
   vec3 bottomCell;
   vec3 topCell;
   vec3 downCell;
   vec3 upCell;
   vec4 pos; 
   // int rtIndex; specifies destination slice : the output 3D texture
};

uniform float textureWidth;
uniform float textureHeight;
uniform float textureDepth;
uniform float timeStep;

vec3 SF_cellIndex2TexCoord( vec3 index )
{
   // convert a value in the range [0, gridSize] to one in the range [0,1].
   return vec3( index.x/textureWidth,
                index.y/textureHeight,
                (index.z+0.5)/textureDepth );
}

vec4 SF_advect_vel( in sim_output simCoord, in sampler2DArray velTex )
{
   vec3 pos = simCoord.cellIndex;
   vec3 cellVel = texture2DArray( velTex, simCoord.centerCell ).xyz; // samplePointClamp
   pos -= timeStep * cellVel;
   pos = cellIndex2TexCoord( pos );
   return texture2DArray( velTex, pos ); // sampling : linear
}

float SF_divergence( in sim_output simCoord, in sampler2DArray velTex )
{
   // Get velocicty values from neighboring cells
   // sampling : clamp
   vec4 fieldL = texture2DArray( velTex, simCoord.leftCell );
   vec4 fieldR = texture2DArray( velTex, simCoord.rightCell );
   vec4 fieldB = texture2DArray( velTex, simCoord.bottomCell );
   vec4 fieldT = texture2DArray( velTex, simCoord.topCell );
   vec4 fieldD = texture2DArray( velTex, simCoord.downCell );
   vec4 fieldU = texture2DArray( velTex, simCoord.upCell );
   // compute the velocity's divergence using central differences.
   float divergence = 0.5 *( (fieldR.x - fieldL.x) + 
                             (fieldT.y - fieldB.y) +
                             (fieldU.z - filedD.z ) );

   return divergence;
}

float SF_jacobi( in sim_output simCoord, in sampler2DArray pressureTex, 
                 in sampler2DArray divergenceTex )
{
   // Get the divergence at the current cell.
   // sampling : clamp
   float dC = texture2DArray( divergenceTex, simCoord.centerCell );
   // Get pressure values from neighboring cells.
   // sampling : clamp
   float pL = texture2DArray( pressureTex, simCoord.leftCell ); 
   float pR = texture2DArray( pressureTex, simCoord.rightCell ); 
   float pB = texture2DArray( pressureTex, simCoord.bottomCell ); 
   float pT = texture2DArray( pressureTex, simCoord.topCell ); 
   float pD = texture2DArray( pressureTex, simCoord.downCell ); 
   float pU = texture2DArray( pressureTex, simCoord.upCell ); 

   // compute the new pressure value for the center cell.
   return (pL + pR + pB + pT + pU + pD - dC)/6.0;
}

vec4 SF_project( in sim_output simCoord, in sampler2DArray pressureTex,
                 in sampler2DArray velTex )
{
   // compute the gradient of pressure at the current cell by taking
   // central differences of neighboring pressure values.
   // sampling : clamp
   float pL = texture2DArray( pressureTex, simCoord.leftCell ); 
   float pR = texture2DArray( pressureTex, simCoord.rightCell );
   float pB = texture2DArray( pressureTex, simCoord.bottomCell );
   float pT = texture2DArray( pressureTex, simCoord.topCell );
   float pD = texture2DArray( pressureTex, simCoord.downCell );
   float pU = texture2DArray( pressureTex, simCoord.upCell );
   vec3 gradP = 0.5*vec3( pR-pL, pT-pB, pU-pD);
   // project the velocity onto its divergence-free component by subtracting 
   // the gradient of pressure.
   vec3 vOld = texture2DArray( velTex, simCoord.centerCell);
   vec3 vNew = vOld - gradP;
   return vec4( vNew, 0);
}

vec4 SF_advect_macCormack( in sim_output simCoord, in float timestep )
{
   // 
   // in sampler2DArray velTex
   // in sampler2DArray phi_n ?
   // in sampler2DArray phi_n_hat
   // in sampler2DArray phi_n_1_hat
   //

   // Trace back along the initial characteristic - we'll use
   // values near this semi-Lagrangian "particle" to clamp our
   // final advected value.
   // sampling : clamp
   vec3 cellVel = texture2DArray( velTex, simCoord.centerCell ).xyz;
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
   nodeValues[0] = texture2DArray( phi_n, npos + vec3( -ht.x, -ht.y, -ht.z ) ); 
   nodeValues[1] = texture2DArray( phi_n, npos + vec3( -ht.x, -ht.y, ht.z ) );
   nodeValues[2] = texture2DArray( phi_n, npos + vec3( -ht.x, ht.y, -ht.z ) );
   nodeValues[3] = texture2DArray( phi_n, npos + vec3( -ht.x, ht.y, ht.z ) );
   nodeValues[4] = texture2DArray( phi_n, npos + vec3( ht.x, -ht.y, -ht.z ) );
   nodeValues[5] = texture2DArray( phi_n, npos + vec3( ht.x, -ht.y, ht.z ) );
   nodeValues[6] = texture2DArray( phi_n, npos + vec3( ht.x, ht.y, -ht.z ) );
   nodeValues[7] = texture2DArray( phi_n, npos + vec3( ht.x, ht.y, ht.z ) );

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
   vec4 r = texture2DArray( phi_n_1_hat, nposTC ) + // sampling linear
            0.5 * ( texture2DArray( phi_n, simCoord.centerCell ) - // sampling clamp
                    texture2DArray( phi_n_hat, simCoord.centerCell ) ); // sampling clamp

   // Clamp result to the desired range.
   r = max( min(r, phiMax) , phiMin );
   return r;
}

// 
// boundary conditions
//


