#version 130

in vec2 Texcoord;

uniform sampler2D GBuffer_Position;
uniform sampler2D GBuffer_Normal;
uniform sampler2D GBuffer_Diffuse;
uniform sampler2D GBuffer_Specular;
uniform sampler2D GBuffer_Depth;

uniform vec3 	AmbientTerm;

uniform vec3 	MainLightPosition;
uniform vec3 	MainLightColor;
uniform float 	MainLightIntensity;

out vec4 outColor;

void main()
{
	vec3 position = texture(GBuffer_Position, Texcoord).xyz;
	vec4 diffuse = texture(GBuffer_Diffuse, Texcoord);
	vec3 normal = texture(GBuffer_Normal, Texcoord).xyz;
	vec4 specular = texture(GBuffer_Specular, Texcoord);
	float depth = texture(GBuffer_Depth, Texcoord).x;

	vec3 lightDir = normalize(MainLightPosition-position);

	// irradiance
	vec3 E_l = MainLightColor * MainLightIntensity;
	vec3 E_theta = E_l * max(dot(lightDir, normal), 0);
	vec3 diffuse_term = diffuse.xyz * E_theta;

	vec3 eyeDir = normalize(-position);
	vec3 vHalfVector = normalize(lightDir + eyeDir);
	float specularTerm =  pow(max(dot(normal,vHalfVector),0.0), 300);

	vec3 exitance =  specularTerm * E_l * specular.xyz + diffuse_term + diffuse.xyz * AmbientTerm * diffuse.w;

	outColor = vec4(exitance, 1.0);
	gl_FragDepth = depth;
}
