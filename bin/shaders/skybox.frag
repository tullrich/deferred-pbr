#version 130

in vec3 TexDir;

uniform samplerCube SkyboxCube;

out vec4 outColor;

void main()
{
	//outColor = vec4(texture(SkyboxCube, vec3(2*Texcoord-1, 1)).xyz, 1);
	outColor = vec4(texture(SkyboxCube, TexDir).xyz, 1);
}
