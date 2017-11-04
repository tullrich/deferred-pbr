#version 130

in vec2 Texcoord;

uniform sampler2D GBuffer_Render;
uniform sampler2D GBuffer_Depth;

out vec4 outColor;

void main()
{
	outColor = vec4(texture(GBuffer_Render, Texcoord).xyz, 1.0f);
	gl_FragDepth = texture(GBuffer_Depth, Texcoord).x;
}
