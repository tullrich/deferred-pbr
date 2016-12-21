#version 130

in vec2 Texcoord;
in vec4 Color;

uniform sampler2D Texture;

out vec4 outColor;

void main()
{
	outColor = texture(Texture, Texcoord) * Color;
}
