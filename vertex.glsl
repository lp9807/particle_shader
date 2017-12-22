#version 330 core
layout(location = 0) in vec3 in_Position;
layout(location = 1) in vec3 in_Color;
layout(location = 2) in vec2 in_UV;

out vec3 vtx_Color;
out vec2 vtx_UV;

void main()
{
	gl_Position.xyz = in_Position;
	vtx_Color = in_Color;
	vtx_UV = in_UV;
}