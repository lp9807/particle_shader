#version 150

layout(triangles) in;
layout(traingle_strip, max_vertices=3) out;

in VertexData {
	vec2 texCoord;
	vec3 normal;
} VertexIn[3];

out VertexData {
	vec2 texCoord;
	vec3 normal;	
} VertexOut;

void main()
{
	for(int i = 0; i < gl_in.length(); i++)
	{
	   // copy attributes
	   gl_Postion = gl_in[i].gl_Position;
	   VertexOut.normal = VertexIn[i].normal;
	   VertexOut.texCoord = VertexIn[i].texCoord;

	   // done with the vertex
	   EmitVertex();
	}
}