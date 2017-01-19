in vec3 Position;
in vec2 Texcoord;
in vec3 Normal;
in vec3 Tangent;
in vec3 Bitangent;

uniform mat4 ModelView;
uniform mat4 invTModelView;

uniform sampler2D DiffuseMap;
uniform sampler2D NormalMap;
uniform sampler2D SpecularMap;
uniform sampler2D AOMap;

out vec3 PositionOut;
out vec4 DiffuseOut;
out vec3 NormalOut;
out vec4 SpecularOut;

void main()
{
	mat3 tbn;
	tbn[0] = Tangent;
	tbn[1] = Bitangent;
	tbn[2] = Normal;

	vec3 normalSample = texture(NormalMap, Texcoord).xyz * 2.0f - 1.0f;
	normalSample = tbn * normalSample;

	PositionOut = Position;
	DiffuseOut = vec4(texture(DiffuseMap, Texcoord).xyz, texture(AOMap, Texcoord).x);
	NormalOut = normalize(mat3(invTModelView) * normalSample);
	SpecularOut = texture(SpecularMap, Texcoord);
}
