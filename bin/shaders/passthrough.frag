#version 130

in vec2 Texcoord;

uniform sampler2D GBuffer_Render;
uniform sampler2D GBuffer_Depth;

out vec4 outColor;

void main()
{
	outColor = vec4(texture(GBuffer_Render, Texcoord).xyz, 1.0f);
#ifdef DEBUG_RENDER_NORMALIZE
	outColor = outColor*0.5f + 0.5f;
#endif
	gl_FragDepth = texture(GBuffer_Depth, Texcoord).x;
}
