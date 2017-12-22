#version 150

layout(triangles) in;
layout(triangle_strip, max_vertices=300) out;

/*in VertexData {
	vec2 texCoord;
	vec3 color;
} VertexIn[3];*/
in vec3 vtx_Color[];
in vec2 vtx_UV[];

/*out VertexData {
	vec2 texCoord;
	vec3 color;	
} VertexOut;*/
out vec3 geom_Color;
out vec2 geom_UV;
out vec2 layerID;

uniform float texDepth;

void main()
{
	int j = 0;
	for( j = 0; j < 100; j++)
	{
		for(int i = 0; i < gl_in.length(); i++)
		{
			gl_Layer = j;
	 		layerID = vec2(j);
			// copy attributes
			gl_Position = vec4( gl_in[i].gl_Position.xy+0.01*j, gl_in[i].gl_Position.zw );
			geom_Color = vec3( vtx_Color[i].xyz );
			geom_UV = vec2( vtx_UV[i].xy );
		   	// done with the vertex
		   	EmitVertex();
		}
		EndPrimitive();
	}

}