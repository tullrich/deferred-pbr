#version 130

in vec3 position;
in vec3 normal;
#ifdef MESH_VERTEX_UV1
in vec2 texcoord;
#endif
#ifdef USE_NORMAL_MAP
in vec4 tangent;
#endif

uniform mat4 ModelViewProj;

out vec3 Normal;
#ifdef MESH_VERTEX_UV1
out vec2 Texcoord;
#endif
#ifdef USE_NORMAL_MAP
out vec3 Tangent;
out vec3 Bitangent;
#endif
#ifdef USE_HEIGHT_MAP
out vec3 FragModelPos;
#endif

void main()
{
	Normal = normal;

#ifdef MESH_VERTEX_UV1
	Texcoord = texcoord;
#endif // USE_UV1

#ifdef USE_NORMAL_MAP
	Tangent = tangent.xyz;
	Bitangent = cross(normal, tangent.xyz)*tangent.w;
#endif // USE_NORMAL_MAP

#ifdef USE_HEIGHT_MAP
  FragModelPos = position;
#endif

	gl_Position = ModelViewProj * vec4(position, 1.0);
}
