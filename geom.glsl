#version 150

layout(triangles) in;
layout(triangle_strip, max_vertices=300) out;

/*in VertexData {
	vec2 texCoord;
	vec3 normal;
} VertexIn[3];*/
in vec3 vtx_Color[];

/*out VertexData {
	vec2 texCoord;
	vec3 normal;	
} VertexOut;*/
out vec3 geom_Color;
out vec2 layerID;

void main()
{
	for( int j = 0; j < 100; j++)
	{
		for(int i = 0; i < gl_in.length(); i++)
		{
	 		gl_Layer = j;
	 		layerID[0] = j;
			// copy attributes
			gl_Position = vec4( gl_in[i].gl_Position.x+0.1*j, gl_in[i].gl_Position.yz, 1.0 );
			//VertexOut.normal = VertexIn[i].normal;
			//VertexOut.texCoord = VertexIn[i].texCoord;
			geom_Color = vec3( vtx_Color[i].xyz );
		   	// done with the vertex
		   	EmitVertex();
		}
		EndPrimitive();
	}

}