#version 330 core

in vec2 layerID;

layout(location=0) out vec4 fragColor;
layout(location=1) out vec4 fragColor1;

uniform float texWidth;
uniform float texHeight;
uniform float texDepth;

uniform sampler3D velocity;
uniform sampler3D pdtex; // [0]pressure, [1]divergence

void main(void)
{
	fragColor = vec4( 0.0, fragCoord.y, abs(layerID[0]-50) );
	fragColor1 = fragColor;
}