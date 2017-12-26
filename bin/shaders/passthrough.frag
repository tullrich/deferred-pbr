#version 130

in vec2 Texcoord;

uniform sampler2D GBuffer_Render;
uniform sampler2D GBuffer_Depth;
uniform float ZNear;
uniform float ZFar;

out vec4 outColor;

void main()
{
	vec3 color = texture(GBuffer_Render, Texcoord).xyz;
#ifdef DEBUG_RENDER_NORMALIZE
	color = color*0.5f + 0.5f;
#endif
#ifdef DEBUG_RENDER_LINEARIZE
	vec3 ndc = color*2.0f - 1.0f;
	color = 2.0 * ZNear / (ZFar + ZNear - ndc * (ZFar - ZNear));
#endif

	outColor = vec4(color, 1.0f);
	gl_FragDepth = texture(GBuffer_Depth, Texcoord).x;
}
