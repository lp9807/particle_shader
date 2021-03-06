#version 330 core

in vec2 layerID;
in vec2 geom_UV;

layout(location=0) out vec4 vColor;
layout(location=1) out vec4 pdColor;
layout(location=2) out vec4 scalarColor; //[0]: temparature, [1]: density

uniform float texWidth;
uniform float texHeight;
uniform float texDepth;

vec3 mod289(vec3 x)
{
	return x - floor(x*(1.0/289.0)) * 289.0;
}

vec4 permute(vec4 x)
{
	return vec4( mod289(((x.xyz * 34.0)+1.0)*x.xyz),
                 mod289(((x.www * 34.0)+1.0)*x.www).x );
}

vec4 taylorInvSqrt( vec4 r)
{
	return 1.79284291400159 - 0.85373472095314 * r;
}

vec3 fade(vec3 t) {
    return t*t*t*(t*(t*6.0-15.0)+10.0);
}

// Classic Perlin noise
float Perlin(vec3 P)
{
    vec3 Pi0 = floor(P);
    vec3 Pf0 = fract(P);
    vec3 Pi1 = Pi0 + vec3(1.0, 1.0, 1.0);
    vec3 Pf1 = Pf0 - vec3(1.0, 1.0, 1.0);

    Pi0 = mod289(Pi0); // To avoid truncation effects in permutation
    Pi1 = mod289(Pi1);

    vec4 ix = vec4(Pi0.x, Pi1.x, Pi0.x, Pi1.x);
    vec4 iy = vec4(Pi0.yy, Pi1.yy );
    vec4 iz0 = vec4(Pi0.z);
    vec4 iz1 = vec4(Pi1.z);
         
    vec4 ixy = permute(permute(ix) + iy);
    vec4 ixy0 = permute( ixy + iz0 );
    vec4 ixy1 = permute( ixy + iz1 ); 

    vec4 gx0 = ixy0 * 1.0 / 7.0;
    vec4 gy0 = fract( floor(gx0) * (1.0 / 7.0)) - 0.5 ;
    gx0 = fract(gx0); 
    vec4 gz0 = vec4(0.5) - abs(gx0) - abs(gy0);
    vec4 sz0 = step(gz0, vec4(0.0));
    gx0 -= sz0 * (step(0, gx0) - 0.5);
    gy0 -= sz0 * (step(0, gy0) - 0.5);

    vec4 gx1 = ixy1 * 1.0 / 7.0;
    vec4 gy1 = fract( floor(gx1) * 1.0 / 7.0 ) - 0.5;
    gx1 = fract(gx1);
    vec4 gz1 = vec4(0.5) - abs(gx1) - abs(gy1);
    vec4 sz1 = step(gz1, vec4(0.0));
    gx1 -= sz1 * (step(0.0, gx1) - 0.5);
    gy1 -= sz1 * (step(0.0, gy1) - 0.5);

    vec3 g000 = vec3(gx0.x, gy0.x, gz0.x);
    vec3 g100 = vec3(gx0.y, gy0.y, gz0.y);
    vec3 g010 = vec3(gx0.z, gy1.z, gz0.z);
    vec3 g110 = vec3(gx0.w, gy1.w, gz0.w);
    vec3 g001 = vec3(gx1.x, gy1.x, gz1.x);
    vec3 g101 = vec3(gx1.x, gy1.y, gz1.y);
    vec3 g011 = vec3(gx1.z, gy1.z, gz1.z);
    vec3 g111 = vec3(gx1.w, gy1.w, gz1.w);
     
    vec4 norm0 = taylorInvSqrt(vec4(dot(g000, g000), dot(g010, g010), dot(g100, g100), dot(g110, g110)));
    g000 *= norm0.x;  
    g010 *= norm0.y;  
    g100 *= norm0.z;  
    g110 *= norm0.w;  

    vec4 norm1 = taylorInvSqrt(vec4(dot(g001, g001), dot(g011, g011), dot(g101, g101), dot(g111, g111)));
    g001 *= norm1.x;  
    g011 *= norm1.y;  
    g101 *= norm1.z;  
    g111 *= norm1.w;  
     
    float n000 = dot(g000, Pf0);
    float n100 = dot(g100, vec3(Pf1.x, Pf0.y, Pf0.z));
    float n010 = dot(g010, vec3(Pf0.x, Pf1.y, Pf0.z));
    float n110 = dot(g110, vec3(Pf1.x, Pf1.y, Pf0.z));
    float n001 = dot(g001, vec3(Pf0.x, Pf0.y, Pf1.z));
    float n101 = dot(g101, vec3(Pf1.x, Pf0.y, Pf1.z));
    float n011 = dot(g011, vec3(Pf0.x, Pf1.y, Pf1.z));
    float n111 = dot(g111, Pf1);
     
    vec3 fade_xyz = fade(Pf0.xyz);
    vec4 n_x = mix(vec4(n000, n001, n010, n011), 
    	           vec4(n100, n101, n110, n111), fade_xyz.x);
    vec2 n_xy = mix(n_x.xy, n_x.zw, fade_xyz.y);
    float n_xyz = mix( n_xy.x, n_xy.y, fade_xyz.z);
    return 2.2 * n_xyz;
}

void main(void)
{
	vec3 cube = vec3( texWidth, texHeight, texDepth );
	vec3 frequency = 20.0 / cube;  
    vec3 pos = vec3( geom_UV.xy * cube.xy, layerID.x );  
    float noise = abs( Perlin( pos*frequency ) );

    
    vec3 center =  cube /( 2.0 + 0.5 );
    vec3 offset = pos - center;
    //float d = sqrt(offset.x*offset.x + offset.y*offset.y + offset.z*offset.z);
    //float color = (d-noise) < r) ? 255 : 0;
    
	//vColor = vec4( noise, noise, noise, 1.0 );
    //vColor = vec4(pos.xyz/cube.xyz, 1.0);
    //vColor = vec4(layerID.xxx/cube.zzz, 1.0);
    /*vec3 emitter = vec3( 3.0, 0.0, 0.4 );
    if( distance(pos,vec3(cube/2.0)) < 50.0 )
    {
        vColor.xyz = noise * emitter;
    }
    else
    {
        vColor.xyz = vec3(noise);
    }*/

    vColor.xyz = vec3(0.3);    
	pdColor.xyz = normalize(offset); //geom_UV.y*9.8 );
    scalarColor.xyz = vec3( 300*noise, noise, 0.0 ); // 300K = 27C
}
