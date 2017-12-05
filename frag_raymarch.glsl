#version 330 core
in vec2 vtx_UV;
out vec4 fragColor;

uniform sampler3D velocity;
uniform vec3 rayOrigin;

void main()
{
	//fragColor = texture( velocity, vec3(1.0,1.0,1.0)-vec3(vtx_UV.xy, 0.0) );

	vec4 sum = vec4(0, 0, 0, 0);
	pos = rayOrigin;
	for(int i=0; i<adskUID_VolumeSteps; i++) {
		vec4 col = texture( velocity, pos);
		col.a *= (adskUID_Density * 15.0 ) * adskUID_Density / float( adskUID_VolumeSteps );

		// pre-multiply alpha
		col.rgb *= col.a;
		sum = sum + col*(1.0 - sum.a);


		if(length(pos-rayOrigin) > z) {
			// We've gone beyond our geo depth! Comp the sum so far over the geo
			vec3 bg = lightboxinput.rgb;
			vec3 comp = bg * (1.0 - sum.a) + sum.rgb;
			return vec4(comp, 1.0);
		}

		pos += 10.0*rayStep/float(adskUID_VolumeSteps);
	}
 
	fragColor = texture( fbo, vec3(vtx_UV.xy, 0.02) );
}