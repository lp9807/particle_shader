#version 330 core

// Interpolated values from the vertex shaders
in vec2 vtx_UV;
in vec3 vtx_Color;
/*in vec2 geom_UV;
in vec3 geom_Color;
in vec2 layerID;*/

out vec4 FragColor;

//uniform sampler3D density;
uniform sampler3D velocity;

uniform vec3 lightPos;
uniform vec3 lightIntensity;
uniform vec3 eyePos;
uniform float absorption;

uniform float texWidth;
uniform float texHeight;
uniform float texDepth;

void main()
{	
	/*
	const float maxDist = sqrt(3.0);
	const int nbSamples = 128;
	const float scale = maxDist/float(nbSamples);

	const int nbLightSamples = 32;
	const float lscale = maxDist / float(nbLightSamples);

    vec3 pos = vec3(gl_FragCoord.xy, 0.0);
	//vec3 pos = vec3(vtx_UV.xy,0.0); // starting point is current UV coordinative
	vec3 eyeDir = normalize(pos-eyePos)*scale;

	vec3 Lo = vec3(0.0); // output RGB
	float T = 1.0;       // Alpha -> transmittance

	for(int i=0; i<nbSamples; i++) 
	{
		float curDensity = texture(density, pos).x;

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
			for( int s = 0; s < nbLightSamples; ++s )
			{
				float ld = texture( density, lpos).x;
				T1 *= 1.0- absorption*lscale*ld;

				if( T1 <= 0.01 ) {
					break;
				}
			}

			vec3 Li = lightIntensity*T1;
			Lo += Li*T*curDensity*scale;
		}
		pos += eyeDir;
	}
 
	fragColor = vec4(Lo.xyz, 1.0-T);
    */

	FragColor = texture( velocity, vec3(vtx_UV.xy, 0.0) );
	//FragColor.xyz = geom_Color;
	//FragColor.xyz = vec3(vtx_UV.xy, 0.0);
}