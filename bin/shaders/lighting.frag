#version 130

#define PI 3.1415926
#define GAMMA 2.2

in vec2 Texcoord;

uniform sampler2D GBuffer_Normal;
uniform sampler2D GBuffer_Albedo;
uniform sampler2D GBuffer_Roughness;
uniform sampler2D GBuffer_Metalness;
uniform sampler2D GBuffer_Depth;
uniform samplerCube EnvIrrMap;
uniform samplerCube EnvPrefilterMap;
uniform sampler2D EnvBrdfLUT;
uniform sampler2D ShadowMap;

uniform vec3 AmbientTerm;
uniform vec4 MainLightPosition;
uniform vec3 MainLightColor;
uniform float MainLightIntensity;
uniform mat4x4 InvView;
uniform mat4x4 InvProjection;
uniform mat4x4 LightSpace;

out vec4 outColor;


struct Material
{
	vec3 Albedo;
	vec3 Emissive;
	float Roughness;
	float Metalness;
	float Occlusion;
};

vec3 Uncharted2ToneMapping(vec3 color)
{
  color *= 2;  // Hardcoded Exposure Adjustment
	float A = 0.15;
	float B = 0.50;
	float C = 0.10;
	float D = 0.20;
	float E = 0.02;
	float F = 0.30;
	float W = 11.2;
	float exposure = 2.;
	color *= exposure;
	color = ((color * (A * color + C * B) + D * E) / (color * (A * color + B) + D * F)) - E / F;
	float white = ((W * (A * W + C * B) + D * E) / (W * (A * W + B) + D * F)) - E / F;
	color /= white;
	color = pow(color, vec3(1. / GAMMA));
	return color;
}

// Schlick-Frensel curve approximation
vec3 FresnelSchlick(vec3 F0, float cosTheta)
{
    return mix(F0, vec3(1.0), pow(1.01 - cosTheta, 5.0));
}

// Schlick-Frensel approximation with added roughness lerp for ambient IBL
// See: https://seblagarde.wordpress.com/2011/08/17/hello-world/
vec3 FresnelSchlickWithRoughness(vec3 F0, float cosTheta, float roughness)
{
	return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(1.0 - cosTheta, 5.0);
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

// NDF
float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a      = roughness*roughness;
    float a2     = a*a;
    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;

    float num   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return num / denom;
}

// G
float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float num   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return num / denom;
}

// F
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2  = GeometrySchlickGGX(NdotV, roughness);
    float ggx1  = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

// Simple phong specular calculation with normalization
vec3 PhongSpecular(vec3 V, vec3 L, vec3 N, vec3 specular, float roughness)
{
    vec3 R = reflect(-L, N);
    float spec = max(0.0, dot(V, R));
    float k = 1.999 / (roughness * roughness);
    return min(1.0, 3.0 * 0.0398 * k) * pow(spec, min(10000.0, k)) * specular;
}

// Full Cook-Torrence BRDF
vec3 CookTorrenceSpecularBRDF(vec3 F, vec3 N, vec3 V, vec3 H, vec3 L, float roughness)
{
  float D = DistributionGGX(N, H, roughness);
  float G = GeometrySmith(N, V, L, roughness);

  vec3 numerator    = D * G * F;
  float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0);
  vec3 specular     = numerator / max(denominator, 0.001);

  return specular;
}

// PBR Direct lighting
vec3 DirectRadiance(vec3 P, vec3 N, vec3 V, Material m, vec3 F0)
{
  // Direction to light in viewspace
  vec3 L = normalize(MainLightPosition.xyz - P * MainLightPosition.w);

  // Half-Vector between light and eye in viewspace
  vec3 H = normalize(L + V);

  // cos(angle) between surface normal and light
  float NdL = max(0.001, dot(N, L));

  // cos(angle) between surface half vector and eye
  float HdV = max(0.001, dot(H, V));

  // Cook Torrence Terms
  vec3 F = FresnelSchlick(F0, HdV);
  vec3 kD =  vec3(1.0) - F;

  // BRDF
  vec3 specBrdf = CookTorrenceSpecularBRDF(F, N, V, H, L, m.Roughness);
	vec3 diffuseBrdf = kD * (m.Albedo / PI) * (1.0 - m.Metalness); // Lambert diffuse

  // Point light attenuation
  float A = mix(1.0f, 1.0 / (1.0 + 0.1 * dot(MainLightPosition.xyz - P, MainLightPosition.xyz - P)), MainLightPosition.w);

  // L
  vec3 radiance = A * MainLightColor * MainLightIntensity;
  return (specBrdf + diffuseBrdf) * radiance * NdL;
}

