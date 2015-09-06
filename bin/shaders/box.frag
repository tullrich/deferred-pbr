#version 130

in vec2 Texcoord;
out vec4 outColor;

void main()
{
	outColor = vec4(Texcoord / 1.5, 0.0, 1.0);
}