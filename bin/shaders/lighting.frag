#version 130

#define PI 3.1415926

in vec2 Texcoord;

uniform sampler2D GBuffer_Normal;
uniform sampler2D GBuffer_Albedo;
uniform sampler2D GBuffer_Roughness;
uniform sampler2D GBuffer_Metalness;
uniform sampler2D GBuffer_Depth;
uniform samplerCube EnvDiffuse;

uniform vec3 AmbientTerm;
uniform vec4 MainLightPosition;
uniform vec3 MainLightColor;
uniform float MainLightIntensity;
uniform mat4x4 InvView;
uniform mat4x4 InvProjection;

out vec4 outColor;

struct Material
{
	vec3 Albedo;
	vec3 Emissive;
	float Roughness;
	float Metalness;
	float Occlusion;
};

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
  float G   = GeometrySmith(N, V, L, roughness);

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

  // L
  vec3 radiance = MainLightColor * MainLightIntensity;
  return (specBrdf + diffuseBrdf) * radiance * NdL;
}

// PBR IBL from Env map
vec3 IBLAmbientRadiance(vec3 N, vec3 V, Material m, vec3 F0)
{
	vec4 worldN = InvView * vec4(N, 0);	// World normal
	vec3 irradiance = texture(EnvDiffuse, worldN.xyz).xyz;

  // cos(angle) between surface normal and eye
  float NdV = max(0.001, dot(N, V));

	vec3 kS = FresnelSchlickWithRoughness(F0, NdV, m.Roughness);
	vec3 kD = 1.0 - kS;

	vec3 diffuseBrdf = kD * m.Albedo * (1.0 - m.Metalness); // Lambert diffuse
	//TODO: IBL Specular

	return diffuseBrdf * irradiance * m.Occlusion; // IBL ambient
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
	m.Albedo = pow(albedoAO.rgb, vec3(2.2)); // Gamme to linear
	m.Emissive = pow(emissiveRough.xyz, vec3(2.2));
	m.Roughness = emissiveRough.w;
	m.Metalness = metal;
	m.Occlusion = albedoAO.w;

	// Recompute viewspace position from UV + depth
  vec3 P = ViewPositionFromDepth(Texcoord, D);

  // Direction to eye in viewspace
  vec3 V = normalize(-P);

	// Lerp between Dia-electric = 0.04f to Metal = albedo
  vec3 F0 = mix(vec3(0.04f), m.Albedo, m.Metalness);

  // Lighting
  vec3 result = vec3(0.0);
  result += DirectRadiance(P, N, V, m, F0);
  result += IBLAmbientRadiance(N, V, m, F0);
  result += m.Emissive * 4.0;

  // Linear back to gamma space via Reinhard tonemapping
  result = result / (result + vec3(1.0));
  result = pow(result, vec3(1.0/2.2));

	// Shader output
	outColor = vec4(result, 1.0);
	gl_FragDepth = D;
}
