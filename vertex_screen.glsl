#version 330 core
layout(location = 0) in vec3 in_Position;
layout(location = 1) in vec3 in_Color;

out vec3 vtx_Color;
//out vec3 vtx_Color;
uniform mat4 MVP;

void main()
{
	gl_Position = MVP * vec4(in_Position.xyz, 1.0);
	vtx_Color = in_Color;
}
