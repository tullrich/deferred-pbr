#version 130

in vec3 TexDir;

uniform samplerCube SkyboxCube;

out vec4 outColor;

void main()
{
	outColor = vec4(texture(SkyboxCube, TexDir).xyz, 1);
}
