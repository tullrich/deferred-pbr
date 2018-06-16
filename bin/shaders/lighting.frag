#version 130

#define PI 3.1415926

in vec2 Texcoord;

uniform sampler2D GBuffer_Normal;
uniform sampler2D GBuffer_Albedo;
uniform sampler2D GBuffer_Roughness;
uniform sampler2D GBuffer_Metalness;
uniform sampler2D GBuffer_Depth;
uniform samplerCube EnvCubemap;

uniform vec3 AmbientTerm;
uniform vec3 MainLightPosition;
uniform vec3 MainLightColor;
uniform float MainLightIntensity;
uniform mat4x4 InvView;
uniform mat4x4 InvProjection;

out vec4 outColor;

// Schlick-Frensel curve approximation
vec3 Fresnel(vec3 f0, float product)
{
    return mix(f0, vec3(1.0), pow(1.01 - product, 5.0));
}

// Reconstruct view space position from depth
vec3 ViewPositionFromDepth(vec2 texcoord, float depth)
{
  // Get x/w and y/w from the viewport position
  vec3 projectedPos = vec3(texcoord, depth) * 2.0f - 1.0f;;

  // Transform by the inverse projection matrix
  vec4 positionVS = InvProjection * vec4(projectedPos, 1.0f);

  // Divide by w to get the view-space position
  return positionVS.xyz / positionVS.w;
}

// simple phong specular calculation with normalization
vec3 PhongSpecular(vec3 V, vec3 L, vec3 N, vec3 specular, float roughness)
{
    vec3 R = reflect(-L, N);
    float spec = max(0.0, dot(V, R));
    float k = 1.999 / (roughness * roughness);
    return min(1.0, 3.0 * 0.0398 * k) * pow(spec, min(10000.0, k)) * specular;
}

void main()
{
  // Surface Properties
	vec4 albedo = texture(GBuffer_Albedo, Texcoord);
	vec3 normal = normalize(texture(GBuffer_Normal, Texcoord).xyz);
	float metalness = texture(GBuffer_Metalness, Texcoord).r;
	float roughness = texture(GBuffer_Roughness, Texcoord).r;
  float occlusion = albedo.w;
	float depth = texture(GBuffer_Depth, Texcoord).x;
  vec3 position = ViewPositionFromDepth(Texcoord, depth);
	vec4 worldNormal = InvView * vec4(normal, 0);

  // Direction to light in viewspace
  vec3 L = normalize(MainLightPosition - position);

  // Direction to eye in viewspace
  vec3 V = normalize(-position);

  // Half-Vector between light and eye in viewspace
  vec3 H = normalize(L + V);

  // cos(angle) between surface normal and light
  float NdL = max(0.001, dot(normal, L));

  // cos(angle) between surface normal and eye
  float NdV = max(0.001, dot(normal, V));

  vec3 lightColor = MainLightColor * MainLightIntensity;
  vec3 diffuseColor = mix(albedo.xyz, vec3(0.0f), metalness);
  vec3 specularColor = mix(vec3(0.04f), albedo.xyz, metalness);
 	vec3 envAmbientColor = texture(EnvCubemap, worldNormal.xyz).xyz;

  // specular reflectance with PHONG
  vec3 specfresnel = Fresnel(specularColor, NdV);
  vec3 specref = PhongSpecular(V, L, normal, specfresnel, roughness) * NdL;

  // lambertian diffuse term
  vec3 diffuse_light = vec3(0);
  diffuse_light += diffuseColor * (vec3(1.0) - specfresnel) * lightColor * NdL;
  diffuse_light += diffuseColor * envAmbientColor;

  // specular reflectance with phong
  vec3 reflected_light = vec3(0);
  reflected_light += specref * lightColor;

  // compute total lighting
  vec3 result = diffuse_light + reflected_light;
	outColor = vec4(result, 1.0);
	gl_FragDepth = depth;
}
