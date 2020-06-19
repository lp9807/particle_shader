#version 330 core

// Interpolated values from the vertex shaders
in vec3 FragInColor;
out vec4 FragColor;

//uniform sampler3D density;
uniform sampler3D ScalarCube;
//uniform sampler3D velocity;

/*uniform vec3 lightPos;
uniform vec3 lightIntensity;*/
uniform float absorption;
uniform mat4 MVP;
uniform mat4 invertMVP;
uniform vec3 eyePos;
uniform vec2 viewport;

uniform float texWidth;
uniform float texHeight;
uniform float texDepth;


vec3 getPos()
{
	vec4 ndcPos = vec4( (gl_FragCoord.x/viewport.x - 0.5)*2.0,
	                    (gl_FragCoord.y/viewport.y - 0.5)*2.0,
	                    (2.0*gl_FragCoord.z-gl_DepthRange.far-gl_DepthRange.near)/(gl_DepthRange.far-gl_DepthRange.near),
	                    1.0 );
	vec4 origPos = invertMVP * (ndcPos/gl_FragCoord.w);
	return origPos.xyz;
}

vec3 normalizedPos( in vec3 pos )
{
	return (pos+1.0)/2.0;
}

bool isOutOfBox( in vec3 pos )
{
	return ( any( lessThan( pos, vec3(-1.0-0.0005) ) ) ||
		     any( greaterThan( pos, vec3(1.0+0.005) ) ) );
}

void main()
{	
    // hard-coded uniforms
    const float Step = 64.0;
    const float LStep = 64.0;
    const vec3 LightPos = vec3( 8, 10, -1 );
    const float Density = 9.0;
    const float LDensity = 5.0;
    const float LThreshold = 0.01;

	const float scale = 1.0/Step;
	const float lscale = 1.0/Step;
    const float density = Density*scale;
    const float ldensity = LDensity*lscale;
    const float lthresh = -log(LThreshold)/ldensity; 
    
    vec3 pos = getPos();
    vec3 eyeDir = normalize(pos-eyePos)*scale;
    vec3 lightDir = normalize(LightPos-pos)*lscale;  

	vec3 Lo = vec3(0.0); // output RGB
	float T = 1.0;       // Alpha -> transmittance

    // 1. display density accumunation result. 
    // 2. display lighting result.

	while( !isOutOfBox(pos) ) 
	{
		float curDensity = texture(ScalarCube, normalizedPos(pos)).y;

		if( curDensity > 0.001 )
		{
			float ld = 0.0;
			vec3 lpos = pos;
			while( !isOutOfBox(lpos) )
			{
               lpos += lightDir;
               ld += texture( ScalarCube, normalizedPos(lpos) ).y;
               if(ld>lthresh) break;
			}

            curDensity = clamp( curDensity*density, 0.0, 1.0 );
            float T1 = exp(-ld*ldensity); 
			vec3 Li = vec3(curDensity*T1);
			Lo += Li*T;
            T *= (1.0-curDensity);
		}
		pos += eyeDir;
	}
 
	FragColor = vec4(Lo.xyz, 1.0-T) + 0.002*vec4(FragInColor,0.0);

    //FragColor += texture( velocity, vec3(geom_UV.xy,1) );
	//FragColor.xyz = FragInColor.xyz;
}
