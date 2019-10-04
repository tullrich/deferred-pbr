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
uniform vec4 MainLightPosition;
uniform vec3 MainLightColor;
uniform float MainLightIntensity;
uniform mat4x4 InvView;
uniform mat4x4 InvProjection;

out vec4 outColor;

// Schlick-Frensel curve approximation
vec3 Fresnel(vec3 f0, float cosTheta)
{
    return mix(f0, vec3(1.0), pow(1.01 - cosTheta, 5.0));
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

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float num   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return num / denom;
}
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2  = GeometrySchlickGGX(NdotV, roughness);
    float ggx1  = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

// simple phong specular calculation with normalization
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
  float NDF = DistributionGGX(N, H, roughness);
  float G   = GeometrySmith(N, V, L, roughness);

  vec3 numerator    = NDF * G * F;
  float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0);
  vec3 specular     = numerator / max(denominator, 0.001);

  return specular;
}

void main()
{
  // Surface Properties
	vec4 albedoAO = texture(GBuffer_Albedo, Texcoord);
  vec3 albedo = pow(albedoAO.rgb, vec3(2.2)); // Convert gamme color space to linear
	vec3 N = normalize(texture(GBuffer_Normal, Texcoord).xyz);
	float metalness = texture(GBuffer_Metalness, Texcoord).r;
	float roughness = texture(GBuffer_Roughness, Texcoord).r;
  float occlusion = albedoAO.w;
	float depth = texture(GBuffer_Depth, Texcoord).x;
  vec3 position = ViewPositionFromDepth(Texcoord, depth);
	vec4 worldN = InvView * vec4(N, 0);
 	vec3 envAmbientColor = texture(EnvCubemap, worldN.xyz).xyz;

  // Direction to light in viewspace
  vec3 L = normalize(MainLightPosition.xyz - position * MainLightPosition.w);

  // Direction to eye in viewspace
  vec3 V = normalize(-position);

  // Half-Vector between light and eye in viewspace
  vec3 H = normalize(L + V);

  // cos(angle) between surface normal and light
  float NdL = max(0.001, dot(N, L));

  // cos(angle) between surface half vector and eye
  float HdV = max(0.001, dot(H, V));

  vec3 radiance = MainLightColor * MainLightIntensity;
  vec3 F0 = mix(vec3(0.04f), albedo, metalness);

  // Cook Torrence Terms
  vec3 F = Fresnel(F0, HdV);
  vec3 kD =  vec3(1.0) - F;
  vec3 specBrdf = CookTorrenceSpecularBRDF(F, N, V, H, L, roughness);

  vec3 result = vec3(0.0);
  result += specBrdf * radiance * NdL; // cook-torrence specular
  result += kD * (albedo / PI * (1.0 - metalness)) * radiance * NdL;  // lambert diffuse
  result += albedo * AmbientTerm * occlusion; // IBL ambient

  // Linear back to gamma space via Reinhard tonemapping
  result = result / (result + vec3(1.0));
  result = pow(result, vec3(1.0/2.2));

	outColor = vec4(result, 1.0);
	gl_FragDepth = depth;
}
