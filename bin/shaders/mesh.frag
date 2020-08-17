#version 130

in vec3 Normal;
#ifdef MESH_VERTEX_UV1
in vec2 Texcoord;
#endif
#ifdef USE_NORMAL_MAP
in vec3 Tangent;
in vec3 Bitangent;
#endif
#ifdef USE_HEIGHT_MAP
in vec3 FragModelPos;
#endif

uniform vec3 ViewPos;
uniform mat4 ModelView;
uniform vec3 AlbedoBase;
uniform vec3 EmissiveBase;
uniform float RoughnessBase;
uniform float MetalnessBase;

#ifdef USE_ALBEDO_MAP
uniform sampler2D AlbedoMap;
#endif
#ifdef USE_NORMAL_MAP
uniform sampler2D NormalMap;
#endif
#ifdef USE_HEIGHT_MAP
uniform sampler2D HeightMap;
uniform float HeightScale;
#endif
#ifdef USE_ROUGHNESS_MAP
uniform sampler2D RoughnessMap;
#endif
#ifdef USE_METALNESS_MAP
uniform sampler2D MetalnessMap;
#endif
#ifdef USE_EMISSIVE_MAP
uniform sampler2D EmissiveMap;
#endif
#ifdef USE_AO_MAP
uniform sampler2D AOMap;
#endif

out vec4 AlbedoOut;
out vec3 NormalOut;
out vec4 RoughnessOut;
out vec4 MetalnessOut;

struct SurfaceOut
{
	vec3 Albedo;
	vec3 Normal;
	vec3 Emissive;
	float Roughness;
	float Metalness;
	float Occlusion;
};

#ifdef USE_HEIGHT_MAP
vec2 ParallaxMapping(vec2 texCoords, vec3 viewDir)
{
  float height = 1.0 - texture(HeightMap, texCoords).r;
  vec2 p = viewDir.xy / viewDir.z * (height * HeightScale);
  return texCoords - p;
}

// Adapted from https://learnopengl.com/Advanced-Lighting/Parallax-Mapping
vec2 MultisampleParallaxMapping(vec2 texCoords, vec3 viewDir)
{
  const float minLayers = 8.0;
  const float maxLayers = 32.0;
  float numLayers = mix(maxLayers, minLayers, max(dot(vec3(0.0, 0.0, 1.0), viewDir), 0.0));

  float layerDepth = 1.0 / numLayers;
  float currentLayerDepth = 0.0;
  vec2 P = viewDir.xy * HeightScale;
  vec2 deltaTexCoords = P / numLayers;

  vec2  currentTexCoords     = texCoords;
  float currentDepthMapValue = 1.0 - texture(HeightMap, currentTexCoords).r;
  while(currentLayerDepth < currentDepthMapValue)
  {
      currentTexCoords -= deltaTexCoords;
      currentDepthMapValue = 1.0 - texture(HeightMap, currentTexCoords).r;
      currentLayerDepth += layerDepth;
  }

  vec2 prevTexCoords = currentTexCoords + deltaTexCoords;

  float afterDepth  = currentDepthMapValue - currentLayerDepth;
  float beforeDepth = 1.0 - texture(HeightMap, prevTexCoords).r - currentLayerDepth + layerDepth;

  float weight = afterDepth / (afterDepth - beforeDepth);
  vec2 finalTexCoords = prevTexCoords * weight + currentTexCoords * (1.0 - weight);

  return finalTexCoords;
}
#endif

void SurfaceShaderTextured(out SurfaceOut surface)
{
#ifdef MESH_VERTEX_UV1
  vec2 texCoord = Texcoord;
#endif // MESH_VERTEX_UV1

#ifdef USE_HEIGHT_MAP
	mat3 tbn;
	tbn[0] = Tangent;
	tbn[1] = Bitangent;
	tbn[2] = Normal;
  vec3 viewDir = normalize(transpose(tbn) * (ViewPos - FragModelPos)); // tangent space view dir
  texCoord = MultisampleParallaxMapping(texCoord, viewDir);
  if (texCoord.x > 1.0 || texCoord.y > 1.0 || texCoord.x < 0.0 || texCoord.y < 0.0) {
    discard;
  }
#endif // USE_NORMAL_MAP

	vec3 normal;
#ifdef USE_NORMAL_MAP
	vec3 normalSample = texture(NormalMap, texCoord).xyz * 2.0f - 1.0f;
	normal = tbn * normalSample;
#else
	normal = Normal;
#endif // USE_NORMAL_MAP
	surface.Normal = normalize(mat3(ModelView) * normal);

	surface.Albedo = AlbedoBase;
#ifdef USE_ALBEDO_MAP
	surface.Albedo *= texture(AlbedoMap, texCoord).rgb;
#endif // USE_ALBEDO_MAP

	surface.Roughness = RoughnessBase;
#ifdef USE_ROUGHNESS_MAP
	surface.Roughness *= texture(RoughnessMap, texCoord).r;
#endif // USE_ROUGHNESS_MAP

	surface.Metalness = MetalnessBase;
#ifdef USE_METALNESS_MAP
	surface.Metalness *= texture(MetalnessMap, texCoord).r;
#endif // USE_METALNESS_MAP

	surface.Emissive = EmissiveBase;
#ifdef USE_EMISSIVE_MAP
	surface.Emissive *= texture(EmissiveMap, texCoord).rgb;
#endif // USE_ALBEDO_MAP

#ifdef USE_AO_MAP
	surface.Occlusion = texture(AOMap, texCoord).r;
#else
	surface.Occlusion = 1.0f;
#endif//  USE_AO_MAP
}

void main()
{
	SurfaceOut surface;
	SurfaceShaderTextured(surface);
	AlbedoOut = vec4(surface.Albedo, surface.Occlusion);
	NormalOut = surface.Normal;
	RoughnessOut = vec4(surface.Emissive, surface.Roughness);
	MetalnessOut = vec4(surface.Metalness, vec3(0.0f));
}
