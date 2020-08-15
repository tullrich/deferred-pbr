#version 130

in vec3 TexDir;

uniform samplerCube SkyboxCube;
uniform float Lod;

out vec4 outColor;

void main()
{
	outColor = vec4(textureLod(SkyboxCube, TexDir, Lod).xyz, 1);
}
