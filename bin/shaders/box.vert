 #version 130

in vec3 position;
in vec3 normal;
in vec3 tangent;
in vec2 texcoord;

uniform mat4 ModelView;
uniform mat4 ModelViewProj;

out vec3 Position;
out vec2 Texcoord;
out vec3 Normal;
out vec3 Tangent;
out vec3 Bitangent;

void main()
{
	Position = (ModelView * vec4(position, 1.0)).xyz;
	Texcoord = texcoord;
	Normal = vec4(normal, 0.0).xyz;
	Tangent = tangent;
	Bitangent = cross(normal, tangent);
	gl_Position = ModelViewProj * vec4(position, 1.0);
}
