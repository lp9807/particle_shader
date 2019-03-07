#version 330 core

// Interpolated values from the vertex shaders
in vec3 vtx_Color;
out vec4 FragColor;

//uniform sampler3D density;
uniform sampler3D scalar;
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
	const float maxDist = sqrt(2.0);
	const float scale = maxDist/64.0;
	const float lscale = maxDist / 32.0;

    vec3 pos = getPos();
    vec3 lightPos = vec3( 8, 10, -1 );
    vec3 lightIntensity = vec3(2.0)*length(normalize(lightPos-pos));
    vec3 eyeDir = normalize(pos-eyePos)*scale;

	vec3 Lo = vec3(0.0); // output RGB
	float T = 1.0;       // Alpha -> transmittance

	while( !isOutOfBox(pos) ) 
	{
		float curDensity = texture(scalar, normalizedPos(pos)).y;

		if( curDensity > 0.0 )
		{
			T *= 1.0-curDensity*scale*absorption;
			if( T <= 0.01 ) 
			{
				break;
			}

			vec3 lightDir = normalize(lightPos-pos)*lscale;

			float T1 = 1.0;
			vec3 lpos = pos + lightDir;
			while( !isOutOfBox(lpos) )
			{
				float ld = texture( scalar, normalizedPos(lpos) ).y;
				T1 *= (1.0 - absorption*lscale*ld);

				if( T1 <= 0.01 ) {
					break;
				}

				lpos += lightDir;
			}

			vec3 Li = lightIntensity*T1;
			Lo += Li*T*curDensity*scale;
		}
		pos += eyeDir;
	}
 
	FragColor = vec4(Lo.xyz, 1.0-T) + 0.002*vec4(vtx_Color,0.0);

    //FragColor += texture( velocity, vec3(geom_UV.xy,1) );
	//FragColor.xyz = vtx_Color.xyz;
}
