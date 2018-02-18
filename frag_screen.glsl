#version 330 core

// Interpolated values from the vertex shaders
in vec2 vtx_UV;
out vec4 FragColor;

//uniform sampler3D density;
uniform sampler3D velocity;

/*uniform vec3 lightPos;
uniform vec3 lightIntensity;
uniform vec3 eyePos;*/
uniform float absorption;

uniform float texWidth;
uniform float texHeight;
uniform float texDepth;

void main()
{	
	const float maxDist = sqrt(3.0);
	const int nbSamples = 128;
	const float scale = maxDist/float(nbSamples);

	const int nbLightSamples = 32;
	const float lscale = maxDist / float(nbLightSamples);

    vec3 pos = vec3(vtx_UV.xy, 0.0);
	vec3 eyePos = vec3( 0.5, 0.5, 0.5);
    vec3 lightPos = vec3( 1, 0.5, -1 );
    vec3 lightIntensity = vec3(2.0);
    vec3 eyeDir = normalize(pos-eyePos)*scale;

	vec3 Lo = vec3(0.0); // output RGB
	float T = 1.0;       // Alpha -> transmittance

	for(int i=0; i<nbSamples; i++) 
	{
		float curDensity = texture(velocity, pos).x;

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
				float ld = texture( velocity, lpos).x;
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
 
	FragColor = vec4(Lo.xyz, 1.0-T);

    //FragColor += texture( velocity, vec3(vtx_UV,i/100.0) );
	//FragColor.xyz = vec3(vtx_UV.xy, 0.0);
}