// PBR IBL from Env map
vec3 IBLAmbientRadiance(vec3 N, vec3 V, Material m, vec3 F0)
{
	vec3 worldN = (InvView * vec4(N, 0)).xyz;	// World normal
	vec3 worldV = (InvView * vec4(V, 0)).xyz;	// World view
	vec3 irradiance = pow(texture(EnvIrrMap, worldN).xyz, vec3(GAMMA));

  // cos(angle) between surface normal and eye
  float NdV = max(0.001, dot(worldN, worldV));
	vec3 kS = FresnelSchlickWithRoughness(F0, NdV, m.Roughness);
	vec3 kD = 1.0 - kS;

	vec3 diffuseBrdf = m.Albedo * (1.0 - m.Metalness); // Lambert diffuse
  vec3 diffuse = kD * diffuseBrdf * irradiance;

  const float MAX_REFLECTION_LOD = 6.0;
  vec3 R = reflect(-worldV, worldN);
  vec2 envBRDF  = texture(EnvBrdfLUT, vec2(NdV, m.Roughness)).rg;
  vec3 prefilteredColor = pow(textureLod(EnvPrefilterMap, R,  m.Roughness * MAX_REFLECTION_LOD).rgb, vec3(GAMMA));
  vec3 specular = prefilteredColor * (kS * envBRDF.x + envBRDF.y);

	return (diffuse + specular) * m.Occlusion; // IBL ambient
}

float ShadowMapVisibility(vec3 P, vec3 N) {
  vec3 PL = (LightSpace * vec4(P, 1.0)).xyz * 0.5 + 0.5;
  float closestDepth = texture(ShadowMap, PL.xy).r;
  vec3 L = normalize(MainLightPosition.xyz - P * MainLightPosition.w);
  float bias = max(0.05 * (1.0 - dot(N, L)), 0.005);
  return ((PL.z - bias ) > closestDepth) ? 1.0 : 0.0;
}

void main()
{
  // Sample G-Buffer
	vec4 albedoAO = texture(GBuffer_Albedo, Texcoord);
	vec4 emissiveRough = texture(GBuffer_Roughness, Texcoord);
	vec3 N = normalize(texture(GBuffer_Normal, Texcoord).xyz);
	float metal = texture(GBuffer_Metalness, Texcoord).r;
	float D = texture(GBuffer_Depth, Texcoord).x;

	// Setup surface material
	Material m;
	m.Albedo = pow(albedoAO.rgb, vec3(GAMMA)); // Gamme to linear
	m.Emissive = pow(emissiveRough.xyz, vec3(GAMMA));
	m.Roughness = emissiveRough.w;
	m.Metalness = metal;
	m.Occlusion = albedoAO.w;

	// Recompute viewspace position from UV + depth
  vec3 P = ViewPositionFromDepth(Texcoord, D);

  // Direction to eye in viewspace
  vec3 V = normalize(-P);

	// Lerp between Dia-electric = 0.04f to Metal = albedo
  vec3 F0 = mix(vec3(0.04), m.Albedo, m.Metalness);

  // Shadow map visibility term
  float Vis = ShadowMapVisibility(P, N);

  // Lighting
  vec3 result = vec3(0.0);
  result += (1.0 - Vis) * DirectRadiance(P, N, V, m, F0);
  result += IBLAmbientRadiance(N, V, m, F0);
  result += m.Emissive * 4.0;

  // Linear back to gamma space via the selected tonemapping operator
#ifdef TONE_MAPPING_REINHARD
  result = result / (result + vec3(1.0));
  result = pow(result, vec3(1.0/GAMMA));
#endif
#ifdef TONE_MAPPING_UNCHARTED2
  result = Uncharted2ToneMapping(result);
#endif

	// Shader output
	outColor = vec4(result, 1.0);
	gl_FragDepth = D;
}
