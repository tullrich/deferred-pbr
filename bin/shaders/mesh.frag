#version 130

in vec3 Normal;
#ifdef MESH_VERTEX_UV1
in vec2 Texcoord;
#endif
#ifdef USE_NORMAL_MAP
in vec3 Tangent;
in vec3 Bitangent;
#endif

uniform mat4 invTModelView;
uniform vec3 AlbedoBase;
uniform vec3 RoughnessBase;
uniform vec3 MetalnessBase;

#ifdef USE_ALBEDO_MAP
uniform sampler2D AlbedoMap;
#endif
#ifdef USE_NORMAL_MAP
uniform sampler2D NormalMap;
#endif
#ifdef USE_ROUGHNESS_MAP
uniform sampler2D RoughnessMap;
#endif
#ifdef USE_METALNESS_MAP
uniform sampler2D MetalnessMap;
#endif
#ifdef USE_AO_MAP
uniform sampler2D AOMap;
#endif

out vec4 AlbedoOut;
out vec3 NormalOut;
out vec3 RoughnessOut;
out vec3 MetalnessOut;

struct SurfaceOut
{
	vec3 Albedo;
	vec3 Normal;
	vec3 Roughness;
	vec3 Metalness;
	float Occlusion;
};

void SurfaceShaderTextured(out SurfaceOut surface)
{
	vec3 normal;
#ifdef USE_NORMAL_MAP
	mat3 tbn;
	tbn[0] = Tangent;
	tbn[1] = Bitangent;
	tbn[2] = Normal;
	vec3 normalSample = texture(NormalMap, Texcoord).xyz * 2.0f - 1.0f;
	normal = tbn * normalSample;
#else
	normal = Normal;
#endif // USE_NORMAL_MAP
	surface.Normal = normalize(mat3(invTModelView) * normal);

	surface.Albedo = AlbedoBase;
#ifdef USE_ALBEDO_MAP
	surface.Albedo *= texture(AlbedoMap, Texcoord).xyz;
#endif // USE_ALBEDO_MAP

	surface.Roughness = RoughnessBase;
#ifdef USE_ROUGHNESS_MAP
	surface.Roughness *= texture(RoughnessMap, Texcoord).xyz;
#endif // USE_ROUGHNESS_MAP

	surface.Metalness = MetalnessBase;
#ifdef USE_METALNESS_MAP
	surface.Metalness *= texture(MetalnessMap, Texcoord).xyz;
#endif // USE_METALNESS_MAP

#ifdef USE_AO_MAP
	surface.Occlusion = texture(AOMap, Texcoord).x;
#else
	surface.Occlusion = 0.0f;
#endif//  USE_AO_MAP
}

void main()
{
	SurfaceOut surface;
	SurfaceShaderTextured(surface);

	AlbedoOut = vec4(surface.Albedo, surface.Occlusion);
	NormalOut = surface.Normal;
	RoughnessOut = surface.Roughness;
	MetalnessOut = surface.Metalness;
}
